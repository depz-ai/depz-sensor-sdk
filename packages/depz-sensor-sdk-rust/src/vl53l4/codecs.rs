//! VL53L4CD register-bridge wire codecs (contracts/10_SENSOR_VL53L4.md §2–§3).
//!
//! Command payload packers (all wire fields little-endian) and report payload
//! unpackers for the thin I2C register bridge. Register *contents* are
//! big-endian sensor bytes passed through untouched — only the bridge's own
//! fields (`addr`, `len`, timestamps, counters) are little-endian.

use crate::protocol::common::CodecError;

/// VL53L4 host→device command opcodes.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
#[repr(u8)]
pub enum Vl53l4Cmd {
    ReadReg = 0x32,
    WriteReg = 0x33,
    Xshut = 0x34,
    StartStream = 0x35,
    StopStream = 0x36,
    GetInfo = 0x37,
    SetI2cSpeed = 0x38,
}

/// VL53L4 device→host report opcodes.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
#[repr(u8)]
pub enum Vl53l4Rpt {
    RegData = 0x91,
    Info = 0x92,
    Stream = 0x93,
}

/// Largest `len` (read) / data length (write) per transfer. The STM32 I2C
/// NBYTES field is 8-bit and a write spends two bytes on the register address;
/// the firmware applies the same 253 to both directions.
pub const XFER_MAX: usize = 253;

/// `VL53_XSHUT` action: drive XSHUT low (sensor powered down).
pub const XSHUT_OFF: u8 = 0;
/// `VL53_XSHUT` action: drive XSHUT high, no boot handshake.
pub const XSHUT_ON: u8 = 1;
/// `VL53_XSHUT` action: pulse low then poll the boot handshake (answered after
/// it completes — allow ≥ 1.5 s).
pub const XSHUT_RESET: u8 = 2;

/// `VL53_START_STREAM` flag: INT active high, mirroring bit 4 of
/// `GPIO_HV_MUX__CTRL` (0x0030). Clear (default): INT active low.
pub const SF_INT_ACT_HIGH: u8 = 0x02;

/// Nominal SCL steps the firmware carries a TIMINGR for (`VL53_SET_I2C_SPEED`
/// clamps to the nearest one).
pub const I2C_KHZ_STEPS: [u16; 9] = [100, 200, 400, 500, 600, 700, 800, 900, 1000];

/// `last_i2c_error` code in [`Vl53l4Info`] → human-readable name.
pub fn i2c_error_name(code: u8) -> &'static str {
    match code {
        0 => "none",
        1 => "NACK",
        2 => "TIMEOUT",
        3 => "BUS_ERROR",
        _ => "unknown",
    }
}

/// `VL53_READ_REG` payload: `addr u16, len u16` (little-endian).
pub fn pack_read_reg(addr: u16, len: u16) -> [u8; 4] {
    let a = addr.to_le_bytes();
    let l = len.to_le_bytes();
    [a[0], a[1], l[0], l[1]]
}

/// `VL53_WRITE_REG` payload: `addr u16` followed by the raw register bytes.
pub fn pack_write_reg(addr: u16, data: &[u8]) -> Vec<u8> {
    let mut out = Vec::with_capacity(2 + data.len());
    out.extend_from_slice(&addr.to_le_bytes());
    out.extend_from_slice(data);
    out
}

/// `VL53_XSHUT` payload: `action u8` ([`XSHUT_OFF`] / [`XSHUT_ON`] /
/// [`XSHUT_RESET`]).
pub fn pack_xshut(action: u8) -> [u8; 1] {
    [action]
}

/// `VL53_START_STREAM` payload: `addr u16, len u16, flags u8`.
pub fn pack_start_stream(addr: u16, len: u16, flags: u8) -> [u8; 5] {
    let a = addr.to_le_bytes();
    let l = len.to_le_bytes();
    [a[0], a[1], l[0], l[1], flags]
}

