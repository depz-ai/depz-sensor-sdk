//! The contract every driver of the 1D family implements (`uld/base.py`), and ST's platform layer
//! the drivers run on.

use std::collections::BTreeMap;
use std::time::{Duration, Instant};

use crate::devices::{DepzError, Result};

/// ST's platform layer (RdByte / WrWord / WaitMs …) for the 1D family, plus what the bridge owns: bus
/// speed, register-address width and the XSHUT line. Register values are big-endian, as the C drivers
/// assume. The device class implements it over the board; a test can script it.
pub trait Vl53lxPlatform: Send + Sync {
    fn read_multi(&self, addr: u16, size: usize) -> Result<Vec<u8>>;

    fn write_multi(&self, addr: u16, data: &[u8]) -> Result<()>;

    /// The bridge's bus timing (not part of the C platform layer).
    fn set_i2c_speed(&self, khz: u16) -> Result<()>;

    /// How wide a register address the bridge puts on the wire; sticky.
    fn set_addr_width(&self, width: u8) -> Result<()>;

    /// Reset through XSHUT, for a sensor that no longer acknowledges writes.
    fn xshut_reset(&self) -> Result<()>;

    fn sleep_ms(&self, ms: u32) {
        std::thread::sleep(Duration::from_millis(ms as u64));
    }

    fn read_byte(&self, addr: u16) -> Result<u8> {
        Ok(self.read_multi(addr, 1)?[0])
    }

    fn read_word(&self, addr: u16) -> Result<u16> {
        let b = self.read_multi(addr, 2)?;
        Ok(u16::from_be_bytes([b[0], b[1]]))
    }

    fn read_dword(&self, addr: u16) -> Result<u32> {
        let b = self.read_multi(addr, 4)?;
        Ok(u32::from_be_bytes([b[0], b[1], b[2], b[3]]))
    }

    fn write_byte(&self, addr: u16, value: u8) -> Result<()> {
        self.write_multi(addr, &[value])
    }

    fn write_word(&self, addr: u16, value: u16) -> Result<()> {
        self.write_multi(addr, &value.to_be_bytes())
    }

    fn write_dword(&self, addr: u16, value: u32) -> Result<()> {
        self.write_multi(addr, &value.to_be_bytes())
    }
}

/// One return of a multi-target frame. `min_range_mm` / `max_range_mm` are the edges of the target's
/// own pulse, which only the histogram driver knows.
#[derive(Debug, Clone, PartialEq)]
pub struct Vl53lxTarget {
    pub distance_mm: i32,
    pub status: i32,
    pub status_text: String,
    pub signal_kcps: f64,
    pub ambient_kcps: f64,
    pub sigma_mm: f64,
    pub min_range_mm: i32,
    pub max_range_mm: i32,
}

/// A value of [`Vl53lxRange::extra`] / [`Vl53lxDriver::driver_info`]: driver-specific numbers.
#[derive(Debug, Clone, Copy, PartialEq)]
pub enum ExtraValue {
    Int(i64),
    Float(f64),
}

/// One range result as a driver reports it, the same shape for every product of the family. On the
/// histogram driver `targets` holds every return and the top level repeats the first; the light
/// drivers leave it empty. `extra` is driver-specific (keys as the reference SDK spells them) — show
/// it, don't branch on it.
#[derive(Debug, Clone, PartialEq)]
pub struct Vl53lxRange {
    pub distance_mm: i32,
    pub status: i32,
    pub status_text: String,
    pub signal_kcps: f64,
    pub ambient_kcps: f64,
    pub sigma_mm: f64,
    pub spads: f64,
    pub targets: Vec<Vl53lxTarget>,
    pub extra: BTreeMap<String, ExtraValue>,
}

impl Vl53lxRange {
    pub fn valid(&self) -> bool {
        self.status == 0
    }
}

/// The distance-window interrupt: `window` is one of the `WINDOW_*` constants of the die.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct Thresholds {
    pub distance_low_mm: u16,
    pub distance_high_mm: u16,
    pub window: u8,
}

/// A raw histogram frame of the histogram driver (its concrete type is the driver's).
pub type BinData = Box<dyn std::any::Any + Send>;

