//! BNO055 register map and the pure codecs every SDK shares
//! (contracts/13_SENSOR_BNO055.md §4, Bosch BST-BNO055-DS000 rev 1.8).
//!
//! Nothing here touches the wire: these functions turn register bytes into
//! values and back, which is what `vectors/bno055.json` pins. All values stay
//! raw integers; scaling is `value = raw / LSB` with the LSBs of
//! [`Units`] and the fixed constants below (§4.2).

use crate::protocol::common::CodecError;

// ---- page 0 ------------------------------------------------------------------

pub const REG_CHIP_ID: u8 = 0x00;
/// Page select — owned by the host (the MCU never touches it); always return
/// to page 0, and never switch pages under a running stream.
pub const REG_PAGE_ID: u8 = 0x07;
/// ACC_DATA: x, y, z i16 LE.
pub const REG_ACC_DATA: u8 = 0x08;
pub const REG_MAG_DATA: u8 = 0x0E;
pub const REG_GYR_DATA: u8 = 0x14;
/// EUL_DATA: heading, roll, pitch.
pub const REG_EUL_DATA: u8 = 0x1A;
/// QUA_DATA: w, x, y, z.
pub const REG_QUA_DATA: u8 = 0x20;
/// Linear acceleration (gravity removed).
pub const REG_LIA_DATA: u8 = 0x28;
/// Gravity vector.
pub const REG_GRV_DATA: u8 = 0x2E;
/// TEMP: i8.
pub const REG_TEMP: u8 = 0x34;
pub const REG_CALIB_STAT: u8 = 0x35;
pub const REG_ST_RESULT: u8 = 0x36;
/// Clears on read — never part of a routine block read.
pub const REG_INT_STA: u8 = 0x37;
pub const REG_SYS_CLK_STATUS: u8 = 0x38;
pub const REG_SYS_STATUS: u8 = 0x39;
pub const REG_SYS_ERR: u8 = 0x3A;
pub const REG_UNIT_SEL: u8 = 0x3B;
/// OPR_MODE bits 3:0 (reads back 0x10 after reset — mask).
pub const REG_OPR_MODE: u8 = 0x3D;
pub const REG_PWR_MODE: u8 = 0x3E;
/// `CLK_SEL 7, RST_INT 6, RST_SYS 5, SELF_TEST 0` — preserve bit 7.
pub const REG_SYS_TRIGGER: u8 = 0x3F;
pub const REG_TEMP_SOURCE: u8 = 0x40;
pub const REG_AXIS_MAP_CONFIG: u8 = 0x41;
pub const REG_AXIS_MAP_SIGN: u8 = 0x42;
/// Soft-iron matrix, 9 × i16 row-major, 1.0 = 16384.
pub const REG_SIC_MATRIX: u8 = 0x43;
/// Calibration profile, [`CALIB_PROFILE_LEN`] bytes (CONFIG mode only).
pub const REG_CALIB_PROFILE: u8 = 0x55;
pub const CALIB_PROFILE_LEN: usize = 22;

// ---- page 1 (configs effective in non-fusion modes only) ---------------------

pub const REG1_ACC_CONFIG: u8 = 0x08;
pub const REG1_MAG_CONFIG: u8 = 0x09;
pub const REG1_GYR_CONFIG_0: u8 = 0x0A;
pub const REG1_GYR_CONFIG_1: u8 = 0x0B;
pub const REG1_ACC_SLEEP_CONFIG: u8 = 0x0C;
pub const REG1_GYR_SLEEP_CONFIG: u8 = 0x0D;
pub const REG1_INT_MSK: u8 = 0x0F;
pub const REG1_INT_EN: u8 = 0x10;
/// First of the motion-interrupt settings 0x11..=0x1F (written raw).
pub const REG1_ACC_AM_THRES: u8 = 0x11;
/// Last of the motion-interrupt settings.
pub const REG1_GYR_AM_SET: u8 = 0x1F;
pub const REG1_UNIQUE_ID: u8 = 0x50;
pub const UNIQUE_ID_LEN: usize = 16;

/// The one block that carries every output channel: `(addr, len)` = 0x08
/// (ACC_DATA_X_LSB) through 0x35 (CALIB_STAT), 46 bytes.
pub const FULL_BLOCK: (u8, u8) = (REG_ACC_DATA, REG_CALIB_STAT - REG_ACC_DATA + 1);
/// Quaternion only — the cheapest orientation read (8 bytes).
pub const QUAT_BLOCK: (u8, u8) = (REG_QUA_DATA, 8);

