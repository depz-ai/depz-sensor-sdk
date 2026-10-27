//! The VL53L8CX / VL53L8CH device classes over the wire (contract 04) against the in-process
//! bridge fake (tests/common/fake_vl53l8.rs, the port of the Python `tests/fake_vl53l8.py`):
//! init (firmware download, checksum, NVM / xtalk / configuration uploads), the configuration
//! setters and getters as DCI round trips, the not-while-ranging guard, start / stop of the
//! stream, 4×4 and 8×8 frames with every layer, the motion and CNH blocks, the reassembly rules
//! through the device, poll mode, xtalk calibration save / load and the power modes. The cases
//! and expected values are those of the Python `tests/test_vl53l8_device.py`.
mod common;

use std::sync::{Arc, Mutex};
use std::time::Duration;

use common::fake_board::until;
use common::fake_vl53l8::*;
use depz_sensor_sdk::devices::{DepzDevice, DepzError, Vl53l8chDevice, Vl53l8cxDevice};
use depz_sensor_sdk::vl53l8::advanced::{
    DetectionThreshold, DIST_MM, NB_THRESHOLDS, POWER_MODE_DEEP_SLEEP, POWER_MODE_SLEEP,
    POWER_MODE_WAKEUP, SIGNAL_PER_SPAD_KCPS,
};
use depz_sensor_sdk::vl53l8::driver::{
    tables, Vl53l8CnhSetup, Vl53l8Firmware, Vl53l8Frame, RANGING_MODE_AUTONOMOUS,
    RANGING_MODE_CONTINUOUS, TARGET_ORDER_CLOSEST, TARGET_ORDER_STRONGEST,
};
use depz_sensor_sdk::vl53l8::{decode_cnh, Vl53l8Cmd, RESOLUTION_4X4, RESOLUTION_8X8};

// Frame sizes the lab boards streamed (START_STREAM frame_size in the captures).
const CX_8X8_FRAME: usize = 1452;
const CH_8X8_FRAME: usize = 1444;
const CH_8X8_CNH_16X20_FRAME: usize = 3156;
const CX_4X4_FRAME: usize = 540;
const CH_4X4_FRAME: usize = 532;

const T: Duration = Duration::from_secs(2);

fn dev(f: &FakeVl53l8) -> DepzDevice {
    DepzDevice::new(f.link(), Some(Duration::from_millis(500)))
}

fn cx() -> (FakeVl53l8, Vl53l8cxDevice) {
    let f = FakeVl53l8::new();
    let d = Vl53l8cxDevice::attach(dev(&f));
    (f, d)
}

fn ch() -> (FakeVl53l8, Vl53l8chDevice) {
    let f = FakeVl53l8::new();
    let d = Vl53l8chDevice::attach(dev(&f));
    (f, d)
}

/// (WRITE_REG, READ_REG) the sensor has seen.
fn bus_ops(f: &FakeVl53l8) -> (usize, usize) {
    let s = f.lock();
    (s.sensor.writes.len(), s.sensor.reads.len())
}

fn cnh_16x20() -> Vl53l8CnhSetup {
    let mut cfg = Vl53l8CnhSetup::new();
    cfg.init_config(10, 20, 2).unwrap();
    cfg.create_agg_map(64, 0, 0, 2, 2, 4, 4).unwrap();
    cfg
}

/// A ULD status error (Python `Vl53l8cxError`), with its code when given.
#[track_caller]
fn assert_uld_status<T: std::fmt::Debug>(r: Result<T, DepzError>, code: Option<u8>) {
    match r {
        Err(DepzError::Sensor(m)) => {
            assert!(m.starts_with("VL53L8CX "), "{m}");
            if let Some(c) = code {
                assert!(m.contains(&format!("(code {c})")), "{m}");
            }
        }
        other => panic!("expected a ULD status error, got {other:?}"),
    }
}

#[track_caller]
fn assert_invalid_argument<T: std::fmt::Debug>(r: Result<T, DepzError>, needle: &str) {
    match r {
        Err(DepzError::InvalidArgument(m)) => assert!(m.contains(needle), "{m}"),
        other => panic!("expected InvalidArgument, got {other:?}"),
    }
}

#[track_caller]
fn assert_frame(frame: &Vl53l8Frame, expected: &Vl53l8Frame) {
    // Every contract 04 §5 field, in its unit, against the scene the fake rendered.
    assert_eq!(frame.timestamp_us, expected.timestamp_us);
    assert_eq!(frame.resolution, expected.resolution);
    assert_eq!(frame.distance_mm, expected.distance_mm);
    assert_eq!(frame.target_status, expected.target_status);
    assert_eq!(frame.nb_target_detected, expected.nb_target_detected);
    assert_eq!(frame.signal_per_spad, expected.signal_per_spad);
    assert_eq!(frame.ambient_per_spad, expected.ambient_per_spad);
    assert_eq!(frame.nb_spads_enabled, expected.nb_spads_enabled);
    assert_eq!(frame.range_sigma_mm, expected.range_sigma_mm);
    assert_eq!(frame.reflectance, expected.reflectance);
    assert_eq!(frame.silicon_temp_degc, expected.silicon_temp_degc);
    assert_eq!(frame.motion, expected.motion);
    assert_eq!(frame.cnh_raw, expected.cnh_raw);
}

// ── is_alive / before init ─────────────────────────────────────────────────

#[test]
fn is_alive_reads_the_ids_on_page_0_and_leaves_page_2() {
    let (f, d) = cx();
    assert!(d.is_alive());
    assert_eq!(f.lock().sensor.page, 0x02);
}

#[test]
fn is_alive_false_on_foreign_silicon() {
    let (f, d) = cx();
    f.lock().sensor.reg.insert((0, 0x00), 0xEB); // not 0xF0 / 0x0C
    assert!(!d.is_alive());
}

#[test]
fn is_alive_false_when_the_bus_fails() {
    let (f, d) = cx();
    f.lock().truncate_reads = true; // READ_REG answers short: an error inside the ULD
    assert!(!d.is_alive());
}

