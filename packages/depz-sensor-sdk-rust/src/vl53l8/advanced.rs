//! VL53L8 advanced-feature DCI codecs (UM3109 plugins, contract 04).
//!
//! Shared by both ToF variants (VL53L8CX and VL53L8CH): the advanced DCI byte
//! layouts are identical, so there is one set of codecs, not a per-variant pair.
//!
//! Pure encoders ported verbatim from the ST ULD (BSD-3): the motion-indicator
//! configuration struct, the crosstalk-margin scaling, and the 64-entry
//! detection-threshold block. These are the byte layouts written over the DCI;
//! the live register bridge that performs the writes is out of scope.

use super::decode::{RESOLUTION_4X4, RESOLUTION_8X8};

/// Detection-threshold measurement selectors (`measurement` field).
pub const DIST_MM: u8 = 1;
pub const SIGNAL_PER_SPAD_KCPS: u8 = 2;
pub const RANGE_SIGMA_MM: u8 = 4;
pub const AMBIENT_PER_SPAD_KCPS: u8 = 8;
pub const NB_TARGET_DETECTED: u8 = 9;
pub const TAR_STATUS: u8 = 12;
pub const NB_SPADS_ENABLED: u8 = 13;
pub const MOTION_INDICATOR: u8 = 19;

pub const NB_THRESHOLDS: usize = 64;

/// Power modes (`vl53l8cx_api.h`).
pub const POWER_MODE_SLEEP: u8 = 0;
pub const POWER_MODE_WAKEUP: u8 = 1;
pub const POWER_MODE_DEEP_SLEEP: u8 = 2;

/// Per-measurement fixed-point scale applied to threshold low/high on set
/// (get divides by the same factor); unlisted measurements scale by 1.
pub(crate) fn thresh_scale(measurement: u8) -> i64 {
    match measurement {
        DIST_MM => 4,
        SIGNAL_PER_SPAD_KCPS => 2048,
        RANGE_SIGMA_MM => 128,
        AMBIENT_PER_SPAD_KCPS => 2048,
        NB_SPADS_ENABLED => 256,
        MOTION_INDICATOR => 65535,
        _ => 1,
    }
}

/// Advanced-codec argument error (contract 07 §3: an argument the driver
/// would refuse, or one that does not fit its DCI field, is rejected before
/// anything is written — the ULD's `VL53L8CX_STATUS_INVALID_PARAM`).
#[derive(Debug, Clone, PartialEq, Eq)]
pub enum AdvancedError {
    /// Motion indicator resolution other than 16 (4×4) or 64 (8×8).
    InvalidResolution(usize),
    /// Xtalk margin above 10000 kcps.
    XtalkMarginOutOfRange,
    /// Threshold `index`: low/high × its measurement scale does not fit i32.
    ThresholdOverflow(usize),
    /// More than [`NB_THRESHOLDS`] thresholds.
    TooManyThresholds(usize),
}

impl std::fmt::Display for AdvancedError {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            AdvancedError::InvalidResolution(r) => {
                write!(f, "VL53L8 motion resolution {}: 16 or 64", r)
            }
            AdvancedError::XtalkMarginOutOfRange => {
                write!(f, "VL53L8 xtalk margin out of range 0..10000")
            }
            AdvancedError::ThresholdOverflow(k) => {
                write!(
                    f,
                    "VL53L8 thresholds[{}]: scaled low/high out of range for i32",
                    k
                )
            }
            AdvancedError::TooManyThresholds(n) => {
                write!(f, "VL53L8 at most {} thresholds, got {}", NB_THRESHOLDS, n)
            }
        }
    }
}

impl std::error::Error for AdvancedError {}

/// Largest xtalk margin `set_xtalk_margin` accepts, whole kcps/SPAD (as ST).
pub const XTALK_MARGIN_MAX_KCPS: u32 = 10000;

