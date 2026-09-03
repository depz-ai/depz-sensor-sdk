//! VL53L8CH CNH (Compact Network Histogram) decode — **CH-only** (contract 04).
//!
//! CNH is the reason to run the VL53L8CH firmware instead of plain VL53L8CX: the
//! sensor returns a per-aggregate distance histogram on top of the normal
//! ranging frame. The raw CNH output block is surfaced by the frame decoder
//! ([`crate::vl53l8::decode::Vl53l8Results::cnh_raw`]); this module unpacks it
//! into per-aggregate integer histograms.
//!
//! This is a 1:1 port of the decode half of the Python
//! `depz_sensor_sdk.vl53l8.cnh` (`decode` / `_decode_aggregate`), which in turn
//! ports `vl53lmz_cnh_get_block_addresses` / `_cnh_get_mem_block_addresses` from
//! the VL53LMZ ULD 2.0.16 CNH plugin for the fixed `cnh_cfg`
//! (ping-pong + variance disabled) used by DEPZ firmware. The offset arithmetic
//! is reproduced exactly; do not "simplify" it.
//!
//! Layout of the byte-swapped CNH block (little-endian 32-bit words):
//! - word[2] (byte offset 8) = `ref_residual_word` (u32).
//! - a 5-word persistent header, then one (ping-pong disabled) buffer whose size
//!   in words is read from the header's buffer-info word; each buffer starts
//!   with a 2-word buffer header, followed by:
//!     - `FEAT_INT`   : `i32[nb_agg * feat]`
//!     - `FEAT_FRAC`  : `i8 [nb_agg * feat]` (padded up to a word boundary)
//!     - `AMBIENT_INT`: `i32[nb_agg]`
//!     - `AMBIENT_FRAC`: `i8[nb_agg]`
//!   The real histogram value is `hist_raw / 2**hist_scaler`.

// ---- persistent-data header layout (plugin_cnh.c) ----
const CNH_PER_HEADER_WORDS: usize = 5; // CNH_PER_HEADER_BYTES / 4
const CNH_PER_BUFFER_HEADER_WORDS: usize = 2; // CNH_PER_BUFFER_HEADER_BYTES / 4
const CNH_PER_HEADER_BUFFER_INFO_IDX: usize = 1;
const CNH_PER_HEADER_FLAGS_IDX: usize = 3;
const BUFFER_INFO_WORDS_MASK: u32 = 0xFFFF;
const BUFFER_INFO_FLAGS_SHIFT: u32 = 24;
const MI_STATE_PING: i32 = 0;

/// Minimal config needed to decode a captured CNH block: the aggregate count and
/// per-aggregate feature (bin) length the sensor was configured with. These must
/// match the `CnhConfig` used when programming the device (see the Python
/// `CnhConfig.nb_of_aggregates` / `feature_length`).
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct CnhDecodeConfig {
    /// Number of CNH aggregates (`cfg.nb_of_aggregates`).
    pub nb_of_aggregates: usize,
    /// CNH bins per aggregate (`cfg.feature_length`).
    pub feature_length: usize,
}

/// One decoded CNH aggregate. The real histogram value for bin `i` is
/// `hist_raw[i] as f64 / 2f64.powi(hist_scaler[i] as i32)`.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct CnhAggregate {
    /// Per-bin integer mantissa (`FEAT_INT`), length == `feature_length`.
    pub hist_raw: Vec<i32>,
    /// Per-bin power-of-two scaler (`FEAT_FRAC`), length == `feature_length`.
    pub hist_scaler: Vec<i8>,
}

/// Result of [`decode_cnh`].
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct CnhData {
    /// Reference residual word, u32 at byte offset 8 (`words[2]`). Real value is
    /// `ref_residual_word as f64 / 2048.0` (11 fractional bits).
    pub ref_residual_word: u32,
    /// Per-aggregate histograms, length == `nb_of_aggregates`.
    pub aggregates: Vec<CnhAggregate>,
}

/// CNH decode failure.
#[derive(Debug, Clone, PartialEq, Eq)]
pub enum CnhError {
    /// `raw` is too short for the header or the computed block extends past it.
    Truncated,
    /// `nb_of_aggregates` or `feature_length` is zero.
    EmptyConfig,
}

impl std::fmt::Display for CnhError {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            CnhError::Truncated => write!(f, "VL53L8 CNH block truncated"),
            CnhError::EmptyConfig => write!(f, "VL53L8 CNH empty config"),
        }
    }
}

