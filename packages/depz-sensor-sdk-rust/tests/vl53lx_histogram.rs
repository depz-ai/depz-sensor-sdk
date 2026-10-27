//! The VL53LX histogram driver (ST's BareDriver + host post-processing): the register-block codec,
//! the bin folding, the die checks, the maths (ported from the Java `Vl53lxHistogramTests`), a replay
//! of the recorded histogram scenarios through the driver, and — ignored by default — the
//! post-processing cross-checked against the Python reference on generated frames.

mod common;

use std::collections::VecDeque;
use std::sync::{Arc, Mutex};

use depz_sensor_sdk::devices::{DepzError, Result};
use depz_sensor_sdk::json::Value as JValue;
use depz_sensor_sdk::vl53lx::bare::{
    hist, BareDriver, HistPostProcessConfig, HistogramBinData, RegBlock, Vl53lxHistogramDriver,
};
use depz_sensor_sdk::vl53lx::driver::{Vl53lxDriver, Vl53lxPlatform};
use depz_sensor_sdk::vl53lx::{RegData, StreamData};
use serde_json::Value;

use common::{recording_packets, reg_reads};

// The BareDriver touches the bus only where a test does not reach.
struct NoBus;

impl Vl53lxPlatform for NoBus {
    fn read_multi(&self, _addr: u16, _size: usize) -> Result<Vec<u8>> {
        panic!("no bus")
    }

    fn write_multi(&self, _addr: u16, _data: &[u8]) -> Result<()> {
        panic!("no bus")
    }

    fn set_i2c_speed(&self, _khz: u16) -> Result<()> {
        Ok(())
    }

    fn set_addr_width(&self, _width: u8) -> Result<()> {
        Ok(())
    }

    fn xshut_reset(&self) -> Result<()> {
        Ok(())
    }
}

#[test]
fn reg_block_round_trips_byte_exact_with_signed_masked_fields() {
    let mut raw = [0u8; 23];
    for (i, b) in raw.iter_mut().enumerate() {
        *b = 0xA0 + i as u8;
    }
    raw[17] = 0xFF; // algo__part_to_part_range_offset_mm: signed, mask 0x1FFF
    raw[18] = 0xF0;
    let mut blk = RegBlock::named("customer_nvm_managed").unwrap();
    blk.decode(&raw).unwrap();
    assert_eq!(blk.get("algo__part_to_part_range_offset_mm"), 0x1FF0); // masked first, so it reads positive
    assert_eq!(blk.get("global_config__spad_enables_ref_0"), raw[0] as i64);
    blk.set("mm_config__inner_offset_mm", -5);
    let enc = blk.encode();
    assert_eq!(&enc[19..21], &[0xFF, 0xFB]);
    let mut back = RegBlock::named("customer_nvm_managed").unwrap();
    back.decode(&enc).unwrap();
    assert_eq!(back.get("mm_config__inner_offset_mm"), -5);
    assert_eq!(blk.try_get("no_such_field"), None);
    assert!(RegBlock::named("no_such_block").is_none());
    assert!(matches!(
        blk.decode(&raw[..5]),
        Err(DepzError::InvalidArgument(_))
    ));
}

#[test]
fn repeated_bin_codes_are_averaged_into_one_bin_each() {
    let mut src = HistogramBinData {
        bin_seq: [7, 0, 1, 1, 2, 2],
        ..HistogramBinData::default()
    };
    for i in 0..24 {
        src.bin_data[i] = if i < 4 { 100 } else { 1000 + i as i64 };
    }
    let dst = hist::average_repeated_bins(&src);
    assert_eq!(dst.bin_seq, [7, 0, 1, 2, 16, 16]);
    assert_eq!(dst.bin_rep, [1, 1, 2, 2, 0, 0]);
    assert_eq!(dst.bins_in_data, 16);
    assert_eq!(dst.number_of_ambient_bins, 4);
    assert_eq!(dst.bin_data[8], (1008 + 1012 + 1) / 2); // codes 1 at bins 8 and 12, rounded half up
}

