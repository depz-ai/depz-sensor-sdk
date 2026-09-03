//! Target 4 — BNO086 SH-2 report parsers (Q-points + 0xFB timebase) vs
//! `contracts/vectors/bno086_reports.json`.

use std::path::PathBuf;

use depz_sensor_sdk::bno086::reports::{
    parse_gyro_rv_cargo, parse_input_cargo, Report, Vector3Kind,
};
use serde_json::{json, Map, Value};

fn vectors_dir() -> PathBuf {
    PathBuf::from(env!("CARGO_MANIFEST_DIR"))
        .join("..")
        .join("..")
        .join("contracts")
        .join("vectors")
}

fn hex_decode(s: &str) -> Vec<u8> {
    (0..s.len())
        .step_by(2)
        .map(|i| u8::from_str_radix(&s[i..i + 2], 16).unwrap())
        .collect()
}

fn hex_encode(b: &[u8]) -> String {
    let mut s = String::with_capacity(b.len() * 2);
    for x in b {
        s.push_str(&format!("{:02x}", x));
    }
    s
}

/// Report → (type string, field map) with the vector's field naming.
fn report_repr(r: &Report) -> (&'static str, Map<String, Value>) {
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

/// Assert every field in `want` is present and equal in the parsed report.
fn assert_fields(got: &Report, ty: &str, want: &Value, ctx: &str) {
    let (got_ty, got_map) = report_repr(got);
    assert_eq!(got_ty, ty, "type {}", ctx);
    for (k, v) in want.as_object().unwrap() {
        assert_eq!(
            got_map.get(k),
            Some(v),
            "field {} in {} (got map = {:?})",
            k,
            ctx,
            got_map
        );
    }
}

#[test]
fn bno086_reports_vectors() {
    let doc: Value = serde_json::from_str(
        &std::fs::read_to_string(vectors_dir().join("bno086_reports.json")).unwrap(),
    )
    .unwrap();

    // ── channel-3/4 input cargos ─────────────────────────────────────────────
    let mut cargos = 0;
    for c in doc["input_cargos"].as_array().unwrap() {
        let payload = hex_decode(c["cargo"].as_str().unwrap());
        let capture = c["capture_timestamp_us"].as_i64().unwrap();
        let reports = parse_input_cargo(&payload, capture);
        let expect = c["expect"].as_array().unwrap();
        assert_eq!(reports.len(), expect.len(), "report count {}", c["name"]);
        for (i, (got, want)) in reports.iter().zip(expect).enumerate() {
            assert_fields(
                got,
                want["type"].as_str().unwrap(),
                &want["fields"],
                &format!("{}[{}]", c["name"], i),
            );
        }
        cargos += 1;
    }

    // ── channel-5 gyro-integrated RV (dense) ─────────────────────────────────
    let mut gyro = 0;
    for c in doc["gyro_rv"].as_array().unwrap() {
        let payload = hex_decode(c["cargo"].as_str().unwrap());
        let capture = c["capture_timestamp_us"].as_i64().unwrap();
        let report = parse_gyro_rv_cargo(&payload, capture).expect("gyro rv parses");
        let expect = &c["expect"];
        assert_fields(
            &report,
            expect["type"].as_str().unwrap(),
            &expect["fields"],
            c["name"].as_str().unwrap(),
        );
        gyro += 1;
    }

    assert!(cargos > 0 && gyro > 0);
    eprintln!("bno086_reports.json: {} input cargos + {} gyro_rv", cargos, gyro);
}
