"""The histogram post-processing (uld/bare/hist.py) on its own, no hardware.

The driver tests run it end to end on the committed frames with ST's default
tuning, which leaves several of its checks switched off (event sigma and the
min/max tolerance are 0 in the tuning table) and never feeds it a degenerate
frame. This covers what those runs cannot reach: the frame-to-frame checks
with their tuning switched on, the sigma and phase-window rejections, the
target order, and the guards of the fixed-point maths, each against the
VL53LX status it is meant to produce (UM2931 range status, ConvertStatusHisto).
"""

from __future__ import annotations

import copy
import json
from pathlib import Path

import pytest

from depz_sensor_sdk.vl53lx import Vl53lx
from depz_sensor_sdk.vl53lx.uld.bare import hist
from depz_sensor_sdk.vl53lx.uld.bare.core import HistogramBinData
from depz_sensor_sdk.vl53lx.uld.bare.hist import (
    DEVICEERROR_EVENTCONSISTENCY, DEVICEERROR_PHASECONSISTENCY,
    DEVICEERROR_PREV_RANGE_NO_TARGETS, DEVICEERROR_RANGECOMPLETE,
    DEVICEERROR_RANGECOMPLETE_MERGED_PULSE, DEVICEERROR_RANGECOMPLETE_NO_WRAP_CHECK,
    DEVICEERROR_RANGEPHASECHECK, DEVICEERROR_SIGMATHRESHOLDCHECK,
    HIST_TARGET_ORDER_CLOSEST_FIRST, MAX_ALLOWED_PHASE, SIGMA_INVALID,
    FrameHistory, PrevTarget, RangeData, RangeResults)
from fake_vl53lx import DIE_OSC, FakeVl53lxBridge

L4CX_BOARD = "DEPZ ToF Sensor VL53L4CX USB v2.1 TOVJALN523"
VECTORS = Path(__file__).resolve().parents[3] / "contracts" / "vectors" / "vl53lx.json"
OUTPUT_DATA = "output_data"


def _raw(name):
    for c in json.loads(VECTORS.read_text(encoding="utf-8"))["histogram_raw"]:
        if c["name"] == name:
            return bytes.fromhex(c["raw"])
    raise KeyError(name)


@pytest.fixture(scope="module")
def frames():
    """Two consecutive VL53L4CX frames (stream 1 and 8) as the driver decodes
    them, and its post-processing config as init + start_ranging leave it."""
    fake = FakeVl53lxBridge(L4CX_BOARD)
    dev = Vl53lx(fake.link, timeout=0.5)
    dev.init()
    dev.driver.start_ranging()
    f2 = dev.driver.bin_data(_raw("vl53l4cx_histogram_medium_50ms_f2"))
    f9 = dev.driver.bin_data(_raw("vl53l4cx_histogram_medium_50ms_f9"))
    hpp = copy.deepcopy(dev.driver.bare.hpp)
    dev.close()
    fake.close()
    return f2, f9, hpp


def _target(*, total, ambient, periods=99, spads=4096, phase=0x2000,
            status=DEVICEERROR_RANGECOMPLETE_NO_WRAP_CHECK, min_mm=500, max_mm=500):
    t = RangeData()
    t.total_events, t.ambient_events = total, ambient
    t.total_periods_elapsed, t.spads = periods, spads
    t.phase_mean, t.range_status = phase, status
    t.min_range_mm, t.max_range_mm = min_mm, max_mm
    return t


def _history(prev_targets, *, periods=99, spads=4096, state=OUTPUT_DATA):
    h = FrameHistory()
    h.rd_device_state = state
    h.total_periods_elapsed, h.spads = periods, spads
    h.targets = prev_targets
    return h


def _hpp(base, *, phase_tol=8, event_sigma=0, min_spads=2048, min_max_mm=0):
    hpp = copy.deepcopy(base)
    hpp.algo__consistency_check__phase_tolerance = phase_tol
    hpp.algo__consistency_check__event_sigma = event_sigma
    hpp.algo__consistency_check__event_min_spad_count = min_spads
    hpp.algo__consistency_check__min_max_tolerance = min_max_mm
    return hpp


