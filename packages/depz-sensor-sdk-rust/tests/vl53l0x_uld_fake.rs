//! The VL53L0X ULD port (`Vl53l0xUldDriver`) over a VL53L0X die model (`common::fake_vl53l0x`, a
//! port of the Python SDK's `tests/fake_vl53l0x.py`) — no hardware.
//!
//! The assertions are those of the Python SDK's `tests/test_vl53l0x_uld.py` and
//! `tests/test_vl53l0x_device_mode.py`, reached through what the Rust driver makes public: its own
//! `pub` methods and the [`Vl53lxDriver`] trait. Python cases that need a private entry point of the
//! Rust port (the raw VCSEL-period setter, the sequence-step setters, the limit-check setters, the
//! GPIO config, `set_device_mode`) are not repeated here; the rest come through `decode`, `set_mode`,
//! the calibrations and `perform_ref_spad_management`. The first test holds the fake to the live
//! capture's init so the rest can trust it.

mod common;

use std::collections::BTreeMap;
use std::sync::Arc;

use depz_sensor_sdk::devices::{DepzError, Result};
use depz_sensor_sdk::vl53lx::driver::{ExtraValue, Vl53lxDriver, Vl53lxPlatform};
use depz_sensor_sdk::vl53lx::uld::{create_driver, RefSpads, Vl53l0xUldDriver};

use common::fake_vl53l0x::{FakeVl53l0x, Op, FRAME_635_MM, FRAME_NO_TARGET, PRODUCT_ID};
use common::recording_packets;

const SYSRANGE_START: u16 = 0x00;
const SYSRANGE_MODE_BACKTOBACK: u8 = 0x02;
const SYSRANGE_MODE_TIMED: u8 = 0x04;
const SYSTEM_INTERRUPT_CLEAR: u16 = 0x0B;
const RESULT_INTERRUPT_STATUS: u16 = 0x13;
const GLOBAL_CONFIG_SPAD_ENABLES_REF_0: u16 = 0xB0;

/// The prologue of every StartMeasurement: the stop variable 0x3C goes back on page 1.
fn arm() -> Vec<(u16, Vec<u8>)> {
    vec![
        (0x80, vec![1]),
        (0xFF, vec![1]),
        (0x00, vec![0]),
        (0x91, vec![0x3C]),
        (0x00, vec![1]),
        (0xFF, vec![0]),
        (0x80, vec![0]),
    ]
}

fn rig(nvm: &[(u8, u32)]) -> (Arc<FakeVl53l0x>, Vl53l0xUldDriver) {
    let fake = FakeVl53l0x::new(nvm);
    let drv = Vl53l0xUldDriver::new(fake.clone() as Arc<dyn Vl53lxPlatform>, "VL53L0X");
    (fake, drv)
}

/// An initialised VL53L0X; the fake's log starts after init.
fn ready_with(nvm: &[(u8, u32)]) -> (Arc<FakeVl53l0x>, Vl53l0xUldDriver) {
    let (fake, mut drv) = rig(nvm);
    drv.sensor_init().unwrap();
    fake.die().ops.clear();
    (fake, drv)
}

fn ready() -> (Arc<FakeVl53l0x>, Vl53l0xUldDriver) {
    ready_with(&[])
}

/// A sensor-level error whose message contains `needle`.
#[track_caller]
fn assert_sensor_err<T: std::fmt::Debug>(r: Result<T>, needle: &str) {
    match r {
        Err(DepzError::Sensor(m)) => assert!(m.contains(needle), "{m:?} lacks {needle:?}"),
        other => panic!("expected a sensor error with {needle:?}, got {other:?}"),
    }
}

/// A 12-byte result block in the layout of FRAME_635_MM.
fn frame(
    device_status: u8,
    distance: u16,
    signal_97: u16,
    ambient_97: u16,
    spads_88: u16,
) -> [u8; 12] {
    let mut f = [0u8; 12];
    f[0] = device_status;
    f[1] = 0x06;
    f[2..4].copy_from_slice(&spads_88.to_be_bytes());
    f[4..6].copy_from_slice(&0x0071u16.to_be_bytes());
    f[6..8].copy_from_slice(&signal_97.to_be_bytes());
    f[8..10].copy_from_slice(&ambient_97.to_be_bytes());
    f[10..12].copy_from_slice(&distance.to_be_bytes());
    f
}

fn f(device_status: u8) -> [u8; 12] {
    frame(device_status, 635, 0x022B, 0x011B, 0xC104)
}

fn int(info: &BTreeMap<String, ExtraValue>, key: &str) -> i64 {
    match info[key] {
        ExtraValue::Int(v) => v,
        ExtraValue::Float(v) => panic!("{key} is a float {v}"),
    }
}

fn float(info: &BTreeMap<String, ExtraValue>, key: &str) -> f64 {
    match info[key] {
        ExtraValue::Float(v) => v,
        ExtraValue::Int(v) => panic!("{key} is an int {v}"),
    }
}

fn le16_or(b: &[u8], at: usize) -> u16 {
    u16::from_le_bytes([b[at], b[at + 1]])
}

fn collapse(items: Vec<Op>) -> Vec<Op> {
    let mut out: Vec<Op> = Vec::new();
    for item in items {
        if out.last() != Some(&item) {
            out.push(item);
        }
    }
    out
}

// ── init against the live capture ────────────────────────────────────────────

