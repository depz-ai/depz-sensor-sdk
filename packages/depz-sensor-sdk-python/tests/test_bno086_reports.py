"""BNO086 SH-2 input-report parsers (reports.py) — every report class, Q-point
scaling, timebase/rebase timestamp math, and the SHTP reassembly state machine
(shtp.py). All pure codecs, no device."""

from __future__ import annotations

import struct

import pytest

from depz_sensor_sdk.bno086.reports import (
    Acceleration,
    GenericEvent,
    GyroIntegratedRV,
    Gyroscope,
    Magnetometer,
    PersonalActivityClassifier,
    RawSensor,
    RotationVector,
    ScalarReport,
    SensorId,
    ShakeDetector,
    SignificantMotion,
    StabilityClassifier,
    StepCounter,
    StepDetector,
    TapDetector,
    UncalibratedGyroscope,
    UncalibratedMagnetometer,
    UnknownReport,
    parse_gyro_rv_cargo,
    parse_input_cargo,
)
from depz_sensor_sdk.bno086.shtp import (
    ShtpChannel,
    ShtpLayer,
    fragment_cargo,
)


def _report(rid: int, body: bytes, *, seq: int = 0, accuracy: int = 0, delay: int = 0) -> bytes:
    status = (accuracy & 0x03) | ((delay >> 8) << 2)
    return bytes((rid, seq, status, delay & 0xFF)) + body


def _timebase(delta_100us: int) -> bytes:
    return bytes([0xFB]) + struct.pack("<i", delta_100us)


def _one(cargo: bytes, capture: int = 10_000):
    reps = parse_input_cargo(cargo, capture)
    assert len(reps) == 1
    return reps[0]


CAP = 100_000


# ── vector reports (Q-point scaling) ──────────────────────────────────────────


def test_accelerometer_q8():
    r = _one(_timebase(0) + _report(0x01, struct.pack("<3h", 256, -256, 512)))
    assert isinstance(r, Acceleration) and r.sensor_id == SensorId.ACCELEROMETER
    assert (r.x, r.y, r.z) == (1.0, -1.0, 2.0)  # /2^8 m/s²


def test_gyroscope_q9():
    r = _one(_timebase(0) + _report(0x02, struct.pack("<3h", 512, 0, -512)))
    assert isinstance(r, Gyroscope)
    assert (r.x, r.y, r.z) == (1.0, 0.0, -1.0)  # /2^9 rad/s


def test_magnetometer_q4():
    r = _one(_timebase(0) + _report(0x03, struct.pack("<3h", 16, 32, -16)))
    assert isinstance(r, Magnetometer)
    assert (r.x, r.y, r.z) == (1.0, 2.0, -1.0)  # /2^4 µT


def test_linear_accel_and_gravity_are_acceleration():
    lin = _one(_timebase(0) + _report(0x04, struct.pack("<3h", 256, 0, 0)))
    grav = _one(_timebase(0) + _report(0x06, struct.pack("<3h", 0, 0, 256)))
    assert isinstance(lin, Acceleration) and lin.sensor_id == SensorId.LINEAR_ACCELERATION
    assert isinstance(grav, Acceleration) and grav.z == 1.0


def test_uncalibrated_gyroscope_with_bias():
    r = _one(_timebase(0) + _report(0x07, struct.pack("<6h", 512, 0, 0, 512, 512, 0)))
    assert isinstance(r, UncalibratedGyroscope)
    assert r.x == 1.0
    assert r.bias == pytest.approx((1.0, 1.0, 0.0))


def test_uncalibrated_magnetometer_with_bias():
    r = _one(_timebase(0) + _report(0x0F, struct.pack("<6h", 16, 0, 0, 16, 0, 32)))
    assert isinstance(r, UncalibratedMagnetometer)
    assert r.x == 1.0 and r.bias == pytest.approx((1.0, 0.0, 2.0))


# ── rotation vectors ──────────────────────────────────────────────────────────


def test_rotation_vector_has_accuracy():
    r = _one(_timebase(0) + _report(0x05, struct.pack("<5h", 0, 0, 0, 16384, 2048)))
    assert isinstance(r, RotationVector)
    assert r.real == 1.0  # 16384 / 2^14
    assert r.accuracy_rad == pytest.approx(0.5)  # 2048 / 2^12


