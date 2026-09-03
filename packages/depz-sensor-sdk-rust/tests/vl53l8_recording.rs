//! Target 1 — VL53L8 frame reassembly + decode, verified by replaying a real
//! `.depzrec` capture through framing → reassembler → decoder and comparing
//! every produced frame to the golden `.expected.json` sidecar.
//!
//! The capture is from a **VL53L8CX** (dev-default) device — its
//! `software_name` is `APP_VL53L8_v0.9` and it streams frames with the ULD
//! 2.1.0 footer geometry, so it decodes under [`Variant::Cx`]. The same shared
//! frame layout serves both ToF variants; only the footer-id offset is
//! variant-specific, so this path is exactly what a CH device exercises too.
//! (CNH histogram decode — the CH-only extension — is not exercised here; it is
//! not yet implemented.)

use std::path::PathBuf;

use depz_sensor_sdk::framing::{Event, PacketParser};
use depz_sensor_sdk::vl53l8::{parse_frame, unpack_frame_chunk, FrameReassembler, Variant};
use serde_json::Value;

const RPT_VL53_FRAME: u8 = 0x93;

fn recordings_dir() -> PathBuf {
    PathBuf::from(env!("CARGO_MANIFEST_DIR"))
        .join("..")
        .join("..")
        .join("contracts")
        .join("vectors")
        .join("recordings")
}

fn hex_decode(s: &str) -> Vec<u8> {
    assert!(s.len() % 2 == 0, "odd hex length");
    (0..s.len())
        .step_by(2)
        .map(|i| u8::from_str_radix(&s[i..i + 2], 16).expect("bad hex"))
        .collect()
}

#[derive(Debug, PartialEq, Eq)]
struct DecodedFrame {
    timestamp_us: i64,
    resolution: usize,
    silicon_temp_degc: i64,
    distance_mm: Vec<i64>,
    target_status: Vec<i64>,
    nb_target_detected: Vec<i64>,
}

#[test]
fn vl53l8_recording_replay() {
    let base = recordings_dir();
    let rec = std::fs::read_to_string(base.join("vl53l8_8x8_15hz_3s.depzrec")).unwrap();
    let expected: Value =
        serde_json::from_str(&std::fs::read_to_string(base.join("vl53l8_8x8_15hz_3s.expected.json")).unwrap())
            .unwrap();

    // Replay all rx bytes through the transport parser, then the VL53L8 frame
    // reassembler + decoder. Feed one recorded chunk at a time (chunking is
    // invariant, contract 01 §5).
    let mut parser = PacketParser::new();
    let mut reasm = FrameReassembler::new();
    let mut frames: Vec<DecodedFrame> = Vec::new();

    let mut lines = rec.lines();
    let _header = lines.next().expect("header line");
    for line in lines {
        let line = line.trim();
        if line.is_empty() {
            continue;
        }
        let ev: Value = serde_json::from_str(line).unwrap();
        if ev["dir"].as_str() != Some("rx") {
            continue;
        }
        let data = hex_decode(ev["data"].as_str().unwrap());
        for pkt_ev in parser.feed(&data) {
            let Event::Packet(pkt) = pkt_ev else { continue };
            if pkt.cmd != RPT_VL53_FRAME || pkt.payload.len() < 12 {
                continue;
            }
            let chunk = unpack_frame_chunk(&pkt.payload).unwrap();
            if let Some(done) = reasm.feed(chunk) {
                // CX (dev-default) capture (software_name APP_VL53L8_v0.9),
                // ULD 2.1.0 footer geometry → `Variant::Cx`. The decode path is
                // shared with CH (see file header). CNH block, if any, is raw.
                let r = parse_frame(&done.frame, Variant::Cx).expect("frame decodes");
                frames.push(DecodedFrame {
                    timestamp_us: done.timestamp_us as i64,
                    resolution: r.resolution(),
                    silicon_temp_degc: r.silicon_temp_degc as i64,
                    distance_mm: r.distance_mm.iter().map(|&x| x as i64).collect(),
                    target_status: r.target_status.iter().map(|&x| x as i64).collect(),
                    nb_target_detected: r.nb_target_detected.iter().map(|&x| x as i64).collect(),
                });
            }
        }
    }

    // The capture holds 45 sidecar-covered frames plus a trailing frame; the
    // reference consumes exactly `len(expected)` frames (islice), so compare
    // the first N and require at least that many were produced.
    let exp_frames = expected["frames"].as_array().unwrap();
    assert!(
        frames.len() >= exp_frames.len(),
        "produced {} frames, need >= {}",
        frames.len(),
        exp_frames.len()
    );

    for (idx, (got, want)) in frames.iter().zip(exp_frames).enumerate() {
        let want_frame = DecodedFrame {
            timestamp_us: want["timestamp_us"].as_i64().unwrap(),
            resolution: want["resolution"].as_u64().unwrap() as usize,
            silicon_temp_degc: want["silicon_temp_degc"].as_i64().unwrap(),
            distance_mm: want["distance_mm"]
                .as_array()
                .unwrap()
                .iter()
                .map(|v| v.as_i64().unwrap())
                .collect(),
            target_status: want["target_status"]
                .as_array()
                .unwrap()
                .iter()
                .map(|v| v.as_i64().unwrap())
                .collect(),
            nb_target_detected: want["nb_target_detected"]
                .as_array()
                .unwrap()
                .iter()
                .map(|v| v.as_i64().unwrap())
                .collect(),
        };
        assert_eq!(*got, want_frame, "frame {}", idx);
    }

    assert_eq!(reasm.discarded, 0, "no frames discarded");
    eprintln!(
        "vl53l8 recording: {} frames verified byte-exact vs sidecar ({} produced total)",
        exp_frames.len(),
        frames.len()
    );
}
