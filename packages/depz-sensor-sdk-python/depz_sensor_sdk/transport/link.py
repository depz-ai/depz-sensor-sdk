"""Byte-link abstraction: the seam between framing and I/O backends.

Real serial ports, loopback pairs (tests), and record/replay wrappers all
implement `Link`. Device classes accept any `Link`, which keeps every layer
above the serial port hardware-free testable.
"""

from __future__ import annotations

import threading
from abc import ABC, abstractmethod
from collections import deque

from ..errors import LinkClosedError


class Link(ABC):
    """Blocking byte pipe."""

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
        with self._cv:
            if not self._rx:
                self._cv.wait(timeout)
            if self._closed and not self._rx:
                raise LinkClosedError("loopback link closed")
            return self._rx.popleft() if self._rx else b""

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
