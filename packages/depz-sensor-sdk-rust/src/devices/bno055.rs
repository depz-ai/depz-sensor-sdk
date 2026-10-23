//! BNO055 absolute-orientation IMU (contract 13): the session logic over the register bridge.

use std::ops::Deref;
use std::sync::atomic::{AtomicBool, AtomicU64, Ordering};
use std::sync::{Arc, Mutex, MutexGuard};
use std::time::{Duration, Instant};

use super::device::{expect_report, DepzDevice, ReportHandler, Requester};
use super::error::{DepzError, Result};
use super::stream::{Fanout, Hook, Subscription};
use crate::bno055::regs::{self as r, CalibStatus, CalibrationProfile, OprMode, PwrMode, TempSource, Units};
use crate::bno055::{
    decode_block, pack_read_reg, pack_start_stream, pack_write_reg, AccelConfig, AxisRemap, Bno055Cmd, Bno055Info,
    Bno055Rpt, GyroConfig, MagConfig, RegData, StreamData, TRIGGER_INT, TRIGGER_TIMER, XFER_MAX,
};
use crate::framing::Packet;

const STREAM_MIN_BYTES: usize = 10;
/// BNO_RESET answers after the sensor's ~0.5 s boot handshake.
const RESET_TIMEOUT: Duration = Duration::from_secs(3);
/// Settle time after an OPR_MODE write out of CONFIG (datasheet: 7 ms).
pub const MODE_SWITCH_FROM_CONFIG: Duration = Duration::from_millis(10);
/// Settle time after an OPR_MODE write into CONFIG (datasheet: 19 ms).
pub const MODE_SWITCH_TO_CONFIG: Duration = Duration::from_millis(25);
/// The built-in self-test runs ~400 ms (datasheet §3.9.2).
pub const SELF_TEST_TIME: Duration = Duration::from_millis(450);
/// How long the sensor's own boot may take.
pub const BOOT_SETTLE_TIMEOUT: Duration = Duration::from_secs(1);
/// SYS_STATUS 4 polls in a row taken as the leftover of a self-test run in CONFIG, not POST.
pub const STUCK_SELF_TEST_POLLS: u32 = 20;
/// After CONFIG → a fusion mode the fusion outputs read zero for ~70 ms.
pub const FUSION_START_TIMEOUT: Duration = Duration::from_secs(1);

/// ST_RESULT (0x36) + SYS_CLK_STATUS / SYS_STATUS / SYS_ERR (0x38..0x3A); INT_STA (0x37) sits in
/// between and clears on read, so it is not read.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct Bno055SystemStatus {
    pub self_test: u8,
    pub clk_status: u8,
    pub status: u8,
    pub error: u8,
}

impl Bno055SystemStatus {
    pub fn status_text(&self) -> String {
        match self.status {
            0 => "idle".into(),
            1 => "system error".into(),
            2 => "initializing peripherals".into(),
            3 => "system initialization".into(),
            4 => "executing self-test".into(),
            5 => "fusion algorithm running".into(),
            6 => "running without fusion".into(),
            s => format!("unknown ({s})"),
        }
    }

    /// Meaningful when `status` is 1 (system error).
    pub fn error_text(&self) -> String {
        const NAMES: [&str; 11] = [
            "no error",
            "peripheral initialization error",
            "system initialization error",
            "self-test failed",
            "register map value out of range",
            "register map address out of range",
            "register map write error",
            "low power mode not available for this operation mode",
            "accelerometer power mode not available",
            "fusion algorithm configuration error",
            "sensor configuration error",
        ];
        NAMES.get(self.error as usize).map_or_else(|| format!("unknown ({})", self.error), |s| s.to_string())
    }

    pub fn self_test_passed(&self) -> bool {
        self.self_test & r::EXPECTED_SELF_TEST == r::EXPECTED_SELF_TEST
    }
}

/// One decoded BNO055 register block (streamed or polled). A channel is `None` when the block did not
/// cover it; values are scaled by `units`, the units in effect when the stream started. Euler is
/// (heading, roll, pitch), the quaternion (w, x, y, z); linear acceleration and gravity are always
/// m/s². In non-fusion modes the fusion outputs read zero; in CONFIG everything does.
#[derive(Debug, Clone, PartialEq)]
pub struct Bno055Sample {
    pub timestamp_us: u64,
    pub addr: u8,
    pub raw: Vec<u8>,
    pub units: Units,
    pub accel: Option<[f64; 3]>,
    pub mag: Option<[f64; 3]>,
    pub gyro: Option<[f64; 3]>,
    pub euler: Option<[f64; 3]>,
    pub quaternion: Option<[f64; 4]>,
    pub linear_accel: Option<[f64; 3]>,
    pub gravity: Option<[f64; 3]>,
    pub temperature: Option<f64>,
    pub calibration: Option<CalibStatus>,
}

