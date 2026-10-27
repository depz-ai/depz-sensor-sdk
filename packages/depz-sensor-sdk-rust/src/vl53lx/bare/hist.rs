//! Histogram post-processing: 24 bins → targets, on the host alone, as `VL53LX_hist_process_data()`
//! does in C (`hist.py` of the reference). Fold repeated bin codes, estimate the ambient floor, drop
//! the ambient-only bins, threshold each bin, take the runs of marked bins as pulses; each pulse is
//! isolated, filtered with a three-tap window and its zero crossing interpolated to a sub-bin phase,
//! which converts to millimetres. ST's source names its functions `VL53LX_f_NNN`; the names here are
//! ours, the C name is in the comment. Crosstalk compensation and the ambient dmax are not ported
//! (dmax needs FMT data our boards ship blank, so it would be zero anyway). Arithmetic is the C
//! driver's: truncating division, and saturation where it caps an intermediate.

use std::collections::BTreeMap;

use crate::devices::{DepzError, Result};
use crate::vl53lx::driver::{ExtraValue, Vl53lxRange, Vl53lxTarget};

use super::core::{HistPostProcessConfig, HistogramBinData, RangeData, RangeResults};
use super::history::FrameHistory;
use super::math;
use super::regs::{HISTOGRAM_BUFFER_SIZE, RANGING_WINDOW_VCSEL_PERIODS};

/// Codes in one bin sequence.
pub const MAX_BIN_SEQUENCE_LENGTH: usize = 6;
/// The largest bin-sequence code.
pub const MAX_BIN_SEQUENCE_CODE: i64 = 15;
/// `VL53LX_D_001`: pulses tracked per frame.
pub const MAX_PULSES: usize = 8;
/// Targets reported per frame.
pub const MAX_RANGE_RESULTS: usize = 4;

/// The phase ceiling.
pub const MAX_ALLOWED_PHASE: i64 = 0xFFFF;
/// The event-count ceiling of the rate maths.
pub const SPAD_TOTAL_COUNT_MAX: i64 = (1 << 29) - 1;
/// Above this many events the rate maths drops resolution.
pub const SPAD_TOTAL_COUNT_RES_THRES: i64 = 1 << 24;
/// The speed of light in air / 8, mm/µs-ish units of the C driver.
pub const SPEED_OF_LIGHT_IN_AIR_DIV8: i64 = 299704 >> 3;

/// `VL53LX_D_002`: an invalid sigma.
pub const SIGMA_INVALID: i64 = 0xFFFF;
// vl53lx_sigma_estimate.h — the saturation ceilings of the sigma estimate.
const D003: i64 = 0xFF_FFFF;
const D004: i64 = 0xFF_FFFF_FFFF_FFFF;
const D005: i64 = 0x7F_FFFF_FFFF;
const D006: i64 = 0x7FFF_FFFF_FFFF_FFFF;
const D007: i64 = 0xFFFF_FFFF;

/// `VL53LX_DEVICEERROR_NOUPDATE`.
pub const DEVICE_ERROR_NO_UPDATE: i64 = 0;
/// `VL53LX_DEVICEERROR_RANGEPHASECHECK`.
pub const DEVICE_ERROR_RANGE_PHASE_CHECK: i64 = 5;
/// `VL53LX_DEVICEERROR_SIGMATHRESHOLDCHECK`.
pub const DEVICE_ERROR_SIGMA_THRESHOLD_CHECK: i64 = 6;
/// `VL53LX_DEVICEERROR_PHASECONSISTENCY`.
pub const DEVICE_ERROR_PHASE_CONSISTENCY: i64 = 7;
/// `VL53LX_DEVICEERROR_RANGECOMPLETE`.
pub const DEVICE_ERROR_RANGE_COMPLETE: i64 = 9;
/// `VL53LX_DEVICEERROR_RANGECOMPLETE_NO_WRAP_CHECK`.
pub const DEVICE_ERROR_RANGE_COMPLETE_NO_WRAP_CHECK: i64 = 19;
/// `VL53LX_DEVICEERROR_EVENTCONSISTENCY`.
pub const DEVICE_ERROR_EVENT_CONSISTENCY: i64 = 20;
/// `VL53LX_DEVICEERROR_RANGECOMPLETE_MERGED_PULSE`.
pub const DEVICE_ERROR_RANGE_COMPLETE_MERGED_PULSE: i64 = 22;
/// `VL53LX_DEVICEERROR_PREV_RANGE_NO_TARGETS`.
pub const DEVICE_ERROR_PREV_RANGE_NO_TARGETS: i64 = 23;

/// Target order: strongest first.
pub const HIST_TARGET_ORDER_STRONGEST_FIRST: i64 = 1;
/// Target order: closest first.
pub const HIST_TARGET_ORDER_CLOSEST_FIRST: i64 = 2;

/// `VL53LX_RANGESTATUS_RANGE_VALID`.
pub const RANGE_STATUS_RANGE_VALID: i32 = 0;
/// `VL53LX_RANGESTATUS_SIGMA_FAIL`.
pub const RANGE_STATUS_SIGMA_FAIL: i32 = 1;
/// `VL53LX_RANGESTATUS_OUTOFBOUNDS_FAIL`.
pub const RANGE_STATUS_OUT_OF_BOUNDS_FAIL: i32 = 4;
/// `VL53LX_RANGESTATUS_RANGE_VALID_NO_WRAP_CHECK_FAIL`.
pub const RANGE_STATUS_RANGE_VALID_NO_WRAP_CHECK: i32 = 6;
/// `VL53LX_RANGESTATUS_WRAP_TARGET_FAIL`.
pub const RANGE_STATUS_WRAP_TARGET_FAIL: i32 = 7;
/// `VL53LX_RANGESTATUS_RANGE_VALID_MERGED_PULSE`.
pub const RANGE_STATUS_RANGE_VALID_MERGED_PULSE: i32 = 11;
/// `VL53LX_RANGESTATUS_TARGET_PRESENT_LACK_OF_SIGNAL`.
pub const RANGE_STATUS_TARGET_PRESENT_LACK_OF_SIGNAL: i32 = 12;
/// `VL53LX_RANGESTATUS_NONE`.
pub const RANGE_STATUS_NONE: i32 = 255;

/// UM2931 range status name, or `None` for a status the histogram path does not produce.
pub fn status_name(status: i32) -> Option<&'static str> {
    Some(match status {
        RANGE_STATUS_RANGE_VALID => "valid",
        RANGE_STATUS_SIGMA_FAIL => "sigma above threshold",
        RANGE_STATUS_OUT_OF_BOUNDS_FAIL => "phase out of valid limit",
        RANGE_STATUS_RANGE_VALID_NO_WRAP_CHECK => "no wrap-around check done",
        RANGE_STATUS_WRAP_TARGET_FAIL => "wrapped target",
        RANGE_STATUS_RANGE_VALID_MERGED_PULSE => "valid, merged pulse",
        RANGE_STATUS_TARGET_PRESENT_LACK_OF_SIGNAL => "no target in the frame before",
        RANGE_STATUS_NONE => "no target",
        _ => return None,
    })
}

