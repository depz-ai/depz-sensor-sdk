//! VL53L4CD golden-vector conformance tests vs `contracts/vectors/vl53l4.json`
//! (contract 10): command encode, report decode, result-block decode, the
//! SetRangeTiming/GetRangeTiming register math, tuning codecs (both
//! directions) and the 91-byte init config block.

mod common;

use depz_sensor_sdk::protocol::identity::{self, SensorType};
use depz_sensor_sdk::vl53l4::{
    config_block, decode_offset, decode_range_timing, decode_sigma_threshold,
    decode_signal_threshold, decode_xtalk, try_offset_raw, pack_read_reg, pack_set_i2c_speed,
    pack_start_stream, pack_write_reg, pack_xshut, parse_result_block, range_timing_registers,
    sigma_threshold_raw, signal_threshold_raw, try_xtalk_raw, RegData, StreamData, Vl53l4Info,
    CONFIG_ADDR, DEFAULT_CONFIGURATION,
};
use serde_json::Value;

use common::{hex_decode, hex_encode, load, recorded_text, recording_packets, reg_reads, seen};

// ---- vl53l4.json: "encode" -------------------------------------------------

#[test]
fn vl53l4_encode_vectors() {
    let doc = load("vl53l4.json");
    let mut n = 0;
    for c in doc["encode"].as_array().unwrap() {
        let want = c["payload"].as_str().unwrap();
        let got = match c["kind"].as_str().unwrap() {
            "read_reg" => hex_encode(&pack_read_reg(
                c["addr"].as_u64().unwrap() as u16,
                c["len"].as_u64().unwrap() as u16,
            )),
            "write_reg" => hex_encode(&pack_write_reg(
                c["addr"].as_u64().unwrap() as u16,
                &hex_decode(c["data"].as_str().unwrap()),
            )),
            "xshut" => hex_encode(&pack_xshut(c["action"].as_u64().unwrap() as u8)),
            "start_stream" => hex_encode(&pack_start_stream(
                c["addr"].as_u64().unwrap() as u16,
                c["len"].as_u64().unwrap() as u16,
                c["flags"].as_u64().unwrap() as u8,
            )),
            "set_i2c_speed" => {
                hex_encode(&pack_set_i2c_speed(c["khz"].as_u64().unwrap() as u16))
            }
            other => panic!("unknown vl53l4 encode kind {}", other),
        };
        assert_eq!(got, want, "encode {}", c["name"]);
        seen(c);
        n += 1;
    }
    assert!(n > 0);
    eprintln!("vl53l4.json: {} encode cases", n);
}

// ---- vl53l4.json: "decode" (0x91 / 0x92 / 0x93) ----------------------------

#[test]
fn vl53l4_decode_vectors() {
    let doc = load("vl53l4.json");
    let mut n = 0;
    for c in doc["decode"].as_array().unwrap() {
        let report = c["report"].as_u64().unwrap();
        let payload = hex_decode(c["payload"].as_str().unwrap());
        let expect = &c["expect"];
        match report {
            0x91 => {
                let r = RegData::unpack(&payload).unwrap();
                assert_eq!(r.cmd as u64, expect["cmd"].as_u64().unwrap());
                assert_eq!(r.timestamp_us, expect["timestamp_us"].as_u64().unwrap());
                assert_eq!(hex_encode(&r.data), expect["data"].as_str().unwrap());
            }
            0x92 => {
                let r = Vl53l4Info::unpack(&payload).unwrap();
                assert_eq!(r.int_edges as u64, expect["int_edges"].as_u64().unwrap());
                assert_eq!(r.slots_skipped as u64, expect["slots_skipped"].as_u64().unwrap());
                assert_eq!(r.i2c_errors as u64, expect["i2c_errors"].as_u64().unwrap());
                assert_eq!(r.last_i2c_error as u64, expect["last_i2c_error"].as_u64().unwrap());
                assert_eq!(r.model_id as u64, expect["model_id"].as_u64().unwrap());
                assert_eq!(r.fw_status as u64, expect["fw_status"].as_u64().unwrap());
                assert_eq!(r.initialized as u64, expect["initialized"].as_u64().unwrap());
                assert_eq!(r.xshut_level as u64, expect["xshut_level"].as_u64().unwrap());
                assert_eq!(r.int_level as u64, expect["int_level"].as_u64().unwrap());
                assert_eq!(r.i2c_khz as u64, expect["i2c_khz"].as_u64().unwrap());
            }
            0x93 => {
                let r = StreamData::unpack(&payload).unwrap();
                assert_eq!(r.timestamp_us, expect["timestamp_us"].as_u64().unwrap());
                assert_eq!(r.addr as u64, expect["addr"].as_u64().unwrap());
                assert_eq!(r.len as u64, expect["len"].as_u64().unwrap());
                assert_eq!(hex_encode(&r.data), expect["data"].as_str().unwrap());
            }
            other => panic!("unhandled vl53l4 report 0x{:02x} in {}", other, c["name"]),
        }
        seen(c);
        n += 1;
    }
    assert!(n > 0);
    eprintln!("vl53l4.json: {} decode cases", n);
}

