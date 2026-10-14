"""XTALK_PLANE_OFFSET_KCPS -> kcps on the VL53L1 die rounds like ST's C round().

Calibration stores kcps x512 with a fraction, so the register can hold an exact
.5 kcps (raw % 512 == 256). ST's VL53L4CD_GetXtalk rounds it away from zero;
Python's round() would send it to the even neighbour (256 -> 0, 1280 -> 2).
"""

import pytest

from depz_sensor_sdk.vl53lx.uld.l4 import VL53L4
from depz_sensor_sdk.vl53lx.uld.vl53l1_die import XTALK_PLANE_OFFSET_KCPS, round_half_away


class _OneWord:
    def __init__(self, raw):
        self.raw = raw

    def rd_word(self, addr):
        assert addr == XTALK_PLANE_OFFSET_KCPS
        return self.raw


@pytest.mark.parametrize("x, want", [
    (0.5, 1), (1.5, 2), (2.5, 3), (2.4999, 2), (-0.5, -1), (-2.5, -3), (-2.4, -2),
    (0.49999999999999994, 0), (0.0, 0),
])
def test_round_half_away(x, want):
    assert round_half_away(x) == want


@pytest.mark.parametrize("raw, kcps", [(256, 1), (767, 1), (768, 2), (1280, 3), (65535, 128)])
def test_get_xtalk_rounds_half_away(raw, kcps):
    assert VL53L4(_OneWord(raw), "VL53L4CD").get_xtalk() == kcps
