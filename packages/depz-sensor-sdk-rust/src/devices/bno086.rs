//! BNO085 / BNO086 9-axis IMU (contract 05): the SH-2 hub session over the SHTP pass-through bridge.

use std::collections::HashMap;
use std::ops::Deref;
use std::sync::atomic::{AtomicU8, Ordering};
use std::sync::mpsc::{channel, Receiver, RecvTimeoutError, Sender};
use std::sync::{Arc, Condvar, Mutex};
use std::time::{Duration, Instant};

use super::device::{DepzDevice, ReportHandler};
use super::error::{DepzError, Result};
use super::stream::{deadline_after, Stream};
use crate::bno086::hub::{
    self, Bno086Cmd, Bno086Data, CalibrationConfig, CommandResponse, Counts, ErrorRecord,
    FeatureResponse, FrsReadResponse, FrsReadSession, FrsWriteResponse, FrsWriteSession,
    MotionIntent, ProductId, SensorMetadata, Sh2HubError, TareBasis,
};
use crate::bno086::{
    parse_gyro_rv_cargo, parse_input_cargo, sh2, shtp, Report, ShtpChannel, ShtpLayer,
};
use crate::framing::Packet;

/// Granted rates outside [0.9, 2.1] × the requested one are worth a warning, never an error
/// (contract 05 §7).
pub const RATE_LOW_FACTOR: f64 = 0.9;
pub const RATE_HIGH_FACTOR: f64 = 2.1;

// Re-reads while enable's verify reads a rate outside the band (contract 05 §7): a stale response of a
// preceding disable, or the hub answering before it runs the new rate — interval 0 once or twice on the
// lab BNO085, and for up to 95 ms (0, or a slower rate) on a hub just reset (bench 2026-11-12). A pause
// before each: about 0.4 s in all, four times the longest seen.
const VERIFY_REREADS: usize = 15;
const VERIFY_PAUSE: Duration = Duration::from_millis(20);
const DEFAULT_TIMEOUT: Duration = Duration::from_secs(1);
const FRS_TIMEOUT: Duration = Duration::from_secs(2);
const RESET_TIMEOUT: Duration = Duration::from_secs(2);
const PRODUCT_ID_SETTLE: Duration = Duration::from_millis(150);

impl From<Sh2HubError> for DepzError {
    fn from(e: Sh2HubError) -> Self {
        DepzError::Sensor(e.0)
    }
}

/// A control response the hub answered a request with, decoded; one shorter than its fields fails
/// that request with [`DepzError::InvalidReply`] (`invalid_reply`, contract 07 §3), never a sensor error.
fn decode_reply<T>(
    raw: &[u8],
    unpack: impl Fn(&[u8]) -> std::result::Result<T, Sh2HubError>,
) -> Result<T> {
    unpack(raw).map_err(|e| DepzError::InvalidReply(e.0))
}

/// Why the hub's waits end: the reader ended on its own (the device is lost, with the cause) or
/// `close()` ended it. The first cause wins.
#[derive(Clone)]
enum Failure {
    Lost(String),
    Closed,
}

impl Failure {
    /// The error every bridge request fails with in the same state (contract 07 §3).
    fn error(&self) -> DepzError {
        match self {
            Failure::Lost(m) => DepzError::DeviceLost(m.clone()),
            Failure::Closed => DepzError::LinkClosed("device closed".into()),
        }
    }
}

/// One waiter: its id (for [`Waiters::forget`]) and where its answers go.
type Waiter<T> = (u64, Sender<Result<T>>);

struct WaiterTable<T> {
    /// Per key (a report id, or the flushed sensor): the waiters listening.
    by_key: HashMap<u8, Vec<Waiter<T>>>,
    /// Set once the device closed or was lost: every wait, running or started later, ends
    /// with it (contract 07 §3; the Python `_Waiters`).
    failure: Option<Failure>,
}

/// Who waits for which SH-2 answer. The reader posts every control report under its report id
/// (and every Flush Completed under its sensor); a caller listens for one key for the span of a
/// wait. One lock guards the table; the channels are thread-safe on their own.
struct Waiters<T> {
    table: Mutex<WaiterTable<T>>,
    next_id: Mutex<u64>,
}

impl<T: Clone> Waiters<T> {
    fn new() -> Waiters<T> {
        Waiters {
            table: Mutex::new(WaiterTable {
                by_key: HashMap::new(),
                failure: None,
            }),
            next_id: Mutex::new(0),
        }
    }

