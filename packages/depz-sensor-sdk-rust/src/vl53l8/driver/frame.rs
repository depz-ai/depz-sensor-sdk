//! One decoded ranging frame, device level.

use crate::vl53l8::decode::Vl53l8Results;

/// The motion-indicator block of a frame (`VL53L8CX_ResultsData MotionIndicator`).
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct Vl53l8Motion {
    /// Global indicator 1.
    pub global_indicator_1: u32,
    /// Global indicator 2.
    pub global_indicator_2: u32,
    /// Motion-indicator status.
    pub status: u8,
    /// Aggregates with motion detected.
    pub nb_of_detected_aggregates: u8,
    /// Aggregates configured.
    pub nb_of_aggregates: u8,
    /// Per-aggregate motion value (32 entries).
    pub motion: Vec<u32>,
}

/// One decoded ranging frame.
///
/// Per-zone vectors are sized to the active resolution (16 for 4×4, 64 for 8×8); zone index runs
/// row-major. Values are in ST GetRangingData's real units (contract 04 §5): a negative distance
/// is clamped to 0, the rates and the reflectance keep their fraction. `cnh_raw` is the raw CNH data block (VL53L8CH / VL53L7CH with CNH armed),
/// else `None`; `motion` the motion-indicator output when it is configured, else `None`.
#[derive(Debug, Clone, PartialEq)]
pub struct Vl53l8Frame {
    /// Host timestamp the caller gave the frame, µs.
    pub timestamp_us: u64,
    /// Zones in this frame (16 or 64).
    pub resolution: usize,
    /// Silicon temperature, °C.
    pub silicon_temp_degc: i8,
    /// Distance per zone and target, mm (≥ 0).
    pub distance_mm: Vec<i32>,
    /// Target status per zone and target: 5 / 9 valid, 255 no target detected.
    pub target_status: Vec<u8>,
    /// Targets detected per zone.
    pub nb_target_detected: Vec<u8>,
    /// Signal rate per zone and target, kcps/SPAD.
    pub signal_per_spad: Vec<f64>,
    /// Ambient rate per zone, kcps/SPAD.
    pub ambient_per_spad: Vec<f64>,
    /// SPADs enabled per zone (raw, ÷256).
    pub nb_spads_enabled: Vec<u32>,
    /// Range sigma per zone and target, mm.
    pub range_sigma_mm: Vec<f64>,
    /// Reflectance estimate per zone and target, %.
    pub reflectance: Vec<f64>,
    /// The raw CNH block (word-swapped like every block), or `None`; decode with
    /// [`crate::vl53l8::decode_cnh`].
    pub cnh_raw: Option<Vec<u8>>,
    /// The motion-indicator output, or `None` when the indicator is not configured.
    pub motion: Option<Vl53l8Motion>,
}

impl Vl53l8Frame {
    pub(crate) fn from_results(
        timestamp_us: u64,
        r: Vl53l8Results,
        motion: Option<Vl53l8Motion>,
    ) -> Self {
        Vl53l8Frame {
            timestamp_us,
            resolution: r.resolution(),
            silicon_temp_degc: r.silicon_temp_degc,
            range_sigma_mm: r.range_sigma_mm,
            distance_mm: r.distance_mm,
            target_status: r.target_status,
            nb_target_detected: r.nb_target_detected,
            signal_per_spad: r.signal_per_spad,
            ambient_per_spad: r.ambient_per_spad,
            nb_spads_enabled: r.nb_spads_enabled,
            reflectance: r.reflectance,
            cnh_raw: r.cnh_raw,
            motion,
        }
    }

    /// This frame with `resolution` replaced (the device's own count of zones in use).
    pub fn with_resolution(mut self, zones: usize) -> Self {
        self.resolution = zones;
        self
    }
}
