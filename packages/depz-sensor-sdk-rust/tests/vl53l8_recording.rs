//! Target 1 — VL53L8 frame reassembly + decode, verified by replaying a real
//! `.depzrec` capture through framing → reassembler → decoder and comparing
//! every produced frame to the golden `.expected.json` sidecar.
//!
//! The capture is from a **VL53L8CX** (dev-default) device — its
//! `software_name` is `APP_VL53L8_v0.9` and it streams frames with the ULD
//! 2.1.0 footer geometry, so it decodes under [`Variant::Cx`]. The same shared
//! frame layout serves both ToF variants; only the footer-id offset is
//! variant-specific, so this path is exactly what a CH device exercises too
//! (with [`Variant::Ch`]). CNH histogram decode is covered by `vl53l8_cnh.rs`.

mod common;

use depz_sensor_sdk::framing::{Event, PacketParser};
use depz_sensor_sdk::vl53l8::{parse_frame, unpack_frame_chunk, FrameReassembler, Variant};
use serde_json::Value;

use common::{hex_decode, hex_encode, load, recorded_text, recording_packets, recordings_dir, seen};

const RPT_VL53_FRAME: u8 = 0x93;

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
    seen("recordings/vl53l8_8x8_15hz_3s.depzrec");
    eprintln!(
        "vl53l8 recording: {} frames verified byte-exact vs sidecar ({} produced total)",
        exp_frames.len(),
        frames.len()
    );
}

// ---- VL53L8CH captures ------------------------------------------------------

/// VL53L8CH captures (APP_VL53L8_v0.92, VL53LMZ firmware): footer at size-4
/// ([`Variant::Ch`]); the CNH capture carries the raw CNH block in every
/// frame, byte-exact vs the sidecar.
fn vl53l8ch_replay(stem: &str, with_cnh: bool) {
    use depz_sensor_sdk::protocol::common::Cmd;

    let exp = load(&format!("recordings/{}.expected.json", stem));
    let pkts = recording_packets(&format!("{}.depzrec", stem));
    assert_eq!(
        recorded_text(&pkts, Cmd::GetNameActiveSoftware).as_deref(),
        exp["software_name"].as_str(),
        "{}",
        stem
    );
    let mut reasm = FrameReassembler::new();
    let mut frames = Vec::new();
    for p in pkts.iter().filter(|p| p.rx && p.cmd == RPT_VL53_FRAME) {
        if let Some(done) = reasm.feed(unpack_frame_chunk(&p.payload).unwrap()) {
            let r = parse_frame(&done.frame, Variant::Ch)
                .unwrap_or_else(|e| panic!("{}: frame decodes: {}", stem, e));
            frames.push((done.timestamp_us, r));
        }
    }
    let want = exp["frames"].as_array().unwrap();
    // The capture may stream past the sidecar; match the first N.
    assert!(frames.len() >= want.len(), "{}: {} < {} frames", stem, frames.len(), want.len());
    let ints = |v: &Value| -> Vec<i64> { v.as_array().unwrap().iter().map(|x| x.as_i64().unwrap()).collect() };
    for (idx, ((ts, r), w)) in frames.iter().zip(want).enumerate() {
        let got = DecodedFrame {
            timestamp_us: *ts as i64,
            resolution: r.resolution(),
            silicon_temp_degc: r.silicon_temp_degc as i64,
            distance_mm: r.distance_mm.iter().map(|&x| x as i64).collect(),
            target_status: r.target_status.iter().map(|&x| x as i64).collect(),
            nb_target_detected: r.nb_target_detected.iter().map(|&x| x as i64).collect(),
        };
        let want_frame = DecodedFrame {
            timestamp_us: w["timestamp_us"].as_i64().unwrap(),
            resolution: w["resolution"].as_u64().unwrap() as usize,
            silicon_temp_degc: w["silicon_temp_degc"].as_i64().unwrap(),
            distance_mm: ints(&w["distance_mm"]),
            target_status: ints(&w["target_status"]),
            nb_target_detected: ints(&w["nb_target_detected"]),
        };
        assert_eq!(got, want_frame, "{} frame {}", stem, idx);
        let want_cnh = if with_cnh {
            Some(w["cnh_raw"].as_str().expect("sidecar cnh_raw"))
        } else {
            assert!(w.get("cnh_raw").is_none(), "{} frame {}: unexpected cnh_raw", stem, idx);
            None
        };
        assert_eq!(r.cnh_raw.as_deref().map(hex_encode).as_deref(), want_cnh, "{} frame {} cnh", stem, idx);
    }
    assert_eq!(reasm.discarded, 0, "{}: no frames discarded", stem);
    seen(&format!("recordings/{}.depzrec", stem));
}

#[test]
fn vl53l8ch_8x8_recording_replay() {
    vl53l8ch_replay("vl53l8ch_8x8_15hz_3s", false);
}

#[test]
fn vl53l8ch_cnh_recording_replay() {
    vl53l8ch_replay("vl53l8ch_cnh_8x8_15hz", true);
}
