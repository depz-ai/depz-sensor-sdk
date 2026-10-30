"""DeviceBase over a scripted peer: the test thread plays the device on the
far end of a LoopbackLink, byte for byte. Covers what the fake firmware never
does on its own: diagnostic events (contract 07 §2), link-level damage
(contract 01), request correlation corner cases and close/disconnect races."""

from __future__ import annotations

import gc
import logging
import struct
import threading

import pytest

from depz_sensor_sdk import BusyError, DepzTimeoutError, InvalidReplyError
from depz_sensor_sdk.device import (
    DeviceBase,
    DisconnectedEvent,
    LinkCrcErrorEvent,
    SeqGapEvent,
    SequenceErrorEvent,
    TemperatureEvent,
    TextEvent,
    TrashEvent,
    UnsolicitedStatusEvent,
)
from depz_sensor_sdk.errors import DeviceLostError, LinkClosedError
from depz_sensor_sdk.protocol.common import Cmd, Rpt, Status
from depz_sensor_sdk.transport import CrcType, PacketParser, build_packet
from depz_sensor_sdk.transport.link import LoopbackLink


class Peer:
    """The device side of the loopback: reads host packets, writes replies."""

    def __init__(self):
        self.host, self.side = LoopbackLink.pair()
        self._parser = PacketParser()
        self.seq = 0

    def send(self, cmd: int, payload: bytes = b"", *, crc_type: CrcType = CrcType.NONE,
             seq: int | None = None) -> None:
        s = self.seq if seq is None else seq
        self.side.write(build_packet(cmd, payload, s, crc_type))
        self.seq = (s + 1) & 0xFF

    def raw(self, data: bytes) -> None:
        self.side.write(data)

    def expect(self, cmd: int, timeout: float = 1.0):
        """The next packet the host sent, asserted to be `cmd`."""
        while True:
            for ev in self._parser.feed(self.side.read(timeout)):
                assert ev.cmd == cmd, f"host sent {ev.cmd:#04x}, expected {cmd:#04x}"
                return ev


@pytest.fixture()
def rig():
    peer = Peer()
    dev = DeviceBase(peer.host, timeout=0.5)
    yield peer, dev
    dev.close()
    peer.side.close()


def test_context_manager_closes():
    peer = Peer()
    with DeviceBase(peer.host) as dev:
        assert dev.port == "<link>"
        assert not dev.closed
    assert dev.closed


def test_unsolicited_reports_become_events(rig):
    peer, dev = rig
    got = []
    unsub = dev.on_event(got.append)
    it = dev.events(maxsize=8)
    peer.send(Rpt.SEQUENCE_ERROR, bytes((7, 9)))
    peer.send(Rpt.STATUS, bytes((0x00, Status.ERR_HARDWARE_FAULT)))
    peer.send(Rpt.STATUS, bytes((0x00, Status.ERROR)))
    peer.send(Rpt.TEXT, bytes((0x04,)) + b"hello\x00")
    peer.send(Rpt.TEMPERATURE, struct.pack("<Qh", 123_456, 251))
    peer.send(0x7F, b"\x01\x02")  # a report no class knows: kept visible
    events = [next(it) for _ in range(6)]
    assert events == got
    assert events[0] == SequenceErrorEvent(7, 9, reported_by_device=True)
    assert dev.stats.device_seq_errors == 1
    assert events[1] == UnsolicitedStatusEvent(Status.ERR_HARDWARE_FAULT)
    assert events[1].is_hardware_fault and not events[2].is_hardware_fault
    assert events[3] == TextEvent(0x04, "hello")
    assert events[4] == TemperatureEvent(123_456, 25.1)
    assert events[5] == TextEvent(0x7F, "0102")
    unsub()
    peer.send(Rpt.TEXT, bytes((0x04,)) + b"again\x00")
    assert next(it) == TextEvent(0x04, "again")
    assert len(got) == 6  # callback unsubscribed, the queue still fed
    it.close()
    it.close()  # idempotent


