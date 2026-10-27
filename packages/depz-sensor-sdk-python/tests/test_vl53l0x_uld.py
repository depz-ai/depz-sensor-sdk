"""The VL53L0X ULD port (uld/l0x.py) over the bridge fake with a VL53L0X die
model behind it (fake_vl53l0x.py) — no hardware.

The full-stack capture (test_scenarios: vl53l0x_uld_long-range_33ms) pins the
wire bytes of init / configure / stream against a live board; the first test
here holds the fake to the same capture, so the rest can trust it. The rest
is what a capture cannot reach: the reference-SPAD management and its
convergence cases, the NVM identification words, the tuning-buffer parameters,
the timing-budget and sequence-step arithmetic, every ranging profile, the
offset and crosstalk calibrations, the PAL range status mapping, and each
error path. Expected values are worked out from ST's vl53l0x_api*.c (API
1.0.x) in the comments; a difference between that and the port is a driver
bug.
"""

from __future__ import annotations

import json
import struct
from pathlib import Path

import pytest

from depz_sensor_sdk.transport import Packet, PacketParser
from depz_sensor_sdk.vl53lx import Vl53l0x
from depz_sensor_sdk.vl53lx._link import Vl53Error
from depz_sensor_sdk.vl53lx.uld import l0x as L
from depz_sensor_sdk.vl53lx.uld.l0x import VL53L0X, RangingMeasurementData
from fake_vl53l0x import (
    FRAME_635_MM,
    FRAME_NO_TARGET,
    L0X_BOARD,
    PRODUCT_ID,
    FakeVl53l0xDie,
    l0x_rig,
)
from fake_vl53lx import CLEAR_I2C_ERRORS, READ_REG, SET_ADDR_WIDTH, WRITE_REG, XSHUT

RECORDING = (Path(__file__).resolve().parents[3] / "contracts" / "vectors" / "recordings"
             / "vl53l0x_uld_long-range_33ms.depzrec")
REG_CMDS = {READ_REG, WRITE_REG, XSHUT, 0x38, SET_ADDR_WIDTH, CLEAR_I2C_ERRORS}

# One good frame of the capture, decoded by hand: device status 0x5A ->
# internal 11 (valid), 0xC104 effective SPADs in 8.8 -> 193 whole, signal
# 0x022B = 555 in 9.7 -> 284160 in 16.16 (4.34 Mcps), ambient 0x011B = 283
# -> 144896 (2.21 Mcps), distance 0x027B = 635 mm.
F635 = dict(distance=635, signal_1616=0x022B << 9, ambient_1616=0x011B << 9, spads=193)


@pytest.fixture()
def rig():
    made = []

    def make(**die_kw):
        fake, die = l0x_rig(**die_kw)
        dev = Vl53l0x(fake.link, timeout=0.5)
        made.append((dev, fake))
        return dev, fake, die

    yield make
    for dev, fake in made:
        dev.close()
        fake.close()


@pytest.fixture()
def ready(rig):
    """An initialised VL53L0X; the fake's log starts after init."""
    def make(**die_kw):
        dev, fake, die = rig(**die_kw)
        dev.init()
        fake.rx.clear()
        return dev, fake, die
    return make


def _collapse(items):
    out = []
    for item in items:
        if not out or out[-1] != item:
            out.append(item)
    return out


def _captured_init_requests() -> list[tuple[int, bytes]]:
    """The register-level requests of init() in the live capture, from its
    first SET_ADDR_WIDTH to its first CLEAR_I2C_ERRORS."""
    parser = PacketParser()
    out = []
    with RECORDING.open() as f:
        next(f)
        for line in f:
            entry = json.loads(line)
            if entry["dir"] != "tx":
                continue
            for ev in parser.feed(bytes.fromhex(entry["data"])):
                if isinstance(ev, Packet) and ev.cmd in REG_CMDS:
                    out.append((ev.cmd, bytes(ev.payload)))
    start = out.index((SET_ADDR_WIDTH, b"\x01"))
    end = out.index((CLEAR_I2C_ERRORS, b""))
    return out[start:end + 1]


def _reg_requests(fake) -> list[tuple[int, bytes]]:
    return [(c, p) for c, p in fake.rx if c in REG_CMDS]


# ── init against the live capture ────────────────────────────────────────────

def test_init_issues_the_wire_sequence_of_the_live_capture(rig):
    """Byte for byte the requests of the capture's init(): reset, DataInit,
    the NVM reads, the reference SPADs from NVM, the tuning settings, GPIO,
    the budget arithmetic, VHV and phase calibration. Polls (model id while
    the die reboots, data-ready) are collapsed: their count is timing."""
    dev, fake, die = rig()
    dev.init()
    assert _collapse(_reg_requests(fake)) == _collapse(_captured_init_requests())


def test_init_state_matches_the_board(ready):
    dev, fake, die = ready()
    drv = dev.driver
    assert isinstance(drv, VL53L0X)
    assert drv.d["StopVariable"] == 0x3C
    assert drv.d["OscFrequencyMHz"] == 0x975C << 4
    assert drv.ref_spads_from_nvm is True
    assert drv.get_reference_spads() == (6, 0)
    # The 6 non-aperture SPADs through the good map FB..: indices 0,1,3,4,5,6
    # (index 2 is bad) -> 0b0111_1011.
    assert die.spad_enables() == bytes.fromhex("7b0000000000")
    assert drv.d["RefSpadEnables"] == [0x7B, 0, 0, 0, 0, 0]
    # The VHV / phase shots of the reference calibration, in order.
    assert die.shots == [(0x41, 0x01), (0x01, 0x02)]
    assert drv.d["SequenceConfig"] == 0xE8  # TCC and MSRC off, as StaticInit leaves it
    assert drv.d["DeviceMode"] == L.DEVICEMODE_CONTINUOUS_RANGING
    assert dev.get_mode() == "default"
    # StaticInit measured the tuning's budget with TCC still on: 2870 overhead
    # + TCC 2029+590 + DSS 2*(2029+690) + pre 8061+660 + final (509-151 mclks
    # at 10 PCLK = 13651)+550 = 33849 us, and re-applied that with TCC off,
    # which reads back as 2870 + 5438 + 8721 + (577-151 mclks = 16244)+550.
    assert drv.d["MeasurementTimingBudgetMicroSeconds"] == 33849
    assert drv.get_measurement_timing_budget() == 33823
    assert dev.get_range_timing() == (33, 0)
    info = dev.identify_product()
    assert (info["model_id"], info["model_id_ok"], info["driver_reach_mm"]) == (0xEE, True, 1200)
    assert info["modes"] == ("default", "long-range", "high-speed", "high-accuracy")


