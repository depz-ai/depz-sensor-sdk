//! The low-level `.depzdata` record writer.

use std::fs::File;
use std::io::{self, BufWriter, Write};
use std::path::Path;
use std::sync::Mutex;
use std::time::{SystemTime, UNIX_EPOCH};

use super::gzip::GzWriter;
use crate::json::Value;

/// The schema id this SDK writes.
pub const SCHEMA: &str = "depz.dataset/2";

enum Sink {
    Plain(BufWriter<File>),
    Gzip(GzWriter<BufWriter<File>>),
}

impl Sink {
    fn out(&mut self) -> &mut dyn Write {
        match self {
            Sink::Plain(w) => w,
            Sink::Gzip(w) => w,
        }
    }

    fn finish(self) -> io::Result<()> {
        match self {
            Sink::Plain(mut w) => w.flush(),
            Sink::Gzip(w) => w.finish().map(|_| ()),
        }
    }
}

struct State {
    sink: Option<Sink>,
    records: u64,
}

/// Low-level thread-safe `.depzdata` record writer (a gzip container when the path ends in `.gz`,
/// stored uncompressed: the crate has no compression dependency). Prefer
/// [`super::SessionRecorder`]. Dropping it closes the file.
pub struct DatasetWriter {
    state: Mutex<State>,
}

impl DatasetWriter {
    /// Create `path` (overwritten) and write the header: `devices` are the header device entries by
    /// device id (`serial`, `sensor_type`, `software_name`, `time_sync`, …), `note` free text.
    pub fn create(path: impl AsRef<Path>, devices: Vec<(String, Value)>, note: Option<&str>) -> io::Result<DatasetWriter> {
        let path = path.as_ref();
        let file = BufWriter::new(File::create(path)?);
        let gz = path.file_name().is_some_and(|n| n.to_string_lossy().to_lowercase().ends_with(".gz"));
        let mut sink = if gz { Sink::Gzip(GzWriter::new(file)?) } else { Sink::Plain(file) };
        let mut header = vec![
            ("schema".to_string(), Value::String(SCHEMA.into())),
            ("created_utc".to_string(), Value::String(utc_now())),
            ("devices".to_string(), Value::Object(devices)),
        ];
        if let Some(n) = note.filter(|n| !n.is_empty()) {
            header.push(("note".to_string(), Value::String(n.into())));
        }
        writeln!(sink.out(), "{}", Value::Object(header).to_json())?;
        Ok(DatasetWriter { state: Mutex::new(State { sink: Some(sink), records: 0 }) })
    }

    pub fn records_written(&self) -> u64 {
        self.state.lock().unwrap().records
    }

    /// One record: device id, host µs, kind, value. Fails once the writer is closed.
    pub fn write(&self, device_id: &str, t_host_us: i64, kind: &str, value: Value) -> io::Result<()> {
        let rec = Value::Object(vec![
            ("d".to_string(), Value::String(device_id.into())),
            ("t".to_string(), Value::int(t_host_us)),
            ("k".to_string(), Value::String(kind.into())),
            ("v".to_string(), value),
        ]);
        let line = rec.to_json();
        let mut st = self.state.lock().unwrap();
        let sink = st.sink.as_mut().ok_or_else(|| io::Error::other("DatasetWriter is closed"))?;
        writeln!(sink.out(), "{line}")?;
        st.records += 1;
        Ok(())
    }

    /// Flush and close the file (a gzip file gets its trailer). Idempotent.
    pub fn close(&self) -> io::Result<()> {
        let sink = self.state.lock().unwrap().sink.take();
        sink.map_or(Ok(()), Sink::finish)
    }
}

impl Drop for DatasetWriter {
    fn drop(&mut self) {
        let _ = self.close();
    }
}

/// Now as `YYYY-MM-DDTHH:MM:SSZ`.
fn utc_now() -> String {
    let secs = SystemTime::now().duration_since(UNIX_EPOCH).map_or(0, |d| d.as_secs()) as i64;
    let (days, rem) = (secs.div_euclid(86_400), secs.rem_euclid(86_400));
    // Civil date from days since 1970-01-01 (Howard Hinnant's algorithm).
    let z = days + 719_468;
    let era = z.div_euclid(146_097);
    let doe = z - era * 146_097;
    let yoe = (doe - doe / 1460 + doe / 36_524 - doe / 146_096) / 365;
    let doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    let mp = (5 * doy + 2) / 153;
    let d = doy - (153 * mp + 2) / 5 + 1;
    let m = if mp < 10 { mp + 3 } else { mp - 9 };
    let y = yoe + era * 400 + (m <= 2) as i64;
    format!("{y:04}-{m:02}-{d:02}T{:02}:{:02}:{:02}Z", rem / 3600, rem / 60 % 60, rem % 60)
}
