//! Differential-testing adapter for the Rust SDK (contracts/16_DIFFTEST.md).
//!
//! A long-lived process speaking JSON Lines on stdin/stdout:
//!     {"id": n, "op": "<op>", "in": {...}}  ->  {"id": n, "out": ...}
//!                                           or  {"id": n, "error": "invalid" | "unsupported"}
//! `invalid` = the SDK rejected the input (an `Err`, or a panic — the detail
//! then starts with `PANIC`, a finding of its own); `unsupported` = see
//! [`UNSUPPORTED`]. Output shapes come from `tests/common/shapes.rs`, the
//! helpers the golden-vector tests assert with.
//!
//! Run (driver: scripts/difftest.py, from packages/depz-sensor-sdk-rust):
//!     cargo run --quiet --example difftest_adapter
//!
//! Integers: the Rust API is typed (`u8`, `u16`, `i64`, …). An input that
//! does not fit the parameter's type cannot be passed by a Rust caller at all
//! — the type system refuses it — so it answers `invalid` (contract 07 §3:
//! an out-of-range argument is rejected, never wrapped). The same holds for
//! enum-typed parameters (`CrcType`).

#[path = "../common/mod.rs"]
mod common;

use std::io::{BufRead, Write};
use std::panic::{self, AssertUnwindSafe};

use common::shapes;
use depz_sensor_sdk::bno055::{self as b055, AccelConfig, AxisRemap, CalibStatus, CalibrationProfile, GyroConfig, MagConfig, Units};
use depz_sensor_sdk::bno086::reports::{try_parse_gyro_rv_cargo, try_parse_input_cargo};
use depz_sensor_sdk::bno086::sh2;
use depz_sensor_sdk::bno086::shtp::{try_pack_shtp_header, try_unpack_shtp_header, ShtpHeader, ShtpLayer};
use depz_sensor_sdk::crc::{crc16_ccitt_false, crc16_modbus, crc32_iso_hdlc, crc8_maxim};
use depz_sensor_sdk::framing::{build_packet, CrcType, PacketParser};
use depz_sensor_sdk::fwdepz;
use depz_sensor_sdk::protocol::{common as proto, identity, sr04};
use depz_sensor_sdk::tmf8829::{self as tmf, CaptureTimer, SensorClock};
use depz_sensor_sdk::usb_ids::{self, PortEntry};
use depz_sensor_sdk::vl53l4 as l4;
use depz_sensor_sdk::vl53l7 as l7;
use depz_sensor_sdk::vl53l8::advanced::{
    try_default_motion_config, try_pack_detection_thresholds, try_xtalk_margin_to_raw, DetectionThreshold,
};
use depz_sensor_sdk::vl53lx as lx;
use depz_sensor_sdk::resolve_sensor_class;
use serde_json::{json, Value};

/// Ops this SDK declares it cannot do, with the reason (never a silent gap).
const UNSUPPORTED: &[(&str, &str)] = &[];

type R = Result<Value, String>;

// ---- input access ------------------------------------------------------------

fn field<'a>(i: &'a Value, k: &str) -> Result<&'a Value, String> {
    i.get(k).ok_or_else(|| format!("missing field {:?}", k))
}

/// An integer that fits `T` (the API's parameter type), else `invalid`.
fn int<T: TryFrom<i128>>(v: &Value, what: &str) -> Result<T, String> {
    let wide: i128 = if let Some(x) = v.as_u64() {
        x as i128
    } else if let Some(x) = v.as_i64() {
        x as i128
    } else {
        return Err(format!("{}: not an integer of 64 bits or less: {}", what, v));
    };
    T::try_from(wide).map_err(|_| format!("{}: {} does not fit the API type {}", what, wide, std::any::type_name::<T>()))
}

fn n<T: TryFrom<i128>>(i: &Value, k: &str) -> Result<T, String> {
    int(field(i, k)?, k)
}

/// A nullable integer (`null` → `None`).
fn opt_n<T: TryFrom<i128>>(i: &Value, k: &str) -> Result<Option<T>, String> {
    let v = field(i, k)?;
    if v.is_null() {
        Ok(None)
    } else {
        int(v, k).map(Some)
    }
}

fn boolean(i: &Value, k: &str) -> Result<bool, String> {
    field(i, k)?.as_bool().ok_or_else(|| format!("{}: not a bool", k))
}

fn string<'a>(i: &'a Value, k: &str) -> Result<&'a str, String> {
    field(i, k)?.as_str().ok_or_else(|| format!("{}: not a string", k))
}

fn opt_string<'a>(i: &'a Value, k: &str) -> Result<Option<&'a str>, String> {
    let v = field(i, k)?;
    if v.is_null() {
        Ok(None)
    } else {
        v.as_str().map(Some).ok_or_else(|| format!("{}: not a string", k))
    }
}

