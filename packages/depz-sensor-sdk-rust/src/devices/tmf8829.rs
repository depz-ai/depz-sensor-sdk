//! TMF8829 multizone dToF (contract 14): the host runs the sensor over the SPI register bridge.

use std::ops::Deref;
use std::sync::atomic::{AtomicBool, Ordering};
use std::sync::{Arc, Mutex};
use std::time::{Duration, Instant};

use super::device::{expect_report, DepzDevice, DeviceEvent, ReportHandler};
use super::error::{DepzError, Result};
use super::stream::{Fanout, Hook, Subscription};
use crate::framing::Packet;
use crate::tmf8829::{
    cfg_get, cfg_set, config_fields, fp_grid, pack_read_reg, pack_start_stream, pack_write_reg,
    preset_cmd, preset_kilo_iterations, FrameChunk, FrameReassembler, MeasurementBuilder, RegData,
    Tmf8829Cmd, Tmf8829Event, Tmf8829Info, Tmf8829Measurement, Tmf8829Rpt, CFG_SIZE,
    DEFAULT_PERIOD_MS, EV_BAD_HEADER, FIRMWARE_IMAGE, FIRMWARE_LOAD_ADDR, INT_HISTOGRAMS,
    INT_RESULTS, PRESETS, READ_MAX, REG_CFG, SF_REF_FRAME, WRITE_MAX,
};

// Host-interface registers (sensor bootloader and application).
const REG_APP_ID: u8 = 0x00;
const REG_CMD_STAT: u8 = 0x08;
const REG_SERIAL: u8 = 0x1C;
const REG_CID_RID: u8 = 0x20;
const REG_INT_STATUS: u8 = 0xE1;
const REG_INT_ENAB: u8 = 0xE2;
const REG_ID: u8 = 0xE3;
const REG_REVID: u8 = 0xE4;
const REG_RESET: u8 = 0xF7;
const REG_ENABLE: u8 = 0xF8;
const REG_FIFO_STATUS: u8 = 0xFA;
const REG_FIFO: u8 = 0xFF;

const DEVICE_ID: u8 = 0x9E;
const APP_ID_APP: u8 = 0x01;
const APP_ID_BL: u8 = 0x80;
const ENABLE_CPU_READY: u8 = 0x80;
const ENABLE_PUSEL_MASK: u8 = 0x30;
const ENABLE_PUSEL_BOOTMON: u8 = 0x10;
const ENABLE_PUSEL_RAM: u8 = 0x20;
const ENABLE_PON: u8 = 0x04;
const RESET_SOFT: u8 = 0x40;
const FIFO_ERROR_FLAGS: u8 = 0x30;

const BL_START_RAM_APP: u8 = 0x16;
const BL_I2C_OFF: u8 = 0x22;
const BL_W_FIFO_BOTH: u8 = 0x45;

const CMD_MEASURE: u8 = 0x10;
const CMD_WRITE_PAGE: u8 = 0x15;
const CMD_LOAD_CONFIG_PAGE: u8 = 0x16;
const CMD_STOP: u8 = 0xFF;
const STAT_OK: u8 = 0x00;
const STAT_ACCEPTED: u8 = 0x01;

const BL_STATUS_NAMES: &[(u8, &str)] = &[
    (0, "READY"),
    (1, "ERR_PARAM"),
    (2, "ERR_ADDR"),
    (4, "ERR_FIFO"),
];

const STAT_NAMES: &[(u8, &str)] = &[
    (0x00, "OK"),
    (0x01, "ACCEPTED"),
    (0x02, "ERR_CONFIG"),
    (0x03, "ERR_APPLICATION"),
    (0x04, "ERR_CONFIG_RESULT_SIZE"),
    (0x05, "ERR_CONFIG_VCSEL"),
    (0x06, "ERR_WAKEUP_TIMED"),
    (0x07, "ERR_RESET_UNEXPECTED"),
    (0x08, "ERR_UNKNOWN_CMD"),
    (0x09, "ERR_UNKNOWN_CID"),
    (0x0A, "ERR_STOP_0"),
    (0x0B, "ERR_STOP_1"),
    (0x0C, "ERR_STOP_2"),
    (0x0D, "ERR_STOP_3"),
    (0x0E, "ERR_OSC_TUNE"),
];

