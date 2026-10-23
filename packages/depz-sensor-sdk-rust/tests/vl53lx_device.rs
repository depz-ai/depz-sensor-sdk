//! The 1D-family device class (`APP_VL53L0_4` bridge, contract 12) over an in-process fake bridge
//! with a VL53L1-family register file behind it — no hardware (the Python test_vl53lx_device.py and
//! its fake_vl53lx.py, which the Java SDK has no counterpart of). The full-stack captures (the
//! contract 15 scenarios) cover the init/stream path of each pair; this covers what they cannot:
//! refusals, bridge error statuses, the capability-gated passthroughs, calibrations, poll mode and
//! queues. The last tests replay a capture up to init (strict tx) and drive configure() in ways the
//! capture never recorded (test_vl53lx_configure_replay.py).
mod common;

use std::collections::{HashMap, VecDeque};
use std::sync::{Arc, Mutex};
use std::time::Duration;

use common::fake_board::{until, FakeBoard};
use depz_sensor_sdk::devices::{
    attach, AnyDevice, DepzDevice, DepzError, Hook, ReplayLink, Vl53l0xDevice, Vl53l1cxDevice, Vl53l4cxDevice,
    Vl53lxDevice, Vl53lxMeasurement, PLOTTABLE_STATUSES,
};
use depz_sensor_sdk::discovery::resolve_sensor_class;
use depz_sensor_sdk::framing::{build_packet, CrcType, Packet};
use depz_sensor_sdk::protocol::common::Rpt;
use depz_sensor_sdk::vl53lx::driver::{ExtraValue, Thresholds, Vl53lxTarget};
use depz_sensor_sdk::vl53lx::uld::die::phase_window_mm;
use depz_sensor_sdk::vl53lx::{XSHUT_OFF, XSHUT_RESET};

const DIE_MODEL_ID: u16 = 0xEBAA;
const DIE_OSC: u16 = 0xBCCC;
const DIE_PLL: u16 = 0x0123;

// vectors/vl53lx.json
const L4_BLOCK: &str = "090901d3200285008900082ba702700285"; // die_block vl53l4cx_uld_as_l4cd_50ms_f2
const L4_BLOCK_F9: &str = "090908d320027f008a00092bce026f027f"; // ..._f9
const L1_BLOCK: &str = "090901439007d9013f000d2234024007d9"; // die_block vl53l1cx_uld_long_33ms_f2
const START_STREAM_DIE: &str = "890011000001860001"; // encode start_stream_die_block
const INFO_WRAPPED: &str = "ffffffff0700000003000000020100e80302010c000000"; // decode info_wrapped_counters

const L4CD_BOARD: &str = "DEPZ ToF Sensor VL53L4CD USB v2.1 TESTSERIAL";
const L4CX_BOARD: &str = "DEPZ ToF Sensor VL53L4CX USB v2.1 TOVJALN523";
const L1CX_BOARD: &str = "DEPZ ToF Sensor VL53L1CX USB v2.1 SER";
const L3CX_BOARD: &str = "DEPZ ToF Sensor VL53L3CX USB v2.1 SER";
const UNSTAMPED: &str = "DEPZ ToF Sensor USB v2.1";

const GET_DEVICE_NAME: u8 = 0x03;
const READ_REG: u8 = 0x32;
const WRITE_REG: u8 = 0x33;
const XSHUT: u8 = 0x34;
const START_STREAM: u8 = 0x35;
const STOP_STREAM: u8 = 0x36;
const GET_INFO: u8 = 0x37;
const SET_I2C_SPEED: u8 = 0x38;
const SET_ADDR_WIDTH: u8 = 0x39;
const CLEAR_I2C_ERRORS: u8 = 0x3A;
const ERR_INVALID_CMD: u8 = 0x02;

/// What the next packet of an opcode gets instead of its answer.
#[derive(Clone, Copy)]
enum Fail {
    Status(u8),
    Silent,
}

struct Bridge {
    regs: Vec<u8>,
    rx: Vec<(u8, Vec<u8>)>,
    mcu_time_us: u64,
    info: Vec<u8>,
    failures: HashMap<u8, VecDeque<Fail>>,
}

/// The `APP_VL53L0_4` bridge: it knows no sensor, so behind it sits a 64 KiB register file
/// pre-loaded with what a booted VL53L1-family die answers. Every packet the host sends is logged.
#[derive(Clone)]
struct FakeBridge {
    board: FakeBoard,
    st: Arc<Mutex<Bridge>>,
}

impl FakeBridge {
    fn new(device_name: &str, no_clear_i2c_errors: bool) -> FakeBridge {
        let mut regs = vec![0u8; 0x10000];
        regs[0x00E5] = 0x03; // booted
        regs[0x0006..0x0008].copy_from_slice(&DIE_OSC.to_be_bytes());
        regs[0x010F..0x0111].copy_from_slice(&DIE_MODEL_ID.to_be_bytes());
        regs[0x00DE..0x00E0].copy_from_slice(&DIE_PLL.to_be_bytes());
        regs[0x013E] = 0x91; // ROI_CONFIG__MODE_ROI_CENTRE_SPAD
        let st = Arc::new(Mutex::new(Bridge {
            regs,
            rx: Vec::new(),
            mcu_time_us: 5_000_000,
            info: vec![0; 23],
            failures: HashMap::new(),
        }));
        let board = FakeBoard::new();
        let (b, s, name) = (board.clone(), st.clone(), device_name.to_string());
        board.on_packet(move |pkt: &Packet| {
            let mut st = s.lock().unwrap();
            let (cmd, p) = (pkt.cmd, pkt.payload.clone());
            st.rx.push((cmd, p.clone()));
            match st.failures.get_mut(&cmd).and_then(VecDeque::pop_front) {
                Some(Fail::Silent) => return vec![],
                Some(Fail::Status(s)) => return vec![FakeBoard::status(cmd, s)],
                None => {}
            }
            match cmd {
                GET_DEVICE_NAME => {
                    let mut t = vec![cmd];
                    t.extend_from_slice(name.as_bytes());
                    t.push(0);
                    vec![b.reply(Rpt::Text as u8, &t)]
                }
                READ_REG => {
                    let addr = u16::from_le_bytes([p[0], p[1]]) as usize;
                    let n = u16::from_le_bytes([p[2], p[3]]) as usize;
                    let mut out = vec![cmd];
                    out.extend_from_slice(&st.mcu_time_us.to_le_bytes());
                    out.extend_from_slice(&st.regs[addr..(addr + n).min(0x10000)]); // short past 0xFFFF
                    st.mcu_time_us += 100;
                    vec![b.reply(0x91, &out)]
                }
                WRITE_REG => {
                    let addr = u16::from_le_bytes([p[0], p[1]]) as usize;
                    st.regs[addr..addr + p.len() - 2].copy_from_slice(&p[2..]);
                    vec![FakeBoard::status(cmd, 0)]
                }
                GET_INFO => vec![b.reply(0x92, &st.info)],
                CLEAR_I2C_ERRORS if no_clear_i2c_errors => vec![FakeBoard::status(cmd, ERR_INVALID_CMD)],
                XSHUT | START_STREAM | STOP_STREAM | SET_I2C_SPEED | SET_ADDR_WIDTH | CLEAR_I2C_ERRORS => {
                    vec![FakeBoard::status(cmd, 0)]
                }
                _ => vec![FakeBoard::status(cmd, ERR_INVALID_CMD)],
            }
        });
        FakeBridge { board, st }
    }

