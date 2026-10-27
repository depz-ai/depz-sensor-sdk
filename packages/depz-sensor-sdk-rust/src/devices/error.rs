//! The live layer's errors (contract 07 §4).

use std::fmt;
use std::time::Duration;

use crate::protocol::common::Status;

/// What a live-device call can fail with. Arguments the SDK refuses before sending anything are
/// [`DepzError::InvalidArgument`].
#[derive(Debug)]
#[non_exhaustive]
pub enum DepzError {
    /// No reply (or no stream item) in time. `cmd` is the request or report id waited for.
    Timeout { cmd: Option<u8>, message: String },
    /// The device answered RPT_STATUS with a non-OK status for `cmd` (ERR_BUSY: [`DepzError::is_busy`]).
    Status { cmd: u8, status: u8 },
    /// The device is gone (unplugged, the link failed) or the stream ended.
    DeviceLost(String),
    /// The link was closed by this side.
    LinkClosed(String),
    /// Nothing DEPZ answered.
    NoDevice(String),
    /// The device runs its bootloader, not an application.
    BootloaderMode,
    /// The device is not of the class asked for.
    WrongType(String),
    /// A strict replay saw the host write bytes the capture does not hold.
    ReplayMismatch(String),
    /// The SDK refused an argument before sending anything.
    InvalidArgument(String),
    /// The product / driver does not serve this.
    Unsupported(String),
    /// Not in this state (e.g. configuring while streaming).
    InvalidState(String),
    /// A reply that does not decode.
    Protocol(String),
    /// A sensor-level failure (a driver status, a refused command).
    Sensor(String),
    /// The OS refused (a port, a file).
    Io(std::io::Error),
}

/// `Result` with [`DepzError`].
pub type Result<T> = std::result::Result<T, DepzError>;

impl DepzError {
    /// A request timeout for `cmd` after `timeout`.
    pub fn timeout(cmd: u8, timeout: Duration) -> DepzError {
        DepzError::Timeout {
            cmd: Some(cmd),
            message: format!(
                "cmd 0x{cmd:02X}: no reply within {:.3}s",
                timeout.as_secs_f64()
            ),
        }
    }

    /// RPT_STATUS ERR_BUSY (or a second request of an opcode already in flight).
    pub fn is_busy(&self) -> bool {
        matches!(self, DepzError::Status { status, .. } if *status == Status::ErrBusy as u8)
    }

    /// The canonical error kind of contract 15 §4, or `None` for the rest.
    pub fn kind(&self) -> Option<&'static str> {
        match self {
            DepzError::Timeout { .. } => Some("timeout"),
            DepzError::Status { status, .. } if *status == Status::ErrCmdNotSupported as u8 => {
                Some("not_supported")
            }
            DepzError::Status { .. } => Some("device_error"),
            DepzError::InvalidArgument(_) => Some("invalid_argument"),
            DepzError::Unsupported(_) => Some("not_supported"),
            DepzError::Protocol(_) => Some("invalid_reply"),
            _ => None,
        }
    }
}

impl fmt::Display for DepzError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            DepzError::Timeout { message, .. } => write!(f, "timeout: {message}"),
            DepzError::Status { cmd, status } if *status == Status::ErrBusy as u8 => {
                write!(f, "cmd 0x{cmd:02X}: busy (ERR_BUSY or already in flight)")
            }
            DepzError::Status { cmd, status } => {
                write!(f, "cmd 0x{cmd:02X}: device status 0x{status:02X}")
            }
            DepzError::DeviceLost(m) => write!(f, "device lost: {m}"),
            DepzError::LinkClosed(m) => write!(f, "link closed: {m}"),
            DepzError::NoDevice(m) => write!(f, "no DEPZ device: {m}"),
            DepzError::BootloaderMode => write!(f, "the device runs its bootloader"),
            DepzError::WrongType(m) => write!(f, "{m}"),
            DepzError::ReplayMismatch(m) => write!(f, "{m}"),
            DepzError::InvalidArgument(m) => write!(f, "invalid argument: {m}"),
            DepzError::Unsupported(m) => write!(f, "not supported: {m}"),
            DepzError::InvalidState(m) => write!(f, "invalid state: {m}"),
            DepzError::Protocol(m) => write!(f, "protocol error: {m}"),
            DepzError::Sensor(m) => write!(f, "{m}"),
            DepzError::Io(e) => write!(f, "I/O error: {e}"),
        }
    }
}

impl std::error::Error for DepzError {
    fn source(&self) -> Option<&(dyn std::error::Error + 'static)> {
        match self {
            DepzError::Io(e) => Some(e),
            _ => None,
        }
    }
}

impl From<std::io::Error> for DepzError {
    fn from(e: std::io::Error) -> Self {
        DepzError::Io(e)
    }
}

impl From<crate::protocol::common::CodecError> for DepzError {
    fn from(e: crate::protocol::common::CodecError) -> Self {
        DepzError::Protocol(e.0.to_string())
    }
}
