//! The SR04 device class over a fake ranger (contract 03).
mod common;

use std::sync::atomic::{AtomicBool, AtomicUsize, Ordering};
use std::sync::{Arc, Mutex};
use std::time::Duration;

use common::fake_board::{until, FakeBoard};
use depz_sensor_sdk::devices::{DepzDevice, DepzError, Sr04Device};
use depz_sensor_sdk::protocol::common::Status;
use depz_sensor_sdk::protocol::sr04::{
    self, Sr04Cmd, Sr04Data, Sr04Rpt, ECHO_DECAY_MAX_US, ECHO_DECAY_MIN_US, ECHO_TIMEOUT,
};

/// An SR04 behind the board: the stored sample period and echo decay (clamped to 4000–65000 as the
/// firmware does), a loop flag, and MEASURE_ONCE answered by an RPT_DATA(0x36) with `echo_us` — or
/// ERR_BUSY while the loop runs. `loop_sample_first` puts a loop sample ahead of that reply.
struct Ranger {
    link: FakeBoard,
    state: Arc<Mutex<State>>,
}

struct State {
    period_us: u32,
    decay_us: u16,
    looping: bool,
    echo_us: u16,
    loop_sample_first: bool,
}

fn data(source: u8, ts: u64, echo_us: u16) -> Vec<u8> {
    let mut p = vec![source];
    p.extend_from_slice(&ts.to_le_bytes());
    p.extend_from_slice(&echo_us.to_le_bytes());
    p
}

impl Ranger {
    fn new() -> Ranger {
        let link = FakeBoard::new();
        let state = Arc::new(Mutex::new(State {
            period_us: sr04::SAMPLE_PERIOD_DEFAULT_US,
            decay_us: sr04::ECHO_DECAY_DEFAULT_US,
            looping: false,
            echo_us: 1000,
            loop_sample_first: false,
        }));
        let (l, s) = (link.clone(), state.clone());
        link.on_packet(move |p| {
            let mut st = s.lock().unwrap();
            let ok = FakeBoard::status(p.cmd, Status::Ok as u8);
            match p.cmd {
                c if c == Sr04Cmd::GetSamplePeriod as u8 => {
                    vec![l.reply(Sr04Rpt::SamplePeriod as u8, &st.period_us.to_le_bytes())]
                }
                c if c == Sr04Cmd::SetSamplePeriod as u8 => {
                    st.period_us = u32::from_le_bytes(p.payload[..4].try_into().unwrap());
                    vec![ok]
                }
                c if c == Sr04Cmd::GetEchoDecay as u8 => {
                    vec![l.reply(Sr04Rpt::EchoDecay as u8, &st.decay_us.to_le_bytes())]
                }
                c if c == Sr04Cmd::SetEchoDecay as u8 => {
                    let v = u16::from_le_bytes([p.payload[0], p.payload[1]]);
                    st.decay_us = v.clamp(ECHO_DECAY_MIN_US, ECHO_DECAY_MAX_US);
                    vec![ok]
                }
                c if c == Sr04Cmd::MeasureOnce as u8 => {
                    if st.looping {
                        return vec![FakeBoard::status(p.cmd, Status::ErrBusy as u8)];
                    }
                    // nothing on success: only the RPT_DATA when the echo completes
                    let mut out = Vec::new();
                    if st.loop_sample_first {
                        let loop_cmd = Sr04Cmd::StartMeasurementLoop as u8;
                        out.push(l.reply(Sr04Rpt::Data as u8, &data(loop_cmd, 5, 77)));
                    }
                    out.push(l.reply(Sr04Rpt::Data as u8, &data(c, 42, st.echo_us)));
                    out
                }
                c if c == Sr04Cmd::StartMeasurementLoop as u8 => {
                    st.looping = true;
                    vec![ok]
                }
                c if c == Sr04Cmd::StopMeasurementLoop as u8 => {
                    st.looping = false;
                    vec![ok]
                }
                _ => vec![ok],
            }
        });
        Ranger { link, state }
    }

    fn open(&self) -> Sr04Device {
        Sr04Device::attach(DepzDevice::new(
            self.link.clone(),
            Some(Duration::from_millis(300)),
        ))
    }

    fn inject_data(&self, source: u8, ts: u64, echo_us: u16) {
        self.link.inject(
            self.link
                .reply(Sr04Rpt::Data as u8, &data(source, ts, echo_us)),
        );
    }
}

#[test]
fn the_sample_period_is_a_u32_le_round_trip() {
    let r = Ranger::new();
    let dev = r.open();
    assert_eq!(dev.get_sample_period_us().unwrap(), 50_000);
    dev.set_sample_period_us(100_000).unwrap();
    let set = r
        .link
        .sent_snapshot()
        .into_iter()
        .find(|p| p.cmd == Sr04Cmd::SetSamplePeriod as u8)
        .unwrap();
    assert_eq!(set.payload, 100_000u32.to_le_bytes());
    assert_eq!(dev.get_sample_period_us().unwrap(), 100_000);
}

