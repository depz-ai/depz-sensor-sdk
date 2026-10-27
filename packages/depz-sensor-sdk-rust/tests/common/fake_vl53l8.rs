//! A VL53L8 board on the wire for the device-class tests: the chip half and the APP_VL53L8 bridge
//! of the Python `tests/fake_vl53l8.py` (`FakeVl53l8Sensor` + `FakeVl53l8`) on a [`FakeBoard`].
//!
//! [`Sensor`] is the chip behind the SPI bridge — paged register map, the UI command area, the DCI
//! store, the firmware banks (the variant is whatever blob the host downloaded, told by comparing
//! it with the shipped `cx` / `ch` firmware), the power registers, the xtalk calibration run and a
//! deterministic scene rendered into frames laid out like the lab captures. [`FakeVl53l8`] answers
//! READ_REG / WRITE_REG / START_STREAM / STOP_STREAM (contract 04) and pushes frames in
//! `RPT_VL53_FRAME` chunks when the test calls [`FakeVl53l8::send_frame`].

use std::collections::HashMap;
use std::sync::{Arc, Mutex, MutexGuard};

use super::fake_board::FakeBoard;
use depz_sensor_sdk::protocol::common::Rpt;
use depz_sensor_sdk::vl53l8::driver::{Vl53l8Frame, Vl53l8Motion};

pub const UI_CMD_STATUS: u16 = 0x2C00;
pub const UI_CMD_START: u16 = 0x2C04;
pub const UI_CMD_END: u16 = 0x2FFF;

pub const DCI_ZONE_CONFIG: u16 = 0x5450;
pub const DCI_FREQ_HZ: u16 = 0x5458;
pub const DCI_INT_TIME: u16 = 0x545C;
pub const DCI_CAL_CFG: u16 = 0x5470;
pub const DCI_DET_THRESH_CONFIG: u16 = 0x5488;
pub const DCI_RANGING_MODE: u16 = 0xAD30;
pub const DCI_DSS_CONFIG: u16 = 0xAD38;
pub const DCI_XTALK_CFG: u16 = 0xAD94;
pub const DCI_TARGET_ORDER: u16 = 0xAE64;
pub const DCI_SHARPENER: u16 = 0xAED8;
pub const DCI_DET_THRESH_GLOBAL_CONFIG: u16 = 0xB6E0;
pub const DCI_DET_THRESH_START: u16 = 0xB6E8;
pub const DCI_DET_THRESH_VALID_STATUS: u16 = 0xB9F0;
pub const DCI_MOTION_DETECTOR_CFG: u16 = 0xBFAC;
pub const DCI_SINGLE_RANGE: u16 = 0xD964;
pub const DCI_OUTPUT_CONFIG: u16 = 0xD968;
pub const DCI_OUTPUT_ENABLES: u16 = 0xD970;
pub const DCI_OUTPUT_LIST: u16 = 0xD980;
pub const DCI_PIPE_CONTROL: u16 = 0xDB80;
pub const DCI_FW_FLAGS: u16 = 0xE0C4;

const METADATA_IDX: u32 = 0x54B4;
const COMMONDATA_IDX: u32 = 0x54C0;
const AMBIENT_RATE_IDX: u32 = 0x54D0;
const SPAD_COUNT_IDX: u32 = 0x55D0;
const NB_TARGET_DETECTED_IDX: u32 = 0xDB84;
const SIGNAL_RATE_IDX: u32 = 0xDBC4;
const RANGE_SIGMA_MM_IDX: u32 = 0xDEC4;
const DISTANCE_IDX: u32 = 0xDF44;
const REFLECTANCE_EST_PC_IDX: u32 = 0xE044;
const TARGET_STATUS_IDX: u32 = 0xE084;
const MOTION_DETEC_IDX: u32 = 0xD858;
const CNH_DATA_IDX: u32 = 0xC048;
const START_BH: u32 = 0x0000000D;

pub const NVM_DATA_SIZE: usize = 492;
pub const XTALK_BUFFER_SIZE: usize = 776;
const NB_THRESHOLDS: usize = 64;

/// Header/footer id the fake stamps on every frame (the lab VL53L8CX streamed 0x0017).
pub const FRAME_ID: u16 = 0x0017;
/// Deterministic scene: a plane tilted away from the sensor, 25 mm per zone step.
pub const SCENE_BASE_MM: i32 = 400;
pub const SCENE_STEP_MM: i32 = 25;
pub const SILICON_TEMP_DEGC: i8 = 37;
pub const MODULE_TYPE_MZPLUS: u8 = 2;
const FW_BANK_PAGES: [u8; 3] = [0x09, 0x0A, 0x0B];
const STATUS_READY: [u8; 4] = [0x02, 0x03, 0x00, 0x00]; // NVM poll wants [0]==2, DCI polls [1]==3
const UI_CMD_START_RANGING: [u8; 4] = [0x00, 0x03, 0x00, 0x00];
pub const CNH_REF_RESIDUAL_RAW: u32 = 3 * 2048;

