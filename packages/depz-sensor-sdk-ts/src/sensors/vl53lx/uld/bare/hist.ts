/**
 * uld/bare/hist — histogram post-processing: 24 bins → targets.
 * Mirror of the Python `depz_sensor_sdk.vl53lx.uld.bare.hist` (a port of
 * `VL53LX_hist_process_data()`; ST ships the C obfuscated, the C names are in
 * the comments next to ours).
 *
 * In one paragraph: fold repeated bin codes together, estimate the ambient
 * floor, drop the ambient-only bins, work out per-bin detection thresholds,
 * and mark every bin above its threshold. Runs of marked bins are pulses.
 * Each pulse is isolated, filtered with a three-tap window, and the zero
 * crossing between the two halves is interpolated to a sub-bin phase. Phase
 * converts to millimetres against `zero_distance_phase`; event sums convert
 * to rates; the filter sums to sigma. `FrameHistory` compares consecutive
 * frames (phase and event consistency), which is what turns "no wrap check
 * done" into "valid" or "wrapped target".
 *
 * Not here (as in Python): crosstalk compensation, the ambient dmax estimate
 * (a stub on purpose — our boards ship the FMT region blank) and the
 * crosstalk-monitor consistency check.
 *
 * **Integers.** Python floors (`//`), C truncates (`cdiv`); both are kept
 * exactly where the Python has them. Three places can leave 2^53 and run in
 * BigInt: the sigma estimate (64-bit saturation ceilings), the phase
 * interpolation (4096² scaling) and the per-SPAD / events-consistency maths.
 */

import { measurement, type Measurement, type Target } from "../base.js";
import {
  DEVICESTATE_RANGING_OUTPUT_DATA,
  HISTOGRAM_BUFFER_SIZE,
  RANGING_WINDOW_VCSEL_PERIODS,
  calcPllPeriodUs,
  decodeVcselPeriod,
  durationMaths,
  type HistPostProcessConfig,
  type HistogramBinData,
} from "./core.js";
import {
  babs,
  bcdiv,
  bfloorDiv,
  bmin,
  cdiv,
  floorDiv,
  isqrtBig,
  isqrtNum,
  pymod,
  shl,
  shr,
} from "./imath.js";

// vl53lx_hist_structs.h / vl53lx_platform_user_config.h
export const MAX_BIN_SEQUENCE_LENGTH = 6;
export const MAX_BIN_SEQUENCE_CODE = 15;
export const MAX_PULSES = 8; // VL53LX_D_001
export const MAX_RANGE_RESULTS = 4;

// vl53lx_ll_device.h
export const MAX_ALLOWED_PHASE = 0xffff;
export const SPAD_TOTAL_COUNT_MAX = 2 ** 29 - 1;
export const SPAD_TOTAL_COUNT_RES_THRES = 2 ** 24;
export const SPEED_OF_LIGHT_IN_AIR_DIV_8 = 299704 >> 3;

// vl53lx_sigma_estimate.h — the saturation ceilings of the sigma estimate.
export const SIGMA_INVALID = 0xffff; // VL53LX_D_002
const D_003 = 0xffffffn;
const D_004 = 0xffffffffffffffn;
const D_005 = 0x7fffffffffn;
const D_006 = 0x7fffffffffffffffn;
const D_007 = 0xffffffffn;

// VL53LX_DeviceError (vl53lx_ll_device.h), the few the histogram path sets.
export const DEVICEERROR_NOUPDATE = 0;
export const DEVICEERROR_RANGEPHASECHECK = 5;
export const DEVICEERROR_SIGMATHRESHOLDCHECK = 6;
export const DEVICEERROR_PHASECONSISTENCY = 7;
export const DEVICEERROR_RANGECOMPLETE = 9;
export const DEVICEERROR_RANGECOMPLETE_NO_WRAP_CHECK = 19;
export const DEVICEERROR_EVENTCONSISTENCY = 20;
export const DEVICEERROR_RANGECOMPLETE_MERGED_PULSE = 22;
export const DEVICEERROR_PREV_RANGE_NO_TARGETS = 23;

// VL53LX_HIST_TARGET_ORDER__*
export const HIST_TARGET_ORDER_STRONGEST_FIRST = 1;
export const HIST_TARGET_ORDER_CLOSEST_FIRST = 2;

// VL53LX_RangeStatus (vl53lx_def.h) — the user-facing status.
export const RANGESTATUS_RANGE_VALID = 0;
export const RANGESTATUS_SIGMA_FAIL = 1;
export const RANGESTATUS_OUTOFBOUNDS_FAIL = 4;
export const RANGESTATUS_RANGE_VALID_NO_WRAP_CHECK = 6;
export const RANGESTATUS_WRAP_TARGET_FAIL = 7;
export const RANGESTATUS_RANGE_VALID_MERGED_PULSE = 11;
export const RANGESTATUS_TARGET_PRESENT_LACK_OF_SIGNAL = 12;
export const RANGESTATUS_NONE = 255;

// ── C arithmetic ─────────────────────────────────────────────────────────────
/** VL53LX_isqrt() — floor of the square root, on a uint32 argument. */
export function isqrt(num: number): number {
  return isqrtNum(pymod(num, 2 ** 32));
}

/** VL53LX_calc_pll_period_mm(). */
export function calcPllPeriodMm(fastOscFrequency: number): number {
  const pllPeriodUs = calcPllPeriodUs(fastOscFrequency);
  const pllPeriodMm = SPEED_OF_LIGHT_IN_AIR_DIV_8 * shr(pllPeriodUs, 2);
  return shr(pllPeriodMm + 2 ** 15, 16);
}

/** VL53LX_rate_maths(): events over a duration → count rate in 9.7 Mcps. */
export function rateMaths(events: number, timeUs: number): number {
  let tmp = 0;
  if (events > SPAD_TOTAL_COUNT_MAX) tmp = SPAD_TOTAL_COUNT_MAX;
  else if (events > 0) tmp = events;

  const fracBits = events > SPAD_TOTAL_COUNT_RES_THRES ? 3 : 7;
  if (timeUs > 0) tmp = floorDiv(shl(tmp, fracBits) + floorDiv(timeUs, 2), timeUs);
  if (events > SPAD_TOTAL_COUNT_RES_THRES) tmp = shl(tmp, 4);
  return Math.min(tmp, 0xffff);
}

/** VL53LX_rate_per_spad_maths(). */
export function ratePerSpadMaths(
  fracBits: number,
  peakCountRate: number,
  numSpads: number,
  maxOutputValue: number,
): number {
  let tmp: number;
  if (numSpads > 0) {
    tmp = shl(shl(peakCountRate, 8), fracBits);
    tmp = floorDiv(tmp + floorDiv(numSpads, 2), numSpads);
  } else {
    tmp = shl(peakCountRate, fracBits);
  }
  return Math.min(tmp, maxOutputValue);
}

