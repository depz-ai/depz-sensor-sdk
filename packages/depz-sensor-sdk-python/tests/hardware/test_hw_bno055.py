"""BNO055 real-hardware tests (contract 13). The board may lie still anywhere:
nothing here depends on orientation or calibration state."""

import math
import time

import pytest

from _support import port_lease
from depz_sensor_sdk import Bno055, DepzError
from depz_sensor_sdk.bno055 import AxisRemap, CalibrationProfile, OprMode, Units
from depz_sensor_sdk.bno055.regs import (
    FULL_BLOCK,
    QUAT_BLOCK,
    REG_AXIS_MAP_CONFIG,
    REG_CALIB_PROFILE,
    REG_UNIT_SEL,
    decode_block,
)

pytestmark = [pytest.mark.hardware, pytest.mark.hardware_bno055, pytest.mark.timeout(120)]


@pytest.fixture
def bno055(bno055_info):
    with port_lease(bno055_info.resolve_port()):
        dev = Bno055(bno055_info.resolve_port())
        try:
            yield dev
        finally:
            try:
                dev.stop_stream()
            except Exception:
                pass
            dev.close()


def _norm(v):
    return math.sqrt(sum(x * x for x in v))


def test_identity(bno055):
    info = bno055.bridge_info()
    assert info.ids_ok
    assert info.initialized == 1
    assert info.i2c_addr == 0x28
    assert bno055.is_alive()
    assert len(bno055.unique_id()) == 16


def test_configure_ndof_fusion_is_live(bno055):
    bno055.configure(OprMode.NDOF, Units())
    assert bno055.get_operation_mode() == OprMode.NDOF
    st = bno055.system_status()
    assert st.status_text == "fusion algorithm running"
    assert st.error == 0
    s = bno055.read_sample()
    assert abs(_norm(s.quaternion) - 1.0) < 0.02
    assert 9.0 < _norm(s.gravity) < 10.6  # m/s², regardless of unit bits
    assert s.calibration is not None


def test_stream_100hz_with_commands_in_flight(bno055):
    bno055.configure()
    it = bno055.samples()
    bno055.start_stream(10)
    try:
        got = []
        t0 = time.monotonic()
        while time.monotonic() - t0 < 2.0:
            got.append(next(it))
            if len(got) % 20 == 0:
                bno055.calibration_status()  # a command against the live stream
        with pytest.raises(DepzError):
            bno055.unique_id()  # page 1 is refused while streaming
    finally:
        bno055.stop_stream()
        it.close()
    ts = [g.timestamp_us for g in got]
    hz = (len(ts) - 1) / ((ts[-1] - ts[0]) / 1e6)
    assert 97 < hz < 103
    assert max(b - a for a, b in zip(ts, ts[1:])) < 15_000
    assert all(abs(_norm(g.quaternion) - 1.0) < 0.02 for g in got)
    assert bno055.stream_parse_errors == 0
    assert bno055.bridge_info().slots_skipped == 0


def test_unit_bits_as_measured(bno055):
    """Contract 13 §4.2: bit 0 → ACC_DATA in mg, LIA/GRV stay m/s²; bit 2 →
    radians; bit 4 → °F at 2 °F/LSB."""
    bno055.configure(OprMode.NDOF, Units())
    si = bno055.read_sample()
    bno055.configure(OprMode.NDOF, Units(accel_mg=True, euler_rad=True, temp_f=True))
    alt = bno055.read_sample()
    assert _norm(alt.accel) == pytest.approx(_norm(si.accel) * 1000 / 9.80665, rel=0.05)
    assert _norm(alt.gravity) == pytest.approx(_norm(si.gravity), rel=0.05)
    assert alt.euler[1] == pytest.approx(math.radians(si.euler[1]), abs=0.05)
    assert alt.temperature == pytest.approx(si.temperature * 9 / 5 + 32, abs=3)
    bno055.configure()


