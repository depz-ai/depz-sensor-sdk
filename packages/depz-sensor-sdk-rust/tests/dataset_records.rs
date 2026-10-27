//! The contract 09 record values (`v`) of the live layer, key for key the Python
//! SessionRecorder's: sr04, vl53l4, vl53l8 (zone arrays cut to the resolution, the four layers
//! on request, the motion block when present), vl53l8_cnh (base64, start_bin =
//! floor(offset / 2048)), vl53lx with and without targets, bno055 with every channel and with
//! one. The same inputs and values as the C and C++ suites' dataset_live_records.

use std::collections::BTreeMap;

use depz_sensor_sdk::bno055::{CalibStatus, Units};
use depz_sensor_sdk::dataset::records as rec;
use depz_sensor_sdk::devices::{
    Bno055Sample, Sr04Measurement, Vl53l4Measurement, Vl53lxMeasurement,
};
use depz_sensor_sdk::json::Value;
use depz_sensor_sdk::vl53l8::driver::{Vl53l8CnhSetup, Vl53l8Frame, Vl53l8Motion};
use depz_sensor_sdk::vl53lx::driver::Vl53lxTarget;

/// `got` is the JSON text `want`, numbers spelled as Python writes them (`2.0`, not `2`).
fn assert_record(got: Value, want: &str) {
    let want = Value::parse(want).expect("the expected value is JSON");
    assert_eq!(got.to_json(), want.to_json());
}

#[test]
fn sr04_and_vl53l4() {
    let m = |echo, source| Sr04Measurement {
        timestamp_us: 1000,
        echo_time_us: echo,
        source,
    };
    assert_record(
        rec::sr04(&m(5831, "loop")),
        r#"{"echo_us": 5831, "source": "loop"}"#,
    );
    assert_record(
        rec::sr04(&m(65535, "once")),
        r#"{"echo_us": 65535, "source": "once"}"#,
    );

    let l4 = Vl53l4Measurement {
        timestamp_us: 0,
        range_status: 0,
        distance_mm: 1234,
        sigma_mm: 3,
        signal_rate_kcps: 120,
        ambient_rate_kcps: 7,
        signal_per_spad_kcps: 1,
        ambient_per_spad_kcps: 1,
        number_of_spad: 180,
        stream_count: 9,
    };
    assert_record(
        rec::vl53l4(&l4),
        r#"{"range_status": 0, "distance_mm": 1234, "sigma_mm": 3, "signal_rate_kcps": 120,
            "ambient_rate_kcps": 7, "number_of_spad": 180, "stream_count": 9}"#,
    );
}

/// A 4×4 frame in 64-zone buffers: the record keeps the resolution's 16 zones.
fn frame() -> Vl53l8Frame {
    let n = 64;
    Vl53l8Frame {
        timestamp_us: 0,
        resolution: 16,
        silicon_temp_degc: 31,
        distance_mm: (0..n).map(|i| 100 + 10 * i).collect(),
        target_status: vec![5; n as usize],
        nb_target_detected: (0..n).map(|i| u8::from(i < 8)).collect(),
        signal_per_spad: (0..n).map(|i| i as f64 + 0.5).collect(),
        ambient_per_spad: (0..n).map(|i| i as f64 + 0.25).collect(),
        nb_spads_enabled: vec![0; n as usize],
        range_sigma_mm: (0..n).map(|i| i as f64 + 0.75).collect(),
        reflectance: (0..n).map(|i| 2.0 * i as f64).collect(),
        cnh_raw: None,
        motion: None,
    }
}

const ZONES: &str = r#""resolution": 16, "silicon_temp_degc": 31,
    "distance_mm": [100, 110, 120, 130, 140, 150, 160, 170, 180, 190, 200, 210, 220, 230, 240, 250],
    "target_status": [5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5],
    "nb_target_detected": [1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0]"#;

#[test]
fn vl53l8_zones_layers_and_motion() {
    let mut f = frame();
    assert_record(rec::vl53l8(&f, false), &format!("{{{ZONES}}}"));
    assert_record(
        rec::vl53l8(&f, true),
        &format!(
            r#"{{{ZONES},
            "signal_per_spad": [0.5, 1.5, 2.5, 3.5, 4.5, 5.5, 6.5, 7.5, 8.5, 9.5, 10.5, 11.5, 12.5, 13.5, 14.5, 15.5],
            "ambient_per_spad": [0.25, 1.25, 2.25, 3.25, 4.25, 5.25, 6.25, 7.25, 8.25, 9.25, 10.25, 11.25, 12.25, 13.25, 14.25, 15.25],
            "range_sigma_mm": [0.75, 1.75, 2.75, 3.75, 4.75, 5.75, 6.75, 7.75, 8.75, 9.75, 10.75, 11.75, 12.75, 13.75, 14.75, 15.75],
            "reflectance": [0.0, 2.0, 4.0, 6.0, 8.0, 10.0, 12.0, 14.0, 16.0, 18.0, 20.0, 22.0, 24.0, 26.0, 28.0, 30.0]}}"#
        ),
    );
    f.motion = Some(Vl53l8Motion {
        global_indicator_1: 7,
        global_indicator_2: 9,
        status: 0,
        nb_of_detected_aggregates: 2,
        nb_of_aggregates: 4,
        motion: (0..32).map(|i| 3 * i).collect(),
    });
    assert_record(
        rec::vl53l8(&f, false),
        &format!(
            r#"{{{ZONES}, "motion": {{"global_indicator_1": 7, "global_indicator_2": 9, "status": 0,
            "nb_of_detected_aggregates": 2, "nb_of_aggregates": 4,
            "motion": [0, 3, 6, 9, 12, 15, 18, 21, 24, 27, 30, 33, 36, 39, 42, 45, 48, 51, 54, 57,
                       60, 63, 66, 69, 72, 75, 78, 81, 84, 87, 90, 93]}}}}"#
        ),
    );
}

