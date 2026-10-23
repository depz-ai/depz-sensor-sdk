//! The VL53L1 die (`uld/vl53l1_die.py`): register map, the 17-byte result block every part of it
//! shares, and the body of the VL53L4CD ULD that [`super::Vl53l4UldDriver`] and
//! [`super::Vl53l3UlpDriver`] fill in; [`super::Vl53l1UldDriver`] (ST's VL53L1X ULD) overrides where
//! that driver differs. A faithful port: register sequences and integer widths as in the C drivers.
//!
//! Rust has no abstract class: the shared body is [`Die`] (the platform plus a [`DieConfig`] of the
//! per-driver constants), the init / temperature / calibration sequences that call back into the
//! driver's own overrides are generic functions over [`DieDriver`], and `die_common!` /
//! `die_base_timing!` stamp the shared trait methods into each driver's `impl Vl53lxDriver`.

use std::collections::BTreeMap;
use std::sync::Arc;
use std::time::{Duration, Instant};

use crate::devices::{DepzError, Result};
use crate::vl53lx::driver::{ExtraValue, Thresholds, Vl53lxDriver, Vl53lxPlatform, Vl53lxRange};

// The register map, spelled as the full VL53L1 map spells it.
/// `SOFT_RESET`.
pub const SOFT_RESET: u16 = 0x0000;
/// `I2C_SLAVE__DEVICE_ADDRESS`.
pub const I2C_SLAVE_DEVICE_ADDRESS: u16 = 0x0001;
/// `OSC_MEASURED__FAST_OSC__FREQUENCY`.
pub const OSC_FREQUENCY: u16 = 0x0006;
/// `VHV_CONFIG__TIMEOUT_MACROP_LOOP_BOUND`.
pub const VHV_CONFIG_TIMEOUT_MACROP_LOOP_BOUND: u16 = 0x0008;
/// `ALGO__CROSSTALK_COMPENSATION_PLANE_OFFSET_KCPS`.
pub const XTALK_PLANE_OFFSET_KCPS: u16 = 0x0016;
/// `ALGO__CROSSTALK_COMPENSATION_X_PLANE_GRADIENT_KCPS`.
pub const XTALK_X_PLANE_GRADIENT_KCPS: u16 = 0x0018;
/// `ALGO__CROSSTALK_COMPENSATION_Y_PLANE_GRADIENT_KCPS`.
pub const XTALK_Y_PLANE_GRADIENT_KCPS: u16 = 0x001A;
/// `ALGO__PART_TO_PART_RANGE_OFFSET_MM`.
pub const RANGE_OFFSET_MM: u16 = 0x001E;
/// `MM_CONFIG__INNER_OFFSET_MM`.
pub const INNER_OFFSET_MM: u16 = 0x0020;
/// `MM_CONFIG__OUTER_OFFSET_MM`.
pub const OUTER_OFFSET_MM: u16 = 0x0022;
/// `GPIO_HV_MUX__CTRL`.
pub const GPIO_HV_MUX_CTRL: u16 = 0x0030;
/// `GPIO__TIO_HV_STATUS`.
pub const GPIO_TIO_HV_STATUS: u16 = 0x0031;
/// `SYSTEM__INTERRUPT_CONFIG_GPIO`.
pub const SYSTEM_INTERRUPT_CONFIG_GPIO: u16 = 0x0046;
/// `PHASECAL_CONFIG__TIMEOUT_MACROP`.
pub const PHASECAL_CONFIG_TIMEOUT_MACROP: u16 = 0x004B;
/// `RANGE_CONFIG__TIMEOUT_MACROP_A_HI`.
pub const RANGE_CONFIG_TIMEOUT_MACROP_A_HI: u16 = 0x005E;
/// `RANGE_CONFIG__VCSEL_PERIOD_A`.
pub const RANGE_CONFIG_VCSEL_PERIOD_A: u16 = 0x0060;
/// `RANGE_CONFIG__TIMEOUT_MACROP_B_HI`.
pub const RANGE_CONFIG_TIMEOUT_MACROP_B_HI: u16 = 0x0061;
/// `RANGE_CONFIG__VCSEL_PERIOD_B`.
pub const RANGE_CONFIG_VCSEL_PERIOD_B: u16 = 0x0063;
/// `RANGE_CONFIG__SIGMA_THRESH`.
pub const RANGE_CONFIG_SIGMA_THRESH: u16 = 0x0064;
/// `RANGE_CONFIG__MIN_COUNT_RATE_RTN_LIMIT_MCPS`.
pub const MIN_COUNT_RATE_RTN_LIMIT_MCPS: u16 = 0x0066;
/// `RANGE_CONFIG__VALID_PHASE_HIGH`.
pub const RANGE_CONFIG_VALID_PHASE_HIGH: u16 = 0x0069;
/// `SYSTEM__INTERMEASUREMENT_PERIOD`.
pub const INTERMEASUREMENT_MS: u16 = 0x006C;
/// `SYSTEM__THRESH_HIGH`.
pub const THRESH_HIGH: u16 = 0x0072;
/// `SYSTEM__THRESH_LOW`.
pub const THRESH_LOW: u16 = 0x0074;
/// `SD_CONFIG__WOI_SD0`.
pub const SD_CONFIG_WOI_SD0: u16 = 0x0078;
/// `SD_CONFIG__INITIAL_PHASE_SD0`.
pub const SD_CONFIG_INITIAL_PHASE_SD0: u16 = 0x007A;
/// `ROI_CONFIG__USER_ROI_CENTRE_SPAD`.
pub const ROI_CONFIG_USER_ROI_CENTRE_SPAD: u16 = 0x007F;
/// `ROI_CONFIG__USER_ROI_REQUESTED_GLOBAL_XY_SIZE`.
pub const ROI_CONFIG_USER_ROI_XY_SIZE: u16 = 0x0080;
/// `SYSTEM__INTERRUPT_CLEAR`.
pub const SYSTEM_INTERRUPT_CLEAR: u16 = 0x0086;
/// `SYSTEM__MODE_START`.
pub const SYSTEM_MODE_START: u16 = 0x0087;
/// `RESULT__RANGE_STATUS`.
pub const RESULT_RANGE_STATUS: u16 = 0x0089;
/// `RESULT__OSC_CALIBRATE_VAL`.
pub const RESULT_OSC_CALIBRATE_VAL: u16 = 0x00DE;
/// `FIRMWARE__SYSTEM_STATUS`.
pub const FIRMWARE_SYSTEM_STATUS: u16 = 0x00E5;
/// `IDENTIFICATION__MODEL_ID`.
pub const IDENTIFICATION_MODEL_ID: u16 = 0x010F;
/// `ROI_CONFIG__MODE_ROI_CENTRE_SPAD`.
pub const ROI_CONFIG_MODE_ROI_CENTRE_SPAD: u16 = 0x013E;