fn arr<'a>(i: &'a Value, k: &str) -> Result<&'a Vec<Value>, String> {
    field(i, k)?.as_array().ok_or_else(|| format!("{}: not a list", k))
}

fn hex_of(v: &Value) -> Result<Vec<u8>, String> {
    let s = v.as_str().ok_or("not a hex string")?;
    if s.len() % 2 != 0 || !s.bytes().all(|c| c.is_ascii_digit() || (b'a'..=b'f').contains(&c)) {
        return Err("not a hex string".into());
    }
    Ok((0..s.len()).step_by(2).map(|k| u8::from_str_radix(&s[k..k + 2], 16).unwrap()).collect())
}

fn hex(i: &Value, k: &str) -> Result<Vec<u8>, String> {
    hex_of(field(i, k)?)
}

fn hexes(i: &Value, k: &str) -> Result<Vec<Vec<u8>>, String> {
    arr(i, k)?.iter().map(hex_of).collect()
}

fn dbg<E: std::fmt::Debug>(e: E) -> String {
    format!("{:?}", e)
}

fn payload(p: &[u8]) -> R {
    Ok(json!({ "payload": common::hex_encode(p) }))
}

fn crc_type(v: u8) -> Result<CrcType, String> {
    // CrcType is a typed enum: only the four listed values can be passed.
    CrcType::from_value(v).ok_or_else(|| format!("crc_type {} is not a CrcType", v))
}

// ---- ops -----------------------------------------------------------------------

fn op_crc(i: &Value) -> R {
    let raw = hex(i, "input")?;
    Ok(json!({
        "crc8_maxim": crc8_maxim(&raw),
        "crc16_modbus": crc16_modbus(&raw),
        "crc32_iso_hdlc": crc32_iso_hdlc(&raw),
        "crc16_ccitt_false": crc16_ccitt_false(&raw),
    }))
}

fn op_framing_encode(i: &Value) -> R {
    let frame = build_packet(n(i, "cmd")?, &hex(i, "payload")?, n(i, "seq")?, crc_type(n(i, "crc_type")?)?)
        .map_err(dbg)?;
    Ok(json!({ "frame": common::hex_encode(&frame) }))
}

fn op_framing_decode(i: &Value) -> R {
    let chunks = if i.get("chunks").is_some() { hexes(i, "chunks")? } else { vec![hex(i, "stream")?] };
    Ok(shapes::framing_collect(&mut PacketParser::new(), &chunks))
}

fn op_identity(i: &Value) -> R {
    let id = identity::parse_software_name(&proto::strip_device_string(&hex(i, "raw")?));
    Ok(json!({
        "mode": id.mode.as_str(),
        "sensor_type": id.sensor_type.map(|t| t.as_str()),
        "software_name": id.software_name,
        "version": id.version,
    }))
}

fn op_usb_identity(i: &Value) -> R {
    let (vid, pid) = (Some(n::<u16>(i, "vid")?), Some(n::<u16>(i, "pid")?));
    Ok(json!({ "known": usb_ids::is_known_depz_usb(vid, pid), "model": usb_ids::usb_model_hint(vid, pid) }))
}

fn op_usb_serial_ordering(i: &Value) -> R {
    let ports = arr(i, "ports")?
        .iter()
        .map(|p| Ok(PortEntry { port: string(p, "port")?.to_string(), serial: opt_string(p, "serial")?.map(str::to_string) }))
        .collect::<Result<Vec<_>, String>>()?;
    let order: Vec<String> = usb_ids::order_ports(ports).into_iter().map(|p| p.port).collect();
    Ok(json!({ "order": order }))
}

fn op_probe_resolve(i: &Value) -> R {
    let class = resolve_sensor_class(
        string(i, "software_name")?,
        string(i, "device_name")?,
        opt_n(i, "usb_vid")?,
        opt_n(i, "usb_pid")?,
    );
    Ok(json!({ "class": class }))
}

fn op_common_encode(i: &Value) -> R {
    match string(i, "kind")? {
        "sync_time_request" => payload(&proto::pack_sync_time(n(i, "pc_timestamp_us")?)),
        "set_payload_crc_type" => {
            // No codec takes a CrcType here: check the enum like the device layer's type does.
            let t = crc_type(n(i, "crc_type")?)?;
            payload(&proto::pack_set_payload_crc_type(t as u8))
        }
        "sync_pin_config" => payload(
            &proto::SyncPinConfig { pin: n(i, "pin")?, mode: n(i, "mode")?, polarity: n(i, "polarity")? }
                .try_pack()
                .map_err(dbg)?,
        ),
        k => Err(format!("unknown kind {}", k)),
    }
}