pub const FW_CX: &[u8] = include_bytes!("../../src/vl53l8/data/cx/firmware.bin");
pub const FW_CH: &[u8] = include_bytes!("../../src/vl53l8/data/ch/firmware.bin");
pub const DEFAULT_XTALK: &[u8] = include_bytes!("../../src/vl53l8/data/cx/default_xtalk.bin");

/// Bridge status codes (contract 04 §2).
pub const ERR_INVALID_CMD: u8 = 0x02;
pub const ERR_PAYLOAD_FORMAT: u8 = 0x03;
pub const ERR_INVALID_PARAM: u8 = 0x04;
pub const READ_MAX: usize = 2295;
pub const WRITE_MAX: usize = 2302;
pub const STREAM_CHUNK: usize = 1528;

/// `VL53L8CX_SwapBuffer`: each 32-bit word byte-reversed.
pub fn swap_buffer(d: &[u8]) -> Vec<u8> {
    let mut out = d.to_vec();
    for i in (0..out.len() / 4 * 4).step_by(4) {
        out[i..i + 4].reverse();
    }
    out
}

fn u32s_le(d: &[u8]) -> Vec<u32> {
    (0..d.len() / 4)
        .map(|i| u32::from_le_bytes([d[4 * i], d[4 * i + 1], d[4 * i + 2], d[4 * i + 3]]))
        .collect()
}

fn ljust(mut d: Vec<u8>, n: usize) -> Vec<u8> {
    d.resize(n, 0);
    d
}

#[derive(Clone, Copy, Debug)]
enum Pending {
    Dci(u16, usize),
    Nvm,
    Xtalk,
}

/// The VL53L8 chip behind the bridge. Deterministic; no threads, no clock.
pub struct Sensor {
    pub page: u8,
    pub reg: HashMap<(u8, u16), u8>,
    pub fw_banks: HashMap<u8, Vec<u8>>,
    /// Set by the firmware the host downloaded (`"cx"` / `"ch"`).
    pub variant: Option<&'static str>,
    pub boots: u32,
    pub writes: Vec<(u8, u16, Vec<u8>)>,
    pub reads: Vec<(u8, u16, usize)>,
    pub offset_uploads: Vec<Vec<u8>>,
    pub xtalk_uploads: Vec<Vec<u8>>,
    pub config_uploads: u32,
    pub calib_tables: Vec<Vec<u8>>,
    /// (distance_mm*4, reflectance*16, nb_samples) per calibration run.
    pub calibrations: Vec<(u16, u16, u8)>,
    pub dci: HashMap<u16, Vec<u8>>,
    pub dci_writes: Vec<u16>,
    pub nvm: Vec<u8>,
    pub xtalk_buf: Vec<u8>,
    pub calibrated_buf: Option<Vec<u8>>,
    pub ranging: bool,
    pub frame_count: u32,
    pub latched: Vec<u8>,
    pub motion_configured: bool,
    // knobs
    pub distance_offset_mm: i32,
    pub silicon_temp_degc: i8,
    pub xtalk_cal_fails: bool,
    pub calib_busy_polls: u32,
    pub laser_safety_fault: bool,
    pub module_type: u8,
    pub corrupt_firmware: bool,
    pub corrupt_footer: bool,
    pending: Option<Pending>,
    calib_armed: bool,
    calib_polls: u32,
}

impl Sensor {
    pub const DEVICE_ID: u8 = 0xF0;
    pub const REVISION_ID: u8 = 0x0C;

    pub fn new() -> Sensor {
        let mut s = Sensor {
            page: 0,
            reg: HashMap::new(),
            fw_banks: HashMap::new(),
            variant: None,
            boots: 0,
            writes: Vec::new(),
            reads: Vec::new(),
            offset_uploads: Vec::new(),
            xtalk_uploads: Vec::new(),
            config_uploads: 0,
            calib_tables: Vec::new(),
            calibrations: Vec::new(),
            dci: default_dci(),
            dci_writes: Vec::new(),
            nvm: (0..NVM_DATA_SIZE)
                .map(|i| ((i * 7 + 3) & 0xFF) as u8)
                .collect(),
            xtalk_buf: vec![0; XTALK_BUFFER_SIZE],
            calibrated_buf: None,
            ranging: false,
            frame_count: 0,
            latched: Vec::new(),
            motion_configured: false,
            distance_offset_mm: 0,
            silicon_temp_degc: SILICON_TEMP_DEGC,
            xtalk_cal_fails: false,
            calib_busy_polls: 1,
            laser_safety_fault: false,
            module_type: MODULE_TYPE_MZPLUS,
            corrupt_firmware: false,
            corrupt_footer: false,
            pending: None,
            calib_armed: false,
            calib_polls: 0,
        };
        // booted sensor, awake, host-visible silicon ids on page 0
        s.reg.insert((0, 0x00), Self::DEVICE_ID);
        s.reg.insert((0, 0x01), Self::REVISION_ID);
        s.reg.insert((0, 0x06), 0x01);
        s.reg.insert((0, 0x09), 0x04);
        s.reg.insert((1, 0x21), 0x04);
        s
    }

