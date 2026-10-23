/**
 * WebSerial transport I/O and browser discovery (contract 07: the browser path
 * selects devices by DEVICE serial read over the protocol, since WebSerial
 * exposes only vid/pid of already-granted ports).
 *
 * `navigator.serial` is stubbed; each granted port is a stream-backed fake
 * whose bytes reach an in-process fake VL53L8 firmware (test/fake-tof.ts), so
 * WebSerialTransport's real read/write/close paths run end to end.
 */

import { afterEach, describe, expect, it, vi } from "vitest";
import { NoDepzDeviceError, Vl53l8cx } from "../src/index.js";
import {
  WebSerialTransport,
  getGrantedPorts,
  isWebSerialSupported,
  listDepzDevices,
  openDevice,
  watchConnect,
  type WebSerialPortLike,
} from "../src/transport/webserial.js";
import { FakeTof } from "./fake-tof.js";

/**
 * A granted port whose readable/writable streams are bridged to a fake
 * firmware. Like Chrome, `readable`/`writable` are null while closed, and a
 * fresh pair is created on every open (probe → close → reopen works).
 */
class StreamPort implements WebSerialPortLike {
  readable: ReadableStream<Uint8Array> | null = null;
  writable: WritableStream<Uint8Array> | null = null;
  opens = 0;
  private ctl: ReadableStreamDefaultController<Uint8Array> | null = null;

  constructor(
    readonly fw: FakeTof,
    private readonly pid = 0xee08,
  ) {
    void this.pump();
  }

  /** One long-lived reader of the firmware's TX; dropped while closed. */
  private async pump(): Promise<void> {
    for await (const chunk of this.fw.transport.readable()) this.ctl?.enqueue(chunk);
  }

  getInfo(): { usbVendorId?: number; usbProductId?: number } {
    return { usbVendorId: 0x1bcf, usbProductId: this.pid };
  }

  open(): Promise<void> {
    this.opens += 1;
    this.readable = new ReadableStream<Uint8Array>({
      start: (c) => {
        this.ctl = c;
      },
      cancel: () => {
        this.ctl = null;
      },
    });
    this.writable = new WritableStream<Uint8Array>({
      write: (chunk) => this.fw.transport.write(chunk),
    });
    return Promise.resolve();
  }

  close(): Promise<void> {
    this.ctl = null;
    this.readable = null;
    this.writable = null;
    return Promise.resolve();
  }
}

class FakeSerial {
  readonly handlers = new Set<(ev: unknown) => void>();
  constructor(readonly ports: WebSerialPortLike[]) {}
  getPorts(): Promise<WebSerialPortLike[]> {
    return Promise.resolve(this.ports);
  }
  addEventListener(_type: "connect" | "disconnect", cb: (ev: unknown) => void): void {
    this.handlers.add(cb);
  }
  removeEventListener(_type: "connect" | "disconnect", cb: (ev: unknown) => void): void {
    this.handlers.delete(cb);
  }
  emit(ev: unknown): void {
    for (const h of [...this.handlers]) h(ev);
  }
}

const fws: FakeTof[] = [];

function grant(...serials: string[]): { serial: FakeSerial; ports: StreamPort[] } {
  const ports = serials.map((s) => {
    const fw = new FakeTof({ serial: s });
    fws.push(fw);
    return new StreamPort(fw);
  });
  const serial = new FakeSerial(ports);
  vi.stubGlobal("navigator", { serial });
  return { serial, ports };
}

afterEach(async () => {
  vi.unstubAllGlobals();
  for (const fw of fws.splice(0)) await fw.close();
});

// ── feature detection / granted ports / connect events ────────────────────────

describe("webserial environment helpers", () => {
  it("unsupported environment: no serial API, no ports, inert watcher", async () => {
    vi.stubGlobal("navigator", {});
    expect(isWebSerialSupported()).toBe(false);
    expect(await getGrantedPorts()).toEqual([]);
    const unsubscribe = watchConnect(() => {
      throw new Error("must never fire");
    });
    unsubscribe();
  });

  it("supported environment lists the granted ports", async () => {
    const { ports } = grant("SN1");
    expect(isWebSerialSupported()).toBe(true);
    expect(await getGrantedPorts()).toEqual(ports);
  });

  it("watchConnect reports the connected port and stops after unsubscribe", () => {
    const { serial, ports } = grant("SN1");
    const seen: WebSerialPortLike[] = [];
    const unsubscribe = watchConnect((p) => seen.push(p));
    serial.emit({ target: ports[0] });
    serial.emit({}); // an event without a port is ignored
    expect(seen).toEqual([ports[0]]);
    unsubscribe();
    expect(serial.handlers.size).toBe(0);
    serial.emit({ target: ports[0] });
    expect(seen).toHaveLength(1);
  });
});

// ── WebSerialTransport I/O ────────────────────────────────────────────────────

