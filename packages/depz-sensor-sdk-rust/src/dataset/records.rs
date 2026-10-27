//! The contract 09 record value (`v`) of each kind, from the device classes' results — what
//! [`super::SessionRecorder`] writes, public for writers of their own. Key for key the reference
//! Python SDK's values (integers as integers, reals as floats, in its key order).

use crate::bno086::reports::{q_point, Report, Vector3Kind, GYRO_RV_ANGVEL_Q, RV_ACCURACY_Q};
use crate::devices::{Bno055Sample, Sr04Measurement, Vl53l4Measurement, Vl53lxMeasurement};
use crate::json::Value;
use crate::tmf8829::Tmf8829Measurement;
use crate::vl53l8::driver::{Vl53l8CnhSetup, Vl53l8Frame};

fn int(v: impl Into<i64>) -> Value {
    Value::int(v.into())
}

fn ints<T: Copy + Into<i64>>(a: &[T]) -> Value {
    Value::Array(a.iter().map(|&x| int(x)).collect())
}

fn floats(a: &[f64]) -> Value {
    Value::Array(a.iter().map(|&x| Value::float(x)).collect())
}

fn obj(entries: Vec<(&str, Value)>) -> Value {
    Value::Object(
        entries
            .into_iter()
            .map(|(k, v)| (k.to_string(), v))
            .collect(),
    )
}

/// `sr04`: `{echo_us, source}`.
pub fn sr04(m: &Sr04Measurement) -> Value {
    obj(vec![
        ("echo_us", int(m.echo_time_us)),
        ("source", Value::String(m.source.to_string())),
    ])
}

/// `vl53l4`: the result fields, distance raw whatever the status.
pub fn vl53l4(m: &Vl53l4Measurement) -> Value {
    obj(vec![
        ("range_status", int(m.range_status)),
        ("distance_mm", int(m.distance_mm)),
        ("sigma_mm", int(m.sigma_mm)),
        ("signal_rate_kcps", int(m.signal_rate_kcps)),
        ("ambient_rate_kcps", int(m.ambient_rate_kcps)),
        ("number_of_spad", int(m.number_of_spad)),
        ("stream_count", int(m.stream_count)),
    ])
}

/// `vl53l8`: per-zone arrays cut to the resolution; `layers` adds signal, ambient, sigma and
/// reflectance; `motion` when the indicator is configured.
pub fn vl53l8(f: &Vl53l8Frame, layers: bool) -> Value {
    let n = f.resolution;
    let cut = |len: usize| n.min(len);
    let mut v = vec![
        ("resolution", int(n as i64)),
        ("silicon_temp_degc", int(f.silicon_temp_degc)),
        (
            "distance_mm",
            ints(&f.distance_mm[..cut(f.distance_mm.len())]),
        ),
        (
            "target_status",
            ints(&f.target_status[..cut(f.target_status.len())]),
        ),
        (
            "nb_target_detected",
            ints(&f.nb_target_detected[..cut(f.nb_target_detected.len())]),
        ),
    ];
    if layers {
        v.push((
            "signal_per_spad",
            floats(&f.signal_per_spad[..cut(f.signal_per_spad.len())]),
        ));
        v.push((
            "ambient_per_spad",
            floats(&f.ambient_per_spad[..cut(f.ambient_per_spad.len())]),
        ));
        v.push((
            "range_sigma_mm",
            floats(&f.range_sigma_mm[..cut(f.range_sigma_mm.len())]),
        ));
        v.push((
            "reflectance",
            floats(&f.reflectance[..cut(f.reflectance.len())]),
        ));
    }
    if let Some(mo) = &f.motion {
        v.push((
            "motion",
            obj(vec![
                ("global_indicator_1", int(mo.global_indicator_1)),
                ("global_indicator_2", int(mo.global_indicator_2)),
                ("status", int(mo.status)),
                (
                    "nb_of_detected_aggregates",
                    int(mo.nb_of_detected_aggregates),
                ),
                ("nb_of_aggregates", int(mo.nb_of_aggregates)),
                ("motion", ints(&mo.motion)),
            ]),
        ));
    }
    obj(v)
}

