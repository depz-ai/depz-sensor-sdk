/**
 * Detection / discovery engine (contract 02 §4). Exercises openDeviceFrom /
 * listDepzDevicesFrom against a fake port lister — no hardware, no serialport.
 */

import { afterEach, beforeEach, describe, expect, it, vi } from "vitest";
import {
  Bno086,
  CrcType,
  DEPZ_VID,
  DEV_PID,
  DEV_VID,
  LoopbackTransport,
  NoDepzDeviceError,
  PacketParser,
  Sr04,
  Vl53l8,
  Vl53l8Ch,
  buildPacket,
  isKnownDepzUsb,
  listDepzDevicesFrom,
  openDeviceByDeviceSerial,
  openDeviceFrom,
  orderDevicesBySerial,
  pidToModel,
  type DepzPortInfo,
  type PacketEvent,
  type SerialTransport,
} from "../src/index.js";

/** Minimal configurable fake firmware: answers identity, else ERR_INVALID_CMD. */
class FakeFirmware {
  readonly transport: LoopbackTransport;
  private readonly side: LoopbackTransport;
  private txSeq = 0;
  private readonly parser = new PacketParser();

  constructor(
    readonly softwareName: string,
    readonly deviceName: string,
    readonly serial: string,
  ) {
    const [host, device] = LoopbackTransport.pair();
    this.transport = host;
    this.side = device;
    void this.run();
  }

  async close(): Promise<void> {
    await this.side.close();
  }

  private async send(cmd: number, payload: Uint8Array): Promise<void> {
    try {
      await this.side.write(buildPacket(cmd, payload, this.txSeq, CrcType.None));
    } catch {
      return;
    }
    this.txSeq = (this.txSeq + 1) & 0xff;
  }

  private text(cmd: number, text: string): Promise<void> {
    const ascii = new TextEncoder().encode(text);
    const payload = new Uint8Array(ascii.length + 2);
    payload[0] = cmd;
    payload.set(ascii, 1);
    return this.send(0x81, payload);
  }

  private async run(): Promise<void> {
    for await (const chunk of this.side.readable()) {
      for (const ev of this.parser.feed(chunk)) {
        if (ev.type === "packet") await this.handle(ev);
      }
    }
  }

  private async handle(pkt: PacketEvent): Promise<void> {
    if (pkt.cmd === 0x03) await this.text(0x03, this.deviceName);
    else if (pkt.cmd === 0x04) await this.text(0x04, this.softwareName);
    else if (pkt.cmd === 0x05) await this.text(0x05, this.serial);
    else await this.send(0x80, Uint8Array.of(pkt.cmd, 0x02));
  }
}

interface FakeSpec {
  port: DepzPortInfo;
  software: string;
  name: string;
  serial: string;
  /** When set, this port never answers (silent link) — probe times out. */
  silent?: boolean;
}

describe("usb-ids", () => {
  it("recognizes production + dev ids, rejects strangers", () => {
    expect(isKnownDepzUsb(DEPZ_VID, 0xec78)).toBe(true); // SR04
    expect(isKnownDepzUsb(DEPZ_VID, 0xed40)).toBe(true); // VL53L8
    expect(isKnownDepzUsb(DEPZ_VID, 0xee08)).toBe(true); // BNO086
    expect(isKnownDepzUsb(DEPZ_VID, 60740)).toBe(true); // inside sensor block
    expect(isKnownDepzUsb(DEV_VID, DEV_PID)).toBe(true); // dev default
    expect(isKnownDepzUsb(DEPZ_VID, 0x0001)).toBe(false); // wrong pid
    expect(isKnownDepzUsb(0x8086, 0x1234)).toBe(false); // unrelated
    expect(isKnownDepzUsb(null, null)).toBe(false);
  });

  it("maps pids to model hints", () => {
    expect(pidToModel(0xec78)?.sensorType).toBe("sr04");
    expect(pidToModel(0xed40)?.sensorType).toBe("vl53l8");
    expect(pidToModel(0xed40)?.name).toBe("vl53l8ch");
    expect(pidToModel(0xee08)?.sensorType).toBe("bno086");
    expect(pidToModel(0xee0a)?.name).toBe("bno055"); // 60938 — recognized, not decodable
    expect(pidToModel(0xee0a)?.sensorType).toBeNull();
    expect(pidToModel(0x9999)).toBeNull();
  });
});

