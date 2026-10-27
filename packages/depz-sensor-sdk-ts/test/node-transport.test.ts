/**
 * Node transport (`@depz/sensor-sdk/node`) over a fake `serialport` module:
 * `NodeSerialTransport` I/O, its end-of-stream / disconnect paths, and the
 * serialport-backed discovery (`listSerialPorts`, `listDepzDevices`,
 * `openDevice`) with each fake port piped to an in-process fake firmware
 * (test/fake-tof.ts), so the real read/write/close paths run end to end.
 * The missing-peer-dependency path is in `node-transport-missing.test.ts`.
 */

import { mkdtempSync, rmSync, symlinkSync, writeFileSync } from "node:fs";
import { tmpdir } from "node:os";
import { join } from "node:path";

import { afterEach, describe, expect, it, vi } from "vitest";
import { DepzDevice, NoDepzDeviceError, Vl53l8ch, Vl53l8cx, type SerialTransport } from "../src/index.js";
import { NodeSerialTransport, listDepzDevices, listSerialPorts, openDevice, withAlias } from "../src/transport/node.js";
import { FakeTof } from "./fake-tof.js";

interface PortRow {
  path: string;
  serialNumber?: string;
  vendorId?: string;
  productId?: string;
}

/**
 * The fake `serialport` module. Hoisted: `vi.mock` factories run before the
 * imports above, so the class is built with no imports of its own. A port's
 * bytes go to the `backend` registered under its path (a LoopbackTransport);
 * inbound bytes arrive through `current` — the one open port of that path.
 */
const { fakeSerial, FakeSerialPort } = vi.hoisted(() => {
  type Backend = { write(data: Uint8Array): Promise<void> };
  const fakeSerial = {
    ports: [] as PortRow[],
    backends: new Map<string, Backend>(),
    current: new Map<string, FakeSerialPort>(),
    instances: [] as FakeSerialPort[],
    openError: null as Error | null,
    writeError: null as Error | null,
  };

  class FakeSerialPort {
    readonly path: string;
    readonly baudRate: number;
    readonly autoOpen: boolean;
    isOpen = false;
    closes = 0;
    private readonly handlers = new Map<string, Array<(...args: unknown[]) => void>>();

    constructor(opts: { path: string; baudRate: number; autoOpen: boolean }) {
      this.path = opts.path;
      this.baudRate = opts.baudRate;
      this.autoOpen = opts.autoOpen;
      fakeSerial.instances.push(this);
    }

    static list(): Promise<PortRow[]> {
      return Promise.resolve(fakeSerial.ports);
    }

    on(event: string, cb: (...args: unknown[]) => void): void {
      const list = this.handlers.get(event) ?? [];
      list.push(cb);
      this.handlers.set(event, list);
    }

    off(event: string, cb: (...args: unknown[]) => void): void {
      this.handlers.set(event, (this.handlers.get(event) ?? []).filter((h) => h !== cb));
    }

    emit(event: string, ...args: unknown[]): void {
      for (const h of [...(this.handlers.get(event) ?? [])]) h(...args);
    }

    open(cb: (err?: Error | null) => void): void {
      if (fakeSerial.openError !== null) return cb(fakeSerial.openError);
      this.isOpen = true;
      fakeSerial.current.set(this.path, this);
      cb(null);
    }

    write(data: Uint8Array, cb: (err?: Error | null) => void): void {
      if (fakeSerial.writeError !== null) return cb(fakeSerial.writeError);
      const backend = fakeSerial.backends.get(this.path);
      if (backend === undefined) return cb(new Error(`no backend on ${this.path}`));
      backend.write(data).then(() => cb(null), cb);
    }

    drain(cb: (err?: Error | null) => void): void {
      cb(null);
    }

    close(cb: (err?: Error | null) => void): void {
      this.closes += 1;
      this.isOpen = false;
      if (fakeSerial.current.get(this.path) === this) fakeSerial.current.delete(this.path);
      this.emit("close");
      cb(null);
    }
  }

  return { fakeSerial, FakeSerialPort };
});