def test_game_rotation_vector_has_no_accuracy():
    r = _one(_timebase(0) + _report(0x08, struct.pack("<4h", 0, 0, 0, 16384)))
    assert isinstance(r, RotationVector)
    assert r.accuracy_raw is None and r.accuracy_rad is None


def test_geomagnetic_and_arvr_rotation_vectors():
    geo = _one(_timebase(0) + _report(0x09, struct.pack("<5h", 0, 0, 0, 16384, 0)))
    arvr = _one(_timebase(0) + _report(0x28, struct.pack("<5h", 0, 0, 0, 16384, 0)))
    arvr_game = _one(_timebase(0) + _report(0x29, struct.pack("<4h", 0, 0, 0, 16384)))
    assert geo.accuracy_rad == 0.0
    assert arvr.accuracy_rad == 0.0
    assert arvr_game.accuracy_raw is None


# ── scalar / detector reports ─────────────────────────────────────────────────


def test_scalar_reports_pressure_and_temperature():
    press = _one(_timebase(0) + _report(0x0A, struct.pack("<I", 1 << 20)))
    temp = _one(_timebase(0) + _report(0x0E, struct.pack("<h", 128)))
    assert isinstance(press, ScalarReport) and press.value == 1.0  # Q20 hPa
    assert isinstance(temp, ScalarReport) and temp.value == 1.0  # 128/2^7 °C


def test_tap_detector_double_tap_bit():
    r = _one(_timebase(0) + _report(0x10, bytes([0x40])))
    assert isinstance(r, TapDetector) and r.double_tap is True


def test_step_counter_layout():
    # latency u32 at bytes 4-7, steps u16 at bytes 8-9 (vendor comment is wrong)
    r = _one(_timebase(0) + _report(0x11, struct.pack("<IH", 5000, 1234) + b"\x00\x00"))
    assert isinstance(r, StepCounter) and r.latency_us == 5000 and r.steps == 1234


def test_step_detector_latency():
    r = _one(_timebase(0) + _report(0x18, struct.pack("<I", 777)))
    assert isinstance(r, StepDetector) and r.latency_us == 777


def test_significant_motion_and_stability():
    sig = _one(_timebase(0) + _report(0x12, struct.pack("<H", 1)))
    stab = _one(_timebase(0) + _report(0x13, bytes([2, 0])))
    assert isinstance(sig, SignificantMotion) and sig.motion == 1
    assert isinstance(stab, StabilityClassifier) and stab.name == "stationary"


def test_shake_detector():
    r = _one(_timebase(0) + _report(0x19, struct.pack("<H", 0b010)))
    assert isinstance(r, ShakeDetector) and r.flags == 0b010


def test_personal_activity_classifier():
    body = bytes([0x80 | 3, 6]) + bytes(range(10))  # page 3, EOS set, most-likely 6
    r = _one(_timebase(0) + _report(0x1E, body))
    assert isinstance(r, PersonalActivityClassifier)
    assert r.page_number == 3 and r.end_of_sequence is True
    assert r.most_likely_state == 6 and r.most_likely_name == "walking"
    assert r.confidences == tuple(range(10))


def test_generic_event_detectors():
    r = _one(_timebase(0) + _report(0x20, struct.pack("<H", 5)))  # tilt detector
    assert isinstance(r, GenericEvent) and r.value_raw == 5


def test_raw_accelerometer_and_gyro_temperature():
    acc = _one(_timebase(0) + _report(0x14, struct.pack("<3hHI", 1, 2, 3, 0, 999)))
    gyr = _one(_timebase(0) + _report(0x15, struct.pack("<4hI", 1, 2, 3, 42, 999)))
    assert isinstance(acc, RawSensor) and acc.sensor_timestamp_us == 999
    assert acc.temperature_raw == 0
    assert isinstance(gyr, RawSensor) and gyr.temperature_raw == 42


def test_unknown_report_stops_parsing():
    r = _one(_timebase(0) + _report(0x7E, b"\x01\x02\x03\x04"))
    assert isinstance(r, UnknownReport) and r.data[0] == 0x7E


