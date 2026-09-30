"""VL53L5CX / VL53L7CX / VL53L7CH hardware contract (contract 11).

Runs against whichever of the three I2C-board sensors is attached. As on the
L8 suite, what the CH adds (CNH) is asserted to exist on the CH and to be
absent on the CX classes, and distances are not judged against a target —
only frame shape, decode sanity, the board commands, and restartability.
"""

from __future__ import annotations

import os

import numpy as np
import pytest

from _support import FAMILY_VL53L5CX, FAMILY_VL53L7CH, FAMILY_VL53L7CX, VL53L7_FAMILIES, port_lease

from depz_sensor_sdk import CnhConfig, DepzError, open_device
from depz_sensor_sdk.vl53l8.uld import (
    DIST_MM,
    POWER_MODE_DEEP_SLEEP,
    POWER_MODE_SLEEP,
    POWER_MODE_WAKEUP,
    THRESH_IN_WINDOW,
    THRESH_OP_OR,
    Vl53l8cxError,
)
from depz_sensor_sdk.vl53l7 import (
    MODULE_TYPE_MZ,
    MODULE_TYPE_MZEVO,
    RESOLUTION_4X4,
    RESOLUTION_8X8,
    PinAction,
    Vl53l5cx,
    Vl53l7ch,
    Vl53l7cx,
)

pytestmark = [pytest.mark.hardware, pytest.mark.hardware_vl53l7, pytest.mark.timeout(180)]

_CLASS = {FAMILY_VL53L5CX: Vl53l5cx, FAMILY_VL53L7CX: Vl53l7cx, FAMILY_VL53L7CH: Vl53l7ch}
_MODULE = {FAMILY_VL53L5CX: MODULE_TYPE_MZ, FAMILY_VL53L7CX: MODULE_TYPE_MZEVO, FAMILY_VL53L7CH: MODULE_TYPE_MZEVO}
_VALID = (5, 9)


@pytest.fixture(params=VL53L7_FAMILIES)
def l7_info(request, hw_inventory):
    matches = [d for d in hw_inventory if d.family == request.param]
    if not matches:
        pytest.skip(f"no {request.param} device attached")
    return matches[0]


@pytest.fixture
def tof(l7_info):
    """An initialised L5/L7 device of each family present on the bench."""
    with port_lease(l7_info.resolve_port()):
        dev = _CLASS[l7_info.family](l7_info.resolve_port())
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


def _frames(dev, n, *, resolution=RESOLUTION_8X8, hz=15, timeout=3.0):
    dev.set_resolution(resolution)
    dev.set_ranging_frequency_hz(hz)
    dev.start_ranging()
    try:
        return [dev.get_frame(timeout=timeout) for _ in range(n)]
    finally:
        dev.stop_ranging()


# ── identity ─────────────────────────────────────────────────────────────────


def test_open_device_resolves_the_family_class(l7_info):
    """APP_VL53L7 cannot tell the three sensors apart; the PID must."""
    with port_lease(l7_info.resolve_port()):
        dev = open_device(l7_info.resolve_port())
        try:
            assert type(dev) is _CLASS[l7_info.family]
            assert dev.get_software_name().startswith("APP_VL53L7_")
        finally:
            dev.close()


def test_module_type_matches_the_family(tof, l7_info):
    """module_type is read from the sensor FW: the silicon's own L5/L7 answer."""
    assert tof.module_type == _MODULE[l7_info.family]


def test_cnh_is_ch_only(tof):
    assert hasattr(tof, "configure_cnh") == isinstance(tof, Vl53l7ch)


# ── frames ───────────────────────────────────────────────────────────────────


@pytest.mark.parametrize("resolution", [RESOLUTION_4X4, RESOLUTION_8X8])
def test_frame_shape_and_decode(tof, resolution):
    frames = _frames(tof, 5, resolution=resolution)
    for f in frames:
        assert f.resolution == resolution
        for field in ("distance_mm", "target_status", "nb_target_detected", "range_sigma_mm"):
            assert getattr(f, field).shape == (resolution,), field
        assert f.grid().shape == ((4, 4) if resolution == RESOLUTION_4X4 else (8, 8))
        assert f.cnh_raw is None
    assert np.isin(frames[-1].target_status, _VALID).any(), "no valid zone at all"
    assert tof.frame_parse_errors == 0
    assert tof.uld.frame_size_mismatch is None


