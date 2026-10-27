//! The TMF8829 device class over a fake SPI register bridge + sensor (the Java Tmf8829DeviceTests, itself
//! the Python tests/test_tmf8829_device.py over fake_tmf8829.py).
mod common;

use std::collections::{HashMap, HashSet};
use std::sync::{Arc, Mutex};
use std::time::Duration;

use common::fake_board::{until, FakeBoard};
use depz_sensor_sdk::devices::{DepzDevice, DepzError, DeviceEvent, Tmf8829Device, Tmf8829Options};
use depz_sensor_sdk::framing::Packet;
use depz_sensor_sdk::protocol::common::Status;
use depz_sensor_sdk::tmf8829::{
    Tmf8829Event, Tmf8829Measurement, EV_BAD_HEADER, EV_INT, FIRMWARE_IMAGE,
};
use serde_json::{json, Value};

const W: u8 = 0x33;
const R: u8 = 0x32;

const REG_APP_ID: usize = 0x00;
const REG_CMD_STAT: usize = 0x08;
const REG_SERIAL: usize = 0x1C;
const REG_CID_RID: usize = 0x20;
const REG_CFG: usize = 0x22;
const CFG_SIZE: usize = 190;
const REG_INT_STATUS: usize = 0xE1;
const REG_ID: usize = 0xE3;
const REG_RESET: usize = 0xF7;
const REG_ENABLE: usize = 0xF8;
const REG_FIFO_STATUS: usize = 0xFA;
const REG_FIFO: usize = 0xFF;

/// fp_mode each preset command loads.
const PRESET_FP_MODE: &[(u8, u8)] = &[
    (0x40, 0),
    (0x41, 0),
    (0x42, 0),
    (0x43, 2),
    (0x44, 2),
    (0x45, 3),
    (0x46, 3),
    (0x47, 5),
    (0x48, 5),
    (0x49, 0),
];

type OnCommand = Arc<dyn Fn(u8) -> Vec<Vec<u8>> + Send + Sync>;

/// The sensor's register file and the fault injection.
struct Sensor {
    regs: [u8; 256],
    cfg: [u8; CFG_SIZE],
    fifo: Vec<u8>,
    w_fifo_params: Vec<u8>,
    mode: &'static str,
    i2c_off: bool,
    device_id: u8,
    bl_status: HashMap<u8, u8>,
    app_status: HashMap<u8, u8>,
    busy_polls: u32,
    stuck_before_bootloader: bool,
    cid_override: Option<u8>,
    stuck_bytes: HashMap<usize, u8>,
    bridge_status: HashMap<u8, u8>,
    silent: HashSet<u8>,
    short_read: usize,
    /// Packets sent after a CMD_STAT write is applied, before its WRITE_REG reply.
    on_command: Option<OnCommand>,
    streaming: bool,
    int_mask: u8,
    flags: u8,
    pending_cmd: u8,
    frame_seq: u8,
}

impl Sensor {
    fn read8(&mut self, a: usize) -> u8 {
        if a == REG_ID {
            return self.device_id;
        }
        if a == REG_ENABLE {
            // 0 over SPI until the sensor's I2C is off
            return if self.mode == "bootloader" && !self.i2c_off {
                0
            } else {
                self.regs[a] | 0x80
            };
        }
        if a == REG_CMD_STAT && self.busy_polls > 0 {
            self.busy_polls -= 1;
            return self.pending_cmd;
        }
        self.regs[a]
    }

    fn write_regs(&mut self, addr: usize, data: &[u8]) {
        if addr == REG_FIFO {
            self.fifo.extend_from_slice(data);
            return;
        }
        if addr == REG_CMD_STAT {
            self.command(data);
            return;
        }
        for (i, &b) in data.iter().enumerate() {
            let a = addr + i;
            if a == REG_RESET && b & 0x40 != 0 {
                self.soft_reset();
            } else if a == REG_INT_STATUS {
                self.regs[a] &= !b; // write-1-to-clear
            } else if a == REG_FIFO_STATUS {
                self.regs[a] &= !(b & 0x30);
            } else {
                self.regs[a] = b;
            }
        }
    }

    fn soft_reset(&mut self) {
        self.mode = "bootloader";
        self.i2c_off = false;
        self.regs[REG_APP_ID] = if self.stuck_before_bootloader {
            0x00
        } else {
            0x80
        };
        self.regs[REG_CMD_STAT] = 0;
        self.fifo.clear();
        self.cfg[0] = 33;
        self.cfg[1] = 0;
        self.cfg[0x2A - REG_CFG] = 1;
    }

