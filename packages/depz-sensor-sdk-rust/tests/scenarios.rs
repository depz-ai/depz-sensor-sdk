//! Contract 15 device scenarios: the Rust runner.
//!
//! Each `contracts/scenarios/<id>.json` names a committed real-hardware capture and the API calls that
//! produced it. The runner opens the capture as its `probe` field says — "canonical":
//! `open_on_link` (the contract 02 §4.4 probe, then the class in place); "legacy": the requests the
//! capture holds, `resolve_sensor_class` and `attach` — over a causal strict-tx replay, so every byte
//! the SDK writes must equal the recorded tx stream, executes the steps (§2) through a per-class
//! adapter (Rust has no reflection; the adapters are the alias table), canonicalises each result (§3)
//! and matches it against `expect`.
mod common;

use std::collections::HashMap;
use std::path::PathBuf;
use std::sync::Arc;
use std::time::{Duration, Instant};

use depz_sensor_sdk::devices::{
    attach, open_on_link, AnyDevice, DepzDevice, DepzError, ReplayLink, Subscription, Vl53lxDevice,
    Vl53lxMeasurement,
};
use depz_sensor_sdk::discovery::resolve_sensor_class;
use depz_sensor_sdk::protocol::identity::{parse_software_name, Identity, SensorType};
use depz_sensor_sdk::usb_ids;
use depz_sensor_sdk::vl53lx::driver::ExtraValue;
use serde_json::{json, Value};

const REQUEST_TIMEOUT: Duration = Duration::from_secs(2); // as every runner
const TAKE_TIMEOUT: Duration = Duration::from_secs(5); // per streamed item

/// The classes this SDK runs scenarios for so far.
const CLASSES: &[&str] = &[
    "sr04", "vl53l4cd", "vl53l8cx", "vl53l8ch", "vl53l5cx", "vl53l7cx", "vl53l7ch", "bno055",
    "bno086", "tmf8829", "vl53lx", "vl53l0x", "vl53l1cx", "vl53l1cb", "vl53l3cx", "vl53l4cx",
];

fn scenarios_dir() -> PathBuf {
    common::vectors_dir().join("..").join("scenarios")
}

// ── canonical values (§3) ───────────────────────────────────────────────────

fn int(v: impl Into<i64>) -> Value {
    json!(v.into())
}

fn dbl(v: f64) -> Value {
    json!(v)
}

fn identity(id: &Identity) -> Value {
    json!({
        "mode": id.mode.as_str(),
        "sensor_type": id.sensor_type.map(|t| t.as_str()),
        "software_name": id.software_name,
        "version": id.version,
    })
}

// ── per-class adapters ──────────────────────────────────────────────────────

type CallResult = Result<Value, DepzError>;

/// A subscription whose items come out canonical.
trait Taker {
    fn take(&self, timeout: Duration) -> CallResult;
    fn dropped_count(&self) -> u64;
}

struct Typed<T> {
    sub: Subscription<T>,
    canon: fn(&T) -> Value,
}

impl<T> Taker for Typed<T> {
    fn take(&self, timeout: Duration) -> CallResult {
        self.sub.take(timeout).map(|t| (self.canon)(&t))
    }

    fn dropped_count(&self) -> u64 {
        self.sub.dropped_count()
    }
}

fn arg_i64(args: &[Value], i: usize) -> Result<i64, DepzError> {
    args.get(i)
        .and_then(Value::as_i64)
        .ok_or_else(|| DepzError::InvalidArgument(format!("argument {i}: not an integer")))
}

fn unknown(what: &str, name: &str, class: &str) -> DepzError {
    panic!("{class} has no {what} '{name}' in the Rust adapter table")
}

/// The calls every class has (the base device).
fn call_common(dev: &DepzDevice, name: &str, _args: &[Value]) -> Option<CallResult> {
    Some(match name {
        "get_software_name" => dev.get_software_name().map(Value::String),
        "get_device_name" => dev.get_device_name().map(Value::String),
        "get_serial_number" => dev.get_serial_number().map(Value::String),
        "read_mcu_temperature" => dev.read_mcu_temperature().map(dbl),
        _ => return None,
    })
}

fn sr04_measurement(m: &depz_sensor_sdk::devices::Sr04Measurement) -> Value {
    json!({ "timestamp_us": m.timestamp_us, "echo_time_us": m.echo_time_us, "source": m.source })
}

fn vl53l4_measurement(m: &depz_sensor_sdk::devices::Vl53l4Measurement) -> Value {
    json!({
        "timestamp_us": m.timestamp_us,
        "range_status": m.range_status,
        "distance_mm": m.distance_mm,
        "sigma_mm": m.sigma_mm,
        "signal_rate_kcps": m.signal_rate_kcps,
        "ambient_rate_kcps": m.ambient_rate_kcps,
        "signal_per_spad_kcps": m.signal_per_spad_kcps,
        "ambient_per_spad_kcps": m.ambient_per_spad_kcps,
        "number_of_spad": m.number_of_spad,
        "stream_count": m.stream_count,
    })
}

