//! The VL53L4CD ULD over a register map seeded like a booted sensor, and the device class over a fake
//! board.
mod common;

use std::collections::HashMap;
use std::sync::{Arc, Mutex};
use std::time::Duration;

use common::fake_board::{until, FakeBoard};
use depz_sensor_sdk::devices::{DepzDevice, DepzError, Result, Vl53l4cdDevice};
use depz_sensor_sdk::protocol::common::Status;
use depz_sensor_sdk::vl53l4::driver::{DetectionThresholds, Vl53l4Platform, Vl53l4cdDriver};
use depz_sensor_sdk::vl53l4::uld_math as uld;
use depz_sensor_sdk::vl53l4::{Vl53l4Cmd, Vl53l4Rpt};

const OSC_FREQUENCY_TYPICAL: u16 = 0x3980;
const CLOCK_PLL_TYPICAL: u16 = 0x0A5C;

/// A plain register map seeded with what a booted VL53L4CD answers; the config block leaves it "data ready".
#[derive(Default)]
struct FakePlatform {
    reg: Mutex<HashMap<u16, u8>>,
    writes: Mutex<Vec<(u16, Vec<u8>)>>,
    speeds: Mutex<Vec<u16>>,
}

impl FakePlatform {
    fn new() -> Arc<FakePlatform> {
        let p = FakePlatform::default();
        p.reg.lock().unwrap().insert(uld::FIRMWARE__SYSTEM_STATUS, 0x03);
        p.reg.lock().unwrap().insert(uld::GPIO_HV_MUX__CTRL, 0x11); // INT active low, data ready
        p.seed_word(uld::IDENTIFICATION__MODEL_ID, uld::MODEL_ID_VL53L4CD);
        p.seed_word(uld::OSC_FREQUENCY, OSC_FREQUENCY_TYPICAL);
        p.seed_word(uld::RESULT__OSC_CALIBRATE_VAL, CLOCK_PLL_TYPICAL);
        Arc::new(p)
    }

    fn seed_word(&self, addr: u16, value: u16) {
        let mut r = self.reg.lock().unwrap();
        r.insert(addr, (value >> 8) as u8);
        r.insert(addr + 1, value as u8);
    }

    fn written_to(&self, addr: u16) -> Vec<Vec<u8>> {
        self.writes.lock().unwrap().iter().filter(|(a, _)| *a == addr).map(|(_, d)| d.clone()).collect()
    }

    fn last(&self, addr: u16) -> Vec<u8> {
        self.written_to(addr).pop().expect("written")
    }

    /// Every result read returns a valid measurement with these values.
    fn seed_result(&self, distance_mm: u16, signal_raw: u16, spads_raw: u16) {
        self.reg.lock().unwrap().insert(uld::RESULT__RANGE_STATUS, valid_raw());
        self.seed_word(uld::RESULT__SPAD_NB, spads_raw);
        self.seed_word(uld::RESULT__SIGNAL_RATE, signal_raw);
        self.seed_word(uld::RESULT__DISTANCE, distance_mm);
    }
}

impl Vl53l4Platform for FakePlatform {
    fn read_multi(&self, addr: u16, size: usize) -> Result<Vec<u8>> {
        let r = self.reg.lock().unwrap();
        Ok((0..size).map(|i| *r.get(&(addr + i as u16)).unwrap_or(&0)).collect())
    }

    fn write_multi(&self, addr: u16, data: &[u8]) -> Result<()> {
        self.writes.lock().unwrap().push((addr, data.to_vec()));
        let mut r = self.reg.lock().unwrap();
        for (i, b) in data.iter().enumerate() {
            r.insert(addr + i as u16, *b);
        }
        Ok(())
    }

    fn sleep_ms(&self, _ms: u32) {}

    fn set_i2c_speed(&self, khz: u16) -> Result<()> {
        self.speeds.lock().unwrap().push(khz);
        Ok(())
    }
}

fn valid_raw() -> u8 {
    uld::STATUS_RTN.iter().position(|s| *s == 0).expect("a valid raw status") as u8
}

fn word(b: &[u8]) -> u16 {
    u16::from_be_bytes([b[0], b[1]])
}

fn driver(p: &Arc<FakePlatform>) -> Vl53l4cdDriver {
    Vl53l4cdDriver::new(p.clone())
}

