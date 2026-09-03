"""VL53L8 register-bridge wire codecs (contracts/04_SENSOR_VL53L8.md)."""

from __future__ import annotations

import struct
from dataclasses import dataclass
from enum import IntEnum


class Vl53l8Cmd(IntEnum):
    READ_REG = 0x32
    WRITE_REG = 0x33
    # 0x34 intentionally unused (gap in the firmware's ID sequence)
    START_STREAM = 0x35
    STOP_STREAM = 0x36


class Vl53l8Rpt(IntEnum):
    REG_DATA = 0x91
    VL53_FRAME = 0x93


# MCU TRANSPORT_PAYLOAD_BUF_SIZE = 2304:
#   WRITE_REG payload = addr(2) + N  → N ≤ 2302
#   RPT_REG_DATA      = hdr(9) + N   → N ≤ 2295
# The reference tool uses a round 2048 for both directions.
READ_MAX_LEN = 2295
CHUNK_SIZE = 2048
STREAM_CHUNK_MAX = 1528  # bytes of frame data per RPT_VL53_FRAME chunk
STREAM_TOTAL_MAX = 8192  # max frame_size accepted by START_STREAM


def pack_read_reg(addr: int, length: int) -> bytes:
    return struct.pack("<HH", addr, length)


def pack_write_reg(addr: int, data: bytes) -> bytes:
    return struct.pack("<H", addr) + data


def pack_start_stream(frame_size: int) -> bytes:
    return struct.pack("<H", frame_size)


@dataclass(frozen=True)
class RegData:
    cmd: int  # echoed READ_REG opcode
    timestamp_us: int
    data: bytes

    @classmethod
    def unpack(cls, payload: bytes) -> "RegData":
        cmd, ts = struct.unpack_from("<BQ", payload)
        return cls(cmd, ts, payload[9:])


@dataclass(frozen=True)
class FrameChunk:
    timestamp_us: int
    full_size: int
    offset: int
    data: bytes

    @classmethod
    def unpack(cls, payload: bytes) -> "FrameChunk":
        ts, full, off = struct.unpack_from("<QHH", payload)
        return cls(ts, full, off, payload[12:])


class FrameReassembler:
    """Rebuilds full sensor frames from chunked RPT_VL53_FRAME reports.

    Rules (contract 04): reset on offset==0; chunks must be contiguous —
    a gap discards the frame in progress; a frame completes when the
    accumulated bytes equal `full_size`.
    """

    def __init__(self) -> None:
        self._buf = bytearray()
        self._full_size = 0
        self._timestamp_us = 0
        self.completed = 0
        self.discarded = 0

    def feed(self, chunk: FrameChunk) -> tuple[int, bytes] | None:
        """Returns (timestamp_us, frame_bytes) when a frame completes."""
        if chunk.offset == 0:
            if self._buf and len(self._buf) != self._full_size:
                self.discarded += 1
            self._buf = bytearray(chunk.data)
            self._full_size = chunk.full_size
            self._timestamp_us = chunk.timestamp_us
        elif chunk.offset == len(self._buf) and self._full_size == chunk.full_size and self._buf:
            self._buf.extend(chunk.data)
        else:
            if self._buf:
                self.discarded += 1
            self._buf = bytearray()
            self._full_size = 0
            return None
        if len(self._buf) == self._full_size and self._full_size > 0:
            frame = bytes(self._buf)
            self._buf = bytearray()
            self._full_size = 0
            self.completed += 1
            return (self._timestamp_us, frame)
        if len(self._buf) > self._full_size:
            self.discarded += 1
            self._buf = bytearray()
            self._full_size = 0
        return None
