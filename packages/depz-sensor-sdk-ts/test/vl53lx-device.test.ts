/**
 * `Vl53lx` (APP_VL53L0_4 bridge, contract 12) driven through an in-process
 * fake bridge with a VL53L1-family register file behind it — no hardware.
 * Wire bytes are checked against vectors/vl53lx.json, decoded frames against
 * its `die_block` / `decode` entries. The full-stack captures
 * (vl53lx-replay, scenarios) cover the init/stream path of each pair; this
 * covers what they cannot: refusals, bridge error statuses, timeouts, the
 * capability-gated passthroughs, calibrations, poll mode, queues and the
 * histogram decode chain.
 */

import { afterEach, describe, expect, it } from "vitest";
import { DepzError, DepzTimeoutError, InvalidStateError, LinkClosedError, StatusError } from "../src/index.js";
import { SensorDriver, measurement, type Measurement } from "../src/sensors/vl53lx/uld/base.js";
import { phaseWindowMm } from "../src/sensors/vl53lx/uld/vl53l1-die.js";
import { NotImplementedError } from "../src/sensors/vl53lx/uld/registry.js";
import {
  PLOTTABLE_STATUSES,
  ProtocolError,
  Vl53Error,
  Vl53l1cx,
  Vl53l4cx,
  Vl53lx,
  XSHUT_OFF,
  XSHUT_RESET,
  plotDistances,
  primaryTarget,
  type Vl53lxMeasurement,
} from "../src/sensors/vl53lx/vl53lx.js";
import { DIE_OSC, FakeVl53lxBridge, hex } from "./fake-vl53lx-bridge.js";

// vectors/vl53lx.json
const L4_BLOCK = hex("090901d3200285008900082ba702700285"); // die_block vl53l4cx_uld_as_l4cd_50ms_f2
const L4_BLOCK_F9 = hex("090908d320027f008a00092bce026f027f"); // ..._f9
const L1_BLOCK = hex("090901439007d9013f000d2234024007d9"); // die_block vl53l1cx_uld_long_33ms_f2
const START_STREAM_DIE = "890011000001860001"; // encode start_stream_die_block
const INFO_WRAPPED = hex("ffffffff0700000003000000020100e80302010c000000"); // decode info_wrapped_counters

const L4CD_BOARD = "DEPZ ToF Sensor VL53L4CD USB v2.1 TESTSERIAL";
const L4CX_BOARD = "DEPZ ToF Sensor VL53L4CX USB v2.1 TOVJALN523";
const L1CX_BOARD = "DEPZ ToF Sensor VL53L1CX USB v2.1 SER";
const L3CX_BOARD = "DEPZ ToF Sensor VL53L3CX USB v2.1 SER";
const UNSTAMPED = "DEPZ ToF Sensor USB v2.1";

const CMD = {
  readReg: 0x32,
  writeReg: 0x33,
  xshut: 0x34,
  startStream: 0x35,
  stopStream: 0x36,
  getInfo: 0x37,
  setI2cSpeed: 0x38,
  setAddrWidth: 0x39,
  clearI2cErrors: 0x3a,
  getDeviceName: 0x03,
};

const toHex = (b: Uint8Array) => Buffer.from(b).toString("hex");

const open: Array<{ dev: Vl53lx; fake: FakeVl53lxBridge }> = [];
afterEach(async () => {
  for (const { dev, fake } of open.splice(0)) {
    await dev.close().catch(() => undefined);
    await fake.close();
  }
});

async function connect(
  board = L4CD_BOARD,
  Cls: typeof Vl53lx = Vl53lx,
  fakeOpts: { noClearI2cErrors?: boolean } = {},
  timeoutMs = 500,
): Promise<{ dev: Vl53lx; fake: FakeVl53lxBridge }> {
  const fake = new FakeVl53lxBridge({ deviceName: board, ...fakeOpts });
  const dev = new Cls(fake.transport, { sleepImpl: () => Promise.resolve(), timeoutMs });
  await dev.open();
  open.push({ dev, fake });
  return { dev, fake };
}

async function ready(board = L4CD_BOARD, product?: string): Promise<{ dev: Vl53lx; fake: FakeVl53lxBridge }> {
  const c = await connect(board);
  await c.dev.init(undefined, { product });
  c.fake.rx.length = 0;
  return c;
}

/** Wait until the read pump has handled everything the fake sent. */
const settle = () => new Promise((r) => setTimeout(r, 20));

// ── pure helpers ──────────────────────────────────────────────────────────────

describe("plottable statuses (contract 12 §4)", () => {
  const t = (distanceMm: number, status: number) => ({
    distanceMm,
    status,
    statusText: "",
    signalKcps: 0,
    ambientKcps: 0,
    sigmaMm: 0,
    minRangeMm: distanceMm,
    maxRangeMm: distanceMm,
  });

  it("are 0, 6 and 11", () => {
    expect([...PLOTTABLE_STATUSES]).toEqual([0, 6, 11]);
  });

  it("plotDistances keeps plottable targets in driver order", () => {
    const m = measurement({ targets: [t(600, 6), t(900, 4), t(1500, 11), t(30, 0)] });
    expect(plotDistances(m)).toEqual([600, 1500, 30]);
    expect(primaryTarget(m)).toEqual(t(600, 6));
  });

  it("on a light driver uses the single distance, or nothing", () => {
    expect(plotDistances(measurement({ distanceMm: 624, status: 0 }))).toEqual([624]);
    expect(plotDistances(measurement({ distanceMm: 624, status: 4 }))).toEqual([]);
    expect(primaryTarget(measurement({ distanceMm: 624 }))).toBeNull();
    expect(primaryTarget(measurement({ targets: [t(1, 7)] }))).toBeNull();
  });
});

