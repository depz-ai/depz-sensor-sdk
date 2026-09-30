//! Stateless decoders of the blocks the 1D-family bridge streams (contract 12
//! §4) — what a block says on its own, without the driver state an init leaves
//! behind:
//!
//! - [`decode_die_block`] — the 17-byte VL53L1-die result block at 0x0089
//!   (L1CX, L1CB, L3CX, L4CD, L4CX light drivers), fully decoded; the two ULDs
//!   that read it differ only in [`DieVariant`].
//! - [`decode_l0x_raw`] — raw fields of the VL53L0X 12-byte block at 0x14. The
//!   PAL range status, sigma and dmax need the device data cached at init.
//! - [`decode_histogram_raw`] — status bytes and the 24 photon bins of the
//!   83-byte histogram block at 0x0088. Bins → targets is the full driver.

use crate::vl53l4::STATUS_RTN;

/// Die result block address (`RESULT__RANGE_STATUS`).
pub const DIE_BLOCK_ADDR: u16 = 0x0089;
/// Die result block length.
pub const DIE_BLOCK_LEN: usize = 17;
/// VL53L0X result block address (8-bit register space).
pub const L0X_BLOCK_ADDR: u16 = 0x14;
/// VL53L0X result block length.
pub const L0X_BLOCK_LEN: usize = 12;
/// Histogram block address (`result__interrupt_status`).
pub const HISTOGRAM_BLOCK_ADDR: u16 = 0x0088;
/// Histogram block length (0x0088..=0x00DA).
pub const HISTOGRAM_BLOCK_LEN: usize = 83;
/// Photon bins per histogram block.
pub const HISTOGRAM_BINS: usize = 24;

// Register addresses inside the histogram block (ST Bare Driver names).
const RESULT_HISTOGRAM_BIN_0_2: usize = 0x008E;
const RESULT_HISTOGRAM_BIN_23_0: usize = 0x00D5;
const PHASECAL_RESULT_REFERENCE_PHASE: usize = 0x00D6;
const PHASECAL_RESULT_VCSEL_START: usize = 0x00D8;
const RESULT_HISTOGRAM_BIN_23_0_MSB: usize = 0x00D9;
const RESULT_HISTOGRAM_BIN_23_0_LSB: usize = 0x00DA;

/// Which ULD reads the die block: `(signal-rate byte offset, per-SPAD K)`.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum DieVariant {
    /// VL53L4CD ULD — also the L3CX ULP and L4CX-as-L4CD: `(5, 256)`.
    L4,
    /// VL53L1X ULD — crosstalk-corrected peak signal at 0x0098: `(15, 25)`.
    L1,
}

impl DieVariant {
    /// Lowercase name (`"l4"` / `"l1"`, matches the golden vectors).
    pub fn as_str(&self) -> &'static str {
        match self {
            DieVariant::L4 => "l4",
            DieVariant::L1 => "l1",
        }
    }

    pub fn from_name(s: &str) -> Option<DieVariant> {
        match s {
            "l4" => Some(DieVariant::L4),
            "l1" => Some(DieVariant::L1),
            _ => None,
        }
    }

    /// `(signal-rate byte offset, per-SPAD scale K)`.
    pub fn params(&self) -> (usize, u32) {
        match self {
            DieVariant::L4 => (5, 256),
            DieVariant::L1 => (15, 25),
        }
    }
}

/// A block shorter than its decoder needs.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct ShortBlock {
    pub need: usize,
    pub got: usize,
}

impl std::fmt::Display for ShortBlock {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        write!(f, "block needs {} bytes, got {}", self.need, self.got)
    }
}

impl std::error::Error for ShortBlock {}

fn need(raw: &[u8], n: usize) -> Result<(), ShortBlock> {
    if raw.len() < n {
        Err(ShortBlock { need: n, got: raw.len() })
    } else {
        Ok(())
    }
}

fn be16(raw: &[u8], i: usize) -> u32 {
    ((raw[i] as u32) << 8) | raw[i + 1] as u32
}

/// The die block decoded as a ULD reads it.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct DieResult {
    /// ULD status via `STATUS_RTN` (0 = valid).
    pub range_status: u8,
    pub distance_mm: u32,
    pub sigma_mm: u32,
    pub signal_rate_kcps: u32,
    pub ambient_rate_kcps: u32,
    pub signal_per_spad_kcps: u32,
    pub ambient_per_spad_kcps: u32,
    pub number_of_spad: u32,
    pub stream_count: u8,
}