/// Detection-threshold window: interrupt below `distance_low_mm`.
pub const WINDOW_BELOW: u8 = 0;
/// Detection-threshold window: interrupt above `distance_high_mm`.
pub const WINDOW_ABOVE: u8 = 1;
/// Detection-threshold window: interrupt outside the window.
pub const WINDOW_OUT: u8 = 2;
/// Detection-threshold window: interrupt inside the window.
pub const WINDOW_IN: u8 = 3;

/// The configuration blob spans 0x2D..0x87 on every part.
pub const CONFIG_ADDR: u16 = 0x002D;
/// End (exclusive) of the configuration blob.
pub const CONFIG_END: u16 = 0x0087;
/// Byte 0 of the blob, always: the Fast Mode Plus byte.
pub const CONFIG_FMP_BYTE: u8 = 0x12;

/// Start of the streamed result block (`RESULT__RANGE_STATUS`).
pub const RESULT_BLOCK_ADDR: u16 = RESULT_RANGE_STATUS;
/// Length of the streamed result block, bytes.
pub const RESULT_BLOCK_LEN: usize = 17;

const I2C_KHZ_BOOT: u16 = 400;

// GetResult() raw status → ULD status (status_rtn[24]).
const STATUS_RTN: [i32; 24] = [
    255, 255, 255, 5, 2, 4, 1, 7, 3, 0, 255, 255, 9, 13, 255, 255, 255, 255, 10, 6, 255, 255, 11, 12,
];

/// What differs between the drivers on the die (the overridable constants of the Java
/// `Vl53l1Die`).
#[derive(Debug, Clone, Copy)]
pub struct DieConfig {
    /// The 91-byte blob written at [`CONFIG_ADDR`] (byte 0 is replaced by [`CONFIG_FMP_BYTE`]).
    pub configuration: &'static [u8; 91],
    /// What stop_ranging writes: the L4CD ULD 0x80, the L3CX ULP and the L1X ULD 0x00.
    pub stop_mode: u8,
    /// Where the signal rate comes from in the block: 0x008E (offset 5) on the L4CD ULD, 0x0098 (15)
    /// on the L1X ULD.
    pub signal_at: usize,
    /// Per-SPAD scale: 256 on the L4CD ULD, 25 on the L1X ULD (ST's own difference, kept).
    pub per_spad_k: i64,
}

