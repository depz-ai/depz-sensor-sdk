"""SR04 ultrasonic sensor (contracts/03_SENSOR_SR04.md)."""

from __future__ import annotations

from dataclasses import dataclass
from typing import Callable

from .device import DeviceBase, StreamIterator, StreamQueue
from .protocol.sr04 import (
    ECHO_DECAY_MAX_US,
    ECHO_DECAY_MIN_US,
    ECHO_DECAY_WIRE_MAX_US,
    ECHO_TIMEOUT,
    SAMPLE_PERIOD_WIRE_MAX_US,
    Sr04Cmd,
    Sr04Data,
    Sr04Rpt,
    distance_mm_from_echo,
    pack_echo_decay,
    pack_sample_period,
    unpack_echo_decay,
    unpack_sample_period,
)
from .transport import Packet


@dataclass(frozen=True)
class Sr04Measurement:
    """One ranging result. `valid` is False for the no-echo timeout."""

    timestamp_us: int
    echo_time_us: int
    source: str  # "once" (host command or SYNC_IN edge) | "loop"

    @property
    def valid(self) -> bool:
        return self.echo_time_us != ECHO_TIMEOUT

    @property
    def distance_mm(self) -> float | None:
        """Distance at 343 m/s, or None when no echo was received."""
        return distance_mm_from_echo(self.echo_time_us)

    def distance_mm_at(self, air_temp_c: float) -> float | None:
        """Distance with temperature-compensated speed of sound."""
        return distance_mm_from_echo(self.echo_time_us, air_temp_c)

    @staticmethod
    def _from_data(data: Sr04Data) -> "Sr04Measurement":
        source = "loop" if data.source_cmd == Sr04Cmd.START_MEASUREMENT_LOOP else "once"
        return Sr04Measurement(data.timestamp_us, data.echo_time_us, source)


class Sr04(DeviceBase):
    """HC-SR04 ultrasonic ranging device.

    Measurements stream via callbacks (`on_measurement`) and/or the pull
    iterator (`stream()`); both receive loop samples *and* unsolicited
    single shots triggered by an AUX SYNC_IN edge.
    """

    def _init_subclass_state(self) -> None:
        self._measure_cbs: list[Callable[[Sr04Measurement], None]] = []
        self._measure_queues: list[StreamQueue] = []

    # ── configuration ────────────────────────────────────────────────────────

    def get_sample_period_us(self) -> int:
        """Configured minimum interval between measurement starts, in µs
        (default 50000). This is the stored value, not the effective rate."""
        return self.request(
            Sr04Cmd.GET_SAMPLE_PERIOD,
            matcher=self.expect_report(Sr04Rpt.SAMPLE_PERIOD, unpack_sample_period),
        )

    def set_sample_period_us(self, period_us: int) -> None:
        """Set the minimum interval between measurement starts, in µs.

        The effective rate is auto-throttled by the echo window (contract 03
        §3) — reading back returns the stored value, not the effective one.

        Raises `ValueError` when `period_us` does not fit the u32 wire field.
        """
        if not 0 <= period_us <= SAMPLE_PERIOD_WIRE_MAX_US:
            raise ValueError(
                f"sample period must be 0..{SAMPLE_PERIOD_WIRE_MAX_US} µs "
                f"(u32 wire field, contract 03 §2): got {period_us}"
            )
        self.request(Sr04Cmd.SET_SAMPLE_PERIOD, pack_sample_period(period_us), ok_completes=True)

    def get_echo_decay_us(self) -> int:
        """Configured post-echo settle pause, in µs (4000–65000)."""
        return self.request(
            Sr04Cmd.GET_ECHO_DECAY,
            matcher=self.expect_report(Sr04Rpt.ECHO_DECAY, unpack_echo_decay),
        )

    def set_echo_decay_us(self, decay_us: int) -> int:
        """Set the settle pause; the device clamps to 4000–65000 µs silently,
        so this re-reads and returns the value actually in effect.

        Values inside the u16 wire field but outside the clamp window are
        legal to send — the device clamps them, which is why this re-reads.
        A value that does not fit the wire field at all is a caller bug and
        raises `ValueError` (rather than letting `struct.error` escape from
        the codec).
        """
        if not 0 <= decay_us <= ECHO_DECAY_WIRE_MAX_US:
            raise ValueError(
                f"echo decay must be 0..{ECHO_DECAY_WIRE_MAX_US} µs "
                f"(u16 wire field; the device then clamps to "
                f"{ECHO_DECAY_MIN_US}..{ECHO_DECAY_MAX_US}, contract 03 §3): got {decay_us}"
            )
        self.request(Sr04Cmd.SET_ECHO_DECAY, pack_echo_decay(decay_us), ok_completes=True)
        return self.get_echo_decay_us()

    # ── measuring ────────────────────────────────────────────────────────────

    def measure_once(self, timeout: float = 1.0) -> Sr04Measurement:
        """Single shot. Raises BusyError while the loop is running. The reply
        arrives only when the echo completes (or times out at ~65.5 ms), so
        the default timeout is generous."""

        def match(pkt: Packet):
            if pkt.cmd != Sr04Rpt.DATA or len(pkt.payload) != 11:
                return DeviceBase.NO_MATCH
            data = Sr04Data.unpack(pkt.payload)
            if data.source_cmd != Sr04Cmd.MEASURE_ONCE:
                return DeviceBase.NO_MATCH
            return Sr04Measurement._from_data(data)

        return self.request(Sr04Cmd.MEASURE_ONCE, matcher=match, timeout=timeout)

    def start(self) -> None:
        """Start the measurement loop (idempotent)."""
        self.request(Sr04Cmd.START_MEASUREMENT_LOOP, ok_completes=True)

    def stop(self) -> None:
        """Stop the measurement loop (idempotent)."""
        self.request(Sr04Cmd.STOP_MEASUREMENT_LOOP, ok_completes=True)

    def on_measurement(self, cb: Callable[[Sr04Measurement], None]) -> Callable[[], None]:
        """Subscribe to measurements (reader-thread context; don't block).
        Returns an unsubscribe function."""
        self._measure_cbs.append(cb)
        return lambda: self._measure_cbs.remove(cb)

    def stream(self, maxsize: int = 256) -> StreamIterator:
        """Blocking iterator over measurements (bounded, drop-oldest;
        `dropped_count` on the returned iterator). Subscribes immediately."""
        return StreamIterator(self._measure_queues, maxsize, lambda: self.closed)

    @property
    def stream_dropped_counts(self) -> list[int]:
        return [q.dropped_count for q in self._measure_queues]

    # ── internal ─────────────────────────────────────────────────────────────

    def _handle_report(self, pkt: Packet) -> bool:
        if pkt.cmd == Sr04Rpt.DATA and len(pkt.payload) == 11:
            m = Sr04Measurement._from_data(Sr04Data.unpack(pkt.payload))
            for cb in list(self._measure_cbs):
                cb(m)
            for q in list(self._measure_queues):
                q.put(m)
            return True
        return False
