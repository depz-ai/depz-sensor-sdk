/**
 * WebSerial transport (Chrome/Edge, secure context). The `SerialPort` must be
 * obtained by the app inside a user gesture (`navigator.serial.requestPort()`)
 * and passed here — the SDK never triggers the permission picker itself.
 */

import {
  isDeviceInfo,
  listDepzDevicesFrom,
  openDeviceByDeviceSerial,
  openDeviceFrom,
  orderDevicesBySerial,
  type DeviceInfo,
  type DepzPortInfo,
  type ListOptions,
  type OpenDeviceOptions,
  type OpenTarget,
} from "../discovery.js";
import type { DepzDevice } from "../device/device.js";
import type { SerialTransport, SerialTransportInfo } from "./types.js";

// Minimal Web Serial API surface (not yet in default TS DOM lib everywhere).
export interface WebSerialPortLike {
  open(options: { baudRate: number }): Promise<void>;
  close(): Promise<void>;
  getInfo(): { usbVendorId?: number; usbProductId?: number };
  readable: ReadableStream<Uint8Array> | null;
  writable: WritableStream<Uint8Array> | null;
  addEventListener?(type: "disconnect", cb: () => void): void;
  removeEventListener?(type: "disconnect", cb: () => void): void;
}

interface NavigatorSerialLike {
  getPorts(): Promise<WebSerialPortLike[]>;
  addEventListener(type: "connect" | "disconnect", cb: (ev: unknown) => void): void;
  removeEventListener(type: "connect" | "disconnect", cb: (ev: unknown) => void): void;
}

function navigatorSerial(): NavigatorSerialLike | null {
  const nav = globalThis.navigator as unknown as { serial?: NavigatorSerialLike } | undefined;
  return nav?.serial ?? null;
}

/** Feature-detect Web Serial support in the current environment. */
export function isWebSerialSupported(): boolean {
  return navigatorSerial() !== null;
}

/** Previously-granted ports (survive page reloads per origin+device). */
export async function getGrantedPorts(): Promise<WebSerialPortLike[]> {
  const serial = navigatorSerial();
  return serial ? serial.getPorts() : [];
}

/**
 * Watch physical connect events; used by the firmware-update flow to
 * re-acquire a device after it reboots. Returns an unsubscribe function.
 */
export function watchConnect(cb: (port: WebSerialPortLike) => void): () => void {
  const serial = navigatorSerial();
  if (!serial) return () => {};
  const handler = (ev: unknown) => {
    const port = (ev as { target?: WebSerialPortLike }).target;
    if (port) cb(port);
  };
  serial.addEventListener("connect", handler);
  return () => serial.removeEventListener("connect", handler);
}

export class WebSerialTransport implements SerialTransport {
  readonly info: SerialTransportInfo;
  private reader: ReadableStreamDefaultReader<Uint8Array> | null = null;
  private writer: WritableStreamDefaultWriter<Uint8Array> | null = null;
  private disconnectCbs: Array<() => void> = [];
  private disconnectHandler: (() => void) | null = null;

  constructor(private port: WebSerialPortLike) {
    const info = port.getInfo();
    this.info = { usbVendorId: info.usbVendorId, usbProductId: info.usbProductId };
  }

  async open(opts?: { baudRate?: number }): Promise<void> {
    await this.port.open({ baudRate: opts?.baudRate ?? 115200 });
    if (this.disconnectHandler === null && this.port.addEventListener) {
      this.disconnectHandler = () => this.fireDisconnect();
      this.port.addEventListener("disconnect", this.disconnectHandler);
    }
  }

  async write(data: Uint8Array): Promise<void> {
    if (!this.port.writable) throw new Error("port not writable (closed?)");
    this.writer ??= this.port.writable.getWriter();
    await this.writer.write(data);
  }

  async *readable(): AsyncIterableIterator<Uint8Array> {
    if (!this.port.readable) throw new Error("port not readable (closed?)");
    this.reader = this.port.readable.getReader();
    try {
      for (;;) {
        const { value, done } = await this.reader.read();
        if (done) return;
        if (value && value.length > 0) yield value;
      }
    } catch {
      // Read failure = cable yank / device reboot.
      this.fireDisconnect();
      return;
    } finally {
      this.reader.releaseLock();
      this.reader = null;
    }
  }

  async close(): Promise<void> {
    try {
      await this.reader?.cancel();
    } catch {
      /* already gone */
    }
    if (this.writer) {
      try {
        this.writer.releaseLock();
      } catch {
        /* already gone */
      }
      this.writer = null;
    }
    if (this.disconnectHandler !== null) {
      this.port.removeEventListener?.("disconnect", this.disconnectHandler);
      this.disconnectHandler = null;
    }
    await this.port.close();
  }

