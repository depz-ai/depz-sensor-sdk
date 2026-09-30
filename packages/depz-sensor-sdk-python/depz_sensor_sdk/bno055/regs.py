"""BNO055 register map and the pure codecs every SDK shares
(contracts/13_SENSOR_BNO055.md §4, Bosch BST-BNO055-DS000 rev 1.8).

Nothing here touches the wire: these functions turn register bytes into
values and back, so they are what `vectors/bno055.json` pins.
"""

from __future__ import annotations

import struct
from dataclasses import dataclass
from enum import IntEnum

# ── page 0 ──────────────────────────────────────────────────────────────────
REG_CHIP_ID = 0x00
REG_PAGE_ID = 0x07
REG_ACC_DATA = 0x08  # x, y, z  i16
REG_MAG_DATA = 0x0E
REG_GYR_DATA = 0x14
REG_EUL_DATA = 0x1A  # heading, roll, pitch
REG_QUA_DATA = 0x20  # w, x, y, z
REG_LIA_DATA = 0x28  # linear acceleration (gravity removed)
REG_GRV_DATA = 0x2E  # gravity vector
REG_TEMP = 0x34  # i8
REG_CALIB_STAT = 0x35
REG_ST_RESULT = 0x36
REG_INT_STA = 0x37  # clear-on-read — never part of a routine block read
REG_SYS_CLK_STATUS = 0x38
REG_SYS_STATUS = 0x39
REG_SYS_ERR = 0x3A
REG_UNIT_SEL = 0x3B
REG_OPR_MODE = 0x3D
REG_PWR_MODE = 0x3E
REG_SYS_TRIGGER = 0x3F
REG_TEMP_SOURCE = 0x40
REG_AXIS_MAP_CONFIG = 0x41
REG_AXIS_MAP_SIGN = 0x42
REG_SIC_MATRIX = 0x43  # 9 × i16, row-major
REG_CALIB_PROFILE = 0x55  # acc/mag/gyr offsets + acc/mag radius, 22 bytes
CALIB_PROFILE_LEN = 22

# ── page 1 ──────────────────────────────────────────────────────────────────
REG1_ACC_CONFIG = 0x08
REG1_MAG_CONFIG = 0x09
REG1_GYR_CONFIG_0 = 0x0A
REG1_GYR_CONFIG_1 = 0x0B
REG1_ACC_SLEEP_CONFIG = 0x0C
REG1_GYR_SLEEP_CONFIG = 0x0D
REG1_INT_MSK = 0x0F
REG1_INT_EN = 0x10
REG1_ACC_AM_THRES = 0x11
REG1_ACC_INT_SETTINGS = 0x12
REG1_ACC_HG_DURATION = 0x13
REG1_ACC_HG_THRES = 0x14
REG1_ACC_NM_THRES = 0x15
REG1_ACC_NM_SET = 0x16
REG1_GYR_INT_SETTING = 0x17
REG1_GYR_HR_X_SET = 0x18
REG1_GYR_DUR_X = 0x19
REG1_GYR_HR_Y_SET = 0x1A
REG1_GYR_DUR_Y = 0x1B
REG1_GYR_HR_Z_SET = 0x1C
REG1_GYR_DUR_Z = 0x1D
REG1_GYR_AM_THRES = 0x1E
REG1_GYR_AM_SET = 0x1F
REG1_UNIQUE_ID = 0x50  # 16 bytes
UNIQUE_ID_LEN = 16

#: Page-1 registers 0x11..0x1F are the motion-interrupt settings, written raw.
INTERRUPT_SETTING_REGS = range(REG1_ACC_AM_THRES, REG1_GYR_AM_SET + 1)

#: The one block that carries every output channel: 0x08 (ACC_DATA_X_LSB)
#: through 0x35 (CALIB_STAT), 46 bytes.
FULL_BLOCK = (REG_ACC_DATA, REG_CALIB_STAT - REG_ACC_DATA + 1)
#: Quaternion only — the cheapest orientation read (8 bytes, ~1.2 ms of bus).
QUAT_BLOCK = (REG_QUA_DATA, 8)

# SYS_TRIGGER bits.
SYS_TRIGGER_SELF_TEST = 0x01
SYS_TRIGGER_RST_SYS = 0x20
SYS_TRIGGER_RST_INT = 0x40
SYS_TRIGGER_CLK_SEL = 0x80

