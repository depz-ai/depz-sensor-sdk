/**
 * uld/bare/core — preset modes, timing maths and the ranging cycle.
 * Mirror of the Python `depz_sensor_sdk.vl53lx.uld.bare.core` (a faithful
 * port of ST's VL53LX BareDriver C).
 *
 * This turns a set of tuning numbers into a device image and back:
 * `VL53LX_read_p2p_data()` (the factory data), the
 * `VL53LX_preset_mode_histogram_*()` builders, `VL53LX_set_timeouts_us()`,
 * `VL53LX_init_and_start_range()` and `VL53LX_get_histogram_bin_data()`.
 *
 * **The device is configured in one shot.** Every preset builder edits the
 * host-side image, and the span 0x0001..0x0087 goes out as one transfer when
 * ranging starts.
 *
 * **Naming.** Methods are camelCase; the C structs (`HistogramBinData`,
 * `HistPostProcessConfig`, register fields, NVM shapes) keep their C field
 * names so they stay diffable against the Python and the C driver.
 *
 * **Integers.** Python `//`, `%`, `<<`, `>>` are mirrored through `imath.ts`
 * (floor vs trunc, no 32-bit wrap); see there.
 */

import type { BridgePlatform } from "../base.js";
import { DeviceImage, RANGE_START_BLOCKS } from "./image.js";
import { floorDiv, cdiv, pymod, shl, shr } from "./imath.js";
import {
  NvmReader,
  type AdditionalOffsetCalData,
  type CalPeakRateMap,
  type OpticalCentre,
} from "./nvm.js";
import type { BlockName, RegBlock } from "./regs.js";
import { DEFAULTS, TUNING } from "./tuning.js";

// ── register map (vl53lx_register_map.h, vl53lx_hist_map.h) ──────────────────
export const POWER_MANAGEMENT__GO1_POWER_FORCE = 0x0083;
export const FIRMWARE__ENABLE = 0x0085;
export const RESULT__OSC_CALIBRATE_VAL = 0x00de;
export const PATCH__CTRL = 0x0470;
export const PATCH__JMP_ENABLES = 0x0472;
export const PATCH__DATA_ENABLES = 0x0474;
export const PATCH__OFFSET_0 = 0x0476;
export const PATCH__ADDRESS_0 = 0x0496;

export const HISTOGRAM_BIN_DATA_I2C_INDEX = 0x0088; // result__interrupt_status
export const RESULT__HISTOGRAM_BIN_0_2 = 0x008e;
export const RESULT__HISTOGRAM_BIN_23_0 = 0x00d5;
export const PHASECAL_RESULT__REFERENCE_PHASE = 0x00d6;
export const PHASECAL_RESULT__VCSEL_START = 0x00d8;
export const RESULT__HISTOGRAM_BIN_23_0_MSB = 0x00d9;
export const RESULT__HISTOGRAM_BIN_23_0_LSB = 0x00da;
export const HISTOGRAM_BIN_DATA_I2C_SIZE_BYTES =
  RESULT__HISTOGRAM_BIN_23_0_LSB - HISTOGRAM_BIN_DATA_I2C_INDEX + 1;

// ── register settings (vl53lx_register_settings.h, vl53lx_ll_device.h) ───────
export const DEVICEMEASUREMENTMODE_BACKTOBACK = 0x20;
export const DEVICEMEASUREMENTMODE_ABORT = 0x80;
export const DEVICEMEASUREMENTMODE_STOP_MASK = 0x0f;
export const DEVICEMEASUREMENTMODE_MODE_MASK = 0xf0;
export const DEVICESCHEDULERMODE_STREAMING = 0x01;
export const DEVICESCHEDULERMODE_HISTOGRAM = 0x02;
export const DEVICEREADOUTMODE_SINGLE_SD = 0x00 << 2;
export const DEVICEREADOUTMODE_DUAL_SD = 0x01 << 2;
export const GROUPEDPARAMETERHOLD_ID_MASK = 0x02;
export const INTERRUPT_CONFIG_NEW_SAMPLE_READY = 0x20;
export const CLEAR_RANGE_INT = 0x01;
export const RANGE_STATUS__RANGE_STATUS_MASK = 0x1f;

export const DEVICEINTERRUPTPOLARITY_ACTIVE_LOW = 0x10;
export const DEVICEGPIOMODE_OUTPUT_RANGE_AND_ERROR_INTERRUPTS = 0x01;
export const DEVICEDSSMODE__TARGET_RATE = 1;

export const SEQUENCE_VHV_EN = 0x01;
export const SEQUENCE_PHASECAL_EN = 0x02;
export const SEQUENCE_DSS1_EN = 0x08;
export const SEQUENCE_DSS2_EN = 0x10;
export const SEQUENCE_MM1_EN = 0x20;
export const SEQUENCE_MM2_EN = 0x40;
export const SEQUENCE_RANGE_EN = 0x80;

export const SPAD_ARRAY_WIDTH = 16;
export const SPAD_ARRAY_HEIGHT = 16;
export const RTN_SPAD_APERTURE_TRANSMISSION = 0x0038;
export const RTN_SPAD_UNITY_TRANSMISSION = 0x0100;

export const AMBIENT_WINDOW_VCSEL_PERIODS = 256;
export const RANGING_WINDOW_VCSEL_PERIODS = 2048;
export const MACRO_PERIOD_VCSEL_PERIODS =
  AMBIENT_WINDOW_VCSEL_PERIODS + RANGING_WINDOW_VCSEL_PERIODS;
export const HISTOGRAM_BUFFER_SIZE = 24;

/** The three preset modes, by the distance mode that picks them. */
export const DISTANCE_MODES = ["short", "medium", "long"] as const;
export type DistanceMode = (typeof DISTANCE_MODES)[number];

// VL53LX_SetMeasurementTimingBudgetMicroSeconds: a fixed guard comes off the
// top and the rest is split six ways.
export const TIMING_GUARD_US = 1700;
export const TIMING_DIVISOR = 6;
export const FDA_MAX_TIMING_BUDGET_US = 550000;

function tp(key: string): number {
  const v = TUNING[key];
  if (v === undefined) throw new Error(`no tuning parameter ${key}`);
  return v;
}

function dflt(key: string): number {
  const v = DEFAULTS[key];
  if (v === undefined) throw new Error(`no tuning default ${key}`);
  return v;
}

// ── SPAD geometry (vl53lx_core.c, vl53lx_core_support.c) ─────────────────────
export function encodeRowCol(row: number, col: number): number {
  if (row > 7) return (128 + (col << 3) + (15 - row)) & 0xff;
  return (((15 - col) << 3) + row) & 0xff;
}

/** → [row, col] */
export function decodeRowCol(spadNumber: number): [number, number] {
  if (spadNumber > 127) return [8 + ((255 - spadNumber) & 0x07), (spadNumber - 128) >> 3];
  return [spadNumber & 0x07, (127 - spadNumber) >> 3];
}

/** → [width, height] */
export function decodeZoneSize(encodedXySize: number): [number, number] {
  return [encodedXySize & 0x0f, encodedXySize >> 4];
}

export function encodeZoneSize(width: number, height: number): number {
  return ((height << 4) + width) & 0xff;
}

/** → [x_ll, y_ll, x_ur, y_ur], clipped to the SPAD array. */
export function decodeZoneLimits(
  encodedXyCentre: number,
  encodedXySize: number,
): [number, number, number, number] {
  const [yCentre, xCentre] = decodeRowCol(encodedXyCentre);
  const [width, height] = decodeZoneSize(encodedXySize);
  const xLl = Math.max(0, xCentre - floorDiv(width + 1, 2));
  const xUr = Math.min(SPAD_ARRAY_WIDTH - 1, xLl + width);
  const yLl = Math.max(0, yCentre - floorDiv(height + 1, 2));
  const yUr = Math.min(SPAD_ARRAY_HEIGHT - 1, yLl + height);
  return [xLl, yLl, xUr, yUr];
}

export function isApertureLocation(row: number, col: number): boolean {
  const r = row % 4;
  const c = col % 4;
  return (r === 0 && c === 2) || (r === 2 && c === 0);
}

