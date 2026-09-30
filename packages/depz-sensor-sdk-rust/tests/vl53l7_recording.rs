//! VL53L5CX / VL53L7CH frame reassembly + decode (contract 11), verified by
//! replaying the rx side of four live-board captures (APP_VL53L7_v0.53)
//! through framing → reassembler → decoder under [`Variant::L7`] and comparing
//! every produced frame to its `.expected.json` sidecar.
//!
//! - `vl53l5cx_8x8_15hz_3s`, `vl53l7ch_8x8_15hz_3s` — plain 8×8 frames.
//! - `vl53l5cx_4x4_15hz` — pins the L5/L7 trim: per-target blocks arrive with
//!   64 entries on the wire and must come out as 16.
//! - `vl53l7ch_cnh_8x8_15hz` — 3156-byte CNH frames over the chunked stream;
//!   `cnh_raw` must match byte-exact (and non-CNH frames carry none).

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

fn hex_encode(b: &[u8]) -> String {
    b.iter().map(|x| format!("{:02x}", x)).collect()
}

#[derive(Debug, PartialEq, Eq)]
struct DecodedFrame {
    timestamp_us: i64,
    resolution: usize,
    silicon_temp_degc: i64,
    distance_mm: Vec<i64>,
    target_status: Vec<i64>,
    nb_target_detected: Vec<i64>,
    cnh_raw: Option<String>,
}

fn ints(v: &Value) -> Vec<i64> {
    v.as_array().unwrap().iter().map(|x| x.as_i64().unwrap()).collect()
}

fn replay(stem: &str, resolution: usize, with_cnh: bool) {
    let base = recordings_dir();
    let rec = std::fs::read_to_string(base.join(format!("{}.depzrec", stem))).unwrap();
    let expected: Value = serde_json::from_str(
        &std::fs::read_to_string(base.join(format!("{}.expected.json", stem))).unwrap(),
    )
    .unwrap();
    assert_eq!(expected["software_name"].as_str(), Some("APP_VL53L7_v0.53"));

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
                let r = parse_frame(&done.frame, Variant::L7)
                    .unwrap_or_else(|e| panic!("{}: frame decodes: {}", stem, e));
                // Every per-zone array is trimmed to the frame's resolution.
                let n = r.resolution();
                assert_eq!(r.distance_mm.len(), n, "{} distance_mm len", stem);
                assert_eq!(r.target_status.len(), n, "{} target_status len", stem);
                assert_eq!(r.signal_per_spad.len(), n, "{} signal_per_spad len", stem);
                assert_eq!(r.ambient_per_spad.len(), n, "{} ambient_per_spad len", stem);
                assert_eq!(r.nb_spads_enabled.len(), n, "{} nb_spads_enabled len", stem);
                assert_eq!(r.range_sigma_mm_raw.len(), n, "{} range_sigma len", stem);
                assert_eq!(r.reflectance.len(), n, "{} reflectance len", stem);
                frames.push(DecodedFrame {
                    timestamp_us: done.timestamp_us as i64,
                    resolution: n,
                    silicon_temp_degc: r.silicon_temp_degc as i64,
                    distance_mm: r.distance_mm.iter().map(|&x| x as i64).collect(),
                    target_status: r.target_status.iter().map(|&x| x as i64).collect(),
                    nb_target_detected: r.nb_target_detected.iter().map(|&x| x as i64).collect(),
                    cnh_raw: r.cnh_raw.as_deref().map(hex_encode),
                });
            }
        }
    }

    // The reference consumes exactly `len(expected)` frames; compare the first
    // N and require at least that many were produced.
    let exp_frames = expected["frames"].as_array().unwrap();
    assert!(
        frames.len() >= exp_frames.len(),
        "{}: produced {} frames, need >= {}",
        stem,
        frames.len(),
        exp_frames.len()
    );

    for (idx, (got, want)) in frames.iter().zip(exp_frames).enumerate() {
        let want_frame = DecodedFrame {
            timestamp_us: want["timestamp_us"].as_i64().unwrap(),
            resolution: want["resolution"].as_u64().unwrap() as usize,
            silicon_temp_degc: want["silicon_temp_degc"].as_i64().unwrap(),
            distance_mm: ints(&want["distance_mm"]),
            target_status: ints(&want["target_status"]),
            nb_target_detected: ints(&want["nb_target_detected"]),
            cnh_raw: if with_cnh {
                Some(want["cnh_raw"].as_str().expect("sidecar cnh_raw").to_string())
            } else {
                assert!(want.get("cnh_raw").is_none(), "{} frame {}: unexpected cnh_raw", stem, idx);
                None
            },
        };
        assert_eq!(want_frame.resolution, resolution, "{} frame {} resolution", stem, idx);
        assert_eq!(*got, want_frame, "{} frame {}", stem, idx);
    }

    assert_eq!(reasm.discarded, 0, "{}: no frames discarded", stem);
    eprintln!(
        "{}: {} frames verified vs sidecar ({} produced total)",
        stem,
        exp_frames.len(),
        frames.len()
    );
}

#[test]
fn vl53l5cx_8x8_recording_replay() {
    replay("vl53l5cx_8x8_15hz_3s", 64, false);
}

#[test]
fn vl53l5cx_4x4_recording_replay() {
    replay("vl53l5cx_4x4_15hz", 16, false);
}

#[test]
fn vl53l7ch_8x8_recording_replay() {
    replay("vl53l7ch_8x8_15hz_3s", 64, false);
}

#[test]
fn vl53l7ch_cnh_recording_replay() {
    replay("vl53l7ch_cnh_8x8_15hz", 64, true);
}
