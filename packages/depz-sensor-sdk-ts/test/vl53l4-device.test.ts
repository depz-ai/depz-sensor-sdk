/**
 * `Vl53l4cd` (APP_VL53L4 bridge, contract 10, protocol v1.00) driven through
 * the in-process fake bridge (test/fake-vl53lx-bridge.ts, firmware "l4") with
 * a VL53L4CD register file behind it — no hardware. Wire bytes against
 * vectors/vl53l4.json (`encode`, `tuning`), decoded frames against its
 * `result_block` / `decode` entries.
 */

import { afterEach, describe, expect, it } from "vitest";
import {
  DepzTimeoutError,
  InvalidStateError,
  LinkClosedError,
  StatusError,
  Vl53l4cd,
  Vl53l4cdError,
  VL53L4_XSHUT_OFF,
  RESULT_BLOCK_LEN,
  parseResultBlock,
  type Vl53l4Measurement,
} from "../src/index.js";
import { FakeVl53lxBridge, hex } from "./fake-vl53lx-bridge.js";

// vectors/vl53l4.json
const VALID_MID = hex("0900070f0001f4006400280000012300ff"); // result_block valid_mid_range
const WRAPPED = hex("070005000000c8000a00100000000456ee"); // result_block wrapped_target_zero_spads
const INFO_BOOTED = hex("40e20100020000000500000001aaeb03010100e803"); // decode info_booted_1mhz
const L4_BLOCK = hex("090901d3200285008900082ba702700285"); // vl53lx die_block, 624 mm

const CMD = {
  readReg: 0x32,
  writeReg: 0x33,
  xshut: 0x34,
  startStream: 0x35,
  stopStream: 0x36,
  setI2cSpeed: 0x38,
};

const toHex = (b: Uint8Array) => Buffer.from(b).toString("hex");
const settle = () => new Promise((r) => setTimeout(r, 20));

const open: Array<{ dev: Vl53l4cd; fake: FakeVl53lxBridge }> = [];
afterEach(async () => {
  for (const { dev, fake } of open.splice(0)) {
    await dev.close().catch(() => undefined);
    await fake.close();
  }
});

async function connect(timeoutMs = 500): Promise<{ dev: Vl53l4cd; fake: FakeVl53lxBridge }> {
  const fake = new FakeVl53lxBridge({ firmware: "l4" });
  const dev = new Vl53l4cd(fake.transport, { sleepImpl: () => Promise.resolve(), timeoutMs });
  await dev.open();
  open.push({ dev, fake });
  return { dev, fake };
}

async function ready(): Promise<{ dev: Vl53l4cd; fake: FakeVl53lxBridge }> {
  const c = await connect();
  await c.dev.init();
  c.fake.rx.length = 0;
  return c;
}