const FAST: Duration = Duration::from_millis(100);
const REGISTER_TIMEOUT: Duration = Duration::from_secs(1);
const APP_CMD_TIMEOUT: Duration = Duration::from_secs(1);

fn status_name(table: &[(u8, &str)], st: u8) -> String {
    table
        .iter()
        .find(|(v, _)| *v == st)
        .map_or_else(|| format!("0x{st:x}"), |(_, n)| n.to_string())
}

/// What [`Tmf8829Device::init`] reports: the RAM application version and the sensor's serial number.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct Tmf8829InitResult {
    pub app_version: String,
    pub sensor_serial: u32,
}

/// The config [`Tmf8829Device::configure`] wrote, as read back from the sensor: the preset name, the
/// 190-byte page 0x22..0xDF, whether histogram frames are dumped and a ref-SPAD frame comes with each
/// measurement, and every named field of the page (table order).
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct Tmf8829AppliedConfig {
    pub preset: String,
    pub page: Vec<u8>,
    pub histograms: bool,
    pub ref_frame: bool,
    pub fields: Vec<(&'static str, u16)>,
}

impl Tmf8829AppliedConfig {
    /// A named field of the page.
    pub fn field(&self, name: &str) -> Option<u16> {
        self.fields
            .iter()
            .find(|(n, _)| *n == name)
            .map(|(_, v)| *v)
    }

    /// (rows, cols) of a measurement.
    pub fn grid(&self) -> (usize, usize) {
        self.field("fp_mode")
            .and_then(|m| fp_grid(m as u8))
            .unwrap_or((0, 0))
    }
}

/// [`Tmf8829Device::configure`]'s options beyond the preset. `Default`: the default period, the
/// preset's `kilo_iterations`, one peak, nothing optional.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct Tmf8829Options {
    pub period_ms: u32,
    /// `None`: the preset's own.
    pub kilo_iterations: Option<u32>,
    /// 1..4.
    pub nr_peaks: u32,
    pub signal_strength: bool,
    pub noise_strength: bool,
    pub xtalk: bool,
    /// Dump histogram frames with each measurement.
    pub histograms: bool,
    /// A ref-SPAD frame before each results frame.
    pub ref_frame: bool,
    /// Any other page field by name, written last.
    pub fields: Vec<(String, u32)>,
}

impl Default for Tmf8829Options {
    fn default() -> Self {
        Tmf8829Options {
            period_ms: DEFAULT_PERIOD_MS as u32,
            kilo_iterations: None,
            nr_peaks: 1,
            signal_strength: false,
            noise_strength: false,
            xtalk: false,
            histograms: false,
            ref_frame: false,
            fields: Vec::new(),
        }
    }
}

struct Frames {
    reassembler: FrameReassembler,
    builder: MeasurementBuilder,
}

struct Streams {
    frames: Mutex<Frames>, // shared with the reader thread
    measurements: Fanout<Tmf8829Measurement>,
    events: Fanout<DeviceEvent>,
    stopping: AtomicBool,
}