def test_configure_long_range_33ms_reads_back_as_the_capture_did(ready):
    dev, fake, die = ready()
    dev.configure(budget_ms=33, mode="long-range")
    assert dev.get_range_timing() == (32, 0)
    assert dev.get_mode() == "long-range"
    drv = dev.driver
    # 0.1 Mcps lands on 12/128 in the 9.7 register; the budget reads back
    # 32983 us, the timeouts being quantised to macro periods.
    assert drv.driver_info() == {"signal_mcps": 0.09375, "sigma_mm": 60.0, "budget_us": 32983,
                                 "pre_pclks": 18, "final_pclks": 14}
    assert drv.reach_mm() == 2000
    b0, b1 = die.banks[0], die.banks[1]
    # set_vcsel_pulse_period(): phase limits, period, re-encoded timeouts,
    # the final-range width / phasecal settings — the registers the capture
    # shows being written.
    assert (b0[0x57], b0[0x56], b0[0x50]) == (0x50, 0x08, 0x08)
    assert (b0[0x51:0x53], b0[0x46]) == (b"\x00\x74", 0x1D)
    assert (b0[0x48], b0[0x47], b0[0x70], b0[0x32], b0[0x30], b1[0x30]) == (
        0x48, 0x08, 0x06, 0x03, 0x07, 0x20)
    assert b0[0x71:0x73] == b"\x01\xca"
    # Every VCSEL change ends in a phase calibration (sequence 0x02).
    assert [s for s in die.shots if s == (0x01, 0x02)]


# ── ranging profiles ─────────────────────────────────────────────────────────

# Budget read-backs worked out from the register arithmetic of the C driver
# (macro period = 2304 * PCLK * 1655 ps; timeouts in (LSB * 2^MSB) + 1 macro
# periods; overheads 1910 + 960, DSS 2 * (MSRC + 690), pre + 660, final +
# 550) for each profile applied on the post-init state (pre 0x96, MSRC 0x25).
@pytest.mark.parametrize("mode, info, reach", [
    ("default", {"signal_mcps": 0.25, "sigma_mm": 18.0, "budget_us": 32908,
                 "pre_pclks": 14, "final_pclks": 10}, 1200),
    ("long-range", {"signal_mcps": 0.09375, "sigma_mm": 60.0, "budget_us": 32983,
                    "pre_pclks": 18, "final_pclks": 14}, 2000),
    ("high-speed", {"signal_mcps": 0.25, "sigma_mm": 32.0, "budget_us": 30010,
                    "pre_pclks": 14, "final_pclks": 10}, 1200),
    ("high-accuracy", {"signal_mcps": 0.25, "sigma_mm": 18.0, "budget_us": 199769,
                       "pre_pclks": 14, "final_pclks": 10}, 1200),
])
def test_each_ranging_profile_reads_back(ready, mode, info, reach):
    dev, fake, die = ready()
    drv = dev.driver
    drv.set_mode(mode)
    assert drv.get_mode() == mode
    assert drv.driver_info() == info
    assert drv.reach_mm() == reach
    assert dev.get_range_timing()[0] == info["budget_us"] // 1000


def test_unknown_mode_is_refused(ready):
    dev, fake, die = ready()
    with pytest.raises(Vl53Error, match="no such mode: fast"):
        dev.driver.set_mode("fast")


def test_reach_is_unknown_for_a_final_period_no_profile_sets(ready):
    dev, fake, die = ready()
    dev.driver.set_vcsel_pulse_period(L.VCSEL_PERIOD_FINAL_RANGE, 12)
    assert dev.driver.reach_mm() is None


# ── VCSEL periods ────────────────────────────────────────────────────────────

# The pre-range (8061 us) and MSRC (2029 us) timeouts of the post-init state,
# re-encoded in the macro period of the new VCSEL period: the register values
# and what they decode back to. 18 PCLK is what the capture's long-range
# configure wrote (0x51 <- 0x0074, 0x46 <- 0x1D).
@pytest.mark.parametrize("pclks, phase_high, pre_reg, msrc_reg, pre_us, msrc_us", [
    (12, 0x18, 0x00AF, 0x2B, 8053, 2013),
    (14, 0x30, 0x0096, 0x25, 8061, 2029),
    (16, 0x40, 0x0083, 0x20, 8053, 2013),
    (18, 0x50, 0x0074, 0x1D, 8030, 2059),
])
def test_pre_range_vcsel_period_sets_the_phase_window_and_recodes_the_timeouts(
        ready, pclks, phase_high, pre_reg, msrc_reg, pre_us, msrc_us):
    dev, fake, die = ready()
    drv = dev.driver
    drv.set_vcsel_pulse_period(L.VCSEL_PERIOD_PRE_RANGE, pclks)
    b0 = die.banks[0]
    assert (b0[0x57], b0[0x56], b0[0x50]) == (phase_high, 0x08, (pclks >> 1) - 1)
    assert (b0[0x51:0x53], b0[0x46]) == (pre_reg.to_bytes(2, "big"), msrc_reg)
    assert drv.get_vcsel_pulse_period(L.VCSEL_PERIOD_PRE_RANGE) == pclks
    assert drv.d["PreRangeVcselPulsePeriod"] == pclks
    assert drv.get_sequence_step_timeout(L.SEQUENCESTEP_PRE_RANGE) == pre_us
    assert drv.get_sequence_step_timeout(L.SEQUENCESTEP_MSRC) == msrc_us


# The final-range timeout (16244 us) re-encoded in the new period, then the
# stored 33849 us budget re-applied on top (its remainder, 16270 us, in the
# new macro period): the register and the read-backs.
@pytest.mark.parametrize("pclks, regs, fin_reg, fin_us, budget_us", [
    # regs = (phase high, VCSEL width, phasecal timeout, phasecal limit on page 1)
    (8, (0x10, 0x02, 0x0C, 0x30), 0x02AA, 16168, 33747),
    (10, (0x28, 0x03, 0x09, 0x20), 0x0290, 16244, 33823),
    (12, (0x38, 0x03, 0x08, 0x20), 0x01FD, 16289, 33868),
    (14, (0x48, 0x03, 0x07, 0x20), 0x01E3, 16229, 33808),
])
def test_final_range_vcsel_period_retunes_width_and_phasecal(ready, pclks, regs, fin_reg,
                                                              fin_us, budget_us):
    dev, fake, die = ready()
    drv = dev.driver
    drv.set_vcsel_pulse_period(L.VCSEL_PERIOD_FINAL_RANGE, pclks)
    b0, b1 = die.banks[0], die.banks[1]
    assert (b0[0x48], b0[0x32], b0[0x30], b1[0x30]) == regs
    assert b0[0x47] == 0x08
    assert b0[0x70] == (pclks >> 1) - 1
    assert b0[0x71:0x73] == fin_reg.to_bytes(2, "big")
    assert drv.d["FinalRangeVcselPulsePeriod"] == pclks
    assert drv.get_sequence_step_timeout(L.SEQUENCESTEP_FINAL_RANGE) == fin_us
    assert drv.get_measurement_timing_budget() == budget_us
    assert die.shots[-1] == (0x01, 0x02)      # the phase calibration after it
    assert drv.d["SequenceConfig"] == 0xE8     # ... restored the sequence


