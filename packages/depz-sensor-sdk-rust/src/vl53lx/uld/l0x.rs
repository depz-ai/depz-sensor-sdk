//! The VL53L0X API 1.0.4 ranging path (vl53l0x_api.c / _api_core.c / _api_calibration.c), ported from
//! the reference Python driver: DataInit, StaticInit (reference SPADs from NVM, tuning settings), the
//! timing-budget arithmetic, start / stop, GetRangingMeasurementData with its PAL range status (sigma
//! estimate included), the reference calibration, PerformRefSpadManagement, offset and crosstalk
//! calibration and the four ST ranging profiles. Register sequences, integer widths and the deliberate
//! uint32 overflows are the C driver's. There is no ROI on this die.

use std::collections::BTreeMap;
use std::sync::Arc;
use std::time::{Duration, Instant};

use super::die::{is_bridge_error, uld_err};
use crate::devices::{DepzError, Result};
use crate::vl53lx::driver::{ExtraValue, Vl53lxDriver, Vl53lxPlatform, Vl53lxRange};

// ── registers (vl53l0x_device.h) ─────────────────────────────────────────────
const SYSRANGE_START: u16 = 0x00;
const SYSRANGE_MODE_SINGLESHOT: u8 = 0x00;
const SYSRANGE_MODE_START_STOP: u8 = 0x01;
const SYSRANGE_MODE_BACKTOBACK: u8 = 0x02;
const SYSRANGE_MODE_TIMED: u8 = 0x04;
const SYSTEM_SEQUENCE_CONFIG: u16 = 0x01;
const SYSTEM_INTERMEASUREMENT_PERIOD: u16 = 0x04;
const SYSTEM_RANGE_CONFIG: u16 = 0x09;
const SYSTEM_INTERRUPT_CONFIG_GPIO: u16 = 0x0A;
const SYSTEM_INTERRUPT_CLEAR: u16 = 0x0B;
const RESULT_INTERRUPT_STATUS: u16 = 0x13;
const RESULT_RANGE_STATUS: u16 = 0x14;
const CROSSTALK_COMPENSATION_PEAK_RATE_MCPS: u16 = 0x20;
const ALGO_PART_TO_PART_RANGE_OFFSET_MM: u16 = 0x28;
const ALGO_PHASECAL_CONFIG_TIMEOUT: u16 = 0x30;
const GLOBAL_CONFIG_VCSEL_WIDTH: u16 = 0x32;
const FINAL_RANGE_CONFIG_MIN_COUNT_RATE_RTN_LIMIT: u16 = 0x44;
const MSRC_CONFIG_TIMEOUT_MACROP: u16 = 0x46;
const FINAL_RANGE_CONFIG_VALID_PHASE_LOW: u16 = 0x47;
const FINAL_RANGE_CONFIG_VALID_PHASE_HIGH: u16 = 0x48;
const DYNAMIC_SPAD_NUM_REQUESTED_REF_SPAD: u16 = 0x4E;
const DYNAMIC_SPAD_REF_EN_START_OFFSET: u16 = 0x4F;
const PRE_RANGE_CONFIG_VCSEL_PERIOD: u16 = 0x50;
const PRE_RANGE_CONFIG_TIMEOUT_MACROP_HI: u16 = 0x51;
const PRE_RANGE_CONFIG_VALID_PHASE_LOW: u16 = 0x56;
const PRE_RANGE_CONFIG_VALID_PHASE_HIGH: u16 = 0x57;
const MSRC_CONFIG_CONTROL: u16 = 0x60;
const PRE_RANGE_MIN_COUNT_RATE_RTN_LIMIT: u16 = 0x64;
const FINAL_RANGE_CONFIG_VCSEL_PERIOD: u16 = 0x70;
const FINAL_RANGE_CONFIG_TIMEOUT_MACROP_HI: u16 = 0x71;
const POWER_MANAGEMENT_GO1_POWER_FORCE: u16 = 0x80;
const GPIO_HV_MUX_ACTIVE_HIGH: u16 = 0x84;
const SOFT_RESET_GO2_SOFT_RESET_N: u16 = 0xBF;
const GLOBAL_CONFIG_SPAD_ENABLES_REF_0: u16 = 0xB0;
const GLOBAL_CONFIG_REF_EN_START_SELECT: u16 = 0xB6;
const IDENTIFICATION_MODEL_ID: u16 = 0xC0;
const OSC_CALIBRATE_VAL: u16 = 0xF8;
// Page 1 (0xFF ← 0x01 first): they share their address with a page-0 register.
const ALGO_PHASECAL_LIM: u16 = 0x30;
const RESULT_PEAK_SIGNAL_RATE_REF: u16 = 0xB6;

/// `IDENTIFICATION_MODEL_ID` (0xC0) of a VL53L0X.
pub const MODEL_ID_VL53L0X: u8 = 0xEE;

const DEVICE_MODE_SINGLE_RANGING: i32 = 0;
const DEVICE_MODE_CONTINUOUS_RANGING: i32 = 1;
const DEVICE_MODE_CONTINUOUS_TIMED: i32 = 3;
const GPIO_FUNCTIONALITY_NEW_MEASURE_READY: u8 = 4;

const SEQ_TCC: usize = 0;
const SEQ_DSS: usize = 1;
const SEQ_MSRC: usize = 2;
const SEQ_PRE_RANGE: usize = 3;
const SEQ_FINAL_RANGE: usize = 4;
const SEQ_SET: [u8; 5] = [0x10, 0x28, 0x04, 0x40, 0x80];
const SEQ_CLEAR: [u8; 5] = [0xEF, 0xD7, 0xFB, 0xBF, 0x7F];
const SEQ_TEST: [u8; 5] = [0x10, 0x08, 0x04, 0x40, 0x80];

const VCSEL_PRE_RANGE: i32 = 0;
const VCSEL_FINAL_RANGE: i32 = 1;

const CHECK_SIGMA_FINAL_RANGE: usize = 0;
const CHECK_SIGNAL_RATE_FINAL_RANGE: usize = 1;
const CHECK_SIGNAL_REF_CLIP: usize = 2;
const CHECK_RANGE_IGNORE_THRESHOLD: usize = 3;
const CHECK_SIGNAL_RATE_MSRC: usize = 4;
const CHECK_SIGNAL_RATE_PRE_RANGE: usize = 5;
const CHECK_NUMBER_OF_CHECKS: usize = 6;

const DEFAULT_MAX_LOOP: usize = 2000;
const TARGET_REF_RATE_DEFAULT: i32 = 0x0A00;
const SPEED_OF_LIGHT_IN_AIR: i64 = 2997;
const REF_SPAD_BUFFER_SIZE: usize = 6;

/// One ST example profile: (signal Mcps, sigma mm, budget µs, pre-range PCLK, final PCLK).
struct ModeSetting {
    name: &'static str,
    signal: f64,
    sigma: i64,
    budget_us: i64,
    pre: i32,
    fin: i32,
}

// The four ST examples.
const MODE_SETTINGS: [ModeSetting; 4] = [
    ModeSetting { name: "default", signal: 0.25, sigma: 18, budget_us: 33000, pre: 14, fin: 10 },
    ModeSetting { name: "long-range", signal: 0.10, sigma: 60, budget_us: 33000, pre: 18, fin: 14 },
    ModeSetting { name: "high-speed", signal: 0.25, sigma: 32, budget_us: 30000, pre: 14, fin: 10 },
    ModeSetting { name: "high-accuracy", signal: 0.25, sigma: 18, budget_us: 200000, pre: 14, fin: 10 },
];

// DefaultTuningSettings[] (vl53l0x_tuning.h, v36): {count, address, bytes…} records, 0 ends.
const DEFAULT_TUNING_SETTINGS: &[u8] = &[
    0x01, 0xFF, 0x01, 0x01, 0x00, 0x00, //
    0x01, 0xFF, 0x00, 0x01, 0x09, 0x00, 0x01, 0x10, 0x00, 0x01, 0x11, 0x00, //
    0x01, 0x24, 0x01, 0x01, 0x25, 0xff, 0x01, 0x75, 0x00, //
    0x01, 0xFF, 0x01, 0x01, 0x4e, 0x2c, 0x01, 0x48, 0x00, 0x01, 0x30, 0x20, //
    0x01, 0xFF, 0x00, 0x01, 0x30, 0x09, 0x01, 0x54, 0x00, 0x01, 0x31, 0x04, 0x01, 0x32, 0x03, //
    0x01, 0x40, 0x83, 0x01, 0x46, 0x25, 0x01, 0x60, 0x00, 0x01, 0x27, 0x00, 0x01, 0x50, 0x06, //
    0x01, 0x51, 0x00, 0x01, 0x52, 0x96, 0x01, 0x56, 0x08, 0x01, 0x57, 0x30, 0x01, 0x61, 0x00, //
    0x01, 0x62, 0x00, 0x01, 0x64, 0x00, 0x01, 0x65, 0x00, 0x01, 0x66, 0xa0, //
    0x01, 0xFF, 0x01, 0x01, 0x22, 0x32, 0x01, 0x47, 0x14, 0x01, 0x49, 0xff, 0x01, 0x4a, 0x00, //
    0x01, 0xFF, 0x00, 0x01, 0x7a, 0x0a, 0x01, 0x7b, 0x00, 0x01, 0x78, 0x21, //
    0x01, 0xFF, 0x01, 0x01, 0x23, 0x34, 0x01, 0x42, 0x00, 0x01, 0x44, 0xff, 0x01, 0x45, 0x26, //
    0x01, 0x46, 0x05, 0x01, 0x40, 0x40, 0x01, 0x0E, 0x06, 0x01, 0x20, 0x1a, 0x01, 0x43, 0x40, //
    0x01, 0xFF, 0x00, 0x01, 0x34, 0x03, 0x01, 0x35, 0x44, //
    0x01, 0xFF, 0x01, 0x01, 0x31, 0x04, 0x01, 0x4b, 0x09, 0x01, 0x4c, 0x05, 0x01, 0x4d, 0x04, //
    0x01, 0xFF, 0x00, 0x01, 0x44, 0x00, 0x01, 0x45, 0x20, 0x01, 0x47, 0x08, 0x01, 0x48, 0x28, //
    0x01, 0x67, 0x00, 0x01, 0x70, 0x04, 0x01, 0x71, 0x01, 0x01, 0x72, 0xfe, 0x01, 0x76, 0x00, //
    0x01, 0x77, 0x00, //
    0x01, 0xFF, 0x01, 0x01, 0x0d, 0x01, //
    0x01, 0xFF, 0x00, 0x01, 0x80, 0x01, 0x01, 0x01, 0xF8, //
    0x01, 0xFF, 0x01, 0x01, 0x8e, 0x01, 0x01, 0x00, 0x01, 0x01, 0xFF, 0x00, 0x01, 0x80, 0x00, //
    0x00, 0x00, 0x00,
];

// Dmax lookup table (FixPoint16.16).
const DMAX_LUT_AMB_RATE_MCPS: [i64; 7] =
    [0x00000000, 0x0000B333, 0x00020000, 0x0003CCCC, 0x00074CCC, 0x000A0000, 0x000F0000];