#[test]
fn with_uld_needs_init() {
    let (f, d) = cx();
    let r = d.with_uld(|u| Ok(u.data_read_size()));
    assert!(matches!(r, Err(DepzError::InvalidState(_))), "{r:?}");
    for r in [
        d.get_ranging_mode().map(|_| ()),
        d.get_power_mode().map(|_| ()),
        d.set_xtalk_margin(1),
        d.get_caldata_xtalk().map(|_| ()),
        d.configure_motion_indicator(None).map(|_| ()),
    ] {
        assert!(matches!(r, Err(DepzError::InvalidState(_))), "{r:?}");
    }
    assert_eq!(bus_ops(&f), (0, 0));
}

// ── init: firmware download + uploads ──────────────────────────────────────

fn init_downloads_the_variant_blob(d: &Vl53l8cxDevice, f: &FakeVl53l8, variant: &str) {
    let mut notes: Vec<String> = Vec::new();
    let progress: Arc<Mutex<Vec<(usize, usize)>>> = Arc::default();
    let p = progress.clone();
    d.init_with_progress(
        &mut |n| notes.push(n.to_string()),
        Some(Box::new(move |done, total| {
            p.lock().unwrap().push((done, total))
        })),
    )
    .unwrap();
    assert!(d.initialized());
    {
        let s = &f.lock().sensor;
        // the sensor booted exactly the blob this class ships: checksum matched
        assert_eq!(s.boots, 1);
        assert_eq!(s.variant, Some(variant));
        assert_eq!(d.firmware().set_name(), variant);
        let banks: Vec<usize> = [0x09, 0x0A, 0x0B]
            .iter()
            .map(|p| s.fw_banks[p].len())
            .collect();
        assert_eq!(banks, vec![0x8000, 0x8000, 0x5000]);
        // NVM read → offset data (488 B), default xtalk (776 B), default config
        let offsets: Vec<usize> = s.offset_uploads.iter().map(Vec::len).collect();
        assert_eq!(offsets, vec![488]);
        let xtalks: Vec<usize> = s.xtalk_uploads.iter().map(Vec::len).collect();
        assert_eq!(xtalks, vec![XTALK_BUFFER_SIZE]);
        assert_eq!(s.config_uploads, 1);
        // pipe control (one target per zone) and single-range DCI writes
        assert_eq!(s.dci(DCI_PIPE_CONTROL), &[1, 0, 1, 0]);
        assert_eq!(s.dci(DCI_SINGLE_RANGE), &1u32.to_le_bytes());
    }
    // write_progress tracks the three bank downloads in 2 KB steps
    let progress = progress.lock().unwrap();
    assert_eq!(progress.first(), Some(&(2048, 0x8000)));
    assert_eq!(progress.last(), Some(&(0x5000, 0x5000)));
    assert!(progress.iter().all(|(d, t)| d <= t));
    assert_eq!(progress.len(), 16 + 16 + 10);
    assert_eq!(notes.first().map(String::as_str), Some("SW reboot..."));
    assert_eq!(notes.last().map(String::as_str), Some("Sensor init done"));
    assert!(notes.iter().any(|n| n == "Sensor FW checksum OK"));
    // ULD default after init: 4x4, 1 Hz, continuous, 5 ms, 5 %, closest, awake
    assert_eq!(d.get_resolution().unwrap(), RESOLUTION_4X4);
    assert_eq!(d.get_ranging_frequency_hz().unwrap(), 1);
    assert_eq!(d.get_ranging_mode().unwrap(), RANGING_MODE_CONTINUOUS);
    assert_eq!(d.get_integration_time_ms().unwrap(), 5);
    assert_eq!(d.get_sharpener_percent().unwrap(), 5);
    assert_eq!(d.get_target_order().unwrap(), TARGET_ORDER_CLOSEST);
    assert_eq!(d.get_power_mode().unwrap(), POWER_MODE_WAKEUP);
    assert_eq!(d.get_xtalk_margin().unwrap(), 0);
}

#[test]
fn init_cx_downloads_the_cx_blob_and_uploads_defaults() {
    let (f, d) = cx();
    assert_eq!(d.firmware(), Vl53l8Firmware::Cx);
    init_downloads_the_variant_blob(&d, &f, "cx");
}

#[test]
fn init_ch_downloads_the_ch_blob_and_uploads_defaults() {
    let (f, d) = ch();
    assert_eq!(d.firmware(), Vl53l8Firmware::Ch);
    init_downloads_the_variant_blob(&d, &f, "ch");
}

#[test]
fn init_refuses_a_corrupted_firmware_download() {
    let (f, d) = cx();
    f.lock().sensor.corrupt_firmware = true;
    assert_uld_status(d.init(), Some(5)); // FW_CHECKSUM_FAIL
    assert!(!d.initialized());
    assert!(matches!(
        d.get_resolution(),
        Err(DepzError::InvalidState(_))
    ));
}

#[test]
fn a_second_init_reboots_the_sensor() {
    let (f, d) = cx();
    d.init().unwrap();
    d.set_ranging_frequency_hz(15).unwrap();
    d.init().unwrap();
    let s = &f.lock().sensor;
    assert_eq!((s.boots, s.config_uploads), (2, 2));
    assert_eq!(s.variant, Some("cx"));
}

// ── configuration round trips (DCI over READ_REG / WRITE_REG) ─────────────

#[test]
fn resolution_round_trip_re_uploads_offset_and_xtalk() {
    let (f, d) = cx();
    d.init().unwrap();
    d.set_resolution(RESOLUTION_8X8).unwrap();
    assert_eq!(d.get_resolution().unwrap(), RESOLUTION_8X8);
    {
        let s = &f.lock().sensor;
        assert_eq!(&s.dci(DCI_ZONE_CONFIG)[..2], &[8, 8]);
        assert_eq!(s.dci(DCI_DSS_CONFIG)[4], 16);
        assert_eq!(s.dci(DCI_DSS_CONFIG)[9], 1);
        // each set_resolution re-sends offset + xtalk shaped for that grid
        assert_eq!((s.offset_uploads.len(), s.xtalk_uploads.len()), (2, 2));
        assert_eq!(s.xtalk_uploads.last().unwrap(), DEFAULT_XTALK); // 8x8: the blob verbatim
    }
    d.set_resolution(RESOLUTION_4X4).unwrap();
    assert_eq!(d.get_resolution().unwrap(), RESOLUTION_4X4);
    {
        let s = &f.lock().sensor;
        assert_eq!(s.dci(DCI_DSS_CONFIG)[4], 64);
        assert_eq!(s.dci(DCI_DSS_CONFIG)[9], 4);
        let last = s.xtalk_uploads.last().unwrap();
        assert_ne!(last.as_slice(), DEFAULT_XTALK); // 4x4: grid folded
        assert_eq!(last.len(), XTALK_BUFFER_SIZE);
    }
    let ops = bus_ops(&f);
    assert_invalid_argument(d.set_resolution(32), "16 (4x4) or 64 (8x8)");
    assert_eq!(bus_ops(&f), ops);
}

