"""Bno086 device behavior against the in-process fake bridge + sensor hub."""

import struct
import time

import pytest

from depz_sensor_sdk import BusyError, Bno086, SensorId
from depz_sensor_sdk.bno086 import (
    Acceleration,
    CalibrationConfig,
    GyroIntegratedRV,
    RotationVector,
    TareAxis,
    TareBasis,
)
from depz_sensor_sdk.bno086.reports import Magnetometer, StepCounter
from depz_sensor_sdk.bno086.sh2 import FrsRecordId, Sh2Error, build_set_feature

from fake_bno086 import FakeBno086


@pytest.fixture()
def rig():
    fake = FakeBno086()
    dev = Bno086(fake.link, timeout=1.0)
    yield fake, dev
    dev.close()
    fake.close()


def _report(rid: int, body: bytes, *, seq: int = 0, accuracy: int = 0, delay: int = 0) -> bytes:
    status = (accuracy & 0x03) | ((delay >> 8) << 2)
    return bytes((rid, seq, status, delay & 0xFF)) + body


def _timebase(delta_100us: int) -> bytes:
    return bytes([0xFB]) + struct.pack("<i", delta_100us)


# ── identity / lifecycle ──────────────────────────────────────────────────────


def test_identity_and_product_id(rig):
    fake, dev = rig
    assert dev.get_software_name() == FakeBno086.SOFTWARE_NAME
    pid = dev.product_id()
    assert (pid.sw_version_major, pid.sw_version_minor, pid.sw_version_patch) == (3, 8, 4)
    assert pid.version == "3.8.4"


def test_hardware_reset_waits_for_reset_complete(rig):
    fake, dev = rig
    dev.hardware_reset(timeout=1.0)
    assert dev.advertisement == FakeBno086.ADVERTISEMENT
    # SHTP TX seq restarted from 0 after reset
    assert dev._shtp.tx_seq(2) == 0


def test_wake(rig):
    _, dev = rig
    dev.wake()  # RPT_STATUS OK — must not raise


# ── enable / feature flow ─────────────────────────────────────────────────────


def test_enable_sends_set_feature_bytes_and_verifies(rig):
    fake, dev = rig
    resp = dev.enable(SensorId.ROTATION_VECTOR, hz=100)
    assert fake.last_set_feature == build_set_feature(SensorId.ROTATION_VECTOR, 10_000)
    assert resp is not None and resp.interval_us == 10_000
    assert fake.features[SensorId.ROTATION_VECTOR] == 10_000


def test_enable_warns_when_granted_rate_out_of_band(rig):
    fake, dev = rig
    fake.interval_factor = 4.0  # granted rate = requested/4 < 0.9x
    with pytest.warns(UserWarning, match="granted"):
        dev.enable(SensorId.ACCELEROMETER, hz=100)


def test_enable_grid_rounding_within_band_no_warning(rig):
    fake, dev = rig
    fake.interval_factor = 0.8  # granted 125 Hz for a 100 Hz ask — in band
    import warnings as _w

    with _w.catch_warnings():
        _w.simplefilter("error")
        dev.enable(SensorId.GYROSCOPE, hz=100)


def test_disable_clears_feature(rig):
    fake, dev = rig
    dev.enable(SensorId.MAGNETOMETER, hz=50)
    dev.disable(SensorId.MAGNETOMETER)
    deadline = time.monotonic() + 1.0
    while SensorId.MAGNETOMETER in fake.features and time.monotonic() < deadline:
        time.sleep(0.005)
    assert SensorId.MAGNETOMETER not in fake.features


def test_enable_argument_validation(rig):
    _, dev = rig
    with pytest.raises(ValueError):
        dev.enable(SensorId.ACCELEROMETER)
    with pytest.raises(ValueError):
        dev.enable(SensorId.ACCELEROMETER, hz=100, interval_us=10_000)


# ── busy / backoff ────────────────────────────────────────────────────────────


def test_busy_then_retry_succeeds(rig):
    fake, dev = rig
    fake.busy_remaining = 1
    t0 = time.monotonic()
    dev.enable(SensorId.ACCELEROMETER, hz=100, verify=False)
    assert time.monotonic() - t0 >= dev.busy_backoff_s  # backed off >= 200 ms
    assert fake.send_shtp_attempts == 2
    assert fake.features[SensorId.ACCELEROMETER] == 10_000


def test_busy_exhausts_retries(rig):
    fake, dev = rig
    fake.busy_remaining = 99
    dev.busy_retries = 3
    dev.busy_backoff_s = 0.02  # keep the test fast
    with pytest.raises(BusyError):
        dev.enable(SensorId.ACCELEROMETER, hz=100, verify=False)
    assert fake.send_shtp_attempts == 3


# ── report streaming through the full stack ───────────────────────────────────


def test_input_reports_parse_through_stack(rig):
    fake, dev = rig
    it = dev.reports(maxsize=16)
    cargo = (
        _timebase(120)
        + _report(0x01, struct.pack("<3h", 256, -512, 2521), seq=7, accuracy=2)
        + _report(0x05, struct.pack("<5h", 100, -200, 300, 16000, 50), seq=8, accuracy=3, delay=17)
    )
    capture = fake.push_input_cargo(cargo)
    acc = next(it)
    rv = next(it)
    assert isinstance(acc, Acceleration)
    assert acc.sensor_id == SensorId.ACCELEROMETER
    assert (acc.x_raw, acc.y_raw, acc.z_raw) == (256, -512, 2521)
    assert acc.x == pytest.approx(1.0)  # 256 / 2^8 m/s²
    assert acc.z == pytest.approx(2521 / 256)
    assert acc.accuracy == 2 and acc.seq == 7
    assert acc.timestamp_us == capture - 120 * 100  # timebase applied
    assert isinstance(rv, RotationVector)
    assert rv.real == pytest.approx(16000 / 16384)
    assert rv.accuracy_rad == pytest.approx(50 / 4096)
    assert rv.timestamp_us == capture - 120 * 100 + 17 * 100  # + report delay