#[test]
fn an_l4_die_refuses_the_short_preset() {
    let mut bare = BareDriver::new(Arc::new(NoBus));
    bare.img_mut()
        .nvm_copy_data_mut()
        .set("identification__module_type", 0xAA);
    bare.img_mut()
        .nvm_copy_data_mut()
        .set("identification__model_id", 0xEB);
    assert!(bare.is_l4(), "an 0xAA / 0xEB die is an L4");
    assert_eq!(bare.distance_modes_for_die(), &["medium", "long"]);
    assert_eq!(bare.fda_max_timing_budget_us(), 200000);
    assert!(matches!(
        bare.set_distance_mode("short"),
        Err(DepzError::InvalidArgument(_))
    ));
}

#[test]
fn an_unknown_mode_is_an_invalid_argument() {
    // Checked before anything touches the bus: the scenarios expect invalid_argument.
    let mut drv = Vl53lxHistogramDriver::new(Arc::new(NoBus), "vl53l4cx");
    let err = drv.set_mode("very_long").unwrap_err();
    assert_eq!(err.kind(), Some("invalid_argument"));
    assert_eq!(drv.supports(), &["mode", "timing"]);
    assert!(drv.histogram(), "the histogram driver says so");
    assert_eq!(drv.clear_steps(), vec![(0x0086, 1)]);
    assert_eq!(drv.stream_block(), (0x0088, 83));
    assert_eq!(drv.max_khz(), 1000);
    assert_eq!(drv.reach_mm().unwrap(), None); // no oscillator frequency before init
}

#[test]
fn isqrt_is_the_floor_of_a_uint32_root() {
    assert_eq!(hist::isqrt(0xFFFF_FFFF), 65535);
    assert_eq!(hist::isqrt(15), 3);
    assert_eq!(hist::isqrt(16), 4);
    assert_eq!(hist::isqrt(1 << 32), 0); // the argument is uint32 in C
}

#[test]
fn bin_data_fields_carry_the_python_names() {
    let f = HistogramBinData::default().fields();
    assert_eq!(f.len(), 30);
    let keys: Vec<&str> = f.iter().map(|(k, _)| *k).collect();
    for k in [
        "result__dss_actual_effective_spads",
        "bin_seq",
        "roi_config__user_roi_requested_global_xy_size",
    ] {
        assert!(keys.contains(&k), "snake_case key {k}");
    }
    let bins = &f.iter().find(|(k, _)| *k == "bin_data").unwrap().1;
    assert_eq!(bins.as_array().unwrap().len(), 24);
    // set_field is the inverse.
    let mut d = HistogramBinData::default();
    let src = HistogramBinData {
        result_stream_count: 7,
        bin_seq: [7, 0, 1, 2, 3, 4],
        ..d.clone()
    };
    for (k, v) in src.fields() {
        assert!(d.set_field(k, &v), "{k}");
    }
    assert_eq!(d.result_stream_count, 7);
    assert_eq!(d.bin_seq, [7, 0, 1, 2, 3, 4]);
}

#[test]
fn a_frame_that_is_not_a_histogram_is_refused() {
    let mut drv = Vl53lxHistogramDriver::new(Arc::new(NoBus), "vl53l1cx");
    let wrong: depz_sensor_sdk::vl53lx::driver::BinData = Box::new(5u8);
    assert!(matches!(
        drv.to_measurement(&wrong),
        Err(DepzError::InvalidArgument(_))
    ));
    assert!(matches!(
        drv.bin_data(Some(&[0u8; 10])),
        Err(DepzError::InvalidArgument(_))
    ));
}

// ── recorded scenarios ──────────────────────────────────────────────────────