/// Xtalk margin, a whole number of kcps/SPAD (as ST) → raw DCI value
/// `kcps * 2048`, as `set_xtalk_margin` writes it: 0..=[`XTALK_MARGIN_MAX_KCPS`],
/// else [`AdvancedError::XtalkMarginOutOfRange`].
pub fn try_xtalk_margin_to_raw(margin_kcps: u32) -> Result<u32, AdvancedError> {
    if margin_kcps > XTALK_MARGIN_MAX_KCPS {
        return Err(AdvancedError::XtalkMarginOutOfRange);
    }
    Ok(margin_kcps * 2048)
}

/// Inverse: raw DCI value → whole kcps/SPAD (`raw / 2048`, integer division as ST).
pub fn xtalk_margin_from_raw(raw: u32) -> u32 {
    raw / 2048
}

/// Mirror of `VL53L8CX_Motion_Configuration` (156 bytes, plugin source).
/// `pack()` reproduces the C struct byte layout (`<i3I12B64b32B32B`).
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct MotionConfig {
    pub ref_bin_offset: i32,
    pub detection_threshold: u32,
    pub extra_noise_sigma: u32,
    pub null_den_clip_value: u32,
    pub mem_update_mode: u8,
    pub mem_update_choice: u8,
    pub sum_span: u8,
    pub feature_length: u8,
    pub nb_of_aggregates: u8,
    pub nb_of_temporal_accumulations: u8,
    pub min_nb_for_global_detection: u8,
    pub global_indicator_format_1: u8,
    pub global_indicator_format_2: u8,
    pub spare1: u8,
    pub spare2: u8,
    pub spare3: u8,
    pub map_id: [i8; 64],
    pub indicator_format_1: [u8; 32],
    pub indicator_format_2: [u8; 32],
}

impl Default for MotionConfig {
    fn default() -> Self {
        MotionConfig {
            ref_bin_offset: 0,
            detection_threshold: 0,
            extra_noise_sigma: 0,
            null_den_clip_value: 0,
            mem_update_mode: 0,
            mem_update_choice: 0,
            sum_span: 0,
            feature_length: 0,
            nb_of_aggregates: 0,
            nb_of_temporal_accumulations: 0,
            min_nb_for_global_detection: 0,
            global_indicator_format_1: 0,
            global_indicator_format_2: 0,
            spare1: 0,
            spare2: 0,
            spare3: 0,
            map_id: [0; 64],
            indicator_format_1: [0; 32],
            indicator_format_2: [0; 32],
        }
    }
}

impl MotionConfig {
    /// Serialize to the 156-byte DCI payload.
    pub fn pack(&self) -> Vec<u8> {
        let mut out = Vec::with_capacity(156);
        out.extend_from_slice(&self.ref_bin_offset.to_le_bytes());
        out.extend_from_slice(&self.detection_threshold.to_le_bytes());
        out.extend_from_slice(&self.extra_noise_sigma.to_le_bytes());
        out.extend_from_slice(&self.null_den_clip_value.to_le_bytes());
        out.push(self.mem_update_mode);
        out.push(self.mem_update_choice);
        out.push(self.sum_span);
        out.push(self.feature_length);
        out.push(self.nb_of_aggregates);
        out.push(self.nb_of_temporal_accumulations);
        out.push(self.min_nb_for_global_detection);
        out.push(self.global_indicator_format_1);
        out.push(self.global_indicator_format_2);
        out.push(self.spare1);
        out.push(self.spare2);
        out.push(self.spare3);
        for b in &self.map_id {
            out.push(*b as u8);
        }
        out.extend_from_slice(&self.indicator_format_1);
        out.extend_from_slice(&self.indicator_format_2);
        out
    }

    /// Set `map_id` for the resolution (pure; mirrors `_set_resolution`):
    /// 16 or 64 zones, else [`AdvancedError::InvalidResolution`] with
    /// `map_id` untouched.
    pub fn try_set_resolution(&mut self, resolution: usize) -> Result<(), AdvancedError> {
        if resolution != RESOLUTION_4X4 && resolution != RESOLUTION_8X8 {
            return Err(AdvancedError::InvalidResolution(resolution));
        }
        self.set_resolution_unchecked(resolution);
        Ok(())
    }

