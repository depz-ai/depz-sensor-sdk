//! VL53L8CX / VL53L8CH multizone ToF (contract 04): the ST ULD on the host over the SPI register
//! bridge. The L5/L7 classes on the I2C bridge (contract 11) run on the same core, see
//! [`super::vl53l7`].

use std::ops::Deref;
use std::sync::atomic::{AtomicBool, AtomicU64, AtomicUsize, Ordering};
use std::sync::{Arc, Mutex};
use std::time::Duration;

use super::device::{expect_report, DepzDevice, ReportHandler, Requester};
use super::error::{DepzError, Result};
use super::stream::{Fanout, Hook, Subscription};
use crate::framing::Packet;
use crate::vl53l8::advanced::{DetectionThreshold, MotionConfig};
use crate::vl53l8::driver::{
    UldResult, Vl53l8CnhSetup, Vl53l8Driver, Vl53l8Firmware, Vl53l8Frame, Vl53l8Platform,
};
use crate::vl53l8::{
    pack_read_reg, pack_start_stream, pack_write_reg, unpack_frame_chunk, FrameReassembler,
    RegData, Vl53l8Cmd, Vl53l8Rpt, RESOLUTION_4X4, RESOLUTION_8X8,
};

/// Below this the VL53L8 never enters its ranging loop and streams nothing (contract 04).
pub const MIN_RANGING_FREQUENCY_HZ: i32 = 2;

/// Highest ranging frequency at `resolution` zones (contract 11 §3; the same
/// silicon limit on VL53L8): 60 Hz at 4×4, 15 Hz at 8×8.
pub fn max_ranging_frequency_hz(resolution: usize) -> i32 {
    if resolution == RESOLUTION_8X8 {
        15
    } else {
        60
    }
}

const REG_TIMEOUT: Duration = Duration::from_secs(2);
const STREAM_MIN_BYTES: usize = 12;

/// The ULD driver a VL53L8 / L5 / L7 device runs ([`Vl53l8cxDevice::with_uld`]).
pub type BridgeUld = Vl53l8Driver<Arc<dyn Vl53l8Platform>>;

// What tells the classes apart: the firmware set, the bridge's transfer sizes, the lowest ranging
// frequency and, on L5/L7, the module type the class expects after init.
pub(crate) struct Profile {
    pub(crate) firmware: Vl53l8Firmware,
    pub(crate) read_chunk: usize,
    pub(crate) write_chunk: usize,
    /// Largest `RPT_VL53_FRAME` chunk the board emits (contract 04 §4 rule 4).
    pub(crate) stream_chunk: usize,
    pub(crate) min_hz: i32,
    pub(crate) expected_module: Option<u8>,
    pub(crate) class: &'static str,
}

const CX: Profile = Profile {
    firmware: Vl53l8Firmware::Cx,
    read_chunk: crate::vl53l8::codecs::CHUNK_SIZE,
    write_chunk: crate::vl53l8::codecs::CHUNK_SIZE,
    stream_chunk: crate::vl53l8::framing::STREAM_CHUNK_MAX,
    min_hz: MIN_RANGING_FREQUENCY_HZ,
    expected_module: None,
    class: "Vl53l8cxDevice",
};

const CH: Profile = Profile {
    firmware: Vl53l8Firmware::Ch,
    class: "Vl53l8chDevice",
    ..CX
};

type WriteProgress = Box<dyn FnMut(usize, usize) + Send>;

struct Shared {
    frames: Fanout<Vl53l8Frame>,
    uld: Mutex<Option<BridgeUld>>,
    reassembler: Mutex<FrameReassembler>,
    resolution: AtomicUsize,
    parse_errors: AtomicU64,
}

