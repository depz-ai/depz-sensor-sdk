"""VL53L8CX / VL53L8CH hardware contract (contract 04).

Runs against both silicon variants. CX and CH are deliberately *not* assumed to
share a capability set: everything the CH adds (CNH histograms) is asserted to
exist on the CH and to be absent on the CX.

Protocol correctness only — that a zone reports 300 mm because a wall is 300 mm
away is not asserted anywhere here; that needs a calibrated rig (QA_MATRIX.md
§ Limitations). What is asserted is frame shape, decode sanity, configuration
round-trips and stream restartability.
"""

from __future__ import annotations

import time

import numpy as np
import pytest

from _support import FAMILY_VL53L8CH, FAMILY_VL53L8CX, port_lease

from depz_sensor_sdk import CnhConfig, DepzError, Vl53l8Ch, Vl53l8Cx
from depz_sensor_sdk.vl53l8 import MIN_RANGING_FREQUENCY_HZ
from depz_sensor_sdk.vl53l8.uld import (
    RANGING_MODE_AUTONOMOUS,
    RANGING_MODE_CONTINUOUS,
    RESOLUTION_4X4,
    RESOLUTION_8X8,
    TARGET_ORDER_CLOSEST,
    TARGET_ORDER_STRONGEST,
)

pytestmark = [pytest.mark.hardware, pytest.mark.hardware_vl53l8, pytest.mark.timeout(180)]


@pytest.fixture(params=[FAMILY_VL53L8CX, FAMILY_VL53L8CH])
def tof(request, hw_inventory):
    """An initialised ToF device of each variant present on the bench."""
    matches = [d for d in hw_inventory if d.family == request.param]
    if not matches:
        pytest.skip(f"no {request.param} device attached")
    info = matches[0]
    cls = Vl53l8Ch if info.family == FAMILY_VL53L8CH else Vl53l8Cx
    with port_lease(info.resolve_port()):
        dev = cls(info.resolve_port())
        try:
            dev.init()
            yield dev
        finally:
            try:
                if dev.ranging:
                    dev.stop_ranging()
            except Exception:
                pass
            dev.close()


# ── identity ─────────────────────────────────────────────────────────────────


def test_variant_matches_the_class(tof):
    expected = "ch" if isinstance(tof, Vl53l8Ch) else "cx"
    assert tof.variant == expected, f"loaded the {tof.variant!r} blob into a {expected!r} class"


def test_is_alive(tof):
    assert tof.is_alive()


def test_init_is_required_before_configuration(hw_inventory):
    """Config before init() must raise, not silently talk to a dead register bank."""
    matches = [d for d in hw_inventory if d.family == FAMILY_VL53L8CX]
    if not matches:
        pytest.skip("no VL53L8CX attached")
    info = matches[0]
    with port_lease(info.resolve_port()):
        dev = Vl53l8Cx(info.resolve_port())
        try:
            with pytest.raises(DepzError):
                dev.get_resolution()
        finally:
            dev.close()


def test_init_rejects_a_mismatched_variant(tof):
    wrong = "ch" if tof.variant == "cx" else "cx"
    with pytest.raises(DepzError):
        tof.init(variant=wrong)


# ── resolution ───────────────────────────────────────────────────────────────


@pytest.mark.parametrize("zones", [RESOLUTION_4X4, RESOLUTION_8X8])
def test_resolution_round_trips_and_shapes_the_frame(tof, zones):
    tof.set_resolution(zones)
    assert tof.get_resolution() == zones

    tof.start_ranging()
    try:
        frame = tof.get_frame(timeout=5.0)
        assert frame.resolution == zones
        for name in ("distance_mm", "target_status", "nb_target_detected"):
            arr = getattr(frame, name)
            assert arr.shape == (zones,), f"{name} has shape {arr.shape}, expected ({zones},)"
        grid = frame.grid("distance_mm")
        side = 4 if zones == RESOLUTION_4X4 else 8
        assert grid.shape == (side, side), f"grid() gave {grid.shape}, expected ({side},{side})"
    finally:
        tof.stop_ranging()


