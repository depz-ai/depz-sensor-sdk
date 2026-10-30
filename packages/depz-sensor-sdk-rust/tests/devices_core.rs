//! The device core over a fake board: requests, errors, events, streams, record / replay.
mod common;

use std::sync::{Arc, Mutex};
use std::time::Duration;

use common::fake_board::{until, FakeBoard};
use depz_sensor_sdk::devices::{
    host_now_us, open_on_link, AnyDevice, DepzDevice, DepzError, DeviceEvent, Link, RecordingLink,
    ReplayLink, ReportHandler, Sr04Device,
};
use depz_sensor_sdk::framing::{build_packet, CrcType};
use depz_sensor_sdk::json::Value;
use depz_sensor_sdk::protocol::common::{Cmd, Rpt, Status, SyncPinConfig};
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
    assert_eq!(
        dev.identify().unwrap().sensor_type.unwrap().as_str(),
        "sr04"
    );
    assert_eq!(dev.stats().tx_packets, 2);
}

#[test]
fn to_host_time_before_sync_is_the_invalid_state_error() {
    // Contract 07 §3 (ERRATA A14): a conversion needs a prior sync_time — the SDK's own error.
    let dev = DepzDevice::new(FakeBoard::new(), SHORT);
    assert!(dev.last_time_sync().is_none());
    assert!(matches!(
        dev.to_host_time_us(0),
        Err(DepzError::InvalidState(m)) if m.contains("sync_time")
    ));
}

#[test]
fn no_reply_times_out_naming_the_opcode() {
    let board = FakeBoard::new();
    let dev = DepzDevice::new(board, SHORT);
    let e = dev.get_serial_number().unwrap_err();
    assert!(
        matches!(e, DepzError::Timeout { cmd: Some(c), .. } if c == Cmd::GetSerial as u8),
        "{e:?}"
    );
    assert!(e.to_string().contains("0x05"), "{e}");
}

#[test]
fn non_ok_status_fails_with_typed_errors() {
    let board = FakeBoard::new();
    board.on_packet(|p| {
        vec![FakeBoard::status(
            p.cmd,
            if p.cmd == Cmd::SetPayloadCrcType as u8 {
                Status::ErrBusy as u8
            } else {
                Status::ErrCmdNotSupported as u8
            },
        )]
    });
    let dev = DepzDevice::new(board, SHORT);
    assert!(dev
        .set_report_payload_crc(CrcType::Crc8)
        .unwrap_err()
        .is_busy());
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
    assert!(
        matches!(r, Err(DepzError::LinkClosed(_))),
        "closing fails the waiting request: {r:?}"
    );
}

#[test]
fn ok_ack_before_the_data_report_keeps_waiting() {
    let board = FakeBoard::new();
    let b = board.clone();
    board.on_packet(move |p| {
        vec![
            FakeBoard::status(p.cmd, Status::Ok as u8),
            b.reply(Rpt::PayloadCrcType as u8, &[2]),
        ]
    });
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
    assert!(matches!(
        dev.get_sync_pin(0),
        Err(DepzError::InvalidArgument(_))
    ));
    assert_eq!(board.sent_count(), 0);
}

#[test]
fn unsolicited_reports_become_events() {
    let board = FakeBoard::new();
    let dev = DepzDevice::new(board.clone(), None);
    let events = dev.events(256);
    // in seq order: a gap would be an event of its own
    board.inject(board.reply(Rpt::Status as u8, &[0, Status::ErrHardwareFault as u8]));
    board.inject(board.reply(Rpt::SequenceError as u8, &[7, 9]));
    board.inject(board.reply(0xC5, &[0xAB]));
    let t = Duration::from_secs(1);
    assert_eq!(
        events.take(t).unwrap(),
        DeviceEvent::UnsolicitedStatus(Status::ErrHardwareFault as u8)
    );
    assert_eq!(
        events.take(t).unwrap(),
        DeviceEvent::SequenceError {
            expected_seq: 7,
            received_seq: 9,
            reported_by_device: true
        }
    );
    assert_eq!(
        events.take(t).unwrap(),
        DeviceEvent::Text {
            cmd: 0xC5,
            text: "ab".into()
        }
    );
    assert_eq!(dev.stats().device_seq_errors, 1);
}

