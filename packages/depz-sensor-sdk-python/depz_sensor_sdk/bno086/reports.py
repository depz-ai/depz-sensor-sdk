"""SH-2 input-report catalog and parsers (contracts/05_SENSOR_BNO086.md §5).

Raw wire integers are authoritative and always preserved; scaled floats are
derived properties using the fixed Q points below (value = raw / 2**Q).
Report IDs and Q points cross-checked against the BNO08X datasheet
(CEVA 1000-3927) and the proven vendor host tool `bno086_tool.py`.

Timestamps: channel-3 cargos start with a Base Timestamp Reference (0xFB,
i32 base delta in 100 µs ticks, SUBTRACTED from the bridge capture time);
each report adds its own 14-bit delay (status bits 7:2 are the upper 6 bits,
byte 3 the lower 8; 100 µs resolution):

    timestamp_us = capture_us - base_delta*100 + delay*100
"""

from __future__ import annotations

import struct
from dataclasses import dataclass
from enum import IntEnum


class SensorId(IntEnum):
    """SH-2 input report IDs (datasheet §1.3.5, sh2 reference driver)."""

    ACCELEROMETER = 0x01  # calibrated, m/s², Q8
    GYROSCOPE = 0x02  # calibrated, rad/s, Q9
    MAGNETOMETER = 0x03  # calibrated, µT, Q4
    LINEAR_ACCELERATION = 0x04  # gravity removed, m/s², Q8
    ROTATION_VECTOR = 0x05  # quaternion Q14 + accuracy rad Q12
    GRAVITY = 0x06  # m/s², Q8
    UNCALIBRATED_GYROSCOPE = 0x07  # rad/s Q9 + bias
    GAME_ROTATION_VECTOR = 0x08  # quaternion Q14, no accuracy
    GEOMAGNETIC_ROTATION_VECTOR = 0x09  # quaternion Q14 + accuracy rad Q12
    PRESSURE = 0x0A  # hPa, Q20 (external sensor; unused on BNO086 boards)
    AMBIENT_LIGHT = 0x0B  # lux, Q8 (external)
    HUMIDITY = 0x0C  # %, Q8 (external)
    PROXIMITY = 0x0D  # cm, Q4 (external)
    TEMPERATURE = 0x0E  # °C, Q7 (external)
    UNCALIBRATED_MAGNETOMETER = 0x0F  # µT Q4 + hard-iron bias
    TAP_DETECTOR = 0x10
    STEP_COUNTER = 0x11
    SIGNIFICANT_MOTION = 0x12
    STABILITY_CLASSIFIER = 0x13
    RAW_ACCELEROMETER = 0x14  # ADC counts + sensor-clock timestamp
    RAW_GYROSCOPE = 0x15
    RAW_MAGNETOMETER = 0x16
    STEP_DETECTOR = 0x18
    SHAKE_DETECTOR = 0x19
    FLIP_DETECTOR = 0x1A
    PICKUP_DETECTOR = 0x1B
    STABILITY_DETECTOR = 0x1C
    PERSONAL_ACTIVITY_CLASSIFIER = 0x1E
    SLEEP_DETECTOR = 0x1F
    TILT_DETECTOR = 0x20
    POCKET_DETECTOR = 0x21
    CIRCLE_DETECTOR = 0x22
    HEART_RATE_MONITOR = 0x23
    ARVR_STABILIZED_RV = 0x28  # quaternion Q14 + accuracy rad Q12
    ARVR_STABILIZED_GAME_RV = 0x29  # quaternion Q14, no accuracy
    GYRO_INTEGRATED_RV = 0x2A  # channel 5 dense: quat Q14 + ang vel Q10


# In-cargo control IDs on the input channels
BASE_TIMESTAMP_REF = 0xFB  # + i32 base delta (100 µs ticks)
TIMESTAMP_REBASE = 0xFA  # + i32 rebase delta (100 µs ticks), batching


