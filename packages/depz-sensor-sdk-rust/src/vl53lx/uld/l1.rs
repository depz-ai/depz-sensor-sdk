//! VL53L1X ULD 3.5.5 (`VL53L1X_api.c` + `VL53L1X_calibration.c`) for VL53L1CX and VL53L1CB.

use std::sync::Arc;

use super::die::{
    collect, die_common, round_half_away, uld_err, Die, DieConfig, DieDriver, INNER_OFFSET_MM,
    INTERMEASUREMENT_MS, OUTER_OFFSET_MM, PHASECAL_CONFIG_TIMEOUT_MACROP,
    RANGE_CONFIG_TIMEOUT_MACROP_A_HI, RANGE_CONFIG_TIMEOUT_MACROP_B_HI,
    RANGE_CONFIG_VALID_PHASE_HIGH, RANGE_CONFIG_VCSEL_PERIOD_A, RANGE_CONFIG_VCSEL_PERIOD_B,
    RANGE_OFFSET_MM, RESULT_OSC_CALIBRATE_VAL, SD_CONFIG_INITIAL_PHASE_SD0, SD_CONFIG_WOI_SD0,
    SYSTEM_INTERRUPT_CONFIG_GPIO, SYSTEM_MODE_START, THRESH_HIGH, THRESH_LOW,
    XTALK_PLANE_OFFSET_KCPS,
};
use crate::devices::{DepzError, Result};
use crate::vl53lx::driver::{Vl53lxDriver, Vl53lxPlatform, Vl53lxRange};

const DISTANCE_SHORT: u8 = 1;
const DISTANCE_LONG: u8 = 2;

static CONFIGURATION: [u8; 91] = [
    0x00, 0x00, 0x00, 0x01, 0x02, 0x00, 0x02, 0x08, 0x00, 0x08, 0x10, 0x01, 0x01, 0x00, 0x00, 0x00,
    0x00, 0xff, 0x00, 0x0F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x0b, 0x00, 0x00, 0x02, 0x0a, 0x21,
    0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x00, 0xc8, 0x00, 0x00, 0x38, 0xff, 0x01, 0x00, 0x08, 0x00,
    0x00, 0x01, 0xcc, 0x0f, 0x01, 0xf1, 0x0d, 0x01, 0x68, 0x00, 0x80, 0x08, 0xb8, 0x00, 0x00, 0x00,
    0x00, 0x0f, 0x89, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x0f, 0x0d, 0x0e, 0x0e, 0x00,
    0x00, 0x02, 0xc7, 0xff, 0x9B, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00,
];

// VL53L1X_SetTimingBudgetInMs(): budget ms → (MACROP_A_HI, MACROP_B_HI), ascending; 15 ms in short only.
/// (timing budget ms, (range config A, range config B)) rows of one distance mode.
type BudgetTable = &'static [(u32, (u16, u16))];

const SHORT_BUDGETS: BudgetTable = &[
    (15, (0x001D, 0x0027)),
    (20, (0x0051, 0x006E)),
    (33, (0x00D6, 0x006E)),
    (50, (0x01AE, 0x01E8)),
    (100, (0x02E1, 0x0388)),
    (200, (0x03E1, 0x0496)),
    (500, (0x0591, 0x05C1)),
];
const LONG_BUDGETS: BudgetTable = &[
    (20, (0x001E, 0x0022)),
    (33, (0x0060, 0x006E)),
    (50, (0x00AD, 0x00C6)),
    (100, (0x01CC, 0x01EA)),
    (200, (0x02D9, 0x02F8)),
    (500, (0x048F, 0x04A4)),
];
// In the Java map's iteration order: short, then long.
const TIMING_BUDGETS: [(u8, BudgetTable); 2] = [
    (DISTANCE_SHORT, SHORT_BUDGETS),
    (DISTANCE_LONG, LONG_BUDGETS),
];

const MODE_NAMES: [&str; 2] = ["long", "short"]; // the blob boots in long

