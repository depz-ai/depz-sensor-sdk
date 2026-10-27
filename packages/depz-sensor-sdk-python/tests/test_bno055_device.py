"""Bno055 driver behaviour against the in-process fake register bridge +
sensor model (tests/fake_bno055.py), checked against
contracts/13_SENSOR_BNO055.md and vectors/bno055.json. No hardware."""

from __future__ import annotations

import threading
import time

import pytest

import depz_sensor_sdk.bno055 as bno055_mod
from depz_sensor_sdk.bno055 import (
    FULL_BLOCK,
    QUAT_BLOCK,
    SIC_IDENTITY,
    TRIGGER_INT,
    AccelConfig,
    AxisRemap,
    Bno055,
    Bno055Sample,
    CalibrationProfile,
    GyroConfig,
    MagConfig,
    OprMode,
    PwrMode,
    TempSource,
    Units,
)
from depz_sensor_sdk.device import TextEvent
from depz_sensor_sdk.errors import DepzError, DepzTimeoutError, InvalidStateError, LinkClosedError, StatusError
from depz_sensor_sdk.protocol.bno055 import Bno055Info
from fake_bno055 import LIVE_INFO_HEX, LIVE_QUAT, FakeBno055

ALL_UNITS = Units(accel_mg=True, gyro_rps=True, euler_rad=True, temp_f=True, android=True)


class VirtualTime:
    """`time` stand-in for the driver module: sleeps are recorded and advance
    a virtual offset instead of blocking, `monotonic` keeps running so real
    waits (queue timeouts) still make progress."""

    def __init__(self):
        self.sleeps: list[float] = []
        self._offset = 0.0

    def sleep(self, s: float) -> None:
        self.sleeps.append(s)
        self._offset += s

    def monotonic(self) -> float:
        return time.monotonic() + self._offset


@pytest.fixture
def fake():
    f = FakeBno055()
    yield f
    f.close()


@pytest.fixture
def vt(monkeypatch):
    t = VirtualTime()
    monkeypatch.setattr(bno055_mod, "time", t)
    return t


@pytest.fixture
def dev(fake, vt):
    d = Bno055(fake.link, timeout=0.3)
    yield d
    d.close()


def W(page: int, addr: int, hex_: str) -> str:
    return f"{page}:{addr:02x}={hex_}"


def wait_for(pred, timeout: float = 1.0) -> None:
    deadline = time.monotonic() + timeout
    while not pred():
        if time.monotonic() > deadline:
            raise AssertionError("condition not met in time")
        time.sleep(0.002)


def vec(vectors, section: str, name: str) -> dict:
    return next(c for c in vectors("bno055.json")[section] if c["name"] == name)


def profile_of(vectors, name: str) -> CalibrationProfile:
    return CalibrationProfile.unpack(bytes.fromhex(vec(vectors, "calibration_profile", name)["bytes"]))


# ── identity & bridge info ───────────────────────────────────────────────────


def test_bridge_info_and_is_alive(fake, dev):
    info = dev.bridge_info()
    assert info == Bno055Info.unpack(bytes.fromhex(LIVE_INFO_HEX))
    assert info.sw_rev_text == "03.11"
    assert fake.sent == [(0x37, b"")]
    assert dev.is_alive()
    fake.info = bytes.fromhex(LIVE_INFO_HEX[:16] + "00" + LIVE_INFO_HEX[18:])  # initialized = 0
    assert not dev.is_alive()
    fake.info = bytes.fromhex("28a1" + LIVE_INFO_HEX[4:])  # wrong chip id
    assert not dev.is_alive()


def test_is_alive_false_not_a_throw_on_errors(fake, dev):
    fake.fail_next[0x37] = [0x08]
    assert not dev.is_alive()
    fake.silent.add(0x37)
    assert not dev.is_alive()


# ── raw register access ──────────────────────────────────────────────────────


def test_read_and_write_registers(fake, dev):
    assert dev.read_registers(0x00, 4) == bytes([0xA0, 0xFB, 0x32, 0x0F])
    assert fake.sent[-1] == (0x32, bytes([0x00, 0x04]))
    assert dev.read_register(0x00) == 0xA0
    dev.write_register(0x40, 0x101)  # masked to a byte
    assert fake.writes()[-1] == W(0, 0x40, "01")
    dev.write_registers(0x55, b"\x01\x02")
    assert fake.regs[0][0x55:0x57] == b"\x01\x02"


