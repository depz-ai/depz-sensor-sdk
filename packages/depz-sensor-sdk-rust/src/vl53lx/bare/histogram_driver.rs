//! The histogram driver of the 1D family: [`BareDriver`] + [`hist`] behind the
//! [`Vl53lxDriver`] contract.

use std::collections::BTreeMap;
use std::sync::Arc;
use std::time::{Duration, Instant};

use crate::devices::{DepzError, Result};
use crate::vl53lx::driver::{BinData, ExtraValue, Vl53lxDriver, Vl53lxPlatform, Vl53lxRange};

use super::bare_driver::BareDriver;
use super::core::HistogramBinData;
use super::hist;
use super::history::FrameHistory;
use super::regs;

const GPIO_HV_MUX_CTRL: u16 = 0x0030;
const GPIO_TIO_HV_STATUS: u16 = 0x0031;
const SYSTEM_INTERRUPT_CLEAR: u16 = 0x0086;
const FIRMWARE_SYSTEM_STATUS: u16 = 0x00E5;
const PAD_I2C_HV_CONFIG: u16 = 0x002D;
const I2C_KHZ_BOOT: u16 = 400;
const MAX_KHZ: u16 = 1000; // with Fast Mode Plus on the sensor pad

// Bits 2 and 5, the byte VL53L4CD_I2C_FAST_MODE_PLUS writes here in the C ULD. ST's histogram preset
// leaves the pad at 0x00; the pad is a property of the die, so it is set here and never cleared.
const FMP_PAD_CONFIG: u8 = 0x12;

const DEFAULT_MODE: &str = "medium";
const GROUPS: &[&str] = &["mode", "timing"];

/// ST's VL53LX BareDriver on the host — the histogram driver of VL53L1CX / L1CB / L3CX / L4CD /
/// L4CX: the die hands over 24 raw photon-count bins per frame and [`hist`] turns them into up to
/// four targets. What it costs against the light drivers: no calibrations and no detection
/// thresholds, and the first frame of a stream reports status 6 (no wrap check done) because the
/// check needs the frame before. [`Vl53lxDriver::bin_data`] hands out a [`HistogramBinData`].
pub struct Vl53lxHistogramDriver {
    p: Arc<dyn Vl53lxPlatform>,
    product: String,
    bare: BareDriver,
    mode: &'static str,
    budget_ms: u32,
    inter_ms: u32,
    modes: &'static [&'static str],
    budget_range: (u32, u32),
    // The A/B alternation rides on the ll-driver state, which the polled C loop advances in the call
    // that clears the interrupt. Here the bridge clears it, so the state is advanced when a new frame
    // arrives — told by the device's own result__stream_count, so decoding the same sample twice does
    // not step it twice.
    last_stream_count: Option<u8>,
    // The phase and event consistency checks read the frame before this one.
    history: FrameHistory,
}

impl Vl53lxHistogramDriver {
    /// A driver for `product` over `platform`; nothing touches the bus until
    /// [`Vl53lxDriver::sensor_init`].
    pub fn new(platform: Arc<dyn Vl53lxPlatform>, product: &str) -> Self {
        Vl53lxHistogramDriver {
            bare: BareDriver::new(platform.clone()),
            p: platform,
            product: product.to_string(),
            mode: DEFAULT_MODE,
            budget_ms: 33,
            inter_ms: 0,
            modes: regs::DISTANCE_MODES,
            budget_range: (2, 550),
            last_stream_count: None,
            history: FrameHistory::new(),
        }
    }

    /// The BareDriver underneath: device image, factory data, preset and post-processing config.
    pub fn bare(&self) -> &BareDriver {
        &self.bare
    }

    /// The BareDriver underneath, to tune.
    pub fn bare_mut(&mut self) -> &mut BareDriver {
        &mut self.bare
    }

    /// The frame history the consistency checks read.
    pub fn history(&self) -> &FrameHistory {
        &self.history
    }

    /// Poll `FIRMWARE__SYSTEM_STATUS` until the die has booted.
    pub fn wait_boot(&self, timeout: Duration) -> Result<()> {
        let deadline = Instant::now() + timeout;
        while self.p.read_byte(FIRMWARE_SYSTEM_STATUS)? != 0x03 {
            if Instant::now() > deadline {
                return Err(DepzError::Sensor(
                    "timeout waiting for FIRMWARE__SYSTEM_STATUS == 0x03".into(),
                ));
            }
            self.p.sleep_ms(1);
        }
        Ok(())
    }

    // Vl53l1Die.PhaseWindowMm of the C# port: the distance a valid-phase-high register value stands for.
    fn phase_window_mm(fast_osc_frequency: i64, valid_phase_high: i64) -> i64 {
        let pll_period = (1i64 << 30) / fast_osc_frequency;
        (valid_phase_high * pll_period * hist::SPEED_OF_LIGHT_IN_AIR_DIV8) >> 25
    }
}

