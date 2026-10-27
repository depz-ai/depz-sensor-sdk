//! The board's USB serial port as a [`Link`], and serial-port enumeration with USB ids — over the
//! `serialport` crate (Windows, macOS, Linux), the way the Python SDK runs on pyserial and the Java
//! SDK on jSerialComm.

use std::io::{ErrorKind, Read, Write};
use std::sync::atomic::{AtomicBool, Ordering};
use std::sync::Mutex;
use std::time::Duration;

use serialport::{SerialPort, SerialPortType};

use super::error::{DepzError, Result};
use super::link::Link;
use crate::usb_ids::{is_known_depz_usb, usb_model_hint};

/// The nominal baud rate the port opens with (115200 8N1, as the other SDKs); the USB CDC boards
/// ignore it.
pub const DEFAULT_BAUD_RATE: u32 = 115_200;

/// The most a single [`Link::read`] returns.
const READ_CHUNK: usize = 16_384;

/// How long a write may block before the link counts the device lost.
const WRITE_TIMEOUT: Duration = Duration::from_secs(2);

/// One serial port the OS lists: its path, and its USB ids and iSerial when it is a USB device
/// (else `None`).
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct SerialPortInfo {
    /// The path (`/dev/ttyACM0`, `COM5`, `/dev/cu.usbmodem…`).
    pub port: String,
    pub vid: Option<u16>,
    pub pid: Option<u16>,
    /// The USB iSerial — the flashed serial on production units (contract 02 §4.3).
    pub usb_serial: Option<String>,
}

impl SerialPortInfo {
    /// The port's USB id is a known DEPZ (or dev-default) one (contract 02 §4.2).
    pub fn is_depz(&self) -> bool {
        is_known_depz_usb(self.vid, self.pid)
    }

    /// The model the USB-id table gives the port, or `None`. Informational: the probe decides.
    pub fn usb_model_hint(&self) -> Option<&'static str> {
        usb_model_hint(self.vid, self.pid)
    }
}

/// Every serial port the OS lists, with its USB identity where known. [`DepzError::Io`] when the
/// OS refuses to enumerate.
pub fn list_serial_ports() -> Result<Vec<SerialPortInfo>> {
    let ports = serialport::available_ports().map_err(|e| DepzError::Io(e.into()))?;
    Ok(ports
        .into_iter()
        .map(|p| match p.port_type {
            SerialPortType::UsbPort(usb) => SerialPortInfo {
                port: p.port_name,
                vid: Some(usb.vid),
                pid: Some(usb.pid),
                usb_serial: usb.serial_number.filter(|s| !s.is_empty()),
            },
            _ => SerialPortInfo {
                port: p.port_name,
                vid: None,
                pid: None,
                usb_serial: None,
            },
        })
        .collect())
}

/// The enumerated port `path` names, under any alias (contract 02 §4.3, ERRATA A5): the one whose
/// path is the same string, else — on POSIX — the one whose resolved path is the same
/// (`/dev/serial/by-id/…` is a symlink to the node), else the one that is the same character
/// device under another name. On Windows a COM name is matched by string only: a COM port is not
/// a file, and resolving one (canonicalize, metadata) opens it — a DTR pulse on every board of the
/// bench. `None` when the OS does not list the port.
pub fn find_port<'a>(ports: &'a [SerialPortInfo], path: &str) -> Option<&'a SerialPortInfo> {
    ports
        .iter()
        .find(|p| p.port == path)
        .or_else(|| ports.iter().find(|p| same_port(path, &p.port)))
}

#[cfg(unix)]
fn same_port(a: &str, b: &str) -> bool {
    use std::os::unix::fs::{FileTypeExt, MetadataExt};
    if let (Ok(ra), Ok(rb)) = (std::fs::canonicalize(a), std::fs::canonicalize(b)) {
        if ra == rb {
            return true;
        }
    }
    // a zero device number is no node: a platform that stats every port as an empty character
    // device must not make them one device
    match (std::fs::metadata(a), std::fs::metadata(b)) {
        (Ok(ma), Ok(mb)) => {
            ma.file_type().is_char_device()
                && mb.file_type().is_char_device()
                && ma.rdev() != 0
                && ma.rdev() == mb.rdev()
        }
        _ => false,
    }
}

#[cfg(not(unix))]
fn same_port(_a: &str, _b: &str) -> bool {
    false
}

/// Ports by USB iSerial ascending, those without one last, ties by path (contract 02 §4.3).
pub fn by_serial(mut ports: Vec<SerialPortInfo>) -> Vec<SerialPortInfo> {
    ports.sort_by(|a, b| {
        let key = |p: &SerialPortInfo| {
            let serial = p.usb_serial.clone().unwrap_or_default();
            (serial.is_empty(), serial, p.port.clone())
        };
        key(a).cmp(&key(b))
    });
    ports
}