def test_transfers_over_128_bytes_are_split(fake, dev):
    fake.regs[1][0x10:0xD8] = bytes(range(200))
    assert dev.read_registers(0x10, 200, page=1) == bytes(range(200))
    assert [(p[0], p[1]) for c, p in fake.sent if c == 0x32] == [(0x10, 128), (0x90, 72)]
    dev.write_registers(0x10, bytes(200), page=1)
    assert [(p[0], len(p) - 1) for c, p in fake.sent if c == 0x33 and p[0] != 0x07] == [
        (0x10, 128), (0x90, 72)]
    assert fake.regs[1][0x10:0xD8] == bytes(200) and fake.regs[1][0xD8] == 0


def test_page_1_is_wrapped_in_page_id_writes(fake, dev):
    assert dev.read_register(0x50, page=1) == 0xC0
    assert fake.writes() == [W(0, 0x07, "01"), W(1, 0x07, "00")]
    assert fake.page == 0


def test_page_1_returns_to_page_0_even_when_the_access_fails(fake, dev):
    fake.fail_next[0x32] = [0x09]
    with pytest.raises(StatusError):
        dev.read_register(0x50, page=1)
    assert fake.writes() == [W(0, 0x07, "01"), W(1, 0x07, "00")]
    # the lock is not wedged
    assert dev.read_register(0x00) == 0xA0


def test_refuses_pages_other_than_0_and_1(fake, dev):
    with pytest.raises(ValueError, match="pages 0 and 1"):
        dev.read_register(0x00, page=2)
    assert fake.sent == []


def test_short_reg_data_is_an_error_not_padding(fake, dev):
    fake.short_read_by = 1
    with pytest.raises(DepzError, match="expected 4, got 3"):
        dev.read_registers(0x00, 4)


def test_silent_bridge_times_out(fake, dev):
    fake.silent.add(0x32)
    with pytest.raises(DepzTimeoutError):
        dev.read_register(0x00)


def test_multi_step_sequences_are_serialised_across_threads(fake, dev):
    fake.set_mode(OprMode.NDOF)
    t = threading.Thread(target=dev.set_units, args=(ALL_UNITS,))
    t.start()
    wait_for(lambda: W(0, 0x3D, "00") in fake.writes())
    assert dev.read_register(0x00) == 0xA0  # blocks until the CONFIG round trip is over
    t.join(1.0)
    kinds = [f"w{o.addr:x}={o.data.hex()}" if o.op == "w" else f"r{o.addr:x}" for o in fake.ops]
    assert kinds.index("r0") > kinds.index("w3d=0c")


# ── operation mode ───────────────────────────────────────────────────────────


def test_operation_mode_masks_bits_7_4(fake, dev):
    fake.regs[0][0x3D] = 0x1C
    assert dev.get_operation_mode() == OprMode.NDOF


def test_config_to_ndof_waits_for_a_nonzero_quaternion(fake, dev, vt):
    dev.set_operation_mode(OprMode.NDOF)
    assert fake.writes() == [W(0, 0x3D, "0c")]
    assert vt.sleeps[0] == pytest.approx(bno055_mod.MODE_SWITCH_FROM_CONFIG_S)
    assert fake.reads(0x20) == 3  # two zero reads, then the live quaternion
    assert fake.opr_mode == OprMode.NDOF


def test_mode_to_mode_goes_through_config(fake, dev):
    fake.set_mode(OprMode.NDOF)
    dev.set_operation_mode(OprMode.AMG)
    assert fake.writes() == [W(0, 0x3D, "00"), W(0, 0x3D, "07")]
    assert fake.opr_mode == OprMode.AMG


def test_same_mode_writes_nothing(fake, dev):
    fake.set_mode(OprMode.NDOF)
    dev.set_operation_mode(0x0C)
    assert fake.writes() == []


def test_to_config_is_one_write_with_the_longer_wait(fake, dev, vt):
    fake.set_mode(OprMode.NDOF)
    dev.set_operation_mode(OprMode.CONFIG)
    assert fake.writes() == [W(0, 0x3D, "00")]
    assert fake.reads(0x3D) == 0
    assert vt.sleeps == [pytest.approx(bno055_mod.MODE_SWITCH_TO_CONFIG_S)]


