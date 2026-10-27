//! `.depzdata` dataset reader (contract 09).
//!
//! A dataset is UTF-8 JSON Lines: line 1 is a header describing the devices on
//! a shared host timeline; every following line is one decoded record. Times
//! (`t`) are host-monotonic µs (the device timestamp mapped through that
//! device's `time_sync`). Readers (stable-)sort the records by `t`.
//!
//! - [`Dataset`] — the reader ([`Dataset::read`] a file, gzip or not; [`Dataset::parse`] text).
//! - [`DatasetWriter`] — the low-level record writer; [`records`] — each kind's record value.
//! - [`SessionRecorder`] — records several devices onto one host timeline ([`Recordable`]).
//! - [`DatasetPlayer`] — paced playback with play / pause / seek / speed.

mod gzip;
mod player;
mod recorder;
pub mod records;
mod writer;

pub use player::{DatasetPlayer, PlayerState, THREAD_NAME as PLAYER_THREAD_NAME};
pub use recorder::{RecordSink, Recordable, SessionRecorder};
pub use writer::{DatasetWriter, SCHEMA};

use std::path::Path;

use crate::json::{is_blank_line, JsonError, Value};

/// Dataset parse failure: a damaged file is the caller's bad argument (contract 09, 07 §3), named
/// by its line.
#[derive(Debug, Clone, PartialEq, Eq)]
pub enum DatasetError {
    /// The file has no header line.
    Empty,
    /// Line `line` (1-based) was not valid JSON.
    Json { line: usize, error: JsonError },
    /// A structural expectation failed on line `line` (not an object, missing field, wrong type, …).
    Structure { line: usize, what: String },
    /// The file could not be read (or its gzip content inflated).
    Io(String),
}

impl std::fmt::Display for DatasetError {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            DatasetError::Empty => write!(f, "empty dataset (no header line)"),
            DatasetError::Json { line, error } => write!(f, "line {}: {}", line, error),
            DatasetError::Structure { line, what } => {
                write!(f, "line {}: dataset structure error: {}", line, what)
            }
            DatasetError::Io(s) => write!(f, "dataset read error: {}", s),
        }
    }
}

impl std::error::Error for DatasetError {}

/// `line` (1-based `lineno`) as the JSON object every dataset line must be.
fn json_object(lineno: usize, line: &str) -> Result<Value, DatasetError> {
    let v = Value::parse(line).map_err(|error| DatasetError::Json {
        line: lineno,
        error,
    })?;
    if v.entries().is_none() {
        return Err(structure(lineno, "not a JSON object"));
    }
    Ok(v)
}

fn structure(line: usize, what: &str) -> DatasetError {
    DatasetError::Structure {
        line,
        what: what.to_string(),
    }
}

/// `v`'s field `k` as the integer it must be (`None` when missing, a fraction, null, …).
fn field_i64(v: &Value, k: &str) -> Option<i64> {
    v.get(k).and_then(Value::as_i64)
}

fn field_str<'a>(v: &'a Value, k: &str) -> Option<&'a str> {
    v.get(k).and_then(Value::as_str)
}

/// A device declared in the dataset header, in `d0`, `d1`, … assignment order.
#[derive(Debug, Clone, PartialEq)]
pub struct Device {
    /// Header key (`d0`, `d1`, …).
    pub id: String,
    pub serial: Option<String>,
    pub sensor_type: Option<String>,
    pub software_name: Option<String>,
    /// `time_sync.offset_us` — `t = device_ts_us − offset_us` (contract 02 §5).
    pub offset_us: Option<i64>,
    pub rtt_us: Option<i64>,
}

/// A decoded record value; unknown kinds, and known ones without their typed field, keep their
/// raw JSON (`Other`).
#[derive(Debug, Clone, PartialEq)]
pub enum RecordValue {
    /// `sr04`: `echo_us` (0xFFFF = timeout), `source` = "once" | "loop".
    Sr04 { echo_us: i64, source: String },
    /// `temperature`: MCU temperature in °C.
    Temperature { celsius: f64 },
    /// Any other (or reserved) kind — the raw `v` object.
    Other(Value),
}

/// One dataset record on the shared host timeline.
#[derive(Debug, Clone, PartialEq)]
pub struct Record {
    /// Device id (`d`), referencing a header device.
    pub device: String,
    /// Host-monotonic µs (`t`).
    pub t: i64,
    /// Record kind (`k`).
    pub kind: String,
    pub value: RecordValue,
}

/// A parsed dataset: header devices plus the record stream, stable-sorted by `t` (records of
/// equal `t` keep their file order).
#[derive(Debug, Clone, PartialEq)]
pub struct Dataset {
    pub schema: String,
    pub created_utc: Option<String>,
    pub note: Option<String>,
    pub devices: Vec<Device>,
    pub records: Vec<Record>,
}

