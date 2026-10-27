//! VL53L4CD ULD 2.2.3 (`VL53L4CD_api.c` + `VL53L4CD_calibration.c`) and the VL53L3CX ULP 1.0.0, both
//! the plain body of the die.

use std::sync::Arc;

use super::die::{
    self, collect, die_base_timing, die_common, uld_err, Die, DieConfig, DieDriver,
    INNER_OFFSET_MM, OUTER_OFFSET_MM, PHASECAL_CONFIG_TIMEOUT_MACROP, RANGE_OFFSET_MM,
    XTALK_PLANE_OFFSET_KCPS,
};
use crate::devices::Result;
use crate::vl53lx::driver::{Vl53lxDriver, Vl53lxPlatform, Vl53lxRange};

static L4_CONFIGURATION: [u8; 91] = [
    0x00, 0x00, 0x00, 0x11, 0x02, 0x00, 0x02, 0x08, 0x00, 0x08, 0x10, 0x01, 0x01, 0x00, 0x00, 0x00,
    0x00, 0xff, 0x00, 0x0F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x0b, 0x00, 0x00, 0x02, 0x14, 0x21,
    0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x00, 0xc8, 0x00, 0x00, 0x38, 0xff, 0x01, 0x00, 0x08, 0x00,
    0x00, 0x01, 0xcc, 0x07, 0x01, 0xf1, 0x05, 0x00, 0xa0, 0x00, 0x80, 0x08, 0x38, 0x00, 0x00, 0x00,
    0x00, 0x0f, 0x89, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x07, 0x05, 0x06, 0x06, 0x00,
    0x00, 0x02, 0xc7, 0xff, 0x9B, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00,
];

static L3_CONFIGURATION: [u8; 91] = [
    0x00, 0x00, 0x00, 0x11, 0x02, 0x00, 0x02, 0x08, 0x00, 0x08, 0x10, 0x01, 0x01, 0x00, 0x00, 0x00,
    0x00, 0xff, 0x00, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x0b, 0x00, 0x00, 0x02, 0x14, 0x21,
    0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x00, 0xc8, 0x00, 0x00, 0x38, 0xff, 0x01, 0x00, 0x08, 0x00,
    0x00, 0x00, 0x01, 0x07, 0x00, 0x02, 0x05, 0x00, 0xb4, 0x00, 0xbb, 0x08, 0x38, 0x00, 0x00, 0x00,
    0x00, 0x0f, 0x89, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x07, 0x05, 0x06, 0x06, 0x00,
    0x00, 0x02, 0xc7, 0xff, 0x9b, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00,
];

const L4_GROUPS: &[&str] = &[
    "timing",
    "offset",
    "xtalk",
    "thresholds",
    "signal_thresh",
    "sigma_thresh",
    "temp_update",
    "calib_offset",
    "calib_xtalk",
];

const L3_GROUPS: &[&str] = &[
    "timing",
    "thresholds",
    "signal_thresh",
    "sigma_thresh",
    "roi",
];

/// VL53L4CD ULD 2.2.3 for VL53L4CD and VL53L4CX alike.
pub struct Vl53l4UldDriver {
    die: Die,
}

impl Vl53l4UldDriver {
    /// A driver over `platform`, bound for `product`.
    pub fn new(platform: Arc<dyn Vl53lxPlatform>, product: &str) -> Vl53l4UldDriver {
        let cfg = DieConfig {
            configuration: &L4_CONFIGURATION,
            stop_mode: 0x80,
            signal_at: 5,
            per_spad_k: 256,
        };
        Vl53l4UldDriver {
            die: Die::new(platform, product, cfg),
        }
    }

    /// The die underneath (boot state, reset, interrupt polarity).
    pub fn die(&self) -> &Die {
        &self.die
    }
}

impl DieDriver for Vl53l4UldDriver {
    fn die(&self) -> &Die {
        &self.die
    }

    // ALGO__RANGE_IGNORE_THRESHOLD_MCPS, the one write VL53L4CD_SensorInit() adds at the end.
    fn init_extra(&mut self) -> Result<()> {
        self.die.platform().write_word(0x0024, 0x0500)
    }
}

impl Vl53lxDriver for Vl53l4UldDriver {
    die_common!("Vl53l4UldDriver");
    die_base_timing!();