const GROUPS: &[&str] = &[
    "mode",
    "timing",
    "offset",
    "xtalk",
    "thresholds",
    "signal_thresh",
    "sigma_thresh",
    "temp_update",
    "calib_offset",
    "calib_xtalk",
    "roi",
];

fn table(mode: u8) -> &'static [(u32, (u16, u16))] {
    if mode == DISTANCE_SHORT {
        SHORT_BUDGETS
    } else {
        LONG_BUDGETS
    }
}

fn code(name: &str) -> u8 {
    match name {
        "short" => DISTANCE_SHORT,
        "long" => DISTANCE_LONG,
        _ => 0,
    }
}

/// VL53L1X ULD 3.5.5 for VL53L1CX and VL53L1CB: a tabulated timing budget, short / long distance
/// modes, an init that resets the die first. Crosstalk is in kcps, the register's own unit (ST's
/// wrapper converts to cps).
pub struct Vl53l1UldDriver {
    die: Die,
    // The mode last applied: budget_choices answers from it (PHASECAL reads 0 before init).
    applied_mode: &'static str,
}

impl Vl53l1UldDriver {
    /// A driver over `platform`, bound for `product`.
    pub fn new(platform: Arc<dyn Vl53lxPlatform>, product: &str) -> Vl53l1UldDriver {
        let cfg = DieConfig {
            configuration: &CONFIGURATION,
            stop_mode: 0x00,
            signal_at: 15,
            per_spad_k: 25,
        };
        Vl53l1UldDriver {
            die: Die::new(platform, product, cfg),
            applied_mode: "long",
        }
    }

    /// The die underneath (boot state, reset, interrupt polarity).
    pub fn die(&self) -> &Die {
        &self.die
    }

    fn mode_code(&self) -> Result<u8> {
        let temp = self
            .die
            .platform()
            .read_byte(PHASECAL_CONFIG_TIMEOUT_MACROP)?;
        match temp {
            0x14 => Ok(DISTANCE_SHORT),
            0x0A => Ok(DISTANCE_LONG),
            _ => Err(uld_err(format!(
                "PHASECAL_CONFIG__TIMEOUT_MACROP reads 0x{temp:02X}, which is neither distance mode"
            ))),
        }
    }

    /// VL53L1X_SetDistanceThreshold(): a read-modify-write keeping the bits outside 0x6F;
    /// `int_on_no_target` sets bit 6.
    pub fn set_detection_thresholds_ext(
        &mut self,
        distance_low_mm: u16,
        distance_high_mm: u16,
        window: u8,
        int_on_no_target: bool,
    ) -> Result<()> {
        let p = self.die.platform();
        let mut temp = p.read_byte(SYSTEM_INTERRUPT_CONFIG_GPIO)? & !0x6F;
        temp |= window;
        if int_on_no_target {
            temp |= 0x40;
        }
        p.write_byte(SYSTEM_INTERRUPT_CONFIG_GPIO, temp)?;
        p.write_word(THRESH_HIGH, distance_high_mm)?;
        p.write_word(THRESH_LOW, distance_low_mm)
    }
}

impl DieDriver for Vl53l1UldDriver {
    fn die(&self) -> &Die {
        &self.die
    }

    // No plain wait: a sensor left hung by a previous session answers reads with zeros.
    fn init_boot(&mut self) -> Result<()> {
        self.die.reset_device(1000)
    }

    // ST ships this blob with an active-HIGH interrupt; the bridge watches the falling edge.
    fn init_after_config(&mut self) -> Result<()> {
        self.die.set_interrupt_polarity(0)
    }

    fn init_extra(&mut self) -> Result<()> {
        self.applied_mode = MODE_NAMES[0];
        Ok(())
    }
}

impl Vl53lxDriver for Vl53l1UldDriver {
    die_common!("Vl53l1UldDriver");