/// A ULD failure (the Java `Vl53lxException`).
pub(crate) fn uld_err(msg: impl Into<String>) -> DepzError {
    DepzError::Sensor(msg.into())
}

/// What the Java catches as `Vl53lxException | Vl53lxProtocolException`.
pub(crate) fn is_uld_or_bridge_error(e: &DepzError) -> bool {
    matches!(e, DepzError::Sensor(_) | DepzError::Status { .. } | DepzError::Timeout { .. } | DepzError::Protocol(_))
}

/// What the Java catches as `Vl53lxProtocolException` alone (a bridge refusal / no answer).
pub(crate) fn is_bridge_error(e: &DepzError) -> bool {
    matches!(e, DepzError::Status { .. } | DepzError::Timeout { .. } | DepzError::Protocol(_))
}

/// C's `round()`: half away from zero.
pub(crate) fn round_half_away(v: f64) -> i64 {
    (if v < 0.0 { -(-v + 0.5).floor() } else { (v + 0.5).floor() }) as i64
}

fn u16be(b: &[u8], off: usize) -> i64 {
    ((b[off] as i64) << 8) | b[off + 1] as i64
}

fn macro_period_us(osc: i64) -> i64 {
    ((2304 * (0x4000_0000_i64 / osc)) & 0xFFFF_FFFF) >> 6
}

/// How far a target can be before the die fails it with status 4, mm.
pub fn phase_window_mm(fast_osc_frequency: i64, valid_phase_high: i64) -> i64 {
    let pll_period = (1_i64 << 30) / fast_osc_frequency;
    (valid_phase_high * pll_period * (299_704 >> 3)) >> 25
}

/// The ULD status text: `"unknown (N)"` for a code the table lacks.
pub fn status_text(status: i32) -> String {
    let known = u8::try_from(status).ok().map(crate::vl53l4::range_status_name);
    match known {
        Some(name) if name != "unknown" => name.to_string(),
        _ => format!("unknown ({status})"),
    }
}

/// The VL53L1 die one of the three die drivers runs: the platform it is reached through, the product
/// it was bound for and the per-driver constants. Each driver exposes it ([`super::Vl53l4UldDriver::die`]
/// …) for the die-level calls the contract has no method for (boot state, reset, interrupt polarity).
pub struct Die {
    p: Arc<dyn Vl53lxPlatform>,
    product: String,
    cfg: DieConfig,
}

impl Die {
    /// The die of a driver bound for `product`.
    pub(crate) fn new(platform: Arc<dyn Vl53lxPlatform>, product: &str, cfg: DieConfig) -> Die {
        Die { p: platform, product: product.to_string(), cfg }
    }

    /// The platform the die is reached through.
    pub fn platform(&self) -> &Arc<dyn Vl53lxPlatform> {
        &self.p
    }

    /// The product the driver was bound for.
    pub fn product(&self) -> &str {
        &self.product
    }

    /// The per-driver constants.
    pub fn config(&self) -> &DieConfig {
        &self.cfg
    }

    // ── result block ─────────────────────────────────────────────────────────

    /// The 17-byte result block → a range.
    pub fn decode(&self, raw: &[u8]) -> Result<Vl53lxRange> {
        if raw.len() < RESULT_BLOCK_LEN {
            return Err(uld_err(format!("result block too short: {} bytes", raw.len())));
        }
        let mut status = (raw[0] & 0x1F) as i32;
        if (status as usize) < STATUS_RTN.len() {
            status = STATUS_RTN[status as usize];
        }
        let raw_spads = u16be(raw, 3); // 0x008C, 8.8
        let signal_kcps = u16be(raw, self.cfg.signal_at) * 8;
        let ambient_kcps = u16be(raw, 7) * 8; // 0x0090
        let k = self.cfg.per_spad_k;
        let mut extra = BTreeMap::new();
        extra.insert(
            "signal_per_spad_kcps".to_string(),
            ExtraValue::Int(if raw_spads != 0 { signal_kcps * k / raw_spads } else { 0 }),
        );
        extra.insert(
            "ambient_per_spad_kcps".to_string(),
            ExtraValue::Int(if raw_spads != 0 { ambient_kcps * k / raw_spads } else { 0 }),
        );
        extra.insert("stream_count".to_string(), ExtraValue::Int(raw[2] as i64));
        Ok(Vl53lxRange {
            distance_mm: u16be(raw, 13) as i32, // 0x0096
            status,
            status_text: status_text(status),
            signal_kcps: signal_kcps as f64,
            ambient_kcps: ambient_kcps as f64,
            sigma_mm: (u16be(raw, 9) / 4) as f64, // 0x0092, 14.2
            spads: (raw_spads / 256) as f64,
            targets: Vec::new(),
            extra,
        })
    }