  onDisconnect(cb: () => void): () => void {
    this.disconnectCbs.push(cb);
    return () => {
      this.disconnectCbs = this.disconnectCbs.filter((c) => c !== cb);
    };
  }

  private fireDisconnect(): void {
    const cbs = this.disconnectCbs;
    this.disconnectCbs = [];
    cbs.forEach((cb) => cb());
  }
}

// ── discovery (WebSerial) ─────────────────────────────────────────────────────
//
// WebSerial CANNOT enumerate the system's ports or read a port's USB iSerial:
// `navigator.serial.getPorts()` returns only ports the user has already
// granted (per origin+device), and `port.getInfo()` exposes just vid/pid.
//
// BUT the device serial is still reachable — over the PROTOCOL: probing a port
// (open → GET_SERIAL → close) yields `DeviceInfo.serialNumber`. So selection
// and ordering by serial ARE supported in the browser, at the cost of opening
// each granted port once. Selection is by DEVICE serial (not granted-port
// order), which matches Node — for programmed units USB iSerial == device
// serial. Explicit targets (a `WebSerialPortLike`, a granted-port path, or a
// `DeviceInfo`) open that exact port without a selection scan.
//
// The app must first obtain ports in a user gesture via
// `navigator.serial.requestPort()`; the SDK never opens the picker itself.

function grantedPortInfos(ports: WebSerialPortLike[]): {
  infos: DepzPortInfo[];
  byPath: Map<string, WebSerialPortLike>;
} {
  const infos: DepzPortInfo[] = [];
  const byPath = new Map<string, WebSerialPortLike>();
  ports.forEach((port, i) => {
    const path = `webserial:${i}`;
    const usb = port.getInfo();
    infos.push({ path, usbVid: usb.usbVendorId, usbPid: usb.usbProductId });
    byPath.set(path, port);
  });
  return { infos, byPath };
}

/**
 * Probe the already-granted WebSerial ports and return the DEPZ devices among
 * them, ordered by DEVICE serial (GET_SERIAL) — the USB iSerial is unavailable
 * in the browser, so the serial is read over the protocol. By default every
 * granted port is probed (`matchUsb: false`) since a granted port is
 * user-curated; pass `matchUsb: true` to probe only known-DEPZ vid/pids.
 */
export async function listDepzDevices(opts: ListOptions = {}): Promise<DeviceInfo[]> {
  const { infos, byPath } = grantedPortInfos(await getGrantedPorts());
  const factory = (p: DepzPortInfo): SerialTransport => new WebSerialTransport(byPath.get(p.path)!);
  const found = await listDepzDevicesFrom(infos, factory, {
    matchUsb: opts.matchUsb ?? false,
    timeoutMs: opts.timeoutMs,
  });
  return orderDevicesBySerial(found);
}

/**
 * Open the right sensor class for a granted WebSerial port.
 *
 * `target` is a `WebSerialPortLike` (typically fresh from
 * `navigator.serial.requestPort()`), a granted-port path string / `DeviceInfo`
 * from `listDepzDevices()`, a candidate index (number, ordered by device
 * serial), or omitted for the smallest-serial DEPZ candidate. `opts.serial`
 * selects the candidate whose device serial matches exactly. Selection probes
 * the granted ports; explicit targets open directly. Throws
 * `NoDepzDeviceError` when nothing matches.
 */
export async function openDevice(
  target?: OpenTarget | WebSerialPortLike,
  opts: OpenDeviceOptions = {},
): Promise<DepzDevice> {
  // A raw WebSerial port handed in directly (the usual requestPort() flow).
  if (isWebSerialPort(target)) {
    const port = target;
    const usb = port.getInfo();
    const info: DepzPortInfo = { path: "webserial:direct", usbVid: usb.usbVendorId, usbPid: usb.usbProductId };
    const factory = (): SerialTransport => new WebSerialTransport(port);
    return openDeviceFrom([info], factory, "webserial:direct", opts);
  }
  const { infos, byPath } = grantedPortInfos(await getGrantedPorts());
  const factory = (p: DepzPortInfo): SerialTransport => new WebSerialTransport(byPath.get(p.path)!);
  // Explicit granted port (path string or DeviceInfo): open exactly it.
  if (typeof target === "string" || isDeviceInfo(target)) {
    return openDeviceFrom(infos, factory, target, opts);
  }
  // Default / index / serial: select by DEVICE serial (probes granted ports).
  return openDeviceByDeviceSerial(infos, factory, target ?? null, opts);
}

function isWebSerialPort(t: unknown): t is WebSerialPortLike {
  return (
    typeof t === "object" &&
    t !== null &&
    typeof (t as WebSerialPortLike).getInfo === "function" &&
    typeof (t as WebSerialPortLike).open === "function"
  );
}
