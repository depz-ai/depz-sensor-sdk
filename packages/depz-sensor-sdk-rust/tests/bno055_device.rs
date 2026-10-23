//! The BNO055 device class over a fake register bridge.
mod common;

use std::sync::{Arc, Mutex};
use std::time::Duration;

use common::fake_board::FakeBoard;
use depz_sensor_sdk::bno055::regs::{self as r, CalibrationProfile, OprMode, PwrMode, Units};
use depz_sensor_sdk::bno055::{AccelConfig, AxisRemap, Bno055Cmd, Bno055Rpt};
use depz_sensor_sdk::devices::{Bno055Device, DepzDevice, DepzError};
use depz_sensor_sdk::protocol::common::Status;

/// A BNO055 behind its register bridge: two register pages, a fused quaternion that is non-zero in
/// fusion modes, and every register write recorded (page, register, value).
struct Board {
    link: FakeBoard,
    state: Arc<Mutex<Regs>>,
}

struct Regs {
    pages: [[u8; 256]; 2],
    writes: Vec<(u8, u8, u8)>,
    page: usize,
}

impl Board {
    fn new() -> Board {
        let link = FakeBoard::new();
        let state = Arc::new(Mutex::new(Regs { pages: [[0; 256]; 2], writes: Vec::new(), page: 0 }));
        let (l, s) = (link.clone(), state.clone());
        link.on_packet(move |p| {
            let mut st = s.lock().unwrap();
            if p.cmd == Bno055Cmd::ReadReg as u8 {
                let (addr, n) = (p.payload[0] as usize, p.payload[1] as usize);
                let page = st.page;
                if page == 0 {
                    let fusion = OprMode::from_reg(st.pages[0][r::REG_OPR_MODE as usize]).is_some_and(|m| m.is_fusion());
                    st.pages[0][r::REG_QUA_DATA as usize] = if fusion { 0x10 } else { 0 };
                }
                let mut reply = vec![0u8; 9 + n];
                reply[0] = Bno055Cmd::ReadReg as u8;
                reply[9..].copy_from_slice(&st.pages[page][addr..addr + n]);
                return vec![l.reply(Bno055Rpt::RegData as u8, &reply)];
            }
            if p.cmd == Bno055Cmd::WriteReg as u8 {
                let addr = p.payload[0];
                for (i, b) in p.payload[1..].iter().enumerate() {
                    let reg = addr + i as u8;
                    let page = st.page as u8;
                    st.writes.push((page, reg, *b));
                    if reg == r::REG_PAGE_ID {
                        st.page = *b as usize;
                    } else {
                        let pg = st.page;
                        st.pages[pg][reg as usize] = *b;
                    }
                }
            }
            vec![FakeBoard::status(p.cmd, Status::Ok as u8)]
        });
        Board { link, state }
    }

    fn written(&self, reg: u8, page: u8) -> Vec<u8> {
        self.state.lock().unwrap().writes.iter().filter(|w| w.1 == reg && w.0 == page).map(|w| w.2).collect()
    }

    fn clear(&self) {
        self.state.lock().unwrap().writes.clear();
    }

    fn set(&self, page: usize, reg: u8, v: u8) {
        self.state.lock().unwrap().pages[page][reg as usize] = v;
    }

    fn open(&self) -> Bno055Device {
        Bno055Device::attach(DepzDevice::new(self.link.clone(), Some(Duration::from_millis(500))))
    }
}

#[test]
fn a_switch_between_two_operating_modes_goes_through_config() {
    let b = Board::new();
    let dev = b.open();
    dev.set_operation_mode(OprMode::Ndof).unwrap();
    dev.set_operation_mode(OprMode::Amg).unwrap();
    assert_eq!(b.written(r::REG_OPR_MODE, 0), vec![OprMode::Ndof as u8, 0, OprMode::Amg as u8]);
    dev.set_operation_mode(OprMode::Amg).unwrap(); // already there: nothing written
    assert_eq!(b.written(r::REG_OPR_MODE, 0).len(), 3);
    assert_eq!(dev.get_operation_mode().unwrap(), OprMode::Amg);
}

#[test]
fn suspend_keeps_the_mode_and_wake_up_puts_it_back() {
    let b = Board::new();
    let dev = b.open();
    dev.configure(OprMode::Ndof, None, None, None).unwrap();
    b.clear();
    dev.set_power_mode(PwrMode::Suspend).unwrap();
    assert_eq!(b.written(r::REG_OPR_MODE, 0), vec![0]);
    assert_eq!(b.written(r::REG_PWR_MODE, 0), vec![PwrMode::Suspend as u8]);
    b.clear();
    dev.set_power_mode(PwrMode::Normal).unwrap();
    assert_eq!(b.written(r::REG_PWR_MODE, 0), vec![PwrMode::Normal as u8]);
    assert_eq!(b.written(r::REG_OPR_MODE, 0), vec![OprMode::Ndof as u8]);
}