/// Contract 01 §5 step 5 (ERRATA A1): a gap the host sees in the board's seq is a `SeqGap`
/// event of its own, ahead of the packet it was seen on; the first packet is no gap.
#[test]
fn host_observed_seq_gap_is_an_event() {
    let board = FakeBoard::new();
    let dev = DepzDevice::new(board.clone(), None);
    let events = dev.events(256);
    for seq in [5u32, 9] {
        board.inject(build_packet(0xC5, &[seq as u8], seq, CrcType::None).unwrap());
    }
    let t = Duration::from_secs(1);
    let text = |s: &str| DeviceEvent::Text {
        cmd: 0xC5,
        text: s.into(),
    };
    assert_eq!(events.take(t).unwrap(), text("05"));
    assert_eq!(
        events.take(t).unwrap(),
        DeviceEvent::SeqGap {
            expected_seq: 6,
            received_seq: 9,
            missed: 3
        }
    );
    assert_eq!(events.take(t).unwrap(), text("09"));
    assert_eq!(dev.stats().seq_gaps, 1);
    assert_eq!(dev.stats().device_seq_errors, 0);
}

#[test]
fn unplug_fails_pending_and_ends_streams() {
    let board = FakeBoard::new();
    let dev = Arc::new(Sr04Device::attach(DepzDevice::new(
        board.clone(),
        Some(Duration::from_secs(5)),
    )));
    let events = dev.events(256);
    let lp = dev.measurements(256);
    let d = dev.clone();
    let waiting = std::thread::spawn(move || d.get_sample_period_us());
    assert!(until(|| board.sent_count() == 1, 1000));
    board.unplug();
    assert!(
        matches!(waiting.join().unwrap(), Err(DepzError::DeviceLost(_))),
        "the pending request did not fail"
    );
    assert!(matches!(
        events.take(Duration::from_secs(1)).unwrap(),
        DeviceEvent::Disconnected(_)
    ));
    assert!(matches!(
        lp.take(Duration::from_secs(1)),
        Err(DepzError::DeviceLost(_))
    ));
    assert!(lp.is_closed());
    // Contract 07 §3: a reader that ended leaves the device closed; a later request fails at once.
    assert!(
        until(|| dev.is_closed(), 1000),
        "the device is open after the loss"
    );
    let t0 = std::time::Instant::now();
    let r = dev.get_sample_period_us();
    assert!(matches!(r, Err(DepzError::DeviceLost(_))), "{r:?}");
    assert!(
        t0.elapsed() < Duration::from_secs(1),
        "waited {:?}",
        t0.elapsed()
    );
}

/// Contract 07 §3 (bug hunt 2026-10-29 RUST-2): a queue subscribed after the device closed is
/// closed at once — take() fails with the closed error instead of waiting out its timeout.
#[test]
fn a_queue_subscribed_after_close_is_closed_at_once() {
    let dev = Sr04Device::attach(DepzDevice::new(FakeBoard::new(), SHORT));
    dev.close();
    let events = dev.events(4);
    let lp = dev.measurements(4);
    assert!(events.is_closed(), "events() after close is open");
    assert!(lp.is_closed(), "measurements() after close is open");
    let t0 = std::time::Instant::now();
    let r = lp.take(Duration::from_secs(5));
    assert!(matches!(r, Err(DepzError::DeviceLost(_))), "{r:?}");
    let r = events.take(Duration::from_secs(5));
    assert!(matches!(r, Err(DepzError::DeviceLost(_))), "{r:?}");
    assert!(
        t0.elapsed() < Duration::from_secs(1),
        "waited {:?}",
        t0.elapsed()
    );
}

/// RUST-2: the same after a loss (the link failed, nobody called close()).
#[test]
fn a_queue_subscribed_after_a_loss_is_closed_at_once() {
    let board = FakeBoard::new();
    let dev = Sr04Device::attach(DepzDevice::new(board.clone(), SHORT));
    let events = dev.events(8);
    board.unplug();
    assert!(
        until(|| events.is_closed(), 1000),
        "the old queue stays open"
    );
    let late = dev.events(4);
    let lp = dev.measurements(4);
    assert!(
        late.is_closed() && lp.is_closed(),
        "a queue subscribed after the loss is open"
    );
    let t0 = std::time::Instant::now();
    let r = late.take(Duration::from_secs(5));
    assert!(matches!(r, Err(DepzError::DeviceLost(_))), "{r:?}");
    let r = dev.get_measurement(Some(Duration::from_secs(5)));
    assert!(matches!(r, Err(DepzError::DeviceLost(_))), "{r:?}");
    assert!(
        t0.elapsed() < Duration::from_secs(1),
        "waited {:?}",
        t0.elapsed()
    );
}

