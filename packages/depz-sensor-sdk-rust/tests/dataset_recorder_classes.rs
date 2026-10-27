//! The dataset recorder (contract 09) over every device class it records, opened as `AnyDevice`:
//! each class's header entry (sensor type from its software name), and the records of the streams
//! a fake board can feed without a sensor behind it — VL53L4CD result blocks, BNO055 samples and
//! BNO086 input reports, VL53L8 frames (on the tests/common VL53L8 fake) with their layers and the
//! sensor-die temperature. A plain device is recorded in the header with no stream hooked.
mod common;

use std::path::PathBuf;
use std::sync::atomic::{AtomicU64, Ordering};
use std::time::Duration;

use common::fake_board::FakeBoard;
use depz_sensor_sdk::bno055::regs as r;
use depz_sensor_sdk::bno055::Bno055Rpt;
use depz_sensor_sdk::bno086::{hub, shtp, ShtpChannel};
use depz_sensor_sdk::dataset::{Dataset, Record, RecordValue, SessionRecorder};
use depz_sensor_sdk::devices::{attach, AnyDevice, DepzDevice};
use depz_sensor_sdk::json::Value;
use depz_sensor_sdk::protocol::common::{Cmd, Rpt, Status};
use depz_sensor_sdk::vl53l4::Vl53l4Rpt;

/// A scratch file removed on drop.
struct TempFile(PathBuf);

impl TempFile {
    fn new(name: &str) -> TempFile {
        static N: AtomicU64 = AtomicU64::new(0);
        TempFile(std::env::temp_dir().join(format!(
            "depz-rec-classes-{}-{}-{name}",
            std::process::id(),
            N.fetch_add(1, Ordering::Relaxed)
        )))
    }
}

impl Drop for TempFile {
    fn drop(&mut self) {
        let _ = std::fs::remove_file(&self.0);
    }
}

fn text(cmd: u8, s: &str) -> Vec<u8> {
    let mut v = vec![cmd];
    v.extend_from_slice(s.as_bytes());
    v.push(0);
    v
}

/// A board that answers what `SessionRecorder::add` asks (sync, serial, software name, MCU
/// temperature) on a device clock standing at 1 s, and acknowledges everything else.
fn board(software_name: &'static str) -> FakeBoard {
    let link = FakeBoard::new();
    let l = link.clone();
    link.on_packet(move |p| match p.cmd {
        x if x == Cmd::SyncTime as u8 => {
            let mut r = p.payload[..8].to_vec();
            r.extend_from_slice(&1_000_000u64.to_le_bytes());
            r.extend_from_slice(&1_000_000u64.to_le_bytes());
            vec![l.reply(Rpt::SyncTime as u8, &r)]
        }
        x if x == Cmd::GetSerial as u8 => vec![l.reply(Rpt::Text as u8, &text(x, "SN7"))],
        x if x == Cmd::GetNameActiveSoftware as u8 => {
            vec![l.reply(Rpt::Text as u8, &text(x, software_name))]
        }
        x if x == Cmd::GetMcuTemperature as u8 => {
            let mut r = vec![0u8; 10];
            r[8..10].copy_from_slice(&300i16.to_le_bytes());
            vec![l.reply(Rpt::Temperature as u8, &r)]
        }
        x => vec![FakeBoard::status(x, Status::Ok as u8)],
    });
    link
}

fn open(class: &str, software_name: &'static str) -> (AnyDevice, FakeBoard) {
    let link = board(software_name);
    let dev = attach(
        DepzDevice::new(link.clone(), Some(Duration::from_secs(1))),
        class,
    )
    .unwrap();
    (dev, link)
}

fn field<'a>(r: &'a Record, k: &str) -> &'a Value {
    match &r.value {
        RecordValue::Other(o) => o.get(k).unwrap_or_else(|| panic!("{k} in {r:?}")),
        v => panic!("{v:?}"),
    }
}

