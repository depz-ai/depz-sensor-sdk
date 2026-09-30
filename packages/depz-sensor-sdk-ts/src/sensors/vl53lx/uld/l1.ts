/**
 * uld/l1 — VL53L1X ULD 3.5.5: VL53L1CX and VL53L1CB. Async port of the Python
 * `depz_sensor_sdk.vl53lx.uld.l1` (VL53L1X_api.c + VL53L1X_calibration.c).
 *
 * The die body comes from `VL53L1Die`; what is this ULD's own is the ranging
 * configuration: a tabulated timing budget, a distance mode (short / long), an
 * init that resets the die first and fixes the interrupt polarity, a start
 * code with no free-running mode, and its own two calibrations. Crosstalk is
 * in kcps (the register's own unit), unlike ST's L1X wrapper.
 */

import { Vl53Error } from "./link.js";
import { floorDiv, pyRound } from "./arith.js";
import {
  CONFIG_ADDR,
  CONFIG_END,
  INNER_OFFSET_MM,
  INTERMEASUREMENT_MS,
  OUTER_OFFSET_MM,
  PHASECAL_CONFIG__TIMEOUT_MACROP,
  RANGE_CONFIG__TIMEOUT_MACROP_A_HI,
  RANGE_CONFIG__TIMEOUT_MACROP_B_HI,
  RANGE_CONFIG__VALID_PHASE_HIGH,
  RANGE_CONFIG__VCSEL_PERIOD_A,
  RANGE_CONFIG__VCSEL_PERIOD_B,
  RANGE_OFFSET_MM,
  RESULT__OSC_CALIBRATE_VAL,
  SD_CONFIG__INITIAL_PHASE_SD0,
  SD_CONFIG__WOI_SD0,
  SYSTEM__INTERRUPT_CONFIG_GPIO,
  SYSTEM__MODE_START,
  THRESH_HIGH,
  THRESH_LOW,
  VL53L1Die,
  XTALK_PLANE_OFFSET_KCPS,
  type DieResultsData,
} from "./vl53l1-die.js";

export const L1_ULD_VERSION = [3, 5, 5] as const;

/** VL51L1X_DEFAULT_CONFIGURATION[] — 91 bytes, registers 0x2D..0x87. */
export const L1_DEFAULT_CONFIGURATION = Uint8Array.from([
  0x00, 0x00, 0x00, 0x01, 0x02, 0x00, 0x02, 0x08, // 0x2D..0x34
  0x00, 0x08, 0x10, 0x01, 0x01, 0x00, 0x00, 0x00, // 0x35..0x3C
  0x00, 0xff, 0x00, 0x0f, 0x00, 0x00, 0x00, 0x00, // 0x3D..0x44
  0x00, 0x20, 0x0b, 0x00, 0x00, 0x02, 0x0a, 0x21, // 0x45..0x4C
  0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x00, 0xc8, // 0x4D..0x54
  0x00, 0x00, 0x38, 0xff, 0x01, 0x00, 0x08, 0x00, // 0x55..0x5C
  0x00, 0x01, 0xcc, 0x0f, 0x01, 0xf1, 0x0d, 0x01, // 0x5D..0x64
  0x68, 0x00, 0x80, 0x08, 0xb8, 0x00, 0x00, 0x00, // 0x65..0x6C
  0x00, 0x0f, 0x89, 0x00, 0x00, 0x00, 0x00, 0x00, // 0x6D..0x74
  0x00, 0x00, 0x01, 0x0f, 0x0d, 0x0e, 0x0e, 0x00, // 0x75..0x7C
  0x00, 0x02, 0xc7, 0xff, 0x9b, 0x00, 0x00, 0x00, // 0x7D..0x84
  0x01, 0x00, 0x00, // 0x85..0x87
]);
if (L1_DEFAULT_CONFIGURATION.length !== CONFIG_END - CONFIG_ADDR + 1) {
  throw new Error("VL53L1X configuration blob must be 91 bytes");
}

