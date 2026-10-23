//! BNO055 golden-vector conformance tests vs `contracts/vectors/bno055.json`
//! (contract 13): encode (0x32..0x37), report decode (RPT_BNO_INFO,
//! REG_DATA, REG_STREAM), UNIT_SEL flags, CALIB_STAT, the calibration
//! profile, axis remap + placements (and the refused non-permutations), the
//! page-1 sensor configs, and register-window block decode.

mod common;

use depz_sensor_sdk::bno055::{
    decode_block, pack_get_info, pack_read_reg, pack_reset, pack_start_stream, pack_stop_stream,
    pack_write_reg, AccelConfig, AxisRemap, Bno055Info, CalibStatus, CalibrationProfile,
    GyroConfig, MagConfig, OprMode, RegData, StreamData, Units, CALIB_PROFILE_LEN, FULL_BLOCK,
    INFO_SIZE, PLACEMENTS, QUAT_BLOCK,
};
use depz_sensor_sdk::protocol::identity::{self, SensorType};
use depz_sensor_sdk::usb_ids::DEPZ_USB_VID;
use depz_sensor_sdk::usb_model_hint;
use serde_json::Value;

use common::shapes::{block_fields, info_fields};
use common::{hex_decode, hex_encode, load, recorded_text, recording_packets, reg_reads, seen};

fn u(v: &Value) -> u64 {
    v.as_u64().unwrap_or_else(|| panic!("not an unsigned int: {}", v))
}

fn b(v: &Value) -> bool {
    v.as_bool().unwrap_or_else(|| panic!("not a bool: {}", v))
}

/// A JSON int array (or null) → `Option<Vec<i64>>`.
fn ints(v: &Value) -> Option<Vec<i64>> {
    v.as_array().map(|a| a.iter().map(|x| x.as_i64().unwrap()).collect())
}

fn i16s(v: &[i16]) -> Vec<i64> {
    v.iter().map(|&x| x as i64).collect()
}

// ---- encode ----------------------------------------------------------------

#[test]
fn bno055_encode_vectors() {
    let doc = load("bno055.json");
    let mut n = 0;
    for c in doc["encode"].as_array().unwrap() {
        let name = c["name"].as_str().unwrap();
        let got: Vec<u8> = match c["kind"].as_str().unwrap() {
            "read_reg" => pack_read_reg(u(&c["addr"]) as u8, u(&c["len"]) as u8).to_vec(),
            "write_reg" => {
                pack_write_reg(u(&c["addr"]) as u8, &hex_decode(c["data"].as_str().unwrap()))
            }
            "reset" => pack_reset().to_vec(),
            "stop_stream" => pack_stop_stream().to_vec(),
            "get_info" => pack_get_info().to_vec(),
            "start_stream" => pack_start_stream(
                u(&c["trigger"]) as u8,
                u(&c["addr"]) as u8,
                u(&c["len"]) as u8,
                u(&c["period_ms"]) as u16,
            )
            .to_vec(),
            k => panic!("{}: unknown encode kind {:?}", name, k),
        };
        assert_eq!(hex_encode(&got), c["payload"].as_str().unwrap(), "{}", name);
        seen(c);
        n += 1;
    }
    assert_eq!(n, 15);
}

// ---- decode ----------------------------------------------------------------

