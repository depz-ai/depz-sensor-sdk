//! TMF8829 golden-vector conformance tests vs `contracts/vectors/tmf8829.json`
//! and `contracts/vectors/tmf8829_frame_time.json` (contract 14): encode
//! (0x32..0x37) and the refused payloads, report decode (REG_DATA, INFO,
//! FRAME chunk, EVENT), chunk reassembly, real-frame parse + pixel decode,
//! measurement assembly, config-field get/set, the embedded firmware image,
//! and the bit-exact capture-time model over 27 recorded streams.

use std::path::PathBuf;

use depz_sensor_sdk::tmf8829::{
    cfg_field, cfg_get, cfg_set, firmware_image, firmware_words, pack_get_info, pack_read_reg,
    pack_start_stream, pack_stop_stream, pack_write_reg, parse_frame, parse_results_info,
    CaptureTimer, FrameChunk, FrameReassembler, MeasurementBuilder, PixelResults, RegData,
    SensorClock, Tmf8829Event, Tmf8829Info, CFG_FIELDS, FIRMWARE_LOAD_ADDR, FID_RESULTS,
};
use serde_json::{json, Value};

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
    let txt =
        std::fs::read_to_string(&p).unwrap_or_else(|e| panic!("read {}: {}", p.display(), e));
    serde_json::from_str(&txt).unwrap_or_else(|e| panic!("parse {}: {}", p.display(), e))
}

fn hex_decode(s: &str) -> Vec<u8> {
    assert!(s.len().is_multiple_of(2), "odd hex length: {:?}", s);
    (0..s.len())
        .step_by(2)
        .map(|i| u8::from_str_radix(&s[i..i + 2], 16).expect("bad hex"))
        .collect()
}

fn hex_encode(b: &[u8]) -> String {
    b.iter().map(|x| format!("{:02x}", x)).collect()
}

fn u(v: &Value) -> u64 {
    v.as_u64().unwrap_or_else(|| panic!("not an unsigned int: {}", v))
}

fn i(v: &Value) -> i64 {
    v.as_i64().unwrap_or_else(|| panic!("not an int: {}", v))
}

fn s(v: &Value) -> &str {
    v.as_str().unwrap_or_else(|| panic!("not a string: {}", v))
}

/// Minimal SHA-256 (FIPS 180-4) — the crate has no hash dependency and the
/// vectors pin frame / firmware bytes by digest.
fn sha256(data: &[u8]) -> String {
    const K: [u32; 64] = [
        0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4,
        0xab1c5ed5, 0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe,
        0x9bdc06a7, 0xc19bf174, 0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f,
        0x4a7484aa, 0x5cb0a9dc, 0x76f988da, 0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
        0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967, 0x27b70a85, 0x2e1b2138, 0x4d2c6dfc,
        0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85, 0xa2bfe8a1, 0xa81a664b,
        0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070, 0x19a4c116,
        0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
        0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7,
        0xc67178f2,
    ];
    let mut h: [u32; 8] = [
        0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a, 0x510e527f, 0x9b05688c, 0x1f83d9ab,
        0x5be0cd19,
    ];
    let mut msg = data.to_vec();
    msg.push(0x80);
    while msg.len() % 64 != 56 {
        msg.push(0);
    }
    msg.extend_from_slice(&((data.len() as u64) * 8).to_be_bytes());
    for block in msg.chunks(64) {
        let mut w = [0u32; 64];
        for t in 0..16 {
            w[t] = u32::from_be_bytes(block[t * 4..t * 4 + 4].try_into().unwrap());
        }
        for t in 16..64 {
            let s0 = w[t - 15].rotate_right(7) ^ w[t - 15].rotate_right(18) ^ (w[t - 15] >> 3);
            let s1 = w[t - 2].rotate_right(17) ^ w[t - 2].rotate_right(19) ^ (w[t - 2] >> 10);
            w[t] = w[t - 16]
                .wrapping_add(s0)
                .wrapping_add(w[t - 7])
                .wrapping_add(s1);
        }
        let mut v = h;
        for t in 0..64 {
            let s1 = v[4].rotate_right(6) ^ v[4].rotate_right(11) ^ v[4].rotate_right(25);
            let ch = (v[4] & v[5]) ^ (!v[4] & v[6]);
            let t1 = v[7]
                .wrapping_add(s1)
                .wrapping_add(ch)
                .wrapping_add(K[t])
                .wrapping_add(w[t]);
            let s0 = v[0].rotate_right(2) ^ v[0].rotate_right(13) ^ v[0].rotate_right(22);
            let maj = (v[0] & v[1]) ^ (v[0] & v[2]) ^ (v[1] & v[2]);
            let t2 = s0.wrapping_add(maj);
            v = [
                t1.wrapping_add(t2),
                v[0],
                v[1],
                v[2],
                v[3].wrapping_add(t1),
                v[4],
                v[5],
                v[6],
            ];
        }
        for k in 0..8 {
            h[k] = h[k].wrapping_add(v[k]);
        }
    }
    h.iter().map(|x| format!("{:08x}", x)).collect()
}