// ── identity and init ─────────────────────────────────────────────────────────

describe("vl53lx init", () => {
  it("before init: no driver, no product", async () => {
    const { dev } = await connect();
    expect(dev.initialized).toBe(false);
    expect(dev.product).toBeNull();
    expect(dev.driverKind).toBeNull();
    expect(() => dev.driver).toThrow(InvalidStateError);
    expect(() => dev.supports("offset")).toThrow(InvalidStateError);
    // every Promise-returning method rejects, it never throws at the call
    await expect(dev.getOffsetMm()).rejects.toThrow(/init/);
  });

  it("binds the product from the device name and sets the address width first", async () => {
    const { dev, fake } = await connect();
    await dev.init();
    expect(dev.initialized).toBe(true);
    expect(dev.product).toBe("VL53L4CD");
    expect(dev.driverKind).toBe("uld");
    expect(dev.supports("offset")).toBe(true);
    expect(dev.supports("roi")).toBe(false);
    expect(dev.modes).toEqual([]);

    const cmds = fake.rx.map((p) => p.cmd);
    // device name, then SET_ADDR_WIDTH 2 before the first register access
    expect(cmds[0]).toBe(CMD.getDeviceName);
    expect(cmds[1]).toBe(CMD.setAddrWidth);
    expect(toHex(fake.rx[1]!.payload)).toBe("02");
    expect(cmds.indexOf(CMD.setAddrWidth)).toBeLessThan(cmds.indexOf(CMD.readReg));
    // every init runs at 400 kHz, then the die's 1 MHz ceiling
    expect(fake.sent(CMD.setI2cSpeed).map(toHex)).toEqual(["9001", "e803"]);
    // the blob at 0x2D with byte 0 forced to FM+ (0x12)
    const blob = fake.writes().find(([a]) => a === 0x2d)!;
    expect(blob[1]).toHaveLength(91);
    expect(blob[1][0]).toBe(0x12);
    // VL53L4CD_SensorInit() closes with ALGO__RANGE_IGNORE_THRESHOLD_MCPS
    expect(fake.writes()).toContainEqual([0x0024, [0x05, 0x00]]);
    // CLEAR_I2C_ERRORS closes every init (contract 12 §2)
    expect(cmds[cmds.length - 1]).toBe(CMD.clearI2cErrors);
    expect(fake.sent(CMD.clearI2cErrors)).toHaveLength(1);
    expect(fake.sent(CMD.clearI2cErrors)[0]).toHaveLength(0);
  });

  it("reads the device name once", async () => {
    const { dev, fake } = await connect();
    await dev.detected();
    await dev.init();
    await dev.notes();
    await dev.identifyProduct();
    expect(fake.sent(CMD.getDeviceName)).toHaveLength(1);
    expect(await dev.boardName()).toBe(L4CD_BOARD);
  });

  it("identifyProduct reports both reaches and the model-id cross-check", async () => {
    const { dev } = await ready();
    const info = await dev.identifyProduct();
    expect(info).toMatchObject({
      board: L4CD_BOARD,
      detected: "VL53L4CD",
      product: "VL53L4CD",
      driver: "uld",
      driverClass: "VL53L4",
      kinds: ["uld", "histogram"],
      modelId: 0xebaa,
      modelIdOk: true,
      modes: [],
      reachMm: 1300,
      budgetMs: [10, 200],
      budgetChoices: [],
      histogram: false,
      maxKhz: 1000,
      caveat: null,
    });
    // §5: the 0x38 window at the die's own oscillator, ~1388 mm
    expect(info.driverReachMm).toBe(phaseWindowMm(DIE_OSC, 0x38));
    expect(info.driverReachMm).toBe(1388);
    expect([...info.supports]).toContain("calib_xtalk");
    expect(await dev.notes()).toEqual([]);
  });

  it("a wrong model id fails the cross-check, not the init", async () => {
    const { dev, fake } = await ready();
    fake.setWord(0x010f, 0xeacc);
    const info = await dev.identifyProduct();
    expect(info.modelId).toBe(0xeacc);
    expect(info.modelIdOk).toBe(false);
  });

  it("borrowing a sibling's driver: the board's rating, the pair's reach, a note", async () => {
    const { dev } = await ready(L4CX_BOARD, "VL53L4CD");
    const info = await dev.identifyProduct();
    expect(info.detected).toBe("VL53L4CX");
    expect(info.product).toBe("VL53L4CD");
    expect(info.reachMm).toBe(6000);
    expect(info.driverReachMm).toBe(1388);
    const notes = await dev.notes();
    expect(notes).toHaveLength(2);
    expect(notes[0]).toMatch(/borrowing this driver - the board says it is a VL53L4CX/);
    expect(notes[1]).toMatch(/reaches 1388 mm, the board is rated 6000 mm/);
  });

  it("an unstamped board needs the product named", async () => {
    const { dev, fake } = await connect(UNSTAMPED);
    expect(await dev.detected()).toBeNull();
    expect(await dev.driverKinds()).toEqual([]);
    await expect(dev.init()).rejects.toThrow(InvalidStateError);
    expect(fake.sent(CMD.setAddrWidth)).toHaveLength(0);
    await dev.init(undefined, { product: "VL53L4CD" });
    expect(dev.product).toBe("VL53L4CD");
    expect(await dev.notes()).toEqual(["VL53L4CD: board name carries no product number, named by hand"]);
  });

  it("a pair the table has no row for is refused before any traffic", async () => {
    const { dev, fake } = await connect(L4CX_BOARD, Vl53l4cx);
    expect(await dev.driverKinds()).toEqual(["histogram"]);
    await expect(dev.init("uld")).rejects.toThrow(NotImplementedError);
    await expect(dev.init("uld")).rejects.toThrow(/VL53L4CX has no 'uld' driver - it has histogram/);
    await expect(dev.init(undefined, { product: "VL53L9ZZ" })).rejects.toThrow(/no such product/);
    expect(fake.rx.filter((p) => p.cmd !== CMD.getDeviceName)).toHaveLength(0);
    expect(dev.initialized).toBe(false);
  });

  it("driverKinds of a named product, and of the class's product", async () => {
    const { dev } = await connect(UNSTAMPED, Vl53l1cx);
    expect(await dev.driverKinds()).toEqual(["uld", "histogram"]);
    expect(await dev.driverKinds("VL53L3CX")).toEqual(["ulp", "histogram"]);
  });

  it("a v2.00 bridge (no CLEAR_I2C_ERRORS) is told to reflash", async () => {
    const { dev } = await connect(L4CD_BOARD, Vl53lx, { noClearI2cErrors: true });
    const err = await dev.init().catch((e: unknown) => e);
    expect(err).toBeInstanceOf(DepzError);
    expect(err).not.toBeInstanceOf(ProtocolError);
    expect((err as Error).message).toMatch(/predates APP_VL53L0_4_v0.24.*reflash/);
    expect(dev.initialized).toBe(false);
  });

  it("any other CLEAR_I2C_ERRORS failure is a ProtocolError", async () => {
    const { dev, fake } = await connect();
    fake.failNext(CMD.clearI2cErrors, 0x09);
    await expect(dev.init()).rejects.toThrow(ProtocolError);
  });

  it("a refused SET_ADDR_WIDTH fails init as ProtocolError", async () => {
    const { dev, fake } = await connect();
    fake.failNext(CMD.setAddrWidth, 0x04);
    await expect(dev.init()).rejects.toThrow(ProtocolError);
    expect(dev.initialized).toBe(false);
  });
});