def test_unsolicited_reports_with_a_tail_decode_like_solicited_ones(rig):
    """Contract 07 §3 (ERRATA A3): a report longer than its fields decodes and
    the tail is ignored — on the unsolicited path as on the solicited one
    (vector common_commands/input_rules/*_unsolicited_trailing_ignored). A
    shorter one is not a report of that kind: it stays visible as hex text."""
    peer, dev = rig
    it = dev.events(maxsize=8)
    temperature = struct.pack("<Qh", 123_456, 225)
    peer.send(Rpt.SEQUENCE_ERROR, bytes((7, 9, 0xAA, 0xBB)))
    peer.send(Rpt.TEMPERATURE, temperature + b"\xaa\xbb")
    peer.send(Rpt.TEMPERATURE, temperature[:9])
    assert next(it) == SequenceErrorEvent(7, 9, reported_by_device=True)
    assert dev.stats.device_seq_errors == 1
    assert next(it) == TemperatureEvent(123_456, 22.5)
    assert next(it) == TextEvent(Rpt.TEMPERATURE, temperature[:9].hex())


def test_link_damage_is_counted_and_reported(rig):
    peer, dev = rig
    it = dev.events()
    peer.raw(b"\x00garbage\x00")  # bytes before any magic
    frame = bytearray(build_packet(Rpt.TEXT, b"\x04ok\x00", 0, CrcType.CRC8))
    frame[-1] ^= 0xFF  # payload CRC trailer corrupted
    peer.raw(bytes(frame))
    peer.send(Rpt.TEXT, bytes((0x04,)) + b"fine\x00", seq=5)
    peer.send(Rpt.TEXT, bytes((0x04,)) + b"gap\x00", seq=9)  # device skipped 6..8

    evs = [next(it)]
    while evs[-1] != TextEvent(4, "gap"):
        evs.append(next(it))
    # Trash event boundaries follow read chunking (contract 01 §5): compare
    # the concatenated bytes only.
    trash = b"".join(e.data for e in evs if isinstance(e, TrashEvent))
    assert trash == b"\x00garbage\x00"
    assert LinkCrcErrorEvent(Rpt.TEXT, 0) in evs
    assert [e for e in evs if isinstance(e, TextEvent)] == [TextEvent(4, "fine"), TextEvent(4, "gap")]
    # ERRATA A1: the host-observed gap is an event of its own, ahead of the
    # packet it was seen on; the first accepted packet (seq 5) is no gap
    assert [e for e in evs if isinstance(e, SeqGapEvent)] == [SeqGapEvent(6, 9, missed=3)]
    assert evs.index(SeqGapEvent(6, 9, missed=3)) == evs.index(TextEvent(4, "gap")) - 1
    assert not any(isinstance(e, SequenceErrorEvent) for e in evs)
    assert dev.stats.trash_bytes == 9
    assert dev.stats.crc_errors == 1
    assert dev.stats.seq_gaps == 1
    assert dev.stats.rx_packets == 2
    assert dev.stats.rx_bytes > 0


def test_request_needs_a_completion_path(rig):
    _, dev = rig
    with pytest.raises(ValueError):
        dev.request(Cmd.GET_SERIAL)


def test_one_in_flight_request_per_opcode(rig):
    peer, dev = rig
    result = []
    t = threading.Thread(target=lambda: result.append(dev.get_serial_number()))
    t.start()
    peer.expect(Cmd.GET_SERIAL)
    with pytest.raises(BusyError):  # same opcode while the first is pending
        dev.get_serial_number()
    dev.send(Cmd.GET_DEVICE_NAME)  # a different opcode is fine
    peer.expect(Cmd.GET_DEVICE_NAME)
    peer.send(Rpt.TEXT, bytes((Cmd.GET_SERIAL,)) + b"SN1\x00")
    t.join(1.0)
    assert result == ["SN1"]
    assert dev.stats.tx_packets == 2  # the refused duplicate never hit the wire


def _in_thread(fn):
    """Run a blocking request on another thread; `out` gets its result or error."""
    out: dict = {}

    def run():
        try:
            out["result"] = fn()
        except Exception as exc:  # noqa: BLE001 — the test inspects it
            out["error"] = exc

    t = threading.Thread(target=run)
    t.start()
    return t, out


