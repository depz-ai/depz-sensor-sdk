"""DepzDevice: reader thread, request correlation, events, common commands.

Behavioral rules per contracts/02 and 07: correlation by echoed request
opcode (never seq), one in-flight request per opcode, 200 ms default timeout,
bounded drop-oldest stream queues, callbacks run on the reader thread.
"""

from __future__ import annotations

import logging
import threading
import time
from dataclasses import dataclass
from typing import Any, Callable, Iterable, TypeVar, Union, overload

from ._waits import wait_limit
from .errors import (
    BusyError,
    DepzTimeoutError,
    DepzError,
    DeviceLostError,
    InvalidReplyError,
    InvalidStateError,
    LinkClosedError,
    StatusError,
)
from .protocol.common import (
    Cmd,
    Rpt,
    SequenceErrorReport,
    Status,
    StatusReport,
    SyncPinConfig,
    SyncTimeReport,
    TemperatureReport,
    TextReport,
    UNSOLICITED,
    check_sync_pin,
    pack_sync_time,
    sync_time_offset_rtt,
    pack_set_payload_crc_type,
    unpack_payload_crc_type,
)
from .protocol.identity import Identity, parse_software_name
from .stream import Stream
from .transport import CrcError, CrcType, Packet, PacketParser, Trash, build_packet
from .transport.link import Link
from .transport.serial_link import SerialLink

DEFAULT_TIMEOUT = 0.2

T = TypeVar("T")


class _NoMatch:
    """The type of `DepzDevice.NO_MATCH` — what a matcher returns to decline a
    packet, so a matcher that may also return None stays unambiguous."""

    __slots__ = ()

    def __repr__(self) -> str:
        return "NO_MATCH"


_SENTINEL = _NoMatch()

#: A request matcher: the parsed result for the packet that completes the
#: request, `DepzDevice.NO_MATCH` for any other packet.
Matcher = Callable[[Packet], Union[T, _NoMatch]]

# ── events ────────────────────────────────────────────────────────────────────


@dataclass(frozen=True, kw_only=True)
class DeviceEvent:
    """Base for unsolicited/diagnostic events (contract 07 §2)."""


@dataclass(frozen=True, kw_only=True)
class SequenceErrorEvent(DeviceEvent):
    expected_seq: int
    received_seq: int
    reported_by_device: bool  # True: device saw a gap in host TX (RPT 0x84)


@dataclass(frozen=True, kw_only=True)
class SeqGapEvent(DeviceEvent):
    """A gap the host saw in the device->host seq (contract 01 §5 step 5):
    the packet still arrives, `missed` packets before it did not. Counted in
    `LinkStats.seq_gaps`; the device-reported gap (RPT 0x84) is
    `SequenceErrorEvent`."""

    expected_seq: int
    received_seq: int
    missed: int  # (received - expected) mod 256


@dataclass(frozen=True, kw_only=True)
class LinkCrcErrorEvent(DeviceEvent):
    cmd: int
    seq: int


@dataclass(frozen=True, kw_only=True)
class TrashEvent(DeviceEvent):
    data: bytes


@dataclass(frozen=True, kw_only=True)
class UnsolicitedStatusEvent(DeviceEvent):
    status: int

    @property
    def is_hardware_fault(self) -> bool:
        return self.status == Status.ERR_HARDWARE_FAULT


@dataclass(frozen=True, kw_only=True)
class TextEvent(DeviceEvent):
    cmd: int
    text: str


@dataclass(frozen=True, kw_only=True)
class TemperatureEvent(DeviceEvent):
    timestamp_us: int
    celsius: float


@dataclass(frozen=True, kw_only=True)
class DisconnectedEvent(DeviceEvent):
    reason: str


@dataclass(frozen=True, kw_only=True)
class TimeSync:
    offset_us: int  # device_clock - host_clock (host clock = host_now_us())
    rtt_us: int
    synced_at_host_us: int