/** → [mm inner, mm outer] effective SPADs, in 1/256 of a SPAD. */
export function calcMmEffectiveSpads(
  encodedMmRoiCentre: number,
  encodedMmRoiSize: number,
  encodedZoneCentre: number,
  encodedZoneSize: number,
  goodSpads: Uint8Array,
  apertureAttenuation: number,
): [number, number] {
  const [mmXLl, mmYLl, mmXUr, mmYUr] = decodeZoneLimits(encodedMmRoiCentre, encodedMmRoiSize);
  const [zXLl, zYLl, zXUr, zYUr] = decodeZoneLimits(encodedZoneCentre, encodedZoneSize);
  let inner = 0;
  let outer = 0;
  for (let y = zYLl; y <= zYUr; y++) {
    for (let x = zXLl; x <= zXUr; x++) {
      const spad = encodeRowCol(y, x);
      if (!(goodSpads[spad >> 3]! & (1 << (spad & 0x07)))) continue;
      const attenuation = isApertureLocation(y, x)
        ? apertureAttenuation
        : RTN_SPAD_UNITY_TRANSMISSION;
      if (mmXLl <= x && x <= mmXUr && mmYLl <= y && y <= mmYUr) inner += attenuation;
      else outer += attenuation;
    }
  }
  return [inner, outer];
}

/**
 * `VL53LX_hist_combine_mm1_mm2_offsets()` → `range_offset_mm`, in quarter
 * millimetres. The two NVM offsets are weighted by the nominal MM1/MM2 peak
 * rates scaled by the share of each region the ROI covers.
 */
export function combineMm1Mm2Offsets(
  mm1OffsetMm: number,
  mm2OffsetMm: number,
  encodedMmRoiCentre: number,
  encodedMmRoiSize: number,
  encodedZoneCentre: number,
  encodedZoneSize: number,
  calData: AdditionalOffsetCalData,
  goodSpads: Uint8Array,
  apertureAttenuation: number,
): number {
  const [maxInner, maxOuter] = calcMmEffectiveSpads(
    encodedMmRoiCentre, encodedMmRoiSize, 0xc7, 0xff, goodSpads, apertureAttenuation,
  );
  if (maxInner === 0 || maxOuter === 0) return 0;

  const [inner, outer] = calcMmEffectiveSpads(
    encodedMmRoiCentre, encodedMmRoiSize, encodedZoneCentre, encodedZoneSize,
    goodSpads, apertureAttenuation,
  );

  const mm1Rate = floorDiv(calData.result__mm_inner_peak_signal_count_rtn_mcps * inner, maxInner);
  const mm2Rate = floorDiv(calData.result__mm_outer_peak_signal_count_rtn_mcps * outer, maxOuter);

  const total = mm1Rate + mm2Rate;
  if (total === 0) return 0;
  // The offsets are signed: the division truncates towards zero, as in C.
  const num = (mm1OffsetMm * mm1Rate + mm2OffsetMm * mm2Rate) * 4;
  return cdiv(num, total);
}

/** The 32 `global_config__spad_enables_rtn_*` bytes as one bitmap. */
export function rtnGoodSpads(nvmCopyData: RegBlock): Uint8Array {
  const out = new Uint8Array(32);
  for (let i = 0; i < 32; i++) out[i] = nvmCopyData.v[`global_config__spad_enables_rtn_${i}`]!;
  return out;
}

// ── timing maths (vl53lx_core.c, vl53lx_core_support.c) ──────────────────────
export function calcPllPeriodUs(fastOscFrequency: number): number {
  return fastOscFrequency > 0 ? floorDiv(2 ** 30, fastOscFrequency) : 0;
}

export function decodeVcselPeriod(vcselPeriodReg: number): number {
  return shl(vcselPeriodReg + 1, 1);
}

export function calcMacroPeriodUs(fastOscFrequency: number, vcselPeriodReg: number): number {
  let macro = MACRO_PERIOD_VCSEL_PERIODS * calcPllPeriodUs(fastOscFrequency);
  macro = shr(macro, 6);
  macro *= decodeVcselPeriod(vcselPeriodReg);
  return shr(macro, 6);
}

export function calcTimeoutMclks(timeoutUs: number, macroPeriodUs: number): number {
  if (macroPeriodUs === 0) return 0;
  return floorDiv(shl(timeoutUs, 12) + shr(macroPeriodUs, 1), macroPeriodUs);
}

export function encodeTimeout(timeoutMclks: number): number {
  if (timeoutMclks <= 0) return 0;
  let lsByte = timeoutMclks - 1;
  let msByte = 0;
  // Python `ls_byte & 0xFFFFFF00` on an unbounded int: bits 8..31 only.
  while (pymod(shr(lsByte, 8), 2 ** 24) !== 0) {
    lsByte = shr(lsByte, 1);
    msByte += 1;
  }
  return pymod(shl(msByte, 8) + pymod(lsByte, 256), 0x10000);
}

export function decodeTimeout(encoded: number): number {
  return shl(encoded & 0x00ff, (encoded & 0xff00) >> 8) + 1;
}

export function calcEncodedTimeout(timeoutUs: number, macroPeriodUs: number): number {
  return encodeTimeout(calcTimeoutMclks(timeoutUs, macroPeriodUs));
}

export function durationMaths(
  pllPeriodUs: number,
  vcselParmPclks: number,
  windowVclks: number,
  elapsedMclks: number,
): number {
  let duration = shr(windowVclks * pllPeriodUs, 12);
  duration *= shr(elapsedMclks * vcselParmPclks, 4);
  return Math.min(shr(duration, 12), 0xffffffff);
}

/** Fill the six timeout registers, for VCSEL period A and then B. */
export function calcTimeoutRegisterValues(
  phasecalUs: number,
  mmUs: number,
  rangeUs: number,
  fastOscFrequency: number,
  genCfg: RegBlock,
  timCfg: RegBlock,
): void {
  if (fastOscFrequency === 0) throw new Error("osc_measured__fast_osc__frequency is 0");
  const g = genCfg.v;
  const t = timCfg.v;

  let macro = calcMacroPeriodUs(fastOscFrequency, t.range_config__vcsel_period_a!);
  g.phasecal_config__timeout_macrop = Math.min(0xff, calcTimeoutMclks(phasecalUs, macro));

  let encoded = calcEncodedTimeout(mmUs, macro);
  t.mm_config__timeout_macrop_a_hi = encoded >> 8;
  t.mm_config__timeout_macrop_a_lo = encoded & 0xff;
  encoded = calcEncodedTimeout(rangeUs, macro);
  t.range_config__timeout_macrop_a_hi = encoded >> 8;
  t.range_config__timeout_macrop_a_lo = encoded & 0xff;

  macro = calcMacroPeriodUs(fastOscFrequency, t.range_config__vcsel_period_b!);
  encoded = calcEncodedTimeout(mmUs, macro);
  t.mm_config__timeout_macrop_b_hi = encoded >> 8;
  t.mm_config__timeout_macrop_b_lo = encoded & 0xff;
  encoded = calcEncodedTimeout(rangeUs, macro);
  t.range_config__timeout_macrop_b_hi = encoded >> 8;
  t.range_config__timeout_macrop_b_lo = encoded & 0xff;
}

// ── the histogram config struct (not a register block) ───────────────────────
export const HIST_CFG_FIELDS = [
  "low_amb_even_bin_0_1", "low_amb_even_bin_2_3", "low_amb_even_bin_4_5",
  "low_amb_odd_bin_0_1", "low_amb_odd_bin_2_3", "low_amb_odd_bin_4_5",
  "mid_amb_even_bin_0_1", "mid_amb_even_bin_2_3", "mid_amb_even_bin_4_5",
  "mid_amb_odd_bin_0_1", "mid_amb_odd_bin_2", "mid_amb_odd_bin_3_4",
  "mid_amb_odd_bin_5", "user_bin_offset",
  "high_amb_even_bin_0_1", "high_amb_even_bin_2_3", "high_amb_even_bin_4_5",
  "high_amb_odd_bin_0_1", "high_amb_odd_bin_2_3", "high_amb_odd_bin_4_5",
  "amb_thresh_low", "amb_thresh_high", "spad_array_selection",
] as const;

/**
 * `VL53LX_histogram_config_t`. It has no registers of its own: the bin
 * sequence is smuggled into static_config and timing_config fields that mean
 * something else in lite mode, which `copyToStaticCfg()` does.
 */