/// `PixelResults` → the vectors' flat-array object (as the Python test's `_arrays`).
fn arrays(r: &PixelResults) -> Value {
    fn flat<T: Copy + Into<u64>>(a: &Option<Vec<T>>) -> Value {
        match a {
            None => Value::Null,
            Some(v) => Value::from(v.iter().map(|&x| x.into()).collect::<Vec<u64>>()),
        }
    }
    let (rows, cols) = r.shape();
    json!({
        "peaks": r.peaks,
        "rows": rows,
        "cols": cols,
        "distance": flat(&r.distance),
        "snr": flat(&r.snr),
        "signal": flat(&r.signal),
        "noise": flat(&r.noise),
        "xtalk": flat(&r.xtalk),
    })
}

// ---- encode ----------------------------------------------------------------

#[test]
fn tmf8829_encode_vectors() {
    let doc = load("tmf8829.json");
    let mut n = 0;
    for c in doc["encode"].as_array().unwrap() {
        let name = s(&c["name"]);
        let got: Vec<u8> = match u(&c["cmd"]) {
            0x32 => pack_read_reg(u(&c["addr"]) as u8, u(&c["len"]) as u16)
                .unwrap()
                .to_vec(),
            0x33 => pack_write_reg(u(&c["addr"]) as u8, &hex_decode(s(&c["data"]))).unwrap(),
            0x35 => pack_start_stream(u(&c["int_mask"]) as u8, u(&c["flags"]) as u8)
                .unwrap()
                .to_vec(),
            0x36 => pack_stop_stream().to_vec(),
            0x37 => pack_get_info().to_vec(),
            k => panic!("{}: unknown cmd 0x{:02X}", name, k),
        };
        assert_eq!(hex_encode(&got), s(&c["payload"]), "{}", name);
        n += 1;
    }
    assert_eq!(n, 13);
}

#[test]
fn tmf8829_encode_error_vectors() {
    let doc = load("tmf8829.json");
    let mut n = 0;
    for c in doc["encode_errors"].as_array().unwrap() {
        let name = s(&c["name"]);
        let refused = match s(&c["kind"]) {
            "read_reg" => {
                // len 1025 does not fit the wire's u16 range check, not the type
                let len = u(&c["len"]);
                assert!(len <= u16::MAX as u64, "{}", name);
                pack_read_reg(u(&c["addr"]) as u8, len as u16).is_err()
            }
            "write_reg" => pack_write_reg(u(&c["addr"]) as u8, &hex_decode(s(&c["data"]))).is_err(),
            "start_stream" => {
                pack_start_stream(u(&c["int_mask"]) as u8, u(&c["flags"]) as u8).is_err()
            }
            k => panic!("{}: unknown kind {:?}", name, k),
        };
        assert!(refused, "{} must be refused", name);
        n += 1;
    }
    assert_eq!(n, 7);
}

// ---- decode ----------------------------------------------------------------

#[test]
fn tmf8829_decode_vectors() {
    let doc = load("tmf8829.json");
    let mut n = 0;
    for c in doc["decode"].as_array().unwrap() {
        let name = s(&c["name"]);
        let raw = hex_decode(s(&c["payload"]));
        let got = match u(&c["report"]) {
            0x91 => {
                let r = RegData::unpack(&raw).unwrap();
                json!({"timestamp_us": r.timestamp_us, "data": hex_encode(&r.data)})
            }
            0x92 => {
                let i = Tmf8829Info::unpack(&raw).unwrap();
                json!({
                    "int_edges": i.int_edges, "frames": i.frames,
                    "frames_aborted": i.frames_aborted, "frame_errors": i.frame_errors,
                    "spi_timeouts": i.spi_timeouts, "int_level": i.int_level,
                    "streaming": i.streaming, "int_mask": i.int_mask,
                    "last_int_status": i.last_int_status,
                    "last_fifo_status": i.last_fifo_status,
                })
            }
            0x93 => {
                let f = FrameChunk::unpack(&raw).unwrap();
                json!({
                    "timestamp_us": f.timestamp_us, "full_size": f.full_size,
                    "offset": f.offset, "frame_seq": f.frame_seq, "edge_us": f.edge_us,
                    "data": hex_encode(&f.data),
                })
            }
            0x94 => {
                let e = Tmf8829Event::unpack(&raw).unwrap();
                json!({
                    "timestamp_us": e.timestamp_us, "int_status": e.int_status,
                    "code": e.code, "frame_id": e.frame_id, "fifo_status": e.fifo_status,
                })
            }
            k => panic!("{}: unknown report 0x{:02X}", name, k),
        };
        assert_eq!(got, c["expect"], "{}", name);
        n += 1;
    }
    assert_eq!(n, 6);
    // REG_DATA must echo 0x32
    assert!(RegData::unpack(&[0x33, 0, 0, 0, 0, 0, 0, 0, 0]).is_err());
}

