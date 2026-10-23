//! VL53L4CD single-zone ToF (contract 10): the ST ULD 2.2.3 on the host over the register bridge.

use std::ops::Deref;
use std::sync::atomic::{AtomicBool, AtomicU64, Ordering};
use std::sync::Arc;
use std::time::Duration;

use super::device::{expect_report, DepzDevice, ReportHandler, Requester};
use super::error::{DepzError, Result};
use super::stream::{Fanout, Hook, Subscription};
use crate::framing::Packet;
use crate::vl53l4::driver::{DetectionThresholds, Vl53l4Platform, Vl53l4cdDriver};
use crate::vl53l4::uld_math::{range_status_name, Vl53l4Results, I2C_KHZ_DEFAULT, RESULT_BLOCK_ADDR, RESULT_BLOCK_LEN};
use crate::vl53l4::{
    pack_read_reg, pack_set_i2c_speed, pack_start_stream, pack_write_reg, pack_xshut, parse_result_block, RegData,
    StreamData, Vl53l4Cmd, Vl53l4Info, Vl53l4Rpt, XFER_MAX, XSHUT_RESET,
};

const REG_TIMEOUT: Duration = Duration::from_secs(2);
const STREAM_MIN_BYTES: usize = 12;

/// One VL53L4CD result block with the MCU timestamp of the read or the INT edge.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct Vl53l4Measurement {
    pub timestamp_us: u64,
    pub range_status: u8,
    pub distance_mm: u16,
    pub sigma_mm: u16,
    pub signal_rate_kcps: u32,
    pub ambient_rate_kcps: u32,
    pub signal_per_spad_kcps: u32,
    pub ambient_per_spad_kcps: u32,
    pub number_of_spad: u16,
    pub stream_count: u8,
}

impl Vl53l4Measurement {
    pub fn valid(&self) -> bool {
        self.range_status == 0
    }

    /// What `range_status` means ("unknown (N)" for a code the table lacks).
    pub fn status_text(&self) -> String {
        match range_status_name(self.range_status) {
            "unknown" => format!("unknown ({})", self.range_status),
            s => s.to_string(),
        }
    }

    fn from(timestamp_us: u64, r: &Vl53l4Results) -> Vl53l4Measurement {
        Vl53l4Measurement {
            timestamp_us,
            range_status: r.range_status,
            distance_mm: r.distance_mm,
            sigma_mm: r.sigma_mm,
            signal_rate_kcps: r.signal_rate_kcps,
            ambient_rate_kcps: r.ambient_rate_kcps,
            signal_per_spad_kcps: r.signal_per_spad_kcps,
            ambient_per_spad_kcps: r.ambient_per_spad_kcps,
            number_of_spad: r.number_of_spad,
            stream_count: r.stream_count,
        }
    }
}

struct Streams {
    measurements: Fanout<Vl53l4Measurement>,
    parse_errors: AtomicU64,
}

impl ReportHandler for Streams {
    fn handle_report(&self, packet: &Packet) -> bool {
        if packet.cmd != Vl53l4Rpt::Stream as u8 || packet.payload.len() < STREAM_MIN_BYTES {
            return false;
        }
        // A stream re-armed on another block, or a corrupt read: count it, keep the reader.
        match StreamData::unpack(&packet.payload).ok().and_then(|s| parse_result_block(&s.data).ok().map(|r| (s, r))) {
            Some((s, r)) => self.measurements.emit(Vl53l4Measurement::from(s.timestamp_us, &r)),
            None => {
                self.parse_errors.fetch_add(1, Ordering::Relaxed);
            }
        }
        true
    }

    fn reader_stopped(&self) {
        self.measurements.close_all();
    }
}

// The ULD platform on the firmware register bridge.
struct BridgePlatform {
    dev: Requester,
    last_timestamp_us: AtomicU64,
}

