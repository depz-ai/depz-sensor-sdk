/**
 * A DEPZ board over WebUSB, shaped as a Web Serial port.
 *
 * Chrome on Android 12–16 has no Web Serial for USB boards (Chrome 148 ships it
 * on Android 17 only, for the ports the device exposes), but it has WebUSB, and
 * Android has no kernel CDC-ACM driver holding the board — so a page can claim
 * the board's CDC interfaces itself, as the Android SDK's `AndroidUsbLink` does
 * through `UsbManager`. `WebUsbSerialPort` does exactly that and offers the
 * `WebSerialPortLike` surface, so `WebSerialTransport`, discovery and every
 * device class run over it unchanged.
 *
 * Open mirrors `AndroidUsbLink.open`: claim the CDC control and data
 * interfaces, SET_LINE_CODING (baud, 1 stop bit, no parity, 8 bits), drop DTR
 * and RTS for 20 ms (a board disarms its streams when DTR drops; a page that
 * died left them up), then raise both. Close drops them and releases the
 * interfaces. Reads are one packet per transfer with several transfers in
 * flight, as the Linux cdc_acm driver reads: a larger transfer would wait for a
 * short packet the board may never send after a reply of a whole number of
 * packets.
 *
 * On a desktop OS the kernel's CDC driver holds the board, so claiming fails
 * there: this path is for Android (and other hosts without that driver).
 */

import { DEPZ_VID, DEV_PID, DEV_VID, isKnownDepzUsb } from "../protocol/usb-ids.js";
import type { WebSerialPortLike } from "./webserial.js";

// ── the WebUSB surface used here (not in the default TS DOM lib) ─────────────

export interface UsbEndpointLike {
  readonly endpointNumber: number;
  readonly direction: "in" | "out";
  readonly type: "bulk" | "interrupt" | "isochronous";
  readonly packetSize: number;
}

export interface UsbInterfaceLike {
  readonly interfaceNumber: number;
  readonly alternate: {
    readonly interfaceClass: number;
    readonly endpoints: readonly UsbEndpointLike[];
  };
}

export interface UsbControlSetup {
  requestType: "standard" | "class" | "vendor";
  recipient: "device" | "interface" | "endpoint" | "other";
  request: number;
  value: number;
  index: number;
}

export interface UsbDeviceLike {
  readonly vendorId: number;
  readonly productId: number;
  readonly serialNumber?: string;
  readonly productName?: string;
  readonly opened: boolean;
  readonly configuration: { readonly interfaces: readonly UsbInterfaceLike[] } | null;
  open(): Promise<void>;
  close(): Promise<void>;
  selectConfiguration(value: number): Promise<void>;
  claimInterface(n: number): Promise<void>;
  releaseInterface(n: number): Promise<void>;
  controlTransferOut(setup: UsbControlSetup, data?: Uint8Array): Promise<{ status: string }>;
  transferIn(endpoint: number, length: number): Promise<{ status: string; data?: DataView }>;
  transferOut(
    endpoint: number,
    data: Uint8Array,
  ): Promise<{ status: string; bytesWritten: number }>;
  clearHalt(direction: "in" | "out", endpoint: number): Promise<void>;
  /** Withdraws the page's permission for the device (Chrome 101+; absent elsewhere). */
  forget?(): Promise<void>;
}

export interface UsbDeviceFilter {
  vendorId?: number;
  productId?: number;
}

interface NavigatorUsbLike {
  requestDevice(opts: { filters: UsbDeviceFilter[] }): Promise<UsbDeviceLike>;
  getDevices(): Promise<UsbDeviceLike[]>;
  addEventListener(type: "connect" | "disconnect", cb: (ev: unknown) => void): void;
  removeEventListener(type: "connect" | "disconnect", cb: (ev: unknown) => void): void;
}

function navigatorUsb(): NavigatorUsbLike | null {
  const nav = globalThis.navigator as unknown as { usb?: NavigatorUsbLike } | undefined;
  return nav?.usb ?? null;
}

/** Feature-detect WebUSB in the current environment. */
export function isWebUsbSupported(): boolean {
  return navigatorUsb() !== null;
}

/** The chooser filter: the production VID (app and bootloader) and unprogrammed dev units. */
export const DEPZ_WEBUSB_FILTERS: readonly UsbDeviceFilter[] = Object.freeze([
  Object.freeze({ vendorId: DEPZ_VID }),
  Object.freeze({ vendorId: DEV_VID, productId: DEV_PID }),
]);

// ── CDC-ACM ──────────────────────────────────────────────────────────────────

const USB_CLASS_COMM = 0x02;
const USB_CLASS_CDC_DATA = 0x0a;
const SET_LINE_CODING = 0x20;
const SET_CONTROL_LINE_STATE = 0x22;
const DTR_AND_RTS = 0x03;
const DTR_LOW_MS = 20;
/** Transfers kept in flight on the IN endpoint (the Linux cdc_acm driver keeps 16). */
export const WEBUSB_READS_IN_FLIGHT = 16;
/** A write that has not completed by then fails (contract 07 §3: `write` settles in bounded time). */
export const WEBUSB_WRITE_TIMEOUT_MS = 1000;