def test_plain_mode_switch_gives_up_waiting_for_fusion_quietly(fake, dev, vt):
    fake.fusion_stuck = True
    dev.set_operation_mode(OprMode.IMU)
    assert fake.opr_mode == OprMode.IMU
    # polled the quaternion for the whole deadline (10 ms steps), then gave up
    assert fake.reads(0x20) >= 50


# ── power mode (§4.6) ────────────────────────────────────────────────────────


def test_suspend_from_ndof_then_wake_into_ndof(fake, dev):
    fake.set_mode(OprMode.NDOF)
    dev.set_power_mode(PwrMode.SUSPEND)
    assert fake.writes() == [W(0, 0x3D, "00"), W(0, 0x3E, "02")]
    assert fake.opr_mode == OprMode.CONFIG
    fake.ops.clear()
    dev.set_power_mode(PwrMode.NORMAL)
    assert fake.writes() == [W(0, 0x3E, "00"), W(0, 0x3D, "0c")]
    assert fake.opr_mode == OprMode.NDOF
    assert dev.get_power_mode() == PwrMode.NORMAL


def test_suspend_from_amg_wakes_into_amg(fake, dev):
    fake.set_mode(OprMode.AMG)
    dev.set_power_mode(PwrMode.SUSPEND)
    fake.ops.clear()
    dev.set_power_mode(PwrMode.LOW_POWER)
    assert fake.writes() == [W(0, 0x3E, "01"), W(0, 0x3D, "07")]


def test_chip_found_asleep_in_config_wakes_into_ndof(fake, dev):
    fake.regs[0][0x3E] = PwrMode.SUSPEND
    dev.set_power_mode(PwrMode.NORMAL)
    assert fake.writes() == [W(0, 0x3E, "00"), W(0, 0x3D, "0c")]
    assert fake.opr_mode == OprMode.NDOF


def test_config_and_awake_stays_in_config(fake, dev):
    dev.set_power_mode(PwrMode.LOW_POWER)
    assert fake.writes() == [W(0, 0x3E, "01")]
    assert fake.opr_mode == OprMode.CONFIG


def test_low_power_keeps_the_operating_mode(fake, dev):
    fake.set_mode(OprMode.IMU)
    dev.set_power_mode(PwrMode.LOW_POWER)
    assert fake.writes() == [W(0, 0x3D, "00"), W(0, 0x3E, "01"), W(0, 0x3D, "08")]
    assert fake.opr_mode == OprMode.IMU


# ── units, axes, temperature source ──────────────────────────────────────────


def test_power_on_unit_sel_reads_as_android(fake, dev):
    assert dev.get_units() == Units(android=True)


def test_set_units_inside_a_config_round_trip(fake, dev):
    fake.set_mode(OprMode.NDOF)
    dev.set_units(ALL_UNITS)
    assert fake.writes() == [W(0, 0x3D, "00"), W(0, 0x3B, "97"), W(0, 0x3D, "0c")]
    assert dev.get_units() == ALL_UNITS


def test_set_units_in_config_does_not_leave_config(fake, dev):
    dev.set_units(Units())
    assert fake.writes() == [W(0, 0x3B, "00")]


def test_previous_mode_restored_when_the_config_side_write_fails(fake, dev):
    fake.set_mode(OprMode.NDOF)
    fake.fail_write_reg[0x3B] = 0x09
    with pytest.raises(StatusError):
        dev.set_units(Units())
    assert fake.writes() == [W(0, 0x3D, "00"), W(0, 0x3D, "0c")]
    assert fake.opr_mode == OprMode.NDOF


def test_axis_remap_by_placement_name(fake, dev, vectors):
    fake.set_mode(OprMode.NDOF)
    dev.set_axis_remap("p0")
    c = vec(vectors, "encode", "write_reg_axis_map_p0")
    assert (0x33, bytes.fromhex(c["payload"])) in fake.sent
    assert fake.writes() == [W(0, 0x3D, "00"), W(0, 0x41, "2104"), W(0, 0x3D, "0c")]
    assert dev.get_axis_remap() == AxisRemap.placement("P0")
    dev.set_axis_remap(AxisRemap())
    assert dev.get_axis_remap() == AxisRemap.placement("P1")