fn unsupported(product: &str, driver: &str, group: &str) -> DepzError {
    DepzError::Unsupported(format!("{product}/{driver} does not support '{group}'"))
}

/// The contract every driver of the 1D family implements (`uld/base.py`). The methods say what the
/// product is as far as the bridge is concerned and which optional groups it serves
/// ([`Vl53lxDriver::supports`]): mode, timing, offset, xtalk, calib_offset, calib_xtalk,
/// thresholds, signal_thresh, sigma_thresh, roi, temp_update, refspad. A group a driver lacks fails
/// with [`DepzError::Unsupported`]. Methods that touch the sensor take `&mut self`: a driver may keep
/// state (the histogram driver keeps the frame pair).
pub trait Vl53lxDriver: Send {
    /// The product this driver was bound for (one driver serves several).
    fn product(&self) -> &str;

    /// The driver's own name, for messages ("Vl53l1UldDriver", …).
    fn driver_name(&self) -> &'static str;

    /// Bytes of register address on the wire.
    fn addr_width(&self) -> u8 {
        2
    }

    /// The interrupt-release writes handed to START_STREAM, `(addr, value)` each.
    fn clear_steps(&self) -> Vec<(u16, u8)> {
        Vec::new()
    }

    /// The bus ceiling; the driver runs the bus there itself.
    fn max_khz(&self) -> u16 {
        400
    }

