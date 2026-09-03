"""BNO086 bridge-level wire codecs (contracts/05_SENSOR_BNO086.md §1–2).

The MCU is a thin SHTP pass-through: SEND_SHTP_PACKET carries a raw SHTP
frame to the sensor, and every inbound SHTP frame arrives as RPT_DATA.
Per ERRATA E2 the RPT_DATA `cmd` echo is always 0x00 — correlation happens
at the SH-2 layer, never here.
"""

from __future__ import annotations

import struct
from dataclasses import dataclass
from enum import IntEnum


class Bno086Cmd(IntEnum):
    SENSOR_RESET = 0x32  # hardware reset via nRST; RPT_STATUS OK
    SENSOR_WAKE_UP = 0x33  # 1 ms WAKE (PS0) pulse; RPT_STATUS OK
    SEND_SHTP_PACKET = 0x34  # payload = raw SHTP frame; RPT_STATUS OK/ERR_BUSY


class Bno086Rpt(IntEnum):
    DATA = 0x91  # rpt_data_t: cmd u8, timestamp_us u64, raw SHTP frame


# The MCU keeps two fixed 64-byte SHTP transmit slots (ERRATA E2). A third
# in-flight SEND_SHTP_PACKET gets RPT_STATUS(ERR_BUSY); back off >= 200 ms.
TX_SLOTS = 2
TX_SLOT_SIZE = 64
BUSY_BACKOFF_S = 0.2


@dataclass(frozen=True)
class Bno086Data:
    """One RPT_DATA report: an SHTP frame captured from the sensor bus."""

    cmd: int  # always 0x00 in practice (ERRATA E2) — never correlate on it
    timestamp_us: int  # MCU uptime at frame capture (µs)
    shtp: bytes  # raw SHTP frame (4-byte header + cargo fragment)

    @classmethod
    def unpack(cls, payload: bytes) -> "Bno086Data":
        cmd, ts = struct.unpack_from("<BQ", payload)
        return cls(cmd, ts, payload[9:])