/// `vl53l8_cnh`: the raw CNH block of a frame with the armed setup's decode parameters.
pub fn vl53l8_cnh(cnh_raw: &[u8], setup: &Vl53l8CnhSetup) -> Value {
    obj(vec![
        ("nb_aggregates", int(setup.nb_of_aggregates())),
        ("feature_length", int(setup.feature_length())),
        ("sum_span", int(setup.sum_span())),
        (
            "start_bin",
            int((setup.ref_bin_offset() as f64 / 2048.0).floor() as i64),
        ),
        ("cnh_raw_b64", Value::String(base64(cnh_raw))),
    ])
}

/// `vl53lx`: the family-wide result, plus `targets` when the driver reported several.
pub fn vl53lx(m: &Vl53lxMeasurement, product: &str, driver: &str) -> Value {
    let mut v = vec![
        ("product", Value::String(product.into())),
        ("driver", Value::String(driver.into())),
        ("status", int(m.status)),
        ("distance_mm", int(m.distance_mm)),
        ("sigma_mm", Value::float(m.sigma_mm)),
        ("signal_kcps", Value::float(m.signal_kcps)),
        ("ambient_kcps", Value::float(m.ambient_kcps)),
        ("spads", Value::float(m.spads)),
    ];
    if !m.targets.is_empty() {
        let targets = m
            .targets
            .iter()
            .map(|t| {
                obj(vec![
                    ("distance_mm", int(t.distance_mm)),
                    ("status", int(t.status)),
                    ("signal_kcps", Value::float(t.signal_kcps)),
                ])
            })
            .collect();
        v.push(("targets", Value::Array(targets)));
    }
    obj(v)
}

/// `bno055`: `unit_sel` and every channel the streamed block covered, scaled.
pub fn bno055(s: &Bno055Sample) -> Value {
    let mut v = vec![("unit_sel", int(s.units.pack()))];
    let channels: [(&str, Option<&[f64]>); 7] = [
        ("quaternion", s.quaternion.as_ref().map(|a| &a[..])),
        ("euler", s.euler.as_ref().map(|a| &a[..])),
        ("accel", s.accel.as_ref().map(|a| &a[..])),
        ("gyro", s.gyro.as_ref().map(|a| &a[..])),
        ("mag", s.mag.as_ref().map(|a| &a[..])),
        ("linear_accel", s.linear_accel.as_ref().map(|a| &a[..])),
        ("gravity", s.gravity.as_ref().map(|a| &a[..])),
    ];
    for (k, a) in channels {
        if let Some(a) = a {
            v.push((k, floats(a)));
        }
    }
    if let Some(t) = s.temperature {
        v.push(("temperature", Value::float(t)));
    }
    if let Some(c) = &s.calibration {
        v.push(("calib", ints(&[c.system, c.gyro, c.accel, c.mag])));
    }
    obj(v)
}

/// `tmf8829`: one measurement — distances in mm rounded to 0.1, the raw per-zone arrays.
pub fn tmf8829(m: &Tmf8829Measurement) -> Value {
    let r = &m.results;
    let (rows, cols) = m.shape();
    let dist = m
        .distance_mm
        .as_deref()
        .unwrap_or(&[])
        .iter()
        .map(|&x| round1(x))
        .collect::<Vec<_>>();
    let mut v = vec![
        ("fp_mode", int(m.fp_mode)),
        ("rows", int(rows as i64)),
        ("cols", int(cols as i64)),
        ("peaks", int(r.peaks as i64)),
        ("f_number", int(m.f_number)),
        ("flags", int(m.flags)),
        ("temperature_c", int(m.temperature_c())),
        ("distance_mm", floats(&dist)),
        ("snr", ints(r.snr.as_deref().unwrap_or(&[]))),
    ];
    for (k, a) in [
        ("signal", &r.signal),
        ("noise", &r.noise),
        ("xtalk", &r.xtalk),
    ] {
        if let Some(a) = a {
            v.push((k, ints(a)));
        }
    }
    obj(v)
}