describe("discovery engine", () => {
  let specs: FakeSpec[];
  const live: FakeFirmware[] = [];

  function factory(port: DepzPortInfo): SerialTransport {
    const spec = specs.find((s) => s.port.path === port.path);
    if (spec === undefined || spec.silent === true) {
      // No firmware behind this port → a peer-less loopback that never answers.
      return new LoopbackTransport();
    }
    const fw = new FakeFirmware(spec.software, spec.name, spec.serial);
    live.push(fw);
    return fw.transport;
  }

  const ports = (): DepzPortInfo[] => specs.map((s) => s.port);

  beforeEach(() => {
    specs = [
      {
        // Programmed SR04, higher serial.
        port: { path: "/dev/ttyACM1", serialNumber: "SN000009", usbVid: DEPZ_VID, usbPid: 0xec78 },
        software: "APP_usonic_SR04_v0.95",
        name: "DEPZ SR04",
        serial: "SN000009",
      },
      {
        // Dev unit (STM default vid/pid), lower serial — should win auto-select.
        port: { path: "/dev/ttyACM0", serialNumber: "SN000005", usbVid: DEV_VID, usbPid: DEV_PID },
        software: "APP_VL53L8_ToF_v1.0",
        name: "DEPZ ToF",
        serial: "SN000005",
      },
      {
        // Unrelated port (not a DEPZ id) but it *does* answer DEPZ protocol —
        // used to exercise the explicit-path warn-and-proceed path.
        port: { path: "/dev/ttyS0", usbVid: 0x8086, usbPid: 0x1234 },
        software: "APP_BNO086_IMU_v2.0",
        name: "DEPZ IMU",
        serial: "SNIMU",
      },
    ];
  });

  afterEach(async () => {
    await Promise.all(live.map((f) => f.close()));
    live.length = 0;
  });

  it("lists only known-USB DEPZ devices, in serial order", async () => {
    const found = await listDepzDevicesFrom(ports(), factory);
    expect(found.map((d) => d.path)).toEqual(["/dev/ttyACM0", "/dev/ttyACM1"]);
    expect(found[0]!.sensorType).toBe("vl53l8");
    expect(found[0]!.serialNumber).toBe("SN000005");
    expect(found[0]!.usbVid).toBe(DEV_VID);
    expect(found[1]!.sensorType).toBe("sr04");
  });

  it("matchUsb:false probes every port", async () => {
    const found = await listDepzDevicesFrom(ports(), factory, { matchUsb: false, timeoutMs: 100 });
    const paths = found.map((d) => d.path).sort();
    expect(paths).toEqual(["/dev/ttyACM0", "/dev/ttyACM1", "/dev/ttyS0"]);
  });

  it("auto-select picks the smallest-serial candidate", async () => {
    const dev = await openDeviceFrom(ports(), factory);
    try {
      expect(dev).toBeInstanceOf(Vl53l8); // SN000005 dev unit
      expect(await dev.getSerialNumber()).toBe("SN000005");
    } finally {
      await dev.close();
    }
  });

  it("integer index selects the Nth candidate by serial", async () => {
    const dev = await openDeviceFrom(ports(), factory, 1);
    try {
      expect(dev).toBeInstanceOf(Sr04); // SN000009
    } finally {
      await dev.close();
    }
  });

  it("serial option selects an exact candidate", async () => {
    const dev = await openDeviceFrom(ports(), factory, undefined, { serial: "SN000009" });
    try {
      expect(dev).toBeInstanceOf(Sr04);
    } finally {
      await dev.close();
    }
  });

  it("explicit port path warns on unknown USB id but proceeds", async () => {
    const warn = vi.spyOn(console, "warn").mockImplementation(() => {});
    const dev = await openDeviceFrom(ports(), factory, "/dev/ttyS0");
    try {
      expect(dev).toBeInstanceOf(Bno086);
      expect(warn).toHaveBeenCalledOnce();
      expect(warn.mock.calls[0]![0]).toContain("unrecognized USB id");
    } finally {
      await dev.close();
      warn.mockRestore();
    }
  });

  it("throws NoDepzDeviceError when nothing matches the serial", async () => {
    await expect(openDeviceFrom(ports(), factory, undefined, { serial: "SNXXX" })).rejects.toThrow(
      NoDepzDeviceError,
    );
  });

  it("throws NoDepzDeviceError for an out-of-range index", async () => {
    await expect(openDeviceFrom(ports(), factory, 5)).rejects.toThrow(NoDepzDeviceError);
  });

  it("throws NoDepzDeviceError when no candidate port exists", async () => {
    const onlyStrangers: DepzPortInfo[] = [{ path: "/dev/ttyS0", usbVid: 0x8086, usbPid: 0x1234 }];
    await expect(openDeviceFrom(onlyStrangers, factory)).rejects.toThrow(NoDepzDeviceError);
  });

  it("explicit path that never answers raises rather than hanging", async () => {
    specs[0]!.silent = true;
    await expect(
      openDeviceFrom(ports(), factory, "/dev/ttyACM1", { timeoutMs: 60 }),
    ).rejects.toThrow(NoDepzDeviceError);
  });
});