// ── working state ────────────────────────────────────────────────────────

// VL53LX_hist_pulse_data_t: one run of bins above the threshold.
#[derive(Debug, Clone, Copy)]
struct PulseData {
    start_bin: i64,      // VL53LX_p_012
    first_bin: i64,      // VL53LX_p_019
    peak_bin: i64,       // VL53LX_p_023
    last_bin: i64,       // VL53LX_p_024
    end_bin: i64,        // VL53LX_p_013
    width_bins: i64,     // VL53LX_p_025
    filter_woi: i64,     // VL53LX_p_051, filter half width
    ambient_events: i64, // VL53LX_p_016
    total_events: i64,   // VL53LX_p_017
    signal_events: i64,  // VL53LX_p_010
    phase_start: i64,    // VL53LX_p_026
    phase_mean: i64,     // VL53LX_p_011
    phase_end: i64,      // VL53LX_p_027
    sigma: i64,          // VL53LX_p_002
}

impl Default for PulseData {
    fn default() -> Self {
        PulseData {
            start_bin: 0,
            first_bin: 0,
            peak_bin: 0xFF,
            last_bin: 0,
            end_bin: 0,
            width_bins: 0,
            filter_woi: 0,
            ambient_events: 0,
            total_events: 0,
            signal_events: 0,
            phase_start: 0,
            phase_mean: 0,
            phase_end: 0,
            sigma: 0,
        }
    }
}

const N: usize = HISTOGRAM_BUFFER_SIZE;

// VL53LX_hist_gen3_algo_private_data_t: per-bin thresholds and flags, the pulse list.
struct Gen3Algo {
    bins_in_data: i64,        // VL53LX_p_021
    vcsel_period: i64,        // VL53LX_p_030, bins per period
    bins_over_threshold: i64, // VL53LX_p_039
    ambient_per_bin: i64,     // VL53LX_p_028
    ambient_threshold: i64,   // VL53LX_p_031, the last one computed
    over_threshold: [i64; N], // VL53LX_p_040
    pulse_mask: [i64; N],     // VL53LX_p_041
    pulse_no: [i64; N],       // VL53LX_p_042
    threshold: [i64; N],      // VL53LX_p_052
    first_rising_bin: i64,    // VL53LX_p_044
    max_pulses: i64,          // VL53LX_p_045
    pulse_count: i64,         // VL53LX_p_046
    pulses: [PulseData; MAX_PULSES],
}

impl Gen3Algo {
    fn new() -> Gen3Algo {
        Gen3Algo {
            bins_in_data: 0,
            vcsel_period: 0,
            bins_over_threshold: 0,
            ambient_per_bin: 0,
            ambient_threshold: 0,
            over_threshold: [0; N],
            pulse_mask: [0; N],
            pulse_no: [0; N],
            threshold: [0; N],
            first_rising_bin: 0,
            max_pulses: MAX_PULSES as i64,
            pulse_count: 0,
            pulses: [PulseData::default(); MAX_PULSES],
        }
    }
}

// VL53LX_hist_gen4_algo_filtered_data_t: the three-tap window sums and the two edge signals whose sign
// change locates the peak.
struct Filtered {
    a: [i64; N],       // VL53LX_p_007
    b: [i64; N],       // VL53LX_p_032
    c: [i64; N],       // VL53LX_p_001
    left: [i64; N],    // VL53LX_p_053
    right: [i64; N],   // VL53LX_p_054
    is_peak: [i64; N], // VL53LX_p_040
}

/// One target of the frame before, reduced to what the consistency checks read.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct PrevTarget {
    /// Its ambient events.
    pub ambient_events: i64,
    /// Its total events.
    pub total_events: i64,
    /// Its mean phase.
    pub phase_mean: i64,
    /// Its device error.
    pub range_status: i64,
}

// ── arithmetic ───────────────────────────────────────────────────────────

/// The modulo of the reference (the sign of the divisor).
pub fn py_mod(a: i64, b: i64) -> i64 {
    let r = a % b;
    if r != 0 && ((r < 0) != (b < 0)) {
        r + b
    } else {
        r
    }
}

// The floor division of the reference (C truncation is plain '/').
fn floor_div(a: i64, b: i64) -> i64 {
    let q = a / b;
    if (a % b != 0) && ((a < 0) != (b < 0)) {
        q - 1
    } else {
        q
    }
}

/// `VL53LX_isqrt()`: the floor of the square root of a uint32 argument.
pub fn isqrt(num: i64) -> i64 {
    let n = num & 0xFFFF_FFFF;
    let mut r = (n as f64).sqrt() as i64;
    while r * r > n {
        r -= 1;
    }
    while (r + 1) * (r + 1) <= n {
        r += 1;
    }
    r
}

// a * b for non-negative operands, held at cap when the true product would pass it.
fn sat_mul(a: i64, b: i64, cap: i64) -> i64 {
    if b != 0 && a > cap / b {
        cap
    } else {
        (a * b).min(cap)
    }
}

/// `VL53LX_calc_pll_period_mm()`.
pub fn calc_pll_period_mm(fast_osc_frequency: i64) -> i64 {
    let pll_period_us = math::calc_pll_period_us(fast_osc_frequency);
    let pll_period_mm = SPEED_OF_LIGHT_IN_AIR_DIV8 * (pll_period_us >> 2);
    (pll_period_mm + (1 << 15)) >> 16
}

/// `VL53LX_rate_maths()`: events over a duration → count rate in 9.7 Mcps.
pub fn rate_maths(events: i64, time_us: i64) -> i64 {
    let mut tmp = if events > SPAD_TOTAL_COUNT_MAX {
        SPAD_TOTAL_COUNT_MAX
    } else if events > 0 {
        events
    } else {
        0
    };
    let frac_bits = if events > SPAD_TOTAL_COUNT_RES_THRES {
        3
    } else {
        7
    };
    if time_us > 0 {
        tmp = ((tmp << frac_bits) + time_us / 2) / time_us;
    }
    if events > SPAD_TOTAL_COUNT_RES_THRES {
        tmp <<= 4;
    }
    tmp.min(0xFFFF)
}

