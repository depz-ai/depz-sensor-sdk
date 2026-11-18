"""TMF8829 SPI register-bridge wire codecs (contracts/14_SENSOR_TMF8829.md §2).

Same command numbering as the VL53 bridges (0x32/0x33/0x35/0x36/0x37) but
TMF8829 payloads: an 8-bit register address and self-sized frames, so the
host never tells the bridge a frame size.
"""

from __future__ import annotations

import struct
from dataclasses import dataclass
from enum import IntEnum

from ._wire import BridgeCmds


class Tmf8829Cmd(IntEnum):
    READ_REG = 0x32
    WRITE_REG = 0x33
    START_STREAM = 0x35
    STOP_STREAM = 0x36
    GET_INFO = 0x37


class Tmf8829Rpt(IntEnum):
    REG_DATA = 0x91
    INFO = 0x92
    FRAME = 0x93
    EVENT = 0x94


class EventCode(IntEnum):
    """RPT_TMF_EVENT.code."""

    INT = 0  # MOTION/PROXIMITY interrupt, no frame; its bits already cleared
    BAD_HEADER = 1  # frame header refused, frame not read (expected once on STOP mid histogram dump)


READ_MAX = 1024  # TMF_READ_REG len 1..1024
WRITE_MAX = 1023  # TMF_WRITE_REG N 1..1023 (1024 B buffer minus the address)
CHUNK_MAX = 1024  # frame bytes per RPT_TMF_FRAME

# INT_STATUS bits the bridge serves (TMF_START_STREAM int_mask)
INT_RESULTS = 0x01
INT_MOTION = 0x02
INT_PROXIMITY = 0x04
INT_HISTOGRAMS = 0x08
INT_MASK_ALL = 0x0F

SF_REF_FRAME = (
    0x01  # TMF_START_STREAM flags: emit the ref-SPAD frame before each results frame
)

REG_DATA_HEADER_SIZE = 9
FRAME_CHUNK_HEADER_SIZE = 21
EVENT_SIZE = 12
INFO_SIZE = 25


def pack_read_reg(addr: int, length: int) -> bytes:
    if not 0 <= addr <= 0xFF:
        raise ValueError("addr must be 0..255")
    if not 1 <= length <= READ_MAX:
        raise ValueError(f"length must be 1..{READ_MAX}")
    return struct.pack("<BH", addr, length)


def pack_write_reg(addr: int, data: bytes) -> bytes:
    if not 0 <= addr <= 0xFF:
        raise ValueError("addr must be 0..255")
    if not 1 <= len(data) <= WRITE_MAX:
        raise ValueError(f"data must be 1..{WRITE_MAX} bytes")
    return bytes([addr]) + bytes(data)


def pack_start_stream(int_mask: int, flags: int = 0) -> bytes:
    if not 1 <= int_mask <= INT_MASK_ALL:
        raise ValueError("int_mask must be a non-zero subset of 0x0F")
    if flags & ~SF_REF_FRAME:
        raise ValueError("flags: only SF_REF_FRAME (0x01) is defined")
    return struct.pack("<BB", int_mask, flags)


@dataclass(frozen=True, kw_only=True)
class RegData:
    """RPT_TMF_REG_DATA: register bytes and the MCU time the SPI read completed."""

    timestamp_us: int
    data: bytes

    @classmethod
    def unpack(cls, payload: bytes) -> "RegData":
        if len(payload) < REG_DATA_HEADER_SIZE:
            raise ValueError(
                f"RPT_TMF_REG_DATA: need ≥{REG_DATA_HEADER_SIZE} bytes, got {len(payload)}"
            )
        cmd, ts = struct.unpack_from("<BQ", payload)
        if cmd != Tmf8829Cmd.READ_REG:
            raise ValueError(f"RPT_TMF_REG_DATA: echoed cmd 0x{cmd:02X}, expected 0x32")
        return cls(timestamp_us=ts, data=bytes(payload[REG_DATA_HEADER_SIZE:]))


#: The register-bridge opcodes (contract 14 §2) for `RegisterBridge`.
TMF8829_BRIDGE = BridgeCmds(
    read_cmd=Tmf8829Cmd.READ_REG,
    write_cmd=Tmf8829Cmd.WRITE_REG,
    reg_data_rpt=Tmf8829Rpt.REG_DATA,
    unpack=RegData.unpack,
    pack_read=pack_read_reg,
    pack_write=pack_write_reg,
    addr_bytes=1,
)


