//! The light drivers of the 1D family: the VL53L1 die ULDs (L4CD ULD, L3CX ULP, L1X ULD), the
//! VL53L0X API, and the (product, kind) registry.
//!
//! - [`die`] — the VL53L1 die the three die drivers share (register map, result block, init, timing
//!   arithmetic, thresholds, offset, xtalk, ROI, temperature update).
//! - [`Vl53l4UldDriver`] (VL53L4CD ULD 2.2.3), [`Vl53l3UlpDriver`] (VL53L3CX ULP 1.0.0),
//!   [`Vl53l1UldDriver`] (VL53L1X ULD 3.5.5), [`Vl53l0xUldDriver`] (VL53L0X API 1.0.4).
//! - [`driver_kinds`] / [`create_driver`] — which (product, kind) pairs exist and their caveats.

pub mod die;
mod l0x;
mod l1;
mod l4;
mod registry;

pub use die::{Die, DieConfig, WINDOW_ABOVE, WINDOW_BELOW, WINDOW_IN, WINDOW_OUT};
pub use l0x::{isqrt, RefCalibration, RefSpads, Vl53l0xUldDriver, MODEL_ID_VL53L0X};
pub use l1::Vl53l1UldDriver;
pub use l4::{Vl53l3UlpDriver, Vl53l4UldDriver};
pub use registry::{create_driver, driver_kinds, DriverCtor, HIST_CAVEAT};