fn op_common_decode(i: &Value) -> R {
    let p = hex(i, "payload")?;
    match n::<u64>(i, "report")? {
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
        r => Err(format!("report {:#x}", r)),
    }
}

fn op_sync_time_math(i: &Value) -> R {
    let (off, rtt) = proto::sync_time_offset_rtt(n(i, "t1")?, n(i, "t2")?, n(i, "t3")?, n(i, "t4")?);
    Ok(json!({ "offset_us": off, "rtt_us": rtt }))
}

fn op_sr04_encode(i: &Value) -> R {
    match string(i, "kind")? {
        "set_sample_period" => payload(&sr04::pack_sample_period(n(i, "period_us")?)),
        "set_echo_decay" => payload(&sr04::pack_echo_decay(n(i, "decay_us")?)),
        k => Err(format!("unknown kind {}", k)),
    }
}

fn op_sr04_decode(i: &Value) -> R {
    let p = hex(i, "payload")?;
    match n::<u64>(i, "report")? {
        0x91 => {
            let d = sr04::Sr04Data::unpack(&p).map_err(dbg)?;
            Ok(json!({ "source_cmd": d.source_cmd, "timestamp_us": d.timestamp_us, "echo_time_us": d.echo_time_us }))
        }
        0x92 => Ok(json!({ "period_us": sr04::unpack_sample_period(&p).map_err(dbg)? })),
        0x93 => Ok(json!({ "decay_us": sr04::unpack_echo_decay(&p).map_err(dbg)? })),
        r => Err(format!("report {:#x}", r)),
    }
}

fn op_fwdepz_parse(i: &Value) -> R {
    let h = fwdepz::parse(&hex(i, "file")?).map_err(dbg)?;
    Ok(json!({
        "load_addr": h.load_addr, "fw_size": h.fw_size, "fw_crc32": h.fw_crc32,
        "cur_sec": h.cur_sec, "tot_sec": h.tot_sec, "payload_crc_ok": h.payload_crc_ok,
    }))
}

// ── BNO055 ──

fn op_bno055_encode(i: &Value) -> R {
    match string(i, "kind")? {
        "read_reg" => payload(&b055::pack_read_reg(n(i, "addr")?, n(i, "len")?)),
        "write_reg" => payload(&b055::try_pack_write_reg(n(i, "addr")?, &hex(i, "data")?).map_err(dbg)?),
        "start_stream" => payload(&b055::pack_start_stream(n(i, "trigger")?, n(i, "addr")?, n(i, "len")?, n(i, "period_ms")?)),
        "reset" => payload(&b055::pack_reset()),
        "stop_stream" => payload(&b055::pack_stop_stream()),
        "get_info" => payload(&b055::pack_get_info()),
        k => Err(format!("unknown kind {}", k)),
    }
}

fn op_bno055_decode(i: &Value) -> R {
    shapes::bno055_decode(n(i, "report")?, &hex(i, "payload")?)
}

fn op_bno055_units(i: &Value) -> R {
    let u = Units::unpack(n(i, "unit_sel")?);
    Ok(json!({
        "accel_mg": u.accel_mg, "gyro_rps": u.gyro_rps, "euler_rad": u.euler_rad,
        "temp_f": u.temp_f, "android": u.android, "repack": u.pack(),
    }))
}

fn op_bno055_calib_stat(i: &Value) -> R {
    let s = CalibStatus::unpack(n(i, "value")?);
    Ok(json!({
        "system": s.system, "gyro": s.gyro, "accel": s.accel, "mag": s.mag,
        "fully_calibrated": s.fully_calibrated(), "repack": s.pack(),
    }))
}

fn op_bno055_calibration_profile(i: &Value) -> R {
    let p = CalibrationProfile::unpack(&hex(i, "bytes")?).map_err(dbg)?;
    Ok(json!({
        "accel_offset": p.accel_offset.to_vec(), "mag_offset": p.mag_offset.to_vec(),
        "gyro_offset": p.gyro_offset.to_vec(), "accel_radius": p.accel_radius,
        "mag_radius": p.mag_radius, "repack": common::hex_encode(&p.pack()),
    }))
}

fn axis_remap_out(a: &AxisRemap) -> R {
    let (config, sign) = a.pack().map_err(dbg)?;
    Ok(json!({
        "x": a.x, "y": a.y, "z": a.z,
        "x_negative": a.x_negative, "y_negative": a.y_negative, "z_negative": a.z_negative,
        "repack": [config, sign],
    }))
}

fn op_bno055_axis_remap(i: &Value) -> R {
    axis_remap_out(&AxisRemap::unpack(n(i, "config")?, n(i, "sign")?))
}