@pytest.mark.parametrize("kind, pclks", [
    (L.VCSEL_PERIOD_PRE_RANGE, 13), (L.VCSEL_PERIOD_PRE_RANGE, 10),
    (L.VCSEL_PERIOD_PRE_RANGE, 20), (L.VCSEL_PERIOD_FINAL_RANGE, 6),
    (L.VCSEL_PERIOD_FINAL_RANGE, 16), (L.VCSEL_PERIOD_FINAL_RANGE, 9),
])
def test_vcsel_period_out_of_range_is_refused_before_any_write(ready, kind, pclks):
    dev, fake, die = ready()
    with pytest.raises(Vl53Error, match="out of range"):
        dev.driver.set_vcsel_pulse_period(kind, pclks)
    assert fake.sent(WRITE_REG) == []


# ── timing budget and sequence steps ─────────────────────────────────────────

def test_budget_below_20ms_is_refused_like_the_c_driver(ready):
    """cMinTimingBudgetMicroSeconds = 20000: refused before a register is
    touched, even though the enabled steps would fit."""
    dev, fake, die = ready()
    with pytest.raises(Vl53Error, match="below the 20000 us minimum"):
        dev.driver.set_measurement_timing_budget(19999)
    assert fake.rx == []
    dev.driver.set_measurement_timing_budget(20000)
    assert dev.get_range_timing()[0] == 19     # quantised down, as read back
    with pytest.raises(Vl53Error, match="below the 20000 us minimum"):
        dev.configure(budget_ms=19)


def test_budget_too_small_for_the_enabled_steps(ready):
    dev, fake, die = ready()
    drv = dev.driver
    drv.set_sequence_step_enable(L.SEQUENCESTEP_TCC, 1)
    drv.set_sequence_step_enable(L.SEQUENCESTEP_MSRC, 1)
    drv.set_sequence_step_timeout(L.SEQUENCESTEP_MSRC, 10000)
    # 20000 - 2870 = 17130 left; TCC takes 10000+590, DSS wants 2*(10000+690).
    with pytest.raises(Vl53Error, match="too small for the enabled sequence steps"):
        drv.set_measurement_timing_budget(20000)


def test_tcc_dss_msrc_share_one_timeout_in_the_budget(ready):
    dev, fake, die = ready()
    drv = dev.driver
    base = drv.get_measurement_timing_budget()
    assert drv.get_sequence_step_enables() == {0: False, 1: True, 2: False, 3: True, 4: True}
    # MSRC_CONFIG_TIMEOUT_MACROP 0x25 -> 38 mclks at 14 PCLK (53384 ns) = 2029 us.
    msrc_us = drv.get_sequence_step_timeout(L.SEQUENCESTEP_MSRC)
    assert msrc_us == 2029
    assert drv.get_sequence_step_timeout(L.SEQUENCESTEP_TCC) == msrc_us
    assert drv.get_sequence_step_timeout(L.SEQUENCESTEP_DSS) == msrc_us
    # Reading the budget stores the read-back as the budget to re-apply, as
    # the C getter does (VL53L0X_SETPARAMETERFIELD at its end): 33823 now.
    assert base == 33823
    assert drv.d["MeasurementTimingBudgetMicroSeconds"] == 33823
    assert drv.get_sequence_step_timeout(L.SEQUENCESTEP_FINAL_RANGE) == 16244
    # Enabling TCC re-applies it: TCC takes 2029 + 590 out of the final
    # range, 16244 -> 13625 us, which encodes as 356 + 151 macro periods =
    # 13575 us; the budget reads back 33773 (and is stored again).
    drv.set_sequence_step_enable(L.SEQUENCESTEP_TCC, 1)
    assert drv.d["SequenceConfig"] == 0xF8
    assert die.banks[0][0x01] == 0xF8
    assert drv.get_sequence_step_timeout(L.SEQUENCESTEP_FINAL_RANGE) == 13575
    assert drv.get_measurement_timing_budget() == 33773
    # Enabling what is on already touches nothing.
    fake.rx.clear()
    drv.set_sequence_step_enable(L.SEQUENCESTEP_TCC, 1)
    assert fake.sent(WRITE_REG) == []
    # DSS off gives its 2 * (2029 + 690) back to the final range (33773:
    # 19013 us -> 498 macro periods = 18989); MSRC on takes 2029 + 660 of it
    # instead of DSS's double share (33749: 16300 us -> 426 = 16244).
    drv.set_sequence_step_enable(L.SEQUENCESTEP_DSS, 0)
    assert drv.d["SequenceConfig"] == 0xD0
    assert drv.get_sequence_step_timeout(L.SEQUENCESTEP_FINAL_RANGE) == 18989
    assert drv.get_measurement_timing_budget() == 33749
    drv.set_sequence_step_enable(L.SEQUENCESTEP_MSRC, 1)
    assert drv.d["SequenceConfig"] == 0xD4
    assert drv.get_sequence_step_timeout(L.SEQUENCESTEP_FINAL_RANGE) == 16244
    assert drv.get_measurement_timing_budget() == 33693


def test_msrc_timeout_encoding_saturates_at_255(ready):
    """set_sequence_step_timeout(): the one-byte MSRC timeout is (mclks - 1),
    255 past 256 mclks."""
    dev, fake, die = ready()
    drv = dev.driver
    drv.set_sequence_step_timeout(L.SEQUENCESTEP_TCC, 3000)   # 56 mclks at 14 PCLK
    assert die.banks[0][0x46] == 55
    assert drv.get_sequence_step_timeout(L.SEQUENCESTEP_TCC) == 2990
    drv.set_sequence_step_timeout(L.SEQUENCESTEP_DSS, 20000)  # 375 mclks -> saturates
    assert die.banks[0][0x46] == 255
    assert drv.get_sequence_step_timeout(L.SEQUENCESTEP_MSRC) == 13666   # 256 mclks
    # A zero timeout encodes as 0 (and decodes as one macro period).
    drv.set_sequence_step_timeout(L.SEQUENCESTEP_PRE_RANGE, 0)
    assert die.banks[0][0x51:0x53] == b"\x00\x00"
    assert drv.get_sequence_step_timeout(L.SEQUENCESTEP_PRE_RANGE) == 53


