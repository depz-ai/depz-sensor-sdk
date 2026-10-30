//! The dataset recorder, writer, reader and player (contract 09) over fake SR04 boards and temp
//! files — the Java DatasetRecorderTests.
mod common;

use std::path::PathBuf;
use std::sync::atomic::{AtomicU64, Ordering};
use std::sync::{Arc, Mutex};
use std::time::Duration;

use common::fake_board::FakeBoard;
use depz_sensor_sdk::bno086::try_parse_input_cargo;
use depz_sensor_sdk::dataset::{
    records, Dataset, DatasetPlayer, DatasetWriter, PlayerState, SessionRecorder, SCHEMA,
};
use depz_sensor_sdk::devices::{host_now_us, DepzDevice, DepzError, Sr04Device};
use depz_sensor_sdk::json::Value;
use depz_sensor_sdk::protocol::common::{Cmd, Rpt, Status};
use depz_sensor_sdk::protocol::sr04::Sr04Rpt;

const EPS: f64 = 1e-6;

/// A scratch directory removed on drop.
struct TempDir(PathBuf);

impl TempDir {
    fn new() -> TempDir {
        static N: AtomicU64 = AtomicU64::new(0);
        let p = std::env::temp_dir().join(format!(
            "depz-dataset-{}-{}",
            std::process::id(),
            N.fetch_add(1, Ordering::Relaxed)
        ));
        std::fs::create_dir_all(&p).unwrap();
        TempDir(p)
    }

    fn path(&self, name: &str) -> PathBuf {
        self.0.join(name)
    }
}

impl Drop for TempDir {
    fn drop(&mut self) {
        let _ = std::fs::remove_dir_all(&self.0);
    }
}

/// An SR04 board that answers the common commands a recorder needs (sync, serial, name, MCU
/// temperature) on a device clock of its own, and streams a measurement on request.
struct FakeSr04 {
    link: FakeBoard,
    mcu_us: Arc<AtomicU64>,
}

fn text(cmd: u8, s: &str) -> Vec<u8> {
    let mut v = vec![cmd];
    v.extend_from_slice(s.as_bytes());
    v.push(0);
    v
}

impl FakeSr04 {
    fn new(mcu_us: u64) -> FakeSr04 {
        let link = FakeBoard::new();
        let clock = Arc::new(AtomicU64::new(mcu_us));
        let (l, c) = (link.clone(), clock.clone());
        link.on_packet(move |p| {
            let now = c.load(Ordering::SeqCst);
            match p.cmd {
                x if x == Cmd::SyncTime as u8 => {
                    let mut r = p.payload[..8].to_vec();
                    r.extend_from_slice(&now.to_le_bytes());
                    r.extend_from_slice(&now.to_le_bytes());
                    vec![l.reply(Rpt::SyncTime as u8, &r)]
                }
                x if x == Cmd::GetSerial as u8 => {
                    vec![l.reply(Rpt::Text as u8, &text(x, "SN000042"))]
                }
                x if x == Cmd::GetNameActiveSoftware as u8 => {
                    vec![l.reply(Rpt::Text as u8, &text(x, "APP_usonic_SR04_v0.97"))]
                }
                x if x == Cmd::GetMcuTemperature as u8 => {
                    let mut r = vec![0u8; 10];
                    r[8..10].copy_from_slice(&251i16.to_le_bytes());
                    vec![l.reply(Rpt::Temperature as u8, &r)]
                }
                x => vec![FakeBoard::status(x, Status::Ok as u8)],
            }
        });
        FakeSr04 {
            link,
            mcu_us: clock,
        }
    }

    fn device(&self) -> Sr04Device {
        Sr04Device::attach(DepzDevice::new(
            self.link.clone(),
            Some(Duration::from_secs(1)),
        ))
    }

    // 50 ms per sample: device time steps must exceed the gap between the two devices' syncs, or
    // the merged order depends on how fast add() ran.
    fn send(&self, source: u8) {
        let t = self.mcu_us.fetch_add(50_000, Ordering::SeqCst) + 50_000;
        let mut p = vec![source];
        p.extend_from_slice(&t.to_le_bytes());
        p.extend_from_slice(&5831u16.to_le_bytes());
        self.link.inject(self.link.reply(Sr04Rpt::Data as u8, &p));
    }
}

fn until(cond: impl Fn() -> bool, ms: u64) -> bool {
    common::fake_board::until(cond, ms)
}