fn op_bno055_axis_remap_pack(i: &Value) -> R {
    let a = AxisRemap {
        x: n(i, "x")?,
        y: n(i, "y")?,
        z: n(i, "z")?,
        x_negative: boolean(i, "x_negative")?,
        y_negative: boolean(i, "y_negative")?,
        z_negative: boolean(i, "z_negative")?,
    };
    let (config, sign) = a.pack().map_err(dbg)?;
    Ok(json!({ "repack": [config, sign] }))
}

fn op_bno055_sensor_config(i: &Value) -> R {
    match string(i, "sensor")? {
        "accel" => {
            let a = AccelConfig::unpack(n(i, "value")?);
            Ok(json!({ "range": a.range, "bandwidth": a.bandwidth, "power": a.power, "repack": a.pack() }))
        }
        "gyro" => {
            let g = GyroConfig::unpack(&hex(i, "bytes")?).map_err(dbg)?;
            Ok(json!({
                "range": g.range, "bandwidth": g.bandwidth, "power": g.power,
                "repack": common::hex_encode(&g.pack()),
            }))
        }
        "mag" => {
            let m = MagConfig::unpack(n(i, "value")?);
            Ok(json!({ "rate": m.rate, "mode": m.mode, "power": m.power, "repack": m.pack() }))
        }
        s => Err(format!("unknown sensor {}", s)),
    }
}

fn op_bno055_block(i: &Value) -> R {
    Ok(shapes::bno055_block(n(i, "addr")?, &hex(i, "data")?))
}

// ── BNO086 ──

fn op_bno086_shtp_header_pack(i: &Value) -> R {
    let hdr = ShtpHeader {
        length: n(i, "length")?,
        channel: n(i, "channel")?,
        seq: n(i, "seq")?,
        continuation: boolean(i, "continuation")?,
    };
    Ok(json!({ "bytes": common::hex_encode(&try_pack_shtp_header(&hdr).map_err(dbg)?) }))
}

fn op_bno086_shtp_header_unpack(i: &Value) -> R {
    let h = try_unpack_shtp_header(&hex(i, "bytes")?).map_err(dbg)?;
    Ok(json!({ "length": h.length, "channel": h.channel, "seq": h.seq, "continuation": h.continuation }))
}

fn op_bno086_shtp_tx_seq(i: &Value) -> R {
    let mut layer = ShtpLayer::new();
    let mut frames = Vec::new();
    for p in arr(i, "packets")? {
        frames.push(common::hex_encode(&layer.try_next_frame(n(p, "channel")?, &hex(p, "payload")?).map_err(dbg)?));
    }
    Ok(json!({ "frames": frames }))
}

fn op_bno086_shtp_reassembly(i: &Value) -> R {
    let frames = hexes(i, "frames")?;
    let captures = arr(i, "captures_us")?;
    if frames.len() != captures.len() {
        return Err("frames / captures_us length".into());
    }
    let mut layer = ShtpLayer::new();
    let mut cargos = Vec::new();
    for (f, t) in frames.iter().zip(captures) {
        if let Some(c) = layer.feed_at(f, int(t, "captures_us")?) {
            cargos.push(shapes::bno086_cargo(&c));
        }
    }
    Ok(json!({ "cargos": cargos, "discarded": layer.discarded }))
}

fn op_bno086_control_encode(i: &Value) -> R {
    match string(i, "kind")? {
        "set_feature" => payload(&sh2::build_set_feature(
            n(i, "sensor_id")?,
            n(i, "interval_us")?,
            n(i, "batch_us")?,
            n(i, "sensitivity")?,
            n(i, "flags")?,
            n(i, "cfg_word")?,
        )),
        "get_feature_request" => payload(&sh2::build_get_feature_request(n(i, "sensor_id")?)),
        "product_id_request" => payload(&sh2::build_product_id_request()),
        "command_request" => {
            payload(&sh2::build_command_request(n(i, "seq")?, n(i, "command")?, &hex(i, "params")?).map_err(dbg)?)
        }
        "frs_read_request" => {
            payload(&sh2::build_frs_read_request(n(i, "frs_type")?, n(i, "offset_words")?, n(i, "block_words")?))
        }
        "frs_write_request" => payload(&sh2::build_frs_write_request(n(i, "frs_type")?, n(i, "length_words")?)),
        "frs_write_data" => {
            let words = arr(i, "words")?.iter().map(|w| int::<u32>(w, "words")).collect::<Result<Vec<_>, _>>()?;
            payload(&sh2::build_frs_write_data(n(i, "offset_words")?, &words).map_err(dbg)?)
        }
        k => Err(format!("unknown kind {}", k)),
    }
}

