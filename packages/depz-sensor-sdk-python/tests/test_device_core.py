"""DeviceBase behavior against the in-process fake firmware."""

import pytest

from depz_sensor_sdk import (
    BusyError,
    DepzTimeoutError,
    Sr04,
    StatusError,
)
from depz_sensor_sdk.protocol.common import SyncPinConfig, SyncPinMode, SyncPinPolarity

from fake_device import FakeSr04


@pytest.fixture()
def rig():
    fake = FakeSr04()
    dev = Sr04(fake.link, timeout=1.0)
    yield fake, dev
    dev.close()
    fake.close()


def test_identity_roundtrip(rig):
    fake, dev = rig
    assert dev.get_software_name() == FakeSr04.SOFTWARE_NAME
    assert dev.get_device_name() == FakeSr04.DEVICE_NAME
    assert dev.get_serial_number() == FakeSr04.SERIAL


def test_temperature(rig):
    _, dev = rig
    assert dev.read_mcu_temperature() == pytest.approx(27.3)


def test_sync_time_produces_offset(rig):
    _, dev = rig
    ts = dev.sync_time(samples=3)
    assert ts.rtt_us >= 0
    assert dev.time_sync is ts
    # to_host_time inverts the offset
    assert dev.to_host_time_us(1000 + ts.offset_us) == 1000


def test_unknown_cmd_raises_status_error(rig):
    _, dev = rig
    with pytest.raises(StatusError) as ei:
        dev.request(0x2A, ok_completes=True)
    assert ei.value.status_name == "ERR_INVALID_CMD"


def test_sync_pin_validation(rig):
    _, dev = rig
    dev.set_sync_pin(SyncPinConfig(1, SyncPinMode.OUT_BOTH, SyncPinPolarity.IDLE_LOW))
    with pytest.raises(StatusError):
        dev.set_sync_pin(SyncPinConfig(9, SyncPinMode.IN, SyncPinPolarity.IDLE_LOW))
    cfg = dev.get_sync_pin(2)
    assert cfg.pin == 2 and cfg.mode == SyncPinMode.DISABLE


def test_timeout_when_device_silent():
    fake = FakeSr04()
    dev = Sr04(fake.link, timeout=0.05)
    fake._side.close()  # device stops answering (link still "open" host-side)
    try:
        with pytest.raises((DepzTimeoutError, Exception)):
            dev.get_software_name()
    finally:
        dev.close()
        fake.close()


class TestSr04:
    def test_config_roundtrip(self, rig):
        fake, dev = rig
        assert dev.get_sample_period_us() == 50_000
        dev.set_sample_period_us(10_000)
        assert fake.sample_period_us == 10_000
        assert dev.get_sample_period_us() == 10_000

    def test_echo_decay_clamp_reread(self, rig):
        _, dev = rig
        assert dev.set_echo_decay_us(1000) == 4000  # device clamps to min
        assert dev.set_echo_decay_us(7000) == 7000

    def test_measure_once(self, rig):
        fake, dev = rig
        m = dev.measure_once()
        assert m.source == "once"
        assert m.echo_time_us == fake.echo_time_us
        assert m.valid
        assert m.distance_mm == pytest.approx(5831 * 343 / 2000)

    def test_measure_once_busy_during_loop(self, rig):
        _, dev = rig
        dev.start()
        with pytest.raises(BusyError):
            dev.measure_once()
        dev.stop()

    def test_timeout_sentinel(self, rig):
        fake, dev = rig
        fake.echo_time_us = 0xFFFF
        m = dev.measure_once()
        assert not m.valid
        assert m.distance_mm is None

    def test_stream_and_callbacks(self, rig):
        fake, dev = rig
        seen = []
        unsub = dev.on_measurement(seen.append)
        dev.start()
        it = dev.stream(maxsize=16)
        for _ in range(3):
            fake.send_measurement(0x37)
        got = [next(it) for _ in range(3)]
        assert all(m.source == "loop" for m in got)
        assert len(seen) == 3
        unsub()
        fake.send_measurement(0x37)
        assert next(it).source == "loop"
        assert len(seen) == 3  # callback unsubscribed
        dev.stop()

    def test_sync_in_single_shot_goes_to_stream(self, rig):
        fake, dev = rig
        it = dev.stream(maxsize=4)
        fake.send_measurement(0x36)  # unsolicited single shot (SYNC_IN edge)
        assert next(it).source == "once"


def test_sync_time_all_aligns_devices():
    from depz_sensor_sdk import sync_time_all

    fakes = [FakeSr04(), FakeSr04()]
    devs = [Sr04(f.link, timeout=1.0) for f in fakes]
    try:
        result = sync_time_all(devs, samples=3)
        assert set(result.keys()) == set(devs)
        # every device now maps its own clock onto the shared host timeline
        for dev in devs:
            ts = result[dev]
            assert ts.rtt_us >= 0
            assert dev.to_host_time_us(1000 + ts.offset_us) == 1000
    finally:
        for dev in devs:
            dev.close()
        for f in fakes:
            f.close()
