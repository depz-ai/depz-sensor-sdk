"""VL53L1X ULD 3.5.5 port (uld/l1.py) over the fake bridge, no hardware: what
the full-stack captures (test_scenarios) and the family tests
(test_vl53lx_device) leave out — the mode change that invalidates a budget,
the unreadable mode byte, the threshold read-modify-write, and the two
calibrations with their refusals. Numbers follow VL53L1X_calibration.c on the
`vl53l1cx_uld_long_33ms_f2` die block of vectors/vl53lx.json.
"""

from __future__ import annotations

import pytest

from depz_sensor_sdk.vl53lx import Vl53Error, Vl53lx
from fake_vl53lx import FakeVl53lxBridge

L1CX_BOARD = "DEPZ ToF Sensor VL53L1CX USB v2.1 SER"
# die_block vl53l1cx_uld_long_33ms_f2: 576 mm, 16072 kcps (0x0098 × 8), 67 SPADs
L1_BLOCK = bytes.fromhex("090901439007d9013f000d2234024007d9")

SYSTEM__INTERRUPT_CONFIG_GPIO = 0x0046
PHASECAL_CONFIG__TIMEOUT_MACROP = 0x004B
XTALK_PLANE_OFFSET_KCPS = 0x0016
RANGE_OFFSET_MM = 0x001E


@pytest.fixture()
def l1():
    fake = FakeVl53lxBridge(L1CX_BOARD)
    dev = Vl53lx(fake.link, timeout=0.5)
    dev.init()
    fake.rx.clear()
    yield dev, fake
    dev.close()
    fake.close()


# ── distance mode ────────────────────────────────────────────────────────────

def test_a_mode_change_moves_an_invalid_budget_to_the_nearest_one(l1):
    """15 ms exists in short mode only: going back to long keeps the period
    and takes the nearest budget long has, 20 ms (VL53L1X_SetDistanceMode
    re-applies the budget; the C driver would fail on the missing row).
    set_mode returns it, so the move is never silent (contract 12 §5)."""
    dev, fake = l1
    dev.configure(mode="short", budget_ms=15)
    assert dev.get_range_timing() == (15, 15)
    fake.rx.clear()
    assert dev.driver.set_mode("long") == 20
    assert dev.get_mode() == "long"
    assert dev.budget_choices() == (20, 33, 50, 100, 200, 500)
    assert dev.get_range_timing() == (20, 15)
    # the long-mode VCSEL/phase registers, then the 20 ms MACROP pair
    assert fake.writes()[:6] == [
        (0x004B, [0x0A]), (0x0060, [0x0F]), (0x0063, [0x0D]), (0x0069, [0xB8]),
        (0x0078, [0x0F, 0x0D]), (0x007A, [0x0E, 0x0E]),
    ]
    assert fake.writes()[6:8] == [(0x005E, [0x00, 0x1E]), (0x0061, [0x00, 0x22])]


def test_a_mode_byte_that_is_neither_mode_is_an_error_not_a_guess(l1):
    dev, fake = l1
    fake.regs[PHASECAL_CONFIG__TIMEOUT_MACROP] = 0x00  # a part fresh out of reset
    with pytest.raises(Vl53Error, match="reads 0x00, which is neither distance mode"):
        dev.get_mode()
    # the budget table depends on the mode, so timing cannot be set either
    with pytest.raises(Vl53Error, match="neither distance mode"):
        dev.driver.set_range_timing(50, 0)


# ── detection thresholds (VL53L1X_SetDistanceThreshold) ──────────────────────

def test_thresholds_rewrite_the_window_bits_and_keep_the_others(l1):
    dev, fake = l1
    assert fake.regs[SYSTEM__INTERRUPT_CONFIG_GPIO] == 0x20  # the blob's byte
    fake.regs[SYSTEM__INTERRUPT_CONFIG_GPIO] = 0x90  # bits outside 0x6F survive
    dev.set_detection_thresholds(100, 300, 3)
    assert fake.writes() == [
        (SYSTEM__INTERRUPT_CONFIG_GPIO, [0x93]),
        (0x0072, [0x01, 0x2C]),  # THRESH_HIGH
        (0x0074, [0x00, 0x64]),  # THRESH_LOW
    ]
    assert dev.get_detection_thresholds() == (100, 300, 3)


