"""Thorough SR04 coverage: config clamp/read-back, measure vs loop, busy,
timeout sentinel, streams + drop-oldest counters, and repeated same-type
(two SR04 over two independent loopbacks)."""

from __future__ import annotations

import time

import pytest

from depz_sensor_sdk import BusyError, Sr04, TextEvent
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


def test_a_raising_measurement_callback_never_stops_the_stream(rig):
    """Contract 07 §3 on a sensor stream: the raising callback is counted, the
    queue and the next measurements still arrive."""
    fake, dev = rig

    def boom(m):
        raise ValueError("consumer bug")

    dev.on_measurement(boom)
    it = dev.stream(maxsize=8)
    dev.start()
    fake.send_measurement(0x37)
    fake.send_measurement(0x37)
    assert [next(it).source for _ in range(2)] == ["loop", "loop"]
    assert dev.stats.callback_errors == 2
    dev.stop()
    assert dev.get_software_name()


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


def test_measure_once_takes_only_a_full_once_report(rig, monkeypatch):
    # The reply to MEASURE_ONCE is the RPT_DATA with cmd 0x36 (contract 03
    # §2). A loop sample still in flight, or a data report too short for its
    # 11 bytes, arriving first must not be taken for it.
    fake, dev = rig
    stream = dev.stream(maxsize=4)
    real = fake._handle

    def stray_reports_first(pkt):
        if pkt.cmd == 0x36:
            fake._send(0x91, bytes((0x36, 1, 2)))     # 3 of 11 bytes
            fake.send_measurement(0x37, echo=1_111)   # a loop sample
            fake.send_measurement(0x36, echo=2_222)
        else:
            real(pkt)

    monkeypatch.setattr(fake, "_handle", stray_reports_first)
    m = dev.measure_once()
    assert (m.source, m.echo_time_us) == ("once", 2_222)
    stray = next(stream)
    assert (stray.source, stray.echo_time_us) == ("loop", 1_111)  # not lost either
    # The reply belongs to the request, the short report to nobody: the next
    # streamed item is the marker sent after them.
    fake.send_measurement(0x37, echo=3_333)
    marker = next(stream)
    assert (marker.source, marker.echo_time_us) == ("loop", 3_333)


@pytest.mark.parametrize("setter, bad", [
    ("set_sample_period_us", -1),
    ("set_sample_period_us", 0x1_0000_0000),   # past the u32 wire field
    ("set_echo_decay_us", -1),
    ("set_echo_decay_us", 0x1_0000),           # past the u16 wire field
])
def test_a_value_the_wire_field_cannot_hold_is_refused_before_the_wire(rig, setter, bad):
    fake, dev = rig
    before = list(fake.received)
    with pytest.raises(ValueError, match="wire field"):
        getattr(dev, setter)(bad)
    assert fake.received == before


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


def test_unknown_source_cmd_is_a_text_event_not_a_measurement(rig):
    # contract 03 §2 defines RPT_DATA.cmd 0x36 / 0x37 only (ERRATA B11)
    fake, dev = rig
    it, events = dev.stream(maxsize=4), dev.events(maxsize=4)
    fake.send_measurement(0x38)
    fake.send_measurement(0x37)
    assert next(it).source == "loop"  # the 0x38 report never reached the stream
    ev = next(events)
    assert isinstance(ev, TextEvent) and ev.cmd == 0x91 and ev.text.startswith("38")


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