vi.mock("serialport", () => ({ SerialPort: FakeSerialPort }));

const fws: FakeTof[] = [];
const opened: Array<{ close(): Promise<void> }> = [];

/** A fake firmware behind `path`; one pump feeds whichever port is open on it. */
function plug(path: string, fw: FakeTof): void {
  fws.push(fw);
  fakeSerial.backends.set(path, fw.transport);
  void (async () => {
    for await (const chunk of fw.transport.readable()) {
      fakeSerial.current.get(path)?.emit("data", Buffer.from(chunk));
    }
  })();
}

function bench(rows: Array<PortRow & { serial?: string; plug?: boolean }>): void {
  fakeSerial.ports = rows.map(({ serial: _s, plug: _p, ...row }) => row);
  for (const row of rows) {
    if (row.plug !== false) plug(row.path, new FakeTof({ serial: row.serial ?? row.serialNumber ?? "SN" }));
  }
}

afterEach(async () => {
  for (const d of opened.splice(0)) await d.close().catch(() => undefined);
  for (const fw of fws.splice(0)) await fw.close();
  fakeSerial.ports = [];
  fakeSerial.backends.clear();
  fakeSerial.current.clear();
  fakeSerial.instances.length = 0;
  fakeSerial.openError = null;
  fakeSerial.writeError = null;
});

const settle = () => new Promise((r) => setTimeout(r, 10));

// ── NodeSerialTransport ───────────────────────────────────────────────────────

describe("NodeSerialTransport", () => {
  it("opens a non-auto-open port at 115200 by default and carries its info", async () => {
    plug("/dev/ttyACM0", new FakeTof({ serial: "SN1" }));
    const t = new NodeSerialTransport("/dev/ttyACM0", "SN1");
    expect(t.info).toEqual({ path: "/dev/ttyACM0", serialNumber: "SN1" });
    await t.open();
    const port = fakeSerial.instances[0]!;
    expect([port.path, port.baudRate, port.autoOpen, port.isOpen]).toEqual(["/dev/ttyACM0", 115200, false, true]);
    await t.close();
    expect(port.isOpen).toBe(false);
  });

  it("honours a baud override", async () => {
    plug("/dev/ttyACM0", new FakeTof());
    const t = new NodeSerialTransport("/dev/ttyACM0");
    await t.open({ baudRate: 921600 });
    expect(fakeSerial.instances[0]!.baudRate).toBe(921600);
    await t.close();
  });

  it("a device talks over it end to end", async () => {
    plug("/dev/ttyACM0", new FakeTof({ serial: "SN1" }));
    const dev = new DepzDevice(new NodeSerialTransport("/dev/ttyACM0", "SN1"), { timeoutMs: 500 });
    opened.push(dev);
    await dev.open();
    expect(await dev.getDeviceName()).toBe("DEPZ VL53L8");
    expect(await dev.getSerialNumber()).toBe("SN1");
  });

  it("write before open, or after close, is refused", async () => {
    plug("/dev/ttyACM0", new FakeTof());
    const t = new NodeSerialTransport("/dev/ttyACM0");
    await expect(t.write(Uint8Array.of(1))).rejects.toThrow(/port not open/);
    await t.open();
    await t.close();
    await expect(t.write(Uint8Array.of(1))).rejects.toThrow(/port not open/);
  });

  it("open() rejects with the port's error", async () => {
    fakeSerial.openError = new Error("EBUSY");
    const t = new NodeSerialTransport("/dev/ttyACM9");
    await expect(t.open()).rejects.toThrow(/EBUSY/);
    await t.close(); // nothing to close: no throw
  });

  it("write() rejects with the port's error", async () => {
    plug("/dev/ttyACM0", new FakeTof());
    const t = new NodeSerialTransport("/dev/ttyACM0");
    await t.open();
    fakeSerial.writeError = new Error("EIO");
    await expect(t.write(Uint8Array.of(1))).rejects.toThrow(/EIO/);
    await t.close();
  });

  it("readable() yields inbound chunks and ends on close; disconnect fires once", async () => {
    plug("/dev/ttyACM0", new FakeTof());
    const t = new NodeSerialTransport("/dev/ttyACM0");
    await t.open();
    const port = fakeSerial.instances[0]!;
    let fired = 0;
    const unsub = t.onDisconnect(() => {
      fired += 1;
    });
    const unsubOther = t.onDisconnect(() => {
      throw new Error("unsubscribed callbacks never fire");
    });
    unsubOther();
    const chunks: number[][] = [];
    const reader = (async () => {
      for await (const c of t.readable()) chunks.push([...c]);
    })();
    port.emit("data", Buffer.from([1, 2]));
    port.emit("data", Buffer.from([3]));
    await settle();
    await t.close();
    await reader;
    expect(chunks).toEqual([[1, 2], [3]]);
    expect(fired).toBe(1);
    port.emit("close"); // a second close event: already ended
    expect(fired).toBe(1);
    unsub();
  });

  it("an 'error' event ends the stream like an unplug", async () => {
    plug("/dev/ttyACM0", new FakeTof());
    const t = new NodeSerialTransport("/dev/ttyACM0");
    await t.open();
    let fired = 0;
    t.onDisconnect(() => {
      fired += 1;
    });
    const reader = (async () => {
      const got: Uint8Array[] = [];
      for await (const c of t.readable()) got.push(c);
      return got;
    })();
    await settle();
    fakeSerial.instances[0]!.emit("error", new Error("unplugged"));
    expect(await reader).toEqual([]);
    expect(fired).toBe(1);
    await t.close();
  });
});