    pub fn r(&self, page: u8, addr: u16) -> u8 {
        *self.reg.get(&(page, addr)).unwrap_or(&0)
    }

    pub fn dci(&self, index: u16) -> &[u8] {
        self.dci.get(&index).map(|v| v.as_slice()).unwrap_or(&[])
    }

    // ── register bridge ─────────────────────────────────────────────────────

    pub fn wr_multi(&mut self, addr: u16, data: &[u8]) {
        self.writes.push((self.page, addr, data.to_vec()));
        if addr == 0x7FFF && data.len() == 1 {
            self.page = data[0];
            return;
        }
        if FW_BANK_PAGES.contains(&self.page) {
            let bank = self.fw_banks.entry(self.page).or_default();
            let end = addr as usize + data.len();
            if bank.len() < end {
                bank.resize(end, 0);
            }
            bank[addr as usize..end].copy_from_slice(data);
            return;
        }
        if self.page == 2 && self.ui_write(addr, data) {
            return;
        }
        for (i, &b) in data.iter().enumerate() {
            self.reg.insert((self.page, addr + i as u16), b);
        }
        if self.page == 0 {
            self.go1_side_effects(addr, data);
        }
    }

    fn go1_side_effects(&mut self, addr: u16, data: &[u8]) {
        for (i, &b) in data.iter().enumerate() {
            let a = addr + i as u16;
            if a == 0x14 {
                // provoke / undo MCU stop
                if b == 0x01 {
                    *self.reg.entry((0, 0x06)).or_default() |= 0x80;
                    self.reg.insert((0, 0x07), 0x84);
                    self.ranging = false;
                } else {
                    *self.reg.entry((0, 0x06)).or_default() &= !0x80;
                }
            } else if a == 0x0B && b == 0x01 {
                self.boot(); // MCU reset: the downloaded FW boots
            } else if a == 0x09 {
                // power: 0x02 sleep (MCU down), 0x04 awake, 0x05 xshut bypass
                if b == 0x02 {
                    *self.reg.entry((0, 0x06)).or_default() &= !0x01;
                } else if b == 0x04 || b == 0x05 {
                    *self.reg.entry((0, 0x06)).or_default() |= 0x01;
                }
            }
        }
    }

    fn boot(&mut self) {
        self.boots += 1;
        let mut fw: Vec<u8> = FW_BANK_PAGES
            .iter()
            .flat_map(|p| self.fw_banks.get(p).cloned().unwrap_or_default())
            .collect();
        if self.corrupt_firmware && fw.len() > 100 {
            fw[100] ^= 0xFF;
        }
        self.variant = if fw == FW_CX {
            Some("cx")
        } else if fw == FW_CH {
            Some("ch")
        } else {
            None
        };
        let crc: u32 = match self.variant {
            Some("cx") => 0xCADF7CAF,
            Some("ch") => 0x0C0B6C9E,
            _ => 0,
        };
        for (i, b) in crc.to_be_bytes().iter().enumerate() {
            self.reg.insert((2, 0x2FFC + i as u16), *b); // read back swapped → crc
        }
        self.reg.insert((0, 0x06), 0x01);
        self.ranging = false;
        self.latched.clear();
    }

    fn ui_write(&mut self, addr: u16, data: &[u8]) -> bool {
        if addr == UI_CMD_END - 11 && data.len() == 12 {
            let index = (data[0] as u16) << 8 | data[1] as u16;
            let size = (data[2] as usize) << 4 | (data[3] as usize) >> 4;
            self.pending = Some(Pending::Dci(index, size));
            return true;
        }
        if addr == 0x2FD8 {
            self.pending = Some(Pending::Nvm);
            return true;
        }
        if addr == 0x2FB8 {
            self.pending = Some(Pending::Xtalk);
            return true;
        }
        if addr == 0x2E18 {
            self.offset_uploads.push(data.to_vec());
            return true;
        }
        if addr == 0x2CF8 {
            self.xtalk_uploads.push(data.to_vec());
            self.xtalk_buf = data.to_vec();
            return true;
        }
        if addr == 0x2C34 {
            self.config_uploads += 1;
            return true;
        }
        if addr == 0x2C28 {
            self.calib_tables.push(data.to_vec());
            self.calib_armed = true;
            return true;
        }
        if addr == UI_CMD_END - 3 && data == UI_CMD_START_RANGING {
            if self.calib_armed {
                self.run_xtalk_calibration();
            } else {
                self.ranging = true;
                self.frame_count = 0;
            }
            return true;
        }
        let n = data.len();
        if n >= 12 && data[n - 5..n - 2] == [0x0F, 0x05, 0x01] {
            // DCI write: header(4) + swap(payload) + footer(8)
            let index = (data[0] as u16) << 8 | data[1] as u16;
            let size = (data[2] as usize) << 4 | (data[3] as usize) >> 4;
            let payload = swap_buffer(&data[4..(4 + size).min(n)]);
            // the same 156-byte struct carries the motion indicator (cnh_cfg byte 0) and a CNH
            // setup (cnh_cfg flags set)
            if index == DCI_MOTION_DETECTOR_CFG {
                self.motion_configured = payload[25] == 0;
            }
            self.dci.insert(index, payload);
            self.dci_writes.push(index);
            return true;
        }
        false
    }