def test_final_range_timeout_without_pre_range(ready):
    """The final-range register holds pre + final; with the pre-range off
    nothing is subtracted / added."""
    dev, fake, die = ready()
    drv = dev.driver
    drv.set_sequence_step_enable(L.SEQUENCESTEP_PRE_RANGE, 0)
    assert drv.d["SequenceConfig"] == 0xA8
    drv.set_sequence_step_timeout(L.SEQUENCESTEP_FINAL_RANGE, 10000)
    # 10000 us at 10 PCLK (38131 ns) = 262 mclks -> encoded (261 >> 1 = 130,
    # MSB 1) = 0x0182, which decodes to 261 mclks = 9952 us.
    assert die.banks[0][0x71:0x73] == b"\x01\x82"
    assert drv.get_sequence_step_timeout(L.SEQUENCESTEP_FINAL_RANGE) == 9952


def test_unknown_sequence_step_is_refused(ready):
    dev, fake, die = ready()
    with pytest.raises(Vl53Error, match="no timeout for sequence step 9"):
        dev.driver.get_sequence_step_timeout(9)
    with pytest.raises(Vl53Error, match="no timeout for sequence step 9"):
        dev.driver.set_sequence_step_timeout(9, 1000)


def test_inter_measurement_period_scales_by_the_oscillator_calibration(ready):
    dev, fake, die = ready()
    drv = dev.driver
    drv.set_inter_measurement_period(100)
    assert die.banks[0][0x04:0x08] == (100 * 0x28).to_bytes(4, "big")
    assert drv.get_inter_measurement_period() == 100
    # An unprogrammed OSC_CALIBRATE_VAL (0) means the period goes in as is.
    die.banks[0][0xF8:0xFA] = b"\x00\x00"
    drv.set_inter_measurement_period(70)
    assert die.banks[0][0x04:0x08] == (70).to_bytes(4, "big")
    assert drv.get_inter_measurement_period() == 70


def test_timed_mode_starts_with_the_timed_start_code(ready):
    dev, fake, die = ready()
    drv = dev.driver
    drv.set_range_timing(50, 100)
    assert drv.d["DeviceMode"] == L.DEVICEMODE_CONTINUOUS_TIMED
    assert dev.get_range_timing() == (49, 100)
    fake.rx.clear()
    drv.start_ranging()
    assert fake.writes()[-1] == (L.SYSRANGE_START, [L.SYSRANGE_MODE_TIMED])
    assert fake.writes()[:7] == [(0x80, [1]), (0xFF, [1]), (0x00, [0]), (0x91, [0x3C]),
                                 (0x00, [1]), (0xFF, [0]), (0x80, [0])]
    with pytest.raises(Vl53Error, match="device mode 7 is not supported"):
        drv.set_device_mode(7)


# ── streaming and polling ────────────────────────────────────────────────────

def test_streamed_and_polled_frames_decode_alike(ready):
    dev, fake, die = ready()
    polled = dev.driver.read_measurement()
    it = dev.measurements(maxsize=4)  # subscribes now: get_measurement() only sees samples sent after it is called
    dev.start_ranging()
    fake.stream(FRAME_635_MM, 471_733_528, addr=0x14, length=12)
    streamed = next(it)
    it.close()
    dev.stop_ranging()
    assert (streamed.distance_mm, streamed.status, streamed.status_text) == (635, 0, "Range Valid")
    assert streamed.signal_kcps == (F635["signal_1616"] * 1000) >> 16
    assert streamed.ambient_kcps == (F635["ambient_1616"] * 1000) >> 16
    assert streamed.spads == F635["spads"]
    assert streamed.extra["device_range_status"] == 0x5A
    for name in ("distance_mm", "status", "signal_kcps", "ambient_kcps", "sigma_mm", "spads", "extra"):
        assert getattr(polled, name) == getattr(streamed, name)
    # StopMeasurement zeroes the stop variable on page 1.
    assert fake.writes()[-6:] == [(0x00, [0]), (0xFF, [1]), (0x00, [0]), (0x91, [0]),
                                  (0x00, [1]), (0xFF, [0])]


def test_single_shot_whose_start_bit_never_clears(ready):
    dev, fake, die = ready()
    die.start_bit_stuck = True
    with pytest.raises(Vl53Error, match="start bit never cleared"):
        dev.driver.perform_single_ranging_measurement()


def test_single_shot_waits_for_the_start_bit_and_data_ready(ready):
    dev, fake, die = ready()
    die.start_bit_reads = 3
    die.ready_after_polls = 4
    data = dev.driver.perform_single_ranging_measurement()
    assert (data.distance_mm, data.range_status) == (635, 0)
    assert dev.driver.d["DeviceMode"] == L.DEVICEMODE_SINGLE_RANGING
    reads = [struct.unpack("<HH", p)[0] for p in fake.sent(READ_REG)]
    assert reads.count(L.SYSRANGE_START) == 4
    assert reads.count(L.RESULT_INTERRUPT_STATUS) == 5 + 1   # polls + the clear's check
    # ... and start_ranging() resumes the continuous mode chosen before.
    fake.rx.clear()
    dev.driver.start_ranging()
    assert fake.writes()[-1] == (L.SYSRANGE_START, [L.SYSRANGE_MODE_BACKTOBACK])


# ── interrupt ────────────────────────────────────────────────────────────────

def test_interrupt_that_will_not_clear(ready):
    dev, fake, die = ready()
    die.interrupt_sticky = True
    die.ready_after_polls = 0
    with pytest.raises(Vl53Error, match="interrupt not cleared after three rounds"):
        dev.driver.perform_single_ranging_measurement()
    assert fake.writes().count((L.SYSTEM_INTERRUPT_CLEAR, [1])) == 3


def test_range_error_bits_in_the_interrupt_status(ready):
    dev, fake, die = ready()
    die.interrupt_idle = 0x48
    with pytest.raises(Vl53Error, match="range error reported in RESULT_INTERRUPT_STATUS 0x48"):
        dev.driver.check_for_data_ready()


def test_data_ready_by_range_status_when_the_gpio_is_not_the_interrupt(ready):
    dev, fake, die = ready()
    drv = dev.driver
    drv.set_gpio_config(0, polarity_low=False)
    assert die.banks[0][0x0A] == 0
    assert die.banks[0][0x84] == 0x11            # bit 4 set: active high
    assert drv.check_for_data_ready() is False
    die.banks[0][0x14] |= 0x01
    assert drv.check_for_data_ready() is True
    assert struct.unpack("<HH", fake.sent(READ_REG)[-1]) == (L.RESULT_RANGE_STATUS, 1)


# ── boot and reset ───────────────────────────────────────────────────────────

