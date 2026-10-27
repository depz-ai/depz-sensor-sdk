/**
 * The VL53L0X ULD port (uld/l0x.ts) over the bridge fake with a VL53L0X die
 * model behind it (fake-vl53l0x-die.ts) — no hardware. Mirror of the Python
 * `tests/test_vl53l0x_uld.py`.
 *
 * The full-stack capture (scenarios: vl53l0x_uld_long-range_33ms) pins the
 * wire bytes of init / configure / stream against a live board; the first
 * test here holds the fake to the same capture, so the rest can trust it. The
 * rest is what a capture cannot reach: the reference-SPAD management and its
 * convergence cases, the NVM identification words, the tuning-buffer
 * parameters, the timing-budget and sequence-step arithmetic, every ranging
 * profile, the offset and crosstalk calibrations, the PAL range status
 * mapping, and each error path. Expected values are worked out from ST's
 * vl53l0x_api*.c (API 1.0.x) in the comments; a difference between that and
 * the port is a driver bug.
 */

import { readFileSync } from "node:fs";
import path from "node:path";
import { fileURLToPath } from "node:url";
import { afterEach, describe, expect, it } from "vitest";
import { PacketParser } from "../src/index.js";
import { BridgePlatform } from "../src/sensors/vl53lx/uld/base.js";
import {
  CHECK_RANGE_IGNORE_THRESHOLD,
  CHECK_SIGMA_FINAL_RANGE,
  CHECK_SIGNAL_RATE_FINAL_RANGE,
  CHECK_SIGNAL_RATE_MSRC,
  CHECK_SIGNAL_RATE_PRE_RANGE,
  CHECK_SIGNAL_REF_CLIP,
  DEVICEMODE_CONTINUOUS_RANGING,
  DEVICEMODE_CONTINUOUS_TIMED,
  DEVICEMODE_SINGLE_RANGING,
  GLOBAL_CONFIG_SPAD_ENABLES_REF_0,
  RESULT_INTERRUPT_STATUS,
  RESULT_RANGE_STATUS,
  SEQUENCESTEP_DSS,
  SEQUENCESTEP_FINAL_RANGE,
  SEQUENCESTEP_MSRC,
  SEQUENCESTEP_PRE_RANGE,
  SEQUENCESTEP_TCC,
  SYSRANGE_MODE_BACKTOBACK,
  SYSRANGE_MODE_TIMED,
  SYSRANGE_START,
  SYSTEM_INTERRUPT_CLEAR,
  VCSEL_PERIOD_FINAL_RANGE,
  VCSEL_PERIOD_PRE_RANGE,
  VL53L0X,
  l0xStatusText,
} from "../src/sensors/vl53lx/uld/l0x.js";
import { Vl53Error, Vl53l0x } from "../src/sensors/vl53lx/vl53lx.js";
import {
  FRAME_635_MM,
  FakeVl53l0xDie,
  L0X_BOARD,
  PRODUCT_ID,
  l0xRig,
  type FakeVl53l0xDieOptions,
} from "./fake-vl53l0x-die.js";
import { FakeVl53lxBridge, hex } from "./fake-vl53lx-bridge.js";

const here = path.dirname(fileURLToPath(import.meta.url));
const RECORDING = path.resolve(
  here,
  "../../../contracts/vectors/recordings/vl53l0x_uld_long-range_33ms.depzrec",
);
const READ_REG = 0x32;
const WRITE_REG = 0x33;
const XSHUT = 0x34;
const SET_ADDR_WIDTH = 0x39;
const CLEAR_I2C_ERRORS = 0x3a;
const REG_CMDS = new Set([READ_REG, WRITE_REG, XSHUT, 0x38, SET_ADDR_WIDTH, CLEAR_I2C_ERRORS]);

// One good frame of the capture, decoded by hand: device status 0x5A ->
// internal 11 (valid), 0xC104 effective SPADs in 8.8 -> 193 whole, signal
// 0x022B = 555 in 9.7 -> 284160 in 16.16 (4.34 Mcps), ambient 0x011B = 283
// -> 144896 (2.21 Mcps), distance 0x027B = 635 mm.
const F635 = { distance: 635, signal1616: 0x022b << 9, ambient1616: 0x011b << 9, spads: 193 };

const toHex = (b: Uint8Array) => Buffer.from(b).toString("hex");
const bytes = (b: Uint8Array | number[]) => [...b];

type Rig = { dev: Vl53l0x; fake: FakeVl53lxBridge; die: FakeVl53l0xDie };
const made: Rig[] = [];
afterEach(async () => {
  for (const { dev, fake } of made.splice(0)) {
    await dev.close().catch(() => undefined);
    await fake.close();
  }
});

async function rig(dieOpts: FakeVl53l0xDieOptions = {}): Promise<Rig> {
  const { fake, die } = l0xRig(L0X_BOARD, dieOpts);
  const dev = new Vl53l0x(fake.transport, { sleepImpl: () => Promise.resolve(), timeoutMs: 500 });
  await dev.open();
  const r = { dev, fake, die };
  made.push(r);
  return r;
}

/** An initialised VL53L0X; the fake's log starts after init. */
async function ready(dieOpts: FakeVl53l0xDieOptions = {}): Promise<Rig & { drv: VL53L0X }> {
  const r = await rig(dieOpts);
  await r.dev.init();
  r.fake.rx.length = 0;
  return { ...r, drv: r.dev.driver as VL53L0X };
}

/** A fresh driver on the device's own platform (boot / reset tests). */
function bareDriver(dev: Vl53l0x): VL53L0X {
  return new VL53L0X(new BridgePlatform(dev.bridge, () => Promise.resolve()), "VL53L0X");
}

function collapse(items: string[]): string[] {
  const out: string[] = [];
  for (const item of items) if (out.length === 0 || out[out.length - 1] !== item) out.push(item);
  return out;
}

/**
 * The register-level requests of init() in the live capture, from its first
 * SET_ADDR_WIDTH to its first CLEAR_I2C_ERRORS.
 */
function capturedInitRequests(): string[] {
  const parser = new PacketParser();
  const out: string[] = [];
  const lines = readFileSync(RECORDING, "utf8").split("\n").slice(1);
  for (const line of lines) {
    if (!line.trim()) continue;
    const entry = JSON.parse(line) as { dir: string; data: string };
    if (entry.dir !== "tx") continue;
    for (const ev of parser.feed(hex(entry.data))) {
      if (ev.type === "packet" && REG_CMDS.has(ev.cmd)) out.push(`${ev.cmd}:${toHex(ev.payload)}`);
    }
  }
  const start = out.indexOf(`${SET_ADDR_WIDTH}:01`);
  const end = out.indexOf(`${CLEAR_I2C_ERRORS}:`);
  return out.slice(start, end + 1);
}

function regRequests(fake: FakeVl53lxBridge): string[] {
  return fake.rx.filter((p) => REG_CMDS.has(p.cmd)).map((p) => `${p.cmd}:${toHex(p.payload)}`);
}

/** A 12-byte result block in the layout of FRAME_635_MM. */
function frame(
  deviceStatus: number,
  { distance = 635, signal97 = 0x022b, ambient97 = 0x011b, spads88 = 0xc104 } = {},
): Uint8Array {
  const out = new Uint8Array(12);
  out[0] = deviceStatus;
  out[1] = 0x06;
  const dv = new DataView(out.buffer);
  dv.setUint16(2, spads88);
  dv.setUint16(4, 0x0071);
  dv.setUint16(6, signal97);
  dv.setUint16(8, ambient97);
  dv.setUint16(10, distance);
  return out;
}

