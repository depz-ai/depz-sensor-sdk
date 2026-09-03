"""Thorough SR04 coverage: config clamp/read-back, measure vs loop, busy,
timeout sentinel, streams + drop-oldest counters, and repeated same-type
(two SR04 over two independent loopbacks)."""

from __future__ import annotations

import time

import pytest

from depz_sensor_sdk import BusyError, Sr04
from depz_sensor_sdk.protocol.sr04 import (
    ECHO_DECAY_MAX_US,
    ECHO_DECAY_MIN_US,
    ECHO_TIMEOUT,
    distance_mm_from_echo,
)

from fake_device import FakeSr04


@pytest.fixture()
def rig():
    fake = FakeSr04()
    dev = Sr04(fake.link, timeout=1.0)
    yield fake, dev
    dev.close()
    fake.close()


# ── configuration clamp / read-back ───────────────────────────────────────────


def test_sample_period_readback_is_stored_not_effective(rig):
    fake, dev = rig
    dev.set_sample_period_us(7_000)
    # read-back returns the stored value even though the device throttles the
    # *effective* rate by the echo window (contract 03 §3).
    assert dev.get_sample_period_us() == 7_000
    assert fake.sample_period_us == 7_000


def test_echo_decay_clamps_both_ends(rig):
    _, dev = rig
    assert dev.set_echo_decay_us(100) == ECHO_DECAY_MIN_US  # below min → 4000
    # NB: the field is u16 on the wire, so the device-side clamp only shows for
    # in-range-but-above-max values (65535 → 65000); larger asks can't be sent.
    assert dev.set_echo_decay_us(65_535) == ECHO_DECAY_MAX_US  # above max → 65000
    assert dev.set_echo_decay_us(9_000) == 9_000  # in range → verbatim
    assert dev.get_echo_decay_us() == 9_000  # direct read matches


# ── measure-once vs loop ──────────────────────────────────────────────────────


def test_measure_once_is_source_once(rig):
    fake, dev = rig
    fake.echo_time_us = 5_831
    m = dev.measure_once()
    assert m.source == "once" and m.valid
    assert m.distance_mm == pytest.approx(distance_mm_from_echo(5_831))


def test_loop_samples_are_source_loop(rig):
    fake, dev = rig
    dev.start()
    it = dev.stream(maxsize=8)
    fake.send_measurement(0x37)
    fake.send_measurement(0x37)
    assert [next(it).source for _ in range(2)] == ["loop", "loop"]
    dev.stop()


def test_start_stop_idempotent(rig):
    _, dev = rig
    dev.start()
    dev.start()  # idempotent — must not raise
    dev.stop()
    dev.stop()


# ── busy + timeout sentinel ────────────────────────────────────────────────────


def test_measure_once_busy_during_loop(rig):
    _, dev = rig
    dev.start()
    with pytest.raises(BusyError):
        dev.measure_once()
    dev.stop()


def test_no_echo_timeout_sentinel(rig):
    fake, dev = rig
    fake.echo_time_us = ECHO_TIMEOUT
    m = dev.measure_once()
    assert not m.valid
    assert m.echo_time_us == ECHO_TIMEOUT
    assert m.distance_mm is None
    assert m.distance_mm_at(20.0) is None


def test_temperature_compensated_distance(rig):
    fake, dev = rig
    m = dev.measure_once()
    warm = m.distance_mm_at(30.0)  # c = 331.3 + 0.606*30 ≈ 349.5 m/s
    assert warm is not None and warm > m.distance_mm  # warmer air → longer range


# ── streams + drop-oldest counters ─────────────────────────────────────────────


def test_stream_drop_oldest_counter(rig):
    fake, dev = rig
    dev.start()
    it = dev.stream(maxsize=4)
    for _ in range(4 + 3):  # 3 beyond capacity, never consumed → 3 dropped
        fake.send_measurement(0x37)
    deadline = time.monotonic() + 1.0
    while it.dropped_count < 3 and time.monotonic() < deadline:
        time.sleep(0.005)
    assert it.dropped_count == 3
    assert dev.stream_dropped_counts == [3]
    # the four survivors are still pullable
    assert all(next(it).source == "loop" for _ in range(4))
    dev.stop()


def test_two_independent_streams_same_device(rig):
    fake, dev = rig
    dev.start()
    a = dev.stream(maxsize=8)
    b = dev.stream(maxsize=8)
    fake.send_measurement(0x37)
    assert next(a).source == "loop"
    assert next(b).source == "loop"  # both subscribers get every sample
    assert len(dev.stream_dropped_counts) == 2
    dev.stop()


def test_sync_in_single_shot_reaches_stream(rig):
    fake, dev = rig
    it = dev.stream(maxsize=4)
    fake.send_measurement(0x36)  # unsolicited single shot (AUX SYNC_IN edge)
    assert next(it).source == "once"


# ── repeated same-type: two SR04 over two loopbacks ───────────────────────────


def test_two_sr04_instances_stream_independently():
    fakes = [FakeSr04(), FakeSr04()]
    devs = [Sr04(f.link, timeout=1.0) for f in fakes]
    try:
        for d in devs:
            d.start()
        its = [d.stream(maxsize=8) for d in devs]
        # drive each fake a different number of times
        for _ in range(2):
            fakes[0].send_measurement(0x37)
        for _ in range(3):
            fakes[1].send_measurement(0x37)
        got0 = [next(its[0]) for _ in range(2)]
        got1 = [next(its[1]) for _ in range(3)]
        assert len(got0) == 2 and len(got1) == 3
        assert all(m.source == "loop" for m in got0 + got1)
        # streams are isolated: device 0 saw exactly its own 2 samples
        assert devs[0].stats.rx_packets >= 2
    finally:
        for d in devs:
            d.stop()
            d.close()
        for f in fakes:
            f.close()