/**
 * WebSerial simulation: ports carry NO USB iSerial (browser can't read it), so
 * selection must fall back to the DEVICE serial obtained by probing.
 */
describe("device-serial selection (browser path)", () => {
  let specs: FakeSpec[];
  const live: FakeFirmware[] = [];

  function factory(port: DepzPortInfo): SerialTransport {
    const spec = specs.find((s) => s.port.path === port.path);
    if (spec === undefined || spec.silent === true) return new LoopbackTransport();
    const fw = new FakeFirmware(spec.software, spec.name, spec.serial);
    live.push(fw);
    return fw.transport;
  }
  const ports = (): DepzPortInfo[] => specs.map((s) => s.port);

  beforeEach(() => {
    // No serialNumber on the ports (browser), distinct GET_SERIAL per device;
    // granted order (path) deliberately NOT the serial order.
    specs = [
      {
        port: { path: "webserial:0", usbVid: DEPZ_VID, usbPid: 0xec78 },
        software: "APP_usonic_SR04_v0.95",
        name: "A",
        serial: "SN-C",
      },
      {
        port: { path: "webserial:1", usbVid: DEPZ_VID, usbPid: 0xee08 },
        software: "APP_BNO086_IMU_v2.0",
        name: "B",
        serial: "SN-A",
      },
      {
        port: { path: "webserial:2", usbVid: DEPZ_VID, usbPid: 0xed40 },
        software: "APP_VL53L8_ToF_v1.0",
        name: "C",
        serial: "SN-B",
      },
    ];
  });

  afterEach(async () => {
    await Promise.all(live.map((f) => f.close()));
    live.length = 0;
  });

  it("orders probed devices by device serial", async () => {
    const infos = await listDepzDevicesFrom(ports(), factory, { matchUsb: false });
    const ordered = orderDevicesBySerial(infos);
    expect(ordered.map((d) => d.serialNumber)).toEqual(["SN-A", "SN-B", "SN-C"]);
    expect(ordered.map((d) => d.path)).toEqual(["webserial:1", "webserial:2", "webserial:0"]);
  });

  it("default opens the smallest device serial", async () => {
    const dev = await openDeviceByDeviceSerial(ports(), factory);
    try {
      expect(await dev.getSerialNumber()).toBe("SN-A"); // BNO086 on webserial:1
      expect(dev).toBeInstanceOf(Bno086);
    } finally {
      await dev.close();
    }
  });

  it("index selects the Nth by device serial", async () => {
    const dev = await openDeviceByDeviceSerial(ports(), factory, 2);
    try {
      expect(await dev.getSerialNumber()).toBe("SN-C"); // SR04 on webserial:0
      expect(dev).toBeInstanceOf(Sr04);
    } finally {
      await dev.close();
    }
  });

  it("serial option selects by exact device serial", async () => {
    const dev = await openDeviceByDeviceSerial(ports(), factory, null, { serial: "SN-B" });
    try {
      // webserial:2 has PID 0xED40 → the CH silicon hint selects Vl53l8Ch
      // (which is also an instanceof the Vl53l8/Vl53l8Cx base).
      expect(dev).toBeInstanceOf(Vl53l8Ch);
      expect(dev).toBeInstanceOf(Vl53l8);
    } finally {
      await dev.close();
    }
  });

  it("unknown serial raises NoDepzDeviceError", async () => {
    await expect(
      openDeviceByDeviceSerial(ports(), factory, null, { serial: "SN-NOPE" }),
    ).rejects.toThrow(NoDepzDeviceError);
  });

  it("out-of-range index raises NoDepzDeviceError", async () => {
    await expect(openDeviceByDeviceSerial(ports(), factory, 9)).rejects.toThrow(NoDepzDeviceError);
  });
});

