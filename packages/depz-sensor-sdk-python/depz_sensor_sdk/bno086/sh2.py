"""SH-2 control-channel builders and parsers (contracts/05_SENSOR_BNO086.md §6).

Pure request-builders + response-parsers — no I/O, no timing. The device
layer owns the SHTP framing, sequence numbers for the SHTP header, and the
waiting/correlation. All multi-byte fields little-endian.
"""

from __future__ import annotations

import struct
from dataclasses import dataclass, field
from enum import IntEnum

from ..errors import DepzError


class Sh2Error(DepzError):
    """SH-2 level failure (bad response status, FRS error, ...)."""


class ControlReport(IntEnum):
    """Report IDs on SHTP channel 2 (control)."""

    COMMAND_RESPONSE = 0xF1
    COMMAND_REQUEST = 0xF2
    FRS_READ_RESPONSE = 0xF3
    FRS_READ_REQUEST = 0xF4
    FRS_WRITE_RESPONSE = 0xF5
    FRS_WRITE_DATA = 0xF6
    FRS_WRITE_REQUEST = 0xF7
    PRODUCT_ID_RESPONSE = 0xF8
    PRODUCT_ID_REQUEST = 0xF9
    GET_FEATURE_RESPONSE = 0xFC
    SET_FEATURE_COMMAND = 0xFD
    GET_FEATURE_REQUEST = 0xFE


class Sh2Command(IntEnum):
    """`command` field of Command Request/Response (0xF2/0xF1)."""

    ERRORS = 0x01
    COUNTER = 0x02
    TARE = 0x03
    INITIALIZE = 0x04
    SAVE_DCD = 0x06
    ME_CALIBRATE = 0x07
    PERIODIC_DCD_CONFIG = 0x09
    GET_OSCILLATOR_TYPE = 0x0A
    CLEAR_DCD_AND_RESET = 0x0B


class OscillatorType(IntEnum):
    """Get-Oscillator-Type (command 0x0A) result (r[0])."""

    INTERNAL = 0
    EXT_CRYSTAL = 1
    EXT_CLOCK = 2


class ErrorSource(IntEnum):
    """`source` field of an error record (SH-2 §6.4.1)."""

    MOTION_ENGINE = 1
    MOTION_HUB = 2
    SENSOR_HUB = 3
    CHIP = 4
    NO_MORE_ERRORS = 255  # sentinel: end of the error queue


# Counter subcommands (command 0x02, P0).
COUNTS_GET = 0
COUNTS_CLEAR = 1


def counts_get_params(sensor_id: int) -> bytes:
    """Counter command 0x02: get event counts for `sensor_id`."""
    return bytes((COUNTS_GET, sensor_id & 0xFF))


def counts_clear_params(sensor_id: int) -> bytes:
    """Counter command 0x02: clear event counts for `sensor_id`."""
    return bytes((COUNTS_CLEAR, sensor_id & 0xFF))


def errors_params(severity: int = 0) -> bytes:
    """Errors command 0x01: return errors of `severity` or greater (0 = all)."""
    return bytes((severity & 0xFF,))


@dataclass(frozen=True)
class ErrorRecord:
    """One error queue entry (command 0x01 response, r[0..5])."""

    severity: int
    seq: int
    source: int  # ErrorSource
    error: int
    module: int
    code: int

    @classmethod
    def from_response(cls, resp: "CommandResponse") -> "ErrorRecord":
        r = resp.r
        return cls(r[0], r[1], r[2], r[3], r[4], r[5])


@dataclass(frozen=True)
class Counts:
    """Per-sensor event counts (command 0x02 get response, 2 messages)."""

    sensor_id: int
    offered: int
    accepted: int
    on: int
    attempted: int


class TareBasis(IntEnum):
    """Rotation vector used as the tare reference (Tare Now P2)."""

    ROTATION_VECTOR = 0
    GAME_ROTATION_VECTOR = 1
    GEOMAGNETIC_ROTATION_VECTOR = 2
    GYRO_INTEGRATED_RV = 3
    ARVR_STABILIZED_RV = 4
    ARVR_STABILIZED_GAME_RV = 5


class TareAxis(IntEnum):
    X = 1
    Y = 2
    Z = 4
    ALL = 7