def test_refuses_non_permutation_and_unknown_placements_before_traffic(fake, dev):
    with pytest.raises(ValueError, match="permutation"):
        dev.set_axis_remap(AxisRemap(x=0, y=0, z=1))
    with pytest.raises(ValueError, match="P0..P7"):
        dev.set_axis_remap("P9")
    assert fake.sent == []


def test_temperature_source(fake, dev):
    fake.set_mode(OprMode.NDOF)
    dev.set_temperature_source(TempSource.GYRO)
    assert fake.writes() == [W(0, 0x3D, "00"), W(0, 0x40, "01"), W(0, 0x3D, "0c")]
    assert dev.get_temperature_source() == TempSource.GYRO


# ── configure / restore_configuration / reset ────────────────────────────────


def test_configure_full_sequence(fake, dev, vectors):
    profile = profile_of(vectors, "extremes")
    fake.sys_status_script = [3, 4, 0]
    dev.configure(units=ALL_UNITS, axis_remap="P2", calibration=profile)
    assert fake.writes() == [
        W(0, 0x3D, "00"),
        W(0, 0x3B, "97"),
        W(0, 0x41, "2406"),
        W(0, 0x55, profile.pack().hex()),
        W(0, 0x3D, "0c"),
    ]
    assert fake.sys_status_script == []  # polled until the boot states were over
    assert fake.opr_mode == OprMode.NDOF
    assert fake.regs[0][0x3B] == 0x97
    assert dev.get_units() == ALL_UNITS


def test_configure_defaults(fake, dev):
    fake.set_mode(OprMode.NDOF)
    dev.configure()
    assert fake.writes() == [W(0, 0x3D, "00"), W(0, 0x3B, "00"), W(0, 0x3D, "0c")]


def test_configure_non_fusion_does_not_wait_for_the_quaternion(fake, dev):
    dev.configure(OprMode.AMG)
    assert fake.reads(0x20) == 0
    assert fake.opr_mode == OprMode.AMG


def test_configure_fails_when_fusion_never_starts(fake, dev):
    fake.fusion_stuck = True
    with pytest.raises(DepzError, match="fusion did not start"):
        dev.configure()


def test_configure_fails_when_the_sensor_never_leaves_boot(fake, dev):
    fake.sys_status_script = [3] * 100_000
    with pytest.raises(DepzError, match="did not finish booting"):
        dev.configure()
    assert fake.writes() == []


def test_sys_status_4_for_20_polls_is_a_self_test_leftover(fake, dev):
    fake.sys_status_script = [4] * 20
    dev.configure(OprMode.AMG)
    assert fake.writes()[:2] == [W(0, 0x3D, "01"), W(0, 0x3D, "00")]
    assert fake.reads(0x39) == 21


def test_restore_configuration(fake, dev, vectors):
    with pytest.raises(InvalidStateError):
        dev.restore_configuration()
    dev.configure(units=ALL_UNITS, axis_remap="P2", calibration=profile_of(vectors, "extremes"))
    first = fake.writes()
    dev.reset_sensor()  # back to power-on registers
    assert fake.regs[0][0x3B] == 0x80
    fake.ops.clear()
    dev.restore_configuration()
    assert fake.writes() == first
    assert fake.regs[0][0x3B] == 0x97


def test_reset_sensor(fake, dev):
    dev.set_units(ALL_UNITS)
    dev.start_stream()
    fake.sys_status_script = [2, 3, 0]
    dev.reset_sensor()
    assert (0x34, b"") in fake.sent
    assert fake.reset_count == 1 and not fake.streaming
    assert not dev.streaming
    assert fake.sys_status_script == []
    # units cache dropped: power-on 0x80 is read back, not the pre-reset units
    assert dev.read_sample((0x34, 1)).units == Units(android=True)


def test_reset_sensor_surfaces_a_failure(fake, dev):
    fake.fail_next[0x34] = [0x09]
    with pytest.raises(StatusError):
        dev.reset_sensor()


# ── status, self-test, calibration ───────────────────────────────────────────