    fn fail_next(&self, cmd: u8, f: Fail) {
        self.st.lock().unwrap().failures.entry(cmd).or_default().push_back(f);
    }

    fn set_word(&self, addr: usize, v: u16) {
        self.st.lock().unwrap().regs[addr..addr + 2].copy_from_slice(&v.to_be_bytes());
    }

    fn set_reg(&self, addr: usize, v: u8) {
        self.st.lock().unwrap().regs[addr] = v;
    }

    fn reg(&self, addr: usize) -> u8 {
        self.st.lock().unwrap().regs[addr]
    }

    fn word(&self, addr: usize) -> u16 {
        let r = &self.st.lock().unwrap().regs;
        u16::from_be_bytes([r[addr], r[addr + 1]])
    }

    fn dword(&self, addr: usize) -> u32 {
        let r = &self.st.lock().unwrap().regs;
        u32::from_be_bytes([r[addr], r[addr + 1], r[addr + 2], r[addr + 3]])
    }

    /// Put a 17-byte result block at 0x0089 (what the next poll reads).
    fn set_result_block(&self, raw: &[u8]) {
        self.st.lock().unwrap().regs[0x0089..0x0089 + raw.len()].copy_from_slice(raw);
    }

    fn mcu_time_us(&self) -> u64 {
        self.st.lock().unwrap().mcu_time_us
    }

    fn clear(&self) {
        self.st.lock().unwrap().rx.clear();
    }

    fn cmds(&self) -> Vec<u8> {
        self.st.lock().unwrap().rx.iter().map(|(c, _)| *c).collect()
    }

    /// Payloads of one opcode, hex, in order.
    fn sent(&self, cmd: u8) -> Vec<String> {
        self.st.lock().unwrap().rx.iter().filter(|(c, _)| *c == cmd).map(|(_, p)| common::hex_encode(p)).collect()
    }

    /// Every WRITE_REG as (addr, data).
    fn writes(&self) -> Vec<(u16, Vec<u8>)> {
        let st = self.st.lock().unwrap();
        st.rx.iter().filter(|(c, _)| *c == WRITE_REG).map(|(_, p)| (u16::from_le_bytes([p[0], p[1]]), p[2..].to_vec())).collect()
    }

    /// RPT_VL53_STREAM: ts u64, addr u16, len u16, data.
    fn stream(&self, data: &[u8], ts_us: u64, addr: u16, len: Option<u16>) {
        let mut p = ts_us.to_le_bytes().to_vec();
        p.extend_from_slice(&addr.to_le_bytes());
        p.extend_from_slice(&len.unwrap_or(data.len() as u16).to_le_bytes());
        p.extend_from_slice(data);
        self.board.inject(self.board.reply(0x93, &p));
    }
}

fn connect_as<T>(board: &str, make: impl FnOnce(DepzDevice) -> T) -> (T, FakeBridge) {
    connect_with(board, false, make)
}

fn connect_with<T>(board: &str, no_clear: bool, make: impl FnOnce(DepzDevice) -> T) -> (T, FakeBridge) {
    let fake = FakeBridge::new(board, no_clear);
    let dev = make(DepzDevice::new(fake.board.clone(), Some(Duration::from_millis(500))));
    (dev, fake)
}

fn connect(board: &str) -> (Vl53lxDevice, FakeBridge) {
    connect_as(board, Vl53lxDevice::attach)
}

fn ready(board: &str, product: Option<&str>) -> (Vl53lxDevice, FakeBridge) {
    let (dev, fake) = connect(board);
    dev.init(None, product).unwrap();
    fake.clear();
    (dev, fake)
}

/// Wait until the reader thread has handled everything the fake sent: a GET_INFO round trip behind
/// the reports.
fn settle(fake: &FakeBridge, dev: &Vl53lxDevice) {
    dev.bridge_info().unwrap();
    fake.st.lock().unwrap().rx.pop();
}

fn later(fake: &FakeBridge, data: Vec<u8>, ts: u64) -> std::thread::JoinHandle<()> {
    let f = fake.clone();
    std::thread::spawn(move || {
        std::thread::sleep(Duration::from_millis(50));
        f.stream(&data, ts, 0x0089, None);
    })
}

fn hex(s: &str) -> Vec<u8> {
    common::hex_decode(s)
}

fn err_text(r: Result<impl std::fmt::Debug, DepzError>) -> String {
    match r {
        Err(e) => e.to_string(),
        Ok(v) => panic!("expected an error, got {v:?}"),
    }
}

fn is_bridge_error(e: &DepzError) -> bool {
    matches!(e, DepzError::Status { .. } | DepzError::Timeout { .. } | DepzError::Protocol(_))
}

// ── pure helpers ─────────────────────────────────────────────────────────────

fn t(distance_mm: i32, status: i32) -> Vl53lxTarget {
    Vl53lxTarget {
        distance_mm,
        status,
        status_text: String::new(),
        signal_kcps: 0.0,
        ambient_kcps: 0.0,
        sigma_mm: 0.0,
        min_range_mm: distance_mm,
        max_range_mm: distance_mm,
    }
}

fn m(distance_mm: i32, status: i32, targets: Vec<Vl53lxTarget>) -> Vl53lxMeasurement {
    Vl53lxMeasurement {
        timestamp_us: 0,
        distance_mm,
        status,
        status_text: String::new(),
        signal_kcps: 0.0,
        ambient_kcps: 0.0,
        sigma_mm: 0.0,
        spads: 0.0,
        targets,
        extra: Default::default(),
        bins: None,
    }
}

#[test]
fn plottable_statuses_are_0_6_11() {
    assert_eq!(PLOTTABLE_STATUSES, [0, 6, 11]);
}

#[test]
fn plot_distances_keeps_plottable_targets_in_driver_order() {
    let x = m(0, 0, vec![t(600, 6), t(900, 4), t(1500, 11), t(30, 0)]);
    assert_eq!(x.plot_distances(), [600, 1500, 30]);
    assert_eq!(x.primary_target(), Some(&t(600, 6)));
}

#[test]
fn light_driver_uses_the_single_distance_or_nothing() {
    assert_eq!(m(624, 0, vec![]).plot_distances(), [624]);
    assert!(m(624, 4, vec![]).plot_distances().is_empty());
    assert_eq!(m(624, 0, vec![]).primary_target(), None);
    assert_eq!(m(0, 0, vec![t(1, 7)]).primary_target(), None);
    let six = m(624, 6, vec![]);
    assert_eq!((six.valid(), six.plottable()), (false, true));
}