/// The register-level requests of init in the live capture, from its first SET_ADDR_WIDTH up to (not
/// including) its first CLEAR_I2C_ERRORS, which the device class issues, not the driver.
fn captured_init_ops() -> Vec<Op> {
    let mut out = Vec::new();
    let mut started = false;
    for p in recording_packets("vl53l0x_uld_long-range_33ms.depzrec") {
        if p.rx {
            continue;
        }
        let pl = &p.payload;
        let op = match p.cmd {
            0x32 => Op::Read(le16_or(pl, 0), le16_or(pl, 2) as usize),
            0x33 => Op::Write(le16_or(pl, 0), pl[2..].to_vec()),
            0x34 => Op::Xshut,
            0x38 => Op::Speed(le16_or(pl, 0)),
            0x39 => Op::Width(pl[0]),
            0x3A if started => break,
            _ => continue,
        };
        if op == Op::Width(1) {
            started = true;
        }
        if started {
            out.push(op);
        }
    }
    out
}

#[test]
fn init_issues_the_wire_sequence_of_the_live_capture() {
    // Polls (model id while the die reboots, data-ready) are collapsed: their count is timing.
    let (fake, mut drv) = rig(&[]);
    drv.sensor_init().unwrap();
    let ops = fake.die().ops.clone();
    let want = captured_init_ops();
    assert!(want.len() > 100, "capture init has {} ops", want.len());
    assert_eq!(collapse(ops), collapse(want));
}

#[test]
fn init_state_matches_the_board() {
    let (fake, mut drv) = ready();
    assert_eq!(drv.osc_frequency_mhz(), 0x975C << 4);
    assert!(drv.ref_spads_from_nvm());
    assert_eq!(
        drv.get_reference_spads(),
        RefSpads {
            count: 6,
            is_aperture: 0
        }
    );
    {
        let mut d = fake.die();
        // The 6 non-aperture SPADs through the good map FB..: indices 0,1,3,4,5,6 (2 is bad).
        assert_eq!(d.spad_enables(), [0x7B, 0, 0, 0, 0, 0]);
        // The VHV / phase shots of the reference calibration, in order.
        assert_eq!(d.shots, [(0x41, 0x01), (0x01, 0x02)]);
        assert_eq!(d.bank(0)[0x01], 0xE8); // TCC and MSRC off, as StaticInit leaves it
    }
    assert_eq!(drv.get_mode().unwrap(), "default");
    // The budget read back with TCC off: 2870 + 5438 + 8721 + 16244 + 550.
    assert_eq!(int(&drv.driver_info().unwrap(), "budget_us"), 33823);
    assert_eq!(drv.get_range_timing().unwrap(), (33, 0));
    assert_eq!(drv.model_id().unwrap(), 0xEE);
    assert_eq!(drv.reach_mm().unwrap(), Some(1200));
    assert_eq!(
        drv.modes(),
        ["default", "long-range", "high-speed", "high-accuracy"]
    );
    // The stop variable DataInit read (0x3C) is what StartMeasurement puts back.
    drv.start_ranging().unwrap();
    let w = fake.die().writes();
    assert_eq!(w[..7], arm()[..]);
    assert_eq!(w[7], (SYSRANGE_START, vec![SYSRANGE_MODE_BACKTOBACK]));
}

#[test]
fn trait_constants_of_the_l0x_driver() {
    let fake = FakeVl53l0x::new(&[]);
    let (drv, _caveat) = create_driver("VL53L0X", "uld", fake.clone()).unwrap();
    assert_eq!(drv.product(), "VL53L0X");
    assert_eq!(drv.driver_name(), "Vl53l0xUldDriver");
    assert_eq!(drv.addr_width(), 1);
    assert_eq!(drv.max_khz(), 400);
    assert_eq!(drv.budget_ms(), (20, 200));
    assert_eq!(drv.stream_block(), (0x14, 12));
    assert_eq!(
        drv.clear_steps(),
        [
            (SYSTEM_INTERRUPT_CLEAR, 0x01),
            (SYSTEM_INTERRUPT_CLEAR, 0x00)
        ]
    );
    assert_eq!(
        drv.supports(),
        [
            "mode",
            "timing",
            "offset",
            "xtalk",
            "calib_offset",
            "calib_xtalk",
            "refspad"
        ]
    );
}

#[test]
fn configure_long_range_33ms_reads_back_as_the_capture_did() {
    let (fake, mut drv) = ready();
    // The device class's configure(): sensor_init, set_mode, set_range_timing.
    drv.sensor_init().unwrap();
    drv.set_mode("long-range").unwrap();
    drv.set_range_timing(33, 0).unwrap();
    assert_eq!(drv.get_range_timing().unwrap(), (32, 0));
    assert_eq!(drv.get_mode().unwrap(), "long-range");
    // 0.1 Mcps lands on 12/128 in the 9.7 register; the budget reads back 32983 us.
    let info = drv.driver_info().unwrap();
    assert_eq!(float(&info, "signal_mcps"), 0.09375);
    assert_eq!(float(&info, "sigma_mm"), 60.0);
    assert_eq!(int(&info, "budget_us"), 32983);
    assert_eq!(int(&info, "pre_pclks"), 18);
    assert_eq!(int(&info, "final_pclks"), 14);
    assert_eq!(drv.reach_mm().unwrap(), Some(2000));
    let mut d = fake.die();
    let b0 = *d.bank(0);
    let b1 = *d.bank(1);
    assert_eq!((b0[0x57], b0[0x56], b0[0x50]), (0x50, 0x08, 0x08));
    assert_eq!((&b0[0x51..0x53], b0[0x46]), (&[0x00, 0x74][..], 0x1D));
    assert_eq!(
        (b0[0x48], b0[0x47], b0[0x70], b0[0x32], b0[0x30], b1[0x30]),
        (0x48, 0x08, 0x06, 0x03, 0x07, 0x20)
    );
    assert_eq!(&b0[0x71..0x73], &[0x01, 0xCA]);
    // Every VCSEL change ends in a phase calibration (sequence 0x02).
    assert!(d.shots.iter().filter(|s| **s == (0x01, 0x02)).count() >= 3);
}

