//! The driver's error: a ULD status with its code, a refused argument, or a platform failure.

use std::fmt;

use crate::devices::DepzError;

/// The VL53L8 / L5 / L7 ULD status codes (`vl53l8cx_api.h`) the driver reports.
pub mod status {
    /// A poll ran out of time (2 s for a command, 500 ms for the MCU boot).
    pub const TIMEOUT_ERROR: u8 = 1;
    /// The header and footer ids of a frame differ (or the frame is too short to hold them).
    pub const CORRUPTED_FRAME: u8 = 2;
    /// The firmware flags a laser-safety fault after the ranging start.
    pub const LASER_SAFETY: u8 = 3;
    /// The downloaded sensor firmware does not answer with the expected checksum.
    pub const FW_CHECKSUM_FAIL: u8 = 5;
    /// The sensor MCU reports an error in the command status.
    pub const MCU_ERROR: u8 = 66;
    /// An argument the ULD refuses (its own range checks).
    pub const INVALID_PARAM: u8 = 127;
    /// A generic ULD failure (wrong silicon, unknown power mode, calibration that never ends).
    pub const ERROR: u8 = 255;

    /// The ULD name of a status code (`TIMEOUT`, `MCU_ERROR` …), or `0x..` for another code.
    pub fn name(code: u8) -> String {
        match code {
            TIMEOUT_ERROR => "TIMEOUT".into(),
            CORRUPTED_FRAME => "CORRUPTED_FRAME".into(),
            LASER_SAFETY => "LASER_SAFETY".into(),
            FW_CHECKSUM_FAIL => "FW_CHECKSUM_FAIL".into(),
            MCU_ERROR => "MCU_ERROR".into(),
            INVALID_PARAM => "INVALID_PARAM".into(),
            ERROR => "ERROR".into(),
            _ => format!("0x{code:x}"),
        }
    }
}

/// What a [`super::Vl53l8Driver`] call can fail with.
///
/// `?` turns it into [`DepzError`]: a ULD status becomes [`DepzError::Sensor`] with the status
/// name and code in the text (`"VL53L8CX TIMEOUT (code 1) sensor boot"`, as the other SDKs word
/// it), a refused argument [`DepzError::InvalidArgument`], and a platform failure the
/// [`DepzError`] the platform returned. Match on this type before converting when the status
/// code matters ([`Vl53l8UldError::status_code`]).
#[derive(Debug)]
pub enum Vl53l8UldError {
    /// A ULD status ([`status`] codes, or the GO2 error byte the sensor reports) and where it
    /// happened.
    Status {
        /// The status code.
        code: u8,
        /// The step that failed (`"sensor boot"`, `"dci read 0x5450"` …); may be empty.
        context: String,
    },
    /// An argument the SDK refuses before sending anything that the ULD itself would not catch (a
    /// field it would wrap): a Java `IllegalArgumentException`.
    InvalidArgument(String),
    /// The platform (the register bridge) failed.
    Platform(DepzError),
}

/// `Result` with [`Vl53l8UldError`].
pub type UldResult<T> = std::result::Result<T, Vl53l8UldError>;

impl Vl53l8UldError {
    /// A ULD status error.
    pub fn status(code: u8, context: impl Into<String>) -> Self {
        Vl53l8UldError::Status {
            code,
            context: context.into(),
        }
    }

    /// The ULD status code, for a [`Vl53l8UldError::Status`].
    pub fn status_code(&self) -> Option<u8> {
        match self {
            Vl53l8UldError::Status { code, .. } => Some(*code),
            _ => None,
        }
    }
}

impl fmt::Display for Vl53l8UldError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            Vl53l8UldError::Status { code, context } => {
                let text = format!(
                    "VL53L8CX {} (code {}) {}",
                    status::name(*code),
                    code,
                    context
                );
                f.write_str(text.trim())
            }
            Vl53l8UldError::InvalidArgument(m) => write!(f, "invalid argument: {m}"),
            Vl53l8UldError::Platform(e) => write!(f, "{e}"),
        }
    }
}

impl std::error::Error for Vl53l8UldError {
    fn source(&self) -> Option<&(dyn std::error::Error + 'static)> {
        match self {
            Vl53l8UldError::Platform(e) => Some(e),
            _ => None,
        }
    }
}

impl From<DepzError> for Vl53l8UldError {
    fn from(e: DepzError) -> Self {
        Vl53l8UldError::Platform(e)
    }
}

impl From<Vl53l8UldError> for DepzError {
    fn from(e: Vl53l8UldError) -> Self {
        match e {
            Vl53l8UldError::Status { .. } => DepzError::Sensor(e.to_string()),
            Vl53l8UldError::InvalidArgument(m) => DepzError::InvalidArgument(m),
            Vl53l8UldError::Platform(e) => e,
        }
    }
}