/**
 * Two identical-model sensors on the bench: selection must be purely by serial,
 * never by port/system order, and duplicate serials fall back to path order.
 */
describe("duplicate model types and serials", () => {
  let specs: FakeSpec[];
  const live: FakeFirmware[] = [];

  function factory(port: DepzPortInfo): SerialTransport {
    const spec = specs.find((s) => s.port.path === port.path);
    if (spec === undefined || spec.silent === true) return new LoopbackTransport();
    const fw = new FakeFirmware(spec.software, spec.name, spec.serial);
    live.push(fw);
    return fw.transport;
  }
  const ports = (): DepzPortInfo[] => specs.map((s) => s.port);

  afterEach(async () => {
    await Promise.all(live.map((f) => f.close()));
    live.length = 0;
  });

  it("orders and selects two SR04 by USB iSerial regardless of port order", async () => {
    // Two SR04s: the higher serial enumerates on the lower path (system order
    // must NOT win).
    specs = [
      {
        port: { path: "/dev/ttyACM0", serialNumber: "SN000009", usbVid: DEPZ_VID, usbPid: 0xec78 },
        software: "APP_usonic_SR04_v0.95",
        name: "SR04 #2",
        serial: "SN000009",
      },
      {
        port: { path: "/dev/ttyACM1", serialNumber: "SN000005", usbVid: DEPZ_VID, usbPid: 0xec78 },
        software: "APP_usonic_SR04_v0.95",
        name: "SR04 #1",
        serial: "SN000005",
      },
    ];
    const found = await listDepzDevicesFrom(ports(), factory);
    expect(found.map((d) => d.serialNumber)).toEqual(["SN000005", "SN000009"]);
    expect(found.every((d) => d.sensorType === "sr04")).toBe(true);

    const auto = await openDeviceFrom(ports(), factory); // smallest serial
    try {
      expect(await auto.getSerialNumber()).toBe("SN000005");
      expect(auto).toBeInstanceOf(Sr04);
    } finally {
      await auto.close();
    }
    const nth = await openDeviceFrom(ports(), factory, 1); // Nth by serial
    try {
      expect(await nth.getSerialNumber()).toBe("SN000009");
    } finally {
      await nth.close();
    }
  });

  it("breaks duplicate device serials by path (browser probe path)", async () => {
    // Same GET_SERIAL on two granted ports with no USB iSerial (WebSerial).
    specs = [
      {
        port: { path: "webserial:1", usbVid: DEPZ_VID, usbPid: 0xec78 },
        software: "APP_usonic_SR04_v0.95",
        name: "dup-b",
        serial: "SN-DUP",
      },
      {
        port: { path: "webserial:0", usbVid: DEPZ_VID, usbPid: 0xec78 },
        software: "APP_usonic_SR04_v0.95",
        name: "dup-a",
        serial: "SN-DUP",
      },
    ];
    const infos = await listDepzDevicesFrom(ports(), factory, { matchUsb: false });
    const ordered = orderDevicesBySerial(infos);
    // Equal serials → deterministic path order.
    expect(ordered.map((d) => d.path)).toEqual(["webserial:0", "webserial:1"]);

    const dev = await openDeviceByDeviceSerial(ports(), factory); // default = first
    try {
      expect(dev).toBeInstanceOf(Sr04);
      expect(await dev.getSerialNumber()).toBe("SN-DUP");
    } finally {
      await dev.close();
    }
  });
});