// ── identity and init ────────────────────────────────────────────────────────

#[test]
fn before_init_no_driver_no_product() {
    let (dev, _fake) = connect(L4CD_BOARD);
    assert!(!dev.initialized());
    assert_eq!((dev.product(), dev.driver_kind()), (None, None));
    assert!(err_text(dev.with_driver(|_| Ok(()))).contains("call init() first"));
    assert!(matches!(dev.supports("offset"), Err(DepzError::InvalidState(_))));
    assert!(err_text(dev.get_offset_mm()).contains("init"));
}

#[test]
fn init_binds_the_product_and_sets_the_address_width_first() {
    let (dev, fake) = connect(L4CD_BOARD);
    dev.init(None, None).unwrap();
    assert!(dev.initialized());
    assert_eq!((dev.product(), dev.driver_kind()), (Some("VL53L4CD"), Some("uld")));
    assert!(dev.supports("offset").unwrap());
    assert!(!dev.supports("roi").unwrap());
    assert!(dev.modes().unwrap().is_empty());

    let cmds = fake.cmds();
    // device name, then SET_ADDR_WIDTH 2 before the first register access
    assert_eq!(cmds[..2], [GET_DEVICE_NAME, SET_ADDR_WIDTH]);
    assert_eq!(fake.sent(SET_ADDR_WIDTH)[0], "02");
    let pos = |c| cmds.iter().position(|x| *x == c).unwrap();
    assert!(pos(SET_ADDR_WIDTH) < pos(READ_REG));
    // every init runs at 400 kHz, then the die's 1 MHz ceiling
    assert_eq!(fake.sent(SET_I2C_SPEED), ["9001", "e803"]);
    // the blob at 0x2D with byte 0 forced to FM+ (0x12)
    let blob = fake.writes().into_iter().find(|(a, _)| *a == 0x2D).unwrap().1;
    assert_eq!((blob.len(), blob[0]), (91, 0x12));
    // VL53L4CD_SensorInit() closes with ALGO__RANGE_IGNORE_THRESHOLD_MCPS
    assert!(fake.writes().contains(&(0x0024, vec![0x05, 0x00])));
    // CLEAR_I2C_ERRORS closes every init (contract 12 §2)
    assert_eq!(*cmds.last().unwrap(), CLEAR_I2C_ERRORS);
    assert_eq!(fake.sent(CLEAR_I2C_ERRORS), [""]);
}

#[test]
fn l0x_sets_address_width_1_before_anything_else() {
    // Contract 12 §1: VL53L0X is the one 1-byte-address part; a class with its own product does not
    // need the device name. The L0X init needs a real die, so it fails here — after the width.
    let (dev, fake) = connect_as("DEPZ ToF Sensor VL53L0X USB v2.1 SER", Vl53l0xDevice::attach);
    let _ = dev.init(None, None);
    assert_eq!(fake.cmds()[0], SET_ADDR_WIDTH);
    assert_eq!(fake.sent(SET_ADDR_WIDTH)[0], "01");
    assert!(fake.sent(GET_DEVICE_NAME).is_empty());
}

#[test]
fn device_name_is_read_once() {
    let (dev, fake) = connect(L4CD_BOARD);
    assert_eq!(dev.detected().unwrap(), Some("VL53L4CD"));
    dev.init(None, None).unwrap();
    dev.notes().unwrap();
    dev.identify_product().unwrap();
    assert_eq!(fake.sent(GET_DEVICE_NAME).len(), 1);
    assert_eq!(dev.board_name().unwrap(), L4CD_BOARD);
}

#[test]
fn identify_product_reports_both_reaches_and_the_model_id_check() {
    let (dev, _fake) = ready(L4CD_BOARD, None);
    let info = dev.identify_product().unwrap();
    assert_eq!(info.board, L4CD_BOARD);
    assert_eq!((info.detected, info.product, info.driver), (Some("VL53L4CD"), "VL53L4CD", "uld"));
    assert_eq!(info.driver_class, "Vl53l4UldDriver");
    assert_eq!(info.kinds, ["uld", "histogram"]);
    assert_eq!((info.model_id, info.model_id_ok), (0xEBAA, true));
    assert!(info.modes.is_empty());
    assert_eq!((info.reach_mm, info.budget_ms), (Some(1300), (10, 200)));
    assert!(info.budget_choices.is_empty());
    assert_eq!((info.histogram, info.max_khz, info.caveat), (false, 1000, None));
    // §5: the 0x38 window at the die's own oscillator, ~1388 mm
    assert_eq!(info.driver_reach_mm, Some(phase_window_mm(DIE_OSC as i64, 0x38) as i32));
    assert_eq!(info.driver_reach_mm, Some(1388));
    assert!(info.supports.contains(&"calib_xtalk"));
    assert!(dev.notes().unwrap().is_empty());
}

#[test]
fn a_wrong_model_id_fails_the_cross_check_not_the_init() {
    let (dev, fake) = ready(L4CD_BOARD, None);
    fake.set_word(0x010F, 0xEACC);
    let info = dev.identify_product().unwrap();
    assert_eq!((info.model_id, info.model_id_ok), (0xEACC, false));
}

#[test]
fn borrowing_a_siblings_driver() {
    let (dev, _fake) = ready(L4CX_BOARD, Some("VL53L4CD"));
    let info = dev.identify_product().unwrap();
    assert_eq!((info.detected, info.product), (Some("VL53L4CX"), "VL53L4CD"));
    assert_eq!((info.reach_mm, info.driver_reach_mm), (Some(6000), Some(1388)));
    let notes = dev.notes().unwrap();
    assert_eq!(notes.len(), 2);
    assert!(notes[0].contains("borrowing this driver - the board says it is a VL53L4CX"));
    assert!(notes[1].contains("reaches 1388 mm, the board is rated 6000 mm"));
}

#[test]
fn an_unstamped_board_needs_the_product_named() {
    let (dev, fake) = connect(UNSTAMPED);
    assert_eq!(dev.detected().unwrap(), None);
    assert!(dev.driver_kinds(None).unwrap().is_empty());
    assert!(err_text(dev.init(None, None)).contains("carries no product number"));
    assert!(fake.sent(SET_ADDR_WIDTH).is_empty());
    dev.init(None, Some("VL53L4CD")).unwrap();
    assert_eq!(dev.product(), Some("VL53L4CD"));
    assert_eq!(dev.notes().unwrap(), ["VL53L4CD: board name carries no product number, named by hand"]);
}

#[test]
fn a_pair_the_table_has_no_row_for_is_refused_before_any_traffic() {
    let (dev, fake) = connect_as(L4CX_BOARD, Vl53l4cxDevice::attach);
    assert_eq!(dev.driver_kinds(None).unwrap(), ["histogram"]);
    let e = dev.init(Some("uld"), None).unwrap_err();
    assert!(matches!(&e, DepzError::Unsupported(m) if m == "VL53L4CX has no 'uld' driver - it has histogram"), "{e}");
    let e = dev.init(None, Some("VL53L9ZZ")).unwrap_err();
    assert!(matches!(&e, DepzError::Unsupported(m) if m.contains("no such product")), "{e}");
    assert!(fake.cmds().iter().all(|c| *c == GET_DEVICE_NAME));
    assert!(!dev.initialized());
}

