/**
 * VL53L5CX / VL53L7CX / VL53L7CH device classes (sensors/vl53l7) over the
 * wire against the in-process I2C board fake (fake-vl53l7-board.ts): what the
 * L5/L7 family does differently from VL53L8 — the module-type read after
 * init() and the class / module-type warning, the 1536-byte read split, the
 * 1 Hz floor, the board commands PIN_CTRL / GET_INFO / SET_I2C_SPEED and
 * their refusals, the xtalk "coverglass too good" flag and the CH-only
 * configureCnh. Everything the classes inherit unchanged is covered by
 * `vl53l8cx-device.test.ts`. Mirror of the Python `tests/test_vl53l7_device.py`
 * minus the blob download: the TS SDK has no in-process L5/L7 die, so the
 * `Vl53l8cx.init()` the L7 classes extend is stubbed.
 */

import { afterEach, describe, expect, it, vi } from "vitest";
import { BusyError, CnhConfig, DepzError } from "../src/index.js";
import {
  PinAction,
  VL53L7_READ_MAX_LEN,
  VL53L7_WRITE_MAX_LEN,
  Vl53l7I2cError,
  packVl53l7ReadReg,
  packVl53l7WriteReg,
} from "../src/protocol/vl53l7.js";
import { packReadReg, packWriteReg } from "../src/protocol/vl53l8.js";
import { MI_CFG_DEV_IDX } from "../src/sensors/vl53l8/cnh.js";
import { MODULE_TYPE_MZ, MODULE_TYPE_MZEVO, RESOLUTION_8X8, type VL53L8CX } from "../src/sensors/vl53l8/uld.js";
import { Vl53l8cx } from "../src/sensors/vl53l8/vl53l8.js";
import { VL53L7_MIN_RANGING_FREQUENCY_HZ, Vl53l5cx, Vl53l7ch, Vl53l7cx } from "../src/sensors/vl53l7/vl53l7.js";
import { FakeVl53l7Board } from "./fake-vl53l7-board.js";
import { makeDriver } from "./fake-vl53l8.js";

type L7 = Vl53l7cx & { uldDriver: VL53L8CX | null; rangingFlag: boolean; readChunk: number; writeChunk: number };

const open: Array<{ dev: Vl53l7cx; fake: FakeVl53l7Board }> = [];
afterEach(async () => {
  vi.restoreAllMocks();
  for (const { dev, fake } of open.splice(0)) {
    await dev.close().catch(() => undefined);
    await fake.close();
  }
});

async function board<T extends Vl53l7cx>(Cls: new (...a: ConstructorParameters<typeof Vl53l7cx>) => T): Promise<{ dev: T & L7; fake: FakeVl53l7Board }> {
  const fake = new FakeVl53l7Board();
  const dev = new Cls(fake.transport, { timeoutMs: 500 });
  await dev.open();
  open.push({ dev, fake });
  return { dev: dev as T & L7, fake };
}

/**
 * Stand in for the blob download: `Vl53l8cx.init()` installs a ULD driver on
 * the fake DCI platform, reporting `moduleType` as the silicon would.
 */
function stubInit(moduleType: number | null, extra: Partial<VL53L8CX> = {}) {
  return vi.spyOn(Vl53l8cx.prototype, "init").mockImplementation(async function (this: L7) {
    const { drv } = makeDriver(); // no class fields: set the ones the device reads
    drv.moduleType = moduleType;
    drv.xtalkCalibrationFailed = false;
    Object.assign(drv, extra);
    this.uldDriver = drv;
  });
}

// ── identity and class wiring ────────────────────────────────────────────────