impl ReportHandler for Shared {
    fn handle_report(&self, packet: &Packet) -> bool {
        if packet.cmd != Vl53l8Rpt::Vl53Frame as u8 || packet.payload.len() < STREAM_MIN_BYTES {
            return false;
        }
        let Some(chunk) = unpack_frame_chunk(&packet.payload) else {
            return true;
        };
        let Some(done) = self.reassembler.lock().unwrap().feed(chunk) else {
            return true;
        };
        // The reader thread never waits for the driver: a caller holding it may be waiting for a
        // reply only this thread delivers. A frame that meets a busy driver is dropped as unparsed.
        let parsed = match self.uld.try_lock() {
            Ok(mut g) => match g.as_mut() {
                Some(d) => d.parse_frame(done.timestamp_us, &done.frame).ok(),
                None => return true,
            },
            Err(_) => None,
        };
        match parsed {
            Some(f) => self
                .frames
                .emit(f.with_resolution(self.resolution.load(Ordering::Relaxed))),
            None => {
                self.parse_errors.fetch_add(1, Ordering::Relaxed);
            }
        }
        true
    }

    fn reader_stopped(&self) {
        self.frames.close_all();
    }
}

// The ULD platform on the firmware register bridge.
struct BridgePlatform {
    dev: Requester,
    read_chunk: usize,
    write_chunk: usize,
    write_progress: Mutex<Option<WriteProgress>>,
}

impl Vl53l8Platform for BridgePlatform {
    fn read_multi(&self, addr: u16, size: usize) -> Result<Vec<u8>> {
        let mut out = Vec::with_capacity(size);
        let mut addr = addr;
        while out.len() < size {
            let n = (size - out.len()).min(self.read_chunk);
            let rep: RegData = self.dev.request(
                Vl53l8Cmd::ReadReg as u8,
                &pack_read_reg(addr, n as u16),
                expect_report(Vl53l8Rpt::RegData as u8, RegData::unpack),
                Some(REG_TIMEOUT),
            )?;
            if rep.data.len() != n {
                return Err(DepzError::Protocol(format!(
                    "READ_REG 0x{addr:04X}: expected {n}, got {}",
                    rep.data.len()
                )));
            }
            out.extend_from_slice(&rep.data);
            addr = addr.wrapping_add(n as u16);
        }
        Ok(out)
    }

    fn write_multi(&self, addr: u16, data: &[u8]) -> Result<()> {
        let mut addr = addr;
        let mut done = 0;
        for chunk in data.chunks(self.write_chunk) {
            self.dev.request_ok(
                Vl53l8Cmd::WriteReg as u8,
                &pack_write_reg(addr, chunk),
                Some(REG_TIMEOUT),
            )?;
            addr = addr.wrapping_add(chunk.len() as u16);
            done += chunk.len();
            if data.len() > self.write_chunk {
                if let Some(progress) = self.write_progress.lock().unwrap().as_mut() {
                    progress(done, data.len());
                }
            }
        }
        Ok(())
    }
}

/// VL53L8CX multizone ToF (contract 04). The board is a thin SPI bridge; the ST ULD runs here
/// ([`Vl53l8cxDevice::with_uld`]). [`Vl53l8cxDevice::init`] downloads the ~84 KB sensor firmware
/// (about a second), then configure and [`Vl53l8cxDevice::start_ranging`]: the board pushes one
/// frame per INT edge in chunks, reassembled and parsed into [`Vl53l8cxDevice::frames`].
/// Configuration needs `init` and fails with [`DepzError::InvalidState`] while ranging — the stream
/// owns the register bank. The VL53L8CH superset is [`Vl53l8chDevice`]; the L5/L7 classes deref to
/// this one too.
pub struct Vl53l8cxDevice {
    dev: DepzDevice,
    shared: Arc<Shared>,
    platform: Arc<BridgePlatform>,
    profile: &'static Profile,
    ranging: AtomicBool,
    armed_cnh: Arc<Mutex<Option<Vl53l8CnhSetup>>>,
}

impl Deref for Vl53l8cxDevice {
    type Target = DepzDevice;

    fn deref(&self) -> &DepzDevice {
        &self.dev
    }
}