@dataclass(kw_only=True)
class LinkStats:
    tx_packets: int = 0
    rx_packets: int = 0
    tx_bytes: int = 0
    rx_bytes: int = 0
    crc_errors: int = 0
    header_errors: int = 0
    trash_bytes: int = 0
    seq_gaps: int = 0  # gaps in device->host seq (host-observed)
    device_seq_errors: int = 0  # RPT_SEQUENCE_ERROR count (device-observed)
    callback_errors: int = 0  # callbacks that raised on the reader (contract 07 §3)
    report_errors: int = 0  # reports whose handler raised: packet dropped (contract 07 §3)


_log = logging.getLogger("depz_sensor_sdk")


def host_now_us() -> int:
    """Monotonic host clock in µs — the host side of all time-sync math.

    perf_counter, not monotonic: before Python 3.13 Windows' monotonic is
    GetTickCount64, in 15.6 ms steps — coarser than a sync round trip. Both
    are CLOCK_MONOTONIC on Linux; perf_counter is QueryPerformanceCounter on
    Windows, as in the C SDK."""
    return time.perf_counter_ns() // 1000


# ── request correlation ───────────────────────────────────────────────────────


class _Pending:
    """One in-flight request. `matcher` may claim any non-status packet and
    return the parsed result; an OK status completes with None only when
    `ok_completes` (commands whose success reply is RPT_STATUS OK)."""

    def __init__(
        self,
        cmd: int,
        matcher: Matcher[Any] | None,
        ok_completes: bool,
    ) -> None:
        self.cmd = cmd
        self.matcher = matcher
        self.ok_completes = ok_completes
        self.done = threading.Event()
        self.result: Any = None
        self.error: Exception | None = None

    def complete(self, result: Any) -> None:
        self.result = result
        self.done.set()

    def fail(self, exc: Exception) -> None:
        self.error = exc
        self.done.set()