#[test]
fn frequency_round_trip_and_floor() {
    let (f, d) = cx();
    d.init().unwrap();
    let ops = bus_ops(&f);
    // the sensor never ranges below 2 Hz (contract 04 §4): refused host-side
    assert_invalid_argument(d.set_ranging_frequency_hz(1), ">= 2 Hz");
    assert_eq!(bus_ops(&f), ops);
    d.set_ranging_frequency_hz(15).unwrap();
    assert_eq!(d.get_ranging_frequency_hz().unwrap(), 15);
    assert_eq!(f.lock().sensor.dci(DCI_FREQ_HZ), &[0, 15, 0, 0]);
}

#[test]
fn frequency_ceiling_follows_the_resolution() {
    // contract 11 §3 (ERRATA A13): at most 60 Hz at 4x4 and 15 Hz at 8x8, refused host-side.
    let (f, d) = cx();
    d.init().unwrap(); // 4x4 after init
    d.set_ranging_frequency_hz(60).unwrap();
    assert_eq!(d.get_ranging_frequency_hz().unwrap(), 60);
    let writes = f.lock().sensor.writes.len();
    assert_invalid_argument(d.set_ranging_frequency_hz(61), "<= 60 Hz");
    assert_eq!(f.lock().sensor.writes.len(), writes);
    d.set_resolution(RESOLUTION_8X8).unwrap();
    d.set_ranging_frequency_hz(15).unwrap();
    let writes = f.lock().sensor.writes.len();
    assert_invalid_argument(d.set_ranging_frequency_hz(16), "<= 15 Hz");
    assert_eq!(f.lock().sensor.writes.len(), writes);
    assert_eq!(d.get_ranging_frequency_hz().unwrap(), 15);
    d.set_resolution(RESOLUTION_4X4).unwrap();
    d.set_ranging_frequency_hz(16).unwrap(); // the ceiling moved back with the resolution
    assert_eq!(d.get_ranging_frequency_hz().unwrap(), 16);
}

#[test]
fn ranging_mode_round_trip() {
    let (f, d) = cx();
    d.init().unwrap();
    d.set_ranging_mode(RANGING_MODE_AUTONOMOUS).unwrap();
    assert_eq!(d.get_ranging_mode().unwrap(), RANGING_MODE_AUTONOMOUS);
    {
        let s = &f.lock().sensor;
        let m = s.dci(DCI_RANGING_MODE);
        assert_eq!([m[1], m[3]], [0x3, 0x2]);
        assert_eq!(s.dci(DCI_SINGLE_RANGE), &1u32.to_le_bytes());
    }
    d.set_ranging_mode(RANGING_MODE_CONTINUOUS).unwrap();
    assert_eq!(d.get_ranging_mode().unwrap(), RANGING_MODE_CONTINUOUS);
    {
        let s = &f.lock().sensor;
        let m = s.dci(DCI_RANGING_MODE);
        assert_eq!([m[1], m[3]], [0x1, 0x3]);
        assert_eq!(s.dci(DCI_SINGLE_RANGE), &0u32.to_le_bytes());
    }
    assert_uld_status(d.set_ranging_mode(2), None);
}

#[test]
fn integration_time_round_trip() {
    let (f, d) = cx();
    d.init().unwrap();
    d.set_integration_time_ms(20).unwrap();
    assert_eq!(d.get_integration_time_ms().unwrap(), 20);
    assert_eq!(
        &f.lock().sensor.dci(DCI_INT_TIME)[..4],
        &20_000u32.to_le_bytes()
    );
    for bad in [1, 1001] {
        assert_uld_status(d.set_integration_time_ms(bad), None);
    }
}

#[test]
fn sharpener_round_trip_reads_back_what_was_set() {
    let (f, d) = cx();
    d.init().unwrap();
    for pct in [0, 25, 99] {
        d.set_sharpener_percent(pct).unwrap();
        assert_eq!(d.get_sharpener_percent().unwrap(), pct as u8);
    }
    assert_eq!(
        f.lock().sensor.dci(DCI_SHARPENER)[0xD],
        (99 * 255 / 100) as u8
    );
    assert_uld_status(d.set_sharpener_percent(100), None);
}

#[test]
fn target_order_round_trip() {
    let (f, d) = cx();
    d.init().unwrap();
    d.set_target_order(TARGET_ORDER_STRONGEST).unwrap();
    assert_eq!(d.get_target_order().unwrap(), TARGET_ORDER_STRONGEST);
    assert_eq!(
        f.lock().sensor.dci(DCI_TARGET_ORDER)[0],
        TARGET_ORDER_STRONGEST
    );
    d.set_target_order(TARGET_ORDER_CLOSEST).unwrap();
    assert_eq!(d.get_target_order().unwrap(), TARGET_ORDER_CLOSEST);
    assert_uld_status(d.set_target_order(3), None);
}

#[test]
fn xtalk_margin_round_trip() {
    let (f, d) = cx();
    d.init().unwrap();
    d.set_xtalk_margin(3).unwrap();
    assert_eq!(d.get_xtalk_margin().unwrap(), 3);
    assert_eq!(
        &f.lock().sensor.dci(DCI_XTALK_CFG)[..4],
        &(3u32 * 2048).to_le_bytes()
    );
    assert_uld_status(d.set_xtalk_margin(10001), None);
}