# ── feature control (0xFD / 0xFE / 0xFC) ─────────────────────────────────────


def build_set_feature(
    sensor_id: int,
    interval_us: int,
    batch_us: int = 0,
    sensitivity: int = 0,
    flags: int = 0,
    cfg_word: int = 0,
) -> bytes:
    """Set Feature Command (0xFD), 17 bytes.

    `interval_us` = 0 disables the sensor. `sensitivity` units are
    sensor-dependent (change sensitivity, u16); `flags` bit meanings per
    SH-2 §6.5.4; `cfg_word` is the sensor-specific configuration u32."""
    return struct.pack(
        "<BBBHIII",
        ControlReport.SET_FEATURE_COMMAND,
        sensor_id,
        flags & 0xFF,
        sensitivity & 0xFFFF,
        interval_us & 0xFFFFFFFF,
        batch_us & 0xFFFFFFFF,
        cfg_word & 0xFFFFFFFF,
    )


def build_get_feature_request(sensor_id: int) -> bytes:
    """Get Feature Request (0xFE), 2 bytes."""
    return bytes((ControlReport.GET_FEATURE_REQUEST, sensor_id))


@dataclass(frozen=True)
class FeatureResponse:
    """Get Feature Response (0xFC), 17 bytes — the rates in effect."""

    sensor_id: int
    flags: int
    sensitivity: int
    interval_us: int  # actual report interval granted by the hub
    batch_us: int
    cfg_word: int

    @classmethod
    def unpack(cls, payload: bytes) -> "FeatureResponse":
        rid, sid, flags, sens, interval, batch, cfg = struct.unpack_from(
            "<BBBHIII", payload
        )
        if rid != ControlReport.GET_FEATURE_RESPONSE:
            raise Sh2Error(f"not a get-feature response: 0x{rid:02X}")
        return cls(sid, flags, sens, interval, batch, cfg)


# ── product ID (0xF9 / 0xF8) ─────────────────────────────────────────────────


def build_product_id_request() -> bytes:
    return bytes((ControlReport.PRODUCT_ID_REQUEST, 0x00))


@dataclass(frozen=True)
class ProductId:
    """Product ID Response (0xF8), 16 bytes. The sensor sends one response
    per subsystem (typically 2); reset_cause per SH-2 §6.4.5.2."""

    reset_cause: int
    sw_version_major: int
    sw_version_minor: int
    sw_part_number: int
    sw_build_number: int
    sw_version_patch: int

    @classmethod
    def unpack(cls, payload: bytes) -> "ProductId":
        rid, cause, maj, mnr, part, build, patch = struct.unpack_from(
            "<BBBBIIH", payload
        )
        if rid != ControlReport.PRODUCT_ID_RESPONSE:
            raise Sh2Error(f"not a product-id response: 0x{rid:02X}")
        return cls(cause, maj, mnr, part, build, patch)

    @property
    def version(self) -> str:
        return f"{self.sw_version_major}.{self.sw_version_minor}.{self.sw_version_patch}"


# ── command channel (0xF2 / 0xF1) ────────────────────────────────────────────


def build_command_request(seq: int, command: int, params: bytes = b"") -> bytes:
    """Command Request (0xF2), 12 bytes: id, seq, command, P0..P8."""
    if len(params) > 9:
        raise ValueError("command request carries at most 9 parameter bytes")
    return bytes((ControlReport.COMMAND_REQUEST, seq & 0xFF, command)) + params.ljust(
        9, b"\x00"
    )


@dataclass(frozen=True)
class CommandResponse:
    """Command Response (0xF1), 16 bytes.

    `command_seq` echoes the request's sequence number (correlate on it plus
    `command`); `response_seq` counts multiple responses to one request.
    R0 is the status word for most commands (0 = success)."""

    seq: int
    command: int
    command_seq: int
    response_seq: int
    r: tuple[int, ...]  # R0..R10

    @classmethod
    def unpack(cls, payload: bytes) -> "CommandResponse":
        if payload[0] != ControlReport.COMMAND_RESPONSE:
            raise Sh2Error(f"not a command response: 0x{payload[0]:02X}")
        return cls(payload[1], payload[2], payload[3], payload[4], tuple(payload[5:16]))

    @property
    def status(self) -> int:
        return self.r[0]