def test_invalid_resolution_rejected(tof):
    for bad in (0, 15, 32, 65, 100):
        with pytest.raises(ValueError):
            tof.set_resolution(bad)


def test_resolution_cannot_change_while_ranging(tof):
    """The stream owns the register bank (contract 04)."""
    tof.set_resolution(RESOLUTION_4X4)
    tof.start_ranging()
    try:
        with pytest.raises(DepzError):
            tof.set_resolution(RESOLUTION_8X8)
    finally:
        tof.stop_ranging()
    # ...and it is configurable again once stopped.
    tof.set_resolution(RESOLUTION_8X8)
    assert tof.get_resolution() == RESOLUTION_8X8


# ── frequency ────────────────────────────────────────────────────────────────


@pytest.mark.parametrize(
    "zones,hz",
    [
        (RESOLUTION_4X4, 2),  # documented minimum
        (RESOLUTION_4X4, 30),
        (RESOLUTION_4X4, 60),  # documented max at 4x4
        (RESOLUTION_8X8, 5),
        (RESOLUTION_8X8, 15),  # documented max at 8x8
    ],
)
def test_supported_frequencies_stream(tof, zones, hz):
    tof.set_resolution(zones)
    tof.set_ranging_frequency_hz(hz)
    assert tof.get_ranging_frequency_hz() == hz
    tof.start_ranging()
    try:
        frame = tof.get_frame(timeout=6.0)
        assert frame.distance_mm.shape == (zones,)
    finally:
        tof.stop_ranging()


def test_frequency_below_minimum_is_rejected(tof):
    """Below 2 Hz the sensor never enters its ranging loop and streams nothing,
    so the SDK must refuse rather than hand back a silent dead stream."""
    for bad in (0, 1, -5):
        with pytest.raises(ValueError):
            tof.set_ranging_frequency_hz(bad)
    assert MIN_RANGING_FREQUENCY_HZ == 2


def test_effective_rate_tracks_the_configured_frequency(tof):
    """Ordering assertion only — absolute rate depends on integration time."""
    tof.set_resolution(RESOLUTION_4X4)

    def rate_at(hz: int) -> float:
        tof.set_ranging_frequency_hz(hz)
        tof.start_ranging()
        try:
            tof.get_frame(timeout=5.0)  # discard the first, it carries startup latency
            t0 = time.monotonic()
            n = 8
            for _ in range(n):
                tof.get_frame(timeout=5.0)
            return n / (time.monotonic() - t0)
        finally:
            tof.stop_ranging()

    slow = rate_at(5)
    fast = rate_at(30)
    assert fast > slow * 1.5, f"30Hz gave {fast:.1f} fps, 5Hz gave {slow:.1f} fps"


# ── other configuration ──────────────────────────────────────────────────────


@pytest.mark.parametrize("mode", [RANGING_MODE_CONTINUOUS, RANGING_MODE_AUTONOMOUS])
def test_ranging_mode_round_trips_and_streams(tof, mode):
    tof.set_ranging_mode(mode)
    assert tof.get_ranging_mode() == mode
    tof.start_ranging()
    try:
        assert tof.get_frame(timeout=6.0) is not None
    finally:
        tof.stop_ranging()
    tof.set_ranging_mode(RANGING_MODE_CONTINUOUS)


@pytest.mark.parametrize("order", [TARGET_ORDER_CLOSEST, TARGET_ORDER_STRONGEST])
def test_target_order_round_trips(tof, order):
    tof.set_target_order(order)
    assert tof.get_target_order() == order


@pytest.mark.parametrize("ms", [2, 20, 100])
def test_integration_time_round_trips(tof, ms):
    """Autonomous mode is the one that honours integration time (contract 04)."""
    tof.set_ranging_mode(RANGING_MODE_AUTONOMOUS)
    try:
        tof.set_integration_time_ms(ms)
        assert tof.get_integration_time_ms() == ms
    finally:
        tof.set_ranging_mode(RANGING_MODE_CONTINUOUS)


