//! `.depzdata` dataset reader (contract 09).
//!
//! A dataset is UTF-8 JSON Lines: line 1 is a header describing the devices on
//! a shared host timeline; every following line is one decoded record. Times
//! (`t`) are host-monotonic µs (the device timestamp mapped through that
//! device's `time_sync`). Players merge records by `t`.

use crate::json::{JsonError, Value};

/// Dataset parse failure.
#[derive(Debug, Clone, PartialEq, Eq)]
pub enum DatasetError {
    /// The file has no header line.
    Empty,
    /// A line was not valid JSON.
    Json(JsonError),
    /// A structural expectation failed (missing field, wrong type, …).
    Structure(String),
}

impl std::fmt::Display for DatasetError {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            DatasetError::Empty => write!(f, "empty dataset (no header line)"),
            DatasetError::Json(e) => write!(f, "{}", e),
            DatasetError::Structure(s) => write!(f, "dataset structure error: {}", s),
        }
    }
}

impl std::error::Error for DatasetError {}

impl From<JsonError> for DatasetError {
    fn from(e: JsonError) -> Self {
        DatasetError::Json(e)
    }
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

/// A decoded record value; unknown kinds keep their raw JSON (`Other`).
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

/// A parsed dataset: header devices plus the record stream (file order).
#[derive(Debug, Clone, PartialEq)]
pub struct Dataset {
    pub schema: String,
    pub created_utc: Option<String>,
    pub note: Option<String>,
    pub devices: Vec<Device>,
    pub records: Vec<Record>,
}

impl Dataset {
    /// Parse a `.depzdata` document from its UTF-8 text.
    pub fn parse(text: &str) -> Result<Dataset, DatasetError> {
        let mut lines = text
            .lines()
            .map(str::trim)
            .filter(|l| !l.is_empty());

        let header_line = lines.next().ok_or(DatasetError::Empty)?;
        let header = Value::parse(header_line)?;

        let schema = header
            .get("schema")
            .and_then(Value::as_str)
            .ok_or_else(|| DatasetError::Structure("header missing schema".into()))?
            .to_string();
        let created_utc = header
            .get("created_utc")
            .and_then(Value::as_str)
            .map(str::to_string);
        let note = header.get("note").and_then(Value::as_str).map(str::to_string);

        let mut devices = Vec::new();
        if let Some(dev_obj) = header.get("devices") {
            let entries = dev_obj
                .entries()
                .ok_or_else(|| DatasetError::Structure("devices is not an object".into()))?;
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
                    rtt_us: time_sync.and_then(|ts| ts.get("rtt_us")).and_then(Value::as_i64),
                });
            }
        }

        let mut records = Vec::new();
        for line in lines {
            let rec = Value::parse(line)?;
            let device = rec
                .get("d")
                .and_then(Value::as_str)
                .ok_or_else(|| DatasetError::Structure("record missing 'd'".into()))?
                .to_string();
            let t = rec
                .get("t")
                .and_then(Value::as_i64)
                .ok_or_else(|| DatasetError::Structure("record missing 't'".into()))?;
            let kind = rec
                .get("k")
                .and_then(Value::as_str)
                .ok_or_else(|| DatasetError::Structure("record missing 'k'".into()))?
                .to_string();
            let v = rec
                .get("v")
                .cloned()
                .ok_or_else(|| DatasetError::Structure("record missing 'v'".into()))?;

            let value = match kind.as_str() {
                "sr04" => {
                    let echo_us = v.get("echo_us").and_then(Value::as_i64).ok_or_else(|| {
                        DatasetError::Structure("sr04 record missing echo_us".into())
                    })?;
                    let source = v
                        .get("source")
                        .and_then(Value::as_str)
                        .unwrap_or("")
                        .to_string();
                    RecordValue::Sr04 { echo_us, source }
                }
                "temperature" => {
                    let celsius = v.get("celsius").and_then(Value::as_f64).ok_or_else(|| {
                        DatasetError::Structure("temperature record missing celsius".into())
                    })?;
                    RecordValue::Temperature { celsius }
                }
                _ => RecordValue::Other(v),
            };

            records.push(Record {
                device,
                t,
                kind,
                value,
            });
        }

        Ok(Dataset {
            schema,
            created_utc,
            note,
            devices,
            records,
        })
    }

    /// Records for one device id, in file order.
    pub fn records_for<'a>(&'a self, device: &'a str) -> impl Iterator<Item = &'a Record> {
        self.records.iter().filter(move |r| r.device == device)
    }
}
