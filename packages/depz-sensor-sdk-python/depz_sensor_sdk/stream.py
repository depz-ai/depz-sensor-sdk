"""Stream[T]: one typed output of a device (contract 07 §3) — DepzDevice has
one for its events, every sensor class one for its measurements / frames /
samples / reports. StreamQueue and StreamIterator are its pull side."""

from __future__ import annotations

import logging
import queue
import threading
import time
import weakref
from typing import TYPE_CHECKING, Any, Callable, Generic, TypeVar

from ._waits import wait_limit
from .errors import DepzTimeoutError, InvalidStateError

if TYPE_CHECKING:
    from .device import DepzDevice

T = TypeVar("T")

_log = logging.getLogger("depz_sensor_sdk")

#: Slice length for close-aware blocking waits. Small enough that `close()`
#: releases a pending wait promptly, large enough to stay off the CPU.
_CLOSE_POLL_S = 0.05


def call_guarded(cb: Callable[[Any], None], item: Any) -> bool:
    """Run a user callback where its exception must not stop the caller (a
    device reader, a dataset player — contract 07 §3, 09 §Player): one that
    raises is logged to ``depz_sensor_sdk`` and dropped. False if it raised,
    so the caller counts it."""
    try:
        cb(item)
    except Exception:
        _log.exception("callback %r raised on %s", cb, type(item).__name__)
        return False
    return True


class StreamQueue(Generic[T]):
    """Bounded drop-oldest queue with a drop counter (contract 07 §3)."""

    def __init__(self, maxsize: int) -> None:
        self._q: queue.Queue[T] = queue.Queue(maxsize)
        self.dropped_count = 0
        self.closed = False

    def put(self, item: T) -> None:
        while True:
            try:
                self._q.put_nowait(item)
                return
            except queue.Full:
                try:
                    self._q.get_nowait()
                    self.dropped_count += 1
                except queue.Empty:
                    pass

    def get(self, timeout: float | None = None) -> T:
        return self._q.get(timeout=None if timeout is None else wait_limit(timeout))


class StreamIterator(Generic[T]):
    """Iterator over a StreamQueue that registers **eagerly** at construction.

    A lazy generator would subscribe only on the first `next()`, losing
    everything emitted in between (a real race on instant replay links).
    Ends quietly (StopIteration) when the device closes, after the program's
    own `close()` and after a loss alike; the device's `lost` then tells them
    apart (contract 07 §3). Deregisters on GC/close().
    """

    def __init__(
        self,
        stream: Stream[T],
        maxsize: int,
        where: Callable[[T], bool] | None = None,
    ) -> None:
        self._queue: StreamQueue[T] = StreamQueue(maxsize)
        self._stream = stream
        stream._attach(self._queue, where)
        # An iterator dropped without close() leaves the stream when collected;
        # the finalizer must not reference the iterator itself.
        self._deregister = weakref.finalize(self, stream._detach, self._queue)

    @property
    def dropped_count(self) -> int:
        return self._queue.dropped_count

    def __iter__(self) -> StreamIterator[T]:
        return self

    def __next__(self) -> T:
        while True:
            try:
                return self._queue.get(timeout=0.2)
            except queue.Empty:
                if self._stream._closed:
                    self.close()
                    raise StopIteration from None

    def close(self) -> None:
        self._deregister()