fn opt3(v: Option<[f64; 3]>) -> Value {
    v.map_or(Value::Null, |a| json!(a))
}

fn bno055_sample(s: &depz_sensor_sdk::devices::Bno055Sample) -> Value {
    let u = s.units;
    json!({
        "timestamp_us": s.timestamp_us,
        "addr": s.addr,
        "raw": common::hex_encode(&s.raw),
        "units": { "accel_mg": u.accel_mg, "gyro_rps": u.gyro_rps, "euler_rad": u.euler_rad, "temp_f": u.temp_f, "android": u.android },
        "accel": opt3(s.accel),
        "mag": opt3(s.mag),
        "gyro": opt3(s.gyro),
        "euler": opt3(s.euler),
        "quaternion": s.quaternion.map_or(Value::Null, |q| json!(q)),
        "linear_accel": opt3(s.linear_accel),
        "gravity": opt3(s.gravity),
        "temperature": s.temperature,
        "calibration": s.calibration.map(|c| json!({ "system": c.system, "gyro": c.gyro, "accel": c.accel, "mag": c.mag })),
    })
}

fn units_arg(v: &Value) -> Result<depz_sensor_sdk::bno055::regs::Units, DepzError> {
    let b = |k: &str| {
        v.get(k)
            .and_then(Value::as_bool)
            .ok_or_else(|| DepzError::InvalidArgument(format!("units.{k}")))
    };
    Ok(depz_sensor_sdk::bno055::regs::Units {
        accel_mg: b("accel_mg")?,
        gyro_rps: b("gyro_rps")?,
        euler_rad: b("euler_rad")?,
        temp_f: b("temp_f")?,
        android: b("android")?,
    })
}

fn block_arg(v: Option<&Value>) -> Result<Option<(u8, u8)>, DepzError> {
    match v {
        None | Some(Value::Null) => Ok(None),
        Some(Value::Array(a)) if a.len() == 2 => Ok(Some((
            a[0].as_u64().unwrap() as u8,
            a[1].as_u64().unwrap() as u8,
        ))),
        Some(other) => Err(DepzError::InvalidArgument(format!("block {other}"))),
    }
}

fn bno086_report(r: &depz_sensor_sdk::bno086::Report) -> Value {
    Value::Object(common::shapes::report_repr(r).1)
}

fn feature(f: &depz_sensor_sdk::bno086::hub::FeatureResponse) -> Value {
    json!({
        "sensor_id": f.sensor_id, "flags": f.flags, "sensitivity": f.sensitivity, "interval_us": f.interval_us,
        "batch_us": f.batch_us, "cfg_word": f.cfg_word,
    })
}

fn tmf8829_measurement(m: &depz_sensor_sdk::tmf8829::Tmf8829Measurement) -> Value {
    let mut v = json!({
        "timestamp_us": m.timestamp_us, "flags": m.flags, "fp_mode": m.fp_mode, "f_number": m.f_number,
        "distance_mm": m.distance_mm, "n_histograms": m.histograms.len(), "n_ref_frames": m.ref_frames.len(),
        "problems": m.problems,
    });
    v.as_object_mut()
        .unwrap()
        .insert("results".into(), common::shapes::arrays(&m.results));
    v
}

// configure(preset, period_ms, kilo_iterations, nr_peaks, signal_strength, noise_strength, xtalk,
// histograms, ref_frame, fields), every argument after the preset optional.
fn tmf8829_configure(d: &depz_sensor_sdk::devices::Tmf8829Device, args: &[Value]) -> CallResult {
    let mut o = depz_sensor_sdk::devices::Tmf8829Options::default();
    let n = |i: usize| args.get(i).and_then(Value::as_u64).map(|v| v as u32);
    let b = |i: usize, d: bool| args.get(i).and_then(Value::as_bool).unwrap_or(d);
    o.period_ms = n(1).unwrap_or(o.period_ms);
    o.kilo_iterations = n(2);
    o.nr_peaks = n(3).unwrap_or(o.nr_peaks);
    (
        o.signal_strength,
        o.noise_strength,
        o.xtalk,
        o.histograms,
        o.ref_frame,
    ) = (
        b(4, false),
        b(5, false),
        b(6, false),
        b(7, false),
        b(8, false),
    );
    if let Some(Value::Object(f)) = args.get(9) {
        o.fields = f
            .iter()
            .map(|(k, v)| (k.clone(), v.as_u64().unwrap_or(0) as u32))
            .collect();
    }
    let preset = args.first().and_then(Value::as_str).unwrap_or("8x8");
    d.configure(preset, &o).map(|c| {
        json!({
            "preset": c.preset, "page": common::hex_encode(&c.page), "histograms": c.histograms, "ref_frame": c.ref_frame,
            "fields": c.fields.iter().map(|(k, v)| (k.to_string(), json!(v))).collect::<serde_json::Map<_, _>>(),
        })
    })
}

