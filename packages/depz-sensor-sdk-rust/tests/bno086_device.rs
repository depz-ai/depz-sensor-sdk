//! The BNO085 / BNO086 device class over a fake SH-2 hub.
mod common;

use std::sync::atomic::{AtomicUsize, Ordering};
use std::sync::{Arc, Mutex};
use std::time::Duration;

use common::fake_board::FakeBoard;
use depz_sensor_sdk::bno086::hub::{self, Bno086Cmd, Counts, ErrorRecord};
use depz_sensor_sdk::bno086::{sh2, shtp, ShtpChannel};
use depz_sensor_sdk::devices::{Bno086Device, DepzDevice, DepzError};
use depz_sensor_sdk::protocol::common::Status;

type OnCargo = Arc<dyn Fn(u8, &[u8]) -> Cargos + Send + Sync>;
/// (SHTP channel, cargo) pairs, as sent or as returned by `OnCargo`.
type Cargos = Vec<(u8, Vec<u8>)>;

/// A SH-2 hub behind the SHTP pass-through bridge: every SEND_SHTP_PACKET is acknowledged, its cargo
/// handed to `on_cargo`, and the cargos that returns come back as RPT_DATA frames. `busy_first`
/// answers that many sends with ERR_BUSY first.
struct Hub {
    link: FakeBoard,
    sent: Arc<Mutex<Cargos>>,
    on_cargo: Arc<Mutex<Option<OnCargo>>>,
    busy_first: Arc<AtomicUsize>,
    seq: Arc<Mutex<[u8; 6]>>,
}

impl Hub {
    fn new() -> Hub {
        let h = Hub {
            link: FakeBoard::new(),
            sent: Arc::new(Mutex::new(Vec::new())),
            on_cargo: Arc::new(Mutex::new(None)),
            busy_first: Arc::new(AtomicUsize::new(0)),
            seq: Arc::new(Mutex::new([0; 6])),
        };
        let (link, sent, on, busy, seq) = (
            h.link.clone(),
            h.sent.clone(),
            h.on_cargo.clone(),
            h.busy_first.clone(),
            h.seq.clone(),
        );
        h.link.on_packet(move |p| {
            if p.cmd != Bno086Cmd::SendShtpPacket as u8 {
                return vec![FakeBoard::status(p.cmd, Status::Ok as u8)];
            }
            if busy.load(Ordering::SeqCst) > 0 {
                busy.fetch_sub(1, Ordering::SeqCst);
                return vec![FakeBoard::status(p.cmd, Status::ErrBusy as u8)];
            }
            let channel = p.payload[2];
            let cargo = p.payload[4..].to_vec();
            sent.lock().unwrap().push((channel, cargo.clone()));
            let mut replies = vec![FakeBoard::status(p.cmd, Status::Ok as u8)];
            let f = on.lock().unwrap().clone();
            if let Some(f) = f {
                for (ch, c) in f(channel, &cargo) {
                    replies.push(data(&link, &seq, ch, &c));
                }
            }
            replies
        });
        h
    }

    fn on(&self, f: impl Fn(u8, &[u8]) -> Vec<(u8, Vec<u8>)> + Send + Sync + 'static) {
        *self.on_cargo.lock().unwrap() = Some(Arc::new(f));
    }

    fn data(&self, channel: u8, cargo: &[u8]) -> Vec<u8> {
        data(&self.link, &self.seq, channel, cargo)
    }

    fn sent_with(&self, report_id: u8) -> Vec<Vec<u8>> {
        self.sent
            .lock()
            .unwrap()
            .iter()
            .filter(|(_, c)| c[0] == report_id)
            .map(|(_, c)| c.clone())
            .collect()
    }

    fn open(&self) -> Bno086Device {
        let dev = Bno086Device::attach(DepzDevice::new(
            self.link.clone(),
            Some(Duration::from_millis(500)),
        ));
        dev.set_busy_retry(5, Duration::from_millis(1)).unwrap();
        dev
    }
}

fn data(link: &FakeBoard, seq: &Mutex<[u8; 6]>, channel: u8, cargo: &[u8]) -> Vec<u8> {
    let s = {
        let mut sq = seq.lock().unwrap();
        let s = sq[channel as usize];
        sq[channel as usize] = s.wrapping_add(1);
        s
    };
    let frame = shtp::try_build_frame(channel, cargo, s).unwrap();
    let mut payload = vec![0u8; 9];
    payload[1..9].copy_from_slice(&1_000_000u64.to_le_bytes());
    payload.extend_from_slice(&frame);
    link.reply(hub::RPT_DATA, &payload)
}

const CONTROL: u8 = ShtpChannel::Control as u8;

fn feature(sensor: u8, interval_us: u32) -> Vec<u8> {
    let mut r = vec![0u8; 17];
    r[0] = sh2::REPORT_GET_FEATURE_RESPONSE;
    r[1] = sensor;
    r[5..9].copy_from_slice(&interval_us.to_le_bytes());
    r
}

