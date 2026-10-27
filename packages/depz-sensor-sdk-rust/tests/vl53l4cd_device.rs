//! `Vl53l4cdDevice` (contract 10 register bridge) over an in-process fake bridge — the Rust twin of
//! the Python FakeVl53l4cd (packages/depz-sensor-sdk-python/tests/fake_vl53l4.py) — and the
//! assertions of the Python test_vl53l4cd_device.py: init over the wire, the XSHUT power states,
//! bridge diagnostics, bus re-timing, every configuration getter/setter as a register round trip,
//! both calibrations, the INT-driven stream, poll-mode measure_once, the bridge error statuses.
mod common;

use std::collections::HashMap;
use std::sync::{Arc, Mutex};
use std::time::{Duration, Instant};

use common::fake_board::{until, FakeBoard};
use depz_sensor_sdk::devices::{DepzDevice, DepzError, Vl53l4Measurement, Vl53l4cdDevice};
use depz_sensor_sdk::protocol::common::Status;
use depz_sensor_sdk::vl53l4::driver::DetectionThresholds;
use depz_sensor_sdk::vl53l4::uld_math as uld;
use depz_sensor_sdk::vl53l4::{Vl53l4Cmd, Vl53l4Rpt, XSHUT_OFF, XSHUT_ON, XSHUT_RESET};

const OSC_FREQUENCY_TYPICAL: u16 = 0x3980;
const CLOCK_PLL_TYPICAL: u16 = 0x0A5C;
const ERR_HARDWARE_FAULT: u8 = Status::ErrHardwareFault as u8;

const READ_REG: u8 = Vl53l4Cmd::ReadReg as u8;
const WRITE_REG: u8 = Vl53l4Cmd::WriteReg as u8;
const XSHUT: u8 = Vl53l4Cmd::Xshut as u8;
const START_STREAM: u8 = Vl53l4Cmd::StartStream as u8;
const STOP_STREAM: u8 = Vl53l4Cmd::StopStream as u8;
const GET_INFO: u8 = Vl53l4Cmd::GetInfo as u8;
const SET_I2C_SPEED: u8 = Vl53l4Cmd::SetI2cSpeed as u8;

/// A failure queued for the next request of an opcode: an error status, or no answer at all.
#[derive(Clone, Copy)]
enum Fail {
    Status(u8),
    Silent,
}

/// The sensor's register file, as a booted VL53L4CD answers it.
fn boot_registers() -> HashMap<u16, u8> {
    let mut reg = HashMap::new();
    reg.insert(uld::FIRMWARE__SYSTEM_STATUS, 0x03);
    for (addr, value) in [
        (uld::IDENTIFICATION__MODEL_ID, uld::MODEL_ID_VL53L4CD),
        (uld::OSC_FREQUENCY, OSC_FREQUENCY_TYPICAL),
        (uld::RESULT__OSC_CALIBRATE_VAL, CLOCK_PLL_TYPICAL),
    ] {
        reg.insert(addr, (value >> 8) as u8);
        reg.insert(addr + 1, value as u8);
    }
    reg
}

struct Bridge {
    reg: HashMap<u16, u8>,
    writes: Vec<(u16, Vec<u8>)>,
    speeds: Vec<u16>,
    rx: Vec<(u8, Vec<u8>)>,
    failures: HashMap<u8, Vec<Fail>>,
    powered: bool,
    streaming: bool,
    stream_addr: u16,
    stream_len: u16,
    int_edges: u32,
    i2c_errors: u32,
    short_read_next: bool,
    mcu_time_us: u64,
}

impl Bridge {
    fn read(&self, addr: u16, n: usize) -> Vec<u8> {
        (0..n)
            .map(|i| *self.reg.get(&(addr + i as u16)).unwrap_or(&0))
            .collect()
    }

    fn write(&mut self, addr: u16, data: &[u8]) {
        self.writes.push((addr, data.to_vec()));
        for (i, b) in data.iter().enumerate() {
            self.reg.insert(addr + i as u16, *b);
        }
    }

    fn i2c_khz(&self) -> u16 {
        *self.speeds.last().unwrap_or(&uld::I2C_KHZ_BOOT)
    }
}

/// The fake VL53L4 bridge firmware: READ_REG / WRITE_REG / XSHUT / START_STREAM / STOP_STREAM /
/// GET_INFO / SET_I2C_SPEED over a register file. XSHUT OFF powers the sensor down (register access
/// answers ERR_HARDWARE_FAULT, contract 10 §2); RESET wipes the registers back to boot values.
#[derive(Clone)]
struct FakeVl53l4cd {
    link: FakeBoard,
    s: Arc<Mutex<Bridge>>,
}