def test_boot_timeout_when_the_model_id_never_answers(rig):
    dev, fake, die = rig()
    die.nacks_after_reset = 10 ** 6
    die.banks[0][0xBF] = 0
    drv = VL53L0X(dev._platform, "VL53L0X")
    dev._bridge.set_addr_width(1)
    die.write(0xBF, b"\x01")                      # arm the NACKs
    with pytest.raises(Vl53Error, match="timeout waiting for MODEL_ID"):
        drv.wait_boot(timeout_s=0.05)


def test_soft_reset_timeouts(rig):
    dev, fake, die = rig()
    dev._bridge.set_addr_width(1)
    drv = VL53L0X(dev._platform, "VL53L0X")
    die.reset_ignored = True                     # the model id never drops to 0
    with pytest.raises(Vl53Error, match="timeout in the soft reset"):
        drv.reset_device(timeout_s=0.05)
    die.reset_ignored = False
    die.nacks_after_reset = 10 ** 6              # ... and never comes back
    with pytest.raises(Vl53Error, match="timeout in the soft reset"):
        drv.reset_device(timeout_s=0.05)


# ── NVM ──────────────────────────────────────────────────────────────────────

def test_identification_words_from_the_nvm(ready):
    dev, fake, die = ready()
    drv = dev.driver
    assert drv.d["ReadDataFromDeviceDone"] == 1
    drv.get_info_from_device(7)
    d = drv.d
    assert d["ReadDataFromDeviceDone"] == 7
    assert d["ProductId"] == PRODUCT_ID
    assert (d["ModuleId"], d["Revision"]) == (0x2A, 0x0C)
    assert (d["PartUIDUpper"], d["PartUIDLower"]) == (0x0C345678, 0x9ABCDEF0)
    assert d["SignalRateMeasFixed400mm"] == 0xA53C << 9
    # 0x1940 = 6464 sixteenths = 404 mm at the 400 mm target: -4 mm.
    assert d["Part2PartOffsetAdjustmentNVMMicroMeter"] == -4000
    # The reference-SPAD record is read once (option 1 was done by init).
    strobes = [p for p in fake.sent(WRITE_REG) if p[:2] == b"\x94\x00"]
    assert [p[2] for p in strobes] == [0x02, 0x7B, 0x77, 0x78, 0x79, 0x7A,
                                       0x7B, 0x7C, 0x73, 0x74, 0x75, 0x76]
    fake.rx.clear()
    drv.get_info_from_device(7)
    assert fake.rx == []


def test_nvm_offset_of_a_target_measured_nearer_than_400mm(ready):
    """0x18C0 = 6336 sixteenths = 396 mm: the C driver's uint32 difference
    wraps and its int16 result wraps back, which is what makes the offset
    come out +4 mm."""
    dev, fake, die = ready(nvm={0x75: 0x00000018, 0x76: 0xC0000000})
    dev.driver.get_info_from_device(4)
    assert dev.driver.d["Part2PartOffsetAdjustmentNVMMicroMeter"] == 4000


def test_nvm_without_a_400mm_measurement_has_no_offset(ready):
    dev, fake, die = ready(nvm={0x75: 0, 0x76: 0})
    dev.driver.get_info_from_device(4)
    assert dev.driver.d["Part2PartOffsetAdjustmentNVMMicroMeter"] == 0


def test_nvm_read_strobe_timeout(rig):
    dev, fake, die = rig()
    die.strobe_stuck = True
    with pytest.raises(Vl53Error, match="NVM read strobe"):
        dev.init()


# ── reference SPADs from NVM ─────────────────────────────────────────────────

def test_nvm_aperture_record_is_applied_from_the_aperture_quadrant(ready):
    # 0x8000 bit set: aperture; count 5 -> indices 12..16 -> byte 1 = 0x1F << 4 | byte 2 bit 0
    dev, fake, die = ready(nvm={0x6B: (1 << 15) | (5 << 8)})
    assert dev.driver.get_reference_spads() == (5, 1)
    assert die.spad_enables() == bytes.fromhex("00f001000000")


@pytest.mark.parametrize("nvm, message", [
    ({0x24: 0, 0x25: 0}, "ran out of good reference SPADs"),
    ({0x24: 0x0000FFFF, 0x25: 0}, "leaves the requested quadrant"),
])
def test_nvm_record_that_the_good_map_cannot_serve(rig, nvm, message):
    dev, fake, die = rig(nvm=nvm)
    with pytest.raises(Vl53Error, match=message):
        dev.init()


def test_reference_spad_map_that_does_not_read_back(rig):
    dev, fake, die = rig()
    die.spad_map_stuck = True
    with pytest.raises(Vl53Error, match="did not read back"):
        dev.init()


# ── reference SPAD management ────────────────────────────────────────────────

@pytest.mark.parametrize("record", [(0 << 15) | (13 << 8), (1 << 15) | (33 << 8)])
def test_invalid_nvm_record_makes_init_measure_the_reference_spads(rig, record):
    dev, fake, die = rig(nvm={0x6B: record})
    dev.init()
    drv = dev.driver
    assert drv.ref_spads_from_nvm is False
    # 400 per SPAD against 2560: 3 -> 1200, ..., 6 -> 2400 (160 short),
    # 7 -> 2800 (240 over): the 6-SPAD map comes back.
    assert die.ref_peaks == [1200, 1600, 2000, 2400, 2800]
    assert drv.get_reference_spads() == (6, 0)
    assert die.spad_enables() == bytes.fromhex("7b0000000000")
    assert drv.d["RefSpadEnables"] == [0x7B, 0, 0, 0, 0, 0]
    # It ran after the rest of StaticInit, with the sequence config restored.
    assert drv.d["SequenceConfig"] == 0xE8
    assert die.banks[0][0x01] == 0xE8
    assert dev.get_range_timing() == (33, 0)


def test_management_keeps_the_map_that_overshoots_less(ready):
    dev, fake, die = ready()
    die.ref_rate_per_spad = 700
    # 3 -> 2100 (460 short), 4 -> 2800 (240 over, closer): keep 4.
    assert dev.driver._ref_spad_management() == (4, 0)
    assert die.ref_peaks == [2100, 2800]
    assert die.spad_enables() == bytes.fromhex("1b0000000000")   # indices 0, 1, 3, 4


def test_management_stops_on_the_target_exactly(ready):
    dev, fake, die = ready()
    die.ref_rate_per_spad = 640
    assert dev.driver._ref_spad_management() == (4, 0)
    assert die.ref_peaks == [1920, 2560]


def test_management_stops_at_the_edge_of_the_non_aperture_quadrant(ready):
    dev, fake, die = ready()
    die.ref_rate_per_spad = 100
    # 11 good non-aperture SPADs (index 2 is bad) make 1100; the next good
    # SPAD is an aperture one, so it stops short of the target.
    assert dev.driver._ref_spad_management() == (11, 0)
    assert die.ref_peaks[-1] == 1100
    assert die.spad_enables() == bytes.fromhex("fb0f00000000")


