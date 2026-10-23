//! Target 3 — BNO086 SHTP framing + SH-2 control encodes vs
//! `contracts/vectors/bno086_shtp.json`.

mod common;

use depz_sensor_sdk::bno086::sh2;
use depz_sensor_sdk::bno086::shtp::{
    build_power_on, build_sleep, try_pack_shtp_header, try_unpack_shtp_header, ExecutableCommand,
    ShtpChannel, ShtpError, ShtpHeader, ShtpLayer,
};
use depz_sensor_sdk::framing::{build_packet, CrcType};
use serde_json::Value;

use common::{hex_decode, hex_encode, load, seen};

#[test]
fn bno086_shtp_vectors() {
    let doc = load("bno086_shtp.json");

    // ── header pack / unpack ─────────────────────────────────────────────────
    let mut headers = 0;
    for c in doc["header"].as_array().unwrap() {
        let hdr = ShtpHeader {
            length: c["length"].as_u64().unwrap() as u16,
            channel: c["channel"].as_u64().unwrap() as u8,
            seq: c["seq"].as_u64().unwrap() as u8,
            continuation: c["continuation"].as_bool().unwrap(),
        };
        let packed = try_pack_shtp_header(&hdr).unwrap();
        assert_eq!(hex_encode(&packed), c["bytes"].as_str().unwrap(), "header {}", c["name"]);
        let round = try_unpack_shtp_header(&packed).unwrap();
        assert_eq!(round, hdr, "header unpack {}", c["name"]);
        seen(c);
        headers += 1;
    }

    // ── per-channel TX seq counters ──────────────────────────────────────────
    let mut layer = ShtpLayer::new();
    let mut tx = 0;
    for c in doc["tx_seq"].as_array().unwrap() {
        let channel = c["channel"].as_u64().unwrap() as u8;
        let payload = hex_decode(c["payload"].as_str().unwrap());
        let frame = layer.try_next_frame(channel, &payload).unwrap();
        assert_eq!(hex_encode(&frame), c["frame"].as_str().unwrap(), "tx_seq {}", tx);
        seen(c);
        tx += 1;
    }

    // ── RX reassembly (continuation-bit) ─────────────────────────────────────
    let mut reasm = 0;
    for c in doc["reassembly"].as_array().unwrap() {
        let mut layer = ShtpLayer::new();
        let mut cargos: Vec<Value> = Vec::new();
        let captures = c["captures_us"].as_array().unwrap();
        for (frame_hex, capture) in c["frames"].as_array().unwrap().iter().zip(captures) {
            let frame = hex_decode(frame_hex.as_str().unwrap());
            if let Some(cargo) = layer.feed_at(&frame, capture.as_u64().unwrap()) {
                cargos.push(serde_json::json!({
                    "channel": cargo.channel,
                    "seq": cargo.seq,
                    "payload": hex_encode(&cargo.payload),
                    "capture_us": cargo.capture_us,
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
            assert_eq!(got["capture_us"], want["capture_us"], "capture_us {}", c["name"]);
        }
        assert_eq!(
            layer.discarded,
            expect["discarded"].as_u64().unwrap(),
            "discarded {}",
            c["name"]
        );
        seen(c);
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
        seen(c);
        enc += 1;
    }

    // ── executable channel: sleep / power_on (contract 05 §6a) ──────────────
    let mut layer = ShtpLayer::new(); // fresh: sleep then power_on → seq 0, 1
    let mut exec = 0;
    for c in doc["executable"].as_array().unwrap() {
        let name = c["name"].as_str().unwrap();
        let (command, payload) = match name {
            "sleep" => (ExecutableCommand::Sleep, build_sleep()),
            "power_on" => (ExecutableCommand::On, build_power_on()),
            other => panic!("unknown executable command {}", other),
        };
        assert_eq!(command as u64, c["command"].as_u64().unwrap(), "command {}", name);
        assert_eq!(c["channel"].as_u64().unwrap(), ShtpChannel::Executable as u64);
        assert_eq!(hex_encode(&payload), c["payload"].as_str().unwrap(), "payload {}", name);
        let frame = layer.try_next_frame(ShtpChannel::Executable as u8, &payload).unwrap();
        assert_eq!(hex_encode(&frame), c["shtp_frame"].as_str().unwrap(), "shtp_frame {}", name);
        assert_eq!(c["crc_type"].as_u64().unwrap(), 0);
        let bridge = build_packet(
            c["bridge_cmd"].as_u64().unwrap() as u8,
            &frame,
            c["bridge_seq"].as_u64().unwrap() as u32,
            CrcType::None,
        )
        .unwrap();
        assert_eq!(hex_encode(&bridge), c["bridge_frame"].as_str().unwrap(), "bridge_frame {}", name);
        seen(c);
        exec += 1;
    }

    // ── Set Feature flag bits ────────────────────────────────────────────────
    let flags = &doc["feature_flags"];
    assert_eq!(flags["wakeup"].as_u64().unwrap(), sh2::FEATURE_FLAG_WAKEUP as u64);
    assert_eq!(flags["always_on"].as_u64().unwrap(), sh2::FEATURE_FLAG_ALWAYS_ON as u64);
    seen("bno086_shtp/feature_flags");

    assert!(headers > 0 && tx > 0 && reasm > 0 && enc > 0 && exec == 2);
    eprintln!(
        "bno086_shtp.json: {} headers + {} tx_seq + {} reassembly + {} control_encode + {} executable",
        headers, tx, reasm, enc, exec
    );
}

/// Regression (difftest): short headers, bad channels, oversize cargos and
/// over-wide lengths are errors, never panics or masked bytes (contract 07 §3).
#[test]
fn shtp_rejects_bad_input_without_panic() {
    for n in 0..4 {
        assert_eq!(try_unpack_shtp_header(&[0x05, 0x00, 0x02][..n.min(3)]), Err(ShtpError::ShortHeader(n.min(3))));
    }
    // A short frame fed to the reassembler is junk, not a panic.
    let mut layer = ShtpLayer::new();
    assert_eq!(layer.feed(&[0x05, 0x00]), None);

    let hdr = |length: u16| ShtpHeader { length, channel: 3, seq: 0, continuation: false };
    assert_eq!(try_pack_shtp_header(&hdr(0x8000)), Err(ShtpError::LengthTooWide(0x8000)));
    assert_eq!(try_pack_shtp_header(&hdr(0x7fff)).unwrap(), [0xff, 0x7f, 3, 0]);

    let mut layer = ShtpLayer::new();
    for ch in [6u8, 22, 127, 255] {
        assert_eq!(layer.try_next_frame(ch, &[0xf9]), Err(ShtpError::ChannelOutOfRange(ch)));
        assert_eq!(layer.try_tx_seq(ch), None);
    }
    assert_eq!(layer.try_next_frame(2, &[0u8; 61]), Err(ShtpError::TxCargoTooLong(61)));
    // Nothing consumed by the refusals: the next good frame still has seq 0.
    assert_eq!(layer.try_next_frame(2, &[0u8; 60]).unwrap()[..4], [64, 0, 2, 0]);
    assert_eq!(layer.try_tx_seq(2), Some(1));
}

/// The interactive-calibration (0x0E) and force-flush (0xF0) payloads, byte-exact.
#[test]
fn interactive_calibration_and_flush_byte_exact() {
    use depz_sensor_sdk::bno086::hub::{force_flush, interactive_calibration_params};
    let doc = load("bno086_shtp.json");
    for c in doc["interactive_and_flush"].as_array().unwrap() {
        let got: Vec<u8> = match c["kind"].as_str().unwrap() {
            "command_request" => sh2::build_command_request(
                c["seq"].as_u64().unwrap() as u8,
                c["command"].as_u64().unwrap() as u8,
                &interactive_calibration_params(hex_decode(c["params"].as_str().unwrap())[0]),
            )
            .unwrap()
            .to_vec(),
            "force_flush" => force_flush(c["sensor_id"].as_u64().unwrap() as u8).to_vec(),
            other => panic!("unknown kind {other}"),
        };
        assert_eq!(hex_encode(&got), c["payload"].as_str().unwrap(), "{}", c["name"]);
        seen(c);
    }
}