fn pair(t: (u32, u32)) -> Value {
    json!([t.0, t.1])
}

fn vl53l8_frame(f: &depz_sensor_sdk::vl53l8::driver::Vl53l8Frame) -> Value {
    json!({
        "timestamp_us": f.timestamp_us, "resolution": f.resolution, "silicon_temp_degc": f.silicon_temp_degc,
        "distance_mm": f.distance_mm, "target_status": f.target_status, "nb_target_detected": f.nb_target_detected,
        "cnh_raw": f.cnh_raw.as_deref().map(common::hex_encode),
    })
}

fn cnh_arg(
    v: Option<&Value>,
) -> Result<depz_sensor_sdk::vl53l8::driver::Vl53l8CnhSetup, DepzError> {
    let r = v.ok_or_else(|| DepzError::InvalidArgument("missing CNH setup".into()))?;
    let n = |v: &Value| {
        v.as_i64()
            .map(|x| x as i32)
            .ok_or_else(|| DepzError::InvalidArgument(format!("{v}: not an integer")))
    };
    let mut setup = depz_sensor_sdk::vl53l8::driver::Vl53l8CnhSetup::new();
    setup.init_config(
        n(&r["start_bin"])?,
        n(&r["num_bins"])?,
        n(&r["sub_sample"])?,
    )?;
    let m = r["agg_map"]
        .as_array()
        .ok_or_else(|| DepzError::InvalidArgument("agg_map".into()))?;
    setup.create_agg_map(
        n(&r["resolution"])? as usize,
        n(&m[0])?,
        n(&m[1])?,
        n(&m[2])?,
        n(&m[3])?,
        n(&m[4])?,
        n(&m[5])?,
    )?;
    Ok(setup)
}

// The calls every VL53L8 / L5 / L7 class has (they all deref to Vl53l8cxDevice).
fn vl53l8_call(
    d: &depz_sensor_sdk::devices::Vl53l8cxDevice,
    name: &str,
    args: &[Value],
) -> Option<CallResult> {
    Some(match name {
        "init" => d.init().map(|_| Value::Null),
        "set_resolution" => arg_i64(args, 0)
            .and_then(|z| d.set_resolution(z as usize))
            .map(|_| Value::Null),
        "get_resolution" => d.get_resolution().map(|z| json!(z)),
        "set_ranging_frequency_hz" => arg_i64(args, 0)
            .and_then(|hz| d.set_ranging_frequency_hz(hz as i32))
            .map(|_| Value::Null),
        "get_ranging_frequency_hz" => d.get_ranging_frequency_hz().map(int),
        "start_ranging" => d.start_ranging().map(|_| Value::Null),
        "stop_ranging" => d.stop_ranging().map(|_| Value::Null),
        _ => return None,
    })
}

fn vl53lx_measurement(m: &Vl53lxMeasurement) -> Value {
    let extra: serde_json::Map<String, Value> = m
        .extra
        .iter()
        .map(|(k, v)| match v {
            ExtraValue::Int(i) => (k.clone(), json!(i)),
            ExtraValue::Float(f) => (k.clone(), json!(f)),
        })
        .collect();
    // The frame's HistogramBinData::fields(), under the reference's names.
    let bins = m.bins.as_ref().map_or(Value::Null, |b| {
        Value::Object(
            b.fields()
                .into_iter()
                .map(|(k, v)| (k.to_string(), serde_json::from_str(&v.to_json()).unwrap()))
                .collect(),
        )
    });
    let targets: Vec<Value> = m
        .targets
        .iter()
        .map(|t| {
            json!({
                "distance_mm": t.distance_mm, "status": t.status, "status_text": t.status_text,
                "signal_kcps": t.signal_kcps, "ambient_kcps": t.ambient_kcps, "sigma_mm": t.sigma_mm,
                "min_range_mm": t.min_range_mm, "max_range_mm": t.max_range_mm,
            })
        })
        .collect();
    json!({
        "timestamp_us": m.timestamp_us,
        "distance_mm": m.distance_mm,
        "status": m.status,
        "status_text": m.status_text,
        "signal_kcps": m.signal_kcps,
        "ambient_kcps": m.ambient_kcps,
        "sigma_mm": m.sigma_mm,
        "spads": m.spads,
        "targets": targets,
        "extra": extra,
        "bins": bins,
    })
}

