"""Common command/report IDs and payload codecs (contracts/02_COMMON_COMMANDS.md).

Payload codecs return raw integers exactly as on the wire; unit conversions
(0.1 °C, µs) happen in the device layer.
"""

from __future__ import annotations

import struct
from dataclasses import dataclass
from enum import IntEnum


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
    THROUGHPUT_TX_START = 0x1C
    THROUGHPUT_TX_STOP = 0x1D
    THROUGHPUT_RX_DATA = 0x1E
    GET_SYNC_PIN_CONFIG = 0x30
    SET_SYNC_PIN_CONFIG = 0x31


class Rpt(IntEnum):
    STATUS = 0x80
    TEXT = 0x81
    SYNC_TIME = 0x82
    TEMPERATURE = 0x83
    SEQUENCE_ERROR = 0x84
    PAYLOAD_CRC_TYPE = 0x87
    THROUGHPUT_DATA = 0x88
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
        c, s = struct.unpack("<BB", payload)
        return cls(c, s)


@dataclass(frozen=True)
class TextReport:
    cmd: int
    text: str

    @classmethod
    def unpack(cls, payload: bytes) -> "TextReport":
        return cls(payload[0], strip_device_string(payload[1:]))


@dataclass(frozen=True)
class SyncTimeReport:
    pc_timestamp_us: int  # T1 echoed
    mcu_rx_us: int  # T2
    mcu_tx_us: int  # T3

    @classmethod
    def unpack(cls, payload: bytes) -> "SyncTimeReport":
        t1, t2, t3 = struct.unpack("<QQQ", payload)
        return cls(t1, t2, t3)


@dataclass(frozen=True)
class TemperatureReport:
    timestamp_us: int
    raw_decidegrees: int  # int16, units of 0.1 °C

    @classmethod
    def unpack(cls, payload: bytes) -> "TemperatureReport":
        ts, t = struct.unpack("<Qh", payload)
        return cls(ts, t)

    @property
    def celsius(self) -> float:
        return self.raw_decidegrees / 10.0


@dataclass(frozen=True)
class SequenceErrorReport:
    expected_seq: int
    received_seq: int

    @classmethod
    def unpack(cls, payload: bytes) -> "SequenceErrorReport":
        e, r = struct.unpack("<BB", payload)
        return cls(e, r)


@dataclass(frozen=True)
class SyncPinConfig:
    pin: int  # 1..5
    mode: SyncPinMode
    polarity: SyncPinPolarity

    def pack(self) -> bytes:
        return struct.pack("<BBB", self.pin, self.mode, self.polarity)

    @classmethod
    def unpack(cls, payload: bytes) -> "SyncPinConfig":
        p, m, pol = struct.unpack("<BBB", payload)
        return cls(p, SyncPinMode(m), SyncPinPolarity(pol))


def pack_sync_time(pc_timestamp_us: int) -> bytes:
    return struct.pack("<Q", pc_timestamp_us)


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