impl ReportHandler for Streams {
    fn handle_report(&self, packet: &Packet) -> bool {
        if packet.cmd == Tmf8829Rpt::Frame as u8 {
            let m = {
                let Ok(chunk) = FrameChunk::unpack(&packet.payload) else {
                    return true;
                };
                let mut f = self.frames.lock().unwrap();
                let Some(raw) = f.reassembler.feed(chunk) else {
                    return true;
                };
                f.builder.add(&raw.data, raw.timestamp_us, raw.edge_us)
            };
            if let Some(m) = m {
                self.measurements.emit(m);
            }
            return true;
        }
        if packet.cmd == Tmf8829Rpt::Event as u8 {
            let Ok(ev) = Tmf8829Event::unpack(&packet.payload) else {
                return true;
            };
            if ev.code == EV_BAD_HEADER && self.stopping.load(Ordering::SeqCst) {
                return true; // STOP mid histogram dump: expected (contract 14 §2)
            }
            self.events.emit(DeviceEvent::Sensor {
                kind: Tmf8829Device::INTERRUPT_EVENT,
                code: ev.code,
                payload: packet.payload.clone(),
            });
            return true;
        }
        false
    }

    fn reader_stopped(&self) {
        self.measurements.close_all();
    }
}

/// TMF8829 multizone dToF (8×8 … 48×32 zones, up to 4 peaks per zone, histograms) over the SPI register
/// bridge (contract 14). The board is a thin bridge: this class runs the sensor.
/// [`Tmf8829Device::init`] downloads the 14.4 KB sensor firmware (~0.15 s),
/// [`Tmf8829Device::configure`] writes the config page, [`Tmf8829Device::start`] streams measurements
/// to [`Tmf8829Device::on_measurement`] / [`Tmf8829Device::measurements`], [`Tmf8829Device::stop`]
/// ends it. The sensor's EN pin is wired high: there is no sleep, and recovery from any state is
/// [`Tmf8829Device::init`] (it starts with a soft reset). Closing (or dropping) the device stops a
/// running stream first.
pub struct Tmf8829Device {
    dev: DepzDevice,
    streams: Arc<Streams>,
    reg: Mutex<()>, // the bridge takes one register command at a time
    streaming: AtomicBool,
    app_version: Mutex<Option<String>>,
    sensor_serial: Mutex<Option<u32>>,
    config: Mutex<Option<Tmf8829AppliedConfig>>,
}

impl Deref for Tmf8829Device {
    type Target = DepzDevice;

    fn deref(&self) -> &DepzDevice {
        &self.dev
    }
}

impl Drop for Tmf8829Device {
    fn drop(&mut self) {
        self.stop_before_close();
    }
}

impl Tmf8829Device {
    /// [`DeviceEvent::Sensor`] `kind` of an `RPT_TMF_EVENT` that is not an expected stop artefact: a
    /// MOTION / PROXIMITY interrupt (code 0) or a refused frame header (code 1 — the sensor is lost,
    /// run [`Tmf8829Device::init`] again). [`Tmf8829Device::interrupt_event`] decodes it.
    pub const INTERRUPT_EVENT: &'static str = "tmf8829_interrupt";

    /// Continue the open connection `dev` as a TMF8829.
    pub fn attach(dev: DepzDevice) -> Tmf8829Device {
        let streams = Arc::new(Streams {
            frames: Mutex::new(Frames {
                reassembler: FrameReassembler::new(),
                builder: MeasurementBuilder::new(),
            }),
            measurements: Fanout::new(dev.callback_errors_counter()),
            events: dev.core.events.clone(),
            stopping: AtomicBool::new(false),
        });
        dev.set_report_handler(streams.clone());
        Tmf8829Device {
            dev,
            streams,
            reg: Mutex::new(()),
            streaming: AtomicBool::new(false),
            app_version: Mutex::new(None),
            sensor_serial: Mutex::new(None),
            config: Mutex::new(None),
        }
    }

    /// The `RPT_TMF_EVENT` an [`Tmf8829Device::INTERRUPT_EVENT`] event carries (`None` for any other
    /// event).
    pub fn interrupt_event(event: &DeviceEvent) -> Option<Tmf8829Event> {
        match event {
            DeviceEvent::Sensor { kind, payload, .. } if *kind == Self::INTERRUPT_EVENT => {
                Tmf8829Event::unpack(payload).ok()
            }
            _ => None,
        }
    }