/// The report type name of the reference SDK (`Acceleration`, `RotationVector`, `ScalarReport` …).
pub fn bno086_type(r: &Report) -> &'static str {
    match r {
        Report::Vector3 {
            kind: Vector3Kind::Acceleration,
            ..
        } => "Acceleration",
        Report::Vector3 {
            kind: Vector3Kind::Gyroscope,
            ..
        } => "Gyroscope",
        Report::Vector3 {
            kind: Vector3Kind::Magnetometer,
            ..
        } => "Magnetometer",
        Report::Vector3WithBias { is_gyro: true, .. } => "UncalibratedGyroscope",
        Report::Vector3WithBias { is_gyro: false, .. } => "UncalibratedMagnetometer",
        Report::RotationVector { .. } => "RotationVector",
        Report::Scalar { .. } => "ScalarReport",
        Report::TapDetector { .. } => "TapDetector",
        Report::StepCounter { .. } => "StepCounter",
        Report::StepDetector { .. } => "StepDetector",
        Report::SignificantMotion { .. } => "SignificantMotion",
        Report::StabilityClassifier { .. } => "StabilityClassifier",
        Report::MotionRequest { .. } => "MotionRequest",
        Report::ShakeDetector { .. } => "ShakeDetector",
        Report::PersonalActivityClassifier { .. } => "PersonalActivityClassifier",
        Report::RawSensor { .. } => "RawSensor",
        Report::GenericEvent { .. } => "GenericEvent",
        Report::GyroIntegratedRv { .. } => "GyroIntegratedRV",
        Report::FlushCompleted { .. } => "FlushCompleted",
        Report::Unknown { .. } => "UnknownReport",
    }
}

fn scale(raw: impl Into<i64>, q: u32) -> Value {
    Value::float(raw.into() as f64 / (1u64 << q) as f64)
}

/// `bno086`: the report in snake_case — type and sensor id, then only the fields the report
/// carries (scaled), the integer counts of the raw ADC streams, and the device timestamp.
pub fn bno086(r: &Report) -> Value {
    let sensor = r.sensor_id();
    let mut v = vec![
        ("type", Value::String(bno086_type(r).to_string())),
        ("sensor_id", int(sensor)),
    ];
    let acc = |a: u8| ("accuracy", int(a));
    match r {
        Report::Vector3 {
            accuracy,
            x_raw,
            y_raw,
            z_raw,
            ..
        } => {
            let q = q_point(sensor);
            v.extend([
                ("x", scale(*x_raw, q)),
                ("y", scale(*y_raw, q)),
                ("z", scale(*z_raw, q)),
                acc(*accuracy),
            ]);
        }
        Report::Vector3WithBias {
            accuracy,
            x_raw,
            y_raw,
            z_raw,
            bias_x_raw,
            bias_y_raw,
            bias_z_raw,
            ..
        } => {
            let q = q_point(sensor);
            v.extend([
                ("x", scale(*x_raw, q)),
                ("y", scale(*y_raw, q)),
                ("z", scale(*z_raw, q)),
                acc(*accuracy),
            ]);
            v.push((
                "bias",
                Value::Array(vec![
                    scale(*bias_x_raw, q),
                    scale(*bias_y_raw, q),
                    scale(*bias_z_raw, q),
                ]),
            ));
        }
        Report::RotationVector {
            accuracy,
            i_raw,
            j_raw,
            k_raw,
            real_raw,
            accuracy_raw,
            ..
        } => {
            v.extend([
                ("i", scale(*i_raw, 14)),
                ("j", scale(*j_raw, 14)),
                ("k", scale(*k_raw, 14)),
                ("real", scale(*real_raw, 14)),
            ]);
            if let Some(a) = accuracy_raw {
                v.push(("accuracy_rad", scale(*a, RV_ACCURACY_Q)));
            }
            v.push(acc(*accuracy));
        }
        Report::GyroIntegratedRv {
            i_raw,
            j_raw,
            k_raw,
            real_raw,
            vx_raw,
            vy_raw,
            vz_raw,
            ..
        } => {
            v.extend([
                ("i", scale(*i_raw, 14)),
                ("j", scale(*j_raw, 14)),
                ("k", scale(*k_raw, 14)),
                ("real", scale(*real_raw, 14)),
            ]);
            let w = GYRO_RV_ANGVEL_Q;
            v.push((
                "angular_velocity",
                Value::Array(vec![
                    scale(*vx_raw, w),
                    scale(*vy_raw, w),
                    scale(*vz_raw, w),
                ]),
            ));
        }
        Report::Scalar {
            accuracy,
            value_raw,
            ..
        } => {
            v.extend([
                acc(*accuracy),
                ("value", scale(*value_raw, q_point(sensor))),
            ]);
        }
        Report::TapDetector {
            accuracy, flags, ..
        } => {
            v.extend([
                acc(*accuracy),
                ("flags", int(*flags)),
                ("double_tap", Value::Bool(flags & 0x40 != 0)),
            ]);
        }
        Report::StepCounter {
            accuracy,
            latency_us,
            steps,
            ..
        } => {
            v.extend([
                acc(*accuracy),
                ("steps", int(*steps)),
                ("latency_us", int(*latency_us)),
            ]);
        }
        Report::StepDetector {
            accuracy,
            latency_us,
            ..
        } => v.extend([acc(*accuracy), ("latency_us", int(*latency_us))]),
        Report::SignificantMotion {
            accuracy, motion, ..
        } => v.extend([acc(*accuracy), ("motion", int(*motion))]),
        Report::StabilityClassifier {
            accuracy,
            classification,
            ..
        } => {
            v.extend([acc(*accuracy), ("classification", int(*classification))]);
        }
        Report::MotionRequest {
            accuracy,
            intent,
            request,
            ..
        } => {
            v.extend([
                acc(*accuracy),
                ("intent", int(*intent)),
                ("request", int(*request)),
            ]);
        }
        Report::ShakeDetector {
            accuracy, flags, ..
        } => v.extend([acc(*accuracy), ("flags", int(*flags))]),
        Report::PersonalActivityClassifier {
            accuracy,
            most_likely_state,
            confidences,
            ..
        } => {
            v.extend([
                acc(*accuracy),
                ("most_likely_state", int(*most_likely_state)),
                ("confidences", ints(confidences)),
            ]);
        }
        Report::RawSensor {
            accuracy,
            x_raw,
            y_raw,
            z_raw,
            sensor_timestamp_us,
            temperature_raw,
            ..
        } => {
            v.push(acc(*accuracy));
            // The raw ADC streams (0x14..0x16) keep their integer counts.
            if (0x14..=0x16).contains(&sensor) {
                v.extend([
                    ("x_raw", int(*x_raw)),
                    ("y_raw", int(*y_raw)),
                    ("z_raw", int(*z_raw)),
                    ("temperature_raw", int(*temperature_raw)),
                    ("sensor_timestamp_us", int(*sensor_timestamp_us)),
                ]);
            }
        }
        Report::GenericEvent {
            accuracy,
            value_raw,
            ..
        } => v.extend([acc(*accuracy), ("value_raw", int(*value_raw))]),
        Report::FlushCompleted { flushed_sensor, .. } => {
            v.push(("flushed_sensor", int(*flushed_sensor)))
        }
        Report::Unknown { .. } => {}
    }
    v.push(("timestamp_us", int(r.timestamp_us())));
    obj(v)
}