// ── discovery over serialport ────────────────────────────────────────────────

describe("node discovery", () => {
  it("listSerialPorts returns the enumerated ports", async () => {
    bench([{ path: "/dev/ttyACM0", serialNumber: "A", vendorId: "1bcf", productId: "ee08", plug: false }]);
    expect(await listSerialPorts()).toEqual(fakeSerial.ports);
  });

  it("listDepzDevices probes only known DEPZ USB ids by default, in serial order", async () => {
    bench([
      { path: "/dev/ttyACM1", serialNumber: "SN2", vendorId: "1bcf", productId: "ee08" },
      { path: "/dev/ttyACM0", serialNumber: "SN1", vendorId: "1bcf", productId: "ee08" },
      { path: "/dev/ttyUSB0", serialNumber: "XX", vendorId: "0403", productId: "6001" }, // an FTDI: never opened
      { path: "/dev/ttyS0" }, // no USB ids at all
    ]);
    const found = await listDepzDevices({ timeoutMs: 500 });
    expect(found.map((d) => [d.path, d.serialNumber, d.sensorType, d.usbVid, d.usbPid])).toEqual([
      ["/dev/ttyACM0", "SN1", "vl53l8", 0x1bcf, 0xee08],
      ["/dev/ttyACM1", "SN2", "vl53l8", 0x1bcf, 0xee08],
    ]);
    expect(fakeSerial.instances.map((p) => p.path)).toEqual(["/dev/ttyACM0", "/dev/ttyACM1"]);
    // every probe closed its port
    expect(fakeSerial.instances.every((p) => !p.isOpen && p.closes === 1)).toBe(true);
  });

  it("listDepzDevices({ matchUsb: false }) probes everything and keeps what answers", async () => {
    bench([
      { path: "/dev/ttyS0", plug: false }, // nothing behind it: the probe times out
      { path: "/dev/ttyUSB0", serialNumber: "XX", vendorId: "0403", productId: "6001", serial: "SN9" },
    ]);
    const found = await listDepzDevices({ matchUsb: false, timeoutMs: 100 });
    expect(found.map((d) => [d.path, d.serialNumber, d.usbVid, d.usbPid])).toEqual([
      ["/dev/ttyUSB0", "SN9", 0x0403, 0x6001],
    ]);
  });

  it("openDevice() picks the lowest serial and returns the sensor class", async () => {
    bench([
      { path: "/dev/ttyACM1", serialNumber: "SN2", vendorId: "1bcf", productId: "ee08" },
      { path: "/dev/ttyACM0", serialNumber: "SN1", vendorId: "1bcf", productId: "ee08" },
    ]);
    const dev = await openDevice(undefined, { timeoutMs: 500 });
    opened.push(dev);
    expect(dev).toBeInstanceOf(Vl53l8cx);
    expect(await dev.getSerialNumber()).toBe("SN1");
    const transport = (dev as unknown as { transport: SerialTransport }).transport;
    expect(transport.info).toEqual({ path: "/dev/ttyACM0", serialNumber: "SN1" });
  });

  it("openDevice(path) and openDevice(index) select explicitly; {serial} too", async () => {
    bench([
      { path: "/dev/ttyACM1", serialNumber: "SN2", vendorId: "1bcf", productId: "ee08" },
      { path: "/dev/ttyACM0", serialNumber: "SN1", vendorId: "1bcf", productId: "ee08" },
    ]);
    const byPath = await openDevice("/dev/ttyACM1", { timeoutMs: 500 });
    opened.push(byPath);
    expect(await byPath.getSerialNumber()).toBe("SN2");
    await byPath.close();
    const byIndex = await openDevice(1, { timeoutMs: 500 });
    opened.push(byIndex);
    expect(await byIndex.getSerialNumber()).toBe("SN2");
    await byIndex.close();
    const bySerial = await openDevice(undefined, { serial: "SN2", timeoutMs: 500 });
    opened.push(bySerial);
    expect(await bySerial.getSerialNumber()).toBe("SN2");
  });

  it("openDevice(alias) resolves the class by the USB id of the port the alias names (ERRATA A5)", async () => {
    // A /dev/serial/by-id/… symlink: a regular file stands in for the node (realpath resolves either).
    const dir = mkdtempSync(join(tmpdir(), "depz-a5-"));
    const node = join(dir, "ttyACM0");
    const alias = join(dir, "usb-DEPZ_ToF-if00");
    writeFileSync(node, "");
    symlinkSync(node, alias);
    try {
      // The OS lists the node with the VL53L8CH PID; the firmware cannot tell CH from CX.
      fakeSerial.ports = [{ path: node, serialNumber: "SN1", vendorId: "1bcf", productId: "ed40" }];
      plug(alias, new FakeTof({ serial: "SN1" }));
      const warn = vi.spyOn(console, "warn").mockImplementation(() => {});
      const dev = await openDevice(alias, { timeoutMs: 500 });
      opened.push(dev);
      expect(dev).toBeInstanceOf(Vl53l8ch);
      expect(fakeSerial.instances.map((p) => p.path)).toEqual([alias, alias]); // the path given, probe then class
      expect(warn).not.toHaveBeenCalled(); // a known DEPZ id: nothing to warn about
      warn.mockRestore();
    } finally {
      rmSync(dir, { recursive: true, force: true });
    }
  });

  it("withAlias leaves a listed path, or an alias of nothing listed, alone", () => {
    const ports = [{ path: "/dev/ttyACM0", usbVid: 0x1bcf, usbPid: 0xed40 }];
    expect(withAlias(ports, "/dev/ttyACM0")).toBe(ports);
    expect(withAlias(ports, "/dev/depz-unlisted")).toBe(ports);
    expect(withAlias([], "/dev/null")).toEqual([]);
  });

  it("openDevice() never grabs an arbitrary port", async () => {
    bench([{ path: "/dev/ttyUSB0", serialNumber: "XX", vendorId: "0403", productId: "6001" }]);
    await expect(openDevice(undefined, { timeoutMs: 100 })).rejects.toThrow(NoDepzDeviceError);
    await expect(openDevice(0, { timeoutMs: 100 })).rejects.toThrow(NoDepzDeviceError);
    await expect(openDevice("/dev/ttyS7", { timeoutMs: 100 })).rejects.toThrow(NoDepzDeviceError);
  });
});