/// `VL53LX_rate_per_spad_maths()`.
pub fn rate_per_spad_maths(
    frac_bits: u32,
    peak_count_rate: i64,
    num_spads: i64,
    max_output_value: i64,
) -> i64 {
    let tmp = if num_spads > 0 {
        let t = (peak_count_rate << 8) << frac_bits;
        (t + num_spads / 2) / num_spads
    } else {
        peak_count_rate << frac_bits
    };
    tmp.min(max_output_value)
}

/// `VL53LX_events_per_spad_maths()` → kcps per SPAD.
pub fn events_per_spad_maths(events: i64, num_spads: i64, duration: i64) -> i64 {
    let total = if num_spads != 0 {
        floor_div(events.wrapping_mul(1000 * 256), num_spads)
    } else {
        0
    };
    let per_spad = if duration > 0 {
        floor_div(total.wrapping_shl(11).wrapping_add(duration / 2), duration)
    } else {
        total.wrapping_shl(11)
    };
    per_spad & 0xFFFF_FFFF
}

/// `VL53LX_range_maths()`: phase → millimetres.
pub fn range_maths(
    fast_osc_frequency: i64,
    phase: i64,
    zero_distance_phase: i64,
    fractional_bits: i32,
    gain_factor: i64,
    range_offset_mm: i64,
) -> i64 {
    let pll_period_us = math::calc_pll_period_us(fast_osc_frequency);
    let mut tmp = phase - zero_distance_phase;
    tmp *= pll_period_us;
    tmp /= 1 << 9;
    tmp *= SPEED_OF_LIGHT_IN_AIR_DIV8;
    tmp /= 1 << 22;

    let mut range_mm = tmp + range_offset_mm;
    range_mm *= gain_factor;
    range_mm += 0x0400;
    range_mm /= 0x0800;

    if fractional_bits == 0 {
        let range_mm10 = range_mm * 10 / (1 << 2);
        range_mm = if (range_mm10 - range_mm10 / 10 * 10).abs() < 5 {
            range_mm10 / 10
        } else {
            range_mm10 / 10 + 1
        };
    } else if fractional_bits == 1 {
        range_mm /= 1 << 1;
    }
    range_mm
}

// ── bin housekeeping ─────────────────────────────────────────────────────

/// `VL53LX_f_031`: fold the six bin-sequence codes together. Repeats of a code land on the same four
/// output bins and are averaged, so what comes out is one bin per distinct code.
pub fn average_repeated_bins(src: &HistogramBinData) -> HistogramBinData {
    let mut dst = src.clone();
    dst.bins_in_data = 0;
    dst.bin_seq = [MAX_BIN_SEQUENCE_CODE + 1; MAX_BIN_SEQUENCE_LENGTH];
    dst.bin_data = vec![0; dst.number_of_bins.max(0) as usize];

    let codes = MAX_BIN_SEQUENCE_CODE as usize + 2;
    let mut initial_index = vec![0usize; codes];
    let mut repeat_count = vec![0i64; codes];
    let mut seq_length = 0usize;
    for lc in 0..MAX_BIN_SEQUENCE_LENGTH {
        let bin_cfg = src.bin_seq[lc] as usize;
        if repeat_count[bin_cfg] == 0 {
            initial_index[bin_cfg] = seq_length * 4;
            dst.bin_seq[seq_length] = bin_cfg as i64;
            seq_length += 1;
        }
        repeat_count[bin_cfg] += 1;
        let b = initial_index[bin_cfg];
        for i in 0..4 {
            dst.bin_data[b + i] += src.bin_data[lc * 4 + i];
        }
    }

    for i in 0..MAX_BIN_SEQUENCE_LENGTH {
        let c = dst.bin_seq[i];
        dst.bin_rep[i] = if c <= MAX_BIN_SEQUENCE_CODE {
            repeat_count[c as usize]
        } else {
            0
        };
    }
    dst.bins_in_data = seq_length as i64 * 4;

    for code in 0..=MAX_BIN_SEQUENCE_CODE as usize {
        let reps = repeat_count[code];
        if reps <= 0 {
            continue;
        }
        let b = initial_index[code];
        for i in 0..4 {
            dst.bin_data[b + i] = (dst.bin_data[b + i] + reps / 2) / reps;
        }
    }

    // Codes 7 and 15 are the ambient-only sequence entries.
    dst.number_of_ambient_bins = if repeat_count[7] != 0 || repeat_count[15] != 0 {
        4
    } else {
        0
    };
    dst
}

/// `VL53LX_hist_calc_zero_distance_phase`.
pub fn calc_zero_distance_phase(bins: &mut HistogramBinData) {
    let period = 2048 * math::decode_vcsel_period(bins.vcsel_period);
    let phase =
        period + bins.phasecal_result_reference_phase + 2048 * bins.phasecal_result_vcsel_start
            - 2048 * bins.cal_config_vcsel_start;
    bins.zero_distance_phase = if period != 0 {
        py_mod(phase, period)
    } else {
        0
    };
}

/// `VL53LX_hist_estimate_ambient_from_thresholded_bins()`: the floor from every bin near the
/// quietest.
pub fn estimate_ambient_from_thresholded_bins(
    ambient_threshold_sigma: i64,
    bins: &mut HistogramBinData,
) {
    let n = (bins.bins_in_data.max(0) as usize).min(bins.bin_data.len());
    let used = &bins.bin_data[..n];
    bins.min_bin_value = used.iter().copied().min().unwrap_or(0);
    bins.max_bin_value = used.iter().copied().max().unwrap_or(0);

    let mut threshold = isqrt(bins.min_bin_value);
    threshold *= ambient_threshold_sigma;
    threshold += 0x07;
    threshold >>= 4;
    threshold += bins.min_bin_value;

    let mut samples = 0;
    let mut sum = 0;
    for &value in used {
        if value < threshold {
            sum += value;
            samples += 1;
        }
    }
    bins.number_of_ambient_samples = samples;
    bins.ambient_events_sum = sum;
    if samples > 0 {
        bins.ambient_per_bin = (sum + samples / 2) / samples;
    }
}

/// `VL53LX_hist_estimate_ambient_from_ambient_bins()`: the better estimate, from the VCSEL-off bins.
pub fn estimate_ambient_from_ambient_bins(bins: &mut HistogramBinData) {
    if bins.number_of_ambient_bins <= 0 {
        return;
    }
    bins.number_of_ambient_samples = bins.number_of_ambient_bins;
    let n = (bins.number_of_ambient_bins as usize).min(bins.bin_data.len());
    bins.ambient_events_sum = bins.bin_data[..n].iter().sum();
    bins.ambient_per_bin =
        (bins.ambient_events_sum + bins.number_of_ambient_bins / 2) / bins.number_of_ambient_bins;
}

