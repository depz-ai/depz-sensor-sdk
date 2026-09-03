"""Real-hardware lifecycle contract, run identically against every family.

Covers the open/start/stop/close matrix from contract 07 plus the misuse
orderings a real application will eventually hit (double stop, read after
stop, start after close, consumer raising mid-stream).

Everything here is bounded: no test may block longer than its declared
timeout, and every path closes the device in a ``finally``.
"""

from __future__ import annotations

import threading
import time

import pytest

from _drivers import SensorDriver
from _support import hang_guard, port_lease, sample_resources, wait_for_threads_to_die

from depz_sensor_sdk import DepzError, DepzTimeoutError, LinkClosedError

pytestmark = [pytest.mark.hardware, pytest.mark.timeout(120)]


# ── the canonical sequences (contract 07) ────────────────────────────────────


def test_open_close(any_device_info):
    with port_lease(any_device_info.resolve_port()):
        drv = SensorDriver.open(any_device_info)
        assert not drv.closed
        drv.close()
        assert drv.closed
    assert not wait_for_threads_to_die(timeout=3.0), "reader thread outlived close()"


def test_open_start_stop_close(any_device_info):
    with port_lease(any_device_info.resolve_port()), hang_guard(
        f"lifecycle-{any_device_info.family}", 60, {"stable_id": any_device_info.stable_id}
    ):
        drv = SensorDriver.open(any_device_info)
        try:
            drv.prepare()
            drv.start()
            sample = drv.read_one()
            assert drv.sample_is_valid(sample), f"first sample invalid: {sample}"
            drv.stop()
        finally:
            drv.close()
    assert not wait_for_threads_to_die(timeout=3.0)


def test_open_start_close_without_stop(any_device_info):
    """close() while streaming must not hang or leak — apps do exactly this."""
    with port_lease(any_device_info.resolve_port()):
        drv = SensorDriver.open(any_device_info)
        try:
            drv.prepare()
            drv.start()
            drv.read_one()
        finally:
            t0 = time.monotonic()
            drv.close()
            assert time.monotonic() - t0 < 5.0, "close() during streaming took too long"
    assert not wait_for_threads_to_die(timeout=3.0)


def test_start_stop_start_stop(any_device_info):
    """Restarting a stream on a still-open device must actually re-stream."""
    with port_lease(any_device_info.resolve_port()):
        drv = SensorDriver.open(any_device_info)
        try:
            drv.prepare()
            for round_no in range(2):
                drv.start()
                sample = drv.read_one()
                assert drv.sample_is_valid(sample), f"round {round_no}: invalid sample"
                drv.stop()
        finally:
            drv.close()


def test_close_then_reopen(any_device_info):
    """The port must be genuinely released — the immediate reopen proves it."""
    info = any_device_info
    with port_lease(info.resolve_port()):
        drv = SensorDriver.open(info)
        drv.prepare()
        drv.start()
        drv.read_one()
        drv.stop()
        drv.close()

        # No sleep: if close() leaves the fd open, this reopen fails outright.
        drv2 = SensorDriver.open(info)
        try:
            drv2.prepare()
            drv2.start()
            assert drv2.sample_is_valid(drv2.read_one()), "no valid data after reopen"
            drv2.stop()
        finally:
            drv2.close()


def test_reopen_after_exception_midstream(any_device_info):
    """An exception escaping user code must not wedge the device."""
    info = any_device_info
    with port_lease(info.resolve_port()):
        drv = SensorDriver.open(info)
        try:
            drv.prepare()
            drv.start()
            drv.read_one()
            raise RuntimeError("simulated consumer failure")
        except RuntimeError:
            pass
        finally:
            drv.close()

        drv2 = SensorDriver.open(info)
        try:
            drv2.prepare()
            drv2.start()
            assert drv2.sample_is_valid(drv2.read_one())
            drv2.stop()
        finally:
            drv2.close()


# ── misuse orderings ─────────────────────────────────────────────────────────


def test_double_stop_is_idempotent(any_device_info):
    with port_lease(any_device_info.resolve_port()):
        drv = SensorDriver.open(any_device_info)
        try:
            drv.prepare()
            drv.start()
            drv.read_one()
            drv.stop()
            drv.stop()  # must not raise
        finally:
            drv.close()


def test_double_close_is_idempotent(any_device_info):
    with port_lease(any_device_info.resolve_port()):
        drv = SensorDriver.open(any_device_info)
        drv.close()
        drv.close()  # must not raise
        assert drv.closed


def test_stop_without_start(any_device_info):
    """Stopping a never-started stream is a no-op, not an error."""
    with port_lease(any_device_info.resolve_port()):
        drv = SensorDriver.open(any_device_info)
        try:
            drv.prepare()
            drv.stop()
        finally:
            drv.close()