// ── ranging profiles ─────────────────────────────────────────────────────────

#[test]
fn each_ranging_profile_reads_back() {
    // (mode, signal Mcps, sigma mm, budget us, pre PCLK, final PCLK, reach mm)
    let cases = [
        ("default", 0.25, 18.0, 32908, 14, 10, 1200),
        ("long-range", 0.09375, 60.0, 32983, 18, 14, 2000),
        ("high-speed", 0.25, 32.0, 30010, 14, 10, 1200),
        ("high-accuracy", 0.25, 18.0, 199769, 14, 10, 1200),
    ];
    for (mode, signal, sigma, budget, pre, fin, reach) in cases {
        let (_fake, mut drv) = ready();
        drv.set_mode(mode).unwrap();
        assert_eq!(drv.get_mode().unwrap(), mode);
        let info = drv.driver_info().unwrap();
        assert_eq!(float(&info, "signal_mcps"), signal, "{mode}");
        assert_eq!(float(&info, "sigma_mm"), sigma, "{mode}");
        assert_eq!(int(&info, "budget_us"), budget, "{mode}");
        assert_eq!(int(&info, "pre_pclks"), pre, "{mode}");
        assert_eq!(int(&info, "final_pclks"), fin, "{mode}");
        assert_eq!(drv.reach_mm().unwrap(), Some(reach), "{mode}");
        assert_eq!(drv.get_range_timing().unwrap().0 as i64, budget / 1000);
    }
}

#[test]
fn unknown_mode_is_refused() {
    let (_fake, mut drv) = ready();
    match drv.set_mode("fast") {
        Err(DepzError::InvalidArgument(m)) => assert!(m.contains("no such mode: fast"), "{m}"),
        other => panic!("{other:?}"),
    }
}

#[test]
fn reach_is_unknown_for_a_final_period_no_profile_sets() {
    let (fake, mut drv) = ready();
    fake.die().bank(0)[0x70] = (12 >> 1) - 1; // a 12 PCLK final range
    assert_eq!(drv.reach_mm().unwrap(), None);
}

// ── timing budget and inter-measurement period ───────────────────────────────

#[test]
fn budget_below_20ms_is_refused_like_the_c_driver() {
    let (fake, mut drv) = ready();
    assert_sensor_err(drv.set_range_timing(19, 0), "below the 20000 us minimum");
    assert_eq!(fake.die().ops, []);
    drv.set_range_timing(20, 0).unwrap();
    assert_eq!(drv.get_range_timing().unwrap().0, 19); // quantised down, as read back
}

#[test]
fn inter_measurement_period_scales_by_the_oscillator_calibration() {
    let (fake, mut drv) = ready();
    drv.set_range_timing(33, 100).unwrap();
    assert_eq!(
        fake.die().bank(0)[0x04..0x08],
        (100u32 * 0x28).to_be_bytes()
    );
    assert_eq!(drv.get_range_timing().unwrap().1, 100);
    // An unprogrammed OSC_CALIBRATE_VAL (0) means the period goes in as is.
    fake.die().bank(0)[0xF8..0xFA].copy_from_slice(&[0, 0]);
    drv.set_range_timing(33, 70).unwrap();
    assert_eq!(fake.die().bank(0)[0x04..0x08], 70u32.to_be_bytes());
    assert_eq!(drv.get_range_timing().unwrap().1, 70);
}

#[test]
fn timed_mode_starts_with_the_timed_start_code() {
    let (fake, mut drv) = ready();
    drv.set_range_timing(50, 100).unwrap();
    assert_eq!(drv.get_range_timing().unwrap(), (49, 100));
    fake.die().ops.clear();
    drv.start_ranging().unwrap();
    let w = fake.die().writes();
    assert_eq!(
        w.last().unwrap(),
        &(SYSRANGE_START, vec![SYSRANGE_MODE_TIMED])
    );
    assert_eq!(w[..7], arm()[..]);
}

// ── streaming and polling ────────────────────────────────────────────────────

#[test]
fn streamed_and_polled_frames_decode_alike() {
    let (fake, mut drv) = ready();
    let polled = drv.read_measurement().unwrap();
    drv.start_ranging().unwrap();
    let streamed = drv.decode(&FRAME_635_MM).unwrap();
    drv.stop_ranging().unwrap();
    assert_eq!(
        (
            streamed.distance_mm,
            streamed.status,
            streamed.status_text.as_str()
        ),
        (635, 0, "Range Valid")
    );
    assert_eq!(
        streamed.signal_kcps,
        (((0x022Bi64 << 9) * 1000) >> 16) as f64
    );
    assert_eq!(
        streamed.ambient_kcps,
        (((0x011Bi64 << 9) * 1000) >> 16) as f64
    );
    assert_eq!(streamed.spads, 193.0);
    assert_eq!(streamed.extra["device_range_status"], ExtraValue::Int(0x5A));
    assert_eq!(polled, streamed);
    // StopMeasurement zeroes the stop variable on page 1.
    let w = fake.die().writes();
    assert_eq!(
        w[w.len() - 6..],
        [
            (0x00, vec![0]),
            (0xFF, vec![1]),
            (0x00, vec![0]),
            (0x91, vec![0]),
            (0x00, vec![1]),
            (0xFF, vec![0]),
        ]
    );
}