// SYS_TRIGGER bits.
pub const SYS_TRIGGER_SELF_TEST: u8 = 0x01;
pub const SYS_TRIGGER_RST_SYS: u8 = 0x20;
pub const SYS_TRIGGER_RST_INT: u8 = 0x40;
pub const SYS_TRIGGER_CLK_SEL: u8 = 0x80;

// INT_EN / INT_MSK / INT_STA bits. The DRDY bits drive the pin only on sensor
// firmware 03.14+; the boards in this line carry 03.11.
pub const INT_ACC_BSX_DRDY: u8 = 0x01;
pub const INT_MAG_DRDY: u8 = 0x02;
pub const INT_GYR_AM: u8 = 0x04;
pub const INT_GYR_HIGH_RATE: u8 = 0x08;
pub const INT_GYR_DRDY: u8 = 0x10;
pub const INT_ACC_HIGH_G: u8 = 0x20;
pub const INT_ACC_AM: u8 = 0x40;
pub const INT_ACC_NM: u8 = 0x80;

// ST_RESULT bits (1 = passed); 0x0F is the healthy value.
pub const ST_ACC: u8 = 0x01;
pub const ST_MAG: u8 = 0x02;
pub const ST_GYR: u8 = 0x04;
pub const ST_MCU: u8 = 0x08;
pub const EXPECTED_SELF_TEST: u8 = ST_ACC | ST_MAG | ST_GYR | ST_MCU;

/// SYS_STATUS values of a sensor still booting after BNO_RESET: poll until it
/// leaves these before configuring (contract 13 §5, ERRATA E13).
pub const SYS_STATUS_BOOTING: [u8; 3] = [2, 3, 4];

/// OPR_MODE (0x3D) bits 3:0.
#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord)]
#[repr(u8)]
pub enum OprMode {
    Config = 0x00,
    AccOnly = 0x01,
    MagOnly = 0x02,
    GyroOnly = 0x03,
    AccMag = 0x04,
    AccGyro = 0x05,
    MagGyro = 0x06,
    Amg = 0x07,
    Imu = 0x08,
    Compass = 0x09,
    M4g = 0x0A,
    NdofFmcOff = 0x0B,
    Ndof = 0x0C,
}

impl OprMode {
    /// From an OPR_MODE register value (bits 7:4 masked off); `None` for the
    /// unused codes 13..15.
    pub fn from_reg(value: u8) -> Option<OprMode> {
        use OprMode::*;
        const ALL: [OprMode; 13] = [
            Config, AccOnly, MagOnly, GyroOnly, AccMag, AccGyro, MagGyro, Amg, Imu, Compass, M4g,
            NdofFmcOff, Ndof,
        ];
        ALL.get((value & 0x0F) as usize).copied()
    }

    /// Fusion modes (IMU and up) run Bosch's on-chip sensor fusion.
    pub fn is_fusion(&self) -> bool {
        *self >= OprMode::Imu
    }
}

/// PWR_MODE (0x3E) bits 1:0.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
#[repr(u8)]
pub enum PwrMode {
    Normal = 0x00,
    LowPower = 0x01,
    Suspend = 0x02,
}

/// TEMP_SOURCE (0x40) bits 1:0.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
#[repr(u8)]
pub enum TempSource {
    Accel = 0x00,
    Gyro = 0x01,
}

// ---- units (UNIT_SEL 0x3B) ---------------------------------------------------

// UNIT_SEL bits as the silicon implements them (SW rev 03.11; datasheet
// Table 3-11 and Bosch's driver). The datasheet §4.3.60 bit table is off by
// one; bits 3/5 do nothing.
pub const UNIT_ACC_MG: u8 = 0x01;
pub const UNIT_GYR_RPS: u8 = 0x02;
pub const UNIT_EUL_RAD: u8 = 0x04;
pub const UNIT_TEMP_F: u8 = 0x10;
pub const UNIT_ORI_ANDROID: u8 = 0x80;