impl FakeVl53l4cd {
    fn new() -> FakeVl53l4cd {
        let link = FakeBoard::new();
        let s = Arc::new(Mutex::new(Bridge {
            reg: boot_registers(),
            writes: Vec::new(),
            speeds: Vec::new(),
            rx: Vec::new(),
            failures: HashMap::new(),
            powered: true,
            streaming: false,
            stream_addr: 0,
            stream_len: 0,
            int_edges: 0,
            i2c_errors: 0,
            short_read_next: false,
            mcu_time_us: 1_000_000,
        }));
        let (l, st) = (link.clone(), s.clone());
        link.on_packet(move |p| {
            let mut b = st.lock().unwrap();
            let (cmd, pl) = (p.cmd, p.payload.as_slice());
            b.rx.push((cmd, pl.to_vec()));
            if let Some(q) = b.failures.get_mut(&cmd).filter(|q| !q.is_empty()) {
                return match q.remove(0) {
                    Fail::Status(s) => vec![FakeBoard::status(cmd, s)],
                    Fail::Silent => vec![],
                };
            }
            let ok = vec![FakeBoard::status(cmd, Status::Ok as u8)];
            if (cmd == READ_REG || cmd == WRITE_REG) && !b.powered {
                b.i2c_errors += 1; // an unpowered sensor NACKs
                return vec![FakeBoard::status(cmd, ERR_HARDWARE_FAULT)];
            }
            match cmd {
                READ_REG => {
                    let addr = u16::from_le_bytes([pl[0], pl[1]]);
                    let n = u16::from_le_bytes([pl[2], pl[3]]) as usize;
                    let mut data = b.read(addr, n);
                    if b.short_read_next {
                        b.short_read_next = false;
                        data.pop();
                    }
                    let mut reply = vec![cmd];
                    reply.extend_from_slice(&b.mcu_time_us.to_le_bytes());
                    reply.extend_from_slice(&data);
                    b.mcu_time_us += 100;
                    vec![l.reply(Vl53l4Rpt::RegData as u8, &reply)]
                }
                WRITE_REG => {
                    let addr = u16::from_le_bytes([pl[0], pl[1]]);
                    b.write(addr, &pl[2..]);
                    ok
                }
                XSHUT => {
                    match pl[0] {
                        XSHUT_OFF => {
                            b.powered = false;
                            b.streaming = false;
                        }
                        XSHUT_ON => b.powered = true,
                        _ => {
                            b.reg = boot_registers();
                            b.writes.clear();
                            b.powered = true;
                            b.streaming = false;
                        }
                    }
                    ok
                }
                START_STREAM => {
                    b.stream_addr = u16::from_le_bytes([pl[0], pl[1]]);
                    b.stream_len = u16::from_le_bytes([pl[2], pl[3]]);
                    b.streaming = true;
                    ok
                }
                STOP_STREAM => {
                    b.streaming = false;
                    ok
                }
                GET_INFO => {
                    let model_id = if b.powered {
                        u16::from_be_bytes(b.read(0x010F, 2).try_into().unwrap())
                    } else {
                        0
                    };
                    let mut r = Vec::new();
                    r.extend_from_slice(&b.int_edges.to_le_bytes());
                    r.extend_from_slice(&0u32.to_le_bytes());
                    r.extend_from_slice(&b.i2c_errors.to_le_bytes());
                    r.push(u8::from(b.i2c_errors > 0));
                    r.extend_from_slice(&model_id.to_le_bytes());
                    r.push(*b.reg.get(&uld::FIRMWARE__SYSTEM_STATUS).unwrap_or(&0));
                    r.push(u8::from(model_id == uld::MODEL_ID_VL53L4CD));
                    r.push(u8::from(b.powered));
                    r.push(b.reg.get(&0x0031).unwrap_or(&0) & 1);
                    r.extend_from_slice(&b.i2c_khz().to_le_bytes());
                    vec![l.reply(Vl53l4Rpt::Info as u8, &r)]
                }
                SET_I2C_SPEED => {
                    let khz = u16::from_le_bytes([pl[0], pl[1]]);
                    b.speeds.push(khz);
                    ok
                }
                _ => vec![FakeBoard::status(cmd, Status::ErrInvalidCmd as u8)],
            }
        });
        FakeVl53l4cd { link, s }
    }

