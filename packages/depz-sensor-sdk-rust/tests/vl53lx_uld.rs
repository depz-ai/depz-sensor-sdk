//! The light drivers of the 1D family (`vl53lx::uld`) replayed against the contract-15 scenario
//! recordings: a fake [`Vl53lxPlatform`] plays the captured register traffic back (READ_REG /
//! WRITE_REG / SET_I2C_SPEED / SET_ADDR_WIDTH / XSHUT, with the bridge's replies) and every
//! operation the driver issues must be the next one captured, byte for byte. The scenario's calls are
//! re-issued the way the device class issues them — init (address width, sensor_init), configure
//! (sensor_init, set_mode, set_range_timing), get_range_timing, start_ranging, stop_ranging — and the
//! streamed blocks are decoded by the configured driver against the `.expected.json` frames.

mod common;

use std::collections::VecDeque;
use std::sync::{Arc, Mutex};
use std::time::Duration;

use depz_sensor_sdk::devices::{DepzError, Result};
use depz_sensor_sdk::protocol::common::{Rpt, StatusReport};
use depz_sensor_sdk::vl53lx::driver::{ExtraValue, Vl53lxDriver, Vl53lxPlatform};
use depz_sensor_sdk::vl53lx::uld::{self, create_driver, driver_kinds};
use depz_sensor_sdk::vl53lx::{RegData, StreamData};
use serde_json::Value;

use common::{hex_decode, recording_packets, vectors_dir};

const READ_REG: u8 = 0x32;
const WRITE_REG: u8 = 0x33;
const XSHUT: u8 = 0x34;
const STREAM: u8 = 0x93;
const SET_I2C_SPEED: u8 = 0x38;
const SET_ADDR_WIDTH: u8 = 0x39;
const REG_DATA: u8 = 0x91;

#[derive(Debug, Clone, PartialEq)]
enum Op {
    Read(u16, u16),
    Write(u16, Vec<u8>),
    Speed(u16),
    Width(u8),
    Xshut(u8),
}

#[derive(Debug, Clone)]
enum Reply {
    Data(Vec<u8>),
    Ok,
    Refused(u8, u8),
    NoAnswer(u8),
}

struct Step {
    op: Op,
    reply: Reply,
}

struct Capture {
    steps: Vec<Step>,
    blocks: Vec<Vec<u8>>,
}

fn le16(b: &[u8]) -> u16 {
    u16::from_le_bytes([b[0], b[1]])
}

/// The driver-level requests of a recording, each with what the bridge answered before the next
/// request went out (nothing = a timeout); the device-level requests (names, START/STOP_STREAM,
/// CLEAR_I2C_ERRORS …) are left out. Plus the streamed blocks, in order.
fn capture(file: &str) -> Capture {
    let mut steps: Vec<Step> = Vec::new();
    let mut blocks = Vec::new();
    let mut current: Option<(usize, u8)> = None;
    for p in recording_packets(file) {
        let pl = &p.payload;
        if !p.rx {
            let op = match p.cmd {
                READ_REG => Some(Op::Read(le16(&pl[0..2]), le16(&pl[2..4]))),
                WRITE_REG => Some(Op::Write(le16(&pl[0..2]), pl[2..].to_vec())),
                SET_I2C_SPEED => Some(Op::Speed(le16(&pl[0..2]))),
                SET_ADDR_WIDTH => Some(Op::Width(pl[0])),
                XSHUT => Some(Op::Xshut(pl[0])),
                _ => None,
            };
            current = op.map(|op| {
                steps.push(Step {
                    op,
                    reply: Reply::NoAnswer(p.cmd),
                });
                (steps.len() - 1, p.cmd)
            });
            continue;
        }
        if p.cmd == STREAM {
            blocks.push(StreamData::unpack(pl).unwrap().data);
            continue;
        }
        let Some((i, cmd)) = current else { continue };
        if p.cmd == REG_DATA && cmd == READ_REG {
            steps[i].reply = Reply::Data(RegData::unpack(pl).unwrap().data);
        } else if p.cmd == Rpt::Status as u8 {
            let s = StatusReport::unpack(pl).unwrap();
            if s.cmd == cmd {
                if s.status != 0 {
                    steps[i].reply = Reply::Refused(s.cmd, s.status);
                } else if matches!(steps[i].reply, Reply::NoAnswer(_)) {
                    steps[i].reply = Reply::Ok;
                }
            }
        }
    }
    Capture { steps, blocks }
}

