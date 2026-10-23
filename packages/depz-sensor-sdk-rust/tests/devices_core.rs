//! The device core over a fake board: requests, errors, events, streams, record / replay.
mod common;

use std::sync::{Arc, Mutex};
use std::time::Duration;

use common::fake_board::{until, FakeBoard};
use depz_sensor_sdk::devices::{
    open_on_link, AnyDevice, DepzDevice, DepzError, DeviceEvent, RecordingLink, ReplayLink, Sr04Device,
};
use depz_sensor_sdk::framing::CrcType;
use depz_sensor_sdk::json::Value;
use depz_sensor_sdk::protocol::common::{Cmd, Rpt, Status};
use depz_sensor_sdk::protocol::sr04::{Sr04Cmd, Sr04Rpt};

const SHORT: Option<Duration> = Some(Duration::from_millis(100));

fn text(cmd: u8, s: &str) -> Vec<u8> {
    let mut out = vec![cmd];
    out.extend_from_slice(s.as_bytes());
    out.push(0);
    out.push(0xFF);
    out
}

#[test]
fn text_request_strips_trailing_padding() {
    let board = FakeBoard::new();
    let b = board.clone();
    board.on_packet(move |p| vec![b.reply(Rpt::Text as u8, &text(p.cmd, "APP_usonic_SR04_v0.97"))]);
    let dev = DepzDevice::new(board.clone(), None);
    assert_eq!(dev.get_software_name().unwrap(), "APP_usonic_SR04_v0.97");
    assert_eq!(dev.identify().unwrap().sensor_type.unwrap().as_str(), "sr04");
    assert_eq!(dev.stats().tx_packets, 2);
}

#[test]
fn no_reply_times_out_naming_the_opcode() {
    let board = FakeBoard::new();
    let dev = DepzDevice::new(board, SHORT);
    let e = dev.get_serial_number().unwrap_err();
    assert!(matches!(e, DepzError::Timeout { cmd: Some(c), .. } if c == Cmd::GetSerial as u8), "{e:?}");
    assert!(e.to_string().contains("0x05"), "{e}");
}

#[test]
fn non_ok_status_fails_with_typed_errors() {
    let board = FakeBoard::new();
    board.on_packet(|p| {
        vec![FakeBoard::status(
            p.cmd,
            if p.cmd == Cmd::SetPayloadCrcType as u8 { Status::ErrBusy as u8 } else { Status::ErrCmdNotSupported as u8 },
        )]
    });
    let dev = DepzDevice::new(board, SHORT);
    assert!(dev.set_report_payload_crc(CrcType::Crc8).unwrap_err().is_busy());
    let e = dev.get_device_name().unwrap_err();
    assert!(matches!(e, DepzError::Status { status: 0x07, .. }), "{e:?}");
    assert_eq!(e.kind(), Some("not_supported"));
}

#[test]
fn one_request_per_opcode_in_flight() {
    let board = FakeBoard::new();
    let dev = Arc::new(DepzDevice::new(board.clone(), Some(Duration::from_secs(1))));
    let d = dev.clone();
    let first = std::thread::spawn(move || d.get_serial_number());
    assert!(until(|| board.sent_count() == 1, 1000));
    assert!(dev.get_serial_number().unwrap_err().is_busy());
    dev.close();
    let r = first.join().unwrap();
    assert!(matches!(r, Err(DepzError::LinkClosed(_))), "closing fails the waiting request: {r:?}");
}

#[test]
fn ok_ack_before_the_data_report_keeps_waiting() {
    let board = FakeBoard::new();
    let b = board.clone();
    board.on_packet(move |p| vec![FakeBoard::status(p.cmd, Status::Ok as u8), b.reply(Rpt::PayloadCrcType as u8, &[2])]);
    let dev = DepzDevice::new(board, SHORT);
    assert_eq!(dev.get_report_payload_crc().unwrap(), CrcType::Crc16);
}

#[test]
fn short_reply_fails_its_request_and_the_reader_goes_on() {
    let board = FakeBoard::new();
    let b = board.clone();
    board.on_packet(move |p| {
        if p.cmd == Cmd::GetMcuTemperature as u8 {
            vec![b.reply(Rpt::Temperature as u8, &[0; 3])]
        } else {
            vec![b.reply(Rpt::Text as u8, &text(p.cmd, "x"))]
        }
    });
    let dev = DepzDevice::new(board, SHORT);
    let e = dev.read_mcu_temperature().unwrap_err();
    assert!(e.to_string().contains("does not decode"), "{e}");
    assert_eq!(dev.get_device_name().unwrap(), "x");
}

#[test]
fn arguments_are_checked_before_anything_is_sent() {
    let board = FakeBoard::new();
    let dev = Sr04Device::attach(DepzDevice::new(board.clone(), SHORT));
    assert!(matches!(dev.get_sync_pin(0), Err(DepzError::InvalidArgument(_))));
    assert_eq!(board.sent_count(), 0);
}

#[test]
fn unsolicited_reports_become_events() {
    let board = FakeBoard::new();
    let dev = DepzDevice::new(board.clone(), None);
    let events = dev.events(256);
    board.inject(FakeBoard::status(0, Status::ErrHardwareFault as u8));
    board.inject(board.reply(Rpt::SequenceError as u8, &[7, 9]));
    board.inject(board.reply(0xC5, &[0xAB]));
    let t = Duration::from_secs(1);
    assert_eq!(events.take(t).unwrap(), DeviceEvent::UnsolicitedStatus(Status::ErrHardwareFault as u8));
    assert_eq!(
        events.take(t).unwrap(),
        DeviceEvent::SequenceError { expected_seq: 7, received_seq: 9, reported_by_device: true }
    );
    assert_eq!(events.take(t).unwrap(), DeviceEvent::Text { cmd: 0xC5, text: "ab".into() });
    assert_eq!(dev.stats().device_seq_errors, 1);
}