    fn fail_next(&self, cmd: u8, fail: Fail) {
        self.s
            .lock()
            .unwrap()
            .failures
            .entry(cmd)
            .or_default()
            .push(fail);
    }

    fn cmds(&self) -> Vec<u8> {
        self.s.lock().unwrap().rx.iter().map(|(c, _)| *c).collect()
    }

    fn last_rx(&self) -> (u8, Vec<u8>) {
        self.s.lock().unwrap().rx.last().cloned().unwrap()
    }

    fn clear_log(&self) {
        let mut b = self.s.lock().unwrap();
        b.rx.clear();
        b.writes.clear();
    }

    fn written_to(&self, addr: u16) -> Vec<Vec<u8>> {
        let b = self.s.lock().unwrap();
        b.writes
            .iter()
            .filter(|(a, _)| *a == addr)
            .map(|(_, d)| d.clone())
            .collect()
    }

    fn set_result_block(&self, raw: &[u8]) {
        let mut b = self.s.lock().unwrap();
        for (i, v) in raw.iter().enumerate() {
            b.reg.insert(uld::RESULT_BLOCK_ADDR + i as u16, *v);
        }
    }

    fn mcu_time_us(&self) -> u64 {
        self.s.lock().unwrap().mcu_time_us
    }

    /// One RPT_VL53_STREAM with `raw`, as the INT edge sends it; returns its MCU timestamp.
    fn send_block(&self, raw: &[u8], len_field: u16) -> u64 {
        let (t, addr) = {
            let mut b = self.s.lock().unwrap();
            b.int_edges += 1;
            b.mcu_time_us += 30_000;
            (b.mcu_time_us, b.stream_addr)
        };
        let mut p = t.to_le_bytes().to_vec();
        p.extend_from_slice(&addr.to_le_bytes());
        p.extend_from_slice(&len_field.to_le_bytes());
        p.extend_from_slice(raw);
        self.link
            .inject(self.link.reply(Vl53l4Rpt::Stream as u8, &p));
        t
    }

    fn send_result(&self, raw: &[u8]) -> u64 {
        self.send_block(raw, raw.len() as u16)
    }
}

/// A raw 17-byte result block (big-endian registers 0x0089..0x0099); `status` is the raw
/// RESULT__RANGE_STATUS (9 → ULD status 0 "valid"), `spads` the raw RESULT__SPAD_NB word.
fn result_block(
    distance_mm: u16,
    status: u8,
    sigma_mm: u16,
    signal_kcps: u16,
    spads: u16,
    stream_count: u8,
) -> Vec<u8> {
    let mut raw = vec![0u8; 17];
    raw[0] = status;
    raw[2] = stream_count;
    raw[3..5].copy_from_slice(&spads.to_be_bytes());
    raw[5..7].copy_from_slice(&(signal_kcps / 8).to_be_bytes());
    raw[9..11].copy_from_slice(&(sigma_mm * 4).to_be_bytes());
    raw[13..15].copy_from_slice(&distance_mm.to_be_bytes());
    raw
}

fn block(distance_mm: u16) -> Vec<u8> {
    result_block(distance_mm, 9, 3, 1200, 2048, 1)
}

fn connect(timeout_ms: u64) -> (Vl53l4cdDevice, FakeVl53l4cd) {
    let fake = FakeVl53l4cd::new();
    let dev = Vl53l4cdDevice::attach(DepzDevice::new(
        fake.link.clone(),
        Some(Duration::from_millis(timeout_ms)),
    ));
    (dev, fake)
}

fn ready() -> (Vl53l4cdDevice, FakeVl53l4cd) {
    let (dev, fake) = connect(500);
    dev.init(None).unwrap();
    fake.clear_log();
    (dev, fake)
}

fn be16(v: u16) -> Vec<u8> {
    v.to_be_bytes().to_vec()
}

// ── init / identity ──────────────────────────────────────────────────────────

#[test]
fn init_runs_the_uld_boot_sequence_over_the_bridge() {
    let (dev, fake) = connect(500);
    assert!(!dev.initialized());
    dev.init(None).unwrap();
    assert!(dev.initialized() && !dev.ranging());
    assert_eq!(
        fake.s.lock().unwrap().speeds,
        [uld::I2C_KHZ_BOOT, uld::I2C_KHZ_DEFAULT]
    );
    assert_eq!(
        fake.written_to(uld::CONFIG_ADDR),
        [uld::config_block().to_vec()]
    );
    let cmds = fake.cmds();
    let speeds: Vec<usize> = (0..cmds.len())
        .filter(|&i| cmds[i] == SET_I2C_SPEED)
        .collect();
    let config_write = cmds.iter().position(|&c| c == WRITE_REG).unwrap();
    assert!(speeds[0] < config_write && config_write < speeds[1]);
    // init's 50 ms continuous default reads back as 49 (ST's integer math)
    assert_eq!(dev.get_range_timing().unwrap(), (49, 0));
    assert_eq!(dev.uld().get_sensor_id().unwrap(), uld::MODEL_ID_VL53L4CD);
}