impl Vl53l4Platform for BridgePlatform {
    fn read_multi(&self, addr: u16, size: usize) -> Result<Vec<u8>> {
        let mut out = Vec::with_capacity(size);
        let mut addr = addr;
        while out.len() < size {
            let n = (size - out.len()).min(XFER_MAX);
            let rep: RegData = self.dev.request(
                Vl53l4Cmd::ReadReg as u8,
                &pack_read_reg(addr, n as u16),
                expect_report(Vl53l4Rpt::RegData as u8, RegData::unpack),
                Some(REG_TIMEOUT),
            )?;
            if rep.data.len() != n {
                return Err(DepzError::Protocol(format!("READ_REG 0x{addr:04X}: expected {n}, got {}", rep.data.len())));
            }
            self.last_timestamp_us.store(rep.timestamp_us, Ordering::Relaxed);
            out.extend_from_slice(&rep.data);
            addr = addr.wrapping_add(n as u16);
        }
        Ok(out)
    }

    fn write_multi(&self, addr: u16, data: &[u8]) -> Result<()> {
        let mut addr = addr;
        for chunk in data.chunks(XFER_MAX) {
            self.dev.request_ok(Vl53l4Cmd::WriteReg as u8, &pack_write_reg(addr, chunk), Some(REG_TIMEOUT))?;
            addr = addr.wrapping_add(chunk.len() as u16);
        }
        Ok(())
    }

    fn set_i2c_speed(&self, khz: u16) -> Result<()> {
        self.dev.request_ok(Vl53l4Cmd::SetI2cSpeed as u8, &pack_set_i2c_speed(khz), None)
    }
}

/// VL53L4CD single-zone ToF (contract 10). The board is a thin I2C bridge; the ST ULD 2.2.3 runs here
/// ([`Vl53l4cdDevice::uld`]). [`Vl53l4cdDevice::init`] first, then configure and
/// [`Vl53l4cdDevice::start_ranging`]: one result block per INT edge streams to
/// [`Vl53l4cdDevice::measurements`]. Configuration while ranging fails with
/// [`DepzError::InvalidState`] — the stream owns the register bank.
pub struct Vl53l4cdDevice {
    dev: DepzDevice,
    streams: Arc<Streams>,
    platform: Arc<BridgePlatform>,
    uld: Vl53l4cdDriver,
    ranging: AtomicBool,
    initialized: AtomicBool,
}

impl Deref for Vl53l4cdDevice {
    type Target = DepzDevice;

    fn deref(&self) -> &DepzDevice {
        &self.dev
    }
}

impl Vl53l4cdDevice {
    /// Continue the open connection `dev` as a VL53L4CD.
    pub fn attach(dev: DepzDevice) -> Vl53l4cdDevice {
        let streams = Arc::new(Streams { measurements: Fanout::new(), parse_errors: AtomicU64::new(0) });
        dev.set_report_handler(streams.clone());
        let platform = Arc::new(BridgePlatform { dev: dev.requester(), last_timestamp_us: AtomicU64::new(0) });
        let uld = Vl53l4cdDriver::new(platform.clone());
        Vl53l4cdDevice { dev, streams, platform, uld, ranging: AtomicBool::new(false), initialized: AtomicBool::new(false) }
    }

    /// The ULD driver (escape hatch for raw register access).
    pub fn uld(&self) -> &Vl53l4cdDriver {
        &self.uld
    }

    /// True after a successful [`Vl53l4cdDevice::init`]; cleared by [`Vl53l4cdDevice::xshut`].
    pub fn initialized(&self) -> bool {
        self.initialized.load(Ordering::SeqCst)
    }

    pub fn ranging(&self) -> bool {
        self.ranging.load(Ordering::SeqCst)
    }

    /// True when the sensor answers with the VL53L4CD model id (0xEBAA).
    pub fn is_alive(&self) -> bool {
        self.uld.is_alive().unwrap_or(false)
    }

    /// Default configuration block + VHV calibration (ULD sensor_init); the bus is left at `bus_khz`
    /// (`None`: 1000).
    pub fn init(&self, bus_khz: Option<u16>) -> Result<()> {
        self.require_not_ranging()?;
        self.uld.sensor_init(bus_khz.unwrap_or(I2C_KHZ_DEFAULT))?;
        self.initialized.store(true, Ordering::SeqCst);
        Ok(())
    }

    // ── sensor power (XSHUT) and the bridge ──────────────────────────────────