#[test]
fn driver_kinds_of_a_named_product_and_of_the_class_product() {
    let (dev, _fake) = connect_as(UNSTAMPED, Vl53l1cxDevice::attach);
    assert_eq!(dev.driver_kinds(None).unwrap(), ["uld", "histogram"]);
    assert_eq!(dev.driver_kinds(Some("VL53L3CX")).unwrap(), ["ulp", "histogram"]);
}

#[test]
fn a_v2_00_bridge_is_told_to_reflash() {
    let (dev, _fake) = connect_with(L4CD_BOARD, true, Vl53lxDevice::attach);
    let e = dev.init(None, None).unwrap_err();
    assert!(e.to_string().contains("predates APP_VL53L0_4_v0.24") && e.to_string().contains("reflash"), "{e}");
    assert!(!is_bridge_error(&e));
    assert!(!dev.initialized());
}

#[test]
fn any_other_clear_i2c_errors_failure_is_a_bridge_error() {
    let (dev, fake) = connect(L4CD_BOARD);
    fake.fail_next(CLEAR_I2C_ERRORS, Fail::Status(0x09));
    assert!(is_bridge_error(&dev.init(None, None).unwrap_err()));
    fake.fail_next(CLEAR_I2C_ERRORS, Fail::Silent);
    assert!(is_bridge_error(&dev.init(None, None).unwrap_err()));
}

#[test]
fn a_refused_set_addr_width_fails_init_as_a_bridge_error() {
    let (dev, fake) = connect(L4CD_BOARD);
    fake.fail_next(SET_ADDR_WIDTH, Fail::Status(0x04));
    assert!(is_bridge_error(&dev.init(None, None).unwrap_err()));
    assert!(!dev.initialized());
}

#[test]
fn bridge_info_decodes_rpt_vl53_info() {
    let (dev, fake) = connect(L4CD_BOARD);
    fake.st.lock().unwrap().info = hex(INFO_WRAPPED);
    let i = dev.bridge_info().unwrap();
    assert_eq!(
        (i.int_edges, i.slots_skipped, i.i2c_errors, i.last_i2c_error, i.xshut_level, i.int_level),
        (4294967295, 7, 3, 2, 1, 0)
    );
    assert_eq!((i.i2c_khz, i.addr_width, i.n_clear, i.frames_dropped), (1000, 2, 1, 12));
}

#[test]
fn xshut_drops_the_driver_and_the_stream() {
    let (dev, fake) = ready(L4CD_BOARD, None);
    dev.start_ranging().unwrap();
    dev.xshut(XSHUT_OFF).unwrap();
    assert_eq!(fake.sent(XSHUT), ["00"]);
    assert!(!dev.ranging());
    assert!(!dev.initialized());
    assert!(err_text(dev.with_driver(|_| Ok(()))).contains("init"));
    assert!(matches!(dev.xshut(XSHUT_RESET + 1), Err(DepzError::InvalidArgument(_))));
}

// ── configuration and the capability-gated passthroughs ──────────────────────

#[test]
fn configure_reinits_then_timing_offset_xtalk_signal() {
    let (dev, fake) = ready(L4CD_BOARD, None);
    dev.configure(33, 0, None, Some(-10), Some(20), Some(1024)).unwrap();
    assert_eq!(fake.sent(CLEAR_I2C_ERRORS).len(), 1);
    // vectors/vl53l4.json tuning: offset -10 → 65496, xtalk 20 → 10240, signal 1024 → 128
    let w = fake.writes();
    assert_eq!(
        w[w.len() - 7..],
        [
            (0x001E, vec![0xFF, 0xD8]),
            (0x0020, vec![0, 0]),
            (0x0022, vec![0, 0]),
            (0x0018, vec![0, 0]),
            (0x001A, vec![0, 0]),
            (0x0016, vec![0x28, 0x00]),
            (0x0066, vec![0x00, 0x80]),
        ]
    );
    assert_eq!(dev.get_offset_mm().unwrap(), -10);
    assert_eq!(dev.get_xtalk_kcps().unwrap(), 20);
    assert_eq!(dev.get_signal_threshold_kcps().unwrap(), 1024);
    let (budget, inter) = dev.get_range_timing().unwrap();
    assert!(inter == 0 && budget.abs_diff(33) <= 1);
    assert_eq!(dev.get_mode().unwrap(), None);
}

#[test]
fn an_inter_measurement_period_selects_autonomous_mode() {
    let (dev, fake) = ready(L4CD_BOARD, None);
    dev.configure(50, 100, None, None, None, None).unwrap();
    assert_eq!(fake.dword(0x006C), (1.055 * 100.0 * (DIE_PLL & 0x3FF) as f64) as u32);
    fake.clear();
    dev.start_ranging().unwrap();
    assert_eq!(fake.writes(), [(0x0087, vec![0x40])]);
}

#[test]
fn a_period_not_above_the_budget_is_refused() {
    let (dev, _fake) = ready(L4CD_BOARD, None);
    assert!(dev.configure(50, 20, None, None, None, None).is_err());
    assert!(err_text(dev.configure(300, 0, None, None, None, None)).contains("10..200"));
}

#[test]
fn a_mode_on_a_product_without_modes_is_refused_before_the_reinit() {
    let (dev, fake) = ready(L4CD_BOARD, None);
    let e = dev.configure(50, 0, Some("long"), None, None, None).unwrap_err();
    assert!(matches!(&e, DepzError::Unsupported(m) if m == "VL53L4CD/uld does not support 'mode'"), "{e}");
    assert!(fake.cmds().is_empty());
}

#[test]
fn snap_budget_clamps_to_the_drivers_range_when_any_budget_goes() {
    let (dev, _fake) = ready(L4CD_BOARD, None);
    assert!(dev.budget_choices().unwrap().is_empty());
    assert_eq!(
        (dev.snap_budget(5).unwrap(), dev.snap_budget(77).unwrap(), dev.snap_budget(500).unwrap()),
        (10, 77, 200)
    );
}

#[test]
fn offset_xtalk_thresholds_signal_sigma_round_trip() {
    let (dev, fake) = ready(L4CD_BOARD, None);
    dev.set_offset_mm(10).unwrap();
    assert_eq!(fake.word(0x001E), 40);
    assert_eq!(dev.get_offset_mm().unwrap(), 10);
    dev.set_xtalk_kcps(3).unwrap();
    assert_eq!(fake.word(0x0016), 1536);
    assert_eq!(dev.get_xtalk_kcps().unwrap(), 3);
    dev.set_detection_thresholds(100, 300, 3).unwrap();
    assert_eq!(fake.reg(0x0046), 3);
    assert_eq!((fake.word(0x0072), fake.word(0x0074)), (300, 100));
    assert_eq!(
        dev.get_detection_thresholds().unwrap(),
        Thresholds { distance_low_mm: 100, distance_high_mm: 300, window: 3 }
    );
    dev.set_signal_threshold_kcps(2048).unwrap();
    assert_eq!(fake.word(0x0066), 256);
    assert_eq!(dev.get_signal_threshold_kcps().unwrap(), 2048);
    dev.set_sigma_threshold_mm(15).unwrap();
    assert_eq!(fake.word(0x0064), 60);
    assert_eq!(dev.get_sigma_threshold_mm().unwrap(), 15);
    assert!(err_text(dev.set_sigma_threshold_mm(16384)).contains("16383"));
}

