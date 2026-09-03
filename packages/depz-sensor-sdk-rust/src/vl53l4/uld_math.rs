//! VL53L4CD host-side ULD codecs and register math (ST STSW-IMG026 2.2.3).
//!
//! Pure functions ported from `VL53L4CD_api.c` / `VL53L4CD_calibration.c` via
//! the hardware-proven Python reference (`depz_sensor_sdk/vl53l4/uld.py`):
//! the 17-byte result-block decode (`GetResult`), the `SetRangeTiming` /
//! `GetRangeTiming` register math (32-bit truncations and the `1.055`/`1.065`
//! PLL factors included), the tuning-register word codecs and the 91-byte
//! default configuration block. Byte-exact with `contracts/vectors/vl53l4.json`.
//!
//! The live register-bridge init/ranging sequences (VHV calibration, ranging
//! loops) are hardware-dependent and intentionally out of scope for this
//! decode-layer crate — this module holds everything the golden vectors can
//! verify.

// ── Registers (VL53L4CD_api.h) ───────────────────────────────────────────────

pub const SOFT_RESET: u16 = 0x0000;
pub const I2C_SLAVE__DEVICE_ADDRESS: u16 = 0x0001;
/// Oscillator-frequency word (unnamed in the C driver).
pub const OSC_FREQUENCY: u16 = 0x0006;
pub const VHV_CONFIG__TIMEOUT_MACROP_LOOP_BOUND: u16 = 0x0008;
pub const XTALK_PLANE_OFFSET_KCPS: u16 = 0x0016;
pub const XTALK_X_PLANE_GRADIENT_KCPS: u16 = 0x0018;
pub const XTALK_Y_PLANE_GRADIENT_KCPS: u16 = 0x001A;
pub const RANGE_OFFSET_MM: u16 = 0x001E;
pub const INNER_OFFSET_MM: u16 = 0x0020;
pub const OUTER_OFFSET_MM: u16 = 0x0022;
pub const GPIO_HV_MUX__CTRL: u16 = 0x0030;
pub const GPIO__TIO_HV_STATUS: u16 = 0x0031;
pub const SYSTEM__INTERRUPT: u16 = 0x0046;
pub const RANGE_CONFIG_A: u16 = 0x005E;
pub const RANGE_CONFIG_B: u16 = 0x0061;
pub const RANGE_CONFIG__SIGMA_THRESH: u16 = 0x0064;
pub const MIN_COUNT_RATE_RTN_LIMIT_MCPS: u16 = 0x0066;
pub const INTERMEASUREMENT_MS: u16 = 0x006C;
pub const THRESH_HIGH: u16 = 0x0072;
pub const THRESH_LOW: u16 = 0x0074;
pub const SYSTEM__INTERRUPT_CLEAR: u16 = 0x0086;
pub const SYSTEM_START: u16 = 0x0087;
pub const RESULT__RANGE_STATUS: u16 = 0x0089;
pub const RESULT__SPAD_NB: u16 = 0x008C;
pub const RESULT__SIGNAL_RATE: u16 = 0x008E;
pub const RESULT__AMBIENT_RATE: u16 = 0x0090;
pub const RESULT__SIGMA: u16 = 0x0092;
pub const RESULT__DISTANCE: u16 = 0x0096;
pub const RESULT__OSC_CALIBRATE_VAL: u16 = 0x00DE;
pub const FIRMWARE__SYSTEM_STATUS: u16 = 0x00E5;
pub const IDENTIFICATION__MODEL_ID: u16 = 0x010F;

/// Expected `IDENTIFICATION__MODEL_ID` word for a VL53L4CD.
pub const MODEL_ID_VL53L4CD: u16 = 0xEBAA;

