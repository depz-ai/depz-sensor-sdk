//! Recording decoded data from several devices onto one host timeline (contract 09).

use std::collections::HashMap;
use std::path::{Path, PathBuf};
use std::sync::mpsc::{self, RecvTimeoutError, Sender};
use std::sync::{Arc, Mutex};
use std::thread::JoinHandle;
use std::time::Duration;

use super::records;
use super::writer::DatasetWriter;
use crate::devices::{
    host_now_us, AnyDevice, Bno055Device, Bno086Device, DepzDevice, DepzError, Hook, Result, Sr04Device,
    Tmf8829Device, Vl53l0xDevice, Vl53l1cbDevice, Vl53l1cxDevice, Vl53l3cxDevice, Vl53l4cdDevice, Vl53l4cxDevice,
    Vl53l5cxDevice, Vl53l7chDevice, Vl53l7cxDevice, Vl53l8chDevice, Vl53l8cxDevice, Vl53lxDevice,
};
use crate::json::Value;
use crate::protocol::identity::parse_software_name;

struct Shared {
    writer: Mutex<Option<Arc<DatasetWriter>>>,
    silicon: Mutex<HashMap<String, f64>>,
    vl53l8_layers: bool,
}

impl Shared {
    fn writer(&self) -> Option<Arc<DatasetWriter>> {
        self.writer.lock().unwrap().clone()
    }
}

/// Where a device class's hooks write their records: the device id and time offset of one
/// [`SessionRecorder`] entry. Records written while the recorder is not started are dropped.
#[derive(Clone)]
pub struct RecordSink {
    id: Arc<str>,
    offset_us: i64,
    shared: Arc<Shared>,
}

impl RecordSink {
    /// True while the recorder writes (build a record only then).
    pub fn recording(&self) -> bool {
        self.shared.writer.lock().unwrap().is_some()
    }

    /// One record of `kind` at device time `device_ts_us` (mapped onto the host timeline).
    pub fn write(&self, device_ts_us: i64, kind: &str, value: Value) {
        if let Some(w) = self.shared.writer() {
            // A write after the file closed (a frame racing stop()) is dropped.
            let _ = w.write(&self.id, device_ts_us - self.offset_us, kind, value);
        }
    }

    /// VL53L8 records also carry signal / ambient / sigma / reflectance.
    pub fn vl53l8_layers(&self) -> bool {
        self.shared.vl53l8_layers
    }

    /// The sensor die's latest temperature, sampled into `temperature` records.
    pub fn note_sensor_temperature(&self, celsius: f64) {
        self.shared.silicon.lock().unwrap().insert(self.id.to_string(), celsius);
    }
}

/// A device class a [`SessionRecorder`] can record: its connection, and hooks on its result
/// stream that write each result to the sink.
pub trait Recordable {
    fn depz_device(&self) -> &DepzDevice;

    fn hook_records(&self, sink: RecordSink) -> Vec<Hook>;
}

macro_rules! recordable {
    ($ty:ty, $hook:ident, $kind:literal, $rec:path) => {
        impl Recordable for $ty {
            fn depz_device(&self) -> &DepzDevice {
                self
            }

            fn hook_records(&self, sink: RecordSink) -> Vec<Hook> {
                vec![self.$hook(move |m| {
                    if sink.recording() {
                        sink.write(m.timestamp_us as i64, $kind, $rec(m));
                    }
                })]
            }
        }
    };
}

recordable!(Sr04Device, on_measurement, "sr04", records::sr04);
recordable!(Vl53l4cdDevice, on_measurement, "vl53l4", records::vl53l4);
recordable!(Bno055Device, on_sample, "bno055", records::bno055);
recordable!(Tmf8829Device, on_measurement, "tmf8829", records::tmf8829);

impl Recordable for Bno086Device {
    fn depz_device(&self) -> &DepzDevice {
        self
    }

    // Every enabled sensor's report at its measurement time; reports of one cargo carry
    // out-of-order device times, so t may step back within a device — readers sort (contract 09).
    fn hook_records(&self, sink: RecordSink) -> Vec<Hook> {
        vec![self.on_report(move |r| {
            if sink.recording() {
                sink.write(r.timestamp_us(), "bno086", records::bno086(r));
            }
        })]
    }
}

impl Recordable for Vl53l8cxDevice {
    fn depz_device(&self) -> &DepzDevice {
        self
    }

    fn hook_records(&self, sink: RecordSink) -> Vec<Hook> {
        let cnh = self.armed_cnh_handle();
        vec![self.on_frame(move |f| {
            sink.note_sensor_temperature(f.silicon_temp_degc as f64);
            if !sink.recording() {
                return;
            }
            let t = f.timestamp_us as i64;
            sink.write(t, "vl53l8", records::vl53l8(f, sink.vl53l8_layers()));
            // CH: the raw CNH block riding this frame, with the armed setup's decode parameters.
            if let (Some(raw), Some(setup)) = (f.cnh_raw.as_deref(), cnh.lock().unwrap().as_ref()) {
                if !raw.is_empty() {
                    sink.write(t, "vl53l8_cnh", records::vl53l8_cnh(raw, setup));
                }
            }
        })]
    }
}