describe("vl53l4cd init and identity", () => {
  it("init runs the ULD sequence at 400 kHz, then re-times the bus", async () => {
    const { dev, fake } = await connect();
    expect(dev.initialized).toBe(false);
    await dev.init();
    expect(dev.initialized).toBe(true);
    expect(fake.rx[0]!.cmd).toBe(CMD.setI2cSpeed);
    expect(fake.sent(CMD.setI2cSpeed).map(toHex)).toEqual(["9001", "e803"]);
    // boot status polled before the blob
    expect(toHex(fake.sent(CMD.readReg)[0]!)).toBe("e5000100");
    const w = fake.writes();
    expect(w[0]![0]).toBe(0x002d);
    expect(w[0]![1]).toHaveLength(91);
    expect(w[0]![1][0]).toBe(0x12);
    expect(w).toContainEqual([0x0087, [0x40]]); // VHV start
    expect(w).toContainEqual([0x0024, [0x05, 0x00]]);
    // v1.00 has no CLEAR_I2C_ERRORS / SET_ADDR_WIDTH
    expect(fake.rx.some((p) => p.cmd === 0x39 || p.cmd === 0x3a)).toBe(false);
    expect(await dev.getRangeTiming()).toEqual({ timingBudgetMs: 49, interMeasurementMs: 0 });
  });

  it("init at 400 kHz stays there", async () => {
    const { dev, fake } = await connect();
    await dev.init(400);
    expect(fake.sent(CMD.setI2cSpeed).map(toHex)).toEqual(["9001"]);
  });

  it("init fails when the die never boots", async () => {
    const { dev, fake } = await connect();
    fake.regs[0x00e5] = 0x00;
    await expect(dev.init()).rejects.toThrow(Vl53l4cdError);
    expect(dev.initialized).toBe(false);
  });

  it("isAlive checks the model id, and is false on a bridge error", async () => {
    const { dev, fake } = await connect();
    expect(await dev.isAlive()).toBe(true);
    expect(toHex(fake.sent(CMD.readReg)[0]!)).toBe("0f010200"); // read_reg_model_id_word
    fake.setWord(0x010f, 0xeacc);
    expect(await dev.isAlive()).toBe(false);
    fake.failNext(CMD.readReg, 0x09);
    expect(await dev.isAlive()).toBe(false);
  });

  it("isAlive does not swallow programming errors", async () => {
    const { dev } = await connect();
    (dev.uld as unknown as { isAlive: () => Promise<boolean> }).isAlive = () =>
      Promise.reject(new TypeError("bug"));
    await expect(dev.isAlive()).rejects.toThrow(TypeError);
  });

  it("bridgeInfo decodes the 21-byte v1.00 report (vector info_booted_1mhz)", async () => {
    const { dev, fake } = await connect();
    fake.info = INFO_BOOTED;
    expect(await dev.bridgeInfo()).toEqual({
      intEdges: 123456,
      slotsSkipped: 2,
      i2cErrors: 5,
      lastI2cError: 1,
      modelId: 60330,
      fwStatus: 3,
      initialized: 1,
      xshutLevel: 1,
      intLevel: 0,
      i2cKhz: 1000,
    });
  });

  it("xshut and resetSensor drop the init and the stream", async () => {
    const { dev, fake } = await ready();
    await dev.startRanging();
    await dev.xshut(VL53L4_XSHUT_OFF);
    expect(dev.ranging).toBe(false);
    expect(dev.initialized).toBe(false);
    await dev.init();
    await dev.resetSensor();
    expect(fake.sent(CMD.xshut).map(toHex)).toEqual(["00", "02"]); // xshut_off, xshut_reset
    expect(dev.initialized).toBe(false);
  });

  it("setI2cSpeedKhz re-times the bus", async () => {
    const { dev, fake } = await ready();
    await dev.setI2cSpeedKhz(400);
    expect(fake.sent(CMD.setI2cSpeed).map(toHex)).toEqual(["9001"]);
    fake.failNext(CMD.setI2cSpeed, 0x06);
    await expect(dev.setI2cSpeedKhz(1000)).rejects.toThrow(StatusError);
  });
});

describe("vl53l4cd result block (contract 10 §5)", () => {
  it("is 17 bytes: 15 and 16 are short (ERRATA A8), a tail is ignored", () => {
    expect(RESULT_BLOCK_LEN).toBe(17);
    for (const n of [0, 14, 15, 16]) {
      expect(() => parseResultBlock(new Uint8Array(n))).toThrow(Vl53l4cdError);
      expect(() => parseResultBlock(new Uint8Array(n))).toThrow(/too short/);
    }
    const block = new Uint8Array(17);
    expect(parseResultBlock(new Uint8Array([...block, 0xff, 0xff]))).toEqual(parseResultBlock(block));
  });
});

