"""CRC algorithms of the DEPZ transport (contracts/01_TRANSPORT_FRAMING.md §3).

All wire CRCs are reflected table implementations, byte-exact with the
firmware (`common/crc/crc8.c`) and the reference host tool
(`bootloader/tools/fw_image_builder.py`). CRC-8 init is 0x00 for every
device — see contracts/ERRATA.md E1.
"""

from __future__ import annotations


def _make_table(poly_reflected: int, width: int) -> list[int]:
    mask = (1 << width) - 1
    table = []
    for i in range(256):
        crc = i
        for _ in range(8):
            crc = (crc >> 1) ^ poly_reflected if crc & 1 else crc >> 1
        table.append(crc & mask)
    return table


_CRC8_TABLE = _make_table(0x8C, 8)
_CRC16_TABLE = _make_table(0xA001, 16)
_CRC32_TABLE = _make_table(0xEDB88320, 32)


def crc8_maxim(data: bytes) -> int:
    """CRC-8/MAXIM: poly 0x31 reflected, init 0x00, xorout 0x00."""
    crc = 0x00
    for b in data:
        crc = _CRC8_TABLE[crc ^ b]
    return crc


def crc16_modbus(data: bytes) -> int:
    """CRC-16/MODBUS: poly 0x8005 reflected, init 0xFFFF, xorout 0x0000."""
    crc = 0xFFFF
    for b in data:
        crc = (crc >> 8) ^ _CRC16_TABLE[(crc ^ b) & 0xFF]
    return crc


def crc32_iso_hdlc(data: bytes) -> int:
    """CRC-32/ISO-HDLC: poly 0x04C11DB7 reflected, init/xorout 0xFFFFFFFF."""
    crc = 0xFFFFFFFF
    for b in data:
        crc = (crc >> 8) ^ _CRC32_TABLE[(crc ^ b) & 0xFF]
    return crc ^ 0xFFFFFFFF


def crc16_ccitt_false(data: bytes) -> int:
    """CRC-16/CCITT-FALSE: poly 0x1021, init 0xFFFF, not reflected.

    Used only for the `.fwdepz` file header (contract 06), never on the wire.
    """
    crc = 0xFFFF
    for b in data:
        crc ^= b << 8
        for _ in range(8):
            crc = ((crc << 1) ^ 0x1021) & 0xFFFF if crc & 0x8000 else (crc << 1) & 0xFFFF
    return crc