fn frs_read(status: u8, len: u8, offset: u16, d0: u32, d1: u32, typ: u16) -> Vec<u8> {
    let mut r = vec![0u8; 16];
    r[0] = sh2::REPORT_FRS_READ_RESPONSE;
    r[1] = len << 4 | status;
    r[2..4].copy_from_slice(&offset.to_le_bytes());
    r[4..8].copy_from_slice(&d0.to_le_bytes());
    r[8..12].copy_from_slice(&d1.to_le_bytes());
    r[12..14].copy_from_slice(&typ.to_le_bytes());
    r
}

fn frs_write(status: u8, offset: u16) -> Vec<u8> {
    vec![
        sh2::REPORT_FRS_WRITE_RESPONSE,
        status,
        offset as u8,
        (offset >> 8) as u8,
    ]
}

fn u32le(b: &[u8], o: usize) -> u32 {
    u32::from_le_bytes([b[o], b[o + 1], b[o + 2], b[o + 3]])
}

#[test]
fn a_busy_bridge_is_retried_then_given_up() {
    let hub = Hub::new();
    let dev = hub.open();
    hub.on(|_, c| vec![(CONTROL, feature(c[1], 10000))]);
    hub.busy_first.store(2, Ordering::SeqCst);
    assert_eq!(dev.get_feature(5, None).unwrap().interval_us, 10000);
    hub.busy_first.store(5, Ordering::SeqCst);
    assert!(dev.get_feature(5, None).unwrap_err().is_busy());
}

#[test]
fn enable_skips_a_stale_disabled_read_back() {
    let hub = Hub::new();
    let dev = hub.open();
    let reads = Arc::new(AtomicUsize::new(0));
    let r = reads.clone();
    hub.on(move |_, c| {
        if c[0] == sh2::REPORT_GET_FEATURE_REQUEST {
            let n = r.fetch_add(1, Ordering::SeqCst);
            vec![(CONTROL, feature(c[1], if n < 2 { 0 } else { 10000 }))]
        } else {
            vec![]
        }
    });
    assert_eq!(dev.enable(0x05, 100.0).unwrap().interval_us, 10000);
    assert_eq!(reads.load(Ordering::SeqCst), 3);
    assert_eq!(
        u32le(&hub.sent_with(sh2::REPORT_SET_FEATURE_COMMAND)[0], 5),
        10000
    );
    assert!(matches!(
        dev.enable(5, 0.0),
        Err(DepzError::InvalidArgument(_))
    ));
    assert!(matches!(
        dev.enable(5, f64::NAN),
        Err(DepzError::InvalidArgument(_))
    ));
}

#[test]
fn frs_read_collects_every_word_of_the_record() {
    let hub = Hub::new();
    let dev = hub.open();
    hub.on(|_, _| {
        vec![
            (
                CONTROL,
                frs_read(hub::FRS_NO_ERROR, 2, 0, 11, 22, hub::FRS_USER_RECORD),
            ),
            (
                CONTROL,
                frs_read(hub::FRS_READ_COMPLETED, 1, 2, 33, 0, hub::FRS_USER_RECORD),
            ),
        ]
    });
    assert_eq!(
        dev.frs_read(hub::FRS_USER_RECORD, None).unwrap(),
        vec![11, 22, 33]
    );
    hub.on(|_, _| {
        vec![(
            CONTROL,
            frs_read(hub::FRS_RECORD_EMPTY, 0, 0, 0, 0, hub::FRS_SERIAL_NUMBER),
        )]
    });
    assert!(matches!(
        dev.frs_read(hub::FRS_SERIAL_NUMBER, None),
        Err(DepzError::Sensor(_))
    ));
}

#[test]
fn frs_write_sends_the_words_two_at_a_time() {
    let hub = Hub::new();
    let dev = hub.open();
    hub.on(|_, c| match c[0] {
        sh2::REPORT_FRS_WRITE_REQUEST => vec![(CONTROL, frs_write(hub::FRS_WRITE_MODE_READY, 0))],
        sh2::REPORT_FRS_WRITE_DATA if c[2] == 2 => {
            vec![(CONTROL, frs_write(hub::FRS_WRITE_COMPLETED, 2))]
        }
        sh2::REPORT_FRS_WRITE_DATA => vec![(
            CONTROL,
            frs_write(hub::FRS_WRITE_WORDS_RECEIVED, c[2] as u16),
        )],
        _ => vec![],
    });
    dev.frs_write(hub::FRS_USER_RECORD, &[1, 2, 3], None)
        .unwrap();
    let data = hub.sent_with(sh2::REPORT_FRS_WRITE_DATA);
    assert_eq!(data.len(), 2);
    assert_eq!((u32le(&data[0], 4), u32le(&data[0], 8)), (1, 2));
    assert_eq!((data[1][2], u32le(&data[1], 4)), (2, 3));
}