fn op_bno086_input_cargo(i: &Value) -> R {
    let reps = try_parse_input_cargo(&hex(i, "cargo")?, n(i, "capture_timestamp_us")?).map_err(dbg)?;
    Ok(json!({ "reports": reps.iter().map(shapes::bno086_typed).collect::<Vec<_>>() }))
}

fn op_bno086_record(i: &Value) -> R {
    let reps = try_parse_input_cargo(&hex(i, "cargo")?, n(i, "capture_timestamp_us")?).map_err(dbg)?;
    let recs = reps
        .iter()
        .map(|r| serde_json::from_str(&depz_sensor_sdk::dataset::records::bno086(r).to_json()).map_err(dbg))
        .collect::<Result<Vec<Value>, String>>()?;
    Ok(json!({ "records": recs }))
}

fn op_bno086_gyro_rv(i: &Value) -> R {
    Ok(match try_parse_gyro_rv_cargo(&hex(i, "cargo")?, n(i, "capture_timestamp_us")?).map_err(dbg)? {
        Some(r) => shapes::bno086_typed(&r),
        None => Value::Null,
    })
}

// ── TMF8829 ──

fn op_tmf8829_encode(i: &Value) -> R {
    match n::<u64>(i, "cmd")? {
        0x32 => payload(&tmf::pack_read_reg(n(i, "addr")?, n(i, "len")?).map_err(dbg)?),
        0x33 => payload(&tmf::pack_write_reg(n(i, "addr")?, &hex(i, "data")?).map_err(dbg)?),
        0x35 => payload(&tmf::pack_start_stream(n(i, "int_mask")?, n(i, "flags")?).map_err(dbg)?),
        _ => payload(&[]), // STOP_STREAM / GET_INFO: no payload
    }
}

fn op_tmf8829_decode(i: &Value) -> R {
    shapes::tmf8829_decode(n(i, "report")?, &hex(i, "payload")?)
}

fn op_tmf8829_reassembly(i: &Value) -> R {
    shapes::tmf8829_reassembly(&hexes(i, "chunks")?)
}

fn op_tmf8829_frame(i: &Value) -> R {
    shapes::tmf8829_frame(&hex(i, "raw")?)
}

fn op_tmf8829_measurements(i: &Value) -> R {
    let frames = arr(i, "frames")?
        .iter()
        .map(|f| Ok((hex(f, "raw")?, n(f, "timestamp_us")?, n(f, "edge_us")?)))
        .collect::<Result<Vec<_>, String>>()?;
    Ok(json!({ "measurements": shapes::tmf8829_measurements(&frames) }))
}

fn op_tmf8829_config_get(i: &Value) -> R {
    Ok(json!({ "value": tmf::cfg_get(&hex(i, "page")?, string(i, "field")?).map_err(dbg)? }))
}

fn op_tmf8829_config_set(i: &Value) -> R {
    let mut pg = hex(i, "page")?;
    tmf::cfg_set(&mut pg, string(i, "field")?, n(i, "value")?).map_err(dbg)?;
    Ok(json!({ "page": common::hex_encode(&pg) }))
}

fn op_tmf8829_frame_time(i: &Value) -> R {
    let mut clock = SensorClock::new();
    let mut timer = CaptureTimer::new();
    let mut times = Vec::new();
    for r in arr(i, "rows")? {
        let id: u8 = n(r, "id")?;
        let kind = id & 0xF0;
        if kind == 0x30 {
            continue;
        }
        let (ts, tick): (u64, u32) = (n(r, "ts")?, n(r, "systick")?);
        let flags = clock.add(ts, tick);
        if kind == 0x20 {
            timer.histograms();
            continue;
        }
        let t = timer.results(&clock, id & 0x0F, tick, n(r, "t0")?, n(r, "t1")?, ts, n(r, "edge_us")?, flags);
        times.push(json!({
            "timestamp_us": t.timestamp_us, "t1_len_us": t.t1_len_us, "flags": t.flags,
            "ratio_q16": clock.ratio_q16,
        }));
    }
    Ok(json!({ "times": times }))
}

// ── VL53L4CD ──

fn op_vl53l4_encode(i: &Value) -> R {
    match string(i, "kind")? {
        "read_reg" => payload(&l4::pack_read_reg(n(i, "addr")?, n(i, "len")?)),
        "write_reg" => payload(&l4::try_pack_write_reg(n(i, "addr")?, &hex(i, "data")?).map_err(dbg)?),
        "xshut" => payload(&l4::pack_xshut(n(i, "action")?)),
        "start_stream" => payload(&l4::pack_start_stream(n(i, "addr")?, n(i, "len")?, n(i, "flags")?)),
        "set_i2c_speed" => payload(&l4::pack_set_i2c_speed(n(i, "khz")?)),
        k => Err(format!("unknown kind {}", k)),
    }
}

