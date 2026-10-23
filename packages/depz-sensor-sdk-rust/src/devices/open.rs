//! Opening devices (contract 02 §4, contract 07 §2): the probe, class resolution and continuing a
//! connection as its sensor class.

use std::time::Duration;

use super::device::DepzDevice;
use super::error::{DepzError, Result};
use super::link::Link;
use super::bno055::Bno055Device;
use super::bno086::Bno086Device;
use super::sr04::Sr04Device;
use super::tmf8829::Tmf8829Device;
use super::vl53l4cd::Vl53l4cdDevice;
use super::vl53l7::{Vl53l5cxDevice, Vl53l7chDevice, Vl53l7cxDevice};
use super::vl53l8::{Vl53l8chDevice, Vl53l8cxDevice};
use super::vl53lx::{Vl53l0xDevice, Vl53l1cbDevice, Vl53l1cxDevice, Vl53l3cxDevice, Vl53l4cxDevice, Vl53lxDevice};
use crate::discovery::resolve_sensor_class;

/// The probe's answers: software name, device name, serial number.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct ProbeResult {
    pub software_name: String,
    pub device_name: String,
    pub serial_number: String,
}

/// A device opened as its sensor class. `Plain` is a device this SDK has no class for (or
/// `"device"`): the common commands work.
#[non_exhaustive]
pub enum AnyDevice {
    Plain(DepzDevice),
    Sr04(Sr04Device),
    Vl53l4cd(Vl53l4cdDevice),
    Bno055(Bno055Device),
    Bno086(Bno086Device),
    Vl53l8cx(Vl53l8cxDevice),
    Vl53l8ch(Vl53l8chDevice),
    Vl53l5cx(Vl53l5cxDevice),
    Vl53l7cx(Vl53l7cxDevice),
    Vl53l7ch(Vl53l7chDevice),
    Tmf8829(Tmf8829Device),
    Vl53lx(Vl53lxDevice),
    Vl53l0x(Vl53l0xDevice),
    Vl53l1cx(Vl53l1cxDevice),
    Vl53l1cb(Vl53l1cbDevice),
    Vl53l3cx(Vl53l3cxDevice),
    Vl53l4cx(Vl53l4cxDevice),
}

impl AnyDevice {
    /// The connection, whatever the class.
    pub fn device(&self) -> &DepzDevice {
        match self {
            AnyDevice::Plain(d) => d,
            AnyDevice::Sr04(d) => d,
            AnyDevice::Vl53l4cd(d) => d,
            AnyDevice::Bno055(d) => d,
            AnyDevice::Bno086(d) => d,
            AnyDevice::Vl53l8cx(d) => d,
            AnyDevice::Vl53l8ch(d) => d,
            AnyDevice::Vl53l5cx(d) => d,
            AnyDevice::Vl53l7cx(d) => d,
            AnyDevice::Vl53l7ch(d) => d,
            AnyDevice::Tmf8829(d) => d,
            AnyDevice::Vl53lx(d) => d,
            AnyDevice::Vl53l0x(d) => d,
            AnyDevice::Vl53l1cx(d) => d,
            AnyDevice::Vl53l1cb(d) => d,
            AnyDevice::Vl53l3cx(d) => d,
            AnyDevice::Vl53l4cx(d) => d,
        }
    }