const DMAX_LUT_DMAX_MM: [i64; 7] = [0x04B00000, 0x044C0000, 0x03840000, 0x02EE0000, 0x02260000, 0x01F40000, 0x01900000];

const RESULT_BLOCK_ADDR: u16 = RESULT_RANGE_STATUS;
const RESULT_BLOCK_LEN: usize = 12;

const REF_ARRAY_QUADRANTS: [i32; 4] = [10, 5, 0, 5];

const GROUPS: &[&str] = &["mode", "timing", "offset", "xtalk", "calib_offset", "calib_xtalk", "refspad"];

const START_OVERHEAD_US: i64 = 1910;
const END_OVERHEAD_US: i64 = 960;
const MSRC_OVERHEAD_US: i64 = 660;
const TCC_OVERHEAD_US: i64 = 590;
const DSS_OVERHEAD_US: i64 = 690;
const PRE_RANGE_OVERHEAD_US: i64 = 660;
const FINAL_RANGE_OVERHEAD_US: i64 = 550;

/// The VHV and phase settings the reference calibration found.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct RefCalibration {
    /// VHV setting.
    pub vhv: u8,
    /// Phase setting.
    pub phase: u8,
}

/// The reference SPAD set: how many and whether they are aperture SPADs (1) or not (0).
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct RefSpads {
    /// Number of reference SPADs.
    pub count: i32,
    /// 1 = aperture SPADs, 0 = not.
    pub is_aperture: i32,
}

// VL53L0X_RangingMeasurementData_t, the fields this port fills.
#[derive(Default)]
struct RangingData {
    range_status: i32,
    distance_mm: i32,
    #[allow(dead_code)] // kept as the C driver fills it
    range_fractional_part: i32,
    signal_rate_mcps: i64,
    ambient_rate_mcps: i64,
    effective_spad_rtn_count: i64,
    dmax_mm: i64,
    sigma_mm: i64,
    device_range_status: i32,
}

fn u32m(v: i64) -> i64 {
    v & 0xFFFF_FFFF
}

/// VL53L0X_isqrt(): the bit-by-bit integer square root.
pub fn isqrt(mut num: i64) -> i64 {
    let mut res: i64 = 0;
    let mut bit: i64 = 1 << 30;
    while bit > num {
        bit >>= 2;
    }
    while bit != 0 {
        if num >= res + bit {
            num -= res + bit;
            res = (res >> 1) + bit;
        } else {
            res >>= 1;
        }
        bit >>= 2;
    }
    res
}

fn decode_vcsel_period(reg: u8) -> i32 {
    (reg as i32 + 1) << 1
}

fn encode_vcsel_period(pclks: i32) -> u8 {
    ((pclks >> 1) - 1) as u8
}

fn encode_timeout(mclks: i64) -> u16 {
    if mclks <= 0 {
        return 0;
    }
    let mut ls = mclks - 1;
    let mut ms: i64 = 0;
    while ls & 0xFFFF_FF00 != 0 {
        ls >>= 1;
        ms += 1;
    }
    (((ms << 8) + (ls & 0xFF)) & 0xFFFF) as u16
}

fn decode_timeout(encoded: i64) -> i64 {
    // A Java long shift: the count is taken mod 64.
    (encoded & 0x00FF).wrapping_shl(((encoded & 0xFF00) >> 8) as u32) + 1
}

fn macro_period_ps(pclks: i32) -> i64 {
    2304 * pclks as i64 * 1655
}

fn timeout_mclks(timeout_us: i64, pclks: i32) -> i64 {
    let macro_period_ns = (macro_period_ps(pclks) + 500) / 1000;
    (timeout_us * 1000 + macro_period_ns / 2) / macro_period_ns
}

fn timeout_us(mclks: i64, pclks: i32) -> i64 {
    let macro_period_ns = (macro_period_ps(pclks) + 500) / 1000;
    (mclks * macro_period_ns + 500) / 1000
}

fn is_aperture(spad_index: i32) -> bool {
    REF_ARRAY_QUADRANTS[(spad_index >> 6) as usize] != 0
}

fn next_good_spad(good: &[u8; REF_SPAD_BUFFER_SIZE], curr: i32) -> i32 {
    let start_index = curr / 8;
    let fine_offset = curr % 8;
    let mut coarse = start_index;
    while coarse < REF_SPAD_BUFFER_SIZE as i32 {
        let mut data_byte = good[coarse as usize] as i32;
        let mut fine = 0;
        if coarse == start_index {
            data_byte >>= fine_offset;
            fine = fine_offset;
        }
        while fine < 8 {
            if data_byte & 1 != 0 {
                return coarse * 8 + fine;
            }
            data_byte >>= 1;
            fine += 1;
        }
        coarse += 1;
    }
    -1
}

fn range_status_name(status: i32) -> String {
    match status {
        0 => "Range Valid".to_string(),
        1 => "Sigma Fail".to_string(),
        2 => "Signal Fail".to_string(),
        3 => "Min Range Fail".to_string(),
        4 => "Phase Fail".to_string(),
        5 => "Hardware Fail".to_string(),
        255 => "No Update".to_string(),
        _ => format!("unknown ({status})"),
    }
}

/// The VL53L0X API 1.0.4 ranging path: 8-bit register addresses, no Fast Mode Plus, four ST ranging
/// profiles as modes, offset / crosstalk calibration and reference SPAD management. There is no ROI
/// and there are no detection thresholds on this die.
pub struct Vl53l0xUldDriver {
    p: Arc<dyn Vl53lxPlatform>,
    product: String,
    // PALDevData / DeviceSpecificParameters: the state the C driver keeps beside the sensor.
    read_data_from_device_done: i32,
    xtalk_compensation_rate_mega_cps: i64,
    xtalk_compensation_enable: i32,
    stop_variable: u8,
    sequence_config: u8,
    range_fractional_enable: i32,
    device_mode: i32,
    continuous_mode: Option<i32>, // not in the C driver: resumed after a single shot
    measurement_timing_budget_us: i64,
    pin0_gpio_functionality: u8,
    reference_spad_count: i32,
    reference_spad_type: i32,
    ref_good_spad_map: [u8; REF_SPAD_BUFFER_SIZE],
    #[allow(dead_code)] // kept as the C driver keeps it
    ref_spad_enables: [u8; REF_SPAD_BUFFER_SIZE],
    pre_range_vcsel_pulse_period: i32,
    final_range_vcsel_pulse_period: i32,
    pre_range_timeout_us: i64,
    final_range_timeout_us: i64,
    target_ref_rate: i32,
    limit_checks_enable: [i32; CHECK_NUMBER_OF_CHECKS],
    limit_checks_value: [i64; CHECK_NUMBER_OF_CHECKS],
    mode: String,

    ref_spads_from_nvm: bool,
    module_id: u8,
    revision: u8,
    product_id_string: String,
    part_uid_upper: i64,
    part_uid_lower: i64,
    signal_rate_meas_fixed_400mm: i64,
    part2part_offset_adjustment_nvm_um: i64,
    osc_frequency_mhz: i64,
}

impl Vl53l0xUldDriver {
    /// A driver over `platform`, bound for `product`.
    pub fn new(platform: Arc<dyn Vl53lxPlatform>, product: &str) -> Vl53l0xUldDriver {
        Vl53l0xUldDriver {
            p: platform,
            product: product.to_string(),
            read_data_from_device_done: 0,
            xtalk_compensation_rate_mega_cps: 0,
            xtalk_compensation_enable: 0,
            stop_variable: 0,
            sequence_config: 0,
            range_fractional_enable: 0,
            device_mode: DEVICE_MODE_SINGLE_RANGING,
            continuous_mode: None,
            measurement_timing_budget_us: 0,
            pin0_gpio_functionality: GPIO_FUNCTIONALITY_NEW_MEASURE_READY,
            reference_spad_count: 0,
            reference_spad_type: 0,
            ref_good_spad_map: [0; REF_SPAD_BUFFER_SIZE],
            ref_spad_enables: [0; REF_SPAD_BUFFER_SIZE],
            pre_range_vcsel_pulse_period: 0,
            final_range_vcsel_pulse_period: 0,
            pre_range_timeout_us: 0,
            final_range_timeout_us: 0,
            target_ref_rate: TARGET_REF_RATE_DEFAULT,
            limit_checks_enable: [0; CHECK_NUMBER_OF_CHECKS],
            limit_checks_value: [0; CHECK_NUMBER_OF_CHECKS],
            mode: "default".to_string(),
            ref_spads_from_nvm: true,
            module_id: 0,
            revision: 0,
            product_id_string: String::new(),
            part_uid_upper: 0,
            part_uid_lower: 0,
            signal_rate_meas_fixed_400mm: 0,
            part2part_offset_adjustment_nvm_um: 0,
            osc_frequency_mhz: 618660,
        }
    }

    /// False when the NVM reference-SPAD record was out of range and was measured instead.
    pub fn ref_spads_from_nvm(&self) -> bool {
        self.ref_spads_from_nvm
    }

    /// NVM module id.
    pub fn module_id(&self) -> u8 {
        self.module_id
    }

    /// NVM revision.
    pub fn revision(&self) -> u8 {
        self.revision
    }

    /// NVM product id string.
    pub fn product_id_string(&self) -> &str {
        &self.product_id_string
    }

    /// NVM part UID, upper 32 bits.
    pub fn part_uid_upper(&self) -> i64 {
        self.part_uid_upper
    }

    /// NVM part UID, lower 32 bits.
    pub fn part_uid_lower(&self) -> i64 {
        self.part_uid_lower
    }

    /// Factory signal rate at 400 mm, Mcps 16.16.
    pub fn signal_rate_meas_fixed_400mm(&self) -> i64 {
        self.signal_rate_meas_fixed_400mm
    }

    /// Factory part-to-part offset, µm.
    pub fn part2part_offset_adjustment_nvm_um(&self) -> i64 {
        self.part2part_offset_adjustment_nvm_um
    }

    /// Oscillator frequency, MHz 16.16.
    pub fn osc_frequency_mhz(&self) -> i64 {
        self.osc_frequency_mhz
    }

    /// The reference SPAD set in use.
    pub fn get_reference_spads(&self) -> RefSpads {
        RefSpads { count: self.reference_spad_count, is_aperture: self.reference_spad_type }
    }

    /// No firmware-status register: the model id answering says the die is up.
    pub fn wait_boot(&self, timeout_ms: u64) -> Result<()> {
        let start = Instant::now();
        let limit = Duration::from_millis(timeout_ms);
        loop {
            match self.p.read_byte(IDENTIFICATION_MODEL_ID) {
                Ok(b) if b == MODEL_ID_VL53L0X => return Ok(()),
                Ok(_) => {}
                Err(e) if is_bridge_error(&e) => {} // NACK while the die is still booting
                Err(e) => return Err(e),
            }
            if start.elapsed() > limit {
                return Err(uld_err("timeout waiting for MODEL_ID 0xEE at 0xC0"));
            }
            self.p.sleep_ms(1);
        }
    }