impl Vl53l8cxDevice {
    /// Continue the open connection `dev` as a VL53L8CX.
    pub fn attach(dev: DepzDevice) -> Vl53l8cxDevice {
        Vl53l8cxDevice::with_profile(dev, &CX)
    }

    pub(crate) fn with_profile(dev: DepzDevice, profile: &'static Profile) -> Vl53l8cxDevice {
        let shared = Arc::new(Shared {
            frames: Fanout::new(dev.callback_errors_counter()),
            uld: Mutex::new(None),
            reassembler: Mutex::new(FrameReassembler::new()),
            resolution: AtomicUsize::new(RESOLUTION_4X4), // the ULD default after init
            parse_errors: AtomicU64::new(0),
        });
        dev.set_report_handler(shared.clone());
        let platform = Arc::new(BridgePlatform {
            dev: dev.requester(),
            read_chunk: profile.read_chunk,
            write_chunk: profile.write_chunk,
            write_progress: Mutex::new(None),
        });
        Vl53l8cxDevice {
            dev,
            shared,
            platform,
            profile,
            ranging: AtomicBool::new(false),
            armed_cnh: Arc::new(Mutex::new(None)),
        }
    }

    /// Run `f` on the ULD driver (escape hatch for DCI access). [`DepzError::InvalidState`] before
    /// [`Vl53l8cxDevice::init`].
    pub fn with_uld<R>(&self, f: impl FnOnce(&mut BridgeUld) -> UldResult<R>) -> Result<R> {
        let mut g = self.shared.uld.lock().unwrap();
        let d = g
            .as_mut()
            .ok_or_else(|| DepzError::InvalidState("call init() first".into()))?;
        Ok(f(d)?)
    }

    /// The sensor firmware [`Vl53l8cxDevice::init`] downloads (cx / ch / l7cx / l7ch).
    pub fn firmware(&self) -> Vl53l8Firmware {
        self.profile.firmware
    }

    /// True after a successful [`Vl53l8cxDevice::init`], until the sensor loses its state.
    pub fn initialized(&self) -> bool {
        self.shared.uld.lock().unwrap().is_some()
    }

    pub fn ranging(&self) -> bool {
        self.ranging.load(Ordering::SeqCst)
    }

    /// True when the sensor answers with its own silicon ids (a bus error or a foreign id: false).
    pub fn is_alive(&self) -> bool {
        let driver = Vl53l8Driver::new(
            self.platform() as Arc<dyn Vl53l8Platform>,
            self.profile.firmware,
        );
        let i2c = matches!(
            self.profile.firmware,
            Vl53l8Firmware::L7cx | Vl53l8Firmware::L7ch
        );
        driver
            .is_alive()
            .map(|(id, rev)| BridgeUld::silicon_id_matches(id, rev, i2c))
            .unwrap_or(false)
    }

    /// Download the sensor firmware and the default configuration.
    pub fn init(&self) -> Result<()> {
        self.init_with_progress(&mut |_| {}, None)
    }

    /// [`Vl53l8cxDevice::init`] reporting its phases to `progress` and the big firmware writes to
    /// `write_progress` (done, total bytes). On L5/L7, a module type other than the class expects
    /// (a board labelled L7 carrying an L5, or the reverse) is a warning on stderr: ranging still
    /// works, the firmware is shared.
    pub fn init_with_progress(
        &self,
        progress: &mut dyn FnMut(&str),
        write_progress: Option<WriteProgress>,
    ) -> Result<()> {
        self.require_not_ranging()?;
        let mut g = self.shared.uld.lock().unwrap();
        *self.platform.write_progress.lock().unwrap() = write_progress;
        let mut driver = Vl53l8Driver::new(
            self.platform() as Arc<dyn Vl53l8Platform>,
            self.profile.firmware,
        );
        let r = driver.init_with_progress(progress);
        *self.platform.write_progress.lock().unwrap() = None;
        r?;
        if let (Some(want), Some(got)) = (self.profile.expected_module, driver.module_type()) {
            if want != got {
                eprintln!(
                    "warning: {} expects module type {}, the sensor reports {} — check the board's device name",
                    self.profile.class,
                    module_name(want),
                    module_name(got)
                );
            }
        }
        *g = Some(driver);
        // A fresh driver has no motion indicator armed; the CNH setup is gone from the sensor too
        // (ERRATA B4).
        *self.armed_cnh.lock().unwrap() = None;
        Ok(())
    }