    fn command(&mut self, data: &[u8]) {
        let cmd = data[0];
        self.pending_cmd = cmd;
        let st;
        if self.mode == "bootloader" {
            st = self.bl_status.get(&cmd).copied().unwrap_or(0);
            if st == 0 {
                if cmd == 0x22 {
                    self.i2c_off = true;
                } else if cmd == 0x45 {
                    self.w_fifo_params = data[2..2 + data[1] as usize].to_vec();
                } else if cmd == 0x16 {
                    self.mode = "app";
                    self.regs[REG_APP_ID] = 0x01;
                    self.regs[1] = 2;
                    self.regs[2] = 200;
                    self.regs[3] = 0;
                    self.regs[REG_SERIAL..REG_SERIAL + 4]
                        .copy_from_slice(&0x12345678u32.to_le_bytes());
                }
            }
        } else {
            st = self
                .app_status
                .get(&cmd)
                .copied()
                .unwrap_or(if cmd == 0x10 { 0x01 } else { 0x00 });
            if st <= 0x01 {
                self.app_command(cmd);
            }
        }
        self.regs[REG_CMD_STAT] = st;
    }

    fn app_command(&mut self, cmd: u8) {
        if let Some(&(_, fp)) = PRESET_FP_MODE.iter().find(|(c, _)| *c == cmd) {
            self.cfg[0x26 - REG_CFG] = fp;
            self.regs[REG_CID_RID] = cmd;
        } else if cmd == 0x16 {
            self.regs[REG_CFG..REG_CFG + CFG_SIZE].copy_from_slice(&self.cfg);
            self.regs[REG_CID_RID] = self.cid_override.unwrap_or(0x16);
        } else if cmd == 0x15 {
            self.cfg
                .copy_from_slice(&self.regs[REG_CFG..REG_CFG + CFG_SIZE]);
            for (&k, &v) in &self.stuck_bytes {
                self.cfg[k] = v;
            }
        }
    }
}

/// Fake TMF8829 SPI register bridge + sensor: the bridge answers READ_REG / WRITE_REG / START_STREAM /
/// STOP_STREAM / GET_INFO; the sensor is a 256-byte register file with the side effects the host sequence
/// relies on — soft reset → bootloader, bootloader commands at CMD_STAT, the firmware download through FIFO
/// 0xFF, application commands (presets, LOAD_CONFIG_PAGE, WRITE_PAGE, MEASURE, STOP) and the config page
/// with the "preset does not reset everything" behaviour.
struct FakeTmf8829 {
    link: FakeBoard,
    s: Arc<Mutex<Sensor>>,
}

impl FakeTmf8829 {
    fn new() -> FakeTmf8829 {
        let mut s = Sensor {
            regs: [0; 256],
            cfg: [0; CFG_SIZE],
            fifo: Vec::new(),
            w_fifo_params: Vec::new(),
            mode: "app-old",
            i2c_off: false,
            device_id: 0x9E,
            bl_status: HashMap::new(),
            app_status: HashMap::new(),
            busy_polls: 0,
            stuck_before_bootloader: false,
            cid_override: None,
            stuck_bytes: HashMap::new(),
            bridge_status: HashMap::new(),
            silent: HashSet::new(),
            short_read: 0,
            on_command: None,
            streaming: false,
            int_mask: 0,
            flags: 0,
            pending_cmd: 0,
            frame_seq: 0,
        };
        s.regs[REG_APP_ID] = 0x01; // an application left running by an earlier session
        s.cfg[0] = 33;
        s.cfg[2..4].copy_from_slice(&586u16.to_le_bytes());
        s.cfg[0x2A - REG_CFG] = 1;
        let f = FakeTmf8829 {
            link: FakeBoard::new(),
            s: Arc::new(Mutex::new(s)),
        };
        let (link, st) = (f.link.clone(), f.s.clone());
        f.link
            .on_packet(move |p| handle(&link, &mut st.lock().unwrap(), p));
        f
    }

    fn with<T>(&self, f: impl FnOnce(&mut Sensor) -> T) -> T {
        f(&mut self.s.lock().unwrap())
    }

    fn sent(&self, cmd: u8) -> Vec<Vec<u8>> {
        self.link
            .sent_snapshot()
            .into_iter()
            .filter(|p| p.cmd == cmd)
            .map(|p| p.payload)
            .collect()
    }

    fn writes(&self) -> Vec<Vec<u8>> {
        self.sent(W)
    }

    fn log_count(&self) -> usize {
        self.link.sent_count()
    }

    fn log_from(&self, n: usize) -> Vec<Packet> {
        self.link.sent_snapshot()[n..].to_vec()
    }

