//! The ST VL53L4CD ULD 2.2.3 (`VL53L4CD_api.c` + `VL53L4CD_calibration.c`) on the host.

use std::sync::Arc;

use super::uld_math::{self as uld, Vl53l4Results};
use crate::devices::{DepzError, Result};

/// What the ULD needs from the board: the register bridge (16-bit addresses, auto-increment), a delay,
/// and the bus speed. `Vl53l4cdDevice` maps it onto the firmware bridge; a test can script it.
pub trait Vl53l4Platform: Send + Sync {
    fn read_multi(&self, addr: u16, size: usize) -> Result<Vec<u8>>;

    fn write_multi(&self, addr: u16, data: &[u8]) -> Result<()>;

    fn sleep_ms(&self, ms: u32) {
        std::thread::sleep(std::time::Duration::from_millis(ms as u64));
    }

    fn set_i2c_speed(&self, khz: u16) -> Result<()>;
}

/// The distance-window interrupt: low and high distance, mm, and the window
/// ([`uld::WINDOW_BELOW`] … [`uld::WINDOW_IN`]).
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct DetectionThresholds {
    pub distance_low_mm: u16,
    pub distance_high_mm: u16,
    pub window: u8,
}

impl From<uld::Vl53l4Error> for DepzError {
    /// A refused argument is `InvalidArgument` (contract 07 §3); what the
    /// sensor produced (a short block, an oscillator word of 0) is `Sensor`.
    fn from(e: uld::Vl53l4Error) -> Self {
        if e.is_argument() {
            DepzError::InvalidArgument(e.to_string())
        } else {
            DepzError::Sensor(e.to_string())
        }
    }
}

/// C's `round()`: half away from zero.
fn round_half_away(v: f64) -> i64 {
    if v < 0.0 {
        -((-v + 0.5).floor() as i64)
    } else {
        (v + 0.5).floor() as i64
    }
}

/// The ST VL53L4CD ULD 2.2.3 on the host, ported from the reference Python driver: init, ranging,
/// timing, offset, crosstalk, thresholds, temperature update, calibration. Every request sequence is
/// the Python one, so captures of either replay in both. Register contents are big-endian.
pub struct Vl53l4cdDriver {
    p: Arc<dyn Vl53l4Platform>,
}

impl Vl53l4cdDriver {
    pub fn new(platform: Arc<dyn Vl53l4Platform>) -> Vl53l4cdDriver {
        Vl53l4cdDriver { p: platform }
    }

    // ── register access ──────────────────────────────────────────────────────

    pub fn read_byte(&self, addr: u16) -> Result<u8> {
        Ok(self.p.read_multi(addr, 1)?[0])
    }

    pub fn read_word(&self, addr: u16) -> Result<u16> {
        let b = self.p.read_multi(addr, 2)?;
        Ok(u16::from_be_bytes([b[0], b[1]]))
    }

    pub fn read_dword(&self, addr: u16) -> Result<u32> {
        let b = self.p.read_multi(addr, 4)?;
        Ok(u32::from_be_bytes([b[0], b[1], b[2], b[3]]))
    }

    pub fn write_byte(&self, addr: u16, value: u8) -> Result<()> {
        self.p.write_multi(addr, &[value])
    }

    pub fn write_word(&self, addr: u16, value: u16) -> Result<()> {
        self.p.write_multi(addr, &value.to_be_bytes())
    }

    pub fn write_dword(&self, addr: u16, value: u32) -> Result<()> {
        self.p.write_multi(addr, &value.to_be_bytes())
    }

    // ── identity and init ────────────────────────────────────────────────────

    pub fn get_sensor_id(&self) -> Result<u16> {
        self.read_word(uld::IDENTIFICATION__MODEL_ID)
    }

    pub fn is_alive(&self) -> Result<bool> {
        Ok(self.get_sensor_id()? == uld::MODEL_ID_VL53L4CD)
    }