    fn supports(&self) -> &'static [&'static str] {
        &[]
    }

    /// The named ranging modes, the first being what init leaves; empty when there are none.
    fn modes(&self) -> Vec<&'static str> {
        Vec::new()
    }

    /// The (min, max) budget a UI should offer, ms; see [`Vl53lxDriver::budget_choices`].
    fn budget_ms(&self) -> (u32, u32) {
        (10, 200)
    }

    /// The frame is a raw histogram (the histogram driver).
    fn histogram(&self) -> bool {
        false
    }

    fn model_id(&mut self) -> Result<u16>;

    /// Reset and configure the sensor; leaves the bus at [`Vl53lxDriver::max_khz`]. Safe to re-run.
    fn sensor_init(&mut self) -> Result<()>;

    fn start_ranging(&mut self) -> Result<()>;

    fn stop_ranging(&mut self) -> Result<()>;

    fn check_for_data_ready(&mut self) -> Result<bool>;

    /// Poll [`Vl53lxDriver::check_for_data_ready`] every 1 ms up to `timeout`.
    fn wait_data_ready(&mut self, timeout: Duration) -> Result<()> {
        let deadline = Instant::now() + timeout;
        while !self.check_for_data_ready()? {
            if Instant::now() > deadline {
                return Err(DepzError::Sensor("timeout waiting for data ready".into()));
            }
            std::thread::sleep(Duration::from_millis(1));
        }
        Ok(())
    }

    fn clear_interrupt(&mut self) -> Result<()>;

    /// (addr, len) — the register block the bridge streams on each INT.
    fn stream_block(&self) -> (u16, u16);

    /// Streamed block bytes → a range.
    fn decode(&mut self, raw: &[u8]) -> Result<Vl53lxRange>;

    /// One polled result: the stream block read and decoded.
    fn read_measurement(&mut self) -> Result<Vl53lxRange>;

    /// Budget of one measurement and the period between two; a period of 0 means continuous.
    fn set_range_timing(&mut self, timing_budget_ms: u32, inter_measurement_ms: u32) -> Result<()>;

    /// (timing budget ms, inter-measurement ms).
    fn get_range_timing(&mut self) -> Result<(u32, u32)>;

    /// The only budgets accepted right now (ascending), or empty when any integer of
    /// [`Vl53lxDriver::budget_ms`] will do.
    fn budget_choices(&self) -> Vec<u32> {
        Vec::new()
    }

    fn set_mode(&mut self, _name: &str) -> Result<()> {
        Err(unsupported(self.product(), self.driver_name(), "mode"))
    }

    fn get_mode(&mut self) -> Result<String> {
        Err(unsupported(self.product(), self.driver_name(), "mode"))
    }

    /// How far the loaded configuration can measure, mm, or `None` when not characterised.
    fn reach_mm(&mut self) -> Result<Option<i32>> {
        Ok(None)
    }

    /// Numbers this driver shows that the contract has no field for.
    fn driver_info(&mut self) -> Result<BTreeMap<String, ExtraValue>> {
        Ok(BTreeMap::new())
    }

    // ── optional groups ──────────────────────────────────────────────────────

    fn get_offset(&mut self) -> Result<i32> {
        Err(unsupported(self.product(), self.driver_name(), "offset"))
    }

    fn set_offset(&mut self, _offset_mm: i32) -> Result<()> {
        Err(unsupported(self.product(), self.driver_name(), "offset"))
    }

    fn get_xtalk(&mut self) -> Result<i32> {
        Err(unsupported(self.product(), self.driver_name(), "xtalk"))
    }

    fn set_xtalk(&mut self, _xtalk_kcps: i32) -> Result<()> {
        Err(unsupported(self.product(), self.driver_name(), "xtalk"))
    }

    /// `nb_samples` `None`: the driver's default.
    fn calibrate_offset(&mut self, _target_dist_mm: i32, _nb_samples: Option<u32>) -> Result<i32> {
        Err(unsupported(self.product(), self.driver_name(), "calib_offset"))
    }

    fn calibrate_xtalk(&mut self, _target_dist_mm: i32, _nb_samples: Option<u32>) -> Result<i32> {
        Err(unsupported(self.product(), self.driver_name(), "calib_xtalk"))
    }

    fn get_detection_thresholds(&mut self) -> Result<Thresholds> {
        Err(unsupported(self.product(), self.driver_name(), "thresholds"))
    }

    fn set_detection_thresholds(&mut self, _distance_low_mm: u16, _distance_high_mm: u16, _window: u8) -> Result<()> {
        Err(unsupported(self.product(), self.driver_name(), "thresholds"))
    }

    fn get_signal_threshold(&mut self) -> Result<u32> {
        Err(unsupported(self.product(), self.driver_name(), "signal_thresh"))
    }

    fn set_signal_threshold(&mut self, _signal_kcps: u32) -> Result<()> {
        Err(unsupported(self.product(), self.driver_name(), "signal_thresh"))
    }

    fn get_sigma_threshold(&mut self) -> Result<u32> {
        Err(unsupported(self.product(), self.driver_name(), "sigma_thresh"))
    }

    fn set_sigma_threshold(&mut self, _sigma_mm: u32) -> Result<()> {
        Err(unsupported(self.product(), self.driver_name(), "sigma_thresh"))
    }

    /// (x, y) SPADs.
    fn get_roi(&mut self) -> Result<(u8, u8)> {
        Err(unsupported(self.product(), self.driver_name(), "roi"))
    }

    fn set_roi(&mut self, _x: u8, _y: u8) -> Result<()> {
        Err(unsupported(self.product(), self.driver_name(), "roi"))
    }

    fn get_roi_center(&mut self) -> Result<u8> {
        Err(unsupported(self.product(), self.driver_name(), "roi"))
    }

    fn set_roi_center(&mut self, _center_spad: u8) -> Result<()> {
        Err(unsupported(self.product(), self.driver_name(), "roi"))
    }

    fn start_temperature_update(&mut self) -> Result<()> {
        Err(unsupported(self.product(), self.driver_name(), "temp_update"))
    }

    /// VL53L0X: re-measure the reference SPADs → (count, is_aperture).
    fn perform_ref_spad_management(&mut self) -> Result<(u32, bool)> {
        Err(unsupported(self.product(), self.driver_name(), "refspad"))
    }

    // ── histogram driver ─────────────────────────────────────────────────────

    /// The raw histogram of a frame (read from the sensor when `raw` is `None`).
    fn bin_data(&mut self, _raw: Option<&[u8]>) -> Result<BinData> {
        Err(DepzError::Unsupported(format!("{} has no histogram", self.driver_name())))
    }

    /// A histogram frame → a range (the host-side ranging of the histogram driver).
    fn to_measurement(&mut self, _bins: &BinData) -> Result<Vl53lxRange> {
        Err(DepzError::Unsupported(format!("{} has no histogram", self.driver_name())))
    }
}