def tare_now_params(axes: int = TareAxis.ALL, basis: int = TareBasis.ROTATION_VECTOR) -> bytes:
    """Tare subcommand 0 — tare `axes` (bitmap X=1,Y=2,Z=4) using `basis`."""
    return bytes((0x00, axes, basis))


def persist_tare_params() -> bytes:
    """Tare subcommand 1 — persist current tare into FRS."""
    return bytes((0x01,))


def set_reorientation_params(x: float, y: float, z: float, w: float) -> bytes:
    """Tare subcommand 2 — set reorientation quaternion.

    P1..P8 are four int16 Q14 components (the 8 available parameter bytes
    only fit Q14 halves; the *FRS* System Orientation record is the one that
    stores Q30 words). All-zero clears the reorientation."""
    q14 = [round(v * (1 << 14)) for v in (x, y, z, w)]
    for v in q14:
        if not -32768 <= v <= 32767:
            raise ValueError("quaternion component out of Q14 int16 range")
    return bytes((0x02,)) + struct.pack("<4h", *q14)


def me_calibration_params(
    accel: bool, gyro: bool, mag: bool, planar: bool = False, subcommand: int = 0
) -> bytes:
    """ME Calibration (command 0x07). subcommand 0 = configure, 1 = get."""
    return bytes((int(accel), int(gyro), int(mag), subcommand, int(planar)))


ME_CAL_GET = 0x01  # subcommand: report current ME calibration config


def periodic_dcd_params(enable: bool) -> bytes:
    """Periodic DCD save config (command 0x09). P0: 0 = enable, 1 = disable.
    No command response is generated."""
    return bytes((0x00 if enable else 0x01,))


# ── FRS (flash record system) ────────────────────────────────────────────────


class FrsRecordId(IntEnum):
    """FRS record IDs used by this SDK (SH-2 figure 28; metadata records)."""

    STATIC_CALIBRATION_AGM = 0x7979
    NOMINAL_CALIBRATION = 0x4D4D
    DYNAMIC_CALIBRATION = 0x1F1F
    ME_POWER_MGMT = 0xD3E2
    SYSTEM_ORIENTATION = 0x2D3E  # mounting quaternion, 4 × Q30 words
    ACCEL_ORIENTATION = 0x2D41
    GYROSCOPE_ORIENTATION = 0x2D46
    MAGNETOMETER_ORIENTATION = 0x2D4C
    ARVR_STABILIZATION_RV = 0x3E2D
    ARVR_STABILIZATION_GRV = 0x3E2E
    # Feature configuration records (SH-2 §5.1; write to tune detectors).
    SIG_MOTION_DETECT_CONFIG = 0xC274
    SHAKE_DETECT_CONFIG = 0x7D7D
    STABILITY_DETECTOR_CONFIG = 0xED85
    ACTIVITY_TRACKER_CONFIG = 0xED88  # personal-activity-classifier config


# Per-sensor metadata FRS record IDs (subset used by get_metadata()).
METADATA_RECORDS: dict[int, int] = {
    0x14: 0xE301,  # raw accelerometer
    0x01: 0xE302,  # accelerometer
    0x04: 0xE303,  # linear acceleration
    0x06: 0xE304,  # gravity
    0x15: 0xE305,  # raw gyroscope
    0x02: 0xE306,  # gyroscope calibrated
    0x07: 0xE307,  # gyroscope uncalibrated
    0x16: 0xE308,  # raw magnetometer
    0x03: 0xE309,  # magnetometer calibrated
    0x0F: 0xE30A,  # magnetometer uncalibrated
    0x05: 0xE30B,  # rotation vector
    0x08: 0xE30C,  # game rotation vector
    0x09: 0xE30D,  # geomagnetic rotation vector
    0x10: 0xE313,  # tap detector
    0x18: 0xE314,  # step detector
    0x11: 0xE315,  # step counter
    0x12: 0xE316,  # significant motion
    0x13: 0xE317,  # stability classifier
    0x19: 0xE318,  # shake detector
    0x1E: 0xE31C,  # personal activity classifier
    0x28: 0xE322,  # ARVR-stabilized RV
    0x29: 0xE323,  # ARVR-stabilized game RV
    0x2A: 0xE324,  # gyro-integrated RV
}