def test_multi_frame_cargo_reassembly_through_stack(rig):
    fake, dev = rig
    it = dev.reports(maxsize=16)
    reports = _timebase(0) + b"".join(
        _report(0x03, struct.pack("<3h", 160 + i, -160, 42), seq=i) for i in range(8)
    )
    fake.push_input_cargo(reports, max_frame=32)  # forces 3+ SHTP fragments
    got = [next(it) for _ in range(8)]
    assert all(isinstance(m, Magnetometer) for m in got)
    assert [m.x_raw for m in got] == [160 + i for i in range(8)]
    assert got[0].x == pytest.approx(10.0)  # 160 / 2^4 µT


def test_report_filtering_and_callbacks(rig):
    fake, dev = rig
    only_steps = dev.reports(sensors=SensorId.STEP_COUNTER, maxsize=8)
    seen: list = []
    unsub = dev.on_report(seen.append, sensors=[SensorId.STEP_COUNTER])
    cargo = (
        _timebase(0)
        + _report(0x01, struct.pack("<3h", 1, 2, 3))
        + _report(0x11, struct.pack("<IH", 5000, 1234) + b"\x00\x00")
    )
    fake.push_input_cargo(cargo)
    step = next(only_steps)
    assert isinstance(step, StepCounter)
    assert step.steps == 1234 and step.latency_us == 5000
    assert len(seen) == 1 and seen[0].steps == 1234
    unsub()


def test_gyro_integrated_rv_channel5(rig):
    fake, dev = rig
    it = dev.reports(sensors=SensorId.GYRO_INTEGRATED_RV, maxsize=8)
    capture = fake.push_gyro_rv_cargo(struct.pack("<7h", 1, 2, 3, 16384, 512, -512, 1024))
    r = next(it)
    assert isinstance(r, GyroIntegratedRV)
    assert r.real == pytest.approx(1.0)
    assert r.angular_velocity == pytest.approx((0.5, -0.5, 1.0))  # Q10 rad/s
    assert r.timestamp_us == capture


# ── tare / calibration ────────────────────────────────────────────────────────


def test_tare_and_calibration_facade(rig):
    fake, dev = rig
    dev.tare_now(axes=TareAxis.ALL, basis=TareBasis.GAME_ROTATION_VECTOR)
    dev.save_dcd()  # waits for command response, status 0
    deadline = time.monotonic() + 1.0
    while not fake.tare_requests and time.monotonic() < deadline:
        time.sleep(0.005)
    (tare,) = fake.tare_requests
    assert tare[2] == 0x03  # TARE command
    assert tare[3:6] == bytes((0x00, TareAxis.ALL, TareBasis.GAME_ROTATION_VECTOR))
    dev.set_calibration(accel=True, gyro=True, mag=False)
    cal = dev.get_calibration()
    assert cal == CalibrationConfig(accel=True, gyro=True, mag=False, planar=False)


# ── FRS ───────────────────────────────────────────────────────────────────────


def test_frs_read_multi_packet(rig):
    fake, dev = rig
    words = dev.frs_read(FrsRecordId.SYSTEM_ORIENTATION)
    assert words == tuple(FakeBno086.FRS_RECORDS[0x2D3E])


def test_frs_read_unknown_record_raises(rig):
    _, dev = rig
    with pytest.raises(Sh2Error, match="UNRECOGNIZED"):
        dev.frs_read(0xBEEF)


def test_frs_write_roundtrip(rig):
    fake, dev = rig
    payload = [0x11111111, 0x22222222, 0x33333333]
    dev.frs_write(FrsRecordId.SYSTEM_ORIENTATION, payload)
    assert fake.frs_writes[FrsRecordId.SYSTEM_ORIENTATION] == payload


def test_get_metadata(rig):
    fake, dev = rig
    md = dev.get_metadata(SensorId.ACCELEROMETER)
    assert md.revision == 4
    assert md.min_period_us == 2500
    assert md.max_period_us == 100000
    assert md.q_point_1 == 8
    assert md.raw_words == tuple(FakeBno086.FRS_RECORDS[0xE302])


# ── diagnostics / housekeeping commands (added coverage) ──────────────────────


def test_get_oscillator_type(rig):
    from depz_sensor_sdk.bno086 import OscillatorType

    _, dev = rig
    assert dev.get_oscillator_type() == OscillatorType.EXT_CRYSTAL


def test_clear_dcd_and_reset(rig):
    fake, dev = rig
    dev.clear_dcd_and_reset(timeout=1.0)
    assert dev.advertisement == FakeBno086.ADVERTISEMENT
    assert dev._shtp.tx_seq(2) == 0


def test_get_errors(rig):
    from depz_sensor_sdk.bno086 import ErrorSource

    _, dev = rig
    errs = dev.get_errors()
    assert len(errs) == 2  # terminator (source 255) is not a record
    assert errs[0].severity == 1
    assert errs[0].source == ErrorSource.SENSOR_HUB
    assert errs[0].error == 0x10
    assert errs[1].source == ErrorSource.CHIP
    assert errs[1].code == 6


def test_get_counts(rig):
    _, dev = rig
    counts = dev.get_counts(SensorId.ROTATION_VECTOR)
    assert counts.offered == 100
    assert counts.accepted == 90
    assert counts.on == 80
    assert counts.attempted == 70


def test_clear_counts(rig):
    _, dev = rig
    dev.clear_counts(SensorId.ROTATION_VECTOR)  # status 0 -> no raise