/// `VL53LX_hist_remove_ambient_bins`: drop the ambient-only bins off the front, so bin 0 is phase
/// zero.
pub fn remove_ambient_bins(bins: &mut HistogramBinData) {
    if bins.bin_seq[0] & 0x07 == 0x07 {
        let mut kept_seq = [MAX_BIN_SEQUENCE_CODE + 1; MAX_BIN_SEQUENCE_LENGTH];
        let mut kept_rep = [0i64; MAX_BIN_SEQUENCE_LENGTH];
        let mut k = 0;
        for lc in 0..MAX_BIN_SEQUENCE_LENGTH {
            if bins.bin_seq[lc] & 0x07 != 0x07 {
                kept_seq[k] = bins.bin_seq[lc];
                kept_rep[k] = bins.bin_rep[lc];
                k += 1;
            }
        }
        bins.bin_seq = kept_seq;
        bins.bin_rep = kept_rep;
    }
    let n = bins.number_of_ambient_bins.max(0) as usize;
    if n > 0 {
        let len = bins.bin_data.len();
        let mut shifted = vec![0i64; len];
        if n < len {
            shifted[..len - n].copy_from_slice(&bins.bin_data[n..]);
        }
        bins.bin_data = shifted;
        bins.bins_in_data -= n as i64;
        bins.number_of_ambient_bins = 0;
    }
}

// VL53LX_f_022: the three-tap window around bin_index over a half width of filter_woi bins → (a, b, c).
fn woi_sums(bin_index: i64, filter_woi: i64, bins: &HistogramBinData) -> (i64, i64, i64) {
    let mut a = 0;
    let b = bins.bin_data[bin_index as usize];
    let mut c = 0;
    for w in 0..(filter_woi << 1) + 1 {
        let j = py_mod(
            bin_index + w + bins.bins_in_data - filter_woi,
            bins.bins_in_data,
        ) as usize;
        if w < filter_woi {
            a += bins.bin_data[j];
        } else if w > filter_woi {
            c += bins.bin_data[j];
        }
    }
    (a, b, c)
}

// VL53LX_f_018: event sums → count rates, over the durations the pulse actually ran.
fn calc_rates(
    target: &mut RangeData,
    vcsel_width: i64,
    fast_osc_frequency: i64,
    total_periods_elapsed: i64,
    spads: i64,
    merge_nb: i64,
) -> bool {
    target.vcsel_width = vcsel_width;
    target.fast_osc_frequency = fast_osc_frequency;
    target.total_periods_elapsed = total_periods_elapsed;
    target.spads = spads;
    if fast_osc_frequency == 0 || total_periods_elapsed == 0 {
        return false;
    }

    let pll_period_us = math::calc_pll_period_us(fast_osc_frequency);
    let periods_elapsed = total_periods_elapsed.wrapping_add(1);
    target.peak_duration_us = math::duration_maths(
        pll_period_us,
        vcsel_width,
        RANGING_WINDOW_VCSEL_PERIODS,
        periods_elapsed,
    );
    target.woi_duration_us = math::duration_maths(
        pll_period_us,
        target.window_bins << 4,
        RANGING_WINDOW_VCSEL_PERIODS,
        periods_elapsed,
    );

    target.peak_signal_count_rate_mcps = rate_maths(target.signal_events, target.peak_duration_us);
    target.avg_signal_count_rate_mcps = rate_maths(target.signal_events, target.woi_duration_us);
    target.ambient_count_rate_mcps = rate_maths(target.ambient_events, target.woi_duration_us);

    let mut count_rate_total = target.peak_signal_count_rate_mcps + target.ambient_count_rate_mcps;
    if merge_nb > 1 {
        count_rate_total = floor_div(count_rate_total, merge_nb);
    }
    target.total_rate_per_spad_mcps = rate_per_spad_maths(0x06, count_rate_total, spads, 0xFFFF);
    target.signal_events_per_spad_kcps =
        events_per_spad_maths(target.signal_events, spads, target.peak_duration_us);
    true
}

// VL53LX_f_019
fn calc_ranges(gain_factor: i64, range_offset_mm: i64, target: &mut RangeData) {
    let r = |phase| {
        range_maths(
            target.fast_osc_frequency,
            phase,
            target.zero_distance_phase,
            0,
            gain_factor,
            range_offset_mm,
        )
    };
    let (min, median, max) = (
        r(target.phase_start),
        r(target.phase_mean),
        r(target.phase_end),
    );
    target.min_range_mm = min;
    target.median_range_mm = median;
    target.max_range_mm = max;
}

// ── sigma (vl53lx_sigma_estimate.c) ──────────────────────────────────────

// VL53LX_f_023: the width of the pulse, 9.7 fixed point mm — how far the window sums lean to one side,
// over how much signal there is above ambient. The ceilings are ST's.
#[allow(clippy::too_many_arguments)] // ST's VL53LX_f_023 parameter list, kept 1:1
fn sigma_estimate(
    sigma_ref_mm: i64,
    a: i64,
    b: i64,
    c: i64,
    a_zp: i64,
    c_zp: i64,
    bx: i64,
    ax_zp: i64,
    cx_zp: i64,
    ambient_per_bin: i64,
    fast_osc_frequency: i64,
) -> i64 {
    if fast_osc_frequency == 0 {
        return SIGMA_INVALID;
    }
    let pll_period_mm = calc_pll_period_mm(fast_osc_frequency);
    let b_minus_amb = (ambient_per_bin - b).abs();
    let a_minus_c = (a - c).abs();
    if b_minus_amb == 0 {
        return SIGMA_INVALID;
    }

    let mut tmp0 = (b + bx + ambient_per_bin).min(D003);

    // (a - c)^2 << 8, capped at D004 — which it passes exactly when |a - c| >= 2^24.
    let mut tmp1 = if a_minus_c >= 1 << 24 {
        D004
    } else {
        ((a_minus_c * a_minus_c) << 8).min(D004)
    };
    tmp1 /= b_minus_amb;
    tmp1 /= b_minus_amb;
    tmp1 = tmp1.min(D005);

    tmp0 *= tmp1; // < 2^39 * 2^24: no overflow

    tmp1 = (c_zp + cx_zp + a_zp + ax_zp).min(D003) << 8;

    // The reference caps tmp0 at D006 = i64::MAX here and after the two divisions below: a
    // no-op for an i64, so the caps are left out (sat_mul still clamps the wide products).
    tmp0 += tmp1;

    tmp0 = if tmp0 > D007 {
        sat_mul(tmp0 / b_minus_amb, pll_period_mm, D006)
    } else {
        tmp0 * pll_period_mm / b_minus_amb
    };

    tmp0 = if tmp0 > D007 {
        sat_mul(tmp0 / b_minus_amb / 4, pll_period_mm, D006)
    } else {
        tmp0 * pll_period_mm / b_minus_amb / 4
    };

    tmp0 = (tmp0 >> 2).min(D007);

    tmp1 = sigma_ref_mm << 7;
    tmp0 = (tmp0 + tmp1 * tmp1).min(D007);
    isqrt(tmp0)
}

