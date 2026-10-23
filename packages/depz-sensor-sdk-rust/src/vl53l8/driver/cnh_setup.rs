//! The CNH (compact network histogram) configuration of a VL53L8CH / VL53L7CH.

use crate::devices::{DepzError, Result};
use crate::vl53l8::cnh::CnhDecodeConfig;

const DISABLE_PING_PONG: u8 = 0x01;
const DISABLE_VARIANCE: u8 = 0x02;
const ENABLE_AMBIENT_LEVEL: u8 = 0x04;
const ENABLE_XTALK_REMOVAL: u8 = 0x08;
const ZERO_NON_VALID_BINS: u8 = 0x10;
const STORE_REF_RESIDUAL: u8 = 0x20;
const PER_HEADER_WORDS: i64 = 5;
const PER_BUFFER_HEADER_WORDS: i64 = 2;

/// The CNH (compact network histogram) configuration of a VL53L8CH / VL53L7CH.
///
/// A mirror of `VL53LMZ_Motion_Configuration` with the plugin helpers that fill it
/// (`vl53lmz_plugin_cnh.c`, ULD 2.0.16), ported from the reference Python `CnhConfig`. Build it
/// with [`Vl53l8CnhSetup::init_config`] and [`Vl53l8CnhSetup::create_agg_map`], check
/// [`Vl53l8CnhSetup::required_memory`], then arm it on the device (the CNH block size goes to
/// [`super::Vl53l8Driver::start_ranging`], the struct to DCI [`Vl53l8CnhSetup::MI_CFG_DEV_IDX`]).
/// The flags are fixed to ping-pong and variance off (`Example_12_cnh_data.c`), which
/// [`crate::vl53l8::decode_cnh`] assumes. Arguments outside their fields are
/// [`DepzError::InvalidArgument`], never wrapped.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct Vl53l8CnhSetup {
    ref_bin_offset: i32,
    sum_span: i32,
    feature_length: i32,
    nb_of_aggregates: i32,
    cnh_cfg: u8,
    cnh_flex_shift: u8,
    map_id: [i8; 64],
}

impl Default for Vl53l8CnhSetup {
    fn default() -> Self {
        Self::new()
    }
}

impl Vl53l8CnhSetup {
    /// The DCI index the 156-byte configuration is written to.
    pub const MI_CFG_DEV_IDX: u16 = 0xBFAC;
    /// The largest on-device CNH buffer, bytes.
    pub const CNH_MAX_DATA_BYTES: u32 = 1540 * 4;
    /// Laser pulse width, device histogram bins.
    pub const PULSE_WIDTH_BIN: i32 = 10;
    /// Width of one device histogram bin, mm.
    pub const BIN_WIDTH_MM: f64 = 37.5348;

    /// An empty configuration: no bins, no aggregates, every zone in none.
    pub fn new() -> Self {
        Vl53l8CnhSetup {
            ref_bin_offset: 0,
            sum_span: 0,
            feature_length: 0,
            nb_of_aggregates: 0,
            cnh_cfg: 0,
            cnh_flex_shift: 0,
            map_id: [-1; 64],
        }
    }

    /// The first device bin, ×2048 (`ref_bin_offset`).
    pub fn ref_bin_offset(&self) -> i32 {
        self.ref_bin_offset
    }

    /// Device bins summed per CNH bin.
    pub fn sum_span(&self) -> i32 {
        self.sum_span
    }

    /// CNH bins per aggregate.
    pub fn feature_length(&self) -> i32 {
        self.feature_length
    }

    /// Aggregates in the map.
    pub fn nb_of_aggregates(&self) -> i32 {
        self.nb_of_aggregates
    }

    /// The CNH flags byte.
    pub fn cnh_cfg(&self) -> u8 {
        self.cnh_cfg
    }

    /// The CNH flex shift.
    pub fn cnh_flex_shift(&self) -> u8 {
        self.cnh_flex_shift
    }