    fn platform(&self) -> Arc<BridgePlatform> {
        self.platform.clone()
    }

    // ── configuration (init first; not while ranging) ───────────────────────

    /// Zones in use: 16 (4×4) or 64 (8×8).
    pub fn get_resolution(&self) -> Result<usize> {
        let zones = self.with_uld(|u| u.get_resolution())?;
        self.shared.resolution.store(zones, Ordering::Relaxed);
        Ok(zones)
    }

    /// 16 (4×4) or 64 (8×8) zones.
    pub fn set_resolution(&self, zones: usize) -> Result<()> {
        if zones != RESOLUTION_4X4 && zones != RESOLUTION_8X8 {
            return Err(DepzError::InvalidArgument(format!(
                "resolution is 16 (4x4) or 64 (8x8) zones, got {zones}"
            )));
        }
        self.require_not_ranging()?;
        self.with_uld(|u| u.set_resolution(zones))?;
        self.shared.resolution.store(zones, Ordering::Relaxed);
        Ok(())
    }

    pub fn get_ranging_frequency_hz(&self) -> Result<u8> {
        self.with_uld(|u| u.get_ranging_frequency_hz())
    }

    /// At least 2 Hz (1 Hz on L5/L7); at most 60 Hz at 4×4 and 15 Hz at 8×8.
    pub fn set_ranging_frequency_hz(&self, hz: i32) -> Result<()> {
        if hz < self.profile.min_hz {
            return Err(DepzError::InvalidArgument(format!(
                "ranging frequency must be >= {} Hz: below that the sensor never enters its ranging loop and streams nothing (contract 04), got {hz}",
                self.profile.min_hz
            )));
        }
        // contract 11 §3: judged by the resolution last set or read through this
        // device (4×4 after init); refused host-side, nothing reaches the bus.
        let zones = self.shared.resolution.load(Ordering::Relaxed);
        let max_hz = max_ranging_frequency_hz(zones);
        if hz > max_hz {
            return Err(DepzError::InvalidArgument(format!(
                "ranging frequency must be <= {max_hz} Hz at {zones} zones (60 Hz at 4x4, 15 Hz at 8x8, contract 11 §3), got {hz}"
            )));
        }
        self.require_not_ranging()?;
        self.with_uld(|u| u.set_ranging_frequency_hz(hz))
    }

    /// [`crate::vl53l8::driver::RANGING_MODE_CONTINUOUS`] or
    /// [`crate::vl53l8::driver::RANGING_MODE_AUTONOMOUS`].
    pub fn get_ranging_mode(&self) -> Result<u8> {
        self.with_uld(|u| u.get_ranging_mode())
    }

    pub fn set_ranging_mode(&self, mode: u8) -> Result<()> {
        self.require_not_ranging()?;
        self.with_uld(|u| u.set_ranging_mode(mode))
    }

    pub fn get_integration_time_ms(&self) -> Result<u32> {
        self.with_uld(|u| u.get_integration_time_ms())
    }

    /// 2–1000 ms; autonomous mode only.
    pub fn set_integration_time_ms(&self, ms: i32) -> Result<()> {
        self.require_not_ranging()?;
        self.with_uld(|u| u.set_integration_time_ms(ms))
    }

    pub fn get_sharpener_percent(&self) -> Result<u8> {
        self.with_uld(|u| u.get_sharpener_percent())
    }