# Q point of the primary fields (value = raw / 2**Q); see module docstring.
Q_POINTS: dict[int, int] = {
    SensorId.ACCELEROMETER: 8,
    SensorId.LINEAR_ACCELERATION: 8,
    SensorId.GRAVITY: 8,
    SensorId.GYROSCOPE: 9,
    SensorId.UNCALIBRATED_GYROSCOPE: 9,
    SensorId.MAGNETOMETER: 4,
    SensorId.UNCALIBRATED_MAGNETOMETER: 4,
    SensorId.ROTATION_VECTOR: 14,
    SensorId.GAME_ROTATION_VECTOR: 14,
    SensorId.GEOMAGNETIC_ROTATION_VECTOR: 14,
    SensorId.ARVR_STABILIZED_RV: 14,
    SensorId.ARVR_STABILIZED_GAME_RV: 14,
    SensorId.GYRO_INTEGRATED_RV: 14,
    SensorId.PRESSURE: 20,
    SensorId.AMBIENT_LIGHT: 8,
    SensorId.HUMIDITY: 8,
    SensorId.PROXIMITY: 4,
    SensorId.TEMPERATURE: 7,
    SensorId.RAW_ACCELEROMETER: 0,
    SensorId.RAW_GYROSCOPE: 0,
    SensorId.RAW_MAGNETOMETER: 0,
}
RV_ACCURACY_Q = 12  # rotation-vector accuracy estimate, radians
GYRO_RV_ANGVEL_Q = 10  # gyro-integrated RV angular velocity, rad/s

# Total report length on the wire, 4-byte SH-2 header included
# (sh2 reference driver report-length table).
REPORT_LENGTHS: dict[int, int] = {
    SensorId.ACCELEROMETER: 10,
    SensorId.GYROSCOPE: 10,
    SensorId.MAGNETOMETER: 10,
    SensorId.LINEAR_ACCELERATION: 10,
    SensorId.ROTATION_VECTOR: 14,
    SensorId.GRAVITY: 10,
    SensorId.UNCALIBRATED_GYROSCOPE: 16,
    SensorId.GAME_ROTATION_VECTOR: 12,
    SensorId.GEOMAGNETIC_ROTATION_VECTOR: 14,
    SensorId.PRESSURE: 8,
    SensorId.AMBIENT_LIGHT: 8,
    SensorId.HUMIDITY: 6,
    SensorId.PROXIMITY: 6,
    SensorId.TEMPERATURE: 6,
    SensorId.UNCALIBRATED_MAGNETOMETER: 16,
    SensorId.TAP_DETECTOR: 5,
    SensorId.STEP_COUNTER: 12,
    SensorId.SIGNIFICANT_MOTION: 6,
    SensorId.STABILITY_CLASSIFIER: 6,
    SensorId.RAW_ACCELEROMETER: 16,
    SensorId.RAW_GYROSCOPE: 16,
    SensorId.RAW_MAGNETOMETER: 16,
    SensorId.STEP_DETECTOR: 8,
    SensorId.SHAKE_DETECTOR: 6,
    SensorId.FLIP_DETECTOR: 6,
    SensorId.PICKUP_DETECTOR: 8,
    SensorId.STABILITY_DETECTOR: 6,
    SensorId.PERSONAL_ACTIVITY_CLASSIFIER: 16,
    SensorId.SLEEP_DETECTOR: 6,
    SensorId.TILT_DETECTOR: 6,
    SensorId.POCKET_DETECTOR: 6,
    SensorId.CIRCLE_DETECTOR: 6,
    SensorId.HEART_RATE_MONITOR: 6,
    SensorId.ARVR_STABILIZED_RV: 14,
    SensorId.ARVR_STABILIZED_GAME_RV: 12,
    SensorId.GYRO_INTEGRATED_RV: 14,
}

