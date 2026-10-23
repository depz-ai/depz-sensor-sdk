//! The VL53L8 / L5 / L7 device classes over a fake register bridge: the host-side guards (the
//! scenarios replay the full sessions).
mod common;

use std::sync::{Arc, Mutex};
use std::time::Duration;

use common::fake_board::FakeBoard;
use depz_sensor_sdk::devices::{DepzDevice, DepzError, Vl53l5cxDevice, Vl53l7cxDevice, Vl53l8chDevice, Vl53l8cxDevice};
use depz_sensor_sdk::protocol::common::Status;
use depz_sensor_sdk::vl53l7::{Vl53l7Cmd, Vl53l7Rpt, PIN_LPN_OFF};
use depz_sensor_sdk::vl53l8::driver::Vl53l8CnhSetup;
use depz_sensor_sdk::vl53l8::{Vl53l8Cmd, Vl53l8Rpt};

/// A bridge whose 64 KB register bank answers every read; the page register 0x7FFF is ordinary.
fn board(bank: Arc<Mutex<Vec<u8>>>) -> FakeBoard {
    let link = FakeBoard::new();
    let l = link.clone();
    link.on_packet(move |p| {
        let mut regs = bank.lock().unwrap();
        if p.cmd == Vl53l8Cmd::ReadReg as u8 {
            let addr = u16::from_le_bytes([p.payload[0], p.payload[1]]) as usize;
            let n = u16::from_le_bytes([p.payload[2], p.payload[3]]) as usize;
            let mut reply = vec![0u8; 9 + n];
            reply[0] = p.cmd;
            reply[9..].copy_from_slice(&regs[addr..addr + n]);
            return vec![l.reply(Vl53l8Rpt::RegData as u8, &reply)];
        }
        if p.cmd == Vl53l8Cmd::WriteReg as u8 {
            let addr = u16::from_le_bytes([p.payload[0], p.payload[1]]) as usize;
            let data = &p.payload[2..];
            regs[addr..addr + data.len()].copy_from_slice(data);
        }
        if p.cmd == Vl53l7Cmd::GetInfo as u8 {
            let mut info = vec![0u8; 20];
            info[15..17].copy_from_slice(&400u16.to_le_bytes());
            return vec![l.reply(Vl53l7Rpt::Info as u8, &info)];
        }
        vec![FakeBoard::status(p.cmd, Status::Ok as u8)]
    });
    link
}

fn dev(link: &FakeBoard) -> DepzDevice {
    DepzDevice::new(link.clone(), Some(Duration::from_millis(500)))
}

fn bank() -> Arc<Mutex<Vec<u8>>> {
    Arc::new(Mutex::new(vec![0u8; 0x10000]))
}

#[test]
fn configuration_before_init_is_refused_without_a_request() {
    let link = board(bank());
    let d = Vl53l8cxDevice::attach(dev(&link));
    assert!(!d.initialized());
    for r in [d.get_resolution().map(|_| ()), d.set_ranging_frequency_hz(15), d.start_ranging()] {
        assert!(matches!(r, Err(DepzError::InvalidState(_))), "{r:?}");
    }
    assert!(d.stop_ranging().is_ok(), "not ranging: nothing to stop");
    assert_eq!(link.sent_count(), 0);
}

#[test]
fn arguments_are_checked_before_the_driver() {
    let link = board(bank());
    let d = Vl53l8cxDevice::attach(dev(&link));
    assert!(matches!(d.set_resolution(32), Err(DepzError::InvalidArgument(_))));
    // Below 2 Hz the L8 never ranges; the L5/L7 go down to 1 Hz (and then need init).
    assert!(matches!(d.set_ranging_frequency_hz(1), Err(DepzError::InvalidArgument(_))));
    let l7 = Vl53l7cxDevice::attach(dev(&link));
    assert!(matches!(l7.set_ranging_frequency_hz(1), Err(DepzError::InvalidState(_))));
    assert!(matches!(l7.set_ranging_frequency_hz(0), Err(DepzError::InvalidArgument(_))));
    assert!(matches!(l7.set_i2c_speed_khz(0), Err(DepzError::InvalidArgument(_))));
    assert_eq!(link.sent_count(), 0);
}

#[test]
fn a_blank_cnh_setup_is_refused_before_anything_is_sent() {
    let link = board(bank());
    let d = Vl53l8chDevice::attach(dev(&link));
    let r = d.configure_cnh(&Vl53l8CnhSetup::new());
    assert!(matches!(r, Err(DepzError::InvalidArgument(_))), "{r:?}");
    assert!(d.cnh_setup().is_none());
    assert_eq!(link.sent_count(), 0);
}

#[test]
fn is_alive_reads_the_silicon_ids_of_the_class() {
    // Each device owns its link (dropping one closes it): a fresh board per probe.
    let alive = |id: u8, rev: u8, attach: fn(DepzDevice) -> bool| {
        let regs = bank();
        regs.lock().unwrap()[..2].copy_from_slice(&[id, rev]);
        attach(dev(&board(regs)))
    };
    let l8 = |d: DepzDevice| Vl53l8cxDevice::attach(d).is_alive();
    let l7 = |d: DepzDevice| Vl53l7cxDevice::attach(d).is_alive();
    let l5 = |d: DepzDevice| Vl53l5cxDevice::attach(d).is_alive();
    assert!(alive(0xF0, 0x0C, l8));
    // The L8 ids are not an L5/L7 over I2C (revision 0x02, or 0x01 with id 0xF0), nor the reverse.
    assert!(!alive(0xF0, 0x0C, l7));
    assert!(alive(0xF0, 0x02, l5));
    assert!(alive(0xF0, 0x01, l7));
    assert!(!alive(0xF0, 0x02, l8));
}

#[test]
fn the_board_commands_of_the_i2c_bridge() {
    let link = board(bank());
    let d = Vl53l7cxDevice::attach(dev(&link));
    assert_eq!(d.get_bridge_info().unwrap().i2c_khz, 400);
    assert_eq!(d.set_i2c_speed_khz(400).unwrap(), 400);
    d.pin_ctrl(PIN_LPN_OFF).unwrap();
    assert!(!d.initialized());
    assert_eq!(d.module_type(), None);
    let sent: Vec<u8> = link.sent_snapshot().iter().map(|p| p.cmd).collect();
    assert_eq!(
        sent,
        vec![Vl53l7Cmd::GetInfo as u8, Vl53l7Cmd::SetI2cSpeed as u8, Vl53l7Cmd::GetInfo as u8, Vl53l7Cmd::PinCtrl as u8]
    );
}