describe("vl53l7 class wiring", () => {
  it("L5 and L7CH are L7CX subclasses with the I2C board's limits", async () => {
    const { dev, fake } = await board(Vl53l7cx);
    expect(new Vl53l5cx(fake.transport)).toBeInstanceOf(Vl53l7cx);
    expect(new Vl53l7ch(fake.transport)).toBeInstanceOf(Vl53l7cx);
    expect(dev).toBeInstanceOf(Vl53l8cx);
    expect(dev.readChunk).toBe(VL53L7_READ_MAX_LEN);
    expect(dev.writeChunk).toBe(VL53L7_WRITE_MAX_LEN);
    expect(VL53L7_MIN_RANGING_FREQUENCY_HZ).toBe(1);
    expect(dev.moduleType).toBeNull(); // readable only after init()
    expect(dev.xtalkCalibrationFailed).toBe(false); // before init(): nothing calibrated
    expect(await dev.getSoftwareName()).toBe(FakeVl53l7Board.SOFTWARE_NAME);
  });

  it("configureCnh is CH-only", async () => {
    const { dev, fake } = await board(Vl53l7ch);
    expect((new Vl53l7cx(fake.transport) as unknown as { configureCnh?: unknown }).configureCnh).toBeUndefined();
    const { drv, p } = makeDriver();
    dev.uldDriver = drv;
    const config = new CnhConfig();
    config.initConfig(0, 24, 4);
    config.createAggMap(64, 0, 0, 2, 2, 4, 4);
    await dev.configureCnh(config);
    expect(p.dci.has(MI_CFG_DEV_IDX)).toBe(true);
    expect(dev.activeCnhConfig).toBe(config);
  });

  it("configureCnh and the motion indicator disarm each other (one DCI block, ERRATA B4)", async () => {
    const { dev } = await board(Vl53l7ch);
    const { drv } = makeDriver();
    dev.uldDriver = drv;
    await drv.setResolution(RESOLUTION_8X8);
    await dev.configureMotionIndicator(400, 1500);
    const config = new CnhConfig();
    config.initConfig(0, 24, 4);
    config.createAggMap(64, 0, 0, 2, 2, 4, 4);
    await dev.configureCnh(config);
    expect(drv.motionPresent).toBe(false);
    expect(dev.activeCnhConfig).toBe(config);
    await dev.configureMotionIndicator(400, 1500);
    expect(drv.motionPresent).toBe(true);
    expect(dev.activeCnhConfig).toBeNull();
  });
});

// ── init: the module-type check ──────────────────────────────────────────────

describe("vl53l7 init", () => {
  it.each([
    [Vl53l7cx, MODULE_TYPE_MZEVO],
    [Vl53l5cx, MODULE_TYPE_MZ],
    [Vl53l7ch, MODULE_TYPE_MZEVO],
  ])("%o with the expected module type is silent", async (Cls, reported) => {
    const { dev } = await board(Cls);
    const warn = vi.spyOn(console, "warn").mockImplementation(() => {});
    stubInit(reported);
    await dev.init();
    expect(dev.moduleType).toBe(reported);
    expect(warn).not.toHaveBeenCalled();
  });

  it.each([
    [Vl53l7cx, MODULE_TYPE_MZ, "MZEVO", "MZ"], // an L5 on a board labelled L7
    [Vl53l5cx, MODULE_TYPE_MZEVO, "MZ", "MZEVO"], // an L7 on a board labelled L5
    [Vl53l7ch, MODULE_TYPE_MZ, "MZEVO", "MZ"],
  ])("%o warns when the module type contradicts the class", async (Cls, reported, expects, got) => {
    const { dev } = await board(Cls);
    const warn = vi.spyOn(console, "warn").mockImplementation(() => {});
    stubInit(reported);
    await dev.init();
    expect(warn).toHaveBeenCalledTimes(1);
    expect(warn.mock.calls[0]![0]).toMatch(
      new RegExp(`${Cls.name} expects module type ${expects}, the sensor reports ${got} — check the board`),
    );
    // a warning, not an error: the blob is shared, ranging works
    expect(dev.moduleType).toBe(reported);
  });

  it("names an unknown module type by number, and keeps quiet when none was read", async () => {
    const { dev } = await board(Vl53l7cx);
    const warn = vi.spyOn(console, "warn").mockImplementation(() => {});
    stubInit(7);
    await dev.init();
    expect(warn.mock.calls[0]![0]).toMatch(/expects module type MZEVO, the sensor reports 7 /);
    expect(dev.moduleType).toBe(7);
    warn.mockClear();
    stubInit(null);
    await dev.init();
    expect(warn).not.toHaveBeenCalled();
    expect(dev.moduleType).toBeNull();
  });

  it("passes the variant and options through to the blob download", async () => {
    const { dev } = await board(Vl53l7ch);
    const init = stubInit(MODULE_TYPE_MZEVO);
    const opts = { progress: () => {} };
    await dev.init("l7ch", opts);
    expect(init).toHaveBeenCalledWith("l7ch", opts);
  });
});

// ── transfer limits and the 1 Hz floor ───────────────────────────────────────