/// Detection-threshold window mode (`SYSTEM__INTERRUPT`): below.
pub const WINDOW_BELOW: u8 = 0;
/// Detection-threshold window mode (`SYSTEM__INTERRUPT`): above.
pub const WINDOW_ABOVE: u8 = 1;
/// Detection-threshold window mode (`SYSTEM__INTERRUPT`): out of window.
pub const WINDOW_OUT: u8 = 2;
/// Detection-threshold window mode (`SYSTEM__INTERRUPT`): in window.
pub const WINDOW_IN: u8 = 3;

/// First register of the init configuration block (0x2D).
pub const CONFIG_ADDR: u16 = 0x002D;
/// Last register of the init configuration block (0x87).
pub const CONFIG_END: u16 = 0x0087;

/// `VL53L4CD_DEFAULT_CONFIGURATION[]` — 91 bytes, registers 0x2D..0x87.
/// [`config_block`] always overrides byte 0 (register 0x2D) with
/// [`CONFIG_FMP_BYTE`].
pub const DEFAULT_CONFIGURATION: [u8; 91] = [
    0x00, 0x00, 0x00, 0x11, 0x02, 0x00, 0x02, 0x08, // 0x2D..0x34
    0x00, 0x08, 0x10, 0x01, 0x01, 0x00, 0x00, 0x00, // 0x35..0x3C
    0x00, 0xff, 0x00, 0x0F, 0x00, 0x00, 0x00, 0x00, // 0x3D..0x44
    0x00, 0x20, 0x0b, 0x00, 0x00, 0x02, 0x14, 0x21, // 0x45..0x4C
    0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x00, 0xc8, // 0x4D..0x54
    0x00, 0x00, 0x38, 0xff, 0x01, 0x00, 0x08, 0x00, // 0x55..0x5C
    0x00, 0x01, 0xcc, 0x07, 0x01, 0xf1, 0x05, 0x00, // 0x5D..0x64
    0xa0, 0x00, 0x80, 0x08, 0x38, 0x00, 0x00, 0x00, // 0x65..0x6C
    0x00, 0x0f, 0x89, 0x00, 0x00, 0x00, 0x00, 0x00, // 0x6D..0x74
    0x00, 0x00, 0x01, 0x07, 0x05, 0x06, 0x06, 0x00, // 0x75..0x7C
    0x00, 0x02, 0xc7, 0xff, 0x9B, 0x00, 0x00, 0x00, // 0x7D..0x84
    0x01, 0x00, 0x00, // 0x85..0x87
];

/// Value forced into byte 0 of the config block (register 0x2D): I2C Fast Mode
/// Plus pad, set unconditionally and never cleared (FM+ pads work at every bus
/// step down to 100 kHz; clearing it mid-block NACKs and truncates the write).
pub const CONFIG_FMP_BYTE: u8 = 0x12;

/// The 91-byte block `sensor_init` writes at [`CONFIG_ADDR`]: the ST default
/// configuration with byte 0 forced to [`CONFIG_FMP_BYTE`] (Fast Mode Plus).
pub fn config_block() -> [u8; 91] {
    let mut block = DEFAULT_CONFIGURATION;
    block[0] = CONFIG_FMP_BYTE;
    block
}

/// The block the MCU streams: `RESULT__RANGE_STATUS` .. 0x0099 — every field of
/// `VL53L4CD_ResultsData_t` in one read.
pub const RESULT_BLOCK_ADDR: u16 = RESULT__RANGE_STATUS;
/// Length of the streamed result block in bytes.
pub const RESULT_BLOCK_LEN: u16 = 17;

/// The bridge boots at 400 kHz; an unconfigured sensor is only specified for
/// that speed, so init always runs its configuration block there.
pub const I2C_KHZ_BOOT: u16 = 400;
/// The bus speed the bridge is left at after init (the result-block read is
/// ~4x faster than at boot speed).
pub const I2C_KHZ_DEFAULT: u16 = 1000;

/// `GetResult()` raw status → ULD status (`status_rtn[24]` in
/// `VL53L4CD_api.c`); raw ≥ 24 passes through unmapped.
pub const STATUS_RTN: [u8; 24] = [
    255, 255, 255, 5, 2, 4, 1, 7, 3, 0, 255, 255, 9, 13, 255, 255, 255, 255, 10, 6, 255, 255, 11,
    12,
];

