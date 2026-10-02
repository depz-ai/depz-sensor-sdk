"""VL53L0X on a live session (both found on TB9BGETA6M, 30.09.2026):

- a single shot (the calibrations run on one) leaves the device mode on single
  ranging; start_ranging() must resume the continuous mode chosen before it
  instead of refusing ("start_ranging() needs a continuous device mode");
- stop_ranging() zeroes register 0x91; the public reference-SPAD re-measure
  must put the stop variable back before its VHV/phase single shots, or it
  picks the aperture SPADs and every frame after is Signal Fail.
"""

import pytest

from depz_sensor_sdk.vl53lx.uld.l0x import (
    DEVICEMODE_CONTINUOUS_RANGING,
    DEVICEMODE_CONTINUOUS_TIMED,
    DEVICEMODE_SINGLE_RANGING,
    SYSRANGE_MODE_BACKTOBACK,
    SYSRANGE_MODE_TIMED,
    SYSRANGE_START,
    VL53L0X,
)
from depz_sensor_sdk.vl53lx._link import Vl53Error


class _WriteLog:
    """Just enough platform for start_ranging(): it only writes."""

    def __init__(self):
        self.writes = []

    def wr_byte(self, addr, value):
        self.writes.append((addr, value))


def _start(mode_before_single):
    p = _WriteLog()
    drv = VL53L0X(p, 'VL53L0X')
    if mode_before_single is not None:
        drv.set_device_mode(mode_before_single)
    drv.set_device_mode(DEVICEMODE_SINGLE_RANGING)   # what a calibration leaves
    drv.start_ranging()
    return drv, p.writes[-1]


@pytest.mark.parametrize('mode, start_value', [
    (DEVICEMODE_CONTINUOUS_RANGING, SYSRANGE_MODE_BACKTOBACK),
    (DEVICEMODE_CONTINUOUS_TIMED, SYSRANGE_MODE_TIMED),
])
def test_start_after_single_shot_resumes_continuous_mode(mode, start_value):
    drv, last = _start(mode)
    assert last == (SYSRANGE_START, start_value)
    assert drv.d['DeviceMode'] == mode


def test_start_without_any_continuous_mode_still_refuses():
    with pytest.raises(Vl53Error, match='continuous device mode'):
        _start(None)


class _Abort(Exception):
    pass


class _WritesUntilRead(_WriteLog):
    """Records writes; the first read ends the run (only the prologue matters)."""

    def rd_byte(self, addr):
        raise _Abort

    rd_word = rd_dword = rd_multi = rd_byte


def test_ref_spad_remeasure_rearms_the_stop_variable_first():
    p = _WritesUntilRead()
    drv = VL53L0X(p, 'VL53L0X')
    drv.d['StopVariable'] = 0x3C
    with pytest.raises(_Abort):
        drv.perform_ref_spad_management()
    arm = [(0x80, 0x01), (0xFF, 0x01), (0x00, 0x00), (0x91, 0x3C),
           (0x00, 0x01), (0xFF, 0x00), (0x80, 0x00)]
    assert p.writes[:len(arm)] == arm