    /// VL53L0X_ResetDevice(): a sensor left configured by a previous session ranges nothing without it.
    pub fn reset_device(&self, timeout_ms: u64) -> Result<()> {
        let start = Instant::now();
        let limit = Duration::from_millis(timeout_ms);
        self.p.write_byte(SOFT_RESET_GO2_SOFT_RESET_N, 0x00)?;
        self.poll_model_id(true, start, limit)?;
        self.p.sleep_ms(1);
        self.p.write_byte(SOFT_RESET_GO2_SOFT_RESET_N, 0x01)?;
        self.poll_model_id(false, start, limit)?;
        self.p.sleep_ms(1);
        Ok(())
    }

    // While the die reboots it NACKs its own address: "not booted yet", not a fault.
    fn poll_model_id(&self, want_zero: bool, start: Instant, limit: Duration) -> Result<()> {
        loop {
            let b = match self.p.read_byte(IDENTIFICATION_MODEL_ID) {
                Ok(b) => Some(b),
                Err(e) if is_bridge_error(&e) => None,
                Err(e) => return Err(e),
            };
            if let Some(b) = b {
                if (b == 0x00) == want_zero {
                    return Ok(());
                }
            }
            if start.elapsed() > limit {
                return Err(uld_err("timeout in the soft reset of the sensor"));
            }
        }
    }

    fn data_init(&mut self) -> Result<()> {
        self.p.write_byte(0x88, 0x00)?; // I2C standard mode
        self.read_data_from_device_done = 0;
        self.osc_frequency_mhz = 618660;
        self.xtalk_compensation_rate_mega_cps = 0;
        self.xtalk_compensation_enable = 0;
        self.device_mode = DEVICE_MODE_SINGLE_RANGING;

        self.p.write_byte(0x80, 0x01)?;
        self.p.write_byte(0xFF, 0x01)?;
        self.p.write_byte(0x00, 0x00)?;
        self.stop_variable = self.p.read_byte(0x91)?;
        self.p.write_byte(0x00, 0x01)?;
        self.p.write_byte(0xFF, 0x00)?;
        self.p.write_byte(0x80, 0x00)?;

        for check in 0..CHECK_NUMBER_OF_CHECKS {
            self.set_limit_check_enable(check, 1)?;
        }
        self.set_limit_check_enable(CHECK_SIGNAL_REF_CLIP, 0)?;
        self.set_limit_check_enable(CHECK_RANGE_IGNORE_THRESHOLD, 0)?;
        self.set_limit_check_enable(CHECK_SIGNAL_RATE_MSRC, 0)?;
        self.set_limit_check_enable(CHECK_SIGNAL_RATE_PRE_RANGE, 0)?;

        self.set_limit_check_value(CHECK_SIGMA_FINAL_RANGE, 18 * 65536)?;
        self.set_limit_check_value(CHECK_SIGNAL_RATE_FINAL_RANGE, 25 * 65536 / 100)?;
        self.set_limit_check_value(CHECK_SIGNAL_REF_CLIP, 35 * 65536)?;
        self.set_limit_check_value(CHECK_RANGE_IGNORE_THRESHOLD, 0)?;

        self.sequence_config = 0xFF;
        self.p.write_byte(SYSTEM_SEQUENCE_CONFIG, 0xFF)
    }

    fn static_init(&mut self) -> Result<()> {
        self.get_info_from_device(1)?;
        let count = self.reference_spad_count;
        let aperture = self.reference_spad_type;
        self.ref_spads_from_nvm = !(aperture > 1 || (aperture == 1 && count > 32) || (aperture == 0 && count > 12));
        if self.ref_spads_from_nvm {
            self.set_reference_spads(count, aperture)?;
        }

        self.load_tuning_settings(DEFAULT_TUNING_SETTINGS)?;
        self.set_gpio_config(GPIO_FUNCTIONALITY_NEW_MEASURE_READY, true)?;

        self.p.write_byte(0xFF, 0x01)?;
        let tempword = self.p.read_word(0x84)?;
        self.p.write_byte(0xFF, 0x00)?;
        self.osc_frequency_mhz = (tempword as i64) << 4; // FixPoint4.12 → 16.16

        self.measurement_timing_budget_us = self.get_measurement_timing_budget()?;
        self.range_fractional_enable = (self.p.read_byte(SYSTEM_RANGE_CONFIG)? & 1) as i32;
        self.sequence_config = self.p.read_byte(SYSTEM_SEQUENCE_CONFIG)?;

        self.set_sequence_step_enable(SEQ_TCC, 0)?;
        self.set_sequence_step_enable(SEQ_MSRC, 0)?;

        self.pre_range_vcsel_pulse_period = self.get_vcsel_pulse_period(VCSEL_PRE_RANGE)?;
        self.final_range_vcsel_pulse_period = self.get_vcsel_pulse_period(VCSEL_FINAL_RANGE)?;
        self.pre_range_timeout_us = self.get_sequence_step_timeout(SEQ_PRE_RANGE)?;
        self.final_range_timeout_us = self.get_sequence_step_timeout(SEQ_FINAL_RANGE)?;
        Ok(())
    }

    // ── reference calibration (VHV / phase) ──────────────────────────────────

    fn perform_single_ref_calibration(&mut self, vhv_init_byte: u8) -> Result<()> {
        self.p.write_byte(SYSRANGE_START, SYSRANGE_MODE_START_STOP | vhv_init_byte)?;
        self.wait_ready(1000)?;
        self.clear_interrupt_mask()?;
        self.p.write_byte(SYSRANGE_START, 0x00)
    }

    fn ref_calibration_io_read(&self, vhv_enable: bool, phase_enable: bool) -> Result<RefCalibration> {
        self.p.write_byte(0xFF, 0x01)?;
        self.p.write_byte(0x00, 0x00)?;
        self.p.write_byte(0xFF, 0x00)?;
        let vhv = if vhv_enable { self.p.read_byte(0xCB)? } else { 0 };
        let phase = if phase_enable { self.p.read_byte(0xEE)? } else { 0 };
        self.p.write_byte(0xFF, 0x01)?;
        self.p.write_byte(0x00, 0x01)?;
        self.p.write_byte(0xFF, 0x00)?;
        Ok(RefCalibration { vhv, phase: phase & 0xEF })
    }

    /// VL53L0X_PerformRefCalibration(): VHV, then phase → (vhv, phase). Not optional in practice.
    pub fn perform_ref_calibration(&mut self) -> Result<RefCalibration> {
        let saved_sequence_config = self.sequence_config;
        self.p.write_byte(SYSTEM_SEQUENCE_CONFIG, 0x01)?; // VHV only
        self.perform_single_ref_calibration(0x40)?;
        let vhv = self.ref_calibration_io_read(true, false)?.vhv;
        self.p.write_byte(SYSTEM_SEQUENCE_CONFIG, 0x02)?; // phase only
        self.perform_single_ref_calibration(0x00)?;
        let phase = self.ref_calibration_io_read(false, true)?.phase;
        self.p.write_byte(SYSTEM_SEQUENCE_CONFIG, saved_sequence_config)?;
        self.sequence_config = saved_sequence_config;
        Ok(RefCalibration { vhv, phase })
    }

    // ── NVM ──────────────────────────────────────────────────────────────────

    fn device_read_strobe(&self) -> Result<()> {
        self.p.write_byte(0x83, 0x00)?;
        let mut done = false;
        for _ in 0..DEFAULT_MAX_LOOP {
            if self.p.read_byte(0x83)? != 0x00 {
                done = true;
                break;
            }
        }
        if !done {
            return Err(uld_err("timeout waiting for the NVM read strobe"));
        }
        self.p.write_byte(0x83, 0x01)
    }

    fn nvm_dword(&self, index: u8) -> Result<i64> {
        self.p.write_byte(0x94, index)?;
        self.device_read_strobe()?;
        Ok(self.p.read_dword(0x90)? as i64)
    }

    /// VL53L0X_get_info_from_device(): the reference SPAD record (option 1), the product
    /// identification (2), the part UID and factory offset (4).
    pub fn get_info_from_device(&mut self, option: i32) -> Result<()> {
        let done = self.read_data_from_device_done;
        if done == 7 {
            return Ok(());
        }
        let p = self.p.clone();
        p.write_byte(0x80, 0x01)?;
        p.write_byte(0xFF, 0x01)?;
        p.write_byte(0x00, 0x00)?;
        p.write_byte(0xFF, 0x06)?;
        let b = p.read_byte(0x83)?;
        p.write_byte(0x83, b | 4)?;
        p.write_byte(0xFF, 0x07)?;
        p.write_byte(0x81, 0x01)?;
        p.sleep_ms(1); // VL53L0X_PollingDelay()
        p.write_byte(0x80, 0x01)?;

        let mut good_spad_map = self.ref_good_spad_map;
        let (mut ref_spad_count, mut ref_spad_type) = (0, 0);
        let mut product_id = String::new();
        let (mut new_module_id, mut new_revision) = (0u8, 0u8);
        let (mut uid_upper, mut uid_lower, mut signal_rate400, mut dist400) = (0i64, 0i64, 0i64, 0i64);

        if option & 1 == 1 && done & 1 == 0 {
            let tmp = self.nvm_dword(0x6B)?;
            ref_spad_count = (tmp >> 8 & 0x7F) as i32;
            ref_spad_type = (tmp >> 15 & 0x01) as i32;
            let tmp = self.nvm_dword(0x24)?;
            good_spad_map[0] = (tmp >> 24 & 0xFF) as u8;
            good_spad_map[1] = (tmp >> 16 & 0xFF) as u8;
            good_spad_map[2] = (tmp >> 8 & 0xFF) as u8;
            good_spad_map[3] = (tmp & 0xFF) as u8;
            let tmp = self.nvm_dword(0x25)?;
            good_spad_map[4] = (tmp >> 24 & 0xFF) as u8;
            good_spad_map[5] = (tmp >> 16 & 0xFF) as u8;
        }

        if option & 2 == 2 && done & 2 == 0 {
            p.write_byte(0x94, 0x02)?;
            self.device_read_strobe()?;
            new_module_id = p.read_byte(0x90)?;
            p.write_byte(0x94, 0x7B)?;
            self.device_read_strobe()?;
            new_revision = p.read_byte(0x90)?;

            let mut chars: Vec<i64> = Vec::new();
            let tmp = self.nvm_dword(0x77)?;
            chars.extend([tmp >> 25 & 0x7F, tmp >> 18 & 0x7F, tmp >> 11 & 0x7F, tmp >> 4 & 0x7F]);
            let b = (tmp & 0x00F) << 3;
            let tmp = self.nvm_dword(0x78)?;
            chars.extend([b + (tmp >> 29 & 0x7F), tmp >> 22 & 0x7F, tmp >> 15 & 0x7F, tmp >> 8 & 0x7F, tmp >> 1 & 0x7F]);
            let b = (tmp & 0x001) << 6;
            let tmp = self.nvm_dword(0x79)?;
            chars.extend([b + (tmp >> 26 & 0x7F), tmp >> 19 & 0x7F, tmp >> 12 & 0x7F, tmp >> 5 & 0x7F]);
            let b = (tmp & 0x01F) << 2;
            let tmp = self.nvm_dword(0x7A)?;
            chars.extend([b + (tmp >> 30 & 0x7F), tmp >> 23 & 0x7F, tmp >> 16 & 0x7F, tmp >> 9 & 0x7F, tmp >> 2 & 0x7F]);
            product_id = chars.iter().map(|&c| (c & 0x7F) as u8 as char).collect();
        }

        if option & 4 == 4 && done & 4 == 0 {
            uid_upper = self.nvm_dword(0x7B)?;
            uid_lower = self.nvm_dword(0x7C)?;
            signal_rate400 = (self.nvm_dword(0x73)? & 0xFF) << 8;
            signal_rate400 |= (self.nvm_dword(0x74)? & 0xFF00_0000) >> 24;
            dist400 = (self.nvm_dword(0x75)? & 0xFF) << 8;
            dist400 |= (self.nvm_dword(0x76)? & 0xFF00_0000) >> 24;
        }

        p.write_byte(0x81, 0x00)?;
        p.write_byte(0xFF, 0x06)?;
        let b = p.read_byte(0x83)?;
        p.write_byte(0x83, b & 0xFB)?;
        p.write_byte(0xFF, 0x01)?;
        p.write_byte(0x00, 0x01)?;
        p.write_byte(0xFF, 0x00)?;
        p.write_byte(0x80, 0x00)?;

        if option & 1 == 1 && done & 1 == 0 {
            self.reference_spad_count = ref_spad_count;
            self.reference_spad_type = ref_spad_type;
            self.ref_good_spad_map = good_spad_map;
        }
        if option & 2 == 2 && done & 2 == 0 {
            self.module_id = new_module_id;
            self.revision = new_revision;
            self.product_id_string = product_id;
        }
        if option & 4 == 4 && done & 4 == 0 {
            self.part_uid_upper = uid_upper;
            self.part_uid_lower = uid_lower;
            self.signal_rate_meas_fixed_400mm = signal_rate400 << 9;
            let mut offset_um = 0;
            if dist400 != 0 {
                let offset1104_mm = u32m(dist400 - (400 << 4));
                offset_um = -((offset1104_mm * 1000) >> 4);
            }
            self.part2part_offset_adjustment_nvm_um = offset_um;
        }
        self.read_data_from_device_done = done | option;
        Ok(())
    }

