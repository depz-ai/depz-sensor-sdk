//! DEPZ USB identity table (contracts/02_COMMON_COMMANDS.md §4).
//!
//! One-edit-to-update table of the USB VID/PID values the DEPZ sensor line ships
//! with, used by discovery to pick the right serial port. The PID→model map is
//! an informational hint only; the protocol probe is authoritative.

/// Production VID shared by every DEPZ sensor (0x1BCF).
pub const DEPZ_USB_VID: u16 = 0x1BCF;

/// Per-model production PIDs (block 60536+, ascending).
pub const PID_SR04: u16 = 0xEC78; // 60536 — HC-SR04 ultrasonic
pub const PID_VL53L8CH: u16 = 0xED40; // 60736 — VL53L8CH ToF
pub const PID_VL53L4CD: u16 = 0xED45; // 60741 — VL53L4CD ToF
pub const PID_VL53L8CX: u16 = 0xED4B; // 60747 — VL53L8CX ToF (hw-verified)
pub const PID_BNO086: u16 = 0xEE08; // 60936 — BNO086 IMU

/// Inclusive PID range under `DEPZ_USB_VID` treated as candidate DEPZ sensors.
pub const DEPZ_PID_RANGE: (u16, u16) = (60536, 65535);

/// Dev / unprogrammed default: STMicroelectronics VID/PID.
pub const DEV_USB_VID: u16 = 0x0483; // 1155
pub const DEV_USB_PID: u16 = 0x56DC; // 22236

/// PID → sensor-model hint. Informational: the protocol probe is authoritative.
const PID_MODEL: &[(u16, &str)] = &[
    (PID_SR04, "sr04"),
    (PID_VL53L8CH, "vl53l8ch"),
    (0xED41, "vl53l0x"),  // 60737
    (0xED42, "vl53l1cb"), // 60738
    (0xED43, "vl53l1cx"), // 60739
    (0xED44, "vl53l3cx"), // 60740
    (PID_VL53L4CD, "vl53l4cd"),
    (0xED46, "vl53l4cx"), // 60742
    (0xED47, "vl53l4ed"), // 60743
    (0xED48, "vl53l5cx"), // 60744
    (0xED49, "vl53l7cx"), // 60745
    (0xED4A, "vl53l7ch"), // 60746
    (PID_VL53L8CX, "vl53l8cx"),
    (PID_BNO086, "bno086"),
    (0xEE09, "bno085"), // 60937
    (0xEE0A, "bno055"), // 60938
];

fn model_for_pid(pid: u16) -> Option<&'static str> {
    PID_MODEL
        .iter()
        .find(|(p, _)| *p == pid)
        .map(|(_, m)| *m)
}

/// True when `(vid, pid)` is a recognized DEPZ (or dev-default) USB id.
pub fn is_known_depz_usb(vid: Option<u16>, pid: Option<u16>) -> bool {
    let (vid, pid) = match (vid, pid) {
        (Some(v), Some(p)) => (v, p),
        _ => return false,
    };
    if vid == DEV_USB_VID && pid == DEV_USB_PID {
        return true;
    }
    if vid != DEPZ_USB_VID {
        return false;
    }
    if model_for_pid(pid).is_some() {
        return true;
    }
    let (lo, hi) = DEPZ_PID_RANGE;
    lo <= pid && pid <= hi
}

/// Best-guess model name for a `(vid, pid)`, or `None`. Informational only.
pub fn usb_model_hint(vid: Option<u16>, pid: Option<u16>) -> Option<&'static str> {
    let (vid, pid) = match (vid, pid) {
        (Some(v), Some(p)) => (v, p),
        _ => return None,
    };
    if vid == DEV_USB_VID && pid == DEV_USB_PID {
        return Some("dev");
    }
    if vid == DEPZ_USB_VID {
        return model_for_pid(pid);
    }
    None
}

/// A candidate serial port for discovery ordering.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct PortEntry {
    pub port: String,
    pub serial: Option<String>,
}

/// Order candidate ports by USB iSerial ascending; `None`/empty serials sort
/// last, tie-broken by port path (contract 02 §4).
pub fn order_ports(mut ports: Vec<PortEntry>) -> Vec<PortEntry> {
    ports.sort_by(|a, b| {
        let a_missing = a.serial.as_deref().map_or(true, |s| s.is_empty());
        let b_missing = b.serial.as_deref().map_or(true, |s| s.is_empty());
        let a_key = if a_missing { "" } else { a.serial.as_deref().unwrap() };
        let b_key = if b_missing { "" } else { b.serial.as_deref().unwrap() };
        (a_missing, a_key, &a.port).cmp(&(b_missing, b_key, &b.port))
    });
    ports
}