/// Output units. `Units::default()` is the SDK default UNIT_SEL = 0x00 (m/s²,
/// dps, degrees, °C, Windows orientation). The sensor's own power-on value is
/// 0x80 (Android), so hosts write UNIT_SEL explicitly. A stream is scaled by
/// the units latched when it was armed.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Default)]
pub struct Units {
    /// ACC_DATA in mg, else m/s² (linear accel / gravity stay m/s²).
    pub accel_mg: bool,
    /// Angular rate in rad/s, else deg/s.
    pub gyro_rps: bool,
    /// Euler angles in radians, else degrees.
    pub euler_rad: bool,
    /// Temperature in °F, else °C.
    pub temp_f: bool,
    /// Android pitch convention, else Windows.
    pub android: bool,
}

impl Units {
    /// → UNIT_SEL. Undefined bits are never set.
    pub fn pack(&self) -> u8 {
        (if self.accel_mg { UNIT_ACC_MG } else { 0 })
            | (if self.gyro_rps { UNIT_GYR_RPS } else { 0 })
            | (if self.euler_rad { UNIT_EUL_RAD } else { 0 })
            | (if self.temp_f { UNIT_TEMP_F } else { 0 })
            | (if self.android { UNIT_ORI_ANDROID } else { 0 })
    }

    /// From UNIT_SEL; the bits that do nothing are ignored.
    pub fn unpack(value: u8) -> Units {
        Units {
            accel_mg: value & UNIT_ACC_MG != 0,
            gyro_rps: value & UNIT_GYR_RPS != 0,
            euler_rad: value & UNIT_EUL_RAD != 0,
            temp_f: value & UNIT_TEMP_F != 0,
            android: value & UNIT_ORI_ANDROID != 0,
        }
    }

    /// ACC_DATA LSB per unit: m/s² 100, mg 1.
    pub fn accel_lsb(&self) -> f64 {
        if self.accel_mg {
            1.0
        } else {
            100.0
        }
    }

    /// Angular-rate LSB per unit: dps 16, rps 900.
    pub fn gyro_lsb(&self) -> f64 {
        if self.gyro_rps {
            900.0
        } else {
            16.0
        }
    }

    /// Euler LSB per unit: degrees 16, radians 900.
    pub fn euler_lsb(&self) -> f64 {
        if self.euler_rad {
            900.0
        } else {
            16.0
        }
    }

    /// Temperature LSB per unit: °C 1, °F 0.5 (1 LSB = 2 °F, measured).
    pub fn temp_lsb(&self) -> f64 {
        if self.temp_f {
            0.5
        } else {
            1.0
        }
    }
}

/// MAG_DATA: 16 LSB per µT, not selectable.
pub const MAG_LSB: f64 = 16.0;
/// Quaternion: 2¹⁴ LSB, unit-less.
pub const QUAT_LSB: f64 = 16384.0;
/// Linear acceleration and gravity ignore the ACC_Unit bit: always m/s² at
/// 100 LSB (measured on SW 03.11; datasheet Tables 3-33/3-35 claim mg).
pub const FUSION_ACCEL_LSB: f64 = 100.0;

// ---- calibration -------------------------------------------------------------

/// CALIB_STAT (0x35) `sys<7:6> gyr<5:4> acc<3:2> mag<1:0>`: 0 = not
/// calibrated … 3 = fully calibrated.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct CalibStatus {
    pub system: u8,
    pub gyro: u8,
    pub accel: u8,
    pub mag: u8,
}

impl CalibStatus {
    pub fn unpack(value: u8) -> CalibStatus {
        CalibStatus {
            system: value >> 6 & 3,
            gyro: value >> 4 & 3,
            accel: value >> 2 & 3,
            mag: value & 3,
        }
    }

    pub fn pack(&self) -> u8 {
        (self.system & 3) << 6 | (self.gyro & 3) << 4 | (self.accel & 3) << 2 | (self.mag & 3)
    }

    /// 3/3/3/3.
    pub fn fully_calibrated(&self) -> bool {
        (self.system, self.gyro, self.accel, self.mag) == (3, 3, 3, 3)
    }
}

/// Sensor offsets and radii, registers 0x55..0x6A (22 bytes, 11 × i16 LE), in
/// the sensor's LSB — independent of UNIT_SEL once written back. Readable and
/// writable only in CONFIG; write all 22 bytes in one transfer (the sensor
/// latches each group on its MSB). A written profile is a starting point: the
/// sensor keeps calibrating in fusion modes.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct CalibrationProfile {
    pub accel_offset: [i16; 3],
    pub mag_offset: [i16; 3],
    pub gyro_offset: [i16; 3],
    pub accel_radius: i16,
    pub mag_radius: i16,
}