fn field<'a>(v: &'a depz_sensor_sdk::dataset::RecordValue, k: &str) -> Option<&'a Value> {
    match v {
        depz_sensor_sdk::dataset::RecordValue::Other(o) => o.get(k),
        _ => None,
    }
}

// ── recorder / writer / reader ──────────────────────────────────────────────

#[test]
fn two_devices_land_on_one_host_timeline() {
    let (a, b) = (FakeSr04::new(1_000_000), FakeSr04::new(9_000_000_000));
    let dir = TempDir::new();
    let path = dir.path("rec.depzdata");
    {
        let (dev_a, dev_b) = (a.device(), b.device());
        let mut rec = SessionRecorder::new(&path, Some("unit test"), false, None);
        assert_eq!(rec.add(&dev_a, None, 5).unwrap(), "d0");
        assert_eq!(rec.add(&dev_b, None, 5).unwrap(), "d1");
        rec.start().unwrap();
        assert!(matches!(
            rec.add(&dev_a, None, 5),
            Err(DepzError::InvalidState(_))
        ));
        for (board, source) in [(&a, 0x37), (&b, 0x37), (&a, 0x37), (&b, 0x36)] {
            board.send(source);
            std::thread::sleep(Duration::from_millis(20));
        }
        assert!(until(|| rec.records_written() >= 4, 2000));
        rec.stop().unwrap();
        assert_eq!(rec.records_written(), 4); // the finished file's count
    }
    let ds = Dataset::read(&path).unwrap();
    assert_eq!(
        ds.devices.iter().map(|d| d.id.as_str()).collect::<Vec<_>>(),
        ["d0", "d1"]
    );
    for d in &ds.devices {
        assert_eq!(
            (d.sensor_type.as_deref(), d.serial.as_deref()),
            (Some("sr04"), Some("SN000042"))
        );
    }
    assert_eq!(ds.note.as_deref(), Some("unit test"));
    assert_eq!(
        ds.records
            .iter()
            .map(|r| r.device.as_str())
            .collect::<Vec<_>>(),
        ["d0", "d1", "d0", "d1"]
    );
    match &ds.records[3].value {
        depz_sensor_sdk::dataset::RecordValue::Sr04 { echo_us, source } => {
            assert_eq!((*echo_us, source.as_str()), (5831, "once"));
        }
        v => panic!("{v:?}"),
    }
    // t = device time - offset: both clocks map onto the host clock
    for r in &ds.records {
        assert!(
            (r.t - host_now_us()).abs() < 10_000_000,
            "t on the host clock: {}",
            r.t
        );
    }
}

#[test]
fn the_temperature_sampler_writes_mcu_records_to_a_gzip_file() {
    let a = FakeSr04::new(1_000_000);
    let dir = TempDir::new();
    let path = dir.path("rec.depzdata.gz");
    {
        let dev = a.device();
        let mut rec = SessionRecorder::new(&path, None, false, Some(50.0));
        rec.add(&dev, None, 5).unwrap();
        rec.start().unwrap();
        std::thread::sleep(Duration::from_millis(150));
    }
    let raw = std::fs::read(&path).unwrap();
    assert_eq!(&raw[..2], &[0x1F, 0x8B], ".gz is written as gzip");
    let ds = Dataset::read(&path).unwrap();
    let temps: Vec<_> = ds
        .records
        .iter()
        .filter(|r| r.kind == "temperature")
        .collect();
    assert!(!temps.is_empty(), "temperature records written");
    for t in temps {
        assert_eq!(
            t.value,
            depz_sensor_sdk::dataset::RecordValue::Temperature { celsius: 25.1 }
        );
    }
}