impl Vl53lxDriver for Vl53lxHistogramDriver {
    fn product(&self) -> &str {
        &self.product
    }

    fn driver_name(&self) -> &'static str {
        "Vl53lxHistogramDriver"
    }

    fn addr_width(&self) -> u8 {
        2
    }

    /// One write clears the interrupt and lets the next frame run (measured over 40 frames on both
    /// dies).
    fn clear_steps(&self) -> Vec<(u16, u8)> {
        vec![(SYSTEM_INTERRUPT_CLEAR, regs::CLEAR_RANGE_INT)]
    }

    fn max_khz(&self) -> u16 {
        MAX_KHZ
    }

    fn supports(&self) -> &'static [&'static str] {
        GROUPS
    }

    /// short / medium / long, narrowed per die by [`Vl53lxDriver::sensor_init`] (an L4 has no short).
    fn modes(&self) -> Vec<&'static str> {
        self.modes.to_vec()
    }

    /// The guard comes off the budget before it is divided and the range timeout is capped: 2..550 ms,
    /// 2..200 on an L4 die.
    fn budget_ms(&self) -> (u32, u32) {
        self.budget_range
    }

    fn histogram(&self) -> bool {
        true
    }

    fn model_id(&mut self) -> Result<u16> {
        self.p.read_word(0x010F)
    }

    /// Reset the part (whatever ran before left it ranging under another configuration), configure it
    /// for histogram ranging and raise the bus only after the FM+ pad byte has reached the device.
    fn sensor_init(&mut self) -> Result<()> {
        self.p.set_addr_width(self.addr_width())?;
        self.p.set_i2c_speed(I2C_KHZ_BOOT)?;
        self.p.xshut_reset()?;
        self.wait_boot(Duration::from_millis(1000))?;

        self.bare.data_init()?;
        // The die is known once NVM is read: an L4 has no short mode and a 200 ms budget ceiling.
        self.modes = self.bare.distance_modes_for_die();
        self.budget_range = (2, (self.bare.fda_max_timing_budget_us() / 1000) as u32);
        let mode = self.mode;
        self.set_mode(mode)?;
        self.p.write_byte(PAD_I2C_HV_CONFIG, FMP_PAD_CONFIG)?;
        if self.max_khz() != I2C_KHZ_BOOT {
            self.p.set_i2c_speed(self.max_khz())?;
        }
        Ok(())
    }

    fn start_ranging(&mut self) -> Result<()> {
        self.last_stream_count = None;
        self.history.reset();
        self.bare.load_patch()?;
        self.bare.init_and_start_range()?;
        // The interrupt line comes out of reset asserted, so the first data-ready would read the
        // untouched register file as a frame; one clear deasserts it (ST's
        // ClearInterruptAndStartMeasurement).
        self.bare.clear_interrupt_and_start_next_range()
    }

    fn stop_ranging(&mut self) -> Result<()> {
        self.bare.stop_range()?;
        self.bare.unload_patch()
    }

    fn check_for_data_ready(&mut self) -> Result<bool> {
        let int_pol = if (self.p.read_byte(GPIO_HV_MUX_CTRL)? & 0x10) >> 4 == 1 {
            0
        } else {
            1
        };
        Ok(self.p.read_byte(GPIO_TIO_HV_STATUS)? & 1 == int_pol)
    }

    fn clear_interrupt(&mut self) -> Result<()> {
        self.p
            .write_byte(SYSTEM_INTERRUPT_CLEAR, regs::CLEAR_RANGE_INT)
    }

    fn stream_block(&self) -> (u16, u16) {
        (
            regs::HISTOGRAM_BIN_DATA_I2C_INDEX,
            regs::HISTOGRAM_BIN_DATA_I2C_SIZE_BYTES,
        )
    }

    fn decode(&mut self, raw: &[u8]) -> Result<Vl53lxRange> {
        let bins = self.bin_data(Some(raw))?;
        self.to_measurement(&bins)
    }

    fn read_measurement(&mut self) -> Result<Vl53lxRange> {
        let bins = self.bin_data(None)?;
        self.to_measurement(&bins)
    }

    /// The budget covers the whole measurement; the range timeout is one sixth of it after a fixed
    /// guard. A budget outside [`Vl53lxDriver::budget_ms`] is an invalid argument.
    fn set_range_timing(&mut self, timing_budget_ms: u32, inter_measurement_ms: u32) -> Result<()> {
        let (low, high) = self.budget_range;
        if timing_budget_ms < low || timing_budget_ms > high {
            return Err(DepzError::InvalidArgument(format!(
                "timing_budget_ms must be {low}..{high}"
            )));
        }
        self.budget_ms = timing_budget_ms;
        self.inter_ms = inter_measurement_ms;
        self.bare
            .set_measurement_timing_budget_us(timing_budget_ms as i64 * 1000)?;
        // 0 means continuous; the device wants a period, and back-to-back is a period of zero.
        self.bare
            .set_inter_measurement_period_ms(inter_measurement_ms as i64)
    }

    fn get_range_timing(&mut self) -> Result<(u32, u32)> {
        let budget_us =
            self.bare.range_config_timeout_us() * regs::TIMING_DIVISOR + regs::TIMING_GUARD_US;
        Ok((
            (budget_us / 1000) as u32,
            self.bare.inter_measurement_period_ms() as u32,
        ))
    }

    /// The device preset mode: the VCSEL period, which sets where the phase wraps (1.6 / 2.4 / 4.0 m),
    /// and the valid phase window inside it ([`Vl53lxDriver::reach_mm`]). The bin is 199 mm wide in
    /// all three. A mode this die lacks is an invalid argument, refused before the bus is touched.
    fn set_mode(&mut self, name: &str) -> Result<Option<u32>> {
        let Some(mode) = self.modes.iter().find(|m| **m == name).copied() else {
            return Err(DepzError::InvalidArgument(format!(
                "no such mode: {name} (have {})",
                self.modes.join(", ")
            )));
        };
        self.mode = mode;
        self.bare.set_distance_mode(mode)?;
        // The preset rewrites the static config, pad byte included, and start_ranging() puts that image
        // on the wire: put FM+ back or that write would switch the pad off under a 1 MHz bus.
        self.bare
            .img_mut()
            .static_config_mut()
            .set("pad_i2c_hv__config", FMP_PAD_CONFIG as i64);
        self.set_range_timing(self.budget_ms, self.inter_ms)?;
        Ok(None)
    }

    fn get_mode(&mut self) -> Result<String> {
        Ok(self.mode.to_string())
    }

    /// The valid phase window the post-processing applies: 992 / 1785 / 3373 mm for short / medium /
    /// long at the usual oscillator. Past it the target is status 4. `None` before
    /// [`Vl53lxDriver::sensor_init`] (no oscillator frequency yet).
    fn reach_mm(&mut self) -> Result<Option<i32>> {
        let freq = self
            .bare
            .img()
            .static_nvm()
            .get("osc_measured__fast_osc__frequency");
        if freq <= 0 {
            return Ok(None);
        }
        Ok(Some(
            Self::phase_window_mm(freq, self.bare.hpp().valid_phase_high) as i32,
        ))
    }

    /// How long one range actually runs (`range timeout us`), and the MM1/MM2 offset the
    /// post-processing applies (`MM1/MM2 offset mm`).
    fn driver_info(&mut self) -> Result<BTreeMap<String, ExtraValue>> {
        let mut m = BTreeMap::new();
        m.insert(
            "range timeout us".to_string(),
            ExtraValue::Int(self.bare.range_config_timeout_us()),
        );
        m.insert(
            "MM1/MM2 offset mm".to_string(),
            ExtraValue::Float(self.bare.hpp().range_offset_mm as f64 / 4.0),
        );
        Ok(m)
    }

    /// The raw frame as a [`HistogramBinData`]; read here when `raw` is `None`.
    fn bin_data(&mut self, raw: Option<&[u8]>) -> Result<BinData> {
        let (addr, len) = self.stream_block();
        let read;
        let raw = match raw {
            Some(r) => r,
            None => {
                read = self.p.read_multi(addr, len as usize)?;
                &read[..]
            }
        };
        if raw.len() < len as usize {
            return Err(DepzError::InvalidArgument(format!(
                "histogram frame is {} bytes, need {}",
                raw.len(),
                len
            )));
        }
        let stream_count = raw[3]; // result__stream_count
        if self.last_stream_count != Some(stream_count) {
            // What clear_interrupt_and_start_next_range() does to the state machine, without the register
            // writes: one frame on, which flips the A/B timing status and steps the bin sequence.
            if self.last_stream_count.is_some() {
                let mode_start = self.bare.img().system_control().get("system__mode_start");
                let gph = self
                    .bare
                    .img()
                    .dynamic_config()
                    .get("system__grouped_parameter_hold");
                self.bare.state_mut().update_rd(mode_start, gph);
            }
            self.last_stream_count = Some(stream_count);
        }
        Ok(Box::new(self.bare.get_histogram_bin_data(Some(raw))?))
    }

    /// A frame → its range. Once per frame: the history is a state machine and would step twice.
    fn to_measurement(&mut self, bins: &BinData) -> Result<Vl53lxRange> {
        let b = bins.downcast_ref::<HistogramBinData>().ok_or_else(|| {
            DepzError::InvalidArgument("not a frame of the histogram driver".into())
        })?;
        let mut results = hist::process_data(b, self.bare.hpp())?;
        let rd_state = self.bare.state().rd_device_state;
        self.history
            .apply(&mut results, b, rd_state, self.bare.hpp());
        Ok(hist::to_range(&results, b))
    }
}
