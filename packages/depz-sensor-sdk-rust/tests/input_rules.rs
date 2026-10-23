//! Contract 07 §3 input rules vs every `input_rules` group of
//! `contracts/vectors/*.json`: a short device payload is rejected, trailing
//! bytes are ignored, and an encoder argument out of its range or wire width
//! is refused before anything is written.
//!
//! Integers: the Rust API is typed (`u8`, `u16`, `u32`, …). A value that does
//! not fit the parameter's type cannot be passed by a Rust caller at all, so
//! such a case holds when the conversion into that type is refused — the
//! type is the range check. Every other case calls the SDK function.

mod common;

use depz_sensor_sdk::bno055::{self as b055, GyroConfig, Units};
use depz_sensor_sdk::bno086::reports::{try_parse_gyro_rv_cargo, try_parse_input_cargo};
use depz_sensor_sdk::bno086::sh2;
use depz_sensor_sdk::bno086::shtp::{try_pack_shtp_header, ShtpHeader, ShtpLayer};
use depz_sensor_sdk::framing::{build_packet, CrcType};
use depz_sensor_sdk::protocol::{common as proto, sr04};
use depz_sensor_sdk::tmf8829 as tmf;
use depz_sensor_sdk::vl53l4 as l4;
use depz_sensor_sdk::vl53l7 as l7;
use depz_sensor_sdk::usb_ids;
use depz_sensor_sdk::vl53l8::driver::Vl53l8CnhSetup;
use depz_sensor_sdk::vl53l8::advanced::{try_pack_detection_thresholds, try_xtalk_margin_to_raw, DetectionThreshold};
use depz_sensor_sdk::vl53lx as lx;
use serde_json::{json, Value};

use common::{hex_decode, hex_encode, load, seen, shapes};

/// Cases this SDK cannot run, with the reason (they stay in
/// contracts/VECTOR_COVERAGE_EXCEPTIONS.json).
const NOT_HERE: &[(&str, &str)] = &[];

type R = Result<Value, String>;

/// `v` as the API's integer type `T`, else the refusal a Rust caller meets.
fn int<T: TryFrom<i128>>(v: &Value, what: &str) -> Result<T, String> {
    let wide: i128 = match (v.as_u64(), v.as_i64()) {
        (Some(x), _) => x as i128,
        (None, Some(x)) => x as i128,
        _ => return Err(format!("{}: not an integer: {}", what, v)),
    };
    T::try_from(wide).map_err(|_| format!("{}: {} does not fit the API type", what, wide))
}

fn n<T: TryFrom<i128>>(i: &Value, k: &str) -> Result<T, String> {
    int(&i[k], k)
}

fn hex(i: &Value, k: &str) -> Vec<u8> {
    hex_decode(i[k].as_str().unwrap_or_else(|| panic!("{} is not hex", k)))
}

fn dbg<E: std::fmt::Debug>(e: E) -> String {
    format!("{:?}", e)
}

fn payload(p: &[u8]) -> R {
    Ok(json!({ "payload": hex_encode(p) }))
}

