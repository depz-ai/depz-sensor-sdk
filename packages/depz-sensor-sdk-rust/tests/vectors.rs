//! Golden-vector conformance tests. Every vector is loaded from
//! `contracts/vectors/*.json` (path relative to this crate) and asserted
//! byte-exact against the DEPZ contract.

use std::path::PathBuf;

use depz_sensor_sdk::crc::{crc16_ccitt_false, crc16_modbus, crc32_iso_hdlc, crc8_maxim};
use depz_sensor_sdk::framing::{build_packet, CrcType, Event, PacketParser};
use depz_sensor_sdk::fwdepz;
use depz_sensor_sdk::protocol::{common, identity, sr04};
use depz_sensor_sdk::usb_ids::{self, PortEntry};
use serde_json::Value;

// ---- helpers ---------------------------------------------------------------

fn vectors_dir() -> PathBuf {
    // CARGO_MANIFEST_DIR = packages/depz-sensor-sdk-rust
    PathBuf::from(env!("CARGO_MANIFEST_DIR"))
        .join("..")
        .join("..")
        .join("contracts")
        .join("vectors")
}

fn load(name: &str) -> Value {
    let p = vectors_dir().join(name);
    let txt = std::fs::read_to_string(&p)
        .unwrap_or_else(|e| panic!("read {}: {}", p.display(), e));
    serde_json::from_str(&txt).unwrap_or_else(|e| panic!("parse {}: {}", p.display(), e))
}

fn hex_decode(s: &str) -> Vec<u8> {
    assert!(s.len() % 2 == 0, "odd hex length: {:?}", s);
    (0..s.len())
        .step_by(2)
        .map(|i| u8::from_str_radix(&s[i..i + 2], 16).expect("bad hex"))
        .collect()
}

fn hex_encode(b: &[u8]) -> String {
    let mut s = String::with_capacity(b.len() * 2);
    for x in b {
        s.push_str(&format!("{:02x}", x));
    }
    s
}

fn crc_type_from(v: u64) -> CrcType {
    CrcType::from_bits(v as u16)
}

// ---- crc.json --------------------------------------------------------------

#[test]
fn crc_vectors() {
    let doc = load("crc.json");
    let mut n = 0;
    for c in doc["cases"].as_array().unwrap() {
        let input = hex_decode(c["input"].as_str().unwrap());
        assert_eq!(
            crc8_maxim(&input) as u64,
            c["crc8_maxim"].as_u64().unwrap(),
            "crc8_maxim {}",
            c["name"]
        );
        assert_eq!(
            crc16_modbus(&input) as u64,
            c["crc16_modbus"].as_u64().unwrap(),
            "crc16_modbus {}",
            c["name"]
        );
        assert_eq!(
            crc32_iso_hdlc(&input) as u64,
            c["crc32_iso_hdlc"].as_u64().unwrap(),
            "crc32_iso_hdlc {}",
            c["name"]
        );
        assert_eq!(
            crc16_ccitt_false(&input) as u64,
            c["crc16_ccitt_false"].as_u64().unwrap(),
            "crc16_ccitt_false {}",
            c["name"]
        );
        n += 1;
    }
    assert!(n > 0);
    eprintln!("crc.json: {} cases", n);
}

// ---- framing_encode.json ---------------------------------------------------

#[test]
fn framing_encode_vectors() {
    let doc = load("framing_encode.json");
    let mut n = 0;
    for c in doc["cases"].as_array().unwrap() {
        let cmd = c["cmd"].as_u64().unwrap() as u8;
        let seq = c["seq"].as_u64().unwrap() as u32;
        let crc_type = crc_type_from(c["crc_type"].as_u64().unwrap());
        let payload = hex_decode(c["payload"].as_str().unwrap());
        let frame = build_packet(cmd, &payload, seq, crc_type).unwrap();
        assert_eq!(
            hex_encode(&frame),
            c["frame"].as_str().unwrap(),
            "frame {}",
            c["name"]
        );
        n += 1;
    }
    assert!(n > 0);
    eprintln!("framing_encode.json: {} cases", n);
}

// ---- framing_decode.json ---------------------------------------------------

/// Normalized parser result: packet/crc_error event list, concatenated trash,
/// residue, and header-error count. Trash boundaries are intentionally dropped.
#[derive(Debug, PartialEq, Eq)]
struct DecodeResult {
    events: Vec<String>,
    trash: String,
    residue: String,
    header_errors: u64,
}