def test_short_reply_fails_its_request_and_the_reader_goes_on(rig):
    """Contract 07 §3 (ERRATA A4): a solicited reply shorter than its fields
    fails that one request with InvalidReplyError. The reader thread survives
    it and the next request is answered normally — the device is not dead."""
    peer, dev = rig
    events = dev.events()

    t, out = _in_thread(dev.read_mcu_temperature)
    peer.expect(Cmd.GET_MCU_TEMPERATURE)
    peer.send(Rpt.TEMPERATURE, b"\x00\x00\x00")  # 3 of the 10 bytes RPT_TEMPERATURE needs
    t.join(1.0)
    assert not t.is_alive()
    err = out["error"]
    assert isinstance(err, InvalidReplyError)
    assert err.cmd == Cmd.GET_MCU_TEMPERATURE
    assert isinstance(err.__cause__, ValueError) and "RPT_TEMPERATURE" in str(err)

    # The ERRATA example: an empty RPT_PAYLOAD_CRC_TYPE.
    t, out = _in_thread(dev.get_report_payload_crc)
    peer.expect(Cmd.GET_PAYLOAD_CRC_TYPE)
    peer.send(Rpt.PAYLOAD_CRC_TYPE, b"")
    t.join(1.0)
    assert isinstance(out["error"], InvalidReplyError)

    # The reader is alive: a well-formed reply completes the next request.
    t, out = _in_thread(dev.get_device_name)
    peer.expect(Cmd.GET_DEVICE_NAME)
    peer.send(Rpt.TEXT, bytes((Cmd.GET_DEVICE_NAME,)) + b"still here\x00")
    t.join(1.0)
    assert out == {"result": "still here"}
    assert dev._reader.is_alive() and not dev.closed
    assert dev.stats.rx_packets == 3
    # Neither short reply was an unsolicited report, let alone a disconnect.
    peer.send(Rpt.TEXT, bytes((0x04,)) + b"marker\x00")
    assert next(events) == TextEvent(0x04, "marker")


def test_a_raising_callback_never_stops_the_reader(rig, caplog):
    """Contract 07 §3: a callback that raises on the reader is counted, logged
    and dropped. The other callback and the queue still get the event, the
    next packet is read and a request completes."""
    peer, dev = rig

    def boom(ev):
        raise RuntimeError("consumer bug")

    got = []
    dev.on_event(boom)
    dev.on_event(got.append)
    events = dev.events()
    with caplog.at_level(logging.ERROR, logger="depz_sensor_sdk"):
        peer.send(Rpt.TEXT, bytes((0x04,)) + b"one\x00")
        assert next(events) == TextEvent(0x04, "one")
    assert got == [TextEvent(0x04, "one")]
    assert dev.stats.callback_errors == 1
    assert "consumer bug" in caplog.text

    t, out = _in_thread(dev.get_device_name)
    peer.expect(Cmd.GET_DEVICE_NAME)
    peer.send(Rpt.TEXT, bytes((Cmd.GET_DEVICE_NAME,)) + b"still here\x00")
    t.join(1.0)
    assert out == {"result": "still here"}
    assert dev._reader.is_alive()
    peer.send(Rpt.TEXT, bytes((0x04,)) + b"two\x00")
    assert next(events) == TextEvent(0x04, "two")
    assert dev.stats.callback_errors == 2  # boom raised again, still counted


def test_text_matcher_ignores_other_echoes(rig):
    """A pending RPT_TEXT matcher claims only the text echoing its own
    opcode; another text in between is an ordinary event."""
    peer, dev = rig
    it = dev.events()
    result = []
    t = threading.Thread(target=lambda: result.append(dev.get_device_name()))
    t.start()
    peer.expect(Cmd.GET_DEVICE_NAME)
    peer.send(Rpt.TEXT, bytes((Cmd.GET_SERIAL,)) + b"SN1\x00")
    peer.send(Rpt.TEMPERATURE, struct.pack("<Qh", 1, 300))
    assert next(it) == TextEvent(Cmd.GET_SERIAL, "SN1")
    assert next(it) == TemperatureEvent(1, 30.0)
    assert not result
    peer.send(Rpt.TEXT, bytes((Cmd.GET_DEVICE_NAME,)) + b"Board\x00")
    t.join(1.0)
    assert result == ["Board"]