@pytest.mark.parametrize("pct", [0, 25, 50, 99])
def test_sharpener_round_trips(tof, pct):
    tof.set_sharpener_percent(pct)
    assert tof.get_sharpener_percent() == pct
    tof.set_sharpener_percent(0)


def test_config_survives_a_stream(tof):
    tof.set_resolution(RESOLUTION_8X8)
    tof.set_ranging_frequency_hz(10)
    tof.set_sharpener_percent(20)
    tof.start_ranging()
    try:
        tof.get_frame(timeout=6.0)
    finally:
        tof.stop_ranging()
    assert tof.get_resolution() == RESOLUTION_8X8
    assert tof.get_ranging_frequency_hz() == 10
    assert tof.get_sharpener_percent() == 20


# ── frame content ────────────────────────────────────────────────────────────


def test_frame_decode_is_structurally_sane(tof):
    """Decoded fields must be finite and in their documented domains."""
    tof.set_resolution(RESOLUTION_8X8)
    tof.set_ranging_frequency_hz(10)
    tof.start_ranging()
    try:
        frames = [tof.get_frame(timeout=6.0) for _ in range(5)]
    finally:
        tof.stop_ranging()

    for f in frames:
        assert f.distance_mm.dtype == np.int32
        assert np.all(np.isfinite(f.signal_per_spad)), "non-finite signal_per_spad"
        assert np.all(np.isfinite(f.ambient_per_spad)), "non-finite ambient_per_spad"
        assert np.all(np.isfinite(f.range_sigma_mm)), "non-finite range_sigma_mm"
        assert np.all(f.target_status <= 255)
        assert np.all(f.nb_target_detected <= 4), "more targets than the ULD can report"
        # 5 = range valid. Zones with no target legitimately carry other codes;
        # we assert the *domain*, not that anything is in front of the sensor.
        assert np.all((f.distance_mm >= -1000) & (f.distance_mm < 10_000)), (
            f"distance out of any plausible domain: {f.distance_mm.min()}..{f.distance_mm.max()}"
        )


def test_valid_zones_report_plausible_distances(tof):
    """Where the sensor says 'range valid' (status 5), the value must be sane.

    Status-gated: invalid zones are allowed to carry anything.
    """
    tof.set_resolution(RESOLUTION_8X8)
    tof.start_ranging()
    try:
        frames = [tof.get_frame(timeout=6.0) for _ in range(5)]
    finally:
        tof.stop_ranging()

    saw_valid = False
    for f in frames:
        valid = f.target_status == 5
        if not valid.any():
            continue
        saw_valid = True
        d = f.distance_mm[valid]
        assert np.all(d >= 0), f"valid zone with a negative distance: {d.min()}"
        assert np.all(d <= 4000), f"valid zone beyond the sensor's 4m range: {d.max()}"
    if not saw_valid:
        pytest.skip("no zone reported status=5 — nothing in range of the bench sensor")


def test_frame_timestamps_advance(tof):
    tof.set_ranging_frequency_hz(10)
    tof.start_ranging()
    try:
        ts = [tof.get_frame(timeout=6.0).timestamp_us for _ in range(6)]
    finally:
        tof.stop_ranging()
    assert ts == sorted(ts), f"frame timestamps went backwards: {ts}"
    assert len(set(ts)) == len(ts), "duplicate frame timestamps"


def test_no_frame_parse_errors_in_a_clean_stream(tof):
    tof.set_resolution(RESOLUTION_8X8)
    tof.set_ranging_frequency_hz(15)
    tof.start_ranging()
    try:
        for _ in range(20):
            tof.get_frame(timeout=6.0)
    finally:
        tof.stop_ranging()
    assert tof.frame_parse_errors == 0, f"{tof.frame_parse_errors} frames failed to parse"
    assert tof.reassembler_discards == 0, f"{tof.reassembler_discards} chunked frames discarded"


# ── stream lifecycle ─────────────────────────────────────────────────────────