    /// The canonical class name (contract 07 §1).
    pub fn class_name(&self) -> &'static str {
        match self {
            AnyDevice::Plain(_) => "device",
            AnyDevice::Sr04(_) => "sr04",
            AnyDevice::Vl53l4cd(_) => "vl53l4cd",
            AnyDevice::Bno055(_) => "bno055",
            AnyDevice::Bno086(_) => "bno086",
            AnyDevice::Vl53l8cx(_) => "vl53l8cx",
            AnyDevice::Vl53l8ch(_) => "vl53l8ch",
            AnyDevice::Vl53l5cx(_) => "vl53l5cx",
            AnyDevice::Vl53l7cx(_) => "vl53l7cx",
            AnyDevice::Vl53l7ch(_) => "vl53l7ch",
            AnyDevice::Tmf8829(_) => "tmf8829",
            AnyDevice::Vl53lx(_) => "vl53lx",
            AnyDevice::Vl53l0x(_) => "vl53l0x",
            AnyDevice::Vl53l1cx(_) => "vl53l1cx",
            AnyDevice::Vl53l1cb(_) => "vl53l1cb",
            AnyDevice::Vl53l3cx(_) => "vl53l3cx",
            AnyDevice::Vl53l4cx(_) => "vl53l4cx",
        }
    }

    /// The SR04 class, or [`DepzError::WrongType`] (the device is closed).
    pub fn into_sr04(self) -> Result<Sr04Device> {
        match self {
            AnyDevice::Sr04(d) => Ok(d),
            other => Err(wrong(other.class_name(), "an SR04")),
        }
    }

    /// The VL53L4CD class, or [`DepzError::WrongType`] (the device is closed).
    pub fn into_vl53l4cd(self) -> Result<Vl53l4cdDevice> {
        match self {
            AnyDevice::Vl53l4cd(d) => Ok(d),
            other => Err(wrong(other.class_name(), "a VL53L4CD")),
        }
    }

    /// The BNO055 class, or [`DepzError::WrongType`] (the device is closed).
    pub fn into_bno055(self) -> Result<Bno055Device> {
        match self {
            AnyDevice::Bno055(d) => Ok(d),
            other => Err(wrong(other.class_name(), "a BNO055")),
        }
    }

    /// The BNO085 / BNO086 class, or [`DepzError::WrongType`] (the device is closed).
    pub fn into_bno086(self) -> Result<Bno086Device> {
        match self {
            AnyDevice::Bno086(d) => Ok(d),
            other => Err(wrong(other.class_name(), "a BNO085 / BNO086")),
        }
    }

    /// The VL53L8CX class, or [`DepzError::WrongType`] (the device is closed).
    pub fn into_vl53l8cx(self) -> Result<Vl53l8cxDevice> {
        match self {
            AnyDevice::Vl53l8cx(d) => Ok(d),
            other => Err(wrong(other.class_name(), "a VL53L8CX")),
        }
    }

    /// The VL53L8CH class, or [`DepzError::WrongType`] (the device is closed).
    pub fn into_vl53l8ch(self) -> Result<Vl53l8chDevice> {
        match self {
            AnyDevice::Vl53l8ch(d) => Ok(d),
            other => Err(wrong(other.class_name(), "a VL53L8CH")),
        }
    }

    /// The VL53L5CX class, or [`DepzError::WrongType`] (the device is closed).
    pub fn into_vl53l5cx(self) -> Result<Vl53l5cxDevice> {
        match self {
            AnyDevice::Vl53l5cx(d) => Ok(d),
            other => Err(wrong(other.class_name(), "a VL53L5CX")),
        }
    }

    /// The VL53L7CX class, or [`DepzError::WrongType`] (the device is closed).
    pub fn into_vl53l7cx(self) -> Result<Vl53l7cxDevice> {
        match self {
            AnyDevice::Vl53l7cx(d) => Ok(d),
            other => Err(wrong(other.class_name(), "a VL53L7CX")),
        }
    }

    /// The VL53L7CH class, or [`DepzError::WrongType`] (the device is closed).
    pub fn into_vl53l7ch(self) -> Result<Vl53l7chDevice> {
        match self {
            AnyDevice::Vl53l7ch(d) => Ok(d),
            other => Err(wrong(other.class_name(), "a VL53L7CH")),
        }
    }

    /// The TMF8829 class, or [`DepzError::WrongType`] (the device is closed).
    pub fn into_tmf8829(self) -> Result<Tmf8829Device> {
        match self {
            AnyDevice::Tmf8829(d) => Ok(d),
            other => Err(wrong(other.class_name(), "a TMF8829")),
        }
    }

    /// A board of the VL53L 1D family, whichever of its classes (the per-product ones give their
    /// family class, product kept), or [`DepzError::WrongType`] (the device is closed).
    pub fn into_vl53lx(self) -> Result<Vl53lxDevice> {
        match self {
            AnyDevice::Vl53lx(d) => Ok(d),
            AnyDevice::Vl53l0x(d) => Ok(d.into_inner()),
            AnyDevice::Vl53l1cx(d) => Ok(d.into_inner()),
            AnyDevice::Vl53l1cb(d) => Ok(d.into_inner()),
            AnyDevice::Vl53l3cx(d) => Ok(d.into_inner()),
            AnyDevice::Vl53l4cx(d) => Ok(d.into_inner()),
            other => Err(wrong(other.class_name(), "a VL53L 1D-family board")),
        }
    }
}