    /// Drive XSHUT: off (0) / on (1) / reset (2). Off and reset stop a stream; init again after.
    pub fn xshut(&self, action: u8) -> Result<()> {
        if action > XSHUT_RESET {
            return Err(DepzError::InvalidArgument(format!("XSHUT action must be 0 (off), 1 (on) or 2 (reset), got {action}")));
        }
        self.request_ok(Vl53l4Cmd::Xshut as u8, &pack_xshut(action), if action == XSHUT_RESET { Some(REG_TIMEOUT) } else { None })?;
        self.ranging.store(false, Ordering::SeqCst);
        self.initialized.store(false, Ordering::SeqCst);
        Ok(())
    }

    /// Hardware reset through XSHUT (~3 ms on the MCU); init again.
    pub fn reset_sensor(&self) -> Result<()> {
        self.xshut(XSHUT_RESET)
    }

    /// RPT_VL53_INFO: sensor identity, pin levels and free-running bridge counters. Safe while streaming.
    pub fn bridge_info(&self) -> Result<Vl53l4Info> {
        self.request(Vl53l4Cmd::GetInfo as u8, &[], expect_report(Vl53l4Rpt::Info as u8, Vl53l4Info::unpack), None)
    }

    /// Re-time the bridge's I2C bus to the step nearest `khz`. Not while ranging.
    pub fn set_i2c_speed_khz(&self, khz: u16) -> Result<()> {
        self.require_not_ranging()?;
        self.platform.set_i2c_speed(khz)
    }

    // ── configuration (init first; not while ranging) ───────────────────────

    /// (timing budget ms, inter-measurement ms; 0 = continuous).
    pub fn get_range_timing(&self) -> Result<(u32, u32)> {
        self.uld.get_range_timing()
    }

    /// Timing budget 10–200 ms; inter-measurement 0 = continuous, larger than the budget = autonomous
    /// low power.
    pub fn set_range_timing(&self, timing_budget_ms: u32, inter_measurement_ms: u32) -> Result<()> {
        self.require_not_ranging()?;
        self.uld.set_range_timing(timing_budget_ms, inter_measurement_ms)
    }

    pub fn get_offset_mm(&self) -> Result<i32> {
        self.uld.get_offset()
    }

    pub fn set_offset_mm(&self, offset_mm: i32) -> Result<()> {
        self.require_not_ranging()?;
        self.uld.set_offset(offset_mm)
    }

    /// Crosstalk compensation, kcps (0 = off).
    pub fn get_xtalk_kcps(&self) -> Result<u16> {
        self.uld.get_xtalk()
    }

    pub fn set_xtalk_kcps(&self, xtalk_kcps: u16) -> Result<()> {
        self.require_not_ranging()?;
        self.uld.set_xtalk(xtalk_kcps)
    }

    pub fn get_detection_thresholds(&self) -> Result<DetectionThresholds> {
        self.uld.get_detection_thresholds()
    }

    /// The distance-window interrupt: INT only fires while the window holds.
    pub fn set_detection_thresholds(&self, distance_low_mm: u16, distance_high_mm: u16, window: u8) -> Result<()> {
        self.require_not_ranging()?;
        self.uld.set_detection_thresholds(distance_low_mm, distance_high_mm, window)
    }

    pub fn get_signal_threshold_kcps(&self) -> Result<u16> {
        self.uld.get_signal_threshold()
    }

    /// Drop measurements whose return signal is below `signal_kcps`.
    pub fn set_signal_threshold_kcps(&self, signal_kcps: u16) -> Result<()> {
        self.require_not_ranging()?;
        self.uld.set_signal_threshold(signal_kcps)
    }

    pub fn get_sigma_threshold_mm(&self) -> Result<u16> {
        self.uld.get_sigma_threshold()
    }

    /// Drop measurements whose sigma exceeds `sigma_mm` (≤ 16383).
    pub fn set_sigma_threshold_mm(&self, sigma_mm: u16) -> Result<()> {
        self.require_not_ranging()?;
        self.uld.set_sigma_threshold(sigma_mm)
    }

    /// Re-run VHV calibration, recommended after a >8 °C ambient change.
    pub fn start_temperature_update(&self) -> Result<()> {
        self.require_not_ranging()?;
        self.uld.start_temperature_update()
    }

