"""Bno086 device behavior against the in-process fake bridge + sensor hub."""

import math
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


def test_product_ids_collects_every_component(rig):
    fake, dev = rig
    ids = dev.product_ids()
    assert [(p.sw_part_number, p.version) for p in ids] == [
        (10003608, "3.8.4"), (10003606, "1.2.4"), (10003254, "4.4.3"), (10003171, "4.2.10")]
    assert ids[0].reset_cause == 0x01 and all(p.reset_cause == 0 for p in ids[1:])


def test_back_to_back_product_id_never_gets_a_late_component(rig):
    """The three late responses arrive in a second cargo; taking the first
    and returning let the next call pick up the bootloader's (seen live: 9
    of 20 calls on a BNO085)."""
    fake, dev = rig
    assert {dev.product_id().sw_part_number for _ in range(5)} == {10003608}


def test_split_control_cargo():
    from depz_sensor_sdk.bno086.sh2 import split_control_cargo

    pid = bytes([0xF8]) + bytes(15)
    feat = bytes([0xFC]) + bytes(16)
    assert split_control_cargo(pid + feat + pid) == [pid, feat, pid]
    # An unknown ID ends the walk: its length is unknowable.
    assert split_control_cargo(pid + b"\x99\x01\x02") == [pid, b"\x99\x01\x02"]
    assert split_control_cargo(pid[:10]) == [pid[:10]]
    assert split_control_cargo(b"") == []


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


def test_set_calibration_planar_sets_p4(rig):
    """Planar accelerometer calibration (SH-2 §6.4.6): P4 = 1 of the ME
    Calibration command; accel on, gyro and mag off, P3 = 0 (configure)."""
    fake, dev = rig
    dev.set_calibration(accel=True, gyro=False, mag=False, planar=True)
    assert fake.me_cal_requests[-1][:5] == bytes((1, 0, 0, 0, 1))


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


def test_frs_dcd_backup_restore_delete(rig):
    """Saved calibration (FRS 0x1F1F): back up = read the record, restore =
    write the words back, delete = a 0-word write (SH-2 §6.3.3) — after which
    the record reads as unrecognized."""
    fake, dev = rig
    backup = dev.frs_read(FrsRecordId.DYNAMIC_CALIBRATION)
    assert backup == tuple(FakeBno086.FRS_RECORDS[0x1F1F])
    dev.frs_write(FrsRecordId.DYNAMIC_CALIBRATION, backup)
    assert fake.frs_writes[FrsRecordId.DYNAMIC_CALIBRATION] == list(backup)
    dev.frs_write(FrsRecordId.DYNAMIC_CALIBRATION, [])
    assert FrsRecordId.DYNAMIC_CALIBRATION in fake.frs_erased
    with pytest.raises(Sh2Error, match="UNRECOGNIZED"):
        dev.frs_read(FrsRecordId.DYNAMIC_CALIBRATION)


def test_frs_user_record_64_words_reads_back(rig):
    """User record (FRS 0x74B4, SH-2 §4.3.21): the 64-word maximum goes out
    in 32 Write Data packets and reads back word for word."""
    _, dev = rig
    words = [(0xA5000000 | i) for i in range(64)]
    dev.frs_write(FrsRecordId.USER_RECORD, words)
    assert dev.frs_read(FrsRecordId.USER_RECORD) == tuple(words)


def test_frs_serial_number_single_word(rig):
    _, dev = rig
    assert dev.frs_read(FrsRecordId.SERIAL_NUMBER) == (0x00086A5C,)


def test_frs_detector_config_records_write_then_read(rig):
    """Detector tuning records (SH-2 §4.3.6/7/15, §4.3.3, §4.3.5, §4.3.24):
    each is a plain word list through frs_write / frs_read."""
    _, dev = rig
    records = {
        FrsRecordId.SIG_MOTION_DETECT_CONFIG: [round(12.0 * 2**24), 3],
        FrsRecordId.SHAKE_DETECT_CONFIG: [50_000, 400_000, round(8.0 * 2**26), 4, 0x03],
        FrsRecordId.STABILITY_DETECTOR_CONFIG: [round(0.784 * 2**24), 500_000],
        FrsRecordId.ME_POWER_MGMT: [round(0.2 * 2**28), 1 << 25, (50 << 16) | 3],
        FrsRecordId.ARVR_STABILIZATION_GRV: [round(0.2 * 2**30), round(0.127 * 2**29), round(0.785 * 2**29), 0],
        FrsRecordId.GYRO_INTEGRATED_RV_CONFIG: [0x0207, 10_000, round(math.pi / 6 * 2**29), round(0.028 * 2**10),
                                                round(0.303072543909142 * 2**20), round(0.113295896384921 * 2**20),
                                                round(0.002776219713054 * 2**20)],
    }
    for record_id, words in records.items():
        dev.frs_write(record_id, words)
        assert dev.frs_read(record_id) == tuple(words), record_id


def test_get_metadata(rig):
    fake, dev = rig
    md = dev.get_metadata(SensorId.ACCELEROMETER)
    assert md.revision == 4
    assert md.power_ma_q10 == 0x0800  # 2 mA
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