def test_operations_after_close_raise_cleanly(any_device_info):
    """Post-close calls must raise a typed SDK error — never hang, never
    succeed silently, never blow up with an unrelated AttributeError."""
    with port_lease(any_device_info.resolve_port()):
        drv = SensorDriver.open(any_device_info)
        drv.close()
        with pytest.raises((LinkClosedError, DepzError, DepzTimeoutError, RuntimeError, TimeoutError)):
            drv.start()


def test_context_manager_closes(any_device_info):
    with port_lease(any_device_info.resolve_port()):
        from _support import open_family

        with open_family(any_device_info) as dev:
            assert not dev.closed
            assert dev.get_device_name()
        assert dev.closed, "context manager did not close the device"
    assert not wait_for_threads_to_die(timeout=3.0)


def test_close_while_read_is_pending(any_device_info):
    """A blocked reader must be released by close(), not stranded forever.

    This is the deadlock that turns a UI 'Disconnect' button into a hang, so
    it is asserted with a hard bound rather than a generous sleep.

    The stream is stopped and drained first: with samples still in flight, a
    straggler — not `close()` — would be what wakes the reader, and the test
    would pass without proving anything.
    """
    info = any_device_info
    with port_lease(info.resolve_port()):
        drv = SensorDriver.open(info)
        try:
            drv.prepare()
            drv.start()
            drv.read_one()  # stream is confirmed live
            drv.stop()
            drv.drain_stragglers()

            released = threading.Event()
            outcome: list[BaseException] = []

            def _reader() -> None:
                try:
                    drv.pending_read()
                except BaseException as exc:  # noqa: BLE001 — recording, not handling
                    outcome.append(exc)
                finally:
                    released.set()

            t = threading.Thread(target=_reader, name="pending-read", daemon=True)
            t.start()
            time.sleep(0.3)  # let the reader actually block
            assert not released.is_set(), "reader was not blocked — test is not exercising close()"

            t0 = time.monotonic()
            drv.close()
            assert released.wait(5.0), (
                "close() did not release a pending read within 5s — reader is stranded"
            )
            elapsed = time.monotonic() - t0
            assert elapsed < 5.0, f"close() took {elapsed:.1f}s to release the reader"
            t.join(timeout=5.0)
            assert not t.is_alive()
        finally:
            drv.close()


def test_consumer_exception_does_not_kill_the_reader(any_device_info):
    """A raising callback must not take the reader thread down with it.

    Callbacks run on the reader thread. If one escapes, an unguarded reader
    loop dies and the device goes silent while still reporting `closed ==
    False` — a hang with no error, the worst failure mode there is.
    """
    info = any_device_info
    with port_lease(info.resolve_port()):
        drv = SensorDriver.open(info)
        try:
            drv.prepare()
            calls: list[int] = []

            def _boom(_evt) -> None:
                calls.append(1)
                raise ValueError("callback raises on purpose")

            drv.dev.on_event(_boom)
            drv.start()
            first = drv.read_one()
            assert drv.sample_is_valid(first)

            # The stream must survive whatever the callback did.
            time.sleep(0.3)
            assert drv.sample_is_valid(drv.read_one()), (
                "stream died after a consumer callback raised"
            )
            drv.stop()
        finally:
            drv.close()


def test_read_timeout_does_not_wedge_the_device(any_device_info):
    """A read timeout must be recoverable: stop/start and data flows again."""
    info = any_device_info
    with port_lease(info.resolve_port()):
        drv = SensorDriver.open(info)
        try:
            drv.prepare()
            drv.start()
            drv.read_one()
            drv.stop()
            drv.drain_stragglers()  # samples already in flight are not a bug

            # With the stream stopped and drained, a read must time out rather
            # than block forever.
            t0 = time.monotonic()
            with pytest.raises((TimeoutError, DepzTimeoutError)):
                drv.read_one(timeout=0.5)
            assert time.monotonic() - t0 < 3.0, "read after stop overran its timeout"

            drv.start()
            assert drv.sample_is_valid(drv.read_one()), "device wedged after a read timeout"
            drv.stop()
        finally:
            drv.close()


def test_port_is_released_to_the_os_after_close(any_device_info):
    """close() must return the fd, not just mark the object closed."""
    import serial

    info = any_device_info
    with port_lease(info.resolve_port()):
        before = sample_resources()
        drv = SensorDriver.open(info)
        drv.prepare()
        drv.start()
        drv.read_one()
        drv.close()
        after = sample_resources()
        assert after.tty_fd_count <= before.tty_fd_count, "serial fd leaked past close()"

        # The strongest possible proof: an exclusive reopen by a fresh handle.
        s = serial.Serial(info.resolve_port(), 115200, timeout=0.05, exclusive=True)
        s.close()