fn wrong(got: &str, what: &str) -> DepzError {
    DepzError::WrongType(format!("the device is not {what} ({got})"))
}

/// The contract 02 §4.4 probe on an open connection: GET_NAME_ACTIVE_SOFTWARE (required: its
/// failure fails the probe), then GET_DEVICE_NAME and GET_SERIAL (a timeout or error leaves "").
pub fn probe(dev: &DepzDevice) -> Result<ProbeResult> {
    let software_name = dev.get_software_name()?;
    let device_name = dev.get_device_name().unwrap_or_default();
    let serial_number = dev.get_serial_number().unwrap_or_default();
    Ok(ProbeResult { software_name, device_name, serial_number })
}

/// Continue the open plain device `dev` as `class_name` (a name [`resolve_sensor_class`] returns)
/// on the same connection — no reopen; the reader and the host sequence numbers run on (contract 02
/// §4.4). `"device"`, and a class this SDK has no device layer for, give [`AnyDevice::Plain`].
pub fn attach(dev: DepzDevice, class_name: &str) -> Result<AnyDevice> {
    if !crate::discovery::SENSOR_CLASS_NAMES.contains(&class_name) {
        return Err(DepzError::InvalidArgument(format!("no sensor class '{class_name}'")));
    }
    Ok(match class_name {
        "bootloader" => return Err(DepzError::BootloaderMode),
        "sr04" => AnyDevice::Sr04(Sr04Device::attach(dev)),
        "vl53l4cd" => AnyDevice::Vl53l4cd(Vl53l4cdDevice::attach(dev)),
        "bno055" => AnyDevice::Bno055(Bno055Device::attach(dev)),
        "bno086" => AnyDevice::Bno086(Bno086Device::attach(dev)),
        "vl53l8cx" => AnyDevice::Vl53l8cx(Vl53l8cxDevice::attach(dev)),
        "vl53l8ch" => AnyDevice::Vl53l8ch(Vl53l8chDevice::attach(dev)),
        "vl53l5cx" => AnyDevice::Vl53l5cx(Vl53l5cxDevice::attach(dev)),
        "vl53l7cx" => AnyDevice::Vl53l7cx(Vl53l7cxDevice::attach(dev)),
        "vl53l7ch" => AnyDevice::Vl53l7ch(Vl53l7chDevice::attach(dev)),
        "tmf8829" => AnyDevice::Tmf8829(Tmf8829Device::attach(dev)),
        "vl53lx" => AnyDevice::Vl53lx(Vl53lxDevice::attach(dev)),
        "vl53l0x" => AnyDevice::Vl53l0x(Vl53l0xDevice::attach(dev)),
        "vl53l1cx" => AnyDevice::Vl53l1cx(Vl53l1cxDevice::attach(dev)),
        "vl53l1cb" => AnyDevice::Vl53l1cb(Vl53l1cbDevice::attach(dev)),
        "vl53l3cx" => AnyDevice::Vl53l3cx(Vl53l3cxDevice::attach(dev)),
        "vl53l4cx" => AnyDevice::Vl53l4cx(Vl53l4cxDevice::attach(dev)),
        _ => AnyDevice::Plain(dev),
    })
}

/// Open a device on a connection that cannot be reopened — a replay or recording link, a link the
/// host opened (contract 02 §4.4). Runs the probe on `link`, resolves the class from the answers and
/// the port's USB id (`None`: unknown; a link knows none) and continues as that class on the same
/// connection. The device owns the link; on failure it is closed.
pub fn open_on_link(
    link: impl Link + 'static,
    usb_vid: Option<u16>,
    usb_pid: Option<u16>,
    timeout: Option<Duration>,
) -> Result<AnyDevice> {
    let name = link.name();
    let dev = DepzDevice::new(link, timeout);
    let p = match probe(&dev) {
        Ok(p) => p,
        Err(e) => {
            dev.close();
            return Err(DepzError::NoDevice(format!("no DEPZ device answered on {name} ({e})")));
        }
    };
    attach(dev, resolve_sensor_class(&p.software_name, &p.device_name, usb_vid, usb_pid))
}