export class HistConfig {
  low_amb_even_bin_0_1 = 0;
  low_amb_even_bin_2_3 = 0;
  low_amb_even_bin_4_5 = 0;
  low_amb_odd_bin_0_1 = 0;
  low_amb_odd_bin_2_3 = 0;
  low_amb_odd_bin_4_5 = 0;
  mid_amb_even_bin_0_1 = 0;
  mid_amb_even_bin_2_3 = 0;
  mid_amb_even_bin_4_5 = 0;
  mid_amb_odd_bin_0_1 = 0;
  mid_amb_odd_bin_2 = 0;
  mid_amb_odd_bin_3_4 = 0;
  mid_amb_odd_bin_5 = 0;
  user_bin_offset = 0;
  high_amb_even_bin_0_1 = 0;
  high_amb_even_bin_2_3 = 0;
  high_amb_even_bin_4_5 = 0;
  high_amb_odd_bin_0_1 = 0;
  high_amb_odd_bin_2_3 = 0;
  high_amb_odd_bin_4_5 = 0;
  amb_thresh_low = 0;
  amb_thresh_high = 0;
  spad_array_selection = 0;

  private get(name: string): number {
    return (this as unknown as Record<string, number>)[name]!;
  }

  /**
   * VL53LX_init_histogram_config_structure(): six even and six odd bin codes,
   * packed two to a byte, then repeated across the three ambient levels.
   */
  setBinSequence(even: readonly number[], odd: readonly number[]): void {
    this.low_amb_even_bin_0_1 = (even[1]! << 4) + even[0]!;
    this.low_amb_even_bin_2_3 = (even[3]! << 4) + even[2]!;
    this.low_amb_even_bin_4_5 = (even[5]! << 4) + even[4]!;
    this.low_amb_odd_bin_0_1 = (odd[1]! << 4) + odd[0]!;
    this.low_amb_odd_bin_2_3 = (odd[3]! << 4) + odd[2]!;
    this.low_amb_odd_bin_4_5 = (odd[5]! << 4) + odd[4]!;

    this.mid_amb_even_bin_0_1 = this.low_amb_even_bin_0_1;
    this.mid_amb_even_bin_2_3 = this.low_amb_even_bin_2_3;
    this.mid_amb_even_bin_4_5 = this.low_amb_even_bin_4_5;
    this.mid_amb_odd_bin_0_1 = this.low_amb_odd_bin_0_1;
    this.mid_amb_odd_bin_2 = odd[2]!;
    this.mid_amb_odd_bin_3_4 = (odd[4]! << 4) + odd[3]!;
    this.mid_amb_odd_bin_5 = odd[5]!;
    this.user_bin_offset = 0x00;

    this.high_amb_even_bin_0_1 = this.low_amb_even_bin_0_1;
    this.high_amb_even_bin_2_3 = this.low_amb_even_bin_2_3;
    this.high_amb_even_bin_4_5 = this.low_amb_even_bin_4_5;
    this.high_amb_odd_bin_0_1 = this.low_amb_odd_bin_0_1;
    this.high_amb_odd_bin_2_3 = this.low_amb_odd_bin_2_3;
    this.high_amb_odd_bin_4_5 = this.low_amb_odd_bin_4_5;

    this.amb_thresh_low = 0xffff;
    this.amb_thresh_high = 0xffff;
    this.spad_array_selection = 0x00;
  }

  /** VL53LX_copy_hist_cfg_to_static_cfg(). */
  copyToStaticCfg(staticCfg: RegBlock, timing: RegBlock, dynamic: RegBlock): void {
    const s = staticCfg.v;
    const t = timing.v;
    const d = dynamic.v;
    s.sigma_estimator__effective_pulse_width_ns = this.high_amb_even_bin_0_1;
    s.sigma_estimator__effective_ambient_width_ns = this.high_amb_even_bin_2_3;
    s.sigma_estimator__sigma_ref_mm = this.high_amb_even_bin_4_5;
    s.algo__crosstalk_compensation_valid_height_mm = this.high_amb_odd_bin_0_1;
    s.spare_host_config__static_config_spare_0 = this.high_amb_odd_bin_2_3;
    s.spare_host_config__static_config_spare_1 = this.high_amb_odd_bin_4_5;

    s.algo__range_ignore_threshold_mcps =
      shl(this.mid_amb_even_bin_0_1, 8) + this.mid_amb_even_bin_2_3;
    s.algo__range_ignore_valid_height_mm = this.mid_amb_even_bin_4_5;
    s.algo__range_min_clip = this.mid_amb_odd_bin_0_1;
    s.algo__consistency_check__tolerance = this.mid_amb_odd_bin_2;
    s.spare_host_config__static_config_spare_2 = this.mid_amb_odd_bin_3_4;
    s.sd_config__reset_stages_msb = this.mid_amb_odd_bin_5;
    s.sd_config__reset_stages_lsb = this.user_bin_offset;

    t.range_config__sigma_thresh = shl(this.low_amb_even_bin_0_1, 8) + this.low_amb_even_bin_2_3;
    t.range_config__min_count_rate_rtn_limit_mcps =
      shl(this.low_amb_even_bin_4_5, 8) + this.low_amb_odd_bin_0_1;
    t.range_config__valid_phase_low = this.low_amb_odd_bin_2_3;
    t.range_config__valid_phase_high = this.low_amb_odd_bin_4_5;

    d.system__thresh_high = this.amb_thresh_low;
    d.system__thresh_low = this.amb_thresh_high;
    d.system__enable_xtalk_per_quadrant = this.spad_array_selection;
  }

  /**
   * VL53LX_hist_get_bin_sequence_config(): which of the three sequences the
   * device used for this frame, even or odd according to the stream count.
   */
  binSequence(streamCount: number, ambientEventsSum: number): number[] {
    let level = "mid";
    if (ambientEventsSum > 1024 * this.amb_thresh_high) level = "high";
    if (ambientEventsSum < 1024 * this.amb_thresh_low) level = "low";

    let packed: number[];
    if ((streamCount & 0x01) === 0) {
      packed = [
        this.get(`${level}_amb_even_bin_0_1`),
        this.get(`${level}_amb_even_bin_2_3`),
        this.get(`${level}_amb_even_bin_4_5`),
      ];
    } else if (level === "mid") {
      // The mid odd sequence is the one packed differently.
      return [
        this.mid_amb_odd_bin_0_1 & 0x0f,
        shr(this.mid_amb_odd_bin_0_1, 4),
        this.mid_amb_odd_bin_2,
        shr(this.mid_amb_odd_bin_3_4, 4),
        this.mid_amb_odd_bin_3_4 & 0x0f,
        this.mid_amb_odd_bin_5 & 0x0f,
      ];
    } else {
      packed = [
        this.get(`${level}_amb_odd_bin_0_1`),
        this.get(`${level}_amb_odd_bin_2_3`),
        this.get(`${level}_amb_odd_bin_4_5`),
      ];
    }
    const seq: number[] = [];
    for (const byte of packed) seq.push(byte & 0x0f, shr(byte, 4));
    return seq;
  }
}

/**
 * `VL53LX_hist_post_process_config_t`, as
 * `VL53LX_init_hist_post_process_config_struct()` leaves it.
 */