    /// Device zone → CNH aggregate, −1 for a zone in none (64 entries).
    pub fn map_id(&self) -> [i8; 64] {
        self.map_id
    }

    /// The counts [`crate::vl53l8::decode_cnh`] needs to unpack this configuration's blocks.
    pub fn decode_config(&self) -> CnhDecodeConfig {
        CnhDecodeConfig {
            nb_of_aggregates: self.nb_of_aggregates as usize,
            feature_length: self.feature_length as usize,
        }
    }

    /// `vl53lmz_cnh_init_config`. `start_bin`: first device histogram bin; `num_bins`: CNH bins;
    /// `sub_sample`: device bins summed per CNH bin.
    pub fn init_config(&mut self, start_bin: i32, num_bins: i32, sub_sample: i32) -> Result<()> {
        // ref_bin_offset = start_bin * 2048 is an i32 field.
        if !(-1048576..=1048575).contains(&start_bin) {
            return Err(DepzError::InvalidArgument(format!(
                "start_bin out of range -1048576..1048575, got {start_bin}"
            )));
        }
        if !(0..=0xFF).contains(&num_bins) {
            return Err(DepzError::InvalidArgument(format!("num_bins out of range 0..255, got {num_bins}")));
        }
        if !(0..=0xFF).contains(&sub_sample) {
            return Err(DepzError::InvalidArgument(format!("sub_sample out of range 0..255, got {sub_sample}")));
        }
        self.ref_bin_offset = start_bin * 2048;
        self.feature_length = num_bins;
        self.sum_span = sub_sample;
        self.cnh_cfg = DISABLE_PING_PONG
            | DISABLE_VARIANCE
            | ENABLE_AMBIENT_LEVEL
            | ENABLE_XTALK_REMOVAL
            | ZERO_NON_VALID_BINS
            | STORE_REF_RESIDUAL;
        self.cnh_flex_shift = 1;
        Ok(())
    }

    /// `vl53lmz_cnh_create_agg_map`: aggregates of `merge_x`×`merge_y` zones, `cols`×`rows` of
    /// them from (`start_x`, `start_y`). `resolution` (16 or 64) must be the sensor's.
    #[allow(clippy::too_many_arguments)]
    pub fn create_agg_map(
        &mut self,
        resolution: usize,
        start_x: i32,
        start_y: i32,
        merge_x: i32,
        merge_y: i32,
        cols: i32,
        rows: i32,
    ) -> Result<()> {
        if resolution != 16 && resolution != 64 {
            return Err(DepzError::InvalidArgument(format!("resolution {resolution}: 16 or 64")));
        }
        let named = [
            ("start_x", start_x, 0),
            ("start_y", start_y, 0),
            ("merge_x", merge_x, 1),
            ("merge_y", merge_y, 1),
            ("cols", cols, 1),
            ("rows", rows, 1),
        ];
        for (name, value, low) in named {
            if value < low {
                return Err(DepzError::InvalidArgument(format!(
                    "agg map {name} {value} must be an integer >= {low}"
                )));
            }
        }
        let zone_res: i32 = if resolution == 16 { 4 } else { 8 };
        // In 64 bits: i32::MAX arguments must not wrap past the check.
        if start_x as i64 + cols as i64 * merge_x as i64 > zone_res as i64
            || start_y as i64 + rows as i64 * merge_y as i64 > zone_res as i64
        {
            return Err(DepzError::InvalidArgument("agg map exceeds zone grid".into()));
        }
        // Validate every aggregate id before touching the map (Java checks inside the loop and
        // leaves a half-filled map; the bounds above already make this unreachable).
        let mut map = [-1i8; 64];
        for row in start_y..start_y + rows * merge_y {
            for col in start_x..start_x + cols * merge_x {
                let agg_id = (row - start_y) / merge_y * cols + (col - start_x) / merge_x;
                if !(0..64).contains(&agg_id) {
                    return Err(DepzError::InvalidArgument("agg id out of range".into()));
                }
                map[(row * zone_res + col) as usize] = agg_id as i8;
            }
        }
        self.map_id = map;
        self.nb_of_aggregates = cols * rows;
        Ok(())
    }

