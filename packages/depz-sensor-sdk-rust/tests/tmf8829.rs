//! TMF8829 golden-vector conformance tests vs `contracts/vectors/tmf8829.json`
//! and `contracts/vectors/tmf8829_frame_time.json` (contract 14): encode
//! (0x32..0x37) and the refused payloads, report decode (REG_DATA, INFO,
//! FRAME chunk, EVENT), chunk reassembly, real-frame parse + pixel decode,
//! measurement assembly, config-field get/set, the embedded firmware image,
//! and the bit-exact capture-time model over 27 recorded streams.

mod common;

use depz_sensor_sdk::tmf8829::{
    cfg_field, cfg_get, cfg_set, firmware_image, firmware_words, pack_get_info, pack_read_reg,
    pack_start_stream, pack_stop_stream, pack_write_reg, CaptureTimer, RegData, SensorClock,
    CFG_FIELDS, FIRMWARE_LOAD_ADDR,
};
use serde_json::{json, Value};

use common::shapes::{self, sha256};
use common::{hex_decode, hex_encode, load, recordings_dir, seen};

fn u(v: &Value) -> u64 {
    v.as_u64().unwrap_or_else(|| panic!("not an unsigned int: {}", v))
}

fn i(v: &Value) -> i64 {
    v.as_i64().unwrap_or_else(|| panic!("not an int: {}", v))
}

fn s(v: &Value) -> &str {
    v.as_str().unwrap_or_else(|| panic!("not a string: {}", v))
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
        seen(c);
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
        seen(c);
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
        let got = shapes::tmf8829_decode(u(&c["report"]), &hex_decode(s(&c["payload"])))
            .unwrap_or_else(|e| panic!("{}: {}", name, e));
        assert_eq!(got, c["expect"], "{}", name);
        seen(c);
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
        let chunks: Vec<Vec<u8>> =
            c["chunks"].as_array().unwrap().iter().map(|ch| hex_decode(s(ch))).collect();
        let got = shapes::tmf8829_reassembly(&chunks).unwrap_or_else(|e| panic!("{}: {}", name, e));
        assert_eq!(got, c["expect"], "{}", name);
        seen(c);
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
        let got = shapes::tmf8829_frame(&hex_decode(s(&c["raw"])))
            .unwrap_or_else(|e| panic!("{}: {}", name, e));
        let mut want = json!({"header": c["header"], "problems": c["problems"]});
        for k in ["results", "data_sha256"] {
            if let Some(v) = c.get(k) {
                want[k] = v.clone();
            }
        }
        assert_eq!(got, want, "{}", name);
        seen(c);
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
        let frames: Vec<(Vec<u8>, u64, u64)> = sc["frames"]
            .as_array()
            .unwrap()
            .iter()
            .map(|f| (raw_by_case[s(&f["raw_case"])].clone(), u(&f["timestamp_us"]), u(&f["edge_us"])))
            .collect();
        assert_eq!(shapes::tmf8829_measurements(&frames), sc["expect"], "{}", name);
        seen(sc);
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
    seen("tmf8829/config");
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
    seen("tmf8829/firmware");
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
        seen(st);
    }
    assert_eq!(checked, 970);
}

// ---- recordings/tmf8829_frame_time.jsonl -----------------------------------

/// The board's own frame-time capture that `tmf8829_frame_time.json` is built
/// from, replayed straight from the JSONL against each row's `exp`.
#[test]
fn tmf8829_frame_time_recording() {
    let text = std::fs::read_to_string(recordings_dir().join("tmf8829_frame_time.jsonl")).unwrap();
    let (mut streams, mut checked) = (0, 0);
    let mut state: Option<(SensorClock, CaptureTimer)> = None;
    for (no, line) in text.lines().enumerate().filter(|(_, l)| !l.trim().is_empty()) {
        let v: Value = serde_json::from_str(line).unwrap();
        if v.get("scenario").is_some() {
            state = Some((SensorClock::new(), CaptureTimer::new()));
            streams += 1;
            continue;
        }
        let (clock, timer) = state.as_mut().expect("frame before a scenario line");
        let fid = u(&v["id"]) as u8;
        let kind = fid & 0xF0;
        if kind == 0x30 {
            continue;
        }
        let tick = u(&v["systick"]) as u32;
        let flags = clock.add(u(&v["ts"]), tick);
        if kind == 0x20 {
            timer.histograms();
            continue;
        }
        let t = timer.results(
            clock,
            fid & 0x0F,
            tick,
            u(&v["t0"]) as u32,
            u(&v["t1"]) as u32,
            u(&v["ts"]),
            u(&v["edge_us"]),
            flags,
        );
        let e = &v["exp"];
        assert_eq!(
            (t.timestamp_us, t.t1_len_us as i64, t.flags as i64, clock.ratio_q16 as i64),
            (i(&e["timestamp_us"]), i(&e["t1_len_us"]), i(&e["flags"]), i(&e["ratio_q16"])),
            "line {}",
            no + 1
        );
        checked += 1;
    }
    assert_eq!((streams, checked), (27, 970));
    seen("recordings/tmf8829_frame_time.jsonl");
}