STABILITY_NAMES = {0: "unknown", 1: "on_table", 2: "stationary", 3: "stable", 4: "motion"}
ACTIVITY_NAMES = {
    0: "unknown",
    1: "in_vehicle",
    2: "on_bicycle",
    3: "on_foot",
    4: "still",
    5: "tilting",
    6: "walking",
    7: "running",
    8: "on_stairs",
}


def _q(raw: int, q: int) -> float:
    return raw / (1 << q)


# ── report dataclasses ────────────────────────────────────────────────────────


@dataclass(frozen=True)
class Report:
    """Base for anything the sensor pushes; `timestamp_us` is absolute in the
    MCU clock domain (bridge capture time corrected by timebase + delay)."""

    sensor_id: int
    timestamp_us: int


@dataclass(frozen=True)
class InputReport(Report):
    """Channel-3/4 report with the common SH-2 header fields."""

    seq: int  # 8-bit rolling sample counter (drop detection)
    accuracy: int  # status bits 1:0 — 0 unreliable … 3 high
    delay_us: int  # report delay already folded into timestamp_us


@dataclass(frozen=True)
class Acceleration(InputReport):
    """0x01 accelerometer / 0x04 linear acceleration / 0x06 gravity (Q8)."""

    x_raw: int
    y_raw: int
    z_raw: int

    @property
    def x(self) -> float:  # m/s²
        return _q(self.x_raw, 8)

    @property
    def y(self) -> float:  # m/s²
        return _q(self.y_raw, 8)

    @property
    def z(self) -> float:  # m/s²
        return _q(self.z_raw, 8)


@dataclass(frozen=True)
class Gyroscope(InputReport):
    """0x02 calibrated gyroscope (Q9)."""

    x_raw: int
    y_raw: int
    z_raw: int

    @property
    def x(self) -> float:  # rad/s
        return _q(self.x_raw, 9)

    @property
    def y(self) -> float:  # rad/s
        return _q(self.y_raw, 9)

    @property
    def z(self) -> float:  # rad/s
        return _q(self.z_raw, 9)


@dataclass(frozen=True)
class Magnetometer(InputReport):
    """0x03 calibrated magnetic field (Q4)."""

    x_raw: int
    y_raw: int
    z_raw: int

    @property
    def x(self) -> float:  # µT
        return _q(self.x_raw, 4)

    @property
    def y(self) -> float:  # µT
        return _q(self.y_raw, 4)

    @property
    def z(self) -> float:  # µT
        return _q(self.z_raw, 4)


@dataclass(frozen=True)
class UncalibratedGyroscope(InputReport):
    """0x07 uncalibrated gyroscope + bias estimate (all Q9, rad/s)."""

    x_raw: int
    y_raw: int
    z_raw: int
    bias_x_raw: int
    bias_y_raw: int
    bias_z_raw: int

    @property
    def x(self) -> float:
        return _q(self.x_raw, 9)

    @property
    def y(self) -> float:
        return _q(self.y_raw, 9)

    @property
    def z(self) -> float:
        return _q(self.z_raw, 9)

    @property
    def bias(self) -> tuple[float, float, float]:  # rad/s
        return (_q(self.bias_x_raw, 9), _q(self.bias_y_raw, 9), _q(self.bias_z_raw, 9))


@dataclass(frozen=True)
class UncalibratedMagnetometer(InputReport):
    """0x0F uncalibrated magnetic field + hard-iron bias (all Q4, µT)."""

    x_raw: int
    y_raw: int
    z_raw: int
    bias_x_raw: int
    bias_y_raw: int
    bias_z_raw: int

    @property
    def x(self) -> float:
        return _q(self.x_raw, 4)

    @property
    def y(self) -> float:
        return _q(self.y_raw, 4)

    @property
    def z(self) -> float:
        return _q(self.z_raw, 4)

    @property
    def bias(self) -> tuple[float, float, float]:  # µT
        return (_q(self.bias_x_raw, 4), _q(self.bias_y_raw, 4), _q(self.bias_z_raw, 4))