# INT_EN / INT_MSK / INT_STA bits. The three DRDY bits exist only on sensor
# firmware 03.14+; the boards in this line carry 03.11, where they never
# drive the pin.
INT_ACC_BSX_DRDY = 0x01
INT_MAG_DRDY = 0x02
INT_GYR_AM = 0x04
INT_GYR_HIGH_RATE = 0x08
INT_GYR_DRDY = 0x10
INT_ACC_HIGH_G = 0x20
INT_ACC_AM = 0x40
INT_ACC_NM = 0x80

# ST_RESULT bits (1 = passed).
ST_ACC = 0x01
ST_MAG = 0x02
ST_GYR = 0x04
ST_MCU = 0x08

EXPECTED_SELF_TEST = ST_ACC | ST_MAG | ST_GYR | ST_MCU


class OprMode(IntEnum):
    """OPR_MODE (0x3D) bits 3:0."""

    CONFIG = 0x00
    ACCONLY = 0x01
    MAGONLY = 0x02
    GYROONLY = 0x03
    ACCMAG = 0x04
    ACCGYRO = 0x05
    MAGGYRO = 0x06
    AMG = 0x07
    IMU = 0x08
    COMPASS = 0x09
    M4G = 0x0A
    NDOF_FMC_OFF = 0x0B
    NDOF = 0x0C

    @property
    def is_fusion(self) -> bool:
        return self >= OprMode.IMU


class PwrMode(IntEnum):
    """PWR_MODE (0x3E) bits 1:0."""

    NORMAL = 0x00
    LOW_POWER = 0x01
    SUSPEND = 0x02


class TempSource(IntEnum):
    """TEMP_SOURCE (0x40) bits 1:0."""

    ACCEL = 0x00
    GYRO = 0x01


#: Datasheet Table 3-6, plus margin: CONFIG → any mode 7 ms, any → CONFIG 19 ms.
MODE_SWITCH_FROM_CONFIG_S = 0.010
MODE_SWITCH_TO_CONFIG_S = 0.025
#: BIST runs ~400 ms (datasheet §3.9.2).
SELF_TEST_S = 0.45
#: BNO_RESET answers at the chip-ID handshake, but the sensor is still booting:
#: SYS_STATUS walks "system initialization" → "executing self-test" → idle for
#: another ~15 ms and then rewrites OPR_MODE to CONFIG, so a mode written in
#: that window is lost (measured, contract 13 §5). Poll until it leaves these.
SYS_STATUS_BOOTING = (2, 3, 4)
BOOT_SETTLE_TIMEOUT_S = 1.0
#: SYS_STATUS 4 polls in a row taken as the leftover of a self-test run in
#: CONFIG mode, not POST: POST shows 4 for ~35 ms (a handful of polls), the
#: leftover stays until the mode leaves CONFIG (measured on SW 03.11 —
#: rewriting CONFIG does not clear it). Counted, not timed, so a replay makes
#: the same decision.
STUCK_SELF_TEST_POLLS = 20
#: After CONFIG → a fusion mode the fusion outputs read zero for ~70 ms.
FUSION_START_TIMEOUT_S = 1.0

SYS_STATUS_NAMES = {
    0: "idle",
    1: "system error",
    2: "initializing peripherals",
    3: "system initialization",
    4: "executing self-test",
    5: "fusion algorithm running",
    6: "running without fusion",
}

SYS_ERR_NAMES = {
    0: "no error",
    1: "peripheral initialization error",
    2: "system initialization error",
    3: "self-test failed",
    4: "register map value out of range",
    5: "register map address out of range",
    6: "register map write error",
    7: "low power mode not available for this operation mode",
    8: "accelerometer power mode not available",
    9: "fusion algorithm configuration error",
    10: "sensor configuration error",
}


# ── units (UNIT_SEL 0x3B) ─────────────────────────────────────────────────────

# UNIT_SEL bits as the sensor actually implements them: datasheet Table 3-11
# and Bosch's driver. The §4.3.60 bit table is off by one — measured on
# SW rev 03.11 (contract 13 §4.2): bit 0 rescales ACC_DATA by 1000/9.80665,
# bit 2 turns Euler into radians (×900), bit 4 halves °F; bits 3/5 do nothing.
UNIT_ACC_MG = 0x01
UNIT_GYR_RPS = 0x02
UNIT_EUL_RAD = 0x04
UNIT_TEMP_F = 0x10
UNIT_ORI_ANDROID = 0x80