/** SET_LINE_CODING payload: baud (u32 LE), 1 stop bit, no parity, 8 data bits. */
export function lineCoding(baudRate: number): Uint8Array {
  const b = new Uint8Array(7);
  new DataView(b.buffer).setUint32(0, baudRate, true);
  b[6] = 8;
  return b;
}

const sleep = async (ms: number): Promise<void> => new Promise((r) => setTimeout(r, ms));

/** The same USB device always maps to the same port object (the viewer dedups by identity). */
const ports = new WeakMap<UsbDeviceLike, WebUsbSerialPort>();

/** The `WebUsbSerialPort` of `device`, created on first use. */
export function webUsbPort(device: UsbDeviceLike): WebUsbSerialPort {
  let port = ports.get(device);
  if (!port) {
    port = new WebUsbSerialPort(device);
    ports.set(device, port);
  }
  return port;
}

interface Claimed {
  control: number;
  data: number;
  inEp: number;
  outEp: number;
  packet: number;
}

export class WebUsbSerialPort implements WebSerialPortLike {
  readable: ReadableStream<Uint8Array> | null = null;
  writable: WritableStream<Uint8Array> | null = null;
  private claimed: Claimed | null = null;
  private listeners = new Set<() => void>();
  private usbListener: ((ev: unknown) => void) | null = null;
  /**
   * Closes the port and withdraws the page's permission for the board
   * (`forgetPort`). Present only where the browser's device has `forget()`.
   */
  readonly forget?: () => Promise<void>;

  constructor(readonly device: UsbDeviceLike) {
    if (typeof device.forget === "function") {
      this.forget = async () => {
        await this.close();
        await device.forget!();
      };
    }
  }

  getInfo(): { usbVendorId?: number; usbProductId?: number } {
    return { usbVendorId: this.device.vendorId, usbProductId: this.device.productId };
  }

  /** The USB iSerial — WebUSB exposes it, Web Serial does not. */
  get serialNumber(): string | undefined {
    return this.device.serialNumber || undefined;
  }

  async open(options: { baudRate: number; bufferSize?: number }): Promise<void> {
    if (this.claimed) throw new Error("port already open");
    const d = this.device;
    if (!d.opened) await d.open();
    try {
      if (d.configuration === null) await d.selectConfiguration(1);
      const c = findCdc(d);
      await d.claimInterface(c.control);
      await d.claimInterface(c.data);
      const setup = (request: number, value: number): UsbControlSetup => ({
        requestType: "class",
        recipient: "interface",
        request,
        value,
        index: c.control,
      });
      await d.controlTransferOut(setup(SET_LINE_CODING, 0), lineCoding(options.baudRate));
      await d.controlTransferOut(setup(SET_CONTROL_LINE_STATE, 0));
      await sleep(DTR_LOW_MS);
      await d.controlTransferOut(setup(SET_CONTROL_LINE_STATE, DTR_AND_RTS));
      this.claimed = c;
    } catch (e) {
      await d.close().catch(() => {});
      throw e;
    }
    this.readable = this.makeReadable(this.claimed);
    this.writable = this.makeWritable(this.claimed);
    this.watchUsb();
  }

  async close(): Promise<void> {
    const c = this.claimed;
    if (!c) return;
    this.claimed = null;
    this.readable = null;
    this.writable = null;
    this.unwatchUsb();
    const d = this.device;
    // Best effort: a board already gone fails each of these.
    await d
      .controlTransferOut({
        requestType: "class",
        recipient: "interface",
        request: SET_CONTROL_LINE_STATE,
        value: 0,
        index: c.control,
      })
      .catch(() => {});
    await d.releaseInterface(c.data).catch(() => {});
    await d.releaseInterface(c.control).catch(() => {});
    await d.close().catch(() => {});
  }

  addEventListener(_type: "disconnect", cb: () => void): void {
    this.listeners.add(cb);
  }

  removeEventListener(_type: "disconnect", cb: () => void): void {
    this.listeners.delete(cb);
  }

  private watchUsb(): void {
    const usb = navigatorUsb();
    if (!usb || this.usbListener) return;
    this.usbListener = (ev: unknown) => {
      if ((ev as { device?: unknown }).device !== this.device) return;
      for (const cb of [...this.listeners]) cb();
    };
    usb.addEventListener("disconnect", this.usbListener);
  }

  private unwatchUsb(): void {
    if (this.usbListener) navigatorUsb()?.removeEventListener("disconnect", this.usbListener);
    this.usbListener = null;
  }

