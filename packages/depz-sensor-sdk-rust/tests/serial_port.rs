//! The serial-port layer without hardware: enumeration, ordering, and the errors of a port that is
//! not there. No real port is opened (the hardware paths run with a board plugged in only).

use std::time::Duration;

use depz_sensor_sdk::devices::{
    by_serial, find_port, list_depz_devices, list_serial_ports, open_device, open_device_at,
    open_device_by_serial, DepzDevice, DepzError, Link, SerialPortInfo, SerialPortLink,
    DEFAULT_BAUD_RATE,
};

const SHORT: Option<Duration> = Some(Duration::from_millis(50));

#[cfg(windows)]
const NO_SUCH_PORT: &str = "COM254";
#[cfg(not(windows))]
const NO_SUCH_PORT: &str = "/dev/depz-no-such-port";

fn info(port: &str, serial: Option<&str>) -> SerialPortInfo {
    SerialPortInfo {
        port: port.into(),
        vid: Some(0x1BCF),
        pid: Some(0xED4B),
        usb_serial: serial.map(str::to_string),
    }
}

#[test]
fn listing_ports_returns_without_error() {
    let ports = list_serial_ports().expect("the OS enumerates serial ports");
    for p in &ports {
        assert!(!p.port.is_empty());
        assert_eq!(
            p.vid.is_some(),
            p.pid.is_some(),
            "{p:?}: a USB port has both ids"
        );
        if p.vid.is_none() {
            assert!(!p.is_depz());
        }
    }
}

#[test]
fn ports_sort_by_usb_serial_missing_last_ties_by_path() {
    let sorted = by_serial(vec![
        info("/dev/ttyACM3", None),
        info("/dev/ttyACM2", Some("DEPZ-0002")),
        info("/dev/ttyACM1", Some("")),
        info("/dev/ttyACM0", Some("DEPZ-0001")),
        info("/dev/ttyACM4", Some("DEPZ-0002")),
    ]);
    let order: Vec<&str> = sorted.iter().map(|p| p.port.as_str()).collect();
    assert_eq!(
        order,
        [
            "/dev/ttyACM0",
            "/dev/ttyACM2",
            "/dev/ttyACM4",
            "/dev/ttyACM1",
            "/dev/ttyACM3"
        ]
    );
}

#[test]
fn port_info_knows_the_depz_ids() {
    assert!(info("x", None).is_depz());
    assert_eq!(info("x", None).usb_model_hint(), Some("vl53l8cx"));
    let other = SerialPortInfo {
        port: "x".into(),
        vid: Some(0x0403),
        pid: Some(0x6001),
        usb_serial: None,
    };
    assert!(!other.is_depz());
    assert_eq!(other.usb_model_hint(), None);
    assert_eq!(DEFAULT_BAUD_RATE, 115_200);
}

#[test]
fn find_port_matches_the_exact_path_and_nothing_the_os_does_not_list() {
    let ports = vec![
        info("/dev/ttyACM0", Some("A")),
        info("/dev/ttyACM1", Some("B")),
    ];
    assert_eq!(find_port(&ports, "/dev/ttyACM1"), Some(&ports[1]));
    assert_eq!(find_port(&ports, "/dev/depz-unlisted"), None);
    assert_eq!(find_port(&[], NO_SUCH_PORT), None);
}

/// A `/dev/serial/by-id/…` symlink names the port it points at (ERRATA A5): a regular file
/// stands in for the node, the resolved path matches either way.
#[cfg(unix)]
#[test]
fn find_port_follows_a_symlink_alias_to_the_enumerated_port() {
    let dir = std::env::temp_dir().join(format!("depz-a5-{}", std::process::id()));
    std::fs::create_dir_all(&dir).unwrap();
    let node = dir.join("ttyACM0");
    let alias = dir.join("usb-DEPZ_ToF-if00");
    std::fs::write(&node, b"").unwrap();
    std::os::unix::fs::symlink(&node, &alias).unwrap();
    let ports = vec![
        info("/dev/ttyACM9", Some("Z")),
        info(node.to_str().unwrap(), Some("SN1")),
    ];
    let found = find_port(&ports, alias.to_str().unwrap());
    std::fs::remove_dir_all(&dir).unwrap();
    assert_eq!(found, Some(&ports[1]));
}

#[test]
fn a_missing_port_is_device_lost() {
    match SerialPortLink::open(NO_SUCH_PORT) {
        Err(DepzError::DeviceLost(m)) => assert!(m.contains(NO_SUCH_PORT), "{m}"),
        other => panic!("expected DeviceLost, got {:?}", other.map(|l| l.name())),
    }
    assert!(matches!(
        DepzDevice::open(NO_SUCH_PORT, SHORT),
        Err(DepzError::DeviceLost(_))
    ));
    assert!(matches!(
        open_device(NO_SUCH_PORT, SHORT),
        Err(DepzError::DeviceLost(_))
    ));
}

#[test]
fn candidates_nobody_has_are_no_device() {
    match open_device_by_serial("DEPZ-serial-nobody-has", SHORT) {
        Err(DepzError::NoDevice(m)) => assert!(m.contains("DEPZ-serial-nobody-has"), "{m}"),
        other => panic!("expected NoDevice, got {:?}", other.map(|d| d.class_name())),
    }
    match open_device_at(usize::MAX, SHORT) {
        Err(DepzError::NoDevice(m)) => assert!(m.contains("candidate"), "{m}"),
        other => panic!("expected NoDevice, got {:?}", other.map(|d| d.class_name())),
    }
}

#[test]
fn listing_devices_on_an_explicit_missing_port_finds_nothing() {
    // An explicit list probes those ports only: a port that is not there is skipped, not an error.
    let found = list_depz_devices(Some(&[NO_SUCH_PORT]), false, SHORT).unwrap();
    assert!(found.is_empty());
    // With the USB filter the same port (no USB id) is not even probed.
    let found = list_depz_devices(Some(&[NO_SUCH_PORT]), true, SHORT).unwrap();
    assert!(found.is_empty());
}