describe("WebSerialTransport I/O", () => {
  it("write / readable refuse a port that is not open", async () => {
    const t = new WebSerialTransport(new StreamPort(new FakeTof()));
    await expect(t.write(Uint8Array.of(1))).rejects.toThrow(/not writable/);
    await expect(t.readable().next()).rejects.toThrow(/not readable/);
  });

  it("opens with a 64 KB buffer and the requested baud rate", async () => {
    const calls: Array<{ baudRate: number; bufferSize?: number }> = [];
    const port = new StreamPort(new FakeTof());
    const open = port.open.bind(port);
    port.open = (o?: { baudRate: number; bufferSize?: number }): Promise<void> => {
      calls.push(o!);
      return open();
    };
    const t = new WebSerialTransport(port);
    await t.open({ baudRate: 921600 });
    await t.close();
    await t.open();
    await t.close();
    expect(calls).toEqual([
      { baudRate: 921600, bufferSize: 65536 },
      { baudRate: 115200, bufferSize: 65536 },
    ]);
  });

  it("a read failure (cable yank) ends the stream and fires onDisconnect", async () => {
    let fail!: (e: Error) => void;
    const port: WebSerialPortLike = {
      readable: new ReadableStream<Uint8Array>({
        start: (c) => {
          c.enqueue(Uint8Array.of(0xaa));
          c.enqueue(new Uint8Array(0)); // empty chunks are skipped
          fail = (e) => c.error(e);
        },
      }),
      writable: null,
      getInfo: () => ({}),
      open: () => Promise.resolve(),
      close: () => Promise.resolve(),
    };
    const t = new WebSerialTransport(port);
    let fired = 0;
    t.onDisconnect(() => (fired += 1));
    const it = t.readable();
    expect((await it.next()).value).toEqual(Uint8Array.of(0xaa));
    const pending = it.next();
    fail(new Error("device lost"));
    expect((await pending).done).toBe(true);
    expect(fired).toBe(1);
    // the reader lock was released: the stream can be locked again
    expect(() => port.readable!.getReader()).not.toThrow();
    await t.close(); // nothing left to release; must not throw
  });

  it("an onDisconnect unsubscribe is honoured", async () => {
    const port: WebSerialPortLike = {
      readable: new ReadableStream<Uint8Array>({ start: (c) => c.error(new Error("x")) }),
      writable: null,
      getInfo: () => ({}),
      open: () => Promise.resolve(),
      close: () => Promise.resolve(),
    };
    const t = new WebSerialTransport(port);
    let fired = 0;
    const off = t.onDisconnect(() => (fired += 1));
    off();
    await t.readable().next();
    expect(fired).toBe(0);
  });

  it("close() cancels a pending read and tolerates a failing port.close()", async () => {
    const port = new StreamPort(new FakeTof());
    port.close = (): Promise<void> => Promise.reject(new Error("already closing"));
    const t = new WebSerialTransport(port);
    await t.open();
    await t.write(Uint8Array.of(0)); // takes the writer lock
    const pending = t.readable().next();
    await t.close();
    expect((await pending).done).toBe(true);
  });
});

// ── discovery over granted ports ──────────────────────────────────────────────

describe("webserial discovery", () => {
  it("listDepzDevices probes every granted port and orders by device serial", async () => {
    grant("SN9", "SN1");
    const found = await listDepzDevices();
    expect(found.map((d) => d.serialNumber)).toEqual(["SN1", "SN9"]);
    expect(found.map((d) => d.path)).toEqual(["webserial:1", "webserial:0"]);
    expect(found.every((d) => d.sensorType === "vl53l8" && d.usbPid === 0xee08)).toBe(true);
  });

  it("openDevice() picks the smallest device serial", async () => {
    grant("SN9", "SN1");
    const dev = await openDevice();
    try {
      expect(dev).toBeInstanceOf(Vl53l8cx);
      expect(await dev.getSerialNumber()).toBe("SN1");
    } finally {
      await dev.close();
    }
  });

  it("openDevice({serial}) / index select by device serial", async () => {
    grant("SN9", "SN1", "SN5");
    const bySerial = await openDevice(undefined, { serial: "SN5" });
    expect(await bySerial.getSerialNumber()).toBe("SN5");
    await bySerial.close();
    const byIndex = await openDevice(2);
    expect(await byIndex.getSerialNumber()).toBe("SN9");
    await byIndex.close();
    await expect(openDevice(undefined, { serial: "NOPE" })).rejects.toThrow(NoDepzDeviceError);
  });

  it("an explicit granted-port path opens exactly that port", async () => {
    const { ports } = grant("SN9", "SN1");
    const dev = await openDevice("webserial:0");
    try {
      expect(await dev.getSerialNumber()).toBe("SN9");
      expect(ports[1]!.opens).toBe(0); // no selection scan of the other port
    } finally {
      await dev.close();
    }
  });

  it("a raw port from requestPort() opens directly, without listing", async () => {
    vi.stubGlobal("navigator", {}); // no granted ports at all
    const fw = new FakeTof({ serial: "SN7" });
    fws.push(fw);
    const dev = await openDevice(new StreamPort(fw));
    try {
      expect(dev).toBeInstanceOf(Vl53l8cx);
      expect(await dev.getSerialNumber()).toBe("SN7");
    } finally {
      await dev.close();
    }
  });

  it("no granted ports → NoDepzDeviceError", async () => {
    vi.stubGlobal("navigator", { serial: new FakeSerial([]) });
    expect(await listDepzDevices()).toEqual([]);
    await expect(openDevice()).rejects.toThrow(NoDepzDeviceError);
  });
});