#[test]
fn single_shot_whose_start_bit_never_clears() {
    let (fake, mut drv) = ready();
    fake.die().start_bit_stuck = true;
    assert_sensor_err(
        drv.perform_offset_calibration(600, 1),
        "start bit never cleared",
    );
}

#[test]
fn single_shot_waits_for_the_start_bit_and_data_ready() {
    let (fake, mut drv) = ready();
    {
        let mut d = fake.die();
        d.start_bit_reads = 3;
        d.ready_after_polls = 4;
    }
    // One single shot (the offset calibration's): 635 mm against 635 mm, offset 0.
    assert_eq!(drv.perform_offset_calibration(635, 1).unwrap(), 0);
    {
        let d = fake.die();
        let reads = d.reads();
        assert_eq!(reads.iter().filter(|a| **a == SYSRANGE_START).count(), 4);
        // polls + the clear's check
        assert_eq!(
            reads
                .iter()
                .filter(|a| **a == RESULT_INTERRUPT_STATUS)
                .count(),
            5 + 1
        );
    }
    // ... and start_ranging() resumes the continuous mode chosen before.
    fake.die().ops.clear();
    drv.start_ranging().unwrap();
    assert_eq!(
        fake.die().writes().last().unwrap(),
        &(SYSRANGE_START, vec![SYSRANGE_MODE_BACKTOBACK])
    );
}

// ── device mode on a live session (test_vl53l0x_device_mode.py) ──────────────

#[test]
fn start_after_single_shot_resumes_the_timed_mode() {
    let (fake, mut drv) = ready();
    drv.set_range_timing(50, 100).unwrap();
    drv.perform_offset_calibration(635, 1).unwrap(); // leaves single ranging
    drv.start_ranging().unwrap();
    assert_eq!(
        fake.die().writes().last().unwrap(),
        &(SYSRANGE_START, vec![SYSRANGE_MODE_TIMED])
    );
}

#[test]
fn start_without_any_continuous_mode_still_refuses() {
    let (_fake, mut drv) = rig(&[]);
    assert_sensor_err(drv.start_ranging(), "continuous device mode");
}

// ── interrupt ────────────────────────────────────────────────────────────────

#[test]
fn interrupt_that_will_not_clear() {
    let (fake, mut drv) = ready();
    {
        let mut d = fake.die();
        d.interrupt_sticky = true;
        d.ready_after_polls = 0;
    }
    assert_sensor_err(
        drv.perform_offset_calibration(600, 1),
        "interrupt not cleared after three rounds",
    );
    let clears = fake
        .die()
        .writes()
        .iter()
        .filter(|w| **w == (SYSTEM_INTERRUPT_CLEAR, vec![1]))
        .count();
    assert_eq!(clears, 3);
}

#[test]
fn range_error_bits_in_the_interrupt_status() {
    let (fake, mut drv) = ready();
    fake.die().interrupt_idle = 0x48;
    assert_sensor_err(
        drv.check_for_data_ready(),
        "range error reported in RESULT_INTERRUPT_STATUS 0x48",
    );
}

// ── boot and reset ───────────────────────────────────────────────────────────

#[test]
fn boot_timeout_when_the_model_id_never_answers() {
    let (fake, drv) = rig(&[]);
    {
        let mut d = fake.die();
        d.nacks_after_reset = 1_000_000;
        d.write(0xBF, &[0x01]); // arm the NACKs
    }
    assert_sensor_err(drv.wait_boot(50), "timeout waiting for MODEL_ID");
}

#[test]
fn soft_reset_timeouts() {
    let (fake, drv) = rig(&[]);
    fake.die().reset_ignored = true; // the model id never drops to 0
    assert_sensor_err(drv.reset_device(50), "timeout in the soft reset");
    {
        let mut d = fake.die();
        d.reset_ignored = false;
        d.nacks_after_reset = 1_000_000; // ... and never comes back
    }
    assert_sensor_err(drv.reset_device(50), "timeout in the soft reset");
}

// ── NVM ──────────────────────────────────────────────────────────────────────

#[test]
fn identification_words_from_the_nvm() {
    let (fake, mut drv) = ready();
    drv.get_info_from_device(7).unwrap();
    assert_eq!(drv.product_id_string(), PRODUCT_ID);
    assert_eq!((drv.module_id(), drv.revision()), (0x2A, 0x0C));
    assert_eq!(
        (drv.part_uid_upper(), drv.part_uid_lower()),
        (0x0C345678, 0x9ABCDEF0)
    );
    assert_eq!(drv.signal_rate_meas_fixed_400mm(), 0xA53C << 9);
    // 0x1940 = 6464 sixteenths = 404 mm at the 400 mm target: -4 mm.
    assert_eq!(drv.part2part_offset_adjustment_nvm_um(), -4000);
    // The reference-SPAD record is read once (option 1 was done by init).
    let strobes: Vec<u8> = fake
        .die()
        .writes()
        .into_iter()
        .filter(|(a, _)| *a == 0x94)
        .map(|(_, d)| d[0])
        .collect();
    assert_eq!(
        strobes,
        [0x02, 0x7B, 0x77, 0x78, 0x79, 0x7A, 0x7B, 0x7C, 0x73, 0x74, 0x75, 0x76]
    );
    fake.die().ops.clear();
    drv.get_info_from_device(7).unwrap();
    assert_eq!(fake.die().ops, []);
}