def test_system_status_never_reads_int_sta(fake, dev):
    fake.regs[0][0x37] = 0x40
    fake.regs[0][0x38] = 1
    fake.set_mode(OprMode.NDOF)
    st = dev.system_status()
    assert (st.self_test, st.clk_status, st.status, st.error) == (0x0F, 1, 5, 0)
    assert st.self_test_passed and st.status_text == "fusion algorithm running"
    assert fake.regs[0][0x37] == 0x40
    assert [(o.addr, len(o.data)) for o in fake.ops] == [(0x36, 1), (0x38, 3)]


def test_self_test_from_ndof(fake, dev, vt):
    fake.set_mode(OprMode.NDOF)
    fake.regs[0][0x3F] = 0x80  # CLK_SEL: must be kept
    st = dev.self_test()
    assert st.self_test_passed and st.error == 0
    assert fake.writes() == [W(0, 0x3D, "00"), W(0, 0x3F, "81"), W(0, 0x3D, "0c")]
    assert bno055_mod.SELF_TEST_S in vt.sleeps
    assert fake.opr_mode == OprMode.NDOF
    assert fake.regs[0][0x3F] == 0x80


def test_self_test_reports_a_failure(fake, dev):
    fake.self_test_result = 0x0B  # gyro bit cleared
    fake.self_test_error = 3
    fake.set_mode(OprMode.NDOF)
    st = dev.self_test()
    assert not st.self_test_passed
    assert st.error_text == "self-test failed"


def test_self_test_in_config_clears_the_leftover(fake, dev):
    st = dev.self_test()
    assert st.status == 4  # as the sensor reports it right after the test
    assert fake.writes() == [W(0, 0x3F, "01"), W(0, 0x3D, "01"), W(0, 0x3D, "00")]
    assert dev.system_status().status == 0


def test_self_test_restores_the_mode_when_the_trigger_write_fails(fake, dev):
    fake.set_mode(OprMode.NDOF)
    fake.fail_write_reg[0x3F] = 0x09
    with pytest.raises(StatusError):
        dev.self_test()
    assert fake.writes() == [W(0, 0x3D, "00"), W(0, 0x3D, "0c")]
    assert fake.opr_mode == OprMode.NDOF


def test_self_test_is_refused_while_streaming(fake, dev):
    dev.start_stream()
    n = len(fake.sent)
    with pytest.raises(InvalidStateError):
        dev.self_test()
    assert len(fake.sent) == n


def test_calibration_status(fake, dev):
    fake.regs[0][0x35] = 0xFF
    assert dev.calibration_status().fully_calibrated
    fake.regs[0][0x35] = 0xC4
    s = dev.calibration_status()
    assert (s.system, s.gyro, s.accel, s.mag) == (3, 0, 1, 0)


def test_calibration_profile_round_trip_in_config(fake, dev, vectors):
    profile = profile_of(vectors, "extremes")
    fake.regs[0][0x55:0x6B] = profile.pack()
    fake.set_mode(OprMode.NDOF)
    assert dev.read_calibration_profile() == profile
    assert fake.writes() == [W(0, 0x3D, "00"), W(0, 0x3D, "0c")]
    kinds = [f"w{o.addr:x}={o.data.hex()}" if o.op == "w" else f"r{o.addr:x}+{len(o.data)}" for o in fake.ops]
    assert kinds.index("w3d=00") < kinds.index("r55+22") < kinds.index("w3d=0c")
    fake.ops.clear()
    typical = profile_of(vectors, "typical")
    dev.write_calibration_profile(typical)
    assert fake.writes() == [W(0, 0x3D, "00"), W(0, 0x55, typical.pack().hex()), W(0, 0x3D, "0c")]
    assert fake.regs[0][0x55:0x6B] == typical.pack()


def test_sic_matrix(fake, dev):
    fake.set_mode(OprMode.NDOF)
    dev.set_sic_matrix()
    assert fake.writes() == [W(0, 0x3D, "00"), W(0, 0x43, "0040" + "0000" * 3 + "0040" + "0000" * 3 + "0040"), W(0, 0x3D, "0c")]
    assert dev.get_sic_matrix() == SIC_IDENTITY


# ── page-1 configuration and interrupts ──────────────────────────────────────