    pub fn rd_multi(&mut self, addr: u16, size: usize) -> Vec<u8> {
        self.reads.push((self.page, addr, size));
        if addr == 0x7FFF {
            return ljust(vec![self.page], size);
        }
        if self.page == 2 {
            if addr == UI_CMD_STATUS {
                let mut v = STATUS_READY.to_vec();
                v.truncate(size);
                return ljust(v, size);
            }
            if addr == UI_CMD_START {
                if let Some(p) = self.pending.take() {
                    let mut v = self.ui_answer(p);
                    v.truncate(size);
                    return ljust(v, size);
                }
            }
            if addr < 0x2C00 {
                return self.result_regs(addr, size);
            }
        }
        (0..size)
            .map(|i| self.r(self.page, addr + i as u16))
            .collect()
    }

    fn ui_answer(&self, p: Pending) -> Vec<u8> {
        match p {
            Pending::Nvm => self.nvm.clone(),
            Pending::Xtalk => {
                let mut v = vec![0u8; 8];
                v.extend_from_slice(&self.xtalk_buf[..XTALK_BUFFER_SIZE - 8]);
                v.extend_from_slice(&[0; 4]);
                v
            }
            Pending::Dci(index, size) => {
                let mut v = vec![0u8; 4];
                v.extend(self.dci_read(index, size));
                v.extend_from_slice(&[0; 8]);
                swap_buffer(&v)
            }
        }
    }

    fn dci_read(&self, index: u16, size: usize) -> Vec<u8> {
        let data = if index == 0x5440 {
            // the FW reports the frame size it will stream
            let configured = u32s_le(self.dci(DCI_OUTPUT_CONFIG))[0];
            let mut d = vec![0u8; 8];
            d.extend_from_slice(&(configured as u16).to_le_bytes());
            d.extend_from_slice(&[0; 2]);
            d
        } else if index == DCI_FW_FLAGS {
            let mut d = vec![0u8; 8];
            d[1] = self.module_type;
            d[6] = self.laser_safety_fault as u8;
            d
        } else {
            self.dci(index).to_vec()
        };
        let mut d = data;
        d.truncate(size);
        ljust(d, size)
    }

    fn result_regs(&mut self, addr: u16, size: usize) -> Vec<u8> {
        if self.calib_polls > 0 {
            self.calib_polls -= 1;
            return vec![0xFF; size];
        }
        let a = (addr as usize).min(self.latched.len());
        let end = (a + size).min(self.latched.len());
        ljust(self.latched[a..end].to_vec(), size)
    }

    // ── xtalk calibration (plugin_xtalk) ────────────────────────────────────

    fn run_xtalk_calibration(&mut self) {
        self.calib_armed = false;
        let cfg = self.dci(DCI_CAL_CFG).to_vec();
        let dist = u16::from_le_bytes([cfg[0], cfg[1]]);
        let refl = u16::from_le_bytes([cfg[2], cfg[3]]);
        let nb = cfg[4];
        self.calibrations.push((dist, refl, nb));
        self.calib_polls = self.calib_busy_polls;
        if self.xtalk_cal_fails {
            self.latched = vec![0x01, 0x05, 0x7F, 0x90]; // GO2 error: XTALK_FAILED
            return;
        }
        self.latched = vec![0x01, 0x05, 0x05, 0x10];
        let buf = self.calibrated_xtalk(dist as u32 / 4, refl as u32 / 16, nb as u32);
        self.xtalk_buf = buf.clone();
        self.calibrated_buf = Some(buf);
    }

    /// Signal-grid entry the fake's calibration computes (deterministic).
    pub fn xtalk_grid_value(distance_mm: u32, reflectance_pc: u32, nb: u32, zone: u32) -> u32 {
        reflectance_pc
            .wrapping_mul(1000)
            .wrapping_add(distance_mm)
            .wrapping_add(nb * 16)
            .wrapping_add(zone)
    }