const readAddrs = (fake: FakeVl53lxBridge) =>
  fake.sent(READ_REG).map((p) => p[0]! | (p[1]! << 8));

// ── init against the live capture ────────────────────────────────────────────

describe("VL53L0X init", () => {
  it("issues the wire sequence of the live capture", async () => {
    // Byte for byte the requests of the capture's init(): reset, DataInit,
    // the NVM reads, the reference SPADs from NVM, the tuning settings, GPIO,
    // the budget arithmetic, VHV and phase calibration. Polls (model id while
    // the die reboots, data-ready) are collapsed: their count is timing.
    const { dev, fake } = await rig();
    await dev.init();
    expect(collapse(regRequests(fake))).toEqual(collapse(capturedInitRequests()));
  });

  it("state matches the board", async () => {
    const { dev, die, drv } = await ready();
    expect(drv).toBeInstanceOf(VL53L0X);
    expect(drv.d.StopVariable).toBe(0x3c);
    expect(drv.d.OscFrequencyMHz).toBe(0x975c << 4);
    expect(drv.refSpadsFromNvm).toBe(true);
    expect(drv.getReferenceSpads()).toEqual([6, 0]);
    // The 6 non-aperture SPADs through the good map FB..: indices 0,1,3,4,5,6
    // (index 2 is bad) -> 0b0111_1011.
    expect(toHex(die.spadEnables())).toBe("7b0000000000");
    expect(drv.d.RefSpadEnables).toEqual([0x7b, 0, 0, 0, 0, 0]);
    // The VHV / phase shots of the reference calibration, in order.
    expect(die.shots).toEqual([
      [0x41, 0x01],
      [0x01, 0x02],
    ]);
    expect(drv.d.SequenceConfig).toBe(0xe8); // TCC and MSRC off, as StaticInit leaves it
    expect(drv.d.DeviceMode).toBe(DEVICEMODE_CONTINUOUS_RANGING);
    expect(await dev.getMode()).toBe("default");
    // StaticInit measured the tuning's budget with TCC still on: 2870 overhead
    // + TCC 2029+590 + DSS 2*(2029+690) + pre 8061+660 + final (509-151 mclks
    // at 10 PCLK = 13651)+550 = 33849 us, and re-applied that with TCC off,
    // which reads back as 2870 + 5438 + 8721 + (577-151 mclks = 16244)+550.
    expect(drv.d.MeasurementTimingBudgetMicroSeconds).toBe(33849);
    expect(await drv.getMeasurementTimingBudget()).toBe(33823);
    expect(await dev.getRangeTiming()).toEqual([33, 0]);
    const info = await dev.identifyProduct();
    expect([info.modelId, info.modelIdOk, info.driverReachMm]).toEqual([0xee, true, 1200]);
    expect(info.modes).toEqual(["default", "long-range", "high-speed", "high-accuracy"]);
  });

  it("configure long-range 33 ms reads back as the capture did", async () => {
    const { dev, die, drv } = await ready();
    await dev.configure({ budgetMs: 33, mode: "long-range" });
    expect(await dev.getRangeTiming()).toEqual([32, 0]);
    expect(await dev.getMode()).toBe("long-range");
    // 0.1 Mcps lands on 12/128 in the 9.7 register; the budget reads back
    // 32983 us, the timeouts being quantised to macro periods.
    expect(await drv.driverInfo()).toEqual({
      signal_mcps: 0.09375,
      sigma_mm: 60.0,
      budget_us: 32983,
      pre_pclks: 18,
      final_pclks: 14,
    });
    expect(await drv.reachMm()).toBe(2000);
    const b0 = die.bank(0);
    const b1 = die.bank(1);
    // setVcselPulsePeriod(): phase limits, period, re-encoded timeouts, the
    // final-range width / phasecal settings — the registers the capture
    // shows being written.
    expect([b0[0x57], b0[0x56], b0[0x50]]).toEqual([0x50, 0x08, 0x08]);
    expect([bytes(b0.slice(0x51, 0x53)), b0[0x46]]).toEqual([[0x00, 0x74], 0x1d]);
    expect([b0[0x48], b0[0x47], b0[0x70], b0[0x32], b0[0x30], b1[0x30]]).toEqual([
      0x48, 0x08, 0x06, 0x03, 0x07, 0x20,
    ]);
    expect(bytes(b0.slice(0x71, 0x73))).toEqual([0x01, 0xca]);
    // Every VCSEL change ends in a phase calibration (sequence 0x02).
    expect(die.shots.some(([a, b]) => a === 0x01 && b === 0x02)).toBe(true);
  });
});

// ── ranging profiles ─────────────────────────────────────────────────────────

describe("VL53L0X ranging profiles", () => {
  // Budget read-backs worked out from the register arithmetic of the C driver
  // (macro period = 2304 * PCLK * 1655 ps; timeouts in (LSB * 2^MSB) + 1 macro
  // periods; overheads 1910 + 960, DSS 2 * (MSRC + 690), pre + 660, final +
  // 550) for each profile applied on the post-init state (pre 0x96, MSRC 0x25).
  it.each([
    ["default", { signal_mcps: 0.25, sigma_mm: 18.0, budget_us: 32908, pre_pclks: 14, final_pclks: 10 }, 1200],
    ["long-range", { signal_mcps: 0.09375, sigma_mm: 60.0, budget_us: 32983, pre_pclks: 18, final_pclks: 14 }, 2000],
    ["high-speed", { signal_mcps: 0.25, sigma_mm: 32.0, budget_us: 30010, pre_pclks: 14, final_pclks: 10 }, 1200],
    ["high-accuracy", { signal_mcps: 0.25, sigma_mm: 18.0, budget_us: 199769, pre_pclks: 14, final_pclks: 10 }, 1200],
  ] as const)("%s reads back", async (mode, info, reach) => {
    const { dev, drv } = await ready();
    await drv.setMode(mode);
    expect(await drv.getMode()).toBe(mode);
    expect(await drv.driverInfo()).toEqual(info);
    expect(await drv.reachMm()).toBe(reach);
    expect((await dev.getRangeTiming())[0]).toBe(Math.floor(info.budget_us / 1000));
  });

  it("unknown mode is refused", async () => {
    const { drv } = await ready();
    await expect(drv.setMode("fast")).rejects.toThrow(/no such mode: fast/);
  });

  it("reach is unknown for a final period no profile sets", async () => {
    const { drv } = await ready();
    await drv.setVcselPulsePeriod(VCSEL_PERIOD_FINAL_RANGE, 12);
    expect(await drv.reachMm()).toBeNull();
  });
});

// ── VCSEL periods ────────────────────────────────────────────────────────────