#[test]
fn bno055_decode_vectors() {
    let doc = load("bno055.json");
    let (mut info, mut reg, mut stream) = (0, 0, 0);
    for c in doc["decode"].as_array().unwrap() {
        let name = c["name"].as_str().unwrap();
        let raw = hex_decode(c["payload"].as_str().unwrap());
        let e = &c["expect"];
        match u(&c["report"]) {
            0x92 => {
                let i = Bno055Info::unpack(&raw).unwrap();
                let got = info_fields(&i);
                assert_eq!(e.as_object().unwrap().len(), got.len(), "{}", name);
                for (k, v) in got {
                    assert_eq!(v, u(&e[k]), "{}.{}", name, k);
                }
                assert!(i.ids_ok(), "{}", name);
                assert_eq!(i.sw_rev_text(), "03.11", "{}", name);
                seen(c);
                info += 1;
            }
            0x91 => {
                let r = RegData::unpack(&raw).unwrap();
                assert_eq!(r.cmd as u64, u(&e["cmd"]), "{}", name);
                assert_eq!(r.timestamp_us, u(&e["timestamp_us"]), "{}", name);
                assert_eq!(hex_encode(&r.data), e["data"].as_str().unwrap(), "{}", name);
                seen(c);
                reg += 1;
            }
            0x93 => {
                let s = StreamData::unpack(&raw).unwrap();
                assert_eq!(s.timestamp_us, u(&e["timestamp_us"]), "{}", name);
                assert_eq!(s.addr as u64, u(&e["addr"]), "{}", name);
                assert_eq!(s.len as u64, u(&e["len"]), "{}", name);
                assert_eq!(hex_encode(&s.data), e["data"].as_str().unwrap(), "{}", name);
                seen(c);
                stream += 1;
            }
            r => panic!("{}: unknown report {:#x}", name, r),
        }
    }
    assert_eq!((info, reg, stream), (3, 1, 2));
    assert!(Bno055Info::unpack(&[0u8; INFO_SIZE - 1]).is_err());
    assert!(RegData::unpack(&[0u8; 8]).is_err());
    assert!(StreamData::unpack(&[0u8; 9]).is_err());
    // A stream report shorter than its own len field is refused.
    let mut short = vec![0u8; 10];
    short[9] = 4;
    short.extend_from_slice(&[1, 2, 3]);
    assert!(StreamData::unpack(&short).is_err());
}

// ---- units -----------------------------------------------------------------

#[test]
fn bno055_units_vectors() {
    let doc = load("bno055.json");
    let mut n = 0;
    for c in doc["units"].as_array().unwrap() {
        let value = u(&c["unit_sel"]) as u8;
        let s = Units::unpack(value);
        let e = &c["expect"];
        let got = [
            ("accel_mg", s.accel_mg),
            ("gyro_rps", s.gyro_rps),
            ("euler_rad", s.euler_rad),
            ("temp_f", s.temp_f),
            ("android", s.android),
        ];
        assert_eq!(e.as_object().unwrap().len(), got.len(), "unit_sel {:#x}", value);
        for (k, v) in got {
            assert_eq!(v, b(&e[k]), "unit_sel {:#x}.{}", value, k);
        }
        assert_eq!(s.pack() as u64, u(&c["repack"]), "unit_sel {:#x} repack", value);
        seen(c);
        n += 1;
    }
    assert_eq!(n, 8);
    // §4.2 scales.
    let si = Units::default();
    assert_eq!(si.pack(), 0x00);
    assert_eq!((si.accel_lsb(), si.gyro_lsb(), si.euler_lsb(), si.temp_lsb()), (100.0, 16.0, 16.0, 1.0));
    let alt = Units::unpack(0x97);
    assert_eq!((alt.accel_lsb(), alt.gyro_lsb(), alt.euler_lsb(), alt.temp_lsb()), (1.0, 900.0, 900.0, 0.5));
    use depz_sensor_sdk::bno055::{FUSION_ACCEL_LSB, MAG_LSB, QUAT_LSB};
    assert_eq!((MAG_LSB, QUAT_LSB, FUSION_ACCEL_LSB), (16.0, 16384.0, 100.0));
}

// ---- calib_stat ------------------------------------------------------------

