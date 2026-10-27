"""Common command/report IDs and payload codecs (contracts/02_COMMON_COMMANDS.md).

Payload codecs return raw integers exactly as on the wire; unit conversions
(0.1 °C, µs) happen in the device layer.
"""

from __future__ import annotations

import struct
from dataclasses import dataclass
from enum import IntEnum

from ._wire import check_uint, need_bytes


class Cmd(IntEnum):
    BOOTLOADER = 0x01
    DEVICE_RESET = 0x02
    GET_DEVICE_NAME = 0x03
    GET_NAME_ACTIVE_SOFTWARE = 0x04
    GET_SERIAL = 0x05
    SYNC_TIME = 0x06
    GET_MCU_TEMPERATURE = 0x07
    GET_PAYLOAD_CRC_TYPE = 0x08
    SET_PAYLOAD_CRC_TYPE = 0x09
    USB_TEST_TX_START = 0x1C
    USB_TEST_TX_STOP = 0x1D
    USB_TEST_RX_DATA = 0x1E
    USB_TEST_STATS = 0x1F
    GET_SYNC_PIN_CONFIG = 0x30
    SET_SYNC_PIN_CONFIG = 0x31


class Rpt(IntEnum):
    STATUS = 0x80
    TEXT = 0x81
    SYNC_TIME = 0x82
    TEMPERATURE = 0x83
    SEQUENCE_ERROR = 0x84
    PAYLOAD_CRC_TYPE = 0x87
    USB_TEST_DATA = 0x88
    USB_TEST_STATS = 0x8F
    SYNC_PIN_CONFIG = 0x90


class Status(IntEnum):
    OK = 0x00
    ERROR = 0x01
    ERR_INVALID_CMD = 0x02
    ERR_PAYLOAD_FORMAT = 0x03
    ERR_INVALID_PARAM = 0x04
    ERR_PAYLOAD_CRC = 0x05
    ERR_BUSY = 0x06
    ERR_CMD_NOT_SUPPORTED = 0x07
    ERR_NOT_INITIALIZED = 0x08
    ERR_HARDWARE_FAULT = 0x09


class SyncPinMode(IntEnum):
    DISABLE = 0x00
    IN = 0x01
    OUT_START = 0x02
    OUT_END = 0x03
    OUT_BOTH = 0x04


class SyncPinPolarity(IntEnum):
    IDLE_LOW = 0x00
    IDLE_HIGH = 0x01


UNSOLICITED = 0x00  # value of the echoed-cmd byte in unsolicited reports


@dataclass(frozen=True)
class StatusReport:
    cmd: int  # echoed request opcode; 0x00 = unsolicited
    status: int

    @classmethod
    def unpack(cls, payload: bytes) -> "StatusReport":
        need_bytes("RPT_STATUS", payload, 2)
        c, s = struct.unpack_from("<BB", payload)
        return cls(c, s)


@dataclass(frozen=True)
class TextReport:
    cmd: int
    text: str

    @classmethod
    def unpack(cls, payload: bytes) -> "TextReport":
        need_bytes("RPT_TEXT", payload, 1)
        return cls(payload[0], strip_device_string(payload[1:]))


@dataclass(frozen=True)
class SyncTimeReport:
    pc_timestamp_us: int  # T1 echoed
    mcu_rx_us: int  # T2
    mcu_tx_us: int  # T3

    @classmethod
    def unpack(cls, payload: bytes) -> "SyncTimeReport":
        need_bytes("RPT_SYNC_TIME", payload, 24)
        t1, t2, t3 = struct.unpack_from("<QQQ", payload)
        return cls(t1, t2, t3)


@dataclass(frozen=True)
class TemperatureReport:
    timestamp_us: int
    raw_decidegrees: int  # int16, units of 0.1 °C

    @classmethod
    def unpack(cls, payload: bytes) -> "TemperatureReport":
        need_bytes("RPT_TEMPERATURE", payload, 10)
        ts, t = struct.unpack_from("<Qh", payload)
        return cls(ts, t)

    @property
    def celsius(self) -> float:
        return self.raw_decidegrees / 10.0