#[test]
fn nvm_offset_of_a_target_measured_nearer_than_400mm() {
    // 0x18C0 = 6336 sixteenths = 396 mm: the C driver's uint32 difference wraps and its int16
    // result wraps back, which is what makes the offset come out +4 mm.
    let (_fake, mut drv) = ready_with(&[(0x75, 0x00000018), (0x76, 0xC0000000)]);
    drv.get_info_from_device(4).unwrap();
    assert_eq!(drv.part2part_offset_adjustment_nvm_um(), 4000);
}

#[test]
fn nvm_without_a_400mm_measurement_has_no_offset() {
    let (_fake, mut drv) = ready_with(&[(0x75, 0), (0x76, 0)]);
    drv.get_info_from_device(4).unwrap();
    assert_eq!(drv.part2part_offset_adjustment_nvm_um(), 0);
}

#[test]
fn nvm_read_strobe_timeout() {
    let (fake, mut drv) = rig(&[]);
    fake.die().strobe_stuck = true;
    assert_sensor_err(drv.sensor_init(), "NVM read strobe");
}

// ── reference SPADs from NVM ─────────────────────────────────────────────────

#[test]
fn nvm_aperture_record_is_applied_from_the_aperture_quadrant() {
    // 0x8000 bit set: aperture; count 5 -> indices 12..16 -> byte 1 = 0x1F << 4 | byte 2 bit 0
    let (fake, drv) = ready_with(&[(0x6B, (1 << 15) | (5 << 8))]);
    assert_eq!(
        drv.get_reference_spads(),
        RefSpads {
            count: 5,
            is_aperture: 1
        }
    );
    assert_eq!(fake.die().spad_enables(), [0x00, 0xF0, 0x01, 0, 0, 0]);
}

#[test]
fn nvm_record_that_the_good_map_cannot_serve() {
    for (nvm, message) in [
        ([(0x24, 0), (0x25, 0)], "ran out of good reference SPADs"),
        (
            [(0x24, 0x0000FFFF), (0x25, 0)],
            "leaves the requested quadrant",
        ),
    ] {
        let (_fake, mut drv) = rig(&nvm);
        assert_sensor_err(drv.sensor_init(), message);
    }
}

#[test]
fn reference_spad_map_that_does_not_read_back() {
    let (fake, mut drv) = rig(&[]);
    fake.die().spad_map_stuck = true;
    assert_sensor_err(drv.sensor_init(), "did not read back");
}

// ── reference SPAD management ────────────────────────────────────────────────

#[test]
fn invalid_nvm_record_makes_init_measure_the_reference_spads() {
    for record in [13 << 8, (1 << 15) | (33 << 8)] {
        let (fake, mut drv) = rig(&[(0x6B, record)]);
        drv.sensor_init().unwrap();
        assert!(!drv.ref_spads_from_nvm());
        let mut d = fake.die();
        // 400 per SPAD against 2560: 3 -> 1200, ..., 6 -> 2400 (160 short), 7 -> 2800 (240 over):
        // the 6-SPAD map comes back.
        assert_eq!(d.ref_peaks, [1200, 1600, 2000, 2400, 2800]);
        assert_eq!(
            drv.get_reference_spads(),
            RefSpads {
                count: 6,
                is_aperture: 0
            }
        );
        assert_eq!(d.spad_enables(), [0x7B, 0, 0, 0, 0, 0]);
        // It ran after the rest of StaticInit, with the sequence config restored.
        assert_eq!(d.bank(0)[0x01], 0xE8);
        drop(d);
        assert_eq!(drv.get_range_timing().unwrap(), (33, 0));
    }
}

#[test]
fn management_keeps_the_map_that_overshoots_less() {
    let (fake, mut drv) = ready();
    fake.die().ref_rate_per_spad = 700;
    // 3 -> 2100 (460 short), 4 -> 2800 (240 over, closer): keep 4.
    assert_eq!(drv.perform_ref_spad_management().unwrap(), (4, false));
    let mut d = fake.die();
    assert_eq!(d.ref_peaks, [2100, 2800]);
    assert_eq!(d.spad_enables(), [0x1B, 0, 0, 0, 0, 0]); // indices 0, 1, 3, 4
}

#[test]
fn management_stops_on_the_target_exactly() {
    let (fake, mut drv) = ready();
    fake.die().ref_rate_per_spad = 640;
    assert_eq!(drv.perform_ref_spad_management().unwrap(), (4, false));
    assert_eq!(fake.die().ref_peaks, [1920, 2560]);
}

#[test]
fn management_stops_at_the_edge_of_the_non_aperture_quadrant() {
    let (fake, mut drv) = ready();
    fake.die().ref_rate_per_spad = 100;
    // 11 good non-aperture SPADs (index 2 is bad) make 1100; the next good SPAD is an aperture
    // one, so it stops short of the target.
    assert_eq!(drv.perform_ref_spad_management().unwrap(), (11, false));
    let mut d = fake.die();
    assert_eq!(*d.ref_peaks.last().unwrap(), 1100);
    assert_eq!(d.spad_enables(), [0xFB, 0x0F, 0, 0, 0, 0]);
}

#[test]
fn management_switches_to_aperture_spads_when_three_are_too_bright() {
    let (fake, mut drv) = ready();
    {
        let mut d = fake.die();
        d.ref_rate_per_spad = 1000;
        d.ref_rate_per_aperture_spad = 100;
    }
    // 3 non-aperture -> 3000 > 2560; restart at index 12 on aperture SPADs:
    // 3 -> 300 ... 25 -> 2500 (60 short), 26 -> 2600 (40 over, closer): 26.
    assert_eq!(drv.perform_ref_spad_management().unwrap(), (26, true));
    let mut d = fake.die();
    assert_eq!(d.ref_peaks[..2], [3000, 300]);
    assert_eq!(d.ref_peaks[d.ref_peaks.len() - 2..], [2500, 2600]);
    let enables = d.spad_enables();
    assert_eq!((enables[0], enables[1] & 0x0F), (0, 0));
    assert_eq!(enables.iter().map(|b| b.count_ones()).sum::<u32>(), 26);
    assert_eq!(
        drv.get_reference_spads(),
        RefSpads {
            count: 26,
            is_aperture: 1
        }
    );
}