    /// Edge sharpener, 0–99 % (0 = off).
    pub fn set_sharpener_percent(&self, pct: i32) -> Result<()> {
        self.require_not_ranging()?;
        self.with_uld(|u| u.set_sharpener_percent(pct))
    }

    /// [`crate::vl53l8::driver::TARGET_ORDER_CLOSEST`] or
    /// [`crate::vl53l8::driver::TARGET_ORDER_STRONGEST`].
    pub fn get_target_order(&self) -> Result<u8> {
        self.with_uld(|u| u.get_target_order())
    }

    pub fn set_target_order(&self, order: u8) -> Result<()> {
        self.require_not_ranging()?;
        self.with_uld(|u| u.set_target_order(order))
    }

    // ── advanced features (UM3109) ──────────────────────────────────────────

    /// [`crate::vl53l8::advanced::POWER_MODE_SLEEP`], `POWER_MODE_WAKEUP` or `POWER_MODE_DEEP_SLEEP`.
    pub fn get_power_mode(&self) -> Result<u8> {
        self.with_uld(|u| u.get_power_mode())
    }

    /// Waking from deep sleep downloads the firmware again.
    pub fn set_power_mode(&self, mode: u8) -> Result<()> {
        self.require_not_ranging()?;
        self.with_uld(|u| u.set_power_mode(mode))
    }

    /// Xtalk margin, whole kcps/SPAD (as ST).
    pub fn get_xtalk_margin(&self) -> Result<u32> {
        self.with_uld(|u| u.get_xtalk_margin())
    }

    /// Xtalk margin, a whole number of kcps/SPAD, 0..10000 (as ST); above is refused.
    pub fn set_xtalk_margin(&self, margin_kcps: u32) -> Result<()> {
        self.require_not_ranging()?;
        self.with_uld(|u| u.set_xtalk_margin(margin_kcps))
    }

    /// On-device crosstalk calibration against a flat target; read the result with
    /// [`Vl53l8cxDevice::get_caldata_xtalk`].
    pub fn calibrate_xtalk(
        &self,
        reflectance_percent: i32,
        nb_samples: i32,
        distance_mm: i32,
    ) -> Result<()> {
        self.require_not_ranging()?;
        self.with_uld(|u| u.calibrate_xtalk(reflectance_percent, nb_samples, distance_mm))
    }

    /// The 776-byte xtalk calibration buffer (save / restore).
    pub fn get_caldata_xtalk(&self) -> Result<Vec<u8>> {
        self.require_not_ranging()?;
        self.with_uld(|u| u.get_caldata_xtalk())
    }

    pub fn set_caldata_xtalk(&self, blob: &[u8]) -> Result<()> {
        self.require_not_ranging()?;
        self.with_uld(|u| u.set_caldata_xtalk(blob))
    }

    pub fn get_detection_thresholds_enable(&self) -> Result<u8> {
        self.with_uld(|u| u.get_detection_thresholds_enable())
    }

    pub fn set_detection_thresholds_enable(&self, enabled: bool) -> Result<()> {
        self.require_not_ranging()?;
        self.with_uld(|u| u.set_detection_thresholds_enable(enabled))
    }

    pub fn get_detection_thresholds(&self) -> Result<Vec<DetectionThreshold>> {
        self.with_uld(|u| u.get_detection_thresholds())
    }

    /// The 64 detection thresholds (interrupt on threshold).
    pub fn set_detection_thresholds(&self, thresholds: &[DetectionThreshold]) -> Result<()> {
        self.require_not_ranging()?;
        self.with_uld(|u| u.set_detection_thresholds(thresholds))
    }

    pub fn set_detection_thresholds_auto_stop(&self, auto_stop: bool) -> Result<()> {
        self.require_not_ranging()?;
        self.with_uld(|u| u.set_detection_thresholds_auto_stop(auto_stop))
    }