#[test]
fn detection_thresholds_round_trip() {
    let (f, d) = cx();
    d.init().unwrap();
    assert_eq!(d.get_detection_thresholds_enable().unwrap(), 0);
    let thresholds = vec![
        DetectionThreshold {
            low_thresh: 200,
            high_thresh: 600,
            measurement: DIST_MM,
            type_: 0, // THRESH_IN_WINDOW
            zone_num: 5,
            operation: 0, // THRESH_OP_NONE
        },
        DetectionThreshold {
            low_thresh: 1,
            high_thresh: 3,
            measurement: SIGNAL_PER_SPAD_KCPS,
            type_: 3, // THRESH_GREATER_THAN_MAX
            zone_num: 6,
            operation: 2, // THRESH_OP_AND
        },
    ];
    d.set_detection_thresholds(&thresholds).unwrap();
    let got = d.get_detection_thresholds().unwrap();
    assert_eq!(got.len(), NB_THRESHOLDS);
    assert_eq!(&got[..2], thresholds.as_slice());
    assert_eq!(
        got[2],
        DetectionThreshold {
            low_thresh: 0,
            high_thresh: 0,
            measurement: 0,
            type_: 0,
            zone_num: 0,
            operation: 0
        }
    );
    {
        // on the wire the distance is ×4 and the signal ×2048 (plugin scaling)
        let s = &f.lock().sensor;
        let raw = s.dci(DCI_DET_THRESH_START);
        let i32at = |o: usize| i32::from_le_bytes(raw[o..o + 4].try_into().unwrap());
        assert_eq!((i32at(0), i32at(4)), (800, 2400));
        assert_eq!((i32at(12), i32at(16)), (2048, 3 * 2048));
        assert_eq!(s.dci(DCI_DET_THRESH_VALID_STATUS), &[5u8; 8]);
    }
    d.set_detection_thresholds_enable(true).unwrap();
    assert_eq!(d.get_detection_thresholds_enable().unwrap(), 1);
    assert_eq!(f.lock().sensor.dci(DCI_DET_THRESH_CONFIG)[0x11], 0x04);
    d.set_detection_thresholds_enable(false).unwrap();
    assert_eq!(d.get_detection_thresholds_enable().unwrap(), 0);
    assert_eq!(f.lock().sensor.dci(DCI_DET_THRESH_CONFIG)[0x11], 0x0C);
    d.set_detection_thresholds_auto_stop(true).unwrap();
    assert_eq!(f.lock().sensor.dci(DCI_PIPE_CONTROL)[3], 1);
    d.set_detection_thresholds_auto_stop(false).unwrap();
    assert_eq!(f.lock().sensor.dci(DCI_PIPE_CONTROL)[3], 0);
    let ops = bus_ops(&f);
    let too_many = vec![thresholds[0]; NB_THRESHOLDS + 1];
    assert!(matches!(
        d.set_detection_thresholds(&too_many),
        Err(DepzError::InvalidArgument(_))
    ));
    assert_eq!(bus_ops(&f), ops);
}

#[test]
fn power_modes_sleep_wake_deep_sleep() {
    let (f, d) = cx();
    d.init().unwrap();
    assert_eq!(d.get_power_mode().unwrap(), POWER_MODE_WAKEUP);
    d.set_power_mode(POWER_MODE_SLEEP).unwrap();
    assert_eq!(d.get_power_mode().unwrap(), POWER_MODE_SLEEP);
    assert_eq!(f.lock().sensor.r(0, 0x09), 0x02);
    d.set_power_mode(POWER_MODE_SLEEP).unwrap(); // already there: nothing to do
    d.set_power_mode(POWER_MODE_WAKEUP).unwrap();
    assert_eq!(d.get_power_mode().unwrap(), POWER_MODE_WAKEUP);
    assert_eq!(f.lock().sensor.boots, 1); // a plain wake keeps the firmware
    d.set_power_mode(POWER_MODE_DEEP_SLEEP).unwrap();
    assert_eq!(d.get_power_mode().unwrap(), POWER_MODE_DEEP_SLEEP);
    assert_eq!(f.lock().sensor.r(0, 0x0F), 0x43);
    d.set_power_mode(POWER_MODE_WAKEUP).unwrap(); // the blob is lost in deep sleep: init again
    assert_eq!(d.get_power_mode().unwrap(), POWER_MODE_WAKEUP);
    {
        let s = &f.lock().sensor;
        assert_eq!((s.boots, s.config_uploads), (2, 2));
    }
    assert_uld_status(d.set_power_mode(7), None);
    // a register value the ULD does not know → error, page restored
    f.lock().sensor.reg.insert((0, 0x09), 0x07);
    assert_uld_status(d.get_power_mode(), None);
    assert_eq!(f.lock().sensor.page, 0x02);
}

#[test]
fn configure_motion_indicator_writes_the_plugin_config() {
    let (f, d) = cx();
    d.init().unwrap();
    d.set_resolution(RESOLUTION_8X8).unwrap();
    let cfg = d.configure_motion_indicator(Some((400, 1500))).unwrap();
    assert_eq!(cfg.nb_of_aggregates, 16);
    assert_eq!(cfg.map_id[63], 15);
    assert_eq!(cfg.pack().len(), 156);
    assert_eq!(f.lock().sensor.dci(DCI_MOTION_DETECTOR_CFG), cfg.pack());
    assert!(f.lock().sensor.motion_configured);
    assert_uld_status(d.configure_motion_indicator(Some((400, 2000))), None); // span > 1500 mm
}

#[test]
fn configure_motion_indicator_defaults_to_400_1500_at_the_current_resolution() {
    let (f, d) = cx();
    d.init().unwrap();
    let default = d.configure_motion_indicator(None).unwrap();
    // 4x4 after init: the configuration follows the sensor's resolution
    assert_eq!(f.lock().sensor.dci(DCI_MOTION_DETECTOR_CFG), default.pack());
    let explicit = d.configure_motion_indicator(Some((400, 1500))).unwrap();
    assert_eq!(default, explicit);
}

// ── not while ranging ──────────────────────────────────────────────────────

