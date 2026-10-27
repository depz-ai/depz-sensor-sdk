"""BNO086 device-level command coverage complementing test_bno086.py: the
interval_us enable path, standalone get_feature, verify=False, the tare/
reorientation/periodic-DCD facade, and the per-sensor enable sugar."""

from __future__ import annotations

import time

import pytest

from depz_sensor_sdk import Bno086, SensorId
from depz_sensor_sdk.bno086 import FeatureFlag, MotionIntent, TareAxis, TareBasis
from depz_sensor_sdk.errors import DepzTimeoutError
from depz_sensor_sdk.bno086.sh2 import build_set_feature

from fake_bno086 import FakeBno086


@pytest.fixture()
def rig():
    fake = FakeBno086()
    dev = Bno086(fake.link, timeout=1.0)
    yield fake, dev
    dev.close()
    fake.close()


def _wait(pred, timeout=1.0):
    deadline = time.monotonic() + timeout
    while not pred() and time.monotonic() < deadline:
        time.sleep(0.005)
    assert pred()


# ── enable variants ───────────────────────────────────────────────────────────


def test_enable_via_interval_us(rig):
    fake, dev = rig
    resp = dev.enable(SensorId.GYROSCOPE, interval_us=5_000)  # 200 Hz
    assert fake.last_set_feature == build_set_feature(SensorId.GYROSCOPE, 5_000)
    assert resp is not None and resp.interval_us == 5_000


def test_enable_verify_false_returns_none(rig):
    fake, dev = rig
    assert dev.enable(SensorId.ACCELEROMETER, hz=100, verify=False) is None
    _wait(lambda: fake.features.get(SensorId.ACCELEROMETER) == 10_000)


def test_enable_rejects_nonpositive_hz(rig):
    _, dev = rig
    with pytest.raises(ValueError):
        dev.enable(SensorId.ACCELEROMETER, hz=0)


def test_get_feature_standalone(rig):
    fake, dev = rig
    dev.enable(SensorId.MAGNETOMETER, hz=50)
    fr = dev.get_feature(SensorId.MAGNETOMETER)
    assert fr.sensor_id == SensorId.MAGNETOMETER
    assert fr.interval_us == 20_000  # 50 Hz


def test_enable_sugar_methods(rig):
    fake, dev = rig
    dev.enable_accelerometer(hz=100)
    dev.enable_gyroscope(hz=100)
    dev.enable_rotation_vector(hz=50)
    assert fake.features[SensorId.ACCELEROMETER] == 10_000
    assert fake.features[SensorId.GYROSCOPE] == 10_000
    assert fake.features[SensorId.ROTATION_VECTOR] == 20_000


# ── tare / reorientation / periodic DCD facade ────────────────────────────────


def test_persist_tare_sends_subcommand_1(rig):
    fake, dev = rig
    dev.persist_tare()
    _wait(lambda: len(fake.tare_requests) == 1)
    (req,) = fake.tare_requests
    assert req[2] == 0x03  # TARE command
    assert req[3] == 0x01  # persist subcommand


def test_set_reorientation_quaternion(rig):
    fake, dev = rig
    dev.set_reorientation(0.0, 0.0, 0.0, 1.0)
    _wait(lambda: len(fake.tare_requests) == 1)
    (req,) = fake.tare_requests
    assert req[2] == 0x03 and req[3] == 0x02  # tare / set-reorientation subcommand


def test_set_reorientation_out_of_range_raises(rig):
    _, dev = rig
    with pytest.raises(ValueError):
        dev.set_reorientation(3.0, 0.0, 0.0, 0.0)  # 3.0 * 2^14 overflows int16 Q14


def test_configure_periodic_dcd_no_response(rig):
    _, dev = rig
    dev.configure_periodic_dcd(True)  # no command response — must return promptly
    dev.configure_periodic_dcd(False)


def test_tare_now_default_all_axes(rig):
    fake, dev = rig
    dev.tare_now()
    _wait(lambda: len(fake.tare_requests) == 1)
    (req,) = fake.tare_requests
    assert req[3:6] == bytes((0x00, TareAxis.ALL, TareBasis.ROTATION_VECTOR))


# ── interactive calibration / force flush (BNO086 only) ───────────────────────


def test_set_motion_intent_sends_command_0x0e(rig):
    fake, dev = rig
    dev.set_motion_intent(MotionIntent.STATIONARY_WITH_VIBRATION)  # no response
    _wait(lambda: fake.motion_intents == [2])


def test_set_motion_intent_rejects_out_of_range(rig):
    _, dev = rig
    with pytest.raises(ValueError):
        dev.set_motion_intent(256)


def test_enable_motion_request_skips_rate_check(rig):
    fake, dev = rig
    assert dev.enable_motion_request() is None  # verify off: no granted-rate warning
    _wait(lambda: fake.features.get(SensorId.MOTION_REQUEST) == 1_000_000)


def test_flush_returns_on_flush_completed(rig):
    fake, dev = rig
    seen = []
    dev.on_report(seen.append)
    dev.flush(SensorId.ACCELEROMETER)
    assert fake.flush_requests == [SensorId.ACCELEROMETER]
    assert any(getattr(r, "flushed_sensor", None) == SensorId.ACCELEROMETER for r in seen)


def test_flush_times_out_without_answer(rig, monkeypatch):
    fake, dev = rig
    monkeypatch.setattr(fake, "push_input_cargo", lambda *a, **k: 0)  # hub stays silent
    with pytest.raises(DepzTimeoutError):
        dev.flush(SensorId.ACCELEROMETER, timeout=0.1)
    assert dev._flush_waiters[SensorId.ACCELEROMETER] == []


# ── hub power: sleep / power_on (contract 05 §6a) ─────────────────────────────


def test_sleep_and_power_on_send_executable_frames(rig, vectors):
    fake, dev = rig
    dev.sleep()  # no response from the hub — must return on the bridge ack
    dev.power_on()
    _wait(lambda: len(fake.executable_frames) == 2)
    want = [bytes.fromhex(c["shtp_frame"]) for c in vectors("bno086_shtp.json")["executable"]]
    assert fake.executable_frames == want
    assert fake.tare_requests == []  # nothing on the control channel


def test_enable_activity_classifier_cfg_word_selects_activities(rig):
    """Personal Activity Classifier (SH-2 §6.5.36.1): the Set Feature config
    word is the activity mask — bits 4, 6, 7 = still, walking, running — and
    the sensitivity is the confidence change in percent."""
    fake, dev = rig
    dev.enable(SensorId.PERSONAL_ACTIVITY_CLASSIFIER, hz=2, sensitivity=10,
               cfg_word=0x000000D0, verify=False)
    _wait(lambda: fake.last_set_feature is not None)
    assert fake.last_set_feature == build_set_feature(0x1E, 500_000, 0, 10, 0, 0x000000D0)


def test_enable_with_wakeup_always_on_flags(rig):
    fake, dev = rig
    flags = FeatureFlag.WAKEUP | FeatureFlag.ALWAYS_ON
    dev.enable(SensorId.ACCELEROMETER, interval_us=10_000, flags=flags, verify=False)
    _wait(lambda: fake.last_set_feature is not None)
    assert fake.last_set_feature[2] == 0x0C