macro_rules! recordable_via_deref {
    ($($ty:ty),*) => {$(
        impl Recordable for $ty {
            fn depz_device(&self) -> &DepzDevice {
                self
            }

            fn hook_records(&self, sink: RecordSink) -> Vec<Hook> {
                let core: &Vl53l8cxDevice = self;
                core.hook_records(sink)
            }
        }
    )*};
}

recordable_via_deref!(Vl53l8chDevice, Vl53l7cxDevice, Vl53l5cxDevice, Vl53l7chDevice);

impl Recordable for Vl53lxDevice {
    fn depz_device(&self) -> &DepzDevice {
        self
    }

    // The product and driver as init bound them when the result came.
    fn hook_records(&self, sink: RecordSink) -> Vec<Hook> {
        let bound = self.bound_pair();
        vec![self.on_measurement(move |m| {
            if sink.recording() {
                let (product, driver) = bound().unwrap_or(("", ""));
                sink.write(m.timestamp_us as i64, "vl53lx", records::vl53lx(m, product, driver));
            }
        })]
    }
}

macro_rules! recordable_vl53lx {
    ($($ty:ty),*) => {$(
        impl Recordable for $ty {
            fn depz_device(&self) -> &DepzDevice {
                self
            }

            fn hook_records(&self, sink: RecordSink) -> Vec<Hook> {
                let family: &Vl53lxDevice = self;
                family.hook_records(sink)
            }
        }
    )*};
}

recordable_vl53lx!(Vl53l0xDevice, Vl53l1cxDevice, Vl53l1cbDevice, Vl53l3cxDevice, Vl53l4cxDevice);

impl Recordable for AnyDevice {
    fn depz_device(&self) -> &DepzDevice {
        self.device()
    }

    fn hook_records(&self, sink: RecordSink) -> Vec<Hook> {
        match self {
            AnyDevice::Sr04(d) => d.hook_records(sink),
            AnyDevice::Vl53l4cd(d) => d.hook_records(sink),
            AnyDevice::Vl53l8cx(d) => d.hook_records(sink),
            AnyDevice::Vl53l8ch(d) => d.hook_records(sink),
            AnyDevice::Vl53l5cx(d) => d.hook_records(sink),
            AnyDevice::Vl53l7cx(d) => d.hook_records(sink),
            AnyDevice::Vl53l7ch(d) => d.hook_records(sink),
            AnyDevice::Bno055(d) => d.hook_records(sink),
            AnyDevice::Bno086(d) => d.hook_records(sink),
            AnyDevice::Tmf8829(d) => d.hook_records(sink),
            AnyDevice::Vl53lx(d) => d.hook_records(sink),
            AnyDevice::Vl53l0x(d) => d.hook_records(sink),
            AnyDevice::Vl53l1cx(d) => d.hook_records(sink),
            AnyDevice::Vl53l1cb(d) => d.hook_records(sink),
            AnyDevice::Vl53l3cx(d) => d.hook_records(sink),
            AnyDevice::Vl53l4cx(d) => d.hook_records(sink),
            AnyDevice::Plain(_) => Vec::new(),
        }
    }
}

// Reads the device's MCU temperature (shared with the temperature thread).
type Probe = Arc<dyn Fn() -> Result<f64> + Send + Sync>;

struct Entry {
    id: String,
    meta: Value,
    temperature: Probe,
    _hooks: Vec<Hook>,
}

/// Record decoded data from several devices onto one host timeline (contract 09).
/// [`SessionRecorder::add`] each open device (it runs [`DepzDevice::sync_time`] and hooks the
/// device's result stream), then [`SessionRecorder::start`]: a record per result at device time −
/// offset. It never reconfigures a device — start and stop streaming yourself.
/// [`SessionRecorder::stop`] (or dropping it) closes the file; the hooks go with the recorder.
pub struct SessionRecorder {
    path: PathBuf,
    note: String,
    temperature_hz: Option<f64>,
    shared: Arc<Shared>,
    entries: Vec<Entry>,
    temp_thread: Option<(Sender<()>, JoinHandle<()>)>,
}

impl SessionRecorder {
    /// A recorder writing `path` (`.depzdata`, or `.depzdata.gz`). `vl53l8_layers`: VL53L8 records
    /// also carry signal / ambient / sigma / reflectance. `temperature_hz`: about this many
    /// `temperature` records per device and second (the MCU thermometer, and the sensor die where
    /// frames carry one); `None` for none.
    pub fn new(path: impl AsRef<Path>, note: Option<&str>, vl53l8_layers: bool, temperature_hz: Option<f64>) -> Self {
        SessionRecorder {
            path: path.as_ref().to_path_buf(),
            note: note.unwrap_or("").to_string(),
            temperature_hz,
            shared: Arc::new(Shared { writer: Mutex::new(None), silicon: Mutex::new(HashMap::new()), vl53l8_layers }),
            entries: Vec::new(),
            temp_thread: None,
        }
    }

