//! The Vl53lxDriver trait's defaults (`uld/base.py`): what a driver that implements only the
//! required core reports, and that every optional group it lacks fails with
//! `DepzError::Unsupported` naming the product, the driver and the group.

use std::time::Duration;

use depz_sensor_sdk::devices::{DepzError, Result};
use depz_sensor_sdk::vl53lx::driver::{Vl53lxDriver, Vl53lxRange};

/// The required core only; data is ready after `ready_after` polls.
struct Core {
    polls: u32,
    ready_after: u32,
}

impl Vl53lxDriver for Core {
    fn product(&self) -> &str {
        "vl53l1cx"
    }
    fn driver_name(&self) -> &'static str {
        "CoreOnly"
    }
    fn model_id(&mut self) -> Result<u16> {
        Ok(0xEACC)
    }
    fn sensor_init(&mut self) -> Result<()> {
        Ok(())
    }
    fn start_ranging(&mut self) -> Result<()> {
        Ok(())
    }
    fn stop_ranging(&mut self) -> Result<()> {
        Ok(())
    }
    fn check_for_data_ready(&mut self) -> Result<bool> {
        self.polls += 1;
        Ok(self.polls > self.ready_after)
    }
    fn clear_interrupt(&mut self) -> Result<()> {
        Ok(())
    }
    fn stream_block(&self) -> (u16, u16) {
        (0x0089, 17)
    }
    fn decode(&mut self, _raw: &[u8]) -> Result<Vl53lxRange> {
        Err(DepzError::Sensor("not used".into()))
    }
    fn read_measurement(&mut self) -> Result<Vl53lxRange> {
        Err(DepzError::Sensor("not used".into()))
    }
    fn set_range_timing(&mut self, _budget_ms: u32, _inter_ms: u32) -> Result<()> {
        Ok(())
    }
    fn get_range_timing(&mut self) -> Result<(u32, u32)> {
        Ok((33, 0))
    }
}

fn core() -> Core {
    Core {
        polls: 0,
        ready_after: 0,
    }
}

#[test]
fn defaults_describe_a_plain_driver() {
    let mut d = core();
    assert_eq!(d.addr_width(), 2);
    assert!(d.clear_steps().is_empty());
    assert_eq!(d.max_khz(), 400);
    assert!(d.supports().is_empty());
    assert!(d.modes().is_empty());
    assert_eq!(d.budget_ms(), (10, 200));
    assert!(!d.histogram());
    assert!(d.budget_choices().is_empty());
    assert_eq!(d.reach_mm().unwrap(), None);
    assert!(d.driver_info().unwrap().is_empty());
}

#[test]
fn wait_data_ready_polls_until_ready_or_times_out() {
    let mut d = Core {
        polls: 0,
        ready_after: 3,
    };
    d.wait_data_ready(Duration::from_secs(1)).unwrap();
    assert_eq!(d.polls, 4);

    let mut never = Core {
        polls: 0,
        ready_after: u32::MAX,
    };
    match never.wait_data_ready(Duration::from_millis(5)) {
        Err(DepzError::Sensor(m)) => assert!(m.contains("timeout"), "{m}"),
        other => panic!("expected a timeout, got {other:?}"),
    }
}

/// `r` is Unsupported and names the product, the driver and `group`.
fn assert_unsupported<T: std::fmt::Debug>(r: Result<T>, group: &str) {
    match r {
        Err(DepzError::Unsupported(m)) => {
            assert!(m.contains("vl53l1cx") && m.contains("CoreOnly"), "{m}");
            assert!(m.contains(group), "{m} should name {group}");
        }
        other => panic!("{group}: expected Unsupported, got {other:?}"),
    }
}

#[test]
fn every_optional_group_is_unsupported() {
    let mut d = core();
    assert_unsupported(d.set_mode("short"), "'mode'");
    assert_unsupported(d.get_mode(), "'mode'");
    assert_unsupported(d.get_offset(), "'offset'");
    assert_unsupported(d.set_offset(10), "'offset'");
    assert_unsupported(d.get_xtalk(), "'xtalk'");
    assert_unsupported(d.set_xtalk(10), "'xtalk'");
    assert_unsupported(d.calibrate_offset(100, None), "'calib_offset'");
    assert_unsupported(d.calibrate_xtalk(600, Some(10)), "'calib_xtalk'");
    assert_unsupported(d.get_detection_thresholds(), "'thresholds'");
    assert_unsupported(d.set_detection_thresholds(100, 300, 0), "'thresholds'");
    assert_unsupported(d.get_signal_threshold(), "'signal_thresh'");
    assert_unsupported(d.set_signal_threshold(1024), "'signal_thresh'");
    assert_unsupported(d.get_sigma_threshold(), "'sigma_thresh'");
    assert_unsupported(d.set_sigma_threshold(15), "'sigma_thresh'");
    assert_unsupported(d.get_roi(), "'roi'");
    assert_unsupported(d.set_roi(8, 8), "'roi'");
    assert_unsupported(d.get_roi_center(), "'roi'");
    assert_unsupported(d.set_roi_center(199), "'roi'");
    assert_unsupported(d.start_temperature_update(), "'temp_update'");
    assert_unsupported(d.perform_ref_spad_management(), "'refspad'");
}

#[test]
fn the_histogram_calls_need_the_histogram_driver() {
    let mut d = core();
    match d.bin_data(None) {
        Err(DepzError::Unsupported(m)) => assert!(m.contains("CoreOnly has no histogram"), "{m}"),
        other => panic!("expected Unsupported, got {:?}", other.map(|_| ())),
    }
    let bins: Box<dyn std::any::Any + Send> = Box::new(());
    match d.to_measurement(&bins) {
        Err(DepzError::Unsupported(m)) => assert!(m.contains("CoreOnly has no histogram"), "{m}"),
        other => panic!("expected Unsupported, got {:?}", other.map(|_| ())),
    }
}