/** Distance modes (VL53L1X_SetDistanceMode). */
export const DISTANCE_SHORT = 1;
export const DISTANCE_LONG = 2;
const MODE_CODES: Readonly<Record<string, number>> = { short: DISTANCE_SHORT, long: DISTANCE_LONG };
const MODE_NAMES: Readonly<Record<number, string>> = {
  [DISTANCE_SHORT]: "short",
  [DISTANCE_LONG]: "long",
};

/**
 * VL53L1X_SetTimingBudgetInMs(): the budget is looked up, not computed.
 * {distance mode: [[budget ms, [MACROP_A_HI, MACROP_B_HI]], ...]} in the C
 * order. 15 ms exists in short mode only.
 */
export const L1_TIMING_BUDGETS: ReadonlyMap<
  number,
  ReadonlyMap<number, readonly [number, number]>
> = new Map([
  [
    DISTANCE_SHORT,
    new Map<number, readonly [number, number]>([
      [15, [0x001d, 0x0027]],
      [20, [0x0051, 0x006e]],
      [33, [0x00d6, 0x006e]],
      [50, [0x01ae, 0x01e8]],
      [100, [0x02e1, 0x0388]],
      [200, [0x03e1, 0x0496]],
      [500, [0x0591, 0x05c1]],
    ]),
  ],
  [
    DISTANCE_LONG,
    new Map<number, readonly [number, number]>([
      [20, [0x001e, 0x0022]],
      [33, [0x0060, 0x006e]],
      [50, [0x00ad, 0x00c6]],
      [100, [0x01cc, 0x01ea]],
      [200, [0x02d9, 0x02f8]],
      [500, [0x048f, 0x04a4]],
    ]),
  ],
]);

/** VL53L1X_GetTimingBudgetInMs(): MACROP_A_HI → ms, both modes (no collision). */
const BUDGET_BY_MACROP_A = new Map<number, number>();
for (const table of L1_TIMING_BUDGETS.values()) {
  for (const [ms, [a]] of table) BUDGET_BY_MACROP_A.set(a, ms);
}

function budgetTable(mode: number): ReadonlyMap<number, readonly [number, number]> {
  return L1_TIMING_BUDGETS.get(mode)!;
}

/** Port of VL53L1X_api.c + VL53L1X_calibration.c (ULD 3.5.5). */
export class VL53L1 extends VL53L1Die {
  static readonly SUPPORTS: ReadonlySet<string> = new Set([
    "mode",
    "timing",
    "offset",
    "xtalk",
    "thresholds",
    "signal_thresh",
    "sigma_thresh",
    "temp_update",
    "calib_offset",
    "calib_xtalk",
    "roi",
  ]);
  /** The configuration blob boots in long. */
  static readonly MODES: readonly string[] = ["long", "short"];
  /** Only the tabulated values exist — see budgetChoices(). */
  static readonly BUDGET_MS: readonly [number, number] = [15, 500];

  protected readonly CONFIGURATION = L1_DEFAULT_CONFIGURATION;
  /** VL53L1X_StopRanging() writes 0x00 where the L4CD ULD writes 0x80. */
  protected override readonly STOP_MODE: number = 0x00;
  /** Crosstalk-corrected peak signal at 0x0098, per-SPAD scale 200.0/8. */
  protected override readonly SIGNAL_AT: number = 15;
  protected override readonly PER_SPAD_K: number = 25;

  /** The mode the sensor was last put into (budgetChoices answers from it). */
  private appliedMode: string = VL53L1.MODES[0]!;

  // ── init (VL53L1X_SensorInit(), on the die's sequence) ──
  /** A hung sensor never reaches boot state on its own: reset first. */
  protected override initBoot(): Promise<void> {
    return this.resetDevice();
  }

  /** ST ships this blob with an active-high interrupt; the bridge wants low. */
  protected override initAfterConfig(): Promise<void> {
    return this.setInterruptPolarity(0);
  }

  protected override async initExtra(): Promise<void> {
    this.appliedMode = VL53L1.MODES[0]!;
  }

  // ── ranging ──
  /** VL53L1X_StartRanging(): timed mode, and only timed mode. */
  override async startRanging(): Promise<void> {
    await this.p.wrByte(SYSTEM__MODE_START, 0x40);
  }