fn run(op: &str, i: &Value) -> R {
    match op {
        "bno055_encode" => match i["kind"].as_str().unwrap() {
            "read_reg" => payload(&b055::pack_read_reg(n(i, "addr")?, n(i, "len")?)),
            "start_stream" => payload(&b055::pack_start_stream(
                n(i, "trigger")?,
                n(i, "addr")?,
                n(i, "len")?,
                n(i, "period_ms")?,
            )),
            "write_reg" => payload(&b055::try_pack_write_reg(n(i, "addr")?, &hex(i, "data")).map_err(dbg)?),
            k => panic!("bno055_encode kind {}", k),
        },
        "bno055_decode" => shapes::bno055_decode(n(i, "report")?, &hex(i, "payload")),
        "bno055_units" => Ok(json!({ "repack": Units::unpack(n(i, "unit_sel")?).pack() })),
        "bno055_sensor_config" => {
            let g = GyroConfig::unpack(&hex(i, "bytes")).map_err(dbg)?;
            Ok(json!({ "range": g.range, "bandwidth": g.bandwidth, "power": g.power }))
        }
        "bno086_shtp_header_pack" => {
            let hdr = ShtpHeader {
                length: n(i, "length")?,
                channel: n(i, "channel")?,
                seq: n(i, "seq")?,
                continuation: i["continuation"].as_bool().unwrap(),
            };
            Ok(json!({ "bytes": hex_encode(&try_pack_shtp_header(&hdr).map_err(dbg)?) }))
        }
        "bno086_shtp_tx_seq" => {
            let mut layer = ShtpLayer::new();
            let mut frames = Vec::new();
            for p in i["packets"].as_array().unwrap() {
                frames.push(hex_encode(&layer.try_next_frame(n(p, "channel")?, &hex(p, "payload")).map_err(dbg)?));
            }
            Ok(json!({ "frames": frames }))
        }
        // the only cases are refusals: capture_timestamp_us is the API's u64
        "bno086_input_cargo" => {
            let reps = try_parse_input_cargo(&hex(i, "cargo"), n(i, "capture_timestamp_us")?).map_err(dbg)?;
            Ok(json!({ "reports": reps.len() }))
        }
        "bno086_gyro_rv" => {
            let r = try_parse_gyro_rv_cargo(&hex(i, "cargo"), n(i, "capture_timestamp_us")?).map_err(dbg)?;
            Ok(json!({ "report": r.is_some() }))
        }
        "bno086_shtp_reassembly" => {
            let frames = i["frames"].as_array().unwrap();
            let captures = i["captures_us"].as_array().unwrap();
            assert_eq!(frames.len(), captures.len(), "frames / captures_us length");
            let mut layer = ShtpLayer::new();
            let mut cargos = Vec::new();
            for (f, t) in frames.iter().zip(captures) {
                let frame = hex_decode(f.as_str().unwrap());
                if let Some(c) = layer.feed_at(&frame, int::<u64>(t, "captures_us")?) {
                    cargos.push(shapes::bno086_cargo(&c));
                }
            }
            Ok(json!({ "cargos": cargos, "discarded": layer.discarded }))
        }
        "bno086_control_encode" => match i["kind"].as_str().unwrap() {
            "set_feature" => payload(&sh2::build_set_feature(
                n(i, "sensor_id")?,
                n(i, "interval_us")?,
                n(i, "batch_us")?,
                n(i, "sensitivity")?,
                n(i, "flags")?,
                n(i, "cfg_word")?,
            )),
            "command_request" => {
                payload(&sh2::build_command_request(n(i, "seq")?, n(i, "command")?, &hex(i, "params")).map_err(dbg)?)
            }
            "frs_write_data" => {
                let words = i["words"].as_array().unwrap().iter().map(|w| int::<u32>(w, "words")).collect::<Result<Vec<_>, _>>()?;
                payload(&sh2::build_frs_write_data(n(i, "offset_words")?, &words).map_err(dbg)?)
            }
            k => panic!("bno086_control_encode kind {}", k),
        },
        "common_decode" => {
            let p = hex(i, "payload");
            match i["report"].as_u64().unwrap() {
                0x80 => {
                    let r = proto::StatusReport::unpack(&p).map_err(dbg)?;
                    Ok(json!({ "cmd": r.cmd, "status": r.status }))
                }
                0x81 => {
                    let r = proto::TextReport::unpack(&p).map_err(dbg)?;
                    Ok(json!({ "cmd": r.cmd, "text": r.text }))
                }
                0x83 => {
                    let r = proto::TemperatureReport::unpack(&p).map_err(dbg)?;
                    Ok(json!({ "timestamp_us": r.timestamp_us, "raw_decidegrees": r.raw_decidegrees }))
                }
                0x84 => {
                    let r = proto::SequenceErrorReport::unpack(&p).map_err(dbg)?;
                    Ok(json!({ "expected_seq": r.expected_seq, "received_seq": r.received_seq }))
                }
                r => panic!("common_decode report {:#x}", r),
            }
        }
        "common_encode" => match i["kind"].as_str().unwrap() {
            "sync_pin_config" => payload(
                &proto::SyncPinConfig { pin: n(i, "pin")?, mode: n(i, "mode")?, polarity: n(i, "polarity")? }
                    .try_pack()
                    .map_err(dbg)?,
            ),
            "sync_time_request" => payload(&proto::pack_sync_time(n(i, "pc_timestamp_us")?)),
            k => panic!("common_encode kind {}", k),
        },
        "framing_encode" => {
            let crc = CrcType::from_value(n(i, "crc_type")?).ok_or("crc_type is not a CrcType")?;
            let frame = build_packet(n(i, "cmd")?, &hex(i, "payload"), n(i, "seq")?, crc).map_err(dbg)?;
            Ok(json!({ "frame": hex_encode(&frame) }))
        }
        "sr04_decode" => {
            let p = hex(i, "payload");
            match i["report"].as_u64().unwrap() {
                0x91 => {
                    let d = sr04::Sr04Data::unpack(&p).map_err(dbg)?;
                    Ok(json!({ "source_cmd": d.source_cmd, "timestamp_us": d.timestamp_us, "echo_time_us": d.echo_time_us }))
                }
                0x93 => Ok(json!({ "decay_us": sr04::unpack_echo_decay(&p).map_err(dbg)? })),
                r => panic!("sr04_decode report {:#x}", r),
            }
        }
        "sr04_encode" => match i["kind"].as_str().unwrap() {
            "set_sample_period" => payload(&sr04::pack_sample_period(n(i, "period_us")?)),
            "set_echo_decay" => payload(&sr04::pack_echo_decay(n(i, "decay_us")?)),
            k => panic!("sr04_encode kind {}", k),
        },
        "tmf8829_encode" => match i["cmd"].as_u64().unwrap() {
            0x35 => payload(&tmf::pack_start_stream(n(i, "int_mask")?, n(i, "flags")?).map_err(dbg)?),
            c => panic!("tmf8829_encode cmd {:#x}", c),
        },
        "tmf8829_config_get" => {
            Ok(json!({ "value": tmf::cfg_get(&hex(i, "page"), i["field"].as_str().unwrap()).map_err(dbg)? }))
        }
        "tmf8829_config_set" => {
            let mut pg = hex(i, "page");
            tmf::cfg_set(&mut pg, i["field"].as_str().unwrap(), n(i, "value")?).map_err(dbg)?;
            Ok(json!({ "page": hex_encode(&pg) }))
        }
        "vl53l4_encode" => match i["kind"].as_str().unwrap() {
            "read_reg" => payload(&l4::pack_read_reg(n(i, "addr")?, n(i, "len")?)),
            "xshut" => payload(&l4::pack_xshut(n(i, "action")?)),
            "write_reg" => payload(&l4::try_pack_write_reg(n(i, "addr")?, &hex(i, "data")).map_err(dbg)?),
            k => panic!("vl53l4_encode kind {}", k),
        },
        "vl53l4_timing_encode" => {
            let (a, b, raw) = l4::range_timing_registers(
                n(i, "timing_budget_ms")?,
                n(i, "inter_measurement_ms")?,
                n(i, "osc_frequency")?,
                n(i, "clock_pll")?,
            )
            .map_err(dbg)?;
            Ok(json!({ "range_config_a": a, "range_config_b": b, "intermeasurement_raw": raw }))
        }
        "vl53l4_tuning_encode" => {
            let raw = match i["kind"].as_str().unwrap() {
                "offset" => l4::try_offset_raw(n(i, "value")?).map_err(dbg)?,
                "xtalk" => l4::try_xtalk_raw(n(i, "value")?).map_err(dbg)?,
                "signal_threshold" => l4::signal_threshold_raw(n(i, "value")?),
                "sigma_threshold" => l4::sigma_threshold_raw(n(i, "value")?).map_err(dbg)?,
                k => panic!("vl53l4_tuning_encode kind {}", k),
            };
            Ok(json!({ "raw": raw }))
        }
        "vl53l4_tuning_decode" => {
            let raw: u16 = n(i, "raw")?;
            match i["kind"].as_str().unwrap() {
                "sigma_threshold" => Ok(json!({ "value": l4::decode_sigma_threshold(raw) })),
                k => panic!("vl53l4_tuning_decode kind {}", k),
            }
        }
        "vl53l4_decode" => shapes::vl53l4_decode(n(i, "report")?, &hex(i, "payload")),
        "vl53l4_timing_decode" => {
            let (budget, inter) = l4::decode_range_timing(
                n(i, "intermeasurement_raw")?,
                n(i, "clock_pll")?,
                n(i, "osc_frequency")?,
                n(i, "range_config_a")?,
            )
            .map_err(dbg)?;
            Ok(json!({ "timing_budget_ms": budget, "inter_measurement_ms": inter }))
        }
        "usb_identity" => {
            let (vid, pid) = (Some(n::<u16>(i, "vid")?), Some(n::<u16>(i, "pid")?));
            Ok(json!({ "known": usb_ids::is_known_depz_usb(vid, pid), "model": usb_ids::usb_model_hint(vid, pid) }))
        }
        "vl53l8_xtalk_margin" => {
            let kcps = i["kcps"].as_f64().ok_or("kcps: not a number")?;
            Ok(json!({ "raw": try_xtalk_margin_to_raw(kcps).map_err(dbg)? }))
        }
        "vl53l7_encode" => match i["kind"].as_str().unwrap() {
            "pin_ctrl" => payload(&l7::pack_pin_ctrl(n(i, "action")?)),
            "read_reg" => payload(&l7::pack_read_reg(n(i, "addr")?, n(i, "len")?).map_err(dbg)?),
            "write_reg" => payload(&l7::pack_write_reg(n(i, "addr")?, &hex(i, "data")).map_err(dbg)?),
            k => panic!("vl53l7_encode kind {}", k),
        },
        "vl53l8_detection_thresholds" => {
            let t = i["thresholds"]
                .as_array()
                .unwrap()
                .iter()
                .map(|x| {
                    Ok(DetectionThreshold {
                        low_thresh: n(x, "low_thresh")?,
                        high_thresh: n(x, "high_thresh")?,
                        measurement: n(x, "measurement")?,
                        type_: n(x, "type")?,
                        zone_num: n(x, "zone_num")?,
                        operation: n(x, "operation")?,
                    })
                })
                .collect::<Result<Vec<_>, String>>()?;
            let b = try_pack_detection_thresholds(&t).map_err(dbg)?;
            Ok(json!({ "start_block": hex_encode(&b.start), "valid_status": hex_encode(&b.valid) }))
        }
        "vl53l8_cnh_decode" => {
            let c = &i["config"];
            let mut setup = Vl53l8CnhSetup::new();
            setup.init_config(n(c, "start_bin")?, n(c, "num_bins")?, n(c, "sub_sample")?).map_err(dbg)?;
            let m = c["agg_map"].as_array().unwrap();
            if m.len() != 6 {
                return Err("agg_map: start_x, start_y, merge_x, merge_y, cols, rows".into());
            }
            let a = |k: usize| int::<i32>(&m[k], "agg_map");
            setup.create_agg_map(n(c, "resolution")?, a(0)?, a(1)?, a(2)?, a(3)?, a(4)?, a(5)?).map_err(dbg)?;
            let r = depz_sensor_sdk::vl53l8::decode_cnh(&setup.decode_config(), &hex(i, "cnh_raw")).map_err(dbg)?;
            let aggs: Vec<Value> =
                r.aggregates.iter().map(|a| json!({ "hist_raw": a.hist_raw, "hist_scaler": a.hist_scaler })).collect();
            Ok(json!({ "aggregates": aggs }))
        }
        "vl53lx_encode" => match i["kind"].as_str().unwrap() {
            "write_reg" => payload(&lx::try_pack_write_reg(n(i, "addr")?, &hex(i, "data")).map_err(dbg)?),
            "start_stream" => {
                let clear = i["clear"]
                    .as_array()
                    .unwrap()
                    .iter()
                    .map(|s| Ok((int::<u16>(&s[0], "clear addr")?, int::<u8>(&s[1], "clear value")?)))
                    .collect::<Result<Vec<_>, String>>()?;
                payload(&lx::pack_start_stream(n(i, "addr")?, n(i, "len")?, &clear, n(i, "flags")?).map_err(dbg)?)
            }
            k => panic!("vl53lx_encode kind {}", k),
        },
        op => panic!("input_rules op {} has no runner here", op),
    }
}