/** VL53LX_events_per_spad_maths() → kcps per SPAD (BigInt: `total << 11`). */
export function eventsPerSpadMaths(events: number, numSpads: number, duration: number): number {
  let total = 0n;
  if (numSpads !== 0) total = bfloorDiv(BigInt(events) * 1000n * 256n, BigInt(numSpads));
  let perSpad: bigint;
  if (duration > 0) {
    const d = BigInt(duration);
    perSpad = bfloorDiv((total << 11n) + d / 2n, d);
  } else {
    perSpad = total << 11n;
  }
  return Number(perSpad & 0xffffffffn);
}

/** VL53LX_range_maths(): phase → millimetres. */
export function rangeMaths(
  fastOscFrequency: number,
  phase: number,
  zeroDistancePhase: number,
  fractionalBits: number,
  gainFactor: number,
  rangeOffsetMm: number,
): number {
  const pllPeriodUs = calcPllPeriodUs(fastOscFrequency);

  let tmp = phase - zeroDistancePhase;
  tmp = tmp * pllPeriodUs;
  tmp = cdiv(tmp, 2 ** 9);
  tmp = tmp * SPEED_OF_LIGHT_IN_AIR_DIV_8;
  tmp = cdiv(tmp, 2 ** 22);

  let rangeMm = tmp + rangeOffsetMm;
  rangeMm *= gainFactor;
  rangeMm += 0x0400;
  rangeMm = cdiv(rangeMm, 0x0800);

  if (fractionalBits === 0) {
    const rangeMm10 = cdiv(rangeMm * 10, 2 ** 2);
    if (Math.abs(rangeMm10 - cdiv(rangeMm10, 10) * 10) < 5) rangeMm = cdiv(rangeMm10, 10);
    else rangeMm = cdiv(rangeMm10, 10) + 1;
  } else if (fractionalBits === 1) {
    rangeMm = cdiv(rangeMm, 2 ** 1);
  }
  return rangeMm;
}

// ── results ──────────────────────────────────────────────────────────────────
/** `VL53LX_range_data_t`: one target. C field names kept. */
export class RangeData {
  range_id = 0;
  start_bin = 0; // VL53LX_p_012, window start
  first_bin = 0; // VL53LX_p_019, first bin over threshold
  peak_bin = 0; // VL53LX_p_023
  last_bin = 0; // VL53LX_p_024
  end_bin = 0; // VL53LX_p_013
  width_bins = 0; // VL53LX_p_025, bins over threshold
  window_bins = 0; // VL53LX_p_029, bins in the window
  vcsel_width = 0;
  fast_osc_frequency = 0;
  zero_distance_phase = 0;
  spads = 0; // VL53LX_p_004
  total_periods_elapsed = 0;
  peak_duration_us = 0;
  woi_duration_us = 0;
  ambient_events = 0; // VL53LX_p_016
  total_events = 0; // VL53LX_p_017
  signal_events = 0; // VL53LX_p_010
  peak_signal_count_rate_mcps = 0;
  avg_signal_count_rate_mcps = 0;
  ambient_count_rate_mcps = 0;
  total_rate_per_spad_mcps = 0;
  signal_events_per_spad_kcps = 0; // VL53LX_p_009
  sigma = 0; // VL53LX_p_002, 9.7 fixed point mm
  phase_start = 0; // VL53LX_p_026
  phase_mean = 0; // VL53LX_p_011
  phase_end = 0; // VL53LX_p_027
  min_range_mm = 0;
  median_range_mm = 0;
  max_range_mm = 0;
  range_status = DEVICEERROR_NOUPDATE;

  get sigma_mm(): number {
    return shr(this.sigma, 7);
  }

  /** peak_signal_count_rate_mcps is 9.7 Mcps. */
  get signal_kcps(): number {
    return floorDiv(this.peak_signal_count_rate_mcps * 1000, 128);
  }

  get ambient_kcps(): number {
    return floorDiv(this.ambient_count_rate_mcps * 1000, 128);
  }
}

/** `VL53LX_range_results_t`, reduced to the single-zone case. */
export class RangeResults {
  stream_count = 0;
  targets: RangeData[] = []; // VL53LX_p_003, active_results entries
  ambient_dmax_mm: number[] = [0, 0, 0, 0, 0]; // VL53LX_p_022
  wrap_dmax_mm = 0;
}

/** `VL53LX_hist_pulse_data_t`: one run of bins above the threshold. */
export class PulseData {
  start_bin = 0; // VL53LX_p_012
  first_bin = 0; // VL53LX_p_019
  peak_bin = 0xff; // VL53LX_p_023
  last_bin = 0; // VL53LX_p_024
  end_bin = 0; // VL53LX_p_013
  width_bins = 0; // VL53LX_p_025
  filter_woi = 0; // VL53LX_p_051, filter half width
  ambient_events = 0; // VL53LX_p_016
  total_events = 0; // VL53LX_p_017
  signal_events = 0; // VL53LX_p_010
  phase_start = 0; // VL53LX_p_026
  phase_mean = 0; // VL53LX_p_011
  phase_end = 0; // VL53LX_p_027
  sigma = 0; // VL53LX_p_002
}

const zeros = (n: number): number[] => new Array<number>(n).fill(0);

/** `VL53LX_hist_gen3_algo_private_data_t` (reset = VL53LX_f_003). */
class Gen3Algo {
  first_bin = 0; // VL53LX_p_019
  buffer_size = HISTOGRAM_BUFFER_SIZE; // VL53LX_p_020
  bins_in_data = 0; // VL53LX_p_021
  vcsel_period = 0; // VL53LX_p_030, bins per period
  bins_over_threshold = 0; // VL53LX_p_039
  ambient_per_bin = 0; // VL53LX_p_028
  ambient_threshold = 0; // VL53LX_p_031
  over_threshold = zeros(HISTOGRAM_BUFFER_SIZE); // VL53LX_p_040
  pulse_mask = zeros(HISTOGRAM_BUFFER_SIZE); // VL53LX_p_041
  pulse_no = zeros(HISTOGRAM_BUFFER_SIZE); // VL53LX_p_042
  threshold = zeros(HISTOGRAM_BUFFER_SIZE); // VL53LX_p_052
  first_rising_bin = 0; // VL53LX_p_044
  max_pulses = MAX_PULSES; // VL53LX_p_045
  pulse_count = 0; // VL53LX_p_046
  pulses: PulseData[] = Array.from({ length: MAX_PULSES }, () => new PulseData());
  bins: HistogramBinData | null = null; // VL53LX_p_006
  xtalk!: HistogramBinData; // VL53LX_p_047
  pulse_amb!: HistogramBinData; // VL53LX_p_048
  pulse_zero!: HistogramBinData; // VL53LX_p_049
  pulse_xtalk!: HistogramBinData; // VL53LX_p_050
}

