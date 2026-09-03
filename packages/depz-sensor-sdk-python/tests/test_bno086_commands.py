"""BNO086 device-level command coverage complementing test_bno086.py: the
interval_us enable path, standalone get_feature, verify=False, the tare/
reorientation/periodic-DCD facade, and the per-sensor enable sugar."""

from __future__ import annotations

import time

import pytest

from depz_sensor_sdk import Bno086, SensorId
from depz_sensor_sdk.bno086 import TareAxis, TareBasis
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