    /// One block read instead of the C driver's six register reads; the decoding is identical.
    pub fn get_result(&self) -> Result<Vl53lxRange> {
        self.decode(&self.p.read_multi(RESULT_BLOCK_ADDR, RESULT_BLOCK_LEN)?)
    }

    // ── identity and init ────────────────────────────────────────────────────

    /// `IDENTIFICATION__MODEL_ID`.
    pub fn model_id(&self) -> Result<u16> {
        self.p.read_word(IDENTIFICATION_MODEL_ID)
    }

    /// `FIRMWARE__SYSTEM_STATUS` (0x03 = booted).
    pub fn boot_state(&self) -> Result<u8> {
        self.p.read_byte(FIRMWARE_SYSTEM_STATUS)
    }

    /// Poll the boot state every 1 ms until 0x03, up to `timeout_ms`.
    pub fn wait_boot(&self, timeout_ms: u64) -> Result<()> {
        let deadline = Instant::now() + Duration::from_millis(timeout_ms);
        loop {
            if self.boot_state()? == 0x03 {
                return Ok(());
            }
            if Instant::now() > deadline {
                return Err(uld_err("timeout waiting for FIRMWARE__SYSTEM_STATUS == 0x03"));
            }
            self.p.sleep_ms(1);
        }
    }

    /// The die's soft reset; a part that no longer acknowledges writes is reset through XSHUT.
    pub fn reset_device(&self, timeout_ms: u64) -> Result<()> {
        let soft = (|| {
            self.p.write_byte(SOFT_RESET, 0x00)?;
            self.p.sleep_ms(1);
            self.p.write_byte(SOFT_RESET, 0x01)?;
            self.wait_boot(timeout_ms)
        })();
        match soft {
            Err(e) if is_uld_or_bridge_error(&e) => {
                self.p.xshut_reset()?;
                self.wait_boot(timeout_ms)
            }
            other => other,
        }
    }

    // ── ranging and the interrupt ────────────────────────────────────────────

    /// `SYSTEM__INTERRUPT_CLEAR` ← 1.
    pub fn clear_interrupt(&self) -> Result<()> {
        self.p.write_byte(SYSTEM_INTERRUPT_CLEAR, 0x01)
    }

    /// The L4CD ULD start: continuous when the period register is 0, else autonomous low power.
    pub fn start_ranging(&self) -> Result<()> {
        let mode = if self.p.read_dword(INTERMEASUREMENT_MS)? == 0 { 0x21 } else { 0x40 };
        self.p.write_byte(SYSTEM_MODE_START, mode)
    }

    /// `SYSTEM__MODE_START` ← the driver's stop mode.
    pub fn stop_ranging(&self) -> Result<()> {
        self.p.write_byte(SYSTEM_MODE_START, self.cfg.stop_mode)
    }

    /// 1 = active high, 0 = active low (`GPIO_HV_MUX__CTRL` bit 4, inverted).
    pub fn get_interrupt_polarity(&self) -> Result<u8> {
        Ok(if self.p.read_byte(GPIO_HV_MUX_CTRL)? & 0x10 != 0 { 0 } else { 1 })
    }

    /// Set the interrupt polarity (1 = active high), keeping the other bits.
    pub fn set_interrupt_polarity(&self, polarity: u8) -> Result<()> {
        let temp = self.p.read_byte(GPIO_HV_MUX_CTRL)? & 0xEF;
        let bit = if polarity & 1 == 0 { 1 } else { 0 };
        self.p.write_byte(GPIO_HV_MUX_CTRL, temp | (bit << 4))
    }

    /// The polarity first, as both C drivers read it, then `GPIO__TIO_HV_STATUS` bit 0.
    pub fn check_for_data_ready(&self) -> Result<bool> {
        let int_pol = self.get_interrupt_polarity()?;
        Ok(self.p.read_byte(GPIO_TIO_HV_STATUS)? & 1 == int_pol)
    }

    /// Poll [`Die::check_for_data_ready`] every 1 ms (the platform's sleep) up to `timeout_ms`.
    pub fn wait_data_ready(&self, timeout_ms: u64) -> Result<()> {
        let deadline = Instant::now() + Duration::from_millis(timeout_ms);
        while !self.check_for_data_ready()? {
            if Instant::now() > deadline {
                return Err(uld_err("timeout waiting for data ready"));
            }
            self.p.sleep_ms(1);
        }
        Ok(())
    }