/// Python's `round(x, 1)`: the nearest tenth of the exact binary value, ties to even.
pub fn round1(x: f64) -> f64 {
    if x == 0.0 || !x.is_finite() {
        return x;
    }
    // `{:.1}` rounds the exact decimal expansion of `x`, ties to even, as Python does.
    format!("{x:.1}").parse().unwrap_or(x)
}

/// Standard base64 with padding.
pub(crate) fn base64(data: &[u8]) -> String {
    const A: &[u8; 64] = b"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    let mut out = String::with_capacity(data.len().div_ceil(3) * 4);
    for c in data.chunks(3) {
        let n = (c[0] as u32) << 16
            | (*c.get(1).unwrap_or(&0) as u32) << 8
            | *c.get(2).unwrap_or(&0) as u32;
        out.push(A[(n >> 18) as usize & 63] as char);
        out.push(A[(n >> 12) as usize & 63] as char);
        out.push(if c.len() > 1 {
            A[(n >> 6) as usize & 63] as char
        } else {
            '='
        });
        out.push(if c.len() > 2 {
            A[n as usize & 63] as char
        } else {
            '='
        });
    }
    out
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn round1_ties_to_even_on_the_exact_value() {
        assert_eq!(round1(0.25), 0.2);
        assert_eq!(round1(0.75), 0.8);
        assert_eq!(round1(1.25), 1.2);
        assert_eq!(round1(0.35), 0.3); // 0.35 is 0.34999… in binary
        assert_eq!(round1(-2.25), -2.2);
        assert_eq!(round1(1234.56), 1234.6);
    }

    #[test]
    fn base64_pads() {
        assert_eq!(base64(b""), "");
        assert_eq!(base64(b"f"), "Zg==");
        assert_eq!(base64(b"fo"), "Zm8=");
        assert_eq!(base64(b"foo"), "Zm9v");
        assert_eq!(base64(b"foobar"), "Zm9vYmFy");
    }
}