@dataclass(frozen=True, kw_only=True)
class Tmf8829Info:
    """RPT_TMF_INFO: bridge state only, the sensor is never touched. Carries
    no echoed command byte. `int_edges` runs from MCU power-up; the frame
    counters restart at TMF_START_STREAM."""

    int_edges: int
    frames: int
    frames_aborted: int
    frame_errors: int
    spi_timeouts: int
    int_level: int  # PA4 level now, 0 = asserted
    streaming: bool
    int_mask: int
    last_int_status: int
    last_fifo_status: int

    @classmethod
    def unpack(cls, payload: bytes) -> "Tmf8829Info":
        if len(payload) < INFO_SIZE:
            raise ValueError(
                f"RPT_TMF_INFO: expected {INFO_SIZE} bytes, got {len(payload)}"
            )
        v = struct.unpack_from("<IIIIIBBBBB", payload)
        return cls(
            int_edges=v[0],
            frames=v[1],
            frames_aborted=v[2],
            frame_errors=v[3],
            spi_timeouts=v[4],
            int_level=v[5],
            streaming=bool(v[6]),
            int_mask=v[7],
            last_int_status=v[8],
            last_fifo_status=v[9],
        )


@dataclass(frozen=True, kw_only=True)
class FrameChunk:
    """One RPT_TMF_FRAME chunk. `timestamp_us` + the frame's pre-header
    SYSTICK are a clock pair (not the capture time); `edge_us` is the INT
    falling edge credited to the frame, 0 = none."""

    timestamp_us: int
    full_size: int
    offset: int
    frame_seq: int
    edge_us: int
    data: bytes

    @classmethod
    def unpack(cls, payload: bytes) -> "FrameChunk":
        if len(payload) < FRAME_CHUNK_HEADER_SIZE:
            raise ValueError(
                f"RPT_TMF_FRAME: need ≥{FRAME_CHUNK_HEADER_SIZE} bytes, got {len(payload)}"
            )
        ts, full, off, seq, edge = struct.unpack_from("<QHHBQ", payload)
        return cls(
            timestamp_us=ts,
            full_size=full,
            offset=off,
            frame_seq=seq,
            edge_us=edge,
            data=bytes(payload[FRAME_CHUNK_HEADER_SIZE:]),
        )


@dataclass(frozen=True, kw_only=True)
class Tmf8829Event:
    """RPT_TMF_EVENT. `int_status` is INT_STATUS as read (unmasked)."""

    timestamp_us: int
    int_status: int
    code: int
    frame_id: int  # header id byte (BAD_HEADER), else 0
    fifo_status: int  # pre-header byte (BAD_HEADER), else 0

    @classmethod
    def unpack(cls, payload: bytes) -> "Tmf8829Event":
        if len(payload) < EVENT_SIZE:
            raise ValueError(
                f"RPT_TMF_EVENT: expected {EVENT_SIZE} bytes, got {len(payload)}"
            )
        v = struct.unpack_from("<QBBBB", payload)
        return cls(
            timestamp_us=v[0],
            int_status=v[1],
            code=v[2],
            frame_id=v[3],
            fifo_status=v[4],
        )


@dataclass(frozen=True, kw_only=True)
class RawFrame:
    """A reassembled frame: the sensor bytes from register 0xFA on."""

    timestamp_us: int
    edge_us: int
    frame_seq: int
    data: bytes


class FrameReassembler:
    """Rebuilds frames from RPT_TMF_FRAME chunks (contract 14 §2): reset on
    offset 0; append only a chunk of the same frame_seq at the expected
    offset; complete at full_size. Never stitches chunks of different
    frame_seq. `seq_gaps` counts frames the bridge dropped (frame_seq jumps)."""

    def __init__(self) -> None:
        self.reset()

    def reset(self) -> None:
        """Forget everything — call at every stream start (frame_seq restarts at 0)."""
        self._buf = bytearray()
        self._head: FrameChunk | None = None
        self._next_seq: int | None = None
        self.completed = 0
        self.discarded = 0
        self.seq_gaps = 0

    def feed(self, chunk: FrameChunk) -> RawFrame | None:
        if chunk.offset == 0:
            if self._head is not None:
                self.discarded += 1  # the previous frame never completed
            if self._next_seq is not None and chunk.frame_seq != self._next_seq:
                self.seq_gaps += (chunk.frame_seq - self._next_seq) & 0xFF
            self._next_seq = (chunk.frame_seq + 1) & 0xFF
            self._head = chunk
            self._buf = bytearray(chunk.data)
        elif (
            self._head is not None
            and chunk.frame_seq == self._head.frame_seq
            and chunk.full_size == self._head.full_size
            and chunk.offset == len(self._buf)
        ):
            self._buf.extend(chunk.data)
        else:
            if self._head is not None:
                self.discarded += 1
            self._head = None
            self._buf = bytearray()
            return None
        head = self._head
        if len(self._buf) == head.full_size:
            frame = RawFrame(
                timestamp_us=head.timestamp_us,
                edge_us=head.edge_us,
                frame_seq=head.frame_seq,
                data=bytes(self._buf),
            )
            self._head = None
            self._buf = bytearray()
            self.completed += 1
            return frame
        if len(self._buf) > head.full_size:
            self.discarded += 1
            self._head = None
            self._buf = bytearray()
        return None