describe("VL53L0X VCSEL periods", () => {
  // The pre-range (8061 us) and MSRC (2029 us) timeouts of the post-init state,
  // re-encoded in the macro period of the new VCSEL period: the register values
  // and what they decode back to. 18 PCLK is what the capture's long-range
  // configure wrote (0x51 <- 0x0074, 0x46 <- 0x1D).
  it.each([
    [12, 0x18, 0x00af, 0x2b, 8053, 2013],
    [14, 0x30, 0x0096, 0x25, 8061, 2029],
    [16, 0x40, 0x0083, 0x20, 8053, 2013],
    [18, 0x50, 0x0074, 0x1d, 8030, 2059],
  ])("pre-range %i PCLK sets the phase window and recodes the timeouts", async (pclks, phaseHigh, preReg, msrcReg, preUs, msrcUs) => {
    const { die, drv } = await ready();
    await drv.setVcselPulsePeriod(VCSEL_PERIOD_PRE_RANGE, pclks);
    const b0 = die.bank(0);
    expect([b0[0x57], b0[0x56], b0[0x50]]).toEqual([phaseHigh, 0x08, (pclks >> 1) - 1]);
    expect([bytes(b0.slice(0x51, 0x53)), b0[0x46]]).toEqual([[preReg >> 8, preReg & 0xff], msrcReg]);
    expect(await drv.getVcselPulsePeriod(VCSEL_PERIOD_PRE_RANGE)).toBe(pclks);
    expect(drv.d.PreRangeVcselPulsePeriod).toBe(pclks);
    expect(await drv.getSequenceStepTimeout(SEQUENCESTEP_PRE_RANGE)).toBe(preUs);
    expect(await drv.getSequenceStepTimeout(SEQUENCESTEP_MSRC)).toBe(msrcUs);
  });

  // The final-range timeout (16244 us) re-encoded in the new period, then the
  // stored 33849 us budget re-applied on top (its remainder, 16270 us, in the
  // new macro period): the register and the read-backs.
  it.each([
    // regs = [phase high, VCSEL width, phasecal timeout, phasecal limit on page 1]
    [8, [0x10, 0x02, 0x0c, 0x30], 0x02aa, 16168, 33747],
    [10, [0x28, 0x03, 0x09, 0x20], 0x0290, 16244, 33823],
    [12, [0x38, 0x03, 0x08, 0x20], 0x01fd, 16289, 33868],
    [14, [0x48, 0x03, 0x07, 0x20], 0x01e3, 16229, 33808],
  ])("final-range %i PCLK retunes width and phasecal", async (pclks, regs, finReg, finUs, budgetUs) => {
    const { die, drv } = await ready();
    await drv.setVcselPulsePeriod(VCSEL_PERIOD_FINAL_RANGE, pclks);
    const b0 = die.bank(0);
    const b1 = die.bank(1);
    expect([b0[0x48], b0[0x32], b0[0x30], b1[0x30]]).toEqual(regs);
    expect(b0[0x47]).toBe(0x08);
    expect(b0[0x70]).toBe((pclks >> 1) - 1);
    expect(bytes(b0.slice(0x71, 0x73))).toEqual([finReg >> 8, finReg & 0xff]);
    expect(drv.d.FinalRangeVcselPulsePeriod).toBe(pclks);
    expect(await drv.getSequenceStepTimeout(SEQUENCESTEP_FINAL_RANGE)).toBe(finUs);
    expect(await drv.getMeasurementTimingBudget()).toBe(budgetUs);
    expect(die.shots[die.shots.length - 1]).toEqual([0x01, 0x02]); // the phase calibration after it
    expect(drv.d.SequenceConfig).toBe(0xe8); // ... restored the sequence
  });

  it.each([
    [VCSEL_PERIOD_PRE_RANGE, 13],
    [VCSEL_PERIOD_PRE_RANGE, 10],
    [VCSEL_PERIOD_PRE_RANGE, 20],
    [VCSEL_PERIOD_FINAL_RANGE, 6],
    [VCSEL_PERIOD_FINAL_RANGE, 16],
    [VCSEL_PERIOD_FINAL_RANGE, 9],
  ])("period type %i, %i PCLK is refused before any write", async (kind, pclks) => {
    const { fake, drv } = await ready();
    await expect(drv.setVcselPulsePeriod(kind, pclks)).rejects.toThrow(/out of range/);
    expect(fake.sent(WRITE_REG)).toEqual([]);
  });
});

// ── timing budget and sequence steps ─────────────────────────────────────────