def test_ok_ack_does_not_complete_a_data_request(rig):
    """RPT_STATUS OK for an opcode awaiting a data report is swallowed: the
    request stays pending for its report (contract 02 §1)."""
    peer, dev = rig
    result = []
    t = threading.Thread(target=lambda: result.append(dev.read_mcu_temperature()))
    t.start()
    peer.expect(Cmd.GET_MCU_TEMPERATURE)
    peer.send(Rpt.STATUS, bytes((Cmd.GET_MCU_TEMPERATURE, Status.OK)))
    t.join(0.1)
    assert t.is_alive() and not result
    peer.send(Rpt.TEMPERATURE, struct.pack("<Qh", 5, -15))
    t.join(1.0)
    assert result == [-1.5]


def test_timeout_frees_the_slot_and_a_late_reply_is_an_event(rig):
    peer, dev = rig
    it = dev.events()
    with pytest.raises(DepzTimeoutError) as ei:
        dev.request(Cmd.GET_SERIAL, ok_completes=True, timeout=0.05)
    assert ei.value.cmd == Cmd.GET_SERIAL
    peer.expect(Cmd.GET_SERIAL)
    peer.send(Rpt.STATUS, bytes((Cmd.GET_SERIAL, Status.OK)))  # too late
    assert next(it) == UnsolicitedStatusEvent(Status.OK)
    # the slot is free again: the retry is not refused as busy
    result = []
    t = threading.Thread(target=lambda: result.append(dev.get_serial_number()))
    t.start()
    peer.expect(Cmd.GET_SERIAL)
    peer.send(Rpt.TEXT, bytes((Cmd.GET_SERIAL,)) + b"SN2\x00")
    t.join(1.0)
    assert result == ["SN2"]


def test_peer_drop_fails_pending_and_reports_disconnect(rig):
    peer, dev = rig
    it = dev.events()
    errors = []

    def call():
        try:
            dev.get_serial_number()
        except Exception as exc:  # noqa: BLE001
            errors.append(exc)

    t = threading.Thread(target=call)
    t.start()
    peer.expect(Cmd.GET_SERIAL)
    peer.side.close()  # the device vanished mid-request
    t.join(1.0)
    assert len(errors) == 1 and isinstance(errors[0], DeviceLostError)
    assert isinstance(next(it), DisconnectedEvent)
    with pytest.raises(StopIteration):  # the stream ends with the device
        next(it)
    with pytest.raises(LinkClosedError):
        dev.get_serial_number()


def test_close_while_a_request_is_pending(rig):
    peer, dev = rig
    got = []
    errors = []

    def call():
        try:
            dev.get_serial_number()
        except Exception as exc:  # noqa: BLE001
            errors.append(exc)

    dev.on_event(got.append)
    t = threading.Thread(target=call)
    t.start()
    peer.expect(Cmd.GET_SERIAL)
    dev.close()
    t.join(1.0)
    assert len(errors) == 1 and isinstance(errors[0], LinkClosedError)
    assert got == []  # an intentional close is not a disconnect


def test_close_from_an_event_callback(rig):
    """A callback runs on the reader thread; closing from there must not
    deadlock on joining that same thread."""
    peer, dev = rig
    dev.on_event(lambda e: dev.close() if isinstance(e, TextEvent) else None)
    peer.send(Rpt.TEXT, bytes((0x04,)) + b"bye\x00")
    dev._reader.join(1.0)
    assert dev.closed and not dev._reader.is_alive()