describe("vl53l4cd configuration", () => {
  it("timing: continuous and autonomous", async () => {
    const { dev, fake } = await ready();
    await dev.setRangeTiming(50, 100);
    expect(fake.dword(0x006c)).toBe(Math.floor(1.055 * 100 * (0x0123 & 0x3ff)));
    fake.rx.length = 0;
    await dev.startRanging();
    expect(fake.writes()).toEqual([[0x0087, [0x40]]]);
    await dev.stopRanging();
    await dev.setRangeTiming(33);
    expect(fake.dword(0x006c)).toBe(0);
    expect((await dev.getRangeTiming()).interMeasurementMs).toBe(0);
    // ERRATA A16: a refused argument is invalid_argument (RangeError), not the
    // driver's Vl53l4cdError, and is refused before any traffic, reads included
    fake.rx.length = 0;
    await expect(dev.setRangeTiming(5)).rejects.toThrow(RangeError);
    await expect(dev.setRangeTiming(50, 30)).rejects.toThrow(RangeError);
    await expect(dev.setRangeTiming(50, 50)).rejects.toThrow(RangeError);
    await expect(dev.calibrateOffset(5)).rejects.toThrow(RangeError);
    await expect(dev.calibrateXtalk(100, 4)).rejects.toThrow(RangeError);
    expect(fake.rx).toEqual([]);
  });

  it("offset, xtalk, thresholds, signal and sigma (vectors tuning)", async () => {
    const { dev, fake } = await ready();
    await dev.setOffsetMm(-10);
    expect(fake.word(0x001e)).toBe(65496);
    expect(fake.word(0x0020)).toBe(0);
    expect(await dev.getOffsetMm()).toBe(-10);
    await dev.setXtalkKcps(20);
    expect(fake.word(0x0016)).toBe(10240);
    expect(await dev.getXtalkKcps()).toBe(20);
    await dev.setDetectionThresholds(100, 300, 2);
    expect(fake.writes().slice(-3)).toEqual([
      [0x0046, [2]],
      [0x0072, [0x01, 0x2c]],
      [0x0074, [0x00, 0x64]],
    ]);
    expect(await dev.getDetectionThresholds()).toEqual({
      distanceLowMm: 100,
      distanceHighMm: 300,
      window: 2,
    });
    await dev.setSignalThresholdKcps(1024);
    expect(fake.word(0x0066)).toBe(128);
    expect(await dev.getSignalThresholdKcps()).toBe(1024);
    await dev.setSigmaThresholdMm(15);
    expect(fake.word(0x0064)).toBe(60);
    expect(await dev.getSigmaThresholdMm()).toBe(15);
    await expect(dev.setSigmaThresholdMm(16384)).rejects.toThrow(RangeError);
    await expect(dev.setXtalkKcps(128)).rejects.toThrow(RangeError); // tuning_xtalk_too_wide
  });

  it("temperature update", async () => {
    const { dev, fake } = await ready();
    await dev.startTemperatureUpdate();
    expect(fake.writes()).toEqual([
      [0x0008, [0x81]],
      [0x000b, [0x92]],
      [0x0087, [0x21]],
      [0x0086, [0x01]],
      [0x0087, [0x80]],
      [0x0008, [0x09]],
      [0x000b, [0x00]],
    ]);
  });

  it("calibrations program what they return", async () => {
    const { dev, fake } = await ready();
    fake.setResultBlock(L4_BLOCK); // 624 mm, 5160 kcps, 211 SPADs
    expect(await dev.calibrateOffset(600, 5)).toBe(-24);
    expect(await dev.getOffsetMm()).toBe(-24);
    expect(await dev.calibrateXtalk(650, 5)).toBe(1);
    expect(fake.word(0x0016)).toBe(500);
  });

  it("configuration is refused while ranging", async () => {
    const { dev, fake } = await ready();
    await dev.startRanging();
    fake.rx.length = 0;
    for (const call of [
      () => dev.init(),
      () => dev.setI2cSpeedKhz(400),
      () => dev.setRangeTiming(50),
      () => dev.setOffsetMm(1),
      () => dev.setXtalkKcps(1),
      () => dev.setDetectionThresholds(1, 2, 3),
      () => dev.setSignalThresholdKcps(1),
      () => dev.setSigmaThresholdMm(1),
      () => dev.startTemperatureUpdate(),
      () => dev.calibrateOffset(100),
      () => dev.calibrateXtalk(100),
      () => dev.startRanging(),
      () => dev.measureOnce(),
    ]) {
      await expect(call()).rejects.toThrow(InvalidStateError);
    }
    expect(fake.rx).toHaveLength(0);
  });
});