// VL53LX_f_014: the sigma of one pulse, from its two padded copies (and its crosstalk copy).
fn pulse_sigma(
    pulse: &PulseData,
    sigma_ref_mm: i64,
    vcsel_period: i64,
    pulse_zero: &HistogramBinData,
    pulse_amb: &HistogramBinData,
    pulse_xtalk: Option<&HistogramBinData>,
) -> i64 {
    if vcsel_period == 0 {
        return SIGMA_INVALID;
    }
    let i = pulse.peak_bin % vcsel_period;
    let zp = woi_sums(i, pulse.filter_woi, pulse_zero);
    let amb = woi_sums(i, pulse.filter_woi, pulse_amb);
    let x = pulse_xtalk
        .map(|x| woi_sums(i, pulse.filter_woi, x))
        .unwrap_or((0, 0, 0));
    sigma_estimate(
        sigma_ref_mm,
        amb.0,
        amb.1,
        amb.2,
        zp.0,
        zp.2,
        x.1,
        x.0,
        x.2,
        pulse_amb.ambient_per_bin,
        pulse_amb.fast_osc_frequency,
    )
}

// ── pulse detection (vl53lx_hist_algos_gen3.c) ───────────────────────────

// VL53LX_f_006: a detection threshold per bin — the ambient floor plus a sigma margin that shrinks with
// the number of samples in the bin — and the flags saying which bins clear it.
fn ambient_thresholds(
    scaler: i64,
    threshold_sigma: i64,
    min_threshold_events: i64,
    xtalk: Option<&HistogramBinData>,
    bins: &HistogramBinData,
    algo: &mut Gen3Algo,
) {
    algo.bins_in_data = bins.bins_in_data;
    algo.ambient_per_bin = bins.ambient_per_bin;
    algo.vcsel_period = math::decode_vcsel_period(bins.vcsel_period);

    let amb_events = (bins.ambient_per_bin * scaler + 2048) / 4096;

    for lb in 0..bins.bins_in_data.max(0) {
        let samples = bins.bin_rep[(lb >> 2) as usize];
        if samples <= 0 {
            continue;
        }
        let mut value = match xtalk {
            Some(x) if lb < x.bins_in_data => samples * (amb_events + x.bin_data[lb as usize]),
            _ => samples * amb_events,
        };
        value = isqrt(value);
        value += samples / 2;
        value = floor_div(value, samples);
        value *= threshold_sigma;
        value += 8;
        value = floor_div(value, 16);
        value += amb_events;
        value = value.max(min_threshold_events);
        algo.threshold[lb as usize] = value;
        algo.ambient_threshold = value;
    }

    algo.bins_over_threshold = 0;
    for lb in bins.first_bin.max(0)..bins.bins_in_data.max(0) {
        let lb = lb as usize;
        let over = i64::from(bins.bin_data[lb] > algo.threshold[lb]);
        algo.over_threshold[lb] = over;
        algo.pulse_mask[lb] = over;
        algo.bins_over_threshold += over;
    }
}

// VL53LX_f_007: the first quiet-to-loud step, so a pulse wrapped across the end of the period stays
// whole.
fn find_first_rising_edge(algo: &mut Gen3Algo) {
    algo.first_rising_bin = 0;
    let mut found = false;
    for i in 0..algo.vcsel_period {
        let j = (i + 1) % algo.vcsel_period;
        if i < algo.bins_in_data
            && j < algo.bins_in_data
            && algo.pulse_mask[i as usize] == 0
            && algo.pulse_mask[j as usize] == 1
            && !found
        {
            algo.first_rising_bin = i;
            found = true;
        }
    }
}

// VL53LX_f_008: number the runs of over-threshold bins, from the first rising edge.
fn assign_pulse_numbers(algo: &mut Gen3Algo) {
    for lb in algo.first_rising_bin..algo.first_rising_bin + algo.vcsel_period {
        let i = lb % algo.vcsel_period;
        let j = (lb + 1) % algo.vcsel_period;
        if !(i < algo.bins_in_data && j < algo.bins_in_data) {
            continue;
        }
        let (i, j) = (i as usize, j as usize);
        if algo.pulse_mask[i] == 0 && algo.pulse_mask[j] == 1 {
            algo.pulse_count += 1;
        }
        algo.pulse_count = algo.pulse_count.min(algo.max_pulses);
        algo.pulse_no[i] = if algo.pulse_mask[i] > 0 {
            algo.pulse_count
        } else {
            0
        };
    }
}

// VL53LX_f_009: each numbered run → its window, width and filter half width.
fn pulse_extents(algo: &mut Gen3Algo) {
    let max_filter_half_width = (algo.vcsel_period - 1) >> 1;
    for blb in algo.first_rising_bin..algo.first_rising_bin + algo.vcsel_period {
        let i = blb % algo.vcsel_period;
        let j = (blb + 1) % algo.vcsel_period;
        if !(i < algo.bins_in_data && j < algo.bins_in_data) {
            continue;
        }
        let (i, j) = (i as usize, j as usize);
        if algo.pulse_no[i] == 0 && algo.pulse_no[j] > 0 {
            let no = algo.pulse_no[j] - 1;
            if no < algo.max_pulses {
                let pulse = &mut algo.pulses[no as usize];
                pulse.start_bin = blb;
                pulse.first_bin = blb + 1;
                pulse.peak_bin = 0xFF;
                pulse.last_bin = 0;
                pulse.end_bin = 0;
            }
        }
        if algo.pulse_no[i] > 0 && algo.pulse_no[j] == 0 {
            let no = algo.pulse_no[i] - 1;
            if no < algo.max_pulses {
                let pulse = &mut algo.pulses[no as usize];
                pulse.last_bin = blb;
                pulse.end_bin = blb + 1;
                pulse.width_bins = pulse.last_bin + 1 - pulse.first_bin;
                pulse.filter_woi = (pulse.end_bin + 1 - pulse.start_bin).min(max_filter_half_width);
            }
        }
    }
}

// VL53LX_f_010: total and ambient events inside the pulse window.
fn pulse_event_sums(
    pulse: &mut PulseData,
    bins: &HistogramBinData,
    vcsel_period: i64,
    ambient_per_bin: i64,
) {
    pulse.total_events = 0;
    pulse.ambient_events = 0;
    for lb in pulse.start_bin..=pulse.end_bin {
        let i = (lb % vcsel_period) as usize;
        pulse.total_events += bins.bin_data[i];
        pulse.ambient_events += ambient_per_bin;
    }
    pulse.signal_events = pulse.total_events - pulse.ambient_events;
}