fn scale3(v: Option<[i16; 3]>, lsb: f64) -> Option<[f64; 3]> {
    v.map(|a| [a[0] as f64 / lsb, a[1] as f64 / lsb, a[2] as f64 / lsb])
}

impl Bno055Sample {
    /// Decode `data` read at `addr` with `units`.
    pub fn decode(timestamp_us: u64, addr: u8, data: &[u8], units: Units) -> Bno055Sample {
        let b = decode_block(addr, data);
        Bno055Sample {
            timestamp_us,
            addr,
            raw: data.to_vec(),
            units,
            accel: scale3(b.accel, units.accel_lsb()),
            mag: scale3(b.mag, r::MAG_LSB),
            gyro: scale3(b.gyro, units.gyro_lsb()),
            euler: scale3(b.euler, units.euler_lsb()),
            quaternion: b.quaternion.map(|q| q.map(|x| x as f64 / r::QUAT_LSB)),
            linear_accel: scale3(b.linear_accel, r::FUSION_ACCEL_LSB),
            gravity: scale3(b.gravity, r::FUSION_ACCEL_LSB),
            temperature: b.temperature.map(|t| t as f64 / units.temp_lsb()),
            calibration: b.calib_stat.map(CalibStatus::unpack),
        }
    }
}

struct Streams {
    samples: Fanout<Bno055Sample>,
    parse_errors: AtomicU64,
    units: Mutex<Units>,
}

impl ReportHandler for Streams {
    fn handle_report(&self, packet: &Packet) -> bool {
        if packet.cmd != Bno055Rpt::Stream as u8 {
            return false;
        }
        // shorter than its own length field: refused
        let s = if packet.payload.len() < STREAM_MIN_BYTES { None } else { StreamData::unpack(&packet.payload).ok() };
        match s {
            Some(s) => {
                let units = *self.units.lock().unwrap();
                self.samples.emit(Bno055Sample::decode(s.timestamp_us, s.addr, &s.data, units));
            }
            None => {
                self.parse_errors.fetch_add(1, Ordering::Relaxed);
            }
        }
        true
    }

    fn reader_stopped(&self) {
        self.samples.close_all();
    }
}

#[derive(Clone, Copy)]
struct Configured {
    mode: OprMode,
    units: Units,
    remap: Option<AxisRemap>,
    calibration: Option<CalibrationProfile>,
}

struct State {
    units: Option<Units>,
    configured: Option<Configured>,
    mode_before_suspend: Option<OprMode>,
}

/// BNO055 absolute-orientation IMU (contract 13). The board is a thin I2C register bridge: mode,
/// units, axis remap and calibration are register access here; the sensor fuses on chip at 100 Hz.
/// Typical use: [`Bno055Device::configure`], then [`Bno055Device::start_stream`] and read
/// [`Bno055Device::samples`], or poll [`Bno055Device::read_sample`]. Page 1 (sensor configs,
/// interrupts, unique id) is reached through PAGE_ID and refused while streaming.
pub struct Bno055Device {
    dev: DepzDevice,
    req: Requester,
    streams: Arc<Streams>,
    // One lock serialises multi-step register sequences (page switch + access, CONFIG round trips).
    state: Mutex<State>,
    streaming: AtomicBool,
}

impl Deref for Bno055Device {
    type Target = DepzDevice;

    fn deref(&self) -> &DepzDevice {
        &self.dev
    }
}

type Locked<'a> = MutexGuard<'a, State>;

impl Bno055Device {
    /// Continue the open connection `dev` as a BNO055.
    pub fn attach(dev: DepzDevice) -> Bno055Device {
        let streams = Arc::new(Streams {
            samples: Fanout::new(),
            parse_errors: AtomicU64::new(0),
            units: Mutex::new(Units::unpack(0)),
        });
        dev.set_report_handler(streams.clone());
        let req = dev.requester();
        Bno055Device {
            dev,
            req,
            streams,
            state: Mutex::new(State { units: None, configured: None, mode_before_suspend: None }),
            streaming: AtomicBool::new(false),
        }
    }