@dataclass(frozen=True)
class Units:
    """Output units. The default is SI-ish: m/s², dps, degrees, °C, Windows
    orientation (UNIT_SEL = 0x00). The sensor's own power-on value is 0x80
    (Android orientation), so a fresh sensor must be told."""

    accel_mg: bool = False  # ACC_DATA in mg, else m/s² (linear accel / gravity stay m/s²)
    gyro_rps: bool = False  # angular rate in rad/s, else deg/s
    euler_rad: bool = False  # Euler angles in radians, else degrees
    temp_f: bool = False  # temperature in °F, else °C
    android: bool = False  # Android pitch convention, else Windows

    def pack(self) -> int:
        return (
            (UNIT_ACC_MG if self.accel_mg else 0)
            | (UNIT_GYR_RPS if self.gyro_rps else 0)
            | (UNIT_EUL_RAD if self.euler_rad else 0)
            | (UNIT_TEMP_F if self.temp_f else 0)
            | (UNIT_ORI_ANDROID if self.android else 0)
        )

    @classmethod
    def unpack(cls, value: int) -> "Units":
        return cls(
            accel_mg=bool(value & UNIT_ACC_MG),
            gyro_rps=bool(value & UNIT_GYR_RPS),
            euler_rad=bool(value & UNIT_EUL_RAD),
            temp_f=bool(value & UNIT_TEMP_F),
            android=bool(value & UNIT_ORI_ANDROID),
        )

    # LSB per unit (datasheet §3.6.4–3.6.5). Temperature in °F is 1 LSB = 2 °F.
    @property
    def accel_lsb(self) -> float:
        return 1.0 if self.accel_mg else 100.0

    @property
    def gyro_lsb(self) -> float:
        return 900.0 if self.gyro_rps else 16.0

    @property
    def euler_lsb(self) -> float:
        return 900.0 if self.euler_rad else 16.0

    @property
    def temp_lsb(self) -> float:
        return 0.5 if self.temp_f else 1.0


MAG_LSB = 16.0  # µT, not selectable
QUAT_LSB = 16384.0  # 2^14, unit-less
#: Linear acceleration and gravity ignore the ACC_Unit bit: always m/s²,
#: 100 LSB — measured on SW rev 03.11 (datasheet Tables 3-33/3-35 claim mg
#: too; Table 3-11 lists only m/s², and the silicon agrees with 3-11).
FUSION_ACCEL_LSB = 100.0


# ── calibration ──────────────────────────────────────────────────────────────


@dataclass(frozen=True)
class CalibStatus:
    """CALIB_STAT (0x35): 0 = not calibrated … 3 = fully calibrated."""

    system: int
    gyro: int
    accel: int
    mag: int

    @classmethod
    def unpack(cls, value: int) -> "CalibStatus":
        return cls(value >> 6 & 3, value >> 4 & 3, value >> 2 & 3, value & 3)

    def pack(self) -> int:
        return (self.system & 3) << 6 | (self.gyro & 3) << 4 | (self.accel & 3) << 2 | (
            self.mag & 3
        )

    @property
    def fully_calibrated(self) -> bool:
        return (self.system, self.gyro, self.accel, self.mag) == (3, 3, 3, 3)


@dataclass(frozen=True)
class CalibrationProfile:
    """Sensor offsets and radii, registers 0x55..0x6A (22 bytes, all i16 LE).

    Read it after a full calibration (sensor in CONFIG mode), store it, and
    write it back after every power-on reset to skip the calibration dance
    (datasheet §3.11.5). Offsets are in the sensor's LSB — the value does not
    depend on UNIT_SEL once written back."""

    accel_offset: tuple[int, int, int]
    mag_offset: tuple[int, int, int]
    gyro_offset: tuple[int, int, int]
    accel_radius: int
    mag_radius: int

    def pack(self) -> bytes:
        return struct.pack(
            "<11h",
            *self.accel_offset,
            *self.mag_offset,
            *self.gyro_offset,
            self.accel_radius,
            self.mag_radius,
        )

    @classmethod
    def unpack(cls, data: bytes) -> "CalibrationProfile":
        if len(data) != CALIB_PROFILE_LEN:
            raise ValueError(f"calibration profile is {CALIB_PROFILE_LEN} bytes, got {len(data)}")
        v = struct.unpack("<11h", data)
        return cls(v[0:3], v[3:6], v[6:9], v[9], v[10])  # type: ignore[arg-type]

    def to_dict(self) -> dict:
        return {
            "accel_offset": list(self.accel_offset),
            "mag_offset": list(self.mag_offset),
            "gyro_offset": list(self.gyro_offset),
            "accel_radius": self.accel_radius,
            "mag_radius": self.mag_radius,
        }

    @classmethod
    def from_dict(cls, d: dict) -> "CalibrationProfile":
        return cls(
            tuple(d["accel_offset"]),  # type: ignore[arg-type]
            tuple(d["mag_offset"]),  # type: ignore[arg-type]
            tuple(d["gyro_offset"]),  # type: ignore[arg-type]
            int(d["accel_radius"]),
            int(d["mag_radius"]),
        )