def test_accel_config(fake, dev):
    fake.set_mode(OprMode.NDOF)
    assert dev.get_accel_config() == AccelConfig(range=1, bandwidth=3, power=0)
    fake.ops.clear()
    dev.set_accel_config(AccelConfig(range=3, bandwidth=7, power=0))
    assert fake.writes() == [
        W(0, 0x3D, "00"),
        W(0, 0x07, "01"),
        W(1, 0x08, "1f"),
        W(1, 0x07, "00"),
        W(0, 0x3D, "0c"),
    ]
    assert fake.page == 0 and fake.opr_mode == OprMode.NDOF


def test_gyro_and_mag_configs(fake, dev):
    assert dev.get_gyro_config() == GyroConfig(range=0, bandwidth=7, power=0)
    assert dev.get_mag_config() == MagConfig(rate=5, mode=1, power=3)
    dev.set_gyro_config(GyroConfig(range=2, bandwidth=3, power=1))
    dev.set_mag_config(MagConfig(rate=7, mode=3, power=0))
    assert [w for w in fake.writes() if w.startswith(("1:0a", "1:09"))] == [
        W(1, 0x0A, "1a01"), W(1, 0x09, "1f")]
    assert dev.get_gyro_config() == GyroConfig(range=2, bandwidth=3, power=1)
    assert dev.get_mag_config() == MagConfig(rate=7, mode=3, power=0)


def test_unique_id(fake, dev):
    assert dev.unique_id() == bytes(range(0xC0, 0xD0))


def test_page_1_is_refused_while_streaming(fake, dev):
    dev.start_stream()
    n = len(fake.sent)
    with pytest.raises(InvalidStateError):
        dev.unique_id()
    with pytest.raises(InvalidStateError):
        dev.set_accel_config(AccelConfig())
    assert len(fake.sent) == n
    dev.stop_stream()
    assert dev.unique_id()[0] == 0xC0


def test_interrupt_enable_and_mask_skip_the_config_round_trip(fake, dev):
    fake.set_mode(OprMode.NDOF)
    dev.set_interrupt_enable(0x40)
    dev.set_interrupt_mask(0xC0)
    assert fake.writes() == [
        W(0, 0x07, "01"), W(1, 0x10, "40"), W(1, 0x07, "00"),
        W(0, 0x07, "01"), W(1, 0x0F, "c0"), W(1, 0x07, "00"),
    ]
    assert dev.get_interrupt_enable() == 0x40
    assert dev.get_interrupt_mask() == 0xC0


def test_interrupt_setting_accepts_only_page_1_0x11_to_0x1f(fake, dev):
    for bad in (0x10, 0x20):
        with pytest.raises(ValueError, match="interrupt setting"):
            dev.set_interrupt_setting(bad, 1)
    assert fake.sent == []
    fake.set_mode(OprMode.AMG)
    dev.set_interrupt_setting(0x11, 0x10A)
    assert fake.writes() == [
        W(0, 0x3D, "00"), W(0, 0x07, "01"), W(1, 0x11, "0a"), W(1, 0x07, "00"), W(0, 0x3D, "07"),
    ]


def test_int_sta_clears_on_read_and_clear_interrupt_keeps_clk_sel(fake, dev):
    fake.regs[0][0x37] = 0x40
    fake.regs[0][0x3F] = 0x80
    assert dev.read_interrupt_status() == 0x40
    assert dev.read_interrupt_status() == 0
    dev.clear_interrupt()
    assert fake.writes() == [W(0, 0x3F, "c0")]
    assert fake.regs[0][0x3F] == 0x80


# ── polling ──────────────────────────────────────────────────────────────────


def test_read_sample_full_block_units_cached(fake, dev, vectors):
    c = vec(vectors, "blocks", "full_block_live")
    fake.regs[0][0x08:0x36] = bytes.fromhex(c["data"])
    s = dev.read_sample()
    assert s.units == Units(android=True)
    assert s.addr == FULL_BLOCK[0] and s.raw == bytes.fromhex(c["data"])
    assert s.timestamp_us == fake.mcu_time_us
    e = c["expect"]
    assert s.accel == pytest.approx(tuple(v / 100 for v in e["accel"]))
    assert s.mag == pytest.approx(tuple(v / 16 for v in e["mag"]))
    assert s.gyro == pytest.approx(tuple(v / 16 for v in e["gyro"]))
    assert s.euler == pytest.approx(tuple(v / 16 for v in e["euler"]))
    assert s.quaternion == pytest.approx(tuple(v / 16384 for v in e["quaternion"]))
    assert s.linear_accel == pytest.approx(tuple(v / 100 for v in e["linear_accel"]))
    assert s.gravity == pytest.approx(tuple(v / 100 for v in e["gravity"]))
    assert s.temperature == 26 and s.calibration.pack() == 0
    dev.read_sample()
    assert fake.reads(0x3B) == 1  # UNIT_SEL read once, then cached