impl Dataset {
    /// Read the `.depzdata` file at `path`; gzip content (by its magic bytes) is inflated.
    pub fn read(path: impl AsRef<Path>) -> Result<Dataset, DatasetError> {
        let raw = std::fs::read(path).map_err(|e| DatasetError::Io(e.to_string()))?;
        let raw = if gzip::is_gzip(&raw) {
            gzip::gunzip(&raw).map_err(DatasetError::Io)?
        } else {
            raw
        };
        let text =
            String::from_utf8(raw).map_err(|e| DatasetError::Io(format!("not UTF-8: {e}")))?;
        Dataset::parse(&text)
    }

    /// Span of the timeline: the last record's `t` minus the first's (0 with fewer than two).
    pub fn duration_us(&self) -> i64 {
        match (self.records.first(), self.records.last()) {
            (Some(a), Some(b)) => b.t - a.t,
            _ => 0,
        }
    }

    /// Parse a `.depzdata` document from its UTF-8 text. A damaged document (contract 09: an empty
    /// one, a header that is not a JSON object or whose `schema` is not `depz.dataset/*`, a record
    /// line that is not an object or whose `d` / `k` are not strings, `t` not an integer, `v` not
    /// an object) is a [`DatasetError`] naming the line; blank lines are skipped; a record of an
    /// unknown `k` (or a known one without its typed field) is kept as [`RecordValue::Other`].
    pub fn parse(text: &str) -> Result<Dataset, DatasetError> {
        let mut lines = text.lines().enumerate();

        let (_, header_line) = lines.next().ok_or(DatasetError::Empty)?;
        let header = json_object(1, header_line)?;

        let schema = match field_str(&header, "schema") {
            Some(s) if s.starts_with("depz.dataset/") => s.to_string(),
            Some(s) => {
                return Err(structure(
                    1,
                    &format!("not a depz.dataset file: schema {s:?}"),
                ))
            }
            None => return Err(structure(1, "header missing schema")),
        };
        let created_utc = header
            .get("created_utc")
            .and_then(Value::as_str)
            .map(str::to_string);
        let note = header
            .get("note")
            .and_then(Value::as_str)
            .map(str::to_string);

        let mut devices = Vec::new();
        if let Some(dev_obj) = header.get("devices") {
            let entries = dev_obj
                .entries()
                .ok_or_else(|| structure(1, "devices is not an object"))?;
            for (id, dv) in entries {
                let time_sync = dv.get("time_sync");
                devices.push(Device {
                    id: id.clone(),
                    serial: dv.get("serial").and_then(Value::as_str).map(str::to_string),
                    sensor_type: dv
                        .get("sensor_type")
                        .and_then(Value::as_str)
                        .map(str::to_string),
                    software_name: dv
                        .get("software_name")
                        .and_then(Value::as_str)
                        .map(str::to_string),
                    offset_us: time_sync
                        .and_then(|ts| ts.get("offset_us"))
                        .and_then(Value::as_i64),
                    rtt_us: time_sync
                        .and_then(|ts| ts.get("rtt_us"))
                        .and_then(Value::as_i64),
                });
            }
        }

        let mut records = Vec::new();
        for (idx, line) in lines {
            if is_blank_line(line) {
                continue;
            }
            let lineno = idx + 1;
            let rec = json_object(lineno, line)?;
            let device = field_str(&rec, "d")
                .ok_or_else(|| structure(lineno, "'d' must be a device id string"))?
                .to_string();
            let t = field_i64(&rec, "t")
                .ok_or_else(|| structure(lineno, "'t' must be an integer (host µs)"))?;
            let kind = field_str(&rec, "k")
                .ok_or_else(|| structure(lineno, "'k' must be a record kind string"))?
                .to_string();
            let v = match rec.get("v") {
                Some(v) if v.entries().is_some() => v.clone(),
                _ => return Err(structure(lineno, "'v' must be an object")),
            };

            // The reader checks only that `v` is an object (contract 09); a known kind whose `v`
            // lacks its typed field is kept like an unknown one, as Other.
            let value = match kind.as_str() {
                "sr04" => match field_i64(&v, "echo_us") {
                    Some(echo_us) => RecordValue::Sr04 {
                        echo_us,
                        source: field_str(&v, "source").unwrap_or("").to_string(),
                    },
                    None => RecordValue::Other(v),
                },
                "temperature" => match v.get("celsius").and_then(Value::as_f64) {
                    Some(celsius) => RecordValue::Temperature { celsius },
                    None => RecordValue::Other(v),
                },
                _ => RecordValue::Other(v),
            };

            records.push(Record {
                device,
                t,
                kind,
                value,
            });
        }

        // Per-device order is near monotonic; a stable sort by t merges the devices.
        records.sort_by_key(|r| r.t);
        Ok(Dataset {
            schema,
            created_utc,
            note,
            devices,
            records,
        })
    }

    /// Records for one device id, in timeline order.
    pub fn records_for<'a>(&'a self, device: &'a str) -> impl Iterator<Item = &'a Record> {
        self.records.iter().filter(move |r| r.device == device)
    }
}