  // ── ranging mode (VL53L1X_SetDistanceMode) ──
  override async setMode(name: string): Promise<void> {
    const mode = MODE_CODES[name];
    if (mode === undefined) {
      throw new Vl53Error(`no such mode: ${name} (have ${this.MODES.join(", ")})`);
    }
    let [budgetMs, interMs] = await this.getRangeTiming();
    this.appliedMode = name;

    if (mode === DISTANCE_SHORT) {
      await this.p.wrByte(PHASECAL_CONFIG__TIMEOUT_MACROP, 0x14);
      await this.p.wrByte(RANGE_CONFIG__VCSEL_PERIOD_A, 0x07);
      await this.p.wrByte(RANGE_CONFIG__VCSEL_PERIOD_B, 0x05);
      await this.p.wrByte(RANGE_CONFIG__VALID_PHASE_HIGH, 0x38);
      await this.p.wrWord(SD_CONFIG__WOI_SD0, 0x0705);
      await this.p.wrWord(SD_CONFIG__INITIAL_PHASE_SD0, 0x0606);
    } else {
      await this.p.wrByte(PHASECAL_CONFIG__TIMEOUT_MACROP, 0x0a);
      await this.p.wrByte(RANGE_CONFIG__VCSEL_PERIOD_A, 0x0f);
      await this.p.wrByte(RANGE_CONFIG__VCSEL_PERIOD_B, 0x0d);
      await this.p.wrByte(RANGE_CONFIG__VALID_PHASE_HIGH, 0xb8);
      await this.p.wrWord(SD_CONFIG__WOI_SD0, 0x0f0d);
      await this.p.wrWord(SD_CONFIG__INITIAL_PHASE_SD0, 0x0e0e);
    }

    // 15 ms exists in short mode only: fall back to the nearest budget
    // (first one in table order on a tie, as Python's min()).
    const table = budgetTable(mode);
    if (!table.has(budgetMs)) {
      let best: number | null = null;
      for (const ms of table.keys()) {
        if (best === null || Math.abs(ms - budgetMs) < Math.abs(best - budgetMs)) best = ms;
      }
      budgetMs = best!;
    }
    await this.setRangeTiming(budgetMs, interMs);
  }

  /** → 'short' or 'long'. VL53L1X_GetDistanceMode(). */
  override async getMode(): Promise<string> {
    return MODE_NAMES[await this.modeCode()]!;
  }

  private async modeCode(): Promise<number> {
    const temp = await this.p.rdByte(PHASECAL_CONFIG__TIMEOUT_MACROP);
    if (temp === 0x14) return DISTANCE_SHORT;
    if (temp === 0x0a) return DISTANCE_LONG;
    throw new Vl53Error(
      `PHASECAL_CONFIG__TIMEOUT_MACROP reads 0x${temp.toString(16).toUpperCase().padStart(2, "0")}, ` +
        "which is neither distance mode",
    );
  }

  // ── timing ──
  /**
   * VL53L1X_SetTimingBudgetInMs() + SetInterMeasurementInMs(). Only the
   * tabulated budgets exist; a zero period is written as the budget itself
   * (this part has no free-running mode).
   */
  override async setRangeTiming(timingBudgetMs: number, interMeasurementMs: number): Promise<void> {
    const mode = await this.modeCode();
    const table = budgetTable(mode);
    const pair = table.get(timingBudgetMs);
    if (pair === undefined) {
      const choices = [...table.keys()].sort((a, b) => a - b).join(", ");
      throw new Vl53Error(
        `timing budget ${timingBudgetMs} ms is not one the ${MODE_NAMES[mode]} mode has - ` +
          `pick one of ${choices} ms`,
      );
    }
    const [macropA, macropB] = pair;
    await this.p.wrWord(RANGE_CONFIG__TIMEOUT_MACROP_A_HI, macropA);
    await this.p.wrWord(RANGE_CONFIG__TIMEOUT_MACROP_B_HI, macropB);

    if (interMeasurementMs === 0) interMeasurementMs = timingBudgetMs;
    const clockPll = (await this.p.rdWord(RESULT__OSC_CALIBRATE_VAL)) & 0x3ff;
    await this.p.wrDword(INTERMEASUREMENT_MS, Math.trunc(clockPll * interMeasurementMs * 1.075));
  }