@dataclass(frozen=True)
class RotationVector(InputReport):
    """Quaternion reports 0x05/0x08/0x09/0x28/0x29 (unit quaternion, Q14).

    `accuracy_raw` (Q12, radians) is present only for 0x05/0x09/0x28."""

    i_raw: int
    j_raw: int
    k_raw: int
    real_raw: int
    accuracy_raw: int | None = None

    @property
    def i(self) -> float:
        return _q(self.i_raw, 14)

    @property
    def j(self) -> float:
        return _q(self.j_raw, 14)

    @property
    def k(self) -> float:
        return _q(self.k_raw, 14)

    @property
    def real(self) -> float:
        return _q(self.real_raw, 14)

    @property
    def accuracy_rad(self) -> float | None:
        """Estimated heading accuracy in radians (None for game variants)."""
        return None if self.accuracy_raw is None else _q(self.accuracy_raw, RV_ACCURACY_Q)


@dataclass(frozen=True)
class GyroIntegratedRV(Report):
    """0x2A gyro-integrated rotation vector (channel 5, dense — no SH-2
    header). Quaternion Q14, angular velocity Q10 rad/s."""

    i_raw: int
    j_raw: int
    k_raw: int
    real_raw: int
    vx_raw: int
    vy_raw: int
    vz_raw: int

    @property
    def i(self) -> float:
        return _q(self.i_raw, 14)

    @property
    def j(self) -> float:
        return _q(self.j_raw, 14)

    @property
    def k(self) -> float:
        return _q(self.k_raw, 14)

    @property
    def real(self) -> float:
        return _q(self.real_raw, 14)

    @property
    def angular_velocity(self) -> tuple[float, float, float]:  # rad/s
        return (
            _q(self.vx_raw, GYRO_RV_ANGVEL_Q),
            _q(self.vy_raw, GYRO_RV_ANGVEL_Q),
            _q(self.vz_raw, GYRO_RV_ANGVEL_Q),
        )


@dataclass(frozen=True)
class ScalarReport(InputReport):
    """Environment reports 0x0A–0x0E: single value, Q from Q_POINTS."""

    value_raw: int

    @property
    def value(self) -> float:
        return _q(self.value_raw, Q_POINTS[self.sensor_id])


@dataclass(frozen=True)
class TapDetector(InputReport):
    """0x10 tap detector; `flags` bit 6 = double tap, bits 0–5 axis/sign."""

    flags: int

    @property
    def double_tap(self) -> bool:
        return bool(self.flags & 0x40)


@dataclass(frozen=True)
class StepCounter(InputReport):
    """0x11 step counter. NOTE: latency u32 µs at bytes 4–7, steps u16 at
    bytes 8–9 per SH-2; the vendor tool's `latency(2)+steps(2)` comment is
    wrong."""

    latency_us: int
    steps: int


@dataclass(frozen=True)
class StepDetector(InputReport):
    """0x18 step detector; latency from step event to report, µs."""

    latency_us: int


@dataclass(frozen=True)
class SignificantMotion(InputReport):
    """0x12 significant motion (1 = motion detected; sensor auto-disables)."""

    motion: int


@dataclass(frozen=True)
class StabilityClassifier(InputReport):
    """0x13 stability classification (see STABILITY_NAMES)."""

    classification: int

    @property
    def name(self) -> str:
        return STABILITY_NAMES.get(self.classification, str(self.classification))


@dataclass(frozen=True)
class ShakeDetector(InputReport):
    """0x19 shake detector; bits 0/1/2 = X/Y/Z shake."""

    flags: int


@dataclass(frozen=True)
class GenericEvent(InputReport):
    """Simple u16-value detectors: 0x1A flip, 0x1B pickup, 0x1C stability
    detector, 0x1F sleep, 0x20 tilt, 0x21 pocket, 0x22 circle, 0x23 HR."""

    value_raw: int


