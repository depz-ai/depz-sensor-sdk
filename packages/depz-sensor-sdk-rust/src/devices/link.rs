//! Links: a blocking byte pipe to one device (contract 07 §1), and the `.depzrec` record / replay
//! wrappers (contract 08).

use std::fs::File;
use std::io::{BufWriter, Write};
use std::path::Path;
use std::sync::{Arc, Condvar, Mutex};
use std::time::{Duration, Instant, SystemTime, UNIX_EPOCH};

use super::error::{DepzError, Result};
use crate::json::{is_blank_line, Value};

/// A blocking byte pipe to one device: a serial port, a `.depzrec` record / replay wrapper, or
/// anything a host supplies. Device classes accept any link, which keeps every layer above the port
/// testable without hardware. Implementations are shared between the reader thread and callers, so
/// every method takes `&self`.
pub trait Link: Send + Sync {
    /// The next available chunk (one byte or more), or an empty vector when nothing arrived within
    /// `timeout`. [`DepzError::LinkClosed`] / [`DepzError::DeviceLost`] when the link is gone.
    fn read(&self, timeout: Duration) -> Result<Vec<u8>>;

    fn write(&self, data: &[u8]) -> Result<()>;

    fn close(&self);

    fn is_closed(&self) -> bool;

    /// A name for messages: the port path, or what the link wraps.
    fn name(&self) -> String;
}

pub(crate) fn hex_encode(b: &[u8]) -> String {
    let mut s = String::with_capacity(b.len() * 2);
    for x in b {
        s.push_str(&format!("{x:02x}"));
    }
    s
}

/// `s` as bytes when it is hex digit pairs, else `None` (walked by byte: a non-ASCII character
/// is a refusal, never a slice inside it).
pub(crate) fn hex_decode(s: &str) -> Option<Vec<u8>> {
    let b = s.as_bytes();
    if !b.len().is_multiple_of(2) {
        return None;
    }
    let nibble = |c: u8| (c as char).to_digit(16).map(|d| d as u8);
    b.as_chunks::<2>()
        .0
        .iter()
        .map(|[hi, lo]| Some(nibble(*hi)? << 4 | nibble(*lo)?))
        .collect()
}

/// One `.depzrec` event line (contract 08) as (t_us, dir, data), else what is wrong with it.
fn replay_event(line: &str) -> std::result::Result<(u64, String, Vec<u8>), String> {
    let ev = Value::parse(line).map_err(|e| format!("not JSON: {e}"))?;
    if ev.entries().is_none() {
        return Err("not a JSON object".into());
    }
    let t_us = match ev.get("t") {
        Some(t) => match t.as_i64() {
            Some(t) if t >= 0 => t as u64,
            _ => {
                return Err(format!(
                    "'t' must be a non-negative integer, got {}",
                    t.to_json()
                ))
            }
        },
        None => return Err("'t' must be a non-negative integer, got nothing".into()),
    };
    let dir = match ev.get("dir") {
        Some(d) => d
            .as_str()
            .ok_or_else(|| format!("'dir' must be a string, got {}", d.to_json()))?,
        None => return Err("'dir' must be a string, got nothing".into()),
    };
    let data = match ev.get("data") {
        Some(d) => {
            let hex = d
                .as_str()
                .ok_or_else(|| format!("'data' must be a hex string, got {}", d.to_json()))?;
            hex_decode(hex).ok_or_else(|| format!("'data' is not hex: {}", d.to_json()))?
        }
        None => return Err("'data' must be a hex string, got nothing".into()),
    };
    Ok((t_us, dir.to_string(), data))
}

/// `YYYY-MM-DDTHH:MM:SSZ` for now (UTC).
pub(crate) fn utc_now_iso() -> String {
    let secs = SystemTime::now()
        .duration_since(UNIX_EPOCH)
        .map(|d| d.as_secs() as i64)
        .unwrap_or(0);
    let days = secs.div_euclid(86_400);
    let rem = secs.rem_euclid(86_400);
    // civil from days (Howard Hinnant)
    let z = days + 719_468;
    let era = z.div_euclid(146_097);
    let doe = z - era * 146_097;
    let yoe = (doe - doe / 1460 + doe / 36_524 - doe / 146_096) / 365;
    let doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    let mp = (5 * doy + 2) / 153;
    let d = doy - (153 * mp + 2) / 5 + 1;
    let m = if mp < 10 { mp + 3 } else { mp - 9 };
    let y = yoe + era * 400 + if m <= 2 { 1 } else { 0 };
    format!(
        "{y:04}-{m:02}-{d:02}T{:02}:{:02}:{:02}Z",
        rem / 3600,
        rem % 3600 / 60,
        rem % 60
    )
}

