//! `.fwdepz` firmware container parsing (contracts/06_BOOTLOADER_FLASHING.md §2).

use crate::crc::{crc16_ccitt_false, crc32_iso_hdlc};

/// 64-byte fixed header.
pub const HEADER_SIZE: usize = 64;
/// Magic prefix.
pub const MAGIC: &[u8; 8] = b"FWDEPZ00";

/// Parsed `.fwdepz` header plus a payload-CRC verdict.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct FwdepzHeader {
    pub load_addr: u32,
    pub fw_size: u32,
    pub fw_crc32: u32,
    pub cur_sec: u8,
    pub tot_sec: u8,
    /// `true` when CRC-32/ISO-HDLC of the payload equals `fw_crc32`.
    pub payload_crc_ok: bool,
}

/// Reasons a `.fwdepz` blob fails validation, in the normative check order.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum FwdepzError {
    /// Too short to hold a header, or bad magic.
    Magic,
    /// Header CRC-16/CCITT-FALSE mismatch.
    HeaderCrc,
    /// `fw_size` != payload length.
    Size,
}

impl std::fmt::Display for FwdepzError {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        let s = match self {
            FwdepzError::Magic => "magic",
            FwdepzError::HeaderCrc => "header_crc",
            FwdepzError::Size => "size",
        };
        write!(f, "fwdepz error: {}", s)
    }
}

impl std::error::Error for FwdepzError {}

/// Parse and validate a `.fwdepz` blob. Validation order: magic → header CRC →
/// `fw_size == len(payload)` (contract 06 §2).
pub fn parse(file: &[u8]) -> Result<FwdepzHeader, FwdepzError> {
    if file.len() < HEADER_SIZE || &file[0..8] != MAGIC {
        return Err(FwdepzError::Magic);
    }
    let stored_hdr_crc = u16::from_le_bytes([file[62], file[63]]);
    if crc16_ccitt_false(&file[0..62]) != stored_hdr_crc {
        return Err(FwdepzError::HeaderCrc);
    }
    let load_addr = u32::from_le_bytes(file[8..12].try_into().unwrap());
    let fw_size = u32::from_le_bytes(file[12..16].try_into().unwrap());
    let fw_crc32 = u32::from_le_bytes(file[16..20].try_into().unwrap());
    let cur_sec = file[20];
    let tot_sec = file[21];

    let payload = &file[HEADER_SIZE..];
    if fw_size as usize != payload.len() {
        return Err(FwdepzError::Size);
    }
    let payload_crc_ok = crc32_iso_hdlc(payload) == fw_crc32;

    Ok(FwdepzHeader {
        load_addr,
        fw_size,
        fw_crc32,
        cur_sec,
        tot_sec,
        payload_crc_ok,
    })
}