/// Plays a capture back; the first operation that differs from the capture stops the driver with
/// [`DepzError::ReplayMismatch`] and is kept for the assertion.
struct Replay {
    state: Mutex<ReplayState>,
}

struct ReplayState {
    script: VecDeque<Step>,
    done: usize,
    mismatch: Option<String>,
}

impl Replay {
    fn new(steps: Vec<Step>) -> Replay {
        Replay {
            state: Mutex::new(ReplayState {
                script: steps.into(),
                done: 0,
                mismatch: None,
            }),
        }
    }

    fn next(&self, op: Op) -> Result<Reply> {
        let mut st = self.state.lock().unwrap();
        if st.mismatch.is_some() {
            return Err(DepzError::ReplayMismatch("after a mismatch".into()));
        }
        let done = st.done;
        match st.script.pop_front() {
            Some(step) if step.op == op => {
                st.done += 1;
                match step.reply {
                    Reply::Ok | Reply::Data(_) => Ok(step.reply),
                    Reply::Refused(cmd, status) => Err(DepzError::Status { cmd, status }),
                    Reply::NoAnswer(cmd) => {
                        Err(DepzError::timeout(cmd, Duration::from_millis(100)))
                    }
                }
            }
            other => {
                let next: Vec<String> = other
                    .iter()
                    .map(|s| format!("{:?}", s.op))
                    .chain(st.script.iter().take(4).map(|s| format!("{:?}", s.op)))
                    .collect();
                let msg = format!("op #{done}: the driver issued {op:?}, the capture has {next:?}");
                st.mismatch = Some(msg.clone());
                Err(DepzError::ReplayMismatch(msg))
            }
        }
    }

    fn ack(&self, op: Op) -> Result<()> {
        match self.next(op)? {
            Reply::Ok => Ok(()),
            r => Err(DepzError::ReplayMismatch(format!("a write answered {r:?}"))),
        }
    }

    fn mismatch(&self) -> Option<String> {
        self.state.lock().unwrap().mismatch.clone()
    }

    fn done(&self) -> usize {
        self.state.lock().unwrap().done
    }

    fn left(&self) -> Vec<String> {
        self.state
            .lock()
            .unwrap()
            .script
            .iter()
            .map(|s| format!("{:?}", s.op))
            .collect()
    }
}

impl Vl53lxPlatform for Replay {
    fn read_multi(&self, addr: u16, size: usize) -> Result<Vec<u8>> {
        match self.next(Op::Read(addr, size as u16))? {
            Reply::Data(d) => Ok(d),
            r => Err(DepzError::ReplayMismatch(format!("a read answered {r:?}"))),
        }
    }

    fn write_multi(&self, addr: u16, data: &[u8]) -> Result<()> {
        self.ack(Op::Write(addr, data.to_vec()))
    }

    fn set_i2c_speed(&self, khz: u16) -> Result<()> {
        self.ack(Op::Speed(khz))
    }

    fn set_addr_width(&self, width: u8) -> Result<()> {
        self.ack(Op::Width(width))
    }

    fn xshut_reset(&self) -> Result<()> {
        self.ack(Op::Xshut(2))
    }

    fn sleep_ms(&self, _ms: u32) {}
}

fn scenario(id: &str) -> Value {
    let p = vectors_dir()
        .join("..")
        .join("scenarios")
        .join(format!("{id}.json"));
    serde_json::from_str(&std::fs::read_to_string(&p).unwrap()).unwrap()
}

fn call<'a>(sc: &'a Value, name: &str) -> &'a Value {
    sc["steps"]
        .as_array()
        .unwrap()
        .iter()
        .find(|s| s["call"] == name)
        .unwrap_or_else(|| panic!("no '{name}' step"))
}

struct Outcome {
    ops: usize,
    timeouts: usize,
    refusals: usize,
    frames: usize,
}