    fn calibrated_xtalk(&self, distance_mm: u32, reflectance_pc: u32, nb: u32) -> Vec<u8> {
        let mut buf = swap_buffer(&ljust(self.xtalk_buf.clone(), XTALK_BUFFER_SIZE));
        for z in 0..64u32 {
            let v = Self::xtalk_grid_value(distance_mm, reflectance_pc, nb, z);
            let o = 0x34 + 4 * z as usize;
            buf[o..o + 4].copy_from_slice(&v.to_le_bytes());
        }
        swap_buffer(&buf)
    }

    // ── scene → frame ───────────────────────────────────────────────────────

    pub fn resolution(&self) -> usize {
        let zc = self.dci(DCI_ZONE_CONFIG);
        zc[0] as usize * zc[1] as usize
    }

    pub fn frequency_hz(&self) -> u32 {
        self.dci(DCI_FREQ_HZ)[1] as u32
    }

    fn cnh_dims(&self) -> Option<(usize, usize)> {
        let list = u32s_le(self.dci(DCI_OUTPUT_LIST));
        if !list.iter().any(|bh| bh >> 16 == CNH_DATA_IDX) {
            return None;
        }
        let cfg = self.dci(DCI_MOTION_DETECTOR_CFG);
        // nb_of_aggregates, feature_length (CnhConfig.pack layout)
        Some((cfg[20] as usize, cfg[19] as usize))
    }

    /// What a correct host parses from the next frame (contract 04 §5 units); `timestamp_us` 0.
    pub fn expected_frame(&self) -> Vl53l8Frame {
        let r = self.resolution();
        let side = if r == 16 { 4 } else { 8 };
        let last = r - 1; // the far corner has no target
        let motion = self.motion_configured.then(|| Vl53l8Motion {
            global_indicator_1: 0x1234,
            global_indicator_2: 0,
            status: 0,
            nb_of_detected_aggregates: 1,
            nb_of_aggregates: 16,
            motion: (0..32).map(|a| 1000 * a).collect(),
        });
        Vl53l8Frame {
            timestamp_us: 0,
            resolution: r,
            silicon_temp_degc: self.silicon_temp_degc,
            distance_mm: (0..r)
                .map(|z| {
                    if z == last {
                        0
                    } else {
                        SCENE_BASE_MM
                            + SCENE_STEP_MM * ((z / side) + (z % side)) as i32
                            + self.distance_offset_mm
                    }
                })
                .collect(),
            target_status: (0..r)
                .map(|z| match z {
                    _ if z == last => 255,
                    0 => 9,
                    _ => 5,
                })
                .collect(),
            nb_target_detected: (0..r).map(|z| if z == last { 0 } else { 1 }).collect(),
            signal_per_spad: (0..r).map(|z| 1200.0 + z as f64 / 4.0).collect(),
            ambient_per_spad: (0..r).map(|z| 3.0 + z as f64 / 8.0).collect(),
            nb_spads_enabled: vec![16384 / r as u32; r],
            range_sigma_mm: (0..r).map(|z| 2.0 + z as f64 / 4.0).collect(),
            reflectance: (0..r).map(|z| 30.0 + z as f64 / 2.0).collect(),
            cnh_raw: self.cnh_dims().map(|(a, f)| cnh_block(a, f)),
            motion,
        }
    }

    fn block_data(&self, idx: u32, msize: usize, r: usize) -> Vec<u8> {
        let exp = self.expected_frame();
        let u32s = |it: &mut dyn Iterator<Item = u32>| -> Vec<u8> {
            it.flat_map(|v| v.to_le_bytes()).collect()
        };
        match idx {
            METADATA_IDX => {
                let mut d = 0x2BDu32.to_le_bytes().to_vec();
                d.extend_from_slice(&[0; 4]);
                d.push(self.silicon_temp_degc as u8);
                d.extend_from_slice(&[0; 3]);
                d
            }
            COMMONDATA_IDX => 0x02400FCEu32.to_le_bytes().to_vec(),
            AMBIENT_RATE_IDX => u32s(&mut (0..r as u32).map(|z| 2048 * 3 + 256 * z)),
            SPAD_COUNT_IDX => u32s(&mut exp.nb_spads_enabled.iter().copied()),
            NB_TARGET_DETECTED_IDX => exp.nb_target_detected.clone(),
            SIGNAL_RATE_IDX => u32s(&mut (0..r as u32).map(|z| 2048 * 1200 + 512 * z)),
            RANGE_SIGMA_MM_IDX => (0..r as u16)
                .flat_map(|z| (256 + 32 * z).to_le_bytes())
                .collect(),
            DISTANCE_IDX => exp
                .distance_mm
                .iter()
                .zip(&exp.nb_target_detected)
                .flat_map(|(&d, &n)| {
                    // the empty zone holds a negative raw: clamped to 0 by the host
                    let raw: i16 = if n == 0 { -4 } else { (d * 4) as i16 };
                    raw.to_le_bytes()
                })
                .collect(),
            REFLECTANCE_EST_PC_IDX => (0..r).map(|z| 60 + z as u8).collect(),
            TARGET_STATUS_IDX => exp
                .target_status
                .iter()
                .map(|&s| if s == 255 { 0 } else { s })
                .collect(),
            MOTION_DETEC_IDX => match exp.motion {
                None => vec![0; msize],
                Some(m) => {
                    let mut d = m.global_indicator_1.to_le_bytes().to_vec();
                    d.extend_from_slice(&m.global_indicator_2.to_le_bytes());
                    d.extend_from_slice(&[
                        m.status,
                        m.nb_of_detected_aggregates,
                        m.nb_of_aggregates,
                        0,
                    ]);
                    d.extend(m.motion.iter().flat_map(|v| v.to_le_bytes()));
                    d
                }
            },
            CNH_DATA_IDX => {
                let block = exp.cnh_raw.expect("CNH block in the output list");
                assert_eq!(block.len(), msize);
                block
            }
            _ => vec![0; msize],
        }
    }