/** `VL53LX_hist_gen4_algo_filtered_data_t`. */
class Filtered {
  a = zeros(HISTOGRAM_BUFFER_SIZE); // VL53LX_p_007
  b = zeros(HISTOGRAM_BUFFER_SIZE); // VL53LX_p_032
  c = zeros(HISTOGRAM_BUFFER_SIZE); // VL53LX_p_001
  left = zeros(HISTOGRAM_BUFFER_SIZE); // VL53LX_p_053
  right = zeros(HISTOGRAM_BUFFER_SIZE); // VL53LX_p_054
  is_peak = zeros(HISTOGRAM_BUFFER_SIZE); // VL53LX_p_040
}

function sumOf(xs: readonly number[]): number {
  let s = 0;
  for (const x of xs) s += x;
  return s;
}

// ── bin housekeeping (vl53lx_hist_core.c, vl53lx_core_support.c) ─────────────
/** VL53LX_f_031: fold the six bin-sequence codes together (repeats averaged). */
export function averageRepeatedBins(src: HistogramBinData): HistogramBinData {
  const dst = src.clone();
  dst.bins_in_data = 0;
  dst.bin_seq = new Array<number>(MAX_BIN_SEQUENCE_LENGTH).fill(MAX_BIN_SEQUENCE_CODE + 1);
  dst.bin_data = zeros(dst.number_of_bins);

  const initialIndex = zeros(MAX_BIN_SEQUENCE_CODE + 2);
  const repeatCount = zeros(MAX_BIN_SEQUENCE_CODE + 2);
  let seqLength = 0;

  for (let lc = 0; lc < MAX_BIN_SEQUENCE_LENGTH; lc++) {
    const binCfg = src.bin_seq[lc]!;
    if (repeatCount[binCfg] === 0) {
      initialIndex[binCfg] = seqLength * 4;
      dst.bin_seq[seqLength] = binCfg;
      seqLength += 1;
    }
    repeatCount[binCfg]! += 1;

    const base = initialIndex[binCfg]!;
    for (let i = 0; i < 4; i++) dst.bin_data[base + i]! += src.bin_data[lc * 4 + i]!;
  }

  dst.bin_rep = dst.bin_seq.map((c) => (c <= MAX_BIN_SEQUENCE_CODE ? repeatCount[c]! : 0));
  dst.bins_in_data = seqLength * 4;

  for (let code = 0; code <= MAX_BIN_SEQUENCE_CODE; code++) {
    const reps = repeatCount[code]!;
    if (reps > 0) {
      const base = initialIndex[code]!;
      for (let i = 0; i < 4; i++) {
        dst.bin_data[base + i] = floorDiv(dst.bin_data[base + i]! + floorDiv(reps, 2), reps);
      }
    }
  }

  // Codes 7 and 15 are the ambient-only sequence entries.
  dst.number_of_ambient_bins = repeatCount[7] || repeatCount[15] ? 4 : 0;
  return dst;
}

/** VL53LX_hist_calc_zero_distance_phase */
export function calcZeroDistancePhase(bins: HistogramBinData): void {
  const period = 2048 * decodeVcselPeriod(bins.vcsel_period);
  const phase =
    period + bins.phasecal_result__reference_phase +
    2048 * bins.phasecal_result__vcsel_start - 2048 * bins.cal_config__vcsel_start;
  bins.zero_distance_phase = period ? pymod(phase, period) : 0;
}

/** VL53LX_hist_estimate_ambient_from_thresholded_bins() */
export function estimateAmbientFromThresholdedBins(
  ambientThresholdSigma: number,
  bins: HistogramBinData,
): void {
  const data = bins.bin_data.slice(0, bins.bins_in_data);
  bins.min_bin_value = data.length ? Math.min(...data) : 0;
  bins.max_bin_value = data.length ? Math.max(...data) : 0;

  let threshold = isqrt(bins.min_bin_value);
  threshold *= ambientThresholdSigma;
  threshold += 0x07;
  threshold = shr(threshold, 4);
  threshold += bins.min_bin_value;

  bins.number_of_ambient_samples = 0;
  bins.ambient_events_sum = 0;
  for (const value of data) {
    if (value < threshold) {
      bins.ambient_events_sum += value;
      bins.number_of_ambient_samples += 1;
    }
  }

  if (bins.number_of_ambient_samples > 0) {
    bins.ambient_per_bin = cdiv(
      bins.ambient_events_sum + floorDiv(bins.number_of_ambient_samples, 2),
      bins.number_of_ambient_samples,
    );
  }
}

/** VL53LX_hist_estimate_ambient_from_ambient_bins() */
export function estimateAmbientFromAmbientBins(bins: HistogramBinData): void {
  if (bins.number_of_ambient_bins > 0) {
    bins.number_of_ambient_samples = bins.number_of_ambient_bins;
    bins.ambient_events_sum = sumOf(bins.bin_data.slice(0, bins.number_of_ambient_bins));
    bins.ambient_per_bin = cdiv(
      bins.ambient_events_sum + floorDiv(bins.number_of_ambient_bins, 2),
      bins.number_of_ambient_bins,
    );
  }
}

/** VL53LX_hist_remove_ambient_bins: drop the ambient-only bins off the front. */
export function removeAmbientBins(bins: HistogramBinData): void {
  if ((bins.bin_seq[0]! & 0x07) === 0x07) {
    const keptSeq: number[] = [];
    const keptRep: number[] = [];
    for (let lc = 0; lc < MAX_BIN_SEQUENCE_LENGTH; lc++) {
      if ((bins.bin_seq[lc]! & 0x07) !== 0x07) {
        keptSeq.push(bins.bin_seq[lc]!);
        keptRep.push(bins.bin_rep[lc]!);
      }
    }
    const pad = MAX_BIN_SEQUENCE_LENGTH - keptSeq.length;
    bins.bin_seq = keptSeq.concat(new Array<number>(pad).fill(MAX_BIN_SEQUENCE_CODE + 1));
    bins.bin_rep = keptRep.concat(zeros(pad));
  }

  const n = bins.number_of_ambient_bins;
  if (n > 0) {
    bins.bin_data = bins.bin_data.slice(n).concat(zeros(n));
    bins.bins_in_data -= n;
    bins.number_of_ambient_bins = 0;
  }
}

/** VL53LX_f_022: the three-tap window sums around `binIndex`. */
function woiSums(
  binIndex: number,
  filterWoi: number,
  bins: HistogramBinData,
): [number, number, number] {
  let a = 0;
  const b = bins.bin_data[binIndex]!;
  let c = 0;
  for (let w = 0; w < shl(filterWoi, 1) + 1; w++) {
    const j = pymod(binIndex + w + bins.bins_in_data - filterWoi, bins.bins_in_data);
    if (w < filterWoi) a += bins.bin_data[j]!;
    else if (w > filterWoi) c += bins.bin_data[j]!;
  }
  return [a, b, c];
}