def _check(history, targets, hpp):
    results = RangeResults()
    results.targets = targets
    history._phase_consistency_check(results, hpp)
    return [t.range_status for t in targets]


# ── frame to frame ───────────────────────────────────────────────────────────

def test_the_same_target_two_frames_running_is_valid_and_the_first_frame_is_not(frames):
    f2, f9, hpp = frames
    h = FrameHistory()
    r2 = hist.process_data(f2, hpp)
    h.apply(r2, f2, OUTPUT_DATA, hpp)
    # Nothing to compare against yet: the first frame keeps 'no wrap check'.
    assert {t.range_status for t in r2.targets} == {DEVICEERROR_RANGECOMPLETE_NO_WRAP_CHECK}
    r9 = hist.process_data(f9, hpp)
    h.apply(r9, f9, OUTPUT_DATA, hpp)
    by_mm = {t.median_range_mm: t.range_status for t in r9.targets}
    # 618 mm was at 613 mm a frame before: within the phase tolerance, valid.
    # 1387 mm had no counterpart in frame 1: on the other side of a wrap.
    assert by_mm == {618: DEVICEERROR_RANGECOMPLETE, 1387: DEVICEERROR_PHASECONSISTENCY}
    m = hist.to_measurement(r9, f9)
    assert [(t.distance_mm, t.status, t.status_text) for t in m.targets] == [
        (1387, 7, "wrapped target"), (618, 0, "valid")]


def test_a_zero_phase_tolerance_switches_the_check_off(frames):
    _, _, base = frames
    t = _target(total=10000, ambient=2000)
    assert _check(_history([]), [t], _hpp(base, phase_tol=0)) == [
        DEVICEERROR_RANGECOMPLETE_NO_WRAP_CHECK]


def test_a_target_after_an_empty_frame_is_present_but_unconfirmed(frames):
    _, _, base = frames
    t = _target(total=10000, ambient=2000)
    assert _check(_history([]), [t], _hpp(base)) == [DEVICEERROR_PREV_RANGE_NO_TARGETS]
    assert hist.convert_status(DEVICEERROR_PREV_RANGE_NO_TARGETS) == 12


def test_only_ranged_targets_are_checked_and_a_far_one_is_wrapped(frames):
    _, _, base = frames
    prev = [PrevTarget(2000, 10000, 0x2000, DEVICEERROR_RANGECOMPLETE)]
    sigma_fail = _target(total=10000, ambient=2000, status=DEVICEERROR_SIGMATHRESHOLDCHECK)
    # tolerance 8 phase units = 0x800 in 8.8: 0x800 away is already outside
    far = _target(total=10000, ambient=2000, phase=0x2000 + 0x800)
    near = _target(total=10000, ambient=2000, phase=0x2000 + 0x7FF)
    assert _check(_history(prev), [sigma_fail, far, near], _hpp(base)) == [
        DEVICEERROR_SIGMATHRESHOLDCHECK, DEVICEERROR_PHASECONSISTENCY,
        DEVICEERROR_RANGECOMPLETE]


def test_event_consistency_allows_the_shot_noise_of_the_pair_and_no_more(frames):
    # prev: 10000 total, 2000 ambient -> 8000 signal events, same exposure.
    # noise^2 = 10000 + total + 2000/4 + 2000/4; event_sigma 16/64 of
    # isqrt(16 * noise^2) is one sigma: isqrt(16 * 21145) = 581 -> 145.
    _, _, base = frames
    prev = [PrevTarget(2000, 10000, 0x2000, DEVICEERROR_RANGECOMPLETE)]
    hpp = _hpp(base, event_sigma=16)
    inside = _target(total=10145, ambient=2000)      # delta 145 == tolerance
    outside = _target(total=10146, ambient=2000)     # delta 146 > tolerance 145
    assert _check(_history(prev), [inside], hpp) == [DEVICEERROR_RANGECOMPLETE]
    assert _check(_history(prev), [outside], hpp) == [DEVICEERROR_EVENTCONSISTENCY]
    assert hist.convert_status(DEVICEERROR_EVENTCONSISTENCY) == 7


