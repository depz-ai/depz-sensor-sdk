//! Target 2 — VL53L8 advanced DCI codecs (motion config, detection thresholds,
//! xtalk margin) vs `contracts/vectors/vl53l8_advanced.json`.
//!
//! These codecs are shared by both ToF variants (VL53L8CX and VL53L8CH): the
//! advanced DCI byte layouts are identical, so this coverage applies to both.

use std::path::PathBuf;

use depz_sensor_sdk::vl53l8::advanced::{
    default_motion_config, pack_detection_thresholds, xtalk_margin_to_raw, DetectionThreshold,
};
use serde_json::Value;

fn vectors_dir() -> PathBuf {
    PathBuf::from(env!("CARGO_MANIFEST_DIR"))
        .join("..")
        .join("..")
        .join("contracts")
        .join("vectors")
}

fn hex_encode(b: &[u8]) -> String {
    let mut s = String::with_capacity(b.len() * 2);
    for x in b {
        s.push_str(&format!("{:02x}", x));
    }
    s
}

#[test]
fn vl53l8_advanced_vectors() {
    let doc: Value = serde_json::from_str(
        &std::fs::read_to_string(vectors_dir().join("vl53l8_advanced.json")).unwrap(),
    )
    .unwrap();

    // ── motion-indicator default config (156-byte pack) ──────────────────────
    let mut motion = 0;
    for c in doc["motion"].as_array().unwrap() {
        let resolution = c["resolution"].as_u64().unwrap() as usize;
        let cfg = default_motion_config(resolution);
        let packed = cfg.pack();
        assert_eq!(packed.len(), 156, "motion pack length {}", c["name"]);
        assert_eq!(hex_encode(&packed), c["pack"].as_str().unwrap(), "motion {}", c["name"]);
        motion += 1;
    }

    // ── detection thresholds (start block + valid status) ────────────────────
    let mut thresholds = 0;
    for c in doc["thresholds"].as_array().unwrap() {
        let entries: Vec<DetectionThreshold> = c["thresholds"]
            .as_array()
            .unwrap()
            .iter()
            .map(|t| DetectionThreshold {
                low_thresh: t["low_thresh"].as_i64().unwrap() as i32,
                high_thresh: t["high_thresh"].as_i64().unwrap() as i32,
                measurement: t["measurement"].as_u64().unwrap() as u8,
                type_: t["type"].as_u64().unwrap() as u8,
                zone_num: t["zone_num"].as_u64().unwrap() as u8,
                operation: t["operation"].as_u64().unwrap() as u8,
            })
            .collect();
        let blocks = pack_detection_thresholds(&entries);
        assert_eq!(
            hex_encode(&blocks.start),
            c["start_block"].as_str().unwrap(),
            "threshold start {}",
            c["name"]
        );
        assert_eq!(
            hex_encode(&blocks.valid),
            c["valid_status"].as_str().unwrap(),
            "threshold valid {}",
            c["name"]
        );
        thresholds += 1;
    }

    // ── xtalk margin (kcps → raw) ────────────────────────────────────────────
    let mut xtalk = 0;
    for c in doc["xtalk_margin"].as_array().unwrap() {
        let kcps = c["kcps"].as_f64().unwrap();
        assert_eq!(
            xtalk_margin_to_raw(kcps) as u64,
            c["raw"].as_u64().unwrap(),
            "xtalk margin {}",
            c["name"]
        );
        xtalk += 1;
    }

    assert!(motion > 0 && thresholds > 0 && xtalk > 0);
    eprintln!(
        "vl53l8_advanced.json: {} motion + {} thresholds + {} xtalk",
        motion, thresholds, xtalk
    );
}