/// Contract 07 §3 (bug hunt 2026-10-29 RUST-1): a matcher that panics fails its own request with
/// the invalid-reply error — never a panic in the caller or a poisoned device; the reader goes on.
#[test]
fn a_panicking_matcher_fails_only_its_request() {
    let board = FakeBoard::new();
    let b = board.clone();
    board.on_packet(move |p| vec![b.reply(Rpt::Text as u8, &text(p.cmd, "name"))]);
    let dev = Sr04Device::attach(DepzDevice::new(board.clone(), Some(Duration::from_secs(2))));
    let events = dev.events(8);
    let meas = dev.measurements(8);
    let r = dev.request::<()>(
        Cmd::GetDeviceName as u8,
        &[],
        |_| -> Option<depz_sensor_sdk::devices::Result<()>> { panic!("matcher boom") },
        None,
    );
    match r {
        Err(DepzError::Protocol(m)) => {
            assert!(
                m.contains("does not decode") && m.contains("matcher boom"),
                "{m}"
            )
        }
        other => panic!("expected the invalid-reply error, got {other:?}"),
    }
    assert!(!dev.is_closed());
    assert!(!events.is_closed() && !meas.is_closed());
    assert_eq!(dev.get_device_name().unwrap(), "name");
}

/// Routes 0x9E into a panic — a stream decoder bug.
struct PanickingReports;

impl ReportHandler for PanickingReports {
    fn handle_report(&self, packet: &depz_sensor_sdk::framing::Packet) -> bool {
        if packet.cmd == 0x9E {
            panic!("stream decoder bug");
        }
        false
    }
}

/// Contract 07 §3: a report handler that panics drops that packet, counted; the reader goes on.
#[test]
fn a_panicking_report_handler_drops_that_packet() {
    let board = FakeBoard::new();
    let b = board.clone();
    board.on_packet(move |p| vec![b.reply(Rpt::Text as u8, &text(p.cmd, "still here"))]);
    let dev = DepzDevice::new(board.clone(), SHORT);
    dev.set_report_handler(Arc::new(PanickingReports));
    let events = dev.events(8);
    board.inject(board.reply(0x9E, &[1, 2]));
    board.inject(board.reply(0xC5, &[0xAB]));
    assert_eq!(
        events.take(Duration::from_secs(1)).unwrap(),
        DeviceEvent::Text {
            cmd: 0xC5,
            text: "ab".into()
        }
    );
    assert_eq!(dev.stats().report_errors, 1);
    assert_eq!(dev.get_device_name().unwrap(), "still here");
    assert!(!dev.is_closed());
}

/// A link whose read panics — something no link is expected to do.
struct PanickingReadLink {
    inner: FakeBoard,
}

impl Link for PanickingReadLink {
    fn read(&self, _timeout: Duration) -> depz_sensor_sdk::devices::Result<Vec<u8>> {
        panic!("read broke")
    }
    fn write(&self, data: &[u8]) -> depz_sensor_sdk::devices::Result<()> {
        self.inner.write(data)
    }
    fn close(&self) {
        self.inner.close()
    }
    fn is_closed(&self) -> bool {
        self.inner.is_closed()
    }
    fn name(&self) -> String {
        "broken".into()
    }
}