#[test]
fn configuration_is_refused_while_ranging() {
    // The stream owns the register bank: a setter while ranging is refused before anything
    // reaches the bus (contract 04 §5).
    let (f, d) = ch();
    d.init().unwrap();
    d.set_ranging_frequency_hz(15).unwrap();
    d.start_ranging().unwrap();
    let ops = bus_ops(&f);
    let sent = f.board.sent_count();
    let calls: Vec<(&str, Result<(), DepzError>)> = vec![
        ("set_resolution", d.set_resolution(RESOLUTION_8X8)),
        ("set_ranging_frequency_hz", d.set_ranging_frequency_hz(15)),
        (
            "set_ranging_mode",
            d.set_ranging_mode(RANGING_MODE_AUTONOMOUS),
        ),
        ("set_integration_time_ms", d.set_integration_time_ms(20)),
        ("set_sharpener_percent", d.set_sharpener_percent(10)),
        (
            "set_target_order",
            d.set_target_order(TARGET_ORDER_STRONGEST),
        ),
        ("set_power_mode", d.set_power_mode(POWER_MODE_SLEEP)),
        ("set_xtalk_margin", d.set_xtalk_margin(1)),
        ("calibrate_xtalk", d.calibrate_xtalk(50, 4, 600)),
        ("get_caldata_xtalk", d.get_caldata_xtalk().map(|_| ())),
        (
            "set_caldata_xtalk",
            d.set_caldata_xtalk(&[0; XTALK_BUFFER_SIZE]),
        ),
        (
            "set_detection_thresholds_enable",
            d.set_detection_thresholds_enable(true),
        ),
        ("set_detection_thresholds", d.set_detection_thresholds(&[])),
        (
            "set_detection_thresholds_auto_stop",
            d.set_detection_thresholds_auto_stop(true),
        ),
        (
            "configure_motion_indicator",
            d.configure_motion_indicator(None).map(|_| ()),
        ),
        ("start_ranging", d.start_ranging()),
        ("init", d.init()),
        ("configure_cnh", d.configure_cnh(&cnh_16x20())),
    ];
    for (name, r) in calls {
        match r {
            Err(DepzError::InvalidState(m)) => assert!(m.contains("stop_ranging"), "{name}: {m}"),
            other => panic!("{name}: {other:?}"),
        }
    }
    assert_eq!(bus_ops(&f), ops);
    assert_eq!(f.board.sent_count(), sent);
    assert!(d.cnh_setup().is_none());
    d.stop_ranging().unwrap();
}

// ── start / stop ranging ───────────────────────────────────────────────────

fn start_ranging_arms_the_stream(d: &Vl53l8cxDevice, f: &FakeVl53l8, res: usize, size: usize) {
    d.init().unwrap();
    d.set_resolution(res).unwrap();
    d.set_ranging_frequency_hz(15).unwrap();
    assert!(!d.ranging());
    d.start_ranging().unwrap();
    assert!(d.ranging());
    {
        let b = f.lock();
        assert!(b.streaming && b.sensor.ranging);
        assert_eq!(b.frame_size, size);
        // the xshut bypass is on and the sensor sits on page 2 for the stream
        assert_eq!(b.sensor.r(0, 0x09), 0x05);
        assert_eq!(b.sensor.page, 0x02);
    }
    let start = f.board.sent_snapshot();
    let start = start.last().unwrap();
    assert_eq!(start.cmd, Vl53l8Cmd::StartStream as u8);
    assert_eq!(start.payload, (size as u16).to_le_bytes());
    // the FW agreed with the formula
    let (n, mismatch) = d
        .with_uld(|u| Ok((u.data_read_size(), u.frame_size_mismatch())))
        .unwrap();
    assert_eq!((n, mismatch), (size, None));
    d.stop_ranging().unwrap();
    assert!(!d.ranging());
    {
        let b = f.lock();
        assert!(!b.streaming && !b.sensor.ranging);
        // STOP_STREAM went first: the host never touched the bus while streaming
        assert_eq!(b.bus_ops_while_streaming, 0);
        assert_eq!(b.sensor.r(0, 0x09), 0x04);
    }
    let ops = bus_ops(f);
    let sent = f.board.sent_count();
    d.stop_ranging().unwrap(); // idempotent, nothing on the bus
    assert_eq!(bus_ops(f), ops);
    assert_eq!(f.board.sent_count(), sent);
    let b = f.lock();
    assert_eq!(b.sensor.writes.last().unwrap().1, 0x7FFF);
}

#[test]
fn start_ranging_cx_8x8() {
    let (f, d) = cx();
    start_ranging_arms_the_stream(&d, &f, RESOLUTION_8X8, CX_8X8_FRAME);
}

#[test]
fn start_ranging_ch_8x8() {
    let (f, d) = ch();
    start_ranging_arms_the_stream(&d, &f, RESOLUTION_8X8, CH_8X8_FRAME);
}

#[test]
fn start_ranging_cx_4x4() {
    let (f, d) = cx();
    start_ranging_arms_the_stream(&d, &f, RESOLUTION_4X4, CX_4X4_FRAME);
}

#[test]
fn start_ranging_ch_4x4() {
    let (f, d) = ch();
    start_ranging_arms_the_stream(&d, &f, RESOLUTION_4X4, CH_4X4_FRAME);
}

#[test]
fn start_ranging_refuses_a_laser_safety_fault() {
    let (f, d) = cx();
    d.init().unwrap();
    d.set_ranging_frequency_hz(15).unwrap();
    f.lock().sensor.laser_safety_fault = true;
    assert_uld_status(d.start_ranging(), Some(3)); // LASER_SAFETY
    assert!(!d.ranging());
    assert!(!f.lock().streaming);
}

#[test]
fn start_stream_rejected_by_the_bridge_leaves_the_host_not_ranging() {
    let (f, d) = cx();
    d.init().unwrap();
    d.set_ranging_frequency_hz(15).unwrap();
    f.lock()
        .refuse
        .insert(Vl53l8Cmd::StartStream as u8, ERR_INVALID_PARAM);
    match d.start_ranging() {
        Err(DepzError::Status { cmd, status }) => {
            assert_eq!(
                (cmd, status),
                (Vl53l8Cmd::StartStream as u8, ERR_INVALID_PARAM)
            )
        }
        other => panic!("{other:?}"),
    }
    assert!(!d.ranging());
}

#[test]
fn stop_ranging_clears_state_even_if_stop_stream_fails() {
    let (f, d) = cx();
    d.init().unwrap();
    d.set_ranging_frequency_hz(15).unwrap();
    d.start_ranging().unwrap();
    f.lock().refuse.insert(Vl53l8Cmd::StopStream as u8, 0x01); // ERROR
    assert!(matches!(
        d.stop_ranging(),
        Err(DepzError::Status { status: 0x01, .. })
    ));
    assert!(!d.ranging());
    assert!(!f.lock().sensor.ranging); // the ULD stop still ran
    d.set_ranging_frequency_hz(30).unwrap(); // not wedged: reconfiguration works
    assert_eq!(d.get_ranging_frequency_hz().unwrap(), 30);
}

// ── frames ─────────────────────────────────────────────────────────────────