impl CalibrationProfile {
    pub fn pack(&self) -> [u8; CALIB_PROFILE_LEN] {
        let words = self
            .accel_offset
            .iter()
            .chain(&self.mag_offset)
            .chain(&self.gyro_offset)
            .chain([&self.accel_radius, &self.mag_radius]);
        let mut out = [0u8; CALIB_PROFILE_LEN];
        for (i, w) in words.enumerate() {
            out[2 * i..2 * i + 2].copy_from_slice(&w.to_le_bytes());
        }
        out
    }

    /// Exactly [`CALIB_PROFILE_LEN`] bytes, else an error.
    pub fn unpack(data: &[u8]) -> Result<CalibrationProfile, CodecError> {
        if data.len() != CALIB_PROFILE_LEN {
            return Err(CodecError("calibration profile is 22 bytes"));
        }
        let w = |i: usize| i16_le(data, 2 * i);
        Ok(CalibrationProfile {
            accel_offset: [w(0), w(1), w(2)],
            mag_offset: [w(3), w(4), w(5)],
            gyro_offset: [w(6), w(7), w(8)],
            accel_radius: w(9),
            mag_radius: w(10),
        })
    }
}

/// Soft-iron matrix (SIC_MATRIX 0x43), 9 × i16 row-major, 1.0 = 16384.
pub fn pack_sic_matrix(m: &[i16; 9]) -> [u8; 18] {
    let mut out = [0u8; 18];
    for (i, v) in m.iter().enumerate() {
        out[2 * i..2 * i + 2].copy_from_slice(&v.to_le_bytes());
    }
    out
}

/// Inverse of [`pack_sic_matrix`]; needs at least 18 bytes.
pub fn unpack_sic_matrix(data: &[u8]) -> Result<[i16; 9], CodecError> {
    if data.len() < 18 {
        return Err(CodecError("SIC matrix is 18 bytes"));
    }
    Ok(std::array::from_fn(|i| i16_le(data, 2 * i)))
}

/// The identity soft-iron matrix.
pub const SIC_IDENTITY: [i16; 9] = [16384, 0, 0, 0, 16384, 0, 0, 0, 16384];

// ---- axis remap (AXIS_MAP_CONFIG 0x41 / AXIS_MAP_SIGN 0x42) ------------------

pub const AXIS_X: u8 = 0;
pub const AXIS_Y: u8 = 1;
pub const AXIS_Z: u8 = 2;

/// Which chip axis feeds each output axis, and its sign. `x = AXIS_Y` means
/// "output X is the chip's Y axis". The sensor silently keeps its old value
/// when given a mapping that uses one axis twice, so [`AxisRemap::pack`]
/// refuses it. `AxisRemap::default()` is placement P1 (identity).
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct AxisRemap {
    pub x: u8,
    pub y: u8,
    pub z: u8,
    pub x_negative: bool,
    pub y_negative: bool,
    pub z_negative: bool,
}

impl Default for AxisRemap {
    fn default() -> AxisRemap {
        AxisRemap {
            x: AXIS_X,
            y: AXIS_Y,
            z: AXIS_Z,
            x_negative: false,
            y_negative: false,
            z_negative: false,
        }
    }
}

impl AxisRemap {
    /// → `(AXIS_MAP_CONFIG, AXIS_MAP_SIGN)`: config `z<5:4> y<3:2> x<1:0>`,
    /// sign `x 2, y 1, z 0` (1 = negative). Refuses a non-permutation of
    /// X/Y/Z.
    pub fn pack(&self) -> Result<(u8, u8), CodecError> {
        let mut axes = [self.x, self.y, self.z];
        axes.sort_unstable();
        if axes != [AXIS_X, AXIS_Y, AXIS_Z] {
            return Err(CodecError("axis remap must be a permutation of X/Y/Z"));
        }
        let config = self.z << 4 | self.y << 2 | self.x;
        let sign = (if self.x_negative { 4 } else { 0 })
            | (if self.y_negative { 2 } else { 0 })
            | (if self.z_negative { 1 } else { 0 });
        Ok((config, sign))
    }