    fn emit_frame_at(
        &self,
        raw: &[u8],
        timestamp_us: u64,
        edge_us: u64,
        seq: Option<u8>,
        chunks: Option<&[usize]>,
    ) -> u8 {
        let mut st = self.s.lock().unwrap();
        let s = seq.unwrap_or(st.frame_seq);
        st.frame_seq = s.wrapping_add(1);
        let offsets: Vec<usize> = match chunks {
            Some(c) => c.to_vec(),
            None => (0..raw.len().div_ceil(1024)).map(|i| i * 1024).collect(),
        };
        for (i, &off) in offsets.iter().enumerate() {
            let end = offsets.get(i + 1).copied().unwrap_or(raw.len());
            let len = end.min(off + 1024) - off;
            let mut p = Vec::with_capacity(21 + len);
            p.extend_from_slice(&timestamp_us.to_le_bytes());
            p.extend_from_slice(&(raw.len() as u16).to_le_bytes());
            p.extend_from_slice(&(off as u16).to_le_bytes());
            p.push(s);
            p.extend_from_slice(&edge_us.to_le_bytes());
            p.extend_from_slice(&raw[off..off + len]);
            self.link.inject(self.link.reply(0x93, &p));
        }
        s
    }

    fn emit_frame(&self, raw: &[u8], timestamp_us: u64, edge_us: u64) -> u8 {
        self.emit_frame_at(raw, timestamp_us, edge_us, None, None)
    }

    fn emit_event(&self, code: u8, int_status: u8, frame_id: u8) {
        self.link
            .inject(event(&self.link, code, int_status, frame_id, 0));
    }

    fn open(&self) -> Tmf8829Device {
        Tmf8829Device::attach(DepzDevice::new(
            self.link.clone(),
            Some(Duration::from_millis(500)),
        ))
    }
}

fn event(link: &FakeBoard, code: u8, int_status: u8, frame_id: u8, fifo_status: u8) -> Vec<u8> {
    let mut p = 987654321u64.to_le_bytes().to_vec();
    p.extend_from_slice(&[int_status, code, frame_id, fifo_status]);
    link.reply(0x94, &p)
}

fn handle(link: &FakeBoard, s: &mut Sensor, pkt: &Packet) -> Vec<Vec<u8>> {
    let (cmd, payload) = (pkt.cmd, &pkt.payload);
    if s.silent.contains(&cmd) {
        return vec![];
    }
    if let Some(&bs) = s.bridge_status.get(&cmd) {
        return vec![FakeBoard::status(cmd, bs)];
    }
    match cmd {
        0x32 => {
            let addr = payload[0] as usize;
            let n = u16::from_le_bytes([payload[1], payload[2]]) as usize;
            let m = n.saturating_sub(s.short_read);
            let mut p = vec![0x32];
            p.extend_from_slice(&5_000_000u64.to_le_bytes());
            for i in 0..m {
                p.push(s.read8((addr + i).min(REG_FIFO)));
            }
            vec![link.reply(0x91, &p)]
        }
        0x33 => {
            s.write_regs(payload[0] as usize, &payload[1..]);
            let mut out = Vec::new();
            if payload[0] as usize == REG_CMD_STAT {
                if let Some(oc) = s.on_command.clone() {
                    out.extend(oc(payload[1]));
                }
            }
            out.push(FakeBoard::status(cmd, Status::Ok as u8));
            out
        }
        0x35 => {
            s.streaming = true;
            s.int_mask = payload[0];
            s.flags = payload[1];
            s.frame_seq = 0;
            vec![FakeBoard::status(cmd, Status::Ok as u8)]
        }
        0x36 => {
            s.streaming = false;
            vec![FakeBoard::status(cmd, Status::Ok as u8)]
        }
        0x37 => {
            let mut p = vec![0u8; 25];
            p[0..4].copy_from_slice(&7u32.to_le_bytes());
            p[4..8].copy_from_slice(&6u32.to_le_bytes());
            p[8..12].copy_from_slice(&1u32.to_le_bytes());
            p[20] = 1;
            p[21] = s.streaming as u8;
            p[22] = s.int_mask;
            p[23] = 0x01;
            p[24] = 0x00;
            vec![link.reply(0x92, &p)]
        }
        _ => vec![FakeBoard::status(cmd, Status::ErrInvalidCmd as u8)],
    }
}

// ── helpers ──────────────────────────────────────────────────────────────

fn make() -> (FakeTmf8829, Tmf8829Device) {
    let fake = FakeTmf8829::new();
    let dev = fake.open();
    (fake, dev)
}

fn wait_for<T>(list: &Mutex<Vec<T>>, n: usize) {
    until(|| list.lock().unwrap().len() >= n, 2000);
    assert_eq!(list.lock().unwrap().len(), n, "items");
}

fn collect(dev: &Tmf8829Device) -> Arc<Mutex<Vec<Tmf8829Measurement>>> {
    let got = Arc::new(Mutex::new(Vec::new()));
    let g = got.clone();
    dev.on_measurement(move |m| g.lock().unwrap().push(m.clone()))
        .forget();
    got
}