describe("VL53L0X timing budget and sequence steps", () => {
  it("budget below 20 ms is refused like the C driver", async () => {
    // cMinTimingBudgetMicroSeconds = 20000: refused before a register is
    // touched, even though the enabled steps would fit.
    const { dev, fake, drv } = await ready();
    await expect(drv.setMeasurementTimingBudget(19999)).rejects.toThrow(/below the 20000 us minimum/);
    expect(fake.rx).toEqual([]);
    await drv.setMeasurementTimingBudget(20000);
    expect((await dev.getRangeTiming())[0]).toBe(19); // quantised down, as read back
    await expect(dev.configure({ budgetMs: 19 })).rejects.toThrow(/below the 20000 us minimum/);
  });

  it("budget too small for the enabled steps", async () => {
    const { drv } = await ready();
    await drv.setSequenceStepEnable(SEQUENCESTEP_TCC, 1);
    await drv.setSequenceStepEnable(SEQUENCESTEP_MSRC, 1);
    await drv.setSequenceStepTimeout(SEQUENCESTEP_MSRC, 10000);
    // 20000 - 2870 = 17130 left; TCC takes 10000+590, DSS wants 2*(10000+690).
    await expect(drv.setMeasurementTimingBudget(20000)).rejects.toThrow(
      /too small for the enabled sequence steps/,
    );
  });

  it("TCC, DSS and MSRC share one timeout in the budget", async () => {
    const { die, fake, drv } = await ready();
    const base = await drv.getMeasurementTimingBudget();
    expect(await drv.getSequenceStepEnables()).toEqual([false, true, false, true, true]);
    // MSRC_CONFIG_TIMEOUT_MACROP 0x25 -> 38 mclks at 14 PCLK (53384 ns) = 2029 us.
    const msrcUs = await drv.getSequenceStepTimeout(SEQUENCESTEP_MSRC);
    expect(msrcUs).toBe(2029);
    expect(await drv.getSequenceStepTimeout(SEQUENCESTEP_TCC)).toBe(msrcUs);
    expect(await drv.getSequenceStepTimeout(SEQUENCESTEP_DSS)).toBe(msrcUs);
    // Reading the budget stores the read-back as the budget to re-apply, as
    // the C getter does (VL53L0X_SETPARAMETERFIELD at its end): 33823 now.
    expect(base).toBe(33823);
    expect(drv.d.MeasurementTimingBudgetMicroSeconds).toBe(33823);
    expect(await drv.getSequenceStepTimeout(SEQUENCESTEP_FINAL_RANGE)).toBe(16244);
    // Enabling TCC re-applies it: TCC takes 2029 + 590 out of the final
    // range, 16244 -> 13625 us, which encodes as 356 + 151 macro periods =
    // 13575 us; the budget reads back 33773 (and is stored again).
    await drv.setSequenceStepEnable(SEQUENCESTEP_TCC, 1);
    expect(drv.d.SequenceConfig).toBe(0xf8);
    expect(die.bank(0)[0x01]).toBe(0xf8);
    expect(await drv.getSequenceStepTimeout(SEQUENCESTEP_FINAL_RANGE)).toBe(13575);
    expect(await drv.getMeasurementTimingBudget()).toBe(33773);
    // Enabling what is on already touches nothing.
    fake.rx.length = 0;
    await drv.setSequenceStepEnable(SEQUENCESTEP_TCC, 1);
    expect(fake.sent(WRITE_REG)).toEqual([]);
    // DSS off gives its 2 * (2029 + 690) back to the final range (33773:
    // 19013 us -> 498 macro periods = 18989); MSRC on takes 2029 + 660 of it
    // instead of DSS's double share (33749: 16300 us -> 426 = 16244).
    await drv.setSequenceStepEnable(SEQUENCESTEP_DSS, 0);
    expect(drv.d.SequenceConfig).toBe(0xd0);
    expect(await drv.getSequenceStepTimeout(SEQUENCESTEP_FINAL_RANGE)).toBe(18989);
    expect(await drv.getMeasurementTimingBudget()).toBe(33749);
    await drv.setSequenceStepEnable(SEQUENCESTEP_MSRC, 1);
    expect(drv.d.SequenceConfig).toBe(0xd4);
    expect(await drv.getSequenceStepTimeout(SEQUENCESTEP_FINAL_RANGE)).toBe(16244);
    expect(await drv.getMeasurementTimingBudget()).toBe(33693);
  });

  it("MSRC timeout encoding saturates at 255", async () => {
    // setSequenceStepTimeout(): the one-byte MSRC timeout is (mclks - 1),
    // 255 past 256 mclks.
    const { die, drv } = await ready();
    await drv.setSequenceStepTimeout(SEQUENCESTEP_TCC, 3000); // 56 mclks at 14 PCLK
    expect(die.bank(0)[0x46]).toBe(55);
    expect(await drv.getSequenceStepTimeout(SEQUENCESTEP_TCC)).toBe(2990);
    await drv.setSequenceStepTimeout(SEQUENCESTEP_DSS, 20000); // 375 mclks -> saturates
    expect(die.bank(0)[0x46]).toBe(255);
    expect(await drv.getSequenceStepTimeout(SEQUENCESTEP_MSRC)).toBe(13666); // 256 mclks
    // A zero timeout encodes as 0 (and decodes as one macro period).
    await drv.setSequenceStepTimeout(SEQUENCESTEP_PRE_RANGE, 0);
    expect(bytes(die.bank(0).slice(0x51, 0x53))).toEqual([0x00, 0x00]);
    expect(await drv.getSequenceStepTimeout(SEQUENCESTEP_PRE_RANGE)).toBe(53);
  });

  it("final-range timeout without pre-range", async () => {
    // The final-range register holds pre + final; with the pre-range off
    // nothing is subtracted / added.
    const { die, drv } = await ready();
    await drv.setSequenceStepEnable(SEQUENCESTEP_PRE_RANGE, 0);
    expect(drv.d.SequenceConfig).toBe(0xa8);
    await drv.setSequenceStepTimeout(SEQUENCESTEP_FINAL_RANGE, 10000);
    // 10000 us at 10 PCLK (38131 ns) = 262 mclks -> encoded (261 >> 1 = 130,
    // MSB 1) = 0x0182, which decodes to 261 mclks = 9952 us.
    expect(bytes(die.bank(0).slice(0x71, 0x73))).toEqual([0x01, 0x82]);
    expect(await drv.getSequenceStepTimeout(SEQUENCESTEP_FINAL_RANGE)).toBe(9952);
  });

  it("unknown sequence step is refused", async () => {
    const { drv } = await ready();
    await expect(drv.getSequenceStepTimeout(9)).rejects.toThrow(/no timeout for sequence step 9/);
    await expect(drv.setSequenceStepTimeout(9, 1000)).rejects.toThrow(/no timeout for sequence step 9/);
  });

  it("inter-measurement period scales by the oscillator calibration", async () => {
    const { die, drv } = await ready();
    await drv.setInterMeasurementPeriod(100);
    const v = 100 * 0x28;
    expect(bytes(die.bank(0).slice(0x04, 0x08))).toEqual([0, 0, v >> 8, v & 0xff]);
    expect(await drv.getInterMeasurementPeriod()).toBe(100);
    // An unprogrammed OSC_CALIBRATE_VAL (0) means the period goes in as is.
    die.bank(0).set([0x00, 0x00], 0xf8);
    await drv.setInterMeasurementPeriod(70);
    expect(bytes(die.bank(0).slice(0x04, 0x08))).toEqual([0, 0, 0, 70]);
    expect(await drv.getInterMeasurementPeriod()).toBe(70);
  });

  it("timed mode starts with the timed start code", async () => {
    const { dev, fake, drv } = await ready();
    await drv.setRangeTiming(50, 100);
    expect(drv.d.DeviceMode).toBe(DEVICEMODE_CONTINUOUS_TIMED);
    expect(await dev.getRangeTiming()).toEqual([49, 100]);
    fake.rx.length = 0;
    await drv.startRanging();
    const writes = fake.writes();
    expect(writes[writes.length - 1]).toEqual([SYSRANGE_START, [SYSRANGE_MODE_TIMED]]);
    expect(writes.slice(0, 7)).toEqual([
      [0x80, [1]],
      [0xff, [1]],
      [0x00, [0]],
      [0x91, [0x3c]],
      [0x00, [1]],
      [0xff, [0]],
      [0x80, [0]],
    ]);
    expect(() => drv.setDeviceMode(7)).toThrow(/device mode 7 is not supported/);
  });
});

// ── streaming and polling ────────────────────────────────────────────────────

describe("VL53L0X streaming and polling", () => {
  it("streamed and polled frames decode alike", async () => {
    const { dev, fake, drv } = await ready();
    const polled = await drv.readMeasurement();
    await dev.startRanging();
    const got = dev.getMeasurement(1000);
    await fake.stream(FRAME_635_MM, 471_733_528n, 0x14, 12);
    const streamed = await got;
    await dev.stopRanging();
    expect([streamed.distanceMm, streamed.status, streamed.statusText]).toEqual([635, 0, "Range Valid"]);
    expect(streamed.signalKcps).toBe((F635.signal1616 * 1000) >> 16);
    expect(streamed.ambientKcps).toBe((F635.ambient1616 * 1000) >> 16);
    expect(streamed.spads).toBe(F635.spads);
    expect(streamed.extra.device_range_status).toBe(0x5a);
    for (const name of ["distanceMm", "status", "signalKcps", "ambientKcps", "sigmaMm", "spads", "extra"] as const) {
      expect(polled[name]).toEqual(streamed[name]);
    }
    // StopMeasurement zeroes the stop variable on page 1.
    expect(fake.writes().slice(-6)).toEqual([
      [0x00, [0]],
      [0xff, [1]],
      [0x00, [0]],
      [0x91, [0]],
      [0x00, [1]],
      [0xff, [0]],
    ]);
  });

  it("single shot whose start bit never clears", async () => {
    const { die, drv } = await ready();
    die.startBitStuck = true;
    await expect(drv.performSingleRangingMeasurement()).rejects.toThrow(/start bit never cleared/);
  }, 20000);

  it("single shot waits for the start bit and data ready", async () => {
    const { fake, die, drv } = await ready();
    die.startBitReads = 3;
    die.readyAfterPolls = 4;
    const data = await drv.performSingleRangingMeasurement();
    expect([data.distanceMm, data.rangeStatus]).toEqual([635, 0]);
    expect(drv.d.DeviceMode).toBe(DEVICEMODE_SINGLE_RANGING);
    const reads = readAddrs(fake);
    expect(reads.filter((a) => a === SYSRANGE_START)).toHaveLength(4);
    expect(reads.filter((a) => a === RESULT_INTERRUPT_STATUS)).toHaveLength(5 + 1); // polls + the clear's check
    // ... and startRanging() resumes the continuous mode chosen before.
    fake.rx.length = 0;
    await drv.startRanging();
    const writes = fake.writes();
    expect(writes[writes.length - 1]).toEqual([SYSRANGE_START, [SYSRANGE_MODE_BACKTOBACK]]);
  });
});