/// The DEPZ candidates: every port with a known DEPZ USB id, by USB serial (contract 02 §4.3).
pub fn list_depz_ports() -> Result<Vec<SerialPortInfo>> {
    Ok(by_serial(
        list_serial_ports()?
            .into_iter()
            .filter(SerialPortInfo::is_depz)
            .collect(),
    ))
}

/// A serial port as a [`Link`]: 115200 8N1 with DTR and RTS raised, opened exclusively (a second
/// opener of a CDC-ACM tty would silently share the byte stream). Reads block up to their timeout,
/// so [`Link::close`] returns within one read slice; a lost port fails the next read or write with
/// [`DepzError::DeviceLost`].
pub struct SerialPortLink {
    reader: Mutex<Option<Box<dyn SerialPort>>>,
    writer: Mutex<Option<Box<dyn SerialPort>>>,
    name: String,
    closed: AtomicBool,
    read_timeout: Mutex<Option<Duration>>,
}

impl SerialPortLink {
    /// Open `port` at [`DEFAULT_BAUD_RATE`]. [`DepzError::DeviceLost`] when it does not open (no
    /// such port, busy, no permission).
    pub fn open(port: &str) -> Result<SerialPortLink> {
        SerialPortLink::open_with_baud_rate(port, DEFAULT_BAUD_RATE)
    }

    /// [`SerialPortLink::open`] at `baud_rate`.
    pub fn open_with_baud_rate(port: &str, baud_rate: u32) -> Result<SerialPortLink> {
        let lost = |e: serialport::Error| DepzError::DeviceLost(format!("cannot open {port}: {e}"));
        let mut reader = serialport::new(port, baud_rate)
            .data_bits(serialport::DataBits::Eight)
            .parity(serialport::Parity::None)
            .stop_bits(serialport::StopBits::One)
            .flow_control(serialport::FlowControl::None)
            .timeout(WRITE_TIMEOUT)
            .open()
            .map_err(lost)?;
        reader.write_data_terminal_ready(true).map_err(lost)?;
        reader.write_request_to_send(true).map_err(lost)?;
        // A handle of its own for writes: a request must go out while the reader thread sits in
        // a blocking read.
        let writer = reader.try_clone().map_err(lost)?;
        Ok(SerialPortLink {
            reader: Mutex::new(Some(reader)),
            writer: Mutex::new(Some(writer)),
            name: port.to_string(),
            closed: AtomicBool::new(false),
            read_timeout: Mutex::new(None),
        })
    }

    /// The port path.
    pub fn port(&self) -> &str {
        &self.name
    }

    fn closed_err(&self) -> DepzError {
        DepzError::LinkClosed(format!("{} closed", self.name))
    }

    fn lost(&self, what: &str, e: &std::io::Error) -> DepzError {
        DepzError::DeviceLost(format!("{}: {what} failed ({e})", self.name))
    }
}

impl Link for SerialPortLink {
    fn read(&self, timeout: Duration) -> Result<Vec<u8>> {
        if self.closed.load(Ordering::SeqCst) {
            return Err(self.closed_err());
        }
        let mut guard = self.reader.lock().unwrap();
        let port = guard.as_mut().ok_or_else(|| self.closed_err())?;
        let timeout = timeout.max(Duration::from_millis(1));
        let mut current = self.read_timeout.lock().unwrap();
        if *current != Some(timeout) {
            port.set_timeout(timeout)
                .map_err(|e| self.lost("read", &e.into()))?;
            *current = Some(timeout);
        }
        drop(current);
        let mut buf = [0u8; READ_CHUNK];
        match port.read(&mut buf) {
            Ok(0) => Ok(Vec::new()),
            Ok(n) => Ok(buf[..n].to_vec()),
            Err(e)
                if matches!(
                    e.kind(),
                    ErrorKind::TimedOut | ErrorKind::WouldBlock | ErrorKind::Interrupted
                ) =>
            {
                Ok(Vec::new())
            }
            Err(_) if self.closed.load(Ordering::SeqCst) => Err(self.closed_err()),
            Err(e) => Err(self.lost("read", &e)),
        }
    }

    fn write(&self, data: &[u8]) -> Result<()> {
        if self.closed.load(Ordering::SeqCst) {
            return Err(self.closed_err());
        }
        let mut guard = self.writer.lock().unwrap();
        let port = guard.as_mut().ok_or_else(|| self.closed_err())?;
        port.write_all(data)
            .and_then(|_| port.flush())
            .map_err(|e| {
                if self.closed.load(Ordering::SeqCst) {
                    self.closed_err()
                } else {
                    self.lost("write", &e)
                }
            })
    }

    fn close(&self) {
        if self.closed.swap(true, Ordering::SeqCst) {
            return;
        }
        self.writer.lock().unwrap().take();
        // Waits for a read in progress (one read slice at most), then releases the port.
        self.reader.lock().unwrap().take();
    }

    fn is_closed(&self) -> bool {
        self.closed.load(Ordering::SeqCst)
    }

    fn name(&self) -> String {
        self.name.clone()
    }
}