    // ── reference SPADs ──────────────────────────────────────────────────────

    fn set_reference_spads(&mut self, count: i32, is_aperture_spads: i32) -> Result<()> {
        let start_select: i32 = 0xB4;
        let max_spad_count: i32 = 44;
        self.p.write_byte(0xFF, 0x01)?;
        self.p.write_byte(DYNAMIC_SPAD_REF_EN_START_OFFSET, 0x00)?;
        self.p.write_byte(DYNAMIC_SPAD_NUM_REQUESTED_REF_SPAD, 0x2C)?;
        self.p.write_byte(0xFF, 0x00)?;
        self.p.write_byte(GLOBAL_CONFIG_REF_EN_START_SELECT, start_select as u8)?;

        let mut spad_array = [0u8; REF_SPAD_BUFFER_SIZE];
        let mut current: i32 = 0;
        if is_aperture_spads != 0 {
            while !is_aperture(start_select + current) && current < max_spad_count {
                current += 1;
            }
        }
        for _ in 0..count {
            let next = next_good_spad(&self.ref_good_spad_map, current);
            if next == -1 {
                return Err(uld_err("ran out of good reference SPADs"));
            }
            if is_aperture(start_select + next) != (is_aperture_spads != 0) {
                return Err(uld_err("the good SPAD map leaves the requested quadrant"));
            }
            current = next;
            spad_array[(current / 8) as usize] |= 1 << (current % 8);
            current += 1;
        }
        self.p.write_multi(GLOBAL_CONFIG_SPAD_ENABLES_REF_0, &spad_array)?;
        if self.p.read_multi(GLOBAL_CONFIG_SPAD_ENABLES_REF_0, REF_SPAD_BUFFER_SIZE)? != spad_array {
            return Err(uld_err("reference SPAD map did not read back"));
        }
        self.ref_spad_enables = spad_array;
        self.reference_spad_count = count;
        self.reference_spad_type = is_aperture_spads;
        Ok(())
    }

    // ── tuning settings ──────────────────────────────────────────────────────

    fn load_tuning_settings(&self, buffer: &[u8]) -> Result<()> {
        let mut index = 0;
        while buffer[index] != 0 {
            let writes = buffer[index] as usize;
            index += 1;
            if writes == 0xFF {
                index += 3; // host-side sigma parameters, constants in the 1.0.4 sigma estimate
            } else if writes <= 4 {
                let address = buffer[index] as u16;
                index += 1;
                self.p.write_multi(address, &buffer[index..index + writes])?;
                index += writes;
            } else {
                return Err(uld_err(format!("bad tuning record at offset {index}")));
            }
        }
        Ok(())
    }

    // ── GPIO / interrupt ─────────────────────────────────────────────────────

    fn set_gpio_config(&mut self, functionality: u8, polarity_low: bool) -> Result<()> {
        self.p.write_byte(SYSTEM_INTERRUPT_CONFIG_GPIO, functionality)?;
        let data = if polarity_low { 0x00 } else { 0x10 };
        let current = self.p.read_byte(GPIO_HV_MUX_ACTIVE_HIGH)?;
        self.p.write_byte(GPIO_HV_MUX_ACTIVE_HIGH, (current & 0xEF) | data)?;
        self.pin0_gpio_functionality = functionality;
        self.clear_interrupt_mask()
    }

    /// VL53L0X_ClearInterruptMask(): two writes, then confirm — up to three rounds.
    fn clear_interrupt_mask(&self) -> Result<()> {
        for _ in 0..3 {
            self.p.write_byte(SYSTEM_INTERRUPT_CLEAR, 0x01)?;
            self.p.write_byte(SYSTEM_INTERRUPT_CLEAR, 0x00)?;
            if self.p.read_byte(RESULT_INTERRUPT_STATUS)? & 0x07 == 0x00 {
                return Ok(());
            }
        }
        Err(uld_err("interrupt not cleared after three rounds"))
    }

    fn get_interrupt_mask_status(&self) -> Result<u8> {
        let b = self.p.read_byte(RESULT_INTERRUPT_STATUS)?;
        if b & 0x18 != 0 {
            return Err(uld_err(format!("range error reported in RESULT_INTERRUPT_STATUS 0x{b:02X}")));
        }
        Ok(b & 0x07)
    }

    fn data_ready(&self) -> Result<bool> {
        if self.pin0_gpio_functionality == GPIO_FUNCTIONALITY_NEW_MEASURE_READY {
            Ok(self.get_interrupt_mask_status()? == GPIO_FUNCTIONALITY_NEW_MEASURE_READY)
        } else {
            Ok(self.p.read_byte(RESULT_RANGE_STATUS)? & 0x01 != 0)
        }
    }

    // The base driver's waitDataReady: poll every 1 ms of the platform's sleep.
    fn wait_ready(&self, timeout_ms: u64) -> Result<()> {
        let deadline = Instant::now() + Duration::from_millis(timeout_ms);
        while !self.data_ready()? {
            if Instant::now() > deadline {
                return Err(uld_err("timeout waiting for data ready"));
            }
            self.p.sleep_ms(1);
        }
        Ok(())
    }

    // ── sequence steps ───────────────────────────────────────────────────────

    fn get_sequence_step_enables(&self) -> Result<[bool; 5]> {
        let config = self.p.read_byte(SYSTEM_SEQUENCE_CONFIG)?;
        let mut steps = [false; 5];
        for (i, s) in steps.iter_mut().enumerate() {
            *s = config & SEQ_TEST[i] != 0;
        }
        Ok(steps)
    }

    fn set_sequence_step_enable(&mut self, step: usize, enabled: i32) -> Result<()> {
        let config = self.p.read_byte(SYSTEM_SEQUENCE_CONFIG)?;
        let next = if enabled != 0 { config | SEQ_SET[step] } else { config & SEQ_CLEAR[step] };
        if next == config {
            return Ok(());
        }
        self.p.write_byte(SYSTEM_SEQUENCE_CONFIG, next)?;
        self.sequence_config = next;
        // The budget is spread over the enabled steps, so it is re-applied.
        self.set_measurement_timing_budget(self.measurement_timing_budget_us)
    }

    fn get_vcsel_pulse_period(&self, kind: i32) -> Result<i32> {
        let reg = if kind == VCSEL_PRE_RANGE { PRE_RANGE_CONFIG_VCSEL_PERIOD } else { FINAL_RANGE_CONFIG_VCSEL_PERIOD };
        Ok(decode_vcsel_period(self.p.read_byte(reg)?))
    }

    fn get_sequence_step_timeout(&self, step: usize) -> Result<i64> {
        match step {
            SEQ_TCC | SEQ_DSS | SEQ_MSRC => {
                let vcsel = self.get_vcsel_pulse_period(VCSEL_PRE_RANGE)?;
                let mclks = decode_timeout(self.p.read_byte(MSRC_CONFIG_TIMEOUT_MACROP)? as i64);
                Ok(timeout_us(mclks, vcsel))
            }
            SEQ_PRE_RANGE => {
                let vcsel = self.get_vcsel_pulse_period(VCSEL_PRE_RANGE)?;
                let mclks = decode_timeout(self.p.read_word(PRE_RANGE_CONFIG_TIMEOUT_MACROP_HI)? as i64);
                Ok(timeout_us(mclks, vcsel))
            }
            SEQ_FINAL_RANGE => {
                let steps = self.get_sequence_step_enables()?;
                let mut pre = 0;
                if steps[SEQ_PRE_RANGE] {
                    pre = decode_timeout(self.p.read_word(PRE_RANGE_CONFIG_TIMEOUT_MACROP_HI)? as i64);
                }
                let vcsel = self.get_vcsel_pulse_period(VCSEL_FINAL_RANGE)?;
                let fin = decode_timeout(self.p.read_word(FINAL_RANGE_CONFIG_TIMEOUT_MACROP_HI)? as i64);
                Ok(timeout_us((fin - pre) & 0xFFFF, vcsel))
            }
            _ => Err(uld_err(format!("no timeout for sequence step {step}"))),
        }
    }

    fn set_sequence_step_timeout(&mut self, step: usize, timeout_us: i64) -> Result<()> {
        match step {
            SEQ_TCC | SEQ_DSS | SEQ_MSRC => {
                let vcsel = self.get_vcsel_pulse_period(VCSEL_PRE_RANGE)?;
                let mclks = timeout_mclks(timeout_us, vcsel);
                let encoded = if mclks > 256 { 255 } else { ((mclks - 1) & 0xFF) as u8 };
                self.p.write_byte(MSRC_CONFIG_TIMEOUT_MACROP, encoded)
            }
            SEQ_PRE_RANGE => {
                let vcsel = self.get_vcsel_pulse_period(VCSEL_PRE_RANGE)?;
                self.p.write_word(PRE_RANGE_CONFIG_TIMEOUT_MACROP_HI, encode_timeout(timeout_mclks(timeout_us, vcsel)))?;
                self.pre_range_timeout_us = timeout_us;
                Ok(())
            }
            SEQ_FINAL_RANGE => {
                // The final-range register carries pre-range + final range, in macro periods.
                let steps = self.get_sequence_step_enables()?;
                let mut pre = 0;
                if steps[SEQ_PRE_RANGE] {
                    pre = decode_timeout(self.p.read_word(PRE_RANGE_CONFIG_TIMEOUT_MACROP_HI)? as i64);
                }
                let vcsel = self.get_vcsel_pulse_period(VCSEL_FINAL_RANGE)?;
                self.p.write_word(
                    FINAL_RANGE_CONFIG_TIMEOUT_MACROP_HI,
                    encode_timeout(timeout_mclks(timeout_us, vcsel) + pre),
                )?;
                self.final_range_timeout_us = timeout_us;
                Ok(())
            }
            _ => Err(uld_err(format!("no timeout for sequence step {step}"))),
        }
    }