    // ── identity & bridge ────────────────────────────────────────────────────

    /// RPT_BNO_INFO: chip ids, sensor firmware revision, bridge counters. Safe while streaming.
    pub fn bridge_info(&self) -> Result<Bno055Info> {
        self.request(Bno055Cmd::GetInfo as u8, &[], expect_report(Bno055Rpt::Info as u8, Bno055Info::unpack), None)
    }

    /// True when the bridge passed the chip-id handshake and the sensor answers with the BNO055 ids.
    pub fn is_alive(&self) -> bool {
        self.bridge_info().map(|i| i.initialized != 0 && i.ids_ok()).unwrap_or(false)
    }

    /// Hardware reset through nRESET (the bridge answers after the ~0.5 s boot handshake). Stops a
    /// stream; the sensor returns in CONFIG with power-on units and no calibration — configure again,
    /// or [`Bno055Device::restore_configuration`].
    pub fn reset_sensor(&self) -> Result<()> {
        let mut st = self.state.lock().unwrap();
        self.request_ok(Bno055Cmd::Reset as u8, &[], Some(RESET_TIMEOUT))?;
        self.streaming.store(false, Ordering::SeqCst);
        st.units = None;
        self.wait_booted(&mut st)
    }

    // ── raw register access ──────────────────────────────────────────────────

    /// Read `length` bytes at `addr` on `page` (0 or 1; 128-byte transfers). Page 1 is refused while
    /// streaming.
    pub fn read_registers(&self, addr: u8, length: usize, page: u8) -> Result<Vec<u8>> {
        let _st = self.state.lock().unwrap();
        self.on_page(page, || Ok(self.read(addr, length)?.1))
    }

    /// Write at `addr` on `page`. Most configuration registers only take writes in CONFIG mode (the
    /// sensor ignores the rest); the typed setters handle that.
    pub fn write_registers(&self, addr: u8, data: &[u8], page: u8) -> Result<()> {
        let _st = self.state.lock().unwrap();
        self.on_page(page, || self.write(addr, data))
    }

    pub fn read_register(&self, addr: u8, page: u8) -> Result<u8> {
        Ok(self.read_registers(addr, 1, page)?[0])
    }

    pub fn write_register(&self, addr: u8, value: u8, page: u8) -> Result<()> {
        self.write_registers(addr, &[value], page)
    }

    // ── mode, power, units, axes ─────────────────────────────────────────────

    pub fn get_operation_mode(&self) -> Result<OprMode> {
        opr_mode(self.read_register(r::REG_OPR_MODE, 0)?)
    }

    /// Switch OPR_MODE and wait out the switching time; into a fusion mode also wait (up to 1 s) for
    /// the fusion outputs. The sensor only switches between CONFIG and another mode, so a switch
    /// between two operating modes goes through CONFIG.
    pub fn set_operation_mode(&self, mode: OprMode) -> Result<()> {
        let _st = self.state.lock().unwrap();
        if mode != OprMode::Config {
            let current = self.current_mode()?;
            if current == mode {
                return Ok(());
            }
            if current != OprMode::Config {
                self.switch_mode(OprMode::Config)?;
            }
        }
        self.switch_mode(mode)
    }

    pub fn get_power_mode(&self) -> Result<PwrMode> {
        pwr_mode(self.read_register(r::REG_PWR_MODE, 0)?)
    }

    /// Normal / low power / suspend, written in CONFIG with the operating mode restored after. A
    /// suspended chip ignores OPR_MODE writes, so the mode is kept here and put back on wake-up (NDOF
    /// when the chip was already asleep at connect, contract 13 §4.6).
    pub fn set_power_mode(&self, mode: PwrMode) -> Result<()> {
        let mut st = self.state.lock().unwrap();
        let opr = self.current_mode()?;
        let pwr = self.read(r::REG_PWR_MODE, 1)?.1[0] & 0x03;
        let resume = if opr != OprMode::Config {
            self.switch_mode(OprMode::Config)?;
            Some(opr)
        } else if pwr == PwrMode::Suspend as u8 {
            Some(st.mode_before_suspend.unwrap_or(OprMode::Ndof))
        } else {
            None // CONFIG on purpose: stay there
        };
        self.write(r::REG_PWR_MODE, &[mode as u8])?;
        if mode == PwrMode::Suspend {
            st.mode_before_suspend = resume;
        } else {
            st.mode_before_suspend = None;
            if let Some(m) = resume {
                self.switch_mode(m)?;
            }
        }
        Ok(())
    }