#[test]
fn init_runs_at_boot_speed_then_retimes() {
    let p = FakePlatform::new();
    driver(&p).sensor_init(uld::I2C_KHZ_DEFAULT).unwrap();
    let speeds = p.speeds.lock().unwrap().clone();
    assert_eq!(speeds[0], uld::I2C_KHZ_BOOT);
    assert_eq!(*speeds.last().unwrap(), uld::I2C_KHZ_DEFAULT);
    let p2 = FakePlatform::new();
    driver(&p2).sensor_init(uld::I2C_KHZ_BOOT).unwrap();
    assert_eq!(*p2.speeds.lock().unwrap(), vec![uld::I2C_KHZ_BOOT]);
}

#[test]
fn init_writes_the_config_block_and_the_vhv_sequence() {
    let p = FakePlatform::new();
    let drv = driver(&p);
    drv.sensor_init(uld::I2C_KHZ_DEFAULT).unwrap();
    let block = p.written_to(uld::CONFIG_ADDR)[0].clone();
    assert_eq!(block, uld::config_block().to_vec());
    assert_eq!(block[0], uld::CONFIG_FMP_BYTE);
    let starts = p.written_to(uld::SYSTEM_START);
    assert_eq!((starts[0][0], starts[1][0]), (0x40, 0x80));
    assert_eq!(p.written_to(uld::SYSTEM__INTERRUPT_CLEAR)[0][0], 1);
    assert_eq!(p.last(uld::VHV_CONFIG__TIMEOUT_MACROP_LOOP_BOUND)[0], 0x09);
    assert_eq!(p.last(0x000B)[0], 0x00);
    assert_eq!(word(&p.last(0x0024)), 0x0500);
    assert_eq!(drv.get_range_timing().unwrap().1, 0); // 50 ms continuous by default
}

#[test]
fn range_timing_writes_what_decodes_back() {
    let p = FakePlatform::new();
    let drv = driver(&p);
    drv.set_range_timing(50, 0).unwrap();
    let (a, b, _) = uld::range_timing_registers(50, 0, OSC_FREQUENCY_TYPICAL, 0).unwrap();
    assert_eq!(word(&p.last(uld::RANGE_CONFIG_A)), a);
    assert_eq!(word(&p.last(uld::RANGE_CONFIG_B)), b);
    let (budget, inter) = drv.get_range_timing().unwrap();
    assert_eq!(inter, 0);
    assert!((49..=51).contains(&budget), "budget {budget}"); // the C math loses up to 1 ms on the way back
    drv.start_ranging().unwrap();
    assert_eq!(p.last(uld::SYSTEM_START)[0], 0x21); // continuous
    drv.stop_ranging().unwrap();
    assert_eq!(p.last(uld::SYSTEM_START)[0], 0x80);
}

#[test]
fn autonomous_mode_starts_with_0x40() {
    let p = FakePlatform::new();
    let drv = driver(&p);
    drv.set_range_timing(50, 1000).unwrap();
    assert!(drv.get_range_timing().unwrap().1 > 0, "autonomous");
    drv.start_ranging().unwrap();
    assert_eq!(p.last(uld::SYSTEM_START)[0], 0x40);
}

#[test]
fn range_timing_bounds_and_settings_round_trip() {
    let p = FakePlatform::new();
    let drv = driver(&p);
    assert!(matches!(drv.set_range_timing(9, 0), Err(DepzError::Sensor(_))));
    assert!(matches!(drv.set_range_timing(201, 0), Err(DepzError::Sensor(_))));
    assert!(matches!(drv.set_range_timing(50, 30), Err(DepzError::Sensor(_)))); // 0 < inter <= budget
    for mm in [10, -10, 0, 255] {
        drv.set_offset(mm).unwrap();
        assert_eq!(drv.get_offset().unwrap(), mm);
    }
    drv.set_xtalk(20).unwrap();
    assert_eq!(drv.get_xtalk().unwrap(), 20);
    drv.set_detection_thresholds(100, 300, uld::WINDOW_IN).unwrap();
    assert_eq!(
        drv.get_detection_thresholds().unwrap(),
        DetectionThresholds { distance_low_mm: 100, distance_high_mm: 300, window: uld::WINDOW_IN }
    );
    drv.set_signal_threshold(1024).unwrap();
    assert_eq!(drv.get_signal_threshold().unwrap(), 1024);
    drv.set_sigma_threshold(15).unwrap();
    assert_eq!(drv.get_sigma_threshold().unwrap(), 15);
    assert!(drv.set_sigma_threshold(0x4000).is_err());
}