// Serves the recorded register reads in order (skipping reads the driver does not make) and checks every
// write against the recorded WRITE_REG traffic, byte for byte.
struct Replay {
    reads: Mutex<VecDeque<(u16, u16, Vec<u8>)>>,
    writes: Mutex<VecDeque<(u16, Vec<u8>)>>,
    skipped_reads: Mutex<usize>,
}

impl Replay {
    fn new(file: &str) -> (Arc<Replay>, Vec<StreamData>) {
        let pkts = recording_packets(file);
        let reads = reg_reads(&pkts, true)
            .into_iter()
            .map(|r| (r.addr, r.len, RegData::unpack(&r.reply).unwrap().data))
            .collect();
        let writes = pkts
            .iter()
            .filter(|p| !p.rx && p.cmd == 0x33)
            .map(|p| {
                (
                    u16::from_le_bytes([p.payload[0], p.payload[1]]),
                    p.payload[2..].to_vec(),
                )
            })
            .collect();
        let stream = pkts
            .iter()
            .filter(|p| p.rx && p.cmd == 0x93)
            .map(|p| StreamData::unpack(&p.payload).unwrap())
            .collect();
        (
            Arc::new(Replay {
                reads: Mutex::new(reads),
                writes: Mutex::new(writes),
                skipped_reads: Mutex::new(0),
            }),
            stream,
        )
    }
}

impl Vl53lxPlatform for Replay {
    fn read_multi(&self, addr: u16, size: usize) -> Result<Vec<u8>> {
        let mut q = self.reads.lock().unwrap();
        while let Some((a, l, data)) = q.pop_front() {
            if a == addr && l as usize == size {
                return Ok(data);
            }
            *self.skipped_reads.lock().unwrap() += 1;
        }
        Err(DepzError::ReplayMismatch(format!(
            "no recorded read of 0x{addr:04X}+{size}"
        )))
    }

    fn write_multi(&self, addr: u16, data: &[u8]) -> Result<()> {
        match self.writes.lock().unwrap().pop_front() {
            Some((a, d)) if a == addr && d == data => Ok(()),
            Some((a, d)) => Err(DepzError::ReplayMismatch(format!(
                "wrote 0x{addr:04X} {data:02X?}, the capture has 0x{a:04X} {d:02X?}"
            ))),
            None => Err(DepzError::ReplayMismatch(format!(
                "wrote 0x{addr:04X} past the capture"
            ))),
        }
    }

    fn set_i2c_speed(&self, _khz: u16) -> Result<()> {
        Ok(())
    }

    fn set_addr_width(&self, _width: u8) -> Result<()> {
        Ok(())
    }

    fn xshut_reset(&self) -> Result<()> {
        Ok(())
    }

    fn sleep_ms(&self, _ms: u32) {}
}

fn scenario(name: &str) -> Value {
    let p = common::vectors_dir()
        .join("..")
        .join("scenarios")
        .join(name);
    serde_json::from_str(&std::fs::read_to_string(&p).unwrap()).unwrap()
}

fn step<'a>(sc: &'a Value, key: &str, name: &str) -> &'a Value {
    sc["steps"]
        .as_array()
        .unwrap()
        .iter()
        .find(|s| s[key].as_str() == Some(name))
        .unwrap_or_else(|| panic!("no step {key}={name}"))
}