    pub fn unpack(config: u8, sign: u8) -> AxisRemap {
        AxisRemap {
            x: config & 3,
            y: config >> 2 & 3,
            z: config >> 4 & 3,
            x_negative: sign & 4 != 0,
            y_negative: sign & 2 != 0,
            z_negative: sign & 1 != 0,
        }
    }

    /// Datasheet §3.4 mounting presets `P0`..`P7` (case-insensitive; P1 is the
    /// default). `None` for an unknown name.
    pub fn placement(name: &str) -> Option<AxisRemap> {
        PLACEMENTS
            .iter()
            .find(|(n, _, _)| n.eq_ignore_ascii_case(name))
            .map(|&(_, config, sign)| AxisRemap::unpack(config, sign))
    }
}

/// Datasheet §3.4: `(placement, AXIS_MAP_CONFIG, AXIS_MAP_SIGN)`.
pub const PLACEMENTS: [(&str, u8, u8); 8] = [
    ("P0", 0x21, 0x04),
    ("P1", 0x24, 0x00),
    ("P2", 0x24, 0x06),
    ("P3", 0x21, 0x02),
    ("P4", 0x24, 0x03),
    ("P5", 0x21, 0x01),
    ("P6", 0x21, 0x07),
    ("P7", 0x24, 0x05),
];

// ---- page-1 sensor configuration (non-fusion modes only) ---------------------

/// Accelerometer range per `AccelConfig::range` code, g.
pub const ACC_RANGE_G: [u8; 4] = [2, 4, 8, 16];
/// Accelerometer bandwidth per `AccelConfig::bandwidth` code, Hz.
pub const ACC_BANDWIDTH_HZ: [f64; 8] = [7.81, 15.63, 31.25, 62.5, 125.0, 250.0, 500.0, 1000.0];
/// Accelerometer power mode per `AccelConfig::power` code.
pub const ACC_POWER_NAMES: [&str; 6] =
    ["normal", "suspend", "low power 1", "standby", "low power 2", "deep suspend"];
/// Gyroscope range per `GyroConfig::range` code, dps.
pub const GYR_RANGE_DPS: [u16; 5] = [2000, 1000, 500, 250, 125];
/// Gyroscope bandwidth per `GyroConfig::bandwidth` code, Hz.
pub const GYR_BANDWIDTH_HZ: [u16; 8] = [523, 230, 116, 47, 23, 12, 64, 32];
/// Gyroscope power mode per `GyroConfig::power` code.
pub const GYR_POWER_NAMES: [&str; 5] =
    ["normal", "fast power up", "deep suspend", "suspend", "advanced powersave"];
/// Magnetometer output rate per `MagConfig::rate` code, Hz.
pub const MAG_RATE_HZ: [u8; 8] = [2, 6, 8, 10, 15, 20, 25, 30];
/// Magnetometer operation mode per `MagConfig::mode` code.
pub const MAG_OPR_NAMES: [&str; 4] = ["low power", "regular", "enhanced regular", "high accuracy"];
/// Magnetometer power mode per `MagConfig::power` code.
pub const MAG_POWER_NAMES: [&str; 4] = ["normal", "sleep", "suspend", "force"];

/// ACC_CONFIG (page 1, 0x08) as register codes: `range<1:0>`,
/// `bandwidth<4:2>`, `power<7:5>`. Default = power-on 0x0D (±4 g, 62.5 Hz,
/// normal).
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct AccelConfig {
    pub range: u8,
    pub bandwidth: u8,
    pub power: u8,
}

impl Default for AccelConfig {
    fn default() -> AccelConfig {
        AccelConfig { range: 1, bandwidth: 3, power: 0 }
    }
}

impl AccelConfig {
    pub fn pack(&self) -> u8 {
        (self.power & 7) << 5 | (self.bandwidth & 7) << 2 | (self.range & 3)
    }

    pub fn unpack(value: u8) -> AccelConfig {
        AccelConfig { range: value & 3, bandwidth: value >> 2 & 7, power: value >> 5 & 7 }
    }
}

/// GYR_CONFIG_0/1 (page 1, 0x0A/0x0B), two bytes: byte 0 `range<2:0>`
/// `bandwidth<5:3>`, byte 1 `power<2:0>`. Default = power-on 0x38/0x00
/// (2000 dps, 32 Hz, normal).
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct GyroConfig {
    pub range: u8,
    pub bandwidth: u8,
    pub power: u8,
}