#[test]
fn an_unsupported_group_is_refused_with_no_traffic() {
    let (dev, fake) = ready(L4CD_BOARD, None);
    let refusals = [
        err_text(dev.get_roi()),
        err_text(dev.set_roi(4, 4)),
        err_text(dev.get_roi_center()),
        err_text(dev.set_roi_center(199)),
        err_text(dev.perform_ref_spad_management()),
    ];
    for r in refusals {
        assert!(r.contains("VL53L4CD/uld does not support"), "{r}");
    }
    assert!(fake.cmds().is_empty());
}

#[test]
fn the_refusal_names_the_caveat_and_comes_before_the_reinit() {
    let (dev, fake) = ready(L3CX_BOARD, None);
    assert_eq!(dev.driver_kind(), Some("ulp"));
    let e = err_text(dev.configure(50, 0, None, Some(5), None, None));
    assert!(e.contains("VL53L3CX/ulp does not support 'offset' (single-target ranging only"), "{e}");
    assert!(err_text(dev.calibrate_xtalk(600, None)).contains("'calib_xtalk'"));
    assert!(fake.cmds().is_empty());
    assert!(dev.notes().unwrap().iter().any(|n| n.starts_with("VL53L3CX/ulp: single-target")));
}

#[test]
fn temperature_update_reruns_vhv_and_restores_the_loop_bound() {
    let (dev, fake) = ready(L4CD_BOARD, None);
    dev.start_temperature_update().unwrap();
    assert_eq!(
        fake.writes(),
        [
            (0x0008, vec![0x81]),
            (0x000B, vec![0x92]),
            (0x0087, vec![0x21]),
            (0x0086, vec![0x01]),
            (0x0087, vec![0x80]),
            (0x0008, vec![0x09]),
            (0x000B, vec![0x00]),
        ]
    );
}

// ── calibrations (VL53L4CD_calibration.c) ────────────────────────────────────

#[test]
fn offset_calibration_target_minus_mean_programmed_x4() {
    let (dev, fake) = ready(L4CD_BOARD, None);
    fake.set_result_block(&hex(L4_BLOCK)); // 624 mm
    assert_eq!(dev.calibrate_offset(600, Some(5)).unwrap(), -24);
    assert_eq!(fake.word(0x001E), (-96i16) as u16);
    assert_eq!(dev.get_offset_mm().unwrap(), -24);
    // offsets zeroed first, 10 heat frames + 5 samples each released
    let w = fake.writes();
    assert_eq!(w[..3], [(0x001E, vec![0, 0]), (0x0020, vec![0, 0]), (0x0022, vec![0, 0])]);
    assert_eq!(w.iter().filter(|(a, _)| *a == 0x0086).count(), 15);
}

#[test]
fn xtalk_calibration_from_the_valid_samples_after_the_first() {
    let (dev, fake) = ready(L4CD_BOARD, None);
    fake.set_result_block(&hex(L4_BLOCK)); // 624 mm, 5160 kcps, 211 SPADs
    assert_eq!(dev.calibrate_xtalk(650, Some(5)).unwrap(), 1);
    // (1 - 624/650) × 5160/211 = 0.978 kcps → ×512
    assert_eq!(fake.word(0x0016), 500);
}

#[test]
fn xtalk_calibration_fails_with_no_valid_sample_or_past_127_kcps() {
    let (dev, fake) = ready(L4CD_BOARD, None);
    let mut bad = hex(L4_BLOCK);
    bad[0] = 0x04; // raw 4 → status 2, signal below threshold
    fake.set_result_block(&bad);
    assert!(err_text(dev.calibrate_xtalk(650, Some(5))).contains("no valid samples"));
    let mut hot = hex(L4_BLOCK);
    hot[3..7].copy_from_slice(&[0x01, 0x00, 0xFF, 0xFF]); // 1 SPAD, 524280 kcps
    fake.set_result_block(&hot);
    assert!(err_text(dev.calibrate_xtalk(5000, Some(5))).contains("> 127"));
}

#[test]
fn calibration_arguments_out_of_range_are_refused_before_any_traffic() {
    let (dev, fake) = ready(L4CD_BOARD, None);
    assert!(err_text(dev.calibrate_offset(600, Some(4))).contains("5..255"));
    assert!(err_text(dev.calibrate_offset(1001, None)).contains("10..1000"));
    assert!(err_text(dev.calibrate_xtalk(5001, None)).contains("10..5000"));
    assert!(dev.calibrate_xtalk(600, Some(256)).is_err());
    assert!(fake.cmds().is_empty());
}

// ── VL53L1X ULD: modes and tabulated budgets ─────────────────────────────────

#[test]
fn l1_modes_budget_choices_that_move_with_the_mode_and_snap() {
    let (dev, _fake) = ready(L1CX_BOARD, None);
    assert_eq!(dev.modes().unwrap(), ["long", "short"]);
    assert_eq!(dev.get_mode().unwrap().as_deref(), Some("long"));
    assert_eq!(dev.budget_choices().unwrap(), [20, 33, 50, 100, 200, 500]);
    assert_eq!((dev.snap_budget(40).unwrap(), dev.snap_budget(1000).unwrap()), (33, 500));
    dev.configure(15, 0, Some("short"), None, None, None).unwrap();
    assert_eq!(dev.get_mode().unwrap().as_deref(), Some("short"));
    assert_eq!(dev.budget_choices().unwrap()[0], 15);
    assert_eq!(dev.get_range_timing().unwrap(), (15, 15));
    assert_eq!(dev.identify_product().unwrap().driver_reach_mm, Some(phase_window_mm(DIE_OSC as i64, 0x38) as i32));
}

#[test]
fn l1_a_budget_the_mode_does_not_have_is_refused_not_rounded() {
    let (dev, _fake) = ready(L1CX_BOARD, None);
    assert!(err_text(dev.configure(40, 0, None, None, None, None)).contains("pick one of 20, 33, 50, 100, 200, 500"));
    assert!(err_text(dev.configure(50, 0, Some("medium"), None, None, None)).contains("no such mode"));
}