/// Range-status → human-readable description (UM2931, "Range status
/// description"). Unknown codes map to `"unknown"`.
pub fn range_status_name(status: u8) -> &'static str {
    match status {
        0 => "valid",
        1 => "sigma above threshold",
        2 => "signal below threshold",
        3 => "distance below detection threshold",
        4 => "phase out of valid limit",
        5 => "hardware fail",
        6 => "no wrap-around check done",
        7 => "wrapped target, phase mismatch",
        8 => "processing fail",
        9 => "crosstalk signal fail",
        10 => "interrupt error",
        11 => "merged target",
        12 => "signal too low",
        255 => "other error",
        _ => "unknown",
    }
}

/// VL53L4 codec/math failure.
#[derive(Debug, Clone, PartialEq, Eq)]
pub enum Vl53l4Error {
    /// Result block shorter than the 15 bytes the decode reads.
    ShortResultBlock,
    /// `osc_frequency` register reads 0.
    OscFrequencyZero,
    /// `timing_budget_ms` outside 10..=200.
    TimingBudgetOutOfRange,
    /// `inter_measurement_ms` must be 0 (continuous) or greater than the
    /// timing budget (autonomous).
    InterMeasurementInvalid,
    /// `sigma_mm` exceeds the 16383 mm register ceiling.
    SigmaTooLarge,
}

impl std::fmt::Display for Vl53l4Error {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            Vl53l4Error::ShortResultBlock => write!(f, "VL53L4 result block too short"),
            Vl53l4Error::OscFrequencyZero => write!(f, "VL53L4 osc_frequency reads 0"),
            Vl53l4Error::TimingBudgetOutOfRange => {
                write!(f, "VL53L4 timing_budget_ms must be 10..200")
            }
            Vl53l4Error::InterMeasurementInvalid => {
                write!(f, "VL53L4 inter_measurement_ms must be 0 or > timing_budget_ms")
            }
            Vl53l4Error::SigmaTooLarge => write!(f, "VL53L4 sigma_mm must be <= 16383"),
        }
    }
}

impl std::error::Error for Vl53l4Error {}

/// `VL53L4CD_ResultsData_t` plus the sensor's own frame counter.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct Vl53l4Results {
    /// 0 = valid ([`range_status_name`]); raw ≥ 24 passes through unmapped.
    pub range_status: u8,
    pub distance_mm: u16,
    pub ambient_rate_kcps: u32,
    pub ambient_per_spad_kcps: u32,
    pub signal_rate_kcps: u32,
    pub signal_per_spad_kcps: u32,
    pub number_of_spad: u16,
    pub sigma_mm: u16,
    /// 0x008B `RESULT__STREAM_COUNT`: wraps at 255. The C ULD ignores it; it
    /// tells a frame the host never received from one the sensor never
    /// produced.
    pub stream_count: u8,
}

impl Vl53l4Results {
    /// Human-readable [`Self::range_status`] ([`range_status_name`]).
    pub fn status_text(&self) -> &'static str {
        range_status_name(self.range_status)
    }
}