    pub fn get_units(&self) -> Result<Units> {
        let u = Units::unpack(self.read_register(r::REG_UNIT_SEL, 0)?);
        self.state.lock().unwrap().units = Some(u);
        Ok(u)
    }

    pub fn set_units(&self, units: Units) -> Result<()> {
        let mut st = self.state.lock().unwrap();
        self.in_config_mode(|| self.write(r::REG_UNIT_SEL, &[units.pack()]))?;
        st.units = Some(units);
        Ok(())
    }

    pub fn get_axis_remap(&self) -> Result<AxisRemap> {
        let b = self.read_registers(r::REG_AXIS_MAP_CONFIG, 2, 0)?;
        Ok(AxisRemap::unpack(b[0], b[1]))
    }

    /// Remap the output axes for the board's mounting (`AxisRemap::placement` gives the datasheet
    /// placements P0..P7; P1 is the default).
    pub fn set_axis_remap(&self, remap: &AxisRemap) -> Result<()> {
        let (config, sign) = remap.pack().map_err(|e| DepzError::InvalidArgument(e.0.to_string()))?;
        let _st = self.state.lock().unwrap();
        self.in_config_mode(|| self.write(r::REG_AXIS_MAP_CONFIG, &[config, sign]))
    }

    pub fn get_temperature_source(&self) -> Result<TempSource> {
        Ok(if self.read_register(r::REG_TEMP_SOURCE, 0)? & 0x03 == TempSource::Gyro as u8 {
            TempSource::Gyro
        } else {
            TempSource::Accel
        })
    }

    pub fn set_temperature_source(&self, source: TempSource) -> Result<()> {
        let _st = self.state.lock().unwrap();
        self.in_config_mode(|| self.write(r::REG_TEMP_SOURCE, &[source as u8]))
    }

    /// The usual session setup: CONFIG → units → axis remap → calibration profile → `mode`; in a
    /// fusion mode it returns once the fusion outputs are live (~70 ms). Remembered for
    /// [`Bno055Device::restore_configuration`]. `units` `None`: the power-on units; `axis_remap` /
    /// `calibration` `None` leave the sensor's.
    pub fn configure(
        &self,
        mode: OprMode,
        units: Option<Units>,
        axis_remap: Option<AxisRemap>,
        calibration: Option<CalibrationProfile>,
    ) -> Result<()> {
        let u = units.unwrap_or_else(|| Units::unpack(0));
        let remap = match &axis_remap {
            Some(a) => Some(a.pack().map_err(|e| DepzError::InvalidArgument(e.0.to_string()))?),
            None => None,
        };
        let mut st = self.state.lock().unwrap();
        self.wait_booted(&mut st)?;
        self.switch_mode(OprMode::Config)?;
        self.write(r::REG_UNIT_SEL, &[u.pack()])?;
        st.units = Some(u);
        if let Some((config, sign)) = remap {
            self.write(r::REG_AXIS_MAP_CONFIG, &[config, sign])?;
        }
        if let Some(c) = &calibration {
            self.write(r::REG_CALIB_PROFILE, &c.pack())?;
        }
        self.switch_mode(mode)?;
        if mode.is_fusion() {
            self.wait_fusion_started(true)?;
        }
        st.configured = Some(Configured { mode, units: u, remap: axis_remap, calibration });
        Ok(())
    }

    /// Re-apply the last [`Bno055Device::configure`] — after [`Bno055Device::reset_sensor`], or when
    /// `bridge_info().sensor_resets` rose (bus recovery pulses nRESET). A running stream keeps running.
    pub fn restore_configuration(&self) -> Result<()> {
        let c = self.state.lock().unwrap().configured;
        match c {
            Some(c) => self.configure(c.mode, Some(c.units), c.remap, c.calibration),
            None => Err(DepzError::InvalidState("configure() has not been called".into())),
        }
    }

    // ── status, self-test, calibration ───────────────────────────────────────

