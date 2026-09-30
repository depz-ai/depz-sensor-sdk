"""BNO055 register-bridge wire codecs (contracts/13_SENSOR_BNO055.md)."""

from __future__ import annotations

import struct
from dataclasses import dataclass
from enum import IntEnum


class Bno055Cmd(IntEnum):
    READ_REG = 0x32
    WRITE_REG = 0x33
    RESET = 0x34
    START_STREAM = 0x35
    STOP_STREAM = 0x36
    GET_INFO = 0x37


class Bno055Rpt(IntEnum):
    REG_DATA = 0x91
    INFO = 0x92
    STREAM = 0x93


#: Max bytes per READ_REG / WRITE_REG / streamed block; `addr + len` ≤ 0x100.
XFER_MAX = 128

# BNO_START_STREAM trigger.
TRIGGER_TIMER = 0  # read every period_ms (the only data trigger on SW rev 03.11)
TRIGGER_INT = 1  # read on the INT rising edge; period_ms is a missed-edge watchdog

#: BNO_RESET answers after the sensor's ~0.5 s boot handshake.
RESET_TIMEOUT_S = 3.0


def pack_read_reg(addr: int, length: int) -> bytes:
    return struct.pack("<BB", addr, length)


def pack_write_reg(addr: int, data: bytes) -> bytes:
    return struct.pack("<B", addr) + data


def pack_start_stream(trigger: int, addr: int, length: int, period_ms: int) -> bytes:
    return struct.pack("<BBBH", trigger, addr, length, period_ms)


@dataclass(frozen=True)
class RegData:
    cmd: int  # echoed READ_REG opcode
    timestamp_us: int  # MCU uptime at I2C-read completion
    data: bytes

    @classmethod
    def unpack(cls, payload: bytes) -> "RegData":
        cmd, ts = struct.unpack_from("<BQ", payload)
        return cls(cmd, ts, payload[9:])


_INFO_FORMAT = "<BBBBBHBBBIHHHIIIHBBH"  # rpt_bno_info_t, 38 bytes

#: last_i2c_error values in RPT_BNO_INFO.
I2C_ERROR_NAMES = {0: "none", 1: "NACK", 2: "TIMEOUT", 3: "BUS_ERROR"}

#: Identity registers 0x00..0x03 of a healthy BNO055.
EXPECTED_CHIP_ID = 0xA0
EXPECTED_ACC_ID = 0xFB
EXPECTED_MAG_ID = 0x32
EXPECTED_GYR_ID = 0x0F


@dataclass(frozen=True)
class Bno055Info:
    """RPT_BNO_INFO — sensor identity (registers 0x00..0x06) plus bridge
    diagnostics. Counters are free-running and wrap silently; watch
    increments, not absolute values. A rising `sensor_resets` means the
    bridge pulsed nRESET to recover the bus: the sensor is back in CONFIG
    mode and the host must restore its configuration."""

    i2c_addr: int  # 7-bit sensor address in use (0x28)
    chip_id: int  # expected 0xA0
    acc_id: int  # expected 0xFB
    mag_id: int  # expected 0x32
    gyr_id: int  # expected 0x0F
    sw_rev: int  # sensor firmware, e.g. 0x0311 = 03.11
    bl_rev: int
    initialized: int  # 1 = chip-ID handshake passed
    int_level: int  # current INT pin level
    int_edges: int  # EXTI rising edges (counted only while an INT stream is armed)
    read_min_us: int  # streamed block read, since the last START_STREAM
    read_max_us: int
    read_avg_us: int
    tx_dropped: int  # packets refused by a full USB TX ring
    i2c_errors: int  # failed transfers since the last reset handshake
    slots_skipped: int  # stream slots dropped: bus still busy
    bus_recoveries: int  # rung-2 bit-bang bus releases
    last_i2c_error: int  # I2C_ERROR_NAMES
    sensor_resets: int  # rung-3 nRESET recoveries (sensor back in CONFIG)
    loop_max_us: int  # longest main-loop iteration since START_STREAM

    @classmethod
    def unpack(cls, payload: bytes) -> "Bno055Info":
        return cls(*struct.unpack_from(_INFO_FORMAT, payload))

    @property
    def ids_ok(self) -> bool:
        return (self.chip_id, self.acc_id, self.mag_id, self.gyr_id) == (
            EXPECTED_CHIP_ID,
            EXPECTED_ACC_ID,
            EXPECTED_MAG_ID,
            EXPECTED_GYR_ID,
        )

    @property
    def sw_rev_text(self) -> str:
        """Sensor firmware revision as Bosch writes it: 0x0311 → "03.11"."""
        return f"{self.sw_rev >> 8:02X}.{self.sw_rev & 0xFF:02X}"


@dataclass(frozen=True)
class StreamData:
    """RPT_BNO_REG_STREAM — one streamed register block. `addr`/`length`
    echo the stream configuration so each report is self-describing."""

    timestamp_us: int  # MCU uptime at the trigger (timer expiry or INT edge)
    addr: int
    length: int
    data: bytes

    @classmethod
    def unpack(cls, payload: bytes) -> "StreamData":
        ts, addr, length = struct.unpack_from("<QBB", payload)
        return cls(ts, addr, length, payload[10 : 10 + length])
