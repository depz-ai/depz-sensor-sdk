//! Target 3 — BNO086 SHTP framing + SH-2 control encodes vs
//! `contracts/vectors/bno086_shtp.json`.

use std::path::PathBuf;

use depz_sensor_sdk::bno086::sh2;
use depz_sensor_sdk::bno086::shtp::{pack_shtp_header, unpack_shtp_header, ShtpHeader, ShtpLayer};
use serde_json::Value;

fn vectors_dir() -> PathBuf {
    PathBuf::from(env!("CARGO_MANIFEST_DIR"))
        .join("..")
        .join("..")
        .join("contracts")
        .join("vectors")
}

fn hex_decode(s: &str) -> Vec<u8> {
    (0..s.len())
        .step_by(2)
        .map(|i| u8::from_str_radix(&s[i..i + 2], 16).unwrap())
        .collect()
}

fn hex_encode(b: &[u8]) -> String {
    let mut s = String::with_capacity(b.len() * 2);
    for x in b {
        s.push_str(&format!("{:02x}", x));
    }
    s
}

#[test]
fn bno086_shtp_vectors() {
    let doc: Value =
        serde_json::from_str(&std::fs::read_to_string(vectors_dir().join("bno086_shtp.json")).unwrap())
            .unwrap();

    // ── header pack / unpack ─────────────────────────────────────────────────
    let mut headers = 0;
    for c in doc["header"].as_array().unwrap() {
        let hdr = ShtpHeader {
            length: c["length"].as_u64().unwrap() as u16,
            channel: c["channel"].as_u64().unwrap() as u8,
            seq: c["seq"].as_u64().unwrap() as u8,
            continuation: c["continuation"].as_bool().unwrap(),
        };
        let packed = pack_shtp_header(&hdr);
        assert_eq!(hex_encode(&packed), c["bytes"].as_str().unwrap(), "header {}", c["name"]);
        let round = unpack_shtp_header(&packed);
        assert_eq!(round, hdr, "header unpack {}", c["name"]);
        headers += 1;
    }

    // ── per-channel TX seq counters ──────────────────────────────────────────
    let mut layer = ShtpLayer::new();
    let mut tx = 0;
    for c in doc["tx_seq"].as_array().unwrap() {
        let channel = c["channel"].as_u64().unwrap() as u8;
        let payload = hex_decode(c["payload"].as_str().unwrap());
        let frame = layer.next_frame(channel, &payload);
        assert_eq!(hex_encode(&frame), c["frame"].as_str().unwrap(), "tx_seq {}", tx);
        tx += 1;
    }

    // ── RX reassembly (continuation-bit) ─────────────────────────────────────
    let mut reasm = 0;
    for c in doc["reassembly"].as_array().unwrap() {
        let mut layer = ShtpLayer::new();
        let mut cargos: Vec<Value> = Vec::new();
        for frame_hex in c["frames"].as_array().unwrap() {
            let frame = hex_decode(frame_hex.as_str().unwrap());
            if let Some(cargo) = layer.feed(&frame) {
                cargos.push(serde_json::json!({
                    "channel": cargo.channel,
                    "seq": cargo.seq,
                    "payload": hex_encode(&cargo.payload),
                }));
            }
        }
        let expect = &c["expect"];
        let want_cargos = expect["cargos"].as_array().unwrap();
        assert_eq!(cargos.len(), want_cargos.len(), "cargo count {}", c["name"]);
        for (got, want) in cargos.iter().zip(want_cargos) {
            assert_eq!(got["channel"], want["channel"], "channel {}", c["name"]);
            assert_eq!(got["seq"], want["seq"], "seq {}", c["name"]);
            assert_eq!(got["payload"], want["payload"], "payload {}", c["name"]);
        }
        assert_eq!(
            layer.discarded,
            expect["discarded"].as_u64().unwrap(),
            "discarded {}",
            c["name"]
        );
        reasm += 1;
    }

    // ── SH-2 control encodes ─────────────────────────────────────────────────
    let mut enc = 0;
    for c in doc["control_encode"].as_array().unwrap() {
        let want = c["payload"].as_str().unwrap();
        let got: Vec<u8> = match c["kind"].as_str().unwrap() {
            "set_feature" => sh2::build_set_feature(
                c["sensor_id"].as_u64().unwrap() as u8,
                c["interval_us"].as_u64().unwrap() as u32,
                c["batch_us"].as_u64().unwrap() as u32,
                c["sensitivity"].as_u64().unwrap() as u16,
                c["flags"].as_u64().unwrap() as u8,
                c["cfg_word"].as_u64().unwrap() as u32,
            )
            .to_vec(),
            "get_feature_request" => {
                sh2::build_get_feature_request(c["sensor_id"].as_u64().unwrap() as u8).to_vec()
            }
            "product_id_request" => sh2::build_product_id_request().to_vec(),
            "command_request" => {
                let params = hex_decode(c["params"].as_str().unwrap());
                sh2::build_command_request(
                    c["seq"].as_u64().unwrap() as u8,
                    c["command"].as_u64().unwrap() as u8,
                    &params,
                )
                .unwrap()
                .to_vec()
            }
            "frs_read_request" => sh2::build_frs_read_request(
                c["frs_type"].as_u64().unwrap() as u16,
                c["offset_words"].as_u64().unwrap() as u16,
                c["block_words"].as_u64().unwrap() as u16,
            )
            .to_vec(),
            "frs_write_request" => sh2::build_frs_write_request(
                c["frs_type"].as_u64().unwrap() as u16,
                c["length_words"].as_u64().unwrap() as u16,
            )
            .to_vec(),
            "frs_write_data" => {
                let words: Vec<u32> = c["words"]
                    .as_array()
                    .unwrap()
                    .iter()
                    .map(|v| v.as_u64().unwrap() as u32)
                    .collect();
                sh2::build_frs_write_data(c["offset_words"].as_u64().unwrap() as u16, &words)
                    .unwrap()
                    .to_vec()
            }
            other => panic!("unknown control_encode kind {}", other),
        };
        assert_eq!(hex_encode(&got), want, "control_encode {}", c["name"]);
        enc += 1;
    }

    assert!(headers > 0 && tx > 0 && reasm > 0 && enc > 0);
    eprintln!(
        "bno086_shtp.json: {} headers + {} tx_seq + {} reassembly + {} control_encode",
        headers, tx, reasm, enc
    );
}