    /// Fails when FIRMWARE__SYSTEM_STATUS never reads 0x03.
    pub fn wait_boot(&self, timeout_ms: u32) -> Result<()> {
        for _ in 0..timeout_ms.max(1) {
            if self.read_byte(uld::FIRMWARE__SYSTEM_STATUS)? == 0x03 {
                return Ok(());
            }
            self.p.sleep_ms(1);
        }
        Err(DepzError::Sensor(
            "timeout waiting for FIRMWARE__SYSTEM_STATUS == 0x03".into(),
        ))
    }

    /// Initialise the sensor and leave the bus at `bus_khz`. The configuration block goes at 400 kHz,
    /// the only speed an unconfigured sensor is specified for; the bridge is re-timed after.
    pub fn sensor_init(&self, bus_khz: u16) -> Result<()> {
        self.p.set_i2c_speed(uld::I2C_KHZ_BOOT)?;
        self.wait_boot(1000)?;
        // The C driver writes the 91 bytes one register at a time; auto-increment makes it one write.
        self.p.write_multi(uld::CONFIG_ADDR, &uld::config_block())?;
        if bus_khz != uld::I2C_KHZ_BOOT {
            self.p.set_i2c_speed(bus_khz)?;
        }
        self.write_byte(uld::SYSTEM_START, 0x40)?; // start VHV
        self.wait_data_ready(1000)?;
        self.clear_interrupt()?;
        self.stop_ranging()?;
        self.write_byte(uld::VHV_CONFIG__TIMEOUT_MACROP_LOOP_BOUND, 0x09)?;
        self.write_byte(0x000B, 0x00)?;
        self.write_word(0x0024, 0x0500)?;
        self.set_range_timing(50, 0)
    }

    // ── ranging ──────────────────────────────────────────────────────────────

    pub fn clear_interrupt(&self) -> Result<()> {
        self.write_byte(uld::SYSTEM__INTERRUPT_CLEAR, 0x01)
    }

    pub fn start_ranging(&self) -> Result<()> {
        // INTERMEASUREMENT 0 = continuous, anything else = autonomous low power.
        let mode = if self.read_dword(uld::INTERMEASUREMENT_MS)? == 0 {
            0x21
        } else {
            0x40
        };
        self.write_byte(uld::SYSTEM_START, mode)
    }

    pub fn stop_ranging(&self) -> Result<()> {
        self.write_byte(uld::SYSTEM_START, 0x80)
    }

    pub fn check_for_data_ready(&self) -> Result<bool> {
        let int_pol = if (self.read_byte(uld::GPIO_HV_MUX__CTRL)? & 0x10) >> 4 == 1 {
            0
        } else {
            1
        };
        Ok(self.read_byte(uld::GPIO__TIO_HV_STATUS)? & 1 == int_pol)
    }

    /// Fails when no data is ready within `timeout_ms` polls.
    pub fn wait_data_ready(&self, timeout_ms: u32) -> Result<()> {
        for _ in 0..timeout_ms.max(1) {
            if self.check_for_data_ready()? {
                return Ok(());
            }
            self.p.sleep_ms(1);
        }
        Err(DepzError::Sensor("timeout waiting for data ready".into()))
    }

    /// One block read instead of the C driver's six register reads; the decoding is identical.
    pub fn get_result(&self) -> Result<Vl53l4Results> {
        Ok(uld::parse_result_block(&self.p.read_multi(
            uld::RESULT_BLOCK_ADDR,
            uld::RESULT_BLOCK_LEN as usize,
        )?)?)
    }

    // ── timing ───────────────────────────────────────────────────────────────

    pub fn set_range_timing(&self, timing_budget_ms: u32, inter_measurement_ms: u32) -> Result<()> {
        // refused before any register traffic, not only before the writes
        uld::check_range_timing(timing_budget_ms, inter_measurement_ms)?;
        let clock_pll = if inter_measurement_ms > 0 {
            self.read_word(uld::RESULT__OSC_CALIBRATE_VAL)?
        } else {
            0
        };
        let (a, b, im) = uld::range_timing_registers(
            timing_budget_ms,
            inter_measurement_ms,
            self.read_word(uld::OSC_FREQUENCY)?,
            clock_pll,
        )?;
        self.write_dword(uld::INTERMEASUREMENT_MS, im)?;
        self.write_word(uld::RANGE_CONFIG_A, a)?;
        self.write_word(uld::RANGE_CONFIG_B, b)
    }