// VL53LX_f_011: a copy of the frame with everything outside the pulse replaced by pad_value.
fn isolate_pulse(
    pulse: &PulseData,
    bins: &HistogramBinData,
    first_rising_bin: i64,
    vcsel_period: i64,
    pad_value: i64,
) -> HistogramBinData {
    let mut output = bins.clone();
    for lb in first_rising_bin..first_rising_bin + vcsel_period {
        if lb < pulse.start_bin || lb > pulse.end_bin {
            let i = lb % vcsel_period;
            if i < output.bins_in_data {
                output.bin_data[i as usize] = pad_value;
            }
        }
    }
    output
}

// VL53LX_f_020: the centre of mass of the bins in [start, end], in 1/2048ths of a bin.
fn weighted_phase(
    start: i64,
    end: i64,
    vcsel_period: i64,
    clip_events: bool,
    bins: &HistogramBinData,
) -> i64 {
    if vcsel_period == 0 {
        return MAX_ALLOWED_PHASE;
    }
    let mut event_sum = 0i64;
    let mut weighted_sum = 0i64;
    for lb in start..=end {
        let i = if lb < 0 {
            lb + vcsel_period
        } else {
            lb % vcsel_period
        };
        if i >= 0 && (i as usize) < HISTOGRAM_BUFFER_SIZE {
            let mut value = bins.bin_data[i as usize] - bins.ambient_per_bin;
            if clip_events && value < 0 {
                value = 0;
            }
            event_sum += value;
            weighted_sum += value * (1024 + 2048 * lb);
        }
    }
    if event_sum <= 0 {
        return MAX_ALLOWED_PHASE;
    }
    weighted_sum += event_sum / 2;
    weighted_sum /= event_sum;
    weighted_sum.max(0)
}

// VL53LX_f_015: the near and far phase of the pulse — the centres of mass of its leading and trailing
// bins.
fn pulse_phase_limits(
    pulse: &mut PulseData,
    clip_events: bool,
    bins: &HistogramBinData,
    vcsel_period: i64,
) {
    if pulse.peak_bin == 0xFF {
        pulse.peak_bin = 1;
    }
    let i = pulse.peak_bin % vcsel_period;
    let start = i + pulse.start_bin - pulse.peak_bin;
    let end = i + pulse.end_bin - pulse.peak_bin;
    let window_width = (end - start).min(3);

    pulse.phase_start =
        weighted_phase(start, start + window_width, vcsel_period, clip_events, bins);
    pulse.phase_end = weighted_phase(end - window_width, end, vcsel_period, clip_events, bins);
    if pulse.phase_start > pulse.phase_end {
        std::mem::swap(&mut pulse.phase_start, &mut pulse.phase_end);
    }
    pulse.phase_start = pulse.phase_start.min(pulse.phase_mean);
    pulse.phase_end = pulse.phase_end.max(pulse.phase_mean);
}

// VL53LX_f_016: strongest first or closest first. Stable, as the reference's sort is (sort_by keeps
// ties in their original order).
fn sort_pulses(target_order: i64, algo: &mut Gen3Algo) {
    if algo.pulse_count <= 1 {
        return;
    }
    let head = &mut algo.pulses[..algo.pulse_count as usize];
    if target_order == HIST_TARGET_ORDER_STRONGEST_FIRST {
        head.sort_by_key(|p| std::cmp::Reverse(p.signal_events));
    } else {
        head.sort_by_key(|p| p.phase_mean);
    }
}

// VL53LX_f_017: pulse → target, plus the checks that can fail it: sigma over the threshold, and a phase
// outside the window the preset mode is valid for.
fn fill_target(
    range_id: i64,
    valid_phase_low: i64,
    valid_phase_high: i64,
    sigma_thresh: i64,
    bins: &HistogramBinData,
    pulse: &PulseData,
) -> RangeData {
    let mut target = RangeData {
        range_id,
        start_bin: pulse.start_bin,
        first_bin: pulse.first_bin,
        peak_bin: pulse.peak_bin,
        last_bin: pulse.last_bin,
        end_bin: pulse.end_bin,
        width_bins: pulse.width_bins,
        window_bins: pulse.end_bin + 1 - pulse.start_bin,
        zero_distance_phase: bins.zero_distance_phase,
        sigma: pulse.sigma,
        phase_start: pulse.phase_start & 0xFFFF,
        phase_mean: pulse.phase_mean & 0xFFFF,
        phase_end: pulse.phase_end & 0xFFFF,
        total_events: pulse.total_events,
        signal_events: pulse.signal_events,
        ambient_events: pulse.ambient_events,
        total_periods_elapsed: bins.total_periods_elapsed,
        range_status: DEVICE_ERROR_RANGE_COMPLETE_NO_WRAP_CHECK,
        ..RangeData::default()
    };
    if sigma_thresh > 0 && pulse.sigma > sigma_thresh << 5 {
        target.range_status = DEVICE_ERROR_SIGMA_THRESHOLD_CHECK;
    }

    let mut lower = (valid_phase_low << 8) & 0xFFFF;
    lower = if lower < target.zero_distance_phase {
        target.zero_distance_phase - lower
    } else {
        0
    };
    let upper = ((valid_phase_high << 8) + bins.zero_distance_phase) & 0xFFFF;
    if target.phase_mean < lower || target.phase_mean > upper {
        target.range_status = DEVICE_ERROR_RANGE_PHASE_CHECK;
    }
    target
}

// ── peak location (vl53lx_hist_algos_gen4.c) ─────────────────────────────

// VL53LX_f_026: the three-tap window over the pulse; where left and right are both positive the centre
// of mass is inside this bin.
fn filter_pulse(
    pulse: &PulseData,
    pulse_bins: &HistogramBinData,
    vcsel_period: i64,
    ambient_per_bin: i64,
    filtered: &mut Filtered,
) {
    for lb in pulse.start_bin..=pulse.end_bin {
        let i = lb % vcsel_period;
        let (a, b, c) = woi_sums(i, pulse.filter_woi, pulse_bins);
        let i = i as usize;
        filtered.a[i] = a;
        filtered.b[i] = b;
        filtered.c[i] = c;
        filtered.left[i] = a + b - (c + ambient_per_bin);
        filtered.right[i] = b + c - (a + ambient_per_bin);
    }
}

