//! Opening devices without a port (contract 02 §4.4, contract 07 §1–2): `attach` to every class
//! name, the `AnyDevice` accessors and `into_*` conversions, the refusals of `attach`, and the
//! `open_device_info` / `probe_port` paths that fail before or at the port. No real port is opened:
//! `open_first_device` would open the first DEPZ board plugged in, so it is not called here.
mod common;

use std::time::Duration;

use common::fake_board::FakeBoard;
use depz_sensor_sdk::devices::{
    attach, open_device_info, probe_port, AnyDevice, DepzDevice, DepzError, DeviceInfo,
};
use depz_sensor_sdk::discovery::SENSOR_CLASS_NAMES;
use depz_sensor_sdk::protocol::identity::{Mode, SensorType};

const SHORT: Option<Duration> = Some(Duration::from_millis(50));

#[cfg(windows)]
const NO_SUCH_PORT: &str = "COM254";
#[cfg(not(windows))]
const NO_SUCH_PORT: &str = "/dev/depz-no-such-port";

/// `class` attached to a fresh fake board (the board kept to check it closes).
fn attached(class: &str) -> (AnyDevice, FakeBoard) {
    let board = FakeBoard::new();
    let dev = attach(DepzDevice::new(board.clone(), SHORT), class).unwrap();
    (dev, board)
}

/// The classes `attach` gives a device layer for.
const CLASSES: [&str; 16] = [
    "sr04", "vl53l4cd", "vl53l8cx", "vl53l8ch", "vl53l5cx", "vl53l7cx", "vl53l7ch", "vl53l0x",
    "vl53l1cx", "vl53l1cb", "vl53l3cx", "vl53l4cx", "vl53lx", "bno086", "bno055", "tmf8829",
];

#[test]
fn attach_gives_every_class_its_own_variant_on_the_same_connection() {
    for class in CLASSES.iter().chain(&["device"]) {
        let (dev, board) = attached(class);
        assert_eq!(dev.class_name(), *class);
        assert!(!dev.device().is_closed(), "{class}");
        dev.device().close();
        assert!(
            board.is_closed_for_test(),
            "{class}: device() is the attached connection"
        );
        assert_eq!(board.sent_count(), 0, "{class}: attaching sends nothing");
    }
    // Every name the resolver returns is either a class above, "device" or "bootloader".
    for name in SENSOR_CLASS_NAMES {
        assert!(
            CLASSES.contains(&name) || name == "device" || name == "bootloader",
            "{name}"
        );
    }
}

#[test]
fn attach_refuses_the_bootloader_and_names_it_does_not_know() {
    let board = FakeBoard::new();
    match attach(DepzDevice::new(board.clone(), SHORT), "vl53l9x") {
        Err(DepzError::InvalidArgument(m)) => assert_eq!(m, "no sensor class 'vl53l9x'"),
        other => panic!("{:?}", other.map(|d| d.class_name())),
    }
    assert!(matches!(
        attach(DepzDevice::new(FakeBoard::new(), SHORT), "bootloader"),
        Err(DepzError::BootloaderMode)
    ));
}

/// `into_<class>` of `dev`: `Ok(class)` or the error.
fn into(dev: AnyDevice, class: &str) -> Result<&'static str, DepzError> {
    Ok(match class {
        "sr04" => dev.into_sr04().map(|_| "sr04")?,
        "vl53l4cd" => dev.into_vl53l4cd().map(|_| "vl53l4cd")?,
        "bno055" => dev.into_bno055().map(|_| "bno055")?,
        "bno086" => dev.into_bno086().map(|_| "bno086")?,
        "vl53l8cx" => dev.into_vl53l8cx().map(|_| "vl53l8cx")?,
        "vl53l8ch" => dev.into_vl53l8ch().map(|_| "vl53l8ch")?,
        "vl53l5cx" => dev.into_vl53l5cx().map(|_| "vl53l5cx")?,
        "vl53l7cx" => dev.into_vl53l7cx().map(|_| "vl53l7cx")?,
        "vl53l7ch" => dev.into_vl53l7ch().map(|_| "vl53l7ch")?,
        "tmf8829" => dev.into_tmf8829().map(|_| "tmf8829")?,
        "vl53lx" => dev.into_vl53lx().map(|_| "vl53lx")?,
        other => panic!("no into_ for {other}"),
    })
}