#[test]
fn l1_roi_size_and_centre() {
    let (dev, fake) = ready(L1CX_BOARD, None);
    dev.set_roi(8, 6).unwrap();
    assert_eq!(fake.reg(0x007F), 0x91); // the optical centre the die reports
    assert_eq!(fake.reg(0x0080), 0x57);
    assert_eq!(dev.get_roi().unwrap(), (8, 6));
    dev.set_roi(16, 16).unwrap(); // over 10: the centre moves to 199
    assert_eq!(dev.get_roi_center().unwrap(), 199);
    dev.set_roi_center(0x88).unwrap();
    assert_eq!(dev.get_roi_center().unwrap(), 0x88);
}

#[test]
fn l1_streams_decode_with_the_l1_variant() {
    let (dev, fake) = ready(L1CX_BOARD, None);
    dev.start_ranging().unwrap();
    let h = later(&fake, hex(L1_BLOCK), 1);
    let x = dev.get_measurement(Some(Duration::from_secs(1))).unwrap();
    h.join().unwrap();
    assert_eq!(
        (x.distance_mm, x.status, x.sigma_mm, x.signal_kcps, x.ambient_kcps, x.spads),
        (576, 0, 3.0, 16072.0, 2552.0, 67.0)
    );
    let extra: Vec<(&str, ExtraValue)> = x.extra.iter().map(|(k, v)| (k.as_str(), *v)).collect();
    assert_eq!(
        extra,
        [
            ("ambient_per_spad_kcps", ExtraValue::Int(3)),
            ("signal_per_spad_kcps", ExtraValue::Int(23)),
            ("stream_count", ExtraValue::Int(1)),
        ]
    );
}

// ── ranging ──────────────────────────────────────────────────────────────────

#[test]
fn start_arms_the_stream_with_the_die_block_and_its_release() {
    let (dev, fake) = ready(L4CD_BOARD, None);
    dev.start_ranging().unwrap();
    assert_eq!(fake.writes(), [(0x0087, vec![0x21])]); // continuous
    assert_eq!(fake.sent(START_STREAM), [START_STREAM_DIE]);
    assert!(dev.ranging());
}

#[test]
fn each_streamed_block_is_decoded_once_to_every_subscriber() {
    let (dev, fake) = ready(L4CD_BOARD, None);
    let seen: Arc<Mutex<Vec<Vl53lxMeasurement>>> = Arc::default();
    let s = seen.clone();
    let hook: Hook = dev.on_measurement(move |x| s.lock().unwrap().push(x.clone()));
    let q = dev.measurements(64);
    dev.start_ranging().unwrap();
    fake.stream(&hex(L4_BLOCK), 123_456, 0x0089, None);
    fake.stream(&hex(L4_BLOCK_F9), 123_789, 0x0089, None);
    settle(&fake, &dev);
    {
        let seen = seen.lock().unwrap();
        assert_eq!(seen.iter().map(|x| x.timestamp_us).collect::<Vec<_>>(), [123_456, 123_789]);
        let first = &seen[0];
        assert_eq!((first.distance_mm, first.status, first.status_text.as_str()), (624, 0, "valid"));
        assert_eq!((first.signal_kcps, first.ambient_kcps, first.sigma_mm, first.spads), (5160.0, 1096.0, 2.0, 211.0));
        assert!(first.targets.is_empty() && first.bins.is_none());
        assert_eq!(first.extra["signal_per_spad_kcps"], ExtraValue::Int(24));
        assert_eq!(first.extra["ambient_per_spad_kcps"], ExtraValue::Int(5));
        assert_eq!(first.extra["stream_count"], ExtraValue::Int(1));
        assert_eq!((first.valid(), first.plottable()), (true, true));
        assert_eq!(seen[1].distance_mm, 623);
        assert_eq!(seen[1].extra["stream_count"], ExtraValue::Int(8));
    }
    let take = || q.take(Duration::from_secs(1)).unwrap().distance_mm;
    assert_eq!([take(), take()], [624, 623]);
    hook.cancel();
    fake.stream(&hex(L4_BLOCK), 1, 0x0089, None);
    settle(&fake, &dev);
    assert_eq!(seen.lock().unwrap().len(), 2);
}

#[test]
fn a_nonzero_status_is_not_valid_but_6_is_plottable() {
    let (dev, fake) = ready(L4CD_BOARD, None);
    dev.start_ranging().unwrap();
    let mut blk = hex(L4_BLOCK);
    blk[0] = 0x13; // raw 19 → status 6
    let h = later(&fake, blk, 1);
    let x = dev.get_measurement(Some(Duration::from_secs(1))).unwrap();
    h.join().unwrap();
    assert_eq!((x.status, x.valid(), x.plottable()), (6, false, true));
}

#[test]
fn a_block_the_driver_cannot_decode_is_counted_and_the_reader_survives() {
    let (dev, fake) = ready(L4CD_BOARD, None);
    dev.start_ranging().unwrap();
    fake.stream(&hex(L4_BLOCK)[..5], 1, 0x0089, None);
    settle(&fake, &dev);
    assert_eq!(dev.stream_parse_errors(), 1);
    // The length field says 17, only 5 bytes follow (contract 07 §3).
    fake.stream(&hex(L4_BLOCK)[..5], 1, 0x0089, Some(17));
    settle(&fake, &dev);
    assert_eq!(dev.stream_parse_errors(), 2);
    let h = later(&fake, hex(L4_BLOCK), 2);
    assert_eq!(dev.get_measurement(Some(Duration::from_secs(1))).unwrap().distance_mm, 624);
    h.join().unwrap();
    assert_eq!(dev.bridge_info().unwrap().addr_width, 0); // requests still answered
}

#[test]
fn a_stream_report_shorter_than_its_header_or_before_init_is_no_measurement() {
    let (dev, fake) = connect(L4CD_BOARD);
    let q = dev.measurements(8);
    fake.stream(&hex(L4_BLOCK), 1, 0x0089, None); // before init: swallowed, not decoded
    settle(&fake, &dev);
    dev.init(None, None).unwrap();
    dev.start_ranging().unwrap();
    fake.board.inject(build_packet(0x93, &[0u8; 11], 0, CrcType::None).unwrap());
    settle(&fake, &dev);
    assert!(q.is_empty());
    assert_eq!(dev.stream_parse_errors(), 0);
}

#[test]
fn measurements_is_bounded_drop_oldest_and_deregisters_on_drop() {
    let (dev, fake) = ready(L4CD_BOARD, None);
    let q = dev.measurements(1);
    assert_eq!(dev.stream_dropped_counts(), [0]);
    dev.start_ranging().unwrap();
    fake.stream(&hex(L4_BLOCK), 1, 0x0089, None);
    fake.stream(&hex(L4_BLOCK), 2, 0x0089, None);
    fake.stream(&hex(L4_BLOCK_F9), 3, 0x0089, None);
    settle(&fake, &dev);
    assert_eq!(dev.stream_dropped_counts(), [2]);
    assert_eq!(q.take(Duration::from_secs(1)).unwrap().timestamp_us, 3);
    drop(q);
    assert!(dev.stream_dropped_counts().is_empty());
}