#[test]
fn vl53l8_cnh_base64_and_floor_start_bin() {
    let mut setup = Vl53l8CnhSetup::new();
    setup.init_config(-3, 6, 2).unwrap(); // ref_bin_offset -6144: floor, not truncation
    let raw = [1u8, 2, 3];
    assert_record(
        rec::vl53l8_cnh(&raw, &setup),
        r#"{"nb_aggregates": 0, "feature_length": 6, "sum_span": 2, "start_bin": -3, "cnh_raw_b64": "AQID"}"#,
    );
    setup.init_config(2, 6, 2).unwrap();
    for (bytes, b64) in [(&raw[..2], "AQI="), (&raw[..1], "AQ=="), (&raw[..0], "")] {
        assert_record(
            rec::vl53l8_cnh(bytes, &setup),
            &format!(
                r#"{{"nb_aggregates": 0, "feature_length": 6, "sum_span": 2, "start_bin": 2, "cnh_raw_b64": "{b64}"}}"#
            ),
        );
    }
}

fn target(distance_mm: i32, status: i32, signal_kcps: f64) -> Vl53lxTarget {
    Vl53lxTarget {
        distance_mm,
        status,
        status_text: String::new(),
        signal_kcps,
        ambient_kcps: 0.0,
        sigma_mm: 0.0,
        min_range_mm: 0,
        max_range_mm: 0,
    }
}

#[test]
fn vl53lx_with_and_without_targets() {
    let mut m = Vl53lxMeasurement {
        timestamp_us: 0,
        distance_mm: 1234,
        status: 0,
        status_text: String::new(),
        signal_kcps: 120.25,
        ambient_kcps: 3.5,
        sigma_mm: 2.5,
        spads: 180.0,
        targets: Vec::new(),
        extra: BTreeMap::new(),
        bins: None,
    };
    assert_record(
        rec::vl53lx(&m, "vl53l1cb", "uld"),
        r#"{"product": "vl53l1cb", "driver": "uld", "status": 0, "distance_mm": 1234,
            "sigma_mm": 2.5, "signal_kcps": 120.25, "ambient_kcps": 3.5, "spads": 180.0}"#,
    );
    m.targets = vec![target(1234, 0, 120.25), target(2100, 6, 8.5)];
    assert_record(
        rec::vl53lx(&m, "", ""),
        r#"{"product": "", "driver": "", "status": 0, "distance_mm": 1234,
            "sigma_mm": 2.5, "signal_kcps": 120.25, "ambient_kcps": 3.5, "spads": 180.0,
            "targets": [{"distance_mm": 1234, "status": 0, "signal_kcps": 120.25},
                        {"distance_mm": 2100, "status": 6, "signal_kcps": 8.5}]}"#,
    );
}

#[test]
fn bno055_every_channel_and_one() {
    let mut s = Bno055Sample {
        timestamp_us: 0,
        addr: 0x08,
        raw: Vec::new(),
        units: Units {
            accel_mg: true,
            euler_rad: true,
            android: true,
            ..Units::default()
        },
        quaternion: Some([1.0, 0.0, 0.0, 0.0]),
        euler: Some([0.5, -0.25, 1.75]),
        accel: Some([0.0, 0.0, 9.81]),
        gyro: Some([0.1, 0.2, 0.3]),
        mag: Some([10.5, -20.25, 30.0]),
        linear_accel: Some([0.01, 0.02, 0.03]),
        gravity: Some([0.0, 0.0, 9.8]),
        temperature: Some(24.0),
        calibration: Some(CalibStatus {
            system: 3,
            gyro: 3,
            accel: 2,
            mag: 1,
        }),
    };
    assert_record(
        rec::bno055(&s),
        r#"{"unit_sel": 133, "quaternion": [1.0, 0.0, 0.0, 0.0], "euler": [0.5, -0.25, 1.75],
            "accel": [0.0, 0.0, 9.81], "gyro": [0.1, 0.2, 0.3], "mag": [10.5, -20.25, 30.0],
            "linear_accel": [0.01, 0.02, 0.03], "gravity": [0.0, 0.0, 9.8], "temperature": 24.0,
            "calib": [3, 3, 2, 1]}"#,
    );
    s.units = Units::default();
    s.quaternion = None;
    s.euler = None;
    s.gyro = None;
    s.mag = None;
    s.linear_accel = None;
    s.gravity = None;
    s.temperature = None;
    s.calibration = None;
    assert_record(
        rec::bno055(&s),
        r#"{"unit_sel": 0, "accel": [0.0, 0.0, 9.81]}"#,
    );
}