#[test]
fn bno055_calib_stat_vectors() {
    let doc = load("bno055.json");
    let mut n = 0;
    for c in doc["calib_stat"].as_array().unwrap() {
        let value = u(&c["value"]) as u8;
        let s = CalibStatus::unpack(value);
        let e = &c["expect"];
        let got = [("system", s.system), ("gyro", s.gyro), ("accel", s.accel), ("mag", s.mag)];
        assert_eq!(e.as_object().unwrap().len(), got.len(), "{:#x}", value);
        for (k, v) in got {
            assert_eq!(v as u64, u(&e[k]), "{:#x}.{}", value, k);
        }
        assert_eq!(s.fully_calibrated(), b(&c["fully_calibrated"]), "{:#x}", value);
        assert_eq!(s.pack(), value, "{:#x} repack", value);
        seen(c);
        n += 1;
    }
    assert_eq!(n, 5);
}

// ---- calibration_profile ---------------------------------------------------

#[test]
fn bno055_calibration_profile_vectors() {
    let doc = load("bno055.json");
    let mut n = 0;
    for c in doc["calibration_profile"].as_array().unwrap() {
        let name = c["name"].as_str().unwrap();
        let bytes = hex_decode(c["bytes"].as_str().unwrap());
        let p = CalibrationProfile::unpack(&bytes).unwrap();
        let e = &c["expect"];
        assert_eq!(e.as_object().unwrap().len(), 5, "{}", name);
        assert_eq!(Some(i16s(&p.accel_offset)), ints(&e["accel_offset"]), "{}", name);
        assert_eq!(Some(i16s(&p.mag_offset)), ints(&e["mag_offset"]), "{}", name);
        assert_eq!(Some(i16s(&p.gyro_offset)), ints(&e["gyro_offset"]), "{}", name);
        assert_eq!(p.accel_radius as i64, e["accel_radius"].as_i64().unwrap(), "{}", name);
        assert_eq!(p.mag_radius as i64, e["mag_radius"].as_i64().unwrap(), "{}", name);
        assert_eq!(hex_encode(&p.pack()), c["bytes"].as_str().unwrap(), "{} repack", name);
        seen(c);
        n += 1;
    }
    assert_eq!(n, 3);
    assert!(CalibrationProfile::unpack(&[0u8; CALIB_PROFILE_LEN - 1]).is_err());
    assert!(CalibrationProfile::unpack(&[0u8; CALIB_PROFILE_LEN + 1]).is_err());
}

// ---- axis_remap / axis_remap_invalid ---------------------------------------

#[test]
fn bno055_axis_remap_vectors() {
    let doc = load("bno055.json");
    let rows = doc["axis_remap"].as_array().unwrap();
    let names: Vec<&str> = rows.iter().map(|r| r["name"].as_str().unwrap()).collect();
    let ours: Vec<&str> = PLACEMENTS.iter().map(|p| p.0).collect();
    assert_eq!(names, ours);
    for c in rows {
        let name = c["name"].as_str().unwrap();
        let a = AxisRemap::unpack(u(&c["config"]) as u8, u(&c["sign"]) as u8);
        let e = &c["expect"];
        assert_eq!(e.as_object().unwrap().len(), 6, "{}", name);
        assert_eq!((a.x as u64, a.y as u64, a.z as u64), (u(&e["x"]), u(&e["y"]), u(&e["z"])), "{}", name);
        assert_eq!(
            (a.x_negative, a.y_negative, a.z_negative),
            (b(&e["x_negative"]), b(&e["y_negative"]), b(&e["z_negative"])),
            "{}",
            name
        );
        let (config, sign) = a.pack().unwrap();
        let repack: Vec<u64> = c["repack"].as_array().unwrap().iter().map(u).collect();
        assert_eq!(vec![config as u64, sign as u64], repack, "{} repack", name);
        assert_eq!(AxisRemap::placement(name), Some(a), "{} placement", name);
        assert_eq!(AxisRemap::placement(&name.to_ascii_lowercase()), Some(a), "{}", name);
        seen(c);
    }
    assert_eq!(AxisRemap::placement("P1"), Some(AxisRemap::default()));
    assert_eq!(AxisRemap::placement("P8"), None);
}