def pack_sic_matrix(m: tuple[int, ...]) -> bytes:
    """Soft-iron matrix, 9 × i16 row-major, 1.0 = 16384 (datasheet §3.11.4)."""
    if len(m) != 9:
        raise ValueError("SIC matrix has 9 elements")
    return struct.pack("<9h", *m)


def unpack_sic_matrix(data: bytes) -> tuple[int, ...]:
    return struct.unpack("<9h", data)


SIC_IDENTITY = (16384, 0, 0, 0, 16384, 0, 0, 0, 16384)


# ── axis remap (AXIS_MAP_CONFIG 0x41 / AXIS_MAP_SIGN 0x42) ──────────────────

AXIS_X, AXIS_Y, AXIS_Z = 0, 1, 2


@dataclass(frozen=True)
class AxisRemap:
    """Which physical axis feeds each output axis, and its sign.

    `x = AXIS_Y` means "output X is the chip's Y axis". The sensor rejects a
    mapping that uses one axis twice and silently keeps the old one, so
    `pack()` refuses it instead."""

    x: int = AXIS_X
    y: int = AXIS_Y
    z: int = AXIS_Z
    x_negative: bool = False
    y_negative: bool = False
    z_negative: bool = False

    def pack(self) -> tuple[int, int]:
        """→ (AXIS_MAP_CONFIG, AXIS_MAP_SIGN)."""
        if sorted((self.x, self.y, self.z)) != [0, 1, 2]:
            raise ValueError(f"axis remap must be a permutation of X/Y/Z, got {self}")
        config = self.z << 4 | self.y << 2 | self.x
        sign = (4 if self.x_negative else 0) | (2 if self.y_negative else 0) | (
            1 if self.z_negative else 0
        )
        return config, sign

    @classmethod
    def unpack(cls, config: int, sign: int) -> "AxisRemap":
        return cls(
            x=config & 3,
            y=config >> 2 & 3,
            z=config >> 4 & 3,
            x_negative=bool(sign & 4),
            y_negative=bool(sign & 2),
            z_negative=bool(sign & 1),
        )

    @classmethod
    def placement(cls, name: str) -> "AxisRemap":
        """Datasheet §3.4 mounting presets P0..P7 (P1 is the default)."""
        try:
            config, sign = PLACEMENTS[name.upper()]
        except KeyError:
            raise ValueError(f"unknown placement {name!r}; expected P0..P7") from None
        return cls.unpack(config, sign)


#: Datasheet §3.4: placement → (AXIS_MAP_CONFIG, AXIS_MAP_SIGN).
PLACEMENTS: dict[str, tuple[int, int]] = {
    "P0": (0x21, 0x04),
    "P1": (0x24, 0x00),
    "P2": (0x24, 0x06),
    "P3": (0x21, 0x02),
    "P4": (0x24, 0x03),
    "P5": (0x21, 0x01),
    "P6": (0x21, 0x07),
    "P7": (0x24, 0x05),
}


# ── page-1 sensor configuration (non-fusion modes only) ─────────────────────

ACC_RANGE_G = (2, 4, 8, 16)
ACC_BANDWIDTH_HZ = (7.81, 15.63, 31.25, 62.5, 125.0, 250.0, 500.0, 1000.0)
ACC_POWER_NAMES = ("normal", "suspend", "low power 1", "standby", "low power 2", "deep suspend")
GYR_RANGE_DPS = (2000, 1000, 500, 250, 125)
GYR_BANDWIDTH_HZ = (523, 230, 116, 47, 23, 12, 64, 32)
GYR_POWER_NAMES = ("normal", "fast power up", "deep suspend", "suspend", "advanced powersave")
MAG_RATE_HZ = (2, 6, 8, 10, 15, 20, 25, 30)
MAG_OPR_NAMES = ("low power", "regular", "enhanced regular", "high accuracy")
MAG_POWER_NAMES = ("normal", "sleep", "suspend", "force")


@dataclass(frozen=True)
class AccelConfig:
    """ACC_CONFIG (page 1, 0x08) as register codes: `range` indexes
    ACC_RANGE_G, `bandwidth` ACC_BANDWIDTH_HZ, `power` ACC_POWER_NAMES.
    Power-on value 0x0D = ±4 g, 62.5 Hz, normal."""

    range: int = 1
    bandwidth: int = 3
    power: int = 0

    def pack(self) -> int:
        return (self.power & 7) << 5 | (self.bandwidth & 7) << 2 | (self.range & 3)

    @classmethod
    def unpack(cls, value: int) -> "AccelConfig":
        return cls(value & 3, value >> 2 & 7, value >> 5 & 7)


