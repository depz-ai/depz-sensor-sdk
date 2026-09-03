//! VL53L8CH CNH (Compact Network Histogram) decode vs the golden vector
//! `contracts/vectors/vl53l8_cnh.json`, captured from a live VL53L8CH.
//!
//! Decodes `cnh_raw` with [`decode_cnh`] and asserts every aggregate's
//! `hist_raw` + `hist_scaler` and the `ref_residual_word` match exactly.

use std::path::PathBuf;

use depz_sensor_sdk::vl53l8::{decode_cnh, CnhDecodeConfig};
use serde_json::Value;

fn vectors_dir() -> PathBuf {
    PathBuf::from(env!("CARGO_MANIFEST_DIR"))
        .join("..")
        .join("..")
        .join("contracts")
        .join("vectors")
}

fn hex_decode(s: &str) -> Vec<u8> {
    assert!(s.len() % 2 == 0, "odd-length hex");
    (0..s.len())
        .step_by(2)
        .map(|i| u8::from_str_radix(&s[i..i + 2], 16).expect("hex byte"))
        .collect()
}

#[test]
fn vl53l8_cnh_vector() {
    let doc: Value = serde_json::from_str(
        &std::fs::read_to_string(vectors_dir().join("vl53l8_cnh.json")).unwrap(),
    )
    .unwrap();

    let cfg = CnhDecodeConfig {
        nb_of_aggregates: doc["config"]["nb_of_aggregates"].as_u64().unwrap() as usize,
        feature_length: doc["config"]["feature_length"].as_u64().unwrap() as usize,
    };
    let raw = hex_decode(doc["cnh_raw"].as_str().unwrap());

    let decoded = decode_cnh(&cfg, &raw).expect("decode_cnh");

    // ── ref_residual_word (u32 at byte offset 8) ─────────────────────────────
    let expected_rrw = doc["expected"]["ref_residual_word"].as_u64().unwrap() as u32;
    assert_eq!(
        decoded.ref_residual_word, expected_rrw,
        "ref_residual_word mismatch"
    );

    // ── per-aggregate hist_raw + hist_scaler ─────────────────────────────────
    let expected_aggs = doc["expected"]["aggregates"].as_array().unwrap();
    assert_eq!(
        decoded.aggregates.len(),
        expected_aggs.len(),
        "aggregate count"
    );
    assert_eq!(decoded.aggregates.len(), cfg.nb_of_aggregates);

    let mut passed = 0;
    for (agg_id, (got, exp)) in decoded.aggregates.iter().zip(expected_aggs).enumerate() {
        let exp_raw: Vec<i32> = exp["hist_raw"]
            .as_array()
            .unwrap()
            .iter()
            .map(|v| v.as_i64().unwrap() as i32)
            .collect();
        let exp_scaler: Vec<i8> = exp["hist_scaler"]
            .as_array()
            .unwrap()
            .iter()
            .map(|v| v.as_i64().unwrap() as i8)
            .collect();

        assert_eq!(got.hist_raw.len(), cfg.feature_length, "agg {agg_id} raw len");
        assert_eq!(
            got.hist_scaler.len(),
            cfg.feature_length,
            "agg {agg_id} scaler len"
        );
        assert_eq!(got.hist_raw, exp_raw, "agg {agg_id} hist_raw mismatch");
        assert_eq!(
            got.hist_scaler, exp_scaler,
            "agg {agg_id} hist_scaler mismatch"
        );
        passed += 1;
    }

    assert_eq!(passed, cfg.nb_of_aggregates);
    eprintln!(
        "vl53l8_cnh.json: {}/{} aggregates exact-match, ref_residual_word={}",
        passed, cfg.nb_of_aggregates, decoded.ref_residual_word
    );
}