fn frames_carry_every_layer(resolution: usize, side: usize) {
    let (f, d) = cx();
    d.init().unwrap();
    d.set_resolution(resolution).unwrap();
    d.set_ranging_frequency_hz(15).unwrap();
    let it = d.frames(4);
    d.start_ranging().unwrap();
    let mut frame = None;
    for _ in 0..3 {
        let expected = f.send_frame();
        let got = it.take(T).unwrap();
        assert_frame(&got, &expected);
        frame = Some(got);
    }
    let frame = frame.unwrap();
    // the scene: a plane 25 mm further per zone step, the far corner empty
    assert_eq!(frame.distance_mm[0], SCENE_BASE_MM);
    assert_eq!(frame.distance_mm[1], SCENE_BASE_MM + SCENE_STEP_MM);
    assert_eq!(frame.distance_mm[side], SCENE_BASE_MM + SCENE_STEP_MM);
    assert_eq!(frame.distance_mm[side * side - 1], 0);
    assert_eq!(frame.target_status[side * side - 1], 255);
    assert_eq!(frame.nb_target_detected[side * side - 1], 0);
    assert!(frame.cnh_raw.is_none() && frame.motion.is_none());
    assert_eq!(d.frame_parse_errors(), 0);
    assert_eq!(d.reassembler_discards(), 0);
    assert_eq!(f.lock().chunks_sent, 3); // ≤ 1528 B: one chunk per frame
    d.stop_ranging().unwrap();
}

#[test]
fn frames_carry_every_layer_4x4() {
    frames_carry_every_layer(RESOLUTION_4X4, 4);
}

#[test]
fn frames_carry_every_layer_8x8() {
    frames_carry_every_layer(RESOLUTION_8X8, 8);
}

#[test]
fn frame_timestamps_follow_the_ranging_period() {
    let (f, d) = cx();
    d.init().unwrap();
    d.set_ranging_frequency_hz(15).unwrap();
    let it = d.frames(4);
    d.start_ranging().unwrap();
    let mut stamps = Vec::new();
    for _ in 0..2 {
        f.send_frame();
        stamps.push(it.take(T).unwrap().timestamp_us);
    }
    assert_eq!(stamps[1] - stamps[0], 1_000_000 / 15);
    d.stop_ranging().unwrap();
}

#[test]
fn frames_iterator_and_on_frame_callback() {
    let (f, d) = cx();
    d.init().unwrap();
    d.set_ranging_frequency_hz(15).unwrap();
    let seen: Arc<Mutex<Vec<u64>>> = Arc::default();
    let s = seen.clone();
    let hook = d.on_frame(move |fr| s.lock().unwrap().push(fr.timestamp_us));
    let it = d.frames(4);
    d.start_ranging().unwrap();
    let expected: Vec<Vl53l8Frame> = (0..3).map(|_| f.send_frame()).collect();
    for exp in &expected {
        assert_frame(&it.take(T).unwrap(), exp);
    }
    let want: Vec<u64> = expected.iter().map(|e| e.timestamp_us).collect();
    assert!(until(|| seen.lock().unwrap().len() == 3, 2000));
    assert_eq!(*seen.lock().unwrap(), want);
    hook.cancel();
    f.send_frame();
    it.take(T).unwrap();
    assert_eq!(seen.lock().unwrap().len(), 3); // unsubscribed callbacks see nothing more
    assert_eq!(it.dropped_count(), 0);
    assert_eq!(d.stream_dropped_counts(), vec![0]);
    d.stop_ranging().unwrap();
}

#[test]
fn get_frame_returns_the_next_frame() {
    let (f, d) = cx();
    d.init().unwrap();
    d.set_ranging_frequency_hz(15).unwrap();
    d.start_ranging().unwrap();
    std::thread::scope(|sc| {
        let waiter = sc.spawn(|| d.get_frame(Some(T)));
        // get_frame subscribes when called: send once its subscription is in place
        let expected = Mutex::new(None);
        assert!(until(
            || {
                if waiter.is_finished() {
                    return true;
                }
                let mut e = expected.lock().unwrap();
                if d.stream_dropped_counts().len() == 1 && e.is_none() {
                    *e = Some(f.send_frame());
                }
                false
            },
            2000
        ));
        let expected = expected.into_inner().unwrap().unwrap();
        assert_frame(&waiter.join().unwrap().unwrap(), &expected);
    });
    d.stop_ranging().unwrap();
}

#[test]
fn a_scene_change_shows_in_the_next_frame() {
    let (f, d) = cx();
    d.init().unwrap();
    d.set_ranging_frequency_hz(15).unwrap();
    let it = d.frames(4);
    d.start_ranging().unwrap();
    f.send_frame();
    let first = it.take(T).unwrap();
    {
        let mut b = f.lock();
        b.sensor.distance_offset_mm = 100;
        b.sensor.silicon_temp_degc = 41;
    }
    let expected = f.send_frame();
    let second = it.take(T).unwrap();
    assert_frame(&second, &expected);
    for z in 0..15 {
        assert_eq!(second.distance_mm[z] - first.distance_mm[z], 100);
    }
    assert_eq!(second.silicon_temp_degc, 41);
    d.stop_ranging().unwrap();
}

#[test]
fn motion_indicator_output_rides_in_the_frame() {
    let (f, d) = cx();
    d.init().unwrap();
    d.set_resolution(RESOLUTION_8X8).unwrap();
    d.set_ranging_frequency_hz(15).unwrap();
    d.configure_motion_indicator(Some((400, 1500))).unwrap();
    let it = d.frames(4);
    d.start_ranging().unwrap();
    let expected = f.send_frame();
    let frame = it.take(T).unwrap();
    assert_frame(&frame, &expected);
    let m = frame.motion.unwrap();
    assert_eq!(m.global_indicator_1, 0x1234);
    assert_eq!(m.nb_of_aggregates, 16);
    assert_eq!(&m.motion[..3], &[0, 1000, 2000]);
    d.stop_ranging().unwrap();
}