def test_one_hz_streams(tof):
    """L5/L7 range at 1 Hz (the L8 does not enter its loop below 2 Hz)."""
    frames = _frames(tof, 2, hz=1, timeout=3.0)
    dt_ms = (frames[1].timestamp_us - frames[0].timestamp_us) / 1000
    assert 800 < dt_ms < 1300, dt_ms


def test_restart_after_reconfigure(tof):
    _frames(tof, 2, resolution=RESOLUTION_4X4, hz=30)
    assert _frames(tof, 2, resolution=RESOLUTION_8X8, hz=10)[-1].resolution == RESOLUTION_8X8


def test_ch_streams_cnh_histograms(tof):
    if not isinstance(tof, Vl53l7ch):
        pytest.skip("CNH is a VL53L7CH capability")
    cfg = CnhConfig()
    cfg.init_config(start_bin=10, num_bins=20, sub_sample=2)
    cfg.create_agg_map(RESOLUTION_8X8, 0, 0, 2, 2, 4, 4)
    tof.set_resolution(RESOLUTION_8X8)
    tof.configure_cnh(cfg)
    tof.start_ranging()
    try:
        frame = tof.get_frame(timeout=8.0)
    finally:
        tof.stop_ranging()
    assert frame.cnh_raw is not None and len(frame.cnh_raw) == cfg.required_memory()
    # > 1536 B: the frame crossed the chunked-stream path intact.
    assert tof.uld.data_read_size > 1536
    assert tof.reassembler_discards == 0


# ── board commands ───────────────────────────────────────────────────────────


def test_bridge_counters_stay_clean_over_a_run(tof):
    before = tof.get_bridge_info()
    _frames(tof, 10)
    after = tof.get_bridge_info()
    assert after.int_edges > before.int_edges
    assert after.i2c_errors == before.i2c_errors
    assert after.frames_dropped == before.frames_dropped
    assert not after.streaming and after.lpn_level == 1


def test_i2c_speed_snaps_and_ranging_survives(tof):
    try:
        assert tof.set_i2c_speed_khz(450) in (400, 500)
        assert _frames(tof, 3)[-1].resolution == RESOLUTION_8X8
    finally:
        assert tof.set_i2c_speed_khz(1000) == 1000


def test_soft_cycle_requires_reinit(tof):
    tof.pin_ctrl(PinAction.SOFT_CYCLE)
    with pytest.raises(DepzError):
        tof.start_ranging()  # the sensor lost its firmware: init() first
    tof.init()
    assert _frames(tof, 2)[-1].resolution == RESOLUTION_8X8


# ── ULD plugins (contract 11 §4) ─────────────────────────────────────────────


def test_sleep_and_wake_keep_the_configuration(tof):
    tof.set_resolution(RESOLUTION_4X4)
    tof.set_power_mode(POWER_MODE_SLEEP)
    assert tof.get_power_mode() == POWER_MODE_SLEEP
    tof.set_power_mode(POWER_MODE_WAKEUP)
    assert tof.get_power_mode() == POWER_MODE_WAKEUP
    # Sleep retains the configuration: no init() needed to range again.
    assert _frames(tof, 2, resolution=RESOLUTION_4X4)[-1].resolution == RESOLUTION_4X4


def test_deep_sleep_is_ch_only(tof):
    if isinstance(tof, Vl53l7ch):
        tof.set_power_mode(POWER_MODE_DEEP_SLEEP)
        assert tof.get_power_mode() == POWER_MODE_DEEP_SLEEP
        tof.set_power_mode(POWER_MODE_WAKEUP)  # re-runs init(): the FW was lost
        assert _frames(tof, 2)[-1].resolution == RESOLUTION_8X8
    else:
        with pytest.raises(Vl53l8cxError, match="no deep sleep"):
            tof.set_power_mode(POWER_MODE_DEEP_SLEEP)