#[test]
fn bno055_axis_remap_invalid_vectors() {
    let doc = load("bno055.json");
    let mut n = 0;
    for c in doc["axis_remap_invalid"].as_array().unwrap() {
        let name = c["name"].as_str().unwrap();
        let a = AxisRemap {
            x: u(&c["x"]) as u8,
            y: u(&c["y"]) as u8,
            z: u(&c["z"]) as u8,
            ..AxisRemap::default()
        };
        assert!(a.pack().is_err(), "{} must be refused", name);
        seen(c);
        n += 1;
    }
    assert_eq!(n, 2);
    // An out-of-range axis code is refused too.
    assert!(AxisRemap { x: 3, ..AxisRemap::default() }.pack().is_err());
}

// ---- sensor_config ---------------------------------------------------------

#[test]
fn bno055_sensor_config_vectors() {
    let doc = load("bno055.json");
    let sc = &doc["sensor_config"];
    let mut n = 0;
    for c in sc["accel"].as_array().unwrap() {
        let value = u(&c["value"]) as u8;
        let e = &c["expect"];
        let want = AccelConfig {
            range: u(&e["range"]) as u8,
            bandwidth: u(&e["bandwidth"]) as u8,
            power: u(&e["power"]) as u8,
        };
        assert_eq!(e.as_object().unwrap().len(), 3);
        assert_eq!(AccelConfig::unpack(value), want, "accel {:#x}", value);
        assert_eq!(want.pack(), value, "accel {:#x} repack", value);
        n += 1;
    }
    for c in sc["gyro"].as_array().unwrap() {
        let hex = c["bytes"].as_str().unwrap();
        let e = &c["expect"];
        let want = GyroConfig {
            range: u(&e["range"]) as u8,
            bandwidth: u(&e["bandwidth"]) as u8,
            power: u(&e["power"]) as u8,
        };
        assert_eq!(e.as_object().unwrap().len(), 3);
        assert_eq!(GyroConfig::unpack(&hex_decode(hex)).unwrap(), want, "gyro {}", hex);
        assert_eq!(hex_encode(&want.pack()), hex, "gyro {} repack", hex);
        n += 1;
    }
    for c in sc["mag"].as_array().unwrap() {
        let value = u(&c["value"]) as u8;
        let e = &c["expect"];
        let want = MagConfig {
            rate: u(&e["rate"]) as u8,
            mode: u(&e["mode"]) as u8,
            power: u(&e["power"]) as u8,
        };
        assert_eq!(e.as_object().unwrap().len(), 3);
        assert_eq!(MagConfig::unpack(value), want, "mag {:#x}", value);
        // Bit 7 is unused: a repack clears it.
        assert_eq!(want.pack(), value & 0x7F, "mag {:#x} repack", value);
        n += 1;
    }
    assert_eq!(n, 11);
    assert!(GyroConfig::unpack(&[0x38]).is_err());
    // Power-on values.
    assert_eq!(AccelConfig::default().pack(), 0x0D);
    assert_eq!(GyroConfig::default().pack(), [0x38, 0x00]);
    assert_eq!(MagConfig::default().pack(), 0x0B);
    seen("bno055/sensor_config");
}

// ---- blocks ----------------------------------------------------------------

