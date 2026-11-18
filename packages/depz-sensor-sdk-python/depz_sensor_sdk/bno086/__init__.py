"""BNO086 9-axis IMU: SHTP + SH-2 host stack over the firmware pass-through
bridge (contracts/05_SENSOR_BNO086.md).

The MCU only shuttles raw SHTP frames (SEND_SHTP_PACKET / RPT_DATA); the
whole sensor-hub protocol runs here. Per ERRATA E2 the bridge ACKs every
send with RPT_STATUS immediately and *all* inbound SHTP arrives as
RPT_DATA(cmd=0x00) — correlation happens at the SH-2 layer only.
"""

from __future__ import annotations

import queue
import struct
import threading
import time
import warnings
from contextlib import contextmanager
from dataclasses import dataclass
from typing import Any, Callable, Iterable, Iterator, NoReturn, Protocol, TypeVar

from .._waits import wait_limit
from ..device import DepzDevice
from ..stream import Stream
from ..errors import BusyError, DepzTimeoutError, InvalidReplyError
from ..protocol.bno086 import BUSY_BACKOFF_S, Bno086Cmd, Bno086Data, Bno086Rpt
from ..transport import Packet
from .reports import (
    FLUSH_COMPLETED,
    Acceleration,
    FlushCompleted,
    GyroIntegratedRV,
    Gyroscope,
    InputReport,
    Magnetometer,
    MotionRequest,
    Report,
    RotationVector,
    SensorId,
    bno086_record_value,
    parse_gyro_rv_cargo,
    parse_input_cargo,
)
from .sh2 import (
    CommandResponse,
    ControlReport,
    Counts,
    ErrorRecord,
    ErrorSource,
    FeatureFlag,
    FeatureResponse,
    FrsReadResponse,
    FrsReadSession,
    FrsWriteResponse,
    FrsWriteSession,
    ME_CAL_GET,
    METADATA_RECORDS,
    MotionIntent,
    OscillatorType,
    ProductId,
    SensorMetadata,
    Sh2Command,
    Sh2Error,
    TareAxis,
    TareBasis,
    build_command_request,
    build_force_flush,
    build_get_feature_request,
    build_product_id_request,
    split_control_cargo,
    build_set_feature,
    counts_clear_params,
    counts_get_params,
    errors_params,
    interactive_calibration_params,
    me_calibration_params,
    periodic_dcd_params,
    persist_tare_params,
    set_reorientation_params,
    tare_now_params,
)
from .shtp import ExecutableCommand, ShtpCargo, ShtpChannel, ShtpRx, ShtpTx

__all__ = [
    "Bno086",
    "SensorId",
    "Report",
    "InputReport",
    "Acceleration",
    "Gyroscope",
    "Magnetometer",
    "RotationVector",
    "GyroIntegratedRV",
    "MotionRequest",
    "MotionIntent",
    "FlushCompleted",
    "FeatureResponse",
    "FeatureFlag",
    "ExecutableCommand",
    "ProductId",
    "CommandResponse",
    "SensorMetadata",
    "TareAxis",
    "TareBasis",
    "Sh2Error",
    "CalibrationConfig",
    "OscillatorType",
    "ErrorRecord",
    "ErrorSource",
    "Counts",
]

# Rate verification bounds (contract 05 §7): the hub grants a grid rate; a
# result outside [0.9, 2.1]× the requested rate is worth a warning, never an
# error.
RATE_LOW_FACTOR = 0.9
RATE_HIGH_FACTOR = 2.1

#: Re-reads allowed while `enable()`'s verify reads a rate outside the band:
#: a "disabled" response left over from a preceding `disable()`, or the hub
#: answering before it runs the new rate — the lab BNO085 (SH-2 3.2.13)
#: answers interval 0 for one or two read-backs right after Set Feature
#: (capture 2026-09-29), and a hub just reset for much longer (bench
#: 2026-11-12, a BNO085 and a BNO086 asked for 100 Hz: the gyroscope read 0
#: for 95 ms, the magnetometer 0 or 50 Hz for 48 ms; both then ran at 100 Hz).
#: Fifteen, `_VERIFY_PAUSE_S` apart — about 0.4 s with the round trips, four
#: times the longest seen — before a genuine refusal reaches the warning.
_VERIFY_REREADS = 15
_VERIFY_PAUSE_S = 0.02