/// The 1D-family class underneath any of its six classes.
fn vl53lx(dev: &AnyDevice) -> Option<&Vl53lxDevice> {
    match dev {
        AnyDevice::Vl53lx(d) => Some(d),
        AnyDevice::Vl53l0x(d) => Some(d),
        AnyDevice::Vl53l1cx(d) => Some(d),
        AnyDevice::Vl53l1cb(d) => Some(d),
        AnyDevice::Vl53l3cx(d) => Some(d),
        AnyDevice::Vl53l4cx(d) => Some(d),
        _ => None,
    }
}

fn arg_str(args: &[Value], i: usize) -> Option<&str> {
    args.get(i).and_then(Value::as_str)
}

fn call_vl53lx(d: &Vl53lxDevice, name: &str, args: &[Value]) -> CallResult {
    match name {
        "init" => d
            .init(arg_str(args, 0), arg_str(args, 1))
            .map(|_| Value::Null),
        "configure" => d
            .configure(
                arg_i64(args, 0)? as u32,
                arg_i64(args, 1)? as u32,
                arg_str(args, 2),
                None,
                None,
                None,
            )
            .map(|_| Value::Null),
        "get_range_timing" => d.get_range_timing().map(pair),
        "measure_once" => d.measure_once(None).map(|m| vl53lx_measurement(&m)),
        "start_ranging" => d.start_ranging().map(|_| Value::Null),
        "stop_ranging" => d.stop_ranging().map(|_| Value::Null),
        _ => Err(unknown("call", name, "vl53lx")),
    }
}

fn get_vl53lx(d: &Vl53lxDevice, prop: &str) -> CallResult {
    match prop {
        "stream_parse_errors" => Ok(json!(d.stream_parse_errors())),
        "product" => Ok(json!(d.product())),
        "modes" => d.modes().map(|m| json!(m)),
        "budget_range" => d.budget_range().map(pair),
        "driver_reach_mm" => d.driver_reach_mm().map(|r| json!(r)),
        _ => Err(unknown("property", prop, "vl53lx")),
    }
}