#[test]
fn set_echo_decay_returns_the_value_the_device_clamped_to() {
    let r = Ranger::new();
    let dev = r.open();
    assert_eq!(dev.get_echo_decay_us().unwrap(), 5_000);
    assert_eq!(dev.set_echo_decay_us(6_000).unwrap(), 6_000);
    assert_eq!(dev.set_echo_decay_us(1_000).unwrap(), ECHO_DECAY_MIN_US);
    assert_eq!(dev.set_echo_decay_us(65_535).unwrap(), ECHO_DECAY_MAX_US);
    // every set is followed by a read-back
    let gets = r
        .link
        .sent_snapshot()
        .iter()
        .filter(|p| p.cmd == Sr04Cmd::GetEchoDecay as u8)
        .count();
    assert_eq!(gets, 4);
}

#[test]
fn measure_once_waits_for_its_own_sample_and_is_busy_while_the_loop_runs() {
    let r = Ranger::new();
    let dev = r.open();
    r.state.lock().unwrap().loop_sample_first = true;
    let m = dev.measure_once(None).unwrap();
    assert_eq!(m.source, "once");
    assert_eq!((m.timestamp_us, m.echo_time_us), (42, 1000));
    assert!(m.valid());
    assert_eq!(m.distance_mm(), Some(171.5)); // 1000 µs · 343 / 2000
    assert!(
        (m.distance_mm_at(20.0).unwrap() - 1000.0 * (331.3 + 0.606 * 20.0) / 2000.0).abs() < 1e-9
    );

    dev.start().unwrap();
    dev.start().unwrap(); // idempotent
    let e = dev.measure_once(None).unwrap_err();
    assert!(e.is_busy(), "{e:?}");
    dev.stop().unwrap();
    r.state.lock().unwrap().echo_us = ECHO_TIMEOUT;
    let m = dev.measure_once(Some(Duration::from_millis(500))).unwrap();
    assert!(!m.valid());
    assert_eq!(m.distance_mm(), None);
    assert_eq!(m.distance_mm_at(20.0), None);
}

#[test]
fn loop_samples_reach_queues_callbacks_and_get_measurement() {
    let r = Ranger::new();
    let dev = r.open();
    let q = dev.measurements(1);
    let seen = Arc::new(AtomicUsize::new(0));
    let s = seen.clone();
    let _hook = dev.on_measurement(move |m| {
        assert_eq!(m.source, "loop");
        s.fetch_add(1, Ordering::SeqCst);
    });
    let lp = Sr04Cmd::StartMeasurementLoop as u8;
    r.inject_data(lp, 1, 500);
    r.inject_data(lp, 2, 600);
    assert!(until(|| seen.load(Ordering::SeqCst) == 2, 1000));
    // capacity 1: the older sample dropped and counted
    assert!(until(|| dev.stream_dropped_counts().contains(&1), 1000));
    assert_eq!(q.take(Duration::from_secs(1)).unwrap().timestamp_us, 2);

    // a source the contract does not define (ERRATA B11) and a short report are not measurements
    r.inject_data(0x34, 3, 700);
    r.link
        .inject(r.link.reply(Sr04Rpt::Data as u8, &data(lp, 4, 800)[..10]));
    r.inject_data(lp, 5, 900);
    assert_eq!(q.take(Duration::from_secs(1)).unwrap().timestamp_us, 5);
    assert!(until(|| seen.load(Ordering::SeqCst) >= 3, 1000));
    assert_eq!(seen.load(Ordering::SeqCst), 3); // 1, 2, 5: the two non-measurements skipped

    // get_measurement takes the next sample after the call: keep one coming until it returns
    let done = AtomicBool::new(false);
    let m = std::thread::scope(|sc| {
        sc.spawn(|| {
            while !done.load(Ordering::SeqCst) {
                r.inject_data(lp, 6, 1000);
                std::thread::sleep(Duration::from_millis(5));
            }
        });
        let m = dev.get_measurement(Some(Duration::from_secs(2)));
        done.store(true, Ordering::SeqCst);
        m
    })
    .unwrap();
    assert_eq!(
        (m.timestamp_us, m.echo_time_us, m.source),
        (6, 1000, "loop")
    );

    match dev.get_measurement(Some(Duration::from_millis(30))) {
        Err(DepzError::Timeout { cmd, .. }) => assert_eq!(cmd, Some(Sr04Rpt::Data as u8)),
        other => panic!("expected a timeout, got {other:?}"),
    }
    assert!(!dev.device().is_closed());
}

#[test]
fn short_payloads_are_refused_and_trailing_bytes_ignored() {
    assert!(Sr04Data::unpack(&[0x37; 10]).is_err());
    let mut p = data(0x37, 9, ECHO_TIMEOUT);
    p.push(0xAA);
    let d = Sr04Data::unpack(&p).unwrap();
    assert!(d.is_timeout());
    assert_eq!(d.timestamp_us, 9);
    assert!(sr04::unpack_sample_period(&[1, 2, 3]).is_err());
    assert_eq!(sr04::unpack_sample_period(&[1, 0, 0, 0, 9]).unwrap(), 1);
    assert!(sr04::unpack_echo_decay(&[1]).is_err());
    assert_eq!(sr04::unpack_echo_decay(&[0x88, 0x13]).unwrap(), 5000);
    assert_eq!(sr04::pack_echo_decay(5000), [0x88, 0x13]);
}