    /// One raw results frame (wire order) for the output list the host programmed.
    pub fn build_frame(&mut self) -> Vec<u8> {
        let variant = self.variant.unwrap_or("cx");
        let output = u32s_le(self.dci(DCI_OUTPUT_LIST));
        let enables = u32s_le(self.dci(DCI_OUTPUT_ENABLES));
        let size = u32s_le(self.dci(DCI_OUTPUT_CONFIG))[0] as usize;
        let r = self.resolution();
        self.frame_count += 1;
        let mut buf = vec![0x31, 0x9D, 0x05, self.frame_count as u8, 0, 0, 0, 0];
        buf.extend_from_slice(&FRAME_ID.to_le_bytes());
        buf.extend_from_slice(&[0; 2]);
        buf.extend_from_slice(&START_BH.to_le_bytes());
        for (i, &bh) in output.iter().enumerate().skip(1) {
            if (enables[i / 32] >> (i % 32)) & 1 == 0 {
                continue;
            }
            let (kind, bsize, idx) = (bh & 0xF, (bh >> 4) & 0xFFF, bh >> 16);
            let msize = if 0x1 < kind && kind < 0xD {
                kind * bsize
            } else {
                bsize
            } as usize;
            buf.extend_from_slice(&bh.to_le_bytes());
            buf.extend(ljust(self.block_data(idx, msize, r), msize));
        }
        buf.extend_from_slice(&0x0Eu32.to_le_bytes());
        buf.extend_from_slice(&0x0Fu32.to_le_bytes());
        assert!(buf.len() + 4 <= size, "{} vs {size}", buf.len());
        let mut buf = ljust(buf, size);
        let foff = if variant == "cx" { 12 } else { 4 };
        let footer_id = if self.corrupt_footer {
            FRAME_ID ^ 0xFFFF
        } else {
            FRAME_ID
        };
        self.corrupt_footer = false;
        buf[size - foff..size - foff + 2].copy_from_slice(&footer_id.to_le_bytes());
        swap_buffer(&buf)
    }

    /// Make the next frame readable at register 0x00 (poll mode, §6).
    pub fn latch_frame(&mut self) -> Vec<u8> {
        self.latched = self.build_frame();
        self.latched.clone()
    }
}

impl Default for Sensor {
    fn default() -> Self {
        Self::new()
    }
}

fn default_dci() -> HashMap<u16, Vec<u8>> {
    let mut dss = vec![0u8; 16];
    dss[4] = 64;
    dss[6] = 64;
    dss[9] = 4;
    let mut sharp = vec![0u8; 16];
    sharp[0xD] = (5 * 255 / 100) as u8;
    let mut int_time = 5000u32.to_le_bytes().to_vec();
    int_time.extend_from_slice(&[0; 16]);
    HashMap::from([
        (DCI_ZONE_CONFIG, vec![4, 4, 0, 0, 8, 8, 0, 0]),
        (DCI_DSS_CONFIG, dss),
        (DCI_FREQ_HZ, vec![0, 1, 0, 0]), // ST default: 1 Hz
        (DCI_INT_TIME, int_time),
        (DCI_RANGING_MODE, vec![0, 1, 0, 3, 0, 0, 0, 0]),
        (DCI_TARGET_ORDER, vec![1, 0, 0, 0]), // closest
        (DCI_SHARPENER, sharp),
        (DCI_XTALK_CFG, vec![0; 16]),
        (DCI_DET_THRESH_GLOBAL_CONFIG, vec![1, 0, 1, 0, 0, 0, 0, 0]),
        (DCI_DET_THRESH_CONFIG, vec![0; 20]),
        (DCI_DET_THRESH_START, vec![0; NB_THRESHOLDS * 12]),
        (DCI_DET_THRESH_VALID_STATUS, vec![0; 8]),
        (DCI_PIPE_CONTROL, vec![1, 0, 1, 0]),
        (DCI_SINGLE_RANGE, vec![1, 0, 0, 0]),
        (DCI_CAL_CFG, vec![0; 8]),
    ])
}