#[test]
fn a_chip_found_asleep_wakes_into_ndof() {
    let b = Board::new();
    let dev = b.open();
    b.set(0, r::REG_PWR_MODE, PwrMode::Suspend as u8); // CONFIG + suspended at connect
    dev.set_power_mode(PwrMode::Normal).unwrap();
    assert_eq!(b.written(r::REG_OPR_MODE, 0), vec![OprMode::Ndof as u8]);
}

#[test]
fn configure_writes_units_remap_and_profile_in_config() {
    let b = Board::new();
    let dev = b.open();
    let units = Units { accel_mg: true, temp_f: true, ..Units::default() };
    let profile = CalibrationProfile {
        accel_offset: [1, 2, 3],
        mag_offset: [4, 5, 6],
        gyro_offset: [7, 8, 9],
        accel_radius: 1000,
        mag_radius: 480,
    };
    dev.configure(OprMode::Ndof, Some(units), AxisRemap::placement("P2"), Some(profile)).unwrap();
    assert_eq!(b.written(r::REG_OPR_MODE, 0), vec![0, OprMode::Ndof as u8]);
    assert_eq!(b.written(r::REG_UNIT_SEL, 0), vec![units.pack()]);
    let start = r::REG_CALIB_PROFILE as usize;
    assert_eq!(b.state.lock().unwrap().pages[0][start..start + r::CALIB_PROFILE_LEN], profile.pack());
    assert_eq!(dev.get_units().unwrap(), units);
    b.clear();
    dev.restore_configuration().unwrap();
    assert_eq!(b.written(r::REG_UNIT_SEL, 0), vec![units.pack()]);
}

#[test]
fn page1_is_entered_and_left_and_refused_while_streaming() {
    let b = Board::new();
    let dev = b.open();
    let cfg = AccelConfig { range: 2, bandwidth: 3, power: 0 };
    dev.set_accel_config(&cfg).unwrap();
    let page_writes: Vec<u8> =
        b.state.lock().unwrap().writes.iter().filter(|w| w.1 == r::REG_PAGE_ID).map(|w| w.2).collect();
    assert_eq!(page_writes, vec![1, 0]);
    assert_eq!(b.state.lock().unwrap().pages[1][r::REG1_ACC_CONFIG as usize], cfg.pack());
    assert_eq!(dev.get_accel_config().unwrap(), cfg);
    dev.start_stream(10, None, false).unwrap();
    assert!(matches!(dev.unique_id(), Err(DepzError::InvalidState(_))));
    assert!(matches!(dev.set_interrupt_enable(1), Err(DepzError::InvalidState(_))));
    dev.stop_stream().unwrap();
    assert_eq!(dev.unique_id().unwrap().len(), 16);
}

#[test]
fn self_test_runs_in_config_and_restores_the_mode() {
    let b = Board::new();
    let dev = b.open();
    dev.set_operation_mode(OprMode::Ndof).unwrap();
    b.set(0, r::REG_ST_RESULT, r::EXPECTED_SELF_TEST);
    b.clear();
    assert!(dev.self_test().unwrap().self_test_passed());
    assert_eq!(b.written(r::REG_SYS_TRIGGER, 0), vec![r::SYS_TRIGGER_SELF_TEST]);
    assert_eq!(b.written(r::REG_OPR_MODE, 0), vec![0, OprMode::Ndof as u8]);
}

#[test]
fn arguments_outside_their_fields_are_refused_before_sending() {
    let b = Board::new();
    let dev = b.open();
    assert!(matches!(dev.set_interrupt_setting(0x10, 1), Err(DepzError::InvalidArgument(_))));
    assert!(matches!(dev.start_stream(10, Some((0xF0, 0x20)), false), Err(DepzError::InvalidArgument(_))));
    assert!(matches!(dev.read_registers(0, 1, 2), Err(DepzError::InvalidArgument(_))));
    assert_eq!(b.link.sent_count(), 0);
}

#[test]
fn samples_decode_with_the_units_of_the_stream_start() {
    let b = Board::new();
    let dev = b.open();
    let q = dev.samples(64);
    dev.set_units(Units { accel_mg: true, ..Units::default() }).unwrap();
    dev.start_stream(20, Some((r::REG_ACC_DATA, 6)), false).unwrap();
    let mut payload = vec![0u8; 16];
    payload[0] = 77;
    payload[8] = r::REG_ACC_DATA;
    payload[9] = 6;
    payload[10..12].copy_from_slice(&1000i16.to_le_bytes());
    b.link.inject(b.link.reply(Bno055Rpt::Stream as u8, &payload));
    let s = q.take(Duration::from_secs(1)).unwrap();
    assert_eq!(s.accel.unwrap()[0], 1000.0); // mg: LSB 1
    assert!(s.quaternion.is_none());
    assert_eq!(s.timestamp_us, 77);
}
