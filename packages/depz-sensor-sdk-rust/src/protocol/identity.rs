//! Firmware-name parsing (contracts/02_COMMON_COMMANDS.md §4).

/// Product family classified from the active-software name.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum SensorType {
    Sr04,
    Vl53l4,
    Vl53l8,
    Bno086,
    Unknown,
}

impl SensorType {
    /// Lowercase wire-string form (matches the golden vectors).
    pub fn as_str(&self) -> &'static str {
        match self {
            SensorType::Sr04 => "sr04",
            SensorType::Vl53l4 => "vl53l4",
            SensorType::Vl53l8 => "vl53l8",
            SensorType::Bno086 => "bno086",
            SensorType::Unknown => "unknown",
        }
    }
}

/// Firmware operating mode.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum Mode {
    App,
    Bootloader,
    Unknown,
}

impl Mode {
    pub fn as_str(&self) -> &'static str {
        match self {
            Mode::App => "app",
            Mode::Bootloader => "bootloader",
            Mode::Unknown => "unknown",
        }
    }
}

/// Classified `GET_NAME_ACTIVE_SOFTWARE` result.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct Identity {
    pub mode: Mode,
    /// `None` in bootloader/unknown mode.
    pub sensor_type: Option<SensorType>,
    pub software_name: String,
    /// `""` when not parseable.
    pub version: String,
}

const PRODUCT_TOKENS: &[(&str, SensorType)] = &[
    ("SR04", SensorType::Sr04),
    ("VL53L4", SensorType::Vl53l4),
    ("VL53L8", SensorType::Vl53l8),
    ("BNO086", SensorType::Bno086),
];

/// Match `_v(\d+(\.\d+)*)$`: find the leftmost `_v` whose entire remainder is a
/// dotted numeric version ending at the string end. Returns the captured version.
fn extract_version(name: &str) -> String {
    let bytes = name.as_bytes();
    if bytes.len() < 2 {
        return String::new();
    }
    for i in 0..=bytes.len() - 2 {
        if &bytes[i..i + 2] == b"_v" {
            let rest = &name[i + 2..];
            if is_dotted_version(rest) {
                return rest.to_string();
            }
        }
    }
    String::new()
}

/// `\d+(\.\d+)*` matched against the whole string.
fn is_dotted_version(s: &str) -> bool {
    if s.is_empty() {
        return false;
    }
    let mut prev_dot = true; // require a digit at the start of each group
    for c in s.chars() {
        if c == '.' {
            if prev_dot {
                return false; // empty group / leading dot / double dot
            }
            prev_dot = true;
        } else if c.is_ascii_digit() {
            prev_dot = false;
        } else {
            return false;
        }
    }
    !prev_dot // must not end on a dot
}

/// Classify an already-stripped active-software string.
pub fn parse_software_name(name: &str) -> Identity {
    let version = extract_version(name);
    if name.starts_with("BOOTDEPZ") {
        return Identity {
            mode: Mode::Bootloader,
            sensor_type: None,
            software_name: name.to_string(),
            version,
        };
    }
    if name.starts_with("APP_") {
        for (token, sensor) in PRODUCT_TOKENS {
            if name.contains(token) {
                return Identity {
                    mode: Mode::App,
                    sensor_type: Some(*sensor),
                    software_name: name.to_string(),
                    version,
                };
            }
        }
        return Identity {
            mode: Mode::App,
            sensor_type: Some(SensorType::Unknown),
            software_name: name.to_string(),
            version,
        };
    }
    Identity {
        mode: Mode::Unknown,
        sensor_type: None,
        software_name: name.to_string(),
        version,
    }
}