@dataclass(frozen=True)
class GyroConfig:
    """GYR_CONFIG_0/1 (page 1, 0x0A/0x0B): `range` indexes GYR_RANGE_DPS,
    `bandwidth` GYR_BANDWIDTH_HZ, `power` GYR_POWER_NAMES. Power-on value
    0x38/0x00 = 2000 dps, 32 Hz, normal."""

    range: int = 0
    bandwidth: int = 7
    power: int = 0

    def pack(self) -> bytes:
        return bytes([(self.bandwidth & 7) << 3 | (self.range & 7), self.power & 7])

    @classmethod
    def unpack(cls, data: bytes) -> "GyroConfig":
        return cls(data[0] & 7, data[0] >> 3 & 7, data[1] & 7)


@dataclass(frozen=True)
class MagConfig:
    """MAG_CONFIG (page 1, 0x09): `rate` indexes MAG_RATE_HZ, `mode`
    MAG_OPR_NAMES, `power` MAG_POWER_NAMES. Power-on value 0x0B = 10 Hz,
    regular, normal."""

    rate: int = 3
    mode: int = 1
    power: int = 0

    def pack(self) -> int:
        return (self.power & 3) << 5 | (self.mode & 3) << 3 | (self.rate & 7)

    @classmethod
    def unpack(cls, value: int) -> "MagConfig":
        return cls(value & 7, value >> 3 & 3, value >> 5 & 3)


# ── system status ────────────────────────────────────────────────────────────


@dataclass(frozen=True)
class SystemStatus:
    """ST_RESULT (0x36) + SYS_CLK_STATUS/SYS_STATUS/SYS_ERR (0x38..0x3A).
    INT_STA (0x37) sits in between and clears on read, so it is not here."""

    self_test: int  # ST_RESULT bits, EXPECTED_SELF_TEST when all passed
    clk_status: int  # 1 = clock source being configured
    status: int  # SYS_STATUS_NAMES
    error: int  # SYS_ERR_NAMES, meaningful when status == 1

    @property
    def status_text(self) -> str:
        return SYS_STATUS_NAMES.get(self.status, f"unknown ({self.status})")

    @property
    def error_text(self) -> str:
        return SYS_ERR_NAMES.get(self.error, f"unknown ({self.error})")

    @property
    def self_test_passed(self) -> bool:
        return self.self_test & EXPECTED_SELF_TEST == EXPECTED_SELF_TEST


# ── output block decode ──────────────────────────────────────────────────────

#: Vector channels: name → (first register, word count). Order is the
#: register order; Euler is (heading, roll, pitch), quaternion (w, x, y, z).
CHANNELS: dict[str, tuple[int, int]] = {
    "accel": (REG_ACC_DATA, 3),
    "mag": (REG_MAG_DATA, 3),
    "gyro": (REG_GYR_DATA, 3),
    "euler": (REG_EUL_DATA, 3),
    "quaternion": (REG_QUA_DATA, 4),
    "linear_accel": (REG_LIA_DATA, 3),
    "gravity": (REG_GRV_DATA, 3),
}


@dataclass(frozen=True)
class RawBlock:
    """Raw register values found in one block read. A channel is None when the
    window `addr..addr+len` does not cover it completely."""

    accel: tuple[int, int, int] | None = None
    mag: tuple[int, int, int] | None = None
    gyro: tuple[int, int, int] | None = None
    euler: tuple[int, int, int] | None = None
    quaternion: tuple[int, int, int, int] | None = None
    linear_accel: tuple[int, int, int] | None = None
    gravity: tuple[int, int, int] | None = None
    temperature: int | None = None  # i8
    calib_stat: int | None = None  # CALIB_STAT byte


def decode_block(addr: int, data: bytes) -> RawBlock:
    """Unpack whatever channels the register window starting at `addr` holds."""
    end = addr + len(data)
    out: dict = {}
    for name, (reg, n) in CHANNELS.items():
        if addr <= reg and reg + 2 * n <= end:
            out[name] = struct.unpack_from(f"<{n}h", data, reg - addr)
    if addr <= REG_TEMP < end:
        out["temperature"] = struct.unpack_from("<b", data, REG_TEMP - addr)[0]
    if addr <= REG_CALIB_STAT < end:
        out["calib_stat"] = data[REG_CALIB_STAT - addr]
    return RawBlock(**out)