# ── timestamp math: base delta + per-report delay + rebase ───────────────────


def test_timebase_and_delay_applied():
    cargo = _timebase(120) + _report(0x05, struct.pack("<5h", 0, 0, 0, 16384, 0), delay=17)
    r = _one(cargo, capture=CAP)
    # ts = capture - base_delta*100 + delay*100
    assert r.timestamp_us == CAP - 120 * 100 + 17 * 100


def test_rebase_shifts_base_forward():
    cargo = (
        _timebase(0)
        + _report(0x01, struct.pack("<3h", 0, 0, 0))
        + bytes([0xFA]) + struct.pack("<i", 50)  # rebase +50 ticks
        + _report(0x01, struct.pack("<3h", 0, 0, 0))
    )
    reps = parse_input_cargo(cargo, CAP)
    assert len(reps) == 2
    assert reps[1].timestamp_us - reps[0].timestamp_us == 50 * 100


# ── channel-5 gyro-integrated RV: both wire shapes ────────────────────────────


def test_gyro_rv_bare_shape():
    r = parse_gyro_rv_cargo(struct.pack("<7h", 0, 0, 0, 16384, 1024, -1024, 512), 5_000)
    assert isinstance(r, GyroIntegratedRV)
    assert r.real == 1.0
    assert r.angular_velocity == pytest.approx((1.0, -1.0, 0.5))  # Q10 rad/s
    assert r.timestamp_us == 5_000


def test_gyro_rv_timebase_prefixed_shape():
    payload = _timebase(30) + struct.pack("<H", 7) + struct.pack("<7h", 0, 0, 0, 16384, 0, 0, 0)
    r = parse_gyro_rv_cargo(payload, 8_000)
    assert r is not None
    assert r.timestamp_us == 8_000 - 30 * 100 + 7 * 100


def test_gyro_rv_truncated_returns_none():
    assert parse_gyro_rv_cargo(b"\x01\x02", 0) is None


# ── SHTP reassembly state machine (shtp.py) ───────────────────────────────────


def test_shtp_single_frame_roundtrip():
    layer = ShtpLayer()
    (frame,) = fragment_cargo(ShtpChannel.CONTROL, b"hello", 0, max_frame=64)
    cargo = layer.feed(frame)
    assert cargo is not None and cargo.payload == b"hello" and cargo.channel == 2


def test_shtp_multi_fragment_reassembly():
    layer = ShtpLayer()
    payload = bytes(range(100))
    frames = fragment_cargo(ShtpChannel.CONTROL, payload, 0, max_frame=32)
    assert len(frames) >= 4
    out = [layer.feed(f) for f in frames]
    assert out[-1] is not None and out[-1].payload == payload
    assert all(o is None for o in out[:-1])  # completes only on the last frame


def test_shtp_continuation_without_start_is_dropped():
    layer = ShtpLayer()
    # a lone continuation frame (bit 15 set) with no cargo in progress
    frames = fragment_cargo(ShtpChannel.CONTROL, bytes(50), 0, max_frame=16)
    assert layer.feed(frames[1]) is None
    assert layer.discarded == 1


def test_shtp_new_start_discards_partial():
    layer = ShtpLayer()
    frames = fragment_cargo(ShtpChannel.CONTROL, bytes(50), 0, max_frame=16)
    layer.feed(frames[0])  # start a partial cargo
    fresh = fragment_cargo(ShtpChannel.CONTROL, b"x", 5, max_frame=64)[0]
    cargo = layer.feed(fresh)  # a non-continuation frame preempts the partial
    assert cargo is not None and cargo.payload == b"x"
    assert layer.discarded == 1


def test_shtp_reset_clears_tx_and_rx():
    layer = ShtpLayer()
    layer.next_frame(ShtpChannel.CONTROL, b"a")
    assert layer.tx_seq(ShtpChannel.CONTROL) == 1
    layer.reset()
    assert layer.tx_seq(ShtpChannel.CONTROL) == 0


def test_shtp_tx_cargo_too_large_raises():
    layer = ShtpLayer()
    with pytest.raises(ValueError):
        layer.next_frame(ShtpChannel.CONTROL, bytes(200))  # exceeds 64B MCU slot