#[test]
fn ch_cnh_frame_streams_in_chunks_and_decodes() {
    // A CH frame with a 16×20 CNH block is 3156 B (the lab capture): three ≤1528 B chunks
    // reassembled into one frame whose CNH block decodes.
    let (f, d) = ch();
    d.init().unwrap();
    d.set_resolution(RESOLUTION_8X8).unwrap();
    d.set_ranging_frequency_hz(15).unwrap();
    assert!(d.cnh_setup().is_none());
    let cfg = cnh_16x20();
    d.configure_cnh(&cfg).unwrap();
    assert_eq!(d.cnh_setup(), Some(cfg.clone()));
    assert_eq!(
        f.lock().sensor.dci(Vl53l8CnhSetup::MI_CFG_DEV_IDX),
        cfg.pack()
    );
    let it = d.frames(4);
    d.start_ranging().unwrap();
    assert_eq!(f.lock().frame_size, CH_8X8_CNH_16X20_FRAME);
    let expected = f.send_frame();
    assert_eq!(f.lock().chunks_sent, 3);
    let frame = it.take(T).unwrap();
    assert_frame(&frame, &expected);
    let raw = frame.cnh_raw.unwrap();
    assert_eq!(raw.len(), 1708);
    assert_eq!(cfg.required_memory().unwrap(), 1708);
    let decoded = decode_cnh(&cfg.decode_config(), &raw).unwrap();
    assert_eq!(decoded.ref_residual_word as f64 / 2048.0, 3.0);
    let want = expected_cnh(16, 20);
    assert_eq!(decoded.aggregates.len(), 16);
    for (got, (raw, scaler, _)) in decoded.aggregates.iter().zip(&want) {
        assert_eq!(&got.hist_raw, raw);
        assert_eq!(&got.hist_scaler, scaler);
    }
    assert_eq!(d.reassembler_discards(), 0);
    d.stop_ranging().unwrap();
}

#[test]
fn corrupted_footer_counts_as_a_parse_error() {
    let (f, d) = cx();
    d.init().unwrap();
    d.set_ranging_frequency_hz(15).unwrap();
    let it = d.frames(4);
    d.start_ranging().unwrap();
    f.lock().sensor.corrupt_footer = true;
    f.send_frame();
    assert!(until(|| d.frame_parse_errors() == 1, 2000));
    assert!(it.is_empty());
    assert_eq!(d.reassembler_discards(), 0);
    let expected = f.send_frame(); // the next frame is fine
    assert_frame(&it.take(T).unwrap(), &expected);
    assert_eq!(d.frame_parse_errors(), 1);
    d.stop_ranging().unwrap();
}

#[test]
fn frames_before_init_are_dropped_silently() {
    let (f, d) = cx();
    let it = d.frames(4);
    f.send_chunk(1, 8, 0, vec![0; 8]); // a complete frame, no ULD to parse it
    f.send_report(0x93, &[0; 5]); // too short for a chunk header: not a frame
    f.send_report(0x91, &[0; 9]); // an unsolicited REG_DATA: nobody asked
                                  // a marker the reader handles after the three: once its event is out, they were handled
    let events = d.events(8);
    f.send_report(0x7F, &[0]);
    assert!(events.take(T).is_ok());
    assert!(it.is_empty());
    assert_eq!(d.frame_parse_errors(), 0);
    assert_eq!(d.reassembler_discards(), 0);
    assert!(matches!(
        d.get_frame(Some(Duration::from_millis(50))),
        Err(DepzError::Timeout {
            cmd: Some(0x93),
            ..
        })
    ));
}

#[test]
fn a_frame_meeting_a_busy_driver_is_dropped_as_unparsed() {
    // The reader never waits for the driver (a caller holding it may wait for a reply only the
    // reader delivers): the frame is counted as unparsed, the next one parses.
    let (f, d) = cx();
    d.init().unwrap();
    d.set_ranging_frequency_hz(15).unwrap();
    let it = d.frames(4);
    d.start_ranging().unwrap();
    d.with_uld(|_| {
        f.send_frame();
        assert!(until(|| d.frame_parse_errors() == 1, 2000));
        Ok(())
    })
    .unwrap();
    assert!(it.is_empty());
    let expected = f.send_frame();
    assert_frame(&it.take(T).unwrap(), &expected);
    assert_eq!(d.frame_parse_errors(), 1);
    d.stop_ranging().unwrap();
}

fn parts(raw: &[u8], n: usize) -> Vec<Vec<u8>> {
    raw.chunks(n).map(<[u8]>::to_vec).collect()
}

#[test]
fn reassembly_gap_drops_the_frame_and_the_orphan() {
    let (f, d) = ch();
    d.init().unwrap();
    d.set_resolution(RESOLUTION_8X8).unwrap();
    d.set_ranging_frequency_hz(15).unwrap();
    d.configure_cnh(&cnh_16x20()).unwrap();
    let it = d.frames(4);
    d.start_ranging().unwrap();
    let raw = f.lock().sensor.build_frame();
    let p = parts(&raw, STREAM_CHUNK);
    let (a, b, c) = (p[0].clone(), p[1].clone(), p[2].clone());
    let n = raw.len();
    // middle chunk lost: the frame in progress is discarded, the tail is an orphan
    f.send_chunk(10, n, 0, a.clone());
    f.send_chunk(10, n, a.len() + b.len(), c);
    // overlap: a chunk that re-sends bytes already accumulated
    f.send_chunk(11, n, 0, a.clone());
    f.send_chunk(11, n, a.len() - 4, b.clone());
    // full_size mismatch mid-frame
    f.send_chunk(12, n, 0, a.clone());
    f.send_chunk(12, n + 4, a.len(), b.clone());
    // restart on offset 0 while a frame is in progress
    f.send_chunk(13, n, 0, a.clone());
    f.send_chunk(14, n, 0, a.clone());
    // over-run: more bytes than full_size announced
    f.send_chunk(15, a.len() + 4, a.len(), b);
    // then a good frame completes
    let expected = f.send_frame();
    assert_frame(&it.take(T).unwrap(), &expected);
    assert_eq!(d.reassembler_discards(), 5);
    assert_eq!(d.frame_parse_errors(), 0);
    d.stop_ranging().unwrap();
}

// ── poll mode (contract 04 §6) ─────────────────────────────────────────────