export class HistPostProcessConfig {
  hist_algo_select = dflt("TUNINGPARM_HIST_ALGO_SELECT_DEFAULT");
  hist_target_order = dflt("TUNINGPARM_HIST_TARGET_ORDER_DEFAULT");
  filter_woi0 = dflt("TUNINGPARM_HIST_FILTER_WOI_0_DEFAULT");
  filter_woi1 = dflt("TUNINGPARM_HIST_FILTER_WOI_1_DEFAULT");
  hist_amb_est_method = dflt("TUNINGPARM_HIST_AMB_EST_METHOD_DEFAULT");
  ambient_thresh_sigma0 = dflt("TUNINGPARM_HIST_AMB_THRESH_SIGMA_0_DEFAULT");
  ambient_thresh_sigma1 = dflt("TUNINGPARM_HIST_AMB_THRESH_SIGMA_1_DEFAULT");
  ambient_thresh_events_scaler = dflt("TUNINGPARM_HIST_AMB_EVENTS_SCALER_DEFAULT");
  min_ambient_thresh_events = dflt("TUNINGPARM_HIST_MIN_AMB_THRESH_EVENTS_DEFAULT");
  noise_threshold = dflt("TUNINGPARM_HIST_NOISE_THRESHOLD_DEFAULT");
  signal_total_events_limit = dflt("TUNINGPARM_HIST_SIGNAL_TOTAL_EVENTS_LIMIT_DEFAULT");
  sigma_estimator__sigma_ref_mm = dflt("TUNINGPARM_HIST_SIGMA_EST_REF_MM_DEFAULT");
  sigma_thresh = dflt("TUNINGPARM_HIST_SIGMA_THRESH_MM_DEFAULT");
  range_offset_mm = 0;
  gain_factor = dflt("TUNINGPARM_HIST_GAIN_FACTOR_DEFAULT");
  valid_phase_low = 0x08;
  valid_phase_high = 0x88;
  algo__consistency_check__phase_tolerance = dflt(
    "TUNINGPARM_CONSISTENCY_HIST_PHASE_TOLERANCE_DEFAULT",
  );
  algo__consistency_check__event_sigma = dflt("TUNINGPARM_CONSISTENCY_HIST_EVENT_SIGMA_DEFAULT");
  algo__consistency_check__event_min_spad_count = dflt(
    "TUNINGPARM_CONSISTENCY_HIST_EVENT_SIGMA_MIN_SPAD_LIMIT_DEFAULT",
  );
  algo__consistency_check__min_max_tolerance = dflt(
    "TUNINGPARM_CONSISTENCY_HIST_MIN_MAX_TOLERANCE_MM_DEFAULT",
  );
  algo__crosstalk_compensation_enable: number;
  algo__crosstalk_detect_min_valid_range_mm = dflt(
    "TUNINGPARM_XTALK_DETECT_MIN_VALID_RANGE_MM_DEFAULT",
  );
  algo__crosstalk_detect_max_valid_range_mm = dflt(
    "TUNINGPARM_XTALK_DETECT_MAX_VALID_RANGE_MM_DEFAULT",
  );
  algo__crosstalk_detect_max_valid_rate_kcps = dflt(
    "TUNINGPARM_XTALK_DETECT_MAX_VALID_RATE_KCPS_DEFAULT",
  );
  algo__crosstalk_detect_max_sigma_mm = dflt("TUNINGPARM_XTALK_DETECT_MAX_SIGMA_MM_DEFAULT");
  algo__crosstalk_detect_event_sigma = dflt("TUNINGPARM_XTALK_DETECT_EVENT_SIGMA_DEFAULT");
  algo__crosstalk_detect_min_max_tolerance = dflt(
    "TUNINGPARM_XTALK_DETECT_MIN_MAX_TOLERANCE_DEFAULT",
  );
  // Filled in by readP2pData() from the customer NVM block.
  algo__crosstalk_compensation_plane_offset_kcps = 0;
  algo__crosstalk_compensation_x_plane_gradient_kcps = 0;
  algo__crosstalk_compensation_y_plane_gradient_kcps = 0;

  constructor(xtalkCompensationEnable = 0) {
    this.algo__crosstalk_compensation_enable = xtalkCompensationEnable;
  }
}

// ── the histogram frame ──────────────────────────────────────────────────────
/**
 * `VL53LX_histogram_bin_data_t`: 24 bins plus everything the post-processing
 * needs to read them — the VCSEL period they were taken at, the phase of zero
 * distance, the bin sequence and the ambient estimate. C field names kept.
 */
export class HistogramBinData {
  result__interrupt_status = 0;
  result__range_status = 0;
  result__report_status = 0;
  result__stream_count = 0;
  result__dss_actual_effective_spads = 0;
  phasecal_result__reference_phase = 0;
  phasecal_result__vcsel_start = 0;
  bin_data: number[] = new Array<number>(HISTOGRAM_BUFFER_SIZE).fill(0);
  zone_id = 0;
  first_bin = 0; // VL53LX_p_019
  number_of_bins = HISTOGRAM_BUFFER_SIZE; // VL53LX_p_020, buffer
  bins_in_data = HISTOGRAM_BUFFER_SIZE; // VL53LX_p_021, in use
  cal_config__vcsel_start = 0;
  vcsel_width = 0;
  fast_osc_frequency = 0; // VL53LX_p_015
  vcsel_period = 0; // VL53LX_p_005, the register value
  bin_seq: number[] = [0, 0, 0, 0, 0, 0];
  bin_rep: number[] = [0, 0, 0, 0, 0, 0]; // how often each code was sampled
  min_bin_value = 0;
  max_bin_value = 0;
  number_of_ambient_bins = 0;
  number_of_ambient_samples = 0;
  ambient_events_sum = 0;
  ambient_per_bin = 0; // VL53LX_p_028
  total_periods_elapsed = 0;
  peak_duration_us = 0;
  woi_duration_us = 0;
  zero_distance_phase = 0;
  roi_config__user_roi_centre_spad = 0;
  roi_config__user_roi_requested_global_xy_size = 0;

  get range_status(): number {
    return this.result__range_status & RANGE_STATUS__RANGE_STATUS_MASK;
  }

  /** `copy.deepcopy()` — every field is a number or a number array. */
  clone(): HistogramBinData {
    const c = Object.assign(new HistogramBinData(), this);
    c.bin_data = this.bin_data.slice();
    c.bin_seq = this.bin_seq.slice();
    c.bin_rep = this.bin_rep.slice();
    return c;
  }
}

// ── the ll driver state machine ──────────────────────────────────────────────
// `VL53LX_update_ll_driver_{rd,cfg}_state()`, reduced to the single-zone case.
// The GPH id stamped into the dynamic config tells the device a configuration
// is new; the timing status says whether this frame ran on VCSEL period A or B.
export const DEVICESTATE_SW_STANDBY = "sw_standby";
export const DEVICESTATE_RANGING_WAIT_GPH_SYNC = "wait_gph_sync";
export const DEVICESTATE_RANGING_OUTPUT_DATA = "output_data";
export const DEVICESTATE_RANGING_DSS_AUTO = "dss_auto";

export class LLState {
  cfg_device_state = DEVICESTATE_SW_STANDBY;
  cfg_stream_count = 0;
  cfg_gph_id = GROUPEDPARAMETERHOLD_ID_MASK;
  cfg_timing_status = 0;
  rd_device_state = DEVICESTATE_SW_STANDBY;
  rd_stream_count = 0;
  rd_gph_id = GROUPEDPARAMETERHOLD_ID_MASK;
  rd_timing_status = 0;

  reset(): void {
    this.cfg_device_state = DEVICESTATE_SW_STANDBY;
    this.cfg_stream_count = 0;
    this.cfg_gph_id = GROUPEDPARAMETERHOLD_ID_MASK;
    this.cfg_timing_status = 0;
    this.rd_device_state = DEVICESTATE_SW_STANDBY;
    this.rd_stream_count = 0;
    this.rd_gph_id = GROUPEDPARAMETERHOLD_ID_MASK;
    this.rd_timing_status = 0;
  }

  updateRd(modeStart: number, groupedParameterHold: number): void {
    if ((modeStart & DEVICEMEASUREMENTMODE_MODE_MASK) === 0) {
      this.rd_device_state = DEVICESTATE_SW_STANDBY;
      this.rd_stream_count = 0;
      this.rd_gph_id = GROUPEDPARAMETERHOLD_ID_MASK;
      this.rd_timing_status = 0;
      return;
    }
    this.rd_stream_count = this.rd_stream_count === 0xff ? 0x80 : this.rd_stream_count + 1;
    this.rd_gph_id ^= GROUPEDPARAMETERHOLD_ID_MASK;

    if (this.rd_device_state === DEVICESTATE_SW_STANDBY) {
      this.rd_device_state =
        groupedParameterHold & GROUPEDPARAMETERHOLD_ID_MASK
          ? DEVICESTATE_RANGING_WAIT_GPH_SYNC
          : DEVICESTATE_RANGING_OUTPUT_DATA;
      this.rd_stream_count = 0;
      this.rd_timing_status = 0;
    } else if (this.rd_device_state === DEVICESTATE_RANGING_WAIT_GPH_SYNC) {
      this.rd_stream_count = 0;
      this.rd_device_state = DEVICESTATE_RANGING_OUTPUT_DATA;
    } else if (this.rd_device_state === DEVICESTATE_RANGING_OUTPUT_DATA) {
      this.rd_timing_status ^= 0x01;
    } else {
      this.reset();
    }
  }

  updateCfg(modeStart: number): void {
    if ((modeStart & DEVICEMEASUREMENTMODE_MODE_MASK) === 0) {
      this.cfg_device_state = DEVICESTATE_SW_STANDBY;
      this.cfg_stream_count = 0;
      this.cfg_gph_id = GROUPEDPARAMETERHOLD_ID_MASK;
      this.cfg_timing_status = 0;
      return;
    }
    this.cfg_stream_count = this.cfg_stream_count === 0xff ? 0x80 : this.cfg_stream_count + 1;
    this.cfg_gph_id ^= GROUPEDPARAMETERHOLD_ID_MASK;

    if (this.cfg_device_state === DEVICESTATE_SW_STANDBY) {
      this.cfg_timing_status ^= 0x01;
      this.cfg_stream_count = 1;
      this.cfg_device_state = DEVICESTATE_RANGING_DSS_AUTO;
    } else if (this.cfg_device_state === DEVICESTATE_RANGING_DSS_AUTO) {
      this.cfg_timing_status ^= 0x01;
    } else {
      this.reset();
    }
  }
}