    // ── timing ───────────────────────────────────────────────────────────────

    /// The L4CD ULD timing: any budget of 10..200 ms; `inter_measurement_ms` 0 = continuous, else it
    /// must exceed the budget (autonomous low power).
    pub fn set_range_timing(&self, timing_budget_ms: u32, inter_measurement_ms: u32) -> Result<()> {
        let osc = self.p.read_word(OSC_FREQUENCY)? as i64;
        if osc == 0 {
            return Err(uld_err("osc_frequency reads 0"));
        }
        if !(10..=200).contains(&timing_budget_ms) {
            return Err(uld_err("timing_budget_ms must be 10..200"));
        }

        let mut budget_us = timing_budget_ms as i64 * 1000;
        let macro_period_us = macro_period_us(osc);
        if inter_measurement_ms == 0 {
            // continuous
            self.p.write_dword(INTERMEASUREMENT_MS, 0)?;
            budget_us -= 2500;
        } else if inter_measurement_ms > timing_budget_ms {
            // autonomous low power
            let clock_pll = (self.p.read_word(RESULT_OSC_CALIBRATE_VAL)? & 0x3FF) as f64;
            let factor = 1.055 * inter_measurement_ms as f64 * clock_pll;
            self.p.write_dword(INTERMEASUREMENT_MS, factor as i64 as u32)?;
            budget_us = (budget_us - 4300) / 2;
        } else {
            return Err(uld_err("inter_measurement_ms must be 0 or > timing_budget_ms"));
        }

        budget_us = (budget_us << 12) & 0xFFFF_FFFF;
        for (reg, k) in [(RANGE_CONFIG_TIMEOUT_MACROP_A_HI, 16_i64), (RANGE_CONFIG_TIMEOUT_MACROP_B_HI, 12)] {
            let tmp = ((macro_period_us * k) & 0xFFFF_FFFF) >> 6;
            let mut ls = (budget_us + (tmp >> 1)) / tmp - 1;
            let mut ms: i64 = 0;
            while ls & 0xFFFF_FF00 != 0 {
                ls >>= 1;
                ms += 1;
            }
            self.p.write_word(reg, (((ms << 8) + (ls & 0xFF)) & 0xFFFF) as u16)?;
        }
        Ok(())
    }

    /// The L4CD ULD read-back: (budget ms, inter-measurement ms).
    pub fn get_range_timing(&self) -> Result<(u32, u32)> {
        let tmp = self.p.read_dword(INTERMEASUREMENT_MS)? as i64;
        let clock_pll = (self.p.read_word(RESULT_OSC_CALIBRATE_VAL)? & 0x3FF) as f64;
        let clock_pll = ((1.065 * clock_pll) as i32 & 0xFFFF) as i64;
        let inter_ms = if clock_pll != 0 { (tmp / clock_pll) & 0xFFFF } else { 0 };

        let osc = self.p.read_word(OSC_FREQUENCY)? as i64;
        if osc == 0 {
            return Err(uld_err("osc_frequency reads 0"));
        }
        let macrop_high = self.p.read_word(RANGE_CONFIG_TIMEOUT_MACROP_A_HI)? as i64;

        let mut macro_period_us = macro_period_us(osc);
        let ls = (macrop_high & 0x00FF) << 4;
        let mut ms = (macrop_high & 0xFF00) >> 8;
        ms = (0x04 - (ms - 1) - 1) & 0xFFFF_FFFF;
        macro_period_us = (macro_period_us * 16) & 0xFFFF_FFFF;
        let mut budget = ((((ls + 1) * (macro_period_us >> 6)) - ((macro_period_us >> 6) >> 1)) & 0xFFFF_FFFF) >> 12;
        if ms < 12 {
            budget >>= ms;
        }
        budget = if tmp == 0 { budget + 2500 } else { budget * 2 + 4300 };
        Ok(((budget / 1000) as i32 as u32, inter_ms as i32 as u32))
    }

    /// Read off the loaded configuration: 1388 mm for the 0x38 window, 4563 mm for 0xB8.
    pub fn reach_mm(&self) -> Result<Option<i32>> {
        let osc = self.p.read_word(OSC_FREQUENCY)? as i64;
        if osc == 0 {
            return Err(uld_err("osc_frequency reads 0"));
        }
        let phase = self.p.read_byte(RANGE_CONFIG_VALID_PHASE_HIGH)? as i64;
        Ok(Some(phase_window_mm(osc, phase) as i32))
    }

