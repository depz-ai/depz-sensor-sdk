//! VL53L 1D-family golden-vector conformance tests vs
//! `contracts/vectors/vl53lx.json` (contract 12): encode (SET_ADDR_WIDTH,
//! START_STREAM with clear lists, contract-10 codecs), RPT_VL53_INFO decode,
//! the product table, class/product resolution, and the three stateless block
//! decoders (die block l4/l1, VL53L0X raw, histogram raw).

use std::path::PathBuf;

use depz_sensor_sdk::protocol::identity::{self, SensorType};
use depz_sensor_sdk::usb_ids::DEPZ_USB_VID;
use depz_sensor_sdk::usb_model_hint;
use depz_sensor_sdk::vl53lx::{
    decode_die_block, decode_histogram_raw, decode_l0x_raw, pack_read_reg, pack_set_addr_width,
    pack_set_i2c_speed, pack_start_stream, pack_write_reg, pack_xshut, product,
    product_from_board_name, resolve_class, DieVariant, DriverKind, Vl53lxInfo, DIE_BLOCK_LEN,
    HISTOGRAM_BLOCK_LEN, INFO_SIZE, L0X_BLOCK_LEN, PRODUCTS,
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
    b.iter().map(|x| format!("{:02x}", x)).collect()
}

fn u(v: &Value) -> u64 {
    v.as_u64().unwrap_or_else(|| panic!("not an unsigned int: {}", v))
}

fn steps(v: &Value) -> Vec<(u16, u8)> {
    v.as_array()
        .unwrap()
        .iter()
        .map(|s| (u(&s[0]) as u16, u(&s[1]) as u8))
        .collect()
}

// ---- encode ----------------------------------------------------------------

#[test]
fn vl53lx_encode_vectors() {
    let doc = load("vl53lx.json");
    let mut n = 0;
    for c in doc["encode"].as_array().unwrap() {
        let name = c["name"].as_str().unwrap();
        let got: Vec<u8> = match c["kind"].as_str().unwrap() {
            "set_addr_width" => pack_set_addr_width(u(&c["width"]) as u8).unwrap().to_vec(),
            "read_reg" => pack_read_reg(u(&c["addr"]) as u16, u(&c["len"]) as u16).to_vec(),
            "write_reg" => {
                pack_write_reg(u(&c["addr"]) as u16, &hex_decode(c["data"].as_str().unwrap()))
            }
            "xshut" => pack_xshut(u(&c["action"]) as u8).to_vec(),
            "set_i2c_speed" => pack_set_i2c_speed(u(&c["khz"]) as u16).to_vec(),
            "start_stream" => {
                let clear = steps(&c["clear"]);
                let p = pack_start_stream(
                    u(&c["addr"]) as u16,
                    u(&c["len"]) as u16,
                    &clear,
                    u(&c["flags"]) as u8,
                )
                .unwrap();
                assert_eq!(p.len(), 6 + 3 * clear.len(), "{}", name);
                p
            }
            k => panic!("{}: unknown encode kind {:?}", name, k),
        };
        assert_eq!(hex_encode(&got), c["payload"].as_str().unwrap(), "{}", name);
        n += 1;
    }
    assert_eq!(n, 11);
}

#[test]
fn vl53lx_encode_refusals() {
    assert!(pack_start_stream(0x0089, 17, &[(0x86, 1); 5], 0).is_err());
    assert!(pack_start_stream(0x0089, 17, &[(0x86, 1); 4], 0).is_ok());
    assert!(pack_set_addr_width(0).is_err());
    assert!(pack_set_addr_width(3).is_err());
}

// ---- decode ----------------------------------------------------------------