def test_management_switches_to_aperture_spads_when_three_are_too_bright(ready):
    dev, fake, die = ready()
    die.ref_rate_per_spad = 1000
    die.ref_rate_per_aperture_spad = 100
    # 3 non-aperture -> 3000 > 2560; restart at index 12 on aperture SPADs:
    # 3 -> 300 ... 25 -> 2500 (60 short), 26 -> 2600 (40 over, closer): 26.
    assert dev.driver._ref_spad_management() == (26, 1)
    assert die.ref_peaks[:2] == [3000, 300]
    assert die.ref_peaks[-2:] == [2500, 2600]
    enables = die.spad_enables()
    assert enables[0] == enables[1] & 0x0F == 0
    assert sum(bin(b).count("1") for b in enables) == 26
    assert dev.driver.get_reference_spads() == (26, 1)


def test_management_settles_for_three_aperture_spads_when_even_those_are_too_bright(ready):
    dev, fake, die = ready()
    die.ref_rate_per_spad = 1000
    die.ref_rate_per_aperture_spad = 1000
    assert dev.driver._ref_spad_management() == (3, 1)
    assert die.ref_peaks == [3000, 3000]
    assert die.spad_enables() == bytes.fromhex("007000000000")   # indices 12, 13, 14


def test_management_runs_out_of_good_spads(ready):
    dev, fake, die = ready()
    dev.driver.d["RefGoodSpadMap"] = [0x0B, 0, 0, 0, 0, 0]       # indices 0, 1, 3 only
    with pytest.raises(Vl53Error, match="ran out of good reference SPADs"):
        dev.driver._ref_spad_management()


def test_management_needs_the_minimum_from_the_good_map(ready):
    dev, fake, die = ready()
    dev.driver.d["RefGoodSpadMap"] = [0x03, 0, 0, 0, 0, 0]
    with pytest.raises(Vl53Error, match="ran out of good reference SPADs"):
        dev.driver._ref_spad_management()
    dev.driver.d["RefGoodSpadMap"] = [0, 0, 0xFF, 0, 0, 0]       # aperture SPADs only
    with pytest.raises(Vl53Error, match="leaves the requested quadrant"):
        dev.driver._ref_spad_management()


def test_management_map_that_does_not_read_back(ready):
    dev, fake, die = ready()
    die.spad_map_stuck = True
    with pytest.raises(Vl53Error, match="did not read back"):
        dev.driver._ref_spad_management()


def test_public_remeasure_rearms_the_stop_variable_and_restores_the_sequence(ready):
    dev, fake, die = ready()
    dev.start_ranging()
    dev.stop_ranging()
    fake.rx.clear()
    assert dev.perform_ref_spad_management() == (6, 0)
    writes = fake.writes()
    assert writes[:7] == [(0x80, [1]), (0xFF, [1]), (0x00, [0]), (0x91, [0x3C]),
                          (0x00, [1]), (0xFF, [0]), (0x80, [0])]
    # The reference measurements run on the reference steps alone (0xC0),
    # after a VHV / phase calibration, and leave the sequence as it was.
    assert die.shots[-7:] == [(0x41, 0x01), (0x01, 0x02)] + [(0x01, 0xC0)] * 5
    assert die.banks[0][0x01] == 0xE8
    # The last write puts the 6-SPAD map back after the 7th overshot.
    assert writes[-1] == (L.GLOBAL_CONFIG_SPAD_ENABLES_REF_0, [0x7B, 0, 0, 0, 0, 0])
    assert die.banks[0][0x80] == 0x00


# ── tuning settings ──────────────────────────────────────────────────────────

def test_tuning_parameters_in_the_buffer(ready):
    dev, fake, die = ready()
    drv = dev.driver
    fake.rx.clear()
    drv.load_tuning_settings(bytes([
        0xFF, 0x00, 0x00, 0x64,         # SigmaEstRefArray 100
        0xFF, 0x01, 0x03, 0x84,         # SigmaEstEffPulseWidth 900
        0xFF, 0x02, 0x01, 0xF4,         # SigmaEstEffAmbWidth 500
        0xFF, 0x03, 0x0B, 0x00,         # targetRefRate 22 Mcps in 9.7
        0x02, 0x51, 0x00, 0x96,         # a two-byte register write
        0x00,
    ]))
    assert fake.writes() == [(0x51, [0x00, 0x96])]
    d = drv.d
    assert (d["SigmaEstRefArray"], d["SigmaEstEffPulseWidth"], d["SigmaEstEffAmbWidth"]) == (
        100, 900, 500)
    assert d["TargetRefRate"] == 0x0B00
    # ... and the SPAD management converges on the new target: 400 per SPAD,
    # 7 -> 2800 (16 short of 2816), 8 -> 3200 (384 over): 7.
    assert drv._ref_spad_management() == (7, 0)


def test_tuning_buffer_errors(ready):
    dev, fake, die = ready()
    with pytest.raises(Vl53Error, match="bad tuning parameter 4 at offset 0"):
        dev.driver.load_tuning_settings(bytes([0xFF, 0x04, 0x00, 0x00, 0x00]))
    with pytest.raises(Vl53Error, match="bad tuning record at offset 4"):
        dev.driver.load_tuning_settings(bytes([0x01, 0x80, 0x01, 0x05, 0x00, 0, 0, 0, 0, 0, 0]))


# ── result decoding and the PAL range status ─────────────────────────────────

def _frame(device_status, distance=635, signal_97=0x022B, ambient_97=0x011B, spads_88=0xC104):
    """A 12-byte result block in the layout of FRAME_635_MM."""
    return bytes([device_status, 0x06]) + struct.pack(">HHHHH", spads_88, 0x0071,
                                                        signal_97, ambient_97, distance)


@pytest.mark.parametrize("internal, status, text", [
    (11, 0, "Range Valid"),
    (1, 5, "Hardware Fail"), (2, 5, "Hardware Fail"), (3, 5, "Hardware Fail"),
    (6, 4, "Phase Fail"), (9, 4, "Phase Fail"),
    (8, 3, "Min Range Fail"), (10, 3, "Min Range Fail"),
    (4, 2, "Signal Fail"),
    (0, 255, "No Update"), (5, 255, "No Update"), (7, 255, "No Update"),
    (12, 255, "No Update"), (13, 255, "No Update"), (14, 255, "No Update"), (15, 255, "No Update"),
])
def test_device_status_maps_onto_the_pal_range_status(ready, internal, status, text):
    dev, fake, die = ready()
    data = dev.driver.parse_result_block(_frame((internal << 3) | 0x02))
    assert (data.range_status, data.status_text) == (status, text)
    assert data.device_range_status == (internal << 3) | 0x02