#[test]
fn init_at_a_chosen_bus_speed() {
    let (dev, fake) = connect(500);
    dev.init(Some(400)).unwrap();
    assert_eq!(fake.s.lock().unwrap().speeds, [400]);
    assert_eq!(dev.bridge_info().unwrap().i2c_khz, 400);
}

#[test]
fn is_alive_checks_the_model_id_and_swallows_bridge_errors() {
    let (dev, fake) = connect(500);
    assert!(dev.is_alive());
    fake.s
        .lock()
        .unwrap()
        .reg
        .insert(uld::IDENTIFICATION__MODEL_ID + 1, 0xCC);
    assert!(!dev.is_alive());
    fake.fail_next(READ_REG, Fail::Status(ERR_HARDWARE_FAULT));
    assert!(!dev.is_alive());
}

// ── XSHUT ────────────────────────────────────────────────────────────────────

#[test]
fn xshut_off_powers_the_sensor_down_and_forgets_the_init() {
    let (dev, fake) = ready();
    dev.start_ranging().unwrap();
    dev.xshut(XSHUT_OFF).unwrap();
    assert_eq!(fake.last_rx(), (XSHUT, vec![XSHUT_OFF]));
    {
        let b = fake.s.lock().unwrap();
        assert!(!b.powered && !b.streaming);
    }
    assert!(!dev.initialized() && !dev.ranging());
    // every register access now answers ERR_HARDWARE_FAULT (contract 10 §2)
    assert!(!dev.is_alive());
    let fault = |r: Result<(), DepzError>, cmd: u8| match r {
        Err(DepzError::Status { cmd: c, status }) => {
            assert_eq!((c, status), (cmd, ERR_HARDWARE_FAULT))
        }
        other => panic!("{other:?}"),
    };
    fault(dev.get_offset_mm().map(|_| ()), READ_REG);
    fault(dev.init(None), READ_REG);
    let info = dev.bridge_info().unwrap();
    assert_eq!(
        (info.xshut_level, info.initialized, info.model_id),
        (0, 0, 0)
    );
    assert!(info.i2c_errors >= 3 && info.last_i2c_error == 1);
    dev.xshut(XSHUT_ON).unwrap();
    assert!(fake.s.lock().unwrap().powered && dev.is_alive());
}

#[test]
fn an_xshut_action_past_reset_is_refused_before_anything_is_sent() {
    let (dev, fake) = ready();
    match dev.xshut(3) {
        Err(DepzError::InvalidArgument(m)) => assert!(m.contains("got 3"), "{m}"),
        other => panic!("{other:?}"),
    }
    assert!(fake.cmds().is_empty());
}

#[test]
fn reset_sensor_wipes_the_configuration_and_needs_init_again() {
    let (dev, fake) = ready();
    dev.set_offset_mm(7).unwrap();
    dev.reset_sensor().unwrap();
    assert_eq!(fake.last_rx(), (XSHUT, vec![XSHUT_RESET]));
    assert!(!dev.initialized());
    assert_eq!(dev.get_offset_mm().unwrap(), 0); // back to the boot register file
    assert!(fake.written_to(uld::CONFIG_ADDR).is_empty());
    dev.init(None).unwrap();
    assert!(dev.initialized());
    assert_eq!(
        fake.written_to(uld::CONFIG_ADDR),
        [uld::config_block().to_vec()]
    );
}

// ── bridge diagnostics / bus speed ───────────────────────────────────────────

#[test]
fn bridge_info_decodes_the_21_byte_report() {
    let (dev, fake) = ready();
    let info = dev.bridge_info().unwrap();
    assert_eq!(fake.last_rx(), (GET_INFO, vec![]));
    assert_eq!(
        (info.model_id, info.fw_status, info.initialized),
        (uld::MODEL_ID_VL53L4CD, 0x03, 1)
    );
    assert_eq!(
        (info.xshut_level, info.i2c_khz, info.int_edges),
        (1, uld::I2C_KHZ_DEFAULT, 0)
    );
    // counters move with the INT edges; GET_INFO is safe while streaming
    dev.start_ranging().unwrap();
    fake.send_result(&block(100));
    fake.send_result(&block(101));
    assert_eq!(dev.bridge_info().unwrap().int_edges, 2);
}