def test_abandoned_iterators_deregister_on_gc():
    """stream()/events() iterators dropped without close() leave the
    registry once collected (the reader stops filling their queues)."""
    from depz_sensor_sdk import Sr04
    from depz_sensor_sdk.transport.link import LoopbackLink

    link, side = LoopbackLink.pair()
    dev = Sr04(link, timeout=0.1)
    try:
        for _ in range(20):
            it = dev.stream()
            ev = dev.events()
            del it, ev
        gc.collect()
        assert dev.stream_dropped_counts == []
        assert dev._event_queues == []
        kept = dev.stream()
        assert len(dev.stream_dropped_counts) == 1
        kept.close()
        kept.close()  # idempotent
        assert dev.stream_dropped_counts == []
    finally:
        dev.close()
        side.close()


class _RaisingReports(DeviceBase):
    """A sensor class whose stream handler raises on report 0x9E — a decoder
    bug, or a frame it does not expect (bug hunt 2026-10-29, group A)."""

    def _handle_report(self, pkt):
        if pkt.cmd == 0x9E:
            raise RuntimeError("stream decoder bug")
        return False


def test_a_raising_report_handler_drops_that_packet_and_the_reader_goes_on(caplog):
    """Contract 07 §3: an exception in the report path (`_handle_report`, any
    stream handler) drops that one packet, counts it in
    `stats.report_errors`, logs it — the reader reads on and requests
    complete."""
    peer = Peer()
    dev = _RaisingReports(peer.host, timeout=0.5)
    try:
        after = threading.Event()
        dev.on_event(lambda ev: ev == TextEvent(0x04, "after") and after.set())
        with caplog.at_level(logging.ERROR, logger="depz_sensor_sdk"):
            peer.send(0x9E, b"\x01\x02")
            peer.send(Rpt.TEXT, bytes((0x04,)) + b"after\x00")
            assert after.wait(1.0)  # the packet after the bad one was read
        assert dev.stats.report_errors == 1
        assert "stream decoder bug" in caplog.text
        t, out = _in_thread(dev.get_device_name)
        peer.expect(Cmd.GET_DEVICE_NAME)
        peer.send(Rpt.TEXT, bytes((Cmd.GET_DEVICE_NAME,)) + b"still here\x00")
        t.join(1.0)
        assert out == {"result": "still here"}
        assert dev._reader.is_alive() and not dev.closed
    finally:
        dev.close()
        peer.side.close()


class _BrokenRead(LoopbackLink):
    """A link whose read fails with something other than closed/lost."""

    def read(self, timeout=None):
        raise ValueError("I/O operation on closed file.")


def test_a_reader_that_ends_for_any_reason_closes_the_device(caplog):
    """The reader ending on anything but close() leaves the device closed:
    `closed` is true and a later request fails at once with DeviceLostError — never a timeout on a device that looks open."""
    host = _BrokenRead()
    host.peer = LoopbackLink()
    with caplog.at_level(logging.ERROR, logger="depz_sensor_sdk"):
        dev = DeviceBase(host, timeout=5.0)
        dev._reader.join(2.0)
    try:
        assert not dev._reader.is_alive()
        assert dev.closed
        t, out = _in_thread(dev.get_device_name)
        t.join(1.0)
        assert not t.is_alive()  # at once, not after the 5 s timeout
        assert isinstance(out["error"], DeviceLostError)
        assert isinstance(out["error"].__cause__, ValueError)
        assert "I/O operation on closed file" in caplog.text
    finally:
        dev.close()


class _BrokenReadAndClose(_BrokenRead):
    """…and whose close() raises too, so the link never reports closed."""

    def close(self):
        raise OSError("close failed")


def test_a_dead_reader_leaves_the_device_closed_even_if_the_link_close_raises(caplog):
    host = _BrokenReadAndClose()
    host.peer = LoopbackLink()
    with caplog.at_level(logging.ERROR, logger="depz_sensor_sdk"):
        dev = DeviceBase(host, timeout=5.0)
        dev._reader.join(2.0)
    assert not dev._reader.is_alive()
    assert not host.closed  # the link itself never got closed
    assert dev.closed
    t, out = _in_thread(dev.get_device_name)
    t.join(1.0)
    assert not t.is_alive()
    assert isinstance(out["error"], DeviceLostError)