def _rate_ok(requested_interval_us: int, granted_interval_us: int) -> bool:
    """Whether a granted report interval is the rate asked for: within
    [RATE_LOW_FACTOR, RATE_HIGH_FACTOR] × the requested rate (contract 05 §7).
    An interval of 0 — the sensor disabled — never is."""
    requested_rate = 1_000_000 / requested_interval_us
    actual_rate = 1_000_000 / granted_interval_us if granted_interval_us else 0.0
    return RATE_LOW_FACTOR * requested_rate <= actual_rate <= RATE_HIGH_FACTOR * requested_rate

_EXECUTABLE_RESET_COMPLETE = 0x01


@dataclass(frozen=True, kw_only=True)
class CalibrationConfig:
    """ME calibration enables as reported by the sensor."""

    accel: bool
    gyro: bool
    mag: bool
    planar: bool


_R = TypeVar("_R")


def _decode_reply(rid: int, raw: bytes, unpack: Callable[[bytes], _R]) -> _R:
    """A control response the hub answered a request with, decoded; one
    shorter than its fields fails that request with InvalidReplyError
    (contract 07 §3), never a bare error."""
    try:
        return unpack(raw)
    except ValueError as bad:
        raise InvalidReplyError(rid, bad) from None


class _Take(Protocol):
    def __call__(self, limit: float | None = None) -> bytes: ...


class _Waiters:
    """Who is waiting for which SH-2 answer.

    The reader posts every control report under its report id (and every
    Flush Completed under (0xEF, sensor)); a caller listens for one key for
    the span of a `listen` block. One lock guards the table; the queues are
    thread-safe on their own."""

    def __init__(self) -> None:
        self._lock = threading.Lock()
        self._queues: dict[
            tuple[int, int | None], list[queue.SimpleQueue[bytes | Exception]]
        ] = {}
        # Set once the device is closed or lost: every wait, running or
        # started later, ends with it (contract 07 §3).
        self.failure: Exception | None = None

    def post(self, rid: int, raw: bytes, *, sub: int | None = None) -> None:
        with self._lock:
            waiting = list(self._queues.get((rid, sub), ()))
        for q in waiting:
            q.put(raw)

    def fail_all(self, exc: Exception) -> None:
        """The device closed or was lost: release every waiter with `exc`,
        and refuse every later wait with it."""
        with self._lock:
            if self.failure is None:  # the first cause wins: lost, then "closed"
                self.failure = exc
            waiting = [q for queues in self._queues.values() for q in queues]
        for q in waiting:
            q.put(exc)

    def _raise(self, exc: Exception) -> NoReturn:
        # A fresh instance per waiter: several threads may be released by one
        # failure, and an exception object carries its own traceback.
        raise type(exc)(*exc.args) from exc.__cause__

    @contextmanager
    def listen(
        self, rid: int, timeout: float, *, sub: int | None = None
    ) -> Iterator[_Take]:
        """Listen for report `rid` (narrowed by `sub`) while the block runs.

        Yields `take(limit=None)`: the next matching raw report, waited for
        at most `limit` s — by default what is left of `timeout`, counted
        from the first take (the request has been sent by then) — else
        DepzTimeoutError(rid, …). Leaving the block unregisters, so a late
        answer never satisfies a later request."""
        key = (rid, sub)
        q: queue.SimpleQueue[bytes | Exception] = queue.SimpleQueue()
        deadline: float | None = None

        def take(limit: float | None = None) -> bytes:
            nonlocal deadline
            if deadline is None:
                deadline = time.monotonic() + timeout
            wait = max(0.0, deadline - time.monotonic()) if limit is None else limit
            try:
                item = q.get(timeout=wait_limit(wait))
            except queue.Empty:
                raise DepzTimeoutError(rid, timeout if limit is None else limit) from None
            if isinstance(item, Exception):
                self._raise(item)
            return item

        with self._lock:
            if self.failure is not None:
                self._raise(self.failure)
            self._queues.setdefault(key, []).append(q)
        try:
            yield take
        finally:
            with self._lock:
                self._queues[key].remove(q)
                if not self._queues[key]:
                    del self._queues[key]