fn sleep_ms(ms: u64) {
    std::thread::sleep(Duration::from_millis(ms));
}

fn sensor_message(r: Result<impl std::fmt::Debug, DepzError>) -> String {
    match r {
        Err(DepzError::Sensor(m)) => m,
        other => panic!("expected a sensor error, got {other:?}"),
    }
}

fn is_invalid_argument<T: std::fmt::Debug>(r: Result<T, DepzError>) -> bool {
    matches!(r, Err(DepzError::InvalidArgument(_)))
}

// ── vectors: contracts/vectors/tmf8829.json ──────────────────────────────

struct Vectors {
    raw: HashMap<String, Vec<u8>>,
    meas: Vec<(String, Value)>,
    corrupt: Vec<String>, // results frames whose pixel decode fails
}

fn vectors() -> Vectors {
    let v = common::load("tmf8829.json");
    let raw = v["frames"]
        .as_array()
        .unwrap()
        .iter()
        .map(|c| {
            (
                c["name"].as_str().unwrap().to_string(),
                common::hex_decode(c["raw"].as_str().unwrap()),
            )
        })
        .collect();
    let meas = v["measurements"]
        .as_array()
        .unwrap()
        .iter()
        .map(|m| (m["name"].as_str().unwrap().to_string(), m.clone()))
        .collect();
    let corrupt = v["frames"]
        .as_array()
        .unwrap()
        .iter()
        .filter(|c| c.get("error").is_some())
        .map(|c| c["name"].as_str().unwrap().to_string())
        .collect();
    Vectors { raw, meas, corrupt }
}

impl Vectors {
    fn raw(&self, name: &str) -> &[u8] {
        &self.raw[name]
    }

    fn meas(&self, name: &str) -> &Value {
        &self.meas.iter().find(|(n, _)| n == name).unwrap().1
    }

    fn play(&self, fake: &FakeTmf8829, name: &str) {
        for f in self.meas(name)["frames"].as_array().unwrap() {
            fake.emit_frame(
                self.raw(f["raw_case"].as_str().unwrap()),
                f["timestamp_us"].as_u64().unwrap(),
                f["edge_us"].as_u64().unwrap(),
            );
        }
    }
}

/// A measurement in the vector's field shape (the same fields the vector tests compare for tmf8829.json
/// "measurements"); the ratio is the one the measurement carries (no shadow builder's clock).
fn meas_fields(m: &Tmf8829Measurement) -> Value {
    let mut got = json!({
        "timestamp_us": m.timestamp_us,
        "flags": m.flags,
        "fp_mode": m.fp_mode,
        "f_number": m.f_number,
        "frame_times_us": m.times.iter().map(|t| t.timestamp_us).collect::<Vec<_>>(),
        "t1_len_us": m.times.iter().map(|t| t.t1_len_us).collect::<Vec<_>>(),
        "ratio_q16": m.times.last().unwrap().ratio_q16,
        "n_histograms": m.histograms.len(),
        "n_ref_frames": m.ref_frames.len(),
        "n_problems": m.problems.len(),
        "distance_in_mm": m.frames.last().unwrap().distance_in_mm(),
    });
    got.as_object_mut().unwrap().extend(
        common::shapes::arrays(&m.results)
            .as_object()
            .unwrap()
            .clone(),
    );
    got
}

/// The same field set, every value equal.
fn assert_fields(name: &str, expect: &Value, got: &Value) {
    let (e, g) = (expect.as_object().unwrap(), got.as_object().unwrap());
    let mut ek: Vec<&String> = e.keys().collect();
    let mut gk: Vec<&String> = g.keys().collect();
    ek.sort();
    gk.sort();
    assert_eq!(ek, gk, "{name}: field set");
    for (k, v) in g {
        assert_eq!(&e[k], v, "{name}.{k}");
    }
}

// ── tests ────────────────────────────────────────────────────────────────

