"""SR04-specific hardware contract (contract 03).

Protocol correctness only. Whether the reported distance matches physical
reality needs a calibrated target at a known range; that is out of scope and
recorded as such in QA_MATRIX.md. What is asserted is that the decode is sane,
the configuration round-trips, and no-echo is handled as data rather than as an
error.
"""

from __future__ import annotations

import time

import pytest

from _support import SR04_MODULE_NO_ECHO_US, port_lease

from depz_sensor_sdk import BusyError, Sr04
from depz_sensor_sdk.protocol.sr04 import (
    ECHO_DECAY_MAX_US,
    ECHO_DECAY_MIN_US,
    ECHO_DECAY_WIRE_MAX_US,
    ECHO_TIMEOUT,
    SAMPLE_PERIOD_WIRE_MAX_US,
)

pytestmark = [pytest.mark.hardware, pytest.mark.hardware_sr04, pytest.mark.timeout(120)]


@pytest.fixture
def sr04(sr04_info):
    with port_lease(sr04_info.resolve_port()):
        dev = Sr04(sr04_info.resolve_port())
        try:
            yield dev
        finally:
            try:
                dev.stop()
            except Exception:
                pass
            dev.close()


def _drain(it, n, timeout=5.0):
    out = []
    deadline = time.monotonic() + timeout
    while len(out) < n and time.monotonic() < deadline:
        try:
            out.append(it._queue.get(timeout=max(0.05, deadline - time.monotonic())))
        except Exception:
            break
    return out


def test_measure_once(sr04):
    m = sr04.measure_once(timeout=1.0)
    assert m.source == "once"
    assert m.timestamp_us > 0
    if m.valid and m.echo_time_us < SR04_MODULE_NO_ECHO_US:
        assert 0 < m.distance_mm < 10_000, f"implausible distance {m.distance_mm} mm"
    elif m.valid:
        # The HC-SR04 module's own no-echo pulse (~58.3 ms) arrives as data,
        # not as the firmware's 0xFFFF sentinel (F-23) — legitimate no-echo.
        pass
    else:
        # No echo is a legitimate reading (nothing in range), not a fault.
        assert m.echo_time_us == ECHO_TIMEOUT
        assert m.distance_mm is None


def test_measurement_loop_streams(sr04):
    it = sr04.stream()
    sr04.start()
    samples = _drain(it, 10)
    sr04.stop()
    it.close()
    assert len(samples) == 10, f"only {len(samples)} samples in 5s"
    assert all(s.source == "loop" for s in samples), "loop samples mislabelled"


def test_timestamps_are_monotonic(sr04):
    it = sr04.stream()
    sr04.start()
    samples = _drain(it, 15)
    sr04.stop()
    it.close()
    assert len(samples) >= 10
    ts = [s.timestamp_us for s in samples]
    assert ts == sorted(ts), "timestamps went backwards"
    assert len(set(ts)) == len(ts), "duplicate timestamps in the stream"


def test_measure_once_is_rejected_while_looping(sr04):
    """The device owns one ranging engine — a single shot during the loop must
    be refused with ERR_BUSY, not silently interleaved."""
    sr04.start()
    time.sleep(0.2)
    try:
        with pytest.raises(BusyError):
            sr04.measure_once(timeout=1.0)
    finally:
        sr04.stop()


def test_sample_period_round_trips(sr04):
    original = sr04.get_sample_period_us()
    try:
        for period in (20_000, 50_000, 100_000):
            sr04.set_sample_period_us(period)
            assert sr04.get_sample_period_us() == period, f"{period}µs did not stick"
    finally:
        sr04.set_sample_period_us(original)
    assert sr04.get_sample_period_us() == original


def test_sample_period_survives_a_stream(sr04):
    original = sr04.get_sample_period_us()
    try:
        sr04.set_sample_period_us(30_000)
        it = sr04.stream()
        sr04.start()
        assert _drain(it, 3)
        sr04.stop()
        it.close()
        assert sr04.get_sample_period_us() == 30_000, "config lost across a stream"
    finally:
        sr04.set_sample_period_us(original)


def test_sample_period_resets_on_reopen(sr04_info):
    """Pin whichever way config persistence actually goes, so a change to it
    is a deliberate decision rather than a surprise."""
    with port_lease(sr04_info.resolve_port()):
        a = Sr04(sr04_info.resolve_port())
        try:
            baseline = a.get_sample_period_us()
            a.set_sample_period_us(77_000)
            assert a.get_sample_period_us() == 77_000
        finally:
            a.close()

        b = Sr04(sr04_info.resolve_port())
        try:
            after = b.get_sample_period_us()
            # The device keeps its configuration across a host reconnect: the
            # MCU is not reset by closing the CDC port.
            assert after == 77_000, (
                f"sample period after reopen is {after}, expected the configured 77000 "
                "(config persistence across reopen changed)"
            )
            b.set_sample_period_us(baseline)
        finally:
            b.close()


