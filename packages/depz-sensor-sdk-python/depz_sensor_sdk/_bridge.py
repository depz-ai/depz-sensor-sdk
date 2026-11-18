"""The host side of a board that is a register bridge (contracts
04/10/11/12/13/14), over one open device: `RegisterBridge` — READ_REG /
WRITE_REG split at the bridge's transfer limit, opcodes from the protocol
module's `BridgeCmds` table, limits from the sensor class — and `StreamArm`,
the START_STREAM / STOP_STREAM pair with the "not while streaming" guard."""

from __future__ import annotations

import time
from typing import Callable

from .device import DepzDevice
from .errors import DepzError, InvalidStateError
from .protocol._wire import BridgeCmds


class ShortReplyError(DepzError):
    """REG_DATA carried fewer (or more) bytes than READ_REG asked for."""


class RegisterBridge:
    """READ_REG / WRITE_REG over `dev`, each transfer split at `read_max` /
    `write_max` bytes; `last_timestamp_us` is the MCU time of the latest
    register read (the timestamp of a polled measurement).

    `sticky_addr` names a register the address does not advance past: the
    TMF8829 FIFO port. A read that reaches it stays there; a write stays only
    when it starts there — a write that would run past it is refused by the
    protocol module's `pack_write` (the address is out of range), not folded
    into the FIFO.

    `rd_multi` / `wr_multi` / `sleep_ms` are the same thing under the names the
    ST ULD ports were written against, so the bridge is their platform object;
    `wr_multi` reports the writes it had to split (a blob download) through
    `write_progress`, the per-call `progress` of `write` runs after every chunk."""

    def __init__(
        self,
        dev: DepzDevice,
        *,
        cmds: BridgeCmds,
        read_max: int,
        write_max: int,
        timeout: float | None,
        sticky_addr: int | None = None,
    ) -> None:
        self._dev = dev
        self._cmds = cmds
        self.read_max = read_max
        self.write_max = write_max
        self._timeout = timeout
        self._sticky_addr = sticky_addr
        self.last_timestamp_us = 0
        self.write_progress: Callable[[int, int], None] | None = None


    def read(self, addr: int, length: int) -> bytes:
        c = self._cmds
        out = bytearray()
        while length > 0:
            n = min(length, self.read_max)
            rep = self._dev.request(
                c.read_cmd,
                c.pack_read(addr, n),
                matcher=DepzDevice.expect_report(c.reg_data_rpt, c.unpack),
                timeout=self._timeout,
            )
            if len(rep.data) != n:
                raise ShortReplyError(
                    f"READ_REG 0x{addr:0{2 * c.addr_bytes}X}: expected {n}, got {len(rep.data)}"
                )
            self.last_timestamp_us = rep.timestamp_us
            out += rep.data
            addr += n
            if self._sticky_addr is not None:
                addr = min(addr, self._sticky_addr)
            length -= n
        return bytes(out)

    def write(
        self, addr: int, data: bytes, progress: Callable[[int, int], None] | None = None
    ) -> None:
        c = self._cmds
        data = bytes(data)
        done = 0
        while done < len(data):
            chunk = data[done : done + self.write_max]
            self._dev.request(
                c.write_cmd, c.pack_write(addr, chunk), ok_completes=True, timeout=self._timeout
            )
            done += len(chunk)
            if addr != self._sticky_addr:
                addr += len(chunk)
            if progress is not None:
                progress(done, len(data))

    # ── the ST ULD ports' platform surface ───────────────────────────────────

    rd_multi = read

    def wr_multi(self, addr: int, data: bytes) -> None:
        split = len(data) > self.write_max
        self.write(addr, data, self.write_progress if split else None)

    def sleep_ms(self, ms: int) -> None:
        time.sleep(ms / 1000.0)


class StreamArm:
    """The MCU stream of a bridge board: armed by START_STREAM, released by
    STOP_STREAM. Owns the `active` flag the configuration guard reads;
    `busy` is what `require_idle` says while the stream runs."""

    def __init__(self, dev: DepzDevice, start_cmd: int, stop_cmd: int, *, busy: str) -> None:
        self._dev = dev
        self._start_cmd = start_cmd
        self._stop_cmd = stop_cmd
        self._busy = busy
        self.active = False

    def require_idle(self) -> None:
        # A device lost (or closed) mid-stream keeps `active` set: it says it is
        # lost, as every request of it does (contract 07 §3), not that it streams.
        self._dev._check_open()
        if self.active:
            raise InvalidStateError(self._busy)

    def start(self, payload: bytes = b"", *, undo: Callable[[], None] | None = None) -> None:
        """START_STREAM; `active` once the MCU acked. On any failure `undo`
        runs first (the sensor-side start to take back — otherwise the sensor
        ranges but the host says it does not, and a later stop could never
        reach it); its own error is swallowed, the START_STREAM error is the
        one the caller gets."""
        try:
            self._dev.request(self._start_cmd, payload, ok_completes=True)
        except BaseException:
            if undo is not None:
                try:
                    undo()
                except Exception:
                    pass
            raise
        self.active = True

    def stop(self, *, then: Callable[[], None] | None = None) -> None:
        """STOP_STREAM, then `then` (the sensor-side stop). `active` is
        cleared and `then` runs even when the MCU ack fails (link hiccup):
        otherwise the device is wedged "streaming" and no reconfiguration is
        possible. A no-op while the stream is not armed."""
        if not self.active:
            return
        try:
            self._dev.request(self._stop_cmd, ok_completes=True)
        finally:
            self.active = False
            if then is not None:
                then()