@dataclass(frozen=True)
class PersonalActivityClassifier(InputReport):
    """0x1E personal activity classifier (see ACTIVITY_NAMES).

    `confidences` are 0–100 per state, states 0–9 of the current page."""

    page_number: int
    end_of_sequence: bool
    most_likely_state: int
    confidences: tuple[int, ...]

    @property
    def most_likely_name(self) -> str:
        return ACTIVITY_NAMES.get(self.most_likely_state, str(self.most_likely_state))


@dataclass(frozen=True)
class RawSensor(InputReport):
    """0x14/0x15/0x16 raw ADC samples + sensor-clock timestamp (u32 µs).
    `temperature_raw` is populated only for the raw gyroscope."""

    x_raw: int
    y_raw: int
    z_raw: int
    sensor_timestamp_us: int
    temperature_raw: int = 0


@dataclass(frozen=True)
class UnknownReport(Report):
    """Unrecognized report ID: raw bytes from the ID to end of cargo (the
    length is unknowable, so parsing stops here)."""

    data: bytes


# ── parsers ───────────────────────────────────────────────────────────────────


def _i16(payload: bytes, off: int) -> int:
    return struct.unpack_from("<h", payload, off)[0]


def parse_input_cargo(payload: bytes, capture_timestamp_us: int) -> list[Report]:
    """Parse a channel-3/4 cargo into typed reports.

    `capture_timestamp_us` is the bridge RPT_DATA capture time (MCU uptime).
    Handles 0xFB base timestamp references and 0xFA rebases; every report's
    timestamp is `base + delay` where base = capture − base_delta·100 µs.
    """
    out: list[Report] = []
    base_us = capture_timestamp_us
    pos = 0
    n = len(payload)
    while pos < n:
        rid = payload[pos]
        if rid == BASE_TIMESTAMP_REF and pos + 5 <= n:
            (delta,) = struct.unpack_from("<i", payload, pos + 1)
            base_us = capture_timestamp_us - delta * 100
            pos += 5
            continue
        if rid == TIMESTAMP_REBASE and pos + 5 <= n:
            (delta,) = struct.unpack_from("<i", payload, pos + 1)
            base_us += delta * 100
            pos += 5
            continue
        length = REPORT_LENGTHS.get(rid)
        if length is None or pos + length > n:
            out.append(UnknownReport(rid, base_us, payload[pos:]))
            break
        rep = payload[pos : pos + length]
        seq, status, delay_lsb = rep[1], rep[2], rep[3]
        accuracy = status & 0x03
        delay_us = (((status >> 2) << 8) | delay_lsb) * 100
        ts = base_us + delay_us
        out.append(_decode_report(rid, rep, ts, seq, accuracy, delay_us))
        pos += length
    return out