def test_event_consistency_scales_the_previous_exposure_onto_this_one(frames):
    # This frame ran twice the periods: (1 + 199) = 2 * (1 + 99). Twice the
    # events is then the same target, and the same event count is not.
    _, _, base = frames
    prev = [PrevTarget(2000, 10000, 0x2000, DEVICEERROR_RANGECOMPLETE)]
    hpp = _hpp(base, event_sigma=16)
    doubled = _target(total=20000, ambient=4000, periods=199)
    same = _target(total=10000, ambient=2000, periods=199)
    assert _check(_history(prev), [doubled], hpp) == [DEVICEERROR_RANGECOMPLETE]
    assert _check(_history(prev), [same], hpp) == [DEVICEERROR_EVENTCONSISTENCY]


def test_event_consistency_is_waived_at_or_below_the_spad_limit(frames):
    _, _, base = frames
    prev = [PrevTarget(2000, 10000, 0x2000, DEVICEERROR_RANGECOMPLETE)]
    hpp = _hpp(base, event_sigma=16, min_spads=4096)
    t = _target(total=20000, ambient=2000, spads=4096)
    assert _check(_history(prev, spads=4096), [t], hpp) == [DEVICEERROR_RANGECOMPLETE]


def test_a_pulse_wider_than_the_min_max_tolerance_is_a_merged_pulse(frames):
    _, _, base = frames
    prev = [PrevTarget(2000, 10000, 0x2000, DEVICEERROR_RANGECOMPLETE)]
    hpp = _hpp(base, min_max_mm=50)
    wide = _target(total=10000, ambient=2000, min_mm=500, max_mm=551)
    edge = _target(total=10000, ambient=2000, min_mm=500, max_mm=550)
    assert _check(_history(prev), [wide, edge], hpp) == [
        DEVICEERROR_RANGECOMPLETE_MERGED_PULSE, DEVICEERROR_RANGECOMPLETE]
    data = _target(total=10000, ambient=2000, min_mm=500, max_mm=551,
                   status=DEVICEERROR_RANGECOMPLETE_MERGED_PULSE)
    results = RangeResults()
    results.targets = [data]
    m = hist.to_measurement(results, HistogramBinData())
    assert (m.status, m.status_text) == (11, "valid, merged pulse")


# ── one frame ────────────────────────────────────────────────────────────────

def test_a_sigma_over_the_threshold_fails_the_target(frames):
    f2, _, base = frames
    hpp = copy.deepcopy(base)
    hpp.sigma_thresh = 1                      # 1 mm, compared as 1 << 5 in 9.5
    r = hist.process_data(f2, hpp)
    assert r.targets and all(t.sigma > 1 << 5 for t in r.targets)
    assert {t.range_status for t in r.targets} == {DEVICEERROR_SIGMATHRESHOLDCHECK}
    m = hist.to_measurement(r, f2)
    assert (m.status, m.status_text) == (1, "sigma above threshold")


def test_a_phase_beyond_the_valid_window_is_out_of_bounds(frames):
    f2, _, base = frames
    hpp = copy.deepcopy(base)
    hpp.valid_phase_high = 0                  # upper limit = zero-distance phase
    r = hist.process_data(f2, hpp)
    beyond = [t for t in r.targets if t.phase_mean > f2.zero_distance_phase]
    assert beyond
    assert {t.range_status for t in beyond} == {DEVICEERROR_RANGEPHASECHECK}
    assert hist.convert_status(DEVICEERROR_RANGEPHASECHECK) == 4


