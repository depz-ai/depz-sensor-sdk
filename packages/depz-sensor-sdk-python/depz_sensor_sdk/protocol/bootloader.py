"""Bootloader wire codecs and .fwdepz container (contracts/06)."""

from __future__ import annotations

import struct
from dataclasses import dataclass
from enum import IntEnum
from pathlib import Path

from ..transport.crc import crc16_ccitt_false, crc32_iso_hdlc

FWDEPZ_MAGIC = b"FWDEPZ00"
FWDEPZ_HEADER_SIZE = 64


class BlCmd(IntEnum):
    BOOT_APPLICATION = 0x01
    DEVICE_RESET = 0x02
    GET_DEVICE_NAME = 0x03
    GET_FIRMWARE_NAME = 0x04
    GET_SERIAL = 0x05
    GET_MCU_ID = 0x06
    GET_MCU_UID = 0x07
    GET_FLASH_INFO = 0x08
    ERASE_APP = 0x09
    WRITE_PAGE = 0x0A
    READ_PAGE = 0x0B
    VERIFY_APP_CRC = 0x0C
    # 0x0D ENTER_DFU deliberately absent: out of SDK scope (contract 06)


class BlRpt(IntEnum):
    STATUS = 0x80
    STRING = 0x81
    MCU_ID = 0x86
    MCU_UID = 0x87
    FLASH_INFO = 0x89
    WRITE_PAGE = 0x8A
    READ_PAGE = 0x8B
    VERIFY_APP_CRC = 0x8C


class BlStatus(IntEnum):
    ACK = 0x00
    ERROR = 0x01
    ERR_ADDR = 0x03
    ERR_CRC_HDR = 0x04
    ERR_CRC_PKT = 0x05
    ERR_FLASH = 0x06


# The report each command's success reply arrives in (contract 06 §1).
# BOOT_APPLICATION is absent: fire-and-forget, nothing comes back.
BL_REPLY_RPT: dict[int, BlRpt] = {
    BlCmd.DEVICE_RESET: BlRpt.STATUS,
    BlCmd.GET_DEVICE_NAME: BlRpt.STRING,
    BlCmd.GET_FIRMWARE_NAME: BlRpt.STRING,
    BlCmd.GET_SERIAL: BlRpt.STRING,
    BlCmd.GET_MCU_ID: BlRpt.MCU_ID,
    BlCmd.GET_MCU_UID: BlRpt.MCU_UID,
    BlCmd.GET_FLASH_INFO: BlRpt.FLASH_INFO,
    BlCmd.ERASE_APP: BlRpt.STATUS,
    BlCmd.WRITE_PAGE: BlRpt.WRITE_PAGE,
    BlCmd.READ_PAGE: BlRpt.READ_PAGE,
    BlCmd.VERIFY_APP_CRC: BlRpt.VERIFY_APP_CRC,
}


def reply_report(cmd: int) -> BlRpt:
    """The report id that answers `cmd`; ValueError for a command without a
    reply (BOOT_APPLICATION) or outside the bootloader opcode space."""
    try:
        return BL_REPLY_RPT[cmd]
    except KeyError:
        raise ValueError(f"bootloader cmd 0x{cmd:02X} has no reply to wait for") from None


@dataclass(frozen=True)
class FlashInfo:
    page_size: int
    app_start: int
    app_size: int

    @classmethod
    def unpack(cls, payload: bytes) -> "FlashInfo":
        # payload: cmd u8, page_size u16, reserved u16, app_start u32, app_size u32
        page_size, _rsvd, app_start, app_size = struct.unpack_from("<HHII", payload, 1)
        return cls(page_size, app_start, app_size)


def pack_write_page(addr: int, data: bytes) -> bytes:
    return struct.pack("<IH", addr, len(data)) + data


def pack_read_page(addr: int, size: int) -> bytes:
    return struct.pack("<IH", addr, size)


class FwDepzError(ValueError):
    pass


@dataclass(frozen=True)
class FwDepzImage:
    """Parsed and validated `.fwdepz` firmware container."""

    load_addr: int
    fw_size: int
    fw_crc32: int
    cur_sec: int
    tot_sec: int
    payload: bytes

    @classmethod
    def parse(cls, blob: bytes) -> "FwDepzImage":
        if len(blob) < FWDEPZ_HEADER_SIZE:
            raise FwDepzError(f"file too short: {len(blob)} < {FWDEPZ_HEADER_SIZE}")
        if blob[:8] != FWDEPZ_MAGIC:
            raise FwDepzError("bad magic (not a .fwdepz file)")
        (hdr_crc,) = struct.unpack_from("<H", blob, 62)
        actual = crc16_ccitt_false(blob[:62])
        if hdr_crc != actual:
            raise FwDepzError(
                f"header CRC mismatch: stored=0x{hdr_crc:04X} actual=0x{actual:04X}"
            )
        load_addr, fw_size, fw_crc32 = struct.unpack_from("<III", blob, 8)
        cur_sec, tot_sec = blob[20], blob[21]
        payload = blob[FWDEPZ_HEADER_SIZE:]
        if fw_size != len(payload):
            raise FwDepzError(f"fw_size={fw_size} but payload is {len(payload)} bytes")
        # Sections are 0-based and every shipped file holds one (ERRATA B9).
        if (cur_sec, tot_sec) != (0, 1):
            raise FwDepzError(
                f"section {cur_sec} of {tot_sec}: only section 0 of 1 is supported "
                "(multi-section files are not)"
            )
        return cls(load_addr, fw_size, fw_crc32, cur_sec, tot_sec, payload)

    @classmethod
    def load(cls, path: str | Path) -> "FwDepzImage":
        return cls.parse(Path(path).read_bytes())

    @classmethod
    def build(cls, load_addr: int, payload: bytes, *, cur_sec: int = 0, tot_sec: int = 1) -> bytes:
        """Assemble a container (test/tooling helper; the fw_crc32 covers the
        payload exactly as flashed). Sections are 0-based: a shipped file is
        section 0 of 1, the only header parse() accepts."""
        hdr = bytearray(FWDEPZ_HEADER_SIZE)
        hdr[:8] = FWDEPZ_MAGIC
        struct.pack_into(
            "<IIIBB", hdr, 8, load_addr, len(payload), crc32_iso_hdlc(payload), cur_sec, tot_sec
        )
        struct.pack_into("<H", hdr, 62, crc16_ccitt_false(bytes(hdr[:62])))
        return bytes(hdr) + payload

    @property
    def payload_crc_ok(self) -> bool:
        return crc32_iso_hdlc(self.payload) == self.fw_crc32
