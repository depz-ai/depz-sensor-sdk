//! SDK results → golden-vector shapes (the JSON each vector group writes under
//! `expect`), shared by the vector tests and the differential-testing adapter
//! (`tests/difftest/adapter.rs`, contracts/16_DIFFTEST.md) — the Rust
//! counterpart of TS `test/golden/shapes.ts` / Python `tests/vector_shapes.py`.
//!
//! Decoders return `Err(detail)` where the SDK refuses the input.

use depz_sensor_sdk::bno055::{decode_block, Bno055Info, RawBlock, RegData as BnoRegData, StreamData as BnoStream};
use depz_sensor_sdk::bno086::reports::{Report, Vector3Kind};
use depz_sensor_sdk::bno086::shtp::ShtpCargo;
use depz_sensor_sdk::framing::{Event, PacketParser};
use depz_sensor_sdk::tmf8829::{
    parse_frame, parse_results_info, FrameChunk, FrameReassembler, MeasurementBuilder,
    PixelResults, RegData as TmfRegData, Tmf8829Event, Tmf8829Info, FID_RESULTS,
};
use depz_sensor_sdk::vl53l4::{parse_result_block, RegData as L4RegData, StreamData as L4Stream, Vl53l4Info};
use depz_sensor_sdk::vl53l7::Vl53l7Info;
use depz_sensor_sdk::vl53lx::{decode_die_block, decode_histogram_raw, decode_l0x_raw, DieVariant, Vl53lxInfo};
use serde_json::{json, Map, Value};

use super::hex_encode;

fn err<E: std::fmt::Debug>(e: E) -> String {
    format!("{:?}", e)
}

fn i16s(v: &[i16]) -> Vec<i64> {
    v.iter().map(|&x| x as i64).collect()
}

/// `(name, value)` pairs → a JSON object.
fn object<V: Into<Value>, const N: usize>(pairs: [(&str, V); N]) -> Value {
    Value::Object(pairs.into_iter().map(|(k, v)| (k.to_string(), v.into())).collect())
}

// ---- framing (contract 01 §5) ------------------------------------------------

/// Feed `chunks` in order to one parser; the framing_decode.json `expect` shape.
pub fn framing_collect(parser: &mut PacketParser, chunks: &[Vec<u8>]) -> Value {
    let mut events = Vec::new();
    let mut trash = Vec::new();
    for chunk in chunks {
        for ev in parser.feed(chunk) {
            match ev {
                Event::Packet(p) => events.push(json!({
                    "type": "packet", "cmd": p.cmd, "seq": p.seq, "payload": hex_encode(&p.payload),
                })),
                Event::CrcError { cmd, seq } => {
                    events.push(json!({"type": "crc_error", "cmd": cmd, "seq": seq}))
                }
                Event::Trash(data) => trash.extend_from_slice(&data),
            }
        }
    }
    json!({
        "events": events,
        "trash": hex_encode(&trash),
        "residue": hex_encode(parser.residue()),
        "header_errors": parser.header_errors,
    })
}