// init, configure, start, every streamed frame through bin_data / to_measurement, stop — the calls the
// device layer makes — and the frames compared with the scenario's.
fn replay_histogram_scenario(name: &str, product: &str) {
    let sc = scenario(name);
    let (platform, stream) = Replay::new(sc["recording"].as_str().unwrap());
    let mut drv = Vl53lxHistogramDriver::new(platform.clone(), product);

    drv.sensor_init().unwrap(); // init("histogram")
    let cfg = step(&sc, "call", "configure")["args"].as_array().unwrap();
    drv.sensor_init().unwrap();
    drv.set_mode(cfg[2].as_str().unwrap()).unwrap();
    drv.set_range_timing(
        cfg[0].as_u64().unwrap() as u32,
        cfg[1].as_u64().unwrap() as u32,
    )
    .unwrap();
    let want_timing = step(&sc, "call", "get_range_timing")["expect"]
        .as_array()
        .unwrap();
    let (b, i) = drv.get_range_timing().unwrap();
    assert_eq!(
        (b as u64, i as u64),
        (
            want_timing[0].as_u64().unwrap(),
            want_timing[1].as_u64().unwrap()
        )
    );
    drv.start_ranging().unwrap();

    let take = sc["steps"]
        .as_array()
        .unwrap()
        .iter()
        .find(|s| s.get("take").is_some())
        .unwrap();
    let want = take["expect"].as_array().unwrap();
    assert!(
        stream.len() >= want.len(),
        "{name}: {} streamed frames",
        stream.len()
    );
    for (k, (s, w)) in stream.iter().zip(want).enumerate() {
        let bins = drv.bin_data(Some(&s.data)).unwrap();
        let m = drv.to_measurement(&bins).unwrap();
        let ctx = format!("{name} frame {k}");
        assert_eq!(s.timestamp_us, w["timestamp_us"].as_u64().unwrap(), "{ctx}");
        assert_eq!(
            m.distance_mm as i64,
            w["distance_mm"].as_i64().unwrap(),
            "{ctx}"
        );
        assert_eq!(m.status as i64, w["status"].as_i64().unwrap(), "{ctx}");
        let wt = w["targets"].as_array().unwrap();
        assert_eq!(m.targets.len(), wt.len(), "{ctx}");
        for (t, wt) in m.targets.iter().zip(wt) {
            assert_eq!(
                t.distance_mm as i64,
                wt["distance_mm"].as_i64().unwrap(),
                "{ctx}"
            );
            assert_eq!(t.status as i64, wt["status"].as_i64().unwrap(), "{ctx}");
        }
        let fields = bins.downcast_ref::<HistogramBinData>().unwrap().fields();
        for (key, wv) in w["bins"].as_object().unwrap() {
            let got = &fields
                .iter()
                .find(|(k, _)| k == key)
                .unwrap_or_else(|| panic!("{ctx}: no key {key}"))
                .1;
            let got: Value = serde_json::from_str(&got.to_json()).unwrap();
            assert_eq!(&got, wv, "{ctx}: {key}");
        }
    }
    drv.stop_ranging().unwrap();
    let skipped = *platform.skipped_reads.lock().unwrap();
    assert_eq!(
        skipped, 0,
        "{name}: {skipped} recorded reads the driver did not make"
    );
    let left = platform.writes.lock().unwrap().len();
    assert_eq!(
        left, 0,
        "{name}: {left} recorded writes the driver did not make"
    );
}

#[test]
fn vl53l1cb_histogram_medium_33ms_replays() {
    replay_histogram_scenario("vl53l1cb_histogram_medium_33ms.json", "vl53l1cb");
}

#[test]
fn vl53l1cx_histogram_long_33ms_replays() {
    replay_histogram_scenario("vl53l1cx_histogram_long_33ms.json", "vl53l1cx");
}

#[test]
fn vl53l3cx_histogram_medium_33ms_replays() {
    replay_histogram_scenario("vl53l3cx_histogram_medium_33ms.json", "vl53l3cx");
}

#[test]
fn vl53l4cx_histogram_medium_50ms_replays() {
    replay_histogram_scenario("vl53l4cx_histogram_medium_50ms.json", "vl53l4cx");
}