    /// The RAM application version [`Tmf8829Device::init`] started ("2.200.0").
    pub fn app_version(&self) -> Option<String> {
        self.app_version.lock().unwrap().clone()
    }

    /// The sensor's serial number, read by [`Tmf8829Device::init`].
    pub fn sensor_serial(&self) -> Option<u32> {
        *self.sensor_serial.lock().unwrap()
    }

    /// The last [`Tmf8829Device::configure`] result (`None` after [`Tmf8829Device::init`]).
    pub fn config(&self) -> Option<Tmf8829AppliedConfig> {
        self.config.lock().unwrap().clone()
    }

    pub fn streaming(&self) -> bool {
        self.streaming.load(Ordering::SeqCst)
    }

    // ── register access ──────────────────────────────────────────────────────
    // The `*_raw` forms run under `reg`, held by the caller.

    /// Read `length` bytes from `addr` in ≤1024-byte requests; reads that reach the FIFO port 0xFF stay
    /// there (the sensor does not auto-increment it). A reply shorter or longer than asked is
    /// [`DepzError::Protocol`].
    pub fn read_registers(&self, addr: u8, length: usize) -> Result<Vec<u8>> {
        let _g = self.reg.lock().unwrap();
        self.read_registers_raw(addr, length)
    }

    fn read_registers_raw(&self, addr: u8, length: usize) -> Result<Vec<u8>> {
        let mut out = Vec::with_capacity(length);
        let (mut addr, mut length) = (addr as usize, length);
        while length > 0 {
            let n = length.min(READ_MAX);
            let rep: RegData = self.request(
                Tmf8829Cmd::ReadReg as u8,
                &pack_read_reg(addr as u8, n as u16)?,
                expect_report(Tmf8829Rpt::RegData as u8, RegData::unpack),
                Some(REGISTER_TIMEOUT),
            )?;
            if rep.data.len() != n {
                return Err(DepzError::Protocol(format!(
                    "READ_REG 0x{addr:02X}: expected {n}, got {}",
                    rep.data.len()
                )));
            }
            out.extend_from_slice(&rep.data);
            addr = (addr + n).min(REG_FIFO as usize);
            length -= n;
        }
        Ok(out)
    }

    pub fn read_register(&self, addr: u8) -> Result<u8> {
        Ok(self.read_registers(addr, 1)?[0])
    }

    fn read_register_raw(&self, addr: u8) -> Result<u8> {
        Ok(self.read_registers_raw(addr, 1)?[0])
    }

    /// Write `data` from `addr` in ≤1023-byte requests (writes to 0xFF stay at 0xFF); `progress` gets
    /// (done, total) after each.
    pub fn write_registers(
        &self,
        addr: u8,
        data: &[u8],
        progress: Option<&mut dyn FnMut(usize, usize)>,
    ) -> Result<()> {
        let _g = self.reg.lock().unwrap();
        self.write_registers_raw(addr, data, progress)
    }

    fn write_registers_raw(
        &self,
        addr: u8,
        data: &[u8],
        mut progress: Option<&mut dyn FnMut(usize, usize)>,
    ) -> Result<()> {
        let mut addr = addr as usize;
        let mut done = 0;
        while done < data.len() {
            let n = (data.len() - done).min(WRITE_MAX);
            if addr > REG_FIFO as usize {
                return Err(DepzError::InvalidArgument(format!(
                    "TMF_WRITE_REG: addr 0x{addr:X} past 0xFF"
                )));
            }
            let payload = pack_write_reg(addr as u8, &data[done..done + n])?;
            self.request_ok(Tmf8829Cmd::WriteReg as u8, &payload, Some(REGISTER_TIMEOUT))?;
            done += n;
            if addr != REG_FIFO as usize {
                addr += n;
            }
            if let Some(p) = progress.as_mut() {
                p(done, data.len());
            }
        }
        Ok(())
    }

    /// Write one register.
    pub fn write_register(&self, addr: u8, value: u8) -> Result<()> {
        self.write_registers(addr, &[value], None)
    }

