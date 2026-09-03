"""VL53L8CH (ToF + CNH superset) surface — the CH-only half of the symmetric
CX/CH test split. Covers the CNH addition (configure_cnh present, CnhConfig
sizing), that CH inherits the CX configuration/guard behaviour, the CX↔CH
firmware-variant mismatch rejection, and that the CX base has NO configure_cnh.
Everything CH shares with the base ToF sensor is tested in
test_vl53l8cx_config.py."""

from __future__ import annotations

import pytest

from depz_sensor_sdk.errors import DepzError
from depz_sensor_sdk.transport.link import LoopbackLink
from depz_sensor_sdk.vl53l8 import (
    RESOLUTION_8X8,
    CnhConfig,
    Vl53l8Ch,
    Vl53l8Cx,
)


def _dev(cls=Vl53l8Ch):
    a, _b = LoopbackLink.pair()
    return cls(a, timeout=0.2)


# ── the CX/CH split: configure_cnh is CH-only ─────────────────────────────────


def test_configure_cnh_is_ch_only():
    assert hasattr(Vl53l8Ch, "configure_cnh")
    assert not hasattr(Vl53l8Cx, "configure_cnh")
    assert Vl53l8Cx._VARIANT == "cx"
    assert Vl53l8Ch._VARIANT == "ch"


def test_cx_instance_has_no_configure_cnh():
    a, _b = LoopbackLink.pair()
    cx = Vl53l8Cx(a, timeout=0.2)
    try:
        assert not hasattr(cx, "configure_cnh")
    finally:
        cx.close()


# ── firmware-variant mismatch rejection (both directions) ─────────────────────


def test_ch_init_rejects_cx_variant():
    dev = _dev(Vl53l8Ch)
    try:
        with pytest.raises(DepzError):
            dev.init("cx")  # Vl53l8Ch loads the 'ch' blob only
    finally:
        dev.close()


def test_cx_init_rejects_ch_variant():
    dev = _dev(Vl53l8Cx)
    try:
        with pytest.raises(DepzError):
            dev.init("ch")  # Vl53l8Cx loads the 'cx' blob only
    finally:
        dev.close()


# ── CH inherits the CX configuration/guard surface ────────────────────────────


def test_ch_inherits_frequency_guard():
    dev = _dev(Vl53l8Ch)
    try:
        with pytest.raises(ValueError):
            dev.set_ranging_frequency_hz(1)  # inherited < 2 Hz guard
    finally:
        dev.close()


def test_ch_inherits_config_while_ranging_guard():
    dev = _dev(Vl53l8Ch)
    try:
        dev._ranging = True  # pretend the stream owns the register bank
        with pytest.raises(DepzError):
            dev.set_resolution(RESOLUTION_8X8)  # inherited guard
        with pytest.raises(DepzError):
            dev.configure_cnh(CnhConfig())  # CH-only method honours the guard too
    finally:
        dev.close()


def test_ch_inherits_uld_before_init_guard():
    dev = _dev(Vl53l8Ch)
    try:
        with pytest.raises(DepzError):
            _ = dev.uld  # inherited: init() not called
    finally:
        dev.close()


# ── CnhConfig sizing ──────────────────────────────────────────────────────────


def test_cnh_config_required_memory_and_distance():
    cfg = CnhConfig()
    cfg.init_config(start_bin=10, num_bins=20, sub_sample=2)
    cfg.create_agg_map(RESOLUTION_8X8, 0, 0, 2, 2, 4, 4)
    assert cfg.nb_of_aggregates == 16
    size = cfg.required_memory()
    assert 0 < size <= 6160  # within CNH_MAX_DATA_BYTES
    lo, hi = cfg.min_max_distance_mm()
    assert lo < hi