def test_stream_restart(tof):
    for _ in range(3):
        tof.start_ranging()
        assert tof.get_frame(timeout=6.0) is not None
        tof.stop_ranging()
        assert not tof.ranging


def test_double_start_ranging_is_rejected(tof):
    tof.start_ranging()
    try:
        with pytest.raises(DepzError):
            tof.start_ranging()
    finally:
        tof.stop_ranging()


def test_stop_ranging_without_start_is_a_noop(tof):
    tof.stop_ranging()
    assert not tof.ranging


def test_mode_switch_between_streams(tof):
    """Resolution switching across restarts must reshape the frame each time."""
    for zones in (RESOLUTION_4X4, RESOLUTION_8X8, RESOLUTION_4X4):
        tof.set_resolution(zones)
        tof.start_ranging()
        try:
            f = tof.get_frame(timeout=6.0)
            assert f.resolution == zones
            assert f.distance_mm.shape == (zones,)
        finally:
            tof.stop_ranging()


def test_no_stale_frames_after_restart(tof):
    tof.set_ranging_frequency_hz(10)
    tof.start_ranging()
    first = [tof.get_frame(timeout=6.0) for _ in range(3)]
    tof.stop_ranging()

    time.sleep(0.3)

    tof.start_ranging()
    second = tof.get_frame(timeout=6.0)
    tof.stop_ranging()

    assert second.timestamp_us > first[-1].timestamp_us, (
        "restarted stream replayed a frame from the previous session"
    )


# ── CH-only: CNH ─────────────────────────────────────────────────────────────


def test_cnh_is_ch_only(tof):
    """The CX must not expose CNH — the variants are not interchangeable."""
    if isinstance(tof, Vl53l8Ch):
        assert hasattr(tof, "configure_cnh")
    else:
        assert not hasattr(tof, "configure_cnh"), (
            "Vl53l8Cx exposes configure_cnh — CH capability leaked onto the CX"
        )


def test_ch_streams_cnh_histograms(tof):
    """CNH is the reason to run CH firmware: frames must carry cnh_raw."""
    if not isinstance(tof, Vl53l8Ch):
        pytest.skip("CNH is a VL53L8CH capability")

    # 8x8 zones merged 2x2 into a 4x4 aggregate grid, 20 histogram bins —
    # the same shape the vl53l8_cnh golden vector pins.
    cfg = CnhConfig()
    cfg.init_config(start_bin=10, num_bins=20, sub_sample=2)
    cfg.create_agg_map(RESOLUTION_8X8, 0, 0, 2, 2, 4, 4)
    assert cfg.required_memory() > 0

    tof.set_resolution(RESOLUTION_8X8)
    tof.configure_cnh(cfg)
    tof.start_ranging()
    try:
        frame = tof.get_frame(timeout=8.0)
        assert frame.cnh_raw is not None, "CH configured for CNH but the frame carries none"
        assert len(frame.cnh_raw) > 0
    finally:
        tof.stop_ranging()


def test_cnh_config_rejects_a_map_beyond_the_zone_grid(tof):
    """The aggregate map must fit the configured resolution."""
    if not isinstance(tof, Vl53l8Ch):
        pytest.skip("CNH is a VL53L8CH capability")
    from depz_sensor_sdk.vl53l8.cnh import CnhConfigError

    cfg = CnhConfig()
    cfg.init_config(start_bin=10, num_bins=20, sub_sample=2)
    with pytest.raises(CnhConfigError):
        # 4x4 grid cannot hold 4 aggregates merged 2x2 starting at (2,2).
        cfg.create_agg_map(RESOLUTION_4X4, 2, 2, 2, 2, 4, 4)


def test_cx_frames_carry_no_cnh(tof):
    if isinstance(tof, Vl53l8Ch):
        pytest.skip("CX-only assertion")
    tof.start_ranging()
    try:
        assert tof.get_frame(timeout=6.0).cnh_raw is None
    finally:
        tof.stop_ranging()