/// Replay one scenario; panics with the first divergence.
fn replay(id: &str, product_by_class: &str) -> Outcome {
    let sc = scenario(id);
    let expected: Value = serde_json::from_str(
        &std::fs::read_to_string(
            vectors_dir()
                .join("recordings")
                .join(format!("{id}.expected.json")),
        )
        .unwrap(),
    )
    .unwrap();
    let cap = capture(sc["recording"].as_str().unwrap());
    let timeouts = cap
        .steps
        .iter()
        .filter(|s| matches!(s.reply, Reply::NoAnswer(_)))
        .count();
    let refusals = cap
        .steps
        .iter()
        .filter(|s| matches!(s.reply, Reply::Refused(..)))
        .count();
    let n_steps = cap.steps.len();
    let platform = Arc::new(Replay::new(cap.steps));
    let shared: Arc<dyn Vl53lxPlatform> = platform.clone();

    let init = call(&sc, "init")["args"].as_array().unwrap();
    let kind = init[0].as_str().unwrap();
    let product = init
        .get(1)
        .and_then(|v| v.as_str())
        .unwrap_or(product_by_class);
    assert_eq!(expected["product"], product, "{id}");
    let (mut drv, _caveat) = create_driver(product, kind, shared).unwrap();

    let conf = call(&sc, "configure")["args"].as_array().unwrap();
    let budget = conf[0].as_u64().unwrap() as u32;
    let inter = conf[1].as_u64().unwrap() as u32;
    let mode = conf.get(2).and_then(|v| v.as_str());

    let run = |drv: &mut Box<dyn Vl53lxDriver>| -> Result<(u32, u32)> {
        // init: the address width first (sticky on the bridge), then the sensor
        platform.set_addr_width(drv.addr_width())?;
        drv.sensor_init()?;
        // configure
        drv.sensor_init()?;
        if let Some(m) = mode {
            drv.set_mode(m)?;
        }
        drv.set_range_timing(budget, inter)?;
        let timing = drv.get_range_timing()?;
        drv.start_ranging()?;
        drv.stop_ranging()?;
        Ok(timing)
    };
    let result = run(&mut drv);
    if let Some(m) = platform.mismatch() {
        panic!("{id}: {m}");
    }
    let timing = result.unwrap_or_else(|e| panic!("{id}: {e}"));
    assert!(
        platform.left().is_empty(),
        "{id}: the capture has more: {:?}",
        platform.left()
    );
    assert_eq!(platform.done(), n_steps);

    let want = call(&sc, "get_range_timing")["expect"].as_array().unwrap();
    assert_eq!(
        timing,
        (
            want[0].as_u64().unwrap() as u32,
            want[1].as_u64().unwrap() as u32
        ),
        "{id} timing"
    );

    let frames = expected["frames"].as_array().unwrap();
    assert!(!frames.is_empty());
    for (i, f) in frames.iter().enumerate() {
        let raw = hex_decode(f["raw"].as_str().unwrap());
        assert_eq!(cap.blocks[i], raw, "{id} frame {i}: the streamed block");
        let r = drv.decode(&raw).unwrap();
        assert_eq!(
            r.distance_mm as i64,
            f["distance_mm"].as_i64().unwrap(),
            "{id} frame {i} distance"
        );
        assert_eq!(
            r.status as i64,
            f["status"].as_i64().unwrap(),
            "{id} frame {i} status"
        );
        assert!(r.targets.is_empty());
        assert_eq!(r.valid(), r.status == 0);
        assert!(!r.status_text.is_empty());
    }
    Outcome {
        ops: n_steps,
        timeouts,
        refusals,
        frames: frames.len(),
    }
}

fn report(id: &str, o: &Outcome) {
    eprintln!(
        "{id}: {} register ops replayed ({} timeouts, {} refusals in the capture), {} frames decoded",
        o.ops, o.timeouts, o.refusals, o.frames
    );
}

#[test]
fn replay_vl53l1cx_uld_long_33ms() {
    let o = replay("vl53l1cx_uld_long_33ms", "VL53L1CX");
    report("vl53l1cx_uld_long_33ms", &o);
}

#[test]
fn replay_vl53l1cb_uld_short_50ms() {
    let o = replay("vl53l1cb_uld_short_50ms", "VL53L1CB");
    report("vl53l1cb_uld_short_50ms", &o);
}

#[test]
fn replay_vl53l3cx_ulp_33ms() {
    let o = replay("vl53l3cx_ulp_33ms", "VL53L3CX");
    report("vl53l3cx_ulp_33ms", &o);
}