#[test]
fn get_measurement_times_out_and_leaves_no_queue() {
    let (dev, _fake) = ready(L4CD_BOARD, None);
    assert!(matches!(dev.get_measurement(Some(Duration::from_millis(20))), Err(DepzError::Timeout { .. })));
    assert!(dev.stream_dropped_counts().is_empty());
}

#[test]
fn configuration_is_refused_while_the_stream_owns_the_bus() {
    let (dev, fake) = ready(L4CD_BOARD, None);
    dev.start_ranging().unwrap();
    fake.clear();
    let refusals = [
        err_text(dev.init(None, None)),
        err_text(dev.configure(50, 0, None, None, None, None)),
        err_text(dev.set_offset_mm(1)),
        err_text(dev.set_xtalk_kcps(1)),
        err_text(dev.calibrate_offset(100, None)),
        err_text(dev.calibrate_xtalk(100, None)),
        err_text(dev.set_detection_thresholds(1, 2, 3)),
        err_text(dev.set_signal_threshold_kcps(1)),
        err_text(dev.set_sigma_threshold_mm(1)),
        err_text(dev.set_roi(4, 4)),
        err_text(dev.set_roi_center(1)),
        err_text(dev.start_temperature_update()),
        err_text(dev.perform_ref_spad_management()),
        err_text(dev.start_ranging()),
        err_text(dev.measure_once(None)),
    ];
    for r in refusals {
        assert!(r.contains("stop_ranging() first"), "{r}");
    }
    assert!(fake.cmds().is_empty());
    dev.bridge_info().unwrap(); // touches no sensor register: allowed while streaming
}

#[test]
fn stop_ranging_stops_the_stream_then_the_sensor() {
    let (dev, fake) = ready(L4CD_BOARD, None);
    dev.stop_ranging().unwrap(); // not ranging: no-op
    assert!(fake.cmds().is_empty());
    dev.start_ranging().unwrap();
    fake.clear();
    dev.stop_ranging().unwrap();
    assert_eq!(fake.cmds(), [STOP_STREAM, WRITE_REG]);
    assert_eq!(fake.writes(), [(0x0087, vec![0x80])]);
    assert!(!dev.ranging());
}

#[test]
fn a_failed_stop_stream_still_stops_the_sensor_and_clears_the_flag() {
    let (dev, fake) = ready(L4CD_BOARD, None);
    dev.start_ranging().unwrap();
    fake.clear();
    fake.fail_next(STOP_STREAM, Fail::Status(0x01));
    assert!(matches!(dev.stop_ranging(), Err(DepzError::Status { .. })));
    assert!(!dev.ranging());
    assert_eq!(fake.writes(), [(0x0087, vec![0x80])]);
    dev.set_offset_mm(0).unwrap(); // configuration allowed again
}

#[test]
fn a_refused_start_stream_leaves_the_device_not_ranging() {
    let (dev, fake) = ready(L4CD_BOARD, None);
    fake.fail_next(START_STREAM, Fail::Status(0x03));
    assert!(matches!(dev.start_ranging(), Err(DepzError::Status { .. })));
    assert!(!dev.ranging());
}

// ── measure_once (poll) ──────────────────────────────────────────────────────

#[test]
fn measure_once_starts_waits_reads_releases_stops() {
    let (dev, fake) = ready(L4CD_BOARD, None);
    fake.set_result_block(&hex(L4_BLOCK));
    let x = dev.measure_once(None).unwrap();
    assert_eq!((x.distance_mm, x.valid(), x.bins.is_none()), (624, true, true));
    // the timestamp of the last register read: the block itself
    assert_eq!(x.timestamp_us, fake.mcu_time_us() - 100);
    assert!(fake.sent(READ_REG).contains(&"89001100".to_string()));
    let w = fake.writes();
    assert_eq!(w[0], (0x0087, vec![0x21]));
    assert_eq!(w[w.len() - 2..], [(0x0086, vec![0x01]), (0x0087, vec![0x80])]);
    assert!(fake.sent(START_STREAM).is_empty());
}

#[test]
fn a_data_ready_timeout_still_stops_the_sensor() {
    let (dev, fake) = ready(L4CD_BOARD, None);
    fake.set_reg(0x0031, 0x01); // INT never reaches the active level
    assert!(err_text(dev.measure_once(Some(Duration::from_millis(30)))).contains("timeout waiting for data ready"));
    assert_eq!(fake.writes().last(), Some(&(0x0087, vec![0x80])));
    assert!(!dev.ranging());
}

// ── the stream while a call holds the driver ─────────────────────────────────

#[test]
fn a_frame_that_arrives_during_a_driver_call_is_decoded_when_the_call_ends() {
    let (dev, fake) = ready(L4CD_BOARD, None);
    let q = dev.measurements(8);
    dev.start_ranging().unwrap();
    // The reader gets the frame while this thread holds the driver, and must not wait for it: the
    // request inside the call is answered by that same reader.
    let model = dev
        .with_driver(|d| {
            fake.stream(&hex(L4_BLOCK), 7, 0x0089, None);
            dev.bridge_info().unwrap(); // the reader is past the frame: it went to the backlog
            assert!(q.is_empty());
            d.model_id()
        })
        .unwrap();
    assert_eq!(model, DIE_MODEL_ID);
    let x = q.take(Duration::from_secs(1)).unwrap();
    assert_eq!((x.timestamp_us, x.distance_mm), (7, 624));
}

// ── class resolution ─────────────────────────────────────────────────────────

#[test]
fn every_1d_class_attaches_and_gives_its_family_class() {
    for (class, fixed) in [
        ("vl53lx", None),
        ("vl53l0x", Some("VL53L0X")),
        ("vl53l1cx", Some("VL53L1CX")),
        ("vl53l1cb", Some("VL53L1CB")),
        ("vl53l3cx", Some("VL53L3CX")),
        ("vl53l4cx", Some("VL53L4CX")),
    ] {
        let fake = FakeBridge::new(UNSTAMPED, false);
        let any = attach(DepzDevice::new(fake.board.clone(), None), class).unwrap();
        assert_eq!(any.class_name(), class);
        let dev = any.into_vl53lx().unwrap();
        let kinds = dev.driver_kinds(None).unwrap();
        match fixed {
            Some(p) => assert_eq!(kinds, depz_sensor_sdk::vl53lx::uld::driver_kinds(p).unwrap()),
            None => assert!(kinds.is_empty()),
        }
    }
    let fake = FakeBridge::new(UNSTAMPED, false);
    let any = attach(DepzDevice::new(fake.board.clone(), None), "device").unwrap();
    assert!(matches!(any.into_vl53lx(), Err(DepzError::WrongType(_))));
}

// ── configure() over a capture (strict tx) ───────────────────────────────────

