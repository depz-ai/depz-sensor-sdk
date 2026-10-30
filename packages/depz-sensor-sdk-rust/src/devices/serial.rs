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
/// [`DepzError::DeviceLost`] and the link is closed from then on.
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

    /// An I/O error: the port is gone, so the link is closed from now on (as Python's
    /// `SerialLink`); the device reports itself closed.
    fn lost(&self, what: &str, e: &std::io::Error) -> DepzError {
        self.closed.store(true, Ordering::SeqCst);
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
        // No flush: it is tcdrain, which waits with no timeout for a board that stopped reading
        // (ERRATA E10). write_all already handed the bytes to the OS, as Python's SerialLink does.
        let r = port.write_all(data);
        if self.closed.load(Ordering::SeqCst) {
            // close() came while this write was in the OS and left the handle to it.
            guard.take();
            return Err(self.closed_err());
        }
        r.map_err(|e| self.lost("write", &e))
    }

    fn close(&self) {
        // Not skipped when an I/O error already marked the link closed: the handles go here.
        self.closed.store(true, Ordering::SeqCst);
        // Never waits for a write stuck in the OS: that write releases the handle when it returns.
        if let Ok(mut w) = self.writer.try_lock() {
            w.take();
        }
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

#[cfg(all(test, unix))]
mod tests {
    use super::*;

    /// A link over the slave side of a pseudo-terminal; dropping the master is an unplug.
    fn pty_link() -> (serialport::TTYPort, SerialPortLink) {
        let (master, slave) = serialport::TTYPort::pair().expect("a pseudo-terminal");
        let writer = slave.try_clone().expect("a second handle");
        let link = SerialPortLink {
            reader: Mutex::new(Some(Box::new(slave))),
            writer: Mutex::new(Some(writer)),
            name: "pty".into(),
            closed: AtomicBool::new(false),
            read_timeout: Mutex::new(None),
        };
        (master, link)
    }

    /// Bug hunt 2026-10-29 RUST-2: a read that fails with an I/O error marks the link closed, as
    /// Python's SerialLink does, so the device reports itself closed after an unplug.
    #[test]
    fn a_read_io_error_marks_the_link_closed() {
        let (master, link) = pty_link();
        assert!(!link.is_closed());
        drop(master);
        let r = link.read(Duration::from_millis(200));
        assert!(matches!(r, Err(DepzError::DeviceLost(_))), "{r:?}");
        assert!(link.is_closed(), "the link is open after a failed read");
        assert!(matches!(
            link.read(Duration::from_millis(10)),
            Err(DepzError::LinkClosed(_))
        ));
    }

    /// Bug hunt 2026-10-29 RUST-5: close() returns while a write is stuck in the OS (a board that
    /// stops reading its OUT endpoint, ERRATA E10); the stuck write fails once the port goes.
    #[test]
    fn close_does_not_wait_for_a_write_stuck_in_the_os() {
        let (master, link) = pty_link();
        let link = std::sync::Arc::new(link);
        let w = link.clone();
        // Far more than the pty buffer: with nobody reading, the blocking write never completes.
        let writer = std::thread::spawn(move || w.write(&vec![0x55u8; 1 << 20]));
        let deadline = std::time::Instant::now() + Duration::from_secs(5);
        while master.bytes_to_read().unwrap_or(0) == 0 {
            assert!(
                std::time::Instant::now() < deadline,
                "the write never started"
            );
            std::thread::sleep(Duration::from_millis(5));
        }
        let (done_tx, done) = std::sync::mpsc::channel();
        let c = link.clone();
        std::thread::spawn(move || {
            c.close();
            let _ = done_tx.send(());
        });
        let closed = done.recv_timeout(Duration::from_secs(1));
        drop(master);
        assert!(closed.is_ok(), "close() waited for the stuck write");
        assert!(link.is_closed());
        let r = writer.join().unwrap();
        assert!(matches!(r, Err(DepzError::LinkClosed(_))), "{r:?}");
        assert!(
            link.writer.lock().unwrap().is_none(),
            "the write kept the port"
        );
    }

    /// RUST-2: the same for a write.
    #[test]
    fn a_write_io_error_marks_the_link_closed() {
        let (master, link) = pty_link();
        drop(master);
        let r = link.write(&[1, 2, 3]);
        assert!(matches!(r, Err(DepzError::DeviceLost(_))), "{r:?}");
        assert!(link.is_closed(), "the link is open after a failed write");
        assert!(matches!(link.write(&[1]), Err(DepzError::LinkClosed(_))));
    }
}