    // ── thresholds, offset, xtalk, ROI ───────────────────────────────────────

    /// The L4CD ULD thresholds: the window byte written whole.
    pub fn set_detection_thresholds(&self, distance_low_mm: u16, distance_high_mm: u16, window: u8) -> Result<()> {
        self.p.write_byte(SYSTEM_INTERRUPT_CONFIG_GPIO, window)?;
        self.p.write_word(THRESH_HIGH, distance_high_mm)?;
        self.p.write_word(THRESH_LOW, distance_low_mm)
    }

    /// The distance window and its mode.
    pub fn get_detection_thresholds(&self) -> Result<Thresholds> {
        let high = self.p.read_word(THRESH_HIGH)?;
        let low = self.p.read_word(THRESH_LOW)?;
        let window = self.p.read_byte(SYSTEM_INTERRUPT_CONFIG_GPIO)? & 0x07;
        Ok(Thresholds { distance_low_mm: low, distance_high_mm: high, window })
    }

    /// Minimum signal, kcps (the register keeps kcps / 8).
    pub fn set_signal_threshold(&self, signal_kcps: u32) -> Result<()> {
        self.p.write_word(MIN_COUNT_RATE_RTN_LIMIT_MCPS, (signal_kcps >> 3) as u16)
    }

    /// Minimum signal, kcps.
    pub fn get_signal_threshold(&self) -> Result<u32> {
        Ok(((self.p.read_word(MIN_COUNT_RATE_RTN_LIMIT_MCPS)? as u32) << 3) & 0xFFFF)
    }

    /// Maximum sigma, mm (≤ 16383).
    pub fn set_sigma_threshold(&self, sigma_mm: u32) -> Result<()> {
        if sigma_mm > 0xFFFF >> 2 {
            return Err(uld_err("sigma_mm must be <= 16383"));
        }
        self.p.write_word(RANGE_CONFIG_SIGMA_THRESH, (sigma_mm << 2) as u16)
    }

    /// Maximum sigma, mm.
    pub fn get_sigma_threshold(&self) -> Result<u32> {
        Ok((self.p.read_word(RANGE_CONFIG_SIGMA_THRESH)? >> 2) as u32)
    }

    /// The range offset, mm (the inner / outer offsets are zeroed).
    pub fn set_offset(&self, offset_mm: i32) -> Result<()> {
        self.p.write_word(RANGE_OFFSET_MM, (offset_mm.wrapping_mul(4) & 0xFFFF) as u16)?;
        self.p.write_word(INNER_OFFSET_MM, 0)?;
        self.p.write_word(OUTER_OFFSET_MM, 0)
    }

    /// The range offset, mm.
    pub fn get_offset(&self) -> Result<i32> {
        let temp = (((self.p.read_word(RANGE_OFFSET_MM)? as i32) << 3) & 0xFFFF) >> 5;
        Ok(if temp > 1024 { temp - 2048 } else { temp })
    }

    /// The crosstalk plane offset, kcps (the gradients are zeroed).
    pub fn set_xtalk(&self, xtalk_kcps: i32) -> Result<()> {
        self.p.write_word(XTALK_X_PLANE_GRADIENT_KCPS, 0x0000)?;
        self.p.write_word(XTALK_Y_PLANE_GRADIENT_KCPS, 0x0000)?;
        self.p.write_word(XTALK_PLANE_OFFSET_KCPS, ((xtalk_kcps << 9) & 0xFFFF) as u16)
    }

    /// The crosstalk plane offset, kcps.
    pub fn get_xtalk(&self) -> Result<i32> {
        Ok(round_half_away(self.p.read_word(XTALK_PLANE_OFFSET_KCPS)? as f64 / 512.0) as i32)
    }

    /// An X by Y window of SPADs, 4..16 each way.
    pub fn set_roi(&self, x: u8, y: u8) -> Result<()> {
        let mut optical_center = self.p.read_byte(ROI_CONFIG_MODE_ROI_CENTRE_SPAD)?;
        let x = (x as i32).min(16);
        let y = (y as i32).min(16);
        if x > 10 || y > 10 {
            optical_center = 199;
        }
        self.p.write_byte(ROI_CONFIG_USER_ROI_CENTRE_SPAD, optical_center)?;
        self.p.write_byte(ROI_CONFIG_USER_ROI_XY_SIZE, ((y - 1) << 4 | (x - 1)) as u8)
    }

    /// (x, y) SPADs.
    pub fn get_roi(&self) -> Result<(u8, u8)> {
        let temp = self.p.read_byte(ROI_CONFIG_USER_ROI_XY_SIZE)?;
        Ok(((temp & 0x0F) + 1, ((temp & 0xF0) >> 4) + 1))
    }