    /// ST_RESULT, SYS_CLK_STATUS, SYS_STATUS, SYS_ERR.
    pub fn system_status(&self) -> Result<Bno055SystemStatus> {
        let _st = self.state.lock().unwrap();
        self.system_status_locked()
    }

    fn system_status_locked(&self) -> Result<Bno055SystemStatus> {
        let st = self.read(r::REG_ST_RESULT, 1)?.1[0];
        let b = self.read(r::REG_SYS_CLK_STATUS, 3)?.1;
        Ok(Bno055SystemStatus { self_test: st, clk_status: b[0], status: b[1], error: b[2] })
    }

    /// The built-in self-test (datasheet §3.9.2), in CONFIG, ~400 ms; the operating mode is restored
    /// after. Not while streaming.
    pub fn self_test(&self) -> Result<Bno055SystemStatus> {
        self.require_not_streaming()?;
        let _st = self.state.lock().unwrap();
        let previous = self.current_mode()?;
        if previous != OprMode::Config {
            self.switch_mode(OprMode::Config)?;
        }
        let r = (|| -> Result<Bno055SystemStatus> {
            let trigger = self.read(r::REG_SYS_TRIGGER, 1)?.1[0] & r::SYS_TRIGGER_CLK_SEL;
            self.write(r::REG_SYS_TRIGGER, &[trigger | r::SYS_TRIGGER_SELF_TEST])?;
            std::thread::sleep(SELF_TEST_TIME);
            let status = self.system_status_locked()?;
            // Staying in CONFIG would leave SYS_STATUS at 4 for good.
            if previous == OprMode::Config {
                self.clear_self_test_status()?;
            }
            Ok(status)
        })();
        if previous != OprMode::Config {
            self.switch_mode(previous)?;
        }
        r
    }

    /// CALIB_STAT: 0..3 per sensor; in NDOF the sensor calibrates in the background.
    pub fn calibration_status(&self) -> Result<CalibStatus> {
        Ok(CalibStatus::unpack(self.read_register(r::REG_CALIB_STAT, 0)?))
    }

    /// Offsets and radii (0x55..0x6A), only readable in CONFIG: the driver switches there and back.
    pub fn read_calibration_profile(&self) -> Result<CalibrationProfile> {
        let _st = self.state.lock().unwrap();
        let data = self.in_config_mode(|| Ok(self.read(r::REG_CALIB_PROFILE, r::CALIB_PROFILE_LEN)?.1))?;
        Ok(CalibrationProfile::unpack(&data)?)
    }

    /// Restore a stored profile (datasheet §3.11.5). A starting point, not a lock: the background
    /// calibration refines it once fusion resumes.
    pub fn write_calibration_profile(&self, profile: &CalibrationProfile) -> Result<()> {
        let packed = profile.pack();
        let _st = self.state.lock().unwrap();
        self.in_config_mode(|| self.write(r::REG_CALIB_PROFILE, &packed))
    }

    /// Soft-iron compensation matrix, 9 × i16 row-major, 1.0 = 16384.
    pub fn get_sic_matrix(&self) -> Result<[i16; 9]> {
        Ok(r::unpack_sic_matrix(&self.read_registers(r::REG_SIC_MATRIX, 18, 0)?)?)
    }

    /// `matrix` `None` writes the identity.
    pub fn set_sic_matrix(&self, matrix: Option<[i16; 9]>) -> Result<()> {
        let packed = r::pack_sic_matrix(&matrix.unwrap_or(r::SIC_IDENTITY));
        let _st = self.state.lock().unwrap();
        self.in_config_mode(|| self.write(r::REG_SIC_MATRIX, &packed))
    }

    // ── page 1: raw sensor configuration (non-fusion modes) ──────────────────

    pub fn get_accel_config(&self) -> Result<AccelConfig> {
        Ok(AccelConfig::unpack(self.read_register(r::REG1_ACC_CONFIG, 1)?))
    }

    /// Accelerometer range / bandwidth / power; fusion modes override it.
    pub fn set_accel_config(&self, config: &AccelConfig) -> Result<()> {
        self.write_page1_config(r::REG1_ACC_CONFIG, &[config.pack()])
    }

    pub fn get_gyro_config(&self) -> Result<GyroConfig> {
        Ok(GyroConfig::unpack(&self.read_registers(r::REG1_GYR_CONFIG_0, 2, 1)?)?)
    }