impl Default for GyroConfig {
    fn default() -> GyroConfig {
        GyroConfig { range: 0, bandwidth: 7, power: 0 }
    }
}

impl GyroConfig {
    pub fn pack(&self) -> [u8; 2] {
        [(self.bandwidth & 7) << 3 | (self.range & 7), self.power & 7]
    }

    /// Needs at least 2 bytes (GYR_CONFIG_0, GYR_CONFIG_1).
    pub fn unpack(data: &[u8]) -> Result<GyroConfig, CodecError> {
        if data.len() < 2 {
            return Err(CodecError("gyro config is 2 bytes"));
        }
        Ok(GyroConfig { range: data[0] & 7, bandwidth: data[0] >> 3 & 7, power: data[1] & 7 })
    }
}

/// MAG_CONFIG (page 1, 0x09): `rate<2:0>`, `mode<4:3>`, `power<6:5>` (bit 7
/// unused — a repack clears it). Default = power-on 0x0B (10 Hz, regular,
/// normal).
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct MagConfig {
    pub rate: u8,
    pub mode: u8,
    pub power: u8,
}

impl Default for MagConfig {
    fn default() -> MagConfig {
        MagConfig { rate: 3, mode: 1, power: 0 }
    }
}

impl MagConfig {
    pub fn pack(&self) -> u8 {
        (self.power & 3) << 5 | (self.mode & 3) << 3 | (self.rate & 7)
    }

    pub fn unpack(value: u8) -> MagConfig {
        MagConfig { rate: value & 7, mode: value >> 3 & 3, power: value >> 5 & 3 }
    }
}

// ---- output block decode -----------------------------------------------------

/// Raw register values found in one register window. A channel is `None` when
/// the window `addr..addr+len` does not cover all of its bytes. Euler is
/// `[heading, roll, pitch]`, quaternion `[w, x, y, z]`.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Default)]
pub struct RawBlock {
    pub accel: Option<[i16; 3]>,
    pub mag: Option<[i16; 3]>,
    pub gyro: Option<[i16; 3]>,
    pub euler: Option<[i16; 3]>,
    pub quaternion: Option<[i16; 4]>,
    pub linear_accel: Option<[i16; 3]>,
    pub gravity: Option<[i16; 3]>,
    /// TEMP, i8.
    pub temperature: Option<i8>,
    /// CALIB_STAT byte ([`CalibStatus::unpack`]).
    pub calib_stat: Option<u8>,
}

fn i16_le(data: &[u8], at: usize) -> i16 {
    i16::from_le_bytes([data[at], data[at + 1]])
}

/// `N` × i16 LE starting at register `reg`, if the window covers all of them.
fn words<const N: usize>(addr: usize, data: &[u8], reg: u8) -> Option<[i16; N]> {
    let reg = reg as usize;
    if addr <= reg && reg + 2 * N <= addr + data.len() {
        let at = reg - addr;
        Some(std::array::from_fn(|i| i16_le(data, at + 2 * i)))
    } else {
        None
    }
}

/// One byte at register `reg`, if the window covers it.
fn byte(addr: usize, data: &[u8], reg: u8) -> Option<u8> {
    (reg as usize).checked_sub(addr).and_then(|at| data.get(at)).copied()
}

/// Unpack whatever channels the register window starting at `addr` holds
/// (offsets are `reg − addr`). `data` is the block as read — e.g.
/// [`crate::bno055::StreamData::data`] with its `addr`.
pub fn decode_block(addr: u8, data: &[u8]) -> RawBlock {
    let a = addr as usize;
    RawBlock {
        accel: words(a, data, REG_ACC_DATA),
        mag: words(a, data, REG_MAG_DATA),
        gyro: words(a, data, REG_GYR_DATA),
        euler: words(a, data, REG_EUL_DATA),
        quaternion: words(a, data, REG_QUA_DATA),
        linear_accel: words(a, data, REG_LIA_DATA),
        gravity: words(a, data, REG_GRV_DATA),
        temperature: byte(a, data, REG_TEMP).map(|b| b as i8),
        calib_stat: byte(a, data, REG_CALIB_STAT),
    }
}
