"""SR04 wire codecs (contracts/03_SENSOR_SR04.md)."""

from __future__ import annotations

import struct
from dataclasses import dataclass
from enum import IntEnum


class Sr04Cmd(IntEnum):
    GET_SAMPLE_PERIOD = 0x32
    SET_SAMPLE_PERIOD = 0x33
    GET_ECHO_DECAY = 0x34
    SET_ECHO_DECAY = 0x35
    MEASURE_ONCE = 0x36
    START_MEASUREMENT_LOOP = 0x37
    STOP_MEASUREMENT_LOOP = 0x38


class Sr04Rpt(IntEnum):
    DATA = 0x91
    SAMPLE_PERIOD = 0x92
    ECHO_DECAY = 0x93


ECHO_TIMEOUT = 0xFFFF  # echo_time_us sentinel: no echo received

SAMPLE_PERIOD_DEFAULT_US = 50_000
ECHO_DECAY_DEFAULT_US = 5_000
#: Window the *device* silently clamps SET_ECHO_DECAY into (contract 03 §3).
ECHO_DECAY_MIN_US = 4_000
ECHO_DECAY_MAX_US = 65_000
#: Widest value each wire field can carry. Distinct from the clamp window
#: above: sending 0xFFFF is legal (the device clamps it), sending 0x10000 is
#: not representable at all. `sr04.py` validates against these so an
#: out-of-range argument raises ValueError instead of leaking struct.error.
ECHO_DECAY_WIRE_MAX_US = 0xFFFF  # u16
SAMPLE_PERIOD_WIRE_MAX_US = 0xFFFF_FFFF  # u32


@dataclass(frozen=True)
class Sr04Data:
    source_cmd: int  # 0x36 single shot (host or SYNC_IN), 0x37 loop sample
    timestamp_us: int
    echo_time_us: int

    @classmethod
    def unpack(cls, payload: bytes) -> "Sr04Data":
        cmd, ts, echo = struct.unpack("<BQH", payload)
        return cls(cmd, ts, echo)


def pack_sample_period(period_us: int) -> bytes:
    return struct.pack("<I", period_us)


def unpack_sample_period(payload: bytes) -> int:
    return struct.unpack("<I", payload)[0]


def pack_echo_decay(decay_us: int) -> bytes:
    return struct.pack("<H", decay_us)


def unpack_echo_decay(payload: bytes) -> int:
    return struct.unpack("<H", payload)[0]


def distance_mm_from_echo(echo_time_us: int, air_temp_c: float | None = None) -> float | None:
    """Round-trip echo time → distance in mm; None for the timeout sentinel.

    Default speed of sound 343 m/s; with `air_temp_c` uses
    c = 331.3 + 0.606·T (m/s).
    """
    if echo_time_us == ECHO_TIMEOUT:
        return None
    c_m_s = 343.0 if air_temp_c is None else 331.3 + 0.606 * air_temp_c
    return echo_time_us * c_m_s / 2000.0