    fn write_register_raw(&self, addr: u8, value: u8) -> Result<()> {
        self.write_registers_raw(addr, &[value], None)
    }

    fn poll(
        &self,
        addr: u8,
        done: impl Fn(u8) -> bool,
        timeout: Duration,
        what: &str,
    ) -> Result<u8> {
        let deadline = Instant::now() + timeout;
        loop {
            let v = self.read_register_raw(addr)?;
            if done(v) {
                return Ok(v);
            }
            if Instant::now() > deadline {
                return Err(DepzError::Sensor(format!(
                    "{what}: timeout, reg 0x{addr:02X} = 0x{v:02X}"
                )));
            }
        }
    }

    /// Raw host-interface registers, for diagnostics (name, value; in a fixed order).
    pub fn sensor_state(&self) -> Result<Vec<(&'static str, u8)>> {
        let regs = [
            ("ID", REG_ID),
            ("REVID", REG_REVID),
            ("ENABLE", REG_ENABLE),
            ("RESET", REG_RESET),
            ("APP_ID", REG_APP_ID),
            ("CMD_STAT", REG_CMD_STAT),
            ("INT_STATUS", REG_INT_STATUS),
            ("INT_ENAB", REG_INT_ENAB),
            ("FIFOSTATUS", REG_FIFO_STATUS),
        ];
        regs.iter()
            .map(|&(n, r)| Ok((n, self.read_register(r)?)))
            .collect()
    }

    // ── bridge ───────────────────────────────────────────────────────────────

    /// Bridge counters (never touches the sensor, safe mid-stream).
    pub fn get_bridge_info(&self) -> Result<Tmf8829Info> {
        self.request(
            Tmf8829Cmd::GetInfo as u8,
            &[],
            expect_report(Tmf8829Rpt::Info as u8, Tmf8829Info::unpack),
            None,
        )
    }

    // ── init: sensor bootloader + firmware download ──────────────────────────

    fn bl_cmd(&self, cmd: u8, params: &[u8]) -> Result<()> {
        let mut payload = vec![cmd];
        if !params.is_empty() {
            payload.push(params.len() as u8);
            payload.extend_from_slice(params);
        }
        self.write_registers_raw(REG_CMD_STAT, &payload, None)?;
        let st = self.poll(
            REG_CMD_STAT,
            |v| v != cmd,
            FAST,
            &format!("bootloader cmd 0x{cmd:02X}"),
        )?;
        if st != 0 {
            return Err(DepzError::Sensor(format!(
                "bootloader cmd 0x{cmd:02X} -> {}",
                status_name(BL_STATUS_NAMES, st)
            )));
        }
        Ok(())
    }