fn op_vl53l4_decode(i: &Value) -> R {
    shapes::vl53l4_decode(n(i, "report")?, &hex(i, "payload")?)
}

fn op_vl53l4_result_block(i: &Value) -> R {
    shapes::vl53l4_result(&hex(i, "raw")?)
}

fn op_vl53l4_timing_encode(i: &Value) -> R {
    let (a, b, raw) = l4::range_timing_registers(
        n(i, "timing_budget_ms")?,
        n(i, "inter_measurement_ms")?,
        n(i, "osc_frequency")?,
        n(i, "clock_pll")?,
    )
    .map_err(dbg)?;
    Ok(json!({ "range_config_a": a, "range_config_b": b, "intermeasurement_raw": raw }))
}

fn op_vl53l4_timing_decode(i: &Value) -> R {
    let (budget, inter) = l4::decode_range_timing(
        n(i, "intermeasurement_raw")?,
        n(i, "clock_pll")?,
        n(i, "osc_frequency")?,
        n(i, "range_config_a")?,
    )
    .map_err(dbg)?;
    Ok(json!({ "timing_budget_ms": budget, "inter_measurement_ms": inter }))
}

fn op_vl53l4_tuning_encode(i: &Value) -> R {
    let raw = match string(i, "kind")? {
        "offset" => l4::try_offset_raw(n(i, "value")?).map_err(dbg)?,
        "xtalk" => l4::try_xtalk_raw(n(i, "value")?).map_err(dbg)?,
        "signal_threshold" => l4::signal_threshold_raw(n(i, "value")?),
        "sigma_threshold" => l4::sigma_threshold_raw(n(i, "value")?).map_err(dbg)?,
        k => return Err(format!("unknown kind {}", k)),
    };
    Ok(json!({ "raw": raw }))
}

fn op_vl53l4_tuning_decode(i: &Value) -> R {
    let raw: u16 = n(i, "raw")?;
    let value: i64 = match string(i, "kind")? {
        "offset" => l4::decode_offset(raw) as i64,
        "xtalk" => l4::decode_xtalk(raw) as i64,
        "signal_threshold" => l4::decode_signal_threshold(raw) as i64,
        "sigma_threshold" => l4::decode_sigma_threshold(raw) as i64,
        k => return Err(format!("unknown kind {}", k)),
    };
    Ok(json!({ "value": value }))
}

// ── VL53L5CX / L7 ──

fn op_vl53l7_encode(i: &Value) -> R {
    match string(i, "kind")? {
        "read_reg" => payload(&l7::pack_read_reg(n(i, "addr")?, n(i, "len")?).map_err(dbg)?),
        "write_reg" => payload(&l7::pack_write_reg(n(i, "addr")?, &hex(i, "data")?).map_err(dbg)?),
        "pin_ctrl" => payload(&l7::pack_pin_ctrl(n(i, "action")?)),
        "set_i2c_speed" => payload(&l7::pack_set_i2c_speed(n(i, "khz")?)),
        k => Err(format!("unknown kind {}", k)),
    }
}

fn op_vl53l7_decode(i: &Value) -> R {
    match n::<u64>(i, "report")? {
        0x92 => shapes::vl53l7_info(&hex(i, "payload")?),
        r => Err(format!("report {:#x}", r)),
    }
}

fn op_vl53l7_model(i: &Value) -> R {
    Ok(json!({ "class": l7::resolve_model(opt_string(i, "usb_model")?, string(i, "device_name")?).as_str() }))
}

// ── VL53L 1D family ──

fn op_vl53lx_encode(i: &Value) -> R {
    match string(i, "kind")? {
        "set_addr_width" => payload(&lx::pack_set_addr_width(n(i, "width")?).map_err(dbg)?),
        "read_reg" => payload(&lx::pack_read_reg(n(i, "addr")?, n(i, "len")?)),
        "write_reg" => payload(&lx::try_pack_write_reg(n(i, "addr")?, &hex(i, "data")?).map_err(dbg)?),
        "xshut" => payload(&lx::pack_xshut(n(i, "action")?)),
        "set_i2c_speed" => payload(&lx::pack_set_i2c_speed(n(i, "khz")?)),
        "start_stream" => {
            let clear = arr(i, "clear")?
                .iter()
                .map(|s| match s.as_array().map(Vec::as_slice) {
                    Some([a, v]) => Ok((int::<u16>(a, "clear addr")?, int::<u8>(v, "clear value")?)),
                    _ => Err("clear: not an [addr, value] pair".to_string()),
                })
                .collect::<Result<Vec<_>, String>>()?;
            payload(&lx::pack_start_stream(n(i, "addr")?, n(i, "len")?, &clear, n(i, "flags")?).map_err(dbg)?)
        }
        k => Err(format!("unknown kind {}", k)),
    }
}