def test_detection_thresholds_round_trip_and_gate_the_stream(tof):
    # One rule per zone of an 8x8 grid: "distance below 3 m" — a wall at
    # ~0.5 m matches everywhere, so the gated stream must still deliver.
    rules = [
        {"low_thresh": 0, "high_thresh": 3000, "measurement": DIST_MM,
         "type": THRESH_IN_WINDOW, "zone_num": z, "operation": THRESH_OP_OR}
        for z in range(64)
    ]
    rules[-1]["zone_num"] = 63 | 0x80  # LAST_THRESHOLD marks the end of the list
    tof.set_resolution(RESOLUTION_8X8)
    tof.set_detection_thresholds(rules)
    tof.set_detection_thresholds_enable(True)
    try:
        assert tof.get_detection_thresholds_enable() == 1
        back = tof.get_detection_thresholds()
        assert back[0]["high_thresh"] == 3000 and back[0]["measurement"] == DIST_MM
        assert _frames(tof, 2, hz=10)[-1].resolution == RESOLUTION_8X8
    finally:
        tof.set_detection_thresholds_enable(False)
    assert tof.get_detection_thresholds_enable() == 0


def test_threshold_auto_stop_is_ch_only(tof):
    if isinstance(tof, Vl53l7ch):
        tof.set_detection_thresholds_auto_stop(False)
    else:
        with pytest.raises(Vl53l8cxError, match="auto-stop"):
            tof.set_detection_thresholds_auto_stop(True)


def test_motion_indicator_rides_the_frames(tof):
    tof.set_resolution(RESOLUTION_8X8)
    tof.configure_motion_indicator(400, 1500)
    frames = _frames(tof, 4, hz=10)
    motion = frames[-1].motion
    assert motion is not None, "motion indicator configured but frames carry none"
    assert motion["nb_of_aggregates"] == 16
    assert len(motion["motion"]) == 32


def test_xtalk_caldata_round_trip(tof):
    blob = tof.get_caldata_xtalk()
    assert len(blob) == 776
    assert blob[-8:] == bytes([0x00, 0x00, 0x00, 0x0F, 0x00, 0x01, 0x03, 0x04])
    tof.set_caldata_xtalk(blob)
    assert tof.get_caldata_xtalk() == blob
    assert _frames(tof, 2)[-1].resolution == RESOLUTION_8X8


def test_xtalk_margin_round_trip(tof):
    tof.set_xtalk_margin(80.0)
    assert tof.get_xtalk_margin() == pytest.approx(80.0, abs=1 / 2048)


@pytest.mark.skipif(
    "DEPZ_XTALK_TARGET_MM" not in os.environ,
    reason="set DEPZ_XTALK_TARGET_MM (600..3000) to the measured distance to a flat target "
    "(and DEPZ_XTALK_REFLECTANCE, % — default 3 as in ST's example)",
)
def test_xtalk_calibration_runs_and_ranging_survives(tof):
    """Runs the ST calibration against a flat target at the measured distance.

    Without a cover glass there is little to calibrate; the run must still
    complete (possibly reporting XTALK_FAILED = "coverglass too good"), leave a
    well-formed buffer and a sensor that ranges the target correctly."""
    target_mm = int(os.environ["DEPZ_XTALK_TARGET_MM"])
    # ST's example uses a 3 % (black) target; a white wall / paper is ~80 %.
    reflectance = int(os.environ.get("DEPZ_XTALK_REFLECTANCE", "3"))
    tof.set_resolution(RESOLUTION_4X4)
    tof.set_ranging_frequency_hz(10)
    tof.calibrate_xtalk(reflectance_percent=reflectance, nb_samples=4, distance_mm=target_mm)
    blob = tof.get_caldata_xtalk()
    assert len(blob) == 776
    # Configuration restored around the run.
    assert tof.get_resolution() == RESOLUTION_4X4
    assert tof.get_ranging_frequency_hz() == 10
    f = _frames(tof, 5, resolution=RESOLUTION_4X4, hz=10)[-1]
    centre = f.grid()[1:3, 1:3]
    assert abs(float(np.median(centre)) - target_mm) < 0.1 * target_mm, centre