    pub fn set_gyro_config(&self, config: &GyroConfig) -> Result<()> {
        self.write_page1_config(r::REG1_GYR_CONFIG_0, &config.pack())
    }

    pub fn get_mag_config(&self) -> Result<MagConfig> {
        Ok(MagConfig::unpack(self.read_register(r::REG1_MAG_CONFIG, 1)?))
    }

    pub fn set_mag_config(&self, config: &MagConfig) -> Result<()> {
        self.write_page1_config(r::REG1_MAG_CONFIG, &[config.pack()])
    }

    /// The chip's 16-byte unique id (page 1, 0x50..0x5F).
    pub fn unique_id(&self) -> Result<Vec<u8>> {
        self.read_registers(r::REG1_UNIQUE_ID, r::UNIQUE_ID_LEN, 1)
    }

    // ── interrupts (motion only on SW 03.11) ─────────────────────────────────

    pub fn get_interrupt_enable(&self) -> Result<u8> {
        self.read_register(r::REG1_INT_EN, 1)
    }

    /// INT_EN: which interrupt engines run.
    pub fn set_interrupt_enable(&self, mask: u8) -> Result<()> {
        self.write_register(r::REG1_INT_EN, mask, 1)
    }

    pub fn get_interrupt_mask(&self) -> Result<u8> {
        self.read_register(r::REG1_INT_MSK, 1)
    }

    /// INT_MSK: which enabled interrupts drive the INT pin.
    pub fn set_interrupt_mask(&self, mask: u8) -> Result<()> {
        self.write_register(r::REG1_INT_MSK, mask, 1)
    }

    /// One motion-interrupt setting register (page 1, 0x11..0x1F), as a raw byte. Not while streaming.
    pub fn set_interrupt_setting(&self, register: u8, value: u8) -> Result<()> {
        if !(r::REG1_ACC_AM_THRES..=r::REG1_GYR_AM_SET).contains(&register) {
            return Err(DepzError::InvalidArgument(format!(
                "0x{register:02X} is not a page-1 interrupt setting (0x11..0x1F)"
            )));
        }
        self.write_page1_config(register, &[value])
    }

    /// INT_STA — which interrupts fired. Clears on read.
    pub fn read_interrupt_status(&self) -> Result<u8> {
        self.read_register(r::REG_INT_STA, 0)
    }

    /// SYS_TRIGGER RST_INT: reset the interrupt status bits and the INT pin.
    pub fn clear_interrupt(&self) -> Result<()> {
        let _st = self.state.lock().unwrap();
        let clk = self.read(r::REG_SYS_TRIGGER, 1)?.1[0] & r::SYS_TRIGGER_CLK_SEL;
        self.write(r::REG_SYS_TRIGGER, &[clk | r::SYS_TRIGGER_RST_INT])
    }

    // ── data ─────────────────────────────────────────────────────────────────

    /// Poll one register block `(addr, length)` (`None`: the full 46 bytes 0x08..0x35) and decode it.
    /// Works alongside a stream.
    pub fn read_sample(&self, block: Option<(u8, u8)>) -> Result<Bno055Sample> {
        let (addr, length) = block.unwrap_or(r::FULL_BLOCK);
        let mut st = self.state.lock().unwrap();
        let units = self.current_units(&mut st)?;
        let (ts, data) = self.read(addr, length as usize)?;
        Ok(Bno055Sample::decode(ts, addr, &data, units))
    }

    /// (w, x, y, z) — the cheapest orientation read (8 bytes).
    pub fn read_quaternion(&self) -> Result<[f64; 4]> {
        Ok(self.read_sample(Some(r::QUAT_BLOCK))?.quaternion.unwrap_or_default())
    }

    /// Arm the bridge to read `block` (`None`: the full block) every `period_ms` and push it. Fusion
    /// runs at 100 Hz, so 10 ms is the useful floor. `int_trigger` reads on the INT edge, `period_ms`
    /// then a missed-edge watchdog (0 = none). Replaces a running stream.
    pub fn start_stream(&self, period_ms: u16, block: Option<(u8, u8)>, int_trigger: bool) -> Result<()> {
        let (addr, length) = block.unwrap_or(r::FULL_BLOCK);
        if length < 1 || length as usize > XFER_MAX || addr as usize + length as usize > 0x100 {
            return Err(DepzError::InvalidArgument(format!("block 0x{addr:02X}+{length} outside 1..{XFER_MAX} / page")));
        }
        let mut st = self.state.lock().unwrap();
        // Units first: the reader may decode the first sample before the request returns.
        *self.streams.units.lock().unwrap() = self.current_units(&mut st)?;
        let trigger = if int_trigger { TRIGGER_INT } else { TRIGGER_TIMER };
        self.request_ok(Bno055Cmd::StartStream as u8, &pack_start_stream(trigger, addr, length, period_ms), None)?;
        self.streaming.store(true, Ordering::SeqCst);
        Ok(())
    }