fn run_parser(stream: &[u8], chunks: &[&[u8]]) -> DecodeResult {
    let mut parser = PacketParser::new();
    let mut events = Vec::new();
    let mut trash = Vec::new();
    for chunk in chunks {
        for ev in parser.feed(chunk) {
            match ev {
                Event::Packet(p) => events.push(format!(
                    "packet:{}:{}:{}",
                    p.cmd,
                    p.seq,
                    hex_encode(&p.payload)
                )),
                Event::CrcError { cmd, seq } => {
                    events.push(format!("crc_error:{}:{}", cmd, seq))
                }
                Event::Trash(data) => trash.extend_from_slice(&data),
            }
        }
    }
    let _ = stream;
    DecodeResult {
        events,
        trash: hex_encode(&trash),
        residue: hex_encode(parser.residue()),
        header_errors: parser.header_errors,
    }
}

/// Deterministic pseudo-random split points (reproducible across runs).
fn random_chunks(stream: &[u8], seed: u64) -> Vec<&[u8]> {
    let mut chunks = Vec::new();
    let mut state = seed.wrapping_mul(0x9E37_79B9_7F4A_7C15).wrapping_add(1);
    let mut i = 0usize;
    while i < stream.len() {
        state = state
            .wrapping_mul(6364136223846793005)
            .wrapping_add(1442695040888963407);
        let max = stream.len() - i;
        let step = ((state >> 33) as usize % max) + 1;
        chunks.push(&stream[i..i + step]);
        i += step;
    }
    chunks
}

fn expected_result(expect: &Value) -> DecodeResult {
    let mut events = Vec::new();
    for ev in expect["events"].as_array().unwrap() {
        match ev["type"].as_str().unwrap() {
            "packet" => events.push(format!(
                "packet:{}:{}:{}",
                ev["cmd"].as_u64().unwrap(),
                ev["seq"].as_u64().unwrap(),
                ev["payload"].as_str().unwrap()
            )),
            "crc_error" => events.push(format!(
                "crc_error:{}:{}",
                ev["cmd"].as_u64().unwrap(),
                ev["seq"].as_u64().unwrap()
            )),
            other => panic!("unknown expected event type {}", other),
        }
    }
    DecodeResult {
        events,
        trash: expect["trash"].as_str().unwrap().to_string(),
        residue: expect["residue"].as_str().unwrap().to_string(),
        header_errors: expect["header_errors"].as_u64().unwrap(),
    }
}

#[test]
fn framing_decode_vectors_chunking_invariant() {
    let doc = load("framing_decode.json");
    let mut n = 0;
    for c in doc["cases"].as_array().unwrap() {
        let name = c["name"].as_str().unwrap();
        let stream = hex_decode(c["stream"].as_str().unwrap());
        let expected = expected_result(&c["expect"]);

        // whole stream in one chunk
        let whole = run_parser(&stream, &[&stream[..]]);
        assert_eq!(whole, expected, "{} (whole)", name);

        // byte-by-byte
        let single: Vec<&[u8]> = stream.iter().map(std::slice::from_ref).collect();
        let bybyte = run_parser(&stream, &single);
        assert_eq!(bybyte, expected, "{} (byte-by-byte)", name);

        // several random splittings
        for seed in 0..8u64 {
            let chunks = random_chunks(&stream, seed.wrapping_add(1) + name.len() as u64);
            let rnd = run_parser(&stream, &chunks);
            assert_eq!(rnd, expected, "{} (random seed {})", name, seed);
        }
        n += 1;
    }
    assert!(n > 0);
    eprintln!("framing_decode.json: {} cases x (whole/byte/8 random)", n);
}

// ---- usb_ids.json ----------------------------------------------------------

#[test]
fn usb_ids_vectors() {
    let doc = load("usb_ids.json");

    assert_eq!(doc["vid"].as_u64().unwrap() as u16, usb_ids::DEPZ_USB_VID);
    assert_eq!(doc["dev_vid"].as_u64().unwrap() as u16, usb_ids::DEV_USB_VID);
    assert_eq!(doc["dev_pid"].as_u64().unwrap() as u16, usb_ids::DEV_USB_PID);
    let range = doc["pid_range"].as_array().unwrap();
    assert_eq!(range[0].as_u64().unwrap() as u16, usb_ids::DEPZ_PID_RANGE.0);
    assert_eq!(range[1].as_u64().unwrap() as u16, usb_ids::DEPZ_PID_RANGE.1);

    let mut n = 0;
    for e in doc["identity"].as_array().unwrap() {
        let vid = Some(e["vid"].as_u64().unwrap() as u16);
        let pid = Some(e["pid"].as_u64().unwrap() as u16);
        assert_eq!(
            usb_ids::is_known_depz_usb(vid, pid),
            e["known"].as_bool().unwrap(),
            "known vid={:?} pid={:?}",
            vid,
            pid
        );
        let model = usb_ids::usb_model_hint(vid, pid);
        let want = e["model"].as_str();
        assert_eq!(model, want, "model vid={:?} pid={:?}", vid, pid);
        n += 1;
    }

    let mut m = 0;
    for c in doc["serial_ordering"].as_array().unwrap() {
        let ports: Vec<PortEntry> = c["ports"]
            .as_array()
            .unwrap()
            .iter()
            .map(|p| PortEntry {
                port: p["port"].as_str().unwrap().to_string(),
                serial: p["serial"].as_str().map(str::to_string),
            })
            .collect();
        let got: Vec<String> = usb_ids::order_ports(ports)
            .into_iter()
            .map(|p| p.port)
            .collect();
        let want: Vec<String> = c["order"]
            .as_array()
            .unwrap()
            .iter()
            .map(|v| v.as_str().unwrap().to_string())
            .collect();
        assert_eq!(got, want, "ordering {}", c["name"]);
        m += 1;
    }
    assert!(n > 0 && m > 0);
    eprintln!("usb_ids.json: {} identity + {} ordering cases", n, m);
}

