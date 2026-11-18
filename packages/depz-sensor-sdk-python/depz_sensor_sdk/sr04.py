"""SR04 ultrasonic sensor (contracts/03_SENSOR_SR04.md)."""

from __future__ import annotations

from dataclasses import dataclass
from typing import Any

from .device import DepzDevice, _NoMatch
from .stream import Stream
from .protocol.sr04 import (
    ECHO_TIMEOUT,
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


@dataclass(frozen=True, kw_only=True)
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
    def _from_data(data: Sr04Data) -> "Sr04Measurement | None":
        """None for a source cmd contract 03 §2 does not define: not a
        measurement (it falls through to the hex text event)."""
        source = _SOURCES.get(data.source_cmd)
        if source is None:
            return None
        return Sr04Measurement(
            timestamp_us=data.timestamp_us,
            echo_time_us=data.echo_time_us,
            source=source,
        )


#: RPT_DATA.cmd → measurement source (contract 03 §2, ERRATA E3/B11).
_SOURCES: dict[int, str] = {Sr04Cmd.MEASURE_ONCE: "once", Sr04Cmd.START_MEASUREMENT_LOOP: "loop"}


class Sr04(DepzDevice):
    """HC-SR04 ultrasonic ranging device.

    Measurements stream through `measurements` (callbacks, iterator,
    `next()`); it carries loop samples *and* unsolicited single shots
    triggered by an AUX SYNC_IN edge.
    """

    _record_kind = "sr04"

    measurements: Stream[Sr04Measurement]
    """The measurement stream: `measurements.on(cb)` subscribes (reader-thread
    context; don't block), `measurements(maxsize)` iterates (bounded,
    drop-oldest, subscribes immediately), `measurements.next(timeout)` waits
    for one; `dropped_counts` and `parse_errors` (reports shorter than a
    measurement) are its counters."""

    def _init_subclass_state(self) -> None:
        self.measurements = Stream[Sr04Measurement](self, maxsize=256, what=Sr04Rpt.DATA)

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

        Raises `ValueError` when `period_us` does not fit the u32 wire field
        (the codec checks it before a byte is written).
        """
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
        A value that does not fit the wire field at all raises `ValueError`
        (the codec checks it before a byte is written).
        """
        self.request(Sr04Cmd.SET_ECHO_DECAY, pack_echo_decay(decay_us), ok_completes=True)
        return self.get_echo_decay_us()

    # ── measuring ────────────────────────────────────────────────────────────

    def measure_once(self, timeout: float = 1.0) -> Sr04Measurement:
        """Single shot. Raises BusyError while the loop is running. The reply
        arrives only when the echo completes (or times out at ~65.5 ms), so
        the default timeout is generous."""

        def match(pkt: Packet) -> Sr04Measurement | _NoMatch:
            # the reply is the RPT_DATA sourced by this command; a short one
            # fails the request (InvalidReplyError, contract 07 §3), no timeout
            if pkt.cmd != Sr04Rpt.DATA or pkt.payload[:1] != bytes([Sr04Cmd.MEASURE_ONCE]):
                return DepzDevice.NO_MATCH
            data = Sr04Data.unpack(pkt.payload)
            return Sr04Measurement(
                timestamp_us=data.timestamp_us,
                echo_time_us=data.echo_time_us,
                source="once",
            )

        return self.request(Sr04Cmd.MEASURE_ONCE, matcher=match, timeout=timeout)

    def start(self) -> None:
        """Start the measurement loop (idempotent)."""
        self.request(Sr04Cmd.START_MEASUREMENT_LOOP, ok_completes=True)

    def stop(self) -> None:
        """Stop the measurement loop (idempotent)."""
        self.request(Sr04Cmd.STOP_MEASUREMENT_LOOP, ok_completes=True)

    # ── internal ─────────────────────────────────────────────────────────────

    @property
    def _record_stream(self) -> Stream[Sr04Measurement]:
        return self.measurements

    def _record_value(self, m: Sr04Measurement, *, layers: bool = False) -> dict[str, Any]:
        return {"echo_us": m.echo_time_us, "source": m.source}

    def _handle_report(self, pkt: Packet) -> bool:
        if pkt.cmd != Sr04Rpt.DATA:
            return False
        try:
            # short: refused (ValueError) and counted — contract 07 §2
            m = Sr04Measurement._from_data(Sr04Data.unpack(pkt.payload))
        except ValueError:
            self.measurements.parse_errors += 1
            return True
        if m is None:
            return False
        self.measurements._emit(m)
        return True