    pub fn stop_stream(&self) -> Result<()> {
        if !self.streaming() {
            return Ok(());
        }
        let r = self.request_ok(Bno055Cmd::StopStream as u8, &[], None);
        self.streaming.store(false, Ordering::SeqCst);
        r
    }

    pub fn streaming(&self) -> bool {
        self.streaming.load(Ordering::SeqCst)
    }

    /// Call `callback` for every streamed sample (reader thread).
    pub fn on_sample(&self, callback: impl Fn(&Bno055Sample) + Send + Sync + 'static) -> Hook {
        self.streams.samples.on_item(callback)
    }

    /// A pull queue of streamed samples (bounded, drop-oldest).
    pub fn samples(&self, capacity: usize) -> Subscription<Bno055Sample> {
        self.streams.samples.subscribe(capacity)
    }

    /// The next streamed sample (contract 07 §3); `timeout` `None`: 1 s.
    pub fn get_sample(&self, timeout: Option<Duration>) -> Result<Bno055Sample> {
        self.streams.samples.next(timeout.unwrap_or(Duration::from_secs(1)), Bno055Rpt::Stream as u8, self.is_closed())
    }

    /// Stream reports dropped because they did not decode (a short block).
    pub fn stream_parse_errors(&self) -> u64 {
        self.streams.parse_errors.load(Ordering::Relaxed)
    }

    pub fn stream_dropped_counts(&self) -> Vec<u64> {
        self.streams.samples.dropped_counts()
    }

    // ── internal ─────────────────────────────────────────────────────────────

    // READ_REG in ≤128-byte pieces on the current page → (MCU timestamp of the last piece, bytes).
    fn read(&self, addr: u8, length: usize) -> Result<(u64, Vec<u8>)> {
        let mut out = Vec::with_capacity(length);
        let mut a = addr as usize;
        let mut ts = 0;
        while out.len() < length {
            let n = (length - out.len()).min(XFER_MAX);
            if a > 0xFF {
                return Err(DepzError::InvalidArgument(format!("register 0x{a:X} past the page")));
            }
            let rep: RegData = self.req.request(
                Bno055Cmd::ReadReg as u8,
                &pack_read_reg(a as u8, n as u8),
                expect_report(Bno055Rpt::RegData as u8, RegData::unpack),
                None,
            )?;
            if rep.data.len() != n {
                return Err(DepzError::Protocol(format!("READ_REG 0x{a:02X}: expected {n}, got {}", rep.data.len())));
            }
            ts = rep.timestamp_us;
            out.extend_from_slice(&rep.data);
            a += n;
        }
        Ok((ts, out))
    }

    fn write(&self, addr: u8, data: &[u8]) -> Result<()> {
        let mut a = addr as usize;
        for chunk in data.chunks(XFER_MAX) {
            if a > 0xFF {
                return Err(DepzError::InvalidArgument(format!("register 0x{a:X} past the page")));
            }
            self.req.request_ok(Bno055Cmd::WriteReg as u8, &pack_write_reg(a as u8, chunk), None)?;
            a += chunk.len();
        }
        Ok(())
    }

    fn current_mode(&self) -> Result<OprMode> {
        opr_mode(self.read(r::REG_OPR_MODE, 1)?.1[0])
    }

    // One OPR_MODE write plus its settle time (no via-CONFIG logic).
    fn switch_mode(&self, mode: OprMode) -> Result<()> {
        self.write(r::REG_OPR_MODE, &[mode as u8])?;
        std::thread::sleep(if mode == OprMode::Config { MODE_SWITCH_TO_CONFIG } else { MODE_SWITCH_FROM_CONFIG });
        if mode.is_fusion() {
            self.wait_fusion_started(false)?;
        }
        Ok(())
    }