#[test]
fn writer_and_reader_round_trip_and_sort_within_a_device() {
    let dir = TempDir::new();
    let path = dir.path("rt.depzdata");
    {
        let w = DatasetWriter::create(
            &path,
            vec![
                ("d0".into(), Value::Object(vec![])),
                ("d1".into(), Value::Object(vec![])),
            ],
            None,
        )
        .unwrap();
        for (d, t) in [
            ("d0", 300),
            ("d0", 100),
            ("d1", 200),
            ("d0", 250),
            ("d1", 50),
        ] {
            let v = Value::Object(vec![(
                "arr".into(),
                Value::Array(vec![Value::int(1), Value::float(2.5)]),
            )]);
            w.write(d, t, "event", v).unwrap();
        }
        assert_eq!(w.records_written(), 5);
        w.close().unwrap();
        assert!(matches!(
            w.write("d0", 400, "event", Value::Object(vec![])),
            Err(DepzError::InvalidState(_))
        ));
        assert!(w.write("d0", 1, "event", Value::Null).is_err(), "closed");
    }
    let ds = Dataset::read(&path).unwrap();
    assert_eq!(ds.schema, SCHEMA);
    let order: Vec<String> = ds
        .records
        .iter()
        .map(|r| format!("{}:{}", r.device, r.t))
        .collect();
    assert_eq!(order, ["d1:50", "d0:100", "d1:200", "d0:250", "d0:300"]);
    assert_eq!(ds.duration_us(), 250);
    assert_eq!(
        field(&ds.records[0].value, "arr").unwrap().to_json(),
        "[1,2.5]"
    );
}

#[test]
fn the_bno086_record_of_every_report_matches_the_vectors() {
    let doc = common::load("bno086_reports.json");
    let mut n = 0;
    for c in doc["input_cargos"].as_array().unwrap() {
        let cargo = common::hex_decode(c["cargo"].as_str().unwrap());
        let reports =
            try_parse_input_cargo(&cargo, c["capture_timestamp_us"].as_i64().unwrap()).unwrap();
        let want = c["expect"].as_array().unwrap();
        assert_eq!(reports.len(), want.len(), "{}", c["id"]);
        for (r, e) in reports.iter().zip(want) {
            let got: serde_json::Value =
                serde_json::from_str(&records::bno086(r).to_json()).unwrap();
            assert_eq!(got, e["record"], "{}", c["id"]);
            n += 1;
        }
    }
    assert!(n > 10);
}

// ── player ──────────────────────────────────────────────────────────────────

/// A fake monotonic clock, seconds.
#[derive(Clone)]
struct FakeClock(Arc<Mutex<f64>>);

impl FakeClock {
    fn new() -> FakeClock {
        FakeClock(Arc::new(Mutex::new(100.0)))
    }

    fn add(&self, s: f64) {
        *self.0.lock().unwrap() += s;
    }

    fn boxed(&self) -> Box<dyn Fn() -> f64 + Send + Sync> {
        let c = self.0.clone();
        Box::new(move || *c.lock().unwrap())
    }
}

fn write(dir: &TempDir, events: &[(&str, i64)]) -> Dataset {
    static N: AtomicU64 = AtomicU64::new(0);
    let path = dir.path(&format!("p{}.depzdata", N.fetch_add(1, Ordering::Relaxed)));
    let devices = vec![
        ("d0".to_string(), Value::Object(vec![])),
        ("d1".to_string(), Value::Object(vec![])),
    ];
    let w = DatasetWriter::create(&path, devices, Some("test")).unwrap();
    for (i, (d, t)) in events.iter().enumerate() {
        w.write(
            d,
            *t,
            "event",
            Value::Object(vec![("n".into(), Value::int(i as i64))]),
        )
        .unwrap();
    }
    w.close().unwrap();
    Dataset::read(&path).unwrap()
}

fn sample(dir: &TempDir) -> Dataset {
    write(
        dir,
        &[("d0", 1000), ("d1", 900), ("d0", 51000), ("d1", 30900)],
    )
}

fn evenly(dir: &TempDir, n: i64, gap_us: i64) -> Dataset {
    let events: Vec<(&str, i64)> = (0..n).map(|i| ("d0", 1_000_000 + i * gap_us)).collect();
    write(dir, &events)
}

struct Made {
    p: DatasetPlayer,
    clock: FakeClock,
    seen: Arc<Mutex<Vec<i64>>>,
    states: Arc<Mutex<Vec<PlayerState>>>,
    _hooks: Vec<depz_sensor_sdk::devices::Hook>,
}

fn make(ds: &Dataset, speed: f64) -> Made {
    let clock = FakeClock::new();
    let p = DatasetPlayer::new(ds, speed, Some(clock.boxed()), false).unwrap();
    let seen = Arc::new(Mutex::new(Vec::new()));
    let states = Arc::new(Mutex::new(Vec::new()));
    let (s, st) = (seen.clone(), states.clone());
    let hooks = vec![
        p.on_record(move |r| s.lock().unwrap().push(r.t)),
        p.on_state(move |x| st.lock().unwrap().push(x)),
    ];
    Made {
        p,
        clock,
        seen,
        states,
        _hooks: hooks,
    }
}