#[test]
fn bno055_block_vectors() {
    let doc = load("bno055.json");
    let mut n = 0;
    for c in doc["blocks"].as_array().unwrap() {
        let name = c["name"].as_str().unwrap();
        let r = decode_block(u(&c["addr"]) as u8, &hex_decode(c["data"].as_str().unwrap()));
        let e = &c["expect"];
        let got = block_fields(&r);
        assert_eq!(e.as_object().unwrap().len(), got.len() + 2, "{}", name);
        for (k, v) in got {
            assert_eq!(v, ints(&e[k]), "{}.{}", name, k);
        }
        assert_eq!(r.temperature.map(|t| t as i64), e["temperature"].as_i64(), "{}.temperature", name);
        assert_eq!(r.calib_stat.map(|s| s as u64), e["calib_stat"].as_u64(), "{}.calib_stat", name);
        seen(c);
        n += 1;
    }
    assert_eq!(n, 7);
    // The two canonical windows.
    assert_eq!(FULL_BLOCK, (0x08, 46));
    assert_eq!(QUAT_BLOCK, (0x20, 8));
    let full = decode_block(FULL_BLOCK.0, &[0u8; 46]);
    assert!(full.accel.is_some() && full.calib_stat.is_some());
    let quat = decode_block(QUAT_BLOCK.0, &[0u8; 8]);
    assert!(quat.quaternion.is_some() && quat.linear_accel.is_none() && quat.accel.is_none());
    // A window that stops one byte short of a channel leaves it out.
    let cut = decode_block(0x2E, &[0u8; 5]);
    assert!(cut.gravity.is_none());
}

// ---- modes, identity.json, usb id ------------------------------------------

#[test]
fn bno055_opr_mode() {
    assert_eq!(OprMode::from_reg(0x1C), Some(OprMode::Ndof)); // reads back with bit 4 set
    assert_eq!(OprMode::from_reg(0x00), Some(OprMode::Config));
    assert_eq!(OprMode::from_reg(0x0D), None);
    assert!(OprMode::Imu.is_fusion() && OprMode::Ndof.is_fusion());
    assert!(!OprMode::Amg.is_fusion() && !OprMode::Config.is_fusion());
}

#[test]
fn bno055_identity() {
    let id = identity::parse_software_name("APP_BNO055_v0.12");
    assert_eq!(id.sensor_type, Some(SensorType::Bno055));
    assert_eq!(id.sensor_type.unwrap().as_str(), "bno055");
    assert_eq!(id.version, "0.12");
    // The BNO085 boards keep their own class.
    assert_eq!(
        identity::parse_software_name("APP_BNO086_v1.00").sensor_type,
        Some(SensorType::Bno086)
    );
    assert_eq!(usb_model_hint(Some(DEPZ_USB_VID), Some(0xEE0A)), Some("bno055"));
}

// ---- recordings/bno055_*.depzrec -------------------------------------------