    fn supports(&self) -> &'static [&'static str] {
        L4_GROUPS
    }

    fn calibrate_offset(&mut self, target_dist_mm: i32, nb_samples: Option<u32>) -> Result<i32> {
        let n = nb_samples.unwrap_or(20);
        if !(5..=255).contains(&n) || !(10..=1000).contains(&target_dist_mm) {
            return Err(uld_err("nb_samples must be 5..255, target 10..1000 mm"));
        }
        let p = self.die.platform().clone();
        p.write_word(RANGE_OFFSET_MM, 0)?;
        p.write_word(INNER_OFFSET_MM, 0)?;
        p.write_word(OUTER_OFFSET_MM, 0)?;
        collect(self, 10, |_, _| {})?; // device heat loop
        let mut sum: i64 = 0;
        collect(self, n, |_, r| sum += r.distance_mm as i64)?;
        let offset_mm = target_dist_mm - sum.div_euclid(n as i64) as i32;
        p.write_word(RANGE_OFFSET_MM, (offset_mm.wrapping_mul(4) & 0xFFFF) as u16)?;
        Ok(offset_mm)
    }

    fn calibrate_xtalk(&mut self, target_dist_mm: i32, nb_samples: Option<u32>) -> Result<i32> {
        let n = nb_samples.unwrap_or(20);
        if !(5..=255).contains(&n) || !(10..=5000).contains(&target_dist_mm) {
            return Err(uld_err("nb_samples must be 5..255, target 10..5000 mm"));
        }
        let p = self.die.platform().clone();
        p.write_word(XTALK_PLANE_OFFSET_KCPS, 0)?; // compensation off
        collect(self, 10, |_, _| {})?;
        let mut samples: Vec<Vl53lxRange> = Vec::new();
        collect(self, n, |i, r| {
            if r.status == 0 && i > 0 {
                // invalid measurements and the first frame are dropped
                samples.push(r);
            }
        })?;
        if samples.is_empty() {
            return Err(uld_err("xtalk calibration failed: no valid samples"));
        }
        let count = samples.len() as f64;
        let avg_distance = samples.iter().map(|s| s.distance_mm as f64).sum::<f64>() / count;
        let avg_spad_nb = samples.iter().map(|s| s.spads).sum::<f64>() / count;
        let avg_signal = samples.iter().map(|s| s.signal_kcps).sum::<f64>() / count;
        let tmp = (1.0 - avg_distance / target_dist_mm as f64) * (avg_signal / avg_spad_nb);
        if tmp > 127.0 {
            // 65536 / 512
            return Err(uld_err(format!(
                "xtalk calibration failed: {tmp:.1} kcps > 127"
            )));
        }
        p.write_word(
            XTALK_PLANE_OFFSET_KCPS,
            ((tmp * 512.0) as i32 & 0xFFFF) as u16,
        )?;
        Ok(die::round_half_away(tmp) as i32)
    }
}

/// VL53L3CX ULP 1.0.0: plain single-target ranging on the histogram part.
pub struct Vl53l3UlpDriver {
    die: Die,
}

impl Vl53l3UlpDriver {
    /// A driver over `platform`, bound for `product`.
    pub fn new(platform: Arc<dyn Vl53lxPlatform>, product: &str) -> Vl53l3UlpDriver {
        let cfg = DieConfig {
            configuration: &L3_CONFIGURATION,
            stop_mode: 0x00,
            signal_at: 5,
            per_spad_k: 256,
        };
        Vl53l3UlpDriver {
            die: Die::new(platform, product, cfg),
        }
    }

    /// The die underneath (boot state, reset, interrupt polarity).
    pub fn die(&self) -> &Die {
        &self.die
    }
}

impl DieDriver for Vl53l3UlpDriver {
    fn die(&self) -> &Die {
        &self.die
    }

    fn init_extra(&mut self) -> Result<()> {
        let p = self.die.platform();
        p.write_word(0x0024, 0x0500)?;
        p.write_byte(0x0081, 0x8A)?; // ULP only
        p.write_byte(PHASECAL_CONFIG_TIMEOUT_MACROP, 0x03)
    }
}

impl Vl53lxDriver for Vl53l3UlpDriver {
    die_common!("Vl53l3UlpDriver");
    die_base_timing!();

    fn supports(&self) -> &'static [&'static str] {
        L3_GROUPS
    }
}