    /// Listen for `key`: the waiter id (for [`Waiters::forget`]) and its channel — or the
    /// failure the device already ended with, so a wait started after it fails at once.
    fn listen(&self, key: u8) -> Result<(u64, Receiver<Result<T>>)> {
        let mut t = self.table.lock().unwrap();
        if let Some(f) = &t.failure {
            return Err(f.error());
        }
        let id = {
            let mut n = self.next_id.lock().unwrap();
            *n += 1;
            *n
        };
        let (tx, rx) = channel();
        t.by_key.entry(key).or_default().push((id, tx));
        Ok((id, rx))
    }

    fn forget(&self, key: u8, id: u64) {
        if let Some(l) = self.table.lock().unwrap().by_key.get_mut(&key) {
            l.retain(|(i, _)| *i != id);
        }
    }

    /// An answer under `key`, to every waiter listening now.
    fn post(&self, key: u8, item: &T) {
        let waiting: Vec<Sender<Result<T>>> = self
            .table
            .lock()
            .unwrap()
            .by_key
            .get(&key)
            .map(|l| l.iter().map(|(_, s)| s.clone()).collect())
            .unwrap_or_default();
        for w in waiting {
            let _ = w.send(Ok(item.clone()));
        }
    }

    /// The device closed or was lost: release every waiter with the failure's error, and refuse
    /// every later wait with it.
    fn fail_all(&self, failure: Failure) {
        let (failure, waiting): (Failure, Vec<Sender<Result<T>>>) = {
            let mut t = self.table.lock().unwrap();
            let failure = t.failure.get_or_insert(failure).clone();
            let waiting = t
                .by_key
                .values()
                .flat_map(|l| l.iter().map(|(_, s)| s.clone()))
                .collect();
            (failure, waiting)
        };
        for w in waiting {
            let _ = w.send(Err(failure.error()));
        }
    }

    fn failure(&self) -> Option<Failure> {
        self.table.lock().unwrap().failure.clone()
    }
}

/// `rx`'s next answer within `deadline` (`None`: no deadline), else the timeout error naming
/// `rid`; a failure posted by the reader is its error.
fn take<T>(
    rx: &Receiver<Result<T>>,
    deadline: Option<Instant>,
    rid: u8,
    timeout: Duration,
) -> Result<T> {
    let got = match deadline {
        None => rx.recv().map_err(|_| RecvTimeoutError::Disconnected),
        Some(d) => rx.recv_timeout(d.saturating_duration_since(Instant::now())),
    };
    got.map_err(|_| DepzError::timeout(rid, timeout))?
}

struct Hub {
    shtp: Mutex<ShtpLayer>,
    control_waiters: Waiters<Vec<u8>>,
    flush_waiters: Waiters<()>,
    reports: Stream<Report>,
    advertisement: Mutex<Vec<u8>>,
    features: Mutex<HashMap<u8, FeatureResponse>>,
    reset_complete: (Mutex<bool>, Condvar),
}

impl Hub {
    fn dispatch_cargo(&self, cargo: shtp::ShtpCargo) {
        let payload = &cargo.payload;
        match cargo.channel {
            c if c == ShtpChannel::Command as u8 => self
                .advertisement
                .lock()
                .unwrap()
                .extend_from_slice(payload),
            c if c == ShtpChannel::Executable as u8 => {
                if payload.first() == Some(&hub::EXECUTABLE_RESET_COMPLETE) {
                    let (m, cv) = &self.reset_complete;
                    *m.lock().unwrap() = true;
                    cv.notify_all();
                }
            }
            c if c == ShtpChannel::Control as u8 => {
                for report in hub::split_control_cargo(payload) {
                    let rid = report[0];
                    if rid == sh2::REPORT_GET_FEATURE_RESPONSE && report.len() >= 17 {
                        if let Ok(resp) = FeatureResponse::unpack(&report) {
                            self.features.lock().unwrap().insert(resp.sensor_id, resp);
                        }
                    }
                    self.control_waiters.post(rid, &report);
                }
            }
            c if c == ShtpChannel::InputNormal as u8 || c == ShtpChannel::InputWake as u8 => {
                for rep in parse_input_cargo(payload, cargo.capture_us as i64) {
                    self.emit_report(rep);
                }
            }
            c if c == ShtpChannel::GyroRv as u8 => {
                if let Some(rep) = parse_gyro_rv_cargo(payload, cargo.capture_us as i64) {
                    self.emit_report(rep);
                }
            }
            _ => {}
        }
    }

    fn emit_report(&self, rep: Report) {
        if let Report::FlushCompleted { flushed_sensor, .. } = rep {
            self.flush_waiters.post(flushed_sensor, &());
        }
        self.reports.emit(rep);
    }
}