def test_read_sample_scales_by_every_unit_bit(fake, dev, vectors):
    c = vec(vectors, "blocks", "full_block_live")
    fake.regs[0][0x08:0x36] = bytes.fromhex(c["data"])
    fake.regs[0][0x3B] = 0x97
    s = dev.read_sample()
    e = c["expect"]
    assert s.units == ALL_UNITS
    assert s.accel == pytest.approx(tuple(float(v) for v in e["accel"]))  # mg: 1 LSB
    assert s.gyro == pytest.approx(tuple(v / 900 for v in e["gyro"]))
    assert s.euler == pytest.approx(tuple(v / 900 for v in e["euler"]))
    assert s.temperature == 52  # °F: 1 LSB = 2 °F
    assert s.linear_accel == pytest.approx(tuple(v / 100 for v in e["linear_accel"]))  # always m/s²
    assert s.gravity == pytest.approx(tuple(v / 100 for v in e["gravity"]))


def test_read_sample_window_leaves_uncovered_channels_none(fake, dev):
    fake.regs[0][0x34] = 0xE7  # -25 °C
    fake.regs[0][0x35] = 0xFF
    s = dev.read_sample((0x34, 2))
    assert s.temperature == -25 and s.calibration.fully_calibrated
    assert (s.accel, s.mag, s.gyro, s.euler, s.quaternion, s.linear_accel, s.gravity) == (None,) * 7


def test_read_quaternion_reads_only_the_8_byte_block(fake, dev):
    fake.set_mode(OprMode.NDOF)
    q = dev.read_quaternion()
    assert q == pytest.approx((10685 / 16384, -1693 / 16384, -12305 / 16384, 0.0))
    assert [(o.addr, len(o.data)) for o in fake.ops if o.op == "r" and o.addr != 0x3B] == [(0x20, 8)]


# ── streaming ────────────────────────────────────────────────────────────────


def test_start_stream_wire_bytes_and_state(fake, dev, vectors):
    assert not dev.streaming
    dev.start_stream()
    dev.start_stream(20, QUAT_BLOCK)
    dev.start_stream(1000, (0x35, 1), TRIGGER_INT)
    assert [p.hex() for p in fake.streams] == [
        vec(vectors, "encode", n)["payload"]
        for n in ("start_stream_full_block_100hz", "start_stream_quaternion_50hz",
                  "start_stream_int_status_watchdog_1s")
    ]
    assert dev.streaming and fake.streaming


def test_start_stream_refuses_bad_blocks_before_traffic(fake, dev):
    for block in ((0x08, 0), (0x08, 129), (0xF9, 8)):
        with pytest.raises(ValueError, match="outside"):
            dev.start_stream(10, block)
    assert fake.sent == []
    assert not dev.streaming


def test_start_stream_refuses_a_timer_period_outside_1_60000_before_traffic(fake, dev):
    """contract 13 §2 (ERRATA A13): TIMER period 1..60000 ms; INT takes 0 (no watchdog)."""
    for period_ms in (0, 60001, 65536):
        with pytest.raises(ValueError, match="period_ms"):
            dev.start_stream(period_ms)
    assert fake.sent == []
    assert not dev.streaming


def test_stream_scales_by_the_units_latched_before_start_stream(fake, dev):
    fake.regs[0][0x3B] = 0x97
    dev.start_stream()
    # UNIT_SEL was read before START_STREAM went out
    assert [c for c, _ in fake.sent] == [0x32, 0x35]
    got: list[Bno055Sample] = []
    dev.on_sample(got.append)
    dev.set_units(Units())  # later units do not apply to the running stream
    fake.push_stream(0x08, bytes.fromhex("a2037fff73ff"), 1)
    wait_for(lambda: len(got) == 1)
    assert got[0].units == ALL_UNITS
    assert got[0].accel == pytest.approx((930.0, -129.0, -141.0))