    fn set_resolution_unchecked(&mut self, resolution: usize) {
        if resolution == RESOLUTION_4X4 {
            for i in 0..16 {
                self.map_id[i] = i as i8;
            }
            for i in 16..64 {
                self.map_id[i] = -1;
            }
        } else if resolution == RESOLUTION_8X8 {
            for i in 0..64 {
                self.map_id[i] = (((i % 8) >> 1) + 4 * (i / 16)) as i8;
            }
        }
    }
}

/// The default motion-indicator configuration `motion_indicator_init` programs
/// for a resolution (the exact bytes written to the sensor); a resolution
/// other than 16 or 64 is [`AdvancedError::InvalidResolution`].
pub fn try_default_motion_config(resolution: usize) -> Result<MotionConfig, AdvancedError> {
    let mut cfg = base_motion_config();
    cfg.try_set_resolution(resolution)?;
    Ok(cfg)
}

fn base_motion_config() -> MotionConfig {
    MotionConfig {
        ref_bin_offset: 13633,
        detection_threshold: 2_883_584,
        extra_noise_sigma: 0,
        null_den_clip_value: 0,
        mem_update_mode: 6,
        mem_update_choice: 2,
        sum_span: 4,
        feature_length: 9,
        nb_of_aggregates: 16,
        nb_of_temporal_accumulations: 16,
        min_nb_for_global_detection: 1,
        global_indicator_format_1: 8,
        global_indicator_format_2: 0,
        ..MotionConfig::default()
    }
}

/// One detection-threshold entry (real units; scaled on pack).
#[derive(Debug, Clone, Copy, PartialEq, Eq, Default)]
pub struct DetectionThreshold {
    pub low_thresh: i32,
    pub high_thresh: i32,
    pub measurement: u8,
    pub type_: u8,
    pub zone_num: u8,
    pub operation: u8,
}

/// The two DCI blocks written by `set_detection_thresholds`.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct DetectionThresholdBlocks {
    /// `DCI_DET_THRESH_START` payload: 64 × 12 bytes.
    pub start: Vec<u8>,
    /// `DCI_DET_THRESH_VALID_STATUS`: 8 bytes, all `0x05`.
    pub valid: Vec<u8>,
}

/// Pack up to 64 detection thresholds into their DCI blocks. Entries beyond
/// the supplied list are zero-filled. Each entry's low/high are multiplied by
/// its measurement's fixed-point scale; a product that does not fit the i32
/// field is [`AdvancedError::ThresholdOverflow`] (never wrapped), more than
/// 64 entries [`AdvancedError::TooManyThresholds`].
pub fn try_pack_detection_thresholds(
    thresholds: &[DetectionThreshold],
) -> Result<DetectionThresholdBlocks, AdvancedError> {
    if thresholds.len() > NB_THRESHOLDS {
        return Err(AdvancedError::TooManyThresholds(thresholds.len()));
    }
    for (k, t) in thresholds.iter().enumerate() {
        let scale = thresh_scale(t.measurement);
        for v in [t.low_thresh, t.high_thresh] {
            if i32::try_from(v as i64 * scale).is_err() {
                return Err(AdvancedError::ThresholdOverflow(k));
            }
        }
    }
    Ok(pack_detection_thresholds_unchecked(thresholds))
}

fn pack_detection_thresholds_unchecked(
    thresholds: &[DetectionThreshold],
) -> DetectionThresholdBlocks {
    let valid = vec![0x05u8; 8];
    let mut start = vec![0u8; NB_THRESHOLDS * 12];
    for k in 0..NB_THRESHOLDS {
        let t = thresholds.get(k).copied().unwrap_or_default();
        let scale = thresh_scale(t.measurement);
        let low = (t.low_thresh as i64 * scale) as i32;
        let high = (t.high_thresh as i64 * scale) as i32;
        let off = k * 12;
        start[off..off + 4].copy_from_slice(&low.to_le_bytes());
        start[off + 4..off + 8].copy_from_slice(&high.to_le_bytes());
        start[off + 8] = t.measurement;
        start[off + 9] = t.type_;
        start[off + 10] = t.zone_num;
        start[off + 11] = t.operation;
    }
    DetectionThresholdBlocks { start, valid }
}