def test_sigma_fail_and_the_sigma_estimate(ready):
    dev, fake, die = ready()
    drv = dev.driver
    assert drv.parse_result_block(FRAME_635_MM) .__class__ is RangingMeasurementData
    assert _frame(0x5A) == FRAME_635_MM
    # The sigma estimate of the C driver on this frame, worked by hand with
    # the post-init timeouts (final 16244 us at 10 PCLK, pre 8061 us at 14):
    # 577 macro periods -> 5867 us of VCSEL, 25439 return events; p2 = 2211
    # kcps ambient / 4336 kcps peak; sigma_rtn 152, sigma_ref 67 -> 166000 in
    # 16.16 = 2 mm. Dmax from the 2.21 Mcps ambient: between the 2.0 Mcps
    # (900 mm) and 3.8 Mcps (750 mm) knees -> 882 mm (kept on a valid frame;
    # the GUI shows it).
    good = drv.parse_result_block(FRAME_635_MM)
    assert drv.calc_sigma_estimate(good) == 166000
    assert (good.range_status, good.sigma_mm, good.dmax_mm) == (0, 2, 882)
    # No signal at all: the estimate pins at its maximum (0x028F87AE >> 16 = 655 mm).
    none = drv.parse_result_block(_frame(0x5A, signal_97=0))
    assert (none.range_status, none.sigma_mm) == (1, 655)
    # A faint signal under heavy ambient over a long integration: sigma over 18 mm.
    faint = drv.parse_result_block(_frame(0x5A, distance=1800, signal_97=4, ambient_97=0x0800))
    assert faint.range_status == 1
    assert faint.sigma_mm > 18
    # The sigma check off: the same frame is valid.
    drv.set_limit_check_enable(L.CHECK_SIGMA_FINAL_RANGE, 0)
    off = drv.parse_result_block(_frame(0x5A, distance=1800, signal_97=4, ambient_97=0x0800))
    assert (off.range_status, off.sigma_mm, off.dmax_mm) == (0, 0, 0)
    # ... and so is a zero limit value.
    drv.set_limit_check_enable(L.CHECK_SIGMA_FINAL_RANGE, 1)
    drv.set_limit_check_value(L.CHECK_SIGMA_FINAL_RANGE, 0)
    assert drv.parse_result_block(_frame(0x5A, signal_97=0)).range_status == 0


