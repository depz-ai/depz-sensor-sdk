/**
 * VL53L1X ULD 3.5.5 port (uld/l1.ts) over the fake bridge, no hardware: what
 * the full-stack captures (scenarios) and the family tests (vl53lx-device)
 * leave out — the mode change that invalidates a budget, the unreadable mode
 * byte, the threshold read-modify-write, and the two calibrations with their
 * refusals. Numbers follow VL53L1X_calibration.c on the
 * `vl53l1cx_uld_long_33ms_f2` die block of vectors/vl53lx.json. Mirror of
 * the Python `tests/test_vl53l1_uld.py`.
 */

import { afterEach, describe, expect, it } from "vitest";
import { VL53L1 } from "../src/sensors/vl53lx/uld/l1.js";
import { Vl53lx } from "../src/sensors/vl53lx/vl53lx.js";
import { FakeVl53lxBridge, hex } from "./fake-vl53lx-bridge.js";

const L1CX_BOARD = "DEPZ ToF Sensor VL53L1CX USB v2.1 SER";
// die_block vl53l1cx_uld_long_33ms_f2: 576 mm, 16072 kcps (0x0098 × 8), 67 SPADs
const L1_BLOCK = hex("090901439007d9013f000d2234024007d9");

const SYSTEM__INTERRUPT_CONFIG_GPIO = 0x0046;
const PHASECAL_CONFIG__TIMEOUT_MACROP = 0x004b;
const XTALK_PLANE_OFFSET_KCPS = 0x0016;
const RANGE_OFFSET_MM = 0x001e;

const open: Array<{ dev: Vl53lx; fake: FakeVl53lxBridge }> = [];
afterEach(async () => {
  for (const { dev, fake } of open.splice(0)) {
    await dev.close().catch(() => undefined);
    await fake.close();
  }
});

async function l1(): Promise<{ dev: Vl53lx; fake: FakeVl53lxBridge; drv: VL53L1 }> {
  const fake = new FakeVl53lxBridge({ deviceName: L1CX_BOARD });
  const dev = new Vl53lx(fake.transport, { sleepImpl: () => Promise.resolve(), timeoutMs: 500 });
  await dev.open();
  open.push({ dev, fake });
  await dev.init();
  fake.rx.length = 0;
  return { dev, fake, drv: dev.driver as VL53L1 };
}

// ── distance mode ────────────────────────────────────────────────────────────

describe("VL53L1X distance mode", () => {
  it("a mode change moves an invalid budget to the nearest one", async () => {
    // 15 ms exists in short mode only: going back to long keeps the period
    // and takes the nearest budget long has, 20 ms (VL53L1X_SetDistanceMode
    // re-applies the budget; the C driver would fail on the missing row).
    // setMode returns it, so the move is never silent (contract 12 §5).
    const { dev, fake, drv } = await l1();
    await dev.configure({ mode: "short", budgetMs: 15 });
    expect(await dev.getRangeTiming()).toEqual([15, 15]);
    fake.rx.length = 0;
    expect(await drv.setMode("long")).toBe(20);
    expect(await dev.getMode()).toBe("long");
    expect(dev.budgetChoices()).toEqual([20, 33, 50, 100, 200, 500]);
    expect(await dev.getRangeTiming()).toEqual([20, 15]);
    // the long-mode VCSEL/phase registers, then the 20 ms MACROP pair
    const w = fake.writes();
    expect(w.slice(0, 6)).toEqual([
      [0x004b, [0x0a]],
      [0x0060, [0x0f]],
      [0x0063, [0x0d]],
      [0x0069, [0xb8]],
      [0x0078, [0x0f, 0x0d]],
      [0x007a, [0x0e, 0x0e]],
    ]);
    expect(w.slice(6, 8)).toEqual([
      [0x005e, [0x00, 0x1e]],
      [0x0061, [0x00, 0x22]],
    ]);
  });

  it("a mode byte that is neither mode is an error, not a guess", async () => {
    const { dev, fake, drv } = await l1();
    fake.regs[PHASECAL_CONFIG__TIMEOUT_MACROP] = 0x00; // a part fresh out of reset
    await expect(dev.getMode()).rejects.toThrow(/reads 0x00, which is neither distance mode/);
    // the budget table depends on the mode, so timing cannot be set either
    await expect(drv.setRangeTiming(50, 0)).rejects.toThrow(/neither distance mode/);
  });

  it("an unknown mode is refused before any traffic", async () => {
    const { fake, drv } = await l1();
    await expect(drv.setMode("medium")).rejects.toThrow(/no such mode: medium \(have long, short\)/);
    expect(fake.rx).toEqual([]);
  });
});

// ── detection thresholds (VL53L1X_SetDistanceThreshold) ──────────────────────