    /// Enable the motion indicator over [min, max] mm (`None`: 400–1500); every frame then carries
    /// [`Vl53l8Frame::motion`]. Returns the configuration for further tuning.
    ///
    /// On CH firmware the motion indicator and CNH share one DCI struct (0xBFAC): this disarms CNH
    /// (`cnh_setup()` → `None`, no [`Vl53l8Frame::cnh_raw`]).
    pub fn configure_motion_indicator(
        &self,
        distance_mm: Option<(i32, i32)>,
    ) -> Result<MotionConfig> {
        self.require_not_ranging()?;
        let (min, max) = distance_mm.unwrap_or((400, 1500));
        let zones = self.get_resolution()?;
        self.with_uld(|u| {
            let mut cfg = u.motion_indicator_init(zones)?;
            *self.armed_cnh.lock().unwrap() = None;
            u.motion_indicator_set_distance_motion(&mut cfg, min, max)?;
            Ok(cfg)
        })
    }

    // ── CNH (the CH classes) ────────────────────────────────────────────────

    /// The CNH setup armed for the next start, or `None`; recorders keep it next to the raw blocks.
    pub(crate) fn armed_cnh(&self) -> Option<Vl53l8CnhSetup> {
        self.armed_cnh.lock().unwrap().clone()
    }

    // The armed setup as the recorder's frame hook reads it, frame by frame.
    pub(crate) fn armed_cnh_handle(&self) -> Arc<Mutex<Option<Vl53l8CnhSetup>>> {
        self.armed_cnh.clone()
    }

    // CH classes: arm CNH for the next start_ranging (the setup is checked before it is written).
    pub(crate) fn arm_cnh(&self, setup: &Vl53l8CnhSetup) -> Result<()> {
        self.require_not_ranging()?;
        setup.required_memory()?; // a blank or oversized setup is refused before anything is sent
        self.with_uld(|u| {
            u.dci_write_data(Vl53l8CnhSetup::MI_CFG_DEV_IDX, &setup.pack())?;
            u.motion_indicator_disarm(); // one DCI block (ERRATA B4)
            Ok(())
        })?;
        *self.armed_cnh.lock().unwrap() = Some(setup.clone());
        Ok(())
    }

    // The sensor lost its state (power or reset pin): init again.
    pub(crate) fn forget_sensor(&self) {
        self.ranging.store(false, Ordering::SeqCst);
        *self.shared.uld.lock().unwrap() = None;
    }

    // ── ranging ─────────────────────────────────────────────────────────────

    /// Program the output list, start the sensor and the board's stream.
    pub fn start_ranging(&self) -> Result<()> {
        self.require_not_ranging()?;
        let cnh_size = match self.armed_cnh() {
            Some(s) => Some(s.required_memory()?),
            None => None,
        };
        self.get_resolution()?; // refresh the zone count that shapes frames
        let frame_size = self.with_uld(|u| {
            u.start_ranging(cnh_size)?;
            Ok(u.data_read_size())
        })?;
        *self.shared.reassembler.lock().unwrap() =
            FrameReassembler::with_chunk_max(self.profile.stream_chunk);
        let started = u16::try_from(frame_size)
            .ok()
            .and_then(|n| pack_start_stream(n).ok())
            .ok_or_else(|| {
                DepzError::InvalidArgument(format!("frame_size {frame_size} outside 1..8192"))
            })
            .and_then(|payload| self.request_ok(Vl53l8Cmd::StartStream as u8, &payload, None));
        if let Err(e) = started {
            // The sensor ranges but the host will say it does not: stop it again, or
            // stop_ranging() could never reach it. The START_STREAM error is the one the caller
            // gets, even if this stop fails too.
            let _ = self.with_uld(|u| u.stop_ranging());
            return Err(e);
        }
        self.ranging.store(true, Ordering::SeqCst);
        Ok(())
    }

    pub fn stop_ranging(&self) -> Result<()> {
        if !self.ranging() {
            return Ok(());
        }
        // Leave the host "not ranging" and stop the sensor even when the STOP_STREAM ack fails.
        let r = self.request_ok(Vl53l8Cmd::StopStream as u8, &[], None);
        self.ranging.store(false, Ordering::SeqCst);
        let s = self.with_uld(|u| u.stop_ranging());
        r.and(s)
    }