struct Rx {
    t_us: u64,
    data: Vec<u8>,
    tx_prefix: usize,
}

struct ReplayState {
    rx_idx: usize,
    tx_written: usize,
    closed: bool,
}

/// Replays the rx side of a `.depzrec` capture causally (contract 08): an rx chunk is served only
/// after the host has written as many tx bytes as preceded it in the capture. With `strict_tx` every
/// written byte must also equal the recorded tx stream ([`DepzError::ReplayMismatch`]). A damaged
/// capture is refused by [`ReplayLink::open`] with [`DepzError::InvalidArgument`] naming the line.
pub struct ReplayLink {
    rx: Vec<Rx>,
    tx: Vec<u8>,
    strict_tx: bool,
    realtime: bool,
    t0: Instant,
    name: String,
    header: Value,
    state: Mutex<ReplayState>,
    cond: Condvar,
}

impl ReplayLink {
    /// Load `path`; `strict_tx` fails a write that differs from the recorded tx stream, `realtime`
    /// paces rx chunks by their recorded timestamps. A damaged capture (contract 08: an empty file,
    /// a header that is not a JSON object, an event line that is not one or whose `t` / `dir` /
    /// `data` are not a non-negative integer / a string / hex digit pairs) is
    /// [`DepzError::InvalidArgument`] naming the line; blank lines are skipped, any `dir` other
    /// than `"rx"` counts as tx.
    pub fn open(path: impl AsRef<Path>, strict_tx: bool, realtime: bool) -> Result<ReplayLink> {
        let path = path.as_ref();
        // not UTF-8 (a gzip container, ERRATA A6) is a damaged capture, not an I/O failure
        let text = String::from_utf8(std::fs::read(path)?).map_err(|e| {
            DepzError::InvalidArgument(format!("{}: not UTF-8 text: {e}", path.display()))
        })?;
        let bad = |lineno: usize, m: String| {
            DepzError::InvalidArgument(format!("{}:{lineno}: {m}", path.display()))
        };
        let mut lines = text.lines().enumerate();
        let header = match lines.next() {
            Some((_, l)) => Value::parse(l).map_err(|e| bad(1, format!("not JSON: {e}")))?,
            None => {
                return Err(DepzError::InvalidArgument(format!(
                    "{}: empty, not a .depzrec capture (no header line)",
                    path.display()
                )))
            }
        };
        if header.entries().is_none() {
            return Err(bad(1, "header is not a JSON object".into()));
        }
        let mut rx = Vec::new();
        let mut tx = Vec::new();
        for (idx, line) in lines {
            if is_blank_line(line) {
                continue;
            }
            let (t_us, dir, data) = replay_event(line).map_err(|m| bad(idx + 1, m))?;
            if dir == "rx" {
                rx.push(Rx {
                    t_us,
                    data,
                    tx_prefix: tx.len(),
                });
            } else {
                tx.extend_from_slice(&data);
            }
        }
        Ok(ReplayLink {
            rx,
            tx,
            strict_tx,
            realtime,
            t0: Instant::now(),
            name: format!(
                "replay:{}",
                path.file_name()
                    .map(|n| n.to_string_lossy().into_owned())
                    .unwrap_or_default()
            ),
            header,
            state: Mutex::new(ReplayState {
                rx_idx: 0,
                tx_written: 0,
                closed: false,
            }),
            cond: Condvar::new(),
        })
    }

    /// The capture's header object (contract 08).
    pub fn header(&self) -> &Value {
        &self.header
    }

    /// Every rx chunk was delivered.
    pub fn rx_exhausted(&self) -> bool {
        self.state.lock().unwrap().rx_idx >= self.rx.len()
    }

    /// Recorded rx chunks not delivered yet.
    pub fn rx_remaining(&self) -> usize {
        self.rx.len() - self.state.lock().unwrap().rx_idx
    }

    /// Recorded tx bytes the host has not written yet.
    pub fn tx_remaining(&self) -> usize {
        self.tx
            .len()
            .saturating_sub(self.state.lock().unwrap().tx_written)
    }
}

impl Link for ReplayLink {
    fn read(&self, timeout: Duration) -> Result<Vec<u8>> {
        let deadline = Instant::now() + timeout;
        let idx;
        {
            let mut s = self.state.lock().unwrap();
            loop {
                if s.closed {
                    return Err(DepzError::LinkClosed("replay link closed".into()));
                }
                if s.rx_idx >= self.rx.len() {
                    return Ok(Vec::new());
                }
                if s.tx_written >= self.rx[s.rx_idx].tx_prefix {
                    break;
                }
                let now = Instant::now();
                if now >= deadline {
                    return Ok(Vec::new());
                }
                s = self.cond.wait_timeout(s, deadline - now).unwrap().0;
            }
            idx = s.rx_idx;
        }
        let ev = &self.rx[idx];
        if self.realtime {
            let due = self.t0 + Duration::from_micros(ev.t_us);
            let now = Instant::now();
            if due > now {
                if due > deadline {
                    std::thread::sleep(deadline.saturating_duration_since(now));
                    return Ok(Vec::new());
                }
                std::thread::sleep(due - now);
            }
        }
        self.state.lock().unwrap().rx_idx += 1;
        Ok(ev.data.clone())
    }

