"""VL53L8CX (base ToF) configuration surface: ULD get/set roundtrips over the
fake DCI platform, the >=2 Hz frequency guard and not-while-ranging guards on
the device wrapper, xtalk caldata save/restore. CH-only behaviour lives in
test_vl53l8ch_config.py — the two files keep the CX/CH coverage symmetric."""

from __future__ import annotations

import pytest

from depz_sensor_sdk.errors import DepzError
from depz_sensor_sdk.transport.link import LoopbackLink
from depz_sensor_sdk.vl53l8 import (
    RANGING_MODE_AUTONOMOUS,
    RANGING_MODE_CONTINUOUS,
    RESOLUTION_4X4,
    RESOLUTION_8X8,
    TARGET_ORDER_CLOSEST,
    TARGET_ORDER_STRONGEST,
    Vl53l8Cx,
)
from depz_sensor_sdk.vl53l8 import uld as uld_mod

from fake_vl53l8 import make_driver


def _ready_driver():
    """A fake-platform ULD driver with the NVM/xtalk/config buffers primed so
    set_resolution() (which re-uploads them) works end to end."""
    drv, p = make_driver()
    drv.offset_data = bytes(uld_mod.OFFSET_BUFFER_SIZE)
    drv.xtalk_data = bytes(uld_mod.XTALK_BUFFER_SIZE)
    drv.default_cfg = bytes(4)
    return drv, p


# ── ULD get/set roundtrips ────────────────────────────────────────────────────


def test_resolution_roundtrip():
    drv, _ = _ready_driver()
    drv.set_resolution(RESOLUTION_8X8)
    assert drv.get_resolution() == RESOLUTION_8X8
    drv.set_resolution(RESOLUTION_4X4)
    assert drv.get_resolution() == RESOLUTION_4X4


def test_resolution_invalid_raises():
    drv, _ = _ready_driver()
    with pytest.raises(uld_mod.Vl53l8cxError):
        drv.set_resolution(25)


def test_ranging_frequency_roundtrip():
    drv, _ = make_driver()
    drv.set_ranging_frequency_hz(15)
    assert drv.get_ranging_frequency_hz() == 15


def test_ranging_mode_roundtrip():
    drv, _ = make_driver()
    drv.set_ranging_mode(RANGING_MODE_AUTONOMOUS)
    assert drv.get_ranging_mode() == RANGING_MODE_AUTONOMOUS
    drv.set_ranging_mode(RANGING_MODE_CONTINUOUS)
    assert drv.get_ranging_mode() == RANGING_MODE_CONTINUOUS


def test_integration_time_roundtrip_and_bounds():
    drv, _ = make_driver()
    drv.set_integration_time_ms(20)
    assert drv.get_integration_time_ms() == 20
    with pytest.raises(uld_mod.Vl53l8cxError):
        drv.set_integration_time_ms(1)  # below 2 ms
    with pytest.raises(uld_mod.Vl53l8cxError):
        drv.set_integration_time_ms(2000)  # above 1000 ms


def test_sharpener_roundtrip_and_bound():
    drv, _ = make_driver()
    drv.set_sharpener_percent(20)
    assert drv.get_sharpener_percent() == 20
    with pytest.raises(uld_mod.Vl53l8cxError):
        drv.set_sharpener_percent(100)  # must be < 100


def test_sharpener_roundtrip_is_exact_for_every_legal_value():
    """set(x) -> get() == x for all 0..99, with no tolerance.

    The register holds the percentage scaled to 0..255. Truncating on the way
    back (ST's C ULD does) loses a count for 95 of the 100 legal values, and
    calibrate_xtalk's save/restore then decayed the setting by 1% per run
    (25 -> 24 -> 23 -> ...). This assertion is exact on purpose: the previous
    `approx(20, abs=1)` tolerance is what let the off-by-one live here.
    """
    drv, _ = make_driver()
    for pct in range(100):
        drv.set_sharpener_percent(pct)
        assert drv.get_sharpener_percent() == pct, f"sharpener {pct} did not round-trip"


def test_sharpener_survives_repeated_save_restore():
    """The calibrate_xtalk save/restore pattern must not drift the setting."""
    drv, _ = make_driver()
    drv.set_sharpener_percent(25)
    for _ in range(6):
        saved = drv.get_sharpener_percent()
        drv.set_sharpener_percent(saved)
    assert drv.get_sharpener_percent() == 25, "sharpener decayed across save/restore cycles"


def test_target_order_roundtrip_and_invalid():
    drv, _ = make_driver()
    drv.set_target_order(TARGET_ORDER_STRONGEST)
    assert drv.get_target_order() == TARGET_ORDER_STRONGEST
    drv.set_target_order(TARGET_ORDER_CLOSEST)
    assert drv.get_target_order() == TARGET_ORDER_CLOSEST
    with pytest.raises(uld_mod.Vl53l8cxError):
        drv.set_target_order(9)


# ── xtalk caldata save / restore ──────────────────────────────────────────────


def test_caldata_xtalk_restore_roundtrip():
    drv, _ = _ready_driver()
    drv.set_resolution(RESOLUTION_8X8)  # so get_resolution() != 0 on restore
    blob = bytes(range(256)) * 3 + bytes(uld_mod.XTALK_BUFFER_SIZE - 768)
    assert len(blob) == uld_mod.XTALK_BUFFER_SIZE
    drv.set_caldata_xtalk(blob)  # restore a saved 776-byte xtalk blob
    assert drv.xtalk_data == blob  # re-uploaded via set_resolution


def test_set_caldata_xtalk_wrong_length_raises():
    drv, _ = _ready_driver()
    with pytest.raises(uld_mod.Vl53l8cxError):
        drv.set_caldata_xtalk(b"\x00" * 5)


# ── device wrapper guards ─────────────────────────────────────────────────────


def _dev(cls=Vl53l8Cx):
    a, _b = LoopbackLink.pair()
    return cls(a, timeout=0.2)


def test_frequency_guard_below_2hz_raises():
    dev = _dev()
    try:
        with pytest.raises(ValueError):
            dev.set_ranging_frequency_hz(1)  # < MIN_RANGING_FREQUENCY_HZ
    finally:
        dev.close()


def test_resolution_guard_invalid_zones_raises():
    dev = _dev()
    try:
        with pytest.raises(ValueError):
            dev.set_resolution(37)
    finally:
        dev.close()


def test_config_while_ranging_raises():
    dev = _dev()
    try:
        dev._ranging = True  # pretend the stream owns the register bank
        with pytest.raises(DepzError):
            dev.set_resolution(RESOLUTION_8X8)
        with pytest.raises(DepzError):
            dev.set_ranging_frequency_hz(15)
    finally:
        dev.close()


def test_uld_property_before_init_raises():
    dev = _dev()
    try:
        with pytest.raises(DepzError):
            _ = dev.uld  # init() not called
    finally:
        dev.close()