#[test]
fn init_runs_the_contract_sequence_and_downloads_the_firmware() {
    let (fake, dev) = make();
    let mut progress = Vec::new();
    let res = dev.init(Some(&mut |d, t| progress.push((d, t)))).unwrap();
    assert_eq!(res.app_version, "2.200.0");
    assert_eq!(res.sensor_serial, 0x12345678);
    assert_eq!(dev.app_version().as_deref(), Some("2.200.0"));
    assert_eq!(dev.sensor_serial(), Some(0x12345678));
    assert_eq!(dev.config(), None);

    let w = fake.writes();
    assert_eq!(w[0], [0xF8, 0x14]); // PON | bootmonitor
    assert_eq!(w[1], [0xF7, 0x40]); // soft reset
    assert_eq!(fake.sent(R)[0], [0xE3, 1, 0]);
    assert_eq!(w[2], [0x08, 0x22]); // BL I2C_OFF
    assert_eq!(
        w[3],
        [
            0x08,
            0x45,
            6,
            0x00,
            0x00,
            0x01,
            0x00,
            (3611 & 0xFF) as u8,
            (3611 >> 8) as u8
        ]
    );
    assert_eq!(
        fake.with(|s| s.w_fifo_params.clone()),
        [0x00, 0x00, 0x01, 0x00, 0x1B, 0x0E]
    );
    let fifo: Vec<usize> = (0..w.len()).filter(|&i| w[i][0] == 0xFF).collect();
    assert_eq!(fifo.len(), 14444_usize.div_ceil(1023), "fifo writes");
    assert_eq!(
        fake.with(|s| s.fifo.clone()),
        FIRMWARE_IMAGE,
        "downloaded image"
    );
    let after = &w[fifo[fifo.len() - 1] + 1..];
    assert_eq!(after[0], [0x08, 0x16]); // START_RAM_APP
    assert_eq!(after[1], [0xF8, 0xA4]); // powerup_select = RAM
    assert_eq!(progress.len(), fifo.len(), "progress calls");
    assert_eq!(progress.last(), Some(&(14444, 14444)));
    let reads = fake.sent(R);
    assert_eq!(
        reads[reads.len() - 2..],
        [vec![0x00, 4, 0], vec![0x1C, 4, 0]]
    );
    assert_eq!(fake.with(|s| s.mode), "app");
}

#[test]
fn init_survives_a_busy_cmd_stat_and_refuses_a_wrong_sensor() {
    let (fake, dev) = make();
    fake.with(|s| s.busy_polls = 3);
    assert_eq!(dev.init(None).unwrap().app_version, "2.200.0");
    fake.with(|s| s.device_id = 0x00);
    let m = sensor_message(dev.init(None));
    assert!(m.contains("sensor ID 0x00, expected 0x9E"), "{m}");
    fake.with(|s| {
        s.device_id = 0x9E;
        s.bl_status.insert(0x45, 2);
    });
    assert_eq!(
        sensor_message(dev.init(None)),
        "bootloader cmd 0x45 -> ERR_ADDR"
    );
    fake.with(|s| {
        s.bl_status.clear();
        s.bl_status.insert(0x16, 7);
    });
    assert_eq!(sensor_message(dev.init(None)), "bootloader cmd 0x16 -> 0x7");
    fake.with(|s| {
        s.bl_status.clear();
        s.stuck_before_bootloader = true;
    });
    let m = sensor_message(dev.init(None));
    assert!(
        m.contains("sensor bootloader: timeout, reg 0x00 = 0x00"),
        "{m}"
    );
}

#[test]
fn configure_writes_every_field_a_preset_does_not_reset() {
    let (fake, dev) = make();
    fake.with(|s| {
        s.cfg[0x2B - 0x22] = 1;
        s.cfg[0x2C - 0x22] = 1;
        s.cfg[0x2A - 0x22] = 0x3C;
        s.cfg[0] = 100;
    });
    let cfg = dev.configure("8x8", &Tmf8829Options::default()).unwrap();
    let w = fake.writes();
    assert_eq!(w[0], [0x08, 0x40]);
    assert_eq!(w[1], [0x08, 0x16]);
    assert_eq!(w[2][0], 0x22);
    assert_eq!(w[2].len(), 191);
    assert_eq!(w[3], [0x08, 0x15]);
    assert_eq!(w[4], [0x08, 0x16]);
    assert_eq!(cfg.preset, "8x8");
    assert_eq!(cfg.grid(), (8, 8));
    assert!(!cfg.histograms);
    assert!(!cfg.ref_frame);
    for (k, v) in [
        ("period_ms", 33),
        ("kilo_iterations", 586),
        ("fp_mode", 0),
        ("nr_peaks", 1),
        ("signal_strength", 0),
        ("noise_strength", 0),
        ("xtalk", 0),
        ("dump_histograms", 0),
        ("ref_spad_frame", 0),
    ] {
        assert_eq!(cfg.field(k), Some(v), "{k}");
    }
    assert_eq!(
        dev.config(),
        Some(cfg),
        "config() is the configure() result"
    );
}

