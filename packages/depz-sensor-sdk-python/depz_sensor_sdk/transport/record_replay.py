"""`.depzrec` capture and replay links (contracts/08_RECORDING_FORMAT.md)."""

from __future__ import annotations

import json
import time
from pathlib import Path
from typing import IO, Any

from ..errors import LinkClosedError
from .link import Link

SCHEMA = "depz.rec/1"


class RecordingLink(Link):
    """Wraps any Link and tees both directions into a `.depzrec` file.

    TX is journaled **before** the physical write: a fast device reply
    (hundreds of µs) can otherwise reach the reader thread and be journaled
    ahead of its own request, which breaks the causal rx-gating that
    ReplayLink relies on (contract 08)."""

    def __init__(self, inner: Link, path: str | Path, *, header_extra: dict[str, Any] | None = None):
        import threading

        self._inner = inner
        self._file: IO[str] = open(path, "w", encoding="utf-8")
        self._lock = threading.Lock()
        self._t0 = time.monotonic_ns()
        header = {
            "schema": SCHEMA,
            "created_utc": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
        }
        if header_extra:
            header.update(header_extra)
        self._file.write(json.dumps(header, ensure_ascii=False) + "\n")

    def _stamp(self) -> int:
        return (time.monotonic_ns() - self._t0) // 1000

    def _emit(self, direction: str, data: bytes) -> None:
        if data:
            with self._lock:
                self._file.write(
                    json.dumps({"t": self._stamp(), "dir": direction, "data": data.hex()}) + "\n"
                )

    def read(self, timeout: float | None = None) -> bytes:
        data = self._inner.read(timeout)
        self._emit("rx", data)
        return data

    def write(self, data: bytes) -> None:
        self._emit("tx", data)  # journal first — see class docstring
        self._inner.write(data)

    def close(self) -> None:
        try:
            self._file.flush()
            self._file.close()
        finally:
            self._inner.close()

    @property
    def closed(self) -> bool:
        return self._inner.closed


class ReplayLink(Link):
    """Replays the `rx` side of a `.depzrec` capture **causally**: an rx event
    is served only after the host has written at least as many tx bytes as
    preceded that event in the recording. Without this gating a replay would
    deliver responses before the SDK even sends the requests.

    `strict_tx=True` additionally asserts the written bytes match the
    recorded tx stream byte-for-byte (protocol regression mode).
    `realtime=True` paces rx events by their recorded timestamps.
    """

    def __init__(self, path: str | Path, *, realtime: bool = False, strict_tx: bool = False):
        import threading

        lines = Path(path).read_text(encoding="utf-8").splitlines()
        self.header: dict[str, Any] = json.loads(lines[0]) if lines else {}
        self._rx: list[tuple[int, bytes, int]] = []  # (t_us, data, tx_prefix_bytes)
        tx_stream = bytearray()
        for line in lines[1:]:
            if not line.strip():
                continue
            ev = json.loads(line)
            data = bytes.fromhex(ev["data"])
            if ev["dir"] == "rx":
                self._rx.append((ev["t"], data, len(tx_stream)))
            else:
                tx_stream.extend(data)
        self._tx_stream = bytes(tx_stream)
        self._rx_idx = 0
        self._tx_written = 0
        self._realtime = realtime
        self._strict_tx = strict_tx
        self._t0 = time.monotonic_ns()
        self._cv = threading.Condition()
        self._closed = False

    @property
    def exhausted(self) -> bool:
        return self._rx_idx >= len(self._rx)

    def read(self, timeout: float | None = None) -> bytes:
        deadline = time.monotonic() + (timeout if timeout is not None else 0.05)
        with self._cv:
            while True:
                if self._closed:
                    raise LinkClosedError("replay link closed")
                if self.exhausted:
                    return b""
                t_ev, data, tx_prefix = self._rx[self._rx_idx]
                if self._tx_written >= tx_prefix:
                    break
                remaining = deadline - time.monotonic()
                if remaining <= 0:
                    return b""
                self._cv.wait(remaining)
        if self._realtime:
            due_ns = self._t0 + t_ev * 1000
            now = time.monotonic_ns()
            if now < due_ns:
                wait_s = (due_ns - now) / 1e9
                budget = deadline - time.monotonic()
                if wait_s > budget:
                    time.sleep(max(budget, 0))
                    return b""
                time.sleep(wait_s)
        with self._cv:
            self._rx_idx += 1
        return data

    def write(self, data: bytes) -> None:
        with self._cv:
            if self._closed:
                raise LinkClosedError("replay link closed")
            if self._strict_tx:
                expected = self._tx_stream[self._tx_written : self._tx_written + len(data)]
                if expected != bytes(data):
                    raise AssertionError(
                        f"replay strict_tx mismatch at offset {self._tx_written}: "
                        f"expected {expected.hex()}, got {data.hex()}"
                    )
            self._tx_written += len(data)
            self._cv.notify_all()

    def close(self) -> None:
        with self._cv:
            self._closed = True
            self._cv.notify_all()

    @property
    def closed(self) -> bool:
        return self._closed
