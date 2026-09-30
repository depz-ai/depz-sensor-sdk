//! VL53L8 register-bridge wire codecs (contracts/04_SENSOR_VL53L8.md §1–§4).
//!
//! Command payload packers (bridge fields little-endian; register contents
//! are big-endian sensor bytes passed through untouched) and the
//! `RPT_VL53_REG_DATA` unpacker. The VL53L5/L7 I2C bridge (contract 11) reuses
//! these bit-for-bit — only its transfer ceilings differ (see
//! [`crate::vl53l7`]). `RPT_VL53_FRAME` lives in [`super::framing`].

use crate::protocol::common::CodecError;

/// VL53L8 host→device command opcodes (0x34 is unused on VL53L8).
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
#[repr(u8)]
pub enum Vl53l8Cmd {
    ReadReg = 0x32,
    WriteReg = 0x33,
    StartStream = 0x35,
    StopStream = 0x36,
}

/// VL53L8 device→host report opcodes.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
#[repr(u8)]
pub enum Vl53l8Rpt {
    RegData = 0x91,
    Vl53Frame = 0x93,
}

/// Largest `VL53_READ_REG` `len` on VL53L8: the MCU transport buffer (2304 B)
/// minus the 9-byte `RPT_VL53_REG_DATA` header.
pub const READ_MAX_LEN: usize = 2295;
/// Transfer size the VL53L8 host splits reads/writes at.
pub const CHUNK_SIZE: usize = 2048;

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

/// `VL53_START_STREAM` payload: `frame_size u16`.
pub fn pack_start_stream(frame_size: u16) -> [u8; 2] {
    frame_size.to_le_bytes()
}

/// A decoded `RPT_VL53_REG_DATA` report (one register read).
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct RegData {
    /// Echoed `VL53_READ_REG` opcode (0x32).
    pub cmd: u8,
    pub timestamp_us: u64,
    /// Raw register bytes (big-endian sensor contents, passed through).
    pub data: Vec<u8>,
}

impl RegData {
    pub fn unpack(payload: &[u8]) -> Result<RegData, CodecError> {
        if payload.len() < 9 {
            return Err(CodecError("VL53L8 reg data must be at least 9 bytes"));
        }
        Ok(RegData {
            cmd: payload[0],
            timestamp_us: u64::from_le_bytes(payload[1..9].try_into().unwrap()),
            data: payload[9..].to_vec(),
        })
    }
}