impl ReportHandler for Hub {
    fn handle_report(&self, packet: &Packet) -> bool {
        if packet.cmd != hub::RPT_DATA {
            return false;
        }
        // shorter than its 9-byte header: refused and counted — contract 07 §2
        let Ok(data) = Bno086Data::unpack(&packet.payload) else {
            self.reports.count_parse_error();
            return true;
        };
        let cargo = self
            .shtp
            .lock()
            .unwrap()
            .feed_at(&data.shtp, data.timestamp_us);
        if let Some(c) = cargo {
            self.dispatch_cargo(c); // the first fragment's capture: 0xFB base deltas refer to it
        }
        true
    }

    // The reader ended: the SH-2 waits end with the same error as the bridge requests, at
    // once (contract 07 §3) — not after their own 1–5 s timeout (bug hunt 2026-11-03 RUST-1).
    fn reader_stopped(&self, lost: Option<&str>) {
        let failure = lost.map_or(Failure::Closed, |m| Failure::Lost(m.to_string()));
        self.control_waiters.fail_all(failure.clone());
        self.flush_waiters.fail_all(failure);
        // a reset wait wakes and finds the failure
        let (m, cv) = &self.reset_complete;
        *m.lock().unwrap() = true;
        cv.notify_all();
        self.reports.close(lost);
    }
}

// A registered control-response waiter; dropping it unregisters.
struct Wait<'a> {
    hub: &'a Hub,
    rid: u8,
    id: u64,
    rx: Receiver<Result<Vec<u8>>>,
}

impl Drop for Wait<'_> {
    fn drop(&mut self) {
        self.hub.control_waiters.forget(self.rid, self.id);
    }
}

impl Wait<'_> {
    /// The next report within `deadline` (`None`: no deadline), else the timeout error naming
    /// the report; the device closing or lost meanwhile is its error.
    fn next(&self, deadline: Option<Instant>, timeout: Duration) -> Result<Vec<u8>> {
        take(&self.rx, deadline, self.rid, timeout)
    }
}

/// BNO085 / BNO086 9-axis IMU (contract 05). The board only shuttles raw SHTP frames
/// (SEND_SHTP_PACKET / RPT_DATA); the SH-2 hub protocol runs here, ported from the reference SDK.
/// Every send is acknowledged by the bridge at once and every inbound frame arrives as
/// RPT_DATA(cmd 0x00), so replies are correlated at the SH-2 layer only. Never call a blocking device
/// method (`enable`, `tare_now`, …) from a [`Bno086Device::reports`] callback.
pub struct Bno086Device {
    dev: DepzDevice,
    hub: Arc<Hub>,
    tx: Mutex<()>, // serialise SEND_SHTP_PACKET
    cmd_seq: AtomicU8,
    busy: Mutex<(u32, Duration)>,
}

impl Deref for Bno086Device {
    type Target = DepzDevice;

    fn deref(&self) -> &DepzDevice {
        &self.dev
    }
}

impl Bno086Device {
    /// Continue the open connection `dev` as a BNO085 / BNO086.
    pub fn attach(dev: DepzDevice) -> Bno086Device {
        let hub = Arc::new(Hub {
            shtp: Mutex::new(ShtpLayer::new()),
            control_waiters: Waiters::new(),
            flush_waiters: Waiters::new(),
            reports: dev.stream(hub::RPT_DATA),
            advertisement: Mutex::new(Vec::new()),
            features: Mutex::new(HashMap::new()),
            reset_complete: (Mutex::new(false), Condvar::new()),
        });
        dev.set_report_handler(hub.clone());
        Bno086Device {
            dev,
            hub,
            tx: Mutex::new(()),
            cmd_seq: AtomicU8::new(0),
            busy: Mutex::new((5, Duration::from_millis(hub::BUSY_BACKOFF_MS))),
        }
    }

    /// SEND_SHTP_PACKET attempts on ERR_BUSY before giving up, and the back-off between them (at least
    /// 200 ms per the bridge spec).
    pub fn set_busy_retry(&self, retries: u32, backoff: Duration) -> Result<()> {
        if retries < 1 {
            return Err(DepzError::InvalidArgument(format!(
                "busy retries must be at least 1, got {retries}"
            )));
        }
        *self.busy.lock().unwrap() = (retries, backoff);
        Ok(())
    }

    /// The raw SHTP channel-0 advertisement seen since open or reset.
    pub fn advertisement(&self) -> Vec<u8> {
        self.hub.advertisement.lock().unwrap().clone()
    }

    // ── TX path ──────────────────────────────────────────────────────────────

