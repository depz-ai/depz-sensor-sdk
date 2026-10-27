//! Target 2 — VL53L8 advanced DCI codecs (motion config, detection thresholds,
//! xtalk margin) vs `contracts/vectors/vl53l8_advanced.json`.
//!
//! These codecs are shared by both ToF variants (VL53L8CX and VL53L8CH): the
//! advanced DCI byte layouts are identical, so this coverage applies to both.

mod common;

use depz_sensor_sdk::vl53l8::advanced::{
    try_default_motion_config, try_pack_detection_thresholds, try_xtalk_margin_to_raw,
    xtalk_margin_from_raw, DetectionThreshold,
};

use common::{hex_encode, load, seen};

#[test]
fn vl53l8_advanced_vectors() {
    let doc = load("vl53l8_advanced.json");

    // ── motion-indicator default config (156-byte pack) ──────────────────────
    let mut motion = 0;
    for c in doc["motion"].as_array().unwrap() {
        let resolution = c["resolution"].as_u64().unwrap() as usize;
        let cfg = try_default_motion_config(resolution).unwrap();
        let packed = cfg.pack();
        assert_eq!(packed.len(), 156, "motion pack length {}", c["name"]);
        assert_eq!(
            hex_encode(&packed),
            c["pack"].as_str().unwrap(),
            "motion {}",
            c["name"]
        );
        seen(c);
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
        let blocks = try_pack_detection_thresholds(&entries).unwrap();
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
        seen(c);
        thresholds += 1;
    }

    // ── xtalk margin (kcps → raw) ────────────────────────────────────────────
    let mut xtalk = 0;
    for c in doc["xtalk_margin"].as_array().unwrap() {
        let kcps = c["kcps"].as_u64().unwrap() as u32;
        let raw = try_xtalk_margin_to_raw(kcps).unwrap();
        assert_eq!(
            raw as u64,
            c["raw"].as_u64().unwrap(),
            "xtalk margin {}",
            c["name"]
        );
        assert_eq!(
            xtalk_margin_from_raw(raw),
            kcps,
            "xtalk margin from raw {}",
            c["name"]
        );
        seen(c);
        xtalk += 1;
    }

    assert!(motion > 0 && thresholds > 0 && xtalk > 0);
    eprintln!(
        "vl53l8_advanced.json: {} motion + {} thresholds + {} xtalk",
        motion, thresholds, xtalk
    );
}