// ---- vl53l4.json: "result_block" -------------------------------------------

#[test]
fn vl53l4_result_block_vectors() {
    let doc = load("vl53l4.json");
    let mut n = 0;
    for c in doc["result_block"].as_array().unwrap() {
        let raw = hex_decode(c["raw"].as_str().unwrap());
        let r = parse_result_block(&raw).unwrap();
        let expect = &c["expect"];
        assert_eq!(r.range_status as u64, expect["range_status"].as_u64().unwrap(), "{}", c["name"]);
        assert_eq!(r.distance_mm as u64, expect["distance_mm"].as_u64().unwrap(), "{}", c["name"]);
        assert_eq!(
            r.ambient_rate_kcps as u64,
            expect["ambient_rate_kcps"].as_u64().unwrap(),
            "{}",
            c["name"]
        );
        assert_eq!(
            r.ambient_per_spad_kcps as u64,
            expect["ambient_per_spad_kcps"].as_u64().unwrap(),
            "{}",
            c["name"]
        );
        assert_eq!(
            r.signal_rate_kcps as u64,
            expect["signal_rate_kcps"].as_u64().unwrap(),
            "{}",
            c["name"]
        );
        assert_eq!(
            r.signal_per_spad_kcps as u64,
            expect["signal_per_spad_kcps"].as_u64().unwrap(),
            "{}",
            c["name"]
        );
        assert_eq!(
            r.number_of_spad as u64,
            expect["number_of_spad"].as_u64().unwrap(),
            "{}",
            c["name"]
        );
        assert_eq!(r.sigma_mm as u64, expect["sigma_mm"].as_u64().unwrap(), "{}", c["name"]);
        assert_eq!(r.stream_count as u64, expect["stream_count"].as_u64().unwrap(), "{}", c["name"]);
        seen(c);
        n += 1;
    }
    assert!(n > 0);
    assert!(parse_result_block(&[0u8; 14]).is_err(), "short block must error");
    eprintln!("vl53l4.json: {} result_block cases", n);
}

// ---- vl53l4.json: "timing" (encode + decode) -------------------------------