// ── interrupt ────────────────────────────────────────────────────────────────

describe("VL53L0X interrupt", () => {
  it("that will not clear", async () => {
    const { fake, die, drv } = await ready();
    die.interruptSticky = true;
    die.readyAfterPolls = 0;
    await expect(drv.performSingleRangingMeasurement()).rejects.toThrow(
      /interrupt not cleared after three rounds/,
    );
    expect(fake.writes().filter(([a, d]) => a === SYSTEM_INTERRUPT_CLEAR && d[0] === 1)).toHaveLength(3);
  });

  it("range error bits in the interrupt status", async () => {
    const { die, drv } = await ready();
    die.interruptIdle = 0x48;
    await expect(drv.checkForDataReady()).rejects.toThrow(
      /range error reported in RESULT_INTERRUPT_STATUS 0x48/,
    );
  });

  it("data ready by range status when the GPIO is not the interrupt", async () => {
    const { fake, die, drv } = await ready();
    await drv.setGpioConfig(0, false);
    expect(die.bank(0)[0x0a]).toBe(0);
    expect(die.bank(0)[0x84]).toBe(0x11); // bit 4 set: active high
    expect(await drv.checkForDataReady()).toBe(false);
    die.bank(0)[0x14] = die.bank(0)[0x14]! | 0x01;
    expect(await drv.checkForDataReady()).toBe(true);
    const last = fake.sent(READ_REG).slice(-1)[0]!;
    expect([last[0]! | (last[1]! << 8), last[2]! | (last[3]! << 8)]).toEqual([RESULT_RANGE_STATUS, 1]);
  });
});

// ── boot and reset ───────────────────────────────────────────────────────────

describe("VL53L0X boot and reset", () => {
  it("boot timeout when the model id never answers", async () => {
    const { dev, die } = await rig();
    die.nacksAfterReset = 10 ** 6;
    die.bank(0)[0xbf] = 0;
    const drv = bareDriver(dev);
    await dev.bridge.setAddrWidth(1);
    die.write(0xbf, Uint8Array.of(0x01)); // arm the NACKs
    await expect(drv.waitBoot(0.05)).rejects.toThrow(/timeout waiting for MODEL_ID/);
  });

  it("soft reset timeouts", async () => {
    const { dev, die } = await rig();
    await dev.bridge.setAddrWidth(1);
    const drv = bareDriver(dev);
    die.resetIgnored = true; // the model id never drops to 0
    await expect(drv.resetDevice(0.05)).rejects.toThrow(/timeout in the soft reset/);
    die.resetIgnored = false;
    die.nacksAfterReset = 10 ** 6; // ... and never comes back
    await expect(drv.resetDevice(0.05)).rejects.toThrow(/timeout in the soft reset/);
  });
});

// ── NVM ──────────────────────────────────────────────────────────────────────

describe("VL53L0X NVM", () => {
  it("identification words from the NVM", async () => {
    const { fake, drv } = await ready();
    expect(drv.d.ReadDataFromDeviceDone).toBe(1);
    await drv.getInfoFromDevice(7);
    const d = drv.d;
    expect(d.ReadDataFromDeviceDone).toBe(7);
    expect(d.ProductId).toBe(PRODUCT_ID);
    expect([d.ModuleId, d.Revision]).toEqual([0x2a, 0x0c]);
    expect([d.PartUIDUpper, d.PartUIDLower]).toEqual([0x0c345678, 0x9abcdef0]);
    expect(d.SignalRateMeasFixed400mm).toBe(0xa53c << 9);
    // 0x1940 = 6464 sixteenths = 404 mm at the 400 mm target: -4 mm.
    expect(d.Part2PartOffsetAdjustmentNVMMicroMeter).toBe(-4000);
    // The reference-SPAD record is read once (option 1 was done by init).
    const strobes = fake.sent(WRITE_REG).filter((p) => p[0] === 0x94 && p[1] === 0x00);
    expect(strobes.map((p) => p[2])).toEqual([
      0x02, 0x7b, 0x77, 0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x73, 0x74, 0x75, 0x76,
    ]);
    fake.rx.length = 0;
    await drv.getInfoFromDevice(7);
    expect(fake.rx).toEqual([]);
  });

  it("offset of a target measured nearer than 400 mm", async () => {
    // 0x18C0 = 6336 sixteenths = 396 mm: the C driver's uint32 difference
    // wraps and its int16 result wraps back, which is what makes the offset
    // come out +4 mm.
    const { drv } = await ready({ nvm: { 0x75: 0x00000018, 0x76: 0xc0000000 } });
    await drv.getInfoFromDevice(4);
    expect(drv.d.Part2PartOffsetAdjustmentNVMMicroMeter).toBe(4000);
  });

  it("without a 400 mm measurement has no offset", async () => {
    const { drv } = await ready({ nvm: { 0x75: 0, 0x76: 0 } });
    await drv.getInfoFromDevice(4);
    expect(drv.d.Part2PartOffsetAdjustmentNVMMicroMeter).toBe(0);
  });

  it("read strobe timeout", async () => {
    const { dev, die } = await rig();
    die.strobeStuck = true;
    await expect(dev.init()).rejects.toThrow(/NVM read strobe/);
  }, 20000);
});

// ── reference SPADs from NVM ─────────────────────────────────────────────────

describe("VL53L0X reference SPADs from NVM", () => {
  it("aperture record is applied from the aperture quadrant", async () => {
    // 0x8000 bit set: aperture; count 5 -> indices 12..16 -> byte 1 = 0x1F << 4 | byte 2 bit 0
    const { die, drv } = await ready({ nvm: { 0x6b: (1 << 15) | (5 << 8) } });
    expect(drv.getReferenceSpads()).toEqual([5, 1]);
    expect(toHex(die.spadEnables())).toBe("00f001000000");
  });

  it.each([
    [{ 0x24: 0, 0x25: 0 }, /ran out of good reference SPADs/],
    [{ 0x24: 0x0000ffff, 0x25: 0 }, /leaves the requested quadrant/],
  ])("record that the good map cannot serve (%o)", async (nvm, message) => {
    const { dev } = await rig({ nvm });
    await expect(dev.init()).rejects.toThrow(message);
  });

  it("map that does not read back", async () => {
    const { dev, die } = await rig();
    die.spadMapStuck = true;
    await expect(dev.init()).rejects.toThrow(/did not read back/);
  });
});

// ── reference SPAD management ────────────────────────────────────────────────