#[test]
fn management_settles_for_three_aperture_spads_when_even_those_are_too_bright() {
    let (fake, mut drv) = ready();
    {
        let mut d = fake.die();
        d.ref_rate_per_spad = 1000;
        d.ref_rate_per_aperture_spad = 1000;
    }
    assert_eq!(drv.perform_ref_spad_management().unwrap(), (3, true));
    let mut d = fake.die();
    assert_eq!(d.ref_peaks, [3000, 3000]);
    assert_eq!(d.spad_enables(), [0x00, 0x70, 0, 0, 0, 0]); // indices 12, 13, 14
}

#[test]
fn management_runs_out_of_good_spads() {
    // The Python test sets the good map host-side; here it comes from the NVM, with a record that
    // makes init measure the reference SPADs instead of applying it.
    for (map, message) in [
        (0x0B000000, "ran out of good reference SPADs"), // indices 0, 1, 3 only
        (0x03000000, "ran out of good reference SPADs"), // fewer than the minimum
        (0x0000FF00, "leaves the requested quadrant"),   // aperture SPADs only
    ] {
        let (_fake, mut drv) = rig(&[(0x6B, 13 << 8), (0x24, map), (0x25, 0)]);
        assert_sensor_err(drv.sensor_init(), message);
    }
}

#[test]
fn management_map_that_does_not_read_back() {
    let (fake, mut drv) = ready();
    fake.die().spad_map_stuck = true;
    assert_sensor_err(drv.perform_ref_spad_management(), "did not read back");
}

#[test]
fn public_remeasure_rearms_the_stop_variable_and_restores_the_sequence() {
    let (fake, mut drv) = ready();
    drv.start_ranging().unwrap();
    drv.stop_ranging().unwrap();
    fake.die().ops.clear();
    assert_eq!(drv.perform_ref_spad_management().unwrap(), (6, false));
    let mut d = fake.die();
    let writes = d.writes();
    assert_eq!(writes[..7], arm()[..]);
    // The reference measurements run on the reference steps alone (0xC0), after a VHV / phase
    // calibration, and leave the sequence as it was.
    let mut want = vec![(0x41, 0x01), (0x01, 0x02)];
    want.extend([(0x01, 0xC0); 5]);
    assert_eq!(d.shots[d.shots.len() - 7..], want[..]);
    assert_eq!(d.bank(0)[0x01], 0xE8);
    // The last write puts the 6-SPAD map back after the 7th overshot.
    assert_eq!(
        writes.last().unwrap(),
        &(GLOBAL_CONFIG_SPAD_ENABLES_REF_0, vec![0x7B, 0, 0, 0, 0, 0])
    );
    assert_eq!(d.bank(0)[0x80], 0x00);
}

// ── tuning settings ──────────────────────────────────────────────────────────

#[test]
fn tuning_parameters_in_the_buffer() {
    let (fake, mut drv) = ready();
    drv.load_tuning_settings(&[
        0xFF, 0x00, 0x00, 0x64, // SigmaEstRefArray 100
        0xFF, 0x01, 0x03, 0x84, // SigmaEstEffPulseWidth 900
        0xFF, 0x02, 0x01, 0xF4, // SigmaEstEffAmbWidth 500
        0xFF, 0x03, 0x0B, 0x00, // targetRefRate 22 Mcps in 9.7
        0x02, 0x51, 0x00, 0x96, // a two-byte register write
        0x00,
    ])
    .unwrap();
    assert_eq!(fake.die().writes(), [(0x51, vec![0x00, 0x96])]);
    assert_eq!(
        (
            drv.sigma_est_ref_array(),
            drv.sigma_est_eff_pulse_width(),
            drv.sigma_est_eff_amb_width()
        ),
        (100, 900, 500)
    );
    assert_eq!(drv.target_ref_rate(), 0x0B00);
    // ... and the SPAD management converges on the new target: 400 per SPAD,
    // 7 -> 2800 (16 short of 2816), 8 -> 3200 (384 over): 7.
    assert_eq!(drv.perform_ref_spad_management().unwrap(), (7, false));
}

#[test]
fn tuning_buffer_errors() {
    let (_fake, mut drv) = ready();
    assert_sensor_err(
        drv.load_tuning_settings(&[0xFF, 0x04, 0x00, 0x00, 0x00]),
        "bad tuning parameter 4 at offset 0",
    );
    assert_sensor_err(
        drv.load_tuning_settings(&[0x01, 0x80, 0x01, 0x05, 0x00, 0, 0, 0, 0, 0, 0]),
        "bad tuning record at offset 4",
    );
}

// ── result decoding and the PAL range status ─────────────────────────────────