describe("vl53l4cd ranging", () => {
  it("arms a 5-byte START_STREAM on the result block", async () => {
    const { dev, fake } = await ready();
    await dev.startRanging();
    expect(fake.writes()).toEqual([[0x0087, [0x21]]]); // write_reg_system_start_continuous
    expect(fake.sent(CMD.startStream).map(toHex)).toEqual(["8900110000"]);
    expect(dev.ranging).toBe(true);
  });

  it("a START_STREAM timeout stops the sensor again and keeps the timeout", async () => {
    const { dev, fake } = await connect(200);
    await dev.init();
    fake.rx.length = 0;
    fake.failNext(CMD.startStream, "silent");
    await expect(dev.startRanging()).rejects.toThrow(DepzTimeoutError);
    expect(dev.ranging).toBe(false);
    expect(fake.writes()).toEqual([
      [0x0087, [0x21]],
      [0x0087, [0x80]], // the ULD started the die, so it is stopped again
    ]);
    await dev.setOffsetMm(3); // not wedged: reconfiguration works
    await dev.startRanging(); // and a retry starts cleanly
    expect(dev.ranging).toBe(true);
  });

  it("decodes streamed blocks (vector result_block)", async () => {
    const { dev, fake } = await ready();
    const seen: Vl53l4Measurement[] = [];
    const unsub = dev.onMeasurement((m) => seen.push(m));
    const q = dev.measurements();
    await dev.startRanging();
    await fake.stream(VALID_MID, 555_666_777n);
    await fake.stream(WRAPPED, 555_700_000n);
    await settle();
    expect(seen[0]).toEqual({
      timestampUs: 555_666_777n,
      rangeStatus: 0,
      distanceMm: 291,
      sigmaMm: 10,
      signalRateKcps: 4000,
      ambientRateKcps: 800,
      signalPerSpadKcps: 266,
      ambientPerSpadKcps: 53,
      numberOfSpad: 15,
      streamCount: 7,
      valid: true,
      statusText: "valid",
    });
    expect(seen[1]).toMatchObject({ rangeStatus: 7, distanceMm: 4, numberOfSpad: 0, valid: false });
    expect((await q.next()).value!.distanceMm).toBe(291);
    unsub();
    await fake.stream(VALID_MID, 1n);
    await settle();
    expect(seen).toHaveLength(2);
  });

  it("a short block is counted, a short report is not ours", async () => {
    const { dev, fake } = await ready();
    await dev.startRanging();
    await fake.stream(VALID_MID.slice(0, 10), 1n);
    await fake.sendRaw(0x93, new Uint8Array(4));
    await settle();
    expect(dev.streamParseErrors).toBe(1);
    const got = dev.getMeasurement(1000);
    await fake.stream(VALID_MID, 2n);
    expect((await got).distanceMm).toBe(291);
  });

  it("malformed stream report is counted and dropped; the link keeps working", async () => {
    // length field says 17, only 5 bytes follow (contract 07 §3 strict decode)
    const { dev, fake } = await ready();
    await dev.startRanging();
    await fake.stream(VALID_MID.slice(0, 5), 1n, 0x0089, 17);
    await settle();
    expect(dev.streamParseErrors).toBe(1);
    const got = dev.getMeasurement(1000);
    await fake.stream(VALID_MID, 2n);
    expect((await got).distanceMm).toBe(291);
    expect(await dev.isAlive()).toBe(true); // requests still answered
  });

  it("measurements() drops the oldest and deregisters on break", async () => {
    const { dev, fake } = await ready();
    const q = dev.measurements(1);
    await dev.startRanging();
    await fake.stream(VALID_MID, 1n);
    await fake.stream(WRAPPED, 2n);
    await settle();
    expect(dev.streamDroppedCounts).toEqual([1]);
    for await (const m of q) {
      expect(m.timestampUs).toBe(2n);
      break;
    }
    expect(dev.streamDroppedCounts).toEqual([]);
  });

  it("getMeasurement times out, and ends when the device closes", async () => {
    const { dev } = await ready();
    await expect(dev.getMeasurement(20)).rejects.toThrow(DepzTimeoutError);
    const pending = dev.getMeasurement(2000);
    await dev.close();
    await expect(pending).rejects.toThrow(LinkClosedError);
    await expect(pending).rejects.toThrow(/device closed/);
  });

  it("stopRanging: STOP_STREAM, then the sensor; a failed ack still stops", async () => {
    const { dev, fake } = await ready();
    await dev.stopRanging(); // no-op
    expect(fake.rx).toHaveLength(0);
    await dev.startRanging();
    fake.rx.length = 0;
    await dev.stopRanging();
    expect(fake.rx.map((p) => p.cmd)).toEqual([CMD.stopStream, CMD.writeReg]);
    expect(fake.writes()).toEqual([[0x0087, [0x80]]]);
    await dev.startRanging();
    fake.rx.length = 0;
    fake.failNext(CMD.stopStream, 0x01);
    await expect(dev.stopRanging()).rejects.toThrow(StatusError);
    expect(dev.ranging).toBe(false);
    expect(fake.writes()).toEqual([[0x0087, [0x80]]]);
  });

  it("measureOnce polls one block and stamps it with the read's time", async () => {
    const { dev, fake } = await ready();
    fake.setResultBlock(VALID_MID);
    const m = await dev.measureOnce();
    expect(m.distanceMm).toBe(291);
    expect(m.timestampUs).toBe(fake.mcuTimeUs - 100n);
    expect(fake.sent(CMD.readReg).map(toHex)).toContain("89001100"); // read_reg_result_block
    expect(fake.writes().slice(-2)).toEqual([
      [0x0086, [0x01]], // write_reg_interrupt_clear
      [0x0087, [0x80]],
    ]);
  });

  it("a data-ready timeout still stops the sensor", async () => {
    const { dev, fake } = await ready();
    fake.regs[0x0031] = 0x01;
    await expect(dev.measureOnce(5)).rejects.toThrow(/data ready/);
    expect(fake.writes().slice(-1)).toEqual([[0x0087, [0x80]]]);
  });
});
