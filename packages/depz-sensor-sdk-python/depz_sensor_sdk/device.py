"""DeviceBase: reader thread, request correlation, events, common commands.

Behavioral rules per contracts/02 and 07: correlation by echoed request
opcode (never seq), one in-flight request per opcode, 200 ms default timeout,
bounded drop-oldest stream queues, callbacks run on the reader thread.
"""

from __future__ import annotations

import queue
import threading
import time
from dataclasses import dataclass
from typing import Any, Callable, Iterator

from .errors import (
    BusyError,
    DepzTimeoutError,
    DeviceLostError,
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
    pack_sync_time,
    sync_time_offset_rtt,
)
from .transport import CrcError, CrcType, Packet, PacketParser, Trash, build_packet
from .transport.link import Link
from .transport.serial_link import SerialLink

DEFAULT_TIMEOUT = 0.2

# ── events ────────────────────────────────────────────────────────────────────


@dataclass(frozen=True)
class DeviceEvent:
    """Base for unsolicited/diagnostic events (contract 07 §2)."""


@dataclass(frozen=True)
class SequenceErrorEvent(DeviceEvent):
    expected_seq: int
    received_seq: int
    reported_by_device: bool  # True: device saw a gap in host TX (RPT 0x84)


@dataclass(frozen=True)
class LinkCrcErrorEvent(DeviceEvent):
    cmd: int
    seq: int


@dataclass(frozen=True)
class TrashEvent(DeviceEvent):
    data: bytes


@dataclass(frozen=True)
class UnsolicitedStatusEvent(DeviceEvent):
    status: int

    @property
    def is_hardware_fault(self) -> bool:
        return self.status == Status.ERR_HARDWARE_FAULT


@dataclass(frozen=True)
class TextEvent(DeviceEvent):
    cmd: int
    text: str


@dataclass(frozen=True)
class TemperatureEvent(DeviceEvent):
    timestamp_us: int
    celsius: float


@dataclass(frozen=True)
class DisconnectedEvent(DeviceEvent):
    reason: str


@dataclass(frozen=True)
class TimeSync:
    offset_us: int  # device_clock - host_clock (host clock = host_now_us())
    rtt_us: int
    synced_at_host_us: int


@dataclass
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


def host_now_us() -> int:
    """Monotonic host clock in µs — the host side of all time-sync math."""
    return time.monotonic_ns() // 1000


def sync_time_all(
    devices: "Iterable[DeviceBase]", samples: int = 5
) -> "dict[DeviceBase, TimeSync]":
    """Software-sync several devices to the common host monotonic clock.

    Runs `sync_time()` on each device so that every device's
    `to_host_time_us()` maps its own timestamps onto ONE shared host timeline —
    the basis for correlating reports from multiple sensors live (the same
    alignment SessionRecorder uses for recordings). Returns {device: TimeSync}.
    """
    return {dev: dev.sync_time(samples) for dev in devices}


# ── request correlation ───────────────────────────────────────────────────────


class _Pending:
    """One in-flight request. `matcher` may claim any non-status packet and
    return the parsed result; an OK status completes with None only when
    `ok_completes` (commands whose success reply is RPT_STATUS OK)."""

    def __init__(
        self,
        cmd: int,
        matcher: Callable[[Packet], Any] | None,
        ok_completes: bool,
    ):
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


class StreamQueue:
    """Bounded drop-oldest queue with a drop counter (contract 07 §3)."""

    def __init__(self, maxsize: int):
        self._q: queue.Queue[Any] = queue.Queue(maxsize)
        self.dropped_count = 0
        self.closed = False

    def put(self, item: Any) -> None:
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

    def get(self, timeout: float | None = None) -> Any:
        return self._q.get(timeout=timeout)


_SENTINEL = object()


class StreamIterator:
    """Iterator over a StreamQueue that registers **eagerly** at construction.

    A lazy generator would subscribe only on the first `next()`, losing
    everything emitted in between (a real race on instant replay links).
    Ends when the device closes; deregisters on GC/close().
    """

    def __init__(self, registry: list["StreamQueue"], maxsize: int, closed_fn):
        self._queue = StreamQueue(maxsize)
        self._registry = registry
        self._closed_fn = closed_fn
        registry.append(self._queue)

    @property
    def dropped_count(self) -> int:
        return self._queue.dropped_count

    def __iter__(self) -> "StreamIterator":
        return self

    def __next__(self):
        while True:
            try:
                return self._queue.get(timeout=0.2)
            except queue.Empty:
                if self._closed_fn():
                    self.close()
                    raise StopIteration from None

    def close(self) -> None:
        if self._queue in self._registry:
            self._registry.remove(self._queue)