// ---- common_commands.json --------------------------------------------------

#[test]
fn common_commands_vectors() {
    let doc = load("common_commands.json");

    let mut enc = 0;
    for c in doc["encode"].as_array().unwrap() {
        let want = c["payload"].as_str().unwrap();
        let got = match c["kind"].as_str().unwrap() {
            "sync_time_request" => {
                hex_encode(&common::pack_sync_time(c["pc_timestamp_us"].as_u64().unwrap()))
            }
            "set_payload_crc_type" => hex_encode(&common::pack_set_payload_crc_type(
                c["crc_type"].as_u64().unwrap() as u8,
            )),
            "sync_pin_config" => {
                let cfg = common::SyncPinConfig {
                    pin: c["pin"].as_u64().unwrap() as u8,
                    mode: c["mode"].as_u64().unwrap() as u8,
                    polarity: c["polarity"].as_u64().unwrap() as u8,
                };
                hex_encode(&cfg.pack())
            }
            other => panic!("unknown encode kind {}", other),
        };
        assert_eq!(got, want, "encode {}", c["name"]);
        enc += 1;
    }

    let mut dec = 0;
    for c in doc["decode"].as_array().unwrap() {
        let report = c["report"].as_u64().unwrap();
        let payload = hex_decode(c["payload"].as_str().unwrap());
        let expect = &c["expect"];
        match report {
            0x80 => {
                let r = common::StatusReport::unpack(&payload).unwrap();
                assert_eq!(r.cmd as u64, expect["cmd"].as_u64().unwrap());
                assert_eq!(r.status as u64, expect["status"].as_u64().unwrap());
            }
            0x81 => {
                let r = common::TextReport::unpack(&payload).unwrap();
                assert_eq!(r.cmd as u64, expect["cmd"].as_u64().unwrap());
                assert_eq!(r.text, expect["text"].as_str().unwrap());
            }
            0x83 => {
                let r = common::TemperatureReport::unpack(&payload).unwrap();
                assert_eq!(r.timestamp_us, expect["timestamp_us"].as_u64().unwrap());
                assert_eq!(
                    r.raw_decidegrees as i64,
                    expect["raw_decidegrees"].as_i64().unwrap()
                );
            }
            0x84 => {
                let r = common::SequenceErrorReport::unpack(&payload).unwrap();
                assert_eq!(r.expected_seq as u64, expect["expected_seq"].as_u64().unwrap());
                assert_eq!(r.received_seq as u64, expect["received_seq"].as_u64().unwrap());
            }
            other => panic!("unhandled report 0x{:02x} in {}", other, c["name"]),
        }
        dec += 1;
    }

    let mut math = 0;
    for c in doc["sync_time_math"].as_array().unwrap() {
        let (offset, rtt) = common::sync_time_offset_rtt(
            c["t1"].as_i64().unwrap(),
            c["t2"].as_i64().unwrap(),
            c["t3"].as_i64().unwrap(),
            c["t4"].as_i64().unwrap(),
        );
        assert_eq!(offset, c["offset_us"].as_i64().unwrap(), "offset {}", c["name"]);
        assert_eq!(rtt, c["rtt_us"].as_i64().unwrap(), "rtt {}", c["name"]);
        math += 1;
    }

    assert!(enc > 0 && dec > 0 && math > 0);
    eprintln!(
        "common_commands.json: {} encode + {} decode + {} sync_time_math",
        enc, dec, math
    );
}

// ---- identity.json ---------------------------------------------------------