def test_echo_decay_is_clamped_and_reported(sr04):
    """set_echo_decay_us re-reads because the device clamps silently — the
    returned value must be the one actually in effect (contract 03 §3)."""
    original = sr04.get_echo_decay_us()
    try:
        assert sr04.set_echo_decay_us(10_000) == 10_000

        # Inside the u16 wire field but outside the clamp window: legal to
        # send, and the device clamps it.
        low = sr04.set_echo_decay_us(1)
        assert low == sr04.get_echo_decay_us(), "returned value disagrees with the device"
        assert ECHO_DECAY_MIN_US <= low <= ECHO_DECAY_MAX_US, (
            f"clamped value {low} outside the documented window"
        )

        high = sr04.set_echo_decay_us(ECHO_DECAY_WIRE_MAX_US)
        assert high == sr04.get_echo_decay_us()
        assert ECHO_DECAY_MIN_US <= high <= ECHO_DECAY_MAX_US, (
            f"clamped value {high} outside the documented window"
        )
    finally:
        sr04.set_echo_decay_us(original)


def test_out_of_range_config_raises_value_error(sr04):
    """An unrepresentable value must raise a typed SDK error, not leak
    struct.error from the codec (matches Vl53l8Cx's setters)."""
    original_decay = sr04.get_echo_decay_us()
    original_period = sr04.get_sample_period_us()
    try:
        with pytest.raises(ValueError):
            sr04.set_echo_decay_us(ECHO_DECAY_WIRE_MAX_US + 1)
        with pytest.raises(ValueError):
            sr04.set_echo_decay_us(-1)
        with pytest.raises(ValueError):
            sr04.set_sample_period_us(SAMPLE_PERIOD_WIRE_MAX_US + 1)
        with pytest.raises(ValueError):
            sr04.set_sample_period_us(-1)
        # A rejected set must not have disturbed the device.
        assert sr04.get_echo_decay_us() == original_decay
        assert sr04.get_sample_period_us() == original_period
    finally:
        sr04.set_echo_decay_us(original_decay)
        sr04.set_sample_period_us(original_period)


def test_config_change_between_streams(sr04):
    original = sr04.get_sample_period_us()
    try:
        for period in (25_000, 60_000):
            sr04.set_sample_period_us(period)
            it = sr04.stream()
            sr04.start()
            assert len(_drain(it, 5)) == 5, f"stream broke at period {period}"
            sr04.stop()
            it.close()
    finally:
        sr04.set_sample_period_us(original)


def test_effective_rate_tracks_the_sample_period(sr04):
    """A shorter period must actually produce a faster stream.

    Only an ordering assertion: the device auto-throttles to the echo window
    (contract 03 §3), so the absolute rate is not the SDK's to promise.
    """
    original = sr04.get_sample_period_us()

    def measure_rate(period_us: float) -> float:
        sr04.set_sample_period_us(int(period_us))
        it = sr04.stream()
        sr04.start()
        samples = _drain(it, 12, timeout=8.0)
        sr04.stop()
        it.close()
        assert len(samples) >= 6, f"too few samples at {period_us}µs"
        span_s = (samples[-1].timestamp_us - samples[0].timestamp_us) / 1e6
        return (len(samples) - 1) / span_s if span_s > 0 else 0.0

    try:
        slow = measure_rate(150_000)
        fast = measure_rate(30_000)
        assert fast > slow * 1.3, (
            f"30ms period gave {fast:.1f}/s, 150ms gave {slow:.1f}/s — "
            "the period is not affecting the rate"
        )
    finally:
        sr04.set_sample_period_us(original)


def test_stream_restarts_after_stop(sr04):
    for _ in range(3):
        it = sr04.stream()
        sr04.start()
        assert len(_drain(it, 3)) == 3
        sr04.stop()
        it.close()


def test_no_stale_samples_after_restart(sr04):
    """A restarted stream must not replay the previous session's samples."""
    it1 = sr04.stream()
    sr04.start()
    first = _drain(it1, 5)
    sr04.stop()
    it1.close()
    assert first

    time.sleep(0.3)

    it2 = sr04.stream()
    sr04.start()
    second = _drain(it2, 5)
    sr04.stop()
    it2.close()
    assert second

    assert second[0].timestamp_us > first[-1].timestamp_us, (
        "the restarted stream replayed samples from the previous session"
    )


def test_start_is_idempotent(sr04):
    it = sr04.stream()
    sr04.start()
    sr04.start()  # documented idempotent
    assert len(_drain(it, 3)) == 3
    sr04.stop()
    it.close()


def test_stream_dropped_count_is_exposed(sr04):
    """A slow consumer must lose the *oldest* samples and say how many."""
    it = sr04.stream(maxsize=2)
    original = sr04.get_sample_period_us()
    try:
        sr04.set_sample_period_us(20_000)
        sr04.start()
        time.sleep(1.5)  # deliberately do not read: overflow the bounded queue
        sr04.stop()
        assert it.dropped_count > 0, "a bounded queue silently kept everything"
        assert any(c > 0 for c in sr04.stream_dropped_counts)
    finally:
        sr04.set_sample_period_us(original)
        it.close()