    fn on_page<T>(&self, page: u8, body: impl FnOnce() -> Result<T>) -> Result<T> {
        match page {
            0 => body(),
            1 => {
                self.require_not_streaming()?;
                self.write(r::REG_PAGE_ID, &[0x01])?;
                let out = body();
                let back = self.write(r::REG_PAGE_ID, &[0x00]);
                let out = out?;
                back?;
                Ok(out)
            }
            _ => Err(DepzError::InvalidArgument(format!("BNO055 has register pages 0 and 1, got {page}"))),
        }
    }

    // Run a block in CONFIG mode and put the previous mode back (the state lock is held by the caller).
    fn in_config_mode<T>(&self, body: impl FnOnce() -> Result<T>) -> Result<T> {
        let previous = self.current_mode()?;
        if previous != OprMode::Config {
            self.switch_mode(OprMode::Config)?;
        }
        let out = body();
        if previous != OprMode::Config {
            self.switch_mode(previous)?;
        }
        out
    }

    // A self-test run in CONFIG leaves SYS_STATUS at 4 until the mode leaves CONFIG: ACCONLY and back.
    fn clear_self_test_status(&self) -> Result<()> {
        self.switch_mode(OprMode::AccOnly)?;
        self.switch_mode(OprMode::Config)
    }

    // Poll SYS_STATUS until the sensor's own boot (init + POST) is over; a 4 that outlasts POST is a
    // self-test leftover, cleared.
    fn wait_booted(&self, _st: &mut Locked<'_>) -> Result<()> {
        let deadline = Instant::now() + BOOT_SETTLE_TIMEOUT;
        let mut in_self_test = 0;
        let mut cleared = false;
        loop {
            let status = self.read(r::REG_SYS_STATUS, 1)?.1[0];
            if !r::SYS_STATUS_BOOTING.contains(&status) {
                return Ok(());
            }
            in_self_test = if status == 4 { in_self_test + 1 } else { 0 };
            if in_self_test >= STUCK_SELF_TEST_POLLS && !cleared {
                self.clear_self_test_status()?;
                cleared = true;
                in_self_test = 0;
                continue;
            }
            if Instant::now() > deadline {
                return Err(DepzError::Sensor("BNO055 did not finish booting (SYS_STATUS stuck)".into()));
            }
            std::thread::sleep(Duration::from_millis(5));
        }
    }

    // A running fusion never outputs the all-zero quaternion. Strict: fail on timeout; else give up
    // quietly (a suspended sensor never updates its registers).
    fn wait_fusion_started(&self, strict: bool) -> Result<()> {
        let deadline = Instant::now() + FUSION_START_TIMEOUT;
        while self.read(r::REG_QUA_DATA, 8)?.1.iter().all(|b| *b == 0) {
            if Instant::now() > deadline {
                if strict {
                    return Err(DepzError::Sensor("BNO055 fusion did not start (quaternion stays zero)".into()));
                }
                return Ok(());
            }
            std::thread::sleep(Duration::from_millis(10));
        }
        Ok(())
    }

    fn write_page1_config(&self, addr: u8, data: &[u8]) -> Result<()> {
        self.require_not_streaming()?;
        let _st = self.state.lock().unwrap();
        self.in_config_mode(|| self.on_page(1, || self.write(addr, data)))
    }

    fn current_units(&self, st: &mut Locked<'_>) -> Result<Units> {
        if let Some(u) = st.units {
            return Ok(u);
        }
        let u = Units::unpack(self.read(r::REG_UNIT_SEL, 1)?.1[0]);
        st.units = Some(u);
        Ok(u)
    }

    fn require_not_streaming(&self) -> Result<()> {
        if self.streaming() {
            return Err(DepzError::InvalidState("stop_stream() first — the stream reads page 0".into()));
        }
        Ok(())
    }
}

fn opr_mode(value: u8) -> Result<OprMode> {
    OprMode::from_reg(value).ok_or_else(|| DepzError::Protocol(format!("OPR_MODE reads 0x{:X}, not a BNO055 mode", value & 0x0F)))
}

fn pwr_mode(value: u8) -> Result<PwrMode> {
    match value & 0x03 {
        0 => Ok(PwrMode::Normal),
        1 => Ok(PwrMode::LowPower),
        2 => Ok(PwrMode::Suspend),
        v => Err(DepzError::Protocol(format!("PWR_MODE reads {v}, not a BNO055 power mode"))),
    }
}
