"""SHTP framing layer for the BNO086 (contracts/05_SENSOR_BNO086.md §3).

Pure codec — no I/O. A frame is a 4-byte header plus a cargo fragment:

    length u16 LE  — bits 14:0 cargo length *including* the 4-byte header;
                     bit 15 set marks a continuation fragment
    channel u8     — see ShtpChannel
    seq u8         — per-channel, per-direction free-running counter

For a cargo that spans several bridge frames, the first fragment's length
field carries the TOTAL cargo length (header included) even though the frame
itself holds fewer bytes; each continuation fragment carries the remaining
length (its own header included) with bit 15 set. The receiver trusts the
first fragment's total and the actual frame sizes; continuation length
fields are informative only.
"""

from __future__ import annotations

import struct
from dataclasses import dataclass
from enum import IntEnum


class ShtpChannel(IntEnum):
    COMMAND = 0  # SHTP command channel (advertisements)
    EXECUTABLE = 1  # device executable: reset/on/sleep; RX 0x01 = reset done
    CONTROL = 2  # SH-2 control: feature/FRS/command reports
    INPUT_NORMAL = 3  # non-wake input reports (0xFB timebase + sensors)
    INPUT_WAKE = 4  # wake input reports (same cargo format as channel 3)
    GYRO_RV = 5  # gyro-integrated rotation vector, dense format


SHTP_HEADER_SIZE = 4
LENGTH_MASK = 0x7FFF
CONTINUATION_BIT = 0x8000
NUM_CHANNELS = 6

# Host->sensor frames must fit one MCU transmit slot (ERRATA E2: 2 x 64 B).
MAX_TX_FRAME = 64


@dataclass(frozen=True)
class ShtpHeader:
    length: int  # bits 14:0 — cargo length incl. this 4-byte header
    channel: int
    seq: int
    continuation: bool = False

    def pack(self) -> bytes:
        word = (self.length & LENGTH_MASK) | (CONTINUATION_BIT if self.continuation else 0)
        return struct.pack("<HBB", word, self.channel, self.seq)

    @classmethod
    def unpack(cls, data: bytes) -> "ShtpHeader":
        word, channel, seq = struct.unpack_from("<HBB", data)
        return cls(word & LENGTH_MASK, channel, seq, bool(word & CONTINUATION_BIT))


@dataclass(frozen=True)
class ShtpCargo:
    """One reassembled cargo: `payload` excludes all SHTP headers."""

    channel: int
    seq: int  # seq of the first fragment
    payload: bytes


def build_frame(channel: int, payload: bytes, seq: int) -> bytes:
    """Single-fragment frame: length = header + payload."""
    return ShtpHeader(SHTP_HEADER_SIZE + len(payload), channel, seq & 0xFF).pack() + payload


def fragment_cargo(
    channel: int, payload: bytes, seq_start: int, max_frame: int = MAX_TX_FRAME
) -> list[bytes]:
    """Split a cargo into wire frames of at most `max_frame` bytes.

    First fragment advertises the TOTAL cargo length; continuations carry the
    remaining length with the continuation bit set. seq increments per frame.
    """
    if max_frame <= SHTP_HEADER_SIZE:
        raise ValueError("max_frame must exceed the 4-byte SHTP header")
    room = max_frame - SHTP_HEADER_SIZE
    frames: list[bytes] = []
    off = 0
    seq = seq_start & 0xFF
    total = SHTP_HEADER_SIZE + len(payload)
    while True:
        chunk = payload[off : off + room]
        remaining = total - off  # includes one header
        hdr = ShtpHeader(remaining, channel, seq, continuation=off > 0)
        frames.append(hdr.pack() + chunk)
        off += len(chunk)
        seq = (seq + 1) & 0xFF
        if off >= len(payload):
            return frames


class _ChannelRx:
    __slots__ = ("buf", "expected", "seq")

    def __init__(self) -> None:
        self.buf = bytearray()
        self.expected = 0  # total cargo payload bytes (headers excluded)
        self.seq = 0


class ShtpLayer:
    """Per-channel TX sequence counters + RX cargo reassembly.

    Feed every inbound frame (the RPT_DATA payload after cmd/timestamp) to
    `feed()`; it returns a completed ShtpCargo or None. Build outbound frames
    with `next_frame()` which consumes the channel's TX seq. Not thread-safe;
    the device layer serializes access.
    """

    def __init__(self) -> None:
        self._tx_seq = [0] * NUM_CHANNELS
        self._rx = [_ChannelRx() for _ in range(NUM_CHANNELS)]
        self.discarded = 0  # incomplete cargos thrown away

    # ── TX ───────────────────────────────────────────────────────────────────

    def next_frame(self, channel: int, payload: bytes) -> bytes:
        """Build a single-fragment frame, consuming the channel's TX seq.

        Host-side cargos always fit one MCU slot (control payloads are <= 21
        bytes); larger payloads are a caller bug."""
        if SHTP_HEADER_SIZE + len(payload) > MAX_TX_FRAME:
            raise ValueError(
                f"TX cargo {len(payload)}B exceeds the {MAX_TX_FRAME}B MCU slot"
            )
        seq = self._tx_seq[channel]
        self._tx_seq[channel] = (seq + 1) & 0xFF
        return build_frame(channel, payload, seq)

    def tx_seq(self, channel: int) -> int:
        return self._tx_seq[channel]

    # ── RX ───────────────────────────────────────────────────────────────────

    def feed(self, frame: bytes) -> ShtpCargo | None:
        """Consume one inbound frame; return the cargo when complete.

        Rules (contract 05 §3): a non-continuation fragment starts a new
        cargo (discarding any partial one on that channel); a continuation
        without a cargo in progress is dropped; the cargo completes when the
        accumulated bytes reach the first fragment's advertised total."""
        if len(frame) < SHTP_HEADER_SIZE:
            return None
        hdr = ShtpHeader.unpack(frame)
        if hdr.channel >= NUM_CHANNELS or hdr.length < SHTP_HEADER_SIZE:
            return None  # empty/padding header ("no data" read) or junk
        chunk = frame[SHTP_HEADER_SIZE:]
        rx = self._rx[hdr.channel]
        if not hdr.continuation:
            if rx.expected and rx.buf:
                self.discarded += 1
            rx.buf = bytearray(chunk)
            rx.expected = hdr.length - SHTP_HEADER_SIZE
            rx.seq = hdr.seq
        else:
            if not rx.expected:
                self.discarded += 1
                return None
            rx.buf.extend(chunk)
        if len(rx.buf) < rx.expected:
            return None
        if len(rx.buf) > rx.expected:  # overrun — junk framing
            self.discarded += 1
            rx.buf = bytearray()
            rx.expected = 0
            return None
        cargo = ShtpCargo(hdr.channel, rx.seq, bytes(rx.buf))
        rx.buf = bytearray()
        rx.expected = 0
        return cargo

    def reset(self) -> None:
        """Forget all TX seq counters and partial cargos (sensor reset)."""
        self._tx_seq = [0] * NUM_CHANNELS
        self._rx = [_ChannelRx() for _ in range(NUM_CHANNELS)]
