"""Unit tests for the VL53L8 advanced ULD features (power modes, xtalk margin,
detection thresholds, motion indicator) against a fake register/DCI platform.

The fake emulates the DCI read/write transport (swap_buffer + header/footer
framing) so the real byte sequences in uld.py are exercised end to end without
hardware.
"""

from __future__ import annotations

import struct

import pytest

from depz_sensor_sdk.vl53l8 import uld
from depz_sensor_sdk.vl53l8.uld import (
    POWER_MODE_DEEP_SLEEP,
    POWER_MODE_SLEEP,
    POWER_MODE_WAKEUP,
)

from fake_vl53l8 import make_driver as _driver


# ── power modes ───────────────────────────────────────────────────────────────


def test_get_power_mode_wakeup():
    drv, p = _driver()
    p.reg[0x09] = 0x04
    assert drv.get_power_mode() == POWER_MODE_WAKEUP


def test_get_power_mode_sleep_vs_deep():
    drv, p = _driver()
    p.reg[0x09] = 0x02
    p.reg[0x000F] = 0x00
    assert drv.get_power_mode() == POWER_MODE_SLEEP
    p.reg[0x000F] = 0x43
    assert drv.get_power_mode() == POWER_MODE_DEEP_SLEEP


def test_set_power_mode_sleep_sequence():
    drv, p = _driver()
    p.reg[0x09] = 0x04  # currently awake
    p.reg[0x06] = 0x00  # poll target: (buf[0] & 0x01) == 0
    drv.set_power_mode(POWER_MODE_SLEEP)
    # wrote 0x02 to reg 0x09 (sleep)
    assert (0x09, b"\x02") in p.writes


# ── xtalk margin ──────────────────────────────────────────────────────────────


def test_xtalk_margin_roundtrip():
    drv, _ = _driver()
    drv.set_xtalk_margin(50.0)
    assert drv.get_xtalk_margin() == pytest.approx(50.0, abs=0.001)


# ── detection thresholds ──────────────────────────────────────────────────────


def test_detection_thresholds_enable_roundtrip():
    drv, _ = _driver()
    drv.set_detection_thresholds_enable(True)
    assert drv.get_detection_thresholds_enable() == 0x01
    drv.set_detection_thresholds_enable(False)
    assert drv.get_detection_thresholds_enable() == 0x00


def test_detection_thresholds_scaling_roundtrip():
    drv, p = _driver()
    thr = [
        {
            "low_thresh": 200,
            "high_thresh": 600,
            "measurement": uld.DIST_MM,  # scale 4
            "type": uld.THRESH_OUT_OF_WINDOW,
            "zone_num": uld.LAST_THRESHOLD,
            "operation": uld.THRESH_OP_NONE,
        }
    ]
    drv.set_detection_thresholds(thr)
    got = drv.get_detection_thresholds()
    assert len(got) == uld.NB_THRESHOLDS
    assert got[0]["low_thresh"] == 200
    assert got[0]["high_thresh"] == 600
    assert got[0]["measurement"] == uld.DIST_MM
    assert got[0]["zone_num"] == uld.LAST_THRESHOLD
    # verify raw storage was scaled ×4 (distance)
    raw = p.dci[uld.DCI_DET_THRESH_START]
    low_raw = struct.unpack_from("<i", raw, 0)[0]
    assert low_raw == 200 * 4


def test_detection_thresholds_auto_stop():
    drv, p = _driver()
    p.dci[uld.DCI_PIPE_CONTROL] = bytes([1, 0, 1, 0])
    drv.set_detection_thresholds_auto_stop(True)
    assert p.dci[uld.DCI_PIPE_CONTROL][3] == 1


# ── motion indicator ──────────────────────────────────────────────────────────


def test_motion_indicator_init_writes_config():
    drv, p = _driver()
    cfg = drv.motion_indicator_init(uld.RESOLUTION_8X8)
    assert drv._motion_present is True
    assert len(p.dci[uld.DCI_MOTION_DETECTOR_CFG]) == 156
    # 8x8 map_id follows ((i%8)//2) + 4*(i//16)
    assert cfg.map_id[0] == 0
    assert cfg.map_id[63] == (7 % 8) // 2 + 4 * (63 // 16)


def test_motion_set_distance_validates():
    drv, _ = _driver()
    cfg = drv.motion_indicator_init(uld.RESOLUTION_8X8)
    with pytest.raises(uld.Vl53l8cxError):
        drv.motion_indicator_set_distance_motion(cfg, 100, 200)  # min < 400
    drv.motion_indicator_set_distance_motion(cfg, 400, 1500)  # valid


# ── caldata xtalk (buffer copy) ───────────────────────────────────────────────


def test_set_caldata_xtalk_length_check():
    drv, _ = _driver()
    with pytest.raises(uld.Vl53l8cxError):
        drv.set_caldata_xtalk(b"\x00" * 10)