/** VL53LX_f_018: event sums → count rates. */
function calcRates(
  target: RangeData,
  vcselWidth: number,
  fastOscFrequency: number,
  totalPeriodsElapsed: number,
  spads: number,
  mergeNb: number,
): boolean {
  target.vcsel_width = vcselWidth;
  target.fast_osc_frequency = fastOscFrequency;
  target.total_periods_elapsed = totalPeriodsElapsed;
  target.spads = spads;

  if (fastOscFrequency === 0 || totalPeriodsElapsed === 0) return false;

  const pllPeriodUs = calcPllPeriodUs(fastOscFrequency);
  const periodsElapsed = totalPeriodsElapsed + 1;

  target.peak_duration_us = durationMaths(
    pllPeriodUs, vcselWidth, RANGING_WINDOW_VCSEL_PERIODS, periodsElapsed,
  );
  target.woi_duration_us = durationMaths(
    pllPeriodUs, shl(target.window_bins, 4), RANGING_WINDOW_VCSEL_PERIODS, periodsElapsed,
  );

  target.peak_signal_count_rate_mcps = rateMaths(target.signal_events, target.peak_duration_us);
  target.avg_signal_count_rate_mcps = rateMaths(target.signal_events, target.woi_duration_us);
  target.ambient_count_rate_mcps = rateMaths(target.ambient_events, target.woi_duration_us);

  let countRateTotal = target.peak_signal_count_rate_mcps + target.ambient_count_rate_mcps;
  if (mergeNb > 1) countRateTotal = floorDiv(countRateTotal, mergeNb);

  target.total_rate_per_spad_mcps = ratePerSpadMaths(0x06, countRateTotal, spads, 0xffff);
  target.signal_events_per_spad_kcps = eventsPerSpadMaths(
    target.signal_events, spads, target.peak_duration_us,
  );
  return true;
}

/** VL53LX_f_019 */
function calcRanges(gainFactor: number, rangeOffsetMm: number, target: RangeData): void {
  const r = (phase: number): number =>
    rangeMaths(target.fast_osc_frequency, phase, target.zero_distance_phase, 0, gainFactor,
      rangeOffsetMm);
  target.min_range_mm = r(target.phase_start);
  target.median_range_mm = r(target.phase_mean);
  target.max_range_mm = r(target.phase_end);
}

// ── sigma (vl53lx_sigma_estimate.c) ──────────────────────────────────────────
/**
 * VL53LX_f_023: the width of the pulse, in 9.7 fixed point millimetres. All
 * in BigInt: the intermediates run up to the 63-bit saturation ceilings.
 */
function sigmaEstimate(
  sigmaRefMm: number,
  a: number, b: number, c: number,
  aZp: number, cZp: number,
  bx: number, axZp: number, cxZp: number,
  ambientPerBin: number,
  fastOscFrequency: number,
): number {
  if (fastOscFrequency === 0) return SIGMA_INVALID;

  const pllPeriodMm = BigInt(calcPllPeriodMm(fastOscFrequency));
  const bMinusAmb = babs(BigInt(ambientPerBin) - BigInt(b));
  const aMinusC = babs(BigInt(a) - BigInt(c));

  if (bMinusAmb === 0n) return SIGMA_INVALID;

  let tmp0 = bmin(BigInt(b) + BigInt(bx) + BigInt(ambientPerBin), D_003);

  let tmp1 = (aMinusC * aMinusC) << 8n;
  tmp1 = bmin(tmp1, D_004);
  tmp1 = bfloorDiv(tmp1, bMinusAmb);
  tmp1 = bfloorDiv(tmp1, bMinusAmb);
  tmp1 = bmin(tmp1, D_005);

  tmp0 = tmp1 * tmp0;

  tmp1 = bmin(BigInt(cZp) + BigInt(cxZp) + BigInt(aZp) + BigInt(axZp), D_003) << 8n;

  tmp0 = bmin(tmp1 + tmp0, D_006);

  if (tmp0 > D_007) tmp0 = bfloorDiv(tmp0, bMinusAmb) * pllPeriodMm;
  else tmp0 = bfloorDiv(tmp0 * pllPeriodMm, bMinusAmb);
  tmp0 = bmin(tmp0, D_006);

  if (tmp0 > D_007) tmp0 = bfloorDiv(bfloorDiv(tmp0, bMinusAmb), 4n) * pllPeriodMm;
  else tmp0 = bfloorDiv(bfloorDiv(tmp0 * pllPeriodMm, bMinusAmb), 4n);
  tmp0 = bmin(tmp0, D_006);

  tmp0 = bmin(tmp0 >> 2n, D_007);

  tmp1 = BigInt(sigmaRefMm) << 7n;
  tmp0 = bmin(tmp0 + tmp1 * tmp1, D_007);

  // isqrt() masks to uint32; tmp0 is already <= D_007 and non-negative.
  return Number(isqrtBig(tmp0 & 0xffffffffn));
}

/** VL53LX_f_014: the sigma of one pulse, from its padded copies. */
function pulseSigma(
  pulse: PulseData,
  sigmaRefMm: number,
  algo: Gen3Algo,
  xtalkEnable: number,
): number {
  if (algo.vcsel_period === 0) return SIGMA_INVALID;
  const i = pymod(pulse.peak_bin, algo.vcsel_period);

  const [aZp, , cZp] = woiSums(i, pulse.filter_woi, algo.pulse_zero);
  const [a, b, c] = woiSums(i, pulse.filter_woi, algo.pulse_amb);
  let ax = 0;
  let bx = 0;
  let cx = 0;
  if (xtalkEnable) [ax, bx, cx] = woiSums(i, pulse.filter_woi, algo.pulse_xtalk);

  return sigmaEstimate(
    sigmaRefMm, a, b, c, aZp, cZp, bx, ax, cx,
    algo.pulse_amb.ambient_per_bin, algo.pulse_amb.fast_osc_frequency,
  );
}

