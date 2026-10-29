//! The BNO055 device class over a fake register bridge.
mod common;

use std::sync::{Arc, Mutex};
use std::time::Duration;

use common::fake_board::FakeBoard;
use depz_sensor_sdk::bno055::regs::{self as r, CalibrationProfile, OprMode, PwrMode, Units};
use depz_sensor_sdk::bno055::{
    AccelConfig, AxisRemap, Bno055Cmd, Bno055Rpt, CalibStatus, GyroConfig, MagConfig, TempSource,
    EXPECTED_ACC_ID, EXPECTED_CHIP_ID, EXPECTED_GYR_ID, EXPECTED_MAG_ID, INFO_SIZE, SIC_IDENTITY,
};
use depz_sensor_sdk::devices::Bno055SystemStatus;
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
    info: Vec<u8>,
    resets: u32,
}

impl Board {
    fn new() -> Board {
        let link = FakeBoard::new();
        let state = Arc::new(Mutex::new(Regs {
            pages: [[0; 256]; 2],
            writes: Vec::new(),
            page: 0,
            info: vec![0; INFO_SIZE],
            resets: 0,
        }));
        let (l, s) = (link.clone(), state.clone());
        link.on_packet(move |p| {
            let mut st = s.lock().unwrap();
            if p.cmd == Bno055Cmd::ReadReg as u8 {
                let (addr, n) = (p.payload[0] as usize, p.payload[1] as usize);
                let page = st.page;
                if page == 0 {
                    let fusion = OprMode::from_reg(st.pages[0][r::REG_OPR_MODE as usize])
                        .is_some_and(|m| m.is_fusion());
                    st.pages[0][r::REG_QUA_DATA as usize] = if fusion { 0x10 } else { 0 };
                }
                let mut reply = vec![0u8; 9 + n];
                reply[0] = Bno055Cmd::ReadReg as u8;
                reply[9..].copy_from_slice(&st.pages[page][addr..addr + n]);
                return vec![l.reply(Bno055Rpt::RegData as u8, &reply)];
            }
            if p.cmd == Bno055Cmd::GetInfo as u8 {
                return vec![l.reply(Bno055Rpt::Info as u8, &st.info)];
            }
            if p.cmd == Bno055Cmd::Reset as u8 {
                // nRESET: the sensor comes back in CONFIG on page 0
                st.resets += 1;
                st.page = 0;
                st.pages[0][r::REG_OPR_MODE as usize] = 0;
                st.pages[0][r::REG_UNIT_SEL as usize] = 0;
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
                        // a self-test leftover (SYS_STATUS 4) clears only when the mode leaves
                        // CONFIG (contract 13 §5)
                        if st.page == 0 && reg == r::REG_OPR_MODE && *b != 0 {
                            let sys = &mut st.pages[0][r::REG_SYS_STATUS as usize];
                            if *sys == 4 {
                                *sys = 0;
                            }
                        }
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
        self.state
            .lock()
            .unwrap()
            .writes
            .iter()
            .filter(|w| w.1 == reg && w.0 == page)
            .map(|w| w.2)
            .collect()
    }

    fn clear(&self) {
        self.state.lock().unwrap().writes.clear();
    }

    fn set(&self, page: usize, reg: u8, v: u8) {
        self.state.lock().unwrap().pages[page][reg as usize] = v;
    }

    fn open(&self) -> Bno055Device {
        Bno055Device::attach(DepzDevice::new(
            self.link.clone(),
            Some(Duration::from_millis(500)),
        ))
    }
}

#[test]
fn a_switch_between_two_operating_modes_goes_through_config() {
    let b = Board::new();
    let dev = b.open();
    dev.set_operation_mode(OprMode::Ndof).unwrap();
    dev.set_operation_mode(OprMode::Amg).unwrap();
    assert_eq!(
        b.written(r::REG_OPR_MODE, 0),
        vec![OprMode::Ndof as u8, 0, OprMode::Amg as u8]
    );
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
    let units = Units {
        accel_mg: true,
        temp_f: true,
        ..Units::default()
    };
    let profile = CalibrationProfile {
        accel_offset: [1, 2, 3],
        mag_offset: [4, 5, 6],
        gyro_offset: [7, 8, 9],
        accel_radius: 1000,
        mag_radius: 480,
    };
    dev.configure(
        OprMode::Ndof,
        Some(units),
        AxisRemap::placement("P2"),
        Some(profile),
    )
    .unwrap();
    assert_eq!(b.written(r::REG_OPR_MODE, 0), vec![0, OprMode::Ndof as u8]);
    assert_eq!(b.written(r::REG_UNIT_SEL, 0), vec![units.pack()]);
    let start = r::REG_CALIB_PROFILE as usize;
    assert_eq!(
        b.state.lock().unwrap().pages[0][start..start + r::CALIB_PROFILE_LEN],
        profile.pack()
    );
    assert_eq!(dev.get_units().unwrap(), units);
    b.clear();
    dev.restore_configuration().unwrap();
    assert_eq!(b.written(r::REG_UNIT_SEL, 0), vec![units.pack()]);
}

#[test]
fn page1_is_entered_and_left_and_refused_while_streaming() {
    let b = Board::new();
    let dev = b.open();
    let cfg = AccelConfig {
        range: 2,
        bandwidth: 3,
        power: 0,
    };
    dev.set_accel_config(&cfg).unwrap();
    let page_writes: Vec<u8> = b
        .state
        .lock()
        .unwrap()
        .writes
        .iter()
        .filter(|w| w.1 == r::REG_PAGE_ID)
        .map(|w| w.2)
        .collect();
    assert_eq!(page_writes, vec![1, 0]);
    assert_eq!(
        b.state.lock().unwrap().pages[1][r::REG1_ACC_CONFIG as usize],
        cfg.pack()
    );
    assert_eq!(dev.get_accel_config().unwrap(), cfg);
    dev.start_stream(10, None, false).unwrap();
    assert!(matches!(dev.unique_id(), Err(DepzError::InvalidState(_))));
    assert!(matches!(
        dev.set_interrupt_enable(1),
        Err(DepzError::InvalidState(_))
    ));
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
    assert_eq!(
        b.written(r::REG_SYS_TRIGGER, 0),
        vec![r::SYS_TRIGGER_SELF_TEST]
    );
    assert_eq!(b.written(r::REG_OPR_MODE, 0), vec![0, OprMode::Ndof as u8]);
}

#[test]
fn arguments_outside_their_fields_are_refused_before_sending() {
    let b = Board::new();
    let dev = b.open();
    assert!(matches!(
        dev.set_interrupt_setting(0x10, 1),
        Err(DepzError::InvalidArgument(_))
    ));
    assert!(matches!(
        dev.start_stream(10, Some((0xF0, 0x20)), false),
        Err(DepzError::InvalidArgument(_))
    ));
    assert!(matches!(
        dev.read_registers(0, 1, 2),
        Err(DepzError::InvalidArgument(_))
    ));
    assert_eq!(b.link.sent_count(), 0);
}

#[test]
fn samples_decode_with_the_units_of_the_stream_start() {
    let b = Board::new();
    let dev = b.open();
    let q = dev.samples(64);
    dev.set_units(Units {
        accel_mg: true,
        ..Units::default()
    })
    .unwrap();
    dev.start_stream(20, Some((r::REG_ACC_DATA, 6)), false)
        .unwrap();
    let mut payload = vec![0u8; 16];
    payload[0] = 77;
    payload[8] = r::REG_ACC_DATA;
    payload[9] = 6;
    payload[10..12].copy_from_slice(&1000i16.to_le_bytes());
    b.link
        .inject(b.link.reply(Bno055Rpt::Stream as u8, &payload));
    let s = q.take(Duration::from_secs(1)).unwrap();
    assert_eq!(s.accel.unwrap()[0], 1000.0); // mg: LSB 1
    assert!(s.quaternion.is_none());
    assert_eq!(s.timestamp_us, 77);
}

#[test]
fn system_status_reads_its_four_registers_and_names_them() {
    let b = Board::new();
    let dev = b.open();
    b.set(0, r::REG_ST_RESULT, 0x0D); // gyro self-test bit cleared
    b.set(0, r::REG_SYS_CLK_STATUS, 0);
    b.set(0, r::REG_SYS_STATUS, 1);
    b.set(0, r::REG_SYS_STATUS + 1, 3);
    let s = dev.system_status().unwrap();
    assert_eq!(
        s,
        Bno055SystemStatus {
            self_test: 0x0D,
            clk_status: 0,
            status: 1,
            error: 3,
        }
    );
    assert!(!s.self_test_passed());
    assert_eq!(s.status_text(), "system error");
    assert_eq!(s.error_text(), "self-test failed");
    let named = |status: u8, error: u8| {
        let s = Bno055SystemStatus {
            self_test: r::EXPECTED_SELF_TEST,
            clk_status: 0,
            status,
            error,
        };
        (s.status_text(), s.error_text())
    };
    assert_eq!(named(0, 0).0, "idle");
    assert_eq!(named(5, 0).0, "fusion algorithm running");
    assert_eq!(named(6, 0).0, "running without fusion");
    assert_eq!(named(9, 0).0, "unknown (9)");
    assert_eq!(named(1, 9).1, "fusion algorithm configuration error");
    assert_eq!(named(1, 11).1, "unknown (11)");
}

#[test]
fn a_self_test_started_in_config_steps_through_acconly_to_clear_its_status() {
    let b = Board::new();
    let dev = b.open();
    b.set(0, r::REG_ST_RESULT, r::EXPECTED_SELF_TEST);
    b.set(0, r::REG_SYS_TRIGGER, r::SYS_TRIGGER_CLK_SEL); // external clock selected
    let s = dev.self_test().unwrap();
    assert!(s.self_test_passed());
    // CLK_SEL kept next to the trigger bit; ACCONLY → CONFIG after, the mode stays CONFIG
    assert_eq!(
        b.written(r::REG_SYS_TRIGGER, 0),
        vec![r::SYS_TRIGGER_CLK_SEL | r::SYS_TRIGGER_SELF_TEST]
    );
    assert_eq!(
        b.written(r::REG_OPR_MODE, 0),
        vec![OprMode::AccOnly as u8, 0]
    );
    dev.start_stream(10, None, false).unwrap();
    assert!(matches!(dev.self_test(), Err(DepzError::InvalidState(_))));
}

#[test]
fn a_status_stuck_at_self_test_is_cleared_before_configuring() {
    let b = Board::new();
    let dev = b.open();
    b.set(0, r::REG_SYS_STATUS, 4); // outlasts POST: the leftover of a self-test in CONFIG
    dev.configure(OprMode::Ndof, None, None, None).unwrap();
    assert_eq!(
        b.written(r::REG_OPR_MODE, 0),
        vec![OprMode::AccOnly as u8, 0, 0, OprMode::Ndof as u8]
    );
}

#[test]
fn a_sensor_that_never_leaves_its_boot_states_fails_configure() {
    let b = Board::new();
    let dev = b.open();
    b.set(0, r::REG_SYS_STATUS, 2); // initializing peripherals, for good
    match dev.configure(OprMode::Ndof, None, None, None) {
        Err(DepzError::Sensor(m)) => assert!(m.contains("did not finish booting"), "{m}"),
        other => panic!("expected a sensor error, got {other:?}"),
    }
    assert!(b.written(r::REG_OPR_MODE, 0).is_empty());
}

#[test]
fn the_calibration_profile_is_read_and_written_in_config_and_the_mode_restored() {
    let b = Board::new();
    let dev = b.open();
    dev.set_operation_mode(OprMode::Ndof).unwrap();
    let profile = CalibrationProfile {
        accel_offset: [-20, 15, 4],
        mag_offset: [100, -200, 300],
        gyro_offset: [-1, 0, 2],
        accel_radius: 1000,
        mag_radius: 700,
    };
    let start = r::REG_CALIB_PROFILE as usize;
    b.state.lock().unwrap().pages[0][start..start + r::CALIB_PROFILE_LEN]
        .copy_from_slice(&profile.pack());
    b.clear();
    assert_eq!(dev.read_calibration_profile().unwrap(), profile);
    assert_eq!(b.written(r::REG_OPR_MODE, 0), vec![0, OprMode::Ndof as u8]);

    let stored = CalibrationProfile {
        accel_radius: 999,
        ..profile
    };
    b.clear();
    dev.write_calibration_profile(&stored).unwrap();
    // all 22 bytes in one WRITE_REG (the sensor latches each group on its MSB), inside CONFIG
    let writes = b.state.lock().unwrap().writes.clone();
    let profile_writes: Vec<u8> = writes
        .iter()
        .filter(|w| (start..start + r::CALIB_PROFILE_LEN).contains(&(w.1 as usize)))
        .map(|w| w.2)
        .collect();
    assert_eq!(profile_writes, stored.pack());
    let sent = b.link.sent_snapshot();
    let profile_packet = sent
        .iter()
        .find(|p| p.cmd == Bno055Cmd::WriteReg as u8 && p.payload[0] == r::REG_CALIB_PROFILE)
        .unwrap();
    assert_eq!(profile_packet.payload.len(), 1 + r::CALIB_PROFILE_LEN);
    assert_eq!(b.written(r::REG_OPR_MODE, 0), vec![0, OprMode::Ndof as u8]);

    b.set(0, r::REG_CALIB_STAT, 0b11_10_01_11);
    let c = dev.calibration_status().unwrap();
    assert_eq!(
        c,
        CalibStatus {
            system: 3,
            gyro: 2,
            accel: 1,
            mag: 3,
        }
    );
    assert!(!c.fully_calibrated());
}

#[test]
fn the_soft_iron_matrix_defaults_to_identity_and_round_trips() {
    let b = Board::new();
    let dev = b.open();
    dev.set_sic_matrix(None).unwrap();
    let start = r::REG_SIC_MATRIX as usize;
    assert_eq!(
        b.state.lock().unwrap().pages[0][start..start + 18],
        r::pack_sic_matrix(&SIC_IDENTITY)
    );
    assert_eq!(b.written(r::REG_SIC_MATRIX, 0), vec![0x00]); // 16384 = 0x4000, LSB first
    assert_eq!(b.written(r::REG_SIC_MATRIX + 1, 0), vec![0x40]);
    let m = [16000, -12, 7, 3, 16500, -9, 0, 1, -16384];
    dev.set_sic_matrix(Some(m)).unwrap();
    assert_eq!(dev.get_sic_matrix().unwrap(), m);
}

#[test]
fn gyro_and_magnetometer_configs_live_on_page1() {
    let b = Board::new();
    let dev = b.open();
    let gyro = GyroConfig {
        range: 1,
        bandwidth: 5,
        power: 2,
    };
    dev.set_gyro_config(&gyro).unwrap();
    assert_eq!(b.written(r::REG1_GYR_CONFIG_0, 1), vec![gyro.pack()[0]]);
    assert_eq!(b.written(r::REG1_GYR_CONFIG_0 + 1, 1), vec![gyro.pack()[1]]);
    assert_eq!(dev.get_gyro_config().unwrap(), gyro);

    let mag = MagConfig {
        rate: 7,
        mode: 3,
        power: 1,
    };
    dev.set_mag_config(&mag).unwrap();
    assert_eq!(b.written(r::REG1_MAG_CONFIG, 1), vec![0b0011_1111]);
    assert_eq!(dev.get_mag_config().unwrap(), mag);
    // every page-1 access left page 0 selected
    assert_eq!(b.state.lock().unwrap().page, 0);
    assert!(b.written(r::REG1_MAG_CONFIG, 0).is_empty());
}

#[test]
fn interrupt_registers_enable_mask_settings_and_reset() {
    let b = Board::new();
    let dev = b.open();
    dev.set_interrupt_enable(0x40).unwrap();
    dev.set_interrupt_mask(0x44).unwrap();
    assert_eq!(b.written(r::REG1_INT_EN, 1), vec![0x40]);
    assert_eq!(b.written(r::REG1_INT_MSK, 1), vec![0x44]);
    assert_eq!(dev.get_interrupt_enable().unwrap(), 0x40);
    assert_eq!(dev.get_interrupt_mask().unwrap(), 0x44);

    dev.set_interrupt_setting(r::REG1_ACC_AM_THRES, 0x14)
        .unwrap();
    dev.set_interrupt_setting(r::REG1_GYR_AM_SET, 0x0A).unwrap();
    assert_eq!(b.written(r::REG1_ACC_AM_THRES, 1), vec![0x14]);
    assert_eq!(b.written(r::REG1_GYR_AM_SET, 1), vec![0x0A]);
    assert!(matches!(
        dev.set_interrupt_setting(r::REG1_GYR_AM_SET + 1, 0),
        Err(DepzError::InvalidArgument(_))
    ));

    b.set(0, r::REG_INT_STA, 0x40);
    assert_eq!(dev.read_interrupt_status().unwrap(), 0x40);
    b.set(0, r::REG_SYS_TRIGGER, r::SYS_TRIGGER_CLK_SEL);
    dev.clear_interrupt().unwrap();
    assert_eq!(
        b.written(r::REG_SYS_TRIGGER, 0),
        vec![r::SYS_TRIGGER_CLK_SEL | r::SYS_TRIGGER_RST_INT]
    );
}

#[test]
fn a_polled_sample_decodes_the_full_block_and_the_quaternion() {
    let b = Board::new();
    let dev = b.open();
    let acc = r::REG_ACC_DATA as usize;
    {
        let mut st = b.state.lock().unwrap();
        st.pages[0][acc..acc + 2].copy_from_slice(&981i16.to_le_bytes());
        // w = 0x4000 = 1.0; the low byte is the fake's fusion marker (0 outside fusion)
        st.pages[0][r::REG_QUA_DATA as usize + 1] = 0x40;
        st.pages[0][r::REG_CALIB_STAT as usize] = 0xFF;
    }
    let s = dev.read_sample(None).unwrap();
    assert_eq!(s.addr, r::FULL_BLOCK.0);
    assert_eq!(s.raw.len(), r::FULL_BLOCK.1 as usize);
    assert_eq!(s.accel.unwrap(), [9.81, 0.0, 0.0]); // m/s²: LSB 100
    assert_eq!(s.quaternion.unwrap(), [1.0, 0.0, 0.0, 0.0]);
    assert!(s.calibration.unwrap().fully_calibrated());
    assert_eq!(dev.read_quaternion().unwrap(), [1.0, 0.0, 0.0, 0.0]);
    let read = b
        .link
        .sent_snapshot()
        .into_iter()
        .rfind(|p| p.cmd == Bno055Cmd::ReadReg as u8)
        .unwrap();
    assert_eq!(read.payload[..2], [r::QUAT_BLOCK.0, r::QUAT_BLOCK.1]);
}

#[test]
fn a_short_stream_report_is_counted_not_delivered() {
    let b = Board::new();
    let dev = b.open();
    dev.start_stream(10, Some((r::REG_ACC_DATA, 6)), false)
        .unwrap();
    let q = dev.samples(8);
    let mut payload = vec![0u8; 12];
    payload[8] = r::REG_ACC_DATA;
    payload[9] = 6; // claims 6 bytes, carries 2
    b.link
        .inject(b.link.reply(Bno055Rpt::Stream as u8, &payload));
    b.link
        .inject(b.link.reply(Bno055Rpt::Stream as u8, &[0u8; 5]));
    assert!(common::fake_board::until(
        || dev.stream_parse_errors() == 2,
        1000
    ));
    assert!(q.take(Duration::from_millis(50)).is_err());
    match dev.get_sample(Some(Duration::from_millis(50))) {
        Err(DepzError::Timeout { cmd, .. }) => assert_eq!(cmd, Some(Bno055Rpt::Stream as u8)),
        other => panic!("expected a timeout, got {other:?}"),
    }

    // a full pull queue drops its oldest sample and counts it (contract 07 §3)
    let one = dev.samples(1);
    for ts in [1u8, 2] {
        let mut good = vec![0u8; 16];
        good[0] = ts;
        good[8] = r::REG_ACC_DATA;
        good[9] = 6;
        b.link.inject(b.link.reply(Bno055Rpt::Stream as u8, &good));
    }
    assert!(common::fake_board::until(
        || dev.stream_dropped_counts().contains(&1),
        1000
    ));
    assert_eq!(one.take(Duration::from_secs(1)).unwrap().timestamp_us, 2);

    dev.stop_stream().unwrap();
    assert!(!dev.streaming());
    let stops = || {
        b.link
            .sent_snapshot()
            .iter()
            .filter(|p| p.cmd == Bno055Cmd::StopStream as u8)
            .count()
    };
    assert_eq!(stops(), 1);
    dev.stop_stream().unwrap(); // not streaming: nothing sent
    assert_eq!(stops(), 1);
}

#[test]
fn bridge_info_decodes_and_is_alive_needs_the_handshake_and_the_ids() {
    let b = Board::new();
    let dev = b.open();
    assert!(!dev.is_alive()); // all zero: no handshake, wrong ids
    {
        let mut st = b.state.lock().unwrap();
        st.info[0] = 0x28;
        st.info[1..5].copy_from_slice(&[
            EXPECTED_CHIP_ID,
            EXPECTED_ACC_ID,
            EXPECTED_MAG_ID,
            EXPECTED_GYR_ID,
        ]);
        st.info[5..7].copy_from_slice(&0x0311u16.to_le_bytes());
        st.info[35] = 2; // sensor_resets
    }
    let i = dev.bridge_info().unwrap();
    assert_eq!(i.i2c_addr, 0x28);
    assert_eq!(i.sw_rev_text(), "03.11");
    assert_eq!(i.sensor_resets, 2);
    assert!(!dev.is_alive()); // ids right, handshake not passed
    b.state.lock().unwrap().info[8] = 1;
    assert!(dev.is_alive());
}

#[test]
fn reset_stops_the_stream_and_forgets_the_units() {
    let b = Board::new();
    let dev = b.open();
    dev.set_units(Units {
        accel_mg: true,
        ..Units::default()
    })
    .unwrap();
    dev.start_stream(10, None, false).unwrap();
    dev.reset_sensor().unwrap();
    assert_eq!(b.state.lock().unwrap().resets, 1);
    assert!(!dev.streaming());
    // power-on units again: 1000 LSB reads as 10 m/s², no longer 1000 mg
    let acc = r::REG_ACC_DATA as usize;
    b.state.lock().unwrap().pages[0][acc..acc + 2].copy_from_slice(&1000i16.to_le_bytes());
    let s = dev.read_sample(Some((r::REG_ACC_DATA, 6))).unwrap();
    assert_eq!(s.accel.unwrap()[0], 10.0);
}

#[test]
fn restore_configuration_needs_a_configure_first() {
    let b = Board::new();
    let dev = b.open();
    assert!(matches!(
        dev.restore_configuration(),
        Err(DepzError::InvalidState(_))
    ));
    assert_eq!(b.link.sent_count(), 0);
}

#[test]
fn axis_remap_and_temperature_source_are_written_in_config() {
    let b = Board::new();
    let dev = b.open();
    dev.set_operation_mode(OprMode::Imu).unwrap();
    b.clear();
    let p6 = AxisRemap::placement("p6").unwrap();
    dev.set_axis_remap(&p6).unwrap();
    let (config, sign) = p6.pack().unwrap();
    assert_eq!(b.written(r::REG_AXIS_MAP_CONFIG, 0), vec![config]);
    assert_eq!(b.written(r::REG_AXIS_MAP_SIGN, 0), vec![sign]);
    assert_eq!(b.written(r::REG_OPR_MODE, 0), vec![0, OprMode::Imu as u8]);
    assert_eq!(dev.get_axis_remap().unwrap(), p6);

    let sent = b.link.sent_count();
    let twice = AxisRemap {
        y: r::AXIS_X,
        ..AxisRemap::default()
    };
    assert!(matches!(
        dev.set_axis_remap(&twice),
        Err(DepzError::InvalidArgument(_))
    ));
    assert_eq!(b.link.sent_count(), sent);

    assert_eq!(dev.get_temperature_source().unwrap(), TempSource::Accel);
    dev.set_temperature_source(TempSource::Gyro).unwrap();
    assert_eq!(
        b.written(r::REG_TEMP_SOURCE, 0),
        vec![TempSource::Gyro as u8]
    );
    assert_eq!(dev.get_temperature_source().unwrap(), TempSource::Gyro);
}

#[test]
fn mode_registers_outside_their_codes_are_protocol_errors() {
    let b = Board::new();
    let dev = b.open();
    b.set(0, r::REG_PWR_MODE, PwrMode::LowPower as u8);
    assert_eq!(dev.get_power_mode().unwrap(), PwrMode::LowPower);
    b.set(0, r::REG_PWR_MODE, 3);
    assert!(matches!(dev.get_power_mode(), Err(DepzError::Protocol(_))));
    b.set(0, r::REG_OPR_MODE, 0x7D); // bits 7:4 masked: 13 is unused
    assert!(matches!(
        dev.get_operation_mode(),
        Err(DepzError::Protocol(_))
    ));
}