class DeviceBase:
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
    ):
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
        self._event_cbs: list[Callable[[DeviceEvent], None]] = []
        self._event_queues: list[StreamQueue] = []
        self._last_rx_seq: int | None = None
        self._time_sync: TimeSync | None = None
        self._closing = False

        # Subclass state must exist before the reader thread can dispatch a
        # report into `_handle_report` (replay links deliver instantly).
        self._init_subclass_state()

        self._reader = threading.Thread(
            target=self._reader_loop, name=f"depz-reader-{self._port_name}", daemon=True
        )
        self._reader.start()

    # ── lifecycle ────────────────────────────────────────────────────────────

    def __enter__(self) -> "DeviceBase":
        return self

    def __exit__(self, *exc: object) -> None:
        self.close()

    def close(self) -> None:
        self._closing = True
        self._link.close()
        if threading.current_thread() is not self._reader:
            self._reader.join(timeout=2.0)

    @property
    def port(self) -> str:
        return self._port_name

    @property
    def closed(self) -> bool:
        return self._link.closed

    # ── reader ───────────────────────────────────────────────────────────────

    def _reader_loop(self) -> None:
        try:
            while not self._closing:
                try:
                    data = self._link.read(0.05)
                except (LinkClosedError, DeviceLostError) as exc:
                    if not self._closing:
                        self._fail_all_pending(DeviceLostError(str(exc)))
                        self._emit_event(DisconnectedEvent(str(exc)))
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
                        self._emit_event(LinkCrcErrorEvent(ev.cmd, ev.seq))
                    elif isinstance(ev, Trash):
                        self.stats.trash_bytes += len(ev.data)
                        self._emit_event(TrashEvent(ev.data))
                self.stats.header_errors = self._parser.header_errors
        finally:
            self._fail_all_pending(LinkClosedError("device closed"))

    def _track_seq(self, seq: int) -> None:
        if self._last_rx_seq is not None and seq != (self._last_rx_seq + 1) & 0xFF:
            self.stats.seq_gaps += 1
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
            self._emit_event(UnsolicitedStatusEvent(rep.status))
            return

        # Give pending matchers first shot at any non-status packet.
        with self._pending_lock:
            for p in list(self._pending.values()):
                if p.matcher is None:
                    continue
                result = p.matcher(pkt)
                if result is not _SENTINEL:
                    del self._pending[p.cmd]
                    p.complete(result)
                    return

        if pkt.cmd == Rpt.SEQUENCE_ERROR and len(pkt.payload) == 2:
            rep = SequenceErrorReport.unpack(pkt.payload)
            self.stats.device_seq_errors += 1
            self._emit_event(
                SequenceErrorEvent(rep.expected_seq, rep.received_seq, reported_by_device=True)
            )
            return
        if pkt.cmd == Rpt.TEXT and pkt.payload:
            rep = TextReport.unpack(pkt.payload)
            self._emit_event(TextEvent(rep.cmd, rep.text))
            return
        if pkt.cmd == Rpt.TEMPERATURE and len(pkt.payload) == 10:
            rep = TemperatureReport.unpack(pkt.payload)
            self._emit_event(TemperatureEvent(rep.timestamp_us, rep.celsius))
            return

        if self._handle_report(pkt):
            return
        # Unknown/unrouted report: surface as trash-level diagnostics? Keep
        # it visible as an event so new firmware is debuggable.
        self._emit_event(TextEvent(pkt.cmd, pkt.payload.hex()))

    def _take_pending_if(self, cmd: int) -> _Pending | None:
        with self._pending_lock:
            return self._pending.pop(cmd, None)

    def _init_subclass_state(self) -> None:
        """Hook for sensor subclasses: create stream state here, NOT in
        __init__ after super().__init__ — the reader thread is already
        running by then."""

    def _handle_report(self, pkt: Packet) -> bool:
        """Sensor subclasses route their streaming reports here.

        Return True when the packet was consumed."""
        return False

    # ── events ───────────────────────────────────────────────────────────────

    def on_event(self, cb: Callable[[DeviceEvent], None]) -> Callable[[], None]:
        """Subscribe to unsolicited/diagnostic events (reader-thread context;
        do not block). Returns an unsubscribe function."""
        self._event_cbs.append(cb)
        return lambda: self._event_cbs.remove(cb)

    def events(self, maxsize: int = 256) -> StreamIterator:
        """Pull-style event stream (bounded, drop-oldest). Subscribes
        immediately — events emitted after this call are never missed."""
        return StreamIterator(self._event_queues, maxsize, lambda: self.closed)

    def _emit_event(self, event: DeviceEvent) -> None:
        for cb in list(self._event_cbs):
            cb(event)
        for q in list(self._event_queues):
            q.put(event)

    # ── TX / requests ────────────────────────────────────────────────────────

    def send(self, cmd: int, payload: bytes = b"", *, crc_type: CrcType | None = None) -> None:
        """Fire-and-forget packet (escape hatch; prefer `request`)."""
        ct = self.tx_crc_type if crc_type is None else crc_type
        with self._tx_lock:
            frame = build_packet(cmd, payload, self._tx_seq, ct)
            self._tx_seq = (self._tx_seq + 1) & 0xFF
            self._link.write(frame)
            self.stats.tx_packets += 1
            self.stats.tx_bytes += len(frame)

    def request(
        self,
        cmd: int,
        payload: bytes = b"",
        *,
        matcher: Callable[[Packet], Any] | None = None,
        ok_completes: bool = False,
        timeout: float | None = None,
    ) -> Any:
        """Send `cmd` and wait for its correlated completion.

        Exactly one of the completion paths must be configured:
        - `ok_completes=True` — RPT_STATUS(cmd, OK) finishes with None;
        - `matcher` — first packet for which `matcher(pkt) is not
          request.NO_MATCH` finishes with the matcher's return value.
        Non-OK RPT_STATUS echoing `cmd` always raises (BusyError for
        ERR_BUSY, StatusError otherwise). One in-flight request per opcode.
        """
        if matcher is None and not ok_completes:
            raise ValueError("configure matcher or ok_completes")
        pending = _Pending(cmd, matcher, ok_completes)
        with self._pending_lock:
            if cmd in self._pending:
                raise BusyError(cmd)
            self._pending[cmd] = pending
        try:
            self.send(cmd, payload)
        except Exception:
            self._take_pending_if(cmd)
            raise
        t = self.timeout if timeout is None else timeout
        if not pending.done.wait(t):
            self._take_pending_if(cmd)
            raise DepzTimeoutError(cmd, t)
        if pending.error is not None:
            raise pending.error
        return pending.result

    NO_MATCH = _SENTINEL

    @staticmethod
    def expect_report(report_id: int, unpack: Callable[[bytes], Any]) -> Callable[[Packet], Any]:
        """Matcher for a typed report identified by its report ID alone."""

        def match(pkt: Packet) -> Any:
            if pkt.cmd != report_id:
                return _SENTINEL
            return unpack(pkt.payload)

        return match

    @staticmethod
    def expect_text(request_cmd: int) -> Callable[[Packet], Any]:
        """Matcher for RPT_TEXT echoing `request_cmd`."""

        def match(pkt: Packet) -> Any:
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

    def read_mcu_temperature(self) -> float:
        """Last cached MCU temperature in °C (device refreshes ~2 Hz)."""
        rep: TemperatureReport = self.request(
            Cmd.GET_MCU_TEMPERATURE,
            matcher=self.expect_report(Rpt.TEMPERATURE, TemperatureReport.unpack),
        )
        return rep.celsius

    def sync_time(self, samples: int = 5) -> TimeSync:
        """NTP-style sync; keeps the lowest-RTT sample (contract 02 §5)."""
        best: TimeSync | None = None
        for _ in range(max(1, samples)):
            t1 = host_now_us()
            rep: SyncTimeReport = self.request(
                Cmd.SYNC_TIME,
                pack_sync_time(t1),
                matcher=self.expect_report(Rpt.SYNC_TIME, SyncTimeReport.unpack),
            )
            t4 = host_now_us()
            offset, rtt = sync_time_offset_rtt(t1, rep.mcu_rx_us, rep.mcu_tx_us, t4)
            if best is None or rtt < best.rtt_us:
                best = TimeSync(offset, rtt, t4)
        assert best is not None
        self._time_sync = best
        return best

    @property
    def time_sync(self) -> TimeSync | None:
        return self._time_sync

    def to_host_time_us(self, device_timestamp_us: int) -> int:
        """Device µs → host monotonic µs (requires a prior `sync_time`)."""
        if self._time_sync is None:
            raise RuntimeError("call sync_time() first")
        return device_timestamp_us - self._time_sync.offset_us

    def get_report_payload_crc(self) -> CrcType:
        raw = self.request(
            Cmd.GET_PAYLOAD_CRC_TYPE,
            matcher=self.expect_report(Rpt.PAYLOAD_CRC_TYPE, lambda p: p[0]),
        )
        return CrcType(raw)

    def set_report_payload_crc(self, crc_type: CrcType) -> None:
        """Set the device→host payload CRC mode (host→device is per-packet)."""
        self.request(Cmd.SET_PAYLOAD_CRC_TYPE, bytes([crc_type]), ok_completes=True)

    def get_sync_pin(self, pin: int) -> SyncPinConfig:
        return self.request(
            Cmd.GET_SYNC_PIN_CONFIG,
            bytes([pin]),
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