fn op_vl53lx_decode(i: &Value) -> R {
    match n::<u64>(i, "report")? {
        0x92 => shapes::vl53lx_info(&hex(i, "payload")?),
        r => Err(format!("report {:#x}", r)),
    }
}

fn op_vl53lx_model(i: &Value) -> R {
    let device_name = string(i, "device_name")?;
    Ok(json!({
        "class": lx::resolve_class(opt_string(i, "usb_model")?, device_name).as_str(),
        "product": lx::product_from_board_name(device_name),
    }))
}

fn op_vl53lx_die_block(i: &Value) -> R {
    shapes::vl53lx_die_block(&hex(i, "raw")?, string(i, "variant")?)
}

fn op_vl53lx_l0x_raw(i: &Value) -> R {
    shapes::vl53lx_l0x_raw(&hex(i, "raw")?)
}

fn op_vl53lx_histogram_raw(i: &Value) -> R {
    shapes::vl53lx_histogram_raw(&hex(i, "raw")?)
}

// ── VL53L8 advanced ──
// The crate has no ULD driver (no live DCI writes): these run the pure DCI
// encoders the driver methods would write; their `try_` forms carry the
// driver methods' parameter checks (resolution, xtalk margin ≤ 10000).

fn op_vl53l8_motion_config(i: &Value) -> R {
    Ok(json!({ "pack": common::hex_encode(&try_default_motion_config(n(i, "resolution")?).map_err(dbg)?.pack()) }))
}

fn op_vl53l8_detection_thresholds(i: &Value) -> R {
    let t = arr(i, "thresholds")?
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
    Ok(json!({ "start_block": common::hex_encode(&b.start), "valid_status": common::hex_encode(&b.valid) }))
}

fn op_vl53l8_cnh_decode(i: &Value) -> R {
    let c = field(i, "config")?;
    let mut setup = depz_sensor_sdk::vl53l8::driver::Vl53l8CnhSetup::new();
    setup.init_config(n(c, "start_bin")?, n(c, "num_bins")?, n(c, "sub_sample")?).map_err(dbg)?;
    let m = field(c, "agg_map")?.as_array().ok_or("agg_map: not an array")?;
    if m.len() != 6 {
        return Err("agg_map: start_x, start_y, merge_x, merge_y, cols, rows".into());
    }
    let a = |k: usize| int::<i32>(&m[k], "agg_map");
    setup.create_agg_map(n(c, "resolution")?, a(0)?, a(1)?, a(2)?, a(3)?, a(4)?, a(5)?).map_err(dbg)?;
    let r = depz_sensor_sdk::vl53l8::decode_cnh(&setup.decode_config(), &hex(i, "cnh_raw")?).map_err(dbg)?;
    let aggs: Vec<Value> = r.aggregates.iter().map(|a| json!({ "hist_raw": a.hist_raw, "hist_scaler": a.hist_scaler })).collect();
    Ok(json!({ "aggregates": aggs }))
}

fn op_vl53l8_xtalk_margin(i: &Value) -> R {
    let kcps = field(i, "kcps")?.as_f64().ok_or("kcps: not a number")?;
    Ok(json!({ "raw": try_xtalk_margin_to_raw(kcps).map_err(dbg)? }))
}

type Op = fn(&Value) -> R;

