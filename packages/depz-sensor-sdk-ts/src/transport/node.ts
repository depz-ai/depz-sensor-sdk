/**
 * Node.js transport over the optional `serialport` peer dependency.
 * Import via `@depz/sensor-sdk/node` only — never from the root export, so
 * browser bundles stay serialport-free.
 */

import {
  listDepzDevicesFrom,
  openDeviceFrom,
  type DeviceInfo,
  type DepzPortInfo,
  type ListOptions,
  type OpenDeviceOptions,
  type OpenTarget,
} from "../discovery.js";
import type { DepzDevice } from "../device/device.js";
import type { SerialTransport, SerialTransportInfo } from "./types.js";

interface SerialPortLike {
  on(event: "data", cb: (data: Uint8Array) => void): void;
  on(event: "close" | "error", cb: (err?: unknown) => void): void;
  off(event: string, cb: (...args: never[]) => void): void;
  write(data: Uint8Array, cb: (err?: Error | null) => void): void;
  drain(cb: (err?: Error | null) => void): void;
  close(cb: (err?: Error | null) => void): void;
  open(cb: (err?: Error | null) => void): void;
  readonly isOpen: boolean;
}

async function loadSerialport(): Promise<{
  SerialPort: new (opts: { path: string; baudRate: number; autoOpen: boolean }) => SerialPortLike;
  list: () => Promise<Array<{ path: string; serialNumber?: string; vendorId?: string; productId?: string }>>;
}> {
  try {
    const mod = (await import("serialport")) as unknown as {
      SerialPort: {
        new (opts: { path: string; baudRate: number; autoOpen: boolean }): SerialPortLike;
        list(): Promise<Array<{ path: string; serialNumber?: string; vendorId?: string; productId?: string }>>;
      };
    };
    return { SerialPort: mod.SerialPort, list: () => mod.SerialPort.list() };
  } catch (err) {
    throw new Error(
      "the optional peer dependency 'serialport' is required for NodeSerialTransport " +
        "(npm i serialport)",
      { cause: err },
    );
  }
}

/** Enumerate system serial ports (Node only). */
export async function listSerialPorts(): Promise<
  Array<{ path: string; serialNumber?: string; vendorId?: string; productId?: string }>
> {
  const { list } = await loadSerialport();
  return list();
}

export class NodeSerialTransport implements SerialTransport {
  readonly info: SerialTransportInfo;
  private port: SerialPortLike | null = null;
  private queue: Uint8Array[] = [];
  private waiter: (() => void) | null = null;
  private ended = false;
  private disconnectCbs: Array<() => void> = [];

  constructor(private path: string, serialNumber?: string) {
    this.info = { path, serialNumber };
  }

  async open(opts?: { baudRate?: number }): Promise<void> {
    const { SerialPort } = await loadSerialport();
    const port = new SerialPort({
      path: this.path,
      baudRate: opts?.baudRate ?? 115200,
      autoOpen: false,
    });
    await new Promise<void>((resolve, reject) =>
      port.open((err) => (err ? reject(err) : resolve())),
    );
    port.on("data", (data) => {
      this.queue.push(new Uint8Array(data));
      this.waiter?.();
    });
    port.on("close", () => this.end());
    port.on("error", () => this.end());
    this.port = port;
  }

  private end(): void {
    if (this.ended) return;
    this.ended = true;
    this.waiter?.();
    const cbs = this.disconnectCbs;
    this.disconnectCbs = [];
    cbs.forEach((cb) => cb());
  }

  async write(data: Uint8Array): Promise<void> {
    const port = this.port;
    if (!port || !port.isOpen) throw new Error("port not open");
    await new Promise<void>((resolve, reject) =>
      port.write(data, (err) => (err ? reject(err) : resolve())),
    );
  }

  async *readable(): AsyncIterableIterator<Uint8Array> {
    for (;;) {
      while (this.queue.length === 0) {
        if (this.ended) return;
        await new Promise<void>((resolve) => (this.waiter = resolve));
        this.waiter = null;
      }
      yield this.queue.shift()!;
    }
  }

  async close(): Promise<void> {
    const port = this.port;
    if (port && port.isOpen) {
      await new Promise<void>((resolve) => port.close(() => resolve()));
    }
    this.end();
  }

  onDisconnect(cb: () => void): () => void {
    this.disconnectCbs.push(cb);
    return () => {
      this.disconnectCbs = this.disconnectCbs.filter((c) => c !== cb);
    };
  }
}

// ── discovery (Node / serialport — full behavior) ─────────────────────────────

function toPortInfo(p: {
  path: string;
  serialNumber?: string;
  vendorId?: string;
  productId?: string;
}): DepzPortInfo {
  return {
    path: p.path,
    serialNumber: p.serialNumber,
    usbVid: p.vendorId !== undefined ? parseInt(p.vendorId, 16) : undefined,
    usbPid: p.productId !== undefined ? parseInt(p.productId, 16) : undefined,
  };
}

const nodeFactory = (port: DepzPortInfo): SerialTransport =>
  new NodeSerialTransport(port.path, port.serialNumber);

/**
 * Enumerate DEPZ devices on the system's serial ports (Node only).
 *
 * With `matchUsb` (default) only ports carrying a known DEPZ (vid,pid) are
 * probed — fast, and it never pokes unrelated ports. Set `matchUsb: false`
 * for a legacy probe-everything scan.
 */
export async function listDepzDevices(opts: ListOptions = {}): Promise<DeviceInfo[]> {
  const ports = (await listSerialPorts()).map(toPortInfo);
  return listDepzDevicesFrom(ports, nodeFactory, opts);
}

/**
 * Open the right sensor class for `target` over `serialport` (Node only).
 *
 * `target` is a port path (string), a candidate index (number, sorted by USB
 * iSerial), a `DeviceInfo` from `listDepzDevices()`, or omitted for the
 * lowest-serial candidate. Throws `NoDepzDeviceError` when nothing matches —
 * it never grabs an arbitrary system port. See {@link openDeviceFrom}.
 */
export async function openDevice(
  target?: OpenTarget,
  opts: OpenDeviceOptions = {},
): Promise<DepzDevice> {
  const ports = (await listSerialPorts()).map(toPortInfo);
  return openDeviceFrom(ports, nodeFactory, target, opts);
}