    fn write(&self, data: &[u8]) -> Result<()> {
        let mut s = self.state.lock().unwrap();
        if s.closed {
            return Err(DepzError::LinkClosed("replay link closed".into()));
        }
        if self.strict_tx {
            let at = s.tx_written;
            let end = at + data.len();
            if end > self.tx.len() || self.tx[at..end] != *data {
                let from = at.min(self.tx.len());
                let to = end.min(self.tx.len());
                return Err(DepzError::ReplayMismatch(format!(
                    "replay strict_tx mismatch at offset {at}: expected {}, got {}",
                    hex_encode(&self.tx[from..to]),
                    hex_encode(data)
                )));
            }
        }
        s.tx_written += data.len();
        self.cond.notify_all();
        Ok(())
    }

    fn close(&self) {
        self.state.lock().unwrap().closed = true;
        self.cond.notify_all();
    }

    fn is_closed(&self) -> bool {
        self.state.lock().unwrap().closed
    }

    fn name(&self) -> String {
        self.name.clone()
    }
}

impl<L: Link + ?Sized> Link for Arc<L> {
    fn read(&self, timeout: Duration) -> Result<Vec<u8>> {
        (**self).read(timeout)
    }

    fn write(&self, data: &[u8]) -> Result<()> {
        (**self).write(data)
    }

    fn close(&self) {
        (**self).close()
    }

    fn is_closed(&self) -> bool {
        (**self).is_closed()
    }

    fn name(&self) -> String {
        (**self).name()
    }
}

/// Wraps any link and tees both directions into a `.depzrec` file (contract 08). TX is journaled
/// before the physical write: a fast device reply could otherwise reach the reader and be journaled
/// ahead of its own request, which breaks the causal rx gating replay relies on.
pub struct RecordingLink<L: Link> {
    inner: L,
    file: Mutex<Option<BufWriter<File>>>,
    t0: Instant,
}

/// The schema id of a `.depzrec` capture.
pub const RECORDING_SCHEMA: &str = "depz.rec/1";

impl<L: Link> RecordingLink<L> {
    /// Record `inner` into `path` (overwritten); `header_extra` adds header fields (contract 08),
    /// e.g. `device`. Closing this link closes `inner`.
    pub fn create(
        inner: L,
        path: impl AsRef<Path>,
        header_extra: Vec<(String, Value)>,
    ) -> Result<RecordingLink<L>> {
        let mut file = BufWriter::new(File::create(path)?);
        let mut header = vec![
            ("schema".to_string(), Value::String(RECORDING_SCHEMA.into())),
            ("created_utc".to_string(), Value::String(utc_now_iso())),
        ];
        header.extend(header_extra);
        writeln!(file, "{}", Value::Object(header).to_json())?;
        Ok(RecordingLink {
            inner,
            file: Mutex::new(Some(file)),
            t0: Instant::now(),
        })
    }

    fn emit(&self, dir: &str, data: &[u8]) -> Result<()> {
        if data.is_empty() {
            return Ok(());
        }
        let mut f = self.file.lock().unwrap();
        if let Some(file) = f.as_mut() {
            let ev = Value::Object(vec![
                ("t".into(), Value::int(self.t0.elapsed().as_micros() as i64)),
                ("dir".into(), Value::String(dir.into())),
                ("data".into(), Value::String(hex_encode(data))),
            ]);
            writeln!(file, "{}", ev.to_json())?;
        }
        Ok(())
    }
}

impl<L: Link> Link for RecordingLink<L> {
    fn read(&self, timeout: Duration) -> Result<Vec<u8>> {
        let data = self.inner.read(timeout)?;
        self.emit("rx", &data)?;
        Ok(data)
    }

    fn write(&self, data: &[u8]) -> Result<()> {
        self.emit("tx", data)?; // journal first: see the type's comment
        self.inner.write(data)
    }

    fn close(&self) {
        if let Some(mut f) = self.file.lock().unwrap().take() {
            let _ = f.flush(); // the capture is as complete as it gets
        }
        self.inner.close();
    }

    fn is_closed(&self) -> bool {
        self.inner.is_closed()
    }

    fn name(&self) -> String {
        self.inner.name()
    }
}