    /// Register a device before recording starts: runs [`DepzDevice::sync_time`] with
    /// `sync_samples` rounds so its timestamps land on the shared timeline, and hooks its stream.
    /// Returns its device id (`d0`, `d1`, … unless given).
    pub fn add(&mut self, device: &dyn Recordable, device_id: Option<&str>, sync_samples: usize) -> Result<String> {
        if self.recording() {
            return Err(DepzError::InvalidState("add() every device before recording starts".into()));
        }
        let dev = device.depz_device();
        let id = device_id.map_or_else(|| format!("d{}", self.entries.len()), str::to_string);
        let sync = dev.sync_time(sync_samples)?;
        let mut meta = vec![(
            "time_sync".to_string(),
            Value::Object(vec![
                ("offset_us".to_string(), Value::int(sync.offset_us)),
                ("rtt_us".to_string(), Value::int(sync.rtt_us)),
            ]),
        )];
        // A board that does not answer these is still recorded.
        let mut software = String::new();
        if let (Ok(serial), Ok(sw)) = (dev.get_serial_number(), dev.get_software_name()) {
            meta.push(("serial".to_string(), Value::String(serial)));
            meta.push(("software_name".to_string(), Value::String(sw.clone())));
            software = sw;
        }
        let sensor_type = parse_software_name(&software).sensor_type.map_or("unknown", |t| t.as_str());
        meta.push(("sensor_type".to_string(), Value::String(sensor_type.to_string())));
        let sink = RecordSink { id: Arc::from(id.as_str()), offset_us: sync.offset_us, shared: self.shared.clone() };
        let requester = dev.requester();
        self.entries.push(Entry {
            id: id.clone(),
            meta: Value::Object(meta),
            temperature: Arc::new(move || requester.read_mcu_temperature()),
            _hooks: device.hook_records(sink),
        });
        Ok(id)
    }

    fn recording(&self) -> bool {
        self.shared.writer.lock().unwrap().is_some()
    }

    pub fn records_written(&self) -> u64 {
        self.shared.writer().map_or(0, |w| w.records_written())
    }

    /// Open the file and start writing. No-op while recording.
    pub fn start(&mut self) -> Result<()> {
        if self.recording() {
            return Ok(());
        }
        let devices = self.entries.iter().map(|e| (e.id.clone(), e.meta.clone())).collect();
        let writer = Arc::new(DatasetWriter::create(&self.path, devices, Some(&self.note))?);
        *self.shared.writer.lock().unwrap() = Some(writer);
        if let Some(hz) = self.temperature_hz.filter(|&hz| hz > 0.0) {
            let (stop, rx) = mpsc::channel::<()>();
            let period = Duration::from_secs_f64(1.0 / hz);
            let shared = self.shared.clone();
            let probes: Vec<(String, Probe)> = self.entries.iter().map(|e| (e.id.clone(), e.temperature.clone())).collect();
            let handle = std::thread::Builder::new()
                .name("depz-rec-temp".into())
                .spawn(move || temperature_loop(probes, shared, period, rx))
                .map_err(DepzError::Io)?;
            self.temp_thread = Some((stop, handle));
        }
        Ok(())
    }

    /// Stop writing and close the file. Idempotent.
    pub fn stop(&mut self) -> Result<()> {
        if let Some((stop, handle)) = self.temp_thread.take() {
            let _ = stop.send(());
            let _ = handle.join();
        }
        let writer = self.shared.writer.lock().unwrap().take();
        if let Some(w) = writer {
            w.close()?;
        }
        Ok(())
    }
}

impl Drop for SessionRecorder {
    fn drop(&mut self) {
        let _ = self.stop();
    }
}

fn temperature_record(celsius: f64, source: &str) -> Value {
    Value::Object(vec![
        ("celsius".to_string(), Value::float(celsius)),
        ("source".to_string(), Value::String(source.into())),
    ])
}

fn temperature_loop(probes: Vec<(String, Probe)>, shared: Arc<Shared>, period: Duration, stop: mpsc::Receiver<()>) {
    loop {
        match stop.recv_timeout(period) {
            Err(RecvTimeoutError::Timeout) => {}
            _ => return,
        }
        let Some(w) = shared.writer() else { return };
        for (id, read) in &probes {
            let silicon = shared.silicon.lock().unwrap().get(id).copied();
            if let Some(c) = silicon {
                let _ = w.write(id, host_now_us(), "temperature", temperature_record(c, "sensor"));
            }
            if let Ok(c) = read() {
                let _ = w.write(id, host_now_us(), "temperature", temperature_record(c, "mcu"));
            }
        }
    }
}