def test_targets_come_strongest_first_by_default_and_closest_first_on_request(frames):
    _, f9, base = frames
    strongest = hist.process_data(f9, base)
    assert [t.median_range_mm for t in strongest.targets] == [1387, 618]
    assert strongest.targets[0].signal_events > strongest.targets[1].signal_events
    hpp = copy.deepcopy(base)
    hpp.hist_target_order = HIST_TARGET_ORDER_CLOSEST_FIRST
    closest = hist.process_data(f9, hpp)
    assert [t.median_range_mm for t in closest.targets] == [618, 1387]
    assert [t.range_id for t in closest.targets] == [0, 1]


def test_pulses_under_the_signal_limit_leave_a_frame_with_no_target(frames):
    f2, _, base = frames
    hpp = copy.deepcopy(base)
    hpp.signal_total_events_limit = 1 << 30
    r = hist.process_data(f2, hpp)
    assert r.targets == []
    m = hist.to_measurement(r, f2)
    assert (m.distance_mm, m.status, m.status_text) == (8191, 255, "no target")
    assert m.extra["stream_count"] == 1


def test_merged_frames_share_the_count_rate_per_spad(frames):
    f2, _, base = frames
    one = hist.process_data(f2, base, merge_nb=1).targets
    two = hist.process_data(f2, base, merge_nb=2).targets
    # Only the per-SPAD total rate is divided; ranges and peak rates are not.
    assert [t.median_range_mm for t in one] == [t.median_range_mm for t in two]
    for a, b in zip(one, two):
        assert b.peak_signal_count_rate_mcps == a.peak_signal_count_rate_mcps
        assert abs(b.total_rate_per_spad_mcps * 2 - a.total_rate_per_spad_mcps) <= 1


def test_crosstalk_compensation_is_refused_rather_than_ignored(frames):
    f2, _, base = frames
    hpp = copy.deepcopy(base)
    hpp.algo__crosstalk_compensation_enable = 1
    with pytest.raises(NotImplementedError, match="E8.6"):
        hist.process_data(f2, hpp)


def test_range_data_repr_names_range_window_sigma_signal_and_status():
    t = _target(total=10000, ambient=2000, min_mm=498, max_mm=503)
    t.median_range_mm, t.sigma, t.peak_signal_count_rate_mcps = 500, 3 << 7, 128
    assert repr(t) == ("<RangeData 500 mm [498..503] sigma 3 mm signal 1000 kcps "
                       f"status {DEVICEERROR_RANGECOMPLETE_NO_WRAP_CHECK}>")


# ── fixed-point maths ────────────────────────────────────────────────────────

def test_rate_maths_saturates_the_event_count_and_keeps_9_7_scale():
    # 2^29 events over 2 s is 268 Mcps; above 2^24 events the fraction drops
    # to 3 bits and is shifted back, so the 9.7 value stays 268.4 * 128.
    sat = hist.rate_maths(hist.SPAD_TOTAL_COUNT_MAX, 2_000_000)
    assert sat == 34352 and sat >> 7 == 268
    assert hist.rate_maths(1 << 30, 2_000_000) == sat
    assert hist.rate_maths(1 << 30, 1_000_000) == 0xFFFF       # clamped to 16 bit
    assert hist.rate_maths(1000, 1000) == 128                  # 1 Mcps
    assert hist.rate_maths(-5, 1000) == 0


def test_per_spad_maths_without_spads_or_duration_skip_the_division():
    assert hist.rate_per_spad_maths(6, 100, 0, 0xFFFF) == 100 << 6
    assert hist.rate_per_spad_maths(6, 100, 256, 0xFFFF) == 100 << 6
    assert hist.rate_per_spad_maths(6, 0xFFFF, 0, 0xFFFF) == 0xFFFF
    assert hist.events_per_spad_maths(1, 256, 0) == 1000 << 11
    assert hist.events_per_spad_maths(1, 256, 2048) == 1000
    assert hist.events_per_spad_maths(1, 0, 2048) == 0


