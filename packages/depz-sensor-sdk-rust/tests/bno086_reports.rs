//! Target 4 — BNO086 SH-2 report parsers (Q-points + 0xFB timebase) vs
//! `contracts/vectors/bno086_reports.json`.

mod common;

use depz_sensor_sdk::bno086::reports::{
    parse_gyro_rv_cargo, parse_input_cargo, q_point, Report, GYRO_RV_ANGVEL_Q,
    RV_ACCURACY_Q,
};
use serde_json::Value;

use common::shapes::report_repr;
use common::{hex_decode, load, recorded_text, recording_packets, seen};

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
    let doc = load("bno086_reports.json");

    // ── channel-3/4 input cargos (+ interactive_and_flush: MotionRequest 0x2B
    // and the bare 0xEF Flush Completed, same shape) ──────────────────────────
    let mut cargos = 0;
    let groups = [&doc["input_cargos"], &doc["interactive_and_flush"]];
    for c in groups.iter().flat_map(|g| g.as_array().unwrap()) {
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
        seen(c);
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
        seen(c);
        gyro += 1;
    }

    assert!(cargos > 0 && gyro > 0);
    eprintln!("bno086_reports.json: {} input cargos + {} gyro_rv", cargos, gyro);
}

#[test]
fn bno086_q_point_vectors() {
    let doc = load("bno086_reports.json");
    // The primary-field Q table, both ways: every listed sensor at its Q, and
    // no sensor outside the table with a non-zero Q.
    let table = doc["q_points"].as_object().unwrap();
    for (k, v) in table {
        let id = u8::from_str_radix(k.trim_start_matches("0x"), 16).unwrap();
        assert_eq!(q_point(id) as u64, v.as_u64().unwrap(), "q_point {}", k);
    }
    for id in 0..=255u8 {
        if q_point(id) != 0 {
            assert!(table.contains_key(&format!("0x{:02X}", id)), "q_point 0x{:02X} not in vector", id);
        }
    }
    seen("bno086_reports/q_points");
    assert_eq!(doc["rv_accuracy_q"].as_u64().unwrap(), RV_ACCURACY_Q as u64);
    seen("bno086_reports/rv_accuracy_q");
    assert_eq!(doc["gyro_rv_angvel_q"].as_u64().unwrap(), GYRO_RV_ANGVEL_Q as u64);
    seen("bno086_reports/gyro_rv_angvel_q");
}

// ---- recordings/bno086_session.depzrec -------------------------------------

/// The live capture decoded rx-side only (no device layer to re-issue the tx
/// half): every RPT_DATA (0x91: cmd u8, capture timestamp_us u64 LE, raw SHTP
/// frame) through the SHTP layer, input-channel cargos (3 normal, 4 wake) and
/// gyro-RV cargos (5) through the SH-2 parsers, vs the sidecar's reports
/// field by field.
#[test]
fn bno086_session_recording() {
    use depz_sensor_sdk::bno086::shtp::{ShtpChannel, ShtpLayer};

    let exp = load("recordings/bno086_session.expected.json");
    let mut layer = ShtpLayer::new();
    let mut got = Vec::new();
    for p in recording_packets("bno086_session.depzrec") {
        if !p.rx || p.cmd != 0x91 || p.payload.len() < 9 {
            continue;
        }
        let capture = u64::from_le_bytes(p.payload[1..9].try_into().unwrap());
        let Some(cargo) = layer.feed_at(&p.payload[9..], capture) else { continue };
        match cargo.channel {
            c if c == ShtpChannel::InputNormal as u8 || c == ShtpChannel::InputWake as u8 => {
                got.extend(parse_input_cargo(&cargo.payload, cargo.capture_us as i64))
            }
            c if c == ShtpChannel::GyroRv as u8 => {
                got.extend(parse_gyro_rv_cargo(&cargo.payload, cargo.capture_us as i64))
            }
            _ => {}
        }
    }
    let want = exp["reports"].as_array().unwrap();
    assert!(got.len() >= want.len(), "decoded {} reports, need >= {}", got.len(), want.len());
    for (i, (g, w)) in got.iter().zip(want).enumerate() {
        let mut fields = w.as_object().unwrap().clone();
        let ty = fields.remove("type").unwrap();
        let (got_ty, got_map) = report_repr(g);
        assert_eq!(got_ty, ty.as_str().unwrap(), "report {} type", i);
        assert_eq!(got_map, fields, "report {}", i);
    }
    seen("recordings/bno086_session.depzrec");
}

// ---- recordings/bno086_canonical.depzrec -----------------------------------

/// The canonical capture (contract 15 `bno086_canonical`, opened the §4.4 way)
/// decoded rx-side only, like `bno086_session`: its sidecar's `results` hold
/// the probe answers (software name first, then device name) and, at the
/// scenario's `collect` step, the 50 reports the subscription saw — compared
/// field by field against the first 50 SH-2 reports of the rx stream.
#[test]
fn bno086_canonical_recording() {
    use depz_sensor_sdk::bno086::shtp::{ShtpChannel, ShtpLayer};
    use depz_sensor_sdk::protocol::common::Cmd;
    use depz_sensor_sdk::{resolve_sensor_class, usb_model_hint};

    let exp = load("recordings/bno086_canonical.expected.json");
    let pkts = recording_packets("bno086_canonical.depzrec");
    // The probe (contract 02 §4.4): its two text answers, then the class.
    let text = |cmd| {
        recorded_text(&pkts, cmd).map(|t| t.trim_end_matches('\0').trim().to_string())
    };
    let sw = text(Cmd::GetNameActiveSoftware).expect("GET_NAME_ACTIVE_SOFTWARE answer");
    let name = text(Cmd::GetDeviceName).unwrap_or_default();
    assert_eq!(sw, exp["software_name"].as_str().unwrap());
    assert_eq!(name, exp["device_name"].as_str().unwrap());
    let vid = exp["usb_vid"].as_u64().map(|v| v as u16);
    let pid = exp["usb_pid"].as_u64().map(|v| v as u16);
    assert_eq!(usb_model_hint(vid, pid), exp["usb_model"].as_str());
    assert_eq!(resolve_sensor_class(&sw, &name, vid, pid), exp["class"].as_str().unwrap());

    let mut layer = ShtpLayer::new();
    let mut got = Vec::new();
    for p in pkts {
        if !p.rx || p.cmd != 0x91 || p.payload.len() < 9 {
            continue;
        }
        let capture = u64::from_le_bytes(p.payload[1..9].try_into().unwrap());
        let Some(cargo) = layer.feed_at(&p.payload[9..], capture) else { continue };
        match cargo.channel {
            c if c == ShtpChannel::InputNormal as u8 || c == ShtpChannel::InputWake as u8 => {
                got.extend(parse_input_cargo(&cargo.payload, cargo.capture_us as i64))
            }
            c if c == ShtpChannel::GyroRv as u8 => {
                got.extend(parse_gyro_rv_cargo(&cargo.payload, cargo.capture_us as i64))
            }
            _ => {}
        }
    }
    let want = exp["results"]
        .as_array()
        .unwrap()
        .iter()
        .find_map(|r| r.as_array().filter(|a| !a.is_empty()))
        .expect("a non-empty reports result");
    assert!(got.len() >= want.len(), "decoded {} reports, need >= {}", got.len(), want.len());
    for (i, (g, w)) in got.iter().zip(want).enumerate() {
        let (_, got_map) = report_repr(g);
        assert_eq!(&got_map, w.as_object().unwrap(), "report {}", i);
    }
    seen("recordings/bno086_canonical.depzrec");
}