#[test]
fn vl53l4_timing_vectors() {
    let doc = load("vl53l4.json");

    let mut enc = 0;
    for c in doc["timing"]["encode"].as_array().unwrap() {
        let (a, b, raw) = range_timing_registers(
            c["timing_budget_ms"].as_u64().unwrap() as u32,
            c["inter_measurement_ms"].as_u64().unwrap() as u32,
            c["osc_frequency"].as_u64().unwrap() as u16,
            c["clock_pll"].as_u64().unwrap() as u16,
        )
        .unwrap();
        assert_eq!(a as u64, c["range_config_a"].as_u64().unwrap(), "A {}", c["name"]);
        assert_eq!(b as u64, c["range_config_b"].as_u64().unwrap(), "B {}", c["name"]);
        assert_eq!(raw as u64, c["intermeasurement_raw"].as_u64().unwrap(), "raw {}", c["name"]);
        enc += 1;
    }

    let mut dec = 0;
    for c in doc["timing"]["decode"].as_array().unwrap() {
        let (budget_ms, inter_ms) = decode_range_timing(
            c["intermeasurement_raw"].as_u64().unwrap() as u32,
            c["clock_pll"].as_u64().unwrap() as u16,
            c["osc_frequency"].as_u64().unwrap() as u16,
            c["range_config_a"].as_u64().unwrap() as u16,
        )
        .unwrap();
        assert_eq!(budget_ms as u64, c["timing_budget_ms"].as_u64().unwrap(), "budget {}", c["name"]);
        assert_eq!(
            inter_ms as u64,
            c["inter_measurement_ms"].as_u64().unwrap(),
            "inter {}",
            c["name"]
        );
        dec += 1;
    }

    assert!(enc > 0 && dec > 0);
    // Contract error paths: osc == 0, budget out of 10..=200, inter <= budget.
    assert!(range_timing_registers(50, 0, 0, 0).is_err());
    assert!(range_timing_registers(9, 0, 14720, 0).is_err());
    assert!(range_timing_registers(201, 0, 14720, 0).is_err());
    assert!(range_timing_registers(50, 50, 14720, 0).is_err());
    assert!(decode_range_timing(0, 2652, 0, 403).is_err());
    eprintln!("vl53l4.json: {} timing encode + {} timing decode", enc, dec);
    seen("vl53l4/timing");
}

// ---- vl53l4.json: "tuning" (both directions) -------------------------------

#[test]
fn vl53l4_tuning_vectors() {
    let doc = load("vl53l4.json");
    let mut n = 0;
    for c in doc["tuning"].as_array().unwrap() {
        let value = c["value"].as_i64().unwrap();
        let raw = c["raw"].as_u64().unwrap() as u16;
        match c["kind"].as_str().unwrap() {
            "offset" => {
                assert_eq!(try_offset_raw(value as i32).unwrap(), raw, "offset raw {}", c["name"]);
                assert_eq!(decode_offset(raw) as i64, value, "offset decode {}", c["name"]);
            }
            "xtalk" => {
                assert_eq!(try_xtalk_raw(value as u16).unwrap(), raw, "xtalk raw {}", c["name"]);
                assert_eq!(decode_xtalk(raw) as i64, value, "xtalk decode {}", c["name"]);
            }
            "signal_threshold" => {
                assert_eq!(signal_threshold_raw(value as u16), raw, "signal raw {}", c["name"]);
                assert_eq!(
                    decode_signal_threshold(raw) as i64,
                    value,
                    "signal decode {}",
                    c["name"]
                );
            }
            "sigma_threshold" => {
                assert_eq!(
                    sigma_threshold_raw(value as u16).unwrap(),
                    raw,
                    "sigma raw {}",
                    c["name"]
                );
                assert_eq!(decode_sigma_threshold(raw) as i64, value, "sigma decode {}", c["name"]);
            }
            other => panic!("unknown vl53l4 tuning kind {}", other),
        }
        seen(c);
        n += 1;
    }
    assert!(n > 0);
    assert!(sigma_threshold_raw(16384).is_err(), "sigma_mm > 16383 must error");
    eprintln!("vl53l4.json: {} tuning cases (both directions)", n);
}

// ---- vl53l4.json: "xtalk_decode" -------------------------------------------

/// C `round()`: an exact .5 goes away from zero (256 -> 1), as ST's GetXtalk.
#[test]
fn vl53l4_xtalk_decode_vectors() {
    let doc = load("vl53l4.json");
    let cases = doc["xtalk_decode"].as_array().unwrap();
    assert!(!cases.is_empty());
    for c in cases {
        let raw = c["raw"].as_u64().unwrap() as u16;
        assert_eq!(decode_xtalk(raw) as u64, c["kcps"].as_u64().unwrap(), "xtalk decode {}", raw);
        seen(c);
    }
}

// ---- vl53l4.json: "config_block" -------------------------------------------