/// Contract 07 §3: a reader that ends for any reason other than close() closes the device —
/// `is_closed()` is true, the queues close and a later request fails at once with DeviceLost.
#[test]
fn a_reader_that_ends_for_any_reason_closes_the_device() {
    let board = FakeBoard::new();
    let dev = Sr04Device::attach(DepzDevice::new(
        PanickingReadLink {
            inner: board.clone(),
        },
        Some(Duration::from_secs(5)),
    ));
    let meas = dev.measurements(8);
    assert!(
        until(|| dev.is_closed(), 1000),
        "the device is open after its reader ended"
    );
    assert!(board.is_closed_for_test(), "the link is open");
    assert!(until(|| meas.is_closed(), 1000), "the queue is open");
    let t0 = std::time::Instant::now();
    let r = dev.get_device_name();
    assert!(matches!(r, Err(DepzError::DeviceLost(_))), "{r:?}");
    assert!(
        t0.elapsed() < Duration::from_secs(1),
        "waited {:?}",
        t0.elapsed()
    );
    assert_eq!(board.sent_count(), 0);
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
    assert_eq!(
        q.drain().iter().map(|m| m.echo_time_us).collect::<Vec<_>>(),
        vec![4, 5]
    );
    assert_eq!(*seen.lock().unwrap(), vec![1, 2, 3, 4, 5]);
    assert!(q.try_take().is_none());
    cb.cancel();
}

/// Contract 07 §3: a panicking callback is counted and dropped — the other callback and the queue
/// still get the item, the reader goes on (a request completes, the next report is read).
#[test]
fn a_panicking_callback_never_stops_the_reader() {
    let board = FakeBoard::new();
    let b = board.clone();
    board.on_packet(move |p| vec![b.reply(Rpt::Text as u8, &text(p.cmd, "APP_usonic_SR04_v0.97"))]);
    let dev = Sr04Device::attach(DepzDevice::new(board.clone(), None));
    let _boom = dev.on_event(|_| panic!("consumer bug"));
    let seen = Arc::new(Mutex::new(Vec::new()));
    let s = seen.clone();
    let _keep = dev.on_event(move |e| s.lock().unwrap().push(e.clone()));
    let events = dev.events(8);
    board.inject(board.reply(0xC5, &[0xAB]));
    let t = Duration::from_secs(1);
    let text_ev = DeviceEvent::Text {
        cmd: 0xC5,
        text: "ab".into(),
    };
    assert_eq!(events.take(t).unwrap(), text_ev);
    assert_eq!(*seen.lock().unwrap(), vec![text_ev]);
    assert_eq!(dev.stats().callback_errors, 1);
    assert_eq!(dev.get_software_name().unwrap(), "APP_usonic_SR04_v0.97");

    let _boom_m = dev.on_measurement(|_| panic!("consumer bug"));
    let q = dev.measurements(4);
    let mut payload = [0u8; 11];
    payload[0] = Sr04Cmd::StartMeasurementLoop as u8;
    payload[9] = 7;
    board.inject(board.reply(Sr04Rpt::Data as u8, &payload));
    assert_eq!(q.take(t).unwrap().echo_time_us, 7);
    assert_eq!(dev.stats().callback_errors, 2);
}

#[test]
fn recording_replays_byte_for_byte() {
    let dir = std::env::temp_dir().join(format!("depz-rec-{}", std::process::id()));
    std::fs::create_dir_all(&dir).unwrap();
    let path = dir.join("t.depzrec");
    {
        let board = FakeBoard::new();
        let b = board.clone();
        board.on_packet(move |p| {
            vec![b.reply(Rpt::Text as u8, &text(p.cmd, "APP_usonic_SR04_v0.97"))]
        });
        let link = RecordingLink::create(
            board,
            &path,
            vec![("note".into(), Value::String("test".into()))],
        )
        .unwrap();
        let rec = DepzDevice::new(link, None);
        assert_eq!(rec.get_software_name().unwrap(), "APP_usonic_SR04_v0.97");
    }
    let replay = Arc::new(ReplayLink::open(&path, true, false).unwrap());
    assert_eq!(
        replay.header().get("note").and_then(Value::as_str),
        Some("test")
    );
    let dev = DepzDevice::new(replay.clone(), None);
    assert_eq!(dev.get_software_name().unwrap(), "APP_usonic_SR04_v0.97");
    assert!(replay.rx_exhausted());
    assert_eq!(replay.tx_remaining(), 0);
    assert!(matches!(
        dev.send(Cmd::GetSerial as u8, &[]),
        Err(DepzError::ReplayMismatch(_))
    ));
    drop(dev);
    let _ = std::fs::remove_dir_all(&dir);
}