    /// (timing budget ms, inter-measurement ms; 0 = continuous).
    pub fn get_range_timing(&self) -> Result<(u32, u32)> {
        Ok(uld::decode_range_timing(
            self.read_dword(uld::INTERMEASUREMENT_MS)?,
            self.read_word(uld::RESULT__OSC_CALIBRATE_VAL)?,
            self.read_word(uld::OSC_FREQUENCY)?,
            self.read_word(uld::RANGE_CONFIG_A)?,
        )?)
    }

    // ── offset and crosstalk ─────────────────────────────────────────────────

    pub fn set_offset(&self, offset_mm: i32) -> Result<()> {
        self.write_word(uld::RANGE_OFFSET_MM, uld::try_offset_raw(offset_mm)?)?;
        self.write_word(uld::INNER_OFFSET_MM, 0)?;
        self.write_word(uld::OUTER_OFFSET_MM, 0)
    }

    pub fn get_offset(&self) -> Result<i32> {
        Ok(uld::decode_offset(self.read_word(uld::RANGE_OFFSET_MM)?))
    }

    pub fn set_xtalk(&self, xtalk_kcps: u16) -> Result<()> {
        let raw = uld::try_xtalk_raw(xtalk_kcps)?;
        self.write_word(uld::XTALK_X_PLANE_GRADIENT_KCPS, 0x0000)?;
        self.write_word(uld::XTALK_Y_PLANE_GRADIENT_KCPS, 0x0000)?;
        self.write_word(uld::XTALK_PLANE_OFFSET_KCPS, raw)
    }

    pub fn get_xtalk(&self) -> Result<u16> {
        Ok(uld::decode_xtalk(
            self.read_word(uld::XTALK_PLANE_OFFSET_KCPS)?,
        ))
    }

    // ── thresholds ───────────────────────────────────────────────────────────

    /// The distance-window interrupt; `window` is one of [`uld::WINDOW_BELOW`] … [`uld::WINDOW_IN`].
    pub fn set_detection_thresholds(
        &self,
        distance_low_mm: u16,
        distance_high_mm: u16,
        window: u8,
    ) -> Result<()> {
        self.write_byte(uld::SYSTEM__INTERRUPT, window)?;
        self.write_word(uld::THRESH_HIGH, distance_high_mm)?;
        self.write_word(uld::THRESH_LOW, distance_low_mm)
    }

    pub fn get_detection_thresholds(&self) -> Result<DetectionThresholds> {
        let high = self.read_word(uld::THRESH_HIGH)?;
        let low = self.read_word(uld::THRESH_LOW)?;
        Ok(DetectionThresholds {
            distance_low_mm: low,
            distance_high_mm: high,
            window: self.read_byte(uld::SYSTEM__INTERRUPT)? & 0x07,
        })
    }

    pub fn set_signal_threshold(&self, signal_kcps: u16) -> Result<()> {
        self.write_word(
            uld::MIN_COUNT_RATE_RTN_LIMIT_MCPS,
            uld::signal_threshold_raw(signal_kcps),
        )
    }

    pub fn get_signal_threshold(&self) -> Result<u16> {
        Ok(uld::decode_signal_threshold(
            self.read_word(uld::MIN_COUNT_RATE_RTN_LIMIT_MCPS)?,
        ))
    }

    pub fn set_sigma_threshold(&self, sigma_mm: u16) -> Result<()> {
        self.write_word(
            uld::RANGE_CONFIG__SIGMA_THRESH,
            uld::sigma_threshold_raw(sigma_mm)?,
        )
    }

    pub fn get_sigma_threshold(&self) -> Result<u16> {
        Ok(uld::decode_sigma_threshold(
            self.read_word(uld::RANGE_CONFIG__SIGMA_THRESH)?,
        ))
    }

    // ── temperature and calibration ──────────────────────────────────────────