describe("vl53l7 transfer limits and ranging floor", () => {
  it("register reads split at 1536 bytes", async () => {
    const { dev, fake } = await board(Vl53l7cx);
    const platform = (dev as unknown as { platform: { rdMulti(a: number, n: number): Promise<Uint8Array> } }).platform;
    fake.regs[0x0c00 + 927] = 0xa5;
    const data = await platform.rdMulti(0x0000, 4000);
    expect(data.length).toBe(4000);
    expect(data[3999]).toBe(0xa5);
    const reads = fake.rx.filter((p) => p.cmd === 0x32).map((p) => {
      const dv = new DataView(p.payload.buffer, p.payload.byteOffset);
      return [dv.getUint16(0, true), dv.getUint16(2, true)];
    });
    expect(reads).toEqual([
      [0x0000, 1536],
      [0x0600, 1536],
      [0x0c00, 928],
    ]);
  });

  it("ranging at 1 Hz is allowed, 0 is not", async () => {
    const { dev } = await board(Vl53l7cx);
    const { drv, p } = makeDriver();
    dev.uldDriver = drv;
    await dev.setRangingFrequencyHz(1); // L8 refuses this (contract 04 §4); L5/L7 range at 1 Hz
    expect(await dev.getRangingFrequencyHz()).toBe(1);
    expect(p.dci.size).toBeGreaterThan(0);
    await expect(dev.setRangingFrequencyHz(0)).rejects.toThrow(/>= 1 Hz/);
  });
});

// ── board commands (contract 11 §2) ──────────────────────────────────────────

describe("vl53l7 board commands", () => {
  it("getBridgeInfo decodes the unechoed report", async () => {
    const { dev, fake } = await board(Vl53l7cx);
    expect(await dev.getBridgeInfo()).toEqual({
      intEdges: 0,
      framesDropped: 0,
      i2cErrors: 0,
      lastI2cError: Vl53l7I2cError.Ok,
      lpnLevel: 1,
      intLevel: 0,
      i2cKhz: 1000,
      frameSize: 0,
      streaming: false,
    });
    fake.intEdges = 1;
    fake.frameSize = 1444;
    fake.streaming = true;
    fake.intLevel = 1;
    const info = await dev.getBridgeInfo();
    expect([info.intEdges, info.frameSize, info.streaming, info.intLevel]).toEqual([1, 1444, true, 1]);
  });

  it("setI2cSpeedKhz snaps to the nearest step and returns it", async () => {
    const { dev, fake } = await board(Vl53l7cx);
    expect(await dev.setI2cSpeedKhz(430)).toBe(400);
    expect(await dev.setI2cSpeedKhz(960)).toBe(1000);
    expect(await dev.setI2cSpeedKhz(1)).toBe(100);
    expect(await dev.setI2cSpeedKhz(65535)).toBe(1000);
    expect(fake.speedRequests).toEqual([430, 960, 1, 65535]);
    expect((await dev.getBridgeInfo()).i2cKhz).toBe(1000);
  });

  it.each([0, 65536, -1, 1.5])("setI2cSpeedKhz(%s) is refused before sending", async (khz) => {
    const { dev, fake } = await board(Vl53l7cx);
    await expect(dev.setI2cSpeedKhz(khz)).rejects.toThrow(/khz must be an integer 1\.\.65535/);
    expect(fake.speedRequests).toEqual([]);
  });

  it("setI2cSpeedKhz is busy while streaming", async () => {
    const { dev, fake } = await board(Vl53l7cx);
    fake.streaming = true;
    await expect(dev.setI2cSpeedKhz(400)).rejects.toThrow(BusyError);
    expect(fake.i2cKhz).toBe(1000);
    fake.streaming = false;
    expect(await dev.setI2cSpeedKhz(400)).toBe(400);
  });

  it("LpnOff drops the sensor state and needs init", async () => {
    const { dev, fake } = await board(Vl53l7cx);
    stubInit(MODULE_TYPE_MZEVO);
    await dev.init();
    dev.rangingFlag = true;
    await dev.pinCtrl(PinAction.LpnOff);
    expect(fake.pinActions).toEqual([PinAction.LpnOff]);
    expect(dev.ranging).toBe(false);
    expect(dev.moduleType).toBeNull();
    await expect(dev.getResolution()).rejects.toThrow(/call init/); // nothing goes to the bus
    // LPn low: the sensor NACKs — the bridge reports a hardware fault and counts it
    expect(await dev.isAlive()).toBe(false);
    let info = await dev.getBridgeInfo();
    expect([info.lpnLevel, info.i2cErrors, info.lastI2cError]).toEqual([0, 1, Vl53l7I2cError.Nack]);
    await dev.pinCtrl(PinAction.LpnOn);
    info = await dev.getBridgeInfo();
    expect(info.lpnLevel).toBe(1);
    expect(dev.moduleType).toBeNull(); // LpnOn restores the bus, not the firmware
    await dev.init();
    expect(dev.moduleType).toBe(MODULE_TYPE_MZEVO);
  });

  it("I2cRst keeps the sensor state", async () => {
    const { dev, fake } = await board(Vl53l7cx);
    stubInit(MODULE_TYPE_MZEVO);
    await dev.init();
    await dev.pinCtrl(PinAction.I2cRst);
    expect(fake.pinActions).toEqual([PinAction.I2cRst]);
    expect(dev.moduleType).toBe(MODULE_TYPE_MZEVO);
  });

  it("SoftCycle stops the stream, clears the I2C counters and needs init", async () => {
    const { dev, fake } = await board(Vl53l7cx);
    stubInit(MODULE_TYPE_MZEVO);
    await dev.init();
    dev.rangingFlag = true;
    fake.streaming = true;
    fake.i2cErrors = 3;
    fake.lastI2cError = Vl53l7I2cError.Timeout;
    await dev.pinCtrl(3); // a plain number is accepted
    expect(fake.pinActions).toEqual([PinAction.SoftCycle]);
    expect(fake.streaming).toBe(false);
    expect(dev.ranging).toBe(false);
    expect(dev.moduleType).toBeNull();
    const info = await dev.getBridgeInfo();
    expect([info.streaming, info.lpnLevel, info.i2cErrors, info.lastI2cError]).toEqual([
      false, 1, 0, Vl53l7I2cError.Ok,
    ]);
  });

  it("an unknown pin action is refused before sending", async () => {
    const { dev, fake } = await board(Vl53l7cx);
    await expect(dev.pinCtrl(4 as PinAction)).rejects.toThrow(DepzError);
    await expect(dev.pinCtrl(4 as PinAction)).rejects.toThrow(/unknown pin action 4/);
    expect(fake.pinActions).toEqual([]);
  });
});