/// ERRATA A6: there is no `.depzrec.gz` — a capture is plain text, a gzip
/// container is not inflated but refused like any damaged file. The gzipped
/// bytes (mtime 0) are the same 70 in every SDK's test; `PLAIN` is what they
/// inflate to, a valid capture.
#[test]
fn replay_refuses_a_gzipped_capture() {
    const PLAIN: &str =
        "{\"schema\": \"depz.rec/1\"}\n{\"t\": 0, \"dir\": \"rx\", \"data\": \"aa\"}\n";
    const GZIPPED: [u8; 70] = [
        0x1f, 0x8b, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x03, 0xab, 0x56, 0x2a, 0x4e, 0xce,
        0x48, 0xcd, 0x4d, 0x54, 0xb2, 0x52, 0x50, 0x4a, 0x49, 0x2d, 0xa8, 0xd2, 0x2b, 0x4a, 0x4d,
        0xd6, 0x37, 0x54, 0xaa, 0xe5, 0xaa, 0x56, 0x2a, 0x01, 0x8a, 0x19, 0xe8, 0x00, 0x85, 0x33,
        0x8b, 0x40, 0xb2, 0x45, 0x15, 0x4a, 0x20, 0x4e, 0x62, 0x09, 0x58, 0x6d, 0x62, 0x22, 0x50,
        0x0d, 0x00, 0x7e, 0x3b, 0xc0, 0x0b, 0x3d, 0x00, 0x00, 0x00,
    ];
    let dir = std::env::temp_dir().join(format!("depz-rec-gz-{}", std::process::id()));
    std::fs::create_dir_all(&dir).unwrap();
    let path = dir.join("t.depzrec");
    std::fs::write(&path, PLAIN).unwrap();
    drop(ReplayLink::open(&path, false, false).unwrap());
    std::fs::write(&path, GZIPPED).unwrap();
    assert!(matches!(
        ReplayLink::open(&path, false, false),
        Err(DepzError::InvalidArgument(_))
    ));
    let _ = std::fs::remove_dir_all(&dir);
}

/// A capture with nothing left behaves like a quiet port: a read returns empty at its timeout
/// (or at close), never at once — the reader thread would spin a core on empty reads
/// (contract 08 «Replay semantics»).
#[test]
fn exhausted_replay_waits_its_timeout() {
    let dir = std::env::temp_dir().join(format!("depz-rec-exhausted-{}", std::process::id()));
    std::fs::create_dir_all(&dir).unwrap();
    let path = dir.join("t.depzrec");
    std::fs::write(&path, "{\"schema\": \"depz.rec/1\"}\n").unwrap();
    let link = Arc::new(ReplayLink::open(&path, false, false).unwrap());
    assert!(link.rx_exhausted());
    let t0 = std::time::Instant::now();
    assert!(link.read(Duration::from_millis(100)).unwrap().is_empty());
    assert!(t0.elapsed() >= Duration::from_millis(80));
    // a long read ends at close(), not at its 30 s timeout
    let reader = {
        let link = link.clone();
        std::thread::spawn(move || link.read(Duration::from_secs(30)))
    };
    let t0 = std::time::Instant::now();
    link.close();
    assert!(matches!(
        reader.join().unwrap(),
        Err(DepzError::LinkClosed(_))
    ));
    assert!(t0.elapsed() < Duration::from_secs(5));
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
    let AnyDevice::Sr04(sr04) = dev else {
        panic!("resolved {}", dev.class_name())
    };
    assert_eq!(sr04.get_echo_decay_us().unwrap(), 5000);
    assert_eq!(
        board
            .sent_snapshot()
            .iter()
            .map(|p| p.seq)
            .collect::<Vec<_>>(),
        vec![0, 1, 2, 3]
    );
}

#[test]
fn open_on_link_refuses_bootloader_and_silence() {
    let boot = FakeBoard::new();
    let b = boot.clone();
    boot.on_packet(move |p| vec![b.reply(Rpt::Text as u8, &text(p.cmd, "BOOTDEPZ_v1.11"))]);
    assert!(matches!(
        open_on_link(boot.clone(), None, None, SHORT),
        Err(DepzError::BootloaderMode)
    ));
    assert!(boot.is_closed_for_test());
    let silent = FakeBoard::new();
    assert!(matches!(
        open_on_link(silent.clone(), None, None, SHORT),
        Err(DepzError::NoDevice(_))
    ));
    assert!(silent.is_closed_for_test());
}