describe("VL53L1X detection thresholds", () => {
  it("rewrite the window bits and keep the others", async () => {
    const { dev, fake } = await l1();
    expect(fake.regs[SYSTEM__INTERRUPT_CONFIG_GPIO]).toBe(0x20); // the blob's byte
    fake.regs[SYSTEM__INTERRUPT_CONFIG_GPIO] = 0x90; // bits outside 0x6F survive
    await dev.setDetectionThresholds(100, 300, 3);
    expect(fake.writes()).toEqual([
      [SYSTEM__INTERRUPT_CONFIG_GPIO, [0x93]],
      [0x0072, [0x01, 0x2c]], // THRESH_HIGH
      [0x0074, [0x00, 0x64]], // THRESH_LOW
    ]);
    expect(await dev.getDetectionThresholds()).toEqual([100, 300, 3]);
  });

  it("a new window replaces the old one and int-on-no-target is bit 6", async () => {
    // Unlike the C driver's `Temp & 0x47`, which keeps the previous window
    // bits and the no-target flag, this port clears them: a window set after
    // another one is the one that was asked for, and getDetectionThresholds()
    // reports it.
    const { dev, fake, drv } = await l1();
    await drv.setDetectionThresholds(100, 300, 3);
    await drv.setDetectionThresholds(100, 300, 1, 1);
    expect(fake.regs[SYSTEM__INTERRUPT_CONFIG_GPIO]).toBe(0x41);
    await drv.setDetectionThresholds(100, 300, 0);
    expect(fake.regs[SYSTEM__INTERRUPT_CONFIG_GPIO]).toBe(0x00);
    expect(await dev.getDetectionThresholds()).toEqual([100, 300, 0]);
  });
});

// ── calibrations (VL53L1X_calibration.c) ─────────────────────────────────────

describe("VL53L1X calibrations", () => {
  it("offset is target minus the mean, programmed ×4", async () => {
    const { dev, fake } = await l1();
    fake.setResultBlock(L1_BLOCK); // 576 mm
    expect(await dev.calibrateOffset(600, 5)).toBe(24);
    expect(fake.word(RANGE_OFFSET_MM)).toBe(96);
    expect(await dev.getOffsetMm()).toBe(24);
    // the three offsets zeroed, timed start (0x40), 5 frames each released,
    // the L1X stop code (0x00), then the result
    expect(fake.writes()).toEqual([
      [0x001e, [0, 0]],
      [0x0020, [0, 0]],
      [0x0022, [0, 0]],
      [0x0087, [0x40]],
      ...Array(5).fill([0x0086, [0x01]]),
      [0x0087, [0x00]],
      [0x001e, [0x00, 0x60]],
    ]);
  });

  it("offset can come out negative", async () => {
    const { dev, fake } = await l1();
    fake.setResultBlock(L1_BLOCK); // 576 mm
    expect(await dev.calibrateOffset(500, 5)).toBe(-76);
    expect(fake.word(RANGE_OFFSET_MM)).toBe((-76 * 4) & 0xffff);
    expect(await dev.getOffsetMm()).toBe(-76);
  });

  it("xtalk is 512 × signal × (1 − d/target) / spads", async () => {
    const { dev, fake } = await l1();
    fake.setResultBlock(L1_BLOCK); // 576 mm, 16072 kcps, 67 SPADs
    // 512 × 16072 × (1 − 576/650) / 67 = 13982.6 → 13982 in the 7.9 register
    expect(await dev.calibrateXtalk(650, 5)).toBe(27); // kcps, rounded half away
    expect(fake.word(XTALK_PLANE_OFFSET_KCPS)).toBe(13982);
    expect(await dev.getXtalkKcps()).toBe(27);
    // compensation disabled first, 5 frames, then the result
    const w = fake.writes();
    expect(w[0]).toEqual([XTALK_PLANE_OFFSET_KCPS, [0, 0]]);
    expect(w[1]).toEqual([0x0087, [0x40]]);
    expect(w.filter(([a]) => a === 0x0086)).toHaveLength(5);
    expect(w.slice(-2)).toEqual([
      [0x0087, [0x00]],
      [XTALK_PLANE_OFFSET_KCPS, [0x36, 0x9e]],
    ]);
  });

  it("xtalk clamps a target nearer than the reading to zero", async () => {
    // (1 − 576/500) is negative; the C driver's uint16 cast is undefined
    // there, this port writes 0 (no compensation).
    const { dev, fake } = await l1();
    fake.setResultBlock(L1_BLOCK);
    expect(await dev.calibrateXtalk(500, 5)).toBe(0);
    expect(fake.word(XTALK_PLANE_OFFSET_KCPS)).toBe(0);
  });

  it("xtalk fails with no SPADs enabled", async () => {
    const { dev, fake } = await l1();
    const noSpads = L1_BLOCK.slice();
    noSpads[3] = 0;
    noSpads[4] = 0; // RESULT__DSS_ACTUAL_EFFECTIVE_SPADS_SD0 = 0
    fake.setResultBlock(noSpads);
    await expect(dev.calibrateXtalk(650, 5)).rejects.toThrow(/no SPADs enabled/);
    expect(fake.word(XTALK_PLANE_OFFSET_KCPS)).toBe(0); // left disabled
  });

  it("the driver refuses fewer than 5 samples before any traffic", async () => {
    const { fake, drv } = await l1();
    await expect(drv.calibrateOffset(600, 4)).rejects.toThrow(/at least 5/);
    await expect(drv.calibrateXtalk(600, 4)).rejects.toThrow(/at least 5/);
    expect(fake.rx).toEqual([]);
  });
});