/// The per-aggregate histogram the fake's CNH block carries: (hist_raw, hist_scaler, ambient raw).
pub fn expected_cnh(nb_agg: usize, feat: usize) -> Vec<(Vec<i32>, Vec<i8>, i32)> {
    (0..nb_agg)
        .map(|a| {
            let raw = (0..feat).map(|b| 100 * (a as i32 + 1) + b as i32).collect();
            (raw, vec![(a % 2) as i8; feat], 7 + a as i32)
        })
        .collect()
}

/// A CNH data block (host byte order) for `nb_agg` aggregates × `feat` bins as the VL53LMZ CNH
/// pipeline lays it out with ping-pong and variance disabled (`cnh_block` of the Python fake).
pub fn cnh_block(nb_agg: usize, feat: usize) -> Vec<u8> {
    let pad4 = |n: usize| n.div_ceil(4) * 4;
    let agg_x_feat = nb_agg * feat;
    let pp_words = (2 * 4 + agg_x_feat * 4 + pad4(agg_x_feat) + nb_agg * 4 + pad4(nb_agg)) / 4;
    let mut out = Vec::new();
    for w in [
        0u32, // MI_STATE_PING
        pp_words as u32 | (0x01 << 24),
        CNH_REF_RESIDUAL_RAW,
        0x10,
        0,
    ] {
        out.extend_from_slice(&w.to_le_bytes());
    }
    let exp = expected_cnh(nb_agg, feat);
    let mut data: Vec<u8> = [0u32, 1].iter().flat_map(|w| w.to_le_bytes()).collect();
    for (raw, _, _) in &exp {
        data.extend(raw.iter().flat_map(|v| v.to_le_bytes()));
    }
    let frac: Vec<u8> = exp
        .iter()
        .flat_map(|(_, s, _)| s.iter().map(|&x| x as u8))
        .collect();
    data.extend(ljust(frac, pad4(agg_x_feat)));
    for (_, _, amb) in &exp {
        data.extend_from_slice(&amb.to_le_bytes());
    }
    data.extend(ljust(vec![1u8; nb_agg], pad4(nb_agg)));
    assert_eq!(data.len(), pp_words * 4);
    out.extend(data);
    out
}

/// The bridge firmware's state and its sensor.
pub struct Bridge {
    pub sensor: Sensor,
    pub streaming: bool,
    pub frame_size: usize,
    pub frames_sent: u32,
    pub chunks_sent: u32,
    /// READ/WRITE_REG seen between START and STOP_STREAM.
    pub bus_ops_while_streaming: u32,
    /// Bridge fault: answer READ_REG one byte short.
    pub truncate_reads: bool,
    /// Answer these commands with this status instead of running them.
    pub refuse: HashMap<u8, u8>,
    pub mcu_time_us: u64,
}

/// The in-process APP_VL53L8 bridge (contract 04) on a [`FakeBoard`].
pub struct FakeVl53l8 {
    pub board: FakeBoard,
    state: Arc<Mutex<Bridge>>,
}

impl FakeVl53l8 {
    pub fn new() -> FakeVl53l8 {
        let board = FakeBoard::new();
        let state = Arc::new(Mutex::new(Bridge {
            sensor: Sensor::new(),
            streaming: false,
            frame_size: 0,
            frames_sent: 0,
            chunks_sent: 0,
            bus_ops_while_streaming: 0,
            truncate_reads: false,
            refuse: HashMap::new(),
            mcu_time_us: 1_000_000,
        }));
        let st = state.clone();
        let b = board.clone();
        board.on_packet(move |p| {
            let mut s = st.lock().unwrap();
            let (rpt, payload) = s.handle(p.cmd, &p.payload);
            vec![b.reply(rpt, &payload)]
        });
        FakeVl53l8 { board, state }
    }

    /// The link to hand to `DepzDevice::new`.
    pub fn link(&self) -> FakeBoard {
        self.board.clone()
    }

