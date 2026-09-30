"""VL53L 1D family hardware contract (contract 12): VL53L0X, VL53L1CX,
VL53L1CB, VL53L3CX, VL53L4CX on the APP_VL53L0_4 bridge.

Runs against whichever of them is attached, once per driver kind the product
has. Capabilities are asserted from the product table, not assumed: a group the
driver does not serve must refuse with a clear error. Distances are not judged
against a target (that needs a rig) — only decode sanity, timing, the bridge
counters, configuration round-trips and restartability. Calibrations run only
when DEPZ_CALIB_TARGET_MM names the measured distance to a flat target.
"""

from __future__ import annotations

import os

import pytest

from _support import VL53LX_FAMILIES, port_lease

from depz_sensor_sdk import DepzError, open_device
from depz_sensor_sdk.vl53lx import (
    CLASS_BY_PRODUCT,
    PLOTTABLE_STATUSES,
    XSHUT_RESET,
    Vl53lx,
    plot_distances,
)
from depz_sensor_sdk.vl53lx.uld import registry

pytestmark = [pytest.mark.hardware, pytest.mark.hardware_vl53lx, pytest.mark.timeout(240)]

_GROUP_CALLS = {
    # group: (getter name, setter name, value to round-trip)
    "offset": ("get_offset_mm", "set_offset_mm", 12),
    "xtalk": ("get_xtalk_kcps", "set_xtalk_kcps", 8),
    "sigma_thresh": ("get_sigma_threshold_mm", "set_sigma_threshold_mm", 20),
    "signal_thresh": ("get_signal_threshold_kcps", "set_signal_threshold_kcps", 1200),
}


def _pairs():
    out = []
    for family in VL53LX_FAMILIES:
        for kind in registry.driver_kinds(family.upper()):
            out.append(pytest.param((family, kind), id=f"{family}-{kind}"))
    return out


@pytest.fixture(params=_pairs())
def tof(request, hw_inventory):
    """An initialised device of each (product, driver kind) on the bench."""
    family, kind = request.param
    matches = [d for d in hw_inventory if d.family == family]
    if not matches:
        pytest.skip(f"no {family} device attached")
    info = matches[0]
    with port_lease(info.resolve_port()):
        dev = open_device(info.resolve_port())
        try:
            dev.init(kind)
            yield dev
        finally:
            try:
                if dev.ranging:
                    dev.stop_ranging()
            except Exception:
                pass
            dev.close()


def _stream(dev: Vl53lx, n: int, budget_ms: int = 33, mode=None):
    dev.configure(budget_ms=dev.snap_budget(budget_ms), mode=mode)
    dev.start_ranging()
    try:
        return [dev.get_measurement(timeout=3.0) for _ in range(n)]
    finally:
        dev.stop_ranging()


# ── identity ─────────────────────────────────────────────────────────────────


def test_open_device_resolves_the_product(tof):
    assert type(tof) is CLASS_BY_PRODUCT[tof.product]
    assert tof.detected == tof.product
    assert tof.get_software_name().startswith("APP_VL53L0_4_")
    idf = tof.identify()
    assert idf["model_id_ok"], hex(idf["model_id"])
    assert idf["kinds"] == registry.driver_kinds(tof.product)


def test_bridge_carries_the_product_parameters(tof):
    info = tof.bridge_info()
    assert info.addr_width == tof.driver.ADDR_WIDTH
    assert info.i2c_khz == tof.driver.MAX_KHZ  # init leaves the bus at the ceiling


# ── frames ───────────────────────────────────────────────────────────────────


def test_stream_decodes_and_paces(tof):
    budget = tof.snap_budget(33)
    # Counters are taken around the stream only: configure() soft-resets the
    # sensor, and a VL53L0X legitimately NACKs a request or two after that.
    tof.configure(budget_ms=budget)
    before = tof.bridge_info()
    tof.start_ranging()
    try:
        ms = [tof.get_measurement(timeout=3.0) for _ in range(12)]
    finally:
        tof.stop_ranging()
    after = tof.bridge_info()
    assert tof.stream_parse_errors == 0
    assert any(m.plottable for m in ms), [(m.distance_mm, m.status) for m in ms]
    assert all(isinstance(m.distance_mm, int) for m in ms)
    if tof.driver.HISTOGRAM:
        assert all(m.bins is not None and m.targets for m in ms[1:])
    else:
        assert all(m.bins is None and not m.targets for m in ms)
    dts = [(b.timestamp_us - a.timestamp_us) / 1000 for a, b in zip(ms[1:], ms[2:])]
    assert all(0.5 * budget < dt < 3 * budget + 20 for dt in dts), dts
    assert after.i2c_errors == before.i2c_errors
    # One slot may lose the bus to STOP_STREAM / stop_ranging itself.
    assert after.slots_skipped <= 1 and after.frames_dropped == 0, after


