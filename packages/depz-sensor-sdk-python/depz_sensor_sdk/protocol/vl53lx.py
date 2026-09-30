"""VL53L 1D-family register-bridge wire codecs, protocol v2.01
(contracts/12_SENSOR_VL53LX.md).

One firmware (`APP_VL53L0_4`) serves VL53L0X, VL53L1CX, VL53L1CB, VL53L3CX,
VL53L4CD and VL53L4CX. It is the VL53L4CD bridge of contract 10 with the three
sensor-specific facts moved to the host: the register-address width
(VL53_SET_ADDR_WIDTH, new), the interrupt-release writes (carried by
VL53_START_STREAM) and the boot handshake (no longer inside VL53_XSHUT). READ_REG,
WRITE_REG, XSHUT, STOP_STREAM, SET_I2C_SPEED and the REG_DATA / STREAM reports
are the contract-10 codecs, reused from `protocol.vl53l4`. v2.01 (firmware
v0.24) adds VL53_CLEAR_I2C_ERRORS; a v2.00 bridge refuses it with ERR_INVALID_CMD.
"""

from __future__ import annotations

import struct
from dataclasses import dataclass
from enum import IntEnum

from .vl53l4 import (  # noqa: F401  (re-exported: identical on the wire)
    I2C_KHZ_STEPS,
    SF_INT_ACT_HIGH,
    XFER_MAX,
    XSHUT_OFF,
    XSHUT_ON,
    XSHUT_RESET,
    RegData,
    StreamData,
    pack_read_reg,
    pack_set_i2c_speed,
    pack_write_reg,
    pack_xshut,
)


class Vl53lxCmd(IntEnum):
    READ_REG = 0x32
    WRITE_REG = 0x33
    XSHUT = 0x34
    START_STREAM = 0x35
    STOP_STREAM = 0x36
    GET_INFO = 0x37
    SET_I2C_SPEED = 0x38
    SET_ADDR_WIDTH = 0x39
    CLEAR_I2C_ERRORS = 0x3A  # v2.01


class Vl53lxRpt(IntEnum):
    REG_DATA = 0x91
    INFO = 0x92
    STREAM = 0x93


#: Interrupt-release steps a stream may carry (VL53_CLEAR_STEPS_WIRE_MAX).
CLEAR_STEPS_MAX = 4
INFO_SIZE = 23


def pack_start_stream(
    addr: int, length: int, clear: tuple = (), flags: int = 0
) -> bytes:
    """vl53_start_stream_t: block, flags, then `clear` = ((addr, value), ...)
    played by the bridge after every block read (0..4 steps)."""
    if len(clear) > CLEAR_STEPS_MAX:
        raise ValueError(f"at most {CLEAR_STEPS_MAX} interrupt-release steps")
    out = struct.pack("<HHBB", addr, length, flags, len(clear))
    for step_addr, value in clear:
        out += struct.pack("<HB", step_addr, value)
    return out


def pack_set_addr_width(width: int) -> bytes:
    if width not in (1, 2):
        raise ValueError("register address width is 1 or 2 bytes")
    return struct.pack("<B", width)


@dataclass(frozen=True)
class Vl53lxInfo:
    """RPT_VL53_INFO (v2.00, 23 bytes) — bridge state only; the bridge reads
    no sensor register. Counters are free-running (wrap silently): watch
    increments. `slots_skipped` = a slot that never got the bus, `i2c_errors`
    = a bus that answered badly (since power-up, the last XSHUT reset or
    VL53_CLEAR_I2C_ERRORS — the SDK sends that at the end of every sensor
    init, so the NACKs of a resetting die are not counted), `frames_dropped` =
    a good sample the USB TX ring had no room for (since the stream was armed)."""

    int_edges: int
    slots_skipped: int
    i2c_errors: int
    last_i2c_error: int  # 0 none, 1 NACK, 2 TIMEOUT, 3 BUS_ERROR
    xshut_level: int
    int_level: int
    i2c_khz: int
    addr_width: int
    n_clear: int
    frames_dropped: int

    @classmethod
    def unpack(cls, payload: bytes) -> "Vl53lxInfo":
        if len(payload) < INFO_SIZE:
            raise ValueError(f"RPT_VL53_INFO: expected {INFO_SIZE} bytes, got {len(payload)}")
        return cls(*struct.unpack_from("<IIIBBBHBBI", payload))