const INTO: [(&str, &str); 11] = [
    ("sr04", "an SR04"),
    ("vl53l4cd", "a VL53L4CD"),
    ("bno055", "a BNO055"),
    ("bno086", "a BNO085 / BNO086"),
    ("vl53l8cx", "a VL53L8CX"),
    ("vl53l8ch", "a VL53L8CH"),
    ("vl53l5cx", "a VL53L5CX"),
    ("vl53l7cx", "a VL53L7CX"),
    ("vl53l7ch", "a VL53L7CH"),
    ("tmf8829", "a TMF8829"),
    ("vl53lx", "a VL53L 1D-family board"),
];

#[test]
fn into_a_class_takes_its_own_variant_and_refuses_the_others() {
    for (target, what) in INTO {
        assert_eq!(into(attached(target).0, target).unwrap(), target);
        for other in ["device", "sr04", "bno055"] {
            if other == target {
                continue;
            }
            match into(attached(other).0, target) {
                Err(DepzError::WrongType(m)) => {
                    assert_eq!(m, format!("the device is not {what} ({other})"))
                }
                r => panic!("{other} into {target}: {r:?}"),
            }
        }
    }
}

#[test]
fn every_vl53l_1d_product_class_converts_to_the_family_class() {
    for class in [
        "vl53l0x", "vl53l1cx", "vl53l1cb", "vl53l3cx", "vl53l4cx", "vl53lx",
    ] {
        let (dev, board) = attached(class);
        let family = dev.into_vl53lx().unwrap();
        assert!(!family.is_closed(), "{class}");
        family.close();
        assert!(
            board.is_closed_for_test(),
            "{class}: the family class keeps the connection"
        );
    }
    // The 2D VL53L sensors are not of that family.
    assert!(matches!(
        attached("vl53l8cx").0.into_vl53lx(),
        Err(DepzError::WrongType(_))
    ));
}

fn info(port: &str, software_name: &str, vid: Option<u16>, pid: Option<u16>) -> DeviceInfo {
    DeviceInfo {
        port: port.into(),
        mode: Mode::App,
        sensor_type: Some(SensorType::Sr04),
        software_name: software_name.into(),
        fw_version: "0.97".into(),
        device_name: String::new(),
        serial_number: String::new(),
        usb_vid: vid,
        usb_pid: pid,
        usb_serial: None,
    }
}

#[test]
fn device_info_names_the_usb_model_of_its_port() {
    // contracts/vectors/usb_ids.json identity #0, #6, the dev default and an unknown pid.
    let hint = |vid, pid| info(NO_SUCH_PORT, "", vid, pid).usb_model_hint();
    assert_eq!(hint(Some(0x1BCF), Some(60536)), Some("sr04"));
    assert_eq!(hint(Some(0x1BCF), Some(60741)), Some("vl53l4cd"));
    assert_eq!(hint(Some(0x0483), Some(0x56DC)), Some("dev"));
    assert_eq!(hint(Some(0x0483), Some(0x5740)), None);
    assert_eq!(hint(None, Some(60536)), None);
}

#[test]
fn opening_a_listed_device_fails_before_or_at_the_port() {
    // A bootloader's answers resolve to "bootloader": refused before the port is opened.
    let boot = info(NO_SUCH_PORT, "BOOTDEPZ_v1.11", Some(0x1BCF), Some(60536));
    assert!(matches!(
        open_device_info(&boot, SHORT),
        Err(DepzError::BootloaderMode)
    ));
    // An application's answers open the port as its class: a port that is gone is DeviceLost.
    let app = info(
        NO_SUCH_PORT,
        "APP_usonic_SR04_v0.97",
        Some(0x1BCF),
        Some(60536),
    );
    match open_device_info(&app, SHORT) {
        Err(DepzError::DeviceLost(m)) => assert!(m.contains(NO_SUCH_PORT), "{m}"),
        other => panic!("{:?}", other.map(|d| d.class_name())),
    }
}

#[test]
fn probing_a_port_that_is_not_there_finds_nothing() {
    assert_eq!(probe_port(NO_SUCH_PORT, SHORT), None);
}