fn replay_to_init(stem: &str) -> (Vl53lxDevice, serde_json::Value) {
    let rec = common::recordings_dir();
    let expected: serde_json::Value =
        serde_json::from_str(&std::fs::read_to_string(rec.join(format!("{stem}.expected.json"))).unwrap()).unwrap();
    // The captures hold the pre-§4.4 probe (their scenarios are `probe: legacy`).
    let link = ReplayLink::open(rec.join(format!("{stem}.depzrec")), true, false).unwrap();
    let dev = DepzDevice::new(link, Some(Duration::from_secs(2)));
    let software = dev.get_software_name().unwrap();
    let name = dev.get_device_name().unwrap();
    let any: AnyDevice = attach(dev, resolve_sensor_class(&software, &name, None, None)).unwrap();
    let dev = any.into_vl53lx().unwrap();
    dev.get_software_name().unwrap();
    dev.init(expected["driver"].as_str(), expected["product_arg"].as_str()).unwrap();
    (dev, expected)
}

#[test]
fn configure_refuses_an_unsupported_group_before_the_reinit() {
    // L0X has no signal threshold: refused with no traffic, so the capture's own configure still
    // replays strictly afterwards.
    let (dev, exp) = replay_to_init("vl53l0x_uld_long-range_33ms");
    let budget = exp["budget_ms"].as_u64().unwrap() as u32;
    let mode = exp["mode"].as_str();
    assert!(err_text(dev.configure(budget, 0, mode, None, None, Some(512))).contains("signal_thresh"));
    dev.configure(budget, 0, mode, None, None, None).unwrap();
    let t = dev.get_range_timing().unwrap();
    assert_eq!(serde_json::json!([t.0, t.1]), exp["timing"]);
}

#[test]
fn stream_dropped_counts_per_measurement_queue() {
    // Each live measurements() queue reports its own drop-oldest count; a finished
    // get_measurement() leaves none behind.
    let (dev, exp) = replay_to_init("vl53l1cx_uld_long_33ms");
    assert!(dev.stream_dropped_counts().is_empty());
    dev.configure(exp["budget_ms"].as_u64().unwrap() as u32, 0, exp["mode"].as_str(), None, None, None).unwrap();
    dev.get_range_timing().unwrap();
    let (small, big) = (dev.measurements(2), dev.measurements(4096));
    dev.start_ranging().unwrap();
    assert!(until(|| big.len() >= 20, 5000));
    let n = big.len() as u64;
    assert_eq!(dev.stream_dropped_counts(), [n - 2, 0]);
    assert_eq!(dev.stream_dropped_counts(), [small.dropped_count(), big.dropped_count()]);
    // the capture has no more frames
    assert!(matches!(dev.get_measurement(Some(Duration::from_millis(100))), Err(DepzError::Timeout { .. })));
    assert_eq!(dev.stream_dropped_counts().len(), 2);
    drop(small);
    assert_eq!(dev.stream_dropped_counts(), [0]);
}

// ── the histogram and L0X captures through the device class ──────────────────
//
// What only the stateful drivers give: the histogram ranging (frame pair, consistency checks) and the
// L0X PAL status. Each capture is replayed strict-tx through init / configure / start_ranging, and
// every streamed frame must match its `.expected.json` sidecar.

const DRIVER_CAPTURES: &[&str] = &[
    "vl53l0x_uld_long-range_33ms",
    "vl53l1cb_histogram_medium_33ms",
    "vl53l1cx_histogram_long_33ms",
    "vl53l3cx_histogram_medium_33ms",
    "vl53l4cx_histogram_medium_50ms",
    "vl53l4cx_histogram_short_33ms",
];

fn frame_matches(got: &Vl53lxMeasurement, want: &serde_json::Value) -> Result<(), String> {
    let int = |k: &str| want[k].as_i64().unwrap();
    if (got.timestamp_us as i64, got.distance_mm as i64, got.status as i64)
        != (int("timestamp_us"), int("distance_mm"), int("status"))
    {
        return Err(format!("({}, {}, {})", got.timestamp_us, got.distance_mm, got.status));
    }
    let targets: Vec<(i64, i64)> = got.targets.iter().map(|t| (t.distance_mm as i64, t.status as i64)).collect();
    let want_targets: Vec<(i64, i64)> =
        want["targets"].as_array().unwrap().iter().map(|t| (t[0].as_i64().unwrap(), t[1].as_i64().unwrap())).collect();
    if targets != want_targets {
        return Err(format!("targets {targets:?}"));
    }
    match (&got.bins, want.get("bins")) {
        (None, None) => Ok(()),
        (Some(b), Some(w)) => {
            let bins: Vec<i64> = w["bin_data"].as_array().unwrap().iter().map(|x| x.as_i64().unwrap()).collect();
            let n = w["number_of_bins"].as_i64().unwrap();
            if b.bin_data[..n as usize] != bins[..]
                || b.number_of_bins != n
                || b.vcsel_period != w["vcsel_period"].as_i64().unwrap()
                || b.result_stream_count != w["stream_count"].as_i64().unwrap()
                || b.result_range_status != w["range_status"].as_i64().unwrap() // the raw register
            {
                return Err(format!("bins {b:?}"));
            }
            Ok(())
        }
        (b, w) => Err(format!("bins {} where the sidecar has {}", b.is_some(), w.is_some())),
    }
}

#[test]
fn driver_captures_replay_through_the_device_class() {
    for stem in DRIVER_CAPTURES {
        let (dev, exp) = replay_to_init(stem);
        assert_eq!(dev.product(), exp["product"].as_str(), "{stem}");
        assert_eq!(dev.driver_kind(), exp["driver"].as_str(), "{stem}");
        let budget = exp["budget_ms"].as_u64().unwrap() as u32;
        let mode = exp["mode"].as_str();
        if let Some(refused) = exp.get("refused") {
            // A preset the driver no longer has: refused as an argument; the frames are history.
            let after = &refused["after_init"];
            assert_eq!(serde_json::json!(dev.modes().unwrap()), after["modes"], "{stem}");
            let (lo, hi) = dev.budget_range().unwrap();
            assert_eq!(serde_json::json!([lo, hi]), after["budget_ms"], "{stem}");
            assert_eq!(serde_json::json!(dev.driver_reach_mm().unwrap()), after["driver_reach_mm"], "{stem}");
            let e = dev.configure(budget, 0, mode, None, None, None).unwrap_err();
            assert_eq!(e.kind(), Some("invalid_argument"), "{stem}: {e}");
            common::seen(format!("recordings/{stem}.depzrec").as_str());
            continue;
        }
        dev.configure(budget, 0, mode, None, None, None).unwrap();
        let t = dev.get_range_timing().unwrap();
        assert_eq!(serde_json::json!([t.0, t.1]), exp["timing"], "{stem}");
        let q = dev.measurements(4096);
        dev.start_ranging().unwrap();
        for (i, want) in exp["frames"].as_array().unwrap().iter().enumerate() {
            let got = q.take(Duration::from_secs(5)).unwrap_or_else(|e| panic!("{stem} frame {i}: {e}"));
            if let Err(m) = frame_matches(&got, want) {
                panic!("{stem} frame {i}: got {m}, want {want}");
            }
        }
        dev.stop_ranging().unwrap();
        assert_eq!(dev.stream_parse_errors(), 0, "{stem}");
        common::seen(format!("recordings/{stem}.depzrec").as_str());
    }
}