#[test]
fn vl53l4_config_block_vector() {
    let doc = load("vl53l4.json");
    let cb = &doc["config_block"];
    assert_eq!(cb["addr"].as_u64().unwrap() as u16, CONFIG_ADDR);
    let block = config_block();
    assert_eq!(hex_encode(&block), cb["data"].as_str().unwrap());
    // Byte 0 (register 0x2D) is forced to the I2C FM+ pad value; the rest is
    // the ST default configuration untouched.
    assert_eq!(block[0], 0x12);
    assert_eq!(block[1..], DEFAULT_CONFIGURATION[1..]);
    seen("vl53l4/config_block");
    eprintln!("vl53l4.json: config_block ok ({} bytes)", block.len());
}

// ---- identity: APP_VL53L4 firmware-name parsing ----------------------------

#[test]
fn vl53l4_identity_parse() {
    let id = identity::parse_software_name("APP_VL53L4_v0.80");
    assert_eq!(id.mode.as_str(), "app");
    assert_eq!(id.sensor_type, Some(SensorType::Vl53l4));
    assert_eq!(id.sensor_type.unwrap().as_str(), "vl53l4");
    assert_eq!(id.version, "0.80");
}

// ---- recordings/vl53l4cd_session.depzrec -----------------------------------

/// A sidecar measurement (timestamp + the nine result fields) vs a decoded
/// result block.
fn assert_measurement(raw: &[u8], timestamp_us: u64, want: &Value, ctx: &str) {
    let r = parse_result_block(raw).unwrap_or_else(|e| panic!("{}: {:?}", ctx, e));
    let got = [
        ("timestamp_us", timestamp_us),
        ("range_status", r.range_status as u64),
        ("distance_mm", r.distance_mm as u64),
        ("sigma_mm", r.sigma_mm as u64),
        ("signal_rate_kcps", r.signal_rate_kcps as u64),
        ("ambient_rate_kcps", r.ambient_rate_kcps as u64),
        ("signal_per_spad_kcps", r.signal_per_spad_kcps as u64),
        ("ambient_per_spad_kcps", r.ambient_per_spad_kcps as u64),
        ("number_of_spad", r.number_of_spad as u64),
        ("stream_count", r.stream_count as u64),
    ];
    assert_eq!(want.as_object().unwrap().len(), got.len(), "{}", ctx);
    for (k, v) in got {
        assert_eq!(v, want[k].as_u64().unwrap(), "{}.{}", ctx, k);
    }
}