#[test]
fn set_i2c_speed_khz_retimes_the_bridge() {
    let (dev, fake) = ready();
    dev.set_i2c_speed_khz(400).unwrap();
    assert_eq!(
        fake.last_rx(),
        (SET_I2C_SPEED, 400u16.to_le_bytes().to_vec())
    );
    assert_eq!(dev.bridge_info().unwrap().i2c_khz, 400);
}

// ── configuration round trips (init first; not while ranging) ────────────────

#[test]
fn every_configuration_method_round_trips_through_the_registers() {
    let (dev, fake) = ready();
    dev.set_range_timing(100, 250).unwrap();
    let (a, _b, inter) =
        uld::range_timing_registers(100, 250, OSC_FREQUENCY_TYPICAL, CLOCK_PLL_TYPICAL).unwrap();
    assert_eq!(dev.uld().read_word(uld::RANGE_CONFIG_A).unwrap(), a);
    assert_eq!(
        dev.uld().read_dword(uld::INTERMEASUREMENT_MS).unwrap(),
        inter
    );
    let timing = dev.get_range_timing().unwrap();
    assert_eq!(
        timing,
        uld::decode_range_timing(inter, CLOCK_PLL_TYPICAL, OSC_FREQUENCY_TYPICAL, a).unwrap()
    );
    assert!((240..250).contains(&timing.1), "{timing:?}"); // the 1.055 / 1.065 factor pair reads short
    dev.set_range_timing(33, 0).unwrap();
    assert_eq!(dev.get_range_timing().unwrap().1, 0); // continuous again
    dev.set_offset_mm(-12).unwrap();
    assert_eq!(dev.get_offset_mm().unwrap(), -12);
    assert_eq!(
        fake.written_to(uld::RANGE_OFFSET_MM).pop().unwrap(),
        be16((-48i16) as u16)
    );
    dev.set_xtalk_kcps(20).unwrap();
    assert_eq!(dev.get_xtalk_kcps().unwrap(), 20);
    assert_eq!(
        fake.written_to(uld::XTALK_PLANE_OFFSET_KCPS).pop().unwrap(),
        be16(20 << 9)
    );
    dev.set_detection_thresholds(100, 300, uld::WINDOW_OUT)
        .unwrap();
    assert_eq!(
        dev.get_detection_thresholds().unwrap(),
        DetectionThresholds {
            distance_low_mm: 100,
            distance_high_mm: 300,
            window: uld::WINDOW_OUT
        }
    );
    assert_eq!(
        fake.written_to(uld::SYSTEM__INTERRUPT).pop().unwrap(),
        [uld::WINDOW_OUT]
    );
    dev.set_signal_threshold_kcps(1024).unwrap();
    assert_eq!(dev.get_signal_threshold_kcps().unwrap(), 1024);
    assert_eq!(
        fake.written_to(uld::MIN_COUNT_RATE_RTN_LIMIT_MCPS)
            .pop()
            .unwrap(),
        be16(1024 >> 3)
    );
    dev.set_sigma_threshold_mm(15).unwrap();
    assert_eq!(dev.get_sigma_threshold_mm().unwrap(), 15);
    assert_eq!(
        fake.written_to(uld::RANGE_CONFIG__SIGMA_THRESH)
            .pop()
            .unwrap(),
        be16(15 << 2)
    );
}

#[test]
fn argument_errors_are_invalid_argument_and_reach_no_register() {
    let (dev, fake) = ready();
    // ERRATA A16: a refused argument is invalid_argument, not the driver's sensor error
    let refused = |r: Result<(), DepzError>| {
        assert!(matches!(r, Err(DepzError::InvalidArgument(_))), "{r:?}")
    };
    refused(dev.set_range_timing(50, 30));
    refused(dev.set_range_timing(9, 0));
    assert!(fake.cmds().is_empty()); // range timing refuses before any traffic, reads included
    refused(dev.set_offset_mm(5000));
    refused(dev.set_xtalk_kcps(128));
    refused(dev.set_sigma_threshold_mm(16384));
    for reg in [
        uld::RANGE_CONFIG_A,
        uld::INTERMEASUREMENT_MS,
        uld::RANGE_OFFSET_MM,
        uld::XTALK_PLANE_OFFSET_KCPS,
        uld::RANGE_CONFIG__SIGMA_THRESH,
    ] {
        assert!(fake.written_to(reg).is_empty(), "0x{reg:04X}");
    }
    fake.clear_log();
    refused(dev.calibrate_offset(5, None).map(|_| ()));
    refused(dev.calibrate_xtalk(100, Some(4)).map(|_| ()));
    assert!(fake.cmds().is_empty()); // calibrations refuse before any traffic
}