    // ── timing budget ────────────────────────────────────────────────────────

    fn set_measurement_timing_budget(&mut self, budget_us: i64) -> Result<()> {
        let steps = self.get_sequence_step_enables()?;
        let mut final_budget_us = budget_us - (START_OVERHEAD_US + END_OVERHEAD_US);
        let take = |final_budget_us: &mut i64, sub: i64| -> Result<()> {
            if sub >= *final_budget_us {
                return Err(uld_err(format!(
                    "timing budget {budget_us} us is too small for the enabled sequence steps"
                )));
            }
            *final_budget_us -= sub;
            Ok(())
        };
        if steps[SEQ_TCC] || steps[SEQ_MSRC] || steps[SEQ_DSS] {
            let msrc_us = self.get_sequence_step_timeout(SEQ_MSRC)?; // TCC, MSRC and DSS share one timeout
            if steps[SEQ_TCC] {
                take(&mut final_budget_us, msrc_us + TCC_OVERHEAD_US)?;
            }
            if steps[SEQ_DSS] {
                take(&mut final_budget_us, 2 * (msrc_us + DSS_OVERHEAD_US))?;
            } else if steps[SEQ_MSRC] {
                take(&mut final_budget_us, msrc_us + MSRC_OVERHEAD_US)?;
            }
        }
        if steps[SEQ_PRE_RANGE] {
            let pre = self.get_sequence_step_timeout(SEQ_PRE_RANGE)?;
            take(&mut final_budget_us, pre + PRE_RANGE_OVERHEAD_US)?;
        }
        if steps[SEQ_FINAL_RANGE] {
            final_budget_us -= FINAL_RANGE_OVERHEAD_US; // whatever is left goes to the final range
            self.set_sequence_step_timeout(SEQ_FINAL_RANGE, final_budget_us)?;
            self.measurement_timing_budget_us = budget_us;
        }
        Ok(())
    }

    fn get_measurement_timing_budget(&mut self) -> Result<i64> {
        let steps = self.get_sequence_step_enables()?;
        let mut budget_us = START_OVERHEAD_US + END_OVERHEAD_US;
        if steps[SEQ_TCC] || steps[SEQ_MSRC] || steps[SEQ_DSS] {
            let msrc_us = self.get_sequence_step_timeout(SEQ_MSRC)?;
            if steps[SEQ_TCC] {
                budget_us += msrc_us + TCC_OVERHEAD_US;
            }
            if steps[SEQ_DSS] {
                budget_us += 2 * (msrc_us + DSS_OVERHEAD_US);
            } else if steps[SEQ_MSRC] {
                budget_us += msrc_us + MSRC_OVERHEAD_US;
            }
        }
        if steps[SEQ_PRE_RANGE] {
            budget_us += self.get_sequence_step_timeout(SEQ_PRE_RANGE)? + PRE_RANGE_OVERHEAD_US;
        }
        if steps[SEQ_FINAL_RANGE] {
            budget_us += self.get_sequence_step_timeout(SEQ_FINAL_RANGE)? + FINAL_RANGE_OVERHEAD_US;
        }
        self.measurement_timing_budget_us = budget_us;
        Ok(budget_us)
    }

    fn set_inter_measurement_period(&self, period_ms: i64) -> Result<()> {
        let osc_calibrate_val = self.p.read_word(OSC_CALIBRATE_VAL)? as i64;
        let value = if osc_calibrate_val != 0 { period_ms * osc_calibrate_val } else { period_ms };
        self.p.write_dword(SYSTEM_INTERMEASUREMENT_PERIOD, value as u32)
    }

    fn get_inter_measurement_period(&self) -> Result<i64> {
        let osc_calibrate_val = self.p.read_word(OSC_CALIBRATE_VAL)? as i64;
        let value = self.p.read_dword(SYSTEM_INTERMEASUREMENT_PERIOD)? as i64;
        Ok(if osc_calibrate_val != 0 { value / osc_calibrate_val } else { value })
    }

    // ── ranging ──────────────────────────────────────────────────────────────

    fn set_device_mode(&mut self, new_mode: i32) -> Result<()> {
        if new_mode != DEVICE_MODE_SINGLE_RANGING
            && new_mode != DEVICE_MODE_CONTINUOUS_RANGING
            && new_mode != DEVICE_MODE_CONTINUOUS_TIMED
        {
            return Err(uld_err(format!("device mode {new_mode} is not supported")));
        }
        self.device_mode = new_mode;
        if new_mode != DEVICE_MODE_SINGLE_RANGING {
            self.continuous_mode = Some(new_mode);
        }
        Ok(())
    }

    // The undocumented prologue of every VL53L0X_StartMeasurement(): the stop variable goes back.
    fn arm_stop_variable(&self) -> Result<()> {
        self.p.write_byte(0x80, 0x01)?;
        self.p.write_byte(0xFF, 0x01)?;
        self.p.write_byte(0x00, 0x00)?;
        self.p.write_byte(0x91, self.stop_variable)?;
        self.p.write_byte(0x00, 0x01)?;
        self.p.write_byte(0xFF, 0x00)?;
        self.p.write_byte(0x80, 0x00)
    }

    fn perform_single_ranging_measurement(&mut self) -> Result<RangingData> {
        self.set_device_mode(DEVICE_MODE_SINGLE_RANGING)?;
        self.arm_stop_variable()?;
        self.p.write_byte(SYSRANGE_START, 0x01)?;
        let mut cleared = false;
        for _ in 0..DEFAULT_MAX_LOOP {
            if self.p.read_byte(SYSRANGE_START)? & SYSRANGE_MODE_START_STOP == 0 {
                cleared = true;
                break;
            }
        }
        if !cleared {
            return Err(uld_err("the single-shot start bit never cleared"));
        }
        self.wait_ready(1000)?;
        let data = self.get_result()?;
        self.clear_interrupt_mask()?;
        Ok(data)
    }

    fn get_result(&self) -> Result<RangingData> {
        self.parse_result_block(&self.p.read_multi(RESULT_BLOCK_ADDR, RESULT_BLOCK_LEN)?)
    }

    // ── result decoding ──────────────────────────────────────────────────────

    /// VL53L0X_GetRangingMeasurementData() on the 12 bytes at 0x14: pure arithmetic over the block and
    /// the cached device data, so a streamed frame decodes exactly like a polled one.
    fn parse_result_block(&self, raw: &[u8]) -> Result<RangingData> {
        if raw.len() < RESULT_BLOCK_LEN {
            return Err(uld_err(format!("result block too short: {} bytes", raw.len())));
        }
        let distance = ((raw[10] as i32) << 8) + raw[11] as i32;
        let mut data = RangingData {
            signal_rate_mcps: ((((raw[6] as i64) << 8) + raw[7] as i64) << 9), // 9.7 → 16.16
            ambient_rate_mcps: ((((raw[8] as i64) << 8) + raw[9] as i64) << 9),
            effective_spad_rtn_count: ((raw[2] as i64) << 8) + raw[3] as i64, // 8.8
            device_range_status: raw[0] as i32,
            ..RangingData::default()
        };
        if self.range_fractional_enable != 0 {
            data.distance_mm = distance >> 2;
            data.range_fractional_part = (distance & 0x03) << 6;
        } else {
            data.distance_mm = distance;
        }
        self.get_pal_range_status(&mut data);
        Ok(data)
    }

    fn get_total_xtalk_rate(&self, data: &RangingData) -> i64 {
        if self.xtalk_compensation_enable == 0 {
            return 0;
        }
        let total = data.effective_spad_rtn_count.wrapping_mul(self.xtalk_compensation_rate_mega_cps);
        (total + 0x80) >> 8
    }

    /// VL53L0X_calc_sigma_estimate(), FixPoint16.16 in and out.
    fn calc_sigma_estimate(&self, data: &RangingData) -> i64 {
        const C_PULSE_EFFECTIVE_WIDTH_CENTI_NS: i64 = 800;
        const C_AMBIENT_EFFECTIVE_WIDTH_CENTI_NS: i64 = 600;
        const C_DFLT_FINAL_RANGE_INTEGRATION_TIME_MS: i64 = 0x00190000; // 25 ms
        const C_VCSEL_PULSE_WIDTH_PS: i64 = 4700;
        const C_SIGMA_EST_MAX: i64 = 0x028F87AE;
        const C_SIGMA_EST_RTN_MAX: i64 = 0xF000;
        const C_AMB_TO_SIGNAL_RATIO_MAX: i64 = 0xF000_0000 / C_AMBIENT_EFFECTIVE_WIDTH_CENTI_NS;
        const C_TOF_PER_MM_PS: i64 = 0x0006999A;
        const C_16BIT_ROUNDING_PARAM: i64 = 0x00008000;
        const C_MAX_XTALK_KCPS: i64 = 0x00320000;
        const C_PLL_PERIOD_PS: i64 = 1655;

        let ambient_rate_kcps = (data.ambient_rate_mcps * 1000) >> 16;
        let xtalk_comp_rate_mcps = self.get_total_xtalk_rate(data);
        let total_signal_rate_mcps = data.signal_rate_mcps + xtalk_comp_rate_mcps;
        let peak_signal_rate_kcps = ((total_signal_rate_mcps * 1000) + 0x8000) >> 16;
        let xtalk_comp_rate_kcps = (xtalk_comp_rate_mcps * 1000).min(C_MAX_XTALK_KCPS);

        let final_range_timeout_us_cached = self.final_range_timeout_us;
        let final_range_vcsel_pclks = self.final_range_vcsel_pulse_period;
        let final_range_macro_pclks = timeout_mclks(final_range_timeout_us_cached, final_range_vcsel_pclks);
        let pre_range_timeout_us_cached = self.pre_range_timeout_us;
        let pre_range_macro_pclks = timeout_mclks(pre_range_timeout_us_cached, self.pre_range_vcsel_pulse_period);

        let vcsel_width: i64 = if final_range_vcsel_pclks == 8 { 2 } else { 3 };
        let mut peak_vcsel_duration_us = vcsel_width * 2048 * (pre_range_macro_pclks + final_range_macro_pclks);
        peak_vcsel_duration_us = (peak_vcsel_duration_us + 500) / 1000;
        peak_vcsel_duration_us *= C_PLL_PERIOD_PS;
        peak_vcsel_duration_us = (peak_vcsel_duration_us + 500) / 1000;

        let total_signal_rate2408 = (total_signal_rate_mcps + 0x80) >> 8;
        let mut vcsel_total_events_rtn = ((total_signal_rate2408 * peak_vcsel_duration_us) + 0x80) >> 8;

        if peak_signal_rate_kcps == 0 {
            return C_SIGMA_EST_MAX;
        }
        vcsel_total_events_rtn = vcsel_total_events_rtn.max(1);

        let p1 = C_PULSE_EFFECTIVE_WIDTH_CENTI_NS;
        let mut p2 = (ambient_rate_kcps << 16) / peak_signal_rate_kcps;
        p2 = p2.min(C_AMB_TO_SIGNAL_RATIO_MAX);
        p2 *= C_AMBIENT_EFFECTIVE_WIDTH_CENTI_NS;
        let p3 = 2 * isqrt(vcsel_total_events_rtn * 12);

        let delta_t_ps = data.distance_mm as i64 * C_TOF_PER_MM_PS;
        let mut diff1_mcps = u32m(u32m((peak_signal_rate_kcps << 16) - 2 * xtalk_comp_rate_kcps) + 500) / 1000;
        let diff2_mcps = ((peak_signal_rate_kcps << 16) + 500) / 1000;
        diff1_mcps <<= 8;
        let xtalk_correction = (diff1_mcps / diff2_mcps).abs() << 8;

        let mut pw_mult = delta_t_ps / C_VCSEL_PULSE_WIDTH_PS;
        pw_mult = u32m(pw_mult.wrapping_mul(u32m((1 << 16) - xtalk_correction)));
        pw_mult = (pw_mult + C_16BIT_ROUNDING_PARAM) >> 16;
        pw_mult += 1 << 16;
        pw_mult >>= 1; // squaring 1.xx would leave 32 bits, so the C driver halves it first
        pw_mult = (pw_mult * pw_mult) >> 14;

        let mut sqr1 = ((pw_mult * p1) + 0x8000) >> 16;
        sqr1 *= sqr1;
        let mut sqr2 = (p2 + 0x8000) >> 16;
        sqr2 *= sqr2;

        let sqrt_result_centi_ns = isqrt(sqr1 + sqr2) << 16;
        let mut sigma_est_rtn = ((sqrt_result_centi_ns + 50) / 100) / p3;
        sigma_est_rtn *= SPEED_OF_LIGHT_IN_AIR;
        sigma_est_rtn = (sigma_est_rtn + 5000) / 10000;
        sigma_est_rtn = sigma_est_rtn.min(C_SIGMA_EST_RTN_MAX);

        let final_range_integration_time_ms = (final_range_timeout_us_cached + pre_range_timeout_us_cached + 500) / 1000;
        let mut sigma_est_ref = isqrt(
            (C_DFLT_FINAL_RANGE_INTEGRATION_TIME_MS + final_range_integration_time_ms / 2) / final_range_integration_time_ms,
        ) << 8;
        sigma_est_ref = (sigma_est_ref + 500) / 1000;

        let mut sigma_estimate = 1000 * isqrt(sigma_est_rtn * sigma_est_rtn + sigma_est_ref * sigma_est_ref);
        if peak_signal_rate_kcps < 1 || vcsel_total_events_rtn < 1 || sigma_estimate > C_SIGMA_EST_MAX {
            sigma_estimate = C_SIGMA_EST_MAX;
        }
        sigma_estimate
    }

