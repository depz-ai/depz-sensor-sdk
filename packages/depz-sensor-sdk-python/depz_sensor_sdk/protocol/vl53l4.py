"""VL53L4 register-bridge wire codecs (contracts/10_SENSOR_VL53L4.md)."""

from __future__ import annotations

import struct
from dataclasses import dataclass
from enum import IntEnum


class Vl53l4Cmd(IntEnum):
    READ_REG = 0x32
    WRITE_REG = 0x33
    XSHUT = 0x34
    START_STREAM = 0x35
    STOP_STREAM = 0x36
    GET_INFO = 0x37
    SET_I2C_SPEED = 0x38


class Vl53l4Rpt(IntEnum):
    REG_DATA = 0x91
    INFO = 0x92
    STREAM = 0x93


# STM32 I2C NBYTES is 8 bit and a write spends two of them on the register
# address; the firmware applies the same 253 to both directions.
XFER_MAX = 253

# VL53_XSHUT actions.
XSHUT_OFF = 0
XSHUT_ON = 1
XSHUT_RESET = 2  # blocking on the MCU (~3 ms); answered after the boot handshake

# VL53_START_STREAM flags: interrupt polarity, mirroring bit 4 of
# GPIO_HV_MUX__CTRL (0x0030). Clear (default): INT active low.
SF_INT_ACT_HIGH = 0x02

# Nominal SCL steps the firmware carries a TIMINGR for (VL53_SET_I2C_SPEED
# clamps to the nearest one).
I2C_KHZ_STEPS = (100, 200, 400, 500, 600, 700, 800, 900, 1000)


def pack_read_reg(addr: int, length: int) -> bytes:
    return struct.pack("<HH", addr, length)


def pack_write_reg(addr: int, data: bytes) -> bytes:
    return struct.pack("<H", addr) + data


def pack_xshut(action: int) -> bytes:
    return struct.pack("<B", action)


def pack_start_stream(addr: int, length: int, flags: int = 0) -> bytes:
    return struct.pack("<HHB", addr, length, flags)


def pack_set_i2c_speed(khz: int) -> bytes:
    return struct.pack("<H", khz)


@dataclass(frozen=True)
class RegData:
    cmd: int  # echoed READ_REG opcode
    timestamp_us: int  # MCU uptime at I2C-read completion
    data: bytes

    @classmethod
    def unpack(cls, payload: bytes) -> "RegData":
        cmd, ts = struct.unpack_from("<BQ", payload)
        return cls(cmd, ts, payload[9:])


_INFO_FORMAT = "<IIIBHBBBBH"  # rpt_vl53_info_t, 21 bytes

#: last_i2c_error values in RPT_VL53_INFO.
I2C_ERROR_NAMES = {0: "none", 1: "NACK", 2: "TIMEOUT", 3: "BUS_ERROR"}


@dataclass(frozen=True)
class Vl53l4Info:
    """RPT_VL53_INFO — bridge diagnostics. Counters are free-running and wrap
    silently; watch increments, not absolute values."""

    int_edges: int
    slots_skipped: int
    i2c_errors: int
    last_i2c_error: int
    model_id: int  # 0x010F..0x0110 — expected 0xEBAA
    fw_status: int  # 0x00E5 — expected 0x03 (booted)
    initialized: int  # 1 = MODEL_ID matched on this read
    xshut_level: int
    int_level: int
    i2c_khz: int

    @classmethod
    def unpack(cls, payload: bytes) -> "Vl53l4Info":
        return cls(*struct.unpack_from(_INFO_FORMAT, payload))


@dataclass(frozen=True)
class StreamData:
    """RPT_VL53_STREAM — one streamed register block. `addr`/`length` echo the
    stream configuration so each report is self-describing."""

    timestamp_us: int  # MCU uptime at the INT edge (the sensor event)
    addr: int
    length: int
    data: bytes

    @classmethod
    def unpack(cls, payload: bytes) -> "StreamData":
        ts, addr, length = struct.unpack_from("<QHH", payload)
        return cls(ts, addr, length, payload[12 : 12 + length])