#[test]
fn timeout_crc_and_port_settings() {
    let board = FakeBoard::new();
    let b = board.clone();
    board.on_packet(move |p| vec![b.reply(Rpt::Text as u8, &text(p.cmd, "APP_usonic_SR04_v0.97"))]);
    let dev = DepzDevice::new(board.clone(), SHORT);
    assert_eq!(dev.port(), "fake");
    assert_eq!(dev.timeout(), Duration::from_millis(100));
    assert!(matches!(
        dev.set_timeout(Duration::ZERO),
        Err(DepzError::InvalidArgument(_))
    ));
    dev.set_timeout(Duration::from_millis(250)).unwrap();
    assert_eq!(dev.timeout(), Duration::from_millis(250));
    assert_eq!(dev.tx_crc_type(), CrcType::None);
    dev.set_tx_crc_type(CrcType::Crc16);
    assert_eq!(dev.tx_crc_type(), CrcType::Crc16);
    // the board's parser checks the CRC-16 the packet now carries
    assert_eq!(dev.get_software_name().unwrap(), "APP_usonic_SR04_v0.97");
}

#[test]
fn sync_time_keeps_the_round_with_the_lowest_rtt() {
    // Contract 02 §5: offset = ((t2 − t1) + (t3 − t4)) / 2, rtt = (t4 − t1) − (t3 − t2). The board's
    // clock runs 5 s ahead; round 1 claims 40 ms spent between rx and tx, so its rtt is the lowest
    // and its offset reads 20 ms more.
    let board = FakeBoard::new();
    let b = board.clone();
    let round = Arc::new(Mutex::new(0u64));
    let r = round.clone();
    board.on_packet(move |p| {
        if p.cmd != Cmd::SyncTime as u8 {
            return vec![];
        }
        let mut n = r.lock().unwrap();
        let held: u64 = if *n == 1 { 40_000 } else { 0 };
        *n += 1;
        let rx = (host_now_us() + 5_000_000) as u64;
        let mut payload = p.payload[..8].to_vec(); // the host's t1 echoed
        payload.extend_from_slice(&rx.to_le_bytes());
        payload.extend_from_slice(&(rx + held).to_le_bytes());
        vec![b.reply(Rpt::SyncTime as u8, &payload)]
    });
    let dev = DepzDevice::new(board.clone(), Some(Duration::from_secs(1)));
    let s = dev.sync_time(3).unwrap();
    assert_eq!(*round.lock().unwrap(), 3);
    assert!(s.rtt_us < 0, "the held round was not kept: {s:?}");
    assert!((s.offset_us - 5_020_000).abs() < 20_000, "{s:?}");
    assert_eq!(dev.last_time_sync(), Some(s));
    assert_eq!(
        dev.to_host_time_us(6_000_000).unwrap(),
        6_000_000 - s.offset_us
    );
    assert!(s.synced_at_host_us <= host_now_us());
    dev.sync_time(0).unwrap(); // at least one round
    assert_eq!(*round.lock().unwrap(), 4);
}

/// The next event that is not a host-observed seq gap.
fn next_event(events: &depz_sensor_sdk::devices::Subscription<DeviceEvent>) -> DeviceEvent {
    loop {
        match events.take(Duration::from_secs(1)).unwrap() {
            DeviceEvent::SeqGap { .. } => continue,
            e => return e,
        }
    }
}

