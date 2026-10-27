//! Opening devices (contract 02 §4, contract 07 §2): the probe, class resolution and continuing a
//! connection as its sensor class.

use std::time::Duration;

use super::bno055::Bno055Device;
use super::bno086::Bno086Device;
use super::device::DepzDevice;
use super::error::{DepzError, Result};
use super::link::Link;
use super::serial::{
    by_serial, find_port, list_depz_ports, list_serial_ports, SerialPortInfo, SerialPortLink,
};
use super::sr04::Sr04Device;
use super::tmf8829::Tmf8829Device;
use super::vl53l4cd::Vl53l4cdDevice;
use super::vl53l7::{Vl53l5cxDevice, Vl53l7chDevice, Vl53l7cxDevice};
use super::vl53l8::{Vl53l8chDevice, Vl53l8cxDevice};
use super::vl53lx::{
    Vl53l0xDevice, Vl53l1cbDevice, Vl53l1cxDevice, Vl53l3cxDevice, Vl53l4cxDevice, Vl53lxDevice,
};
use crate::discovery::resolve_sensor_class;
use crate::protocol::identity::{parse_software_name, Mode, SensorType};
use crate::usb_ids::usb_model_hint;

/// The probe's answers: software name, device name, serial number.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct ProbeResult {
    pub software_name: String,
    pub device_name: String,
    pub serial_number: String,
}

/// What the contract 02 §4.4 probe learned about one port ([`probe_port`], [`list_depz_devices`]).
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct DeviceInfo {
    pub port: String,
    pub mode: Mode,
    /// `None` in bootloader / unknown mode.
    pub sensor_type: Option<SensorType>,
    pub software_name: String,
    pub fw_version: String,
    pub device_name: String,
    /// The device's protocol serial (GET_SERIAL).
    pub serial_number: String,
    /// The port's USB id, informative only — never filtered on.
    pub usb_vid: Option<u16>,
    pub usb_pid: Option<u16>,
    /// The USB iSerial, the ordering / selection key (contract 02 §4.3).
    pub usb_serial: Option<String>,
}

impl DeviceInfo {
    /// The model the USB-id table gives the port (contract 02 §4.2), or `None`.
    pub fn usb_model_hint(&self) -> Option<&'static str> {
        usb_model_hint(self.usb_vid, self.usb_pid)
    }
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
    Ok(ProbeResult {
        software_name,
        device_name,
        serial_number,
    })
}

/// Continue the open plain device `dev` as `class_name` (a name [`resolve_sensor_class`] returns)
/// on the same connection — no reopen; the reader and the host sequence numbers run on (contract 02
/// §4.4). `"device"`, and a class this SDK has no device layer for, give [`AnyDevice::Plain`].
pub fn attach(dev: DepzDevice, class_name: &str) -> Result<AnyDevice> {
    if !crate::discovery::SENSOR_CLASS_NAMES.contains(&class_name) {
        return Err(DepzError::InvalidArgument(format!(
            "no sensor class '{class_name}'"
        )));
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
            return Err(DepzError::NoDevice(format!(
                "no DEPZ device answered on {name} ({e})"
            )));
        }
    };
    attach(
        dev,
        resolve_sensor_class(&p.software_name, &p.device_name, usb_vid, usb_pid),
    )
}

// ── serial ports (contract 02 §4.3) ──────────────────────────────────────────

/// Probe one port: open it, run [`probe`], close. `None` when the port does not open or nothing
/// DEPZ-shaped answers. Probing opens the port: leave ports other software owns alone.
pub fn probe_port(port: &str, timeout: Option<Duration>) -> Option<DeviceInfo> {
    let dev = DepzDevice::open(port, timeout).ok()?;
    let p = probe(&dev);
    dev.close();
    let p = p.ok()?;
    let ident = parse_software_name(&p.software_name);
    Some(DeviceInfo {
        port: port.to_string(),
        mode: ident.mode,
        sensor_type: ident.sensor_type,
        software_name: ident.software_name,
        fw_version: ident.version,
        device_name: p.device_name,
        serial_number: p.serial_number,
        usb_vid: None,
        usb_pid: None,
        usb_serial: None,
    })
}

/// Probe the candidate ports and return every DEPZ device found, ordered by USB serial. With
/// `match_usb` only ports with a known DEPZ USB id are probed; `ports` (`None`: enumerate)
/// replaces enumeration with an explicit list (still USB-annotated when the OS knows the port).
/// [`DepzError::Io`] when the OS refuses to enumerate.
pub fn list_depz_devices(
    ports: Option<&[&str]>,
    match_usb: bool,
    timeout: Option<Duration>,
) -> Result<Vec<DeviceInfo>> {
    let enumerated = list_serial_ports()?;
    let mut candidates: Vec<SerialPortInfo> = match ports {
        None => enumerated,
        Some(ports) => ports.iter().map(|p| port_id(&enumerated, p)).collect(),
    };
    if match_usb {
        candidates.retain(SerialPortInfo::is_depz);
    }
    let mut found = Vec::new();
    for pi in by_serial(candidates) {
        if let Some(mut info) = probe_port(&pi.port, timeout) {
            info.usb_vid = pi.vid;
            info.usb_pid = pi.pid;
            info.usb_serial = pi.usb_serial;
            found.push(info);
        }
    }
    Ok(found)
}

