//! VL53L5CX / VL53L7CX / VL53L7CH golden-vector conformance tests vs
//! `contracts/vectors/vl53l7.json` (contract 11): command encode (READ_REG at
//! the 1536-byte ceiling, WRITE_REG, PIN_CTRL, SET_I2C_SPEED), RPT_VL53_INFO
//! decode, and the class-resolution table.

use std::path::PathBuf;

use depz_sensor_sdk::protocol::identity::{self, SensorType};
use depz_sensor_sdk::vl53l7::{
    pack_pin_ctrl, pack_read_reg, pack_set_i2c_speed, pack_write_reg, resolve_model, Vl53l7Info,
    Vl53l7Rpt, INFO_SIZE, READ_MAX_LEN,
};
use serde_json::Value;

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
    let mut s = String::with_capacity(b.len() * 2);
    for x in b {
        s.push_str(&format!("{:02x}", x));
    }
    s
}

// ---- vl53l7.json: "encode" -------------------------------------------------

#[test]
fn vl53l7_encode_vectors() {
    let doc = load("vl53l7.json");
    let mut n = 0;
    for c in doc["encode"].as_array().unwrap() {
        let want = c["payload"].as_str().unwrap();
        let got = match c["kind"].as_str().unwrap() {
            "read_reg" => {
                let len = c["len"].as_u64().unwrap();
                assert!(len as usize <= READ_MAX_LEN, "{} exceeds READ_MAX_LEN", c["name"]);
                hex_encode(&pack_read_reg(c["addr"].as_u64().unwrap() as u16, len as u16))
            }
            "write_reg" => hex_encode(&pack_write_reg(
                c["addr"].as_u64().unwrap() as u16,
                &hex_decode(c["data"].as_str().unwrap()),
            )),
            "pin_ctrl" => hex_encode(&pack_pin_ctrl(c["action"].as_u64().unwrap() as u8)),
            "set_i2c_speed" => {
                hex_encode(&pack_set_i2c_speed(c["khz"].as_u64().unwrap() as u16))
            }
            other => panic!("unknown encode kind {:?} in {}", other, c["name"]),
        };
        assert_eq!(got, want, "encode {}", c["name"]);
        n += 1;
    }
    assert!(n > 0);
    eprintln!("vl53l7.json: {} encode cases", n);
}

// ---- vl53l7.json: "decode" (RPT_VL53_INFO 0x92) -----------------------------

#[test]
fn vl53l7_decode_vectors() {
    let doc = load("vl53l7.json");
    let mut n = 0;
    for c in doc["decode"].as_array().unwrap() {
        assert_eq!(c["report"].as_u64().unwrap(), Vl53l7Rpt::Info as u64, "{}", c["name"]);
        let info = Vl53l7Info::unpack(&hex_decode(c["payload"].as_str().unwrap()))
            .unwrap_or_else(|e| panic!("{}: {}", c["name"], e));
        let e = &c["expect"];
        let name = &c["name"];
        let u = |k: &str| e[k].as_u64().unwrap_or_else(|| panic!("{}: {}", name, k));
        assert_eq!(info.int_edges as u64, u("int_edges"), "{} int_edges", name);
        assert_eq!(info.frames_dropped as u64, u("frames_dropped"), "{} frames_dropped", name);
        assert_eq!(info.i2c_errors as u64, u("i2c_errors"), "{} i2c_errors", name);
        assert_eq!(info.last_i2c_error as u64, u("last_i2c_error"), "{} last_i2c_error", name);
        assert_eq!(info.lpn_level as u64, u("lpn_level"), "{} lpn_level", name);
        assert_eq!(info.int_level as u64, u("int_level"), "{} int_level", name);
        assert_eq!(info.i2c_khz as u64, u("i2c_khz"), "{} i2c_khz", name);
        assert_eq!(info.frame_size as u64, u("frame_size"), "{} frame_size", name);
        assert_eq!(info.streaming, e["streaming"].as_bool().unwrap(), "{} streaming", name);
        n += 1;
    }
    assert!(n > 0);
    eprintln!("vl53l7.json: {} decode cases", n);
}

#[test]
fn vl53l7_info_rejects_a_short_payload() {
    assert!(Vl53l7Info::unpack(&[0u8; INFO_SIZE - 1]).is_err());
    assert!(Vl53l7Info::unpack(&[0u8; INFO_SIZE]).is_ok());
}

// ---- vl53l7.json: "model" (class resolution) --------------------------------

#[test]
fn vl53l7_model_vectors() {
    let doc = load("vl53l7.json");
    let mut n = 0;
    for c in doc["model"].as_array().unwrap() {
        let got = resolve_model(c["usb_model"].as_str(), c["device_name"].as_str().unwrap());
        assert_eq!(got.as_str(), c["expect"].as_str().unwrap(), "model {}", c["name"]);
        n += 1;
    }
    assert!(n > 0);
    eprintln!("vl53l7.json: {} model cases", n);
}

#[test]
fn vl53l7_model_first_match_only() {
    // First regex match decides; a part without its own class (L5CH) is CX.
    assert_eq!(resolve_model(None, "VL53L5CH then VL53L7CH").as_str(), "vl53l7cx");
    assert_eq!(resolve_model(None, "VL53L8CH VL53L5CX").as_str(), "vl53l5cx");
    assert_eq!(resolve_model(Some("vl53l8ch"), "VL53L7CH").as_str(), "vl53l7ch");
}

#[test]
fn vl53l7_identity() {
    let id = identity::parse_software_name("APP_VL53L7_v0.53");
    assert_eq!(id.sensor_type, Some(SensorType::Vl53l7));
    assert_eq!(id.sensor_type.unwrap().as_str(), "vl53l7");
    assert_eq!(id.version, "0.53");
}