#[test]
fn identity_vectors() {
    let doc = load("identity.json");
    let mut n = 0;
    for c in doc["cases"].as_array().unwrap() {
        // `raw` is wire bytes; strip trailing NUL/FF before parsing.
        let raw = hex_decode(c["raw"].as_str().unwrap());
        let name = common::strip_device_string(&raw);
        let id = identity::parse_software_name(&name);
        let expect = &c["expect"];

        assert_eq!(id.mode.as_str(), expect["mode"].as_str().unwrap(), "mode {}", c["name"]);
        match expect["sensor_type"].as_str() {
            Some(s) => assert_eq!(
                id.sensor_type.map(|t| t.as_str()),
                Some(s),
                "sensor_type {}",
                c["name"]
            ),
            None => assert_eq!(id.sensor_type, None, "sensor_type {}", c["name"]),
        }
        assert_eq!(
            id.software_name,
            expect["software_name"].as_str().unwrap(),
            "software_name {}",
            c["name"]
        );
        assert_eq!(id.version, expect["version"].as_str().unwrap(), "version {}", c["name"]);
        n += 1;
    }
    assert!(n > 0);
    eprintln!("identity.json: {} cases", n);
}

// ---- sr04.json -------------------------------------------------------------

#[test]
fn sr04_vectors() {
    let doc = load("sr04.json");

    let mut enc = 0;
    for c in doc["encode"].as_array().unwrap() {
        let want = c["payload"].as_str().unwrap();
        let got = match c["kind"].as_str().unwrap() {
            "set_sample_period" => {
                hex_encode(&sr04::pack_sample_period(c["period_us"].as_u64().unwrap() as u32))
            }
            "set_echo_decay" => {
                hex_encode(&sr04::pack_echo_decay(c["decay_us"].as_u64().unwrap() as u16))
            }
            other => panic!("unknown sr04 encode kind {}", other),
        };
        assert_eq!(got, want, "encode {}", c["name"]);
        enc += 1;
    }

    let mut dec = 0;
    for c in doc["decode"].as_array().unwrap() {
        let report = c["report"].as_u64().unwrap();
        let payload = hex_decode(c["payload"].as_str().unwrap());
        let expect = &c["expect"];
        match report {
            0x91 => {
                let d = sr04::Sr04Data::unpack(&payload).unwrap();
                assert_eq!(d.source_cmd as u64, expect["source_cmd"].as_u64().unwrap());
                assert_eq!(d.timestamp_us, expect["timestamp_us"].as_u64().unwrap());
                assert_eq!(d.echo_time_us as u64, expect["echo_time_us"].as_u64().unwrap());
            }
            0x92 => {
                let p = sr04::unpack_sample_period(&payload).unwrap();
                assert_eq!(p as u64, expect["period_us"].as_u64().unwrap());
            }
            0x93 => {
                let d = sr04::unpack_echo_decay(&payload).unwrap();
                assert_eq!(d as u64, expect["decay_us"].as_u64().unwrap());
            }
            other => panic!("unhandled sr04 report 0x{:02x} in {}", other, c["name"]),
        }
        dec += 1;
    }
    assert!(enc > 0 && dec > 0);
    eprintln!("sr04.json: {} encode + {} decode", enc, dec);
}

// ---- fwdepz.json -----------------------------------------------------------

#[test]
fn fwdepz_vectors() {
    let doc = load("fwdepz.json");
    let mut n = 0;
    for c in doc["cases"].as_array().unwrap() {
        let file = hex_decode(c["file"].as_str().unwrap());
        let result = fwdepz::parse(&file);
        if let Some(err) = c["error"].as_str() {
            let got = result.expect_err(&format!("expected error in {}", c["name"]));
            let got_str = match got {
                fwdepz::FwdepzError::Magic => "magic",
                fwdepz::FwdepzError::HeaderCrc => "header_crc",
                fwdepz::FwdepzError::Size => "size",
            };
            assert_eq!(got_str, err, "error kind {}", c["name"]);
        } else {
            let h = result.unwrap_or_else(|e| panic!("{} unexpected err {:?}", c["name"], e));
            let expect = &c["expect"];
            assert_eq!(h.load_addr as u64, expect["load_addr"].as_u64().unwrap());
            assert_eq!(h.fw_size as u64, expect["fw_size"].as_u64().unwrap());
            assert_eq!(h.fw_crc32 as u64, expect["fw_crc32"].as_u64().unwrap());
            assert_eq!(h.cur_sec as u64, expect["cur_sec"].as_u64().unwrap());
            assert_eq!(h.tot_sec as u64, expect["tot_sec"].as_u64().unwrap());
            assert_eq!(h.payload_crc_ok, expect["payload_crc_ok"].as_bool().unwrap());
        }
        n += 1;
    }
    assert!(n > 0);
    eprintln!("fwdepz.json: {} cases", n);
}