// ── pulse detection (vl53lx_hist_algos_gen3.c) ───────────────────────────────
/** VL53LX_f_006: a detection threshold per bin, and the over-threshold flags. */
function ambientThresholds(
  scaler: number,
  thresholdSigma: number,
  minThresholdEvents: number,
  xtalkEnable: number,
  bins: HistogramBinData,
  algo: Gen3Algo,
): void {
  algo.buffer_size = bins.number_of_bins;
  algo.first_bin = bins.first_bin;
  algo.bins_in_data = bins.bins_in_data;
  algo.ambient_per_bin = bins.ambient_per_bin;
  algo.vcsel_period = decodeVcselPeriod(bins.vcsel_period);

  const ambEvents = cdiv(bins.ambient_per_bin * scaler + 2048, 4096);

  for (let lb = 0; lb < bins.bins_in_data; lb++) {
    const samples = bins.bin_rep[lb >> 2]!;
    if (samples <= 0) continue;

    let value: number;
    if (lb < algo.xtalk.bins_in_data && xtalkEnable) {
      value = samples * (ambEvents + algo.xtalk.bin_data[lb]!);
    } else {
      value = samples * ambEvents;
    }

    value = isqrt(value);
    value += floorDiv(samples, 2);
    value = floorDiv(value, samples);
    value *= thresholdSigma;
    value += 8;
    value = floorDiv(value, 16);
    value += ambEvents;
    value = Math.max(value, minThresholdEvents);

    algo.threshold[lb] = value;
    algo.ambient_threshold = value;
  }

  algo.bins_over_threshold = 0;
  for (let lb = bins.first_bin; lb < bins.bins_in_data; lb++) {
    const over = bins.bin_data[lb]! > algo.threshold[lb]! ? 1 : 0;
    algo.over_threshold[lb] = over;
    algo.pulse_mask[lb] = over;
    algo.bins_over_threshold += over;
  }
}

/** VL53LX_f_007: the first quiet-to-loud step. */
function findFirstRisingEdge(algo: Gen3Algo): void {
  algo.first_rising_bin = 0;
  let found = false;
  for (let i = 0; i < algo.vcsel_period; i++) {
    const j = pymod(i + 1, algo.vcsel_period);
    if (i < algo.bins_in_data && j < algo.bins_in_data) {
      if (algo.pulse_mask[i] === 0 && algo.pulse_mask[j] === 1 && !found) {
        algo.first_rising_bin = i;
        found = true;
      }
    }
  }
}

/** VL53LX_f_008: number the runs of over-threshold bins. */
function assignPulseNumbers(algo: Gen3Algo): void {
  for (let lb = algo.first_rising_bin; lb < algo.first_rising_bin + algo.vcsel_period; lb++) {
    const i = pymod(lb, algo.vcsel_period);
    const j = pymod(lb + 1, algo.vcsel_period);
    if (!(i < algo.bins_in_data && j < algo.bins_in_data)) continue;

    if (algo.pulse_mask[i] === 0 && algo.pulse_mask[j] === 1) algo.pulse_count += 1;
    algo.pulse_count = Math.min(algo.pulse_count, algo.max_pulses);
    algo.pulse_no[i] = algo.pulse_mask[i]! > 0 ? algo.pulse_count : 0;
  }
}

/** VL53LX_f_009: each numbered run → a pulse window. */
function pulseExtents(algo: Gen3Algo): void {
  const maxFilterHalfWidth = shr(algo.vcsel_period - 1, 1);

  for (let blb = algo.first_rising_bin; blb < algo.first_rising_bin + algo.vcsel_period; blb++) {
    const i = pymod(blb, algo.vcsel_period);
    const j = pymod(blb + 1, algo.vcsel_period);
    if (!(i < algo.bins_in_data && j < algo.bins_in_data)) continue;

    if (algo.pulse_no[i] === 0 && algo.pulse_no[j]! > 0) {
      const no = algo.pulse_no[j]! - 1;
      if (no < algo.max_pulses) {
        const pulse = algo.pulses[no]!;
        pulse.start_bin = blb;
        pulse.first_bin = blb + 1;
        pulse.peak_bin = 0xff;
        pulse.last_bin = 0;
        pulse.end_bin = 0;
      }
    }

    if (algo.pulse_no[i]! > 0 && algo.pulse_no[j] === 0) {
      const no = algo.pulse_no[i]! - 1;
      if (no < algo.max_pulses) {
        const pulse = algo.pulses[no]!;
        pulse.last_bin = blb;
        pulse.end_bin = blb + 1;
        pulse.width_bins = pulse.last_bin + 1 - pulse.first_bin;
        pulse.filter_woi = Math.min(pulse.end_bin + 1 - pulse.start_bin, maxFilterHalfWidth);
      }
    }
  }
}

/** VL53LX_f_010: total and ambient events inside the pulse window. */
function pulseEventSums(pulseNo: number, bins: HistogramBinData, algo: Gen3Algo): void {
  const pulse = algo.pulses[pulseNo]!;
  pulse.total_events = 0;
  pulse.ambient_events = 0;
  for (let lb = pulse.start_bin; lb <= pulse.end_bin; lb++) {
    const i = pymod(lb, algo.vcsel_period);
    pulse.total_events += bins.bin_data[i]!;
    pulse.ambient_events += algo.ambient_per_bin;
  }
  pulse.signal_events = pulse.total_events - pulse.ambient_events;
}

/** VL53LX_f_011: a copy of the frame with everything outside the pulse padded. */
function isolatePulse(
  pulseNo: number,
  bins: HistogramBinData,
  algo: Gen3Algo,
  padValue: number,
): HistogramBinData {
  const pulse = algo.pulses[pulseNo]!;
  const out = bins.clone();
  for (let lb = algo.first_rising_bin; lb < algo.first_rising_bin + algo.vcsel_period; lb++) {
    if (lb < pulse.start_bin || lb > pulse.end_bin) {
      const i = pymod(lb, algo.vcsel_period);
      if (i < out.bins_in_data) out.bin_data[i] = padValue;
    }
  }
  return out;
}

/** VL53LX_f_020: centre of mass of the bins in [start, end], 1/2048 bin. */
function weightedPhase(
  start: number,
  end: number,
  vcselPeriod: number,
  clipEvents: number,
  bins: HistogramBinData,
): number {
  let eventSum = 0;
  let weightedSum = 0;
  if (vcselPeriod === 0) return MAX_ALLOWED_PHASE;

  for (let lb = start; lb <= end; lb++) {
    const i = lb < 0 ? lb + vcselPeriod : pymod(lb, vcselPeriod);
    if (i >= 0 && i < HISTOGRAM_BUFFER_SIZE) {
      let value = bins.bin_data[i]! - bins.ambient_per_bin;
      if (clipEvents && value < 0) value = 0;
      eventSum += value;
      weightedSum += value * (1024 + 2048 * lb);
    }
  }

  if (eventSum > 0) {
    weightedSum += cdiv(eventSum, 2);
    weightedSum = cdiv(weightedSum, eventSum);
    return Math.max(weightedSum, 0);
  }
  return MAX_ALLOWED_PHASE;
}