fn call(dev: &AnyDevice, name: &str, args: &[Value]) -> CallResult {
    if let Some(r) = call_common(dev.device(), name, args) {
        return r;
    }
    let l8 = match dev {
        AnyDevice::Vl53l8cx(d) => Some(d),
        AnyDevice::Vl53l8ch(d) => Some(&**d),
        AnyDevice::Vl53l5cx(d) => Some(&***d),
        AnyDevice::Vl53l7cx(d) => Some(&**d),
        AnyDevice::Vl53l7ch(d) => Some(&***d),
        _ => None,
    };
    if let Some(r) = l8.and_then(|d| vl53l8_call(d, name, args)) {
        return r;
    }
    match dev {
        AnyDevice::Vl53l8ch(d) if name == "configure_cnh" => d.configure_cnh(&cnh_arg(args.first())?).map(|_| Value::Null),
        AnyDevice::Vl53l7ch(d) if name == "configure_cnh" => d.configure_cnh(&cnh_arg(args.first())?).map(|_| Value::Null),
        AnyDevice::Sr04(d) => match name {
            "set_sample_period_us" => d.set_sample_period_us(arg_i64(args, 0)? as u32).map(|_| Value::Null),
            "get_sample_period_us" => d.get_sample_period_us().map(int),
            "set_echo_decay_us" => d.set_echo_decay_us(arg_i64(args, 0)? as u16).map(int),
            "get_echo_decay_us" => d.get_echo_decay_us().map(int),
            "measure_once" => d.measure_once(None).map(|m| sr04_measurement(&m)),
            "start" => d.start().map(|_| Value::Null),
            "stop" => d.stop().map(|_| Value::Null),
            _ => Err(unknown("call", name, "sr04")),
        },
        AnyDevice::Vl53l4cd(d) => match name {
            "bridge_info" => d.bridge_info().map(|i| {
                json!({
                    "int_edges": i.int_edges, "slots_skipped": i.slots_skipped, "i2c_errors": i.i2c_errors,
                    "last_i2c_error": i.last_i2c_error, "model_id": i.model_id, "fw_status": i.fw_status,
                    "initialized": i.initialized, "xshut_level": i.xshut_level, "int_level": i.int_level,
                    "i2c_khz": i.i2c_khz,
                })
            }),
            "init" => d.init(None).map(|_| Value::Null),
            "set_range_timing" => d.set_range_timing(arg_i64(args, 0)? as u32, arg_i64(args, 1)? as u32).map(|_| Value::Null),
            "get_range_timing" => d.get_range_timing().map(pair),
            "get_offset_mm" => d.get_offset_mm().map(int),
            "get_xtalk_kcps" => d.get_xtalk_kcps().map(int),
            "measure_once" => d.measure_once(None).map(|m| vl53l4_measurement(&m)),
            "start_ranging" => d.start_ranging().map(|_| Value::Null),
            "stop_ranging" => d.stop_ranging().map(|_| Value::Null),
            _ => Err(unknown("call", name, "vl53l4cd")),
        },
        AnyDevice::Bno055(d) => match name {
            "bridge_info" => d.bridge_info().map(|i| {
                json!({
                    "i2c_addr": i.i2c_addr, "chip_id": i.chip_id, "acc_id": i.acc_id, "mag_id": i.mag_id,
                    "gyr_id": i.gyr_id, "sw_rev": i.sw_rev, "bl_rev": i.bl_rev, "initialized": i.initialized,
                    "int_level": i.int_level, "int_edges": i.int_edges, "read_min_us": i.read_min_us,
                    "read_max_us": i.read_max_us, "read_avg_us": i.read_avg_us, "tx_dropped": i.tx_dropped,
                    "i2c_errors": i.i2c_errors, "slots_skipped": i.slots_skipped, "bus_recoveries": i.bus_recoveries,
                    "last_i2c_error": i.last_i2c_error, "sensor_resets": i.sensor_resets, "loop_max_us": i.loop_max_us,
                })
            }),
            "reset_sensor" => d.reset_sensor().map(|_| Value::Null),
            "configure" => {
                let mode = depz_sensor_sdk::bno055::regs::OprMode::from_reg(arg_i64(args, 0)? as u8)
                    .ok_or_else(|| DepzError::InvalidArgument("mode".into()))?;
                let units = match args.get(1) {
                    Some(v) if !v.is_null() => Some(units_arg(v)?),
                    _ => None,
                };
                d.configure(mode, units, None, None).map(|_| Value::Null)
            }
            "read_calibration_profile" => d.read_calibration_profile().map(|c| {
                json!({
                    "accel_offset": c.accel_offset, "mag_offset": c.mag_offset, "gyro_offset": c.gyro_offset,
                    "accel_radius": c.accel_radius, "mag_radius": c.mag_radius,
                })
            }),
            "system_status" => d.system_status().map(|s| {
                json!({ "self_test": s.self_test, "clk_status": s.clk_status, "status": s.status, "error": s.error })
            }),
            "start_stream" => {
                let period = args.first().and_then(Value::as_u64).unwrap_or(10) as u16;
                d.start_stream(period, block_arg(args.get(1))?, false).map(|_| Value::Null)
            }
            "stop_stream" => d.stop_stream().map(|_| Value::Null),
            _ => Err(unknown("call", name, "bno055")),
        },
        AnyDevice::Bno086(d) => match name {
            "hardware_reset" => d.hardware_reset(None).map(|_| Value::Null),
            "product_id" => d.product_id(None).map(|p| {
                json!({
                    "reset_cause": p.reset_cause, "sw_version_major": p.sw_version_major,
                    "sw_version_minor": p.sw_version_minor, "sw_part_number": p.sw_part_number,
                    "sw_build_number": p.sw_build_number, "sw_version_patch": p.sw_version_patch,
                })
            }),
            "enable" => {
                let hz = args.get(1).and_then(Value::as_f64).ok_or_else(|| DepzError::InvalidArgument("hz".into()))?;
                d.enable(arg_i64(args, 0)? as u8, hz).map(|f| feature(&f))
            }
            "disable" => d.disable(arg_i64(args, 0)? as u8).map(|_| Value::Null),
            "get_errors" => d.get_errors(0).map(|es| {
                Value::Array(
                    es.iter()
                        .map(|e| json!({ "severity": e.severity, "seq": e.seq, "source": e.source, "error": e.error, "module": e.module, "code": e.code }))
                        .collect(),
                )
            }),
            "get_calibration" => d.get_calibration().map(|c| json!({ "accel": c.accel, "gyro": c.gyro, "mag": c.mag, "planar": c.planar })),
            "get_oscillator_type" => d.get_oscillator_type().map(int),
            "get_metadata" => d.get_metadata(arg_i64(args, 0)? as u8, None).map(|m| {
                json!({
                    "me_version": m.me_version, "mh_version": m.mh_version, "sh_version": m.sh_version,
                    "range_raw": m.range_raw, "resolution_raw": m.resolution_raw, "revision": m.revision,
                    "power_ma_q10": m.power_ma_q10, "min_period_us": m.min_period_us, "max_period_us": m.max_period_us,
                    "fifo_max": m.fifo_max, "fifo_reserved": m.fifo_reserved, "batch_buffer_bytes": m.batch_buffer_bytes,
                    "q_point_1": m.q_point_1, "q_point_2": m.q_point_2, "q_point_3": m.q_point_3, "raw_words": m.raw_words,
                })
            }),
            "get_counts" => d.get_counts(arg_i64(args, 0)? as u8).map(|c| {
                json!({ "sensor_id": c.sensor_id, "offered": c.offered, "accepted": c.accepted, "on": c.on, "attempted": c.attempted })
            }),
            _ => Err(unknown("call", name, "bno086")),
        },
        AnyDevice::Tmf8829(d) => match name {
            "init" => d.init(None).map(|r| json!({ "app_version": r.app_version, "sensor_serial": r.sensor_serial })),
            "configure" => tmf8829_configure(d, args),
            "config_page" => d.config_page().map(|p| Value::String(common::hex_encode(&p))),
            "start" => d.start().map(|_| Value::Null),
            "stop" => d.stop().map(|_| Value::Null),
            "get_bridge_info" => d.get_bridge_info().map(|i| {
                json!({
                    "int_edges": i.int_edges, "frames": i.frames, "frames_aborted": i.frames_aborted,
                    "frame_errors": i.frame_errors, "spi_timeouts": i.spi_timeouts, "int_level": i.int_level,
                    "streaming": i.streaming, "int_mask": i.int_mask, "last_int_status": i.last_int_status,
                    "last_fifo_status": i.last_fifo_status,
                })
            }),
            _ => Err(unknown("call", name, "tmf8829")),
        },
        other => match vl53lx(other) {
            Some(d) => call_vl53lx(d, name, args),
            None => Err(unknown("call", name, dev.class_name())),
        },
    }
}

