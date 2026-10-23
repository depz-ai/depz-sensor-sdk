//! The live device layer (contract 07): a connection with its reader thread, requests and replies,
//! streams, events, time sync, `.depzrec` record / replay, and one class per sensor family.
//!
//! Every device runs on a [`Link`] — a replay of a capture ([`ReplayLink`]), a recording wrapper
//! ([`RecordingLink`]), or a byte pipe of the host's own — so every layer above the port is testable
//! without hardware. [`open_on_link`] probes a link and continues it as its sensor class.

mod bno055;
mod bno086;
mod device;
mod error;
mod link;
mod open;
mod sr04;
mod stream;
mod tmf8829;
mod vl53l4cd;
mod vl53l7;
mod vl53l8;
mod vl53lx;

pub use bno055::{Bno055Device, Bno055Sample, Bno055SystemStatus};
pub use bno086::{Bno086Device, RATE_HIGH_FACTOR, RATE_LOW_FACTOR};
pub use device::{
    expect_report, expect_text, host_now_us, DepzDevice, DeviceEvent, LinkStats, ReportHandler, TimeSyncResult,
    DEFAULT_TIMEOUT,
};
pub use error::{DepzError, Result};
pub use link::{Link, RecordingLink, ReplayLink, RECORDING_SCHEMA};
pub use open::{attach, open_on_link, probe, AnyDevice, ProbeResult};
pub use sr04::{Sr04Device, Sr04Measurement};
pub use stream::{Hook, Subscription};
pub use tmf8829::{Tmf8829AppliedConfig, Tmf8829Device, Tmf8829InitResult, Tmf8829Options};
pub use vl53l4cd::{Vl53l4Measurement, Vl53l4cdDevice};
pub use vl53l7::{Vl53l5cxDevice, Vl53l7chDevice, Vl53l7cxDevice};
pub use vl53l8::{BridgeUld, Vl53l8chDevice, Vl53l8cxDevice, MIN_RANGING_FREQUENCY_HZ as VL53L8_MIN_RANGING_FREQUENCY_HZ};
pub use vl53lx::{
    Vl53l0xDevice, Vl53l1cbDevice, Vl53l1cxDevice, Vl53l3cxDevice, Vl53l4cxDevice, Vl53lxDevice, Vl53lxMeasurement,
    Vl53lxProductInfo, PLOTTABLE_STATUSES,
};