/** VL53LX_f_015: the near and far phase of the pulse. */
function pulsePhaseLimits(
  pulseNo: number,
  clipEvents: number,
  bins: HistogramBinData,
  algo: Gen3Algo,
): void {
  const pulse = algo.pulses[pulseNo]!;
  if (pulse.peak_bin === 0xff) pulse.peak_bin = 1;

  const i = pymod(pulse.peak_bin, algo.vcsel_period);
  const start = i + pulse.start_bin - pulse.peak_bin;
  const end = i + pulse.end_bin - pulse.peak_bin;

  const windowWidth = Math.min(end - start, 3);

  pulse.phase_start = weightedPhase(start, start + windowWidth, algo.vcsel_period, clipEvents, bins);
  pulse.phase_end = weightedPhase(end - windowWidth, end, algo.vcsel_period, clipEvents, bins);

  if (pulse.phase_start > pulse.phase_end) {
    [pulse.phase_start, pulse.phase_end] = [pulse.phase_end, pulse.phase_start];
  }
  pulse.phase_start = Math.min(pulse.phase_start, pulse.phase_mean);
  pulse.phase_end = Math.max(pulse.phase_end, pulse.phase_mean);
}

/** VL53LX_f_016: strongest first or closest first (stable, as Python's sort). */
function sortPulses(targetOrder: number, algo: Gen3Algo): void {
  if (algo.pulse_count <= 1) return;
  const pulses = algo.pulses.slice(0, algo.pulse_count);
  if (targetOrder === HIST_TARGET_ORDER_STRONGEST_FIRST) {
    pulses.sort((p, q) => q.signal_events - p.signal_events);
  } else {
    pulses.sort((p, q) => p.phase_mean - q.phase_mean);
  }
  algo.pulses.splice(0, algo.pulse_count, ...pulses);
}

/** VL53LX_f_017: pulse → target, plus the sigma and phase-window checks. */
function fillTarget(
  rangeId: number,
  validPhaseLow: number,
  validPhaseHigh: number,
  sigmaThresh: number,
  bins: HistogramBinData,
  pulse: PulseData,
  target: RangeData,
): void {
  target.range_id = rangeId;
  target.start_bin = pulse.start_bin;
  target.first_bin = pulse.first_bin;
  target.peak_bin = pulse.peak_bin;
  target.last_bin = pulse.last_bin;
  target.end_bin = pulse.end_bin;
  target.width_bins = pulse.width_bins;
  target.window_bins = pulse.end_bin + 1 - pulse.start_bin;

  target.zero_distance_phase = bins.zero_distance_phase;
  target.sigma = pulse.sigma;
  target.phase_start = pymod(pulse.phase_start, 0x10000);
  target.phase_mean = pymod(pulse.phase_mean, 0x10000);
  target.phase_end = pymod(pulse.phase_end, 0x10000);
  target.total_events = pulse.total_events;
  target.signal_events = pulse.signal_events;
  target.ambient_events = pulse.ambient_events;
  target.total_periods_elapsed = bins.total_periods_elapsed;

  target.range_status = DEVICEERROR_RANGECOMPLETE_NO_WRAP_CHECK;

  if (sigmaThresh > 0 && pulse.sigma > shl(sigmaThresh, 5)) {
    target.range_status = DEVICEERROR_SIGMATHRESHOLDCHECK;
  }

  let lower = pymod(shl(validPhaseLow, 8), 0x10000);
  lower = lower < target.zero_distance_phase ? target.zero_distance_phase - lower : 0;
  const upper = pymod(shl(validPhaseHigh, 8) + bins.zero_distance_phase, 0x10000);

  if (target.phase_mean < lower || target.phase_mean > upper) {
    target.range_status = DEVICEERROR_RANGEPHASECHECK;
  }
}

// ── peak location (vl53lx_hist_algos_gen4.c) ─────────────────────────────────
/** VL53LX_f_026: the three-tap window over the pulse. */
function filterPulse(
  pulseNo: number,
  pulseBins: HistogramBinData,
  algo: Gen3Algo,
  filtered: Filtered,
): void {
  const pulse = algo.pulses[pulseNo]!;
  for (let lb = pulse.start_bin; lb <= pulse.end_bin; lb++) {
    const i = pymod(lb, algo.vcsel_period);
    const [a, b, c] = woiSums(i, pulse.filter_woi, pulseBins);
    filtered.a[i] = a;
    filtered.b[i] = b;
    filtered.c[i] = c;
    filtered.left[i] = a + b - (c + algo.ambient_per_bin);
    filtered.right[i] = b + c - (a + algo.ambient_per_bin);
  }
}

/**
 * VL53LX_f_028: sub-bin phase from the window sums. null when the pulse has
 * no height (the C division by zero → "not a peak after all"). BigInt: the
 * 4096 * 4096 scaling can pass 2^53.
 */
function interpolatePhase(
  binIndex: number,
  a: number,
  b: number,
  c: number,
  ambientPerBin: number,
  vcselPeriod: number,
): number | null {
  const numerator = 4096n * (BigInt(c) - BigInt(a));
  const halfBMinusAmb = 4096n * (BigInt(b) - BigInt(ambientPerBin));
  if (halfBMinusAmb === 0n) return null;

  let meanPhase = 4096n * numerator + halfBMinusAmb;
  meanPhase = bcdiv(meanPhase, halfBMinusAmb * 2n);
  meanPhase += 2048n;
  meanPhase += 4096n * BigInt(binIndex);
  meanPhase = bcdiv(meanPhase + 1n, 2n);

  if (meanPhase < 0n) meanPhase = 0n;
  if (meanPhase > BigInt(MAX_ALLOWED_PHASE)) meanPhase = BigInt(MAX_ALLOWED_PHASE);
  return pymod(Number(meanPhase), vcselPeriod * 2048);
}

/** VL53LX_f_027: keep the last bin where the filter says the peak is here. */
function findPeakBin(pulseNo: number, filtered: Filtered, algo: Gen3Algo): void {
  const pulse = algo.pulses[pulseNo]!;
  for (let lb = pulse.start_bin; lb < pulse.end_bin; lb++) {
    const i = pymod(lb, algo.vcsel_period);
    const j = pymod(lb + 1, algo.vcsel_period);
    if (!(i < algo.bins_in_data && j < algo.bins_in_data)) continue;

    const L = filtered.left;
    const R = filtered.right;
    let isPeak: number;
    if (L[i] === 0 && R[i] === 0) isPeak = 0;
    else if (L[i]! >= 0 && R[i]! >= 0) isPeak = 1;
    else if (L[i]! < 0 && R[i]! >= 0 && L[j]! >= 0 && R[j]! < 0) isPeak = 1;
    else isPeak = 0;
    filtered.is_peak[i] = isPeak;

    if (isPeak) {
      pulse.peak_bin = lb;
      const phase = interpolatePhase(
        lb, filtered.a[i]!, filtered.b[i]!, filtered.c[i]!, algo.ambient_per_bin,
        algo.vcsel_period,
      );
      if (phase === null) filtered.is_peak[i] = 0;
      else pulse.phase_mean = phase;
    }
  }
}