// ── the BridgeDevice the ULD ports talk to ───────────────────────────────────

describe("vl53lx bridge", () => {
  it("reads past 253 bytes in chunks (contract 12 §2 read_reg_max_transfer)", async () => {
    const { dev, fake } = await connect();
    for (let i = 0; i < 300; i++) fake.regs[i] = i & 0xff;
    const data = await dev.bridge.readReg(0, 300);
    expect([...data]).toEqual(Array.from({ length: 300 }, (_, i) => i & 0xff));
    expect(fake.sent(CMD.readReg).map(toHex)).toEqual(["0000fd00", "fd002f00"]);
  });

  it("writes past 253 bytes in chunks", async () => {
    const { dev, fake } = await connect();
    const data = Uint8Array.from({ length: 300 }, (_, i) => (i * 7) & 0xff);
    await dev.bridge.writeReg(0x1000, data);
    expect(fake.writes().map(([a, d]) => [a, d.length])).toEqual([
      [0x1000, 253],
      [0x1000 + 253, 47],
    ]);
    expect([...fake.regs.slice(0x1000, 0x1000 + 300)]).toEqual([...data]);
  });

  it("an error status on a register access surfaces as ProtocolError", async () => {
    const { dev, fake } = await connect();
    fake.failNext(CMD.readReg, 0x09);
    await expect(dev.bridge.readReg(0x010f, 2)).rejects.toThrow(ProtocolError);
    fake.failNext(CMD.writeReg, 0x09);
    await expect(dev.bridge.writeReg(0x0087, Uint8Array.of(0x40))).rejects.toThrow(ProtocolError);
    fake.failNext(CMD.setI2cSpeed, 0x04);
    await expect(dev.bridge.setI2cSpeed(1000)).rejects.toThrow(ProtocolError);
    fake.failNext(CMD.xshut, 0x09);
    await expect(dev.bridge.xshut(XSHUT_RESET)).rejects.toThrow(ProtocolError);
  });

  it("a short register read is a ProtocolError", async () => {
    const { dev, fake } = await connect();
    fake.truncateNextRead = 1; // the bridge answers one byte of two
    await expect(dev.bridge.readReg(0x0089, 2)).rejects.toThrow(ProtocolError);
  });

  it("a read past 0xFFFF is refused before the wire (contract 12 §2 / 10 §1, ERRATA A13)", async () => {
    const { dev, fake } = await connect();
    const reads = fake.sent(CMD.readReg).length;
    await expect(dev.bridge.readReg(0xffff, 2)).rejects.toThrow(RangeError);
    expect(fake.sent(CMD.readReg).length).toBe(reads);
  });

  it("no answer is a ProtocolError too", async () => {
    const { dev, fake } = await connect(L4CD_BOARD, Vl53lx, {}, 30);
    fake.failNext(CMD.setI2cSpeed, "silent");
    await expect(dev.bridge.setI2cSpeed(400)).rejects.toThrow(ProtocolError);
  });

  it("errors that are not bridge statuses pass through unchanged", async () => {
    const { dev } = await connect();
    // packSetI2cSpeed refuses a value wider than u16 before any traffic
    await expect(dev.bridge.setI2cSpeed(70000)).rejects.toThrow(RangeError);
    await expect(dev.bridge.setAddrWidth(3)).rejects.toThrow(RangeError);
  });

  it("bridgeInfo decodes RPT_VL53_INFO (vector info_wrapped_counters)", async () => {
    const { dev, fake } = await connect();
    fake.info = INFO_WRAPPED;
    expect(await dev.bridgeInfo()).toEqual({
      intEdges: 4294967295,
      slotsSkipped: 7,
      i2cErrors: 3,
      lastI2cError: 2,
      xshutLevel: 1,
      intLevel: 0,
      i2cKhz: 1000,
      addrWidth: 2,
      nClear: 1,
      framesDropped: 12,
    });
  });

  it("xshut drops the driver and the stream", async () => {
    const { dev, fake } = await ready();
    await dev.startRanging();
    await dev.xshut(XSHUT_OFF);
    expect(toHex(fake.sent(CMD.xshut)[0]!)).toBe("00");
    expect(dev.ranging).toBe(false);
    expect(dev.initialized).toBe(false);
    expect(() => dev.driver).toThrow(/init/);
  });
});

