//! BNO086 SH-2 decode layer (contract 05).
//!
//! - [`shtp`] — SHTP framing + per-channel cargo reassembly.
//! - [`reports`] — SH-2 input-report parsers (Q points + 0xFB timebase).
//! - [`sh2`] — SH-2 control-channel request builders.
//! - [`hub`] — the bridge codecs, the control-response decoders, the FRS sessions and the
//!   request parameters of the SH-2 hub session.

pub mod hub;
pub mod reports;
pub mod sh2;
pub mod shtp;

pub use reports::{
    parse_gyro_rv_cargo, parse_input_cargo, try_parse_gyro_rv_cargo, try_parse_input_cargo, Report, Vector3Kind,
};
pub use shtp::{ExecutableCommand, ShtpCargo, ShtpChannel, ShtpError, ShtpHeader, ShtpLayer};
