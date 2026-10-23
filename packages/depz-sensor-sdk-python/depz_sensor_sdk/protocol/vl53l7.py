"""VL53L5CX/L7CX/L7CH I2C register-bridge wire codecs (contracts/11_SENSOR_VL53L7.md).

Commands 0x32/0x33/0x35/0x36 and reports 0x91/0x93 are bit-for-bit the VL53L8
bridge (contract 04) and are reused from `protocol.vl53l8`; this module holds
only what the I2C board adds: PIN_CTRL, GET_INFO, SET_I2C_SPEED, RPT_VL53_INFO,
and its tighter transfer limits.
"""

from __future__ import annotations

import struct
from dataclasses import dataclass
from enum import IntEnum

from ._wire import check_uint, checked_pack


class Vl53l7Cmd(IntEnum):
    PIN_CTRL = 0x34
    GET_INFO = 0x37
    SET_I2C_SPEED = 0x38


class Vl53l7Rpt(IntEnum):
    VL53_INFO = 0x92


class PinAction(IntEnum):
    """VL53_PIN_CTRL actions. None is a true sensor reset (the board has no
    power GPIO): after LPN_OFF or SOFT_CYCLE the host must re-run init()."""

    LPN_OFF = 0  # stop streaming, drive LPn low: sensor I2C interface off
    LPN_ON = 1  # drive LPn high: interface on (power-up default)
    I2C_RST = 2  # pulse I2C_RST
    SOFT_CYCLE = 3  # stop streaming, LPn low 1 ms, high, I2C_RST pulse; clears I2C counters


class I2cError(IntEnum):
    """RPT_VL53_INFO.last_i2c_error."""

    OK = 0
    NACK = 1
    TIMEOUT = 2
    BUS_ERROR = 3


READ_MAX_LEN = 1536  # VL53LMZ_READ_MAX: READ_REG len 1..1536
WRITE_MAX_LEN = 2048  # VL53LMZ_XFER_MAX: WRITE_REG N 1..2048
STREAM_CHUNK_MAX = 1536  # bytes of frame data per RPT_VL53_FRAME chunk
INFO_SIZE = 20

#: Nominal SCL steps the firmware carries a timing for; others snap to nearest.
I2C_SPEED_STEPS_KHZ = (100, 200, 400, 500, 600, 700, 800, 900, 1000)


_REG_SPACE = 0x10000  # a bridge transfer never runs past register 0xFFFF


def pack_read_reg(addr: int, length: int) -> bytes:
    """VL53_READ_REG payload `addr u16, len u16` with the L5/L7 limits
    (contract 11 §2): `len` 1..1536 and `addr + len <= 0x10000`, else ValueError."""
    payload = checked_pack("<HH", addr=addr, length=length)
    if not 1 <= length <= READ_MAX_LEN or addr + length > _REG_SPACE:
        raise ValueError(f"READ_REG length must be 1..{READ_MAX_LEN} and addr+length <= 0x10000")
    return payload


def pack_write_reg(addr: int, data: bytes) -> bytes:
    """VL53_WRITE_REG payload `addr u16, data[N]` with the L5/L7 limits
    (contract 11 §2): N 1..2048 and `addr + N <= 0x10000`, else ValueError."""
    check_uint("addr", addr, 16)
    if not 1 <= len(data) <= WRITE_MAX_LEN or addr + len(data) > _REG_SPACE:
        raise ValueError(f"WRITE_REG data must be 1..{WRITE_MAX_LEN} bytes and addr+N <= 0x10000")
    return struct.pack("<H", addr) + bytes(data)


def pack_pin_ctrl(action: int) -> bytes:
    return checked_pack("<B", action=action)


def pack_set_i2c_speed(khz: int) -> bytes:
    return checked_pack("<H", khz=khz)


@dataclass(frozen=True)
class Vl53l7Info:
    """RPT_VL53_INFO: bridge state only (the sensor is never probed). All
    counters run from power-up / DEVICE_RESET; SOFT_CYCLE clears the I2C ones.
    Note the report carries no echoed command byte."""

    int_edges: int
    frames_dropped: int
    i2c_errors: int
    last_i2c_error: int
    lpn_level: int
    int_level: int
    i2c_khz: int
    frame_size: int
    streaming: bool

    @classmethod
    def unpack(cls, payload: bytes) -> "Vl53l7Info":
        if len(payload) < INFO_SIZE:
            raise ValueError(f"RPT_VL53_INFO: expected {INFO_SIZE} bytes, got {len(payload)}")
        edges, dropped, errors, last, lpn, intl, khz, fsize, streaming = struct.unpack_from(
            "<IIIBBBHHB", payload
        )
        return cls(edges, dropped, errors, last, lpn, intl, khz, fsize, bool(streaming))