#[test]
fn vl53lx_decode_vectors() {
    let doc = load("vl53lx.json");
    let mut n = 0;
    for c in doc["decode"].as_array().unwrap() {
        let name = c["name"].as_str().unwrap();
        assert_eq!(u(&c["report"]), 0x92, "{}", name);
        let i = Vl53lxInfo::unpack(&hex_decode(c["payload"].as_str().unwrap())).unwrap();
        let e = &c["expect"];
        let got = [
            ("int_edges", i.int_edges as u64),
            ("slots_skipped", i.slots_skipped as u64),
            ("i2c_errors", i.i2c_errors as u64),
            ("last_i2c_error", i.last_i2c_error as u64),
            ("xshut_level", i.xshut_level as u64),
            ("int_level", i.int_level as u64),
            ("i2c_khz", i.i2c_khz as u64),
            ("addr_width", i.addr_width as u64),
            ("n_clear", i.n_clear as u64),
            ("frames_dropped", i.frames_dropped as u64),
        ];
        assert_eq!(e.as_object().unwrap().len(), got.len(), "{}", name);
        for (k, v) in got {
            assert_eq!(v, u(&e[k]), "{}.{}", name, k);
        }
        n += 1;
    }
    assert_eq!(n, 3);
    assert!(Vl53lxInfo::unpack(&[0u8; INFO_SIZE - 1]).is_err());
}

// ---- products --------------------------------------------------------------

#[test]
fn vl53lx_product_table() {
    let doc = load("vl53lx.json");
    let rows = doc["products"].as_array().unwrap();
    let names: Vec<&str> = rows.iter().map(|r| r["product"].as_str().unwrap()).collect();
    let ours: Vec<&str> = PRODUCTS.iter().map(|p| p.name).collect();
    assert_eq!(names, ours);
    for r in rows {
        let name = r["product"].as_str().unwrap();
        let p = product(name).unwrap();
        assert_eq!(p.model_id as u64, u(&r["model_id"]), "{} model_id", name);
        assert!(p.model_id_ok(u(&r["model_id"]) as u16), "{}", name);
        assert_eq!(p.reach_mm as u64, u(&r["reach_mm"]), "{} reach", name);
        let kinds: Vec<&str> = p.driver_kinds().iter().map(|k| k.as_str()).collect();
        let want: Vec<&str> =
            r["driver_kinds"].as_array().unwrap().iter().map(|k| k.as_str().unwrap()).collect();
        assert_eq!(kinds, want, "{} driver kinds", name);
        assert_eq!(p.default_driver.as_str(), r["default_driver"].as_str().unwrap(), "{}", name);
        let bus = p.default_bus_params();
        assert_eq!(bus.addr_width as u64, u(&r["addr_width"]), "{} addr width", name);
        assert_eq!(bus.max_khz as u64, u(&r["max_khz"]), "{} max khz", name);
        assert_eq!(bus.clear_steps.to_vec(), steps(&r["clear_steps"]), "{} clear", name);
        // The production PID is the one the USB id table names.
        assert_eq!(
            usb_model_hint(Some(DEPZ_USB_VID), Some(p.usb_pid)),
            Some(name.to_ascii_lowercase().as_str()),
            "{} pid",
            name
        );
    }
    // A missing pair is a refusal, never a fallback.
    assert!(product("VL53L4CX").unwrap().bus_params(DriverKind::Uld).is_none());
    assert!(product("VL53L0X").unwrap().bus_params(DriverKind::Histogram).is_none());
    assert!(product("VL53L3CX").unwrap().bus_params(DriverKind::Uld).is_none());
    let variant = |p: &str| product(p).unwrap().default_bus_params().die_variant;
    assert_eq!(variant("VL53L1CX"), Some(DieVariant::L1));
    assert_eq!(variant("VL53L4CD"), Some(DieVariant::L4));
    assert!(product("VL53L5CX").is_none());
}

// ---- model -----------------------------------------------------------------

#[test]
fn vl53lx_model_vectors() {
    let doc = load("vl53lx.json");
    let mut n = 0;
    for c in doc["model"].as_array().unwrap() {
        let name = c["name"].as_str().unwrap();
        let usb_model = c["usb_model"].as_str();
        let device_name = c["device_name"].as_str().unwrap();
        assert_eq!(
            resolve_class(usb_model, device_name).as_str(),
            c["expect_class"].as_str().unwrap(),
            "{} class",
            name
        );
        assert_eq!(
            product_from_board_name(device_name),
            c["expect_product"].as_str(),
            "{} product",
            name
        );
        n += 1;
    }
    assert_eq!(n, 7);
}

// ---- die_block / l0x_raw / histogram_raw -----------------------------------