// ---- reassembly ------------------------------------------------------------

#[test]
fn tmf8829_reassembly_vectors() {
    let doc = load("tmf8829.json");
    let mut n = 0;
    for c in doc["reassembly"].as_array().unwrap() {
        let name = s(&c["name"]);
        let mut r = FrameReassembler::new();
        let mut done = Vec::new();
        for ch in c["chunks"].as_array().unwrap() {
            let chunk = FrameChunk::unpack(&hex_decode(s(ch))).unwrap();
            if let Some(f) = r.feed(chunk) {
                done.push(json!({
                    "frame_seq": f.frame_seq,
                    "timestamp_us": f.timestamp_us,
                    "edge_us": f.edge_us,
                    "size": f.data.len(),
                    "sha256": sha256(&f.data),
                }));
            }
        }
        let got = json!({
            "frames": done,
            "completed": r.completed,
            "discarded": r.discarded,
            "seq_gaps": r.seq_gaps,
        });
        assert_eq!(got, c["expect"], "{}", name);
        n += 1;
    }
    assert_eq!(n, 7);
}

// ---- frames ----------------------------------------------------------------

#[test]
fn tmf8829_frame_vectors() {
    let doc = load("tmf8829.json");
    let mut n = 0;
    for c in doc["frames"].as_array().unwrap() {
        let name = s(&c["name"]);
        let raw = hex_decode(s(&c["raw"]));
        let f = parse_frame(&raw).unwrap();
        let got = json!({
            "fifo_status": f.fifo_status, "systick": f.systick, "id": f.id,
            "kind": f.kind(), "fp_mode": f.fp_mode(), "layout": f.layout,
            "payload": f.payload, "f_number": f.f_number,
            "temperature": f.temperature.to_vec(), "bdv": f.bdv,
            "ref_pos": f.ref_pos.to_vec(),
            "t0_integration": f.t0_integration, "t1_integration": f.t1_integration,
            "frame_status": f.frame_status, "reserved": f.reserved, "eof": f.eof,
            "size": f.size,
        });
        let h = &c["header"];
        for (k, v) in got.as_object().unwrap() {
            assert_eq!(v, &h[k.as_str()], "{}.{}", name, k);
        }
        let problems: Vec<Value> = f.problems().into_iter().map(Value::from).collect();
        assert_eq!(Value::from(problems), c["problems"], "{}", name);
        if f.kind() == FID_RESULTS {
            let res = parse_results_info(&raw, &f).unwrap();
            assert_eq!(arrays(&res), c["results"], "{}", name);
        } else {
            assert_eq!(
                sha256(&raw[21..raw.len() - 12]),
                s(&c["data_sha256"]),
                "{}",
                name
            );
        }
        n += 1;
    }
    assert_eq!(n, 18);
}

// ---- measurements ----------------------------------------------------------

#[test]
fn tmf8829_measurement_vectors() {
    let doc = load("tmf8829.json");
    let raw_by_case: std::collections::HashMap<&str, Vec<u8>> = doc["frames"]
        .as_array()
        .unwrap()
        .iter()
        .map(|c| (s(&c["name"]), hex_decode(s(&c["raw"]))))
        .collect();
    let mut n = 0;
    for sc in doc["measurements"].as_array().unwrap() {
        let name = s(&sc["name"]);
        let mut b = MeasurementBuilder::new();
        let mut got = Vec::new();
        for f in sc["frames"].as_array().unwrap() {
            let raw = &raw_by_case[s(&f["raw_case"])];
            let Some(m) = b.add(raw, u(&f["timestamp_us"]), u(&f["edge_us"])) else {
                continue;
            };
            let mut item = json!({
                "timestamp_us": m.timestamp_us,
                "flags": m.flags,
                "fp_mode": m.fp_mode,
                "f_number": m.f_number,
                "frame_times_us": m.times.iter().map(|t| t.timestamp_us).collect::<Vec<_>>(),
                "t1_len_us": m.times.iter().map(|t| t.t1_len_us).collect::<Vec<_>>(),
                "ratio_q16": b.clock.ratio_q16,
                "n_histograms": m.histograms.len(),
                "n_ref_frames": m.ref_frames.len(),
                "n_problems": m.problems.len(),
                "distance_in_mm": m.frames.last().unwrap().distance_in_mm(),
            });
            let arr = arrays(&m.results);
            item.as_object_mut()
                .unwrap()
                .extend(arr.as_object().unwrap().clone());
            got.push(item);
        }
        assert_eq!(Value::from(got), sc["expect"], "{}", name);
        n += 1;
    }
    assert_eq!(n, 9);
}