  /** The tabulated budgets of the mode in use (moves with the mode). */
  override budgetChoices(): number[] {
    return [...budgetTable(MODE_CODES[this.appliedMode]!).keys()].sort((a, b) => a - b);
  }

  override async getRangeTiming(): Promise<[number, number]> {
    const macropA = await this.p.rdWord(RANGE_CONFIG__TIMEOUT_MACROP_A_HI);
    const budgetMs = BUDGET_BY_MACROP_A.get(macropA) ?? 0;
    const tmp = await this.p.rdDword(INTERMEASUREMENT_MS);
    const clockPll = (await this.p.rdWord(RESULT__OSC_CALIBRATE_VAL)) & 0x3ff;
    const interMs = clockPll ? Math.trunc(tmp / (clockPll * 1.065)) : 0;
    return [budgetMs, interMs];
  }

  // ── thresholds ──
  /**
   * VL53L1X_SetDistanceThreshold(): read-modify-write keeping the bits
   * outside 0x6F, plus an "interrupt when no target" flag.
   */
  override async setDetectionThresholds(
    distanceLowMm: number,
    distanceHighMm: number,
    window: number,
    intOnNoTarget = 0,
  ): Promise<void> {
    let temp = (await this.p.rdByte(SYSTEM__INTERRUPT_CONFIG_GPIO)) & ~0x6f & 0xff;
    temp |= window;
    if (intOnNoTarget) temp |= 0x40;
    await this.p.wrByte(SYSTEM__INTERRUPT_CONFIG_GPIO, temp);
    await this.p.wrWord(THRESH_HIGH, distanceHighMm);
    await this.p.wrWord(THRESH_LOW, distanceLowMm);
  }

  // ── calibration (VL53L1X_calibration.c) ──
  /** VL53L1X_CalibrateOffset(), the sample count made an argument. */
  async calibrateOffset(targetDistMm: number, nbSamples = 50): Promise<number> {
    if (nbSamples < 5) throw new Vl53Error("nb_samples must be at least 5");
    await this.p.wrWord(RANGE_OFFSET_MM, 0);
    await this.p.wrWord(INNER_OFFSET_MM, 0);
    await this.p.wrWord(OUTER_OFFSET_MM, 0);

    const distances: number[] = [];
    await this.collect(nbSamples, (_i, r) => distances.push(r.distanceMm));

    const offsetMm = targetDistMm - floorDiv(sum(distances), nbSamples);
    await this.p.wrWord(RANGE_OFFSET_MM, (offsetMm * 4) & 0xffff);
    return offsetMm;
  }

  /** VL53L1X_CalibrateXtalk(). Returns kcps, not the C driver's cps. */
  async calibrateXtalk(targetDistMm: number, nbSamples = 50): Promise<number> {
    if (nbSamples < 5) throw new Vl53Error("nb_samples must be at least 5");
    await this.p.wrWord(XTALK_PLANE_OFFSET_KCPS, 0); // disable compensation

    const samples: DieResultsData[] = [];
    await this.collect(nbSamples, (_i, r) => samples.push(r));

    const n = samples.length;
    const avgDistance = sum(samples.map((s) => s.distanceMm)) / n;
    const avgSpadNb = sum(samples.map((s) => s.numberOfSpad)) / n;
    const avgSignal = sum(samples.map((s) => s.signalRateKcps)) / n;
    if (avgSpadNb === 0) throw new Vl53Error("xtalk calibration failed: no SPADs enabled");

    let calXtalk = Math.trunc((512 * (avgSignal * (1 - avgDistance / targetDistMm))) / avgSpadNb);
    calXtalk = Math.min(Math.max(calXtalk, 0), 0xffff);
    await this.p.wrWord(XTALK_PLANE_OFFSET_KCPS, calXtalk);
    return pyRound(calXtalk / 512.0);
  }
}

function sum(xs: number[]): number {
  let s = 0;
  for (const x of xs) s += x;
  return s;
}