/// Every rx report of the live capture through framing + the VL53L4 decoders
/// and the ULD register math, vs the `.expected.json` sidecar (no device
/// layer: not a strict tx replay). Read-backs are paired with their READ_REG.
#[test]
fn vl53l4cd_session_recording() {
    use depz_sensor_sdk::protocol::common::Cmd;
    use depz_sensor_sdk::vl53l4::uld_math::{
        INTERMEASUREMENT_MS, OSC_FREQUENCY, RANGE_CONFIG_A, RANGE_OFFSET_MM, RESULT_BLOCK_ADDR,
        RESULT_BLOCK_LEN, RESULT__OSC_CALIBRATE_VAL, XTALK_PLANE_OFFSET_KCPS,
    };

    let exp = load("recordings/vl53l4cd_session.expected.json");
    let pkts = recording_packets("vl53l4cd_session.depzrec");
    let sw = recorded_text(&pkts, Cmd::GetNameActiveSoftware).expect("software name");
    assert_eq!(identity::parse_software_name(&sw).sensor_type, Some(SensorType::Vl53l4));
    assert_eq!(sw, exp["software_name"].as_str().unwrap());
    assert_eq!(recorded_text(&pkts, Cmd::GetDeviceName).as_deref(), exp["device_name"].as_str());

    let mut info = None;
    let mut stream = Vec::new();
    for p in pkts.iter().filter(|p| p.rx) {
        match p.cmd {
            0x92 if info.is_none() => info = Some(Vl53l4Info::unpack(&p.payload).unwrap()),
            0x93 => {
                let s = StreamData::unpack(&p.payload).unwrap();
                assert_eq!((s.addr, s.len), (RESULT_BLOCK_ADDR, RESULT_BLOCK_LEN), "stream block");
                stream.push(s);
            }
            _ => {}
        }
    }
    let info = info.expect("RPT_VL53_INFO");
    let wi = &exp["info"];
    assert_eq!(wi.as_object().unwrap().len(), 3);
    assert_eq!(info.model_id as u64, wi["model_id"].as_u64().unwrap());
    assert_eq!(info.fw_status as u64, wi["fw_status"].as_u64().unwrap());
    assert_eq!(info.i2c_khz as u64, wi["i2c_khz"].as_u64().unwrap());

    // Read-backs: the last read of each register is what the getters saw.
    let be = |b: &[u8]| b.iter().fold(0u64, |v, &x| (v << 8) | x as u64);
    let mut last = std::collections::HashMap::new();
    let mut once = Vec::new();
    for r in reg_reads(&pkts, true) {
        let d = RegData::unpack(&r.reply).unwrap();
        assert_eq!(d.data.len(), r.len as usize, "read {:#06x}", r.addr);
        if (r.addr, r.len) == (RESULT_BLOCK_ADDR, RESULT_BLOCK_LEN) {
            once.push(d.clone());
        }
        last.insert(r.addr, d.data);
    }
    let reg = |addr: u16| be(&last[&addr]);
    let (budget, inter) = decode_range_timing(
        reg(INTERMEASUREMENT_MS) as u32,
        reg(RESULT__OSC_CALIBRATE_VAL) as u16,
        reg(OSC_FREQUENCY) as u16,
        reg(RANGE_CONFIG_A) as u16,
    )
    .unwrap();
    let timing: Vec<u64> = exp["timing"].as_array().unwrap().iter().map(|v| v.as_u64().unwrap()).collect();
    assert_eq!(vec![budget as u64, inter as u64], timing, "timing");
    assert_eq!(decode_offset(reg(RANGE_OFFSET_MM) as u16) as i64, exp["offset_mm"].as_i64().unwrap());
    assert_eq!(
        decode_xtalk(reg(XTALK_PLANE_OFFSET_KCPS) as u16) as u64,
        exp["xtalk_kcps"].as_u64().unwrap()
    );

    let want_once = exp["once"].as_array().unwrap();
    assert_eq!(once.len(), want_once.len(), "once reads");
    for (idx, (d, w)) in once.iter().zip(want_once).enumerate() {
        assert_measurement(&d.data, d.timestamp_us, w, &format!("once {}", idx));
    }
    // The stream may run past the sidecar before the stop lands; match the first N.
    let want = exp["frames"].as_array().unwrap();
    assert!(stream.len() >= want.len(), "{} < {} frames", stream.len(), want.len());
    for (idx, (s, w)) in stream.iter().zip(want).enumerate() {
        assert_measurement(&s.data, s.timestamp_us, w, &format!("frame {}", idx));
    }
    seen("recordings/vl53l4cd_session.depzrec");
}

/// Regression (difftest): an `osc_frequency` so small that the macro period
/// truncates to 0 used to divide by zero (panic); an autonomous period whose
/// register overflows u32 used to saturate; out-of-range tuning values used
/// to wrap. All are errors now (contract 07 §3).
#[test]
fn vl53l4_math_rejects_instead_of_panicking() {
    use depz_sensor_sdk::vl53l4::Vl53l4Error;
    for osc in [1u16, 2, 4, 8] {
        assert_eq!(range_timing_registers(50, 0, osc, 0), Err(Vl53l4Error::MacroPeriodZero), "osc {}", osc);
    }
    assert_eq!(
        range_timing_registers(87, 2_147_483_647, 13927, 2559),
        Err(Vl53l4Error::InterMeasurementOverflow)
    );
    assert_eq!(try_offset_raw(-1024), Ok(61440));
    assert_eq!(try_offset_raw(1024), Err(Vl53l4Error::OffsetOutOfRange));
    assert_eq!(try_offset_raw(-1025), Err(Vl53l4Error::OffsetOutOfRange));
    assert_eq!(try_xtalk_raw(127), Ok(127 << 9));
    assert_eq!(try_xtalk_raw(128), Err(Vl53l4Error::XtalkOutOfRange));
}