/// `port` with the USB identity of the enumerated port it names under any alias
/// (`/dev/serial/by-id/…`, [`find_port`]); none when the OS does not list it. The port stays
/// `port`: an explicit path is opened as given.
fn port_id(enumerated: &[SerialPortInfo], port: &str) -> SerialPortInfo {
    let found = find_port(enumerated, port);
    SerialPortInfo {
        port: port.to_string(),
        vid: found.and_then(|p| p.vid),
        pid: found.and_then(|p| p.pid),
        usb_serial: found.and_then(|p| p.usb_serial.clone()),
    }
}

/// Open exactly `port` as its sensor class (contract 02 §4.3, §4.4): the probe runs on a connection
/// of its own, which is closed; the class the answers and the port's USB id resolve to then opens
/// the port afresh (host sequence numbers restart at 0). The USB id is that of the enumerated port
/// `port` names under any alias (`/dev/serial/by-id/…`, a symlink, another name of the same device
/// node). A port whose USB id is not a DEPZ one — or that the OS does not list — is opened too,
/// with a warning on stderr (an unprogrammed unit, a custom setup). `timeout` `None`: 200 ms.
///
/// [`DepzError::DeviceLost`]: the port does not open; [`DepzError::NoDevice`]: nothing DEPZ
/// answered; [`DepzError::BootloaderMode`]: use the contract 06 client.
pub fn open_device(port: &str, timeout: Option<Duration>) -> Result<AnyDevice> {
    let pi = port_id(&list_serial_ports().unwrap_or_default(), port);
    if !pi.is_depz() {
        eprintln!(
            "{port}: USB id {} is not a known DEPZ device — opening anyway (unprogrammed unit / custom setup?)",
            fmt_vid_pid(pi.vid, pi.pid)
        );
    }
    open_probed(port, pi.vid, pi.pid, timeout)
}

/// Open the DEPZ candidate with the alphabetically smallest USB serial as its sensor class
/// (contract 02 §4.3): the first of [`list_depz_ports`]. [`DepzError::NoDevice`] when no serial
/// port has a known DEPZ USB id — never "the first port the OS lists".
pub fn open_first_device(timeout: Option<Duration>) -> Result<AnyDevice> {
    open_device_at(0, timeout).map_err(|e| match e {
        DepzError::NoDevice(_) => DepzError::NoDevice(
            "no DEPZ device found (no serial port has a known DEPZ USB id). Pass a port path \
             explicitly to open an unprogrammed unit."
                .into(),
        ),
        other => other,
    })
}

/// Open the DEPZ candidate whose USB serial is `serial` as its sensor class (contract 02 §4.3).
/// [`DepzError::NoDevice`] when none has it.
pub fn open_device_by_serial(serial: &str, timeout: Option<Duration>) -> Result<AnyDevice> {
    let pi = list_depz_ports()?
        .into_iter()
        .find(|p| p.usb_serial.as_deref() == Some(serial))
        .ok_or_else(|| DepzError::NoDevice(format!("no DEPZ device with USB serial '{serial}'")))?;
    open_probed(&pi.port, pi.vid, pi.pid, timeout)
}

/// Open the `index`-th DEPZ candidate (0-based, candidates sorted by USB serial) as its sensor
/// class (contract 02 §4.3). [`DepzError::NoDevice`] when out of range.
pub fn open_device_at(index: usize, timeout: Option<Duration>) -> Result<AnyDevice> {
    let candidates = list_depz_ports()?;
    let pi = candidates.get(index).ok_or_else(|| {
        DepzError::NoDevice(format!(
            "no DEPZ candidate at index {index} ({} candidate(s) found)",
            candidates.len()
        ))
    })?;
    open_probed(&pi.port, pi.vid, pi.pid, timeout)
}

/// Open the port of a [`list_depz_devices`] result as the class its probe answers resolve to —
/// no second probe.
pub fn open_device_info(info: &DeviceInfo, timeout: Option<Duration>) -> Result<AnyDevice> {
    open_resolved(
        &info.port,
        &info.software_name,
        &info.device_name,
        info.usb_vid,
        info.usb_pid,
        timeout,
    )
}

fn fmt_vid_pid(vid: Option<u16>, pid: Option<u16>) -> String {
    match (vid, pid) {
        (Some(v), Some(p)) => format!("{v:04X}:{p:04X}"),
        _ => "unknown".into(),
    }
}

/// The probe on a connection of its own, closed before the class opens the port afresh.
fn open_probed(
    port: &str,
    vid: Option<u16>,
    pid: Option<u16>,
    timeout: Option<Duration>,
) -> Result<AnyDevice> {
    let dev = DepzDevice::open(port, timeout)?;
    let p = probe(&dev);
    dev.close();
    let p =
        p.map_err(|e| DepzError::NoDevice(format!("no DEPZ device answered on {port} ({e})")))?;
    open_resolved(port, &p.software_name, &p.device_name, vid, pid, timeout)
}

fn open_resolved(
    port: &str,
    software_name: &str,
    device_name: &str,
    vid: Option<u16>,
    pid: Option<u16>,
    timeout: Option<Duration>,
) -> Result<AnyDevice> {
    let class_name = resolve_sensor_class(software_name, device_name, vid, pid);
    if class_name == "bootloader" {
        return Err(DepzError::BootloaderMode);
    }
    attach(
        DepzDevice::new(SerialPortLink::open(port)?, timeout),
        class_name,
    )
}