    /// Bring the sensor from any state to its running RAM application: soft reset, bootloader, firmware
    /// download, start. Stops a running stream first. `progress` (done, total) tracks the image
    /// download. A sensor that refuses or does not answer is [`DepzError::Sensor`].
    pub fn init(
        &self,
        progress: Option<&mut dyn FnMut(usize, usize)>,
    ) -> Result<Tmf8829InitResult> {
        if self.streaming() {
            self.stop()?;
        }
        let res = {
            let _g = self.reg.lock().unwrap();
            self.write_register_raw(REG_ENABLE, ENABLE_PON | ENABLE_PUSEL_BOOTMON)?;
            std::thread::sleep(Duration::from_millis(3));
            self.write_register_raw(REG_RESET, RESET_SOFT)?;
            std::thread::sleep(Duration::from_millis(3));
            let dev = self.read_register_raw(REG_ID)?;
            if dev != DEVICE_ID {
                return Err(DepzError::Sensor(format!(
                    "sensor ID 0x{dev:02X}, expected 0x{DEVICE_ID:02X} (no TMF8829 on the bus?)"
                )));
            }
            self.poll(REG_APP_ID, |v| v == APP_ID_BL, FAST, "sensor bootloader")?;
            self.bl_cmd(BL_I2C_OFF, &[])?;
            // ENABLE reads 0 over SPI until the sensor's I2C is off.
            self.poll(REG_ENABLE, |v| v & ENABLE_CPU_READY != 0, FAST, "cpu_ready")?;
            let mut image = FIRMWARE_IMAGE.to_vec();
            image.resize(image.len().div_ceil(4) * 4, 0xFF);
            let words = (image.len() / 4) as u16;
            let mut params = FIRMWARE_LOAD_ADDR.to_le_bytes().to_vec();
            params.extend_from_slice(&words.to_le_bytes());
            self.bl_cmd(BL_W_FIFO_BOTH, &params)?;
            self.write_registers_raw(REG_FIFO, &image, progress)?;
            self.bl_cmd(BL_START_RAM_APP, &[])?;
            self.poll(REG_APP_ID, |v| v == APP_ID_APP, FAST, "START_RAM_APP")?;
            let en = self.read_register_raw(REG_ENABLE)?; // wake-ups must boot the RAM application
            self.write_register_raw(REG_ENABLE, (en & !ENABLE_PUSEL_MASK) | ENABLE_PUSEL_RAM)?;
            let ver = self.read_registers_raw(REG_APP_ID, 4)?;
            let s = self.read_registers_raw(REG_SERIAL, 4)?;
            Tmf8829InitResult {
                app_version: format!("{}.{}.{}", ver[1], ver[2], ver[3]),
                sensor_serial: u32::from_le_bytes([s[0], s[1], s[2], s[3]]),
            }
        };
        *self.app_version.lock().unwrap() = Some(res.app_version.clone());
        *self.sensor_serial.lock().unwrap() = Some(res.sensor_serial);
        *self.config.lock().unwrap() = None;
        Ok(res)
    }

    // ── application commands, config page ────────────────────────────────────

    // An application command; its status must be one of `accept`.
    fn app_cmd(&self, cmd: u8, accept: &[u8], timeout: Duration) -> Result<u8> {
        self.write_register_raw(REG_CMD_STAT, cmd)?;
        let st = self.poll(
            REG_CMD_STAT,
            |v| v < 0x10,
            timeout,
            &format!("sensor cmd 0x{cmd:02X}"),
        )?;
        if !accept.contains(&st) {
            return Err(DepzError::Sensor(format!(
                "sensor cmd 0x{cmd:02X} -> {}",
                status_name(STAT_NAMES, st)
            )));
        }
        Ok(st)
    }

    /// LOAD_CONFIG_PAGE → the 190-byte page 0x22..0xDF now in the sensor.
    pub fn config_page(&self) -> Result<Vec<u8>> {
        let _g = self.reg.lock().unwrap();
        self.config_page_raw()
    }

    fn config_page_raw(&self) -> Result<Vec<u8>> {
        self.app_cmd(CMD_LOAD_CONFIG_PAGE, &[STAT_OK], APP_CMD_TIMEOUT)?;
        let cid = self.read_register_raw(REG_CID_RID)?;
        if cid != CMD_LOAD_CONFIG_PAGE {
            return Err(DepzError::Sensor(format!(
                "CID_RID 0x{cid:02X} after LOAD_CONFIG_PAGE"
            )));
        }
        self.read_registers_raw(REG_CFG, CFG_SIZE)
    }