class FrsStatus(IntEnum):
    """FRS Read Response status (low nibble of the len/status byte)."""

    NO_ERROR = 0
    UNRECOGNIZED_FRS_TYPE = 1
    BUSY = 2
    READ_COMPLETED = 3
    OFFSET_OUT_OF_RANGE = 4
    RECORD_EMPTY = 5
    BLOCK_COMPLETED = 6
    BLOCK_AND_READ_COMPLETED = 7
    DEVICE_ERROR = 8


class FrsWriteStatus(IntEnum):
    WORDS_RECEIVED = 0
    UNRECOGNIZED_FRS_TYPE = 1
    BUSY = 2
    WRITE_COMPLETED = 3
    WRITE_MODE_READY = 4
    WRITE_FAILED = 5
    NOT_IN_WRITE_MODE = 6
    INVALID_LENGTH = 7
    RECORD_VALID = 8
    RECORD_INVALID = 9


def build_frs_read_request(frs_type: int, offset_words: int = 0, block_words: int = 0) -> bytes:
    """FRS Read Request (0xF4), 8 bytes. block_words = 0 reads the record."""
    return struct.pack(
        "<BBHHH", ControlReport.FRS_READ_REQUEST, 0, offset_words, frs_type, block_words
    )


@dataclass(frozen=True)
class FrsReadResponse:
    """FRS Read Response (0xF3), 16 bytes; up to two data words per packet."""

    status: int  # FrsStatus
    data_length: int  # valid words in data0/data1 (0–2)
    offset_words: int
    data0: int
    data1: int
    frs_type: int

    @classmethod
    def unpack(cls, payload: bytes) -> "FrsReadResponse":
        rid, len_status, off, d0, d1, ftype, _ = struct.unpack_from("<BBHIIHH", payload)
        if rid != ControlReport.FRS_READ_RESPONSE:
            raise Sh2Error(f"not an FRS read response: 0x{rid:02X}")
        return cls(len_status & 0x0F, len_status >> 4, off, d0, d1, ftype)


def build_frs_write_request(frs_type: int, length_words: int) -> bytes:
    """FRS Write Request (0xF7), 6 bytes. length_words = 0 erases the record."""
    return struct.pack("<BBHH", ControlReport.FRS_WRITE_REQUEST, 0, length_words, frs_type)


def build_frs_write_data(offset_words: int, words: tuple[int, ...] | list[int]) -> bytes:
    """FRS Write Data (0xF6), 12 bytes; 1 or 2 words per packet."""
    if not 1 <= len(words) <= 2:
        raise ValueError("FRS write data carries 1 or 2 words")
    w0 = words[0]
    w1 = words[1] if len(words) > 1 else 0
    return struct.pack("<BBHII", ControlReport.FRS_WRITE_DATA, 0, offset_words, w0, w1)


@dataclass(frozen=True)
class FrsWriteResponse:
    """FRS Write Response (0xF5), 4 bytes."""

    status: int  # FrsWriteStatus
    offset_words: int

    @classmethod
    def unpack(cls, payload: bytes) -> "FrsWriteResponse":
        rid, status, off = struct.unpack_from("<BBH", payload)
        if rid != ControlReport.FRS_WRITE_RESPONSE:
            raise Sh2Error(f"not an FRS write response: 0x{rid:02X}")
        return cls(status, off)