def test_crosstalk_compensation_enters_the_sigma_estimate(ready):
    dev, fake, die = ready()
    drv = dev.driver
    plain = drv.parse_result_block(FRAME_635_MM)
    assert drv.get_total_xtalk_rate(plain) == 0
    sigma_plain = drv.calc_sigma_estimate(plain)
    drv.set_xtalk(500)                 # 0.5 Mcps per SPAD
    # (spads 8.8 * rate 16.16 + 0x80) >> 8
    assert drv.get_total_xtalk_rate(plain) == ((0xC104 * ((500 << 16) // 1000)) + 0x80) >> 8
    with_xtalk = drv.parse_result_block(FRAME_635_MM)
    assert with_xtalk.range_status == 0
    assert drv.calc_sigma_estimate(with_xtalk) == 99000 != sigma_plain


def test_dmax_follows_the_ambient_lookup(ready):
    dev, fake, die = ready()
    drv = dev.driver
    assert drv.calc_dmax(0) == 1200
    assert drv.calc_dmax(15 << 16) == 400
    assert drv.calc_dmax(20 << 16) == 400
    assert drv.calc_dmax(0x0000B333) == 1100           # a knee of the table
    assert drv.calc_dmax(0x00020000) == 900
    assert 900 < drv.calc_dmax(0x00018000) < 1100      # between two knees
    # On a bad frame the dmax is what the ambient of that frame allows.
    bad = drv.parse_result_block(_frame(0x22, ambient_97=0x1000))   # 32 Mcps ambient
    assert (bad.range_status, bad.dmax_mm) == (2, 400)


def test_fractional_ranges_when_the_range_config_says_so(ready):
    dev, fake, die = ready()
    drv = dev.driver
    drv.d["RangeFractionalEnable"] = 1
    data = drv.parse_result_block(_frame(0x5A, distance=(635 << 2) | 0x3))
    assert (data.distance_mm, data.range_fractional_part) == (635, 0xC0)
    assert RangingMeasurementData(range_status=9).status_text == "unknown (9)"


def test_short_result_block_is_refused(ready):
    dev, fake, die = ready()
    with pytest.raises(Vl53Error, match="result block too short: 11 bytes"):
        dev.driver.decode(FRAME_635_MM[:11])


# ── offset ───────────────────────────────────────────────────────────────────

@pytest.mark.parametrize("um, reg, back", [
    (0, 0x0000, 0), (35_000, 140, 35_000), (-35_000, 4096 - 140, -35_000),
    (511_000, 2044, 511_000), (600_000, 2044, 511_000),       # clamped to 511000
    (-512_000, 2048, -512_000), (-700_000, 2048, -512_000),   # clamped to -512000
    (-1_249, 4096 - 4, -1_000),                               # truncated toward zero
])
def test_offset_register_is_quarter_millimetres_in_twos_complement(ready, um, reg, back):
    dev, fake, die = ready()
    drv = dev.driver
    drv.set_offset_um(um)
    assert die.banks[0][0x28:0x2A] == reg.to_bytes(2, "big")
    assert drv.get_offset_um() == back


def test_offset_in_whole_millimetres_through_the_facade(ready):
    dev, fake, die = ready()
    dev.set_offset_mm(-7)
    assert dev.get_offset_mm() == -7
    assert die.banks[0][0x28:0x2A] == (4096 - 28).to_bytes(2, "big")


def test_offset_calibration_against_a_target(ready):
    dev, fake, die = ready()
    drv = dev.driver
    drv.set_offset_um(12_000)
    drv.set_sequence_step_enable(L.SEQUENCESTEP_TCC, 1)
    die.frames.extend([_frame(0x22), _frame(0x22), FRAME_635_MM, _frame(0x5A, distance=636)])
    # Three valid of five samples (the last one repeats): mean (635+636+636)/3
    # = 635.67 -> 636; the offset becomes (600 - 636) mm.
    assert dev.calibrate_offset(600, nb_samples=5) == -36
    assert drv.get_offset_um() == -36_000
    assert drv.d["SequenceConfig"] == 0xF8                 # TCC back on
    assert len([s for s in die.shots if s[1] == 0xE8]) == 5  # ... and off while it ran
    assert drv.d["LimitChecksEnable"][L.CHECK_RANGE_IGNORE_THRESHOLD] == 0


def test_offset_calibration_errors(ready):
    dev, fake, die = ready()
    drv = dev.driver
    # With TCC off already there is nothing to put back afterwards.
    assert drv.perform_offset_calibration(635, nb_samples=2) == 0
    assert drv.d["SequenceConfig"] == 0xE8
    with pytest.raises(Vl53Error, match="distance must be positive"):
        drv.perform_offset_calibration(0)
    die.frame = _frame(0x22)
    with pytest.raises(Vl53Error, match="no valid measurement"):
        drv.perform_offset_calibration(400, nb_samples=3)
    die.frame = _frame(0x5A, distance=0)
    with pytest.raises(Vl53Error, match="mean range came out 0 mm"):
        drv.perform_offset_calibration(400, nb_samples=3)


# ── crosstalk ────────────────────────────────────────────────────────────────

def test_xtalk_register_is_3_13_and_follows_the_enable(ready):
    dev, fake, die = ready()
    drv = dev.driver
    dev.set_xtalk_kcps(297)
    rate = (297 << 16) // 1000
    assert die.banks[0][0x20:0x22] == (rate >> 3).to_bytes(2, "big")
    assert dev.get_xtalk_kcps() == ((rate >> 3) << 3) * 1000 >> 16
    drv.set_xtalk_enable(0)
    assert die.banks[0][0x20:0x22] == b"\x00\x00"
    assert drv.d["XTalkCompensationRateMegaCps"] == rate      # kept host-side
    drv.set_xtalk_rate_mcps(1 << 16)                         # disabled: stored only
    assert die.banks[0][0x20:0x22] == b"\x00\x00"
    drv.set_xtalk_enable(1)
    assert die.banks[0][0x20:0x22] == ((1 << 16) >> 3).to_bytes(2, "big")


def test_xtalk_calibration_against_a_target_behind_the_glass(ready):
    dev, fake, die = ready()
    drv = dev.driver
    die.frames.extend([_frame(0x22), FRAME_635_MM])
    # Four valid samples of 635 mm, 284160 (16.16) signal, 193 SPADs at a
    # 1000 mm target: per SPAD 1472 * (65536 - 41615) = 35211712, rounded
    # >> 16 = 537 (0.0082 Mcps) -> 8 kcps.
    assert dev.calibrate_xtalk(1000, nb_samples=5) == 8
    assert drv.d["XTalkCompensationRateMegaCps"] == 537
    assert drv.d["XTalkCompensationEnable"] == 1
    assert die.banks[0][0x20:0x22] == (537 >> 3).to_bytes(2, "big")
    assert die.banks[0][0x01] == 0xE8


def test_xtalk_calibration_with_nothing_to_compensate(ready):
    dev, fake, die = ready()
    drv = dev.driver
    # Target at the measured distance (or nearer): no crosstalk.
    assert drv.perform_xtalk_calibration(635, nb_samples=2) == 0
    assert drv.perform_xtalk_calibration(600, nb_samples=2) == 0
    # No effective SPADs at all: the per-SPAD division is skipped.
    die.frame = _frame(0x5A, spads_88=0x0010)
    assert drv.perform_xtalk_calibration(1000, nb_samples=2) == 0
    assert die.banks[0][0x20:0x22] == b"\x00\x00"


def test_xtalk_calibration_errors(ready):
    dev, fake, die = ready()
    drv = dev.driver
    with pytest.raises(Vl53Error, match="distance must be positive"):
        drv.perform_xtalk_calibration(-1)
    die.frame = _frame(0x22)
    with pytest.raises(Vl53Error, match="no valid measurement"):
        drv.perform_xtalk_calibration(1000, nb_samples=2)


# ── limit checks ─────────────────────────────────────────────────────────────

def test_limit_checks_on_the_device_side(ready):
    dev, fake, die = ready()
    drv = dev.driver
    b0 = die.banks[0]
    assert drv.d["LimitChecksEnable"] == [1, 1, 0, 0, 0, 0]
    # DataInit disables the MSRC and pre-range checks (0x60 <- 0x12, as the
    # capture shows); the tuning settings then write 0x60 <- 0x00 over it.
    assert b0[0x60] == 0x00
    drv.set_limit_check_enable(L.CHECK_SIGNAL_RATE_MSRC, 0)
    drv.set_limit_check_enable(L.CHECK_SIGNAL_RATE_PRE_RANGE, 0)
    assert b0[0x60] == 0x12
    # Re-enabling the MSRC check leaves bit 1 set: the C driver updates the
    # byte with the mask 0xFE (bit 0), as its UpdateByte call says, and the
    # port keeps that. The pre-range mask 0xEF does clear its bit 4.
    drv.set_limit_check_enable(L.CHECK_SIGNAL_RATE_MSRC, 1)
    assert b0[0x60] == 0x12
    drv.set_limit_check_value(L.CHECK_SIGNAL_RATE_MSRC, 3 << 16)
    assert b0[0x64:0x66] == ((3 << 16) >> 9).to_bytes(2, "big")
    assert drv.get_limit_check_value(L.CHECK_SIGNAL_RATE_MSRC) == 3 << 16
    assert drv.get_limit_check_value(L.CHECK_SIGNAL_RATE_PRE_RANGE) == 3 << 16
    drv.set_limit_check_enable(L.CHECK_SIGNAL_RATE_PRE_RANGE, 1)
    assert b0[0x60] == 0x02
    drv.set_limit_check_value(L.CHECK_SIGNAL_RATE_PRE_RANGE, 5 << 16)
    assert b0[0x64:0x66] == ((5 << 16) >> 9).to_bytes(2, "big")
    # Disabled: the value is remembered and applied when enabled.
    drv.set_limit_check_value(L.CHECK_SIGNAL_REF_CLIP, 40 << 16)
    assert b0[0x44:0x46] == (0x4000 >> 9).to_bytes(2, "big")
    assert drv.get_limit_check_value(L.CHECK_SIGNAL_REF_CLIP) == 40 << 16
    drv.set_limit_check_enable(L.CHECK_SIGNAL_RATE_FINAL_RANGE, 0)
    assert b0[0x44:0x46] == b"\x00\x00"
    assert drv.get_limit_check_value(L.CHECK_SIGNAL_RATE_FINAL_RANGE) == 0
    drv.set_limit_check_value(L.CHECK_SIGNAL_RATE_FINAL_RANGE, 1 << 16)
    assert b0[0x44:0x46] == b"\x00\x00"
    drv.set_limit_check_enable(L.CHECK_SIGNAL_RATE_FINAL_RANGE, 1)
    assert b0[0x44:0x46] == ((1 << 16) >> 9).to_bytes(2, "big")
    with pytest.raises(Vl53Error, match="no limit check 6"):
        drv.set_limit_check_enable(6, 1)