impl Made {
    fn seen(&self) -> Vec<i64> {
        self.seen.lock().unwrap().clone()
    }

    fn states(&self) -> Vec<PlayerState> {
        self.states.lock().unwrap().clone()
    }
}

fn near(expected: f64, got: Option<f64>, what: &str) {
    assert!(
        got.is_some_and(|g| (g - expected).abs() < 1e-9),
        "{what}: expected {expected}, got {got:?}"
    );
}

use PlayerState::{Done, Idle, Paused, Playing};

#[test]
fn plays_paced_pauses_resumes_seeks() {
    let dir = TempDir::new();
    let m = make(&sample(&dir), 1.0);
    let p = &m.p;
    assert_eq!(
        (p.state(), p.position_us(), p.duration_us()),
        (Idle, 0, 50100)
    );
    p.play().unwrap();
    assert!(m.seen().is_empty(), "nothing before the first tick");
    let wait = p.tick();
    assert_eq!(m.seen(), [900]);
    near(100e-6, wait, "wait");
    m.clock.add(100e-6 + EPS);
    p.tick();
    assert_eq!(m.seen(), [900, 1000]);
    m.clock.add(0.0299 - 2.0 * EPS);
    p.tick();
    assert_eq!(m.seen().len(), 2);
    m.clock.add(2.0 * EPS);
    p.tick();
    assert_eq!(m.seen(), [900, 1000, 30900]);
    assert_eq!(p.position_us(), 30000);

    p.pause();
    m.clock.add(1.0);
    assert_eq!(p.tick(), None);
    assert_eq!(m.seen().len(), 3);

    p.play().unwrap(); // resume: the next record waits its full gap (20.1 ms) again
    m.clock.add(0.0201 - EPS);
    p.tick();
    assert_eq!(m.seen().len(), 3);
    m.clock.add(2.0 * EPS);
    assert_eq!(p.tick(), None);
    assert_eq!(m.seen(), [900, 1000, 30900, 51000]);
    assert_eq!((p.state(), p.position_us()), (Done, 50100));
    assert_eq!(m.states(), [Playing, Paused, Playing, Done]);

    m.seen.lock().unwrap().clear();
    p.seek_us(30000.0).unwrap();
    assert_eq!((p.state(), p.position_us()), (Done, 100));
    p.set_speed(0.0).unwrap();
    p.play().unwrap();
    p.tick();
    assert_eq!(m.seen(), [30900, 51000]);
}

#[test]
fn a_speed_change_mid_wait_rescales_the_remainder_and_a_slow_consumer_does_not_drift() {
    let dir = TempDir::new();
    let m = make(&evenly(&dir, 5, 100_000), 1.0);
    m.p.play().unwrap();
    m.p.tick();
    m.clock.add(0.05);
    m.p.tick();
    m.p.set_speed(0.5).unwrap(); // remaining 50 ms of media → 100 ms of wall time
    near(0.1, m.p.tick(), "rescaled wait");
    m.clock.add(0.1 + EPS);
    m.p.tick();
    assert_eq!(m.seen().len(), 2);

    let q = make(&evenly(&dir, 5, 100_000), 1.0);
    q.p.play().unwrap();
    q.p.tick();
    q.clock.add(0.35); // records 1..3 overdue
    let w = q.p.tick();
    assert_eq!(q.seen().len(), 4);
    near(0.05, w, "wait after catching up");
    assert!(matches!(
        q.p.set_speed(-1.0),
        Err(DepzError::InvalidArgument(_))
    ));
    assert!(matches!(
        q.p.set_speed(f64::NAN),
        Err(DepzError::InvalidArgument(_))
    ));
    assert!(DatasetPlayer::new(&evenly(&dir, 1, 1), f64::INFINITY, None, false).is_err());
}