#[test]
fn unplug_fails_pending_and_ends_streams() {
    let board = FakeBoard::new();
    let dev = Arc::new(Sr04Device::attach(DepzDevice::new(board.clone(), Some(Duration::from_secs(5)))));
    let events = dev.events(256);
    let lp = dev.measurements(256);
    let d = dev.clone();
    let waiting = std::thread::spawn(move || d.get_sample_period_us());
    assert!(until(|| board.sent_count() == 1, 1000));
    board.unplug();
    assert!(matches!(waiting.join().unwrap(), Err(DepzError::DeviceLost(_))), "the pending request did not fail");
    assert!(matches!(events.take(Duration::from_secs(1)).unwrap(), DeviceEvent::Disconnected(_)));
    assert!(matches!(lp.take(Duration::from_secs(1)), Err(DepzError::DeviceLost(_))));
    assert!(lp.is_closed());
}

#[test]
fn queues_drop_the_oldest_and_count() {
    let board = FakeBoard::new();
    let dev = Sr04Device::attach(DepzDevice::new(board.clone(), None));
    let seen = Arc::new(Mutex::new(Vec::new()));
    let s = seen.clone();
    let cb = dev.on_measurement(move |m| s.lock().unwrap().push(m.echo_time_us));
    let q = dev.measurements(2);
    for i in 1..=5u8 {
        let mut payload = [0u8; 11];
        payload[0] = Sr04Cmd::StartMeasurementLoop as u8;
        payload[9] = i;
        board.inject(board.reply(Sr04Rpt::Data as u8, &payload));
    }
    assert!(until(|| seen.lock().unwrap().len() == 5, 2000));
    assert_eq!(q.dropped_count(), 3);
    assert_eq!(dev.stream_dropped_counts(), vec![3]);
    assert_eq!(q.drain().iter().map(|m| m.echo_time_us).collect::<Vec<_>>(), vec![4, 5]);
    assert_eq!(*seen.lock().unwrap(), vec![1, 2, 3, 4, 5]);
    assert!(q.try_take().is_none());
    cb.cancel();
}

#[test]
fn recording_replays_byte_for_byte() {
    let dir = std::env::temp_dir().join(format!("depz-rec-{}", std::process::id()));
    std::fs::create_dir_all(&dir).unwrap();
    let path = dir.join("t.depzrec");
    {
        let board = FakeBoard::new();
        let b = board.clone();
        board.on_packet(move |p| vec![b.reply(Rpt::Text as u8, &text(p.cmd, "APP_usonic_SR04_v0.97"))]);
        let link = RecordingLink::create(board, &path, vec![("note".into(), Value::String("test".into()))]).unwrap();
        let rec = DepzDevice::new(link, None);
        assert_eq!(rec.get_software_name().unwrap(), "APP_usonic_SR04_v0.97");
    }
    let replay = Arc::new(ReplayLink::open(&path, true, false).unwrap());
    assert_eq!(replay.header().get("note").and_then(Value::as_str), Some("test"));
    let dev = DepzDevice::new(replay.clone(), None);
    assert_eq!(dev.get_software_name().unwrap(), "APP_usonic_SR04_v0.97");
    assert!(replay.rx_exhausted());
    assert_eq!(replay.tx_remaining(), 0);
    assert!(matches!(dev.send(Cmd::GetSerial as u8, &[]), Err(DepzError::ReplayMismatch(_))));
    drop(dev);
    let _ = std::fs::remove_dir_all(&dir);
}

#[test]
fn open_on_link_resolves_and_attaches_in_place() {
    let board = FakeBoard::new();
    let b = board.clone();
    board.on_packet(move |p| {
        if p.cmd == Cmd::GetNameActiveSoftware as u8 {
            vec![b.reply(Rpt::Text as u8, &text(p.cmd, "APP_usonic_SR04_v0.97"))]
        } else if p.cmd == Cmd::GetDeviceName as u8 {
            vec![FakeBoard::status(p.cmd, Status::ErrCmdNotSupported as u8)]
        } else if p.cmd == Sr04Cmd::GetEchoDecay as u8 {
            vec![b.reply(Sr04Rpt::EchoDecay as u8, &[0x88, 0x13])]
        } else {
            vec![]
        }
    });
    let dev = open_on_link(board.clone(), None, None, SHORT).unwrap();
    let AnyDevice::Sr04(sr04) = dev else { panic!("resolved {}", dev.class_name()) };
    assert_eq!(sr04.get_echo_decay_us().unwrap(), 5000);
    assert_eq!(board.sent_snapshot().iter().map(|p| p.seq).collect::<Vec<_>>(), vec![0, 1, 2, 3]);
}

#[test]
fn open_on_link_refuses_bootloader_and_silence() {
    let boot = FakeBoard::new();
    let b = boot.clone();
    boot.on_packet(move |p| vec![b.reply(Rpt::Text as u8, &text(p.cmd, "BOOTDEPZ_v1.11"))]);
    assert!(matches!(open_on_link(boot.clone(), None, None, SHORT), Err(DepzError::BootloaderMode)));
    assert!(boot.is_closed_for_test());
    let silent = FakeBoard::new();
    assert!(matches!(open_on_link(silent.clone(), None, None, SHORT), Err(DepzError::NoDevice(_))));
    assert!(silent.is_closed_for_test());
}