/// The 17-byte die block (0x0089..0x0099) as the named ULD reads it.
pub fn decode_die_block(raw: &[u8], variant: DieVariant) -> Result<DieResult, ShortBlock> {
    need(raw, DIE_BLOCK_LEN)?;
    let (signal_at, k) = variant.params();
    let status = raw[0] & 0x1F;
    let range_status = STATUS_RTN.get(status as usize).copied().unwrap_or(status);
    let raw_spads = be16(raw, 3); // 8.8
    let signal = be16(raw, signal_at) * 8;
    let ambient = be16(raw, 7) * 8;
    let per_spad = |rate: u32| if raw_spads != 0 { rate * k / raw_spads } else { 0 };
    Ok(DieResult {
        range_status,
        distance_mm: be16(raw, 13),
        sigma_mm: be16(raw, 9) / 4,
        signal_rate_kcps: signal,
        ambient_rate_kcps: ambient,
        signal_per_spad_kcps: per_spad(signal),
        ambient_per_spad_kcps: per_spad(ambient),
        number_of_spad: raw_spads / 256,
        stream_count: raw[2],
    })
}

/// Raw fields of the VL53L0X result block.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct L0xRaw {
    /// mm (quarter-mm when RangeFractionalEnable, off by default).
    pub distance_raw: u32,
    /// Raw byte 0; the PAL status needs the init state.
    pub device_range_status: u8,
    /// FixPoint16.16 Mcps (9.7 on the wire `<< 9`).
    pub signal_rate_mcps_1616: u32,
    pub ambient_rate_mcps_1616: u32,
    /// 8.8.
    pub effective_spad_count_88: u32,
}

/// Raw fields of the VL53L0X block at 0x14 (`VL53L0X_GetRangingMeasurementData`
/// before the PAL status/sigma step).
pub fn decode_l0x_raw(raw: &[u8]) -> Result<L0xRaw, ShortBlock> {
    need(raw, L0X_BLOCK_LEN)?;
    Ok(L0xRaw {
        distance_raw: be16(raw, 10),
        device_range_status: raw[0],
        signal_rate_mcps_1616: be16(raw, 6) << 9,
        ambient_rate_mcps_1616: be16(raw, 8) << 9,
        effective_spad_count_88: be16(raw, 2),
    })
}

/// Status bytes and bins of the histogram block.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct HistogramRaw {
    pub interrupt_status: u8,
    pub range_status: u8,
    pub report_status: u8,
    pub stream_count: u8,
    pub dss_actual_effective_spads: u32,
    pub reference_phase: u32,
    pub vcsel_start: u8,
    /// 24 photon counts.
    pub bins: [u32; HISTOGRAM_BINS],
}

/// The 83-byte histogram block at 0x0088: status bytes and the 24 bins
/// (bin 23's low byte is carried in a separate MSB/LSB pair and patched in as
/// `(MSB << 2) + LSB` before the bins are read).
pub fn decode_histogram_raw(raw: &[u8]) -> Result<HistogramRaw, ShortBlock> {
    need(raw, HISTOGRAM_BLOCK_LEN)?;
    let off = HISTOGRAM_BLOCK_ADDR as usize;
    let mut buf = [0u8; HISTOGRAM_BLOCK_LEN];
    buf.copy_from_slice(&raw[..HISTOGRAM_BLOCK_LEN]);
    let msb = buf[RESULT_HISTOGRAM_BIN_23_0_MSB - off];
    let lsb = buf[RESULT_HISTOGRAM_BIN_23_0_LSB - off];
    buf[RESULT_HISTOGRAM_BIN_23_0 - off] = (msb << 2).wrapping_add(lsb);
    let base = RESULT_HISTOGRAM_BIN_0_2 - off;
    let mut bins = [0u32; HISTOGRAM_BINS];
    for (i, bin) in bins.iter_mut().enumerate() {
        let j = base + 3 * i;
        *bin = ((buf[j] as u32) << 16) | ((buf[j + 1] as u32) << 8) | buf[j + 2] as u32;
    }
    Ok(HistogramRaw {
        interrupt_status: buf[0],
        range_status: buf[1],
        report_status: buf[2],
        stream_count: buf[3],
        dss_actual_effective_spads: be16(&buf, 4),
        reference_phase: be16(&buf, PHASECAL_RESULT_REFERENCE_PHASE - off),
        vcsel_start: buf[PHASECAL_RESULT_VCSEL_START - off],
        bins,
    })
}
