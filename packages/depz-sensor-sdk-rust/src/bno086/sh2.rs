//! SH-2 control-channel request builders (contract 05 §6).
//!
//! Pure encoders — no I/O, no timing. The SHTP framing/seq is [`super::shtp`]'s
//! concern. All multi-byte fields little-endian.
//!
//! Mirrors the TS/Python `sh2` builders.

/// Report IDs on SHTP channel 2 (control).
pub const REPORT_COMMAND_RESPONSE: u8 = 0xf1;
pub const REPORT_COMMAND_REQUEST: u8 = 0xf2;
pub const REPORT_FRS_READ_RESPONSE: u8 = 0xf3;
pub const REPORT_FRS_READ_REQUEST: u8 = 0xf4;
pub const REPORT_FRS_WRITE_RESPONSE: u8 = 0xf5;
pub const REPORT_FRS_WRITE_DATA: u8 = 0xf6;
pub const REPORT_FRS_WRITE_REQUEST: u8 = 0xf7;
pub const REPORT_PRODUCT_ID_RESPONSE: u8 = 0xf8;
pub const REPORT_PRODUCT_ID_REQUEST: u8 = 0xf9;
pub const REPORT_GET_FEATURE_RESPONSE: u8 = 0xfc;
pub const REPORT_SET_FEATURE_COMMAND: u8 = 0xfd;
pub const REPORT_GET_FEATURE_REQUEST: u8 = 0xfe;

/// SH-2 command builder error.
#[derive(Debug, Clone, PartialEq, Eq)]
pub enum Sh2Error {
    /// Command request carries at most 9 parameter bytes.
    TooManyParams(usize),
    /// FRS write data carries 1 or 2 words.
    BadWriteWordCount(usize),
}

impl std::fmt::Display for Sh2Error {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            Sh2Error::TooManyParams(n) => {
                write!(f, "command request carries at most 9 params, got {}", n)
            }
            Sh2Error::BadWriteWordCount(n) => write!(f, "FRS write data carries 1 or 2 words, got {}", n),
        }
    }
}

impl std::error::Error for Sh2Error {}

/// Set Feature Command (0xFD), 17 bytes. `interval_us` = 0 disables the sensor.
pub fn build_set_feature(
    sensor_id: u8,
    interval_us: u32,
    batch_us: u32,
    sensitivity: u16,
    flags: u8,
    cfg_word: u32,
) -> [u8; 17] {
    let mut out = [0u8; 17];
    out[0] = REPORT_SET_FEATURE_COMMAND;
    out[1] = sensor_id;
    out[2] = flags;
    out[3..5].copy_from_slice(&sensitivity.to_le_bytes());
    out[5..9].copy_from_slice(&interval_us.to_le_bytes());
    out[9..13].copy_from_slice(&batch_us.to_le_bytes());
    out[13..17].copy_from_slice(&cfg_word.to_le_bytes());
    out
}

/// Get Feature Request (0xFE), 2 bytes.
pub fn build_get_feature_request(sensor_id: u8) -> [u8; 2] {
    [REPORT_GET_FEATURE_REQUEST, sensor_id]
}

/// Product ID Request (0xF9), 2 bytes.
pub fn build_product_id_request() -> [u8; 2] {
    [REPORT_PRODUCT_ID_REQUEST, 0x00]
}

/// Command Request (0xF2), 12 bytes: id, seq, command, P0..P8.
pub fn build_command_request(seq: u8, command: u8, params: &[u8]) -> Result<[u8; 12], Sh2Error> {
    if params.len() > 9 {
        return Err(Sh2Error::TooManyParams(params.len()));
    }
    let mut out = [0u8; 12];
    out[0] = REPORT_COMMAND_REQUEST;
    out[1] = seq;
    out[2] = command;
    out[3..3 + params.len()].copy_from_slice(params);
    Ok(out)
}

/// FRS Read Request (0xF4), 8 bytes. `block_words` = 0 reads the whole record.
pub fn build_frs_read_request(frs_type: u16, offset_words: u16, block_words: u16) -> [u8; 8] {
    let mut out = [0u8; 8];
    out[0] = REPORT_FRS_READ_REQUEST;
    out[1] = 0;
    out[2..4].copy_from_slice(&offset_words.to_le_bytes());
    out[4..6].copy_from_slice(&frs_type.to_le_bytes());
    out[6..8].copy_from_slice(&block_words.to_le_bytes());
    out
}

/// FRS Write Request (0xF7), 6 bytes. `length_words` = 0 erases the record.
pub fn build_frs_write_request(frs_type: u16, length_words: u16) -> [u8; 6] {
    let mut out = [0u8; 6];
    out[0] = REPORT_FRS_WRITE_REQUEST;
    out[1] = 0;
    out[2..4].copy_from_slice(&length_words.to_le_bytes());
    out[4..6].copy_from_slice(&frs_type.to_le_bytes());
    out
}

/// FRS Write Data (0xF6), 12 bytes; 1 or 2 words per packet.
pub fn build_frs_write_data(offset_words: u16, words: &[u32]) -> Result<[u8; 12], Sh2Error> {
    if words.is_empty() || words.len() > 2 {
        return Err(Sh2Error::BadWriteWordCount(words.len()));
    }
    let mut out = [0u8; 12];
    out[0] = REPORT_FRS_WRITE_DATA;
    out[1] = 0;
    out[2..4].copy_from_slice(&offset_words.to_le_bytes());
    out[4..8].copy_from_slice(&words[0].to_le_bytes());
    out[8..12].copy_from_slice(&words.get(1).copied().unwrap_or(0).to_le_bytes());
    Ok(out)
}