#[test]
fn poll_mode_read_through_the_bridge() {
    let (f, d) = ch();
    d.init().unwrap();
    d.set_resolution(RESOLUTION_8X8).unwrap();
    d.set_ranging_frequency_hz(15).unwrap();
    d.configure_cnh(&cnh_16x20()).unwrap();
    d.start_ranging().unwrap();
    assert!(!d.with_uld(|u| u.check_data_ready()).unwrap()); // nothing latched yet
    let expected = {
        let mut b = f.lock();
        let e = b.sensor.expected_frame();
        b.sensor.latch_frame();
        e
    };
    assert!(d.with_uld(|u| u.check_data_ready()).unwrap());
    let reads = f.lock().sensor.reads.len();
    let parsed = d.with_uld(|u| u.get_ranging_data(7)).unwrap(); // 3156 B: two READ_REG chunks
    let sizes: Vec<usize> = f.lock().sensor.reads[reads..].iter().map(|r| r.2).collect();
    assert_eq!(sizes, vec![2048, CH_8X8_CNH_16X20_FRAME - 2048]);
    assert_eq!(parsed.distance_mm, expected.distance_mm);
    assert_eq!(parsed.target_status, expected.target_status);
    assert_eq!(parsed.cnh_raw, Some(cnh_block(16, 20)));
    assert!(!d.with_uld(|u| u.check_data_ready()).unwrap()); // same stream count: no new frame
    d.stop_ranging().unwrap();
}

#[test]
fn short_read_reg_answer_is_an_error() {
    let (f, d) = cx();
    d.init().unwrap();
    f.lock().truncate_reads = true;
    match d.get_resolution() {
        Err(DepzError::Protocol(m)) => assert!(m.contains("READ_REG"), "{m}"),
        other => panic!("{other:?}"),
    }
}

// ── xtalk calibration, save / load (contract 04 §7) ───────────────────────

fn calibrate_xtalk_runs_the_fw_table(d: &Vl53l8cxDevice, f: &FakeVl53l8, table_len: usize) {
    d.init().unwrap();
    d.set_ranging_frequency_hz(15).unwrap();
    d.set_sharpener_percent(25).unwrap();
    d.calibrate_xtalk(50, 4, 600).unwrap();
    let calibrated = {
        let s = &f.lock().sensor;
        // the firmware's own table, the target as the FW wants it (×4 mm, ×16 %)
        let tables: Vec<usize> = s.calib_tables.iter().map(Vec::len).collect();
        assert_eq!(tables, vec![table_len]);
        assert_eq!(s.calibrations, vec![(600 * 4, 50 * 16, 4)]);
        // the dedicated calibration list was programmed, then the default config restored
        assert_eq!(s.config_uploads, 2);
        s.calibrated_buf.clone().unwrap()
    };
    let (failed, blob) = d
        .with_uld(|u| Ok((u.xtalk_calibration_failed(), u.xtalk_data().to_vec())))
        .unwrap();
    assert!(!failed);
    assert_eq!(blob.len(), XTALK_BUFFER_SIZE);
    assert_eq!(blob[..blob.len() - 8], calibrated[..calibrated.len() - 8]);
    assert_eq!(
        blob[blob.len() - 8..],
        [0x00, 0x00, 0x00, 0x0F, 0x00, 0x01, 0x03, 0x04]
    );
    assert_ne!(blob.as_slice(), DEFAULT_XTALK);
    // the captured grid is the one the FW computed for this target
    let swapped = swap_buffer(&blob);
    for z in 0..64u32 {
        let o = 0x34 + 4 * z as usize;
        let v = u32::from_le_bytes(swapped[o..o + 4].try_into().unwrap());
        assert_eq!(v, Sensor::xtalk_grid_value(600, 50, 4, z), "zone {z}");
    }
    // the settings around the run came back
    assert_eq!(d.get_resolution().unwrap(), RESOLUTION_4X4);
    assert_eq!(d.get_ranging_frequency_hz().unwrap(), 15);
    assert_eq!(d.get_sharpener_percent().unwrap(), 25);
    // save: get_caldata_xtalk re-reads the FW buffer (8x8) and restores 4x4
    let saved = d.get_caldata_xtalk().unwrap();
    assert_eq!(saved, blob);
    assert_eq!(d.get_resolution().unwrap(), RESOLUTION_4X4);
    // load: the blob reaches the FW verbatim on the next 8x8 upload
    d.set_caldata_xtalk(&saved).unwrap();
    d.set_resolution(RESOLUTION_8X8).unwrap();
    assert_eq!(f.lock().sensor.xtalk_uploads.last().unwrap(), &saved);
    assert_uld_status(d.set_caldata_xtalk(&saved[..saved.len() - 1]), None);
}

#[test]
fn calibrate_xtalk_cx_runs_the_fw_table_and_captures_the_grid() {
    let (f, d) = cx();
    calibrate_xtalk_runs_the_fw_table(&d, &f, tables::calibrate_xtalk_cx().len());
}

#[test]
fn calibrate_xtalk_ch_runs_the_fw_table_and_captures_the_grid() {
    let (f, d) = ch();
    calibrate_xtalk_runs_the_fw_table(&d, &f, tables::calibrate_xtalk_lmz().len());
}

#[test]
fn calibrate_xtalk_reports_coverglass_too_good() {
    let (f, d) = cx();
    d.init().unwrap();
    f.lock().sensor.xtalk_cal_fails = true;
    d.calibrate_xtalk(20, 1, 3000).unwrap();
    assert!(d.with_uld(|u| Ok(u.xtalk_calibration_failed())).unwrap());
    assert_eq!(f.lock().sensor.calibrations, vec![(3000 * 4, 20 * 16, 1)]);
    assert_eq!(d.get_resolution().unwrap(), RESOLUTION_4X4);
}

#[test]
fn calibrate_xtalk_argument_checks_stay_off_the_bus() {
    let (f, d) = cx();
    d.init().unwrap();
    for (refl, nb, dist) in [
        (0, 4, 600),
        (100, 4, 600),
        (50, 0, 600),
        (50, 17, 600),
        (50, 4, 599),
        (50, 4, 3001),
    ] {
        let writes = f.lock().sensor.writes.len();
        assert_uld_status(d.calibrate_xtalk(refl, nb, dist), None);
        assert_eq!(f.lock().sensor.writes.len(), writes, "{refl} {nb} {dist}");
    }
}

// ── get_frame and close ────────────────────────────────────────────────────

#[test]
fn frames_subscription_ends_when_the_device_closes() {
    let (_f, d) = cx();
    let it = d.frames(4);
    d.close();
    assert!(until(|| it.is_closed(), 2000));
    assert!(it.take(T).is_err());
}
