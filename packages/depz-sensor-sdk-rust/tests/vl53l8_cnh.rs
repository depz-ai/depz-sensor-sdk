//! VL53L8CH CNH (Compact Network Histogram) decode vs the golden vector
//! `contracts/vectors/vl53l8_cnh.json`, captured from a live VL53L8CH.
//!
//! Decodes `cnh_raw` with [`decode_cnh`] and asserts every aggregate's
//! `hist_raw` + `hist_scaler` and the `ref_residual_word` match exactly.

mod common;

use depz_sensor_sdk::vl53l8::{decode_cnh, CnhDecodeConfig};

use common::{hex_decode, load, seen};

#[test]
fn vl53l8_cnh_vector() {
    let doc = load("vl53l8_cnh.json");

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

        assert_eq!(
            got.hist_raw.len(),
            cfg.feature_length,
            "agg {agg_id} raw len"
        );
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
    assert_eq!(
        passed as u64,
        doc["expected"]["nb_aggregates"].as_u64().unwrap()
    );
    seen("vl53l8_cnh");
    eprintln!(
        "vl53l8_cnh.json: {}/{} aggregates exact-match, ref_residual_word={}",
        passed, cfg.nb_of_aggregates, decoded.ref_residual_word
    );
}