export interface FmtDmaxCal {
  ref__actual_effective_spads: number;
  ref__peak_signal_count_rate_mcps: number;
  ref__distance_mm: number;
  ref_reflectance_pc: number;
  coverglass_transmission: number;
}

export interface MmRoi {
  x_centre: number;
  y_centre: number;
  width: number;
  height: number;
}

interface PresetTail {
  even: readonly number[];
  odd: readonly number[];
  vcselA: number;
  vcselB: number;
  mmA: number;
  mmB: number;
  rangeA: number;
  rangeB: number;
  calVcselStart: number;
  validPhaseHigh: number;
  phaseRtn: string;
  phaseRef: string;
  extraSequence?: number;
}

// ── the three distance-mode tails (vl53lx_api_preset_modes.c) ────────────────
const PRESET_TAIL: Record<DistanceMode, PresetTail> = {
  long: {
    even: [7, 0, 1, 2, 3, 4], odd: [0, 1, 2, 3, 4, 5],
    vcselA: 0x09, vcselB: 0x0b, mmA: 0x0021, mmB: 0x001b,
    rangeA: 0x0029, rangeB: 0x0022, calVcselStart: 0x09, validPhaseHigh: 0x88,
    phaseRtn: "tp_init_phase_rtn_hist_long", phaseRef: "tp_init_phase_ref_hist_long",
  },
  medium: {
    even: [7, 0, 1, 1, 2, 2], odd: [0, 1, 2, 1, 2, 3],
    vcselA: 0x05, vcselB: 0x07, mmA: 0x0036, mmB: 0x0028,
    rangeA: 0x0044, rangeB: 0x0033, calVcselStart: 0x05, validPhaseHigh: 0x48,
    phaseRtn: "tp_init_phase_rtn_hist_med", phaseRef: "tp_init_phase_ref_hist_med",
  },
  short: {
    even: [7, 7, 0, 1, 1, 1], odd: [0, 1, 1, 1, 2, 2],
    vcselA: 0x03, vcselB: 0x05, mmA: 0x0052, mmB: 0x0037,
    rangeA: 0x0066, rangeB: 0x0044, calVcselStart: 0x03, validPhaseHigh: 0x28,
    phaseRtn: "tp_init_phase_rtn_hist_short", phaseRef: "tp_init_phase_ref_hist_short",
    extraSequence: SEQUENCE_MM1_EN,
  },
};

const PHASECAL_KEY: Record<DistanceMode, string> = {
  short: "tp_phasecal_timeout_hist_short_us",
  medium: "tp_phasecal_timeout_hist_med_us",
  long: "tp_phasecal_timeout_hist_long_us",
};

function isDistanceMode(mode: string): mode is DistanceMode {
  return (DISTANCE_MODES as readonly string[]).includes(mode);
}

// ── the driver ───────────────────────────────────────────────────────────────
/**
 * The VL53LX BareDriver's ranging path, on top of `DeviceImage`. It owns what
 * the C driver keeps in `VL53LX_LLDriverData_t` and nowhere else: the factory
 * data that has no registers, the histogram config, the post-process config
 * and the timeouts.
 */
export class BareDriver {
  readonly img: DeviceImage;
  readonly nvm: NvmReader;
  readonly tuning: Record<string, number> = { ...TUNING };

  histCfg = new HistConfig();
  hpp = new HistPostProcessConfig();

  // Factory data with no register of its own (readP2pData).
  rtnGoodSpads: Uint8Array = new Uint8Array(32);
  opticalCentre: OpticalCentre | null = null;
  calPeakRateMap: CalPeakRateMap | null = null;
  addOffCalData: AdditionalOffsetCalData | null = null;
  fmtDmaxCal: FmtDmaxCal | null = null;
  mmRoi: MmRoi | null = null;
  result__osc_calibrate_val = 0;

  presetMode: DistanceMode = "medium";
  phasecalConfigTimeoutUs = 1000;
  mmConfigTimeoutUs = 2000;
  rangeConfigTimeoutUs = 13000;
  interMeasurementPeriodMs = 100;
  measurementMode = 0;
  readonly state = new LLState();
  // Carried from frame to frame: the C driver keeps one bin_data struct and
  // reads last frame's ambient out of it when it picks the bin sequence,
  // before this frame's ambient is estimated.
  private ambientEventsSum = 0;

  constructor(readonly p: BridgePlatform) {
    this.img = new DeviceImage(p);
    this.nvm = new NvmReader(p, this.img);
  }

  // ── VL53LX_read_p2p_data ──
  /** Pull the factory data: three register blocks and four NVM regions. */
  async readP2pData(): Promise<void> {
    await this.img.pull("static_nvm_managed");
    await this.img.pull("customer_nvm_managed");
    await this.img.pull("nvm_copy_data");
    this.rtnGoodSpads = rtnGoodSpads(this.img.nvm_copy_data);

    const customer = this.img.customer_nvm_managed.v;
    this.hpp.algo__crosstalk_compensation_plane_offset_kcps =
      customer.algo__crosstalk_compensation_plane_offset_kcps!;
    this.hpp.algo__crosstalk_compensation_x_plane_gradient_kcps =
      customer.algo__crosstalk_compensation_x_plane_gradient_kcps!;
    this.hpp.algo__crosstalk_compensation_y_plane_gradient_kcps =
      customer.algo__crosstalk_compensation_y_plane_gradient_kcps!;

    this.opticalCentre = await this.nvm.opticalCentre();
    this.calPeakRateMap = await this.nvm.calPeakRateMap();
    this.addOffCalData = await this.nvm.additionalOffsetCalData();

    // Our boards have no FMT offset calibration. ST's fallback: nominal
    // MM1/MM2 peak rates, and the effective SPAD counts worked out here.
    const cal = this.addOffCalData;
    const nvmCopy = this.img.nvm_copy_data.v;
    if (
      cal.result__mm_inner_peak_signal_count_rtn_mcps === 0 &&
      cal.result__mm_outer_peak_signal_count_rtn_mcps === 0
    ) {
      const [inner, outer] = calcMmEffectiveSpads(
        nvmCopy.roi_config__mode_roi_centre_spad!,
        nvmCopy.roi_config__mode_roi_xy_size!,
        0xc7, 0xff, this.rtnGoodSpads, RTN_SPAD_APERTURE_TRANSMISSION,
      );
      this.addOffCalData = {
        ...cal,
        result__mm_inner_peak_signal_count_rtn_mcps: 0x0080,
        result__mm_outer_peak_signal_count_rtn_mcps: 0x0180,
        result__mm_inner_actual_effective_spads: inner,
        result__mm_outer_actual_effective_spads: outer,
      };
    }

    const fmt = await this.nvm.fmtRangeResults();
    this.fmtDmaxCal = {
      ref__actual_effective_spads: fmt.result__actual_effective_rtn_spads,
      ref__peak_signal_count_rate_mcps: fmt.result__peak_signal_count_rate_rtn_mcps,
      ref__distance_mm: fmt.measured_distance_mm,
      ref_reflectance_pc: this.calPeakRateMap.cal_reflectance_pc || 0x0014,
      coverglass_transmission: 0x0100,
    };

    this.result__osc_calibrate_val = await this.p.rdWord(RESULT__OSC_CALIBRATE_VAL);

    const statNvm = this.img.static_nvm_managed.v;
    if (statNvm.osc_measured__fast_osc__frequency! < 0x1000) {
      statNvm.osc_measured__fast_osc__frequency = 0xbccc;
    }

    // VL53LX_get_mode_mitigation_roi
    const [y, x] = decodeRowCol(nvmCopy.roi_config__mode_roi_centre_spad!);
    const xySize = nvmCopy.roi_config__mode_roi_xy_size!;
    this.mmRoi = { x_centre: x, y_centre: y, width: xySize & 0x0f, height: xySize >> 4 };

    if (this.opticalCentre.x_centre === 0 && this.opticalCentre.y_centre === 0) {
      this.opticalCentre = { x_centre: x << 4, y_centre: y << 4 };
    }
  }

  // ── VL53LX_data_init + VL53LX_DataInit ──
  /**
   * Everything the L3CX API does between boot and the first start: read the
   * factory data, take the default preset mode and the default timing budget.
   */
  async dataInit(): Promise<void> {
    this.state.reset();
    this.hpp = new HistPostProcessConfig();
    await this.readP2pData();
    this.setPresetMode("medium", null, 1000);
    this.setMeasurementTimingBudgetUs(33333);
  }