#[test]
fn replay_vl53l4cx_uld_as_l4cd_50ms() {
    let o = replay("vl53l4cx_uld_as_l4cd_50ms", "VL53L4CX");
    report("vl53l4cx_uld_as_l4cd_50ms", &o);
}

#[test]
fn replay_vl53l0x_uld_long_range_33ms() {
    let o = replay("vl53l0x_uld_long-range_33ms", "VL53L0X");
    report("vl53l0x_uld_long-range_33ms", &o);
}

// ---- registry ----------------------------------------------------------------

fn unsupported(e: DepzError) -> String {
    match e {
        DepzError::Unsupported(m) => m,
        other => panic!("expected Unsupported, got {other:?}"),
    }
}

#[test]
fn registry_kinds_and_refusals() {
    assert_eq!(driver_kinds("VL53L0X").unwrap(), ["uld"]);
    assert_eq!(driver_kinds("VL53L1CX").unwrap(), ["uld", "histogram"]);
    assert_eq!(driver_kinds("VL53L1CB").unwrap(), ["uld", "histogram"]);
    assert_eq!(driver_kinds("VL53L3CX").unwrap(), ["ulp", "histogram"]);
    assert_eq!(driver_kinds("VL53L4CD").unwrap(), ["uld", "histogram"]);
    assert_eq!(driver_kinds("VL53L4CX").unwrap(), ["histogram"]);
    assert_eq!(
        unsupported(driver_kinds("VL53L9X").unwrap_err()),
        "no such product: VL53L9X - served: VL53L0X, VL53L1CX, VL53L1CB, VL53L3CX, VL53L4CD, VL53L4CX"
    );

    let p: Arc<dyn Vl53lxPlatform> = Arc::new(Replay::new(Vec::new()));
    let e = create_driver("VL53L3CX", "uld", p.clone()).err().unwrap();
    assert_eq!(
        unsupported(e),
        "VL53L3CX has no 'uld' driver - it has ulp, histogram"
    );

    let cases: [(&str, &str, &str, Option<&str>); 5] = [
        (
            "VL53L0X",
            "uld",
            "Vl53l0xUldDriver",
            Some("no detection thresholds; there is no ROI on this die at all"),
        ),
        ("VL53L1CX", "uld", "Vl53l1UldDriver", None),
        ("VL53L1CB", "uld", "Vl53l1UldDriver", None),
        (
            "VL53L3CX",
            "ulp",
            "Vl53l3UlpDriver",
            Some("single-target ranging only - the histogram driver gives several targets instead"),
        ),
        ("VL53L4CD", "uld", "Vl53l4UldDriver", None),
    ];
    for (product, kind, name, caveat) in cases {
        let (d, c) = create_driver(product, kind, p.clone()).unwrap();
        assert_eq!(d.product(), product);
        assert_eq!(d.driver_name(), name);
        assert_eq!(c, caveat);
        assert!(!d.histogram());
    }
    assert!(uld::HIST_CAVEAT.starts_with("the histogram driver has no calibrations"));
    for product in ["VL53L1CX", "VL53L1CB", "VL53L3CX", "VL53L4CD", "VL53L4CX"] {
        let (d, c) = create_driver(product, "histogram", p.clone()).unwrap();
        assert_eq!(d.product(), product);
        assert_eq!(d.driver_name(), "Vl53lxHistogramDriver");
        assert_eq!(c, Some(uld::HIST_CAVEAT));
        assert!(d.histogram());
    }
}

