"""BNO086 9-axis IMU: SHTP + SH-2 host stack over the firmware pass-through
bridge (contracts/05_SENSOR_BNO086.md).

The MCU only shuttles raw SHTP frames (SEND_SHTP_PACKET / RPT_DATA); the
whole sensor-hub protocol runs here. Per ERRATA E2 the bridge ACKs every
send with RPT_STATUS immediately and *all* inbound SHTP arrives as
RPT_DATA(cmd=0x00) — correlation happens at the SH-2 layer only.
"""

from __future__ import annotations

import queue
import threading
import time
import warnings
from dataclasses import dataclass
from typing import Callable, Iterable

from ..device import DeviceBase, StreamIterator, StreamQueue
from ..errors import BusyError, DepzTimeoutError
from ..protocol.bno086 import BUSY_BACKOFF_S, Bno086Cmd, Bno086Data, Bno086Rpt
from ..transport import Packet
from .reports import (
    Acceleration,
    GyroIntegratedRV,
    Gyroscope,
    InputReport,
    Magnetometer,
    Report,
    RotationVector,
    SensorId,
    parse_gyro_rv_cargo,
    parse_input_cargo,
)
from .sh2 import (
    CommandResponse,
    ControlReport,
    Counts,
    ErrorRecord,
    ErrorSource,
    FeatureResponse,
    FrsReadResponse,
    FrsReadSession,
    FrsWriteResponse,
    FrsWriteSession,
    ME_CAL_GET,
    METADATA_RECORDS,
    OscillatorType,
    ProductId,
    SensorMetadata,
    Sh2Command,
    Sh2Error,
    TareAxis,
    TareBasis,
    build_command_request,
    build_get_feature_request,
    build_product_id_request,
    build_set_feature,
    counts_clear_params,
    counts_get_params,
    errors_params,
    me_calibration_params,
    periodic_dcd_params,
    persist_tare_params,
    set_reorientation_params,
    tare_now_params,
)
from .shtp import ShtpCargo, ShtpChannel, ShtpLayer

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
    "FeatureResponse",
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

#: Re-reads allowed when `enable()`'s verify sees a stale "disabled" response
#: left over from a preceding `disable()`. One round trip is enough to pass the
#: in-flight response; two leaves margin without turning a genuine refusal into
#: a long stall.
_VERIFY_STALE_RETRIES = 2

_EXECUTABLE_RESET_COMPLETE = 0x01


@dataclass(frozen=True)
class CalibrationConfig:
    """ME calibration enables as reported by the sensor."""

    accel: bool
    gyro: bool
    mag: bool
    planar: bool