    /// VL53L0X_calc_dmax(): ambient rate → max range, linear between the table's points.
    fn calc_dmax(amb_rate_meas: i64) -> i64 {
        let amb = &DMAX_LUT_AMB_RATE_MCPS;
        let dmax = &DMAX_LUT_DMAX_MM;
        if amb_rate_meas <= amb[0] {
            return dmax[0] >> 16;
        }
        if amb_rate_meas >= amb[amb.len() - 1] {
            return dmax[dmax.len() - 1] >> 16;
        }
        let mut index1: i32 = -1;
        for (i, a) in amb.iter().enumerate() {
            if amb_rate_meas <= *a {
                index1 = i as i32;
                break;
            }
        }
        let index0 = if index1 != 0 { index1 - 1 } else { 0 };
        if index0 == index1 {
            return dmax[index0 as usize] >> 16;
        }
        let (amb0, amb1) = (amb[index0 as usize], amb[index1 as usize]);
        let (dmax0, dmax1) = (dmax[index0 as usize], dmax[index1 as usize]);
        if amb1 == amb0 {
            return dmax0 >> 16;
        }
        let linear_slope = u32m(dmax0 - dmax1) / ((amb1 - amb0) >> 8);
        (((amb1 - amb_rate_meas) >> 8) * linear_slope + dmax1) >> 16
    }

    /// VL53L0X_get_pal_range_status(): the device status and the enabled limit checks mapped onto the
    /// PAL range status. DataInit leaves the reference clip and the range-ignore check off, so only the
    /// sigma check runs — arithmetic over cached data.
    fn get_pal_range_status(&self, data: &mut RangingData) {
        let internal_status = (data.device_range_status & 0x78) >> 3;
        let none_flag = matches!(internal_status, 0 | 5 | 7 | 12 | 13 | 14 | 15);
        let mut sigma_limit_flag = false;
        if self.limit_checks_enable[CHECK_SIGMA_FINAL_RANGE] != 0 {
            let sigma_estimate = self.calc_sigma_estimate(data);
            data.sigma_mm = sigma_estimate >> 16;
            data.dmax_mm = Self::calc_dmax(data.ambient_rate_mcps);
            let sigma_limit_value = self.limit_checks_value[CHECK_SIGMA_FINAL_RANGE];
            sigma_limit_flag = sigma_limit_value > 0 && sigma_estimate > sigma_limit_value;
        }
        data.range_status = if none_flag {
            255
        } else if matches!(internal_status, 1..=3) {
            5 // hardware fail
        } else if internal_status == 6 || internal_status == 9 {
            4 // phase fail
        } else if internal_status == 8 || internal_status == 10 {
            3 // min range
        } else if internal_status == 4 {
            2 // signal fail
        } else if sigma_limit_flag {
            1 // sigma fail
        } else {
            0
        };
    }

    fn as_range(r: &RangingData) -> Vl53lxRange {
        let mut extra = BTreeMap::new();
        extra.insert("dmax_mm".to_string(), ExtraValue::Int(r.dmax_mm));
        extra.insert("device_range_status".to_string(), ExtraValue::Int(r.device_range_status as i64));
        Vl53lxRange {
            distance_mm: r.distance_mm,
            status: r.range_status,
            status_text: range_status_name(r.range_status),
            signal_kcps: ((r.signal_rate_mcps * 1000) >> 16) as f64,
            ambient_kcps: ((r.ambient_rate_mcps * 1000) >> 16) as f64,
            sigma_mm: r.sigma_mm as f64,
            spads: (r.effective_spad_rtn_count >> 8) as f64,
            targets: Vec::new(),
            extra,
        }
    }

    // ── VCSEL period and the ranging profiles ────────────────────────────────

    fn set_vcsel_pulse_period(&mut self, kind: i32, pclks: i32) -> Result<()> {
        let limit = if kind == VCSEL_PRE_RANGE {
            match pclks {
                12 => Some(0x18),
                14 => Some(0x30),
                16 => Some(0x40),
                18 => Some(0x50),
                _ => None,
            }
        } else {
            match pclks {
                8 => Some(0x10),
                10 => Some(0x28),
                12 => Some(0x38),
                14 => Some(0x48),
                _ => None,
            }
        };
        let limit: u8 = match limit {
            Some(l) if pclks % 2 == 0 => l,
            _ => {
                return Err(uld_err(format!(
                    "VCSEL period {pclks} PCLK is out of range for the {}",
                    if kind == VCSEL_PRE_RANGE { "pre-range" } else { "final range" }
                )))
            }
        };
        if kind == VCSEL_PRE_RANGE {
            self.p.write_byte(PRE_RANGE_CONFIG_VALID_PHASE_HIGH, limit)?;
            self.p.write_byte(PRE_RANGE_CONFIG_VALID_PHASE_LOW, 0x08)?;
        } else {
            // The final range also retunes the VCSEL width and the phase-cal limits.
            let (width, timeout, phasecal_lim) = match pclks {
                8 => (0x02, 0x0C, 0x30),
                10 => (0x03, 0x09, 0x20),
                12 => (0x03, 0x08, 0x20),
                _ => (0x03, 0x07, 0x20),
            };
            self.p.write_byte(FINAL_RANGE_CONFIG_VALID_PHASE_HIGH, limit)?;
            self.p.write_byte(FINAL_RANGE_CONFIG_VALID_PHASE_LOW, 0x08)?;
            self.p.write_byte(GLOBAL_CONFIG_VCSEL_WIDTH, width)?;
            self.p.write_byte(ALGO_PHASECAL_CONFIG_TIMEOUT, timeout)?;
            self.p.write_byte(0xFF, 0x01)?;
            self.p.write_byte(ALGO_PHASECAL_LIM, phasecal_lim)?;
            self.p.write_byte(0xFF, 0x00)?;
        }
        let reg = encode_vcsel_period(pclks);
        if kind == VCSEL_PRE_RANGE {
            let pre_timeout = self.get_sequence_step_timeout(SEQ_PRE_RANGE)?;
            let msrc_timeout = self.get_sequence_step_timeout(SEQ_MSRC)?;
            self.p.write_byte(PRE_RANGE_CONFIG_VCSEL_PERIOD, reg)?;
            self.set_sequence_step_timeout(SEQ_PRE_RANGE, pre_timeout)?;
            self.set_sequence_step_timeout(SEQ_MSRC, msrc_timeout)?;
            self.pre_range_vcsel_pulse_period = pclks;
        } else {
            let final_timeout = self.get_sequence_step_timeout(SEQ_FINAL_RANGE)?;
            self.p.write_byte(FINAL_RANGE_CONFIG_VCSEL_PERIOD, reg)?;
            self.set_sequence_step_timeout(SEQ_FINAL_RANGE, final_timeout)?;
            self.final_range_vcsel_pulse_period = pclks;
        }
        self.set_measurement_timing_budget(self.measurement_timing_budget_us)?;
        self.perform_phase_calibration()
    }

    fn perform_phase_calibration(&mut self) -> Result<()> {
        let saved_sequence_config = self.sequence_config;
        self.p.write_byte(SYSTEM_SEQUENCE_CONFIG, 0x02)?;
        self.perform_single_ref_calibration(0x00)?;
        self.p.write_byte(SYSTEM_SEQUENCE_CONFIG, saved_sequence_config)?;
        self.sequence_config = saved_sequence_config;
        Ok(())
    }

    // ── offset ───────────────────────────────────────────────────────────────

    /// The register is 10.2 format in mm: steps of 250 µm.
    pub fn set_offset_um(&self, offset_um: i64) -> Result<()> {
        let offset_um = offset_um.clamp(-512000, 511000);
        let steps = offset_um.abs() / 250;
        let encoded = if offset_um >= 0 { steps } else { 4096 - steps };
        self.p.write_word(ALGO_PART_TO_PART_RANGE_OFFSET_MM, (encoded & 0xFFFF) as u16)
    }

    /// The part-to-part offset, µm.
    pub fn get_offset_um(&self) -> Result<i64> {
        let reg = (self.p.read_word(ALGO_PART_TO_PART_RANGE_OFFSET_MM)? & 0x0FFF) as i64;
        Ok(if reg > 2047 { (reg - 4096) * 250 } else { reg * 250 })
    }