// VL53LX_f_028: the sub-bin phase where (c - a) against (b - ambient) crosses zero; None when the pulse
// has no height (the division by zero C turns into "not a peak after all").
fn interpolate_phase(
    bin_index: i64,
    a: i64,
    b: i64,
    c: i64,
    ambient_per_bin: i64,
    vcsel_period: i64,
) -> Option<i64> {
    let numerator = 4096 * (c - a);
    let half_b_minus_amb = 4096 * (b - ambient_per_bin);
    if half_b_minus_amb == 0 {
        return None;
    }
    let mut mean_phase = 4096 * numerator + half_b_minus_amb;
    mean_phase /= half_b_minus_amb * 2;
    mean_phase += 2048;
    mean_phase += 4096 * bin_index;
    mean_phase = (mean_phase + 1) / 2;
    mean_phase = mean_phase.clamp(0, MAX_ALLOWED_PHASE);
    Some(py_mod(mean_phase, vcsel_period * 2048))
}

// VL53LX_f_027: keep the last bin where the filter puts the centre of mass; that bin and its phase are
// the target.
fn find_peak_bin(
    pulse: &mut PulseData,
    filtered: &mut Filtered,
    vcsel_period: i64,
    bins_in_data: i64,
    ambient_per_bin: i64,
) {
    for lb in pulse.start_bin..pulse.end_bin {
        let i = lb % vcsel_period;
        let j = (lb + 1) % vcsel_period;
        if !(i < bins_in_data && j < bins_in_data) {
            continue;
        }
        let (ii, jj) = (i as usize, j as usize);
        let is_peak = if filtered.left[ii] == 0 && filtered.right[ii] == 0 {
            0
        } else if (filtered.left[ii] >= 0 && filtered.right[ii] >= 0)
            || (filtered.left[ii] < 0
                && filtered.right[ii] >= 0
                && filtered.left[jj] >= 0
                && filtered.right[jj] < 0)
        {
            1
        } else {
            0
        };
        filtered.is_peak[ii] = is_peak;
        if is_peak == 0 {
            continue;
        }
        pulse.peak_bin = lb;
        match interpolate_phase(
            lb,
            filtered.a[ii],
            filtered.b[ii],
            filtered.c[ii],
            ambient_per_bin,
            vcsel_period,
        ) {
            Some(phase) => pulse.phase_mean = phase,
            None => filtered.is_peak[ii] = 0,
        }
    }
}

// ── entry point ──────────────────────────────────────────────────────────

/// `VL53LX_hist_process_data()` + `VL53LX_f_025`, crosstalk off, one merged frame.
pub fn process_data(
    bins_input: &HistogramBinData,
    hpp: &HistPostProcessConfig,
) -> Result<RangeResults> {
    process_data_merged(bins_input, hpp, 1)
}

/// `VL53LX_hist_process_data()` + `VL53LX_f_025`, crosstalk off, `merge_nb` frames merged.
pub fn process_data_merged(
    bins_input: &HistogramBinData,
    hpp: &HistPostProcessConfig,
    merge_nb: i64,
) -> Result<RangeResults> {
    if hpp.crosstalk_compensation_enable != 0 {
        return Err(DepzError::Unsupported(
            "crosstalk compensation is not ported".into(),
        ));
    }

    let mut algo = Gen3Algo::new();
    let mut filtered = Filtered {
        a: [0; N],
        b: [0; N],
        c: [0; N],
        left: [0; N],
        right: [0; N],
        is_peak: [0; N],
    };
    let mut results = RangeResults {
        stream_count: bins_input.result_stream_count,
        ..RangeResults::default()
    };

    // Compensation is off, so the crosstalk histogram (all zeros) is never read and not built.
    let mut bins = average_repeated_bins(bins_input);

    calc_zero_distance_phase(&mut bins);
    estimate_ambient_from_thresholded_bins(hpp.ambient_thresh_sigma0, &mut bins);
    estimate_ambient_from_ambient_bins(&mut bins);
    remove_ambient_bins(&mut bins);

    results.ambient_dmax_mm = [0; 5]; // VL53LX_f_001: zero without the FMT dmax calibration

    ambient_thresholds(
        hpp.ambient_thresh_events_scaler,
        hpp.ambient_thresh_sigma1,
        hpp.min_ambient_thresh_events,
        None,
        &bins,
        &mut algo,
    );
    find_first_rising_edge(&mut algo);
    assign_pulse_numbers(&mut algo);
    pulse_extents(&mut algo);

    let (vp, frb, bid, apb) = (
        algo.vcsel_period,
        algo.first_rising_bin,
        algo.bins_in_data,
        algo.ambient_per_bin,
    );
    for p in 0..algo.pulse_count as usize {
        let pulse = &mut algo.pulses[p];
        pulse_event_sums(pulse, &bins, vp, apb);
        let pulse_amb = isolate_pulse(pulse, &bins, frb, vp, bins.ambient_per_bin);
        let pulse_zero = isolate_pulse(pulse, &bins, frb, vp, 0);

        filter_pulse(pulse, &pulse_amb, vp, apb, &mut filtered);
        find_peak_bin(pulse, &mut filtered, vp, bid, apb);
        pulse.sigma = pulse_sigma(
            pulse,
            hpp.sigma_estimator_sigma_ref_mm,
            vp,
            &pulse_zero,
            &pulse_amb,
            None,
        );
        pulse_phase_limits(pulse, true, &bins, vp);
    }

    sort_pulses(hpp.hist_target_order, &mut algo);

    for p in 0..algo.pulse_count as usize {
        if results.targets.len() >= MAX_RANGE_RESULTS {
            break;
        }
        let pulse = &algo.pulses[p];
        if !(pulse.signal_events > hpp.signal_total_events_limit && pulse.peak_bin < 0xFF) {
            continue;
        }
        let mut target = fill_target(
            results.targets.len() as i64,
            hpp.valid_phase_low,
            hpp.valid_phase_high,
            hpp.sigma_thresh,
            &bins,
            pulse,
        );
        calc_rates(
            &mut target,
            bins.vcsel_width,
            bins.fast_osc_frequency,
            bins.total_periods_elapsed,
            bins.result_dss_actual_effective_spads,
            merge_nb,
        );
        calc_ranges(hpp.gain_factor, hpp.range_offset_mm, &mut target);
        results.targets.push(target);
    }
    Ok(results)
}

// ── frame-to-frame consistency ───────────────────────────────────────────