    fn supports(&self) -> &'static [&'static str] {
        GROUPS
    }

    fn modes(&self) -> Vec<&'static str> {
        MODE_NAMES.to_vec()
    }

    fn budget_ms(&self) -> (u32, u32) {
        (15, 500)
    }

    /// Timed mode only: this part has no free-running mode under the ULD configuration.
    fn start_ranging(&mut self) -> Result<()> {
        self.die.platform().write_byte(SYSTEM_MODE_START, 0x40)
    }

    /// "short" or "long"; the budget is re-applied (the VCSEL periods change what it means).
    /// → the budget in effect after the change, ms: one the new mode does not tabulate moves to
    /// the nearest one it does (contract 12 §5, the one exception to "refuse, don't round"), so
    /// the caller sees it.
    fn set_mode(&mut self, name: &str) -> Result<Option<u32>> {
        let mode = code(name);
        if mode == 0 {
            return Err(DepzError::InvalidArgument(format!(
                "no such mode: {name} (have {})",
                MODE_NAMES.join(", ")
            )));
        }
        let (mut budget_ms, inter_ms) = self.get_range_timing()?;
        self.applied_mode = if mode == DISTANCE_SHORT {
            "short"
        } else {
            "long"
        };
        let p = self.die.platform().clone();
        if mode == DISTANCE_SHORT {
            p.write_byte(PHASECAL_CONFIG_TIMEOUT_MACROP, 0x14)?;
            p.write_byte(RANGE_CONFIG_VCSEL_PERIOD_A, 0x07)?;
            p.write_byte(RANGE_CONFIG_VCSEL_PERIOD_B, 0x05)?;
            p.write_byte(RANGE_CONFIG_VALID_PHASE_HIGH, 0x38)?;
            p.write_word(SD_CONFIG_WOI_SD0, 0x0705)?;
            p.write_word(SD_CONFIG_INITIAL_PHASE_SD0, 0x0606)?;
        } else {
            p.write_byte(PHASECAL_CONFIG_TIMEOUT_MACROP, 0x0A)?;
            p.write_byte(RANGE_CONFIG_VCSEL_PERIOD_A, 0x0F)?;
            p.write_byte(RANGE_CONFIG_VCSEL_PERIOD_B, 0x0D)?;
            p.write_byte(RANGE_CONFIG_VALID_PHASE_HIGH, 0xB8)?;
            p.write_word(SD_CONFIG_WOI_SD0, 0x0F0D)?;
            p.write_word(SD_CONFIG_INITIAL_PHASE_SD0, 0x0E0E)?;
        }
        let t = table(mode);
        if !t.iter().any(|(ms, _)| *ms == budget_ms) {
            // 15 ms exists in short mode only
            let mut best = t[0].0;
            for &(ms, _) in t {
                if (ms as i64 - budget_ms as i64).abs() < (best as i64 - budget_ms as i64).abs() {
                    best = ms;
                }
            }
            budget_ms = best;
        }
        self.set_range_timing(budget_ms, inter_ms)?;
        Ok(Some(budget_ms))
    }

    fn get_mode(&mut self) -> Result<String> {
        Ok(if self.mode_code()? == DISTANCE_SHORT {
            "short"
        } else {
            "long"
        }
        .to_string())
    }

    /// Only the tabulated budgets of the mode exist; a period of 0 is written as the budget itself
    /// (this part has no free-running mode).
    fn set_range_timing(&mut self, timing_budget_ms: u32, inter_measurement_ms: u32) -> Result<()> {
        let mode = self.mode_code()?;
        let t = table(mode);
        let Some(&(_, (macrop_a, macrop_b))) = t.iter().find(|(ms, _)| *ms == timing_budget_ms)
        else {
            let have: Vec<String> = t.iter().map(|(ms, _)| ms.to_string()).collect();
            return Err(uld_err(format!(
                "timing budget {timing_budget_ms} ms is not one the {} mode has - pick one of {} ms",
                if mode == DISTANCE_SHORT { "short" } else { "long" },
                have.join(", ")
            )));
        };
        let p = self.die.platform();
        p.write_word(RANGE_CONFIG_TIMEOUT_MACROP_A_HI, macrop_a)?;
        p.write_word(RANGE_CONFIG_TIMEOUT_MACROP_B_HI, macrop_b)?;
        let inter = if inter_measurement_ms == 0 {
            timing_budget_ms
        } else {
            inter_measurement_ms
        };
        let clock_pll = (p.read_word(RESULT_OSC_CALIBRATE_VAL)? & 0x3FF) as i32;
        let product = clock_pll.wrapping_mul(inter as i32); // a Java int product
        p.write_dword(INTERMEASUREMENT_MS, (product as f64 * 1.075) as i64 as u32)
    }

    fn budget_choices(&self) -> Vec<u32> {
        table(code(self.applied_mode))
            .iter()
            .map(|(ms, _)| *ms)
            .collect()
    }

    fn get_range_timing(&mut self) -> Result<(u32, u32)> {
        let p = self.die.platform();
        let macrop_a = p.read_word(RANGE_CONFIG_TIMEOUT_MACROP_A_HI)?;
        let mut budget_ms = 0;
        for (_, t) in TIMING_BUDGETS {
            for &(ms, (a, _)) in t {
                if a == macrop_a {
                    budget_ms = ms;
                }
            }
        }
        let tmp = p.read_dword(INTERMEASUREMENT_MS)? as f64;
        let clock_pll = (p.read_word(RESULT_OSC_CALIBRATE_VAL)? & 0x3FF) as i32;
        let inter_ms = if clock_pll != 0 {
            (tmp / (clock_pll as f64 * 1.065)) as i32
        } else {
            0
        };
        Ok((budget_ms, inter_ms as u32))
    }

    fn set_detection_thresholds(
        &mut self,
        distance_low_mm: u16,
        distance_high_mm: u16,
        window: u8,
    ) -> Result<()> {
        self.set_detection_thresholds_ext(distance_low_mm, distance_high_mm, window, false)
    }

    fn calibrate_offset(&mut self, target_dist_mm: i32, nb_samples: Option<u32>) -> Result<i32> {
        let n = nb_samples.unwrap_or(50);
        if n < 5 {
            return Err(uld_err("nb_samples must be at least 5"));
        }
        let p = self.die.platform().clone();
        p.write_word(RANGE_OFFSET_MM, 0)?;
        p.write_word(INNER_OFFSET_MM, 0)?;
        p.write_word(OUTER_OFFSET_MM, 0)?;
        let mut sum: i64 = 0;
        collect(self, n, |_, r| sum += r.distance_mm as i64)?;
        let offset_mm = target_dist_mm - sum.div_euclid(n as i64) as i32;
        p.write_word(RANGE_OFFSET_MM, (offset_mm.wrapping_mul(4) & 0xFFFF) as u16)?;
        Ok(offset_mm)
    }

    /// VL53L1X_CalibrateXtalk(); returns kcps, not the C driver's cps.
    fn calibrate_xtalk(&mut self, target_dist_mm: i32, nb_samples: Option<u32>) -> Result<i32> {
        let n = nb_samples.unwrap_or(50);
        if n < 5 {
            return Err(uld_err("nb_samples must be at least 5"));
        }
        let p = self.die.platform().clone();
        p.write_word(XTALK_PLANE_OFFSET_KCPS, 0)?;
        let mut samples: Vec<Vl53lxRange> = Vec::new();
        collect(self, n, |_, r| samples.push(r))?;
        let count = samples.len() as f64;
        let avg_distance = samples.iter().map(|s| s.distance_mm as f64).sum::<f64>() / count;
        let avg_spad_nb = samples.iter().map(|s| s.spads).sum::<f64>() / count;
        let avg_signal = samples.iter().map(|s| s.signal_kcps).sum::<f64>() / count;
        if avg_spad_nb == 0.0 {
            return Err(uld_err("xtalk calibration failed: no SPADs enabled"));
        }
        let cal = (512.0 * (avg_signal * (1.0 - avg_distance / target_dist_mm as f64))
            / avg_spad_nb) as i64;
        let cal = cal.clamp(0, 0xFFFF);
        p.write_word(XTALK_PLANE_OFFSET_KCPS, cal as u16)?;
        Ok(round_half_away(cal as f64 / 512.0) as i32)
    }
}