// ── ULD plugins: the xtalk flag ───────────────────────────────────────────────

describe("vl53l7 xtalk calibration flag", () => {
  it("follows the driver's XTALK_FAILED result", async () => {
    const { dev } = await board(Vl53l7cx);
    stubInit(MODULE_TYPE_MZEVO);
    await dev.init();
    expect(dev.xtalkCalibrationFailed).toBe(false);
    dev.uldDriver!.xtalkCalibrationFailed = true;
    expect(dev.xtalkCalibrationFailed).toBe(true);
  });
});

// ── the I2C board's register-transfer limits (contract 11 §2) ────────────────

describe("vl53l7 register packers", () => {
  it("READ_REG / WRITE_REG are the VL53L8 payloads under tighter limits", () => {
    expect([...packVl53l7ReadReg(0x0c00, 1536)]).toEqual([...packReadReg(0x0c00, 1536)]);
    expect([...packVl53l7ReadReg(0xfa00, 1536)]).toEqual([0x00, 0xfa, 0x00, 0x06]); // ends exactly at 0x10000
    const data = new Uint8Array(2048).fill(0xa5);
    expect([...packVl53l7WriteReg(0xf800, data)]).toEqual([...packWriteReg(0xf800, data)]);
  });

  it.each([
    [0x0000, 0],
    [0x0000, 1537],
    [0xfa01, 1536], // runs past 0xFFFF
    [0xffff, 2],
  ])("READ_REG addr 0x%s len %i is refused", (addr, len) => {
    expect(() => packVl53l7ReadReg(addr, len)).toThrow(RangeError);
  });

  it.each([
    [0x0000, 0],
    [0x0000, 2049],
    [0xf801, 2048], // runs past 0xFFFF
  ])("WRITE_REG addr 0x%s with %i bytes is refused", (addr, n) => {
    expect(() => packVl53l7WriteReg(addr, new Uint8Array(n))).toThrow(RangeError);
  });

  it("out-of-range fields are refused as such, before the limits", () => {
    expect(() => packVl53l7ReadReg(0x10000, 1)).toThrow(RangeError);
    expect(() => packVl53l7WriteReg(-1, Uint8Array.of(1))).toThrow(RangeError);
  });
});