// ---- BNO055 (moved from bno055.rs) ----
/// RPT_BNO_INFO as the vectors' (name, value) pairs.
pub fn info_fields(i: &Bno055Info) -> [(&'static str, u64); 20] {
    [
        ("i2c_addr", i.i2c_addr as u64),
        ("chip_id", i.chip_id as u64),
        ("acc_id", i.acc_id as u64),
        ("mag_id", i.mag_id as u64),
        ("gyr_id", i.gyr_id as u64),
        ("sw_rev", i.sw_rev as u64),
        ("bl_rev", i.bl_rev as u64),
        ("initialized", i.initialized as u64),
        ("int_level", i.int_level as u64),
        ("int_edges", i.int_edges as u64),
        ("read_min_us", i.read_min_us as u64),
        ("read_max_us", i.read_max_us as u64),
        ("read_avg_us", i.read_avg_us as u64),
        ("tx_dropped", i.tx_dropped as u64),
        ("i2c_errors", i.i2c_errors as u64),
        ("slots_skipped", i.slots_skipped as u64),
        ("bus_recoveries", i.bus_recoveries as u64),
        ("last_i2c_error", i.last_i2c_error as u64),
        ("sensor_resets", i.sensor_resets as u64),
        ("loop_max_us", i.loop_max_us as u64),
    ]
}

/// A decoded register window's int channels as (name, values) pairs.
pub fn block_fields(r: &RawBlock) -> [(&'static str, Option<Vec<i64>>); 7] {
    [
        ("accel", r.accel.map(|v| i16s(&v))),
        ("mag", r.mag.map(|v| i16s(&v))),
        ("gyro", r.gyro.map(|v| i16s(&v))),
        ("euler", r.euler.map(|v| i16s(&v))),
        ("quaternion", r.quaternion.map(|v| i16s(&v))),
        ("linear_accel", r.linear_accel.map(|v| i16s(&v))),
        ("gravity", r.gravity.map(|v| i16s(&v))),
    ]
}

// ---- BNO086 (moved from bno086_reports.rs) ----
/// Report → (type string, field map) with the vector's field naming.
pub fn report_repr(r: &Report) -> (&'static str, Map<String, Value>) {
    let mut m = Map::new();
    let ty = match r {
        Report::Vector3 {
            kind,
            sensor_id,
            timestamp_us,
            seq,
            accuracy,
            delay_us,
            x_raw,
            y_raw,
            z_raw,
        } => {
            m.insert("sensor_id".into(), json!(sensor_id));
            m.insert("timestamp_us".into(), json!(timestamp_us));
            m.insert("seq".into(), json!(seq));
            m.insert("accuracy".into(), json!(accuracy));
            m.insert("delay_us".into(), json!(delay_us));
            m.insert("x_raw".into(), json!(x_raw));
            m.insert("y_raw".into(), json!(y_raw));
            m.insert("z_raw".into(), json!(z_raw));
            match kind {
                Vector3Kind::Acceleration => "Acceleration",
                Vector3Kind::Gyroscope => "Gyroscope",
                Vector3Kind::Magnetometer => "Magnetometer",
            }
        }
        Report::Vector3WithBias {
            is_gyro,
            sensor_id,
            timestamp_us,
            seq,
            accuracy,
            delay_us,
            x_raw,
            y_raw,
            z_raw,
            bias_x_raw,
            bias_y_raw,
            bias_z_raw,
        } => {
            m.insert("sensor_id".into(), json!(sensor_id));
            m.insert("timestamp_us".into(), json!(timestamp_us));
            m.insert("seq".into(), json!(seq));
            m.insert("accuracy".into(), json!(accuracy));
            m.insert("delay_us".into(), json!(delay_us));
            m.insert("x_raw".into(), json!(x_raw));
            m.insert("y_raw".into(), json!(y_raw));
            m.insert("z_raw".into(), json!(z_raw));
            m.insert("bias_x_raw".into(), json!(bias_x_raw));
            m.insert("bias_y_raw".into(), json!(bias_y_raw));
            m.insert("bias_z_raw".into(), json!(bias_z_raw));
            if *is_gyro {
                "UncalibratedGyroscope"
            } else {
                "UncalibratedMagnetometer"
            }
        }
        Report::RotationVector {
            sensor_id,
            timestamp_us,
            seq,
            accuracy,
            delay_us,
            i_raw,
            j_raw,
            k_raw,
            real_raw,
            accuracy_raw,
        } => {
            m.insert("sensor_id".into(), json!(sensor_id));
            m.insert("timestamp_us".into(), json!(timestamp_us));
            m.insert("seq".into(), json!(seq));
            m.insert("accuracy".into(), json!(accuracy));
            m.insert("delay_us".into(), json!(delay_us));
            m.insert("i_raw".into(), json!(i_raw));
            m.insert("j_raw".into(), json!(j_raw));
            m.insert("k_raw".into(), json!(k_raw));
            m.insert("real_raw".into(), json!(real_raw));
            m.insert("accuracy_raw".into(), json!(accuracy_raw));
            "RotationVector"
        }
        Report::Scalar {
            sensor_id,
            timestamp_us,
            seq,
            accuracy,
            delay_us,
            value_raw,
        } => {
            m.insert("sensor_id".into(), json!(sensor_id));
            m.insert("timestamp_us".into(), json!(timestamp_us));
            m.insert("seq".into(), json!(seq));
            m.insert("accuracy".into(), json!(accuracy));
            m.insert("delay_us".into(), json!(delay_us));
            m.insert("value_raw".into(), json!(value_raw));
            "ScalarReport"
        }
        Report::TapDetector {
            sensor_id,
            timestamp_us,
            seq,
            accuracy,
            delay_us,
            flags,
        } => {
            m.insert("sensor_id".into(), json!(sensor_id));
            m.insert("timestamp_us".into(), json!(timestamp_us));
            m.insert("seq".into(), json!(seq));
            m.insert("accuracy".into(), json!(accuracy));
            m.insert("delay_us".into(), json!(delay_us));
            m.insert("flags".into(), json!(flags));
            "TapDetector"
        }
        Report::StepCounter {
            sensor_id,
            timestamp_us,
            seq,
            accuracy,
            delay_us,
            latency_us,
            steps,
        } => {
            m.insert("sensor_id".into(), json!(sensor_id));
            m.insert("timestamp_us".into(), json!(timestamp_us));
            m.insert("seq".into(), json!(seq));
            m.insert("accuracy".into(), json!(accuracy));
            m.insert("delay_us".into(), json!(delay_us));
            m.insert("latency_us".into(), json!(latency_us));
            m.insert("steps".into(), json!(steps));
            "StepCounter"
        }
        Report::StepDetector {
            sensor_id,
            timestamp_us,
            seq,
            accuracy,
            delay_us,
            latency_us,
        } => {
            m.insert("sensor_id".into(), json!(sensor_id));
            m.insert("timestamp_us".into(), json!(timestamp_us));
            m.insert("seq".into(), json!(seq));
            m.insert("accuracy".into(), json!(accuracy));
            m.insert("delay_us".into(), json!(delay_us));
            m.insert("latency_us".into(), json!(latency_us));
            "StepDetector"
        }
        Report::SignificantMotion {
            sensor_id,
            timestamp_us,
            seq,
            accuracy,
            delay_us,
            motion,
        } => {
            m.insert("sensor_id".into(), json!(sensor_id));
            m.insert("timestamp_us".into(), json!(timestamp_us));
            m.insert("seq".into(), json!(seq));
            m.insert("accuracy".into(), json!(accuracy));
            m.insert("delay_us".into(), json!(delay_us));
            m.insert("motion".into(), json!(motion));
            "SignificantMotion"
        }
        Report::StabilityClassifier {
            sensor_id,
            timestamp_us,
            seq,
            accuracy,
            delay_us,
            classification,
        } => {
            m.insert("sensor_id".into(), json!(sensor_id));
            m.insert("timestamp_us".into(), json!(timestamp_us));
            m.insert("seq".into(), json!(seq));
            m.insert("accuracy".into(), json!(accuracy));
            m.insert("delay_us".into(), json!(delay_us));
            m.insert("classification".into(), json!(classification));
            "StabilityClassifier"
        }
        Report::MotionRequest {
            sensor_id,
            timestamp_us,
            seq,
            accuracy,
            delay_us,
            intent,
            request,
        } => {
            m.insert("sensor_id".into(), json!(sensor_id));
            m.insert("timestamp_us".into(), json!(timestamp_us));
            m.insert("seq".into(), json!(seq));
            m.insert("accuracy".into(), json!(accuracy));
            m.insert("delay_us".into(), json!(delay_us));
            m.insert("intent".into(), json!(intent));
            m.insert("request".into(), json!(request));
            "MotionRequest"
        }
        Report::ShakeDetector {
            sensor_id,
            timestamp_us,
            seq,
            accuracy,
            delay_us,
            flags,
        } => {
            m.insert("sensor_id".into(), json!(sensor_id));
            m.insert("timestamp_us".into(), json!(timestamp_us));
            m.insert("seq".into(), json!(seq));
            m.insert("accuracy".into(), json!(accuracy));
            m.insert("delay_us".into(), json!(delay_us));
            m.insert("flags".into(), json!(flags));
            "ShakeDetector"
        }
        Report::PersonalActivityClassifier {
            sensor_id,
            timestamp_us,
            seq,
            accuracy,
            delay_us,
            page_number,
            end_of_sequence,
            most_likely_state,
            confidences,
        } => {
            m.insert("sensor_id".into(), json!(sensor_id));
            m.insert("timestamp_us".into(), json!(timestamp_us));
            m.insert("seq".into(), json!(seq));
            m.insert("accuracy".into(), json!(accuracy));
            m.insert("delay_us".into(), json!(delay_us));
            m.insert("page_number".into(), json!(page_number));
            m.insert("end_of_sequence".into(), json!(*end_of_sequence as u8));
            m.insert("most_likely_state".into(), json!(most_likely_state));
            m.insert("confidences".into(), json!(confidences));
            "PersonalActivityClassifier"
        }
        Report::RawSensor {
            sensor_id,
            timestamp_us,
            seq,
            accuracy,
            delay_us,
            x_raw,
            y_raw,
            z_raw,
            sensor_timestamp_us,
            temperature_raw,
        } => {
            m.insert("sensor_id".into(), json!(sensor_id));
            m.insert("timestamp_us".into(), json!(timestamp_us));
            m.insert("seq".into(), json!(seq));
            m.insert("accuracy".into(), json!(accuracy));
            m.insert("delay_us".into(), json!(delay_us));
            m.insert("x_raw".into(), json!(x_raw));
            m.insert("y_raw".into(), json!(y_raw));
            m.insert("z_raw".into(), json!(z_raw));
            m.insert("sensor_timestamp_us".into(), json!(sensor_timestamp_us));
            m.insert("temperature_raw".into(), json!(temperature_raw));
            "RawSensor"
        }
        Report::GenericEvent {
            sensor_id,
            timestamp_us,
            seq,
            accuracy,
            delay_us,
            value_raw,
        } => {
            m.insert("sensor_id".into(), json!(sensor_id));
            m.insert("timestamp_us".into(), json!(timestamp_us));
            m.insert("seq".into(), json!(seq));
            m.insert("accuracy".into(), json!(accuracy));
            m.insert("delay_us".into(), json!(delay_us));
            m.insert("value_raw".into(), json!(value_raw));
            "GenericEvent"
        }
        Report::GyroIntegratedRv {
            sensor_id,
            timestamp_us,
            i_raw,
            j_raw,
            k_raw,
            real_raw,
            vx_raw,
            vy_raw,
            vz_raw,
        } => {
            m.insert("sensor_id".into(), json!(sensor_id));
            m.insert("timestamp_us".into(), json!(timestamp_us));
            m.insert("i_raw".into(), json!(i_raw));
            m.insert("j_raw".into(), json!(j_raw));
            m.insert("k_raw".into(), json!(k_raw));
            m.insert("real_raw".into(), json!(real_raw));
            m.insert("vx_raw".into(), json!(vx_raw));
            m.insert("vy_raw".into(), json!(vy_raw));
            m.insert("vz_raw".into(), json!(vz_raw));
            "GyroIntegratedRV"
        }
        Report::FlushCompleted {
            sensor_id,
            timestamp_us,
            flushed_sensor,
        } => {
            m.insert("sensor_id".into(), json!(sensor_id));
            m.insert("timestamp_us".into(), json!(timestamp_us));
            m.insert("flushed_sensor".into(), json!(flushed_sensor));
            "FlushCompleted"
        }
        Report::Unknown {
            sensor_id,
            timestamp_us,
            data,
        } => {
            m.insert("sensor_id".into(), json!(sensor_id));
            m.insert("timestamp_us".into(), json!(timestamp_us));
            m.insert("data".into(), json!(hex_encode(data)));
            "UnknownReport"
        }
    };
    (ty, m)
}

// ---- TMF8829 (moved from tmf8829.rs) ----
/// Minimal SHA-256 (FIPS 180-4) — the crate has no hash dependency and the
/// vectors pin frame / firmware bytes by digest.
pub fn sha256(data: &[u8]) -> String {
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
pub fn arrays(r: &PixelResults) -> Value {
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


/// bno055.json `decode` shapes (0x91 REG_DATA, 0x92 INFO, 0x93 REG_STREAM).
pub fn bno055_decode(report: u64, raw: &[u8]) -> Result<Value, String> {
    Ok(match report {
        0x92 => object(info_fields(&Bno055Info::unpack(raw).map_err(err)?)),
        0x91 => {
            let r = BnoRegData::unpack(raw).map_err(err)?;
            json!({"cmd": r.cmd, "timestamp_us": r.timestamp_us, "data": hex_encode(&r.data)})
        }
        0x93 => {
            let s = BnoStream::unpack(raw).map_err(err)?;
            json!({"timestamp_us": s.timestamp_us, "addr": s.addr, "len": s.len, "data": hex_encode(&s.data)})
        }
        r => return Err(format!("unknown report {:#x}", r)),
    })
}

/// bno055.json `blocks` shape: every channel (null when outside the window).
pub fn bno055_block(addr: u8, data: &[u8]) -> Value {
    let r = decode_block(addr, data);
    let mut m: Map<String, Value> =
        block_fields(&r).into_iter().map(|(k, v)| (k.to_string(), json!(v))).collect();
    m.insert("temperature".into(), json!(r.temperature));
    m.insert("calib_stat".into(), json!(r.calib_stat));
    Value::Object(m)
}

/// A reassembled SHTP cargo (bno086_shtp.json `reassembly`).
pub fn bno086_cargo(c: &ShtpCargo) -> Value {
    json!({"channel": c.channel, "seq": c.seq, "payload": hex_encode(&c.payload), "capture_us": c.capture_us})
}

/// `{type, fields}` of a decoded SH-2 report.
pub fn bno086_typed(r: &Report) -> Value {
    let (ty, fields) = report_repr(r);
    json!({"type": ty, "fields": fields})
}

// ---- VL53L4CD / L7 / LX ------------------------------------------------------

/// vl53l4.json `decode` shapes.
pub fn vl53l4_decode(report: u64, raw: &[u8]) -> Result<Value, String> {
    Ok(match report {
        0x91 => {
            let r = L4RegData::unpack(raw).map_err(err)?;
            json!({"cmd": r.cmd, "timestamp_us": r.timestamp_us, "data": hex_encode(&r.data)})
        }
        0x92 => {
            let r = Vl53l4Info::unpack(raw).map_err(err)?;
            json!({
                "int_edges": r.int_edges, "slots_skipped": r.slots_skipped,
                "i2c_errors": r.i2c_errors, "last_i2c_error": r.last_i2c_error,
                "model_id": r.model_id, "fw_status": r.fw_status, "initialized": r.initialized,
                "xshut_level": r.xshut_level, "int_level": r.int_level, "i2c_khz": r.i2c_khz,
            })
        }
        0x93 => {
            let r = L4Stream::unpack(raw).map_err(err)?;
            json!({"timestamp_us": r.timestamp_us, "addr": r.addr, "len": r.len, "data": hex_encode(&r.data)})
        }
        r => return Err(format!("unknown report {:#x}", r)),
    })
}

/// vl53l4.json `result_block` shape.
pub fn vl53l4_result(raw: &[u8]) -> Result<Value, String> {
    let r = parse_result_block(raw).map_err(err)?;
    Ok(json!({
        "range_status": r.range_status, "distance_mm": r.distance_mm,
        "ambient_rate_kcps": r.ambient_rate_kcps, "ambient_per_spad_kcps": r.ambient_per_spad_kcps,
        "signal_rate_kcps": r.signal_rate_kcps, "signal_per_spad_kcps": r.signal_per_spad_kcps,
        "number_of_spad": r.number_of_spad, "sigma_mm": r.sigma_mm, "stream_count": r.stream_count,
    }))
}

/// vl53l7.json `decode` shape (RPT_VL53_INFO).
pub fn vl53l7_info(raw: &[u8]) -> Result<Value, String> {
    let i = Vl53l7Info::unpack(raw).map_err(err)?;
    Ok(json!({
        "int_edges": i.int_edges, "frames_dropped": i.frames_dropped,
        "i2c_errors": i.i2c_errors, "last_i2c_error": i.last_i2c_error,
        "lpn_level": i.lpn_level, "int_level": i.int_level, "i2c_khz": i.i2c_khz,
        "frame_size": i.frame_size, "streaming": i.streaming,
    }))
}

/// vl53lx.json `decode` shape (RPT_VL53_INFO).
pub fn vl53lx_info(raw: &[u8]) -> Result<Value, String> {
    let i = Vl53lxInfo::unpack(raw).map_err(err)?;
    Ok(json!({
        "int_edges": i.int_edges, "slots_skipped": i.slots_skipped,
        "i2c_errors": i.i2c_errors, "last_i2c_error": i.last_i2c_error,
        "xshut_level": i.xshut_level, "int_level": i.int_level, "i2c_khz": i.i2c_khz,
        "addr_width": i.addr_width, "n_clear": i.n_clear, "frames_dropped": i.frames_dropped,
    }))
}

/// vl53lx.json `die_block` shape; `variant` by name (`l4` / `l1`).
pub fn vl53lx_die_block(raw: &[u8], variant: &str) -> Result<Value, String> {
    let v = DieVariant::from_name(variant).ok_or_else(|| format!("unknown variant {:?}", variant))?;
    let r = decode_die_block(raw, v).map_err(err)?;
    Ok(json!({
        "range_status": r.range_status, "distance_mm": r.distance_mm, "sigma_mm": r.sigma_mm,
        "signal_rate_kcps": r.signal_rate_kcps, "ambient_rate_kcps": r.ambient_rate_kcps,
        "signal_per_spad_kcps": r.signal_per_spad_kcps, "ambient_per_spad_kcps": r.ambient_per_spad_kcps,
        "number_of_spad": r.number_of_spad, "stream_count": r.stream_count,
    }))
}

/// vl53lx.json `l0x_raw` shape.
pub fn vl53lx_l0x_raw(raw: &[u8]) -> Result<Value, String> {
    let r = decode_l0x_raw(raw).map_err(err)?;
    Ok(json!({
        "distance_raw": r.distance_raw, "device_range_status": r.device_range_status,
        "signal_rate_mcps_1616": r.signal_rate_mcps_1616, "ambient_rate_mcps_1616": r.ambient_rate_mcps_1616,
        "effective_spad_count_88": r.effective_spad_count_88,
    }))
}

/// vl53lx.json `histogram_raw` shape.
pub fn vl53lx_histogram_raw(raw: &[u8]) -> Result<Value, String> {
    let r = decode_histogram_raw(raw).map_err(err)?;
    Ok(json!({
        "interrupt_status": r.interrupt_status, "range_status": r.range_status,
        "report_status": r.report_status, "stream_count": r.stream_count,
        "dss_actual_effective_spads": r.dss_actual_effective_spads,
        "reference_phase": r.reference_phase, "vcsel_start": r.vcsel_start,
        "bins": r.bins.to_vec(),
    }))
}

// ---- TMF8829 -----------------------------------------------------------------

/// tmf8829.json `decode` shapes (0x91 REG_DATA, 0x92 INFO, 0x93 FRAME, 0x94 EVENT).
pub fn tmf8829_decode(report: u64, raw: &[u8]) -> Result<Value, String> {
    Ok(match report {
        0x91 => {
            let r = TmfRegData::unpack(raw).map_err(err)?;
            json!({"timestamp_us": r.timestamp_us, "data": hex_encode(&r.data)})
        }
        0x92 => {
            let i = Tmf8829Info::unpack(raw).map_err(err)?;
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
            let f = FrameChunk::unpack(raw).map_err(err)?;
            json!({
                "timestamp_us": f.timestamp_us, "full_size": f.full_size,
                "offset": f.offset, "frame_seq": f.frame_seq, "edge_us": f.edge_us,
                "data": hex_encode(&f.data),
            })
        }
        0x94 => {
            let e = Tmf8829Event::unpack(raw).map_err(err)?;
            json!({
                "timestamp_us": e.timestamp_us, "int_status": e.int_status,
                "code": e.code, "frame_id": e.frame_id, "fifo_status": e.fifo_status,
            })
        }
        r => return Err(format!("unknown report {:#x}", r)),
    })
}

/// FRAME-chunk payloads through one reassembler; tmf8829.json `reassembly` shape.
pub fn tmf8829_reassembly(chunks: &[Vec<u8>]) -> Result<Value, String> {
    let mut r = FrameReassembler::new();
    let mut done = Vec::new();
    for ch in chunks {
        if let Some(f) = r.feed(FrameChunk::unpack(ch).map_err(err)?) {
            done.push(json!({
                "frame_seq": f.frame_seq,
                "timestamp_us": f.timestamp_us,
                "edge_us": f.edge_us,
                "size": f.data.len(),
                "sha256": sha256(&f.data),
            }));
        }
    }
    Ok(json!({"frames": done, "completed": r.completed, "discarded": r.discarded, "seq_gaps": r.seq_gaps}))
}

/// tmf8829.json `frames` shape: header, problems, then results or data_sha256.
pub fn tmf8829_frame(raw: &[u8]) -> Result<Value, String> {
    let f = parse_frame(raw).map_err(err)?;
    let mut out = json!({
        "header": {
            "fifo_status": f.fifo_status, "systick": f.systick, "id": f.id,
            "kind": f.kind(), "fp_mode": f.fp_mode(), "layout": f.layout,
            "payload": f.payload, "f_number": f.f_number,
            "temperature": f.temperature.to_vec(), "bdv": f.bdv,
            "ref_pos": f.ref_pos.to_vec(),
            "t0_integration": f.t0_integration, "t1_integration": f.t1_integration,
            "frame_status": f.frame_status, "reserved": f.reserved, "eof": f.eof,
            "size": f.size,
        },
        "problems": f.problems(),
    });
    let m = out.as_object_mut().unwrap();
    if f.kind() == FID_RESULTS {
        m.insert("results".into(), arrays(&parse_results_info(raw, &f).map_err(err)?));
    } else {
        let body = raw.get(21..raw.len().saturating_sub(12)).unwrap_or(&[]);
        m.insert("data_sha256".into(), json!(sha256(body)));
    }
    Ok(out)
}

/// `(raw, timestamp_us, edge_us)` frames in order through one builder;
/// tmf8829.json `measurements` `expect` list.
pub fn tmf8829_measurements(frames: &[(Vec<u8>, u64, u64)]) -> Value {
    let mut b = MeasurementBuilder::new();
    let mut got = Vec::new();
    for (raw, ts, edge) in frames {
        let Some(m) = b.add(raw, *ts, *edge) else { continue };
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
            "distance_in_mm": m.frames.last().map(|f| f.distance_in_mm()),
        });
        item.as_object_mut().unwrap().extend(arrays(&m.results).as_object().unwrap().clone());
        got.push(item);
    }
    Value::from(got)
}