#[test]
fn configure_writes_the_caller_options_and_refuses_bad_ones() {
    let (fake, dev) = make();
    let opts = Tmf8829Options {
        period_ms: 300,
        nr_peaks: 2,
        signal_strength: true,
        noise_strength: true,
        xtalk: true,
        histograms: true,
        ref_frame: true,
        fields: vec![("power_modes".into(), 3)],
        ..Tmf8829Options::default()
    };
    let cfg = dev.configure("32x32", &opts).unwrap();
    assert_eq!(fake.writes()[0], [0x08, 0x45]);
    let page = fake.writes()[2][1..].to_vec();
    assert_eq!(u16::from_le_bytes([page[0], page[1]]), 300);
    assert_eq!(u16::from_le_bytes([page[2], page[3]]), 2344);
    assert_eq!(page[0x2A - 0x22], 2 | 0x08 | 0x10 | 0x20);
    assert_eq!(
        [page[0x2B - 0x22], page[0x2C - 0x22], page[0x2E - 0x22]],
        [1, 1, 3]
    );
    assert_eq!(cfg.grid(), (32, 32));
    assert!(cfg.histograms);
    assert!(cfg.ref_frame);
    assert_eq!(cfg.field("fp_mode"), Some(3));

    let n = fake.log_count();
    assert!(is_invalid_argument(
        dev.configure("9x9", &Tmf8829Options::default())
    ));
    assert!(is_invalid_argument(dev.configure(
        "8x8",
        &Tmf8829Options {
            nr_peaks: 0,
            ..Tmf8829Options::default()
        }
    )));
    assert!(is_invalid_argument(dev.configure(
        "8x8",
        &Tmf8829Options {
            nr_peaks: 5,
            ..Tmf8829Options::default()
        }
    )));
    assert_eq!(fake.log_count(), n, "nothing sent for refused arguments");

    fake.with(|s| s.app_status.insert(0x15, 0x04));
    assert_eq!(
        sensor_message(dev.configure(
            "48x32",
            &Tmf8829Options {
                nr_peaks: 4,
                ..Tmf8829Options::default()
            }
        )),
        "sensor cmd 0x15 -> ERR_CONFIG_RESULT_SIZE"
    );
    fake.with(|s| {
        s.app_status.clear();
        s.stuck_bytes.insert(0x78 - 0x22, 5);
    });
    let opts = Tmf8829Options {
        fields: vec![("fov_correction".into(), 3)],
        ..Tmf8829Options::default()
    };
    assert_eq!(
        sensor_message(dev.configure("8x8", &opts)),
        "config fields did not stick: fov_correction wrote 3, reads 5"
    );
    fake.with(|s| {
        s.stuck_bytes.clear();
        s.cid_override = Some(0x40);
    });
    assert_eq!(
        sensor_message(dev.config_page()),
        "CID_RID 0x40 after LOAD_CONFIG_PAGE"
    );
}

#[test]
fn start_arms_the_stream_then_measures_and_stop_undoes_it() {
    let (fake, dev) = make();
    dev.start().unwrap();
    assert!(dev.streaming(), "streaming");
    assert_eq!(dev.config().unwrap().preset, "8x8");
    let w = fake.writes();
    let i = w.iter().position(|x| x[0] == 0xFA).unwrap();
    assert_eq!(
        w[i..i + 3],
        [vec![0xFA, 0x30], vec![0xE1, 0xFF], vec![0xE2, 0x01]]
    );
    assert_eq!(fake.sent(0x35), [vec![0x01, 0x00]]);
    assert_eq!(w[w.len() - 1], [0x08, 0x10]); // MEASURE after the stream is armed
    assert!(matches!(
        dev.configure("8x8", &Tmf8829Options::default()),
        Err(DepzError::InvalidState(_))
    ));
    dev.start().unwrap(); // no-op while streaming
    assert_eq!(fake.sent(0x35).len(), 1, "one START_STREAM");

    let n = fake.log_count();
    dev.stop().unwrap();
    let tail: Vec<Vec<u8>> = fake
        .log_from(n)
        .into_iter()
        .filter(|p| p.cmd != R)
        .map(|p| [vec![p.cmd], p.payload].concat())
        .collect();
    assert_eq!(
        tail,
        [
            vec![W, 0x08, 0xFF],
            vec![W, 0xE2, 0x00],
            vec![W, 0xE1, 0xFF],
            vec![0x36]
        ]
    );
    assert!(!dev.streaming(), "stopped");
    dev.stop().unwrap(); // the sensor is stopped again, the bridge left alone
    assert_eq!(fake.sent(0x36).len(), 1, "one STOP_STREAM");

    dev.configure(
        "8x8",
        &Tmf8829Options {
            histograms: true,
            ref_frame: true,
            ..Tmf8829Options::default()
        },
    )
    .unwrap();
    dev.start().unwrap();
    assert!(
        fake.writes().contains(&vec![0xE2, 0x09]),
        "INT_ENAB 0x09 written"
    );
    assert_eq!(fake.sent(0x35).last().unwrap(), &[0x09, 0x01]);
    assert_eq!(fake.with(|s| (s.int_mask, s.flags)), (0x09, 0x01));
    dev.stop().unwrap();

    fake.with(|s| s.app_status.insert(0x10, 0x03));
    assert_eq!(
        sensor_message(dev.start()),
        "sensor cmd 0x10 -> ERR_APPLICATION"
    );
    assert!(!dev.streaming(), "not streaming after a refused MEASURE");
    assert_eq!(fake.sent(0x36).len(), 3, "STOP_STREAM count");
}