    /// Call `callback` for every parsed frame (reader thread).
    pub fn on_frame(&self, callback: impl Fn(&Vl53l8Frame) + Send + Sync + 'static) -> Hook {
        self.shared.frames.on_item(callback)
    }

    /// A pull queue of parsed frames (bounded, drop-oldest).
    pub fn frames(&self, capacity: usize) -> Subscription<Vl53l8Frame> {
        self.shared.frames.subscribe(capacity)
    }

    /// The next frame (contract 07 §3); `timeout` `None`: 2 s.
    pub fn get_frame(&self, timeout: Option<Duration>) -> Result<Vl53l8Frame> {
        self.shared.frames.next(
            timeout.unwrap_or(Duration::from_secs(2)),
            Vl53l8Rpt::Vl53Frame as u8,
            self.is_closed(),
        )
    }

    /// Frames dropped because they did not parse (corrupt frame, bad size) — not reassembly gaps.
    pub fn frame_parse_errors(&self) -> u64 {
        self.shared.parse_errors.load(Ordering::Relaxed)
    }

    /// Chunked frames the reassembler discarded (gaps, offset errors).
    pub fn reassembler_discards(&self) -> u64 {
        self.shared.reassembler.lock().unwrap().discarded
    }

    pub fn stream_dropped_counts(&self) -> Vec<u64> {
        self.shared.frames.dropped_counts()
    }

    pub(crate) fn require_not_ranging(&self) -> Result<()> {
        if self.ranging() {
            return Err(DepzError::InvalidState(
                "stop_ranging() first — the stream owns the register bank".into(),
            ));
        }
        Ok(())
    }
}

fn module_name(t: u8) -> String {
    use crate::vl53l8::driver::{MODULE_TYPE_MZ, MODULE_TYPE_MZEVO, MODULE_TYPE_MZPLUS};
    match t {
        MODULE_TYPE_MZ => "MZ".into(),
        MODULE_TYPE_MZEVO => "MZEVO".into(),
        MODULE_TYPE_MZPLUS => "MZPlus".into(),
        _ => t.to_string(),
    }
}

/// VL53L8CH: the VL53L8CX superset (every CX method, through `Deref`) plus CNH, compact network
/// histograms — each frame can carry a per-aggregate distance histogram. `init` downloads the CH
/// firmware (VL53LMZ ULD 2.0.16).
pub struct Vl53l8chDevice(Vl53l8cxDevice);

impl Deref for Vl53l8chDevice {
    type Target = Vl53l8cxDevice;

    fn deref(&self) -> &Vl53l8cxDevice {
        &self.0
    }
}

impl Vl53l8chDevice {
    /// Continue the open connection `dev` as a VL53L8CH.
    pub fn attach(dev: DepzDevice) -> Vl53l8chDevice {
        Vl53l8chDevice(Vl53l8cxDevice::with_profile(dev, &CH))
    }

    /// The CNH setup armed by [`Vl53l8chDevice::configure_cnh`], or `None`; recorders keep it next
    /// to the raw blocks to decode them later.
    pub fn cnh_setup(&self) -> Option<Vl53l8CnhSetup> {
        self.0.armed_cnh()
    }

    /// Arm CNH for the next [`Vl53l8cxDevice::start_ranging`]: every frame then carries
    /// [`Vl53l8Frame::cnh_raw`], decoded with [`crate::vl53l8::decode_cnh`] and
    /// [`Vl53l8CnhSetup::decode_config`]. CNH and the motion indicator share one DCI struct
    /// (0xBFAC): this disarms the motion indicator (frames carry no [`Vl53l8Frame::motion`]).
    pub fn configure_cnh(&self, setup: &Vl53l8CnhSetup) -> Result<()> {
        self.0.arm_cnh(setup)
    }
}