#[test]
fn start_temperature_update_runs_the_vhv_burst() {
    let (dev, fake) = ready();
    dev.start_temperature_update().unwrap();
    assert_eq!(
        fake.written_to(uld::VHV_CONFIG__TIMEOUT_MACROP_LOOP_BOUND),
        [vec![0x81], vec![0x09]]
    );
    assert_eq!(
        fake.written_to(uld::SYSTEM_START)
            .iter()
            .map(|d| d[0])
            .collect::<Vec<_>>(),
        [0x21, 0x80]
    );
    assert!(!dev.ranging());
}

#[test]
fn calibrations_run_their_sample_bursts_over_the_bridge() {
    let (dev, fake) = ready();
    fake.set_result_block(&result_block(120, 9, 3, 1200, 2048, 1));
    assert_eq!(dev.calibrate_offset(150, Some(5)).unwrap(), 30);
    assert_eq!(dev.get_offset_mm().unwrap(), 30);
    assert_eq!(dev.calibrate_xtalk(240, Some(5)).unwrap(), 75); // (1 − 120/240) · 1200/8
    assert_eq!(dev.get_xtalk_kcps().unwrap(), 75);
    assert!(!dev.ranging());
    // each burst: start, (ready, result, clear) × n, stop — all over READ/WRITE_REG
    let mut cmds = fake.cmds();
    cmds.sort_unstable();
    cmds.dedup();
    assert_eq!(cmds, [READ_REG, WRITE_REG]);
}

#[test]
fn calibrations_and_temperature_update_refuse_while_ranging() {
    let (dev, fake) = ready();
    dev.start_ranging().unwrap();
    fake.clear_log();
    let busy =
        |r: Result<(), DepzError>| assert!(matches!(r, Err(DepzError::InvalidState(_))), "{r:?}");
    busy(dev.start_temperature_update());
    busy(dev.calibrate_offset(100, None).map(|_| ()));
    busy(dev.calibrate_xtalk(100, None).map(|_| ()));
    busy(dev.init(None));
    busy(dev.set_i2c_speed_khz(400));
    busy(dev.set_range_timing(50, 0));
    busy(dev.set_xtalk_kcps(1));
    busy(dev.set_detection_thresholds(1, 2, uld::WINDOW_IN));
    busy(dev.set_signal_threshold_kcps(1));
    busy(dev.set_sigma_threshold_mm(1));
    assert!(fake.cmds().is_empty());
}

// ── ranging: the INT-driven stream ───────────────────────────────────────────

#[test]
fn start_ranging_starts_the_sensor_then_arms_the_stream() {
    let (dev, fake) = ready();
    dev.start_ranging().unwrap();
    assert!(dev.ranging() && fake.s.lock().unwrap().streaming);
    assert_eq!(fake.written_to(uld::SYSTEM_START).pop().unwrap(), [0x21]);
    assert_eq!(
        fake.last_rx(),
        (START_STREAM, vec![0x89, 0x00, 17, 0x00, 0x00])
    );
    {
        let b = fake.s.lock().unwrap();
        assert_eq!(
            (b.stream_addr, b.stream_len),
            (uld::RESULT_BLOCK_ADDR, uld::RESULT_BLOCK_LEN)
        );
    }
    assert!(matches!(
        dev.start_ranging(),
        Err(DepzError::InvalidState(_))
    )); // already ranging
    dev.stop_ranging().unwrap();
    assert!(!dev.ranging() && !fake.s.lock().unwrap().streaming);
    // the bridge stream is released first, then the sensor is told to stop
    let rx = fake.s.lock().unwrap().rx.clone();
    let mut stop = uld::SYSTEM_START.to_le_bytes().to_vec();
    stop.push(0x80);
    assert_eq!(
        rx[rx.len() - 2..],
        [(STOP_STREAM, vec![]), (WRITE_REG, stop)]
    );
}

