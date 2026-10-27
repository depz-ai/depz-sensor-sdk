//! Golden-vector consumer: probe.json (class resolution, contract 02 §4.4).

mod common;

use common::{load, seen};
use depz_sensor_sdk::{resolve_sensor_class, SENSOR_CLASS_NAMES};

#[test]
fn probe_class_names() {
    let v = load("probe.json");
    let want: Vec<&str> = v["classes"]
        .as_array()
        .unwrap()
        .iter()
        .map(|c| c.as_str().unwrap())
        .collect();
    assert_eq!(SENSOR_CLASS_NAMES.to_vec(), want);
    seen("probe/classes");
}

#[test]
fn probe_resolve_vectors() {
    let v = load("probe.json");
    for c in v["resolve"].as_array().unwrap() {
        let id = |k: &str| c[k].as_u64().map(|x| u16::try_from(x).unwrap());
        let got = resolve_sensor_class(
            c["software_name"].as_str().unwrap(),
            c["device_name"].as_str().unwrap(),
            id("usb_vid"),
            id("usb_pid"),
        );
        assert_eq!(got, c["class"].as_str().unwrap(), "{}", c["id"]);
        assert!(SENSOR_CLASS_NAMES.contains(&got));
        seen(c);
    }
}