    /// Load `preset` (a name of [`PRESETS`]) and write the config page. Every field a preset does not
    /// reset is written (contract 14 §4), so the result never depends on an earlier config;
    /// `opts.fields` sets any other page field by name. Not while streaming
    /// ([`DepzError::InvalidState`]). [`DepzError::Sensor`]: the sensor refuses the page (e.g.
    /// ERR_CONFIG_RESULT_SIZE: the result format is too big for the grid) or a written field does not
    /// read back.
    pub fn configure(&self, preset: &str, opts: &Tmf8829Options) -> Result<Tmf8829AppliedConfig> {
        let Some(preset_command) = preset_cmd(preset) else {
            let names: Vec<&str> = PRESETS.iter().map(|(n, _)| *n).collect();
            return Err(DepzError::InvalidArgument(format!(
                "unknown preset '{preset}'; one of {}",
                names.join(", ")
            )));
        };
        if self.streaming() {
            return Err(DepzError::InvalidState(
                "configure() while streaming: stop() first".into(),
            ));
        }
        // The 3-bit field takes 0..7, the sensor reports at most 4.
        if !(1..=4).contains(&opts.nr_peaks) {
            return Err(DepzError::InvalidArgument(format!(
                "nr_peaks {}, must be 1..4",
                opts.nr_peaks
            )));
        }
        let kilo = opts
            .kilo_iterations
            .unwrap_or_else(|| preset_kilo_iterations(preset).unwrap() as u32);
        let mut want: Vec<(String, u32)> = vec![
            ("period_ms".into(), opts.period_ms),
            ("kilo_iterations".into(), kilo),
            ("nr_peaks".into(), opts.nr_peaks),
            ("signal_strength".into(), opts.signal_strength as u32),
            ("noise_strength".into(), opts.noise_strength as u32),
            ("xtalk".into(), opts.xtalk as u32),
            ("dump_histograms".into(), opts.histograms as u32),
            ("ref_spad_frame".into(), opts.ref_frame as u32),
        ];
        for (k, v) in &opts.fields {
            match want.iter_mut().find(|(n, _)| n == k) {
                Some(e) => e.1 = *v, // a later value replaces, in place (as a map's put)
                None => want.push((k.clone(), *v)),
            }
        }
        let back = {
            let _g = self.reg.lock().unwrap();
            self.app_cmd(preset_command, &[STAT_OK], APP_CMD_TIMEOUT)?;
            let mut page = self.config_page_raw()?;
            for (k, v) in &want {
                cfg_set(&mut page, k, *v)
                    .map_err(|e| DepzError::InvalidArgument(format!("{k}: {v}: {}", e.0)))?;
            }
            self.write_registers_raw(REG_CFG, &page, None)?;
            self.app_cmd(CMD_WRITE_PAGE, &[STAT_OK], APP_CMD_TIMEOUT)?;
            self.config_page_raw()?
        };
        let stuck: Vec<String> = want
            .iter()
            .filter_map(|(k, v)| {
                let got = cfg_get(&back, k).unwrap_or(0) as u32;
                (got != *v).then(|| format!("{k} wrote {v}, reads {got}"))
            })
            .collect();
        if !stuck.is_empty() {
            return Err(DepzError::Sensor(format!(
                "config fields did not stick: {}",
                stuck.join(", ")
            )));
        }
        let cfg = Tmf8829AppliedConfig {
            preset: preset.to_string(),
            fields: config_fields(&back),
            page: back,
            histograms: opts.histograms,
            ref_frame: opts.ref_frame,
        };
        *self.config.lock().unwrap() = Some(cfg.clone());
        Ok(cfg)
    }

    // ── streaming ────────────────────────────────────────────────────────────