// ---- config ----------------------------------------------------------------

#[test]
fn tmf8829_config_vectors() {
    let cfg = &load("tmf8829.json")["config"];
    // the field table itself
    let table = cfg["fields"].as_object().unwrap();
    assert_eq!(table.len(), CFG_FIELDS.len());
    for (name, v) in table {
        let f = cfg_field(name).unwrap_or_else(|| panic!("missing field {}", name));
        let want: Vec<u64> = v.as_array().unwrap().iter().map(u).collect();
        assert_eq!(
            vec![f.reg as u64, f.shift as u64, f.width as u64],
            want,
            "{}",
            name
        );
    }
    let page = hex_decode(s(&cfg["page"]));
    for (name, v) in cfg["get"].as_object().unwrap() {
        assert_eq!(cfg_get(&page, name).unwrap() as u64, u(v), "{}", name);
    }
    for c in cfg["set"].as_array().unwrap() {
        let mut pg = page.clone();
        cfg_set(&mut pg, s(&c["field"]), u(&c["value"]) as u32).unwrap();
        assert_eq!(hex_encode(&pg), s(&c["page"]), "{}", s(&c["field"]));
    }
    for c in cfg["set_errors"].as_array().unwrap() {
        let mut pg = page.clone();
        assert!(
            cfg_set(&mut pg, s(&c["field"]), u(&c["value"]) as u32).is_err(),
            "{}",
            s(&c["field"])
        );
        assert_eq!(pg, page);
    }
}

// ---- firmware --------------------------------------------------------------

#[test]
fn tmf8829_firmware_image() {
    let fw = &load("tmf8829.json")["firmware"];
    let img = firmware_image();
    assert_eq!(FIRMWARE_LOAD_ADDR as u64, u(&fw["load_addr"]));
    assert_eq!(img.len() as u64, u(&fw["size"]));
    assert_eq!(firmware_words() as u64, u(&fw["words"]));
    assert_eq!((img.len() / 4) as u64, u(&fw["words"]));
    assert_eq!(sha256(img), s(&fw["sha256"]));
}

// ---- capture time (tmf8829_frame_time.json) --------------------------------

#[test]
fn tmf8829_frame_time_vectors() {
    let v = load("tmf8829_frame_time.json");
    let col: std::collections::HashMap<&str, usize> = v["columns"]
        .as_array()
        .unwrap()
        .iter()
        .enumerate()
        .map(|(k, c)| (s(c), k))
        .collect();
    let mut checked = 0;
    for st in v["streams"].as_array().unwrap() {
        let mut clock = SensorClock::new();
        let mut timer = CaptureTimer::new();
        for row in st["frames"].as_array().unwrap() {
            let at = |k: &str| &row[col[k]];
            let fid = u(at("id")) as u8;
            let ts = u(at("ts"));
            let tick = u(at("systick")) as u32;
            let kind = fid & 0xF0;
            if kind == 0x30 {
                continue;
            }
            let flags = clock.add(ts, tick);
            if kind == 0x20 {
                timer.histograms();
                continue;
            }
            let t = timer.results(
                &clock,
                fid & 0x0F,
                tick,
                u(at("t0")) as u32,
                u(at("t1")) as u32,
                ts,
                u(at("edge_us")),
                flags,
            );
            assert_eq!(
                (
                    t.timestamp_us,
                    t.t1_len_us as i64,
                    t.flags as i64,
                    clock.ratio_q16 as i64
                ),
                (
                    i(at("exp_timestamp_us")),
                    i(at("exp_t1_len_us")),
                    i(at("exp_flags")),
                    i(at("exp_ratio_q16"))
                ),
                "{} {:?}",
                s(&st["scenario"]),
                row
            );
            checked += 1;
        }
    }
    assert_eq!(checked, 970);
}