fn get(dev: &AnyDevice, prop: &str) -> CallResult {
    if let Some(d) = vl53lx(dev) {
        return get_vl53lx(d, prop);
    }
    match (dev, prop) {
        (AnyDevice::Vl53l4cd(d), "stream_parse_errors") => Ok(json!(d.stream_parse_errors())),
        (AnyDevice::Bno055(d), "stream_parse_errors") => Ok(json!(d.stream_parse_errors())),
        (AnyDevice::Vl53l8cx(d), "frame_parse_errors") => Ok(json!(d.frame_parse_errors())),
        (AnyDevice::Vl53l8ch(d), "frame_parse_errors") => Ok(json!(d.frame_parse_errors())),
        (AnyDevice::Vl53l5cx(d), "frame_parse_errors") => Ok(json!(d.frame_parse_errors())),
        (AnyDevice::Vl53l7cx(d), "frame_parse_errors") => Ok(json!(d.frame_parse_errors())),
        (AnyDevice::Vl53l7ch(d), "frame_parse_errors") => Ok(json!(d.frame_parse_errors())),
        (AnyDevice::Vl53l5cx(d), "module_type") => Ok(json!(d.module_type())),
        (AnyDevice::Vl53l7cx(d), "module_type") => Ok(json!(d.module_type())),
        (AnyDevice::Vl53l7ch(d), "module_type") => Ok(json!(d.module_type())),
        (AnyDevice::Tmf8829(d), "frames_dropped") => Ok(json!(d.frames_dropped())),
        (AnyDevice::Tmf8829(d), "frames_discarded") => Ok(json!(d.frames_discarded())),
        (AnyDevice::Tmf8829(d), "frame_parse_errors") => Ok(json!(d.frame_parse_errors())),
        _ => Err(unknown("property", prop, dev.class_name())),
    }
}

fn subscribe(dev: &AnyDevice, stream: &str, capacity: usize) -> Box<dyn Taker> {
    match (dev, stream) {
        (AnyDevice::Sr04(d), "measurements") => Box::new(Typed {
            sub: d.measurements(capacity),
            canon: sr04_measurement,
        }),
        (AnyDevice::Bno086(d), "reports") => Box::new(Typed {
            sub: d.reports(None, capacity),
            canon: bno086_report,
        }),
        (AnyDevice::Vl53l8cx(d), "frames") => Box::new(Typed {
            sub: d.frames(capacity),
            canon: vl53l8_frame,
        }),
        (AnyDevice::Vl53l8ch(d), "frames") => Box::new(Typed {
            sub: d.frames(capacity),
            canon: vl53l8_frame,
        }),
        (AnyDevice::Vl53l5cx(d), "frames") => Box::new(Typed {
            sub: d.frames(capacity),
            canon: vl53l8_frame,
        }),
        (AnyDevice::Vl53l7cx(d), "frames") => Box::new(Typed {
            sub: d.frames(capacity),
            canon: vl53l8_frame,
        }),
        (AnyDevice::Vl53l7ch(d), "frames") => Box::new(Typed {
            sub: d.frames(capacity),
            canon: vl53l8_frame,
        }),
        (AnyDevice::Bno055(d), "samples") => Box::new(Typed {
            sub: d.samples(capacity),
            canon: bno055_sample,
        }),
        (AnyDevice::Tmf8829(d), "measurements") => Box::new(Typed {
            sub: d.measurements(capacity),
            canon: tmf8829_measurement,
        }),
        (AnyDevice::Vl53l4cd(d), "measurements") => Box::new(Typed {
            sub: d.measurements(capacity),
            canon: vl53l4_measurement,
        }),
        (other, "measurements") if vl53lx(other).is_some() => Box::new(Typed {
            sub: vl53lx(other).unwrap().measurements(capacity),
            canon: vl53lx_measurement,
        }),
        _ => panic!(
            "{} has no stream '{stream}' in the Rust adapter table",
            dev.class_name()
        ),
    }
}