#[test]
fn vl53l4cx_histogram_short_is_refused_on_an_l4_die() {
    let sc = scenario("vl53l4cx_histogram_short_33ms.json");
    let (platform, _) = Replay::new(sc["recording"].as_str().unwrap());
    let mut drv = Vl53lxHistogramDriver::new(platform, "vl53l4cx");
    drv.sensor_init().unwrap();
    let modes: Vec<&str> = step(&sc, "get", "modes")["expect"]
        .as_array()
        .unwrap()
        .iter()
        .map(|v| v.as_str().unwrap())
        .collect();
    assert_eq!(drv.modes(), modes);
    let br = step(&sc, "get", "budget_range")["expect"]
        .as_array()
        .unwrap();
    assert_eq!(
        drv.budget_ms(),
        (
            br[0].as_u64().unwrap() as u32,
            br[1].as_u64().unwrap() as u32
        )
    );
    let reach = step(&sc, "get", "driver_reach_mm")["expect"]
        .as_i64()
        .unwrap();
    assert_eq!(drv.reach_mm().unwrap(), Some(reach as i32));
    // configure(33, 0, "short") re-inits, then the mode is refused.
    drv.sensor_init().unwrap();
    let err = drv.set_mode("short").unwrap_err();
    assert_eq!(
        Some(step(&sc, "call", "configure")["error"].as_str().unwrap()),
        err.kind()
    );
}

// ── cross-check against the Python reference ────────────────────────────────

// `DEPZ_HIST_CASES=a.json:b.json cargo test -j 2 --test vl53lx_histogram -- --ignored`: each file is a
// list of {bins: HistogramBinData.fields(), hpp: {valid_phase_high, range_offset_mm, hist_target_order},
// out: {stream_count, targets: [RangeData fields]}} written by the Python reference's process_data.
#[test]
#[ignore]
fn post_processing_matches_python_on_generated_frames() {
    let files =
        std::env::var("DEPZ_HIST_CASES").expect("DEPZ_HIST_CASES: the case files, ':'-separated");
    let (mut cases, mut targets, mut compared, mut bad) = (0usize, 0usize, 0usize, 0usize);
    for file in files.split(':').filter(|f| !f.is_empty()) {
        let doc = JValue::parse(&std::fs::read_to_string(file).unwrap()).unwrap();
        for c in doc.as_array().unwrap() {
            cases += 1;
            let mut d = HistogramBinData::default();
            for (k, v) in c.get("bins").unwrap().entries().unwrap() {
                assert!(d.set_field(k, v), "case {cases}: bins key {k}");
            }
            let h = c.get("hpp").unwrap();
            let hpp = HistPostProcessConfig {
                valid_phase_high: h.get("valid_phase_high").unwrap().as_i64().unwrap(),
                range_offset_mm: h.get("range_offset_mm").unwrap().as_i64().unwrap(),
                hist_target_order: h.get("hist_target_order").unwrap().as_i64().unwrap(),
                ..HistPostProcessConfig::default()
            };
            let out = c.get("out").unwrap();
            let r = hist::process_data(&d, &hpp);
            let Some(wt) = out.get("targets").and_then(JValue::as_array) else {
                if r.is_ok() {
                    bad += 1;
                    println!("case {cases}: no error, want {:?}", out.get("error"));
                }
                continue;
            };
            let r = r.unwrap();
            if r.stream_count != out.get("stream_count").unwrap().as_i64().unwrap() {
                bad += 1;
            }
            if r.targets.len() != wt.len() {
                bad += 1;
                println!(
                    "case {cases}: {} targets, want {}",
                    r.targets.len(),
                    wt.len()
                );
                continue;
            }
            for (t, w) in r.targets.iter().zip(wt) {
                targets += 1;
                let got = t.fields();
                for (k, v) in w.entries().unwrap() {
                    compared += 1;
                    let g = got
                        .iter()
                        .find(|(gk, _)| gk == k)
                        .unwrap_or_else(|| panic!("no RangeData key {k}"))
                        .1;
                    if g != v.as_i64().unwrap() {
                        bad += 1;
                        if bad < 20 {
                            println!("case {cases} {k}: {g}, want {}", v.as_i64().unwrap());
                        }
                    }
                }
            }
        }
    }
    println!("{cases} cases, {targets} targets, {compared} fields compared, {bad} mismatches");
    assert!(cases > 0);
    assert_eq!(bad, 0);
}