  // ── preset modes ──
  /** VL53LX_set_preset_mode() for the three histogram ranging modes. */
  setPresetMode(
    mode: string = "medium",
    dssTargetMcps: number | null = null,
    interMeasurementPeriodMs: number | null = null,
  ): void {
    if (!isDistanceMode(mode)) throw new RangeError(`unknown distance mode '${mode}'`);

    this.presetMode = mode;
    const dss = dssTargetMcps ?? this.tuning.tp_dss_target_histo_mcps!;
    const imp = interMeasurementPeriodMs ?? this.interMeasurementPeriodMs;

    // Only the phasecal timeout differs between the three modes.
    const phasecalUs = this.tp(PHASECAL_KEY[mode]);
    const mmUs = this.tp("tp_mm_timeout_histo_us");
    const rangeUs = this.tp("tp_range_timeout_histo_us");

    this.state.reset();
    this.presetModeHistogramRanging();
    this.presetTail(PRESET_TAIL[mode]);

    this.img.static_config.v.dss_config__target_total_rate_mcps = dss;
    this.setTimeoutsUs(phasecalUs, mmUs, rangeUs);
    this.setInterMeasurementPeriodMs(imp);
    this.updateRangeOffset();
  }

  private tp(key: string): number {
    const v = this.tuning[key];
    if (v === undefined) throw new Error(`no tuning parameter ${key}`);
    return v;
  }

  /**
   * `VL53LX_get_device_results()` recomputes `hpp.range_offset_mm` every frame
   * under MM1_MM2_OFFSETS; its inputs are the factory data and the user ROI,
   * so once per preset is the same answer for less work.
   */
  private updateRangeOffset(): void {
    if (this.addOffCalData === null) return;
    const customer = this.img.customer_nvm_managed.v;
    const nvm = this.img.nvm_copy_data.v;
    const dynamic = this.img.dynamic_config.v;
    this.hpp.range_offset_mm = combineMm1Mm2Offsets(
      customer.mm_config__inner_offset_mm!,
      customer.mm_config__outer_offset_mm!,
      nvm.roi_config__mode_roi_centre_spad!,
      nvm.roi_config__mode_roi_xy_size!,
      dynamic.roi_config__user_roi_centre_spad!,
      dynamic.roi_config__user_roi_requested_global_xy_size!,
      this.addOffCalData,
      this.rtnGoodSpads,
      this.img.general_config.v.dss_config__aperture_attenuation!,
    );
  }

  /**
   * VL53LX_SetDistanceMode(): the preset mode, with the timeouts put back
   * afterwards (the preset alone would reset them to its own defaults).
   */
  setDistanceMode(mode: string): void {
    const phasecalUs = this.phasecalConfigTimeoutUs;
    const mmUs = this.mmConfigTimeoutUs;
    const rangeUs = this.rangeConfigTimeoutUs;
    this.setPresetMode(mode);
    this.setTimeoutsUs(phasecalUs, mmUs, rangeUs);
  }

  /** VL53LX_preset_mode_standard_ranging(): the base of every histogram mode. */
  private presetModeStandardRanging(): void {
    const s = this.img.static_config.v;
    const g = this.img.general_config.v;
    const t = this.img.timing_config.v;
    const d = this.img.dynamic_config.v;
    const sys = this.img.system_control.v;

    s.dss_config__target_total_rate_mcps = 0x0a00;
    s.debug__ctrl = 0x00;
    s.test_mode__ctrl = 0x00;
    s.clk_gating__ctrl = 0x00;
    s.nvm_bist__ctrl = 0x00;
    s.nvm_bist__num_nvm_words = 0x00;
    s.nvm_bist__start_address = 0x00;
    s.host_if__status = 0x00;
    s.pad_i2c_hv__config = 0x00;
    s.pad_i2c_hv__extsup_config = 0x00;
    s.gpio_hv_pad__ctrl = 0x00;
    s.gpio_hv_mux__ctrl =
      DEVICEINTERRUPTPOLARITY_ACTIVE_LOW | DEVICEGPIOMODE_OUTPUT_RANGE_AND_ERROR_INTERRUPTS;
    s.gpio__tio_hv_status = 0x02;
    s.gpio__fio_hv_status = 0x00;
    s.ana_config__spad_sel_pswidth = 0x02;
    s.ana_config__vcsel_pulse_width_offset = 0x08;
    s.ana_config__fast_osc__config_ctrl = 0x00;
    s.sigma_estimator__effective_pulse_width_ns = this.tp("tp_lite_sigma_est_pulse_width_ns");
    s.sigma_estimator__effective_ambient_width_ns = this.tp("tp_lite_sigma_est_amb_width_ns");
    s.sigma_estimator__sigma_ref_mm = this.tp("tp_lite_sigma_ref_mm");
    s.algo__crosstalk_compensation_valid_height_mm = 0x01;
    s.spare_host_config__static_config_spare_0 = 0x00;
    s.spare_host_config__static_config_spare_1 = 0x00;
    s.algo__range_ignore_threshold_mcps = 0x0000;
    s.algo__range_ignore_valid_height_mm = 0xff;
    s.algo__range_min_clip = this.tp("tp_lite_min_clip");
    s.algo__consistency_check__tolerance = this.tp("tp_consistency_lite_phase_tolerance");
    s.spare_host_config__static_config_spare_2 = 0x00;
    s.sd_config__reset_stages_msb = 0x00;
    s.sd_config__reset_stages_lsb = 0x00;

    g.gph_config__stream_count_update_value = 0x00;
    g.global_config__stream_divider = 0x00;
    g.system__interrupt_config_gpio = INTERRUPT_CONFIG_NEW_SAMPLE_READY;
    g.cal_config__vcsel_start = 0x0b;
    g.cal_config__repeat_rate = this.tp("tp_cal_repeat_rate");
    g.global_config__vcsel_width = 0x02;
    g.phasecal_config__timeout_macrop = 0x0d;
    g.phasecal_config__target = this.tp("tp_phasecal_target");
    g.phasecal_config__override = 0x00;
    g.dss_config__roi_mode_control = DEVICEDSSMODE__TARGET_RATE;
    g.system__thresh_rate_high = 0x0000;
    g.system__thresh_rate_low = 0x0000;
    g.dss_config__manual_effective_spads_select = 0x8c00;
    g.dss_config__manual_block_select = 0x00;
    g.dss_config__aperture_attenuation = 0x38;
    g.dss_config__max_spads_limit = 0xff;
    g.dss_config__min_spads_limit = 0x01;

    t.mm_config__timeout_macrop_a_hi = 0x00;
    t.mm_config__timeout_macrop_a_lo = 0x1a;
    t.mm_config__timeout_macrop_b_hi = 0x00;
    t.mm_config__timeout_macrop_b_lo = 0x20;
    t.range_config__timeout_macrop_a_hi = 0x01;
    t.range_config__timeout_macrop_a_lo = 0xcc;
    t.range_config__vcsel_period_a = 0x0b;
    t.range_config__timeout_macrop_b_hi = 0x01;
    t.range_config__timeout_macrop_b_lo = 0xf5;
    t.range_config__vcsel_period_b = 0x09;
    t.range_config__sigma_thresh = this.tp("tp_lite_med_sigma_thresh_mm");
    t.range_config__min_count_rate_rtn_limit_mcps = this.tp("tp_lite_med_min_count_rate_rtn_mcps");
    t.range_config__valid_phase_low = 0x08;
    t.range_config__valid_phase_high = 0x78;
    t.system__intermeasurement_period = 0x00000000;
    t.system__fractional_enable = 0x00;

    // Standard ranging writes the histogram config out byte by byte; every
    // histogram mode overwrites it, but this is the only definition of the
    // struct's initial state.
    const h = this.histCfg;
    h.low_amb_even_bin_0_1 = 0x07;
    h.low_amb_even_bin_2_3 = 0x21;
    h.low_amb_even_bin_4_5 = 0x43;
    h.low_amb_odd_bin_0_1 = 0x10;
    h.low_amb_odd_bin_2_3 = 0x32;
    h.low_amb_odd_bin_4_5 = 0x54;
    h.mid_amb_even_bin_0_1 = 0x07;
    h.mid_amb_even_bin_2_3 = 0x21;
    h.mid_amb_even_bin_4_5 = 0x43;
    h.mid_amb_odd_bin_0_1 = 0x10;
    h.mid_amb_odd_bin_2 = 0x02;
    h.mid_amb_odd_bin_3_4 = 0x43;
    h.mid_amb_odd_bin_5 = 0x05;
    h.user_bin_offset = 0x00;
    h.high_amb_even_bin_0_1 = 0x07;
    h.high_amb_even_bin_2_3 = 0x21;
    h.high_amb_even_bin_4_5 = 0x43;
    h.high_amb_odd_bin_0_1 = 0x10;
    h.high_amb_odd_bin_2_3 = 0x32;
    h.high_amb_odd_bin_4_5 = 0x54;
    h.amb_thresh_low = 0xffff;
    h.amb_thresh_high = 0xffff;
    h.spad_array_selection = 0x00;

    d.system__grouped_parameter_hold_0 = 0x01;
    d.system__thresh_high = 0x0000;
    d.system__thresh_low = 0x0000;
    d.system__enable_xtalk_per_quadrant = 0x00;
    d.system__seed_config = this.tp("tp_lite_seed_cfg");
    d.sd_config__woi_sd0 = 0x0b;
    d.sd_config__woi_sd1 = 0x09;
    d.sd_config__initial_phase_sd0 = this.tp("tp_init_phase_rtn_lite_med");
    d.sd_config__initial_phase_sd1 = this.tp("tp_init_phase_ref_lite_med");
    d.system__grouped_parameter_hold_1 = 0x01;
    d.sd_config__first_order_select = this.tp("tp_lite_first_order_select");
    d.sd_config__quantifier = this.tp("tp_lite_quantifier");
    d.roi_config__user_roi_centre_spad = 0xc7;
    d.roi_config__user_roi_requested_global_xy_size = 0xff;
    d.system__sequence_config =
      SEQUENCE_VHV_EN | SEQUENCE_PHASECAL_EN | SEQUENCE_DSS1_EN |
      SEQUENCE_DSS2_EN | SEQUENCE_MM2_EN | SEQUENCE_RANGE_EN;
    d.system__grouped_parameter_hold = 0x02;

    sys.system__stream_count_ctrl = 0x00;
    sys.firmware__enable = 0x01;
    sys.system__interrupt_clear = CLEAR_RANGE_INT;
    sys.system__mode_start =
      DEVICESCHEDULERMODE_STREAMING | DEVICEREADOUTMODE_SINGLE_SD |
      DEVICEMEASUREMENTMODE_BACKTOBACK;
  }

