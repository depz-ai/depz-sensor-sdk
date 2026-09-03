//! Target 5 — `.depzdata` dataset reader (contract 09) vs
//! `contracts/vectors/recordings/dataset_dual_sr04.depzdata`.
//!
//! No `.expected.json` sidecar exists; the dataset is validated against the
//! contract's own invariants and the golden fixture's known contents.

use std::path::PathBuf;

use depz_sensor_sdk::dataset::{Dataset, RecordValue};
use serde_json::Value;

fn fixture() -> PathBuf {
    PathBuf::from(env!("CARGO_MANIFEST_DIR"))
        .join("..")
        .join("..")
        .join("contracts")
        .join("vectors")
        .join("recordings")
        .join("dataset_dual_sr04.depzdata")
}

#[test]
fn dataset_dual_sr04() {
    let text = std::fs::read_to_string(fixture()).unwrap();
    let ds = Dataset::parse(&text).expect("dataset parses");

    // Header.
    assert_eq!(ds.schema, "depz.dataset/1");
    assert_eq!(ds.devices.len(), 2, "two devices");

    // Devices in d0, d1 assignment order.
    let d0 = &ds.devices[0];
    let d1 = &ds.devices[1];
    assert_eq!(d0.id, "d0");
    assert_eq!(d1.id, "d1");
    assert_eq!(d0.sensor_type.as_deref(), Some("sr04"));
    assert_eq!(d1.sensor_type.as_deref(), Some("sr04"));
    assert_eq!(d0.serial.as_deref(), Some("SN0042"));
    assert_eq!(d0.software_name.as_deref(), Some("APP_usonic_SR04_v0.95"));
    assert_eq!(d0.offset_us, Some(-8_524_738_852));
    assert_eq!(d0.rtt_us, Some(13));
    assert_eq!(d1.offset_us, Some(-3_525_739_108));
    assert_eq!(d1.rtt_us, Some(9));

    // Cross-check the header fields against the raw JSON (independent of the
    // reader's own object walk).
    let header: Value = serde_json::from_str(text.lines().next().unwrap()).unwrap();
    for dev in &ds.devices {
        let raw = &header["devices"][&dev.id];
        assert_eq!(
            dev.offset_us,
            raw["time_sync"]["offset_us"].as_i64(),
            "offset_us {}",
            dev.id
        );
    }

    // Records: 10 sr04 samples, 5 per device, monotonic t per device.
    assert_eq!(ds.records.len(), 10, "record count");
    for dev_id in ["d0", "d1"] {
        let recs: Vec<_> = ds.records_for(dev_id).collect();
        assert_eq!(recs.len(), 5, "records for {}", dev_id);
        let mut last_t = i64::MIN;
        for r in &recs {
            assert_eq!(r.kind, "sr04");
            assert!(r.t > last_t, "monotonic t for {}", dev_id);
            last_t = r.t;
            match &r.value {
                RecordValue::Sr04 { echo_us, source } => {
                    assert_eq!(*echo_us, 5831, "echo_us for {}", dev_id);
                    assert!(source == "loop" || source == "once", "source {}", source);
                }
                other => panic!("expected sr04 value, got {:?}", other),
            }
        }
    }

    // The whole file merges into 10 records; the interleaving is d0/d1 per t.
    assert!(ds.records.iter().all(|r| r.t > 0));
    eprintln!(
        "dataset: {} devices, {} records parsed",
        ds.devices.len(),
        ds.records.len()
    );
}