#[test]
fn input_rules_vectors() {
    let dir = common::vectors_dir();
    let mut files: Vec<_> = std::fs::read_dir(&dir)
        .unwrap()
        .filter_map(|e| e.ok().map(|e| e.file_name().into_string().unwrap()))
        .filter(|f| f.ends_with(".json"))
        .collect();
    files.sort();
    let (mut asserted, mut skipped) = (0, 0);
    for f in files {
        let doc = load(&f);
        let Some(cases) = doc.get("input_rules").and_then(Value::as_array) else {
            continue;
        };
        for c in cases {
            let id = c["id"].as_str().unwrap();
            let op = c["op"].as_str().unwrap();
            if NOT_HERE.iter().any(|(o, _)| *o == op) {
                skipped += 1;
                continue;
            }
            let got = run(op, &c["in"]);
            let expect = &c["expect"];
            if expect.get("error").is_some() {
                assert!(got.is_err(), "{}: expected a rejection, got {:?}", id, got);
            } else {
                let got = got.unwrap_or_else(|e| panic!("{}: rejected: {}", id, e));
                for (k, want) in expect.as_object().unwrap() {
                    assert_eq!(&got[k], want, "{}: {}", id, k);
                }
            }
            seen(c);
            asserted += 1;
        }
    }
    assert!(asserted >= 39, "only {} input_rules cases found", asserted);
    eprintln!("input_rules: {} asserted, {} not runnable here ({:?})", asserted, skipped, NOT_HERE);
}