class Bno086(DeviceBase):
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

    def _init_subclass_state(self) -> None:
        self._shtp = ShtpLayer()
        self._shtp_lock = threading.Lock()  # RX side (reader thread) + reset
        self._shtp_tx_lock = threading.Lock()  # serialize SEND_SHTP_PACKET
        self._report_cbs: list[tuple[Callable[[Report], None], frozenset[int] | None]] = []
        self._report_queues: list[StreamQueue] = []
        self._control_waiters: dict[int, list[queue.SimpleQueue]] = {}
        self._control_lock = threading.Lock()
        self._cmd_seq = 0
        self._reset_event = threading.Event()
        self._advertisement = bytearray()
        self._features: dict[int, FeatureResponse] = {}

    # ── RX path ──────────────────────────────────────────────────────────────

    def _handle_report(self, pkt: Packet) -> bool:
        if pkt.cmd != Bno086Rpt.DATA or len(pkt.payload) < 9:
            return False
        data = Bno086Data.unpack(pkt.payload)
        with self._shtp_lock:
            cargo = self._shtp.feed(data.shtp)
        if cargo is not None:
            self._dispatch_cargo(cargo, data.timestamp_us)
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
            if not cargo.payload:
                return
            rid = cargo.payload[0]
            if rid == ControlReport.GET_FEATURE_RESPONSE and len(cargo.payload) >= 17:
                resp = FeatureResponse.unpack(cargo.payload)
                self._features[resp.sensor_id] = resp
            with self._control_lock:
                waiters = list(self._control_waiters.get(rid, ()))
            for w in waiters:
                w.put(bytes(cargo.payload))
            return
        if ch in (ShtpChannel.INPUT_NORMAL, ShtpChannel.INPUT_WAKE):
            for rep in parse_input_cargo(cargo.payload, capture_us):
                self._emit_report(rep)
            return
        if ch == ShtpChannel.GYRO_RV:
            rep = parse_gyro_rv_cargo(cargo.payload, capture_us)
            if rep is not None:
                self._emit_report(rep)

    def _emit_report(self, rep: Report) -> None:
        for cb, filt in list(self._report_cbs):
            if filt is None or rep.sensor_id in filt:
                cb(rep)
        for q in list(self._report_queues):
            filt = getattr(q, "sensor_filter", None)
            if filt is None or rep.sensor_id in filt:
                q.put(rep)

    # ── TX path ──────────────────────────────────────────────────────────────

    def _send_shtp(self, channel: int, payload: bytes) -> None:
        """Frame `payload` and push it through SEND_SHTP_PACKET.

        ERR_BUSY (both MCU TX slots full) backs off `busy_backoff_s` and
        retransmits, up to `busy_retries` attempts (contract 05 §2)."""
        with self._shtp_lock:
            frame = self._shtp.next_frame(channel, payload)
        with self._shtp_tx_lock:
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
        parse: Callable[[bytes], object],
        pred: Callable[[object], bool] | None = None,
        timeout: float = 1.0,
    ):
        """Send a control-channel payload and wait for a matching response.

        Correlation is purely SH-2-level: `expect_rid` selects the response
        report ID, `pred` (on the parsed object) narrows further (sensor id,
        command seq, ...)."""
        q: queue.SimpleQueue = queue.SimpleQueue()
        with self._control_lock:
            self._control_waiters.setdefault(expect_rid, []).append(q)
        try:
            self._send_shtp(ShtpChannel.CONTROL, payload)
            deadline = time.monotonic() + timeout
            while True:
                remaining = deadline - time.monotonic()
                if remaining <= 0:
                    raise DepzTimeoutError(expect_rid, timeout)
                try:
                    raw = q.get(timeout=remaining)
                except queue.Empty:
                    raise DepzTimeoutError(expect_rid, timeout) from None
                obj = parse(raw)
                if pred is None or pred(obj):
                    return obj
        finally:
            with self._control_lock:
                self._control_waiters[expect_rid].remove(q)

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
        collect: Callable[[CommandResponse, list], bool],
        timeout: float = 1.0,
    ) -> list[CommandResponse]:
        """Send a Command Request and gather every Command Response correlated
        on (command, command_seq) until `collect(resp, acc)` returns True.

        Used by multi-message commands (Errors, Counts) where the hub streams
        several 0xF1 responses with an incrementing response_seq."""
        seq = self._next_cmd_seq()
        payload = build_command_request(seq, command, params)
        q: queue.SimpleQueue = queue.SimpleQueue()
        with self._control_lock:
            self._control_waiters.setdefault(ControlReport.COMMAND_RESPONSE, []).append(q)
        results: list[CommandResponse] = []
        try:
            self._send_shtp(ShtpChannel.CONTROL, payload)
            deadline = time.monotonic() + timeout
            while True:
                remaining = deadline - time.monotonic()
                if remaining <= 0:
                    raise DepzTimeoutError(ControlReport.COMMAND_RESPONSE, timeout)
                try:
                    raw = q.get(timeout=remaining)
                except queue.Empty:
                    raise DepzTimeoutError(ControlReport.COMMAND_RESPONSE, timeout) from None
                resp = CommandResponse.unpack(raw)
                if resp.command != command or resp.command_seq != seq:
                    continue
                if collect(resp, results):
                    return results
        finally:
            with self._control_lock:
                self._control_waiters[ControlReport.COMMAND_RESPONSE].remove(q)

    # ── lifecycle ────────────────────────────────────────────────────────────

    def hardware_reset(self, timeout: float = 2.0) -> None:
        """Hard-reset the sensor via nRST (0x32). All SHTP state (seq
        counters, partial cargos) and cached features restart from zero.

        The hub's RPT_STATUS OK ack for the 0x32 command is treated as the
        reset confirmation. The SH-2 executable-channel reset-complete (which
        the BNO08X SH-2 spec would emit) is only waited for best-effort:
        older firmware (≤ v0.95) did **not** emit it (ERRATA E9 in
        contracts/ERRATA.md, now fixed in newer firmware); the best-effort wait
        handles both, so its absence is *not* an error — the sensor is fully
        usable without it.
        """
        self._reset_event.clear()
        with self._shtp_lock:
            self._shtp.reset()
        self._advertisement = bytearray()
        self._features.clear()
        # RPT_STATUS OK from the hub confirms the reset command was accepted.
        self.request(Bno086Cmd.SENSOR_RESET, ok_completes=True, timeout=timeout)
        # Best-effort wait for the SH-2 executable reset-complete; older
        # firmware (≤ v0.95) legitimately never sent it (ERRATA E9, now fixed
        # in newer firmware) — the best-effort wait handles both, so do not
        # hard-fail when it is missing.
        self._reset_event.wait(min(timeout, 0.5))

    def wake(self) -> None:
        """Pulse WAKE (PS0): wakes the sensor from sleep, no state loss."""
        self.request(Bno086Cmd.SENSOR_WAKE_UP, ok_completes=True)

    @property
    def advertisement(self) -> bytes:
        """Raw SHTP channel-0 advertisement bytes seen since open/reset."""
        return bytes(self._advertisement)

    # ── SH-2: identification & features ─────────────────────────────────────

    def product_id(self, timeout: float = 1.0) -> ProductId:
        """Product ID Request/Response round trip (first responding
        subsystem)."""
        return self._control_request(
            build_product_id_request(),
            ControlReport.PRODUCT_ID_RESPONSE,
            ProductId.unpack,
            timeout=timeout,
        )

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
        resp = self._verify_feature(sensor, timeout)
        requested_rate = 1_000_000 / interval_us
        actual_rate = 1_000_000 / resp.interval_us if resp.interval_us else 0.0
        if not (RATE_LOW_FACTOR * requested_rate <= actual_rate <= RATE_HIGH_FACTOR * requested_rate):
            warnings.warn(
                f"BNO086 sensor 0x{sensor:02X}: requested {requested_rate:.1f} Hz, "
                f"granted {actual_rate:.1f} Hz (outside {RATE_LOW_FACTOR}–"
                f"{RATE_HIGH_FACTOR}× band)",
                stacklevel=2,
            )
        return resp

    def _verify_feature(self, sensor: int, timeout: float) -> FeatureResponse:
        """Read back a feature we just *enabled*, skipping stale responses.

        The hub emits an **unsolicited** Get Feature Response for every
        state-changing Set Feature — including the `disable()` that typically
        precedes a re-enable — and SH-2 gives Get Feature Response no
        correlation token (contract 05 §6: only `sensorId` identifies it). So
        the GFR(interval=0) produced by a preceding `disable()` can still be in
        flight when this read registers, and would otherwise be reported as
        "granted 0 Hz" for a sensor that is in fact streaming.

        We just asked for a non-zero interval, so a response claiming the
        sensor is disabled is either stale or a genuine refusal. Re-read a
        bounded number of times: a stale one is followed by the real answer
        within one round trip, while a genuine refusal survives the retries and
        still reaches the caller's warning path (contract 05 §7: warn, never
        raise).
        """
        resp = self.get_feature(sensor, timeout=timeout)
        for _ in range(_VERIFY_STALE_RETRIES):
            if resp.interval_us != 0:
                break
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

    # sugar for the everyday sensors
    def enable_rotation_vector(self, hz: float = 100, **kw) -> FeatureResponse | None:
        return self.enable(SensorId.ROTATION_VECTOR, hz, **kw)

    def enable_game_rotation_vector(self, hz: float = 100, **kw) -> FeatureResponse | None:
        return self.enable(SensorId.GAME_ROTATION_VECTOR, hz, **kw)

    def enable_accelerometer(self, hz: float = 100, **kw) -> FeatureResponse | None:
        return self.enable(SensorId.ACCELEROMETER, hz, **kw)

    def enable_gyroscope(self, hz: float = 100, **kw) -> FeatureResponse | None:
        return self.enable(SensorId.GYROSCOPE, hz, **kw)

    def enable_magnetometer(self, hz: float = 50, **kw) -> FeatureResponse | None:
        return self.enable(SensorId.MAGNETOMETER, hz, **kw)

    def enable_linear_acceleration(self, hz: float = 100, **kw) -> FeatureResponse | None:
        return self.enable(SensorId.LINEAR_ACCELERATION, hz, **kw)

    def enable_gravity(self, hz: float = 100, **kw) -> FeatureResponse | None:
        return self.enable(SensorId.GRAVITY, hz, **kw)

    def enable_gyro_integrated_rv(self, hz: float = 400, **kw) -> FeatureResponse | None:
        return self.enable(SensorId.GYRO_INTEGRATED_RV, hz, **kw)

    # ── report streaming ─────────────────────────────────────────────────────

    def on_report(
        self,
        cb: Callable[[Report], None],
        sensors: Iterable[int] | int | None = None,
    ) -> Callable[[], None]:
        """Subscribe to typed sensor reports (reader-thread context; do not
        block). `sensors` filters by SensorId. Returns an unsubscribe fn."""
        filt = _normalize_filter(sensors)
        entry = (cb, filt)
        self._report_cbs.append(entry)
        return lambda: self._report_cbs.remove(entry)

    def reports(
        self,
        sensors: Iterable[int] | int | None = None,
        maxsize: int = 1024,
    ) -> StreamIterator:
        """Blocking iterator over typed reports (bounded, drop-oldest).
        Subscribes eagerly — reports emitted after this call are never
        missed."""
        it = StreamIterator(self._report_queues, maxsize, lambda: self.closed)
        it._queue.sensor_filter = _normalize_filter(sensors)  # type: ignore[attr-defined]
        return it

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
        zeros clears). No response per SH-2."""
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
        return CalibrationConfig(bool(r[1]), bool(r[2]), bool(r[3]), bool(r[4]))

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

    # ── FRS ──────────────────────────────────────────────────────────────────

    def frs_read(self, record_id: int, timeout: float = 2.0) -> tuple[int, ...]:
        """Read a whole FRS record; returns its 32-bit words."""
        session = FrsReadSession(record_id)
        q: queue.SimpleQueue = queue.SimpleQueue()
        with self._control_lock:
            self._control_waiters.setdefault(ControlReport.FRS_READ_RESPONSE, []).append(q)
        try:
            self._send_shtp(ShtpChannel.CONTROL, session.request())
            deadline = time.monotonic() + timeout
            while not session.done:
                remaining = deadline - time.monotonic()
                if remaining <= 0:
                    raise DepzTimeoutError(ControlReport.FRS_READ_RESPONSE, timeout)
                try:
                    raw = q.get(timeout=remaining)
                except queue.Empty:
                    raise DepzTimeoutError(ControlReport.FRS_READ_RESPONSE, timeout) from None
                session.feed(FrsReadResponse.unpack(raw))
        finally:
            with self._control_lock:
                self._control_waiters[ControlReport.FRS_READ_RESPONSE].remove(q)
        return tuple(session.words)

    def frs_write(self, record_id: int, words: Iterable[int], timeout: float = 2.0) -> None:
        """Write a whole FRS record (word list); raises Sh2Error on failure."""
        session = FrsWriteSession(record_id, list(words))
        q: queue.SimpleQueue = queue.SimpleQueue()
        with self._control_lock:
            self._control_waiters.setdefault(ControlReport.FRS_WRITE_RESPONSE, []).append(q)
        try:
            self._send_shtp(ShtpChannel.CONTROL, session.request())
            deadline = time.monotonic() + timeout
            while not session.done:
                remaining = deadline - time.monotonic()
                if remaining <= 0:
                    raise DepzTimeoutError(ControlReport.FRS_WRITE_RESPONSE, timeout)
                try:
                    raw = q.get(timeout=remaining)
                except queue.Empty:
                    raise DepzTimeoutError(ControlReport.FRS_WRITE_RESPONSE, timeout) from None
                nxt = session.feed(FrsWriteResponse.unpack(raw))
                if nxt is not None:
                    self._send_shtp(ShtpChannel.CONTROL, nxt)
        finally:
            with self._control_lock:
                self._control_waiters[ControlReport.FRS_WRITE_RESPONSE].remove(q)

    def get_metadata(self, sensor: int, timeout: float = 2.0) -> SensorMetadata:
        """Read + parse the sensor's FRS metadata record."""
        record = METADATA_RECORDS.get(int(sensor))
        if record is None:
            raise Sh2Error(f"no metadata FRS record known for sensor 0x{int(sensor):02X}")
        return SensorMetadata.from_words(list(self.frs_read(record, timeout=timeout)))

    # ── diagnostics / housekeeping commands ──────────────────────────────────

    def get_oscillator_type(self, timeout: float = 1.0) -> OscillatorType:
        """Get Oscillator Type (command 0x0A). r[0] is the type directly."""
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
        self._reset_event.clear()
        with self._shtp_lock:
            self._shtp.reset()
        self._advertisement = bytearray()
        self._features.clear()
        self._command(Sh2Command.CLEAR_DCD_AND_RESET, b"", wait_response=False)
        if not self._reset_event.wait(timeout):
            raise DepzTimeoutError(Sh2Command.CLEAR_DCD_AND_RESET, timeout)
        # The command send consumed a host TX seq; the device restarted its
        # counters on reset, so realign (the captured advertisement survives —
        # it lives outside the SHTP layer).
        with self._shtp_lock:
            self._shtp.reset()

    def get_errors(self, severity: int = 0, timeout: float = 1.0) -> list[ErrorRecord]:
        """Read the error queue (command 0x01), filtered to `severity` or
        greater. Records stream until one with source == 255 (no more)."""

        def collect(resp: CommandResponse, acc: list) -> bool:
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
        import struct

        def collect(resp: CommandResponse, acc: list) -> bool:
            acc.append(resp)
            return any(r.response_seq == 1 for r in acc)

        responses = self._command_collect(
            Sh2Command.COUNTER, counts_get_params(sensor), collect, timeout=timeout
        )
        by_seq = {r.response_seq: r for r in responses}
        r0 = bytes(by_seq[0].r)
        r1 = bytes(by_seq[1].r)
        offered, accepted = struct.unpack_from("<II", r0, 3)
        on, attempted = struct.unpack_from("<II", r1, 3)
        return Counts(int(sensor), offered, accepted, on, attempted)

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


def _normalize_filter(sensors: Iterable[int] | int | None) -> frozenset[int] | None:
    if sensors is None:
        return None
    if isinstance(sensors, int):
        return frozenset((int(sensors),))
    return frozenset(int(s) for s in sensors)