#[test]
fn device_status_maps_onto_the_pal_range_status() {
    let cases: [(u8, i32, &str); 16] = [
        (11, 0, "Range Valid"),
        (1, 5, "Hardware Fail"),
        (2, 5, "Hardware Fail"),
        (3, 5, "Hardware Fail"),
        (6, 4, "Phase Fail"),
        (9, 4, "Phase Fail"),
        (8, 3, "Min Range Fail"),
        (10, 3, "Min Range Fail"),
        (4, 2, "Signal Fail"),
        (0, 255, "No Update"),
        (5, 255, "No Update"),
        (7, 255, "No Update"),
        (12, 255, "No Update"),
        (13, 255, "No Update"),
        (14, 255, "No Update"),
        (15, 255, "No Update"),
    ];
    let (_fake, mut drv) = ready();
    for (internal, status, text) in cases {
        let device_status = (internal << 3) | 0x02;
        let r = drv.decode(&f(device_status)).unwrap();
        assert_eq!(
            (r.status, r.status_text.as_str()),
            (status, text),
            "{internal}"
        );
        assert_eq!(
            r.extra["device_range_status"],
            ExtraValue::Int(device_status as i64)
        );
    }
}

#[test]
fn sigma_fail_and_the_sigma_estimate() {
    let (_fake, mut drv) = ready();
    assert_eq!(f(0x5A), FRAME_635_MM);
    // The sigma estimate of the C driver on this frame: 166000 in 16.16 = 2 mm. Dmax from the
    // 2.21 Mcps ambient: between the 2.0 Mcps (900 mm) and 3.8 Mcps (750 mm) knees -> 882 mm.
    let good = drv.decode(&FRAME_635_MM).unwrap();
    assert_eq!(
        (good.status, good.sigma_mm, &good.extra["dmax_mm"]),
        (0, 2.0, &ExtraValue::Int(882))
    );
    // No signal at all: the estimate pins at its maximum (0x028F87AE >> 16 = 655 mm).
    let none = drv.decode(&frame(0x5A, 635, 0, 0x011B, 0xC104)).unwrap();
    assert_eq!(
        (none.status, none.status_text.as_str(), none.sigma_mm),
        (1, "Sigma Fail", 655.0)
    );
    // A faint signal under heavy ambient over a long integration: sigma over 18 mm.
    let faint = drv.decode(&frame(0x5A, 1800, 4, 0x0800, 0xC104)).unwrap();
    assert_eq!(faint.status, 1);
    assert!(faint.sigma_mm > 18.0, "{}", faint.sigma_mm);
}

#[test]
fn crosstalk_compensation_enters_the_sigma_estimate() {
    let (_fake, mut drv) = ready();
    let plain = drv.decode(&FRAME_635_MM).unwrap();
    assert_eq!(plain.sigma_mm, 2.0); // 166000 >> 16
    drv.set_xtalk(500).unwrap(); // 0.5 Mcps per SPAD
    let with_xtalk = drv.decode(&FRAME_635_MM).unwrap();
    assert_eq!(with_xtalk.status, 0);
    assert_eq!(with_xtalk.sigma_mm, (99000 >> 16) as f64);
}

#[test]
fn dmax_follows_the_ambient_lookup() {
    let (_fake, mut drv) = ready();
    // The ambient goes in as 9.7, the lookup is in 16.16 (<< 9).
    let mut dmax = |ambient_97: u16, status: u8| -> i64 {
        match drv
            .decode(&frame(status, 635, 0x022B, ambient_97, 0xC104))
            .unwrap()
            .extra["dmax_mm"]
        {
            ExtraValue::Int(v) => v,
            ExtraValue::Float(v) => panic!("{v}"),
        }
    };
    assert_eq!(dmax(0, 0x5A), 1200);
    assert_eq!(dmax(15 << 7, 0x5A), 400); // 15 Mcps
    assert_eq!(dmax(20 << 7, 0x5A), 400);
    assert_eq!(dmax(0x0100, 0x5A), 900); // 0x00020000
    let between = dmax(0x00C0, 0x5A); // 0x00018000
    assert!(900 < between && between < 1100, "{between}");
    // On a bad frame the dmax is what the ambient of that frame allows.
    let bad = drv
        .decode(&frame(0x22, 635, 0x022B, 0x1000, 0xC104))
        .unwrap(); // 32 Mcps ambient
    assert_eq!(
        (bad.status, &bad.extra["dmax_mm"]),
        (2, &ExtraValue::Int(400))
    );
}

#[test]
fn short_result_block_is_refused() {
    let (_fake, mut drv) = ready();
    assert_sensor_err(
        drv.decode(&FRAME_635_MM[..11]),
        "result block too short: 11 bytes",
    );
}

#[test]
fn the_first_frame_of_a_run_has_no_target() {
    let (_fake, mut drv) = ready();
    let r = drv.decode(&FRAME_NO_TARGET).unwrap();
    // Device status 0x58 (internal 11) at 8191 mm with a saturated 0xFFFF signal: valid, as the
    // Python port decodes it (sigma 1 mm, dmax 400 mm from the 0xFFFF ambient).
    assert_eq!((r.distance_mm, r.status, r.sigma_mm), (8191, 0, 1.0));
    assert_eq!(r.extra["dmax_mm"], ExtraValue::Int(400));
}

// ── offset ───────────────────────────────────────────────────────────────────

#[test]
fn offset_register_is_quarter_millimetres_in_twos_complement() {
    let cases: [(i64, u16, i64); 8] = [
        (0, 0x0000, 0),
        (35_000, 140, 35_000),
        (-35_000, 4096 - 140, -35_000),
        (511_000, 2044, 511_000),
        (600_000, 2044, 511_000), // clamped to 511000
        (-512_000, 2048, -512_000),
        (-700_000, 2048, -512_000), // clamped to -512000
        (-1_249, 4096 - 4, -1_000), // truncated toward zero
    ];
    let (fake, drv) = ready();
    for (um, reg, back) in cases {
        drv.set_offset_um(um).unwrap();
        assert_eq!(fake.die().bank(0)[0x28..0x2A], reg.to_be_bytes(), "{um}");
        assert_eq!(drv.get_offset_um().unwrap(), back, "{um}");
    }
}