/**
 * `VL53LX_f_001` — the ambient-limited maximum range — deliberately a stub
 * (see the Python docstring): our boards ship the FMT 140MM_DARK region
 * blank, so a faithful port returns this same zero on every frame.
 */
function ambientDmax(results: RangeResults): void {
  results.ambient_dmax_mm = [0, 0, 0, 0, 0];
}

// ── entry point ──────────────────────────────────────────────────────────────
/**
 * `VL53LX_hist_process_data()` + `VL53LX_f_025`, with crosstalk off.
 * `binsInput` is what `BareDriver.getHistogramBinData()` returns; `hpp` is the
 * driver's `HistPostProcessConfig`. Pure (does not modify `binsInput`).
 */
export function processData(
  binsInput: HistogramBinData,
  hpp: HistPostProcessConfig,
  mergeNb = 1,
): RangeResults {
  if (hpp.algo__crosstalk_compensation_enable) {
    throw new Error("crosstalk compensation is not implemented (E8.6)");
  }

  const algo = new Gen3Algo();
  const filtered = new Filtered();
  const results = new RangeResults();

  const bins = averageRepeatedBins(binsInput); // VL53LX_f_031

  algo.bins = bins;
  // The crosstalk histogram, zero everywhere while compensation is off.
  algo.xtalk = bins.clone();
  algo.xtalk.bin_data = zeros(bins.number_of_bins);

  results.stream_count = binsInput.result__stream_count;

  calcZeroDistancePhase(bins);
  estimateAmbientFromThresholdedBins(hpp.ambient_thresh_sigma0, bins);
  estimateAmbientFromAmbientBins(bins);
  removeAmbientBins(bins);

  ambientDmax(results);

  ambientThresholds(
    hpp.ambient_thresh_events_scaler, hpp.ambient_thresh_sigma1,
    hpp.min_ambient_thresh_events, 0, bins, algo,
  );
  findFirstRisingEdge(algo);
  assignPulseNumbers(algo);
  pulseExtents(algo);

  for (let p = 0; p < algo.pulse_count; p++) {
    const pulse = algo.pulses[p]!;

    pulseEventSums(p, bins, algo);
    algo.pulse_amb = isolatePulse(p, bins, algo, bins.ambient_per_bin);
    algo.pulse_zero = isolatePulse(p, bins, algo, 0);
    algo.pulse_xtalk = isolatePulse(p, algo.xtalk, algo, 0);

    filterPulse(p, algo.pulse_amb, algo, filtered);
    findPeakBin(p, filtered, algo);
    pulse.sigma = pulseSigma(pulse, hpp.sigma_estimator__sigma_ref_mm, algo, 0);
    pulsePhaseLimits(p, 1, bins, algo);
  }

  sortPulses(hpp.hist_target_order, algo);

  for (let p = 0; p < algo.pulse_count; p++) {
    if (results.targets.length >= MAX_RANGE_RESULTS) break;
    const pulse = algo.pulses[p]!;
    if (!(pulse.signal_events > hpp.signal_total_events_limit && pulse.peak_bin < 0xff)) continue;

    const target = new RangeData();
    fillTarget(
      results.targets.length, hpp.valid_phase_low, hpp.valid_phase_high, hpp.sigma_thresh,
      bins, pulse, target,
    );
    calcRates(
      target, bins.vcsel_width, bins.fast_osc_frequency, bins.total_periods_elapsed,
      bins.result__dss_actual_effective_spads, mergeNb,
    );
    calcRanges(hpp.gain_factor, hpp.range_offset_mm, target);
    results.targets.push(target);
  }

  return results;
}

// ── frame-to-frame consistency ───────────────────────────────────────────────
interface PrevTarget {
  ambient_events: number;
  total_events: number;
  phase_mean: number;
  range_status: number;
}

/**
 * The frame before this one, reduced to what the consistency checks read
 * (`VL53LX_zone_hist_info_t` / `VL53LX_zone_objects_t` for our single zone).
 * The driver owns one and hands it every frame, exactly once.
 */
export class FrameHistory {
  rd_device_state: string | null = null;
  total_periods_elapsed = 0;
  spads = 0;
  targets: PrevTarget[] = [];

  /** Forget the stream so far; the next `apply()` only remembers. */
  reset(): void {
    this.rd_device_state = null;
    this.total_periods_elapsed = 0;
    this.spads = 0;
    this.targets = [];
  }

  /** Check `results` against the remembered frame, then remember it (C order). */
  apply(
    results: RangeResults,
    bins: HistogramBinData,
    rdDeviceState: string,
    hpp: HistPostProcessConfig,
  ): void {
    this.phaseConsistencyCheck(results, hpp);
    this.rd_device_state = rdDeviceState;
    this.total_periods_elapsed = bins.total_periods_elapsed;
    this.spads = bins.result__dss_actual_effective_spads;
    this.targets = results.targets.map((t) => ({
      ambient_events: t.ambient_events,
      total_events: t.total_events,
      phase_mean: t.phase_mean,
      range_status: t.range_status,
    }));
  }

  /** `VL53LX_hist_phase_consistency_check()`. */
  private phaseConsistencyCheck(results: RangeResults, hpp: HistPostProcessConfig): void {
    // The tolerance is a whole phase unit, and phases here are 8.8.
    const phaseTolerance = shl(hpp.algo__consistency_check__phase_tolerance, 8);
    if (this.rd_device_state !== DEVICESTATE_RANGING_OUTPUT_DATA) return;
    if (phaseTolerance === 0) return;

    const eventSigma = hpp.algo__consistency_check__event_sigma;
    const minSpads = hpp.algo__consistency_check__event_min_spad_count;
    const minMaxTolerance = hpp.algo__consistency_check__min_max_tolerance;

    for (const target of results.targets) {
      if (
        target.range_status !== DEVICEERROR_RANGECOMPLETE &&
        target.range_status !== DEVICEERROR_RANGECOMPLETE_NO_WRAP_CHECK
      ) {
        continue;
      }

      target.range_status = this.targets.length
        ? DEVICEERROR_PHASECONSISTENCY
        : DEVICEERROR_PREV_RANGE_NO_TARGETS;

      for (const prev of this.targets) {
        if (Math.abs(target.phase_mean - prev.phase_mean) >= phaseTolerance) continue;
        let status = eventsConsistencyCheck(eventSigma, minSpads, this, prev, target);
        if (status === DEVICEERROR_RANGECOMPLETE) status = mergedPulseCheck(minMaxTolerance, target);
        target.range_status = status;
      }
    }
  }
}

/**
 * `VL53LX_hist_events_consistency_check()` → a device error code. BigInt
 * throughout: the scaler squared is unbounded in Python.
 */