// ── matching (§3) ───────────────────────────────────────────────────────────

fn mismatch(got: &Value, want: &Value, path: &str) -> Option<String> {
    let at = if path.is_empty() {
        "result".to_string()
    } else {
        path.to_string()
    };
    match want {
        Value::Object(w) => {
            let Value::Object(g) = got else {
                return Some(format!("{at}: expected an object, got {got}"));
            };
            for (k, wv) in w {
                let Some(gv) = g.get(k) else {
                    return Some(format!(
                        "{at}.{k}: missing (fields: {})",
                        g.keys().cloned().collect::<Vec<_>>().join(", ")
                    ));
                };
                if let Some(m) = mismatch(gv, wv, &format!("{path}.{k}")) {
                    return Some(m);
                }
            }
            None
        }
        Value::Array(w) => {
            let Value::Array(g) = got else {
                return Some(format!("{at}: expected an array, got {got}"));
            };
            if g.len() != w.len() {
                return Some(format!(
                    "{at}: expected {} elements, got {}",
                    w.len(),
                    g.len()
                ));
            }
            w.iter()
                .zip(g)
                .enumerate()
                .find_map(|(i, (wv, gv))| mismatch(gv, wv, &format!("{path}[{i}]")))
        }
        Value::Number(wn) if wn.is_f64() => {
            let wd = wn.as_f64().unwrap();
            match got.as_f64() {
                Some(gd) if got.is_number() && (gd - wd).abs() <= (1e-9 * wd.abs()).max(1e-12) => {
                    None
                }
                _ => Some(format!("{at}: expected {want}, got {got}")),
            }
        }
        Value::Number(wn) => match got {
            // An integer expectation matches a floating-point result of the same value.
            Value::Number(gn) if gn.is_f64() => {
                if gn.as_f64() == wn.as_f64() {
                    None
                } else {
                    Some(format!("{at}: expected {want}, got {got}"))
                }
            }
            Value::Number(gn) if gn.as_i64() == wn.as_i64() && gn.as_u64() == wn.as_u64() => None,
            _ => Some(format!("{at}: expected {want}, got {got}")),
        },
        _ => {
            if got == want {
                None
            } else {
                Some(format!("{at}: expected {want}, got {got}"))
            }
        }
    }
}

// ── one scenario ────────────────────────────────────────────────────────────

fn open(sc: &Value, link: Arc<ReplayLink>) -> AnyDevice {
    let (mut vid, mut pid) = (None, None);
    if let Some(model) = sc.get("usb_model").and_then(Value::as_str) {
        for &(p, m) in usb_ids::DEPZ_PID_MODEL {
            if m == model {
                vid = Some(usb_ids::DEPZ_USB_VID);
                pid = Some(p);
            }
        }
        assert!(
            pid.is_some(),
            "usb_model '{model}' is not in the USB-id table"
        );
    }
    let probe = sc
        .get("probe")
        .and_then(Value::as_str)
        .unwrap_or("canonical");
    if probe == "canonical" {
        assert!(
            sc.get("identity").is_none(),
            "`identity` is for legacy captures only"
        );
        return open_on_link(link, vid, pid, Some(REQUEST_TIMEOUT))
            .unwrap_or_else(|e| panic!("open_on_link: {e}"));
    }
    assert_eq!(probe, "legacy");
    // The probe the SDKs made before contract 02 §4.4: GET_NAME_ACTIVE_SOFTWARE, plus GET_DEVICE_NAME
    // for the L5/L7 and 1D families only; the class attached in place.
    let dev = DepzDevice::new(link, Some(REQUEST_TIMEOUT));
    let software = dev
        .get_software_name()
        .unwrap_or_else(|e| panic!("get_software_name: {e}"));
    let ident = parse_software_name(&software);
    let mut device_name = String::new();
    if matches!(
        ident.sensor_type,
        Some(SensorType::Vl53l7) | Some(SensorType::Vl53lx)
    ) {
        device_name = dev.get_device_name().unwrap_or_default();
    }
    if let Some(want) = sc.get("identity") {
        if let Some(m) = mismatch(&identity(&ident), want, "identity") {
            panic!("{m}");
        }
    }
    attach(dev, resolve_sensor_class(&software, &device_name, vid, pid))
        .unwrap_or_else(|e| panic!("attach: {e}"))
}

