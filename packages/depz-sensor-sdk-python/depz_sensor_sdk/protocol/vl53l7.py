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


def pack_pin_ctrl(action: int) -> bytes:
    return struct.pack("<B", action)


def pack_set_i2c_speed(khz: int) -> bytes:
    return struct.pack("<H", khz)


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
