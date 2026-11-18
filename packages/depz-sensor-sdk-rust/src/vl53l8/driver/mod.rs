//! The VL53L8 / L5 / L7 live ULD driver (the ST ULD on the host over a register bridge).
//!
//! A faithful port of the reference Python driver (`vl53l8/uld.py`) by way of the Java port:
//! ST VL53L8CX ULD 2.1.0, VL53LMZ ULD 2.0.16 (VL53L8CH, VL53L7CH) and VL53L5CX/L7CX ULD 2.0.1.
//! [`Vl53l8Driver`] runs init (the 84 KB sensor firmware download, NVM offsets, xtalk and
//! configuration upload), resolution, frequency, mode, integration time, sharpener, target order,
//! ranging, frame parsing, power modes, xtalk margin and calibration, detection thresholds and the
//! motion indicator, over a [`Vl53l8Platform`] the device class maps onto the firmware bridge.
//! Register sequences are a 1:1 port — every request matches the Python one, so captures replay
//! in every SDK.
//!
//! - [`Vl53l8Platform`] — register access (16-bit addresses, auto-increment) and a delay.
//! - [`Vl53l8Firmware`] — which sensor firmware the driver downloads (cx, ch, l7cx, l7ch).
//! - [`Vl53l8Driver`] — the ULD itself.
//! - [`Vl53l8Frame`] / [`Vl53l8Motion`] — one decoded ranging frame, device level.
//! - [`Vl53l8CnhSetup`] — the CNH (compact network histogram) configuration of an L8CH / L7CH.
//! - [`Vl53l8UldError`] — a ULD status (with its numeric code), a refused argument, or a platform
//!   failure; converts into [`crate::devices::DepzError`] with `?`.
//! - [`tables`] — the ST command tables, generated from the Python SDK.
//!
//! The sensor firmware and configuration blobs live in `src/vl53l8/data` (copied from the Python
//! SDK by `scripts/sync_vl53l8_data.py`) and are compiled in with `include_bytes!`.

mod cnh_setup;
mod error;
mod firmware;
mod frame;
pub mod tables;
mod uld;

use std::sync::Arc;
use std::time::Duration;

use crate::devices::Result;

pub use cnh_setup::Vl53l8CnhSetup;
pub use error::{status, UldResult, Vl53l8UldError};
pub use firmware::Vl53l8Firmware;
pub use frame::{Vl53l8Frame, Vl53l8Motion};
pub use uld::*;

/// What the ULD needs from the board: register access through the bridge (16-bit addresses,
/// auto-increment) and a delay. The device class maps it onto the firmware bridge; a test can
/// script it. `Send + Sync`, so a driver over it can live in a device's `Mutex` and be called from
/// any thread.
pub trait Vl53l8Platform: Send + Sync {
    /// Read `size` bytes from `addr` on. Fewer bytes than asked is a failure the driver reports.
    fn read_multi(&self, addr: u16, size: usize) -> Result<Vec<u8>>;

    /// Write `data` from `addr` on.
    fn write_multi(&self, addr: u16, data: &[u8]) -> Result<()>;

    /// Wait `ms` milliseconds (the default sleeps the thread).
    fn sleep_ms(&self, ms: u32) {
        std::thread::sleep(Duration::from_millis(ms as u64));
    }
}

impl<T: Vl53l8Platform + ?Sized> Vl53l8Platform for Arc<T> {
    fn read_multi(&self, addr: u16, size: usize) -> Result<Vec<u8>> {
        (**self).read_multi(addr, size)
    }

    fn write_multi(&self, addr: u16, data: &[u8]) -> Result<()> {
        (**self).write_multi(addr, data)
    }

    fn sleep_ms(&self, ms: u32) {
        (**self).sleep_ms(ms)
    }
}

impl<T: Vl53l8Platform + ?Sized> Vl53l8Platform for Box<T> {
    fn read_multi(&self, addr: u16, size: usize) -> Result<Vec<u8>> {
        (**self).read_multi(addr, size)
    }

    fn write_multi(&self, addr: u16, data: &[u8]) -> Result<()> {
        (**self).write_multi(addr, data)
    }

    fn sleep_ms(&self, ms: u32) {
        (**self).sleep_ms(ms)
    }
}