/// Every rx report of a live capture through framing + the BNO055 decoders,
/// vs the `.expected.json` sidecar (no device layer: not a strict tx replay).
/// Register read-backs are paired with the READ_REG that asked for them.
fn bno055_recording(stem: &str) {
    use depz_sensor_sdk::bno055::regs::{REG_CALIB_PROFILE, REG_ST_RESULT, REG_SYS_CLK_STATUS};
    use depz_sensor_sdk::protocol::common::Cmd;

    let exp = load(&format!("recordings/{}.expected.json", stem));
    let pkts = recording_packets(&format!("{}.depzrec", stem));
    let sw = recorded_text(&pkts, Cmd::GetNameActiveSoftware).expect("software name");
    assert_eq!(identity::parse_software_name(&sw).sensor_type, Some(SensorType::Bno055));
    assert_eq!(sw, exp["software_name"].as_str().unwrap(), "{}", stem);
    assert_eq!(recorded_text(&pkts, Cmd::GetDeviceName).as_deref(), exp["device_name"].as_str());

    let mut info = None;
    let mut frames = Vec::new();
    for p in pkts.iter().filter(|p| p.rx) {
        match p.cmd {
            0x92 if info.is_none() => info = Some(Bno055Info::unpack(&p.payload).unwrap()),
            0x93 => frames.push(StreamData::unpack(&p.payload).unwrap()),
            _ => {}
        }
    }
    let want_info = &exp["info"];
    let got_info = info_fields(&info.expect("RPT_BNO_INFO"));
    assert_eq!(want_info.as_object().unwrap().len(), got_info.len(), "{} info", stem);
    for (k, v) in got_info {
        assert_eq!(v, u(&want_info[k]), "{} info.{}", stem, k);
    }

    // Read-backs: the last read of each (addr, len) is what the getters saw.
    let mut last = std::collections::HashMap::new();
    for r in reg_reads(&pkts, false) {
        let d = RegData::unpack(&r.reply).unwrap();
        assert_eq!(d.data.len(), r.len as usize, "{} read {:#x}", stem, r.addr);
        last.insert((r.addr as u8, r.len as usize), d.data);
    }
    let prof = CalibrationProfile::unpack(&last[&(REG_CALIB_PROFILE, CALIB_PROFILE_LEN)]).unwrap();
    let wp = &exp["calibration_profile"];
    assert_eq!(wp.as_object().unwrap().len(), 5, "{}", stem);
    assert_eq!(Some(i16s(&prof.accel_offset)), ints(&wp["accel_offset"]), "{}", stem);
    assert_eq!(Some(i16s(&prof.mag_offset)), ints(&wp["mag_offset"]), "{}", stem);
    assert_eq!(Some(i16s(&prof.gyro_offset)), ints(&wp["gyro_offset"]), "{}", stem);
    assert_eq!(prof.accel_radius as i64, wp["accel_radius"].as_i64().unwrap(), "{}", stem);
    assert_eq!(prof.mag_radius as i64, wp["mag_radius"].as_i64().unwrap(), "{}", stem);
    // ST_RESULT, then SYS_CLK_STATUS..SYS_ERR.
    let st = &last[&(REG_ST_RESULT, 1)];
    let sys = &last[&(REG_SYS_CLK_STATUS, 3)];
    let ws = &exp["system_status"];
    assert_eq!(ws.as_object().unwrap().len(), 4, "{}", stem);
    assert_eq!(
        [st[0], sys[0], sys[1], sys[2]].map(u64::from).to_vec(),
        ["self_test", "clk_status", "status", "error"].map(|k| u(&ws[k])).to_vec(),
        "{} system_status",
        stem
    );

    let block: Vec<u64> = exp["block"].as_array().unwrap().iter().map(u).collect();
    let want = exp["frames"].as_array().unwrap();
    // The capture may stream past the sidecar; match the first N.
    assert!(frames.len() >= want.len(), "{}: {} < {} frames", stem, frames.len(), want.len());
    for (idx, (s, w)) in frames.iter().zip(want).enumerate() {
        let ctx = format!("{} frame {}", stem, idx);
        assert_eq!(s.timestamp_us, u(&w["timestamp_us"]), "{}", ctx);
        assert_eq!(vec![s.addr as u64, s.len as u64], block, "{}", ctx);
        assert_eq!(s.addr as u64, u(&w["addr"]), "{}", ctx);
        assert_eq!(hex_encode(&s.data), w["raw"].as_str().unwrap(), "{}", ctx);
        let r = decode_block(s.addr, &s.data);
        let d = &w["decoded"];
        assert_eq!(d.as_object().unwrap().len(), 9, "{}", ctx);
        for (k, v) in block_fields(&r) {
            assert_eq!(v, ints(&d[k]), "{}.{}", ctx, k);
        }
        assert_eq!(r.temperature.map(|t| t as i64), d["temperature"].as_i64(), "{}", ctx);
        assert_eq!(r.calib_stat.map(|c| c as u64), d["calib_stat"].as_u64(), "{}", ctx);
        // Streamed at the requested period on the MCU clock.
        if idx > 0 {
            let dt = s.timestamp_us as i64 - frames[idx - 1].timestamp_us as i64;
            assert!((dt - 1000 * exp["period_ms"].as_i64().unwrap()).abs() < 500, "{} dt {}", ctx, dt);
        }
    }
    seen(&format!("recordings/{}.depzrec", stem));
}

#[test]
fn bno055_imu_quat_recording() {
    bno055_recording("bno055_imu_quat_units_50hz");
}

#[test]
fn bno055_ndof_full_recording() {
    bno055_recording("bno055_ndof_full_100hz");
}