    /// Re-run VHV; recommended after a >8 °C ambient change (ST Example_3).
    pub fn start_temperature_update(&self) -> Result<()> {
        self.write_byte(uld::VHV_CONFIG__TIMEOUT_MACROP_LOOP_BOUND, 0x81)?;
        self.write_byte(0x000B, 0x92)?;
        self.start_ranging()?;
        self.wait_data_ready(1000)?;
        self.clear_interrupt()?;
        self.stop_ranging()?;
        self.write_byte(uld::VHV_CONFIG__TIMEOUT_MACROP_LOOP_BOUND, 0x09)?;
        self.write_byte(0x000B, 0x00)
    }

    // The ranging loop both calibrations share: data ready, result, clear, n times.
    fn collect(
        &self,
        nb_samples: u32,
        mut on_sample: impl FnMut(u32, &Vl53l4Results),
    ) -> Result<()> {
        self.start_ranging()?;
        for i in 0..nb_samples {
            self.wait_data_ready(5000)?;
            let r = self.get_result()?;
            self.clear_interrupt()?;
            on_sample(i, &r);
        }
        self.stop_ranging()
    }

    /// Offset calibration against a target at `target_dist_mm` (10–1000); returns the offset now
    /// programmed, mm.
    pub fn calibrate_offset(&self, target_dist_mm: i32, nb_samples: u32) -> Result<i32> {
        if !(5..=255).contains(&nb_samples) || !(10..=1000).contains(&target_dist_mm) {
            return Err(DepzError::InvalidArgument(
                "nb_samples must be 5..255, target 10..1000 mm".into(),
            ));
        }
        self.write_word(uld::RANGE_OFFSET_MM, 0)?;
        self.write_word(uld::INNER_OFFSET_MM, 0)?;
        self.write_word(uld::OUTER_OFFSET_MM, 0)?;
        self.collect(10, |_, _| {})?; // device heat loop
        let mut sum: i64 = 0;
        self.collect(nb_samples, |_, r| sum += r.distance_mm as i64)?;
        let offset_mm = target_dist_mm - sum.div_euclid(nb_samples as i64) as i32;
        self.write_word(uld::RANGE_OFFSET_MM, uld::try_offset_raw(offset_mm)?)?;
        Ok(offset_mm)
    }

    /// Crosstalk calibration against a target at `target_dist_mm` (10–5000); returns the xtalk now
    /// programmed, kcps (C rounding).
    pub fn calibrate_xtalk(&self, target_dist_mm: i32, nb_samples: u32) -> Result<i32> {
        if !(5..=255).contains(&nb_samples) || !(10..=5000).contains(&target_dist_mm) {
            return Err(DepzError::InvalidArgument(
                "nb_samples must be 5..255, target 10..5000 mm".into(),
            ));
        }
        self.write_word(uld::XTALK_PLANE_OFFSET_KCPS, 0)?; // compensation off
        self.collect(10, |_, _| {})?; // device heat loop
        let mut samples: Vec<Vl53l4Results> = Vec::new();
        self.collect(nb_samples, |i, r| {
            if r.range_status == 0 && i > 0 {
                samples.push(*r); // invalid measurements and the first frame are dropped
            }
        })?;
        if samples.is_empty() {
            return Err(DepzError::Sensor(
                "xtalk calibration failed: no valid samples".into(),
            ));
        }
        let n = samples.len() as f64;
        let avg_distance = samples.iter().map(|s| s.distance_mm as f64).sum::<f64>() / n;
        let avg_spad_nb = samples.iter().map(|s| s.number_of_spad as f64).sum::<f64>() / n;
        let avg_signal = samples
            .iter()
            .map(|s| s.signal_rate_kcps as f64)
            .sum::<f64>()
            / n;
        let tmp = (1.0 - avg_distance / target_dist_mm as f64) * (avg_signal / avg_spad_nb);
        if tmp > 127.0 {
            // 65536 / 512
            return Err(DepzError::Sensor(format!(
                "xtalk calibration failed: {tmp:.1} kcps > 127"
            )));
        }
        self.write_word(
            uld::XTALK_PLANE_OFFSET_KCPS,
            ((tmp * 512.0) as i64 & 0xFFFF) as u16,
        )?;
        Ok(round_half_away(tmp) as i32)
    }
}