def test_failed_start_stream_leaves_the_stream_off(fake, dev):
    fake.fail_next[0x35] = [0x08]
    with pytest.raises(StatusError):
        dev.start_stream()
    assert not dev.streaming


def test_stop_stream(fake, dev):
    dev.stop_stream()  # idle: nothing sent
    assert fake.sent == []
    dev.start_stream()
    dev.stop_stream()
    assert fake.sent[-1] == (0x36, b"") and not dev.streaming and not fake.streaming
    dev.start_stream()
    fake.fail_next[0x36] = [0x01]
    with pytest.raises(StatusError):
        dev.stop_stream()
    assert not dev.streaming  # the flag is cleared even on failure


def test_callbacks_unsubscribe_iterator_and_drop_oldest(fake, dev):
    dev.start_stream(20, QUAT_BLOCK)
    got: list[Bno055Sample] = []
    off = dev.on_sample(got.append)
    it = dev.samples(2)
    for i in range(5):
        fake.push_stream(0x20, LIVE_QUAT, i)
    wait_for(lambda: len(got) == 5)
    assert got[0].quaternion is not None and got[0].accel is None
    assert dev.stream_dropped_counts == [3] and it.dropped_count == 3
    assert next(it).timestamp_us == 3
    off()
    it.close()
    assert dev.stream_dropped_counts == []
    fake.push_stream(0x20, LIVE_QUAT)
    time.sleep(0.05)
    assert len(got) == 5


def test_samples_iterator_ends_when_the_device_closes(fake, dev):
    it = dev.samples()
    fake.push_stream(0x20, LIVE_QUAT, 9)
    assert next(it).timestamp_us == 9
    dev.close()
    assert list(it) == []


def test_get_sample_next_timeout_and_close(fake, dev):
    dev.start_stream()
    result: list[Bno055Sample] = []
    t = threading.Thread(target=lambda: result.append(dev.get_sample(1.0)))
    t.start()
    wait_for(lambda: dev.stream_dropped_counts == [0])
    fake.push_stream(0x34, bytes.fromhex("1a00"), 77)
    t.join(1.0)
    assert result[0].timestamp_us == 77 and result[0].temperature == 26
    assert dev.stream_dropped_counts == []  # its one-shot queue is gone

    with pytest.raises(DepzTimeoutError):
        dev.get_sample(0.05)

    errors: list[Exception] = []

    def pending():
        try:
            dev.get_sample(5.0)
        except Exception as e:  # noqa: BLE001
            errors.append(e)

    t = threading.Thread(target=pending)
    t.start()
    wait_for(lambda: dev.stream_dropped_counts == [0])
    dev.close()
    t.join(1.0)
    assert isinstance(errors[0], LinkClosedError) and "device closed" in str(errors[0])
    with pytest.raises(LinkClosedError, match="device is closed"):
        dev.get_sample(0.1)


def test_malformed_stream_reports_are_counted_and_dropped(fake, dev):
    dev.start_stream()
    got: list[Bno055Sample] = []
    dev.on_sample(got.append)
    fake.push_raw(0x93, bytes(9))  # shorter than the 10-byte header
    fake.push_stream(0x08, bytes.fromhex("a2037fff73ff"), 1, len_field=46)  # truncated block
    wait_for(lambda: dev.stream_parse_errors == 2)
    assert got == []
    fake.push_stream(0x08, bytes.fromhex("a2037fff73ff"), 2)
    wait_for(lambda: len(got) == 1)
    assert got[0].accel == pytest.approx((9.3, -1.29, -1.41))
    assert dev.read_register(0x00) == 0xA0  # the link keeps working


def test_other_reports_are_not_swallowed_as_stream_data(fake, dev):
    texts: list[str] = []
    dev.on_event(lambda e: texts.append(e.text) if isinstance(e, TextEvent) else None)
    fake.push_raw(0x9E, bytes([1, 2]))
    wait_for(lambda: texts == ["0102"])
    assert dev.stream_parse_errors == 0