def test_quaternion_block_stream(bno055):
    bno055.configure(OprMode.IMU)
    bno055.start_stream(20, QUAT_BLOCK)
    try:
        s = bno055.get_sample(timeout=1.0)
    finally:
        bno055.stop_stream()
    assert s.addr == QUAT_BLOCK[0] and len(s.raw) == QUAT_BLOCK[1]
    assert s.accel is None and s.calibration is None
    assert abs(_norm(s.quaternion) - 1.0) < 0.02


def test_calibration_profile_write_lands(bno055):
    bno055.configure()
    saved = bno055.read_calibration_profile()
    probe = CalibrationProfile((1, -2, 3), (10, -20, 30), (-1, 2, -3), 1000, 500)
    try:
        bno055.set_operation_mode(OprMode.CONFIG)
        bno055.write_registers(REG_CALIB_PROFILE, probe.pack())
        # Read back without leaving CONFIG: fusion refines the profile at once.
        got = CalibrationProfile.unpack(bno055.read_registers(REG_CALIB_PROFILE, 22))
        assert got == probe
    finally:
        bno055.write_calibration_profile(saved)
        bno055.set_operation_mode(OprMode.NDOF)


def test_axis_remap_and_self_test(bno055):
    bno055.configure()
    try:
        bno055.set_axis_remap("P0")
        assert bno055.read_registers(REG_AXIS_MAP_CONFIG, 2) == b"\x21\x04"
        assert bno055.get_axis_remap() == AxisRemap.placement("P0")
    finally:
        bno055.set_axis_remap("P1")
    st = bno055.self_test()
    assert st.self_test_passed and st.error == 0
    assert bno055.get_operation_mode() == OprMode.NDOF


def test_self_test_in_config_leaves_the_sensor_configurable(bno055):
    # Measured on SW 03.11: a self-test run in CONFIG left SYS_STATUS at 4
    # ("executing self-test") until the mode left CONFIG, and the next
    # configure() timed out waiting for boot.
    bno055.reset_sensor()
    assert bno055.get_operation_mode() == OprMode.CONFIG
    st = bno055.self_test()
    assert st.self_test_passed
    assert bno055.system_status().status == 0
    bno055.configure(OprMode.NDOF)
    assert bno055.system_status().status == 5


def test_configure_clears_a_leftover_self_test_status(bno055):
    # A sensor left at SYS_STATUS 4 by someone else's self-test in CONFIG.
    from depz_sensor_sdk.bno055.regs import REG_SYS_TRIGGER

    bno055.reset_sensor()
    bno055.write_registers(REG_SYS_TRIGGER, b"\x01")
    time.sleep(0.5)
    assert bno055.system_status().status == 4
    bno055.configure(OprMode.NDOF)
    assert bno055.system_status().status == 5


def test_reset_then_restore(bno055):
    bno055.configure(OprMode.NDOF, Units(accel_mg=True))
    bno055.reset_sensor()
    assert bno055.get_operation_mode() == OprMode.CONFIG
    assert bno055.read_register(REG_UNIT_SEL) == 0x80  # power-on value
    bno055.restore_configuration()
    assert bno055.get_operation_mode() == OprMode.NDOF
    assert bno055.get_units() == Units(accel_mg=True)
    raw = decode_block(FULL_BLOCK[0], bno055.read_registers(*FULL_BLOCK))
    assert any(raw.quaternion)
    bno055.configure()


def test_mode_to_mode_goes_through_config(bno055):
    """The sensor ignores OPR_MODE written from one operating mode straight to
    another (contract 13 §4.1); set_operation_mode() routes via CONFIG."""
    from depz_sensor_sdk.bno055 import AccelConfig

    bno055.configure(OprMode.NDOF)
    try:
        bno055.set_operation_mode(OprMode.AMG)
        assert bno055.get_operation_mode() == OprMode.AMG
        bno055.set_accel_config(AccelConfig(range=2))
        assert bno055.get_accel_config().range == 2  # not overridden by fusion
        bno055.set_operation_mode(OprMode.IMU)
        assert bno055.get_operation_mode() == OprMode.IMU
        assert any(bno055.read_quaternion())
    finally:
        bno055.set_operation_mode(OprMode.CONFIG)
        bno055.set_accel_config(AccelConfig())
        bno055.configure()