#[test]
fn flush_returns_when_flush_completed_arrives() {
    let hub = Hub::new();
    let dev = hub.open();
    hub.on(|_, c| {
        if c[0] == hub::FORCE_SENSOR_FLUSH {
            vec![(ShtpChannel::InputNormal as u8, vec![0xEF, c[1]])]
        } else {
            vec![]
        }
    });
    dev.flush(0x01, None).unwrap();
    hub.on(|_, _| vec![]);
    assert!(matches!(
        dev.flush(0x02, Some(Duration::from_millis(50))),
        Err(DepzError::Timeout { .. })
    ));
}

fn pid(part: u32) -> Vec<u8> {
    let mut b = vec![0xF8, 4, 3, 12];
    b.extend_from_slice(&part.to_le_bytes());
    b.extend_from_slice(&[0; 8]);
    b
}

#[test]
fn product_ids_collect_every_component() {
    let hub = Hub::new();
    let dev = hub.open();
    hub.on(|_, _| vec![(CONTROL, [pid(10004563), pid(10003606)].concat())]); // one cargo, two responses
    let ids = dev
        .product_ids(None, Some(Duration::from_millis(30)))
        .unwrap();
    assert_eq!(
        ids.iter().map(|i| i.sw_part_number).collect::<Vec<_>>(),
        vec![10004563, 10003606]
    );
    assert_eq!(ids[0].version(), "3.12.0");
}

#[test]
fn short_product_id_reply_fails_the_request_as_invalid_reply() {
    let hub = Hub::new();
    let dev = hub.open();
    hub.on(|_, _| vec![(CONTROL, pid(10004563)[..15].to_vec())]); // one byte short of its 16
    let got = dev.product_id(Some(Duration::from_millis(300)));
    assert!(
        matches!(got, Err(DepzError::Protocol(_))),
        "a short reply is invalid_reply, got {:?}",
        got
    );
}

fn resp(command: u8, cmd_seq: u8, resp_seq: u8, r: &[u8]) -> Vec<u8> {
    let mut b = vec![0xF1, 0, command, cmd_seq, resp_seq];
    b.extend_from_slice(r);
    b.resize(16, 0);
    b
}

#[test]
fn errors_and_counts_collect_their_responses() {
    let hub = Hub::new();
    let dev = hub.open();
    hub.on(|_, c| match c[2] {
        hub::CMD_ERRORS => vec![
            (CONTROL, resp(1, c[1], 0, &[1, 7, 3, 9, 2, 4])),
            (CONTROL, resp(1, c[1], 1, &[0, 0, 255])),
        ],
        hub::CMD_COUNTER => vec![
            (
                CONTROL,
                resp(2, c[1], 0, &[0, 0, 0, 5, 0, 0, 0, 6, 0, 0, 0]),
            ),
            (
                CONTROL,
                resp(2, c[1], 1, &[0, 0, 0, 7, 0, 0, 0, 8, 0, 0, 0]),
            ),
        ],
        _ => vec![],
    });
    assert_eq!(
        dev.get_errors(0).unwrap(),
        vec![ErrorRecord {
            severity: 1,
            seq: 7,
            source: 3,
            error: 9,
            module: 2,
            code: 4
        }]
    );
    assert_eq!(
        dev.get_counts(5).unwrap(),
        Counts {
            sensor_id: 5,
            offered: 5,
            accepted: 6,
            on: 7,
            attempted: 8
        }
    );
}

#[test]
fn reports_are_filtered_by_sensor() {
    let hub = Hub::new();
    let dev = hub.open();
    let accel = dev.reports(Some(&[0x01]), 16);
    let all = dev.reports(None, 1024);
    let cargo = [
        &[0xFB, 0, 0, 0, 0][..],
        &[0x01, 0, 0, 0, 1, 0, 2, 0, 3, 0][..], // accelerometer
        &[0x02, 0, 0, 0, 1, 0, 2, 0, 3, 0][..], // gyroscope
    ]
    .concat();
    hub.link
        .inject(hub.data(ShtpChannel::InputNormal as u8, &cargo));
    let t = Duration::from_secs(1);
    assert_eq!(accel.take(t).unwrap().sensor_id(), 0x01);
    assert_eq!(all.take(t).unwrap().sensor_id(), 0x01);
    assert_eq!(all.take(t).unwrap().sensor_id(), 0x02);
    assert!(
        accel.try_take().is_none(),
        "the gyroscope report passed the accelerometer filter"
    );
}

#[test]
fn reorientation_params_are_q14_and_checked() {
    assert_eq!(
        hub::set_reorientation_params(0.0, 0.0, 0.5, 1.0).unwrap(),
        [2, 0, 0, 0, 0, 0x00, 0x20, 0x00, 0x40]
    );
    assert!(hub::set_reorientation_params(2.0, 0.0, 0.0, 0.0).is_err());
}
