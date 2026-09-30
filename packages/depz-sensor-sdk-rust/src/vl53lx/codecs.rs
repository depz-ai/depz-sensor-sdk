//! VL53L 1D-family register-bridge wire codecs, protocol v2.00
//! (contracts/12_SENSOR_VL53LX.md §2).
//!
//! The `APP_VL53L0_4` bridge is the VL53L4CD bridge of contract 10 with three
//! sensor facts moved to the host: the register-address width
//! (`VL53_SET_ADDR_WIDTH`, new), the interrupt-release writes (carried by
//! `VL53_START_STREAM`) and the boot handshake (no longer inside `VL53_XSHUT`).
//! `READ_REG`, `WRITE_REG`, `XSHUT`, `STOP_STREAM`, `SET_I2C_SPEED` and the
//! `REG_DATA` / `STREAM` reports are the contract-10 codecs, re-exported from
//! [`crate::vl53l4`].

use crate::protocol::common::CodecError;

/// VL53LX host→device command opcodes.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
#[repr(u8)]
pub enum Vl53lxCmd {
    ReadReg = 0x32,
    WriteReg = 0x33,
    Xshut = 0x34,
    StartStream = 0x35,
    StopStream = 0x36,
    GetInfo = 0x37,
    SetI2cSpeed = 0x38,
    /// New in v2.00: register-address width, 1 or 2 bytes (sticky, 2 after a
    /// reset). Set it before the first register access of a session and never
    /// under a running stream.
    SetAddrWidth = 0x39,
    /// New in v2.01 (firmware v0.24), no payload: zero `i2c_errors` /
    /// `last_i2c_error`. The host sends it after every sensor init — a
    /// resetting die NACKs for a moment, and that is no bus fault.
    ClearI2cErrors = 0x3A,
}

/// VL53LX device→host report opcodes.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
#[repr(u8)]
pub enum Vl53lxRpt {
    RegData = 0x91,
    Info = 0x92,
    Stream = 0x93,
}

/// Interrupt-release steps a stream may carry (`VL53_CLEAR_STEPS_WIRE_MAX`).
pub const CLEAR_STEPS_MAX: usize = 4;
/// `RPT_VL53_INFO` payload size (v2.00).
pub const INFO_SIZE: usize = 23;

/// `VL53_SET_ADDR_WIDTH` payload: `width u8` — 1 (VL53L0X) or 2 (the rest).
pub fn pack_set_addr_width(width: u8) -> Result<[u8; 1], CodecError> {
    if width != 1 && width != 2 {
        return Err(CodecError("register address width is 1 or 2 bytes"));
    }
    Ok([width])
}

/// `VL53_START_STREAM` payload (v2.00): `addr u16, len u16, flags u8,
/// n_clear u8, clear[n] {addr u16, value u8}` — `6 + 3n` bytes. `clear` is the
/// interrupt-release list the bridge plays after every block read (0..4
/// steps); more than [`CLEAR_STEPS_MAX`] is refused.
pub fn pack_start_stream(
    addr: u16,
    len: u16,
    clear: &[(u16, u8)],
    flags: u8,
) -> Result<Vec<u8>, CodecError> {
    if clear.len() > CLEAR_STEPS_MAX {
        return Err(CodecError("at most 4 interrupt-release steps"));
    }
    let mut out = Vec::with_capacity(6 + 3 * clear.len());
    out.extend_from_slice(&addr.to_le_bytes());
    out.extend_from_slice(&len.to_le_bytes());
    out.push(flags);
    out.push(clear.len() as u8);
    for &(step_addr, value) in clear {
        out.extend_from_slice(&step_addr.to_le_bytes());
        out.push(value);
    }
    Ok(out)
}

/// A decoded `RPT_VL53_INFO` report (v2.00, 23 bytes) — bridge state only; the
/// bridge reads no sensor register. Counters are free-running and wrap
/// silently: watch increments. `slots_skipped` = a slot that never got the
/// bus, `i2c_errors` = a bus that answered badly, `frames_dropped` = a good
/// sample the USB TX ring had no room for (since the stream was armed).
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct Vl53lxInfo {
    pub int_edges: u32,
    pub slots_skipped: u32,
    pub i2c_errors: u32,
    /// 0 none, 1 NACK, 2 TIMEOUT, 3 BUS_ERROR
    /// ([`crate::vl53l4::i2c_error_name`]).
    pub last_i2c_error: u8,
    pub xshut_level: u8,
    pub int_level: u8,
    pub i2c_khz: u16,
    pub addr_width: u8,
    pub n_clear: u8,
    pub frames_dropped: u32,
}

impl Vl53lxInfo {
    pub fn unpack(payload: &[u8]) -> Result<Vl53lxInfo, CodecError> {
        if payload.len() < INFO_SIZE {
            return Err(CodecError("RPT_VL53_INFO (v2.00) must be at least 23 bytes"));
        }
        let u32_at = |i: usize| u32::from_le_bytes(payload[i..i + 4].try_into().unwrap());
        Ok(Vl53lxInfo {
            int_edges: u32_at(0),
            slots_skipped: u32_at(4),
            i2c_errors: u32_at(8),
            last_i2c_error: payload[12],
            xshut_level: payload[13],
            int_level: payload[14],
            i2c_khz: u16::from_le_bytes([payload[15], payload[16]]),
            addr_width: payload[17],
            n_clear: payload[18],
            frames_dropped: u32_at(19),
        })
    }
}
