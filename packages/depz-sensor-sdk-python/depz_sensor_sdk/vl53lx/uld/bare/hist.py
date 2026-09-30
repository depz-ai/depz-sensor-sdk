"""
uld/vl53lx/hist.py — histogram post-processing: 24 bins -> targets.

This is the half of the BareDriver that never touches the sensor. It takes the
`HistogramBinData` frame `core.py` reads off the device and turns it into
ranges, the way `VL53LX_hist_process_data()` does in C.

**The C source is obfuscated.** ST stripped the names out of every VL53LX
shipment: what is left is `VL53LX_f_NNN` for functions and `VL53LX_p_NNN` for
structure fields. The parameter names survived, and the bodies say what each
one does, so the names here are ours and the C name is in the comment next to
it. Numbering differs between shipments, so a number only means anything
against `temp/STSW-IMG033_L3/…/VL53L3CX_BareDriver/`.

**What the algorithm does**, in one paragraph. A frame is 24 bins of photon
counts spread over one VCSEL period; a target is a pulse sitting on top of the
ambient floor. So: fold repeated bin codes together (the device may sample the
same phase several times), estimate the ambient floor, drop the ambient-only
bins, work out per-bin detection thresholds from that floor, and mark every bin
above its threshold. Runs of marked bins are pulses. Each pulse is then isolated
into a histogram of its own, filtered with a three-tap window whose two halves
say which side of the peak the centre of mass is on, and the zero crossing
between them is interpolated to a sub-bin phase. Phase converts to millimetres
against `zero_distance_phase`; the event sums convert to signal and ambient
rates; the filter sums convert to sigma.

One frame is not enough for the last word on a target, so the API also compares
consecutive ones: `FrameHistory` below keeps the previous frame and runs the
phase and event consistency checks against it, which is what turns "no wrap
check done" into either "valid" or "wrapped target". The driver owns the
history and calls it once per frame.

**Not in this file:** crosstalk compensation (E8.6), the dmax estimate (the
ambient-limited maximum range — `_ambient_dmax()` below is a stub on purpose,
see its docstring) and the crosstalk-monitor consistency check
(`VL53LX_hist_xmonitor_consistency_check()`), which only ever sets the status
of the xtalk monitor target that feeds the dynamic crosstalk corrector — and
that corrector is E8.6 too, so the status would go nowhere.

Reference C driver: ../../../temp/STSW-IMG033_L3/…/VL53L3CX_BareDriver/
  vl53lx_hist_funcs.c, vl53lx_hist_core.c, vl53lx_hist_algos_gen3.c,
  vl53lx_hist_algos_gen4.c, vl53lx_sigma_estimate.c, vl53lx_core_support.c
"""

import collections
import copy
import math

from depz_sensor_sdk.vl53lx.uld.base import Measurement, Target
from depz_sensor_sdk.vl53lx.uld.bare.core import (DEVICESTATE_RANGING_OUTPUT_DATA,
                             HISTOGRAM_BUFFER_SIZE, RANGING_WINDOW_VCSEL_PERIODS,
                             calc_pll_period_us, decode_vcsel_period,
                             duration_maths)

# vl53lx_hist_structs.h / vl53lx_platform_user_config.h
MAX_BIN_SEQUENCE_LENGTH = 6
MAX_BIN_SEQUENCE_CODE   = 15
MAX_PULSES              = 8          # VL53LX_D_001
MAX_RANGE_RESULTS       = 4

# vl53lx_ll_device.h
MAX_ALLOWED_PHASE           = 0xFFFF
SPAD_TOTAL_COUNT_MAX        = (1 << 29) - 1
SPAD_TOTAL_COUNT_RES_THRES  = 1 << 24
SPEED_OF_LIGHT_IN_AIR_DIV_8 = 299704 >> 3

# vl53lx_sigma_estimate.h — the saturation ceilings of the sigma estimate.
SIGMA_INVALID = 0xFFFF               # VL53LX_D_002
D_003 = 0xFFFFFF
D_004 = 0xFFFFFFFFFFFFFF
D_005 = 0x7FFFFFFFFF
D_006 = 0x7FFFFFFFFFFFFFFF
D_007 = 0xFFFFFFFF

# VL53LX_DeviceError (vl53lx_ll_device.h), the few the histogram path sets.
DEVICEERROR_NOUPDATE                   = 0
DEVICEERROR_RANGEPHASECHECK            = 5
DEVICEERROR_SIGMATHRESHOLDCHECK        = 6
DEVICEERROR_PHASECONSISTENCY           = 7
DEVICEERROR_RANGECOMPLETE              = 9
DEVICEERROR_RANGECOMPLETE_NO_WRAP_CHECK = 19
DEVICEERROR_EVENTCONSISTENCY           = 20
DEVICEERROR_RANGECOMPLETE_MERGED_PULSE = 22
DEVICEERROR_PREV_RANGE_NO_TARGETS      = 23

# VL53LX_HIST_TARGET_ORDER__* (vl53lx_ll_device.h)
HIST_TARGET_ORDER_STRONGEST_FIRST = 1
HIST_TARGET_ORDER_CLOSEST_FIRST   = 2

# VL53LX_RangeStatus (vl53lx_def.h) — the user-facing status, the same numbers
# the rest of the family reports, so uld/base.Measurement stays one shape.
RANGESTATUS_RANGE_VALID               = 0
RANGESTATUS_SIGMA_FAIL                = 1
RANGESTATUS_OUTOFBOUNDS_FAIL          = 4
RANGESTATUS_RANGE_VALID_NO_WRAP_CHECK = 6
RANGESTATUS_WRAP_TARGET_FAIL          = 7
RANGESTATUS_RANGE_VALID_MERGED_PULSE  = 11
RANGESTATUS_TARGET_PRESENT_LACK_OF_SIGNAL = 12
RANGESTATUS_NONE                      = 255


# ── C arithmetic ─────────────────────────────────────────────────────────────
def cdiv(a: int, b: int) -> int:
    """C integer division: truncates towards zero, where Python floors.
    `do_division_s()`/`do_division_u()` in the driver are plain `/`, and the
    algorithm divides negative event sums often enough for it to matter."""
    q = abs(a) // abs(b)
    return -q if (a < 0) != (b < 0) else q


def isqrt(num: int) -> int:
    """VL53LX_isqrt() — floor of the square root, on a uint32 argument."""
    return math.isqrt(num & 0xFFFFFFFF)


def calc_pll_period_mm(fast_osc_frequency: int) -> int:
    """VL53LX_calc_pll_period_mm()."""
    pll_period_us = calc_pll_period_us(fast_osc_frequency)
    pll_period_mm = SPEED_OF_LIGHT_IN_AIR_DIV_8 * (pll_period_us >> 2)
    return (pll_period_mm + (1 << 15)) >> 16