#[test]
fn stop_ranging_clears_the_state_even_when_the_bridge_refuses() {
    let (dev, fake) = ready();
    dev.start_ranging().unwrap();
    fake.fail_next(STOP_STREAM, Fail::Status(ERR_HARDWARE_FAULT));
    assert!(matches!(
        dev.stop_ranging(),
        Err(DepzError::Status {
            cmd: STOP_STREAM,
            status: ERR_HARDWARE_FAULT
        })
    ));
    assert!(!dev.ranging());
    // the sensor itself was still told to stop, so reconfiguration is possible
    assert_eq!(fake.written_to(uld::SYSTEM_START).pop().unwrap(), [0x80]);
    dev.set_offset_mm(3).unwrap();
    assert_eq!(dev.get_offset_mm().unwrap(), 3);
}

#[test]
fn streamed_blocks_reach_callbacks_and_the_measurement_queue() {
    let (dev, fake) = ready();
    let got: Arc<Mutex<Vec<Vl53l4Measurement>>> = Arc::default();
    let g = got.clone();
    let hook = dev.on_measurement(move |m| g.lock().unwrap().push(*m));
    let q = dev.measurements(4);
    dev.start_ranging().unwrap();
    let raw = result_block(291, 9, 3, 1600, 2048, 7);
    let t = fake.send_result(&raw);
    let m = q.take(Duration::from_secs(2)).unwrap();
    let r = uld::parse_result_block(&raw).unwrap();
    assert_eq!(m.timestamp_us, t);
    assert_eq!(
        (
            m.distance_mm,
            m.sigma_mm,
            m.signal_rate_kcps,
            m.number_of_spad
        ),
        (291, 3, 1600, 8)
    );
    assert_eq!(
        (
            m.distance_mm,
            m.sigma_mm,
            m.signal_rate_kcps,
            m.range_status
        ),
        (
            r.distance_mm,
            r.sigma_mm,
            r.signal_rate_kcps,
            r.range_status
        )
    );
    assert_eq!((m.stream_count, m.valid()), (7, true));
    assert_eq!(m.status_text(), "valid");
    assert!(until(|| got.lock().unwrap().len() == 1, 2000));
    assert_eq!(got.lock().unwrap()[0], m);
    drop(hook);
    fake.send_result(&result_block(77, 4, 3, 1200, 2048, 1));
    let m2 = q.take(Duration::from_secs(2)).unwrap();
    assert_eq!(
        (m2.distance_mm, m2.range_status, m2.valid()),
        (77, 2, false)
    );
    assert_eq!(m2.status_text(), "signal below threshold");
    assert_eq!(got.lock().unwrap().len(), 1); // unsubscribed
}

#[test]
fn a_range_status_the_table_lacks_reads_unknown_with_its_code() {
    let m = Vl53l4Measurement {
        timestamp_us: 0,
        range_status: 200,
        distance_mm: 0,
        sigma_mm: 0,
        signal_rate_kcps: 0,
        ambient_rate_kcps: 0,
        signal_per_spad_kcps: 0,
        ambient_per_spad_kcps: 0,
        number_of_spad: 0,
        stream_count: 0,
    };
    assert_eq!(m.status_text(), "unknown (200)");
    assert!(!m.valid());
}

#[test]
fn the_measurement_queue_is_bounded_drop_oldest() {
    let (dev, fake) = ready();
    let q = dev.measurements(1);
    dev.start_ranging().unwrap();
    for d in [10, 20, 30] {
        fake.send_result(&block(d));
    }
    dev.bridge_info().unwrap(); // queues behind the reports: the reader handled them
    assert_eq!(q.dropped_count(), 2);
    assert_eq!(dev.stream_dropped_counts(), [2]);
    assert_eq!(q.take(Duration::from_secs(1)).unwrap().distance_mm, 30);
    drop(q);
    assert!(dev.stream_dropped_counts().is_empty()); // deregistered
}

#[test]
fn get_measurement_returns_the_next_block_or_times_out() {
    let (dev, fake) = ready();
    dev.start_ranging().unwrap();
    let t0 = Instant::now();
    match dev.get_measurement(Some(Duration::from_millis(150))) {
        Err(e @ DepzError::Timeout { .. }) => assert_eq!(e.kind(), Some("timeout")),
        other => panic!("{other:?}"),
    }
    assert!(t0.elapsed() >= Duration::from_millis(100));
    assert!(dev.stream_dropped_counts().is_empty()); // the one-shot queue is gone
    std::thread::scope(|s| {
        s.spawn(|| {
            // send once get_measurement has subscribed its one-slot queue (a live queue is listed)
            assert!(until(|| dev.stream_dropped_counts().len() == 1, 2000));
            fake.send_result(&block(625));
        });
        assert_eq!(
            dev.get_measurement(Some(Duration::from_secs(5)))
                .unwrap()
                .distance_mm,
            625
        );
    });
}

