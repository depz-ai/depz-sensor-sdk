//! VL53L8 golden-vector conformance tests vs `contracts/vectors/vl53l8.json`
//! (contract 04 §2–§4, the streaming side of the SPI register bridge):
//! START_STREAM encode, RPT_VL53_FRAME chunk decode and the FrameReassembler
//! rules over chunks a live board emitted.

mod common;

use depz_sensor_sdk::vl53l8::{pack_start_stream, unpack_frame_chunk, Vl53l8Rpt};

use common::{hex_decode, hex_encode, load, seen, shapes};

fn s(v: &serde_json::Value) -> &str {
    v.as_str().unwrap()
}

// ---- vl53l8.json: "encode" (VL53_START_STREAM) ------------------------------

#[test]
fn vl53l8_encode_vectors() {
    let doc = load("vl53l8.json");
    let mut n = 0;
    for c in doc["encode"].as_array().unwrap() {
        assert_eq!(s(&c["kind"]), "start_stream", "{}", c["name"]);
        let got = hex_encode(&pack_start_stream(c["frame_size"].as_u64().unwrap() as u16).unwrap());
        assert_eq!(got, s(&c["payload"]), "encode {}", c["name"]);
        seen(c);
        n += 1;
    }
    assert!(n > 0);
}

// ---- vl53l8.json: "decode" (RPT_VL53_FRAME 0x93) ----------------------------

#[test]
fn vl53l8_decode_vectors() {
    let doc = load("vl53l8.json");
    let mut n = 0;
    for c in doc["decode"].as_array().unwrap() {
        let report = c["report"].as_u64().unwrap();
        assert_eq!(report, Vl53l8Rpt::Vl53Frame as u64, "{}", c["name"]);
        let got = shapes::vl53l8_decode(report, &hex_decode(s(&c["payload"])))
            .unwrap_or_else(|e| panic!("{}: {}", c["name"], e));
        assert_eq!(got, c["expect"], "decode {}", c["name"]);
        seen(c);
        n += 1;
    }
    assert!(n > 0);
}

#[test]
fn vl53l8_frame_chunk_rejects_a_short_payload() {
    assert!(unpack_frame_chunk(&[0u8; 11]).is_none());
    assert!(unpack_frame_chunk(&[0u8; 12]).unwrap().data.is_empty());
}

// ---- vl53l8.json: "reassembly" (contract 04 §4 rules 1–3) -------------------

#[test]
fn vl53l8_reassembly_vectors() {
    let doc = load("vl53l8.json");
    let mut n = 0;
    for c in doc["reassembly"].as_array().unwrap() {
        let chunks: Vec<Vec<u8>> = c["chunks"]
            .as_array()
            .unwrap()
            .iter()
            .map(|ch| hex_decode(s(ch)))
            .collect();
        let got =
            shapes::vl53l8_reassembly(&chunks).unwrap_or_else(|e| panic!("{}: {}", c["name"], e));
        assert_eq!(got, c["expect"], "reassembly {}", c["name"]);
        seen(c);
        n += 1;
    }
    assert!(n > 0);
}