#[test]
fn offset_in_whole_millimetres_through_the_trait() {
    let (fake, mut drv) = ready();
    drv.set_offset(-7).unwrap();
    assert_eq!(drv.get_offset().unwrap(), -7);
    assert_eq!(fake.die().bank(0)[0x28..0x2A], (4096u16 - 28).to_be_bytes());
}

#[test]
fn offset_calibration_against_a_target() {
    let (fake, mut drv) = ready();
    drv.set_offset_um(12_000).unwrap();
    // TCC on: the Python test enables it through the driver; the Rust setter is private, so the
    // sequence register is set on the die (the calibration reads it back from there).
    fake.die().bank(0)[0x01] |= 0x10;
    fake.die().frames.extend([
        f(0x22),
        f(0x22),
        FRAME_635_MM,
        frame(0x5A, 636, 0x022B, 0x011B, 0xC104),
    ]);
    // Three valid of five samples (the last one repeats): mean (635+636+636)/3 = 635.67 -> 636;
    // the offset becomes (600 - 636) mm.
    assert_eq!(drv.calibrate_offset(600, Some(5)).unwrap(), -36);
    assert_eq!(drv.get_offset_um().unwrap(), -36_000);
    let mut d = fake.die();
    assert_eq!(d.bank(0)[0x01], 0xF8); // TCC back on
    assert_eq!(d.shots.iter().filter(|s| s.1 == 0xE8).count(), 5); // ... and off while it ran
}

#[test]
fn offset_calibration_errors() {
    let (fake, mut drv) = ready();
    // With TCC off already there is nothing to put back afterwards.
    assert_eq!(drv.perform_offset_calibration(635, 2).unwrap(), 0);
    assert_eq!(fake.die().bank(0)[0x01], 0xE8);
    assert_sensor_err(
        drv.perform_offset_calibration(0, 50),
        "distance must be positive",
    );
    fake.die().frame = f(0x22);
    assert_sensor_err(
        drv.perform_offset_calibration(400, 3),
        "no valid measurement",
    );
    fake.die().frame = frame(0x5A, 0, 0x022B, 0x011B, 0xC104);
    assert_sensor_err(
        drv.perform_offset_calibration(400, 3),
        "mean range came out 0 mm",
    );
}

// ── crosstalk ────────────────────────────────────────────────────────────────

#[test]
fn xtalk_register_is_3_13() {
    let (fake, mut drv) = ready();
    drv.set_xtalk(297).unwrap();
    let rate: i64 = (297 << 16) / 1000;
    assert_eq!(
        fake.die().bank(0)[0x20..0x22],
        ((rate >> 3) as u16).to_be_bytes()
    );
    assert_eq!(
        drv.get_xtalk().unwrap() as i64,
        (((rate >> 3) << 3) * 1000) >> 16
    );
}

#[test]
fn xtalk_calibration_against_a_target_behind_the_glass() {
    let (fake, mut drv) = ready();
    fake.die().frames.extend([f(0x22), FRAME_635_MM]);
    // Four valid samples of 635 mm, 284160 (16.16) signal, 193 SPADs at a 1000 mm target: per
    // SPAD 1472 * (65536 - 41615) = 35211712, rounded >> 16 = 537 (0.0082 Mcps) -> 8 kcps.
    assert_eq!(drv.calibrate_xtalk(1000, Some(5)).unwrap(), 8);
    let mut d = fake.die();
    assert_eq!(d.bank(0)[0x20..0x22], (537u16 >> 3).to_be_bytes());
    assert_eq!(d.bank(0)[0x01], 0xE8);
}

#[test]
fn xtalk_calibration_with_nothing_to_compensate() {
    let (fake, mut drv) = ready();
    // Target at the measured distance (or nearer): no crosstalk.
    assert_eq!(drv.perform_xtalk_calibration(635, 2).unwrap(), 0);
    assert_eq!(drv.perform_xtalk_calibration(600, 2).unwrap(), 0);
    // No effective SPADs at all: the per-SPAD division is skipped.
    fake.die().frame = frame(0x5A, 635, 0x022B, 0x011B, 0x0010);
    assert_eq!(drv.perform_xtalk_calibration(1000, 2).unwrap(), 0);
    assert_eq!(fake.die().bank(0)[0x20..0x22], [0, 0]);
}

#[test]
fn xtalk_calibration_errors() {
    let (fake, mut drv) = ready();
    assert_sensor_err(
        drv.perform_xtalk_calibration(-1, 50),
        "distance must be positive",
    );
    fake.die().frame = f(0x22);
    assert_sensor_err(
        drv.perform_xtalk_calibration(1000, 2),
        "no valid measurement",
    );
}

// ── limit checks ─────────────────────────────────────────────────────────────

#[test]
fn limit_checks_on_the_device_side() {
    let (fake, mut drv) = ready();
    // DataInit disables the MSRC and pre-range checks (0x60 <- 0x12, as the capture shows); the
    // tuning settings then write 0x60 <- 0x00 over it.
    assert_eq!(fake.die().bank(0)[0x60], 0x00);
    // The final-range signal check is on after DataInit: 0.25 Mcps in 9.7 at 0x44.
    assert_eq!(
        fake.die().bank(0)[0x44..0x46],
        (((25i64 * 65536 / 100) >> 9) as u16).to_be_bytes()
    );
    let info = drv.driver_info().unwrap();
    assert_eq!(float(&info, "signal_mcps"), 0.25);
    assert_eq!(float(&info, "sigma_mm"), 18.0);
}