#[test]
fn undecodable_stream_blocks_are_counted_not_fatal() {
    let (dev, fake) = ready();
    let q = dev.measurements(4);
    dev.start_ranging().unwrap();
    fake.send_block(&[0u8; 4], 4); // somebody re-armed the stream on a different block
    fake.send_result(&block(624));
    assert_eq!(q.take(Duration::from_secs(2)).unwrap().distance_mm, 624);
    assert_eq!(dev.stream_parse_errors(), 1);
    // a report shorter than its header is not a stream sample at all
    fake.link
        .inject(fake.link.reply(Vl53l4Rpt::Stream as u8, &[0u8; 11]));
    dev.bridge_info().unwrap();
    assert_eq!(dev.stream_parse_errors(), 1);
}

// ── poll mode ────────────────────────────────────────────────────────────────

#[test]
fn measure_once_polls_one_result_and_stops() {
    let (dev, fake) = ready();
    fake.set_result_block(&result_block(333, 9, 3, 1200, 2048, 5));
    let m = dev.measure_once(None).unwrap();
    assert_eq!((m.distance_mm, m.stream_count, m.valid()), (333, 5, true));
    assert_eq!(m.timestamp_us, fake.mcu_time_us() - 100); // the block read's MCU stamp
    assert_eq!(
        fake.written_to(uld::SYSTEM_START)
            .iter()
            .map(|d| d[0])
            .collect::<Vec<_>>(),
        [0x21, 0x80]
    );
    assert_eq!(fake.written_to(uld::SYSTEM__INTERRUPT_CLEAR), [vec![0x01]]);
    assert!(!fake.cmds().contains(&START_STREAM) && !dev.ranging());
}

#[test]
fn measure_once_times_out_and_still_stops_the_sensor() {
    let (dev, fake) = ready();
    fake.s
        .lock()
        .unwrap()
        .reg
        .insert(uld::GPIO__TIO_HV_STATUS, 0x01); // INT never asserted
    match dev.measure_once(Some(Duration::from_millis(10))) {
        Err(DepzError::Sensor(m)) => assert!(m.contains("data ready"), "{m}"),
        other => panic!("{other:?}"),
    }
    assert_eq!(fake.written_to(uld::SYSTEM_START).pop().unwrap(), [0x80]);
}

// ── register transfers and bridge errors ─────────────────────────────────────

#[test]
fn a_short_register_read_is_an_error() {
    let (dev, fake) = ready();
    fake.s.lock().unwrap().short_read_next = true;
    match dev.get_offset_mm() {
        Err(DepzError::Protocol(m)) => assert!(m.contains("expected 2, got 1"), "{m}"),
        other => panic!("{other:?}"),
    }
}

#[test]
fn bridge_error_statuses_are_status_errors() {
    let (dev, fake) = ready();
    let fault = |r: Result<(), DepzError>, cmd: u8| match r {
        Err(DepzError::Status { cmd: c, status }) => {
            assert_eq!((c, status), (cmd, ERR_HARDWARE_FAULT))
        }
        other => panic!("{other:?}"),
    };
    fake.fail_next(WRITE_REG, Fail::Status(ERR_HARDWARE_FAULT));
    fault(dev.set_offset_mm(1), WRITE_REG);
    fake.fail_next(READ_REG, Fail::Status(ERR_HARDWARE_FAULT));
    fault(dev.get_range_timing().map(|_| ()), READ_REG);
    fake.fail_next(XSHUT, Fail::Status(ERR_HARDWARE_FAULT)); // RESET boot handshake timed out
    fault(dev.reset_sensor(), XSHUT);
    fake.fail_next(START_STREAM, Fail::Status(ERR_HARDWARE_FAULT));
    fault(dev.start_ranging(), START_STREAM);
    assert!(!dev.ranging());
}

#[test]
fn no_answer_is_a_timeout() {
    let (dev, fake) = connect(100);
    fake.fail_next(GET_INFO, Fail::Silent);
    assert!(matches!(
        dev.bridge_info(),
        Err(DepzError::Timeout {
            cmd: Some(GET_INFO),
            ..
        })
    ));
    fake.fail_next(START_STREAM, Fail::Silent);
    dev.init(None).unwrap();
    assert!(matches!(
        dev.start_ranging(),
        Err(DepzError::Timeout {
            cmd: Some(START_STREAM),
            ..
        })
    ));
    assert!(!dev.ranging());
}