#[test]
fn vl53lx_die_block_vectors() {
    let doc = load("vl53lx.json");
    let mut n = 0;
    for c in doc["die_block"].as_array().unwrap() {
        let name = c["name"].as_str().unwrap();
        let variant = DieVariant::from_name(c["variant"].as_str().unwrap()).unwrap();
        let r = decode_die_block(&hex_decode(c["raw"].as_str().unwrap()), variant).unwrap();
        let e = &c["expect"];
        let got = [
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
        assert_eq!(e.as_object().unwrap().len(), got.len(), "{}", name);
        for (k, v) in got {
            assert_eq!(v, u(&e[k]), "{}.{}", name, k);
        }
        n += 1;
    }
    assert!(n > 0);
    assert!(decode_die_block(&[0u8; DIE_BLOCK_LEN - 1], DieVariant::L4).is_err());
    // spads = 0 → per-SPAD rates are 0, not a division by zero.
    let no_spads = [0x09, 0, 0, 0, 0, 0xFF, 0xFF, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0];
    let r = decode_die_block(&no_spads, DieVariant::L4).unwrap();
    assert_eq!((r.signal_per_spad_kcps, r.ambient_per_spad_kcps), (0, 0));
    eprintln!("vl53lx.json die_block: {} cases", n);
}

#[test]
fn vl53lx_l0x_raw_vectors() {
    let doc = load("vl53lx.json");
    let mut n = 0;
    for c in doc["l0x_raw"].as_array().unwrap() {
        let name = c["name"].as_str().unwrap();
        let r = decode_l0x_raw(&hex_decode(c["raw"].as_str().unwrap())).unwrap();
        let e = &c["expect"];
        let got = [
            ("distance_raw", r.distance_raw as u64),
            ("device_range_status", r.device_range_status as u64),
            ("signal_rate_mcps_1616", r.signal_rate_mcps_1616 as u64),
            ("ambient_rate_mcps_1616", r.ambient_rate_mcps_1616 as u64),
            ("effective_spad_count_88", r.effective_spad_count_88 as u64),
        ];
        assert_eq!(e.as_object().unwrap().len(), got.len(), "{}", name);
        for (k, v) in got {
            assert_eq!(v, u(&e[k]), "{}.{}", name, k);
        }
        n += 1;
    }
    assert!(n > 0);
    assert!(decode_l0x_raw(&[0u8; L0X_BLOCK_LEN - 1]).is_err());
}

#[test]
fn vl53lx_histogram_raw_vectors() {
    let doc = load("vl53lx.json");
    let mut n = 0;
    for c in doc["histogram_raw"].as_array().unwrap() {
        let name = c["name"].as_str().unwrap();
        let r = decode_histogram_raw(&hex_decode(c["raw"].as_str().unwrap())).unwrap();
        let e = &c["expect"];
        let got = [
            ("interrupt_status", r.interrupt_status as u64),
            ("range_status", r.range_status as u64),
            ("report_status", r.report_status as u64),
            ("stream_count", r.stream_count as u64),
            ("dss_actual_effective_spads", r.dss_actual_effective_spads as u64),
            ("reference_phase", r.reference_phase as u64),
            ("vcsel_start", r.vcsel_start as u64),
        ];
        assert_eq!(e.as_object().unwrap().len(), got.len() + 1, "{}", name);
        for (k, v) in got {
            assert_eq!(v, u(&e[k]), "{}.{}", name, k);
        }
        let bins: Vec<u64> = e["bins"].as_array().unwrap().iter().map(u).collect();
        let ours: Vec<u64> = r.bins.iter().map(|&b| b as u64).collect();
        assert_eq!(ours, bins, "{}.bins", name);
        n += 1;
    }
    assert!(n > 0);
    assert!(decode_histogram_raw(&[0u8; HISTOGRAM_BLOCK_LEN - 1]).is_err());
}

// ---- identity.json: the 1D-family firmware names ----------------------------

#[test]
fn vl53lx_identity() {
    for name in ["APP_VL53L0_4_v0.23", "APP_VL53LX_v0.20"] {
        let id = identity::parse_software_name(name);
        assert_eq!(id.sensor_type, Some(SensorType::Vl53lx), "{}", name);
        assert_eq!(id.sensor_type.unwrap().as_str(), "vl53lx");
    }
    // The contract-10 bridge keeps its own type.
    assert_eq!(
        identity::parse_software_name("APP_VL53L4_v1.00").sensor_type,
        Some(SensorType::Vl53l4)
    );
}