const OPS: &[(&str, Op)] = &[
    ("crc", op_crc),
    ("framing_encode", op_framing_encode),
    ("framing_decode", op_framing_decode),
    ("identity", op_identity),
    ("usb_identity", op_usb_identity),
    ("usb_serial_ordering", op_usb_serial_ordering),
    ("probe_resolve", op_probe_resolve),
    ("common_encode", op_common_encode),
    ("common_decode", op_common_decode),
    ("sync_time_math", op_sync_time_math),
    ("sr04_encode", op_sr04_encode),
    ("sr04_decode", op_sr04_decode),
    ("fwdepz_parse", op_fwdepz_parse),
    ("bno055_encode", op_bno055_encode),
    ("bno055_decode", op_bno055_decode),
    ("bno055_units", op_bno055_units),
    ("bno055_calib_stat", op_bno055_calib_stat),
    ("bno055_calibration_profile", op_bno055_calibration_profile),
    ("bno055_axis_remap", op_bno055_axis_remap),
    ("bno055_axis_remap_pack", op_bno055_axis_remap_pack),
    ("bno055_sensor_config", op_bno055_sensor_config),
    ("bno055_block", op_bno055_block),
    ("bno086_shtp_header_pack", op_bno086_shtp_header_pack),
    ("bno086_shtp_header_unpack", op_bno086_shtp_header_unpack),
    ("bno086_shtp_tx_seq", op_bno086_shtp_tx_seq),
    ("bno086_shtp_reassembly", op_bno086_shtp_reassembly),
    ("bno086_control_encode", op_bno086_control_encode),
    ("bno086_input_cargo", op_bno086_input_cargo),
    ("bno086_record", op_bno086_record),
    ("bno086_gyro_rv", op_bno086_gyro_rv),
    ("tmf8829_encode", op_tmf8829_encode),
    ("tmf8829_decode", op_tmf8829_decode),
    ("tmf8829_reassembly", op_tmf8829_reassembly),
    ("tmf8829_frame", op_tmf8829_frame),
    ("tmf8829_measurements", op_tmf8829_measurements),
    ("tmf8829_config_get", op_tmf8829_config_get),
    ("tmf8829_config_set", op_tmf8829_config_set),
    ("tmf8829_frame_time", op_tmf8829_frame_time),
    ("vl53l4_encode", op_vl53l4_encode),
    ("vl53l4_decode", op_vl53l4_decode),
    ("vl53l4_result_block", op_vl53l4_result_block),
    ("vl53l4_timing_encode", op_vl53l4_timing_encode),
    ("vl53l4_timing_decode", op_vl53l4_timing_decode),
    ("vl53l4_tuning_encode", op_vl53l4_tuning_encode),
    ("vl53l4_tuning_decode", op_vl53l4_tuning_decode),
    ("vl53l7_encode", op_vl53l7_encode),
    ("vl53l7_decode", op_vl53l7_decode),
    ("vl53l7_model", op_vl53l7_model),
    ("vl53lx_encode", op_vl53lx_encode),
    ("vl53lx_decode", op_vl53lx_decode),
    ("vl53lx_model", op_vl53lx_model),
    ("vl53lx_die_block", op_vl53lx_die_block),
    ("vl53lx_l0x_raw", op_vl53lx_l0x_raw),
    ("vl53lx_histogram_raw", op_vl53lx_histogram_raw),
    ("vl53l8_motion_config", op_vl53l8_motion_config),
    ("vl53l8_detection_thresholds", op_vl53l8_detection_thresholds),
    ("vl53l8_xtalk_margin", op_vl53l8_xtalk_margin),
    ("vl53l8_cnh_decode", op_vl53l8_cnh_decode),
];

// ---- loop ------------------------------------------------------------------------

fn panic_text(p: &(dyn std::any::Any + Send)) -> String {
    p.downcast_ref::<&str>()
        .map(|s| s.to_string())
        .or_else(|| p.downcast_ref::<String>().cloned())
        .unwrap_or_else(|| "panic".into())
}

fn handle(req: &Value) -> Value {
    let id = req.get("id").cloned().unwrap_or(Value::Null);
    let op = req.get("op").and_then(Value::as_str).unwrap_or("");
    if let Some((_, why)) = UNSUPPORTED.iter().find(|(o, _)| *o == op) {
        return json!({ "id": id, "error": "unsupported", "detail": why });
    }
    let Some((_, f)) = OPS.iter().find(|(o, _)| *o == op) else {
        return json!({ "id": id, "error": "unsupported", "detail": "unknown op" });
    };
    let input = req.get("in").cloned().unwrap_or(Value::Null);
    match panic::catch_unwind(AssertUnwindSafe(|| f(&input))) {
        Ok(Ok(out)) => json!({ "id": id, "out": out }),
        Ok(Err(e)) => json!({ "id": id, "error": "invalid", "detail": e.chars().take(300).collect::<String>() }),
        // A panic is not a crash of the process (one bad input must not stop
        // the run) but it is a finding: an SDK should return an error.
        Err(p) => json!({ "id": id, "error": "invalid", "detail": format!("PANIC: {}", panic_text(&*p)) }),
    }
}

fn main() {
    // Panics are answered as `invalid` with a PANIC detail; log them to stderr
    // (the driver keeps it in adapter-rust.stderr.log).
    panic::set_hook(Box::new(|info| eprintln!("PANIC {}", info)));
    let stdin = std::io::stdin();
    let mut out = std::io::stdout().lock();
    for line in stdin.lock().lines() {
        let Ok(line) = line else { break };
        if line.trim().is_empty() {
            continue;
        }
        let resp = match serde_json::from_str::<Value>(&line) {
            Ok(req) => handle(&req),
            Err(e) => json!({ "id": null, "error": "invalid", "detail": format!("bad request line: {}", e) }),
        };
        let _ = writeln!(out, "{}", resp);
        let _ = out.flush();
    }
}