  /** VL53LX_preset_mode_histogram_ranging(): standard ranging, then histogram. */
  private presetModeHistogramRanging(): void {
    this.presetModeStandardRanging();

    this.img.static_config.v.dss_config__target_total_rate_mcps = 0x1400;
    this.histCfg.setBinSequence([7, 0, 1, 2, 3, 4], [0, 1, 2, 3, 4, 5]);

    const t = this.img.timing_config.v;
    const d = this.img.dynamic_config.v;
    t.range_config__vcsel_period_a = 0x09;
    t.range_config__vcsel_period_b = 0x0b;
    d.sd_config__woi_sd0 = 0x09;
    d.sd_config__woi_sd1 = 0x0b;
    t.mm_config__timeout_macrop_a_hi = 0x00;
    t.mm_config__timeout_macrop_a_lo = 0x20;
    t.mm_config__timeout_macrop_b_hi = 0x00;
    t.mm_config__timeout_macrop_b_lo = 0x1a;
    t.range_config__timeout_macrop_a_hi = 0x00;
    t.range_config__timeout_macrop_a_lo = 0x28;
    t.range_config__timeout_macrop_b_hi = 0x00;
    t.range_config__timeout_macrop_b_lo = 0x21;
    this.img.general_config.v.phasecal_config__timeout_macrop = 0xf5;

    this.hpp.valid_phase_low = 0x08;
    this.hpp.valid_phase_high = 0x88;

    this.histCfg.copyToStaticCfg(
      this.img.static_config, this.img.timing_config, this.img.dynamic_config,
    );
    d.system__sequence_config =
      SEQUENCE_VHV_EN | SEQUENCE_PHASECAL_EN | SEQUENCE_DSS1_EN |
      SEQUENCE_DSS2_EN | SEQUENCE_RANGE_EN;
    this.img.system_control.v.system__mode_start =
      DEVICESCHEDULERMODE_HISTOGRAM | DEVICEREADOUTMODE_DUAL_SD |
      DEVICEMEASUREMENTMODE_BACKTOBACK;
  }

  /** The shape all three histogram distance modes share. */
  private presetTail(c: PresetTail): void {
    const g = this.img.general_config.v;
    const t = this.img.timing_config.v;
    const d = this.img.dynamic_config.v;

    this.histCfg.setBinSequence(c.even, c.odd);
    this.histCfg.copyToStaticCfg(
      this.img.static_config, this.img.timing_config, this.img.dynamic_config,
    );

    t.range_config__vcsel_period_a = c.vcselA;
    t.range_config__vcsel_period_b = c.vcselB;
    t.mm_config__timeout_macrop_a_hi = c.mmA >> 8;
    t.mm_config__timeout_macrop_a_lo = c.mmA & 0xff;
    t.mm_config__timeout_macrop_b_hi = c.mmB >> 8;
    t.mm_config__timeout_macrop_b_lo = c.mmB & 0xff;
    t.range_config__timeout_macrop_a_hi = c.rangeA >> 8;
    t.range_config__timeout_macrop_a_lo = c.rangeA & 0xff;
    t.range_config__timeout_macrop_b_hi = c.rangeB >> 8;
    t.range_config__timeout_macrop_b_lo = c.rangeB & 0xff;

    g.cal_config__vcsel_start = c.calVcselStart;
    g.phasecal_config__timeout_macrop = 0xf5;

    d.sd_config__woi_sd0 = c.vcselA;
    d.sd_config__woi_sd1 = c.vcselB;
    d.sd_config__initial_phase_sd0 = this.tp(c.phaseRtn);
    d.sd_config__initial_phase_sd1 = this.tp(c.phaseRef);
    d.system__sequence_config =
      SEQUENCE_VHV_EN | SEQUENCE_PHASECAL_EN | SEQUENCE_DSS1_EN |
      SEQUENCE_DSS2_EN | (c.extraSequence ?? 0) | SEQUENCE_RANGE_EN;

    this.hpp.valid_phase_low = 0x08;
    this.hpp.valid_phase_high = c.validPhaseHigh;

    this.img.system_control.v.system__mode_start =
      DEVICESCHEDULERMODE_HISTOGRAM | DEVICEREADOUTMODE_DUAL_SD |
      DEVICEMEASUREMENTMODE_BACKTOBACK;
  }

  // ── timeouts ──
  setTimeoutsUs(phasecalUs: number, mmUs: number, rangeUs: number): void {
    this.phasecalConfigTimeoutUs = phasecalUs;
    this.mmConfigTimeoutUs = mmUs;
    this.rangeConfigTimeoutUs = rangeUs;
    calcTimeoutRegisterValues(
      phasecalUs, mmUs, rangeUs,
      this.img.static_nvm_managed.v.osc_measured__fast_osc__frequency!,
      this.img.general_config, this.img.timing_config,
    );
  }

  setInterMeasurementPeriodMs(periodMs: number): void {
    if (this.result__osc_calibrate_val === 0) {
      throw new Error("result__osc_calibrate_val is 0");
    }
    this.interMeasurementPeriodMs = periodMs;
    this.img.timing_config.v.system__intermeasurement_period =
      periodMs * this.result__osc_calibrate_val;
  }

  /**
   * VL53LX_SetMeasurementTimingBudgetMicroSeconds(): the range timeout is one
   * sixth of the budget after a fixed guard comes off.
   */
  setMeasurementTimingBudgetUs(budgetUs: number): void {
    if (!(TIMING_GUARD_US < budgetUs && budgetUs <= 10000000)) {
      throw new RangeError(`timing budget ${budgetUs} us out of range`);
    }
    const rangeUs = floorDiv(budgetUs - TIMING_GUARD_US, TIMING_DIVISOR);
    if (rangeUs * TIMING_DIVISOR > FDA_MAX_TIMING_BUDGET_US) {
      throw new RangeError(`timing budget ${budgetUs} us out of range`);
    }
    this.setTimeoutsUs(this.phasecalConfigTimeoutUs, this.mmConfigTimeoutUs, rangeUs);
  }

