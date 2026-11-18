"""Byte-link abstraction: the seam between framing and I/O backends.

Real serial ports, loopback pairs (tests), and record/replay wrappers all
implement `Link`. Device classes accept any `Link`, which keeps every layer
above the serial port hardware-free testable.
"""

from __future__ import annotations

import threading
import time
from abc import ABC, abstractmethod
from collections import deque

from .._waits import wait_limit
from ..errors import LinkClosedError


class Link(ABC):
    """Blocking byte pipe.

    How the SDK calls a link (contract 07 §3, "The host-supplied link") — what
    a link given to `open_on_link` or a device constructor has to cope with:

    - `read` runs only on the device's own daemon thread
      `depz-reader-<link.port>` (`<link>` when the link has no `port`), one
      call at a time, back to back, in short slices: `timeout` is 0.05 s today,
      the SDK's choice, and may change. A device never passes None.
    - `write` runs on the thread of the call that sends (any program thread;
      the reader thread when a callback sends), concurrently with a blocked
      `read`. The SDK serializes the writes of a device: one at a time, one
      whole frame per call.
    - `write` writes everything or raises, within a bounded time. The SDK looks
      at no count, does not retry and passes no timeout: the link picks one. A
      `write` that never returns holds the device's write lock, so every later
      request of that device blocks too, whatever its timeout.
    - `close` runs on the thread that closes the device, possibly during a
      blocked `read` or a `write`, and after a loss on the reader thread and
      then again from the program's own `close()`: it must be idempotent. It
      must make a blocked `read` return or raise promptly, and so must a `read`
      that starts just after it. The device waits up to 2 s for the reader,
      then returns anyway and leaves that thread in the `read`.
    - `closed` is read from any thread.
    - What `read` raises is a loss unless the program is closing the device:
      the device closes, every later request fails with `DeviceLostError`.
      Meant for that: `LinkClosedError`, `DeviceLostError`. Anything else is a
      loss too and is logged as the reader's failure, an `OSError` included (a
      link translates its errors). What is no `Exception` (`SystemExit`) closes
      the device and goes on to the thread's end.
    - What `write` raises fails only that call: the device ends when `read`
      reports the loss. `LinkClosedError` while the program has not closed the
      device reaches the caller as `DeviceLostError`; anything else as it is.
    - What `close` raises reaches the program from its own `close()` (the
      device still ends); on the reader's path it is logged and dropped.

    `BootloaderDevice` has no reader thread: on a link it calls `read` (0.05 s)
    and `write` on the caller's thread, one at a time.
    """

    @abstractmethod
    def read(self, timeout: float | None = None) -> bytes:
        """Return the next available chunk (any size ≥1), or b"" on timeout."""

    @abstractmethod
    def write(self, data: bytes) -> None: ...

    @abstractmethod
    def close(self) -> None: ...

    @property
    @abstractmethod
    def closed(self) -> bool: ...


class LoopbackLink(Link):
    """In-memory link; `peer` sees what we write and vice versa.

    Create with `LoopbackLink.pair()`.
    """

    def __init__(self) -> None:
        self._rx: deque[bytes] = deque()
        self._cv = threading.Condition()
        self._closed = False
        self.peer: "LoopbackLink" | None = None

    @classmethod
    def pair(cls) -> tuple["LoopbackLink", "LoopbackLink"]:
        a, b = cls(), cls()
        a.peer, b.peer = b, a
        return a, b

    def read(self, timeout: float | None = None) -> bytes:
        deadline = None if timeout is None else time.monotonic() + timeout
        with self._cv:
            while not self._rx:
                if self._closed:
                    raise LinkClosedError("loopback link closed")
                if deadline is None:
                    self._cv.wait()
                    continue
                left = deadline - time.monotonic()
                if left <= 0:
                    return b""
                self._cv.wait(wait_limit(left))
            return self._rx.popleft()

    def write(self, data: bytes) -> None:
        if self._closed:
            raise LinkClosedError("loopback link closed")
        assert self.peer is not None
        self.peer._deliver(bytes(data))

    def _deliver(self, data: bytes) -> None:
        with self._cv:
            self._rx.append(data)
            self._cv.notify_all()

    def close(self) -> None:
        for side in (self, self.peer):
            if side is not None:
                with side._cv:
                    side._closed = True
                    side._cv.notify_all()

    @property
    def closed(self) -> bool:
        return self._closed