/// Record `class` while `feed` streams into its board; the dataset read back.
fn record(class: &str, software_name: &'static str, feed: impl Fn(&FakeBoard)) -> Dataset {
    let file = TempFile::new("rec.depzdata");
    {
        let (dev, link) = open(class, software_name);
        let mut rec = SessionRecorder::new(&file.0, None, false, None);
        assert_eq!(rec.add(&dev, Some(class), 3).unwrap(), class);
        rec.start().unwrap();
        feed(&link);
        assert!(
            common::fake_board::until(|| rec.records_written() >= 1, 2000),
            "{class}: no record written"
        );
        rec.stop().unwrap();
    }
    Dataset::read(&file.0).unwrap()
}

#[test]
fn vl53l4cd_result_blocks_become_vl53l4_records() {
    let ds = record("vl53l4cd", "APP_VL53L4_v0.80", |link| {
        let mut block = [0u8; 17];
        block[0] = 9; // raw status 9: valid
        block[13..15].copy_from_slice(&258u16.to_be_bytes());
        let mut p = 1_050_000u64.to_le_bytes().to_vec();
        p.extend_from_slice(&0x0089u16.to_le_bytes());
        p.extend_from_slice(&17u16.to_le_bytes());
        p.extend_from_slice(&block);
        link.inject(link.reply(Vl53l4Rpt::Stream as u8, &p));
    });
    assert_eq!(ds.devices[0].sensor_type.as_deref(), Some("vl53l4"));
    assert_eq!(ds.devices[0].serial.as_deref(), Some("SN7"));
    let r = &ds.records[0];
    assert_eq!((r.device.as_str(), r.kind.as_str()), ("vl53l4cd", "vl53l4"));
    assert_eq!(field(r, "distance_mm").as_i64(), Some(258));
    assert_eq!(field(r, "range_status").as_i64(), Some(0));
}

#[test]
fn bno055_samples_become_bno055_records() {
    let ds = record("bno055", "APP_BNO055_v0.12", |link| {
        let mut p = vec![0u8; 16];
        p[0..8].copy_from_slice(&1_050_000u64.to_le_bytes());
        p[8] = r::REG_ACC_DATA;
        p[9] = 6;
        p[10..12].copy_from_slice(&100i16.to_le_bytes());
        link.inject(link.reply(Bno055Rpt::Stream as u8, &p));
    });
    assert_eq!(ds.devices[0].sensor_type.as_deref(), Some("bno055"));
    let r = &ds.records[0];
    assert_eq!(r.kind, "bno055");
    // 100 LSB at the power-on unit, m/s² (100 LSB per m/s²)
    let accel = field(r, "accel").as_array().unwrap();
    assert_eq!(accel[0].as_f64(), Some(1.0));
}

#[test]
fn bno086_input_reports_become_bno086_records() {
    let ds = record("bno086", "APP_BNO086_v0.95", |link| {
        let cargo = [
            &[0xFB, 0, 0, 0, 0][..],
            &[0x01, 0, 0, 0, 1, 0, 2, 0, 3, 0][..], // accelerometer
        ]
        .concat();
        let frame = shtp::try_build_frame(ShtpChannel::InputNormal as u8, &cargo, 0).unwrap();
        let mut p = vec![0u8; 9];
        p[1..9].copy_from_slice(&1_050_000u64.to_le_bytes());
        p.extend_from_slice(&frame);
        link.inject(link.reply(hub::RPT_DATA, &p));
    });
    assert_eq!(ds.devices[0].sensor_type.as_deref(), Some("bno086"));
    let r = &ds.records[0];
    assert_eq!(r.kind, "bno086");
    assert_eq!(field(r, "sensor_id").as_i64(), Some(1));
}