describe("VL53L0X reference SPAD management", () => {
  const manage = (drv: VL53L0X) =>
    (drv as unknown as { refSpadManagement(): Promise<[number, number]> }).refSpadManagement();

  it.each([(0 << 15) | (13 << 8), (1 << 15) | (33 << 8)])(
    "an invalid NVM record (%i) makes init measure the reference SPADs",
    async (record) => {
      const { dev, die } = await rig({ nvm: { 0x6b: record } });
      await dev.init();
      const drv = dev.driver as VL53L0X;
      expect(drv.refSpadsFromNvm).toBe(false);
      // 400 per SPAD against 2560: 3 -> 1200, ..., 6 -> 2400 (160 short),
      // 7 -> 2800 (240 over): the 6-SPAD map comes back.
      expect(die.refPeaks).toEqual([1200, 1600, 2000, 2400, 2800]);
      expect(drv.getReferenceSpads()).toEqual([6, 0]);
      expect(toHex(die.spadEnables())).toBe("7b0000000000");
      expect(drv.d.RefSpadEnables).toEqual([0x7b, 0, 0, 0, 0, 0]);
      // It ran after the rest of StaticInit, with the sequence config restored.
      expect(drv.d.SequenceConfig).toBe(0xe8);
      expect(die.bank(0)[0x01]).toBe(0xe8);
      expect(await dev.getRangeTiming()).toEqual([33, 0]);
    },
  );

  it("keeps the map that overshoots less", async () => {
    const { die, drv } = await ready();
    die.refRatePerSpad = 700;
    // 3 -> 2100 (460 short), 4 -> 2800 (240 over, closer): keep 4.
    expect(await manage(drv)).toEqual([4, 0]);
    expect(die.refPeaks).toEqual([2100, 2800]);
    expect(toHex(die.spadEnables())).toBe("1b0000000000"); // indices 0, 1, 3, 4
  });

  it("stops on the target exactly", async () => {
    const { die, drv } = await ready();
    die.refRatePerSpad = 640;
    expect(await manage(drv)).toEqual([4, 0]);
    expect(die.refPeaks).toEqual([1920, 2560]);
  });

  it("stops at the edge of the non-aperture quadrant", async () => {
    const { die, drv } = await ready();
    die.refRatePerSpad = 100;
    // 11 good non-aperture SPADs (index 2 is bad) make 1100; the next good
    // SPAD is an aperture one, so it stops short of the target.
    expect(await manage(drv)).toEqual([11, 0]);
    expect(die.refPeaks[die.refPeaks.length - 1]).toBe(1100);
    expect(toHex(die.spadEnables())).toBe("fb0f00000000");
  });

  it("switches to aperture SPADs when three are too bright", async () => {
    const { die, drv } = await ready();
    die.refRatePerSpad = 1000;
    die.refRatePerApertureSpad = 100;
    // 3 non-aperture -> 3000 > 2560; restart at index 12 on aperture SPADs:
    // 3 -> 300 ... 25 -> 2500 (60 short), 26 -> 2600 (40 over, closer): 26.
    expect(await manage(drv)).toEqual([26, 1]);
    expect(die.refPeaks.slice(0, 2)).toEqual([3000, 300]);
    expect(die.refPeaks.slice(-2)).toEqual([2500, 2600]);
    const enables = die.spadEnables();
    expect(enables[0]).toBe(0);
    expect(enables[1]! & 0x0f).toBe(0);
    let ones = 0;
    for (const b of enables) for (let i = 0; i < 8; i++) ones += (b >> i) & 1;
    expect(ones).toBe(26);
    expect(drv.getReferenceSpads()).toEqual([26, 1]);
  });

  it("settles for three aperture SPADs when even those are too bright", async () => {
    const { die, drv } = await ready();
    die.refRatePerSpad = 1000;
    die.refRatePerApertureSpad = 1000;
    expect(await manage(drv)).toEqual([3, 1]);
    expect(die.refPeaks).toEqual([3000, 3000]);
    expect(toHex(die.spadEnables())).toBe("007000000000"); // indices 12, 13, 14
  });

  it("runs out of good SPADs", async () => {
    const { drv } = await ready();
    drv.d.RefGoodSpadMap = [0x0b, 0, 0, 0, 0, 0]; // indices 0, 1, 3 only
    await expect(manage(drv)).rejects.toThrow(/ran out of good reference SPADs/);
  });

  it("needs the minimum from the good map", async () => {
    const { drv } = await ready();
    drv.d.RefGoodSpadMap = [0x03, 0, 0, 0, 0, 0];
    await expect(manage(drv)).rejects.toThrow(/ran out of good reference SPADs/);
    drv.d.RefGoodSpadMap = [0, 0, 0xff, 0, 0, 0]; // aperture SPADs only
    await expect(manage(drv)).rejects.toThrow(/leaves the requested quadrant/);
  });

  it("map that does not read back", async () => {
    const { die, drv } = await ready();
    die.spadMapStuck = true;
    await expect(manage(drv)).rejects.toThrow(/did not read back/);
  });

  it("the public re-measure re-arms the stop variable and restores the sequence", async () => {
    const { dev, fake, die } = await ready();
    await dev.startRanging();
    await dev.stopRanging();
    fake.rx.length = 0;
    expect(await dev.performRefSpadManagement()).toEqual([6, 0]);
    const writes = fake.writes();
    expect(writes.slice(0, 7)).toEqual([
      [0x80, [1]],
      [0xff, [1]],
      [0x00, [0]],
      [0x91, [0x3c]],
      [0x00, [1]],
      [0xff, [0]],
      [0x80, [0]],
    ]);
    // The reference measurements run on the reference steps alone (0xC0),
    // after a VHV / phase calibration, and leave the sequence as it was.
    expect(die.shots.slice(-7)).toEqual([[0x41, 0x01], [0x01, 0x02], ...Array(5).fill([0x01, 0xc0])]);
    expect(die.bank(0)[0x01]).toBe(0xe8);
    // The last write puts the 6-SPAD map back after the 7th overshot.
    expect(writes[writes.length - 1]).toEqual([GLOBAL_CONFIG_SPAD_ENABLES_REF_0, [0x7b, 0, 0, 0, 0, 0]]);
    expect(die.bank(0)[0x80]).toBe(0x00);
  });
});

// ── tuning settings ──────────────────────────────────────────────────────────

describe("VL53L0X tuning settings", () => {
  it("parameters in the buffer", async () => {
    const { fake, drv } = await ready();
    const manage = (drv as unknown as { refSpadManagement(): Promise<[number, number]> }).refSpadManagement.bind(drv);
    fake.rx.length = 0;
    await drv.loadTuningSettings(
      Uint8Array.from([
        0xff, 0x00, 0x00, 0x64, // SigmaEstRefArray 100
        0xff, 0x01, 0x03, 0x84, // SigmaEstEffPulseWidth 900
        0xff, 0x02, 0x01, 0xf4, // SigmaEstEffAmbWidth 500
        0xff, 0x03, 0x0b, 0x00, // targetRefRate 22 Mcps in 9.7
        0x02, 0x51, 0x00, 0x96, // a two-byte register write
        0x00,
      ]),
    );
    expect(fake.writes()).toEqual([[0x51, [0x00, 0x96]]]);
    const d = drv.d;
    expect([d.SigmaEstRefArray, d.SigmaEstEffPulseWidth, d.SigmaEstEffAmbWidth]).toEqual([100, 900, 500]);
    expect(d.TargetRefRate).toBe(0x0b00);
    // ... and the SPAD management converges on the new target: 400 per SPAD,
    // 7 -> 2800 (16 short of 2816), 8 -> 3200 (384 over): 7.
    expect(await manage()).toEqual([7, 0]);
  });

  it("buffer errors", async () => {
    const { drv } = await ready();
    await expect(drv.loadTuningSettings(Uint8Array.from([0xff, 0x04, 0x00, 0x00, 0x00]))).rejects.toThrow(
      /bad tuning parameter 4 at offset 0/,
    );
    await expect(
      drv.loadTuningSettings(Uint8Array.from([0x01, 0x80, 0x01, 0x05, 0x00, 0, 0, 0, 0, 0, 0])),
    ).rejects.toThrow(/bad tuning record at offset 4/);
  });
});

// ── result decoding and the PAL range status ─────────────────────────────────