#[test]
fn driver_properties() {
    let p: Arc<dyn Vl53lxPlatform> = Arc::new(Replay::new(Vec::new()));
    let (l0x, _) = create_driver("VL53L0X", "uld", p.clone()).unwrap();
    assert_eq!(l0x.addr_width(), 1);
    assert_eq!(l0x.max_khz(), 400);
    assert_eq!(l0x.clear_steps(), [(0x0B, 0x01), (0x0B, 0x00)]);
    assert_eq!(l0x.stream_block(), (0x14, 12));
    assert_eq!(
        l0x.modes(),
        ["default", "long-range", "high-speed", "high-accuracy"]
    );
    assert_eq!(l0x.budget_ms(), (20, 200));

    let (l1, _) = create_driver("VL53L1CX", "uld", p.clone()).unwrap();
    assert_eq!(l1.addr_width(), 2);
    assert_eq!(l1.max_khz(), 1000);
    assert_eq!(l1.clear_steps(), [(0x86, 0x01)]);
    assert_eq!(l1.stream_block(), (0x89, 17));
    assert_eq!(l1.modes(), ["long", "short"]);
    assert_eq!(l1.budget_ms(), (15, 500));
    assert_eq!(l1.budget_choices(), [20, 33, 50, 100, 200, 500]);
    assert!(l1.supports().contains(&"roi"));

    let (mut l4, _) = create_driver("VL53L4CD", "uld", p.clone()).unwrap();
    assert!(l4.budget_choices().is_empty());
    assert!(!l4.supports().contains(&"roi"));
    let e = l4.set_mode("short").unwrap_err();
    assert_eq!(
        unsupported(e),
        "VL53L4CD/Vl53l4UldDriver does not support 'mode'"
    );
    let e = l4.perform_ref_spad_management().unwrap_err();
    assert_eq!(
        unsupported(e),
        "VL53L4CD/Vl53l4UldDriver does not support 'refspad'"
    );

    let (mut l3, _) = create_driver("VL53L3CX", "ulp", p).unwrap();
    let e = l3.calibrate_offset(100, None).unwrap_err();
    assert_eq!(
        unsupported(e),
        "VL53L3CX/Vl53l3UlpDriver does not support 'calib_offset'"
    );
}

#[test]
fn die_decode_and_status_text() {
    let p: Arc<dyn Vl53lxPlatform> = Arc::new(Replay::new(Vec::new()));
    let (mut l4, _) = create_driver("VL53L4CD", "uld", p.clone()).unwrap();
    // raw status 13 maps to 13, which the ULD table does not name
    let mut raw = hex_decode("090900d3200296008600092bb902700296");
    let r = l4.decode(&raw).unwrap();
    assert_eq!(
        (r.distance_mm, r.status, r.status_text.as_str()),
        (624, 0, "valid")
    );
    assert_eq!(r.signal_kcps, (0x0296 * 8) as f64);
    assert_eq!(r.ambient_kcps, (0x0086 * 8) as f64);
    assert_eq!(r.sigma_mm, (0x0009 / 4) as f64);
    assert_eq!(r.spads, (0xD320 / 256) as f64);
    assert_eq!(r.extra["stream_count"], ExtraValue::Int(0));
    assert_eq!(
        r.extra["signal_per_spad_kcps"],
        ExtraValue::Int(0x0296 * 8 * 256 / 0xD320)
    );
    raw[0] = 13;
    let r = l4.decode(&raw).unwrap();
    assert_eq!((r.status, r.status_text.as_str()), (13, "unknown (13)"));
    raw[0] = 0x1F; // 31: past the table, passed through
    assert_eq!(l4.decode(&raw).unwrap().status_text, "unknown (31)");
    match l4.decode(&raw[..16]).unwrap_err() {
        DepzError::Sensor(m) => assert_eq!(m, "result block too short: 16 bytes"),
        e => panic!("{e:?}"),
    }

    // the L1X ULD takes the signal from 0x0098 and scales per SPAD by 25
    let (mut l1, _) = create_driver("VL53L1CX", "uld", p).unwrap();
    let raw = hex_decode("090901439008060136000d222702420806");
    let r = l1.decode(&raw).unwrap();
    assert_eq!(r.signal_kcps, (0x0806 * 8) as f64);
    assert_eq!(
        r.extra["signal_per_spad_kcps"],
        ExtraValue::Int(0x0806 * 8 * 25 / 0x4390)
    );

    assert_eq!(uld::die::status_text(255), "other error");
    assert_eq!(uld::die::status_text(-1), "unknown (-1)");
}

#[test]
fn die_arithmetic() {
    assert_eq!(uld::isqrt(0), 0);
    assert_eq!(uld::isqrt(15), 3);
    assert_eq!(uld::isqrt(16), 4);
    assert_eq!(uld::isqrt(1 << 30), 1 << 15);
    // the reach the two blobs' phase windows give at a typical oscillator reading
    let osc = 0xBA00;
    assert!(uld::die::phase_window_mm(osc, 0x38) < uld::die::phase_window_mm(osc, 0xB8));
}