class Bno086(DepzDevice):
    """BNO086 device: enable SH-2 sensors, stream typed reports.

    Typical use::

        with Bno086(port) as imu:
            imu.enable(SensorId.ROTATION_VECTOR, hz=100)
            for r in imu.reports():
                print(r.i, r.j, r.k, r.real)

    Callbacks run on the reader thread — never call blocking device methods
    (enable/tare/...) from inside one."""

    busy_retries = 5  # SEND_SHTP_PACKET attempts before giving up
    busy_backoff_s = BUSY_BACKOFF_S  # >= 200 ms per the bridge spec
    _record_kind = "bno086"

    reports: Stream[Report]
    """The typed-report stream: `reports.on(cb, where=...)` subscribes
    (reader-thread context; don't block), `reports(maxsize, where=...)`
    iterates (bounded, drop-oldest, subscribes eagerly — reports emitted
    after the call are never missed), `reports.next(timeout)` waits for one;
    `where` keeps the reports it accepts, e.g.
    ``lambda r: r.sensor_id == SensorId.ROTATION_VECTOR``. `dropped_counts`
    and `parse_errors` (DATA reports shorter than their header) are its
    counters."""

    def _init_subclass_state(self) -> None:
        self._rx = ShtpRx()  # the reader's; a reset swaps in a fresh one
        self._tx = ShtpTx()
        self._shtp_lock = threading.Lock()  # one host frame at a time: seq + SEND_SHTP_PACKET retries
        self.reports = Stream[Report](self, maxsize=1024, what=Bno086Rpt.DATA)
        self._waiters = _Waiters()
        self._cmd_seq = 0
        self._reset_event = threading.Event()
        self._advertisement = bytearray()
        self._features: dict[int, FeatureResponse] = {}

    def _fail_all_pending(self, exc: Exception) -> None:
        """The reader is ending (close(), or the link dropped): the SH-2
        waits end with the same error as the bridge requests, at once
        (contract 07 §3) — not after their own 1–5 s timeout."""
        super()._fail_all_pending(exc)
        self._waiters.fail_all(exc)
        self._reset_event.set()  # a reset wait wakes and finds `failure`

    # ── RX path ──────────────────────────────────────────────────────────────

    def _handle_report(self, pkt: Packet) -> bool:
        if pkt.cmd != Bno086Rpt.DATA:
            return False
        try:
            # short header: refused (ValueError) and counted — contract 07 §2
            data = Bno086Data.unpack(pkt.payload)
        except ValueError:
            self.reports.parse_errors += 1
            return True
        cargo = self._rx.feed(data.shtp, data.timestamp_us)
        if cargo is not None:
            # The first fragment's capture: 0xFB base deltas refer to it.
            self._dispatch_cargo(cargo, cargo.capture_us)
        return True

    def _dispatch_cargo(self, cargo: ShtpCargo, capture_us: int) -> None:
        ch = cargo.channel
        if ch == ShtpChannel.COMMAND:
            self._advertisement.extend(cargo.payload)
            return
        if ch == ShtpChannel.EXECUTABLE:
            if cargo.payload[:1] == bytes([_EXECUTABLE_RESET_COMPLETE]):
                self._reset_event.set()
            return
        if ch == ShtpChannel.CONTROL:
            # One cargo may hold several responses back to back.
            for report in split_control_cargo(cargo.payload):
                rid = report[0]
                if rid == ControlReport.GET_FEATURE_RESPONSE and len(report) >= 17:
                    resp = FeatureResponse.unpack(report)
                    self._features[resp.sensor_id] = resp
                self._waiters.post(rid, report)
            return
        if ch in (ShtpChannel.INPUT_NORMAL, ShtpChannel.INPUT_WAKE):
            for rep in parse_input_cargo(cargo.payload, capture_us):
                self._emit_report(rep)
            return
        if ch == ShtpChannel.GYRO_RV:
            gyro_rv = parse_gyro_rv_cargo(cargo.payload, capture_us)
            if gyro_rv is not None:
                self._emit_report(gyro_rv)

    def _emit_report(self, rep: Report) -> None:
        if isinstance(rep, FlushCompleted):
            self._waiters.post(FLUSH_COMPLETED, bytes((FLUSH_COMPLETED, rep.flushed_sensor)),
                               sub=rep.flushed_sensor)
        self.reports._emit(rep)

    # ── TX path ──────────────────────────────────────────────────────────────

    def _send_shtp(self, channel: int, payload: bytes) -> None:
        """Frame `payload` and push it through SEND_SHTP_PACKET.

        ERR_BUSY (both MCU TX slots full) backs off `busy_backoff_s` and
        retransmits, up to `busy_retries` attempts (contract 05 §2).

        The lock is held across the retries on purpose: the SHTP sequence
        number is taken when the frame is built, so letting another sender in
        during the back-off would put frame n+1 on the wire before frame n —
        and with both MCU TX slots full that sender would only get BUSY too."""
        with self._shtp_lock:
            frame = self._tx.next_frame(channel, payload)
            for attempt in range(self.busy_retries):
                try:
                    self.request(Bno086Cmd.SEND_SHTP_PACKET, frame, ok_completes=True)
                    return
                except BusyError:
                    if attempt == self.busy_retries - 1:
                        raise
                    time.sleep(self.busy_backoff_s)

    def _control_request(
        self,
        payload: bytes,
        expect_rid: int,
        parse: Callable[[bytes], _R],
        pred: Callable[[_R], bool] | None = None,
        timeout: float = 1.0,
    ) -> _R:
        """Send a control-channel payload and wait for a matching response.

        Correlation is purely SH-2-level: `expect_rid` selects the response
        report ID, `pred` (on the parsed object) narrows further (sensor id,
        command seq, ...)."""

        def step(raw: bytes) -> _R | None:
            obj = _decode_reply(expect_rid, raw, parse)
            return obj if pred is None or pred(obj) else None

        return self._await(expect_rid, payload, step, timeout)

    def _await(
        self,
        rid: int,
        payload: bytes,
        step: Callable[[bytes], _R | None],
        timeout: float,
        *,
        sub: int | None = None,
    ) -> _R:
        """The one way to ask the hub and wait: listen for report `rid`
        (narrowed by `sub`), send `payload` on the control channel, then
        hand every raw report that arrives to `step` until it returns the
        answer — None means "not ours" or "not complete yet" — or `timeout`
        runs out (DepzTimeoutError(rid, timeout)). A reply that does not
        decode is `step`'s InvalidReplyError; it ends the wait like any
        other error."""
        with self._waiters.listen(rid, timeout, sub=sub) as take:
            self._send_shtp(ShtpChannel.CONTROL, payload)
            while (result := step(take())) is None:
                pass
            return result

    def _next_cmd_seq(self) -> int:
        seq = self._cmd_seq
        self._cmd_seq = (seq + 1) & 0xFF
        return seq

    def _command(
        self, command: int, params: bytes, *, wait_response: bool, timeout: float = 1.0
    ) -> CommandResponse | None:
        """SH-2 Command Request; correlates the response on (command,
        command_seq) when the command produces one."""
        seq = self._next_cmd_seq()
        payload = build_command_request(seq, command, params)
        if not wait_response:
            self._send_shtp(ShtpChannel.CONTROL, payload)
            return None
        return self._control_request(
            payload,
            ControlReport.COMMAND_RESPONSE,
            CommandResponse.unpack,
            pred=lambda r: r.command == command and r.command_seq == seq,
            timeout=timeout,
        )

    def _command_collect(
        self,
        command: int,
        params: bytes,
        collect: Callable[[CommandResponse, list[CommandResponse]], bool],
        timeout: float = 1.0,
    ) -> list[CommandResponse]:
        """Send a Command Request and gather every Command Response correlated
        on (command, command_seq) until `collect(resp, acc)` returns True.

        Used by multi-message commands (Errors, Counts) where the hub streams
        several 0xF1 responses with an incrementing response_seq."""
        seq = self._next_cmd_seq()
        payload = build_command_request(seq, command, params)
        rid = ControlReport.COMMAND_RESPONSE
        results: list[CommandResponse] = []

        def step(raw: bytes) -> list[CommandResponse] | None:
            resp = _decode_reply(rid, raw, CommandResponse.unpack)
            if resp.command != command or resp.command_seq != seq:
                return None
            return results if collect(resp, results) else None

        return self._await(rid, payload, step, timeout)

    # ── lifecycle ────────────────────────────────────────────────────────────

    def hardware_reset(self, timeout: float = 2.0) -> None:
        """Hard-reset the sensor via nRST (0x32). All SHTP state (seq
        counters, partial cargos) and cached features restart from zero.

        The hub's RPT_STATUS OK ack for the 0x32 command is treated as the
        reset confirmation. The SH-2 executable-channel reset-complete (which
        the BNO08X SH-2 spec would emit) is only waited for best-effort:
        older firmware (≤ v0.95) did **not** emit it (ERRATA E9, now fixed
        in newer firmware); the best-effort wait
        handles both, so its absence is *not* an error — the sensor is fully
        usable without it.
        """
        self._begin_reset()
        # RPT_STATUS OK from the hub confirms the reset command was accepted.
        self.request(Bno086Cmd.SENSOR_RESET, ok_completes=True, timeout=timeout)
        # Best-effort wait for the SH-2 executable reset-complete; older
        # firmware (≤ v0.95) legitimately never sent it (ERRATA E9, now fixed
        # in newer firmware) — the best-effort wait handles both, so do not
        # hard-fail when it is missing.
        self._reset_event.wait(min(timeout, 0.5))

    def _begin_reset(self) -> None:
        """Forget what the hub told us — it is about to restart: SHTP seq
        counters and partial cargos, the advertisement, the cached features."""
        self._reset_event.clear()
        self._rx = ShtpRx()
        with self._shtp_lock:
            self._tx.reset()
        self._advertisement = bytearray()
        self._features.clear()

    def wake(self) -> None:
        """Pulse WAKE (PS0): wakes the sensor from sleep, no state loss.

        This is the board's hardware pin pulse (board command 0x33), not an
        SH-2 command; the SH-2 "On" command that ends `sleep()` is
        `power_on()`."""
        self.request(Bno086Cmd.SENSOR_WAKE_UP, ok_completes=True)

    def sleep(self) -> None:
        """Put the hub to sleep: SH-2 "Sleep" (0x03 on SHTP channel 1).

        Every report stops, except those enabled with
        ``flags=FeatureFlag.WAKEUP | FeatureFlag.ALWAYS_ON``, which keep their
        rate; the configuration (enabled sensors, rates) is kept. The hub
        sends no response. `power_on()` ends it.

        On a DEPZ BNO086 (SH-2 3.12.6, 2026-10-16): all reports stopped
        within one sample; a WAKEUP|ALWAYS_ON report kept streaming at full
        rate on channel 4."""
        self._send_shtp(ShtpChannel.EXECUTABLE, bytes([ExecutableCommand.SLEEP]))

    def power_on(self) -> None:
        """End `sleep()`: SH-2 "On" (0x02 on SHTP channel 1).

        Every report that was enabled comes back at its rate — no need to
        re-enable. The hub sends no response. Not to be confused with
        `wake()`, which pulses the WAKE pin (board command 0x33) instead of
        sending an SH-2 command."""
        self._send_shtp(ShtpChannel.EXECUTABLE, bytes([ExecutableCommand.ON]))

    @property
    def advertisement(self) -> bytes:
        """Raw SHTP channel-0 advertisement bytes seen since open/reset."""
        return bytes(self._advertisement)

    # ── SH-2: identification & features ─────────────────────────────────────

    def product_id(self, timeout: float = 1.0) -> ProductId:
        """The SH-2 application's Product ID — the first of `product_ids()`."""
        return self.product_ids(timeout=timeout)[0]

    def product_ids(self, timeout: float = 1.0, settle: float = 0.15) -> list[ProductId]:
        """Product ID Request → every component's response, in the sensor's
        order (the SH-2 application first). Collects until no response came
        for `settle` s, so the late ones can't answer the next request (they
        arrive in a second cargo; taking only the first once returned the
        bootloader's version on a back-to-back call)."""
        rid = ControlReport.PRODUCT_ID_RESPONSE
        with self._waiters.listen(rid, timeout) as take:
            self._send_shtp(ShtpChannel.CONTROL, build_product_id_request())
            out = [_decode_reply(rid, take(), ProductId.unpack)]
            while True:
                try:
                    raw = take(settle)
                except DepzTimeoutError:
                    return out
                out.append(_decode_reply(rid, raw, ProductId.unpack))

    def enable(
        self,
        sensor: int,
        hz: float | None = None,
        *,
        interval_us: int | None = None,
        batch_us: int = 0,
        sensitivity: int = 0,
        flags: int = 0,
        cfg_word: int = 0,
        verify: bool = True,
        timeout: float = 1.0,
    ) -> FeatureResponse | None:
        """Enable `sensor` at the requested rate via Set Feature (0xFD).

        Give either `hz` or `interval_us`. The hub rounds to its 1 kHz/2^n
        grid; with `verify` the granted rate is read back via Get Feature and
        a result outside 0.9–2.1× the request emits a UserWarning (contract
        05 §7 — warn, never raise). Returns the FeatureResponse (None when
        `verify=False`)."""
        if (hz is None) == (interval_us is None):
            raise ValueError("give exactly one of hz / interval_us")
        if hz is not None:
            if hz <= 0:
                raise ValueError("hz must be positive; use disable()")
            interval_us = max(1, round(1_000_000 / hz))
        assert interval_us is not None
        self._send_shtp(
            ShtpChannel.CONTROL,
            build_set_feature(sensor, interval_us, batch_us, sensitivity, flags, cfg_word),
        )
        if not verify:
            return None
        resp = self._verify_feature(sensor, interval_us, timeout)
        if not _rate_ok(interval_us, resp.interval_us):
            requested_rate = 1_000_000 / interval_us
            actual_rate = 1_000_000 / resp.interval_us if resp.interval_us else 0.0
            warnings.warn(
                f"BNO086 sensor 0x{sensor:02X}: requested {requested_rate:.1f} Hz, "
                f"granted {actual_rate:.1f} Hz (outside {RATE_LOW_FACTOR}–"
                f"{RATE_HIGH_FACTOR}× band)",
                stacklevel=2,
            )
        return resp

    def _verify_feature(self, sensor: int, interval_us: int, timeout: float) -> FeatureResponse:
        """Read back a feature we just *enabled*, until the hub runs it.

        The hub emits an **unsolicited** Get Feature Response for every
        state-changing Set Feature — including the `disable()` that typically
        precedes a re-enable — and SH-2 gives Get Feature Response no
        correlation token (contract 05 §6: only `sensorId` identifies it). So
        the GFR(interval=0) produced by a preceding `disable()` can still be in
        flight when this read registers, and would otherwise be reported as
        "granted 0 Hz" for a sensor that is in fact streaming.

        The hub also answers before it runs the rate just set: interval 0 for
        one or two read-backs on a hub that has been up for a while, and for
        up to 95 ms — interval 0, or a slower rate — on one just reset
        (`_VERIFY_REREADS`).

        So a response outside the band of the rate asked for is stale, early
        or a genuine refusal. Re-read a bounded number of times, a pause
        before each: the real answer follows, while a genuine refusal survives
        the re-reads and still reaches the caller's warning path (contract 05
        §7: warn, never raise).
        """
        resp = self.get_feature(sensor, timeout=timeout)
        for _ in range(_VERIFY_REREADS):
            if _rate_ok(interval_us, resp.interval_us):
                break
            time.sleep(_VERIFY_PAUSE_S)
            resp = self.get_feature(sensor, timeout=timeout)
        return resp

    def disable(self, sensor: int) -> None:
        """Disable `sensor` (Set Feature with interval 0)."""
        self._send_shtp(ShtpChannel.CONTROL, build_set_feature(sensor, 0))
        self._features.pop(int(sensor), None)

    def get_feature(self, sensor: int, timeout: float = 1.0) -> FeatureResponse:
        """Get Feature Request/Response round trip for `sensor`."""
        return self._control_request(
            build_get_feature_request(sensor),
            ControlReport.GET_FEATURE_RESPONSE,
            FeatureResponse.unpack,
            pred=lambda r: r.sensor_id == sensor,
            timeout=timeout,
        )

    # ── report streaming ─────────────────────────────────────────────────────

    @property
    def _record_stream(self) -> Stream[Report]:
        return self.reports

    def _record_value(self, r: Report, *, layers: bool = False) -> dict[str, Any]:
        """Contract 09 `bno086`: every enabled sensor's report, snake_case, full
        quaternion / vector components, at its measurement time (0xFB/0xFA +
        delay) like every other sensor. Reports of different sensors in one
        cargo carry out-of-order device times, so `t` may step back within a
        device; readers sort."""
        return bno086_record_value(r)

    # ── tare / calibration facade ────────────────────────────────────────────

    def tare_now(
        self,
        axes: int = TareAxis.ALL,
        basis: int = TareBasis.ROTATION_VECTOR,
    ) -> None:
        """Tare the selected axes against `basis` (no response per SH-2)."""
        self._command(Sh2Command.TARE, tare_now_params(axes, basis), wait_response=False)

    def persist_tare(self) -> None:
        """Persist the current tare into FRS (no response per SH-2)."""
        self._command(Sh2Command.TARE, persist_tare_params(), wait_response=False)

    def set_reorientation(self, x: float, y: float, z: float, w: float) -> None:
        """Set the runtime reorientation quaternion (Q14 on the wire; all
        zeros clears). No response per SH-2.

        The rotation is applied on the board's side (new = report * r) and is
        given in the hub's own axes: x and y swapped and z negated relative
        to the axes of the reports — a rotation about x here turns the
        reports about y (bench, 2026-10-22, SH-2 3.12.6). The System
        Orientation record (FRS 0x2D3E) uses the same axes and is read only
        when the hub starts."""
        self._command(
            Sh2Command.TARE, set_reorientation_params(x, y, z, w), wait_response=False
        )

    def set_calibration(
        self,
        accel: bool = True,
        gyro: bool = True,
        mag: bool = True,
        planar: bool = False,
        timeout: float = 1.0,
    ) -> None:
        """Configure ME calibration; raises Sh2Error on non-zero status."""
        resp = self._command(
            Sh2Command.ME_CALIBRATE,
            me_calibration_params(accel, gyro, mag, planar),
            wait_response=True,
            timeout=timeout,
        )
        assert resp is not None
        if resp.status != 0:
            raise Sh2Error(f"ME calibration configure failed: status {resp.status}")

    def get_calibration(self, timeout: float = 1.0) -> CalibrationConfig:
        """Read back which ME calibrations are running."""
        resp = self._command(
            Sh2Command.ME_CALIBRATE,
            me_calibration_params(False, False, False, subcommand=ME_CAL_GET),
            wait_response=True,
            timeout=timeout,
        )
        assert resp is not None
        if resp.status != 0:
            raise Sh2Error(f"ME calibration get failed: status {resp.status}")
        r = resp.r
        return CalibrationConfig(
            accel=bool(r[1]),
            gyro=bool(r[2]),
            mag=bool(r[3]),
            planar=bool(r[4]),
        )

    def save_dcd(self, timeout: float = 1.0) -> None:
        """Save the dynamic calibration data to flash (DCD Save Now)."""
        resp = self._command(Sh2Command.SAVE_DCD, b"", wait_response=True, timeout=timeout)
        assert resp is not None
        if resp.status != 0:
            raise Sh2Error(f"DCD save failed: status {resp.status}")

    def configure_periodic_dcd(self, enable: bool) -> None:
        """Enable/disable the hub's periodic DCD autosave (no response)."""
        self._command(
            Sh2Command.PERIODIC_DCD_CONFIG, periodic_dcd_params(enable), wait_response=False
        )

    # ── interactive calibration (BNO086 only) ────────────────────────────────

    def set_motion_intent(self, intent: int) -> None:
        """Tell the hub how the device is about to move (Interactive
        Calibration, command 0x0E, see MotionIntent), so it can refine the
        gyro zero-rate offset while the device is still. No response per
        SH-2; a BNO085 ignores it. The hub's answer, if it wants the device to
        stop, arrives as a MotionRequest report (enable SensorId.MOTION_REQUEST)."""
        self._command(
            Sh2Command.INTERACTIVE_CALIBRATION,
            interactive_calibration_params(intent),
            wait_response=False,
        )

    def enable_motion_request(self, hz: float = 1, **kw: Any) -> FeatureResponse | None:
        """Enable MotionRequest (0x2B) reports. The hub sends one when its
        request changes, not at `hz`; it reads the rate back late, so the
        granted rate is not checked."""
        kw.setdefault("verify", False)
        return self.enable(SensorId.MOTION_REQUEST, hz, **kw)

    # ── batching ─────────────────────────────────────────────────────────────

    def flush(self, sensor: int, timeout: float = 1.0) -> None:
        """Force Sensor Flush (0xF0): have the hub send `sensor`'s batched
        reports now instead of when its batch interval runs out. Returns once
        the hub's Flush Completed (0xEF) for `sensor` arrives — every report
        batched before the call has been emitted by then."""
        self._await(FLUSH_COMPLETED, build_force_flush(sensor), lambda raw: True, timeout,
                    sub=int(sensor))

    # ── FRS ──────────────────────────────────────────────────────────────────

    def frs_read(self, record_id: int, timeout: float = 2.0) -> tuple[int, ...]:
        """Read a whole FRS record; returns its 32-bit words."""
        session = FrsReadSession(frs_type=record_id)
        rid = ControlReport.FRS_READ_RESPONSE

        def step(raw: bytes) -> tuple[int, ...] | None:
            session.feed(_decode_reply(rid, raw, FrsReadResponse.unpack))
            return tuple(session.words) if session.done else None

        return self._await(rid, session.request(), step, timeout)

    def frs_write(self, record_id: int, words: Iterable[int], timeout: float = 2.0) -> None:
        """Write a whole FRS record (word list); raises Sh2Error on failure."""
        session = FrsWriteSession(frs_type=record_id, words=list(words))
        rid = ControlReport.FRS_WRITE_RESPONSE

        def step(raw: bytes) -> bool | None:
            nxt = session.feed(_decode_reply(rid, raw, FrsWriteResponse.unpack))
            if nxt is not None:
                self._send_shtp(ShtpChannel.CONTROL, nxt)  # the next data block
            return session.done or None

        self._await(rid, session.request(), step, timeout)

    def get_metadata(self, sensor: int, timeout: float = 2.0) -> SensorMetadata:
        """Read + parse the sensor's FRS metadata record."""
        record = METADATA_RECORDS.get(int(sensor))
        if record is None:
            raise Sh2Error(f"no metadata FRS record known for sensor 0x{int(sensor):02X}")
        return SensorMetadata.from_words(list(self.frs_read(record, timeout=timeout)))

    # ── diagnostics / housekeeping commands ──────────────────────────────────

    def get_oscillator_type(self, timeout: float = 1.0) -> OscillatorType | int:
        """Get Oscillator Type (command 0x0A). r[0] is the type directly; a
        value the enum does not know comes back as the raw int."""
        resp = self._command(
            Sh2Command.GET_OSCILLATOR_TYPE, b"", wait_response=True, timeout=timeout
        )
        assert resp is not None
        raw = resp.r[0]
        try:
            return OscillatorType(raw)
        except ValueError:
            return raw  # unknown value — surface the raw int

    def clear_dcd_and_reset(self, timeout: float = 2.0) -> None:
        """Clear the in-RAM dynamic calibration and reset the sensor
        (command 0x0B). There is no command response — the hub resets, so this
        waits for the executable reset-complete like hardware_reset()."""
        self._begin_reset()
        self._command(Sh2Command.CLEAR_DCD_AND_RESET, b"", wait_response=False)
        if not self._reset_event.wait(wait_limit(timeout)):
            raise DepzTimeoutError(Sh2Command.CLEAR_DCD_AND_RESET, timeout)
        if (dead := self._waiters.failure) is not None:  # woken by close() / a lost link
            self._waiters._raise(dead)
        # The command send consumed a host TX seq; the device restarted its
        # counters on reset, so realign (the captured advertisement survives —
        # it lives outside the SHTP layer).
        with self._shtp_lock:
            self._tx.reset()

    def get_errors(self, severity: int = 0, timeout: float = 1.0) -> list[ErrorRecord]:
        """Read the error queue (command 0x01), filtered to `severity` or
        greater. Records stream until one with source == 255 (no more)."""

        def collect(resp: CommandResponse, acc: list[CommandResponse]) -> bool:
            if resp.r[2] == ErrorSource.NO_MORE_ERRORS:
                return True  # terminator, not a real record
            acc.append(resp)
            return False

        responses = self._command_collect(
            Sh2Command.ERRORS, errors_params(severity), collect, timeout=timeout
        )
        return [ErrorRecord.from_response(r) for r in responses]

    def get_counts(self, sensor: int, timeout: float = 1.0) -> Counts:
        """Read a sensor's event counts (command 0x02). The hub answers with
        two responses (response_seq 0 then 1)."""

        def collect(resp: CommandResponse, acc: list[CommandResponse]) -> bool:
            acc.append(resp)
            # both halves, whatever their order: without one the exchange is
            # not complete (a timeout, contract 07 §3 — never a KeyError below)
            return {0, 1} <= {r.response_seq for r in acc}

        responses = self._command_collect(
            Sh2Command.COUNTER, counts_get_params(sensor), collect, timeout=timeout
        )
        by_seq = {r.response_seq: r for r in responses}
        r0 = bytes(by_seq[0].r)
        r1 = bytes(by_seq[1].r)
        offered, accepted = struct.unpack_from("<II", r0, 3)
        on, attempted = struct.unpack_from("<II", r1, 3)
        return Counts(
            sensor_id=int(sensor),
            offered=offered,
            accepted=accepted,
            on=on,
            attempted=attempted,
        )

    def clear_counts(self, sensor: int, timeout: float = 1.0) -> None:
        """Clear a sensor's event counts (command 0x02, subcommand 1)."""
        resp = self._command(
            Sh2Command.COUNTER,
            counts_clear_params(sensor),
            wait_response=True,
            timeout=timeout,
        )
        assert resp is not None
        if resp.status != 0:
            raise Sh2Error(f"clear counts failed: status {resp.status}")