/// `VL53LX_hist_events_consistency_check()`: scale the previous frame's events onto this exposure and
/// ask whether the difference fits inside the shot noise of the pair, `event_sigma` sixty-fourths
/// wide. → a device error.
pub fn events_consistency_check(
    event_sigma: i64,
    min_effective_spad_count: i64,
    prev_frame: &FrameHistory,
    prev: &PrevTarget,
    target: &RangeData,
) -> i64 {
    if event_sigma == 0 {
        return DEVICE_ERROR_RANGE_COMPLETE;
    }

    // Wrapping where Java's long arithmetic would wrap; a real frame never comes near it.
    let tmpp = (1 + prev_frame.total_periods_elapsed()).wrapping_mul(prev_frame.spads());
    let tmpc = (1 + target.total_periods_elapsed).wrapping_mul(target.spads);

    let mut events_scaler = tmpp.wrapping_mul(4096);
    if tmpc != 0 {
        events_scaler = events_scaler.wrapping_add(floor_div(tmpc, 2)) / tmpc;
    }
    let events_scaler_sq = events_scaler.wrapping_mul(events_scaler).wrapping_add(2048) / 4096;

    let c_signal_events = (target.total_events - target.ambient_events)
        .wrapping_mul(events_scaler)
        .wrapping_add(2048)
        / 4096;
    let c_sig_noise_sq = events_scaler_sq
        .wrapping_mul(target.total_events)
        .wrapping_add(2048)
        / 4096;
    let mut c_amb_noise_sq = events_scaler_sq
        .wrapping_mul(target.ambient_events)
        .wrapping_add(2048)
        / 4096;
    // Ambient is subtracted from both frames, so only its noise survives, at a quarter weight.
    c_amb_noise_sq = c_amb_noise_sq.wrapping_add(2) / 4;
    let p_amb_noise_sq = (prev.ambient_events + 2) / 4;

    let noise_sq_sum = prev
        .total_events
        .wrapping_add(c_sig_noise_sq)
        .wrapping_add(p_amb_noise_sq)
        .wrapping_add(c_amb_noise_sq)
        & 0xFFFF_FFFF;
    let mut tolerance = isqrt(noise_sq_sum * 16);
    tolerance = (tolerance * event_sigma + 32) / 64;

    let p_signal_events = prev.total_events - prev.ambient_events;
    let delta = c_signal_events.wrapping_sub(p_signal_events).wrapping_abs();
    if delta > tolerance && target.spads > min_effective_spad_count {
        DEVICE_ERROR_EVENT_CONSISTENCY
    } else {
        DEVICE_ERROR_RANGE_COMPLETE
    }
}

/// `VL53LX_hist_merged_pulse_check()`: a pulse far wider than one target should be is two targets in
/// one. → a device error.
pub fn merged_pulse_check(min_max_tolerance_mm: i64, target: &RangeData) -> i64 {
    let delta_mm = (target.max_range_mm - target.min_range_mm).abs();
    if min_max_tolerance_mm > 0 && delta_mm > min_max_tolerance_mm {
        DEVICE_ERROR_RANGE_COMPLETE_MERGED_PULSE
    } else {
        DEVICE_ERROR_RANGE_COMPLETE
    }
}

// ── the family-wide result shape ─────────────────────────────────────────

/// `ConvertStatusHisto()` (vl53lx_api.c), the cases the histogram path produces here.
pub fn convert_status(device_error: i64) -> i32 {
    match device_error {
        DEVICE_ERROR_RANGE_PHASE_CHECK => RANGE_STATUS_OUT_OF_BOUNDS_FAIL,
        DEVICE_ERROR_SIGMA_THRESHOLD_CHECK => RANGE_STATUS_SIGMA_FAIL,
        DEVICE_ERROR_RANGE_COMPLETE_NO_WRAP_CHECK => RANGE_STATUS_RANGE_VALID_NO_WRAP_CHECK,
        DEVICE_ERROR_PHASE_CONSISTENCY | DEVICE_ERROR_EVENT_CONSISTENCY => {
            RANGE_STATUS_WRAP_TARGET_FAIL
        }
        DEVICE_ERROR_PREV_RANGE_NO_TARGETS => RANGE_STATUS_TARGET_PRESENT_LACK_OF_SIGNAL,
        DEVICE_ERROR_RANGE_COMPLETE_MERGED_PULSE => RANGE_STATUS_RANGE_VALID_MERGED_PULSE,
        DEVICE_ERROR_RANGE_COMPLETE => RANGE_STATUS_RANGE_VALID,
        _ => RANGE_STATUS_NONE,
    }
}

fn status_text(status: i32) -> String {
    status_name(status)
        .map(str::to_string)
        .unwrap_or_else(|| format!("unknown ({status})"))
}

fn to_target(d: &RangeData) -> Vl53lxTarget {
    let status = convert_status(d.range_status);
    Vl53lxTarget {
        distance_mm: d.median_range_mm as i32,
        status,
        status_text: status_text(status),
        signal_kcps: d.signal_kcps() as f64,
        ambient_kcps: d.ambient_kcps() as f64,
        sigma_mm: d.sigma_mm() as f64,
        min_range_mm: d.min_range_mm as i32,
        max_range_mm: d.max_range_mm as i32,
    }
}

/// The frame's results as the family-wide range: every return in `targets`, the first one also the
/// range itself. Effective SPADs come in 1/256 and are reported whole, as the rest of the family
/// does. `extra`: `stream_count`, `ambient_per_bin`, and with a target `min_range_mm`,
/// `max_range_mm`, `peak_bin`.
pub fn to_range(results: &RangeResults, bins: &HistogramBinData) -> Vl53lxRange {
    let mut extra = BTreeMap::new();
    extra.insert(
        "stream_count".to_string(),
        ExtraValue::Int(results.stream_count),
    );
    let Some(first) = results.targets.first() else {
        extra.insert(
            "ambient_per_bin".to_string(),
            ExtraValue::Int(bins.ambient_per_bin),
        );
        return Vl53lxRange {
            distance_mm: 8191,
            status: RANGE_STATUS_NONE,
            status_text: status_text(RANGE_STATUS_NONE),
            signal_kcps: 0.0,
            ambient_kcps: 0.0,
            sigma_mm: 0.0,
            spads: (bins.result_dss_actual_effective_spads >> 8) as f64,
            targets: Vec::new(),
            extra,
        };
    };
    let t0 = to_target(first);
    extra.insert(
        "min_range_mm".to_string(),
        ExtraValue::Int(first.min_range_mm),
    );
    extra.insert(
        "max_range_mm".to_string(),
        ExtraValue::Int(first.max_range_mm),
    );
    extra.insert("peak_bin".to_string(), ExtraValue::Int(first.peak_bin));
    extra.insert(
        "ambient_per_bin".to_string(),
        ExtraValue::Int(bins.ambient_per_bin),
    );
    Vl53lxRange {
        distance_mm: t0.distance_mm,
        status: t0.status,
        status_text: t0.status_text.clone(),
        signal_kcps: t0.signal_kcps,
        ambient_kcps: t0.ambient_kcps,
        sigma_mm: t0.sigma_mm,
        spads: (first.spads >> 8) as f64,
        targets: results.targets.iter().map(to_target).collect(),
        extra,
    }
}