    /// The ROI centre SPAD.
    pub fn set_roi_center(&self, center_spad: u8) -> Result<()> {
        self.p.write_byte(ROI_CONFIG_USER_ROI_CENTRE_SPAD, center_spad)
    }

    /// The ROI centre SPAD.
    pub fn get_roi_center(&self) -> Result<u8> {
        self.p.read_byte(ROI_CONFIG_USER_ROI_CENTRE_SPAD)
    }
}

/// The overridable steps of the die drivers' shared sequences (the protected hooks of the Java
/// `Vl53l1Die`). The sequences themselves call back through [`Vl53lxDriver`], so a driver's own
/// `start_ranging` / `set_range_timing` are the ones they run.
pub(crate) trait DieDriver: Vl53lxDriver {
    fn die(&self) -> &Die;

    /// How a part gets to a booted die (the L1X ULD resets first).
    fn init_boot(&mut self) -> Result<()> {
        self.die().wait_boot(1000)
    }

    /// Writes right after the configuration blob, before VHV.
    fn init_after_config(&mut self) -> Result<()> {
        Ok(())
    }

    /// Writes at the end of init, before the timing is set.
    fn init_extra(&mut self) -> Result<()> {
        Ok(())
    }
}

/// Configuration at 400 kHz (the only speed an unconfigured sensor is specified for), Fast Mode Plus
/// byte first, then the bus at max_khz, VHV, and 50 ms continuous.
pub(crate) fn sensor_init<D: DieDriver>(d: &mut D) -> Result<()> {
    let p = d.die().platform().clone();
    p.set_addr_width(d.addr_width())?;
    p.set_i2c_speed(I2C_KHZ_BOOT)?;
    d.init_boot()?;
    let mut config = *d.die().config().configuration;
    config[0] = CONFIG_FMP_BYTE;
    // The C driver writes the 91 bytes one at a time; auto-increment makes it one write.
    p.write_multi(CONFIG_ADDR, &config)?;
    if d.max_khz() != I2C_KHZ_BOOT {
        p.set_i2c_speed(d.max_khz())?;
    }
    d.init_after_config()?;

    p.write_byte(SYSTEM_MODE_START, 0x40)?; // start VHV
    d.die().wait_data_ready(1000)?;
    d.die().clear_interrupt()?;
    d.die().stop_ranging()?;
    p.write_byte(VHV_CONFIG_TIMEOUT_MACROP_LOOP_BOUND, 0x09)?;
    p.write_byte(0x000B, 0x00)?;
    d.init_extra()?;

    d.set_range_timing(50, 0)
}

/// Recommended after a >8 °C ambient change.
pub(crate) fn start_temperature_update<D: DieDriver>(d: &mut D) -> Result<()> {
    let p = d.die().platform().clone();
    p.write_byte(VHV_CONFIG_TIMEOUT_MACROP_LOOP_BOUND, 0x81)?;
    p.write_byte(0x000B, 0x92)?;
    d.start_ranging()?;
    d.die().wait_data_ready(1000)?;
    d.die().clear_interrupt()?;
    d.die().stop_ranging()?;
    p.write_byte(VHV_CONFIG_TIMEOUT_MACROP_LOOP_BOUND, 0x09)?;
    p.write_byte(0x000B, 0x00)
}

/// The ranging loop every calibration shares: data ready, result, clear, n times.
pub(crate) fn collect<D: DieDriver>(d: &mut D, nb_samples: u32, mut on_sample: impl FnMut(u32, Vl53lxRange)) -> Result<()> {
    d.start_ranging()?;
    for i in 0..nb_samples {
        d.die().wait_data_ready(5000)?;
        let r = d.die().get_result()?;
        d.die().clear_interrupt()?;
        on_sample(i, r);
    }
    d.die().stop_ranging()
}