def _decode_report(
    rid: int, rep: bytes, ts: int, seq: int, accuracy: int, delay_us: int
) -> Report:
    head = (rid, ts, seq, accuracy, delay_us)
    if rid in (
        SensorId.ACCELEROMETER,
        SensorId.LINEAR_ACCELERATION,
        SensorId.GRAVITY,
    ):
        return Acceleration(*head, _i16(rep, 4), _i16(rep, 6), _i16(rep, 8))
    if rid == SensorId.GYROSCOPE:
        return Gyroscope(*head, _i16(rep, 4), _i16(rep, 6), _i16(rep, 8))
    if rid == SensorId.MAGNETOMETER:
        return Magnetometer(*head, _i16(rep, 4), _i16(rep, 6), _i16(rep, 8))
    if rid == SensorId.UNCALIBRATED_GYROSCOPE:
        return UncalibratedGyroscope(*head, *struct.unpack_from("<6h", rep, 4))
    if rid == SensorId.UNCALIBRATED_MAGNETOMETER:
        return UncalibratedMagnetometer(*head, *struct.unpack_from("<6h", rep, 4))
    if rid in (
        SensorId.ROTATION_VECTOR,
        SensorId.GEOMAGNETIC_ROTATION_VECTOR,
        SensorId.ARVR_STABILIZED_RV,
    ):
        i, j, k, real, acc = struct.unpack_from("<5h", rep, 4)
        return RotationVector(*head, i, j, k, real, acc)
    if rid in (SensorId.GAME_ROTATION_VECTOR, SensorId.ARVR_STABILIZED_GAME_RV):
        i, j, k, real = struct.unpack_from("<4h", rep, 4)
        return RotationVector(*head, i, j, k, real, None)
    if rid in (SensorId.PRESSURE, SensorId.AMBIENT_LIGHT):
        return ScalarReport(*head, struct.unpack_from("<I", rep, 4)[0])
    if rid in (SensorId.HUMIDITY, SensorId.PROXIMITY):
        return ScalarReport(*head, struct.unpack_from("<H", rep, 4)[0])
    if rid == SensorId.TEMPERATURE:
        return ScalarReport(*head, _i16(rep, 4))
    if rid == SensorId.TAP_DETECTOR:
        return TapDetector(*head, rep[4])
    if rid == SensorId.STEP_COUNTER:
        latency, steps = struct.unpack_from("<IH", rep, 4)
        return StepCounter(*head, latency, steps)
    if rid == SensorId.STEP_DETECTOR:
        return StepDetector(*head, struct.unpack_from("<I", rep, 4)[0])
    if rid == SensorId.SIGNIFICANT_MOTION:
        return SignificantMotion(*head, struct.unpack_from("<H", rep, 4)[0])
    if rid == SensorId.STABILITY_CLASSIFIER:
        return StabilityClassifier(*head, rep[4])
    if rid == SensorId.SHAKE_DETECTOR:
        return ShakeDetector(*head, struct.unpack_from("<H", rep, 4)[0])
    if rid == SensorId.PERSONAL_ACTIVITY_CLASSIFIER:
        return PersonalActivityClassifier(
            *head, rep[4] & 0x7F, bool(rep[4] & 0x80), rep[5], tuple(rep[6:16])
        )
    if rid in (SensorId.RAW_ACCELEROMETER, SensorId.RAW_MAGNETOMETER):
        x, y, z = struct.unpack_from("<3h", rep, 4)
        (st,) = struct.unpack_from("<I", rep, 12)
        return RawSensor(*head, x, y, z, st)
    if rid == SensorId.RAW_GYROSCOPE:
        x, y, z, temp = struct.unpack_from("<4h", rep, 4)
        (st,) = struct.unpack_from("<I", rep, 12)
        return RawSensor(*head, x, y, z, st, temp)
    # In-table detectors without a dedicated class
    return GenericEvent(*head, struct.unpack_from("<H", rep, 4)[0])


def parse_gyro_rv_cargo(payload: bytes, capture_timestamp_us: int) -> GyroIntegratedRV | None:
    """Parse a channel-5 cargo (gyro-integrated RV, dense format).

    Two shapes seen on hardware (vendor tool): 7×i16 bare, or prefixed with
    0xFB + i32 base delta + u16 delay (both 100 µs ticks)."""
    ts = capture_timestamp_us
    if payload[:1] == bytes([BASE_TIMESTAMP_REF]):
        if len(payload) < 5 + 2 + 14:
            return None
        (delta,) = struct.unpack_from("<i", payload, 1)
        (delay,) = struct.unpack_from("<H", payload, 5)
        ts = capture_timestamp_us - delta * 100 + delay * 100
        payload = payload[7:]
    if len(payload) < 14:
        return None
    i, j, k, real, vx, vy, vz = struct.unpack_from("<7h", payload)
    return GyroIntegratedRV(SensorId.GYRO_INTEGRATED_RV, ts, i, j, k, real, vx, vy, vz)