#[test]
fn unsolicited_text_temperature_crc_errors_and_trash_are_events() {
    let board = FakeBoard::new();
    let dev = DepzDevice::new(board.clone(), None);
    let events = dev.events(256);
    board.inject(board.reply(Rpt::Text as u8, &text(0x00, "boot ok")));
    let mut temp = 1234u64.to_le_bytes().to_vec();
    temp.extend_from_slice(&(-55i16).to_le_bytes());
    board.inject(board.reply(Rpt::Temperature as u8, &temp));
    let mut bad = build_packet(0xC6, &[1, 2, 3, 4], 200, CrcType::Crc16).unwrap();
    let n = bad.len();
    bad[n - 3] ^= 0xFF; // a payload byte: the payload CRC fails
    board.inject(bad);
    board.inject(vec![0x00, 0x11, 0x22]);
    board.inject(board.reply(Rpt::Text as u8, &text(0x00, "after")));
    assert_eq!(
        next_event(&events),
        DeviceEvent::Text {
            cmd: 0,
            text: "boot ok".into()
        }
    );
    assert_eq!(
        next_event(&events),
        DeviceEvent::Temperature {
            timestamp_us: 1234,
            celsius: -5.5
        }
    );
    assert_eq!(
        next_event(&events),
        DeviceEvent::LinkCrcError {
            cmd: 0xC6,
            seq: 200
        }
    );
    // Trash boundaries depend on chunking (contract 01 §5): compare the bytes up to the next packet
    let mut trash = Vec::new();
    loop {
        match next_event(&events) {
            DeviceEvent::Trash(t) => trash.extend(t),
            e => {
                assert_eq!(
                    e,
                    DeviceEvent::Text {
                        cmd: 0,
                        text: "after".into()
                    }
                );
                break;
            }
        }
    }
    assert_eq!(trash, [0x00, 0x11, 0x22]);
    // a sensor class hands its own events to the same subscribers
    let ev = DeviceEvent::Sensor {
        kind: "test",
        code: 7,
        payload: vec![9],
    };
    dev.emit_event(ev.clone());
    assert_eq!(next_event(&events), ev);
    let st = dev.stats();
    assert_eq!((st.crc_errors, st.trash_bytes), (1, 3));
}

#[test]
fn sync_pins_payload_crc_reset_and_bootloader() {
    let board = FakeBoard::new();
    let b = board.clone();
    board.on_packet(move |p| match p.cmd {
        c if c == Cmd::GetSyncPinConfig as u8 => {
            vec![b.reply(Rpt::SyncPinConfig as u8, &[p.payload[0], 2, 1])]
        }
        c if c == Cmd::GetPayloadCrcType as u8 => vec![b.reply(Rpt::PayloadCrcType as u8, &[2])],
        c => vec![FakeBoard::status(c, Status::Ok as u8)],
    });
    let dev = DepzDevice::new(board.clone(), SHORT);
    assert_eq!(
        dev.get_sync_pin(3).unwrap(),
        SyncPinConfig {
            pin: 3,
            mode: 2,
            polarity: 1
        }
    );
    dev.set_sync_pin(&SyncPinConfig {
        pin: 5,
        mode: 4,
        polarity: 0,
    })
    .unwrap();
    let sent = board.sent_count();
    for bad in [
        SyncPinConfig {
            pin: 6,
            mode: 0,
            polarity: 0,
        },
        SyncPinConfig {
            pin: 1,
            mode: 5,
            polarity: 0,
        },
        SyncPinConfig {
            pin: 1,
            mode: 0,
            polarity: 2,
        },
    ] {
        assert!(matches!(
            dev.set_sync_pin(&bad),
            Err(DepzError::InvalidArgument(_))
        ));
    }
    assert_eq!(board.sent_count(), sent);
    assert_eq!(dev.get_report_payload_crc().unwrap(), CrcType::Crc16);
    dev.reset().unwrap();
    assert!(!dev.is_closed());
    dev.enter_bootloader_mode().unwrap();
    assert!(dev.is_closed());
    let cmds: Vec<(u8, Vec<u8>)> = board
        .sent_snapshot()
        .into_iter()
        .map(|p| (p.cmd, p.payload))
        .collect();
    assert_eq!(
        cmds,
        vec![
            (Cmd::GetSyncPinConfig as u8, vec![3]),
            (Cmd::SetSyncPinConfig as u8, vec![5, 4, 0]),
            (Cmd::GetPayloadCrcType as u8, vec![]),
            (Cmd::DeviceReset as u8, vec![]),
            (Cmd::Bootloader as u8, vec![]),
        ]
    );
}

#[test]
fn a_payload_crc_type_outside_the_four_is_a_protocol_error() {
    let board = FakeBoard::new();
    let b = board.clone();
    board.on_packet(move |_| vec![b.reply(Rpt::PayloadCrcType as u8, &[7])]);
    let dev = DepzDevice::new(board.clone(), SHORT);
    assert!(matches!(
        dev.get_report_payload_crc(),
        Err(DepzError::Protocol(_))
    ));
}