#[test]
fn seek_while_paused_skips_and_past_the_end_ends_playback() {
    let dir = TempDir::new();
    let m = make(&evenly(&dir, 5, 100_000), 1.0);
    m.p.play().unwrap();
    m.p.tick();
    m.p.pause();
    m.p.seek_us(250_000.0).unwrap();
    assert_eq!((m.p.position_us(), m.p.state()), (200_000, Paused));
    m.p.play().unwrap();
    near(0.1, m.p.tick(), "wait after seek");
    m.clock.add(0.1);
    m.p.tick();
    assert_eq!(m.seen(), [1_000_000, 1_300_000]);

    let q = make(&evenly(&dir, 5, 100_000), 1.0);
    q.p.play().unwrap();
    q.p.tick();
    q.p.seek_us(1e9).unwrap();
    assert_eq!((q.p.state(), q.p.position_us()), (Done, 400_000));
    assert_eq!(q.states(), [Playing, Done]);
    assert!(matches!(
        q.p.seek_us(f64::NAN),
        Err(DepzError::InvalidArgument(_))
    ));
}

fn dual_sr04() -> Dataset {
    Dataset::read(common::recordings_dir().join("dataset_dual_sr04.depzdata")).unwrap()
}

// Contract 09 §Player: a panicking record / state callback is counted and dropped; the other
// callbacks still get every record and playback runs to the end (tick mode).
#[test]
fn a_panicking_callback_is_counted_and_playback_goes_on() {
    let ds = dual_sr04();
    let p = DatasetPlayer::new(&ds, 0.0, Some(FakeClock::new().boxed()), false).unwrap();
    let first = ds.records[0].t;
    let (seen, states) = (
        Arc::new(Mutex::new(Vec::new())),
        Arc::new(Mutex::new(Vec::new())),
    );
    let (s, st) = (seen.clone(), states.clone());
    let _hooks = [
        p.on_record(move |r| {
            if r.t == first {
                panic!("consumer failed");
            }
        }),
        p.on_record(move |r| s.lock().unwrap().push(r.t)),
        p.on_state(|_| panic!("state consumer failed")),
        p.on_state(move |x| st.lock().unwrap().push(x)),
    ];
    p.play().unwrap();
    assert_eq!(p.tick(), None);
    let want: Vec<i64> = ds.records.iter().map(|r| r.t).collect();
    assert_eq!(want.len(), 10);
    assert_eq!(*seen.lock().unwrap(), want);
    assert_eq!(p.state(), Done);
    assert_eq!(*states.lock().unwrap(), [Playing, Done]);
    assert_eq!(
        p.callback_errors(),
        3,
        "the first record + two state changes"
    );
}

#[test]
fn callbacks_may_control_the_player() {
    let dir = TempDir::new();
    // A record callback may call the player (pause here).
    let q = Arc::new(
        DatasetPlayer::new(
            &evenly(&dir, 5, 100_000),
            0.0,
            Some(FakeClock::new().boxed()),
            false,
        )
        .unwrap(),
    );
    let (seen, states) = (
        Arc::new(Mutex::new(Vec::new())),
        Arc::new(Mutex::new(Vec::new())),
    );
    let (weak, s, st) = (Arc::downgrade(&q), seen.clone(), states.clone());
    let _h2 = q.on_record(move |r| {
        s.lock().unwrap().push(r.t);
        if r.t == 1_100_000 {
            weak.upgrade().unwrap().pause();
        }
    });
    let _h3 = q.on_state(move |x| st.lock().unwrap().push(x));
    q.play().unwrap();
    assert_eq!(q.tick(), None);
    assert_eq!(*seen.lock().unwrap(), [1_000_000, 1_100_000]);
    assert_eq!(*states.lock().unwrap(), [Playing, Paused]);
}

#[test]
fn empty_dataset_and_unsubscribe_and_close() {
    let dir = TempDir::new();
    let m = make(&write(&dir, &[]), 1.0);
    m.p.play().unwrap();
    assert_eq!(m.states(), [Playing, Done]);
    m.p.seek_us(100.0).unwrap();
    assert_eq!(m.p.position_us(), 0);

    let q = DatasetPlayer::new(
        &evenly(&dir, 5, 100_000),
        0.0,
        Some(FakeClock::new().boxed()),
        false,
    )
    .unwrap();
    let got = Arc::new(AtomicU64::new(0));
    let g = got.clone();
    let sub = q.on_record(move |_| {
        g.fetch_add(1, Ordering::Relaxed);
    });
    drop(sub);
    q.play().unwrap();
    q.tick();
    assert_eq!(got.load(Ordering::Relaxed), 0);
    assert!(matches!(
        q.wait(Some(Duration::ZERO)),
        Err(DepzError::InvalidState(_))
    ));
    q.close();
    q.close();
    assert!(matches!(q.play(), Err(DepzError::InvalidState(_))));
}