/// The trait methods every die driver shares, stamped into `impl Vl53lxDriver for $ty` (field
/// `die`). The groups are the die's: a driver's `supports()` lists what it advertises, as the Java.
macro_rules! die_common {
    ($name:literal) => {
        fn product(&self) -> &str {
            self.die.product()
        }

        fn driver_name(&self) -> &'static str {
            $name
        }

        fn addr_width(&self) -> u8 {
            2
        }

        fn clear_steps(&self) -> Vec<(u16, u8)> {
            vec![($crate::vl53lx::uld::die::SYSTEM_INTERRUPT_CLEAR, 0x01)]
        }

        /// With Fast Mode Plus on the sensor pad.
        fn max_khz(&self) -> u16 {
            1000
        }

        fn model_id(&mut self) -> $crate::devices::Result<u16> {
            self.die.model_id()
        }

        fn sensor_init(&mut self) -> $crate::devices::Result<()> {
            $crate::vl53lx::uld::die::sensor_init(self)
        }

        fn stop_ranging(&mut self) -> $crate::devices::Result<()> {
            self.die.stop_ranging()
        }

        fn check_for_data_ready(&mut self) -> $crate::devices::Result<bool> {
            self.die.check_for_data_ready()
        }

        fn wait_data_ready(&mut self, timeout: std::time::Duration) -> $crate::devices::Result<()> {
            self.die.wait_data_ready(timeout.as_millis() as u64)
        }

        fn clear_interrupt(&mut self) -> $crate::devices::Result<()> {
            self.die.clear_interrupt()
        }

        fn stream_block(&self) -> (u16, u16) {
            ($crate::vl53lx::uld::die::RESULT_BLOCK_ADDR, $crate::vl53lx::uld::die::RESULT_BLOCK_LEN as u16)
        }

        fn decode(&mut self, raw: &[u8]) -> $crate::devices::Result<$crate::vl53lx::driver::Vl53lxRange> {
            self.die.decode(raw)
        }

        fn read_measurement(&mut self) -> $crate::devices::Result<$crate::vl53lx::driver::Vl53lxRange> {
            self.die.get_result()
        }

        fn reach_mm(&mut self) -> $crate::devices::Result<Option<i32>> {
            self.die.reach_mm()
        }

        fn get_offset(&mut self) -> $crate::devices::Result<i32> {
            self.die.get_offset()
        }

        fn set_offset(&mut self, offset_mm: i32) -> $crate::devices::Result<()> {
            self.die.set_offset(offset_mm)
        }

        fn get_xtalk(&mut self) -> $crate::devices::Result<i32> {
            self.die.get_xtalk()
        }

        fn set_xtalk(&mut self, xtalk_kcps: i32) -> $crate::devices::Result<()> {
            self.die.set_xtalk(xtalk_kcps)
        }

        fn get_detection_thresholds(&mut self) -> $crate::devices::Result<$crate::vl53lx::driver::Thresholds> {
            self.die.get_detection_thresholds()
        }

        fn get_signal_threshold(&mut self) -> $crate::devices::Result<u32> {
            self.die.get_signal_threshold()
        }

        fn set_signal_threshold(&mut self, signal_kcps: u32) -> $crate::devices::Result<()> {
            self.die.set_signal_threshold(signal_kcps)
        }

        fn get_sigma_threshold(&mut self) -> $crate::devices::Result<u32> {
            self.die.get_sigma_threshold()
        }

        fn set_sigma_threshold(&mut self, sigma_mm: u32) -> $crate::devices::Result<()> {
            self.die.set_sigma_threshold(sigma_mm)
        }

        fn get_roi(&mut self) -> $crate::devices::Result<(u8, u8)> {
            self.die.get_roi()
        }

        fn set_roi(&mut self, x: u8, y: u8) -> $crate::devices::Result<()> {
            self.die.set_roi(x, y)
        }

        fn get_roi_center(&mut self) -> $crate::devices::Result<u8> {
            self.die.get_roi_center()
        }

        fn set_roi_center(&mut self, center_spad: u8) -> $crate::devices::Result<()> {
            self.die.set_roi_center(center_spad)
        }

        fn start_temperature_update(&mut self) -> $crate::devices::Result<()> {
            $crate::vl53lx::uld::die::start_temperature_update(self)
        }
    };
}

/// The L4CD ULD's start / timing / thresholds, which the L1X ULD replaces.
macro_rules! die_base_timing {
    () => {
        fn start_ranging(&mut self) -> $crate::devices::Result<()> {
            self.die.start_ranging()
        }

        fn set_range_timing(&mut self, timing_budget_ms: u32, inter_measurement_ms: u32) -> $crate::devices::Result<()> {
            self.die.set_range_timing(timing_budget_ms, inter_measurement_ms)
        }

        fn get_range_timing(&mut self) -> $crate::devices::Result<(u32, u32)> {
            self.die.get_range_timing()
        }

        fn set_detection_thresholds(
            &mut self,
            distance_low_mm: u16,
            distance_high_mm: u16,
            window: u8,
        ) -> $crate::devices::Result<()> {
            self.die.set_detection_thresholds(distance_low_mm, distance_high_mm, window)
        }
    };
}

pub(crate) use {die_base_timing, die_common};