  private makeReadable(c: Claimed): ReadableStream<Uint8Array> {
    const d = this.device;
    const inFlight: Array<Promise<{ status: string; data?: DataView }>> = [];
    const submit = (): void => {
      const p = d.transferIn(c.inEp, c.packet);
      // Transfers left in flight when the stream ends reject on close: handled here,
      // still rejecting for whoever awaits them.
      p.catch(() => {});
      inFlight.push(p);
    };
    return new ReadableStream<Uint8Array>(
      {
        pull: async (ctl) => {
          for (;;) {
            if (this.claimed !== c) {
              ctl.close();
              return;
            }
            while (inFlight.length < WEBUSB_READS_IN_FLIGHT) submit();
            const r = await inFlight.shift()!;
            if (r.status === "stall") {
              // Clear the halt and go on with the transfers still queued: waiting
              // for them to settle could wait for data that never comes, and a
              // transfer that the halt failed too comes back as a stall itself.
              await d.clearHalt("in", c.inEp);
              continue;
            }
            if (r.status !== "ok") throw new Error(`USB read: ${r.status}`);
            const v = r.data;
            if (v && v.byteLength > 0) {
              ctl.enqueue(
                new Uint8Array(v.buffer.slice(v.byteOffset, v.byteOffset + v.byteLength)),
              );
              return;
            }
          }
        },
      },
      { highWaterMark: 0 },
    );
  }

  private makeWritable(c: Claimed): WritableStream<Uint8Array> {
    const d = this.device;
    return new WritableStream<Uint8Array>({
      write: async (chunk) => {
        let timer: ReturnType<typeof setTimeout> | undefined;
        const timeout = new Promise<never>((_, reject) => {
          timer = setTimeout(
            () => reject(new Error(`USB write timed out after ${WEBUSB_WRITE_TIMEOUT_MS} ms`)),
            WEBUSB_WRITE_TIMEOUT_MS,
          );
        });
        try {
          const r = await Promise.race([d.transferOut(c.outEp, chunk), timeout]);
          if (r.status === "stall") {
            await d.clearHalt("out", c.outEp);
            throw new Error("USB write stalled");
          }
          if (r.status !== "ok" || r.bytesWritten !== chunk.length) {
            throw new Error(`USB write: ${r.status}, ${r.bytesWritten} of ${chunk.length} bytes`);
          }
        } finally {
          clearTimeout(timer);
        }
      },
    });
  }
}

function findCdc(d: UsbDeviceLike): Claimed {
  const ifaces = d.configuration?.interfaces ?? [];
  const control = ifaces.find((i) => i.alternate.interfaceClass === USB_CLASS_COMM);
  const data = ifaces.find((i) => i.alternate.interfaceClass === USB_CLASS_CDC_DATA);
  if (!control) throw new Error("no CDC control interface");
  if (!data) throw new Error("no CDC data interface");
  const bulk = data.alternate.endpoints.filter((e) => e.type === "bulk");
  const inEp = bulk.find((e) => e.direction === "in");
  const outEp = bulk.find((e) => e.direction === "out");
  if (!inEp) throw new Error("no bulk IN endpoint");
  if (!outEp) throw new Error("no bulk OUT endpoint");
  return {
    control: control.interfaceNumber,
    data: data.interfaceNumber,
    inEp: inEp.endpointNumber,
    outEp: outEp.endpointNumber,
    packet: inEp.packetSize,
  };
}

/**
 * Ask the user to pick a board (must run inside a user gesture). Resolves to
 * its port; rejects when the chooser is cancelled (`NotFoundError`).
 */
export async function requestWebUsbPort(
  filters: readonly UsbDeviceFilter[] = DEPZ_WEBUSB_FILTERS,
): Promise<WebUsbSerialPort> {
  const usb = navigatorUsb();
  if (!usb) throw new Error("WebUSB is not available in this browser");
  return webUsbPort(await usb.requestDevice({ filters: [...filters] }));
}

/** Boards this origin was already granted (survive reloads), DEPZ ones only. */
export async function getGrantedWebUsbPorts(): Promise<WebUsbSerialPort[]> {
  const usb = navigatorUsb();
  if (!usb) return [];
  const devices = await usb.getDevices();
  return devices.filter((d) => isKnownDepzUsb(d.vendorId, d.productId)).map(webUsbPort);
}

/**
 * Watch a granted DEPZ board being plugged in (`navigator.usb`'s `connect`
 * event, which fires for devices the page already has permission for). The
 * callback gets the board's port; returns an unsubscribe function.
 */
export function watchWebUsbConnect(cb: (port: WebUsbSerialPort) => void): () => void {
  const usb = navigatorUsb();
  if (!usb) return () => {};
  const handler = (ev: unknown): void => {
    const device = (ev as { device?: UsbDeviceLike }).device;
    if (device && isKnownDepzUsb(device.vendorId, device.productId)) cb(webUsbPort(device));
  };
  usb.addEventListener("connect", handler);
  return () => usb.removeEventListener("connect", handler);
}