#[test]
fn threaded_delivers_on_the_player_thread_and_pauses_from_another() {
    let dir = TempDir::new();
    {
        let p = DatasetPlayer::new(&evenly(&dir, 4, 20_000), 1.0, None, true).unwrap();
        let names = Arc::new(Mutex::new(Vec::new()));
        let n = names.clone();
        let _h = p.on_record(move |_| {
            n.lock()
                .unwrap()
                .push(std::thread::current().name().map(str::to_string))
        });
        p.play().unwrap();
        assert!(
            p.wait(Some(Duration::from_secs(5))).unwrap(),
            "done in time"
        );
        assert_eq!(p.state(), Done);
        let names = names.lock().unwrap();
        assert_eq!(names.len(), 4);
        assert!(names
            .iter()
            .all(|n| n.as_deref() == Some(depz_sensor_sdk::dataset::PLAYER_THREAD_NAME)));
    }
    let q = DatasetPlayer::new(&evenly(&dir, 50, 100_000), 1.0, None, true).unwrap();
    let seen = Arc::new(Mutex::new(Vec::new()));
    let s = seen.clone();
    let _h = q.on_record(move |r| s.lock().unwrap().push(r.t));
    q.play().unwrap();
    assert!(
        until(|| !seen.lock().unwrap().is_empty(), 5000),
        "a first record"
    );
    q.pause();
    let n = seen.lock().unwrap().len();
    assert!(
        q.wait(Some(Duration::ZERO)).unwrap(),
        "not playing after pause"
    );
    q.seek_us(4_000_000.0).unwrap();
    std::thread::sleep(Duration::from_millis(50));
    assert_eq!(
        seen.lock().unwrap().len(),
        n,
        "nothing delivered while paused"
    );
    q.set_speed(0.0).unwrap();
    q.play().unwrap();
    assert!(
        q.wait(Some(Duration::from_secs(5))).unwrap(),
        "done in time"
    );
    let got = seen.lock().unwrap();
    let want: Vec<i64> = (40..50).map(|i| 1_000_000 + i * 100_000).collect();
    assert_eq!(&got[n..], &want[..]);
}

#[test]
fn threaded_a_panicking_callback_does_not_stop_playback() {
    let ds = dual_sr04();
    let p = DatasetPlayer::new(&ds, 0.0, None, true).unwrap();
    let first = ds.records[0].t;
    let seen = Arc::new(Mutex::new(Vec::new()));
    let s = seen.clone();
    let _h1 = p.on_record(move |r| {
        if r.t == first {
            panic!("consumer failed");
        }
    });
    let _h2 = p.on_record(move |r| s.lock().unwrap().push(r.t));
    p.play().unwrap();
    assert!(
        p.wait(Some(Duration::from_secs(5))).unwrap(),
        "done in time"
    );
    assert_eq!(p.state(), Done);
    let want: Vec<i64> = ds.records.iter().map(|r| r.t).collect();
    assert_eq!(*seen.lock().unwrap(), want);
    assert_eq!(p.callback_errors(), 1);
}

/// Bug hunt 2026-10-29 RUST-6: a tiny speed stretches a gap beyond what a `Duration` holds; the
/// player thread waits in capped slices instead of dying under its lock, so `wait()` keeps working
/// and a later speed change plays the rest.
#[test]
fn player_survives_a_tiny_speed() {
    let ds = Dataset::parse(
        "{\"schema\":\"depz.dataset/2\"}\n{\"d\":\"d0\",\"t\":0,\"k\":\"x\",\"v\":{}}\n{\"d\":\"d0\",\"t\":1000000,\"k\":\"x\",\"v\":{}}\n",
    )
    .unwrap();
    let p = DatasetPlayer::new(&ds, 1e-300, None, true).unwrap();
    let n = Arc::new(AtomicU64::new(0));
    let n2 = n.clone();
    let _h = p.on_record(move |_| {
        n2.fetch_add(1, Ordering::SeqCst);
    });
    p.play().unwrap();
    assert!(!p.wait(Some(Duration::from_millis(200))).unwrap());
    assert_eq!(p.state(), PlayerState::Playing);
    p.set_speed(0.0).unwrap();
    assert!(p.wait(Some(Duration::from_secs(5))).unwrap());
    assert_eq!(n.load(Ordering::SeqCst), 2);
    assert_eq!(p.state(), PlayerState::Done);
}