@dataclass
class FrsReadSession:
    """Multi-packet FRS read state machine (pure — feed parsed responses).

    Usage: send `request()`, then `feed()` every 0xF3 for this record until
    it returns True; `words` holds the record. Error statuses raise."""

    frs_type: int
    words: list[int] = field(default_factory=list)
    done: bool = False

    def request(self) -> bytes:
        return build_frs_read_request(self.frs_type)

    def feed(self, resp: FrsReadResponse) -> bool:
        if resp.frs_type != self.frs_type:
            return self.done  # some other record's traffic — not ours
        if resp.status in (
            FrsStatus.UNRECOGNIZED_FRS_TYPE,
            FrsStatus.BUSY,
            FrsStatus.OFFSET_OUT_OF_RANGE,
            FrsStatus.RECORD_EMPTY,
            FrsStatus.DEVICE_ERROR,
        ):
            raise Sh2Error(
                f"FRS read 0x{self.frs_type:04X} failed: {FrsStatus(resp.status).name}"
            )
        for word in (resp.data0, resp.data1)[: resp.data_length]:
            self.words.append(word)
        if resp.status in (FrsStatus.READ_COMPLETED, FrsStatus.BLOCK_AND_READ_COMPLETED):
            self.done = True
        return self.done


@dataclass
class FrsWriteSession:
    """Multi-packet FRS write state machine (pure).

    Usage: send `request()`; then for every 0xF5 call `feed()` — it returns
    the next Write Data payload to send, or None; `done` flips on
    WRITE_COMPLETED. Error statuses raise."""

    frs_type: int
    words: list[int]
    offset: int = 0
    done: bool = False

    def request(self) -> bytes:
        return build_frs_write_request(self.frs_type, len(self.words))

    def _next_data(self) -> bytes | None:
        if self.offset >= len(self.words):
            return None
        chunk = self.words[self.offset : self.offset + 2]
        payload = build_frs_write_data(self.offset, chunk)
        self.offset += len(chunk)
        return payload

    def feed(self, resp: FrsWriteResponse) -> bytes | None:
        if resp.status in (
            FrsWriteStatus.UNRECOGNIZED_FRS_TYPE,
            FrsWriteStatus.BUSY,
            FrsWriteStatus.WRITE_FAILED,
            FrsWriteStatus.NOT_IN_WRITE_MODE,
            FrsWriteStatus.INVALID_LENGTH,
            FrsWriteStatus.RECORD_INVALID,
        ):
            raise Sh2Error(
                f"FRS write 0x{self.frs_type:04X} failed: {FrsWriteStatus(resp.status).name}"
            )
        if resp.status == FrsWriteStatus.WRITE_COMPLETED:
            self.done = True
            return None
        if resp.status in (FrsWriteStatus.WRITE_MODE_READY, FrsWriteStatus.WORDS_RECEIVED):
            return self._next_data()
        return None  # RECORD_VALID and friends: informational


# ── FRS metadata (best-effort, record version 3/4 layout) ───────────────────


@dataclass(frozen=True)
class SensorMetadata:
    """Parsed sensor metadata FRS record; `raw_words` is authoritative.

    Field packing follows the sh2 reference driver (revision-gated fields
    are 0 when the record predates them)."""

    me_version: int
    mh_version: int
    sh_version: int
    range_raw: int  # same units & Q point as the sensor's reports
    resolution_raw: int
    revision: int
    power_ma_q10: int  # mA in Q10
    min_period_us: int
    max_period_us: int  # revision >= 4 only
    fifo_max: int
    fifo_reserved: int
    batch_buffer_bytes: int
    q_point_1: int
    q_point_2: int
    q_point_3: int  # revision >= 3 only
    raw_words: tuple[int, ...]

    @classmethod
    def from_words(cls, words: list[int] | tuple[int, ...]) -> "SensorMetadata":
        w = list(words) + [0] * (10 - len(words))
        revision = w[3] & 0xFFFF
        return cls(
            me_version=w[0] & 0xFF,
            mh_version=(w[0] >> 8) & 0xFF,
            sh_version=(w[0] >> 16) & 0xFF,
            range_raw=w[1],
            resolution_raw=w[2],
            revision=revision,
            power_ma_q10=(w[3] >> 16) & 0xFFFF,
            min_period_us=w[4],
            fifo_max=w[5] & 0xFFFF,
            fifo_reserved=(w[5] >> 16) & 0xFFFF,
            batch_buffer_bytes=w[6] & 0xFFFF,
            q_point_1=w[7] & 0xFFFF,
            q_point_2=(w[7] >> 16) & 0xFFFF,
            q_point_3=(w[8] >> 16) & 0xFFFF if revision >= 3 else 0,
            max_period_us=w[9] if revision >= 4 else 0,
            raw_words=tuple(words),
        )
