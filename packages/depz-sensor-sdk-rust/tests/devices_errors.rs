//! The live layer's errors (contract 07 §4): every `DepzError` variant's message and its canonical
//! contract 15 §4 kind — `timeout`, `device_error`, `not_supported` (ERR_CMD_NOT_SUPPORTED and an
//! unsupported call), `invalid_argument`, `invalid_reply`; the rest have none.

use std::error::Error;
use std::time::Duration;

use depz_sensor_sdk::devices::DepzError;
use depz_sensor_sdk::protocol::common::{CodecError, Status};

fn io(msg: &str) -> std::io::Error {
    std::io::Error::new(std::io::ErrorKind::PermissionDenied, msg.to_string())
}

#[test]
fn every_variant_has_its_message_and_kind() {
    let cases: Vec<(DepzError, &str, Option<&str>)> = vec![
        (
            DepzError::timeout(0x21, Duration::from_millis(200)),
            "timeout: cmd 0x21: no reply within 0.200s",
            Some("timeout"),
        ),
        (
            DepzError::Timeout {
                cmd: None,
                message: "no frame within 2.000s".into(),
            },
            "timeout: no frame within 2.000s",
            Some("timeout"),
        ),
        (
            DepzError::Status {
                cmd: 0x30,
                status: Status::ErrBusy as u8,
            },
            "cmd 0x30: busy (ERR_BUSY or already in flight)",
            Some("device_error"),
        ),
        (
            DepzError::Status {
                cmd: 0x30,
                status: Status::ErrCmdNotSupported as u8,
            },
            "cmd 0x30: device status 0x07",
            Some("not_supported"),
        ),
        (
            DepzError::Status {
                cmd: 0xA2,
                status: 0x03,
            },
            "cmd 0xA2: device status 0x03",
            Some("device_error"),
        ),
        (
            DepzError::DeviceLost("unplugged".into()),
            "device lost: unplugged",
            None,
        ),
        (
            DepzError::LinkClosed("closed".into()),
            "link closed: closed",
            None,
        ),
        (
            DepzError::NoDevice("nothing on COM9".into()),
            "no DEPZ device: nothing on COM9",
            None,
        ),
        (
            DepzError::BootloaderMode,
            "the device runs its bootloader",
            None,
        ),
        (
            DepzError::WrongType("the device is not an SR04 (bno055)".into()),
            "the device is not an SR04 (bno055)",
            None,
        ),
        (
            DepzError::ReplayMismatch("tx byte 12 differs".into()),
            "tx byte 12 differs",
            None,
        ),
        (
            DepzError::InvalidArgument("rate 0".into()),
            "invalid argument: rate 0",
            Some("invalid_argument"),
        ),
        (
            DepzError::Unsupported("no histogram".into()),
            "not supported: no histogram",
            Some("not_supported"),
        ),
        (
            DepzError::InvalidState("stop first".into()),
            "invalid state: stop first",
            None,
        ),
        (
            DepzError::Protocol("short reply".into()),
            "protocol error: short reply",
            Some("invalid_reply"),
        ),
        (
            DepzError::Sensor("driver status -2".into()),
            "driver status -2",
            None,
        ),
        (
            DepzError::Io(io("port in use")),
            "I/O error: port in use",
            None,
        ),
    ];
    for (e, text, kind) in cases {
        assert_eq!(e.to_string(), text, "{e:?}");
        assert_eq!(e.kind(), kind, "{e:?}");
    }
}

#[test]
fn only_err_busy_is_busy() {
    let status = |s: u8| DepzError::Status {
        cmd: 0x30,
        status: s,
    };
    assert!(status(Status::ErrBusy as u8).is_busy());
    assert!(!status(Status::ErrCmdNotSupported as u8).is_busy());
    assert!(!DepzError::timeout(0x30, Duration::from_secs(1)).is_busy());
}

#[test]
fn conversions_keep_the_cause() {
    let e: DepzError = io("denied").into();
    assert!(matches!(e, DepzError::Io(_)));
    assert_eq!(e.source().map(|s| s.to_string()).as_deref(), Some("denied"));
    assert!(DepzError::Protocol("x".into()).source().is_none());

    let e: DepzError = CodecError("payload too short").into();
    assert!(matches!(&e, DepzError::Protocol(m) if m == "payload too short"));
    assert_eq!(e.kind(), Some("invalid_reply"));
}