function eventsConsistencyCheck(
  eventSigma: number,
  minEffectiveSpadCount: number,
  prevFrame: FrameHistory,
  prev: PrevTarget,
  target: RangeData,
): number {
  if (eventSigma === 0) return DEVICEERROR_RANGECOMPLETE;

  const tmpp = BigInt(1 + prevFrame.total_periods_elapsed) * BigInt(prevFrame.spads);
  const tmpc = BigInt(1 + target.total_periods_elapsed) * BigInt(target.spads);

  let eventsScaler = tmpp * 4096n;
  if (tmpc !== 0n) eventsScaler = bcdiv(eventsScaler + bfloorDiv(tmpc, 2n), tmpc);
  const eventsScalerSq = bcdiv(eventsScaler * eventsScaler + 2048n, 4096n);

  const cSignalEvents = bcdiv(
    (BigInt(target.total_events) - BigInt(target.ambient_events)) * eventsScaler + 2048n,
    4096n,
  );
  const cSigNoiseSq = bcdiv(eventsScalerSq * BigInt(target.total_events) + 2048n, 4096n);
  let cAmbNoiseSq = bcdiv(eventsScalerSq * BigInt(target.ambient_events) + 2048n, 4096n);
  // Ambient lands in the estimate a quarter of its weight.
  cAmbNoiseSq = bcdiv(cAmbNoiseSq + 2n, 4n);
  const pAmbNoiseSq = bcdiv(BigInt(prev.ambient_events) + 2n, 4n);

  // Python `& 0xFFFFFFFF` on a possibly negative int: two's complement view.
  const noiseSqSum =
    (BigInt(prev.total_events) + cSigNoiseSq + pAmbNoiseSq + cAmbNoiseSq) & 0xffffffffn;
  // isqrt() masks its argument to uint32.
  let tolerance = isqrtBig((noiseSqSum * 16n) & 0xffffffffn);
  tolerance = bcdiv(tolerance * BigInt(eventSigma) + 32n, 64n);

  const pSignalEvents = BigInt(prev.total_events) - BigInt(prev.ambient_events);
  const delta = babs(cSignalEvents - pSignalEvents);

  if (delta > tolerance && target.spads > minEffectiveSpadCount) {
    return DEVICEERROR_EVENTCONSISTENCY;
  }
  return DEVICEERROR_RANGECOMPLETE;
}

/** `VL53LX_hist_merged_pulse_check()`. */
function mergedPulseCheck(minMaxToleranceMm: number, target: RangeData): number {
  const deltaMm = Math.abs(target.max_range_mm - target.min_range_mm);
  if (minMaxToleranceMm > 0 && deltaMm > minMaxToleranceMm) {
    return DEVICEERROR_RANGECOMPLETE_MERGED_PULSE;
  }
  return DEVICEERROR_RANGECOMPLETE;
}

// ── the family-wide result shape ─────────────────────────────────────────────
const CONVERT_STATUS: Readonly<Record<number, number>> = {
  [DEVICEERROR_RANGEPHASECHECK]: RANGESTATUS_OUTOFBOUNDS_FAIL,
  [DEVICEERROR_SIGMATHRESHOLDCHECK]: RANGESTATUS_SIGMA_FAIL,
  [DEVICEERROR_RANGECOMPLETE_NO_WRAP_CHECK]: RANGESTATUS_RANGE_VALID_NO_WRAP_CHECK,
  [DEVICEERROR_PHASECONSISTENCY]: RANGESTATUS_WRAP_TARGET_FAIL,
  [DEVICEERROR_EVENTCONSISTENCY]: RANGESTATUS_WRAP_TARGET_FAIL,
  [DEVICEERROR_PREV_RANGE_NO_TARGETS]: RANGESTATUS_TARGET_PRESENT_LACK_OF_SIGNAL,
  [DEVICEERROR_RANGECOMPLETE_MERGED_PULSE]: RANGESTATUS_RANGE_VALID_MERGED_PULSE,
  [DEVICEERROR_RANGECOMPLETE]: RANGESTATUS_RANGE_VALID,
};

/** ConvertStatusHisto() (vl53lx_api.c), the cases the histogram path produces. */
export function convertStatus(deviceError: number): number {
  return CONVERT_STATUS[deviceError] ?? RANGESTATUS_NONE;
}

/** UM2931 range status names, as uld/l4 spells them. */
export const STATUS_NAMES: Readonly<Record<number, string>> = {
  [RANGESTATUS_RANGE_VALID]: "valid",
  [RANGESTATUS_SIGMA_FAIL]: "sigma above threshold",
  [RANGESTATUS_OUTOFBOUNDS_FAIL]: "phase out of valid limit",
  [RANGESTATUS_RANGE_VALID_NO_WRAP_CHECK]: "no wrap-around check done",
  [RANGESTATUS_WRAP_TARGET_FAIL]: "wrapped target",
  [RANGESTATUS_RANGE_VALID_MERGED_PULSE]: "valid, merged pulse",
  [RANGESTATUS_TARGET_PRESENT_LACK_OF_SIGNAL]: "no target in the frame before",
  [RANGESTATUS_NONE]: "no target",
};

function toTarget(data: RangeData): Target {
  const status = convertStatus(data.range_status);
  return {
    distanceMm: data.median_range_mm,
    status,
    statusText: STATUS_NAMES[status] ?? `unknown (${status})`,
    signalKcps: data.signal_kcps,
    ambientKcps: data.ambient_kcps,
    sigmaMm: data.sigma_mm,
    minRangeMm: data.min_range_mm,
    maxRangeMm: data.max_range_mm,
  };
}

/**
 * RangeResults → the family-wide Measurement. Every return goes into
 * `targets`; the first one is also the measurement itself. `extra` keeps the
 * Python key names (display only).
 */
export function toMeasurement(results: RangeResults, bins: HistogramBinData): Measurement {
  if (results.targets.length === 0) {
    return measurement({
      distanceMm: 8191,
      status: RANGESTATUS_NONE,
      statusText: STATUS_NAMES[RANGESTATUS_NONE],
      spads: shr(bins.result__dss_actual_effective_spads, 8),
      extra: { stream_count: results.stream_count, ambient_per_bin: bins.ambient_per_bin },
    });
  }

  const first = results.targets[0]!;
  const t0 = toTarget(first);
  return measurement({
    distanceMm: t0.distanceMm,
    status: t0.status,
    statusText: t0.statusText,
    signalKcps: t0.signalKcps,
    ambientKcps: t0.ambientKcps,
    sigmaMm: t0.sigmaMm,
    // The device counts effective SPADs in 1/256; the family reports whole ones.
    spads: shr(first.spads, 8),
    targets: results.targets.map(toTarget),
    extra: {
      stream_count: results.stream_count,
      min_range_mm: first.min_range_mm,
      max_range_mm: first.max_range_mm,
      peak_bin: first.peak_bin,
      ambient_per_bin: bins.ambient_per_bin,
    },
  });
}