def test_measure_once_polls(tof):
    tof.configure(budget_ms=tof.snap_budget(33))
    m = tof.measure_once(timeout=2.0)
    assert isinstance(m.distance_mm, int) and m.timestamp_us > 0


def test_every_mode_ranges(tof):
    if not tof.supports("mode"):
        assert tof.get_mode() is None
        pytest.skip(f"{tof.product}/{tof.driver_kind} has no ranging modes")
    for mode in tof.modes:
        # 10 frames: the histogram driver needs a predecessor before it calls a
        # frame valid (12 on the first one). "Ranges" = a positive distance,
        # not a plottable status: on the short preset every other frame is a
        # negative-phase artefact, so the real target is never confirmed by
        # its predecessor and comes back with status 7 on every frame
        # (measured on L4CX at 600 mm; the firmware repo's tool does the same).
        ms = _stream(tof, 10, mode=mode)
        assert tof.get_mode() == mode
        ranged = [m for m in ms if m.distance_mm > 0 or any(t.distance_mm > 0 for t in m.targets)]
        assert ranged, (mode, [(m.distance_mm, m.status) for m in ms])
        if not tof.driver.HISTOGRAM or mode != "short":
            assert any(plot_distances(m) for m in ms), (mode, [(m.distance_mm, m.status) for m in ms])


def test_budget_is_read_back(tof):
    budget = tof.snap_budget(50)
    tof.configure(budget_ms=budget)
    got, inter = tof.get_range_timing()
    # VL53L1 has no free-running mode: a zero period is written as the budget.
    assert abs(got - budget) <= 2 and (inter == 0 or abs(inter - budget) <= 2), (got, inter, budget)
    if tof.budget_choices():
        with pytest.raises(Exception):  # the driver refuses rather than rounds
            tof.configure(budget_ms=max(tof.budget_choices()) - 1)


# ── capability groups ───────────────────────────────────────────────────────


@pytest.mark.parametrize("group", sorted(_GROUP_CALLS))
def test_setting_round_trips_or_refuses(tof, group):
    getter, setter, value = _GROUP_CALLS[group]
    tof.configure(budget_ms=tof.snap_budget(33))
    if not tof.supports(group):
        with pytest.raises(DepzError, match="does not support"):
            getattr(tof, setter)(value)
        return
    getattr(tof, setter)(value)
    got = getattr(tof, getter)()
    assert abs(got - value) <= max(1, value // 50), (group, got, value)


def test_roi_round_trips_or_refuses(tof):
    tof.configure(budget_ms=tof.snap_budget(33))
    if not tof.supports("roi"):
        with pytest.raises(DepzError, match="does not support"):
            tof.set_roi(8, 8)
        return
    tof.set_roi(8, 8)
    assert tuple(tof.get_roi()) == (8, 8)
    tof.set_roi(16, 16)


def test_detection_window_is_cleared_by_configure(tof):
    if not tof.supports("thresholds"):
        with pytest.raises(DepzError, match="does not support"):
            tof.set_detection_thresholds(100, 300, 0)
        return
    tof.configure(budget_ms=tof.snap_budget(33))
    tof.set_detection_thresholds(100, 300, 0)
    low, high, window = tof.get_detection_thresholds()
    assert (abs(low - 100) <= 2, abs(high - 300) <= 2) == (True, True)
    # contract 12: configure() re-inits, and the window sits in the config block
    assert len(_stream(tof, 3)) == 3


# ── lifecycle ───────────────────────────────────────────────────────────────


def test_xshut_reset_requires_init(tof):
    kind = tof.driver_kind
    tof.xshut(XSHUT_RESET)
    assert not tof.initialized
    tof.init(kind)
    assert len(_stream(tof, 3)) == 3


@pytest.mark.skipif(
    "DEPZ_CALIB_TARGET_MM" not in os.environ,
    reason="set DEPZ_CALIB_TARGET_MM to the measured distance to a flat target",
)
def test_offset_calibration_centres_the_target(tof):
    if not tof.supports("calib_offset"):
        pytest.skip(f"{tof.product}/{tof.driver_kind} has no offset calibration")
    target = int(os.environ["DEPZ_CALIB_TARGET_MM"])
    tof.configure(budget_ms=tof.snap_budget(50))
    offset = tof.calibrate_offset(target)
    assert isinstance(offset, int)
    ms = [m for m in _stream(tof, 10, 50) if m.status in PLOTTABLE_STATUSES]
    med = sorted(m.distance_mm for m in ms)[len(ms) // 2]
    assert abs(med - target) < 25, (med, target, offset)
