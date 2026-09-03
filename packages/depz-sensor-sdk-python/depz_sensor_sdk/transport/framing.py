"""Packet framing and incremental parser (contracts/01_TRANSPORT_FRAMING.md).

Byte-exact with firmware `common/transport/transport.c`. The parser fixes the
reference host tool's empty-payload sizing bug (contracts/ERRATA.md E6): a
packet whose header advertises a payload CRC type but carries an empty
payload has **no** CRC bytes on the wire.
"""

from __future__ import annotations

import struct
from dataclasses import dataclass
from enum import IntEnum

from .crc import crc8_maxim, crc16_modbus, crc32_iso_hdlc

MAGIC = b"\xa5\xc3"
HEADER_SIZE = 7
MAX_PAYLOAD = 0x3FFF


class CrcType(IntEnum):
    NONE = 0
    CRC8 = 1
    CRC16 = 2
    CRC32 = 3


_CRC_SIZES = {CrcType.NONE: 0, CrcType.CRC8: 1, CrcType.CRC16: 2, CrcType.CRC32: 4}


def payload_crc_bytes(crc_type: CrcType, payload: bytes) -> bytes:
    """CRC trailer for a payload; empty payloads never carry CRC bytes."""
    if crc_type == CrcType.NONE or not payload:
        return b""
    if crc_type == CrcType.CRC8:
        return struct.pack("<B", crc8_maxim(payload))
    if crc_type == CrcType.CRC16:
        return struct.pack("<H", crc16_modbus(payload))
    return struct.pack("<I", crc32_iso_hdlc(payload))


def build_packet(
    cmd: int, payload: bytes = b"", seq: int = 0, crc_type: CrcType = CrcType.NONE
) -> bytes:
    """Frame one packet. `crc_type` bits are set in the header even for an
    empty payload (matching device TX), but CRC bytes are only appended for
    non-empty payloads."""
    if len(payload) > MAX_PAYLOAD:
        raise ValueError(f"payload too long: {len(payload)} > {MAX_PAYLOAD}")
    data_size = len(payload) | (int(crc_type) << 14)
    ds = struct.pack("<H", data_size)
    hdr_crc = crc8_maxim(ds + bytes((cmd, seq & 0xFF)))
    return MAGIC + ds + bytes((cmd, seq & 0xFF, hdr_crc)) + payload + payload_crc_bytes(
        crc_type, payload
    )


@dataclass(frozen=True)
class Packet:
    cmd: int
    seq: int
    payload: bytes


@dataclass(frozen=True)
class Trash:
    """Bytes discarded while hunting for a valid frame. Boundaries between
    consecutive Trash events depend on read chunking; only the concatenated
    byte stream is deterministic."""

    data: bytes


@dataclass(frozen=True)
class CrcError:
    """A frame with a valid header whose payload CRC failed; dropped."""

    cmd: int
    seq: int


ParserEvent = Packet | Trash | CrcError


class PacketParser:
    """Incremental frame parser. Feed arbitrary byte chunks; get events.

    Event order is invariant to chunking (contract 01 §5) except Trash event
    boundaries — concatenate Trash data when comparing streams.
    """

    def __init__(self) -> None:
        self._buf = bytearray()
        self.packets = 0
        self.crc_errors = 0
        self.header_errors = 0
        self.trash_bytes = 0

    def feed(self, data: bytes) -> list[ParserEvent]:
        self._buf.extend(data)
        out: list[ParserEvent] = []
        while True:
            ev = self._parse_one()
            if ev is None:
                break
            out.append(ev)
        return out

    def _emit_trash(self, count: int) -> Trash:
        data = bytes(self._buf[:count])
        del self._buf[:count]
        self.trash_bytes += count
        return Trash(data)

    def _parse_one(self) -> ParserEvent | None:
        buf = self._buf
        pos = buf.find(MAGIC)
        if pos == -1:
            # Keep the last byte: it may be a split 0xA5.
            if len(buf) > 1:
                return self._emit_trash(len(buf) - 1)
            return None
        if pos > 0:
            return self._emit_trash(pos)
        if len(buf) < HEADER_SIZE:
            return None
        data_size = struct.unpack_from("<H", buf, 2)[0]
        payload_size = data_size & MAX_PAYLOAD
        crc_type = CrcType((data_size >> 14) & 0x03)
        if crc8_maxim(bytes(buf[2:6])) != buf[6]:
            # Header corrupt: advance one byte past the magic start and let
            # the magic hunt resync (firmware: rb_skip(off + 1)).
            self.header_errors += 1
            return self._emit_trash(1)
        # ERRATA E6: empty payload never carries CRC bytes.
        crc_size = _CRC_SIZES[crc_type] if payload_size > 0 else 0
        total = HEADER_SIZE + payload_size + crc_size
        if len(buf) < total:
            return None
        cmd, seq = buf[4], buf[5]
        payload = bytes(buf[HEADER_SIZE : HEADER_SIZE + payload_size])
        trailer = bytes(buf[HEADER_SIZE + payload_size : total])
        del buf[:total]
        if crc_size and payload_crc_bytes(crc_type, payload) != trailer:
            self.crc_errors += 1
            return CrcError(cmd, seq)
        self.packets += 1
        return Packet(cmd, seq, payload)