#[test]
fn is_alive_and_waits_time_out() {
    let p = FakePlatform::new();
    let drv = driver(&p);
    assert!(drv.is_alive().unwrap());
    p.seed_word(uld::IDENTIFICATION__MODEL_ID, 0x1234);
    assert!(!drv.is_alive().unwrap());
    p.reg.lock().unwrap().insert(uld::FIRMWARE__SYSTEM_STATUS, 0);
    assert!(matches!(drv.wait_boot(5), Err(DepzError::Sensor(_))));
    p.reg.lock().unwrap().insert(uld::GPIO__TIO_HV_STATUS, 1); // INT high: not at its active (low) level
    assert!(matches!(drv.wait_data_ready(5), Err(DepzError::Sensor(_))));
}

#[test]
fn offset_calibration_programs_target_minus_mean() {
    let p = FakePlatform::new();
    let drv = driver(&p);
    p.seed_result(93, 200, 256 * 40);
    assert_eq!(drv.calibrate_offset(100, 5).unwrap(), 7);
    assert_eq!(word(&p.last(uld::RANGE_OFFSET_MM)), uld::try_offset_raw(7).unwrap());
    assert!(matches!(drv.calibrate_offset(5, 20), Err(DepzError::Sensor(_))));
    assert!(matches!(drv.calibrate_offset(100, 4), Err(DepzError::Sensor(_))));
}

#[test]
fn xtalk_calibration_follows_the_st_formula() {
    let p = FakePlatform::new();
    let drv = driver(&p);
    // signal 200*8 = 1600 kcps over 40 SPADs; distance 90 at a 100 mm target: (1 - 90/100) * (1600 / 40) = 4.0 kcps.
    p.seed_result(90, 200, 256 * 40);
    assert_eq!(drv.calibrate_xtalk(100, 5).unwrap(), 4);
    // In binary floating point the product is 3.9999…, and the register takes the truncation.
    assert_eq!(word(&p.last(uld::XTALK_PLANE_OFFSET_KCPS)), 2047);
    p.seed_result(10, 0xFFFF, 256);
    assert!(matches!(drv.calibrate_xtalk(1000, 5), Err(DepzError::Sensor(_)))); // > 127 kcps
}

fn reg_board() -> FakeBoard {
    let board = FakeBoard::new();
    let b = board.clone();
    board.on_packet(move |pkt| {
        if pkt.cmd != Vl53l4Cmd::ReadReg as u8 {
            return vec![FakeBoard::status(pkt.cmd, Status::Ok as u8)];
        }
        let len = u16::from_le_bytes([pkt.payload[2], pkt.payload[3]]) as usize;
        let mut reply = vec![0u8; 9 + len]; // echoed cmd, timestamp, data (all zero: continuous mode)
        reply[0] = Vl53l4Cmd::ReadReg as u8;
        vec![b.reply(Vl53l4Rpt::RegData as u8, &reply)]
    });
    board
}

#[test]
fn configuration_while_ranging_is_refused() {
    let dev = Vl53l4cdDevice::attach(DepzDevice::new(reg_board(), Some(Duration::from_millis(500))));
    dev.start_ranging().unwrap();
    assert!(dev.ranging());
    assert!(matches!(dev.set_offset_mm(3), Err(DepzError::InvalidState(_))));
    assert!(matches!(dev.init(None), Err(DepzError::InvalidState(_))));
    assert!(matches!(dev.measure_once(None), Err(DepzError::InvalidState(_))));
    dev.stop_ranging().unwrap();
    assert!(!dev.ranging());
    dev.stop_ranging().unwrap(); // a no-op once stopped
}

fn stream_payload(data: &[u8], len_field: u16) -> Vec<u8> {
    let mut p = Vec::new();
    p.extend_from_slice(&123456u64.to_le_bytes());
    p.extend_from_slice(&uld::RESULT_BLOCK_ADDR.to_le_bytes());
    p.extend_from_slice(&len_field.to_le_bytes());
    p.extend_from_slice(data);
    p
}

#[test]
fn stream_reports_decode_and_short_ones_are_counted() {
    let board = FakeBoard::new();
    let dev = Vl53l4cdDevice::attach(DepzDevice::new(board.clone(), None));
    let q = dev.measurements(64);
    let mut block = [0u8; 17];
    block[0] = valid_raw();
    block[13] = 0x01; // distance 0x0102 = 258 mm
    block[14] = 0x02;
    board.inject(board.reply(Vl53l4Rpt::Stream as u8, &stream_payload(&block, 17)));
    board.inject(board.reply(Vl53l4Rpt::Stream as u8, &stream_payload(&block[..8], 17)));
    let m = q.take(Duration::from_secs(1)).unwrap();
    assert_eq!((m.timestamp_us, m.distance_mm, m.range_status), (123456, 258, 0));
    assert!(m.valid());
    assert!(until(|| dev.stream_parse_errors() == 1, 1000));
    assert_eq!(q.len(), 0);
}