PAYLOAD_CRC_TYPE_MAX = 3  # SET_PAYLOAD_CRC_TYPE takes 0..3 (contract 02 §2)


def pack_set_payload_crc_type(crc_type: int) -> bytes:
    """SET_PAYLOAD_CRC_TYPE payload `crc_type u8`, 0..3 (contract 02 §2), else
    ValueError — checked before anything is written (contract 07 §3)."""
    if check_uint("crc_type", crc_type, 8) > PAYLOAD_CRC_TYPE_MAX:
        raise ValueError(f"crc_type: {crc_type} is not a CrcType 0..{PAYLOAD_CRC_TYPE_MAX}")
    return bytes([crc_type])


def unpack_payload_crc_type(payload: bytes) -> int:
    """RPT_PAYLOAD_CRC_TYPE: the device→host payload CRC mode (one byte)."""
    need_bytes("RPT_PAYLOAD_CRC_TYPE", payload, 1)
    return payload[0]


@dataclass(frozen=True)
class SequenceErrorReport:
    expected_seq: int
    received_seq: int

    @classmethod
    def unpack(cls, payload: bytes) -> "SequenceErrorReport":
        need_bytes("RPT_SEQUENCE_ERROR", payload, 2)
        e, r = struct.unpack_from("<BB", payload)
        return cls(e, r)


SYNC_PIN_MIN = 1  # logical AUX sync pins (contract 02 §6)
SYNC_PIN_MAX = 5


def check_sync_pin(pin: int) -> int:
    """`pin` as a logical AUX sync pin 1..5 (contract 02 §6), else ValueError."""
    if not SYNC_PIN_MIN <= check_uint("pin", pin, 8) <= SYNC_PIN_MAX:
        raise ValueError(f"pin: {pin} is not a sync pin {SYNC_PIN_MIN}..{SYNC_PIN_MAX}")
    return pin


@dataclass(frozen=True)
class SyncPinConfig:
    pin: int  # 1..5
    mode: SyncPinMode
    polarity: SyncPinPolarity

    def pack(self) -> bytes:
        mode = check_uint("mode", self.mode, 8)
        if mode > SyncPinMode.OUT_BOTH:
            raise ValueError(f"mode: {mode} is not a SyncPinMode")
        polarity = check_uint("polarity", self.polarity, 8)
        if polarity > SyncPinPolarity.IDLE_HIGH:
            raise ValueError(f"polarity: {polarity} is not a SyncPinPolarity")
        return struct.pack("<BBB", check_sync_pin(self.pin), mode, polarity)

    @classmethod
    def unpack(cls, payload: bytes) -> "SyncPinConfig":
        need_bytes("RPT_SYNC_PIN_CONFIG", payload, 3)
        p, m, pol = struct.unpack_from("<BBB", payload)
        return cls(p, SyncPinMode(m), SyncPinPolarity(pol))


def pack_sync_time(pc_timestamp_us: int) -> bytes:
    return struct.pack("<Q", check_uint("pc_timestamp_us", pc_timestamp_us, 64))


def sync_time_offset_rtt(t1: int, t2: int, t3: int, t4: int) -> tuple[int, int]:
    """NTP-style clock math, all µs (contract 02 §5).

    Returns (offset_us, rtt_us) where offset = device_clock - host_clock,
    computed with floor division toward zero on the sum (integer parity rule
    shared by all SDKs: offset = ((T2-T1)+(T3-T4)) // 2 with truncation).
    """
    num = (t2 - t1) + (t3 - t4)
    offset = num // 2 if num >= 0 else -((-num) // 2)
    rtt = (t4 - t1) - (t3 - t2)
    return offset, rtt


def strip_device_string(raw: bytes) -> str:
    """Decode an ASCII device string, dropping trailing NUL/0xFF filler."""
    return raw.rstrip(b"\x00\xff").decode("ascii", errors="replace")
