//! Class resolution (contracts/02_COMMON_COMMANDS.md §4.4, Python reference
//! `resolve_sensor_class`): which SDK class a device opens as, from its probe
//! answers and its port's USB (vid, pid).
//!
//! The pure part of §4.4: [`resolve_sensor_class`] and the [`SENSOR_CLASS_NAMES`]
//! list. Port enumeration, the probe and opening live in [`crate::devices`]
//! (`list_depz_devices`, `open_device`).

use crate::protocol::identity::{parse_software_name, Mode, SensorType};
use crate::usb_ids::usb_model_hint;
use crate::{vl53l7, vl53lx};

/// Every name [`resolve_sensor_class`] returns: the contract 07 sensor
/// classes, `"vl53lx"` (the generic 1D-family class: an unstamped board, or a
/// VL53L4CD on the `APP_VL53L0_4` firmware), `"tmf8829"`, `"device"` (the
/// base device: an unknown application, or a name that is neither `APP_` nor
/// `BOOTDEPZ`) and `"bootloader"` (use the contract 06 client). BNO085 boards
/// run `APP_BNO086` and resolve to `"bno086"`.
pub const SENSOR_CLASS_NAMES: [&str; 18] = [
    "sr04",
    "vl53l4cd",
    "vl53l8cx",
    "vl53l8ch",
    "vl53l5cx",
    "vl53l7cx",
    "vl53l7ch",
    "vl53l0x",
    "vl53l1cx",
    "vl53l1cb",
    "vl53l3cx",
    "vl53l4cx",
    "vl53lx",
    "bno086",
    "bno055",
    "tmf8829",
    "device",
    "bootloader",
];

/// The canonical class name (one of [`SENSOR_CLASS_NAMES`]) for a device's
/// probe answers and its port's USB id — contract 02 §4.4, pure.
///
/// `software_name` is the `GET_NAME_ACTIVE_SOFTWARE` answer, `device_name`
/// the `GET_DEVICE_NAME` answer (`""` when there was none); the USB id is
/// `None` when unknown (a replay, a bridge).
///
/// - `BOOTDEPZ_*` → `"bootloader"`;
/// - `vl53l8` → `"vl53l8ch"` if the USB model is VL53L8CH, else `"vl53l8cx"`;
/// - `vl53l7` → [`vl53l7::resolve_model`] (USB model, else device name, else CX);
/// - `vl53lx` → [`vl53lx::resolve_class`] (USB model, else device name, else
///   the generic `"vl53lx"`; VL53L4CD on this firmware is generic too);
/// - any other known token → its class; anything else → `"device"`.
pub fn resolve_sensor_class(
    software_name: &str,
    device_name: &str,
    usb_vid: Option<u16>,
    usb_pid: Option<u16>,
) -> &'static str {
    let ident = parse_software_name(software_name);
    if ident.mode == Mode::Bootloader {
        return "bootloader";
    }
    let model = usb_model_hint(usb_vid, usb_pid);
    match ident.sensor_type {
        Some(SensorType::Sr04) => "sr04",
        // CX and CH share firmware name and silicon ids: only the production
        // PID tells them apart; the dev default (0483:56DC) falls to the CX base.
        Some(SensorType::Vl53l8) => {
            if model == Some("vl53l8ch") {
                "vl53l8ch"
            } else {
                "vl53l8cx"
            }
        }
        Some(SensorType::Vl53l7) => vl53l7::resolve_model(model, device_name).as_str(),
        Some(SensorType::Vl53lx) => match vl53lx::resolve_class(model, device_name) {
            vl53lx::Vl53lxClass::Vl53l0x => "vl53l0x",
            vl53lx::Vl53lxClass::Vl53l1cx => "vl53l1cx",
            vl53lx::Vl53lxClass::Vl53l1cb => "vl53l1cb",
            vl53lx::Vl53lxClass::Vl53l3cx => "vl53l3cx",
            vl53lx::Vl53lxClass::Vl53l4cx => "vl53l4cx",
            vl53lx::Vl53lxClass::Vl53lx => "vl53lx",
        },
        Some(SensorType::Vl53l4) => "vl53l4cd",
        Some(SensorType::Bno086) => "bno086",
        Some(SensorType::Bno055) => "bno055",
        Some(SensorType::Tmf8829) => "tmf8829",
        Some(SensorType::Unknown) | None => "device",
    }
}