    pub fn lock(&self) -> MutexGuard<'_, Bridge> {
        self.state.lock().unwrap()
    }

    /// One INT edge: the sensor's next frame as `RPT_VL53_FRAME` chunk(s), timestamps spaced by
    /// the configured ranging period. Returns what a correct host parses.
    pub fn send_frame(&self) -> Vl53l8Frame {
        let (expected, raw) = {
            let mut s = self.lock();
            assert!(
                s.streaming && s.sensor.ranging,
                "not streaming: START_STREAM + a ranging sensor first"
            );
            let mut expected = s.sensor.expected_frame();
            let raw = s.sensor.build_frame();
            assert_eq!(
                raw.len(),
                s.frame_size,
                "frame size vs START_STREAM frame_size"
            );
            s.mcu_time_us += 1_000_000 / s.sensor.frequency_hz().max(1) as u64;
            expected.timestamp_us = s.mcu_time_us;
            s.frames_sent += 1;
            (expected, raw)
        };
        for (i, part) in raw.chunks(STREAM_CHUNK).enumerate() {
            self.send_chunk(
                expected.timestamp_us,
                raw.len(),
                i * STREAM_CHUNK,
                part.to_vec(),
            );
        }
        expected
    }

    /// One raw `RPT_VL53_FRAME` chunk (reassembly-rule tests).
    pub fn send_chunk(&self, timestamp_us: u64, full_size: usize, offset: usize, data: Vec<u8>) {
        self.lock().chunks_sent += 1;
        let mut p = timestamp_us.to_le_bytes().to_vec();
        p.extend_from_slice(&(full_size as u16).to_le_bytes());
        p.extend_from_slice(&(offset as u16).to_le_bytes());
        p.extend(data);
        self.send_report(0x93, &p);
    }

    /// Any unsolicited report, raw.
    pub fn send_report(&self, report_id: u8, payload: &[u8]) {
        self.board.inject(self.board.reply(report_id, payload));
    }
}

impl Default for FakeVl53l8 {
    fn default() -> Self {
        Self::new()
    }
}

impl Bridge {
    fn status(cmd: u8, st: u8) -> (u8, Vec<u8>) {
        (Rpt::Status as u8, vec![cmd, st])
    }

    fn handle(&mut self, cmd: u8, payload: &[u8]) -> (u8, Vec<u8>) {
        self.mcu_time_us += 120; // one SPI transaction
        if let Some(&st) = self.refuse.get(&cmd) {
            return Self::status(cmd, st);
        }
        match cmd {
            0x32 => {
                // READ_REG
                if payload.len() != 4 {
                    return Self::status(cmd, ERR_PAYLOAD_FORMAT);
                }
                let addr = u16::from_le_bytes([payload[0], payload[1]]);
                let n = u16::from_le_bytes([payload[2], payload[3]]) as usize;
                if !(1..=READ_MAX).contains(&n) {
                    return Self::status(cmd, ERR_PAYLOAD_FORMAT);
                }
                if self.streaming {
                    self.bus_ops_while_streaming += 1;
                }
                let mut data = self.sensor.rd_multi(addr, n);
                if self.truncate_reads {
                    data.pop();
                }
                let mut out = vec![cmd];
                out.extend_from_slice(&self.mcu_time_us.to_le_bytes());
                out.extend(data);
                (0x91, out)
            }
            0x33 => {
                // WRITE_REG
                if payload.len() < 3 || payload.len() - 2 > WRITE_MAX {
                    return Self::status(cmd, ERR_PAYLOAD_FORMAT);
                }
                if self.streaming {
                    self.bus_ops_while_streaming += 1;
                }
                let addr = u16::from_le_bytes([payload[0], payload[1]]);
                self.sensor.wr_multi(addr, &payload[2..]);
                Self::status(cmd, 0)
            }
            0x35 => {
                // START_STREAM
                if payload.len() != 2 {
                    return Self::status(cmd, ERR_PAYLOAD_FORMAT);
                }
                let n = u16::from_le_bytes([payload[0], payload[1]]) as usize;
                if !(1..=8192).contains(&n) {
                    return Self::status(cmd, ERR_INVALID_PARAM);
                }
                self.frame_size = n;
                self.streaming = true;
                Self::status(cmd, 0)
            }
            0x36 => {
                // STOP_STREAM (idempotent)
                self.streaming = false;
                Self::status(cmd, 0)
            }
            // The common commands, as the Python fake's FakeSr04 base answers them.
            0x03..=0x05 => {
                let text = match cmd {
                    0x03 => DEVICE_NAME,
                    0x04 => SOFTWARE_NAME,
                    _ => SERIAL,
                };
                let mut p = vec![cmd];
                p.extend_from_slice(text.as_bytes());
                p.push(0);
                (Rpt::Text as u8, p)
            }
            0x06 if payload.len() == 8 => {
                // SYNC_TIME: a zero device-processing gap (t3 == t2)
                let t2 = self.mcu_time_us + 500;
                let mut p = payload.to_vec();
                p.extend_from_slice(&t2.to_le_bytes());
                p.extend_from_slice(&t2.to_le_bytes());
                (Rpt::SyncTime as u8, p)
            }
            0x07 => {
                let mut p = self.mcu_time_us.to_le_bytes().to_vec();
                p.extend_from_slice(&MCU_TEMPERATURE_RAW.to_le_bytes());
                (Rpt::Temperature as u8, p)
            }
            _ => Self::status(cmd, ERR_INVALID_CMD),
        }
    }
}

pub const SOFTWARE_NAME: &str = "APP_VL53L8_v0.92";
pub const DEVICE_NAME: &str = "DEPZ-VL53L8";
pub const SERIAL: &str = "SN000005";
/// GET_MCU_TEMPERATURE: 27.3 °C.
pub const MCU_TEMPERATURE_RAW: i16 = 273;