    /// `vl53lmz_cnh_calc_required_memory`: the on-device CNH buffer, bytes. No aggregate map yet,
    /// or more than [`Vl53l8CnhSetup::CNH_MAX_DATA_BYTES`], is [`DepzError::InvalidArgument`].
    pub fn required_memory(&self) -> Result<u32> {
        if self.nb_of_aggregates == 0 {
            return Err(DepzError::InvalidArgument("agg map not created".into()));
        }
        let size = calc_required_memory(self.cnh_cfg, self.nb_of_aggregates as i64, self.feature_length as i64);
        if size > Self::CNH_MAX_DATA_BYTES as i64 {
            return Err(DepzError::InvalidArgument(format!(
                "CNH needs {size} B > max {} B — reduce aggregates or bins",
                Self::CNH_MAX_DATA_BYTES
            )));
        }
        Ok(size as u32)
    }

    /// (min, max) target distance, mm, the histogram captures whole.
    pub fn min_max_distance_mm(&self) -> (i32, i32) {
        let pulse = Self::PULSE_WIDTH_BIN as f64 / 2.0 * Self::BIN_WIDTH_MM;
        let start = self.ref_bin_offset as f64 / 2048.0;
        let span = self.sum_span as f64;
        let first = (start + span / 2.0) * Self::BIN_WIDTH_MM;
        let last = (start + (self.feature_length - 1) as f64 * span + span / 2.0) * Self::BIN_WIDTH_MM;
        ((first + pulse) as i32, (last - pulse) as i32)
    }

    /// Distance, mm, at the centre of CNH bin `bin_idx`.
    pub fn bin_center_mm(&self, bin_idx: i32) -> f64 {
        let span = self.sum_span as f64;
        (self.ref_bin_offset as f64 / 2048.0 + bin_idx as f64 * span + span / 2.0) * Self::BIN_WIDTH_MM
    }

    /// The 156-byte `VL53LMZ_Motion_Configuration` struct for the device (DCI 0xBFAC).
    pub fn pack(&self) -> Vec<u8> {
        let mut buf = vec![0u8; 156];
        buf[0..4].copy_from_slice(&self.ref_bin_offset.to_le_bytes());
        // detection threshold, extra noise sigma, null den clip, mem update mode/choice: 0
        buf[18] = self.sum_span as u8;
        buf[19] = self.feature_length as u8;
        buf[20] = self.nb_of_aggregates as u8;
        buf[21] = 1; // nb_of_temporal_accumulations
        buf[25] = self.cnh_cfg;
        buf[26] = self.cnh_flex_shift;
        for k in 0..64 {
            buf[28 + k] = self.map_id[k] as u8;
        }
        buf
    }
}

fn calc_required_memory(flags: u8, nb_agg: i64, feat_length: i64) -> i64 {
    let agg_x_feat = nb_agg * feat_length;
    let mut size = PER_BUFFER_HEADER_WORDS * 4;
    size += agg_x_feat * 4; // FEAT_INT
    size += (3 + agg_x_feat) / 4 * 4; // FEAT_FRAC (8 bits each, padded)
    size += nb_agg * 4; // AMBIENT_INT
    size += (3 + nb_agg) / 4 * 4; // AMBIENT_FRAC
    if flags & DISABLE_VARIANCE == 0 {
        size += agg_x_feat * 4; // VARIANCE_INT
        size += (3 + agg_x_feat) / 4 * 4; // VARIANCE_FRAC
    }
    size = size / 4 * 4;
    if flags & DISABLE_PING_PONG == 0 {
        size *= 2;
    }
    size + PER_HEADER_WORDS * 4
}