def test_range_maths_fractional_bits_are_quarter_half_and_whole_mm():
    args = (DIE_OSC, 0x3000, 0x1000)
    quarter = hist.range_maths(*args, 2, 0x800, 0)
    half = hist.range_maths(*args, 1, 0x800, 0)
    whole = hist.range_maths(*args, 0, 0x800, 0)
    assert half == hist.cdiv(quarter, 2)
    assert whole == round(quarter / 4)
    assert whole > 0
    # A phase short of zero distance is a negative range; the +0x400 rounding
    # before C's truncating divide makes it one mm short of the mirror image.
    assert hist.range_maths(DIE_OSC, 0x1000, 0x3000, 0, 0x800, 0) == -whole + 1


def test_calc_rates_refuses_a_frame_without_a_clock_or_periods():
    t = RangeData()
    assert hist._calc_rates(t, 10, 0, 99, 4096, 1) is False
    assert hist._calc_rates(t, 10, DIE_OSC, 0, 4096, 1) is False
    assert t.peak_signal_count_rate_mcps == 0


def test_sigma_is_invalid_without_a_clock_or_a_pulse_above_ambient():
    assert hist._sigma_estimate(40, 10, 500, 10, 0, 0, 0, 0, 0, 100, 0) == SIGMA_INVALID
    assert hist._sigma_estimate(40, 10, 100, 10, 0, 0, 0, 0, 0, 100, DIE_OSC) == SIGMA_INVALID
    algo = hist._Gen3Algo()
    assert hist._pulse_sigma(hist.PulseData(), 40, algo, 0) == SIGMA_INVALID


def test_sigma_grows_with_the_imbalance_and_adds_the_reference_in_quadrature():
    def sigma(ref_mm, c):
        return hist._sigma_estimate(ref_mm, 100, 1000, c, 0, 0, 0, 0, 0, 10, DIE_OSC)

    assert sigma(0, 100) == 0                       # balanced window, no width
    assert 0 < sigma(0, 600) < sigma(0, 1000) < sigma(0, 3000)
    for c in (100, 600, 3000):                      # sqrt(own^2 + ref^2), 9.7 mm
        own, ref = sigma(0, c), 40 << 7
        assert abs(sigma(40, c) - hist.isqrt(own * own + ref * ref)) <= 1
    # Past the 56-bit ceiling on the squared imbalance a larger window sum
    # changes nothing: the divide-first branches keep the 64-bit
    # intermediate from wrapping into a small, plausible-looking sigma.
    def huge(a):
        return hist._sigma_estimate(40, a, 1 << 20, 0, 1 << 22, 1 << 22, 0, 0, 0, 0,
                                    DIE_OSC)

    assert huge(1 << 40) == huge(1 << 50)
    assert 40 << 7 <= huge(1 << 50) < SIGMA_INVALID


def test_a_zero_crosstalk_histogram_leaves_the_sigma_unchanged(frames):
    f2, _, base = frames
    algo = hist._Gen3Algo()
    algo.vcsel_period = 12
    pulse = hist.PulseData()
    pulse.peak_bin, pulse.filter_woi = 5, 1
    algo.pulse_amb = f2
    algo.pulse_zero = f2
    algo.pulse_xtalk = HistogramBinData()          # all zeros
    without = hist._pulse_sigma(pulse, 40, algo, 0)
    with_zero_xtalk = hist._pulse_sigma(pulse, 40, algo, 1)
    assert without == with_zero_xtalk != SIGMA_INVALID


def test_phase_helpers_return_no_phase_for_an_empty_window():
    bins = HistogramBinData()
    bins.ambient_per_bin = 50
    bins.bin_data = [50] * len(bins.bin_data)            # ambient only, no pulse
    assert hist._weighted_phase(0, 3, 12, 1, bins) == MAX_ALLOWED_PHASE
    assert hist._weighted_phase(0, 3, 0, 1, bins) == MAX_ALLOWED_PHASE
    bins.bin_data[2] = 150
    assert hist._weighted_phase(0, 3, 12, 1, bins) == 1024 + 2048 * 2   # bin 2's centre
    assert hist._interpolate_phase(4, 10, 50, 10, 50, 12) is None