    // Frame the payload and push it through SEND_SHTP_PACKET; ERR_BUSY backs off and resends.
    fn send_shtp(&self, channel: ShtpChannel, payload: &[u8]) -> Result<()> {
        // The TX lock is held from the sequence number to the OK, across the retries (the reference's
        // _send_shtp; bug hunt 2026-11-03 JAVA-11): a sender let in between would put frame n+1 first.
        let _tx = self.tx.lock().unwrap();
        let frame = self
            .hub
            .shtp
            .lock()
            .unwrap()
            .try_next_frame(channel as u8, payload)
            .map_err(|e| DepzError::InvalidArgument(e.to_string()))?;
        let (retries, backoff) = *self.busy.lock().unwrap();
        let mut attempt = 0;
        loop {
            match self.request_ok(Bno086Cmd::SendShtpPacket as u8, &frame, None) {
                Err(e) if e.is_busy() && attempt < retries - 1 => {
                    attempt += 1;
                    std::thread::sleep(backoff);
                }
                r => return r,
            }
        }
    }

    // Listen for the control response `rid` — or fail at once on a device that closed or was lost.
    fn wait(&self, rid: u8) -> Result<Wait<'_>> {
        let (id, rx) = self.hub.control_waiters.listen(rid)?;
        Ok(Wait {
            hub: &self.hub,
            rid,
            id,
            rx,
        })
    }

    // Send a control payload and wait for the first response of report id `rid` that `accept` takes.
    fn control_request<T>(
        &self,
        payload: &[u8],
        rid: u8,
        parse: impl Fn(&[u8]) -> std::result::Result<T, Sh2HubError>,
        accept: impl Fn(&T) -> bool,
        timeout: Duration,
    ) -> Result<T> {
        let w = self.wait(rid)?;
        self.send_shtp(ShtpChannel::Control, payload)?;
        let deadline = deadline_after(timeout);
        loop {
            let obj = decode_reply(&w.next(deadline, timeout)?, &parse)?;
            if accept(&obj) {
                return Ok(obj);
            }
        }
    }

    fn next_cmd_seq(&self) -> u8 {
        self.cmd_seq.fetch_add(1, Ordering::SeqCst)
    }

    // A Command Request with its response, correlated on (command, command_seq).
    fn command(
        &self,
        command: u8,
        params: &[u8],
        timeout: Option<Duration>,
    ) -> Result<CommandResponse> {
        let seq = self.next_cmd_seq();
        let payload = sh2::build_command_request(seq, command, params)
            .map_err(|e| DepzError::InvalidArgument(e.to_string()))?;
        self.control_request(
            &payload,
            sh2::REPORT_COMMAND_RESPONSE,
            CommandResponse::unpack,
            |r| r.command == command && r.command_seq == seq,
            timeout.unwrap_or(DEFAULT_TIMEOUT),
        )
    }

    // A Command Request the hub does not answer.
    fn command_no_response(&self, command: u8, params: &[u8]) -> Result<()> {
        let payload = sh2::build_command_request(self.next_cmd_seq(), command, params)
            .map_err(|e| DepzError::InvalidArgument(e.to_string()))?;
        self.send_shtp(ShtpChannel::Control, &payload)
    }

    // Gather every Command Response of one request until `collect` says done (errors, counts).
    fn command_collect(
        &self,
        command: u8,
        params: &[u8],
        mut collect: impl FnMut(CommandResponse, &mut Vec<CommandResponse>) -> bool,
        timeout: Duration,
    ) -> Result<Vec<CommandResponse>> {
        let seq = self.next_cmd_seq();
        let payload = sh2::build_command_request(seq, command, params)
            .map_err(|e| DepzError::InvalidArgument(e.to_string()))?;
        let w = self.wait(sh2::REPORT_COMMAND_RESPONSE)?;
        self.send_shtp(ShtpChannel::Control, &payload)?;
        let deadline = deadline_after(timeout);
        let mut results = Vec::new();
        loop {
            let resp = decode_reply(&w.next(deadline, timeout)?, CommandResponse::unpack)?;
            if resp.command != command || resp.command_seq != seq {
                continue;
            }
            if collect(resp, &mut results) {
                return Ok(results);
            }
        }
    }

    fn forget_hub_state(&self) {
        *self.hub.reset_complete.0.lock().unwrap() = false;
        self.hub.shtp.lock().unwrap().reset();
        self.hub.advertisement.lock().unwrap().clear();
        self.hub.features.lock().unwrap().clear();
    }

    fn wait_reset_complete(&self, timeout: Duration) -> bool {
        let (m, cv) = &self.hub.reset_complete;
        let g = m.lock().unwrap();
        *cv.wait_timeout_while(g, timeout, |done| !*done).unwrap().0
    }

    // ── lifecycle ────────────────────────────────────────────────────────────

    /// Hard-reset the sensor through nRST. The bridge's OK confirms it; the SH-2 executable
    /// reset-complete is waited for best-effort (firmware ≤ 0.95 never sent it, ERRATA E9). SHTP state
    /// and cached features restart from zero. `timeout` `None`: 2 s.
    pub fn hardware_reset(&self, timeout: Option<Duration>) -> Result<()> {
        let t = timeout.unwrap_or(RESET_TIMEOUT);
        self.forget_hub_state();
        self.request_ok(Bno086Cmd::SensorReset as u8, &[], Some(t))?;
        self.wait_reset_complete(t.min(Duration::from_millis(500)));
        Ok(())
    }

    /// Pulse WAKE (PS0) — the board's pin, not the SH-2 "On" that ends [`Bno086Device::sleep`] (that
    /// is [`Bno086Device::power_on`]).
    pub fn wake(&self) -> Result<()> {
        self.request_ok(Bno086Cmd::SensorWakeUp as u8, &[], None)
    }

    /// SH-2 "Sleep": every report stops except those enabled with wake-up | always-on; the
    /// configuration is kept. No response.
    pub fn sleep(&self) -> Result<()> {
        self.send_shtp(ShtpChannel::Executable, &shtp::build_sleep())
    }

    /// SH-2 "On": ends [`Bno086Device::sleep`]; every enabled report comes back. No response.
    pub fn power_on(&self) -> Result<()> {
        self.send_shtp(ShtpChannel::Executable, &shtp::build_power_on())
    }

    // ── identification and features ──────────────────────────────────────────

    /// The SH-2 application's Product ID — the first of [`Bno086Device::product_ids`].
    pub fn product_id(&self, timeout: Option<Duration>) -> Result<ProductId> {
        Ok(self.product_ids(timeout, None)?[0])
    }

    /// Every component's Product ID Response, the SH-2 application first. Collects until no response
    /// came for `settle` (default 150 ms), so late ones cannot answer the next request.
    pub fn product_ids(
        &self,
        timeout: Option<Duration>,
        settle: Option<Duration>,
    ) -> Result<Vec<ProductId>> {
        let rid = sh2::REPORT_PRODUCT_ID_RESPONSE;
        let w = self.wait(rid)?;
        self.send_shtp(ShtpChannel::Control, &sh2::build_product_id_request())?;
        let t = timeout.unwrap_or(DEFAULT_TIMEOUT);
        let mut out = vec![decode_reply(
            &w.next(deadline_after(t), t)?,
            ProductId::unpack,
        )?];
        let s = settle.unwrap_or(PRODUCT_ID_SETTLE);
        while let Ok(more) = w.rx.recv_timeout(s) {
            out.push(decode_reply(&more?, ProductId::unpack)?);
        }
        Ok(out)
    }

    /// Hz → the Set Feature interval, rounded like the reference (half to even), at least 1 µs. A
    /// rate whose interval does not fit the u32 of the wire is [`DepzError::InvalidArgument`].
    pub fn interval_us(hz: f64) -> Result<u32> {
        if !hz.is_finite() || hz <= 0.0 {
            return Err(DepzError::InvalidArgument(format!(
                "hz must be positive and finite, got {hz}; use disable()"
            )));
        }
        let us = (1_000_000.0 / hz).round_ties_even().max(1.0);
        if us > u32::MAX as f64 {
            return Err(DepzError::InvalidArgument(format!(
                "rate {hz} Hz: the report interval {us} µs does not fit the u32 wire field"
            )));
        }
        Ok(us as u32)
    }

    /// Enable `sensor` through Set Feature (0xFD) every `interval_us`. The hub rounds to its grid;
    /// with `verify` the granted rate is read back through Get Feature and a rate outside 0.9–2.1 × the
    /// request is reported on stderr as a warning (contract 05 §7). Returns that read-back (`None`
    /// without verify).
    #[allow(clippy::too_many_arguments)] // the Python SDK's enable() keyword list, kept 1:1
    pub fn enable_interval(
        &self,
        sensor: u8,
        interval_us: u32,
        batch_us: u32,
        sensitivity: u16,
        flags: u8,
        cfg_word: u32,
        verify: bool,
        timeout: Option<Duration>,
    ) -> Result<Option<FeatureResponse>> {
        self.send_shtp(
            ShtpChannel::Control,
            &sh2::build_set_feature(sensor, interval_us, batch_us, sensitivity, flags, cfg_word),
        )?;
        if !verify {
            return Ok(None);
        }
        let resp = self.verify_feature(sensor, interval_us, timeout.unwrap_or(DEFAULT_TIMEOUT))?;
        if !Self::rate_in_band(interval_us, resp.interval_us) {
            let (requested, actual) = (Self::rate_hz(interval_us), Self::rate_hz(resp.interval_us));
            eprintln!(
                "warning: BNO086 sensor 0x{sensor:02X}: requested {requested:.1} Hz, granted {actual:.1} Hz (outside {RATE_LOW_FACTOR}–{RATE_HIGH_FACTOR}× band)"
            );
        }
        Ok(Some(resp))
    }

    /// [`Bno086Device::enable_interval`] at `hz`, verified.
    pub fn enable(&self, sensor: u8, hz: f64) -> Result<FeatureResponse> {
        Ok(self
            .enable_interval(sensor, Self::interval_us(hz)?, 0, 0, 0, 0, true, None)?
            .unwrap())
    }

    // The rate of a report interval; 0 (disabled) is 0 Hz.
    fn rate_hz(interval_us: u32) -> f64 {
        if interval_us != 0 {
            1_000_000.0 / interval_us as f64
        } else {
            0.0
        }
    }

    // Whether a granted interval is the rate asked for, within the band of contract 05 §7.
    fn rate_in_band(requested_us: u32, granted_us: u32) -> bool {
        let (requested, actual) = (Self::rate_hz(requested_us), Self::rate_hz(granted_us));
        RATE_LOW_FACTOR * requested <= actual && actual <= RATE_HIGH_FACTOR * requested
    }

    // Read back a feature just enabled until the hub runs it: an answer outside the band is stale, early
    // or a refusal (see VERIFY_REREADS).
    fn verify_feature(
        &self,
        sensor: u8,
        interval_us: u32,
        timeout: Duration,
    ) -> Result<FeatureResponse> {
        let mut resp = self.get_feature(sensor, Some(timeout))?;
        for _ in 0..VERIFY_REREADS {
            if Self::rate_in_band(interval_us, resp.interval_us) {
                break;
            }
            std::thread::sleep(VERIFY_PAUSE);
            resp = self.get_feature(sensor, Some(timeout))?;
        }
        Ok(resp)
    }

    /// Disable `sensor` (Set Feature with interval 0).
    pub fn disable(&self, sensor: u8) -> Result<()> {
        self.send_shtp(
            ShtpChannel::Control,
            &sh2::build_set_feature(sensor, 0, 0, 0, 0, 0),
        )?;
        self.hub.features.lock().unwrap().remove(&sensor);
        Ok(())
    }

    /// Get Feature round trip: the rates in effect for `sensor`.
    pub fn get_feature(&self, sensor: u8, timeout: Option<Duration>) -> Result<FeatureResponse> {
        self.control_request(
            &sh2::build_get_feature_request(sensor),
            sh2::REPORT_GET_FEATURE_RESPONSE,
            FeatureResponse::unpack,
            |r| r.sensor_id == sensor,
            timeout.unwrap_or(DEFAULT_TIMEOUT),
        )
    }

    // ── reports ──────────────────────────────────────────────────────────────

    /// The report stream (contract 07 §3): `reports().on(cb)` calls back on the reader thread,
    /// `reports().subscribe(capacity)` is a bounded drop-oldest queue subscribed at the call,
    /// `reports().next(timeout)` waits for one; `on_where` / `subscribe_where` keep the reports a
    /// predicate accepts (`|r| r.sensor_id() == 0x05`, say); `dropped_counts()` and
    /// `parse_errors()` (RPT_DATA shorter than its 9-byte header) are its counters.
    pub fn reports(&self) -> &Stream<Report> {
        &self.hub.reports
    }

    // ── tare and calibration ─────────────────────────────────────────────────

    /// Tare `axes` (a `hub::TARE_*` bitmap) against `basis` (no response).
    pub fn tare_now(&self, axes: u8, basis: TareBasis) -> Result<()> {
        self.command_no_response(hub::CMD_TARE, &hub::tare_now_params(axes, basis))
    }

    /// Persist the current tare into FRS (no response).
    pub fn persist_tare(&self) -> Result<()> {
        self.command_no_response(hub::CMD_TARE, &hub::persist_tare_params())
    }

    /// The runtime reorientation quaternion (Q14 on the wire; all zeros clears; no response). It is
    /// applied on the hub's side, in the hub's own axes: x and y swapped and z negated relative to the
    /// reports.
    pub fn set_reorientation(&self, x: f64, y: f64, z: f64, w: f64) -> Result<()> {
        let p = hub::set_reorientation_params(x, y, z, w)
            .map_err(|e| DepzError::InvalidArgument(e.0))?;
        self.command_no_response(hub::CMD_TARE, &p)
    }

    /// Configure the motion-engine calibration; fails on a non-zero status.
    pub fn set_calibration(&self, accel: bool, gyro: bool, mag: bool, planar: bool) -> Result<()> {
        let resp = self.command(
            hub::CMD_ME_CALIBRATE,
            &hub::me_calibration_params(accel, gyro, mag, planar, 0),
            None,
        )?;
        if resp.status() != 0 {
            return Err(DepzError::Sensor(format!(
                "ME calibration configure failed: status {}",
                resp.status()
            )));
        }
        Ok(())
    }

    /// Which motion-engine calibrations run.
    pub fn get_calibration(&self) -> Result<CalibrationConfig> {
        let resp = self.command(
            hub::CMD_ME_CALIBRATE,
            &hub::me_calibration_params(false, false, false, false, hub::ME_CAL_GET),
            None,
        )?;
        if resp.status() != 0 {
            return Err(DepzError::Sensor(format!(
                "ME calibration get failed: status {}",
                resp.status()
            )));
        }
        Ok(CalibrationConfig {
            accel: resp.r[1] != 0,
            gyro: resp.r[2] != 0,
            mag: resp.r[3] != 0,
            planar: resp.r[4] != 0,
        })
    }

    /// Save the dynamic calibration data to flash (DCD Save Now).
    pub fn save_dcd(&self) -> Result<()> {
        let resp = self.command(hub::CMD_SAVE_DCD, &[], None)?;
        if resp.status() != 0 {
            return Err(DepzError::Sensor(format!(
                "DCD save failed: status {}",
                resp.status()
            )));
        }
        Ok(())
    }

    /// The hub's periodic DCD autosave (no response).
    pub fn configure_periodic_dcd(&self, enable: bool) -> Result<()> {
        self.command_no_response(
            hub::CMD_PERIODIC_DCD_CONFIG,
            &hub::periodic_dcd_params(enable),
        )
    }

    /// Interactive Calibration (0x0E, BNO086 only): how the device is about to move, so the hub can
    /// refine the gyro zero-rate offset while it is still. No response; a BNO085 ignores it.
    pub fn set_motion_intent(&self, intent: MotionIntent) -> Result<()> {
        self.command_no_response(
            hub::CMD_INTERACTIVE_CALIBRATION,
            &hub::interactive_calibration_params(intent as u8),
        )
    }

    /// Enable MotionRequest (0x2B) reports; the hub sends one when its request changes, and reads the
    /// rate back late, so the granted rate is not checked.
    pub fn enable_motion_request(&self, hz: f64) -> Result<()> {
        self.enable_interval(0x2B, Self::interval_us(hz)?, 0, 0, 0, 0, false, None)
            .map(|_| ())
    }

    // ── batching ─────────────────────────────────────────────────────────────

    /// Force Sensor Flush (0xF0): the hub sends `sensor`'s batched reports now; returns once its Flush
    /// Completed (0xEF) arrives.
    pub fn flush(&self, sensor: u8, timeout: Option<Duration>) -> Result<()> {
        let t = timeout.unwrap_or(DEFAULT_TIMEOUT);
        let (id, rx) = self.hub.flush_waiters.listen(sensor)?;
        let r = self
            .send_shtp(ShtpChannel::Control, &hub::force_flush(sensor))
            .and_then(|_| {
                take(
                    &rx,
                    deadline_after(t),
                    crate::bno086::reports::FLUSH_COMPLETED,
                    t,
                )
            });
        self.hub.flush_waiters.forget(sensor, id);
        r
    }

    // ── FRS ──────────────────────────────────────────────────────────────────

    /// Read a whole FRS record: its 32-bit words.
    pub fn frs_read(&self, record_id: u16, timeout: Option<Duration>) -> Result<Vec<u32>> {
        let t = timeout.unwrap_or(FRS_TIMEOUT);
        let mut session = FrsReadSession::new(record_id);
        let w = self.wait(sh2::REPORT_FRS_READ_RESPONSE)?;
        self.send_shtp(ShtpChannel::Control, &session.request())?;
        let deadline = deadline_after(t);
        while !session.done {
            session.feed(&decode_reply(
                &w.next(deadline, t)?,
                FrsReadResponse::unpack,
            )?)?;
        }
        Ok(session.words)
    }

    /// Write a whole FRS record.
    pub fn frs_write(
        &self,
        record_id: u16,
        words: &[u32],
        timeout: Option<Duration>,
    ) -> Result<()> {
        let t = timeout.unwrap_or(FRS_TIMEOUT);
        let mut session = FrsWriteSession::new(record_id, words);
        let w = self.wait(sh2::REPORT_FRS_WRITE_RESPONSE)?;
        self.send_shtp(ShtpChannel::Control, &session.request())?;
        let deadline = deadline_after(t);
        while !session.done {
            let resp = decode_reply(&w.next(deadline, t)?, FrsWriteResponse::unpack)?;
            if let Some(next) = session.feed(&resp)? {
                self.send_shtp(ShtpChannel::Control, &next)?;
            }
        }
        Ok(())
    }

    /// Read and parse `sensor`'s metadata FRS record.
    pub fn get_metadata(&self, sensor: u8, timeout: Option<Duration>) -> Result<SensorMetadata> {
        let record = hub::METADATA_RECORDS
            .iter()
            .find(|(s, _)| *s == sensor)
            .map(|(_, r)| *r)
            .ok_or_else(|| {
                DepzError::Sensor(format!(
                    "no metadata FRS record known for sensor 0x{sensor:02X}"
                ))
            })?;
        Ok(SensorMetadata::from_words(&self.frs_read(record, timeout)?))
    }

    // ── diagnostics and housekeeping ─────────────────────────────────────────

    /// Get Oscillator Type (0x0A): R0, one of `hub::OSC_*` (an unknown value comes back as is).
    pub fn get_oscillator_type(&self) -> Result<u8> {
        Ok(self.command(hub::CMD_GET_OSCILLATOR_TYPE, &[], None)?.r[0])
    }

    /// Clear the in-RAM dynamic calibration and reset (0x0B). The hub resets instead of answering, so
    /// this waits for the executable reset-complete.
    pub fn clear_dcd_and_reset(&self, timeout: Option<Duration>) -> Result<()> {
        let t = timeout.unwrap_or(RESET_TIMEOUT);
        self.forget_hub_state();
        self.command_no_response(hub::CMD_CLEAR_DCD_AND_RESET, &[])?;
        if !self.wait_reset_complete(t) {
            return Err(DepzError::timeout(hub::CMD_CLEAR_DCD_AND_RESET, t));
        }
        if let Some(f) = self.hub.control_waiters.failure() {
            return Err(f.error()); // woken by close() / a lost link, not by the hub
        }
        // The command consumed a host TX seq; the hub restarted its counters on reset. TX only: what
        // the hub sent since the reset is in the reassembly (bug hunt 2026-11-03 JAVA-12).
        self.hub.shtp.lock().unwrap().reset_tx();
        Ok(())
    }

    /// The error queue (0x01), `severity` or greater; records stream until one with source 255.
    pub fn get_errors(&self, severity: u8) -> Result<Vec<ErrorRecord>> {
        let rs = self.command_collect(
            hub::CMD_ERRORS,
            &hub::errors_params(severity),
            |resp, acc| {
                if resp.r[2] == hub::ERROR_SOURCE_NO_MORE_ERRORS {
                    return true; // the terminator, not a record
                }
                acc.push(resp);
                false
            },
            DEFAULT_TIMEOUT,
        )?;
        Ok(rs.iter().map(ErrorRecord::from_response).collect())
    }

    /// A sensor's event counts (0x02): two responses, response_seq 0 then 1.
    pub fn get_counts(&self, sensor: u8) -> Result<Counts> {
        let rs = self.command_collect(
            hub::CMD_COUNTER,
            &hub::counts_get_params(sensor),
            |resp, acc| {
                acc.push(resp);
                // both halves, whatever their order: without one the exchange is
                // not complete (a timeout, contract 07 §3 — bug hunt 2026-11-03 PY-6)
                acc.iter().any(|r| r.response_seq == 0) && acc.iter().any(|r| r.response_seq == 1)
            },
            DEFAULT_TIMEOUT,
        )?;
        let r0 = rs
            .iter()
            .find(|r| r.response_seq == 0)
            .ok_or_else(|| DepzError::Sensor("counts: no response_seq 0".into()))?
            .r;
        let r1 = rs.iter().find(|r| r.response_seq == 1).unwrap().r;
        let u =
            |b: &[u8; 11], at: usize| u32::from_le_bytes([b[at], b[at + 1], b[at + 2], b[at + 3]]);
        Ok(Counts {
            sensor_id: sensor,
            offered: u(&r0, 3),
            accepted: u(&r0, 7),
            on: u(&r1, 3),
            attempted: u(&r1, 7),
        })
    }

    /// Clear a sensor's event counts (0x02, subcommand 1).
    pub fn clear_counts(&self, sensor: u8) -> Result<()> {
        let resp = self.command(hub::CMD_COUNTER, &hub::counts_clear_params(sensor), None)?;
        if resp.status() != 0 {
            return Err(DepzError::Sensor(format!(
                "clear counts failed: status {}",
                resp.status()
            )));
        }
        Ok(())
    }
}