    /// VL53L0X_perform_offset_calibration() → µm (the contract's default is 50 samples). TCC and the
    /// range-ignore threshold are off for the run.
    pub fn perform_offset_calibration(&mut self, cal_distance_mm: i32, nb_samples: i32) -> Result<i64> {
        if cal_distance_mm <= 0 {
            return Err(uld_err("the calibration distance must be positive"));
        }
        self.set_offset_um(0)?;
        let tcc_was_on = self.get_sequence_step_enables()?[SEQ_TCC];
        self.set_sequence_step_enable(SEQ_TCC, 0)?;
        self.set_limit_check_enable(CHECK_RANGE_IGNORE_THRESHOLD, 0)?;
        let mut sum: i64 = 0;
        let mut count: i64 = 0;
        for _ in 0..nb_samples {
            let data = self.perform_single_ranging_measurement()?;
            if data.range_status == 0 {
                sum = (sum + data.distance_mm as i64) & 0xFFFF;
                count += 1;
            }
        }
        if count == 0 {
            return Err(uld_err(
                "offset calibration got no valid measurement - check that a target is in front of the sensor",
            ));
        }
        let mean_mm = (2 * sum + count) / (2 * count); // round half up
        if mean_mm == 0 {
            return Err(uld_err(
                "the mean range came out 0 mm - the target is closer than the part-to-part offset; \
                 move it out to 100..400 mm and calibrate again",
            ));
        }
        let offset_um = (cal_distance_mm as i64 - mean_mm) * 1000;
        self.set_offset_um(offset_um)?;
        if tcc_was_on {
            self.set_sequence_step_enable(SEQ_TCC, 1)?;
        }
        Ok(offset_um)
    }

    // ── crosstalk ────────────────────────────────────────────────────────────

    fn set_xtalk_enable(&mut self, enable: i32) -> Result<()> {
        let rate = if enable != 0 { self.xtalk_compensation_rate_mega_cps } else { 0 };
        self.p.write_word(CROSSTALK_COMPENSATION_PEAK_RATE_MCPS, ((rate >> 3) & 0xFFFF) as u16)?; // 16.16 → 3.13
        self.xtalk_compensation_enable = if enable != 0 { 1 } else { 0 };
        Ok(())
    }

    fn set_xtalk_rate_mcps(&mut self, rate_mcps: i64) -> Result<()> {
        if self.xtalk_compensation_enable != 0 {
            self.p.write_word(CROSSTALK_COMPENSATION_PEAK_RATE_MCPS, ((rate_mcps >> 3) & 0xFFFF) as u16)?;
        }
        self.xtalk_compensation_rate_mega_cps = rate_mcps;
        Ok(())
    }

    fn get_xtalk_rate_mcps(&self) -> Result<i64> {
        Ok((self.p.read_word(CROSSTALK_COMPENSATION_PEAK_RATE_MCPS)? as i64) << 3)
    }

    /// VL53L0X_perform_xtalk_calibration() → the rate, Mcps 16.16 (the contract's default is 50
    /// samples).
    pub fn perform_xtalk_calibration(&mut self, cal_distance_mm: i32, nb_samples: i32) -> Result<i64> {
        if cal_distance_mm <= 0 {
            return Err(uld_err("the calibration distance must be positive"));
        }
        self.set_xtalk_enable(0)?;
        self.set_limit_check_enable(CHECK_RANGE_IGNORE_THRESHOLD, 0)?;
        let (mut sum_ranging, mut sum_signal, mut sum_spads) = (0i64, 0i64, 0i64);
        let mut count: i64 = 0;
        for _ in 0..nb_samples {
            let data = self.perform_single_ranging_measurement()?;
            if data.range_status == 0 {
                // The C sums are uint16 / uint32; kept that way on purpose.
                sum_ranging = (sum_ranging + data.distance_mm as i64) & 0xFFFF;
                sum_signal = u32m(sum_signal + data.signal_rate_mcps);
                sum_spads = (sum_spads + data.effective_spad_rtn_count / 256) & 0xFFFF;
                count += 1;
            }
        }
        if count == 0 {
            return Err(uld_err(
                "crosstalk calibration got no valid measurement - check that a target is in front of the sensor",
            ));
        }
        let mean_signal = sum_signal / count;
        let mean_range = u32m(sum_ranging << 16) / count;
        let mean_spads = u32m(sum_spads << 16) / count;
        let mean_spads_int = (mean_spads + 0x8000) >> 16;
        let cal_distance = (cal_distance_mm as i64) << 16;
        let rate = if mean_spads_int == 0 || mean_range >= cal_distance {
            0
        } else {
            let mut per_spad = mean_signal / mean_spads_int;
            per_spad = u32m(per_spad.wrapping_mul((1 << 16) - mean_range / cal_distance_mm as i64));
            (per_spad + 0x8000) >> 16
        };
        self.set_xtalk_enable(1)?;
        self.set_xtalk_rate_mcps(rate)?;
        Ok(rate)
    }

    // ── reference SPAD management ────────────────────────────────────────────

    fn set_ref_spad_map(&self, spad_array: &[u8; REF_SPAD_BUFFER_SIZE]) -> Result<()> {
        self.p.write_multi(GLOBAL_CONFIG_SPAD_ENABLES_REF_0, spad_array)
    }

    fn get_ref_spad_map(&self) -> Result<Vec<u8>> {
        self.p.read_multi(GLOBAL_CONFIG_SPAD_ENABLES_REF_0, REF_SPAD_BUFFER_SIZE)
    }

    fn enable_ref_spads(
        &self,
        aperture_spads: i32,
        spad_array: &mut [u8; REF_SPAD_BUFFER_SIZE],
        start: i32,
        offset: i32,
        spad_count: i32,
    ) -> Result<i32> {
        let mut current = offset;
        for _ in 0..spad_count {
            let next = next_good_spad(&self.ref_good_spad_map, current);
            if next == -1 {
                return Err(uld_err("ran out of good reference SPADs"));
            }
            if is_aperture(start + next) != (aperture_spads != 0) {
                return Err(uld_err("the good SPAD map leaves the requested quadrant"));
            }
            current = next;
            spad_array[(current / 8) as usize] |= 1 << (current % 8);
            current += 1;
        }
        self.set_ref_spad_map(spad_array)?;
        if self.get_ref_spad_map()? != spad_array[..] {
            return Err(uld_err("reference SPAD map did not read back"));
        }
        Ok(current)
    }

    fn perform_ref_signal_measurement(&mut self) -> Result<i32> {
        let saved_sequence_config = self.sequence_config;
        self.p.write_byte(SYSTEM_SEQUENCE_CONFIG, 0xC0)?;
        self.perform_single_ranging_measurement()?;
        self.p.write_byte(0xFF, 0x01)?;
        let peak = self.p.read_word(RESULT_PEAK_SIGNAL_RATE_REF)? as i32;
        self.p.write_byte(0xFF, 0x00)?;
        self.p.write_byte(SYSTEM_SEQUENCE_CONFIG, saved_sequence_config)?;
        self.sequence_config = saved_sequence_config;
        Ok(peak)
    }

    // VL53L0X_perform_ref_spad_management(): the minimum non-aperture set, then one SPAD at a time until
    // the reference rate lands nearest 20 Mcps; aperture SPADs when even the minimum overshoots.
    fn ref_spad_management(&mut self) -> Result<RefSpads> {
        let start_select: i32 = 0xB4;
        let minimum_spad_count: i32 = 3;
        let max_spad_count: i32 = 44;
        let mut spad_array = [0u8; REF_SPAD_BUFFER_SIZE];
        self.p.write_byte(0xFF, 0x01)?;
        self.p.write_byte(DYNAMIC_SPAD_REF_EN_START_OFFSET, 0x00)?;
        self.p.write_byte(DYNAMIC_SPAD_NUM_REQUESTED_REF_SPAD, 0x2C)?;
        self.p.write_byte(0xFF, 0x00)?;
        self.p.write_byte(GLOBAL_CONFIG_REF_EN_START_SELECT, start_select as u8)?;
        self.p.write_byte(POWER_MANAGEMENT_GO1_POWER_FORCE, 0x00)?;
        self.perform_ref_calibration()?;

        let mut need_apt: i32 = 0;
        let mut current = self.enable_ref_spads(need_apt, &mut spad_array, start_select, 0, minimum_spad_count)?;
        let mut peak = self.perform_ref_signal_measurement()?;
        if peak > self.target_ref_rate {
            spad_array = [0u8; REF_SPAD_BUFFER_SIZE];
            while !is_aperture(start_select + current) && current < max_spad_count {
                current += 1;
            }
            need_apt = 1;
            current = self.enable_ref_spads(need_apt, &mut spad_array, start_select, current, minimum_spad_count)?;
            peak = self.perform_ref_signal_measurement()?;
            if peak > self.target_ref_rate {
                self.store_ref_spads(minimum_spad_count, 1, &spad_array);
                return Ok(RefSpads { count: minimum_spad_count, is_aperture: 1 });
            }
        }
        let mut ref_spad_count = minimum_spad_count;
        let mut last_spad_array = spad_array;
        let mut last_diff = (peak - self.target_ref_rate).abs();
        while peak < self.target_ref_rate {
            let next = next_good_spad(&self.ref_good_spad_map, current);
            if next == -1 {
                return Err(uld_err("ran out of good reference SPADs"));
            }
            if is_aperture(start_select + next) != (need_apt != 0) {
                break; // the quadrant is exhausted
            }
            ref_spad_count += 1;
            current = next;
            if current / 8 >= REF_SPAD_BUFFER_SIZE as i32 {
                return Err(uld_err("reference SPAD index out of range"));
            }
            spad_array[(current / 8) as usize] |= 1 << (current % 8);
            current += 1;
            self.set_ref_spad_map(&spad_array)?;
            peak = self.perform_ref_signal_measurement()?;
            let diff = (peak - self.target_ref_rate).abs();
            if peak > self.target_ref_rate {
                if diff > last_diff {
                    self.set_ref_spad_map(&last_spad_array)?; // the previous map came closer
                    spad_array = last_spad_array;
                    ref_spad_count -= 1;
                }
                break;
            }
            last_diff = diff;
            last_spad_array = spad_array;
        }
        self.store_ref_spads(ref_spad_count, need_apt, &spad_array);
        Ok(RefSpads { count: ref_spad_count, is_aperture: need_apt })
    }

    fn store_ref_spads(&mut self, count: i32, is_aperture_spads: i32, spad_array: &[u8; REF_SPAD_BUFFER_SIZE]) {
        self.ref_spad_enables = *spad_array;
        self.reference_spad_count = count;
        self.reference_spad_type = is_aperture_spads;
    }

    // ── limit checks ─────────────────────────────────────────────────────────

    fn set_limit_check_enable(&mut self, check: usize, enable: i32) -> Result<()> {
        if check >= CHECK_NUMBER_OF_CHECKS {
            return Err(uld_err(format!("no limit check {check}")));
        }
        let value = if enable == 0 { 0 } else { self.limit_checks_value[check] };
        let disable: u8 = if enable == 0 { 1 } else { 0 };
        if check == CHECK_SIGNAL_RATE_FINAL_RANGE {
            self.p.write_word(FINAL_RANGE_CONFIG_MIN_COUNT_RATE_RTN_LIMIT, ((value >> 9) & 0xFFFF) as u16)?;
        } else if check == CHECK_SIGNAL_RATE_MSRC {
            let current = self.p.read_byte(MSRC_CONFIG_CONTROL)?;
            self.p.write_byte(MSRC_CONFIG_CONTROL, (current & 0xFE) | (disable << 1))?;
        } else if check == CHECK_SIGNAL_RATE_PRE_RANGE {
            let current = self.p.read_byte(MSRC_CONFIG_CONTROL)?;
            self.p.write_byte(MSRC_CONFIG_CONTROL, (current & 0xEF) | (disable << 4))?;
        }
        self.limit_checks_enable[check] = if enable == 0 { 0 } else { 1 }; // the other three checks are host-side only
        Ok(())
    }