#[test]
fn every_class_is_recorded_in_the_header_with_its_sensor_type() {
    let classes = [
        ("sr04", "APP_usonic_SR04_v0.95", "sr04"),
        ("vl53l4cd", "APP_VL53L4_v0.80", "vl53l4"),
        ("bno055", "APP_BNO055_v0.12", "bno055"),
        ("bno086", "APP_BNO086_v0.95", "bno086"),
        ("tmf8829", "APP_TMF8829_v0.52", "tmf8829"),
        ("vl53l8cx", "APP_VL53L8_v0.9", "vl53l8"),
        ("vl53l8ch", "APP_VL53L8_v0.9", "vl53l8"),
        ("vl53l5cx", "APP_VL53L7_v0.53", "vl53l7"),
        ("vl53l7cx", "APP_VL53L7_v0.53", "vl53l7"),
        ("vl53l7ch", "APP_VL53L7_v0.53", "vl53l7"),
        ("vl53lx", "APP_VL53LX_v0.20", "vl53lx"),
        ("vl53l0x", "APP_VL53L0_4_v0.23", "vl53lx"),
        ("vl53l1cx", "APP_VL53L0_4_v0.23", "vl53lx"),
        ("vl53l1cb", "APP_VL53L0_4_v0.23", "vl53lx"),
        ("vl53l3cx", "APP_VL53L0_4_v0.23", "vl53lx"),
        ("vl53l4cx", "APP_VL53L0_4_v0.23", "vl53lx"),
        ("device", "APP_NEWSENSOR_v1.2", "unknown"),
    ];
    let file = TempFile::new("all.depzdata");
    {
        let devs: Vec<(AnyDevice, FakeBoard)> =
            classes.iter().map(|(c, sw, _)| open(c, sw)).collect();
        let mut rec = SessionRecorder::new(&file.0, Some("every class"), true, None);
        for (i, (dev, _)) in devs.iter().enumerate() {
            assert_eq!(rec.add(dev, None, 2).unwrap(), format!("d{i}"));
        }
        rec.start().unwrap();
        rec.start().unwrap(); // a no-op while recording
        rec.stop().unwrap();
        rec.stop().unwrap(); // idempotent
        assert_eq!(rec.records_written(), 0);
    }
    let ds = Dataset::read(&file.0).unwrap();
    assert_eq!(ds.note.as_deref(), Some("every class"));
    assert_eq!(ds.devices.len(), classes.len());
    for (d, (class, sw, sensor_type)) in ds.devices.iter().zip(classes) {
        assert_eq!(d.sensor_type.as_deref(), Some(sensor_type), "{class}");
        assert_eq!(d.software_name.as_deref(), Some(sw), "{class}");
    }
    assert!(ds.records.is_empty());
}

#[test]
fn vl53l8_frames_become_records_with_layers_and_the_sensor_temperature() {
    use common::fake_vl53l8::{FakeVl53l8, SILICON_TEMP_DEGC};
    let fake = FakeVl53l8::new();
    let dev = attach(
        DepzDevice::new(fake.link(), Some(Duration::from_secs(1))),
        "vl53l8cx",
    )
    .unwrap();
    let AnyDevice::Vl53l8cx(l8) = &dev else {
        panic!("{}", dev.class_name())
    };
    l8.init().unwrap();
    let file = TempFile::new("l8.depzdata");
    {
        let mut rec = SessionRecorder::new(&file.0, None, true, Some(200.0));
        rec.add(&dev, None, 3).unwrap();
        l8.start_ranging().unwrap();
        rec.start().unwrap();
        fake.send_frame();
        // the frame record, then both temperature sources: the MCU and the sensor die
        assert!(common::fake_board::until(
            || rec.records_written() >= 3,
            2000
        ));
        rec.stop().unwrap();
        l8.stop_ranging().unwrap();
    }
    let ds = Dataset::read(&file.0).unwrap();
    assert_eq!(ds.devices[0].sensor_type.as_deref(), Some("vl53l8"));
    let frame = ds.records.iter().find(|r| r.kind == "vl53l8").unwrap();
    assert_eq!(field(frame, "distance_mm").as_array().unwrap().len(), 16);
    assert_eq!(
        field(frame, "silicon_temp_degc").as_i64(),
        Some(SILICON_TEMP_DEGC as i64)
    );
    for layer in [
        "signal_per_spad",
        "ambient_per_spad",
        "range_sigma_mm",
        "reflectance",
    ] {
        assert_eq!(field(frame, layer).as_array().unwrap().len(), 16, "{layer}");
    }
    let temps: Vec<f64> = ds
        .records
        .iter()
        .filter_map(|r| match r.value {
            RecordValue::Temperature { celsius } => Some(celsius),
            _ => None,
        })
        .collect();
    assert!(temps.contains(&27.3), "MCU temperature: {temps:?}");
    assert!(
        temps.contains(&(SILICON_TEMP_DEGC as f64)),
        "sensor die: {temps:?}"
    );
}