// ── configuration and the capability-gated passthroughs ──────────────────────

describe("vl53lx configuration (VL53L4CD ULD)", () => {
  it("configure re-inits, then timing, offset, xtalk, signal threshold in that order", async () => {
    const { dev, fake } = await ready();
    await dev.configure({ budgetMs: 33, offsetMm: -10, xtalkKcps: 20, signalKcps: 1024 });
    expect(fake.sent(CMD.clearI2cErrors)).toHaveLength(1);
    const w = fake.writes();
    // vectors/vl53l4.json tuning: offset -10 → 65496, xtalk 20 → 10240, signal 1024 → 128
    expect(w.slice(-7)).toEqual([
      [0x001e, [0xff, 0xd8]],
      [0x0020, [0, 0]],
      [0x0022, [0, 0]],
      [0x0018, [0, 0]],
      [0x001a, [0, 0]],
      [0x0016, [0x28, 0x00]],
      [0x0066, [0x00, 0x80]],
    ]);
    expect(await dev.getOffsetMm()).toBe(-10);
    expect(await dev.getXtalkKcps()).toBe(20);
    expect(await dev.getSignalThresholdKcps()).toBe(1024);
    const [budget, inter] = await dev.getRangeTiming();
    expect(inter).toBe(0);
    expect(Math.abs(budget - 33)).toBeLessThanOrEqual(1);
    expect(await dev.getMode()).toBeNull();
  });

  it("an inter-measurement period selects autonomous mode", async () => {
    const { dev, fake } = await ready();
    await dev.configure({ budgetMs: 50, interMs: 100 });
    expect(fake.dword(0x006c)).toBe(Math.trunc(1.055 * 100 * (0x0123 & 0x3ff)));
    fake.rx.length = 0;
    await dev.startRanging();
    expect(fake.writes()).toEqual([[0x0087, [0x40]]]);
  });

  it("a period not above the budget is refused", async () => {
    const { dev } = await ready();
    await expect(dev.configure({ budgetMs: 50, interMs: 20 })).rejects.toThrow(Vl53Error);
    await expect(dev.configure({ budgetMs: 300 })).rejects.toThrow(/10..200/);
  });

  it("a mode on a product without modes is refused before the re-init", async () => {
    const { dev, fake } = await ready();
    await expect(dev.configure({ mode: "long" })).rejects.toThrow(/VL53L4CD\/uld does not support 'mode'/);
    expect(fake.rx).toHaveLength(0);
  });

  it("snapBudget clamps to the driver's range when any budget goes", async () => {
    const { dev } = await ready();
    expect(dev.budgetChoices()).toEqual([]);
    expect(dev.snapBudget(5)).toBe(10);
    expect(dev.snapBudget(77)).toBe(77);
    expect(dev.snapBudget(500)).toBe(200);
  });

  it("offset, xtalk, thresholds, signal and sigma round-trip through the registers", async () => {
    const { dev, fake } = await ready();
    await dev.setOffsetMm(10);
    expect(fake.word(0x001e)).toBe(40);
    expect(await dev.getOffsetMm()).toBe(10);
    await dev.setXtalkKcps(3);
    expect(fake.word(0x0016)).toBe(1536);
    expect(await dev.getXtalkKcps()).toBe(3);
    await dev.setDetectionThresholds(100, 300, 3);
    expect(fake.regs[0x0046]).toBe(3);
    expect(fake.word(0x0072)).toBe(300);
    expect(fake.word(0x0074)).toBe(100);
    expect(await dev.getDetectionThresholds()).toEqual([100, 300, 3]);
    await dev.setSignalThresholdKcps(2048);
    expect(fake.word(0x0066)).toBe(256);
    expect(await dev.getSignalThresholdKcps()).toBe(2048);
    await dev.setSigmaThresholdMm(15);
    expect(fake.word(0x0064)).toBe(60);
    expect(await dev.getSigmaThresholdMm()).toBe(15);
    await expect(dev.setSigmaThresholdMm(16384)).rejects.toThrow(/16383/);
  });

  it("an unsupported group is refused with no traffic", async () => {
    const { dev, fake } = await ready();
    for (const call of [
      async () => dev.getRoi(),
      () => dev.setRoi(4, 4),
      async () => dev.getRoiCenter(),
      () => dev.setRoiCenter(199),
      () => dev.performRefSpadManagement(),
    ]) {
      await expect(call()).rejects.toThrow(/VL53L4CD\/uld does not support/);
    }
    expect(fake.rx).toHaveLength(0);
  });

  it("the refusal names the pair's caveat, and comes before the re-init", async () => {
    const { dev, fake } = await ready(L3CX_BOARD);
    expect(dev.driverKind).toBe("ulp");
    await expect(dev.configure({ offsetMm: 5 })).rejects.toThrow(
      /VL53L3CX\/ulp does not support 'offset' \(single-target ranging only/,
    );
    await expect(dev.calibrateXtalk(600)).rejects.toThrow(/'calib_xtalk'/);
    expect(fake.rx).toHaveLength(0);
    expect(await dev.notes()).toContainEqual(expect.stringMatching(/^VL53L3CX\/ulp: single-target/));
  });

  it("temperature update re-runs VHV and restores the loop bound", async () => {
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
});

describe("vl53lx calibrations (VL53L4CD_calibration.c)", () => {
  it("offset: target minus the mean distance, programmed ×4", async () => {
    const { dev, fake } = await ready();
    fake.setResultBlock(L4_BLOCK); // 624 mm
    expect(await dev.calibrateOffset(600, 5)).toBe(-24);
    expect(fake.word(0x001e)).toBe(-96 & 0xffff);
    expect(await dev.getOffsetMm()).toBe(-24);
    // offsets zeroed first, 10 heat frames + 5 samples each released
    const w = fake.writes();
    expect(w.slice(0, 3)).toEqual([
      [0x001e, [0, 0]],
      [0x0020, [0, 0]],
      [0x0022, [0, 0]],
    ]);
    expect(w.filter(([a]) => a === 0x0086)).toHaveLength(15);
  });

  it("xtalk: from the valid samples after the first", async () => {
    const { dev, fake } = await ready();
    fake.setResultBlock(L4_BLOCK); // 624 mm, 5160 kcps, 211 SPADs
    expect(await dev.calibrateXtalk(650, 5)).toBe(1);
    // (1 - 624/650) × 5160/211 = 0.978 kcps → ×512
    expect(fake.word(0x0016)).toBe(500);
  });

  it("xtalk fails with no valid sample", async () => {
    const { dev, fake } = await ready();
    const bad = L4_BLOCK.slice();
    bad[0] = 0x04; // raw 4 → status 2, signal below threshold
    fake.setResultBlock(bad);
    await expect(dev.calibrateXtalk(650, 5)).rejects.toThrow(/no valid samples/);
  });

  it("xtalk fails past 127 kcps", async () => {
    const { dev, fake } = await ready();
    const hot = L4_BLOCK.slice();
    hot.set([0x01, 0x00, 0xff, 0xff], 3); // 1 SPAD, 524280 kcps
    fake.setResultBlock(hot);
    await expect(dev.calibrateXtalk(5000, 5)).rejects.toThrow(/> 127/);
  });

  it("arguments out of range are refused before any traffic", async () => {
    const { dev, fake } = await ready();
    await expect(dev.calibrateOffset(600, 4)).rejects.toThrow(/5..255/);
    await expect(dev.calibrateOffset(1001)).rejects.toThrow(/10..1000/);
    await expect(dev.calibrateXtalk(5001)).rejects.toThrow(/10..5000/);
    await expect(dev.calibrateXtalk(600, 256)).rejects.toThrow(Vl53Error);
    expect(fake.rx).toHaveLength(0);
  });
});

describe("vl53lx VL53L1X ULD: modes and tabulated budgets", () => {
  it("modes, budget choices that move with the mode, nearest-budget snap", async () => {
    const { dev } = await ready(L1CX_BOARD);
    expect(dev.modes).toEqual(["long", "short"]);
    expect(await dev.getMode()).toBe("long");
    expect(dev.budgetChoices()).toEqual([20, 33, 50, 100, 200, 500]);
    expect(dev.snapBudget(40)).toBe(33);
    expect(dev.snapBudget(1000)).toBe(500);
    await dev.configure({ mode: "short", budgetMs: 15 });
    expect(await dev.getMode()).toBe("short");
    expect(dev.budgetChoices()[0]).toBe(15);
    expect(await dev.getRangeTiming()).toEqual([15, 15]);
    expect((await dev.identifyProduct()).driverReachMm).toBe(phaseWindowMm(DIE_OSC, 0x38));
  });

  it("a budget the mode does not have is refused, not rounded", async () => {
    const { dev } = await ready(L1CX_BOARD);
    await expect(dev.configure({ budgetMs: 40 })).rejects.toThrow(/pick one of 20, 33, 50, 100, 200, 500/);
    await expect(dev.configure({ mode: "medium" })).rejects.toThrow(/no such mode/);
  });

  it("ROI size and centre", async () => {
    const { dev, fake } = await ready(L1CX_BOARD);
    await dev.setRoi(8, 6);
    expect(fake.regs[0x007f]).toBe(0x91); // the optical centre the die reports
    expect(fake.regs[0x0080]).toBe(0x57);
    expect(await dev.getRoi()).toEqual([8, 6]);
    await dev.setRoi(16, 16); // over 10: the centre moves to 199
    expect(await dev.getRoiCenter()).toBe(199);
    await dev.setRoiCenter(0x88);
    expect(await dev.getRoiCenter()).toBe(0x88);
  });

  it("streams decode with the l1 variant (signal at 0x0098, K = 25)", async () => {
    const { dev, fake } = await ready(L1CX_BOARD);
    await dev.startRanging();
    const got = dev.getMeasurement(1000);
    await fake.stream(L1_BLOCK, 1n);
    const m = await got;
    expect(m).toMatchObject({
      distanceMm: 576,
      status: 0,
      sigmaMm: 3,
      signalKcps: 16072,
      ambientKcps: 2552,
      spads: 67,
    });
    expect(m.extra).toEqual({ signal_per_spad_kcps: 23, ambient_per_spad_kcps: 3, stream_count: 1 });
  });
});

// ── ranging ──────────────────────────────────────────────────────────────────

describe("vl53lx ranging (stream)", () => {
  it("arms the stream with the die block and its interrupt release (vector)", async () => {
    const { dev, fake } = await ready();
    await dev.startRanging();
    expect(fake.writes()).toEqual([[0x0087, [0x21]]]); // continuous
    expect(fake.sent(CMD.startStream).map(toHex)).toEqual([START_STREAM_DIE]);
    expect(dev.ranging).toBe(true);
  });

  it("decodes each streamed block once, to every subscriber", async () => {
    const { dev, fake } = await ready();
    const seen: Vl53lxMeasurement[] = [];
    const unsub = dev.onMeasurement((m) => seen.push(m));
    const q = dev.measurements();
    await dev.startRanging();
    await fake.stream(L4_BLOCK, 123_456n);
    await fake.stream(L4_BLOCK_F9, 123_789n);
    await settle();
    expect(seen.map((m) => m.timestampUs)).toEqual([123_456n, 123_789n]);
    expect(seen[0]).toEqual({
      timestampUs: 123_456n,
      distanceMm: 624,
      status: 0,
      statusText: "valid",
      signalKcps: 5160,
      ambientKcps: 1096,
      sigmaMm: 2,
      spads: 211,
      targets: [],
      extra: { signal_per_spad_kcps: 24, ambient_per_spad_kcps: 5, stream_count: 1 },
      bins: null,
      valid: true,
      plottable: true,
    });
    expect(seen[1]!.distanceMm).toBe(623);
    expect(seen[1]!.extra.stream_count).toBe(8);
    const a = await q.next();
    const b = await q.next();
    expect([a.value!.distanceMm, b.value!.distanceMm]).toEqual([624, 623]);
    unsub();
    await fake.stream(L4_BLOCK, 1n);
    await settle();
    expect(seen).toHaveLength(2);
  });

  it("a non-zero status is not valid; 6 is still plottable", async () => {
    const { dev, fake } = await ready();
    await dev.startRanging();
    const blk = L4_BLOCK.slice();
    blk[0] = 0x13; // raw 19 → status 6
    const got = dev.getMeasurement(1000);
    await fake.stream(blk, 1n);
    const m = await got;
    expect(m.status).toBe(6);
    expect(m.valid).toBe(false);
    expect(m.plottable).toBe(true);
  });

  it("a block the driver cannot decode is counted, and the pump survives", async () => {
    const { dev, fake } = await ready();
    await dev.startRanging();
    await fake.stream(L4_BLOCK.slice(0, 5), 1n);
    await settle();
    expect(dev.streamParseErrors).toBe(1);
    const got = dev.getMeasurement(1000);
    await fake.stream(L4_BLOCK, 2n);
    expect((await got).distanceMm).toBe(624);
  });

  it("malformed stream report is counted and dropped; the link keeps working", async () => {
    // length field says 17, only 5 bytes follow (contract 07 §3 strict decode)
    const { dev, fake } = await ready();
    await dev.startRanging();
    await fake.stream(L4_BLOCK.slice(0, 5), 1n, 0x0089, 17);
    await settle();
    expect(dev.streamParseErrors).toBe(1);
    const got = dev.getMeasurement(1000);
    await fake.stream(L4_BLOCK, 2n);
    expect((await got).distanceMm).toBe(624);
    expect((await dev.bridgeInfo()).addrWidth).toBe(0); // requests still answered
  });

  it("a stream report before init is swallowed, not decoded", async () => {
    const { dev, fake } = await connect();
    const seen: unknown[] = [];
    dev.onMeasurement((m) => seen.push(m));
    await fake.stream(L4_BLOCK, 1n);
    await settle();
    expect(seen).toEqual([]);
    expect(dev.streamParseErrors).toBe(0);
  });

  it("measurements() is bounded, drop-oldest, and deregisters on break", async () => {
    const { dev, fake } = await ready();
    const q = dev.measurements(1);
    expect(dev.streamDroppedCounts).toEqual([0]);
    await dev.startRanging();
    await fake.stream(L4_BLOCK, 1n);
    await fake.stream(L4_BLOCK, 2n);
    await fake.stream(L4_BLOCK_F9, 3n);
    await settle();
    expect(dev.streamDroppedCounts).toEqual([2]);
    for await (const m of q) {
      expect(m.timestampUs).toBe(3n);
      break;
    }
    expect(dev.streamDroppedCounts).toEqual([]);
  });

  it("getMeasurement times out, and ends when the device closes", async () => {
    const { dev } = await ready();
    await expect(dev.getMeasurement(20)).rejects.toThrow(DepzTimeoutError);
    expect(dev.streamDroppedCounts).toEqual([]);
    const pending = dev.getMeasurement(2000);
    await dev.close();
    await expect(pending).rejects.toThrow(LinkClosedError);
    await expect(pending).rejects.toThrow(/device closed/);
  });

  it("configuration is refused while the stream owns the bus", async () => {
    const { dev, fake } = await ready();
    await dev.startRanging();
    fake.rx.length = 0;
    for (const call of [
      () => dev.init(),
      () => dev.configure(),
      () => dev.setOffsetMm(1),
      () => dev.setXtalkKcps(1),
      () => dev.calibrateOffset(100),
      () => dev.calibrateXtalk(100),
      () => dev.setDetectionThresholds(1, 2, 3),
      () => dev.setSignalThresholdKcps(1),
      () => dev.setSigmaThresholdMm(1),
      () => dev.setRoi(4, 4),
      () => dev.setRoiCenter(1),
      () => dev.startTemperatureUpdate(),
      () => dev.performRefSpadManagement(),
      () => dev.startRanging(),
      () => dev.measureOnce(),
    ]) {
      await expect(call()).rejects.toThrow(InvalidStateError);
    }
    expect(fake.rx).toHaveLength(0);
  });

  it("stopRanging stops the stream, then the sensor", async () => {
    const { dev, fake } = await ready();
    await dev.stopRanging(); // not ranging: no-op
    expect(fake.rx).toHaveLength(0);
    await dev.startRanging();
    fake.rx.length = 0;
    await dev.stopRanging();
    expect(fake.rx.map((p) => p.cmd)).toEqual([CMD.stopStream, CMD.writeReg]);
    expect(fake.writes()).toEqual([[0x0087, [0x80]]]);
    expect(dev.ranging).toBe(false);
  });

  it("a refused START_STREAM stops the sensor again and keeps the original error", async () => {
    const { dev, fake } = await ready();
    fake.failNext(CMD.startStream, 0x03);
    const err = await dev.startRanging().catch((e: unknown) => e);
    expect(err).toBeInstanceOf(StatusError);
    expect((err as StatusError).status).toBe(0x03);
    expect(dev.ranging).toBe(false);
    expect(fake.writes().at(-1)).toEqual([0x0087, [0x80]]); // the driver started the die
    await dev.setOffsetMm(0); // configuration allowed again
  });

  it("a failed STOP_STREAM still stops the sensor and clears the flag", async () => {
    const { dev, fake } = await ready();
    await dev.startRanging();
    fake.rx.length = 0;
    fake.failNext(CMD.stopStream, 0x01);
    await expect(dev.stopRanging()).rejects.toThrow(StatusError);
    expect(dev.ranging).toBe(false);
    expect(fake.writes()).toEqual([[0x0087, [0x80]]]);
    await dev.setOffsetMm(0); // configuration allowed again
  });
});

describe("vl53lx measureOnce (poll)", () => {
  it("starts, waits for data-ready, reads the block, releases, stops", async () => {
    const { dev, fake } = await ready();
    fake.setResultBlock(L4_BLOCK);
    const m = await dev.measureOnce();
    expect(m.distanceMm).toBe(624);
    expect(m.valid).toBe(true);
    expect(m.bins).toBeNull();
    // the timestamp of the last register read: the block itself
    expect(m.timestampUs).toBe(fake.mcuTimeUs - 100n);
    expect(fake.sent(CMD.readReg).map(toHex)).toContain("89001100");
    const w = fake.writes();
    expect(w[0]).toEqual([0x0087, [0x21]]);
    expect(w.slice(-2)).toEqual([
      [0x0086, [0x01]],
      [0x0087, [0x80]],
    ]);
    expect(fake.sent(CMD.startStream)).toHaveLength(0);
  });

  it("a data-ready timeout still stops the sensor", async () => {
    const { dev, fake } = await ready();
    fake.regs[0x0031] = 0x01; // INT never reaches the active level
    await expect(dev.measureOnce(30)).rejects.toThrow(/timeout waiting for data ready/);
    expect(fake.writes().slice(-1)).toEqual([[0x0087, [0x80]]]);
    expect(dev.ranging).toBe(false);
  });
});

// ── the histogram (Bare Driver) decode path, with a stub driver ──────────────

/** A histogram driver whose binData/toMeasurement the test controls. */
class StubHistogram extends SensorDriver {
  static override readonly HISTOGRAM = true;
  static override readonly CLEAR_STEPS = [[0x0086, 0x01]] as const;
  binCalls: Uint8Array[] = [];
  bin: (raw: Uint8Array) => unknown = (raw) => ({ first: raw[0] });
  async modelId() {
    return 0xebaa;
  }
  async sensorInit() {}
  async startRanging() {}
  async stopRanging() {}
  async checkForDataReady() {
    return true;
  }
  async clearInterrupt() {}
  streamBlock(): [number, number] {
    return [0x0088, 83];
  }
  decode(): Measurement {
    throw new Error("the histogram path must not call decode()");
  }
  async setRangeTiming() {}
  async getRangeTiming(): Promise<[number, number]> {
    return [33, 0];
  }
  binData(raw: Uint8Array): unknown {
    this.binCalls.push(raw);
    return this.bin(raw);
  }
  toMeasurement(bins: unknown): Measurement {
    const d = (bins as { first: number }).first;
    const target = {
      distanceMm: d,
      status: 0,
      statusText: "valid",
      signalKcps: 1,
      ambientKcps: 0,
      sigmaMm: 1,
      minRangeMm: d,
      maxRangeMm: d,
    };
    return measurement({ distanceMm: d, targets: [target, { ...target, distanceMm: d * 2, status: 4 }] });
  }
}

async function withStub(): Promise<{ dev: Vl53lx; fake: FakeVl53lxBridge; stub: StubHistogram }> {
  const { dev, fake } = await connect(L4CX_BOARD);
  const inner = dev as unknown as { driverInst: SensorDriver; platform: never };
  const stub = new StubHistogram(inner.platform, "VL53L4CX");
  inner.driverInst = stub;
  return { dev, fake, stub };
}

describe("vl53lx histogram decode chain", () => {
  it("each frame steps binData exactly once and carries its bins", async () => {
    const { dev, fake, stub } = await withStub();
    const seen: Vl53lxMeasurement[] = [];
    dev.onMeasurement((m) => seen.push(m));
    await dev.startRanging();
    expect(fake.sent(CMD.startStream).map(toHex)).toEqual(["880053000001860001"]); // start_stream_histogram
    await fake.stream(Uint8Array.of(7, 0, 0), 1n);
    await settle();
    expect(stub.binCalls).toHaveLength(1);
    expect(seen).toHaveLength(1);
    expect(seen[0]!.bins).toEqual({ first: 7 });
    expect(seen[0]!.targets.map((t) => t.distanceMm)).toEqual([7, 14]);
    expect(plotDistances(seen[0]!)).toEqual([7]);
  });

  it("an asynchronous binData keeps frame order", async () => {
    const { dev, fake, stub } = await withStub();
    const resolvers: Array<() => void> = [];
    stub.bin = (raw) =>
      new Promise((resolve) => {
        resolvers.push(() => resolve({ first: raw[0] }));
      });
    const seen: number[] = [];
    dev.onMeasurement((m) => seen.push(m.distanceMm));
    await dev.startRanging();
    await fake.stream(Uint8Array.of(1), 1n);
    await fake.stream(Uint8Array.of(2), 2n);
    await settle();
    expect(resolvers).toHaveLength(2);
    resolvers[1]!(); // the second frame's bins are ready first
    await settle();
    expect(seen).toEqual([]);
    resolvers[0]!();
    await settle();
    expect(seen).toEqual([1, 2]);
  });

  it("a failing decode (sync or async) is counted, not thrown into the pump", async () => {
    const { dev, fake, stub } = await withStub();
    await dev.startRanging();
    stub.bin = () => {
      throw new Error("bad frame");
    };
    await fake.stream(Uint8Array.of(1), 1n);
    stub.bin = () => Promise.reject(new Error("bad frame"));
    await fake.stream(Uint8Array.of(2), 2n);
    await settle();
    expect(dev.streamParseErrors).toBe(2);
    stub.bin = (raw) => ({ first: raw[0] });
    const got = dev.getMeasurement(1000);
    await fake.stream(Uint8Array.of(3), 3n);
    expect((await got).distanceMm).toBe(3);
  });

  it("measureOnce reads the 83-byte histogram block through binData", async () => {
    const { dev, fake, stub } = await withStub();
    fake.regs[0x0088] = 42;
    const m = await dev.measureOnce();
    expect(fake.sent(CMD.readReg).map(toHex)).toEqual(["88005300"]);
    expect(stub.binCalls).toHaveLength(1);
    expect(stub.binCalls[0]).toHaveLength(83);
    expect(m.bins).toEqual({ first: 42 });
    expect(m.distanceMm).toBe(42);
  });
});