    fn set_limit_check_value(&mut self, check: usize, value: i64) -> Result<()> {
        if self.limit_checks_enable[check] == 0 {
            self.limit_checks_value[check] = value; // disabled: kept here
            return Ok(());
        }
        if check == CHECK_SIGNAL_RATE_FINAL_RANGE {
            self.p.write_word(FINAL_RANGE_CONFIG_MIN_COUNT_RATE_RTN_LIMIT, ((value >> 9) & 0xFFFF) as u16)?;
        } else if check == CHECK_SIGNAL_RATE_MSRC || check == CHECK_SIGNAL_RATE_PRE_RANGE {
            self.p.write_word(PRE_RANGE_MIN_COUNT_RATE_RTN_LIMIT, ((value >> 9) & 0xFFFF) as u16)?;
        }
        self.limit_checks_value[check] = value;
        Ok(())
    }

    fn get_limit_check_value(&self, check: usize) -> Result<i64> {
        if check == CHECK_SIGNAL_RATE_FINAL_RANGE {
            return Ok((self.p.read_word(FINAL_RANGE_CONFIG_MIN_COUNT_RATE_RTN_LIMIT)? as i64) << 9);
        }
        if check == CHECK_SIGNAL_RATE_MSRC || check == CHECK_SIGNAL_RATE_PRE_RANGE {
            return Ok((self.p.read_word(PRE_RANGE_MIN_COUNT_RATE_RTN_LIMIT)? as i64) << 9);
        }
        Ok(self.limit_checks_value[check])
    }
}

impl Vl53lxDriver for Vl53l0xUldDriver {
    fn product(&self) -> &str {
        &self.product
    }

    fn driver_name(&self) -> &'static str {
        "Vl53l0xUldDriver"
    }

    /// The only part of the family with 8-bit register addresses.
    fn addr_width(&self) -> u8 {
        1
    }

    fn clear_steps(&self) -> Vec<(u16, u8)> {
        vec![(SYSTEM_INTERRUPT_CLEAR, 0x01), (SYSTEM_INTERRUPT_CLEAR, 0x00)]
    }

    /// No Fast Mode Plus on this die.
    fn max_khz(&self) -> u16 {
        400
    }

    fn supports(&self) -> &'static [&'static str] {
        GROUPS
    }

    fn modes(&self) -> Vec<&'static str> {
        MODE_SETTINGS.iter().map(|m| m.name).collect()
    }

    fn budget_ms(&self) -> (u32, u32) {
        (20, 200)
    }

    fn model_id(&mut self) -> Result<u16> {
        Ok(self.p.read_byte(IDENTIFICATION_MODEL_ID)? as u16)
    }

    /// DataInit + StaticInit, the reference calibration, continuous mode; the bus stays at 400 kHz.
    fn sensor_init(&mut self) -> Result<()> {
        self.p.set_addr_width(self.addr_width())?;
        self.p.set_i2c_speed(400)?;
        self.wait_boot(1000)?;
        self.reset_device(1000)?;
        self.data_init()?;
        self.static_init()?;
        if !self.ref_spads_from_nvm {
            self.ref_spad_management()?;
        }
        self.perform_ref_calibration()?;
        self.set_device_mode(DEVICE_MODE_CONTINUOUS_RANGING)?;
        self.mode = "default".to_string();
        Ok(())
    }

    /// VL53L0X_StartMeasurement() for the continuous modes; a single shot in between is resumed from.
    fn start_ranging(&mut self) -> Result<()> {
        if self.device_mode == DEVICE_MODE_SINGLE_RANGING {
            if let Some(m) = self.continuous_mode {
                self.set_device_mode(m)?;
            }
        }
        self.arm_stop_variable()?;
        if self.device_mode == DEVICE_MODE_CONTINUOUS_RANGING {
            self.p.write_byte(SYSRANGE_START, SYSRANGE_MODE_BACKTOBACK)
        } else if self.device_mode == DEVICE_MODE_CONTINUOUS_TIMED {
            self.p.write_byte(SYSRANGE_START, SYSRANGE_MODE_TIMED)
        } else {
            Err(uld_err("start_ranging() needs a continuous device mode"))
        }
    }

    /// VL53L0X_StopMeasurement().
    fn stop_ranging(&mut self) -> Result<()> {
        self.p.write_byte(SYSRANGE_START, SYSRANGE_MODE_SINGLESHOT)?;
        self.p.write_byte(0xFF, 0x01)?;
        self.p.write_byte(0x00, 0x00)?;
        self.p.write_byte(0x91, 0x00)?;
        self.p.write_byte(0x00, 0x01)?;
        self.p.write_byte(0xFF, 0x00)
    }

    fn check_for_data_ready(&mut self) -> Result<bool> {
        self.data_ready()
    }

    fn wait_data_ready(&mut self, timeout: Duration) -> Result<()> {
        self.wait_ready(timeout.as_millis() as u64)
    }

    fn clear_interrupt(&mut self) -> Result<()> {
        self.clear_interrupt_mask()
    }

    fn stream_block(&self) -> (u16, u16) {
        (RESULT_BLOCK_ADDR, RESULT_BLOCK_LEN as u16)
    }

    fn decode(&mut self, raw: &[u8]) -> Result<Vl53lxRange> {
        Ok(Self::as_range(&self.parse_result_block(raw)?))
    }

    fn read_measurement(&mut self) -> Result<Vl53lxRange> {
        Ok(Self::as_range(&self.get_result()?))
    }

    /// Budget and period as the family asks for them: a period of 0 means back-to-back continuous,
    /// anything else the timed continuous mode.
    fn set_range_timing(&mut self, timing_budget_ms: u32, inter_measurement_ms: u32) -> Result<()> {
        self.set_measurement_timing_budget(timing_budget_ms as i64 * 1000)?;
        self.set_inter_measurement_period(inter_measurement_ms as i64)?;
        self.set_device_mode(if inter_measurement_ms == 0 {
            DEVICE_MODE_CONTINUOUS_RANGING
        } else {
            DEVICE_MODE_CONTINUOUS_TIMED
        })
    }

    fn get_range_timing(&mut self) -> Result<(u32, u32)> {
        let budget_ms = (self.get_measurement_timing_budget()? / 1000) as i32;
        let inter = self.get_inter_measurement_period()? as i32;
        Ok((budget_ms as u32, inter as u32))
    }

    /// Apply one of the modes: the sigma and signal limits, the budget and both VCSEL periods.
    fn set_mode(&mut self, name: &str) -> Result<()> {
        let Some(settings) = MODE_SETTINGS.iter().find(|m| m.name == name) else {
            return Err(DepzError::InvalidArgument(format!(
                "no such mode: {name} (have {})",
                self.modes().join(", ")
            )));
        };
        self.set_limit_check_enable(CHECK_SIGMA_FINAL_RANGE, 1)?;
        self.set_limit_check_enable(CHECK_SIGNAL_RATE_FINAL_RANGE, 1)?;
        self.set_limit_check_value(CHECK_SIGNAL_RATE_FINAL_RANGE, (settings.signal * 65536.0) as i64)?;
        self.set_limit_check_value(CHECK_SIGMA_FINAL_RANGE, settings.sigma * 65536)?;
        self.set_measurement_timing_budget(settings.budget_us)?;
        self.set_vcsel_pulse_period(VCSEL_PRE_RANGE, settings.pre)?;
        self.set_vcsel_pulse_period(VCSEL_FINAL_RANGE, settings.fin)?;
        self.mode = name.to_string();
        Ok(())
    }

    fn get_mode(&mut self) -> Result<String> {
        Ok(self.mode.clone())
    }

    /// By the final-range VCSEL period read from the sensor; `None` for a period no profile sets.
    fn reach_mm(&mut self) -> Result<Option<i32>> {
        Ok(match self.get_vcsel_pulse_period(VCSEL_FINAL_RANGE)? {
            10 => Some(1200),
            14 => Some(2000),
            _ => None,
        })
    }

    fn driver_info(&mut self) -> Result<BTreeMap<String, ExtraValue>> {
        let mut info = BTreeMap::new();
        info.insert(
            "signal_mcps".to_string(),
            ExtraValue::Float(self.get_limit_check_value(CHECK_SIGNAL_RATE_FINAL_RANGE)? as f64 / 65536.0),
        );
        info.insert(
            "sigma_mm".to_string(),
            ExtraValue::Float(self.get_limit_check_value(CHECK_SIGMA_FINAL_RANGE)? as f64 / 65536.0),
        );
        info.insert("budget_us".to_string(), ExtraValue::Int(self.get_measurement_timing_budget()?));
        info.insert("pre_pclks".to_string(), ExtraValue::Int(self.get_vcsel_pulse_period(VCSEL_PRE_RANGE)? as i64));
        info.insert("final_pclks".to_string(), ExtraValue::Int(self.get_vcsel_pulse_period(VCSEL_FINAL_RANGE)? as i64));
        Ok(info)
    }

    fn set_offset(&mut self, offset_mm: i32) -> Result<()> {
        self.set_offset_um(offset_mm as i64 * 1000)
    }

    fn get_offset(&mut self) -> Result<i32> {
        Ok((self.get_offset_um()? as f64 / 1000.0).round_ties_even() as i32)
    }

    fn calibrate_offset(&mut self, target_dist_mm: i32, nb_samples: Option<u32>) -> Result<i32> {
        let um = self.perform_offset_calibration(target_dist_mm, nb_samples.unwrap_or(50) as i32)?;
        Ok((um as f64 / 1000.0).round_ties_even() as i32)
    }

    fn set_xtalk(&mut self, xtalk_kcps: i32) -> Result<()> {
        self.set_xtalk_enable(1)?;
        self.set_xtalk_rate_mcps(((xtalk_kcps as i64) << 16) / 1000)
    }

    fn get_xtalk(&mut self) -> Result<i32> {
        Ok(((self.get_xtalk_rate_mcps()? * 1000) >> 16) as i32)
    }

    fn calibrate_xtalk(&mut self, target_dist_mm: i32, nb_samples: Option<u32>) -> Result<i32> {
        let rate = self.perform_xtalk_calibration(target_dist_mm, nb_samples.unwrap_or(50) as i32)?;
        Ok(((rate * 1000) >> 16) as i32)
    }

    /// Re-measure the reference SPADs on a live session → (count, is aperture). The stop variable goes
    /// back first (a stopped session leaves register 0x91 at 0, and the rate then reads far too high).
    fn perform_ref_spad_management(&mut self) -> Result<(u32, bool)> {
        self.arm_stop_variable()?;
        let r = self.ref_spad_management()?;
        Ok((r.count as u32, r.is_aperture != 0))
    }
}