impl std::error::Error for CnhError {}

/// Read a signed little-endian i32 word at byte offset `off`.
#[inline]
fn read_i32(raw: &[u8], off: usize) -> Result<i32, CnhError> {
    let end = off.checked_add(4).ok_or(CnhError::Truncated)?;
    let bytes = raw.get(off..end).ok_or(CnhError::Truncated)?;
    Ok(i32::from_le_bytes([bytes[0], bytes[1], bytes[2], bytes[3]]))
}

/// Read a signed i8 at byte offset `off`.
#[inline]
fn read_i8(raw: &[u8], off: usize) -> Result<i8, CnhError> {
    raw.get(off).map(|&b| b as i8).ok_or(CnhError::Truncated)
}

/// Decode a captured CNH data block (`raw` bytes, byte-swapped exactly like the
/// standard ranging blocks — i.e. [`crate::vl53l8::decode::Vl53l8Results::cnh_raw`])
/// into per-aggregate integer histograms plus the reference-residual word.
///
/// Faithful port of the Python `cnh.decode` / `_decode_aggregate` for the fixed
/// DEPZ `cnh_cfg` (ping-pong + variance disabled). With ping-pong disabled the
/// device reports a single buffer and the ping/pong selection resolves to the
/// sole buffer.
pub fn decode_cnh(cfg: &CnhDecodeConfig, raw: &[u8]) -> Result<CnhData, CnhError> {
    let nb_agg = cfg.nb_of_aggregates;
    let feat = cfg.feature_length;
    if nb_agg == 0 || feat == 0 {
        return Err(CnhError::EmptyConfig);
    }

    // ref_residual: u32 at word[2] (byte offset 8) — vl53lmz_cnh_get_ref_residual.
    let ref_residual_word = read_i32(raw, 2 * 4)? as u32;

    let mut aggregates = Vec::with_capacity(nb_agg);
    for agg_id in 0..nb_agg {
        aggregates.push(decode_aggregate(raw, nb_agg, feat, agg_id)?);
    }

    Ok(CnhData {
        ref_residual_word,
        aggregates,
    })
}

fn decode_aggregate(
    raw: &[u8],
    nb_agg: usize,
    feat: usize,
    agg_id: usize,
) -> Result<CnhAggregate, CnhError> {
    let agg_x_feat = nb_agg * feat;
    let agg_off = agg_id * feat;

    let state = read_i32(raw, 0)?;
    let info = read_i32(raw, CNH_PER_HEADER_BUFFER_INFO_IDX * 4)? as u32;
    let pp_size = (info & BUFFER_INFO_WORDS_MASK) as usize;
    let _buffer_flags = (info >> BUFFER_INFO_FLAGS_SHIFT) & 0xFF;

    // Select ping or pong buffer exactly as the C code does. With ping-pong
    // disabled the device reports a single buffer and this resolves to ping.
    let flags = read_i32(raw, CNH_PER_HEADER_FLAGS_IDX * 4)?;
    let mut local_pp: usize = 1;
    if (flags & 0x10) == 0x10 {
        local_pp = 1;
    }
    if state == MI_STATE_PING {
        local_pp = 1 - local_pp;
    }

    let mut base = CNH_PER_HEADER_WORDS;
    if local_pp == 1 {
        base += pp_size;
    }
    // buffer header is 2 words (state, nb_accumulated); data starts after it.
    let mut blk = (base + CNH_PER_BUFFER_HEADER_WORDS) * 4; // byte offset of p[2]

    // FEAT_INT: int32 per (agg, feat).
    let feat_int_off = blk + agg_off * 4;
    let mut hist_raw = Vec::with_capacity(feat);
    for i in 0..feat {
        hist_raw.push(read_i32(raw, feat_int_off + i * 4)?);
    }
    blk += agg_x_feat * 4;

    // FEAT_FRAC: int8 scaler per (agg, feat).
    let feat_frac_off = blk + agg_off;
    let mut hist_scaler = Vec::with_capacity(feat);
    for i in 0..feat {
        hist_scaler.push(read_i8(raw, feat_frac_off + i)?);
    }
    // (AMBIENT_INT / AMBIENT_FRAC follow after ((3 + agg_x_feat) / 4) * 4 bytes;
    // not decoded here — the DEPZ CNH surface returns histograms only.)

    Ok(CnhAggregate {
        hist_raw,
        hist_scaler,
    })
}