#[test]
fn stop_swallows_the_bad_header_a_stop_raises_and_other_events_pass() {
    let (fake, dev) = make();
    let events: Arc<Mutex<Vec<Tmf8829Event>>> = Arc::new(Mutex::new(Vec::new()));
    let ev = events.clone();
    let sub = dev.on_event(move |e: &DeviceEvent| {
        if let Some(t) = Tmf8829Device::interrupt_event(e) {
            ev.lock().unwrap().push(t);
        }
    });
    dev.start().unwrap();
    let link = fake.link.clone();
    fake.with(|s| {
        s.on_command = Some(Arc::new(move |cmd| {
            if cmd == 0xFF {
                vec![event(&link, EV_BAD_HEADER, 0x08, 0xFF, 0x04)]
            } else {
                vec![]
            }
        }))
    });
    dev.stop().unwrap();
    sleep_ms(50);
    assert_eq!(events.lock().unwrap().len(), 0, "events after stop");

    fake.link.inject(fake.link.reply(0x94, &[0; 5])); // truncated: ignored
    fake.emit_event(EV_INT, 0x02, 0);
    fake.emit_event(EV_BAD_HEADER, 0x01, 0x55);
    wait_for(&events, 2);
    let got = events.lock().unwrap().clone();
    assert_eq!(
        got[0],
        Tmf8829Event {
            timestamp_us: 987654321,
            int_status: 0x02,
            code: 0,
            frame_id: 0,
            fifo_status: 0
        }
    );
    assert_eq!(got[1].code, 1);
    assert_eq!(got[1].frame_id, 0x55);
    sub.cancel();
}

#[test]
fn close_stops_a_running_stream_even_when_the_stop_is_refused() {
    let (fake, dev) = make();
    dev.start().unwrap();
    dev.close();
    assert!(fake.writes().contains(&vec![0x08, 0xFF]), "STOP written");
    assert_eq!(fake.sent(0x36).len(), 1, "one STOP_STREAM");

    let (fake2, dev2) = make();
    dev2.start().unwrap();
    fake2.with(|s| s.app_status.insert(0xFF, 0x0A));
    dev2.close();
    assert!(dev2.is_closed(), "closed");

    // Dropping does the same as close().
    let (fake3, dev3) = make();
    dev3.start().unwrap();
    drop(dev3);
    assert_eq!(fake3.sent(0x36).len(), 1, "STOP_STREAM on drop");
}

#[test]
fn streamed_frames_match_the_golden_measurements() {
    let v = vectors();
    for (name, m) in &v.meas {
        let (fake, dev) = make();
        let got = collect(&dev);
        dev.start().unwrap();
        v.play(&fake, name);
        let want = m["expect"].as_array().unwrap();
        wait_for(&got, want.len());
        let got = got.lock().unwrap();
        for (k, w) in want.iter().enumerate() {
            assert_fields(&format!("{name}#{k}"), w, &meas_fields(&got[k]));
        }
        let n_corrupt = m["frames"]
            .as_array()
            .unwrap()
            .iter()
            .filter(|f| v.corrupt.iter().any(|c| f["raw_case"] == c.as_str()))
            .count() as u64;
        assert_eq!(
            [
                dev.frames_dropped(),
                dev.frames_discarded(),
                dev.frame_parse_errors()
            ],
            [0, 0, n_corrupt],
            "{name}: dropped/discarded/parse errors"
        );
        assert_eq!(
            dev.clock_ratio_q16() as u64,
            want.last().unwrap()["ratio_q16"].as_u64().unwrap(),
            "{name}: ratio_q16"
        );
    }
}

#[test]
fn histogram_frames_are_reassembled_from_1024_byte_chunks() {
    let v = vectors();
    let (fake, dev) = make();
    let got = collect(&dev);
    dev.start().unwrap();
    v.play(&fake, "8x8_hist");
    wait_for(&got, 1);
    let got = got.lock().unwrap();
    assert_eq!(
        got[0]
            .histograms
            .iter()
            .map(|h| h.raw.len())
            .collect::<Vec<_>>(),
        [25377, 25377]
    );
    assert_eq!(got[0].histograms[0].raw, v.raw("8x8_hist#0"));
}