    /// Offset calibration against a target at `target_dist_mm` (10–1000 mm); returns the offset now
    /// programmed. `nb_samples` `None`: 20.
    pub fn calibrate_offset(&self, target_dist_mm: i32, nb_samples: Option<u32>) -> Result<i32> {
        self.require_not_ranging()?;
        self.uld.calibrate_offset(target_dist_mm, nb_samples.unwrap_or(20))
    }

    /// Crosstalk calibration against a target at `target_dist_mm` (10–5000 mm); returns the xtalk now
    /// programmed, kcps. `nb_samples` `None`: 20.
    pub fn calibrate_xtalk(&self, target_dist_mm: i32, nb_samples: Option<u32>) -> Result<i32> {
        self.require_not_ranging()?;
        self.uld.calibrate_xtalk(target_dist_mm, nb_samples.unwrap_or(20))
    }

    // ── ranging ──────────────────────────────────────────────────────────────

    /// Start the sensor and arm the MCU stream: one RPT_VL53_STREAM per INT edge.
    pub fn start_ranging(&self) -> Result<()> {
        self.require_not_ranging()?;
        self.uld.start_ranging()?;
        self.request_ok(Vl53l4Cmd::StartStream as u8, &pack_start_stream(RESULT_BLOCK_ADDR, RESULT_BLOCK_LEN, 0), None)?;
        self.ranging.store(true, Ordering::SeqCst);
        Ok(())
    }

    pub fn stop_ranging(&self) -> Result<()> {
        if !self.ranging() {
            return Ok(());
        }
        // Leave the host state "not ranging" and stop the sensor even when the STOP_STREAM ack fails,
        // or no reconfiguration would ever be possible again.
        let r = self.request_ok(Vl53l4Cmd::StopStream as u8, &[], None);
        self.ranging.store(false, Ordering::SeqCst);
        let s = self.uld.stop_ranging();
        r.and(s)
    }

    /// One polled measurement: start, wait for data ready, read, stop. Not while streaming. `timeout`
    /// `None`: 1 s.
    pub fn measure_once(&self, timeout: Option<Duration>) -> Result<Vl53l4Measurement> {
        self.require_not_ranging()?;
        let timeout_ms = timeout.map_or(1000, |t| t.as_millis() as u32);
        self.uld.start_ranging()?;
        let r = (|| -> Result<Vl53l4Results> {
            self.uld.wait_data_ready(timeout_ms)?;
            let r = self.uld.get_result()?;
            self.uld.clear_interrupt()?;
            Ok(r)
        })();
        let stop = self.uld.stop_ranging();
        let r = r?;
        stop?;
        Ok(Vl53l4Measurement::from(self.platform.last_timestamp_us.load(Ordering::Relaxed), &r))
    }

    /// Call `callback` for every streamed measurement (reader thread).
    pub fn on_measurement(&self, callback: impl Fn(&Vl53l4Measurement) + Send + Sync + 'static) -> Hook {
        self.streams.measurements.on_item(callback)
    }

    /// A pull queue of streamed measurements (bounded, drop-oldest).
    pub fn measurements(&self, capacity: usize) -> Subscription<Vl53l4Measurement> {
        self.streams.measurements.subscribe(capacity)
    }

    /// The next streamed measurement (contract 07 §3); `timeout` `None`: 2 s.
    pub fn get_measurement(&self, timeout: Option<Duration>) -> Result<Vl53l4Measurement> {
        self.streams.measurements.next(timeout.unwrap_or(Duration::from_secs(2)), Vl53l4Rpt::Stream as u8, self.is_closed())
    }

    /// Stream reports dropped because their result block did not decode.
    pub fn stream_parse_errors(&self) -> u64 {
        self.streams.parse_errors.load(Ordering::Relaxed)
    }

    pub fn stream_dropped_counts(&self) -> Vec<u64> {
        self.streams.measurements.dropped_counts()
    }

    fn require_not_ranging(&self) -> Result<()> {
        if self.ranging() {
            return Err(DepzError::InvalidState("stop_ranging() first — the stream owns the register bank".into()));
        }
        Ok(())
    }
}