/// Decode the streamed 0x0089..0x0099 block exactly as `VL53L4CD_GetResult()`
/// decodes the same registers read one by one. Register contents are
/// big-endian words (the bridge passes them through untouched).
pub fn parse_result_block(raw: &[u8]) -> Result<Vl53l4Results, Vl53l4Error> {
    if raw.len() < 15 {
        return Err(Vl53l4Error::ShortResultBlock);
    }

    let status_raw = raw[0] & 0x1F;
    let range_status = if (status_raw as usize) < STATUS_RTN.len() {
        STATUS_RTN[status_raw as usize]
    } else {
        status_raw
    };

    let raw_spads = u16::from_be_bytes([raw[3], raw[4]]) as u32; // 0x008C
    let signal_kcps = u16::from_be_bytes([raw[5], raw[6]]) as u32 * 8; // 0x008E
    let ambient_kcps = u16::from_be_bytes([raw[7], raw[8]]) as u32 * 8; // 0x0090

    Ok(Vl53l4Results {
        range_status,
        stream_count: raw[2],
        number_of_spad: (raw_spads / 256) as u16,
        signal_rate_kcps: signal_kcps,
        ambient_rate_kcps: ambient_kcps,
        sigma_mm: u16::from_be_bytes([raw[9], raw[10]]) / 4, // 0x0092
        distance_mm: u16::from_be_bytes([raw[13], raw[14]]), // 0x0096
        signal_per_spad_kcps: (signal_kcps * 256).checked_div(raw_spads).unwrap_or(0),
        ambient_per_spad_kcps: (ambient_kcps * 256).checked_div(raw_spads).unwrap_or(0),
    })
}

/// `SetRangeTiming` register math → `(RANGE_CONFIG_A, RANGE_CONFIG_B,
/// INTERMEASUREMENT_MS raw dword)`.
///
/// `osc_frequency` is the word read from 0x0006; `clock_pll` is the word read
/// from `RESULT__OSC_CALIBRATE_VAL` (used only in autonomous mode, i.e. when
/// `inter_measurement_ms > 0`). `inter_measurement_ms == 0` selects continuous
/// mode; a value greater than the budget selects autonomous low power.
pub fn range_timing_registers(
    timing_budget_ms: u32,
    inter_measurement_ms: u32,
    osc_frequency: u16,
    clock_pll: u16,
) -> Result<(u16, u16, u32), Vl53l4Error> {
    if osc_frequency == 0 {
        return Err(Vl53l4Error::OscFrequencyZero);
    }
    if !(10..=200).contains(&timing_budget_ms) {
        return Err(Vl53l4Error::TimingBudgetOutOfRange);
    }

    let mut timing_budget_us = timing_budget_ms * 1000;
    let macro_period_us = 2304u32.wrapping_mul(0x4000_0000 / osc_frequency as u32) >> 6;

    let intermeasurement_raw;
    if inter_measurement_ms == 0 {
        // Continuous mode.
        intermeasurement_raw = 0u32;
        timing_budget_us -= 2500;
    } else if inter_measurement_ms > timing_budget_ms {
        // Autonomous low-power mode (f64 product truncated toward zero).
        let factor = 1.055 * inter_measurement_ms as f64 * (clock_pll & 0x3FF) as f64;
        intermeasurement_raw = factor as u32;
        timing_budget_us = (timing_budget_us - 4300) / 2;
    } else {
        return Err(Vl53l4Error::InterMeasurementInvalid);
    }

    timing_budget_us <<= 12;
    let mut words = [0u16; 2];
    for (word, mult) in words.iter_mut().zip([16u32, 12]) {
        // RANGE_CONFIG_A (x16), RANGE_CONFIG_B (x12).
        let tmp = macro_period_us.wrapping_mul(mult) >> 6;
        let mut ls_byte = (timing_budget_us as u64 + (tmp as u64 >> 1)) / tmp as u64 - 1;
        let mut ms_byte = 0u64;
        while ls_byte & 0xFFFF_FF00 != 0 {
            ls_byte >>= 1;
            ms_byte += 1;
        }
        *word = (((ms_byte << 8) + (ls_byte & 0xFF)) & 0xFFFF) as u16;
    }
    Ok((words[0], words[1], intermeasurement_raw))
}