describe("VL53L0X result decoding and the PAL range status", () => {
  it.each([
    [11, 0, "Range Valid"],
    [1, 5, "Hardware Fail"],
    [2, 5, "Hardware Fail"],
    [3, 5, "Hardware Fail"],
    [6, 4, "Phase Fail"],
    [9, 4, "Phase Fail"],
    [8, 3, "Min Range Fail"],
    [10, 3, "Min Range Fail"],
    [4, 2, "Signal Fail"],
    [0, 255, "No Update"],
    [5, 255, "No Update"],
    [7, 255, "No Update"],
    [12, 255, "No Update"],
    [13, 255, "No Update"],
    [14, 255, "No Update"],
    [15, 255, "No Update"],
  ])("device status %i maps onto PAL status %i", async (internal, status, text) => {
    const { drv } = await ready();
    const data = drv.parseResultBlock(frame((internal << 3) | 0x02));
    expect([data.rangeStatus, l0xStatusText(data.rangeStatus)]).toEqual([status, text]);
    expect(data.deviceRangeStatus).toBe((internal << 3) | 0x02);
  });

  it("sigma fail and the sigma estimate", async () => {
    const { drv } = await ready();
    expect(toHex(frame(0x5a))).toBe(toHex(FRAME_635_MM));
    // The sigma estimate of the C driver on this frame, worked by hand with
    // the post-init timeouts (final 16244 us at 10 PCLK, pre 8061 us at 14):
    // 577 macro periods -> 5867 us of VCSEL, 25439 return events; p2 = 2211
    // kcps ambient / 4336 kcps peak; sigma_rtn 152, sigma_ref 67 -> 166000 in
    // 16.16 = 2 mm. Dmax from the 2.21 Mcps ambient: between the 2.0 Mcps
    // (900 mm) and 3.8 Mcps (750 mm) knees -> 882 mm (kept on a valid frame;
    // the GUI shows it).
    const good = drv.parseResultBlock(FRAME_635_MM);
    expect(drv.calcSigmaEstimate(good)).toBe(166000);
    expect([good.rangeStatus, good.sigmaMm, good.dmaxMm]).toEqual([0, 2, 882]);
    // No signal at all: the estimate pins at its maximum (0x028F87AE >> 16 = 655 mm).
    const none = drv.parseResultBlock(frame(0x5a, { signal97: 0 }));
    expect([none.rangeStatus, none.sigmaMm]).toEqual([1, 655]);
    // A faint signal under heavy ambient over a long integration: sigma over 18 mm.
    const faint = drv.parseResultBlock(frame(0x5a, { distance: 1800, signal97: 4, ambient97: 0x0800 }));
    expect(faint.rangeStatus).toBe(1);
    expect(faint.sigmaMm).toBeGreaterThan(18);
    // The sigma check off: the same frame is valid.
    await drv.setLimitCheckEnable(CHECK_SIGMA_FINAL_RANGE, 0);
    const off = drv.parseResultBlock(frame(0x5a, { distance: 1800, signal97: 4, ambient97: 0x0800 }));
    expect([off.rangeStatus, off.sigmaMm, off.dmaxMm]).toEqual([0, 0, 0]);
    // ... and so is a zero limit value.
    await drv.setLimitCheckEnable(CHECK_SIGMA_FINAL_RANGE, 1);
    await drv.setLimitCheckValue(CHECK_SIGMA_FINAL_RANGE, 0);
    expect(drv.parseResultBlock(frame(0x5a, { signal97: 0 })).rangeStatus).toBe(0);
  });

  it("crosstalk compensation enters the sigma estimate", async () => {
    const { drv } = await ready();
    const plain = drv.parseResultBlock(FRAME_635_MM);
    expect(drv.getTotalXtalkRate(plain)).toBe(0n);
    const sigmaPlain = drv.calcSigmaEstimate(plain);
    await drv.setXtalk(500); // 0.5 Mcps per SPAD
    // (spads 8.8 * rate 16.16 + 0x80) >> 8
    expect(drv.getTotalXtalkRate(plain)).toBe(
      (BigInt(0xc104) * BigInt(Math.floor((500 << 16) / 1000)) + 0x80n) >> 8n,
    );
    const withXtalk = drv.parseResultBlock(FRAME_635_MM);
    expect(withXtalk.rangeStatus).toBe(0);
    expect(drv.calcSigmaEstimate(withXtalk)).toBe(99000);
    expect(sigmaPlain).not.toBe(99000);
  });

  it("dmax follows the ambient lookup", async () => {
    const { drv } = await ready();
    expect(drv.calcDmax(0)).toBe(1200);
    expect(drv.calcDmax(15 << 16)).toBe(400);
    expect(drv.calcDmax(20 << 16)).toBe(400);
    expect(drv.calcDmax(0x0000b333)).toBe(1100); // a knee of the table
    expect(drv.calcDmax(0x00020000)).toBe(900);
    const between = drv.calcDmax(0x00018000); // between two knees
    expect(between).toBeGreaterThan(900);
    expect(between).toBeLessThan(1100);
    // On a bad frame the dmax is what the ambient of that frame allows.
    const bad = drv.parseResultBlock(frame(0x22, { ambient97: 0x1000 })); // 32 Mcps ambient
    expect([bad.rangeStatus, bad.dmaxMm]).toEqual([2, 400]);
  });

  it("fractional ranges when the range config says so", async () => {
    const { drv } = await ready();
    drv.d.RangeFractionalEnable = 1;
    const data = drv.parseResultBlock(frame(0x5a, { distance: (635 << 2) | 0x3 }));
    expect([data.distanceMm, data.rangeFractionalPart]).toEqual([635, 0xc0]);
    expect(l0xStatusText(9)).toBe("unknown (9)");
  });

  it("short result block is refused", async () => {
    const { drv } = await ready();
    expect(() => drv.decode(FRAME_635_MM.subarray(0, 11))).toThrow(/result block too short: 11 bytes/);
  });
});

// ── offset ───────────────────────────────────────────────────────────────────

describe("VL53L0X offset", () => {
  it.each([
    [0, 0x0000, 0],
    [35_000, 140, 35_000],
    [-35_000, 4096 - 140, -35_000],
    [511_000, 2044, 511_000],
    [600_000, 2044, 511_000], // clamped to 511000
    [-512_000, 2048, -512_000],
    [-700_000, 2048, -512_000], // clamped to -512000
    [-1_249, 4096 - 4, -1_000], // truncated toward zero
  ])("register is quarter millimetres in two's complement (%i um)", async (um, reg, back) => {
    const { die, drv } = await ready();
    await drv.setOffsetUm(um);
    expect(bytes(die.bank(0).slice(0x28, 0x2a))).toEqual([reg >> 8, reg & 0xff]);
    expect(await drv.getOffsetUm()).toBe(back);
  });

  it("in whole millimetres through the facade", async () => {
    const { dev, die } = await ready();
    await dev.setOffsetMm(-7);
    expect(await dev.getOffsetMm()).toBe(-7);
    const reg = 4096 - 28;
    expect(bytes(die.bank(0).slice(0x28, 0x2a))).toEqual([reg >> 8, reg & 0xff]);
  });

  it("calibration against a target", async () => {
    const { dev, die, drv } = await ready();
    await drv.setOffsetUm(12_000);
    await drv.setSequenceStepEnable(SEQUENCESTEP_TCC, 1);
    die.frames.push(frame(0x22), frame(0x22), FRAME_635_MM, frame(0x5a, { distance: 636 }));
    // Three valid of five samples (the last one repeats): mean (635+636+636)/3
    // = 635.67 -> 636; the offset becomes (600 - 636) mm.
    expect(await dev.calibrateOffset(600, 5)).toBe(-36);
    expect(await drv.getOffsetUm()).toBe(-36_000);
    expect(drv.d.SequenceConfig).toBe(0xf8); // TCC back on
    expect(die.shots.filter(([, s]) => s === 0xe8)).toHaveLength(5); // ... and off while it ran
    expect(drv.d.LimitChecksEnable[CHECK_RANGE_IGNORE_THRESHOLD]).toBe(0);
  });

  it("calibration errors", async () => {
    const { die, drv } = await ready();
    // With TCC off already there is nothing to put back afterwards.
    expect(await drv.performOffsetCalibration(635, 2)).toBe(0);
    expect(drv.d.SequenceConfig).toBe(0xe8);
    await expect(drv.performOffsetCalibration(0)).rejects.toThrow(/distance must be positive/);
    die.frame = frame(0x22);
    await expect(drv.performOffsetCalibration(400, 3)).rejects.toThrow(/no valid measurement/);
    die.frame = frame(0x5a, { distance: 0 });
    await expect(drv.performOffsetCalibration(400, 3)).rejects.toThrow(/mean range came out 0 mm/);
  });
});