fn run(file: &PathBuf) -> Vec<String> {
    let sc: Value = serde_json::from_str(&std::fs::read_to_string(file).unwrap()).unwrap();
    assert_eq!(sc["schema"], "depz.scenario/1");
    let id = sc["id"].as_str().unwrap().to_string();
    assert_eq!(
        format!("{id}.json"),
        file.file_name().unwrap().to_string_lossy()
    );
    let steps = sc["steps"].as_array().unwrap();
    // No drops: each queue holds everything its scenario takes, plus what streams on after.
    let mut sizes: HashMap<String, usize> = HashMap::new();
    for s in steps {
        if let Some(t) = s.get("take").and_then(Value::as_str) {
            *sizes.entry(t.to_string()).or_default() += s["count"].as_u64().unwrap() as usize;
        }
    }
    let recording = sc["recording"].as_str().unwrap();
    let link =
        Arc::new(ReplayLink::open(common::recordings_dir().join(recording), true, false).unwrap());
    let dev = open(&sc, link.clone());
    assert_eq!(
        dev.class_name(),
        sc["class"].as_str().unwrap(),
        "{id}: resolved class"
    );
    let mut subs: HashMap<String, Box<dyn Taker>> = HashMap::new();
    for (i, step) in steps.iter().enumerate() {
        let fail = |m: String| -> ! { panic!("{id} step {i} {step}: {m}") };
        if let Some(name) = step.get("call").and_then(Value::as_str) {
            let args: Vec<Value> = step
                .get("args")
                .and_then(Value::as_array)
                .cloned()
                .unwrap_or_default();
            let r = call(&dev, name, &args);
            if let Some(want_err) = step.get("error").and_then(Value::as_str) {
                match r {
                    Err(e) if e.kind() == Some(want_err) => {}
                    Err(e) => fail(format!(
                        "expected error {want_err}, got {:?} ({e})",
                        e.kind()
                    )),
                    Ok(v) => fail(format!("expected error {want_err}, succeeded with {v}")),
                }
                continue;
            }
            let v = r.unwrap_or_else(|e| fail(format!("{e}")));
            if let Some(want) = step.get("expect") {
                if let Some(m) = mismatch(&v, want, "") {
                    fail(m);
                }
            }
        } else if let Some(stream) = step.get("subscribe").and_then(Value::as_str) {
            let name = step["as"].as_str().unwrap().to_string();
            let cap = sizes.get(&name).copied().unwrap_or(0) + 4096;
            subs.insert(name, subscribe(&dev, stream, cap));
        } else if let Some(name) = step.get("take").and_then(Value::as_str) {
            let sub = &subs[name];
            let want = step["expect"].as_array().unwrap();
            assert_eq!(step["count"].as_u64().unwrap() as usize, want.len());
            for (k, w) in want.iter().enumerate() {
                let item = sub
                    .take(TAKE_TIMEOUT)
                    .unwrap_or_else(|e| fail(format!("got {k} of {} items, then {e}", want.len())));
                if let Some(m) = mismatch(&item, w, &format!("item[{k}]")) {
                    fail(m);
                }
            }
        } else if let Some(prop) = step.get("get").and_then(Value::as_str) {
            let v = get(&dev, prop).unwrap_or_else(|e| fail(format!("{e}")));
            if let Some(m) = mismatch(&v, &step["expect"], prop) {
                fail(m);
            }
        } else {
            fail("unknown step".into());
        }
    }
    for (name, s) in &subs {
        assert_eq!(s.dropped_count(), 0, "{id}: {name}: items dropped");
    }
    if sc.get("end").and_then(Value::as_str).unwrap_or("open") == "exhausted" {
        let until = Instant::now() + Duration::from_secs(2);
        while !(link.rx_exhausted() && link.tx_remaining() == 0) && Instant::now() < until {
            std::thread::sleep(Duration::from_millis(2));
        }
        assert!(
            link.rx_exhausted() && link.tx_remaining() == 0,
            "{id}: capture not exhausted: {} tx bytes never written, {} rx chunks never delivered",
            link.tx_remaining(),
            link.rx_remaining()
        );
    }
    drop(subs);
    drop(dev);
    vec![format!("scenarios/{id}"), format!("recordings/{recording}")]
}

#[test]
fn scenarios() {
    let mut files: Vec<PathBuf> = std::fs::read_dir(scenarios_dir())
        .unwrap()
        .map(|e| e.unwrap().path())
        .filter(|p| p.extension().is_some_and(|x| x == "json"))
        .collect();
    files.sort();
    let filter = std::env::var("DEPZ_SCENARIO").ok();
    let mut ran = 0;
    for f in files {
        let sc: Value = serde_json::from_str(&std::fs::read_to_string(&f).unwrap()).unwrap();
        let class = sc["class"].as_str().unwrap();
        if !CLASSES.contains(&class)
            || filter
                .as_deref()
                .is_some_and(|x| !sc["id"].as_str().unwrap().contains(x))
        {
            continue;
        }
        for id in run(&f) {
            common::seen(id.as_str());
        }
        ran += 1;
    }
    assert!(ran > 0 || filter.is_some(), "no scenario ran");
}