/// `GetRangeTiming` register math → `(timing_budget_ms, inter_measurement_ms)`.
///
/// Inputs are the raw register reads: the `INTERMEASUREMENT_MS` dword, the
/// `RESULT__OSC_CALIBRATE_VAL` word, the 0x0006 word and the `RANGE_CONFIG_A`
/// word.
pub fn decode_range_timing(
    intermeasurement_raw: u32,
    clock_pll: u16,
    osc_frequency: u16,
    range_config_a: u16,
) -> Result<(u32, u32), Vl53l4Error> {
    if osc_frequency == 0 {
        return Err(Vl53l4Error::OscFrequencyZero);
    }

    let pll = ((1.065 * (clock_pll & 0x3FF) as f64) as u32) & 0xFFFF;
    let inter_measurement_ms = intermeasurement_raw.checked_div(pll).unwrap_or(0) & 0xFFFF;

    let macro_period_us = 2304u32.wrapping_mul(0x4000_0000 / osc_frequency as u32) >> 6;
    let ls_byte = ((range_config_a & 0x00FF) as u64) << 4;
    // (0x04 - (ms_byte - 1) - 1) wraps negative: compute in i64, mask to u32.
    let ms_raw = ((range_config_a & 0xFF00) >> 8) as i64;
    let ms_byte = ((0x04 - (ms_raw - 1) - 1) & 0xFFFF_FFFF) as u32;
    let macro_period_us = macro_period_us.wrapping_mul(16) as u64;

    let mut budget = ((((ls_byte + 1) * (macro_period_us >> 6)) - ((macro_period_us >> 6) >> 1))
        & 0xFFFF_FFFF)
        >> 12;
    if ms_byte < 12 {
        budget >>= ms_byte;
    }
    budget = if intermeasurement_raw == 0 {
        budget + 2500 // continuous
    } else {
        budget * 2 + 4300 // autonomous
    };
    Ok(((budget / 1000) as u32, inter_measurement_ms))
}

// ── Threshold / offset / xtalk raw codecs (register word ↔ user units) ───────

/// `RANGE_OFFSET_MM` word for `SetOffset` (`INNER`/`OUTER` are zeroed
/// alongside).
pub fn offset_raw(offset_mm: i32) -> u16 {
    (offset_mm.wrapping_mul(4) & 0xFFFF) as u16
}

/// `GetOffset`: `RANGE_OFFSET_MM` word → signed millimetres.
pub fn decode_offset(raw_word: u16) -> i32 {
    let temp = (((raw_word as u32) << 3) & 0xFFFF) >> 5;
    if temp > 1024 {
        temp as i32 - 2048
    } else {
        temp as i32
    }
}

/// `XTALK_PLANE_OFFSET_KCPS` word for `SetXtalk`.
pub fn xtalk_raw(xtalk_kcps: u16) -> u16 {
    (((xtalk_kcps as u32) << 9) & 0xFFFF) as u16
}

/// `GetXtalk`: `XTALK_PLANE_OFFSET_KCPS` word → kcps (round half to even,
/// matching the Python reference's `round`).
pub fn decode_xtalk(raw_word: u16) -> u16 {
    (raw_word as f64 / 512.0).round_ties_even() as u16
}

/// `MIN_COUNT_RATE_RTN_LIMIT_MCPS` word for `SetSignalThreshold`.
pub fn signal_threshold_raw(signal_kcps: u16) -> u16 {
    signal_kcps >> 3
}

/// `GetSignalThreshold`: register word → kcps.
pub fn decode_signal_threshold(raw_word: u16) -> u16 {
    (((raw_word as u32) << 3) & 0xFFFF) as u16
}

/// `RANGE_CONFIG__SIGMA_THRESH` word for `SetSigmaThreshold`;
/// [`Vl53l4Error::SigmaTooLarge`] above 16383 mm.
pub fn sigma_threshold_raw(sigma_mm: u16) -> Result<u16, Vl53l4Error> {
    if sigma_mm > (0xFFFF >> 2) {
        return Err(Vl53l4Error::SigmaTooLarge);
    }
    Ok(sigma_mm << 2)
}

/// `GetSigmaThreshold`: register word → millimetres.
pub fn decode_sigma_threshold(raw_word: u16) -> u16 {
    raw_word >> 2
}
