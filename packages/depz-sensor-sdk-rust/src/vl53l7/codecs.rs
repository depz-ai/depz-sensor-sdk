//! VL53L5CX / VL53L7CX / VL53L7CH I2C register-bridge wire codecs
//! (contracts/11_SENSOR_VL53L7.md §2).
//!
//! Commands 0x32/0x33/0x35/0x36 and reports 0x91/0x93 are bit-for-bit the
//! VL53L8 bridge (contract 04) and are reused from [`crate::vl53l8`]; this
//! module holds only what the I2C board adds: `VL53_PIN_CTRL`,
//! `VL53_GET_INFO`, `VL53_SET_I2C_SPEED`, `RPT_VL53_INFO`, and its tighter
//! transfer limits.

use crate::protocol::common::CodecError;

/// Host→device opcodes the L5/L7 bridge adds on top of the VL53L8 set.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
#[repr(u8)]
pub enum Vl53l7Cmd {
    PinCtrl = 0x34,
    GetInfo = 0x37,
    SetI2cSpeed = 0x38,
}

/// Device→host reports the L5/L7 bridge adds on top of the VL53L8 set.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
#[repr(u8)]
pub enum Vl53l7Rpt {
    /// `RPT_VL53_INFO` — carries **no** echoed command byte.
    Info = 0x92,
}

/// `VL53_PIN_CTRL` action: stop streaming, drive LPn low (sensor I2C
/// interface off, reads NACK). None of the actions is a true sensor reset:
/// after [`PIN_LPN_OFF`] or [`PIN_SOFT_CYCLE`] the host must re-run `init()`.
pub const PIN_LPN_OFF: u8 = 0;
/// `VL53_PIN_CTRL` action: drive LPn high (power-up default).
pub const PIN_LPN_ON: u8 = 1;
/// `VL53_PIN_CTRL` action: pulse I2C_RST.
pub const PIN_I2C_RST: u8 = 2;
/// `VL53_PIN_CTRL` action: stop streaming, LPn low 1 ms, high, I2C_RST pulse,
/// then clear the I2C error counters.
pub const PIN_SOFT_CYCLE: u8 = 3;

/// `VL53_READ_REG` `len` ceiling (`VL53LMZ_READ_MAX`): 1..1536. Hosts MUST
/// split reads here — the VL53L8 split (2048) fails with `ERR_INVALID_PARAM`.
pub const READ_MAX_LEN: usize = 1536;
/// `VL53_WRITE_REG` data ceiling (`VL53LMZ_XFER_MAX`): 1..2048.
pub const WRITE_MAX_LEN: usize = 2048;
/// Bytes of frame data per `RPT_VL53_FRAME` chunk (VL53L8: 1528).
pub const STREAM_CHUNK_MAX: usize = 1536;
/// `RPT_VL53_INFO` payload size.
pub const INFO_SIZE: usize = 20;

/// Nominal SCL steps the firmware carries a timing for; others snap to the
/// nearest one.
pub const I2C_SPEED_STEPS_KHZ: [u16; 9] = [100, 200, 400, 500, 600, 700, 800, 900, 1000];

/// `last_i2c_error` code in [`Vl53l7Info`] → human-readable name.
pub fn i2c_error_name(code: u8) -> &'static str {
    match code {
        0 => "OK",
        1 => "NACK",
        2 => "TIMEOUT",
        3 => "BUS_ERROR",
        _ => "unknown",
    }
}

/// `VL53_PIN_CTRL` payload: `action u8` ([`PIN_LPN_OFF`] .. [`PIN_SOFT_CYCLE`]).
pub fn pack_pin_ctrl(action: u8) -> [u8; 1] {
    [action]
}

/// `VL53_SET_I2C_SPEED` payload: `khz u16` (little-endian).
pub fn pack_set_i2c_speed(khz: u16) -> [u8; 2] {
    khz.to_le_bytes()
}

/// A decoded `RPT_VL53_INFO` report — bridge state only (the sensor is never
/// probed). Counters run from power-up / `DEVICE_RESET`; `SOFT_CYCLE` clears
/// the I2C ones.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct Vl53l7Info {
    pub int_edges: u32,
    pub frames_dropped: u32,
    pub i2c_errors: u32,
    /// 0 OK, 1 NACK, 2 TIMEOUT, 3 BUS_ERROR ([`i2c_error_name`]).
    pub last_i2c_error: u8,
    pub lpn_level: u8,
    pub int_level: u8,
    pub i2c_khz: u16,
    pub frame_size: u16,
    pub streaming: bool,
}

impl Vl53l7Info {
    /// Unpack `<IIIBBBHHB` (20 bytes, little-endian); shorter payloads are
    /// rejected.
    pub fn unpack(payload: &[u8]) -> Result<Vl53l7Info, CodecError> {
        if payload.len() < INFO_SIZE {
            return Err(CodecError("VL53L7 info must be at least 20 bytes"));
        }
        Ok(Vl53l7Info {
            int_edges: u32::from_le_bytes(payload[0..4].try_into().unwrap()),
            frames_dropped: u32::from_le_bytes(payload[4..8].try_into().unwrap()),
            i2c_errors: u32::from_le_bytes(payload[8..12].try_into().unwrap()),
            last_i2c_error: payload[12],
            lpn_level: payload[13],
            int_level: payload[14],
            i2c_khz: u16::from_le_bytes(payload[15..17].try_into().unwrap()),
            frame_size: u16::from_le_bytes(payload[17..19].try_into().unwrap()),
            streaming: payload[19] != 0,
        })
    }
}