  // ── the firmware patch StartMeasurement loads ──
  /** VL53LX_load_patch(): a six-instruction phasecal patch. */
  async loadPatch(): Promise<void> {
    const powerMap: Record<number, number> = { 0: 0x00, 1: 0x10, 2: 0x20, 3: 0x40 };
    const power = powerMap[this.tp("tp_phasecal_patch_power")] ?? 0x00;
    await this.p.wrByte(FIRMWARE__ENABLE, 0x00);
    await this.enablePowerforce();
    await this.p.wrMulti(PATCH__OFFSET_0, Uint8Array.of(0x29, 0xc9, 0x0e, 0x40, 0x28, power));
    await this.p.wrMulti(PATCH__ADDRESS_0, Uint8Array.of(0x03, 0x6d, 0x03, 0x6f, 0x07, 0x29));
    await this.p.wrMulti(PATCH__JMP_ENABLES, Uint8Array.of(0x00, 0x07));
    await this.p.wrMulti(PATCH__DATA_ENABLES, Uint8Array.of(0x00, 0x07));
    await this.p.wrByte(PATCH__CTRL, 0x01);
    await this.p.wrByte(FIRMWARE__ENABLE, 0x01);
  }

  async unloadPatch(): Promise<void> {
    await this.p.wrByte(FIRMWARE__ENABLE, 0x00);
    await this.disablePowerforce();
    await this.p.wrByte(PATCH__CTRL, 0x00);
    await this.p.wrByte(FIRMWARE__ENABLE, 0x01);
  }

  private async enablePowerforce(): Promise<void> {
    await this.p.wrByte(POWER_MANAGEMENT__GO1_POWER_FORCE, 0x01);
    await this.p.sleepMs(1); // 250 us settling, rounded up
  }

  private async disablePowerforce(): Promise<void> {
    await this.p.wrByte(POWER_MANAGEMENT__GO1_POWER_FORCE, 0x00);
  }

  // ── VL53LX_init_and_start_range ──
  /**
   * Write the configuration and start. `blocks` is ST's config level: all of
   * them to start, general_config onwards to clear the interrupt.
   */
  async initAndStartRange(blocks: readonly BlockName[] = RANGE_START_BLOCKS): Promise<void> {
    const dynamic = this.img.dynamic_config.v;
    const system = this.img.system_control.v;

    this.measurementMode = DEVICEMEASUREMENTMODE_BACKTOBACK;
    system.system__mode_start =
      (system.system__mode_start! & DEVICEMEASUREMENTMODE_STOP_MASK) | this.measurementMode;
    system.system__interrupt_clear = CLEAR_RANGE_INT;

    const gphId = this.state.cfg_gph_id;
    dynamic.system__grouped_parameter_hold_0 = gphId | 0x01;
    dynamic.system__grouped_parameter_hold_1 = gphId | 0x01;
    dynamic.system__grouped_parameter_hold = gphId;

    await this.img.pushRange(blocks);

    this.state.updateRd(system.system__mode_start, dynamic.system__grouped_parameter_hold);
    this.state.updateCfg(system.system__mode_start);
  }

  /**
   * VL53LX_clear_interrupt_and_enable_next_range(): in histogram mode the
   * interrupt is cleared by rewriting general_config onwards.
   */
  clearInterruptAndStartNextRange(): Promise<void> {
    return this.initAndStartRange([
      "general_config", "timing_config", "dynamic_config", "system_control",
    ]);
  }

  async stopRange(): Promise<void> {
    const system = this.img.system_control.v;
    system.system__mode_start =
      (system.system__mode_start! & DEVICEMEASUREMENTMODE_STOP_MASK) | DEVICEMEASUREMENTMODE_ABORT;
    await this.img.push("system_control");
    system.system__mode_start = system.system__mode_start & DEVICEMEASUREMENTMODE_STOP_MASK;
    this.state.reset();
  }

  // ── VL53LX_get_histogram_bin_data ──
  /**
   * One frame: the 83-byte block decoded into bins plus the metadata the
   * post-processing needs. Pure (no bus access): the caller reads or receives
   * the block. Stateful: carries the ambient sum to the next frame.
   */
  getHistogramBinData(raw: Uint8Array): HistogramBinData {
    if (raw.length < HISTOGRAM_BIN_DATA_I2C_SIZE_BYTES) {
      throw new RangeError(
        `histogram frame is ${raw.length} bytes, need ${HISTOGRAM_BIN_DATA_I2C_SIZE_BYTES}`,
      );
    }
    const buf = raw.slice();
    const off = HISTOGRAM_BIN_DATA_I2C_INDEX;

    const d = new HistogramBinData();
    d.result__interrupt_status = buf[0]!;
    d.result__range_status = buf[1]!;
    d.result__report_status = buf[2]!;
    d.result__stream_count = buf[3]!;
    d.result__dss_actual_effective_spads = (buf[4]! << 8) | buf[5]!;
    const rp = PHASECAL_RESULT__REFERENCE_PHASE - off;
    d.phasecal_result__reference_phase = (buf[rp]! << 8) | buf[rp + 1]!;
    d.phasecal_result__vcsel_start = buf[PHASECAL_RESULT__VCSEL_START - off]!;

    // Bin 23 does not fit in its three bytes: the low byte is carried in a
    // separate MSB/LSB pair and patched back in before the bins are read.
    buf[RESULT__HISTOGRAM_BIN_23_0 - off] =
      ((buf[RESULT__HISTOGRAM_BIN_23_0_MSB - off]! << 2) +
        buf[RESULT__HISTOGRAM_BIN_23_0_LSB - off]!) & 0xff;
    const base = RESULT__HISTOGRAM_BIN_0_2 - off;
    d.bin_data = [];
    for (let i = 0; i < HISTOGRAM_BUFFER_SIZE; i++) {
      const o = base + 3 * i;
      d.bin_data.push(buf[o]! * 65536 + buf[o + 1]! * 256 + buf[o + 2]!);
    }

    const g = this.img.general_config.v;
    const s = this.img.static_config.v;
    const t = this.img.timing_config.v;
    const dyn = this.img.dynamic_config.v;

    d.cal_config__vcsel_start = g.cal_config__vcsel_start!;
    d.vcsel_width = shl(g.global_config__vcsel_width!, 4) + s.ana_config__vcsel_pulse_width_offset!;
    d.fast_osc_frequency = this.img.static_nvm_managed.v.osc_measured__fast_osc__frequency!;
    d.roi_config__user_roi_centre_spad = dyn.roi_config__user_roi_centre_spad!;
    d.roi_config__user_roi_requested_global_xy_size =
      dyn.roi_config__user_roi_requested_global_xy_size!;

    d.zone_id = 0;
    d.bin_seq = this.histCfg.binSequence(this.state.rd_stream_count, this.ambientEventsSum);

    let encoded: number;
    if (this.state.rd_timing_status === 0) {
      encoded = shl(t.range_config__timeout_macrop_a_hi!, 8) + t.range_config__timeout_macrop_a_lo!;
      d.vcsel_period = t.range_config__vcsel_period_a!;
    } else {
      encoded = shl(t.range_config__timeout_macrop_b_hi!, 8) + t.range_config__timeout_macrop_b_lo!;
      d.vcsel_period = t.range_config__vcsel_period_b!;
    }

    d.number_of_ambient_bins = 4 * d.bin_seq.filter((x) => (x & 0x07) === 0x07).length;
    d.total_periods_elapsed = decodeTimeout(encoded);
    d.peak_duration_us = durationMaths(
      calcPllPeriodUs(d.fast_osc_frequency), d.vcsel_width,
      RANGING_WINDOW_VCSEL_PERIODS, d.total_periods_elapsed + 1,
    );
    d.woi_duration_us = 0;

    // VL53LX_hist_calc_zero_distance_phase
    const period = 2048 * decodeVcselPeriod(d.vcsel_period);
    const phase =
      period + d.phasecal_result__reference_phase +
      2048 * d.phasecal_result__vcsel_start - 2048 * d.cal_config__vcsel_start;
    d.zero_distance_phase = period ? pymod(phase, period) : 0;

    // VL53LX_hist_estimate_ambient_from_ambient_bins
    if (d.number_of_ambient_bins > 0) {
      d.number_of_ambient_samples = d.number_of_ambient_bins;
      let sum = 0;
      for (const x of d.bin_data.slice(0, d.number_of_ambient_bins)) sum += x;
      d.ambient_events_sum = sum;
      d.ambient_per_bin = floorDiv(
        d.ambient_events_sum + floorDiv(d.number_of_ambient_bins, 2),
        d.number_of_ambient_bins,
      );
    }
    this.ambientEventsSum = d.ambient_events_sum;
    return d;
  }
}