def test_a_new_window_replaces_the_old_one_and_int_on_no_target_is_bit_6(l1):
    """Unlike the C driver's `Temp & 0x47`, which keeps the previous window
    bits and the no-target flag, this port clears them: a window set after
    another one is the one that was asked for, and get_detection_thresholds()
    reports it."""
    dev, fake = l1
    dev.driver.set_detection_thresholds(100, 300, 3)
    dev.driver.set_detection_thresholds(100, 300, 1, int_on_no_target=1)
    assert fake.regs[SYSTEM__INTERRUPT_CONFIG_GPIO] == 0x41
    dev.driver.set_detection_thresholds(100, 300, 0)
    assert fake.regs[SYSTEM__INTERRUPT_CONFIG_GPIO] == 0x00
    assert dev.get_detection_thresholds() == (100, 300, 0)


# ── calibrations (VL53L1X_calibration.c) ─────────────────────────────────────

def test_offset_calibration_is_target_minus_the_mean_programmed_x4(l1):
    dev, fake = l1
    fake.set_result_block(L1_BLOCK)  # 576 mm
    assert dev.calibrate_offset(600, 5) == 24
    assert fake.word(RANGE_OFFSET_MM) == 96
    assert dev.get_offset_mm() == 24
    # the three offsets zeroed, timed start (0x40), 5 frames each released,
    # the L1X stop code (0x00), then the result
    assert fake.writes() == [
        (0x001E, [0, 0]), (0x0020, [0, 0]), (0x0022, [0, 0]),
        (0x0087, [0x40]),
        *([(0x0086, [0x01])] * 5),
        (0x0087, [0x00]),
        (0x001E, [0x00, 0x60]),
    ]


def test_offset_calibration_can_come_out_negative(l1):
    dev, fake = l1
    fake.set_result_block(L1_BLOCK)  # 576 mm
    assert dev.calibrate_offset(500, 5) == -76
    assert fake.word(RANGE_OFFSET_MM) == (-76 * 4) & 0xFFFF
    assert dev.get_offset_mm() == -76


def test_xtalk_calibration_512_x_signal_x_1_minus_d_over_target_over_spads(l1):
    dev, fake = l1
    fake.set_result_block(L1_BLOCK)  # 576 mm, 16072 kcps, 67 SPADs
    # 512 × 16072 × (1 − 576/650) / 67 = 13982.6 → 13982 in the 7.9 register
    assert dev.calibrate_xtalk(650, 5) == 27  # kcps, rounded half away
    assert fake.word(XTALK_PLANE_OFFSET_KCPS) == 13982
    assert dev.get_xtalk_kcps() == 27
    # compensation disabled first, 5 frames, then the result
    w = fake.writes()
    assert w[0] == (XTALK_PLANE_OFFSET_KCPS, [0, 0])
    assert w[1] == (0x0087, [0x40])
    assert [a for a, _ in w].count(0x0086) == 5
    assert w[-2:] == [(0x0087, [0x00]), (XTALK_PLANE_OFFSET_KCPS, [0x36, 0x9E])]


def test_xtalk_calibration_clamps_a_target_nearer_than_the_reading_to_zero(l1):
    """(1 − 576/500) is negative; the C driver's uint16 cast is undefined
    there, this port writes 0 (no compensation)."""
    dev, fake = l1
    fake.set_result_block(L1_BLOCK)
    assert dev.calibrate_xtalk(500, 5) == 0
    assert fake.word(XTALK_PLANE_OFFSET_KCPS) == 0


def test_xtalk_calibration_fails_with_no_spads_enabled(l1):
    dev, fake = l1
    no_spads = bytearray(L1_BLOCK)
    no_spads[3:5] = b"\x00\x00"  # RESULT__DSS_ACTUAL_EFFECTIVE_SPADS_SD0 = 0
    fake.set_result_block(bytes(no_spads))
    with pytest.raises(Vl53Error, match="no SPADs enabled"):
        dev.calibrate_xtalk(650, 5)
    assert fake.word(XTALK_PLANE_OFFSET_KCPS) == 0  # left disabled


def test_the_driver_refuses_fewer_than_5_samples_before_any_traffic(l1):
    dev, fake = l1
    with pytest.raises(Vl53Error, match="at least 5"):
        dev.driver.calibrate_offset(600, 4)
    with pytest.raises(Vl53Error, match="at least 5"):
        dev.driver.calibrate_xtalk(600, 4)
    assert fake.rx == []