class DepzDevice:
    """Connection to one DEPZ device in application mode.

    Accepts a port name or any `Link` (loopback/replay for tests). Starts a
    reader thread on construction; use as a context manager or call
    `close()`.
    """

    def __init__(
        self,
        port_or_link: str | Link,
        *,
        timeout: float = DEFAULT_TIMEOUT,
        tx_crc_type: CrcType = CrcType.NONE,
    ) -> None:
        if isinstance(port_or_link, str):
            self._link: Link = SerialLink(port_or_link)
            self._port_name = port_or_link
        else:
            self._link = port_or_link
            self._port_name = getattr(port_or_link, "port", "<link>")
        self.timeout = timeout
        self.tx_crc_type = tx_crc_type
        self.stats = LinkStats()

        self._parser = PacketParser()
        self._tx_seq = 0
        self._tx_lock = threading.Lock()
        self._pending: dict[int, _Pending] = {}
        self._pending_lock = threading.Lock()
        self.events = Stream[DeviceEvent](self, maxsize=256)
        self._last_rx_seq: int | None = None
        self._time_sync: TimeSync | None = None
        self._closing = False
        # Set when the reader ended on its own (not by close()): the cause of
        # every later request's DeviceLostError.
        self._lost: DeviceLostError | None = None  # set first: requests fail with it at once
        self._close_tried = False  # set once closing the link was tried, by close() or a lost device's reader

        # Subclass state must exist before the reader thread can dispatch a
        # report into `_handle_report` (replay links deliver instantly).
        # `attach_sensor_class` swaps the class and builds the state later,
        # under `_class_lock`, which the report path holds while it dispatches.
        self._class_lock = threading.Lock()
        self._init_subclass_state()

        self._reader = threading.Thread(
            target=self._reader_loop, name=f"depz-reader-{self._port_name}", daemon=True
        )
        self._reader.start()

    # ── lifecycle ────────────────────────────────────────────────────────────

    def __enter__(self) -> DepzDevice:
        return self

    def __exit__(self, *exc: object) -> None:
        self.close()

    def close(self) -> None:
        self._closing = True
        try:
            self._link.close()
        finally:
            # Whatever the link's close raised, the reader stops at its next
            # read: the device is closed, not one that looks open and times
            # every request out (contract 07 §3).
            self._close_tried = True
            if threading.current_thread() is not self._reader:
                self._reader.join(timeout=2.0)

    @property
    def port(self) -> str:
        return self._port_name

    @property
    def closed(self) -> bool:
        # A device is closed even if closing its link failed — but only after that
        # close was tried, so `closed` never shows an open link.
        return self._link.closed or self._close_tried

    @property
    def lost(self) -> DeviceLostError | None:
        """The loss that ended this device (its `__cause__` is what the link
        raised), or None: still open, or ended by the program's own `close()`.
        Set before `closed` shows the loss, so a pull iterator that has just
        ended quietly tells a loss from its own close here (contract 07 §3)."""
        return self._lost

    @property
    def _stream_ended(self) -> bool:
        # `closed` alone can show a link that dropped a moment before the
        # reader named the loss: a stream ends once the reason is known.
        return self.closed and (self._lost is not None or self._closing)

    def _end_error(self, closed_message: str) -> DepzError:
        """What a pull call raises on an ended device (contract 07 §3): the
        loss, whatever came after it; else the program's own close."""
        if self._lost is not None:
            err = DeviceLostError(str(self._lost))
            err.__cause__ = self._lost.__cause__
            return err
        closed = LinkClosedError(closed_message)
        closed.__suppress_context__ = True  # raised from inside a queue wait
        return closed

    # ── reader ───────────────────────────────────────────────────────────────

    def _reader_loop(self) -> None:
        try:
            while not self._closing:
                try:
                    data = self._link.read(0.05)
                except (LinkClosedError, DeviceLostError) as exc:
                    # The link dropped under the reader (unplug, the far end
                    # gone): the device is lost, not "closed" by this side —
                    # every later request says so (contract 07 §3).
                    if not self._closing:
                        lost = DeviceLostError(str(exc))
                        lost.__cause__ = exc
                        self._lose(lost)
                    return
                if not data:
                    continue
                self.stats.rx_bytes += len(data)
                for ev in self._parser.feed(data):
                    if isinstance(ev, Packet):
                        self.stats.rx_packets += 1
                        self._track_seq(ev.seq)
                        self._dispatch(ev)
                    elif isinstance(ev, CrcError):
                        self.stats.crc_errors += 1
                        self.events._emit(LinkCrcErrorEvent(cmd=ev.cmd, seq=ev.seq))
                    elif isinstance(ev, Trash):
                        self.stats.trash_bytes += len(ev.data)
                        self.events._emit(TrashEvent(data=ev.data))
                self.stats.header_errors = self._parser.header_errors
        except BaseException as exc:
            # Anything else ending the reader (a link that raised something
            # other than closed/lost, `SystemExit` included) must not leave a
            # device that looks open and times every request out: close it, say
            # why. What is no `Exception` goes on to the thread's end.
            if not self._closing:
                _log.exception("reader of %s ended", self._port_name)
                lost = DeviceLostError(f"reader ended: {exc!r}")
                lost.__cause__ = exc
                self._lose(lost)
            if not isinstance(exc, Exception):
                raise
        finally:
            self._fail_all_pending(LinkClosedError("device closed"))

    def _lose(self, lost: DeviceLostError) -> None:
        """The reader ended on its own: mark the device lost first (a request
        at that moment already fails with `lost`), close its link, and only
        then let `closed` show it; fail what is pending, say why (contract 07
        §3 order)."""
        self._lost = lost
        try:
            self._link.close()
        except Exception:
            _log.exception("closing %s after its reader ended", self._port_name)
        self._close_tried = True
        self._fail_all_pending(lost)
        self.events._emit(DisconnectedEvent(reason=str(lost)))

    def _track_seq(self, seq: int) -> None:
        # The first packet after open has nothing to be compared with. The
        # gap is surfaced before that packet is dispatched.
        if self._last_rx_seq is not None:
            expected = (self._last_rx_seq + 1) & 0xFF
            if seq != expected:
                self.stats.seq_gaps += 1
                self.events._emit(SeqGapEvent(
                    expected_seq=expected,
                    received_seq=seq,
                    missed=(seq - expected) & 0xFF,
                ))
        self._last_rx_seq = seq

    def _fail_all_pending(self, exc: Exception) -> None:
        with self._pending_lock:
            pending = list(self._pending.values())
            self._pending.clear()
        for p in pending:
            p.fail(exc)

    def _dispatch(self, pkt: Packet) -> None:
        if pkt.cmd == Rpt.STATUS and len(pkt.payload) >= 2:
            rep = StatusReport.unpack(pkt.payload[:2])
            if rep.cmd != UNSOLICITED:
                completed: _Pending | None = None
                error: Exception | None = None
                with self._pending_lock:
                    pending = self._pending.get(rep.cmd)
                    if pending is not None:
                        if rep.status == Status.ERR_BUSY:
                            error = BusyError(rep.cmd)
                        elif rep.status != Status.OK:
                            error = StatusError(rep.cmd, rep.status)
                        elif pending.ok_completes:
                            completed = pending
                        # else: OK ack while waiting for the data report —
                        # leave the pending in place untouched.
                        if completed is not None or error is not None:
                            del self._pending[rep.cmd]
                if pending is not None:
                    if error is not None:
                        pending.fail(error)
                    elif completed is not None:
                        completed.complete(None)
                    return
            self.events._emit(UnsolicitedStatusEvent(status=rep.status))
            return

        # Give pending matchers first shot at any non-status packet. A matcher
        # that raises has claimed the packet as its reply and failed to decode
        # it (contract 07 §3: a reply shorter than its fields is rejected):
        # that one request fails with InvalidReplyError and the reader goes
        # on — an exception here must never end the reader thread, which
        # would fail every later request on this device.
        claimed: _Pending | None = None
        result: Any = None
        bad: Exception | None = None
        with self._pending_lock:
            for p in list(self._pending.values()):
                if p.matcher is None:
                    continue
                try:
                    result = p.matcher(pkt)
                except Exception as exc:  # noqa: BLE001 — any decode failure is this reply's
                    bad = exc
                if result is not _SENTINEL or bad is not None:
                    del self._pending[p.cmd]
                    claimed = p
                    break
        if claimed is not None:
            if bad is not None:
                claimed.fail(InvalidReplyError(claimed.cmd, bad))
            else:
                claimed.complete(result)
            return

        # Contract 07 §3: a report longer than its fields is decoded and the
        # tail ignored (a newer firmware may append fields); a shorter one is
        # not a report of that kind and stays visible as hex text below.
        if pkt.cmd == Rpt.SEQUENCE_ERROR and len(pkt.payload) >= 2:
            seq_err = SequenceErrorReport.unpack(pkt.payload)
            self.stats.device_seq_errors += 1
            self.events._emit(
                SequenceErrorEvent(
                    expected_seq=seq_err.expected_seq,
                    received_seq=seq_err.received_seq,
                    reported_by_device=True,
                )
            )
            return
        if pkt.cmd == Rpt.TEXT and pkt.payload:
            text = TextReport.unpack(pkt.payload)
            self.events._emit(TextEvent(cmd=text.cmd, text=text.text))
            return
        if pkt.cmd == Rpt.TEMPERATURE and len(pkt.payload) >= 10:
            temp = TemperatureReport.unpack(pkt.payload)
            self.events._emit(TemperatureEvent(
                timestamp_us=temp.timestamp_us,
                celsius=temp.celsius,
            ))
            return

        # The report path runs guarded like the matchers above: a stream
        # handler that raises drops this one packet, counted and logged — it
        # never ends the reader (contract 07 §3).
        try:
            with self._class_lock:
                consumed = self._handle_report(pkt)
        except Exception:
            self.stats.report_errors += 1
            _log.exception("report 0x%02X handler raised on %s", pkt.cmd, self._port_name)
            return
        if consumed:
            return
        # Unknown/unrouted report: surface as trash-level diagnostics? Keep
        # it visible as an event so new firmware is debuggable.
        self.events._emit(TextEvent(cmd=pkt.cmd, text=pkt.payload.hex()))

    def _take_pending_if(self, cmd: int, pending: _Pending) -> _Pending | None:
        """Remove `pending` from the slot of `cmd` if it still holds it: None
        when the reader has claimed it meanwhile (its reply is on the way to
        it) — or when a newer request on the opcode took the slot since."""
        with self._pending_lock:
            if self._pending.get(cmd) is not pending:
                return None
            return self._pending.pop(cmd)

    def _init_subclass_state(self) -> None:
        """Hook for sensor subclasses: create stream state here, NOT in
        __init__ after super().__init__ — the reader thread is already
        running by then."""

    def _handle_report(self, pkt: Packet) -> bool:
        """Sensor subclasses route their streaming reports here.

        Return True when the packet was consumed."""
        return False

    # ── recording (contract 09) ──────────────────────────────────────────────
    # A sensor class declares its contract 09 record next to the item it
    # emits; `SessionRecorder` hooks any class through these three without
    # knowing it. A class without a record stream records nothing.

    #: Record kind of the items of `_record_stream` (contract 09 §3).
    _record_kind: str | None = None

    @property
    def _record_stream(self) -> Stream[Any] | None:
        """The stream the recorder subscribes to (the class's primary one)."""
        return None

    def _record_value(self, item: Any, *, layers: bool = False) -> dict[str, Any]:
        """The record value of one item of `_record_stream`. `layers` is the
        recorder's `vl53l8_layers` flag: the `vl53l8` record adds its per-zone
        layers; every other kind ignores it."""
        raise NotImplementedError(f"{type(self).__name__} has no contract 09 record")

    def _record_extras(self, item: Any) -> Iterable[tuple[str, dict[str, Any]]]:
        """Side records riding one item, as (kind, value) pairs — the CH
        `vl53l8_cnh` block next to its frame. None by default."""
        return ()

    # ── events ───────────────────────────────────────────────────────────────

    events: Stream[DeviceEvent]
    """The unsolicited / diagnostic events (contract 07 §2): `events.on(cb)`
    subscribes (reader-thread context; do not block) and returns an
    unsubscribe function, `events(maxsize)` is a bounded drop-oldest iterator
    that subscribes immediately, `events.dropped_counts` its queues' drops."""

    # ── TX / requests ────────────────────────────────────────────────────────

    def _check_open(self) -> None:
        """A lost or closed device refuses every request at once with its own
        error (contract 07 §3), before a slot is taken or anything is written."""
        if self._lost is not None:
            raise DeviceLostError(str(self._lost)) from self._lost.__cause__
        # From the moment close() began: the reader may have failed what was
        # pending and ended, and a request that goes out now has nobody to
        # answer or fail it (a link whose close takes its time, or raises).
        if self._closing:
            raise LinkClosedError("device closed")

    def send(self, cmd: int, payload: bytes = b"", *, crc_type: CrcType | None = None) -> None:
        """Fire-and-forget packet (escape hatch; prefer `request`)."""
        ct = self.tx_crc_type if crc_type is None else crc_type
        self._check_open()
        with self._tx_lock:
            frame = build_packet(cmd, payload, self._tx_seq, ct)
            self._tx_seq = (self._tx_seq + 1) & 0xFF
            try:
                self._link.write(frame)
            except LinkClosedError as exc:
                if self._closing:
                    raise
                # The link closed under this side's feet, not by close(): the
                # reader is marking the device lost (it closes the link after
                # setting `_lost`) — the caller sees the loss, not a "closed"
                # of a device it never closed (contract 07 §3; bug hunt
                # 2026-11-05 F-JAVA-2).
                lost = self._lost
                if lost is not None:
                    raise DeviceLostError(str(lost)) from exc
                raise DeviceLostError(
                    f"{self._port_name}: link closed under a request: {exc}"
                ) from exc
            self.stats.tx_packets += 1
            self.stats.tx_bytes += len(frame)

    @overload
    def request(
        self,
        cmd: int,
        payload: bytes = b"",
        *,
        matcher: Matcher[T],
        ok_completes: bool = False,
        timeout: float | None = None,
    ) -> T: ...

    @overload
    def request(
        self,
        cmd: int,
        payload: bytes = b"",
        *,
        matcher: None = None,
        ok_completes: bool = False,
        timeout: float | None = None,
    ) -> None: ...

    def request(
        self,
        cmd: int,
        payload: bytes = b"",
        *,
        matcher: Matcher[Any] | None = None,
        ok_completes: bool = False,
        timeout: float | None = None,
    ) -> Any:
        """Send `cmd` and wait for its correlated completion.

        Exactly one of the completion paths must be configured:
        - `ok_completes=True` — RPT_STATUS(cmd, OK) finishes with None;
        - `matcher` — first packet for which `matcher(pkt) is not
          request.NO_MATCH` finishes with the matcher's return value.
        Non-OK RPT_STATUS echoing `cmd` always raises (BusyError for
        ERR_BUSY, StatusError otherwise); a matcher that raises on the packet
        it claims (a reply shorter than its fields) raises InvalidReplyError
        here, and the device stays usable. One in-flight request per opcode.
        """
        if matcher is None and not ok_completes:
            raise ValueError("configure matcher or ok_completes")
        # Before the opcode's slot is taken: on a closed or lost device every
        # request fails with that error, never with "busy" because another
        # caller is giving the slot up at this moment (bug hunt 2026-11-05
        # F-JAVA-1). send() checks again, for a close() that began in between.
        self._check_open()
        pending = _Pending(cmd, matcher, ok_completes)
        with self._pending_lock:
            if cmd in self._pending:
                raise BusyError(cmd)
            self._pending[cmd] = pending
        try:
            self.send(cmd, payload)
        except Exception:
            self._take_pending_if(cmd, pending)
            raise
        t = self.timeout if timeout is None else timeout
        if not pending.done.wait(wait_limit(t)) and self._take_pending_if(cmd, pending) is pending:
            raise DepzTimeoutError(cmd, t)
        # Either the reply came in time, or it landed between the wait and the
        # removal: the reader claimed the pending under its lock and is
        # completing it — the reply is consumed, so it is this caller's.
        if not pending.done.wait(wait_limit(t)):  # only a reader that died mid-completion
            raise DepzTimeoutError(cmd, t)
        if pending.error is not None:
            raise pending.error
        return pending.result

    NO_MATCH = _SENTINEL

    @staticmethod
    def expect_report(report_id: int, unpack: Callable[[bytes], T]) -> Matcher[T]:
        """Matcher for a typed report identified by its report ID alone."""

        def match(pkt: Packet) -> T | _NoMatch:
            if pkt.cmd != report_id:
                return _SENTINEL
            return unpack(pkt.payload)

        return match

    @staticmethod
    def expect_text(request_cmd: int) -> Matcher[str]:
        """Matcher for RPT_TEXT echoing `request_cmd`."""

        def match(pkt: Packet) -> str | _NoMatch:
            if pkt.cmd != Rpt.TEXT or not pkt.payload or pkt.payload[0] != request_cmd:
                return _SENTINEL
            return TextReport.unpack(pkt.payload).text

        return match

    # ── common commands (contract 02) ────────────────────────────────────────

    def get_device_name(self) -> str:
        return self.request(Cmd.GET_DEVICE_NAME, matcher=self.expect_text(Cmd.GET_DEVICE_NAME))

    def get_software_name(self) -> str:
        return self.request(
            Cmd.GET_NAME_ACTIVE_SOFTWARE, matcher=self.expect_text(Cmd.GET_NAME_ACTIVE_SOFTWARE)
        )

    def get_serial_number(self) -> str:
        return self.request(Cmd.GET_SERIAL, matcher=self.expect_text(Cmd.GET_SERIAL))

    def identify(self) -> Identity:
        """Classify the running firmware (contract 02 §4): the software name
        parsed into mode, sensor type and version."""
        return parse_software_name(self.get_software_name())

    def read_mcu_temperature(self) -> float:
        """Last cached MCU temperature in °C (device refreshes ~2 Hz)."""
        rep = self.request(
            Cmd.GET_MCU_TEMPERATURE,
            matcher=self.expect_report(Rpt.TEMPERATURE, TemperatureReport.unpack),
        )
        return rep.celsius

    def sync_time(self, samples: int = 5) -> TimeSync:
        """NTP-style sync; keeps the lowest-RTT sample (contract 02 §5)."""
        best: TimeSync | None = None
        for _ in range(max(1, samples)):
            t1 = host_now_us()
            rep = self.request(
                Cmd.SYNC_TIME,
                pack_sync_time(t1),
                matcher=self.expect_report(Rpt.SYNC_TIME, SyncTimeReport.unpack),
            )
            t4 = host_now_us()
            offset, rtt = sync_time_offset_rtt(t1, rep.mcu_rx_us, rep.mcu_tx_us, t4)
            if best is None or rtt < best.rtt_us:
                best = TimeSync(offset_us=offset, rtt_us=rtt, synced_at_host_us=t4)
        assert best is not None
        self._time_sync = best
        return best

    @property
    def time_sync(self) -> TimeSync | None:
        return self._time_sync

    def to_host_time_us(self, device_timestamp_us: int) -> int:
        """Device µs → host monotonic µs (requires a prior `sync_time`)."""
        if self._time_sync is None:
            raise InvalidStateError("call sync_time() first")
        return device_timestamp_us - self._time_sync.offset_us

    def get_report_payload_crc(self) -> CrcType:
        # In the matcher: a value no CrcType names is an invalid reply (contract 07 §3).
        return self.request(
            Cmd.GET_PAYLOAD_CRC_TYPE,
            matcher=self.expect_report(
                Rpt.PAYLOAD_CRC_TYPE, lambda p: CrcType(unpack_payload_crc_type(p))
            ),
        )

    def set_report_payload_crc(self, crc_type: CrcType) -> None:
        """Set the device→host payload CRC mode (host→device is per-packet)."""
        self.request(Cmd.SET_PAYLOAD_CRC_TYPE, pack_set_payload_crc_type(crc_type), ok_completes=True)

    def get_sync_pin(self, pin: int) -> SyncPinConfig:
        return self.request(
            Cmd.GET_SYNC_PIN_CONFIG,
            bytes([check_sync_pin(pin)]),
            matcher=self.expect_report(Rpt.SYNC_PIN_CONFIG, SyncPinConfig.unpack),
        )

    def set_sync_pin(self, config: SyncPinConfig) -> None:
        self.request(Cmd.SET_SYNC_PIN_CONFIG, config.pack(), ok_completes=True)

    def reset(self) -> None:
        """DEVICE_RESET: device ACKs then reboots; the link will drop."""
        self.request(Cmd.DEVICE_RESET, ok_completes=True)

    def enter_bootloader_mode(self) -> None:
        """Ask the device to reboot into the resident bootloader and close
        this connection. Re-discovery/flash flow lives in the bootloader
        module (contract 06)."""
        self.request(Cmd.BOOTLOADER, ok_completes=True)
        self.close()