/// `VL53_SET_I2C_SPEED` payload: `khz u16`.
pub fn pack_set_i2c_speed(khz: u16) -> [u8; 2] {
    khz.to_le_bytes()
}

/// A decoded `RPT_VL53_REG_DATA` report (one register read).
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct RegData {
    /// Echoed `VL53_READ_REG` opcode (0x32).
    pub cmd: u8,
    /// MCU uptime at I2C-read completion.
    pub timestamp_us: u64,
    /// Raw register bytes (big-endian sensor contents, passed through).
    pub data: Vec<u8>,
}

impl RegData {
    pub fn unpack(payload: &[u8]) -> Result<RegData, CodecError> {
        if payload.len() < 9 {
            return Err(CodecError("VL53L4 reg data must be at least 9 bytes"));
        }
        Ok(RegData {
            cmd: payload[0],
            timestamp_us: u64::from_le_bytes(payload[1..9].try_into().unwrap()),
            data: payload[9..].to_vec(),
        })
    }
}

/// A decoded `RPT_VL53_INFO` report — bridge diagnostics. Counters are
/// free-running and wrap silently; watch increments, not absolute values.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct Vl53l4Info {
    pub int_edges: u32,
    pub slots_skipped: u32,
    pub i2c_errors: u32,
    /// 0 none, 1 NACK, 2 TIMEOUT, 3 BUS_ERROR ([`i2c_error_name`]).
    pub last_i2c_error: u8,
    /// 0x010F..0x0110 — expected [`super::MODEL_ID_VL53L4CD`] (0xEBAA).
    pub model_id: u16,
    /// 0x00E5 — expected 0x03 (booted).
    pub fw_status: u8,
    /// 1 = MODEL_ID matched on this read.
    pub initialized: u8,
    pub xshut_level: u8,
    pub int_level: u8,
    pub i2c_khz: u16,
}

impl Vl53l4Info {
    pub fn unpack(payload: &[u8]) -> Result<Vl53l4Info, CodecError> {
        if payload.len() < 21 {
            return Err(CodecError("VL53L4 info must be at least 21 bytes"));
        }
        Ok(Vl53l4Info {
            int_edges: u32::from_le_bytes(payload[0..4].try_into().unwrap()),
            slots_skipped: u32::from_le_bytes(payload[4..8].try_into().unwrap()),
            i2c_errors: u32::from_le_bytes(payload[8..12].try_into().unwrap()),
            last_i2c_error: payload[12],
            model_id: u16::from_le_bytes(payload[13..15].try_into().unwrap()),
            fw_status: payload[15],
            initialized: payload[16],
            xshut_level: payload[17],
            int_level: payload[18],
            i2c_khz: u16::from_le_bytes(payload[19..21].try_into().unwrap()),
        })
    }
}

/// A decoded `RPT_VL53_STREAM` report — one streamed register block.
/// `addr`/`len` echo the stream configuration so each report is
/// self-describing.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct StreamData {
    /// MCU uptime at the INT edge (the sensor event, not the I2C completion).
    pub timestamp_us: u64,
    pub addr: u16,
    pub len: u16,
    /// Raw register bytes (big-endian sensor contents, passed through).
    pub data: Vec<u8>,
}

impl StreamData {
    pub fn unpack(payload: &[u8]) -> Result<StreamData, CodecError> {
        if payload.len() < 12 {
            return Err(CodecError("VL53L4 stream data must be at least 12 bytes"));
        }
        let len = u16::from_le_bytes(payload[10..12].try_into().unwrap());
        if payload.len() < 12 + len as usize {
            return Err(CodecError("VL53L4 stream data shorter than its len field"));
        }
        Ok(StreamData {
            timestamp_us: u64::from_le_bytes(payload[0..8].try_into().unwrap()),
            addr: u16::from_le_bytes(payload[8..10].try_into().unwrap()),
            len,
            data: payload[12..12 + len as usize].to_vec(),
        })
    }
}