class Stream(Generic[T]):
    """The subscribers, the pull queues and the counters of one kind of item.

    Consumer side: `on(cb)` subscribes a callback (reader-thread context),
    calling the stream makes an eager bounded drop-oldest iterator,
    `next(timeout)` waits for one item and is released by `close()`,
    `dropped_counts` and `parse_errors` are its counters. Owner side (the
    sensor class): the reader calls `_emit(item)` once per decoded item and
    bumps `parse_errors` for a report it could not decode. Subscribers are held as
    copy-on-write tuples: the reader iterates an immutable snapshot while
    callers replace the tuple under `_lock` — nothing is mutated under the
    reader (rule 5 by structure, not by convention).
    """

    parse_errors: int
    """Reports of this stream dropped because they did not decode (contract
    07 §2) — a report shorter than its header, or a value the decoder refuses.
    Monotonic over the device's life, except where a sensor contract resets
    it (TMF8829 `start()`)."""

    def __init__(self, dev: DepzDevice, *, maxsize: int, what: int | None = None) -> None:
        self._dev = dev
        self._maxsize = maxsize  # default queue size of the iterators
        # the report `next()` names in its DepzTimeoutError; None = no blocking
        # wait is offered (the events stream)
        self._what = what
        self._lock = threading.Lock()
        self._cbs: tuple[tuple[Callable[[T], None], Callable[[T], bool] | None], ...] = ()
        self._queues: tuple[tuple[StreamQueue[T], Callable[[T], bool] | None], ...] = ()
        self.parse_errors = 0

    @property
    def _closed(self) -> bool:
        return self._dev._stream_ended

    # ── consumer side ────────────────────────────────────────────────────────

    def on(
        self,
        cb: Callable[[T], None],
        *,
        where: Callable[[T], bool] | None = None,
    ) -> Callable[[], None]:
        """Subscribe `cb` to every item (`where` keeps only the items it
        accepts). Returns an unsubscribe function; calling it twice is a
        no-op."""
        entry = (cb, where)
        with self._lock:
            self._cbs = (*self._cbs, entry)

        def unsubscribe() -> None:
            with self._lock:
                self._cbs = tuple(e for e in self._cbs if e is not entry)

        return unsubscribe

    def __call__(
        self,
        maxsize: int | None = None,
        *,
        where: Callable[[T], bool] | None = None,
    ) -> StreamIterator[T]:
        """Blocking iterator over the items (bounded, drop-oldest;
        `dropped_count` on the returned iterator). Subscribes immediately —
        items emitted after this call are never missed."""
        return StreamIterator(self, self._maxsize if maxsize is None else maxsize, where)

    def next(self, timeout: float = 2.0) -> T:
        """Wait for the next item.

        Raises `DepzTimeoutError(what, timeout)` when nothing arrives in time.
        As soon as the device ends, before or while waiting, raises
        `LinkClosedError` after the program's own `close()` and
        `DeviceLostError` (carrying the cause) after a loss (contract 07 §3) —
        a caller blocked here is released instead of sitting out the full
        timeout on a link that can never deliver again. An item already queued
        wins over a concurrent close; only an *empty* queue on an ended device
        ends the wait.

        The `events` stream offers no `next` (contract 07 §3: no single
        report to name): there the call is refused with `InvalidStateError`
        before anything is subscribed.
        """
        if self._what is None:
            raise InvalidStateError(
                "the events stream offers no next(): no single report to wait for "
                "(contract 07 §3) — subscribe with events.on(cb) or iterate events()"
            )
        if self._closed:
            raise self._dev._end_error("device is closed")
        q: StreamQueue[T] = StreamQueue(1)
        self._attach(q, None)
        try:
            deadline = time.monotonic() + timeout
            while True:
                remaining = deadline - time.monotonic()
                if remaining <= 0:
                    raise DepzTimeoutError(self._what, timeout) from None
                try:
                    return q.get(timeout=min(_CLOSE_POLL_S, remaining))
                except queue.Empty:
                    if self._closed:
                        raise self._dev._end_error("device closed while waiting")
        finally:
            self._detach(q)

    @property
    def dropped_counts(self) -> list[int]:
        """Drop-oldest counters of all live pull queues (diagnostics)."""
        return [q.dropped_count for q, _ in self._queues]

    # ── owner side (reader thread) ───────────────────────────────────────────

    def _emit(self, item: T) -> None:
        """Hand `item` to every callback, then to every queue (contract 07 §3).
        A callback that raises is counted in `stats.callback_errors`, logged
        and dropped — it never stops the reader, the other callbacks or the
        queues."""
        for cb, where in self._cbs:
            if where is None or where(item):
                if not call_guarded(cb, item):
                    self._dev.stats.callback_errors += 1
        for q, where in self._queues:
            if where is None or where(item):
                q.put(item)

    def _attach(self, q: StreamQueue[T], where: Callable[[T], bool] | None) -> None:
        with self._lock:
            self._queues = (*self._queues, (q, where))

    def _detach(self, q: StreamQueue[T]) -> None:
        with self._lock:
            self._queues = tuple(e for e in self._queues if e[0] is not q)