def rate_maths(events: int, time_us: int) -> int:
    """VL53LX_rate_maths(): events over a duration -> count rate in 9.7 Mcps.
    The fractional width drops from 7 to 3 bits above the resolution threshold
    so that the intermediate shift cannot overflow."""
    tmp = 0
    if events > SPAD_TOTAL_COUNT_MAX:
        tmp = SPAD_TOTAL_COUNT_MAX
    elif events > 0:
        tmp = events

    frac_bits = 3 if events > SPAD_TOTAL_COUNT_RES_THRES else 7
    if time_us > 0:
        tmp = ((tmp << frac_bits) + (time_us // 2)) // time_us
    if events > SPAD_TOTAL_COUNT_RES_THRES:
        tmp <<= 4
    return min(tmp, 0xFFFF)


def rate_per_spad_maths(frac_bits: int, peak_count_rate: int, num_spads: int,
                        max_output_value: int) -> int:
    """VL53LX_rate_per_spad_maths()."""
    if num_spads > 0:
        tmp = (peak_count_rate << 8) << frac_bits
        tmp = (tmp + num_spads // 2) // num_spads
    else:
        tmp = peak_count_rate << frac_bits
    return min(tmp, max_output_value)


def events_per_spad_maths(events: int, num_spads: int, duration: int) -> int:
    """VL53LX_events_per_spad_maths() -> kcps per SPAD."""
    total = 0
    if num_spads != 0:
        total = (events * 1000 * 256) // num_spads
    if duration > 0:
        per_spad = ((total << 11) + duration // 2) // duration
    else:
        per_spad = total << 11
    return per_spad & 0xFFFFFFFF


def range_maths(fast_osc_frequency: int, phase: int, zero_distance_phase: int,
                fractional_bits: int, gain_factor: int,
                range_offset_mm: int) -> int:
    """VL53LX_range_maths(): phase -> millimetres."""
    pll_period_us = calc_pll_period_us(fast_osc_frequency)

    tmp = phase - zero_distance_phase
    tmp = tmp * pll_period_us
    tmp = cdiv(tmp, 1 << 9)
    tmp = tmp * SPEED_OF_LIGHT_IN_AIR_DIV_8
    tmp = cdiv(tmp, 1 << 22)

    range_mm = tmp + range_offset_mm
    range_mm *= gain_factor
    range_mm += 0x0400
    range_mm = cdiv(range_mm, 0x0800)

    if fractional_bits == 0:
        range_mm_10 = cdiv(range_mm * 10, 1 << 2)
        if abs(range_mm_10 - cdiv(range_mm_10, 10) * 10) < 5:
            range_mm = cdiv(range_mm_10, 10)
        else:
            range_mm = cdiv(range_mm_10, 10) + 1
    elif fractional_bits == 1:
        range_mm = cdiv(range_mm, 1 << 1)
    return range_mm


# ── results ──────────────────────────────────────────────────────────────────
class RangeData:
    """`VL53LX_range_data_t`: one target. The event sums and phases are what
    the algorithm produced; `*_range_mm` and the rates are what they mean."""

    def __init__(self):
        self.range_id = 0
        self.start_bin = 0           # VL53LX_p_012, window start
        self.first_bin = 0           # VL53LX_p_019, first bin over threshold
        self.peak_bin = 0            # VL53LX_p_023
        self.last_bin = 0            # VL53LX_p_024
        self.end_bin = 0             # VL53LX_p_013
        self.width_bins = 0          # VL53LX_p_025, bins over threshold
        self.window_bins = 0         # VL53LX_p_029, bins in the window
        self.vcsel_width = 0
        self.fast_osc_frequency = 0
        self.zero_distance_phase = 0
        self.spads = 0               # VL53LX_p_004
        self.total_periods_elapsed = 0
        self.peak_duration_us = 0
        self.woi_duration_us = 0
        self.ambient_events = 0      # VL53LX_p_016
        self.total_events = 0        # VL53LX_p_017
        self.signal_events = 0       # VL53LX_p_010
        self.peak_signal_count_rate_mcps = 0
        self.avg_signal_count_rate_mcps = 0
        self.ambient_count_rate_mcps = 0
        self.total_rate_per_spad_mcps = 0
        self.signal_events_per_spad_kcps = 0     # VL53LX_p_009
        self.sigma = 0               # VL53LX_p_002, 9.7 fixed point mm
        self.phase_start = 0         # VL53LX_p_026
        self.phase_mean = 0          # VL53LX_p_011
        self.phase_end = 0           # VL53LX_p_027
        self.min_range_mm = 0
        self.median_range_mm = 0
        self.max_range_mm = 0
        self.range_status = DEVICEERROR_NOUPDATE

    @property
    def sigma_mm(self) -> int:
        return self.sigma >> 7

    @property
    def signal_kcps(self) -> int:
        """peak_signal_count_rate_mcps is 9.7 Mcps."""
        return self.peak_signal_count_rate_mcps * 1000 // 128

    @property
    def ambient_kcps(self) -> int:
        return self.ambient_count_rate_mcps * 1000 // 128

    def __repr__(self):
        return (f'<RangeData {self.median_range_mm} mm '
                f'[{self.min_range_mm}..{self.max_range_mm}] '
                f'sigma {self.sigma_mm} mm signal {self.signal_kcps} kcps '
                f'status {self.range_status}>')


class RangeResults:
    """`VL53LX_range_results_t`, reduced to the single-zone case."""

    def __init__(self):
        self.stream_count = 0
        self.targets = []            # VL53LX_p_003, active_results entries
        self.ambient_dmax_mm = [0] * 5   # VL53LX_p_022, E8.5
        self.wrap_dmax_mm = 0


class PulseData:
    """`VL53LX_hist_pulse_data_t`: one run of bins above the threshold."""

    def __init__(self):
        self.start_bin = 0           # VL53LX_p_012
        self.first_bin = 0           # VL53LX_p_019
        self.peak_bin = 0xFF         # VL53LX_p_023
        self.last_bin = 0            # VL53LX_p_024
        self.end_bin = 0             # VL53LX_p_013
        self.width_bins = 0          # VL53LX_p_025
        self.filter_woi = 0          # VL53LX_p_051, filter half width
        self.ambient_events = 0      # VL53LX_p_016
        self.total_events = 0        # VL53LX_p_017
        self.signal_events = 0       # VL53LX_p_010
        self.phase_start = 0         # VL53LX_p_026
        self.phase_mean = 0          # VL53LX_p_011
        self.phase_end = 0           # VL53LX_p_027
        self.sigma = 0               # VL53LX_p_002


class _Gen3Algo:
    """`VL53LX_hist_gen3_algo_private_data_t`: the scratch the algorithm keeps
    between its steps — per-bin thresholds and flags, the pulse list, and the
    four working copies of the histogram."""

    def __init__(self):
        self.reset()

    def reset(self):                                        # VL53LX_f_003
        n = HISTOGRAM_BUFFER_SIZE
        self.first_bin = 0                # VL53LX_p_019
        self.buffer_size = n              # VL53LX_p_020
        self.bins_in_data = 0             # VL53LX_p_021
        self.vcsel_period = 0             # VL53LX_p_030, bins per period
        self.bins_over_threshold = 0      # VL53LX_p_039
        self.ambient_per_bin = 0          # VL53LX_p_028
        self.ambient_threshold = 0        # VL53LX_p_031, the last one computed
        self.over_threshold = [0] * n     # VL53LX_p_040
        self.pulse_mask = [0] * n         # VL53LX_p_041
        self.pulse_no = [0] * n           # VL53LX_p_042
        self.threshold = [0] * n          # VL53LX_p_052
        self.first_rising_bin = 0         # VL53LX_p_044
        self.max_pulses = MAX_PULSES      # VL53LX_p_045
        self.pulse_count = 0              # VL53LX_p_046
        self.pulses = [PulseData() for _ in range(MAX_PULSES)]   # VL53LX_p_003
        self.bins = None                  # VL53LX_p_006, the working frame
        self.xtalk = None                 # VL53LX_p_047
        self.pulse_amb = None             # VL53LX_p_048, pulse padded ambient
        self.pulse_zero = None            # VL53LX_p_049, pulse padded zero
        self.pulse_xtalk = None           # VL53LX_p_050


class _Filtered:
    """`VL53LX_hist_gen4_algo_filtered_data_t`: the three-tap window sums and
    the two edge signals whose sign change locates the peak."""

    def __init__(self):
        n = HISTOGRAM_BUFFER_SIZE
        self.a = [0] * n                  # VL53LX_p_007
        self.b = [0] * n                  # VL53LX_p_032
        self.c = [0] * n                  # VL53LX_p_001
        self.left = [0] * n               # VL53LX_p_053
        self.right = [0] * n              # VL53LX_p_054
        self.is_peak = [0] * n            # VL53LX_p_040


# ── bin housekeeping (vl53lx_hist_core.c, vl53lx_core_support.c) ─────────────
def average_repeated_bins(src):                             # VL53LX_f_031
    """Fold the six bin-sequence codes together. The device may sample the
    same phase more than once in a frame; repeats land on the same four output
    bins and are averaged, so what comes out is one bin per distinct code."""
    dst = copy.deepcopy(src)
    dst.bins_in_data = 0
    dst.bin_seq = [MAX_BIN_SEQUENCE_CODE + 1] * MAX_BIN_SEQUENCE_LENGTH
    dst.bin_data = [0] * dst.number_of_bins

    initial_index = [0] * (MAX_BIN_SEQUENCE_CODE + 2)
    repeat_count = [0] * (MAX_BIN_SEQUENCE_CODE + 2)
    seq_length = 0

    for lc in range(MAX_BIN_SEQUENCE_LENGTH):
        bin_cfg = src.bin_seq[lc]
        if repeat_count[bin_cfg] == 0:
            initial_index[bin_cfg] = seq_length * 4
            dst.bin_seq[seq_length] = bin_cfg
            seq_length += 1
        repeat_count[bin_cfg] += 1

        base = initial_index[bin_cfg]
        for i in range(4):
            dst.bin_data[base + i] += src.bin_data[lc * 4 + i]

    dst.bin_rep = [repeat_count[c] if c <= MAX_BIN_SEQUENCE_CODE else 0
                   for c in dst.bin_seq]
    dst.bins_in_data = seq_length * 4

    for code in range(MAX_BIN_SEQUENCE_CODE + 1):
        reps = repeat_count[code]
        if reps > 0:
            base = initial_index[code]
            for i in range(4):
                dst.bin_data[base + i] = (dst.bin_data[base + i]
                                          + reps // 2) // reps

    # Codes 7 and 15 are the ambient-only sequence entries.
    dst.number_of_ambient_bins = 4 if (repeat_count[7] or repeat_count[15]) else 0
    return dst


def calc_zero_distance_phase(bins):    # VL53LX_hist_calc_zero_distance_phase
    period = 2048 * decode_vcsel_period(bins.vcsel_period)
    phase = (period + bins.phasecal_result__reference_phase
             + 2048 * bins.phasecal_result__vcsel_start
             - 2048 * bins.cal_config__vcsel_start)
    bins.zero_distance_phase = (phase % period) if period else 0


def estimate_ambient_from_thresholded_bins(ambient_threshold_sigma, bins):
    """VL53LX_hist_estimate_ambient_from_thresholded_bins(): the ambient floor
    from every bin that sits near the quietest one. Used when the frame has no
    ambient-only bins of its own, and always as the first estimate."""
    bins.min_bin_value = min(bins.bin_data[:bins.bins_in_data], default=0)
    bins.max_bin_value = max(bins.bin_data[:bins.bins_in_data], default=0)

    threshold = isqrt(bins.min_bin_value)
    threshold *= ambient_threshold_sigma
    threshold += 0x07
    threshold >>= 4
    threshold += bins.min_bin_value

    bins.number_of_ambient_samples = 0
    bins.ambient_events_sum = 0
    for value in bins.bin_data[:bins.bins_in_data]:
        if value < threshold:
            bins.ambient_events_sum += value
            bins.number_of_ambient_samples += 1

    if bins.number_of_ambient_samples > 0:
        bins.ambient_per_bin = cdiv(
            bins.ambient_events_sum + bins.number_of_ambient_samples // 2,
            bins.number_of_ambient_samples)


def estimate_ambient_from_ambient_bins(bins):
    """VL53LX_hist_estimate_ambient_from_ambient_bins(): the better estimate,
    from the bins the device took with the VCSEL off. Overrides the one above
    whenever the bin sequence has such bins, which ours do."""
    if bins.number_of_ambient_bins > 0:
        bins.number_of_ambient_samples = bins.number_of_ambient_bins
        bins.ambient_events_sum = sum(bins.bin_data[:bins.number_of_ambient_bins])
        bins.ambient_per_bin = cdiv(
            bins.ambient_events_sum + bins.number_of_ambient_bins // 2,
            bins.number_of_ambient_bins)


def remove_ambient_bins(bins):                # VL53LX_hist_remove_ambient_bins
    """Drop the ambient-only bins off the front, so what is left is the
    ranging window and bin 0 is again phase zero."""
    if (bins.bin_seq[0] & 0x07) == 0x07:
        kept_seq, kept_rep = [], []
        for lc in range(MAX_BIN_SEQUENCE_LENGTH):
            if (bins.bin_seq[lc] & 0x07) != 0x07:
                kept_seq.append(bins.bin_seq[lc])
                kept_rep.append(bins.bin_rep[lc])
        pad = MAX_BIN_SEQUENCE_LENGTH - len(kept_seq)
        bins.bin_seq = kept_seq + [MAX_BIN_SEQUENCE_CODE + 1] * pad
        bins.bin_rep = kept_rep + [0] * pad

    n = bins.number_of_ambient_bins
    if n > 0:
        bins.bin_data = bins.bin_data[n:] + [0] * n
        bins.bins_in_data -= n
        bins.number_of_ambient_bins = 0


def _woi_sums(bin_index, filter_woi, bins):                 # VL53LX_f_022
    """The three-tap window around `bin_index`: everything below it, the bin
    itself, everything above it, over a half width of `filter_woi` bins."""
    a, b, c = 0, bins.bin_data[bin_index], 0
    for w in range((filter_woi << 1) + 1):
        j = ((bin_index + w + bins.bins_in_data) - filter_woi) % bins.bins_in_data
        if w < filter_woi:
            a += bins.bin_data[j]
        elif w > filter_woi:
            c += bins.bin_data[j]
    return a, b, c


def _calc_rates(target, vcsel_width, fast_osc_frequency, total_periods_elapsed,
                spads, merge_nb):                           # VL53LX_f_018
    """Event sums -> count rates, over the durations the pulse actually ran."""
    target.vcsel_width = vcsel_width
    target.fast_osc_frequency = fast_osc_frequency
    target.total_periods_elapsed = total_periods_elapsed
    target.spads = spads

    if fast_osc_frequency == 0 or total_periods_elapsed == 0:
        return False

    pll_period_us = calc_pll_period_us(fast_osc_frequency)
    periods_elapsed = total_periods_elapsed + 1

    target.peak_duration_us = duration_maths(
        pll_period_us, vcsel_width, RANGING_WINDOW_VCSEL_PERIODS,
        periods_elapsed)
    target.woi_duration_us = duration_maths(
        pll_period_us, target.window_bins << 4, RANGING_WINDOW_VCSEL_PERIODS,
        periods_elapsed)

    target.peak_signal_count_rate_mcps = rate_maths(target.signal_events,
                                                    target.peak_duration_us)
    target.avg_signal_count_rate_mcps = rate_maths(target.signal_events,
                                                   target.woi_duration_us)
    target.ambient_count_rate_mcps = rate_maths(target.ambient_events,
                                                target.woi_duration_us)

    count_rate_total = (target.peak_signal_count_rate_mcps
                        + target.ambient_count_rate_mcps)
    if merge_nb > 1:
        count_rate_total //= merge_nb

    target.total_rate_per_spad_mcps = rate_per_spad_maths(
        0x06, count_rate_total, spads, 0xFFFF)
    target.signal_events_per_spad_kcps = events_per_spad_maths(
        target.signal_events, spads, target.peak_duration_us)
    return True


def _calc_ranges(gain_factor, range_offset_mm, target):      # VL53LX_f_019
    for phase, name in ((target.phase_start, 'min_range_mm'),
                        (target.phase_mean, 'median_range_mm'),
                        (target.phase_end, 'max_range_mm')):
        setattr(target, name,
                range_maths(target.fast_osc_frequency, phase,
                            target.zero_distance_phase, 0, gain_factor,
                            range_offset_mm))


# ── sigma (vl53lx_sigma_estimate.c) ──────────────────────────────────────────
def _sigma_estimate(sigma_ref_mm, a, b, c, a_zp, c_zp, bx, ax_zp, cx_zp,
                    ambient_per_bin, fast_osc_frequency):    # VL53LX_f_023
    """The width of the pulse, in 9.7 fixed point millimetres.

    The shape is a ratio: how far the window sums lean to one side, over how
    much signal there is above ambient — a lopsided pulse on little signal is
    an uncertain one. The saturation ceilings are ST's, and the two branches
    that reorder a multiply and a divide are there to keep the 64-bit
    intermediate from overflowing either way.
    """
    if fast_osc_frequency == 0:
        return SIGMA_INVALID

    pll_period_mm = calc_pll_period_mm(fast_osc_frequency)
    b_minus_amb = abs(ambient_per_bin - b)
    a_minus_c = abs(a - c)

    if b_minus_amb == 0:
        return SIGMA_INVALID

    tmp0 = min(b + bx + ambient_per_bin, D_003)

    tmp1 = (a_minus_c * a_minus_c) << 8
    tmp1 = min(tmp1, D_004)
    tmp1 //= b_minus_amb
    tmp1 //= b_minus_amb
    tmp1 = min(tmp1, D_005)

    tmp0 = tmp1 * tmp0

    tmp1 = min(c_zp + cx_zp + a_zp + ax_zp, D_003) << 8

    tmp0 = min(tmp1 + tmp0, D_006)

    if tmp0 > D_007:
        tmp0 = (tmp0 // b_minus_amb) * pll_period_mm
    else:
        tmp0 = (tmp0 * pll_period_mm) // b_minus_amb
    tmp0 = min(tmp0, D_006)

    if tmp0 > D_007:
        tmp0 = ((tmp0 // b_minus_amb) // 4) * pll_period_mm
    else:
        tmp0 = ((tmp0 * pll_period_mm) // b_minus_amb) // 4
    tmp0 = min(tmp0, D_006)

    tmp0 = min(tmp0 >> 2, D_007)

    tmp1 = sigma_ref_mm << 7
    tmp0 = min(tmp0 + tmp1 * tmp1, D_007)

    return isqrt(tmp0)


def _pulse_sigma(pulse, sigma_ref_mm, algo, xtalk_enable):   # VL53LX_f_014
    """The sigma of one pulse, from its two padded copies (and its crosstalk
    copy, when compensation is on)."""
    if algo.vcsel_period == 0:
        return SIGMA_INVALID
    i = pulse.peak_bin % algo.vcsel_period

    a_zp, _, c_zp = _woi_sums(i, pulse.filter_woi, algo.pulse_zero)
    a, b, c = _woi_sums(i, pulse.filter_woi, algo.pulse_amb)
    if xtalk_enable:
        ax, bx, cx = _woi_sums(i, pulse.filter_woi, algo.pulse_xtalk)
    else:
        ax, bx, cx = 0, 0, 0

    return _sigma_estimate(sigma_ref_mm, a, b, c, a_zp, c_zp, bx, ax, cx,
                           algo.pulse_amb.ambient_per_bin,
                           algo.pulse_amb.fast_osc_frequency)


# ── pulse detection (vl53lx_hist_algos_gen3.c) ───────────────────────────────
def _ambient_thresholds(scaler, threshold_sigma, min_threshold_events,
                        xtalk_enable, bins, algo):           # VL53LX_f_006
    """A detection threshold per bin, and the flags saying which bins clear it.

    The threshold is the ambient floor plus a sigma margin, and the margin
    shrinks with the number of samples that went into the bin — a bin the
    device visited four times is four times as trustworthy as one it visited
    once, which is what `bin_rep` counts.
    """
    algo.buffer_size = bins.number_of_bins
    algo.first_bin = bins.first_bin
    algo.bins_in_data = bins.bins_in_data
    algo.ambient_per_bin = bins.ambient_per_bin
    algo.vcsel_period = decode_vcsel_period(bins.vcsel_period)

    amb_events = cdiv(bins.ambient_per_bin * scaler + 2048, 4096)

    for lb in range(bins.bins_in_data):
        samples = bins.bin_rep[lb >> 2]
        if samples <= 0:
            continue

        if lb < algo.xtalk.bins_in_data and xtalk_enable:
            value = samples * (amb_events + algo.xtalk.bin_data[lb])
        else:
            value = samples * amb_events

        value = isqrt(value)
        value += samples // 2
        value //= samples
        value *= threshold_sigma
        value += 8
        value //= 16
        value += amb_events
        value = max(value, min_threshold_events)

        algo.threshold[lb] = value
        algo.ambient_threshold = value

    algo.bins_over_threshold = 0
    for lb in range(bins.first_bin, bins.bins_in_data):
        over = 1 if bins.bin_data[lb] > algo.threshold[lb] else 0
        algo.over_threshold[lb] = over
        algo.pulse_mask[lb] = over
        algo.bins_over_threshold += over


def _find_first_rising_edge(algo):                           # VL53LX_f_007
    """Where to start walking the histogram: the first quiet-to-loud step, so
    that a pulse wrapped across the end of the period is not cut in two."""
    algo.first_rising_bin = 0
    found = False
    for i in range(algo.vcsel_period):
        j = (i + 1) % algo.vcsel_period
        if i < algo.bins_in_data and j < algo.bins_in_data:
            if algo.pulse_mask[i] == 0 and algo.pulse_mask[j] == 1 and not found:
                algo.first_rising_bin = i
                found = True


def _assign_pulse_numbers(algo):                             # VL53LX_f_008
    """Number the runs of over-threshold bins, from the first rising edge."""
    for lb in range(algo.first_rising_bin,
                    algo.first_rising_bin + algo.vcsel_period):
        i = lb % algo.vcsel_period
        j = (lb + 1) % algo.vcsel_period
        if not (i < algo.bins_in_data and j < algo.bins_in_data):
            continue

        if algo.pulse_mask[i] == 0 and algo.pulse_mask[j] == 1:
            algo.pulse_count += 1
        algo.pulse_count = min(algo.pulse_count, algo.max_pulses)
        algo.pulse_no[i] = algo.pulse_count if algo.pulse_mask[i] > 0 else 0


def _pulse_extents(algo):                                    # VL53LX_f_009
    """Turn each numbered run into a pulse: its window, its width, and the
    filter half width the rest of the algorithm will use on it."""
    max_filter_half_width = (algo.vcsel_period - 1) >> 1

    for blb in range(algo.first_rising_bin,
                     algo.first_rising_bin + algo.vcsel_period):
        i = blb % algo.vcsel_period
        j = (blb + 1) % algo.vcsel_period
        if not (i < algo.bins_in_data and j < algo.bins_in_data):
            continue

        if algo.pulse_no[i] == 0 and algo.pulse_no[j] > 0:
            no = algo.pulse_no[j] - 1
            if no < algo.max_pulses:
                pulse = algo.pulses[no]
                pulse.start_bin = blb
                pulse.first_bin = blb + 1
                pulse.peak_bin = 0xFF
                pulse.last_bin = 0
                pulse.end_bin = 0

        if algo.pulse_no[i] > 0 and algo.pulse_no[j] == 0:
            no = algo.pulse_no[i] - 1
            if no < algo.max_pulses:
                pulse = algo.pulses[no]
                pulse.last_bin = blb
                pulse.end_bin = blb + 1
                pulse.width_bins = (pulse.last_bin + 1) - pulse.first_bin
                pulse.filter_woi = min((pulse.end_bin + 1) - pulse.start_bin,
                                       max_filter_half_width)


def _pulse_event_sums(pulse_no, bins, algo):                 # VL53LX_f_010
    """Total and ambient events inside the pulse window."""
    pulse = algo.pulses[pulse_no]
    pulse.total_events = 0
    pulse.ambient_events = 0
    for lb in range(pulse.start_bin, pulse.end_bin + 1):
        i = lb % algo.vcsel_period
        pulse.total_events += bins.bin_data[i]
        pulse.ambient_events += algo.ambient_per_bin
    pulse.signal_events = pulse.total_events - pulse.ambient_events


def _isolate_pulse(pulse_no, bins, algo, pad_value):         # VL53LX_f_011
    """A copy of the frame with everything outside the pulse replaced by
    `pad_value` — ambient for the estimate that keeps the floor, zero for the
    one that removes it."""
    pulse = algo.pulses[pulse_no]
    out = copy.deepcopy(bins)
    for lb in range(algo.first_rising_bin,
                    algo.first_rising_bin + algo.vcsel_period):
        if lb < pulse.start_bin or lb > pulse.end_bin:
            i = lb % algo.vcsel_period
            if i < out.bins_in_data:
                out.bin_data[i] = pad_value
    return out


def _weighted_phase(start, end, vcsel_period, clip_events, bins):  # VL53LX_f_020
    """Centre of mass of the bins in [start, end], in 1/2048ths of a bin."""
    event_sum = 0
    weighted_sum = 0
    if vcsel_period == 0:
        return MAX_ALLOWED_PHASE

    for lb in range(start, end + 1):
        i = lb + vcsel_period if lb < 0 else lb % vcsel_period
        if 0 <= i < HISTOGRAM_BUFFER_SIZE:
            value = bins.bin_data[i] - bins.ambient_per_bin
            if clip_events and value < 0:
                value = 0
            event_sum += value
            weighted_sum += value * (1024 + 2048 * lb)

    if event_sum > 0:
        weighted_sum += cdiv(event_sum, 2)
        weighted_sum = cdiv(weighted_sum, event_sum)
        return max(weighted_sum, 0)
    return MAX_ALLOWED_PHASE


def _pulse_phase_limits(pulse_no, clip_events, bins, algo):  # VL53LX_f_015
    """The near and far phase of the pulse: the centre of mass of its leading
    three bins and of its trailing three, which is what min_range/max_range
    come from."""
    pulse = algo.pulses[pulse_no]
    if pulse.peak_bin == 0xFF:
        pulse.peak_bin = 1

    i = pulse.peak_bin % algo.vcsel_period
    start = i + pulse.start_bin - pulse.peak_bin
    end = i + pulse.end_bin - pulse.peak_bin

    window_width = min(end - start, 3)

    pulse.phase_start = _weighted_phase(start, start + window_width,
                                        algo.vcsel_period, clip_events, bins)
    pulse.phase_end = _weighted_phase(end - window_width, end,
                                      algo.vcsel_period, clip_events, bins)

    if pulse.phase_start > pulse.phase_end:
        pulse.phase_start, pulse.phase_end = pulse.phase_end, pulse.phase_start
    pulse.phase_start = min(pulse.phase_start, pulse.phase_mean)
    pulse.phase_end = max(pulse.phase_end, pulse.phase_mean)


def _sort_pulses(target_order, algo):                        # VL53LX_f_016
    """Strongest first or closest first, as the post-processing config says."""
    if algo.pulse_count <= 1:
        return
    pulses = algo.pulses[:algo.pulse_count]
    if target_order == HIST_TARGET_ORDER_STRONGEST_FIRST:
        pulses.sort(key=lambda p: -p.signal_events)
    else:
        pulses.sort(key=lambda p: p.phase_mean)
    algo.pulses[:algo.pulse_count] = pulses


def _fill_target(range_id, valid_phase_low, valid_phase_high, sigma_thresh,
                 bins, pulse, target):                       # VL53LX_f_017
    """Pulse -> target, plus the two checks that can fail it: sigma over the
    threshold, and a phase outside the window the preset mode is valid for."""
    target.range_id = range_id
    target.start_bin = pulse.start_bin
    target.first_bin = pulse.first_bin
    target.peak_bin = pulse.peak_bin
    target.last_bin = pulse.last_bin
    target.end_bin = pulse.end_bin
    target.width_bins = pulse.width_bins
    target.window_bins = (pulse.end_bin + 1) - pulse.start_bin

    target.zero_distance_phase = bins.zero_distance_phase
    target.sigma = pulse.sigma
    target.phase_start = pulse.phase_start & 0xFFFF
    target.phase_mean = pulse.phase_mean & 0xFFFF
    target.phase_end = pulse.phase_end & 0xFFFF
    target.total_events = pulse.total_events
    target.signal_events = pulse.signal_events
    target.ambient_events = pulse.ambient_events
    target.total_periods_elapsed = bins.total_periods_elapsed

    target.range_status = DEVICEERROR_RANGECOMPLETE_NO_WRAP_CHECK

    if sigma_thresh > 0 and pulse.sigma > (sigma_thresh << 5):
        target.range_status = DEVICEERROR_SIGMATHRESHOLDCHECK

    lower = (valid_phase_low << 8) & 0xFFFF
    lower = target.zero_distance_phase - lower if lower < target.zero_distance_phase else 0
    upper = ((valid_phase_high << 8) + bins.zero_distance_phase) & 0xFFFF

    if target.phase_mean < lower or target.phase_mean > upper:
        target.range_status = DEVICEERROR_RANGEPHASECHECK


# ── peak location (vl53lx_hist_algos_gen4.c) ─────────────────────────────────
def _filter_pulse(pulse_no, pulse_bins, algo, filtered):     # VL53LX_f_026
    """Run the three-tap window over the pulse. `left` and `right` are the two
    halves of the window minus the other side; where both are positive the
    centre of mass is inside this bin."""
    pulse = algo.pulses[pulse_no]
    for lb in range(pulse.start_bin, pulse.end_bin + 1):
        i = lb % algo.vcsel_period
        a, b, c = _woi_sums(i, pulse.filter_woi, pulse_bins)
        filtered.a[i] = a
        filtered.b[i] = b
        filtered.c[i] = c
        filtered.left[i] = (a + b) - (c + algo.ambient_per_bin)
        filtered.right[i] = (b + c) - (a + algo.ambient_per_bin)


def _interpolate_phase(bin_index, a, b, c, ambient_per_bin,
                       vcsel_period):                        # VL53LX_f_028
    """Sub-bin phase from the window sums: where between the two neighbours
    the zero crossing of (c - a) against (b - ambient) falls. Returns None
    when the pulse has no height, which is the division by zero the C driver
    turns into 'not a peak after all'."""
    numerator = 4096 * (c - a)
    half_b_minus_amb = 4096 * (b - ambient_per_bin)
    if half_b_minus_amb == 0:
        return None

    mean_phase = (4096 * numerator) + half_b_minus_amb
    mean_phase = cdiv(mean_phase, half_b_minus_amb * 2)
    mean_phase += 2048
    mean_phase += 4096 * bin_index
    mean_phase = cdiv(mean_phase + 1, 2)

    mean_phase = min(max(mean_phase, 0), MAX_ALLOWED_PHASE)
    return mean_phase % (vcsel_period * 2048)


def _find_peak_bin(pulse_no, filtered, algo):                # VL53LX_f_027
    """Walk the pulse and keep the last bin where the filter says the centre
    of mass is here; that bin and its interpolated phase are the target."""
    pulse = algo.pulses[pulse_no]
    for lb in range(pulse.start_bin, pulse.end_bin):
        i = lb % algo.vcsel_period
        j = (lb + 1) % algo.vcsel_period
        if not (i < algo.bins_in_data and j < algo.bins_in_data):
            continue

        if filtered.left[i] == 0 and filtered.right[i] == 0:
            is_peak = 0
        elif filtered.left[i] >= 0 and filtered.right[i] >= 0:
            is_peak = 1
        elif (filtered.left[i] < 0 and filtered.right[i] >= 0
              and filtered.left[j] >= 0 and filtered.right[j] < 0):
            is_peak = 1
        else:
            is_peak = 0
        filtered.is_peak[i] = is_peak

        if is_peak:
            pulse.peak_bin = lb
            phase = _interpolate_phase(lb, filtered.a[i], filtered.b[i],
                                       filtered.c[i], algo.ambient_per_bin,
                                       algo.vcsel_period)
            if phase is None:
                filtered.is_peak[i] = 0
            else:
                pulse.phase_mean = phase


def _ambient_dmax(results):
    """`VL53LX_f_001` — the ambient-limited maximum range, five reflectances of
    it — deliberately left as this stub (E8.5).

    Not laziness: f_001 computes the estimate inside
    `if (pcal->ref__actual_effective_spads != 0 && ...)` and otherwise leaves
    `*pambient_dmax_mm` at the zero it opens with. That calibration comes from
    the FMT 140MM_DARK NVM region, which our boards ship blank (see the NVM
    note in the plan), so a faithful port would return this same zero on every
    frame. Nothing in the target extraction reads the value either. Port it if
    a board ever turns up with the FMT region programmed."""
    results.ambient_dmax_mm = [0] * 5


# ── entry point ──────────────────────────────────────────────────────────────
def process_data(bins_input, hpp, merge_nb=1) -> RangeResults:
    """`VL53LX_hist_process_data()` + `VL53LX_f_025`, with crosstalk off.

    `bins_input` is a `HistogramBinData` as `BareDriver.get_histogram_bin_data()`
    returns it; `hpp` is the driver's `HistPostProcessConfig`.
    """
    if hpp.algo__crosstalk_compensation_enable:
        raise NotImplementedError('crosstalk compensation is E8.6')

    algo = _Gen3Algo()
    filtered = _Filtered()
    results = RangeResults()

    bins = average_repeated_bins(bins_input)                 # VL53LX_f_031

    algo.bins = bins
    # The crosstalk histogram, zero everywhere while compensation is off. It
    # keeps its full width so the window sums over it stay well defined.
    algo.xtalk = copy.deepcopy(bins)
    algo.xtalk.bin_data = [0] * bins.number_of_bins

    results.stream_count = bins_input.result__stream_count

    calc_zero_distance_phase(bins)
    estimate_ambient_from_thresholded_bins(hpp.ambient_thresh_sigma0, bins)
    estimate_ambient_from_ambient_bins(bins)
    remove_ambient_bins(bins)

    _ambient_dmax(results)

    _ambient_thresholds(hpp.ambient_thresh_events_scaler,
                        hpp.ambient_thresh_sigma1,
                        hpp.min_ambient_thresh_events,
                        0, bins, algo)
    _find_first_rising_edge(algo)
    _assign_pulse_numbers(algo)
    _pulse_extents(algo)

    for p in range(algo.pulse_count):
        pulse = algo.pulses[p]

        _pulse_event_sums(p, bins, algo)
        algo.pulse_amb = _isolate_pulse(p, bins, algo, bins.ambient_per_bin)
        algo.pulse_zero = _isolate_pulse(p, bins, algo, 0)
        algo.pulse_xtalk = _isolate_pulse(p, algo.xtalk, algo, 0)

        _filter_pulse(p, algo.pulse_amb, algo, filtered)
        _find_peak_bin(p, filtered, algo)
        pulse.sigma = _pulse_sigma(pulse, hpp.sigma_estimator__sigma_ref_mm,
                                   algo, xtalk_enable=0)
        _pulse_phase_limits(p, 1, bins, algo)

    _sort_pulses(hpp.hist_target_order, algo)

    for p in range(algo.pulse_count):
        if len(results.targets) >= MAX_RANGE_RESULTS:
            break
        pulse = algo.pulses[p]
        if not (pulse.signal_events > hpp.signal_total_events_limit
                and pulse.peak_bin < 0xFF):
            continue

        target = RangeData()
        _fill_target(len(results.targets), hpp.valid_phase_low,
                     hpp.valid_phase_high, hpp.sigma_thresh, bins, pulse,
                     target)
        _calc_rates(target, bins.vcsel_width, bins.fast_osc_frequency,
                    bins.total_periods_elapsed,
                    bins.result__dss_actual_effective_spads, merge_nb)
        _calc_ranges(hpp.gain_factor, hpp.range_offset_mm, target)
        results.targets.append(target)

    return results



# ── frame-to-frame consistency ───────────────────────────────────────────────
PrevTarget = collections.namedtuple(
    'PrevTarget', 'ambient_events total_events phase_mean range_status')


class FrameHistory:
    """The frame before this one, reduced to what the consistency checks read:
    `VL53LX_zone_hist_info_t` and `VL53LX_zone_objects_t` for our single zone.

    The engine alternates two VCSEL periods, so a single frame cannot say
    whether a target sits inside the unambiguous range or is an echo wrapped
    into it. The API answers that by comparing consecutive frames, and this is
    the memory that comparison needs. `BareDriver` keeps no history of its own,
    so the driver owns one of these and hands it every frame.
    """

    def __init__(self):
        self.reset()

    def reset(self):
        """Forget the stream so far. The next `apply()` only remembers: with no
        previous frame there is nothing to be consistent with."""
        self.rd_device_state = None
        self.total_periods_elapsed = 0
        self.spads = 0
        self.targets = []

    def apply(self, results, bins, rd_device_state, hpp):
        """Check `results` against the remembered frame, then remember it.

        The order is C's: `get_device_results()` runs both checks and only
        afterwards copies `hist_data` and `range_results` into the zone
        history.
        """
        self._phase_consistency_check(results, hpp)
        self.rd_device_state = rd_device_state
        self.total_periods_elapsed = bins.total_periods_elapsed
        self.spads = bins.result__dss_actual_effective_spads
        self.targets = [PrevTarget(t.ambient_events, t.total_events,
                                   t.phase_mean, t.range_status)
                        for t in results.targets]

    def _phase_consistency_check(self, results, hpp):
        """`VL53LX_hist_phase_consistency_check()`.

        Every target that came out of this frame as a range starts guilty and
        has to find itself in the previous frame: a target within
        `phase_tolerance` of one there, carrying a consistent number of events,
        is a real target and its status becomes RANGECOMPLETE. One that finds
        nothing was on the other side of a wrap and stays PHASECONSISTENCY.
        """
        # The tolerance is a whole phase unit, and phases here are 8.8.
        phase_tolerance = hpp.algo__consistency_check__phase_tolerance << 8
        # C also admits RANGING_GATHER_DATA, which is the multizone path; the
        # single zone we run goes SW_STANDBY -> WAIT_GPH_SYNC -> OUTPUT_DATA
        # and stays there.
        if self.rd_device_state != DEVICESTATE_RANGING_OUTPUT_DATA:
            return
        if phase_tolerance == 0:
            return

        event_sigma = hpp.algo__consistency_check__event_sigma
        min_spads = hpp.algo__consistency_check__event_min_spad_count
        min_max_tolerance = hpp.algo__consistency_check__min_max_tolerance

        for target in results.targets:
            if target.range_status not in (
                    DEVICEERROR_RANGECOMPLETE,
                    DEVICEERROR_RANGECOMPLETE_NO_WRAP_CHECK):
                continue

            target.range_status = (DEVICEERROR_PHASECONSISTENCY
                                   if self.targets
                                   else DEVICEERROR_PREV_RANGE_NO_TARGETS)

            for prev in self.targets:
                if abs(target.phase_mean - prev.phase_mean) >= phase_tolerance:
                    continue
                status = _events_consistency_check(event_sigma, min_spads,
                                                   self, prev, target)
                if status == DEVICEERROR_RANGECOMPLETE:
                    status = _merged_pulse_check(min_max_tolerance, target)
                target.range_status = status


def _events_consistency_check(event_sigma, min_effective_spad_count,
                              prev_frame, prev, target):
    """`VL53LX_hist_events_consistency_check()` -> a device error code.

    Two frames are not the same exposure: the periods elapsed and the effective
    SPAD count both move. So scale the previous frame's event counts onto this
    one and ask whether what is left of the difference fits inside the shot
    noise of the pair, `event_sigma` sixty-fourths wide.
    """
    if event_sigma == 0:
        return DEVICEERROR_RANGECOMPLETE

    tmpp = (1 + prev_frame.total_periods_elapsed) * prev_frame.spads
    tmpc = (1 + target.total_periods_elapsed) * target.spads

    events_scaler = tmpp * 4096
    if tmpc != 0:
        events_scaler = cdiv(events_scaler + tmpc // 2, tmpc)
    events_scaler_sq = cdiv(events_scaler * events_scaler + 2048, 4096)

    c_signal_events = cdiv(
        (target.total_events - target.ambient_events) * events_scaler + 2048,
        4096)
    c_sig_noise_sq = cdiv(events_scaler_sq * target.total_events + 2048, 4096)
    c_amb_noise_sq = cdiv(events_scaler_sq * target.ambient_events + 2048, 4096)
    # Ambient lands in the estimate a quarter of its weight: it is subtracted
    # from both frames, so only its noise survives.
    c_amb_noise_sq = cdiv(c_amb_noise_sq + 2, 4)
    p_amb_noise_sq = cdiv(prev.ambient_events + 2, 4)

    noise_sq_sum = (prev.total_events + c_sig_noise_sq
                    + p_amb_noise_sq + c_amb_noise_sq) & 0xFFFFFFFF
    tolerance = isqrt(noise_sq_sum * 16)
    tolerance = cdiv(tolerance * event_sigma + 32, 64)

    p_signal_events = prev.total_events - prev.ambient_events
    delta = abs(c_signal_events - p_signal_events)

    if delta > tolerance and target.spads > min_effective_spad_count:
        return DEVICEERROR_EVENTCONSISTENCY
    return DEVICEERROR_RANGECOMPLETE


def _merged_pulse_check(min_max_tolerance_mm, target):
    """`VL53LX_hist_merged_pulse_check()`. A pulse far wider than one target
    should be is two targets the extraction failed to split; the range is still
    good, and saying so is what RANGECOMPLETE_MERGED_PULSE is for."""
    delta_mm = abs(target.max_range_mm - target.min_range_mm)
    if min_max_tolerance_mm > 0 and delta_mm > min_max_tolerance_mm:
        return DEVICEERROR_RANGECOMPLETE_MERGED_PULSE
    return DEVICEERROR_RANGECOMPLETE


# ── the family-wide result shape ─────────────────────────────────────────────
def convert_status(device_error: int) -> int:
    """ConvertStatusHisto() (vl53lx_api.c), the cases the histogram path can
    actually produce here. 'No wrap check done' survives only on the first
    frame of a stream, where `FrameHistory` has nothing to compare against."""
    return {
        DEVICEERROR_RANGEPHASECHECK: RANGESTATUS_OUTOFBOUNDS_FAIL,
        DEVICEERROR_SIGMATHRESHOLDCHECK: RANGESTATUS_SIGMA_FAIL,
        DEVICEERROR_RANGECOMPLETE_NO_WRAP_CHECK:
            RANGESTATUS_RANGE_VALID_NO_WRAP_CHECK,
        DEVICEERROR_PHASECONSISTENCY: RANGESTATUS_WRAP_TARGET_FAIL,
        DEVICEERROR_EVENTCONSISTENCY: RANGESTATUS_WRAP_TARGET_FAIL,
        DEVICEERROR_PREV_RANGE_NO_TARGETS:
            RANGESTATUS_TARGET_PRESENT_LACK_OF_SIGNAL,
        DEVICEERROR_RANGECOMPLETE_MERGED_PULSE:
            RANGESTATUS_RANGE_VALID_MERGED_PULSE,
        DEVICEERROR_RANGECOMPLETE: RANGESTATUS_RANGE_VALID,
    }.get(device_error, RANGESTATUS_NONE)


# UM2931 range status names, as uld/l4.py spells them.
STATUS_NAMES = {
    RANGESTATUS_RANGE_VALID: 'valid',
    RANGESTATUS_SIGMA_FAIL: 'sigma above threshold',
    RANGESTATUS_OUTOFBOUNDS_FAIL: 'phase out of valid limit',
    RANGESTATUS_RANGE_VALID_NO_WRAP_CHECK: 'no wrap-around check done',
    RANGESTATUS_WRAP_TARGET_FAIL: 'wrapped target',
    RANGESTATUS_RANGE_VALID_MERGED_PULSE: 'valid, merged pulse',
    RANGESTATUS_TARGET_PRESENT_LACK_OF_SIGNAL: 'no target in the frame before',
    RANGESTATUS_NONE: 'no target',
}


def _to_target(data: RangeData) -> Target:
    status = convert_status(data.range_status)
    return Target(distance_mm=data.median_range_mm,
                  status=status,
                  status_text=STATUS_NAMES.get(status, f'unknown ({status})'),
                  signal_kcps=data.signal_kcps,
                  ambient_kcps=data.ambient_kcps,
                  sigma_mm=data.sigma_mm,
                  min_range_mm=data.min_range_mm,
                  max_range_mm=data.max_range_mm)


def to_measurement(results: RangeResults, bins) -> Measurement:
    """RangeResults -> the Measurement the CLI and GUI speak. Every return the
    frame produced goes into `targets`; the first one is also the measurement
    itself, because that is the one field every other sensor of the family
    fills."""
    if not results.targets:
        return Measurement(distance_mm=8191, status=RANGESTATUS_NONE,
                           status_text=STATUS_NAMES[RANGESTATUS_NONE],
                           spads=bins.result__dss_actual_effective_spads >> 8,
                           extra={'stream_count': results.stream_count,
                                  'ambient_per_bin': bins.ambient_per_bin})

    first = results.targets[0]
    t0 = _to_target(first)
    return Measurement(
        distance_mm=t0.distance_mm,
        status=t0.status,
        status_text=t0.status_text,
        signal_kcps=t0.signal_kcps,
        ambient_kcps=t0.ambient_kcps,
        sigma_mm=t0.sigma_mm,
        # The device counts effective SPADs in 1/256; the rest of the family
        # reports whole ones, and `Measurement` is one shape for all of them.
        spads=first.spads >> 8,
        targets=tuple(_to_target(t) for t in results.targets),
        extra={'stream_count': results.stream_count,
               'min_range_mm': first.min_range_mm,
               'max_range_mm': first.max_range_mm,
               'peak_bin': first.peak_bin,
               'ambient_per_bin': bins.ambient_per_bin},
    )