#[test]
fn get_measurement_takes_the_next_and_times_out() {
    let v = vectors();
    let (fake, dev) = make();
    let frame = v.raw("8x8#0").to_vec();
    let all = dev.measurements(4);
    dev.start().unwrap();
    let (link, s) = (fake.link.clone(), fake.s.clone());
    let t = std::thread::spawn(move || {
        sleep_ms(50);
        FakeTmf8829 { link, s }.emit_frame(&frame, 2125575099, 2125575071);
    });
    assert_eq!(
        dev.get_measurement(Some(Duration::from_secs(2)))
            .unwrap()
            .f_number,
        1
    );
    t.join().unwrap();
    assert_eq!(all.take(Duration::from_secs(1)).unwrap().f_number, 1);
    assert!(matches!(
        dev.get_measurement(Some(Duration::from_millis(100))),
        Err(DepzError::Timeout { .. })
    ));
}

#[test]
fn lost_and_broken_frames_are_counted_not_raised() {
    let v = vectors();
    let (fake, dev) = make();
    let got = collect(&dev);
    dev.start().unwrap();
    fake.emit_frame_at(v.raw("8x8#0"), 2125575099, 2125575071, Some(0), None);
    fake.emit_frame_at(v.raw("8x8#1"), 2125672180, 2125672161, Some(3), None); // two frames dropped by the bridge
    fake.emit_frame_at(v.raw("8x8_full#0"), 2125937131, 0, Some(4), Some(&[0])); // cut short
    fake.emit_frame_at(&v.raw("8x8#1")[..40], 1, 0, Some(5), None); // parse error
    fake.link.inject(fake.link.reply(0x93, &[0; 10])); // truncated chunk header
    wait_for(&got, 2);
    sleep_ms(50);
    assert_eq!(
        [
            dev.frames_dropped(),
            dev.frames_discarded(),
            dev.frame_parse_errors()
        ],
        [2, 1, 1]
    );
    dev.stop().unwrap();
    dev.start().unwrap();
    assert_eq!(
        [
            dev.frames_dropped(),
            dev.frames_discarded(),
            dev.frame_parse_errors()
        ],
        [0, 0, 0]
    );
}

#[test]
fn register_access_chunks_and_checks_its_replies() {
    let (fake, dev) = make();
    assert_eq!(dev.read_registers(0xF0, 2000).unwrap().len(), 2000);
    assert_eq!(
        fake.sent(R),
        [
            vec![0xF0, 0x00, 0x04],
            vec![0xFF, (976 & 0xFF) as u8, (976 >> 8) as u8]
        ]
    );

    let mut progress = Vec::new();
    dev.write_registers(0xFF, &[7; 2100], Some(&mut |d, t| progress.push((d, t))))
        .unwrap();
    let shape: Vec<(u8, usize)> = fake.sent(W).iter().map(|p| (p[0], p.len() - 1)).collect();
    assert_eq!(shape, [(0xFF, 1023), (0xFF, 1023), (0xFF, 54)]);
    assert_eq!(progress, [(1023, 2100), (2046, 2100), (2100, 2100)]);

    dev.write_register(0xE2, 0x09).unwrap();
    assert_eq!(fake.sent(W).last().unwrap(), &[0xE2, 0x09]);

    // An address past 0xFF (refused unsent in Java) does not type-check here: addresses are u8.

    fake.with(|s| s.short_read = 1);
    match dev.read_registers(0x00, 4) {
        Err(DepzError::Protocol(m)) => assert_eq!(m, "READ_REG 0x00: expected 4, got 3"),
        other => panic!("expected a protocol error, got {other:?}"),
    }
    fake.with(|s| {
        s.short_read = 0;
        s.bridge_status.insert(W, 0x03);
    });
    assert!(matches!(
        dev.write_registers(0xE2, &[1], None),
        Err(DepzError::Status {
            cmd: W,
            status: 0x03
        })
    ));
    fake.with(|s| s.bridge_status.clear());
    dev.write_registers(0xE2, &[1], None).unwrap();
    assert_eq!(fake.with(|s| s.regs[0xE2]), 1);
}

#[test]
fn sensor_state_and_bridge_info() {
    let (fake, dev) = make();
    dev.init(None).unwrap();
    let st = dev.sensor_state().unwrap();
    assert_eq!(
        st.iter().map(|(n, _)| *n).collect::<Vec<_>>(),
        [
            "ID",
            "REVID",
            "ENABLE",
            "RESET",
            "APP_ID",
            "CMD_STAT",
            "INT_STATUS",
            "INT_ENAB",
            "FIFOSTATUS"
        ]
    );
    let get = |k: &str| st.iter().find(|(n, _)| *n == k).unwrap().1;
    assert_eq!(get("ID"), 0x9E);
    assert_eq!(get("APP_ID"), 0x01);
    assert_eq!(get("ENABLE") & 0x80, 0x80);
    dev.start().unwrap();
    let info = dev.get_bridge_info().unwrap();
    assert_eq!(
        [info.int_edges, info.frames, info.frames_aborted],
        [7, 6, 1]
    );
    assert!(info.streaming);
    assert_eq!(info.int_mask, 0x01);
    assert!(fake.with(|s| s.streaming));
}