    /// MEASURE with the bridge stream armed. Configures the default 8x8 preset first when
    /// [`Tmf8829Device::configure`] was not called since [`Tmf8829Device::init`]. No-op while streaming.
    pub fn start(&self) -> Result<()> {
        if self.streaming() {
            return Ok(());
        }
        let cfg = match self.config() {
            Some(c) => c,
            None => self.configure("8x8", &Tmf8829Options::default())?,
        };
        let int_mask = INT_RESULTS | if cfg.histograms { INT_HISTOGRAMS } else { 0 };
        let _g = self.reg.lock().unwrap();
        self.write_register_raw(REG_FIFO_STATUS, FIFO_ERROR_FLAGS)?; // sticky under/overrun
        self.write_register_raw(REG_INT_STATUS, 0xFF)?;
        self.write_register_raw(REG_INT_ENAB, int_mask)?;
        {
            let mut f = self.streams.frames.lock().unwrap();
            f.reassembler.reset();
            f.builder.reset();
        }
        let flags = if cfg.ref_frame { SF_REF_FRAME } else { 0 };
        self.request_ok(
            Tmf8829Cmd::StartStream as u8,
            &pack_start_stream(int_mask, flags)?,
            None,
        )?;
        self.streaming.store(true, Ordering::SeqCst);
        if let Err(e) = self.app_cmd(CMD_MEASURE, &[STAT_OK, STAT_ACCEPTED], APP_CMD_TIMEOUT) {
            self.streaming.store(false, Ordering::SeqCst);
            self.request_ok(Tmf8829Cmd::StopStream as u8, &[], None)?;
            return Err(e);
        }
        Ok(())
    }

    /// Sensor STOP, then the bridge stream (the frames already queued drain). The bad-header event a STOP
    /// raises mid histogram dump is expected and swallowed.
    pub fn stop(&self) -> Result<()> {
        let _g = self.reg.lock().unwrap();
        self.streams.stopping.store(true, Ordering::SeqCst);
        let r = (|| {
            self.app_cmd(CMD_STOP, &[STAT_OK], APP_CMD_TIMEOUT)?;
            self.write_register_raw(REG_INT_ENAB, 0)?;
            self.write_register_raw(REG_INT_STATUS, 0xFF)?;
            if self.streaming() {
                self.request_ok(Tmf8829Cmd::StopStream as u8, &[], None)?;
            }
            Ok(())
        })();
        self.streaming.store(false, Ordering::SeqCst);
        self.streams.stopping.store(false, Ordering::SeqCst);
        r
    }

    /// Stop a running stream (a failing stop is ignored), then close the link.
    pub fn close(&self) {
        self.stop_before_close();
        self.dev.close();
    }

    fn stop_before_close(&self) {
        if self.streaming() && !self.dev.is_closed() {
            let _ = self.stop(); // closing anyway
        }
    }

    /// Call `callback` for every measurement (reader thread; don't block).
    pub fn on_measurement(
        &self,
        callback: impl Fn(&Tmf8829Measurement) + Send + Sync + 'static,
    ) -> Hook {
        self.streams.measurements.on_item(callback)
    }

    /// A pull queue of measurements (bounded, drop-oldest); subscribed when this returns.
    pub fn measurements(&self, capacity: usize) -> Subscription<Tmf8829Measurement> {
        self.streams.measurements.subscribe(capacity)
    }

    /// The next measurement (contract 07 §3); `timeout` `None`: 2 s.
    pub fn get_measurement(&self, timeout: Option<Duration>) -> Result<Tmf8829Measurement> {
        self.streams.measurements.next(
            timeout.unwrap_or(Duration::from_secs(2)),
            Tmf8829Rpt::Frame as u8,
            self.is_closed(),
        )
    }

    pub fn stream_dropped_counts(&self) -> Vec<u64> {
        self.streams.measurements.dropped_counts()
    }

    /// Frames the bridge dropped (frame_seq gaps) in this stream.
    pub fn frames_dropped(&self) -> u64 {
        self.streams.frames.lock().unwrap().reassembler.seq_gaps
    }

    /// Frames lost to reassembly (incomplete, out-of-order chunks) in this stream.
    pub fn frames_discarded(&self) -> u64 {
        self.streams.frames.lock().unwrap().reassembler.discarded
    }

    pub fn frame_parse_errors(&self) -> u64 {
        self.streams.frames.lock().unwrap().builder.parse_errors
    }

    /// Current sensor-vs-board clock ratio in Q16 (contract 14 §5; nominal 8 << 16 = 8 µs per
    /// SYSTICK), for diagnostics.
    pub fn clock_ratio_q16(&self) -> u32 {
        self.streams.frames.lock().unwrap().builder.clock.ratio_q16
    }
}