// ── crosstalk ────────────────────────────────────────────────────────────────

describe("VL53L0X crosstalk", () => {
  it("register is 3.13 and follows the enable", async () => {
    const { dev, die, drv } = await ready();
    await dev.setXtalkKcps(297);
    const rate = Math.floor((297 << 16) / 1000);
    const reg = rate >> 3;
    expect(bytes(die.bank(0).slice(0x20, 0x22))).toEqual([reg >> 8, reg & 0xff]);
    expect(await dev.getXtalkKcps()).toBe(((reg << 3) * 1000) >> 16);
    await drv.setXtalkEnable(0);
    expect(bytes(die.bank(0).slice(0x20, 0x22))).toEqual([0, 0]);
    expect(drv.d.XTalkCompensationRateMegaCps).toBe(rate); // kept host-side
    await drv.setXtalkRateMcps(1 << 16); // disabled: stored only
    expect(bytes(die.bank(0).slice(0x20, 0x22))).toEqual([0, 0]);
    await drv.setXtalkEnable(1);
    const r2 = (1 << 16) >> 3;
    expect(bytes(die.bank(0).slice(0x20, 0x22))).toEqual([r2 >> 8, r2 & 0xff]);
  });

  it("calibration against a target behind the glass", async () => {
    const { dev, die, drv } = await ready();
    die.frames.push(frame(0x22), FRAME_635_MM);
    // Four valid samples of 635 mm, 284160 (16.16) signal, 193 SPADs at a
    // 1000 mm target: per SPAD 1472 * (65536 - 41615) = 35211712, rounded
    // >> 16 = 537 (0.0082 Mcps) -> 8 kcps.
    expect(await dev.calibrateXtalk(1000, 5)).toBe(8);
    expect(drv.d.XTalkCompensationRateMegaCps).toBe(537);
    expect(drv.d.XTalkCompensationEnable).toBe(1);
    expect(bytes(die.bank(0).slice(0x20, 0x22))).toEqual([0, 537 >> 3]);
    expect(die.bank(0)[0x01]).toBe(0xe8);
  });

  it("calibration with nothing to compensate", async () => {
    const { die, drv } = await ready();
    // Target at the measured distance (or nearer): no crosstalk.
    expect(await drv.performXtalkCalibration(635, 2)).toBe(0);
    expect(await drv.performXtalkCalibration(600, 2)).toBe(0);
    // No effective SPADs at all: the per-SPAD division is skipped.
    die.frame = frame(0x5a, { spads88: 0x0010 });
    expect(await drv.performXtalkCalibration(1000, 2)).toBe(0);
    expect(bytes(die.bank(0).slice(0x20, 0x22))).toEqual([0, 0]);
  });

  it("calibration errors", async () => {
    const { die, drv } = await ready();
    await expect(drv.performXtalkCalibration(-1)).rejects.toThrow(/distance must be positive/);
    die.frame = frame(0x22);
    await expect(drv.performXtalkCalibration(1000, 2)).rejects.toThrow(/no valid measurement/);
  });
});

// ── limit checks ─────────────────────────────────────────────────────────────

describe("VL53L0X limit checks", () => {
  it("on the device side", async () => {
    const { die, drv } = await ready();
    const b0 = die.bank(0);
    const word = (addr: number) => (b0[addr]! << 8) | b0[addr + 1]!;
    expect(drv.d.LimitChecksEnable).toEqual([1, 1, 0, 0, 0, 0]);
    // DataInit disables the MSRC and pre-range checks (0x60 <- 0x12, as the
    // capture shows); the tuning settings then write 0x60 <- 0x00 over it.
    expect(b0[0x60]).toBe(0x00);
    await drv.setLimitCheckEnable(CHECK_SIGNAL_RATE_MSRC, 0);
    await drv.setLimitCheckEnable(CHECK_SIGNAL_RATE_PRE_RANGE, 0);
    expect(b0[0x60]).toBe(0x12);
    // Re-enabling the MSRC check leaves bit 1 set: the C driver updates the
    // byte with the mask 0xFE (bit 0), as its UpdateByte call says, and the
    // port keeps that. The pre-range mask 0xEF does clear its bit 4.
    await drv.setLimitCheckEnable(CHECK_SIGNAL_RATE_MSRC, 1);
    expect(b0[0x60]).toBe(0x12);
    await drv.setLimitCheckValue(CHECK_SIGNAL_RATE_MSRC, 3 << 16);
    expect(word(0x64)).toBe((3 << 16) >> 9);
    expect(await drv.getLimitCheckValue(CHECK_SIGNAL_RATE_MSRC)).toBe(3 << 16);
    expect(await drv.getLimitCheckValue(CHECK_SIGNAL_RATE_PRE_RANGE)).toBe(3 << 16);
    await drv.setLimitCheckEnable(CHECK_SIGNAL_RATE_PRE_RANGE, 1);
    expect(b0[0x60]).toBe(0x02);
    await drv.setLimitCheckValue(CHECK_SIGNAL_RATE_PRE_RANGE, 5 << 16);
    expect(word(0x64)).toBe((5 << 16) >> 9);
    // Disabled: the value is remembered and applied when enabled.
    await drv.setLimitCheckValue(CHECK_SIGNAL_REF_CLIP, 40 << 16);
    expect(word(0x44)).toBe(0x4000 >> 9);
    expect(await drv.getLimitCheckValue(CHECK_SIGNAL_REF_CLIP)).toBe(40 << 16);
    await drv.setLimitCheckEnable(CHECK_SIGNAL_RATE_FINAL_RANGE, 0);
    expect(word(0x44)).toBe(0);
    expect(await drv.getLimitCheckValue(CHECK_SIGNAL_RATE_FINAL_RANGE)).toBe(0);
    await drv.setLimitCheckValue(CHECK_SIGNAL_RATE_FINAL_RANGE, 1 << 16);
    expect(word(0x44)).toBe(0);
    await drv.setLimitCheckEnable(CHECK_SIGNAL_RATE_FINAL_RANGE, 1);
    expect(word(0x44)).toBe((1 << 16) >> 9);
    await expect(drv.setLimitCheckEnable(6, 1)).rejects.toThrow(/no limit check 6/);
    expect(Vl53Error).toBeDefined();
  });
});
