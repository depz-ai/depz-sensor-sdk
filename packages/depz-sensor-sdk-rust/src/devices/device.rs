//! A connection to one DEPZ device in application mode (contracts 02, 07): the reader thread,
//! replies correlated to requests by echoed opcode, events, and the common commands.

use std::any::Any;
use std::collections::HashMap;
use std::panic::{catch_unwind, AssertUnwindSafe};
use std::sync::atomic::{AtomicBool, AtomicU64, Ordering};
use std::sync::mpsc::{sync_channel, SyncSender};
use std::sync::{Arc, Mutex, MutexGuard, OnceLock, PoisonError, RwLock, Weak};
use std::thread::JoinHandle;
use std::time::{Duration, Instant};

use super::error::{DepzError, Result};
use super::link::{hex_encode, Link};
use super::serial::SerialPortLink;
use super::stream::{Fanout, Hook, Subscription};
use crate::framing::{build_packet, CrcType, Event, Packet, PacketParser};
use crate::protocol::common::{
    self, Cmd, Rpt, SequenceErrorReport, Status, StatusReport, SyncPinConfig, SyncTimeReport,
    TemperatureReport, TextReport,
};
use crate::protocol::identity::{parse_software_name, Identity};

/// The default request timeout, 200 ms (contract 07 §3).
pub const DEFAULT_TIMEOUT: Duration = Duration::from_millis(200);

const READ_SLICE: Duration = Duration::from_millis(50);

/// Something the device or the link reports outside any request (contract 07 §4).
#[derive(Debug, Clone, PartialEq)]
#[non_exhaustive]
pub enum DeviceEvent {
    /// The board saw a gap in host TX (RPT_SEQUENCE_ERROR); `reported_by_device` is always true.
    SequenceError {
        expected_seq: u8,
        received_seq: u8,
        reported_by_device: bool,
    },
    /// The host saw a gap in the board's packets (contract 01 §5 step 5): the packet still
    /// arrives, `missed` = (received − expected) mod 256 did not. Counted in `seq_gaps`.
    SeqGap {
        expected_seq: u8,
        received_seq: u8,
        missed: u8,
    },
    /// A frame whose payload CRC failed (dropped).
    LinkCrcError { cmd: u8, seq: u8 },
    /// Bytes discarded while hunting for a frame.
    Trash(Vec<u8>),
    /// RPT_STATUS not answering a request in flight.
    UnsolicitedStatus(u8),
    /// RPT_TEXT outside a request, or an unrouted report (`text` is then its payload in hex).
    Text { cmd: u8, text: String },
    /// The MCU temperature the board pushed.
    Temperature { timestamp_us: u64, celsius: f64 },
    /// The link dropped or the reader stopped.
    Disconnected(String),
    /// A sensor class's own event: `kind` names it, `payload` is the report's payload.
    Sensor {
        kind: &'static str,
        code: u8,
        payload: Vec<u8>,
    },
}

/// Link counters (a snapshot).
#[derive(Debug, Clone, Default, PartialEq, Eq)]
pub struct LinkStats {
    pub tx_packets: u64,
    pub tx_bytes: u64,
    pub rx_packets: u64,
    pub rx_bytes: u64,
    pub crc_errors: u64,
    pub header_errors: u64,
    pub trash_bytes: u64,
    pub seq_gaps: u64,
    pub device_seq_errors: u64,
    /// Callbacks that panicked on the reader thread (contract 07 §3).
    pub callback_errors: u64,
    /// Reports whose handler panicked: the packet was dropped (contract 07 §3).
    pub report_errors: u64,
}

/// Result of [`DepzDevice::sync_time`]: device clock − host clock, the round trip of the kept
/// sample, and when (host µs) it was taken.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct TimeSyncResult {
    pub offset_us: i64,
    pub rtt_us: i64,
    pub synced_at_host_us: i64,
}

/// The host monotonic clock in µs — the host side of all time-sync math.
pub fn host_now_us() -> i64 {
    static EPOCH: OnceLock<Instant> = OnceLock::new();
    EPOCH.get_or_init(Instant::now).elapsed().as_micros() as i64 + 1_000_000
}

/// Routes the streaming reports of a sensor class (runs on the reader thread: keep it short).
pub trait ReportHandler: Send + Sync {
    /// Return true when the packet was consumed.
    fn handle_report(&self, packet: &Packet) -> bool;

    /// The reader stopped: end the class's streams.
    fn reader_stopped(&self) {}
}

type AnyBox = Box<dyn Any + Send>;
type ErasedMatcher = Box<dyn Fn(&Packet) -> Option<Result<AnyBox>> + Send>;

struct Pending {
    // Shared so the reader can run it with `pending` unlocked; only the reader ever locks it.
    matcher: Option<Arc<Mutex<ErasedMatcher>>>,
    ok_completes: bool,
    done: SyncSender<Result<Option<AnyBox>>>,
}

struct Tx {
    seq: u8,
}

pub(crate) struct Core {
    link: Box<dyn Link>,
    stats: Mutex<LinkStats>,
    callback_errors: Arc<AtomicU64>,
    pub(crate) events: Fanout<DeviceEvent>,
    timeout: Mutex<Duration>,
    tx_crc: Mutex<CrcType>,
    time_sync: Mutex<Option<TimeSyncResult>>,
    tx: Mutex<Tx>,
    pending: Mutex<HashMap<u8, Pending>>,
    handler: RwLock<Option<Arc<dyn ReportHandler>>>,
    closing: AtomicBool,
    /// Set when the reader ended on its own (not by close()): every later request fails with it.
    lost: Mutex<Option<String>>,
    reader: Mutex<Option<JoinHandle<()>>>,
}

/// Lock `m` even if a panic poisoned it: a panic on the reader must never turn into a panic in a
/// caller's thread (contract 07 §3). Every critical section below leaves its data consistent.
fn relock<T: ?Sized>(m: &Mutex<T>) -> MutexGuard<'_, T> {
    m.lock().unwrap_or_else(PoisonError::into_inner)
}

/// What a caught panic said, for the error it becomes.
fn panic_message(p: &(dyn Any + Send)) -> String {
    if let Some(s) = p.downcast_ref::<&str>() {
        (*s).to_string()
    } else if let Some(s) = p.downcast_ref::<String>() {
        s.clone()
    } else {
        "a panic".into()
    }
}

/// A connection to one DEPZ device in application mode (contracts 02, 07): a reader thread, replies
/// correlated to requests by echoed opcode (one request in flight per opcode), events, and the common
/// commands. Sensor classes wrap it (and deref to it). Requests block the calling thread up to
/// [`DepzDevice::timeout`] (default 200 ms). Callbacks run on the reader thread: keep them short,
/// never block, never close the device from one. Dropping the device closes the link.
pub struct DepzDevice {
    pub(crate) core: Arc<Core>,
}

impl DepzDevice {
    /// Open a connection on any link; the device owns it and closes it. `timeout` `None`: 200 ms.
    pub fn new(link: impl Link + 'static, timeout: Option<Duration>) -> DepzDevice {
        let callback_errors = Arc::new(AtomicU64::new(0));
        let core = Arc::new(Core {
            link: Box::new(link),
            stats: Mutex::new(LinkStats::default()),
            events: Fanout::new(callback_errors.clone()),
            callback_errors,
            timeout: Mutex::new(timeout.unwrap_or(DEFAULT_TIMEOUT)),
            tx_crc: Mutex::new(CrcType::None),
            time_sync: Mutex::new(None),
            tx: Mutex::new(Tx { seq: 0 }),
            pending: Mutex::new(HashMap::new()),
            handler: RwLock::new(None),
            closing: AtomicBool::new(false),
            lost: Mutex::new(None),
            reader: Mutex::new(None),
        });
        let rc = core.clone();
        let handle = std::thread::Builder::new()
            .name(format!("depz-reader-{}", core.link.name()))
            .spawn(move || reader_loop(rc))
            .expect("spawn the reader thread");
        *core.reader.lock().unwrap() = Some(handle);
        DepzDevice { core }
    }

    /// Open a plain connection on the serial port `port` ([`SerialPortLink::open`]): the common
    /// commands, no sensor class. `timeout` `None`: 200 ms. [`DepzError::DeviceLost`] when the port
    /// does not open.
    pub fn open(port: &str, timeout: Option<Duration>) -> Result<DepzDevice> {
        Ok(DepzDevice::new(SerialPortLink::open(port)?, timeout))
    }

    /// Route this connection's streaming reports to a sensor class (one handler; the last wins).
    pub fn set_report_handler(&self, handler: Arc<dyn ReportHandler>) {
        *self.core.handler.write().unwrap() = Some(handler);
    }

    // ── lifecycle ────────────────────────────────────────────────────────────

    /// The link's name: the port path, or what the link wraps.
    pub fn port(&self) -> String {
        self.core.link.name()
    }

    pub fn is_closed(&self) -> bool {
        // A reader that ended marks the device lost even if closing its link failed.
        self.core.link.is_closed() || relock(&self.core.lost).is_some()
    }

    /// The timeout of requests that take none of their own.
    pub fn timeout(&self) -> Duration {
        *self.core.timeout.lock().unwrap()
    }

    pub fn set_timeout(&self, timeout: Duration) -> Result<()> {
        if timeout.is_zero() {
            return Err(DepzError::InvalidArgument(
                "timeout must be positive".into(),
            ));
        }
        *self.core.timeout.lock().unwrap() = timeout;
        Ok(())
    }

    /// The payload CRC the host puts on its packets (contract 01).
    pub fn tx_crc_type(&self) -> CrcType {
        *self.core.tx_crc.lock().unwrap()
    }

    pub fn set_tx_crc_type(&self, crc: CrcType) {
        *self.core.tx_crc.lock().unwrap() = crc;
    }

    pub fn stats(&self) -> LinkStats {
        let mut s = relock(&self.core.stats).clone();
        s.callback_errors = self.core.callback_errors.load(Ordering::Relaxed);
        s
    }

    /// The counter a sensor class's streams share with [`LinkStats::callback_errors`].
    pub(crate) fn callback_errors_counter(&self) -> Arc<AtomicU64> {
        self.core.callback_errors.clone()
    }

    /// Close the link and stop the reader; pending requests and streams end. Dropping does the same.
    pub fn close(&self) {
        self.core.closing.store(true, Ordering::SeqCst);
        self.core.link.close();
        let handle = self.core.reader.lock().unwrap().take();
        if let Some(h) = handle {
            if h.thread().id() != std::thread::current().id() {
                let _ = h.join();
            }
        }
    }

    // ── events ───────────────────────────────────────────────────────────────

    /// Call `callback` for every event (reader thread). Drop the hook to stop.
    pub fn on_event(&self, callback: impl Fn(&DeviceEvent) + Send + Sync + 'static) -> Hook {
        self.core.events.on_item(callback)
    }

    /// A pull queue of events (bounded, drop-oldest); subscribed when this returns.
    pub fn events(&self, capacity: usize) -> Subscription<DeviceEvent> {
        self.core.events.subscribe(capacity)
    }

    /// Hand an event to the event subscribers (a sensor class's own events).
    pub fn emit_event(&self, event: DeviceEvent) {
        self.core.events.emit(event);
    }

    // ── TX / requests ────────────────────────────────────────────────────────

    /// Send a packet without waiting for anything (escape hatch; prefer a request).
    pub fn send(&self, cmd: u8, payload: &[u8]) -> Result<()> {
        let crc = self.tx_crc_type();
        self.core.send(cmd, payload, crc)
    }

    /// Send `cmd` and wait for the first packet `matcher` claims (escape hatch, contract 07 §2).
    /// The matcher answers `None` for "not mine", `Some(Err)` for "mine, but it does not decode". A
    /// non-OK RPT_STATUS echoing `cmd` fails with [`DepzError::Status`]; one request per opcode may
    /// be in flight.
    pub fn request<T: Send + 'static>(
        &self,
        cmd: u8,
        payload: &[u8],
        matcher: impl Fn(&Packet) -> Option<Result<T>> + Send + 'static,
        timeout: Option<Duration>,
    ) -> Result<T> {
        self.requester().request(cmd, payload, matcher, timeout)
    }

    /// Send `cmd` and wait for RPT_STATUS OK echoing it.
    pub fn request_ok(&self, cmd: u8, payload: &[u8], timeout: Option<Duration>) -> Result<()> {
        self.core
            .request(
                cmd,
                payload,
                None,
                true,
                timeout.unwrap_or_else(|| self.timeout()),
            )
            .map(|_| ())
    }

    // ── common commands (contract 02) ────────────────────────────────────────

    pub fn get_device_name(&self) -> Result<String> {
        self.request(
            Cmd::GetDeviceName as u8,
            &[],
            expect_text(Cmd::GetDeviceName as u8),
            None,
        )
    }

    pub fn get_software_name(&self) -> Result<String> {
        self.request(
            Cmd::GetNameActiveSoftware as u8,
            &[],
            expect_text(Cmd::GetNameActiveSoftware as u8),
            None,
        )
    }

    pub fn get_serial_number(&self) -> Result<String> {
        self.request(
            Cmd::GetSerial as u8,
            &[],
            expect_text(Cmd::GetSerial as u8),
            None,
        )
    }

    /// The running firmware: the software name parsed into mode, sensor type and version (contract 02 §4).
    pub fn identify(&self) -> Result<Identity> {
        Ok(parse_software_name(&self.get_software_name()?))
    }

    /// The MCU temperature the device last cached, °C (it refreshes about twice a second).
    pub fn read_mcu_temperature(&self) -> Result<f64> {
        let r = self.request(
            Cmd::GetMcuTemperature as u8,
            &[],
            expect_report(Rpt::Temperature as u8, TemperatureReport::unpack),
            None,
        )?;
        Ok(r.celsius())
    }

    /// NTP-style time sync keeping the lowest-RTT of `samples` rounds (contract 02 §5).
    pub fn sync_time(&self, samples: usize) -> Result<TimeSyncResult> {
        let mut best: Option<TimeSyncResult> = None;
        for _ in 0..samples.max(1) {
            let t1 = host_now_us();
            let rep = self.request(
                Cmd::SyncTime as u8,
                &common::pack_sync_time(t1 as u64),
                expect_report(Rpt::SyncTime as u8, SyncTimeReport::unpack),
                None,
            )?;
            let t4 = host_now_us();
            let (offset, rtt) =
                common::sync_time_offset_rtt(t1, rep.mcu_rx_us as i64, rep.mcu_tx_us as i64, t4);
            if best.is_none_or(|b| rtt < b.rtt_us) {
                best = Some(TimeSyncResult {
                    offset_us: offset,
                    rtt_us: rtt,
                    synced_at_host_us: t4,
                });
            }
        }
        *self.core.time_sync.lock().unwrap() = best;
        Ok(best.unwrap())
    }

    /// The last [`DepzDevice::sync_time`] result.
    pub fn last_time_sync(&self) -> Option<TimeSyncResult> {
        *self.core.time_sync.lock().unwrap()
    }

    /// Device µs → host monotonic µs ([`host_now_us`]); needs a [`DepzDevice::sync_time`] first.
    pub fn to_host_time_us(&self, device_timestamp_us: u64) -> Result<i64> {
        match self.last_time_sync() {
            Some(s) => Ok(device_timestamp_us as i64 - s.offset_us),
            None => Err(DepzError::InvalidState("call sync_time() first".into())),
        }
    }

    /// The device → host payload CRC mode.
    pub fn get_report_payload_crc(&self) -> Result<CrcType> {
        let v = self.request(
            Cmd::GetPayloadCrcType as u8,
            &[],
            expect_report(Rpt::PayloadCrcType as u8, |p| {
                p.first()
                    .copied()
                    .ok_or(common::CodecError("RPT_PAYLOAD_CRC_TYPE: need 1 byte"))
            }),
            None,
        )?;
        CrcType::from_value(v).ok_or_else(|| DepzError::Protocol(format!("payload CRC type {v}")))
    }

    /// Set the device → host payload CRC mode (host → device is per packet: [`DepzDevice::set_tx_crc_type`]).
    pub fn set_report_payload_crc(&self, crc: CrcType) -> Result<()> {
        self.request_ok(
            Cmd::SetPayloadCrcType as u8,
            &common::pack_set_payload_crc_type(crc as u8),
            None,
        )
    }

    pub fn get_sync_pin(&self, pin: u8) -> Result<SyncPinConfig> {
        if !(common::SYNC_PIN_MIN..=common::SYNC_PIN_MAX).contains(&pin) {
            return Err(DepzError::InvalidArgument(format!(
                "sync pin must be {}..{}, got {pin}",
                common::SYNC_PIN_MIN,
                common::SYNC_PIN_MAX
            )));
        }
        self.request(
            Cmd::GetSyncPinConfig as u8,
            &[pin],
            expect_report(Rpt::SyncPinConfig as u8, SyncPinConfig::unpack),
            None,
        )
    }

    pub fn set_sync_pin(&self, config: &SyncPinConfig) -> Result<()> {
        let payload = config
            .try_pack()
            .map_err(|e| DepzError::InvalidArgument(e.0.to_string()))?;
        self.request_ok(Cmd::SetSyncPinConfig as u8, &payload, None)
    }

    /// DEVICE_RESET: the device acknowledges, then reboots; the link drops.
    pub fn reset(&self) -> Result<()> {
        self.request_ok(Cmd::DeviceReset as u8, &[], None)
    }

    /// Reboot into the resident bootloader and close this connection (flashing: contract 06).
    pub fn enter_bootloader_mode(&self) -> Result<()> {
        self.request_ok(Cmd::Bootloader as u8, &[], None)?;
        self.close();
        Ok(())
    }
}

/// A cloneable handle for requests on a device's connection — what a sensor class gives its driver's
/// platform. It does not keep the device open: requests fail once the device closes. It holds the
/// connection weakly: the driver it lives in is owned by the connection's report handler, and a
/// strong reference back would keep the whole connection alive after the device is dropped.
#[derive(Clone)]
pub(crate) struct Requester {
    core: Weak<Core>,
}

impl Requester {
    fn core(&self) -> Result<Arc<Core>> {
        self.core
            .upgrade()
            .ok_or_else(|| DepzError::LinkClosed("device closed".into()))
    }

    pub(crate) fn request<T: Send + 'static>(
        &self,
        cmd: u8,
        payload: &[u8],
        matcher: impl Fn(&Packet) -> Option<Result<T>> + Send + 'static,
        timeout: Option<Duration>,
    ) -> Result<T> {
        let core = self.core()?;
        let erased: ErasedMatcher =
            Box::new(move |p| matcher(p).map(|r| r.map(|v| Box::new(v) as AnyBox)));
        let t = timeout.unwrap_or_else(|| *core.timeout.lock().unwrap());
        match core.request(cmd, payload, Some(erased), false, t)? {
            Some(b) => Ok(*b.downcast::<T>().expect("matcher type")),
            None => Err(DepzError::Protocol(format!(
                "cmd 0x{cmd:02X}: no reply value"
            ))),
        }
    }

    pub(crate) fn request_ok(
        &self,
        cmd: u8,
        payload: &[u8],
        timeout: Option<Duration>,
    ) -> Result<()> {
        let core = self.core()?;
        let t = timeout.unwrap_or_else(|| *core.timeout.lock().unwrap());
        core.request(cmd, payload, None, true, t).map(|_| ())
    }

    /// [`DepzDevice::read_mcu_temperature`] through the handle.
    pub(crate) fn read_mcu_temperature(&self) -> Result<f64> {
        let r = self.request(
            Cmd::GetMcuTemperature as u8,
            &[],
            expect_report(Rpt::Temperature as u8, TemperatureReport::unpack),
            None,
        )?;
        Ok(r.celsius())
    }
}

impl DepzDevice {
    pub(crate) fn requester(&self) -> Requester {
        Requester {
            core: Arc::downgrade(&self.core),
        }
    }
}

impl Drop for DepzDevice {
    fn drop(&mut self) {
        self.close();
    }
}

/// A matcher for a report identified by its report id alone.
pub fn expect_report<T, E: Into<DepzError>>(
    report_id: u8,
    unpack: impl Fn(&[u8]) -> std::result::Result<T, E> + Send + 'static,
) -> impl Fn(&Packet) -> Option<Result<T>> + Send + 'static {
    move |p: &Packet| {
        if p.cmd == report_id {
            Some(unpack(&p.payload).map_err(Into::into))
        } else {
            None
        }
    }
}

/// A matcher for RPT_TEXT echoing `request_cmd`.
pub fn expect_text(request_cmd: u8) -> impl Fn(&Packet) -> Option<Result<String>> + Send + 'static {
    move |p: &Packet| {
        if p.cmd == Rpt::Text as u8 && !p.payload.is_empty() && p.payload[0] == request_cmd {
            Some(
                TextReport::unpack(&p.payload)
                    .map(|r| r.text)
                    .map_err(Into::into),
            )
        } else {
            None
        }
    }
}

impl Core {
    fn send(&self, cmd: u8, payload: &[u8], crc: CrcType) -> Result<()> {
        if let Some(m) = relock(&self.lost).clone() {
            return Err(DepzError::DeviceLost(m));
        }
        let mut tx = relock(&self.tx);
        let frame = build_packet(cmd, payload, tx.seq as u32, crc)
            .map_err(|e| DepzError::InvalidArgument(e.to_string()))?;
        self.link.write(&frame)?;
        tx.seq = tx.seq.wrapping_add(1);
        let mut s = relock(&self.stats);
        s.tx_packets += 1;
        s.tx_bytes += frame.len() as u64;
        Ok(())
    }

    fn request(
        &self,
        cmd: u8,
        payload: &[u8],
        matcher: Option<ErasedMatcher>,
        ok_completes: bool,
        timeout: Duration,
    ) -> Result<Option<AnyBox>> {
        let (done, wait) = sync_channel(1);
        {
            let mut pending = relock(&self.pending);
            if pending.contains_key(&cmd) {
                return Err(DepzError::Status {
                    cmd,
                    status: Status::ErrBusy as u8,
                });
            }
            pending.insert(
                cmd,
                Pending {
                    matcher: matcher.map(|m| Arc::new(Mutex::new(m))),
                    ok_completes,
                    done,
                },
            );
        }
        let crc = *self.tx_crc.lock().unwrap();
        if let Err(e) = self.send(cmd, payload, crc) {
            relock(&self.pending).remove(&cmd);
            return Err(e);
        }
        match wait.recv_timeout(timeout) {
            Ok(r) => r,
            Err(_) => {
                relock(&self.pending).remove(&cmd);
                // The reply may have landed between the wait and the removal.
                match wait.try_recv() {
                    Ok(r) => r,
                    Err(_) => Err(DepzError::timeout(cmd, timeout)),
                }
            }
        }
    }

    fn fail_all(&self, make: impl Fn() -> DepzError) {
        let all: Vec<Pending> = relock(&self.pending).drain().map(|(_, p)| p).collect();
        for p in all {
            let _ = p.done.try_send(Err(make()));
        }
    }

    fn emit(&self, e: DeviceEvent) {
        self.events.emit(e);
    }

    fn dispatch(&self, pkt: &Packet, last_rx_seq: &mut Option<u8>) {
        // The first packet after open is no gap; a gap is surfaced ahead of its packet.
        if let Some(last) = *last_rx_seq {
            let expected = last.wrapping_add(1);
            if pkt.seq != expected {
                relock(&self.stats).seq_gaps += 1;
                self.emit(DeviceEvent::SeqGap {
                    expected_seq: expected,
                    received_seq: pkt.seq,
                    missed: pkt.seq.wrapping_sub(expected),
                });
            }
        }
        *last_rx_seq = Some(pkt.seq);
        let payload = &pkt.payload;
        if pkt.cmd == Rpt::Status as u8 && payload.len() >= 2 {
            if let Ok(rep) = StatusReport::unpack(payload) {
                if rep.cmd != common::UNSOLICITED {
                    let mut pending = relock(&self.pending);
                    if let Some(p) = pending.get(&rep.cmd) {
                        let outcome = if rep.status != Status::Ok as u8 {
                            Some(Err(DepzError::Status {
                                cmd: rep.cmd,
                                status: rep.status,
                            }))
                        } else if p.ok_completes {
                            Some(Ok(None))
                        } else {
                            None // an OK ack while the data report is still to come — keep waiting
                        };
                        if let Some(o) = outcome {
                            let p = pending.remove(&rep.cmd).unwrap();
                            drop(pending);
                            let _ = p.done.try_send(o);
                        }
                        return;
                    }
                }
                self.emit(DeviceEvent::UnsolicitedStatus(rep.status));
                return;
            }
        }

        // Pending matchers get the first shot at any other packet. They are user code: they run
        // with `pending` unlocked and under catch_unwind — a matcher that panics has claimed the
        // packet and failed to decode it, so only its own request fails (contract 07 §3).
        let matchers: Vec<(u8, Arc<Mutex<ErasedMatcher>>)> = relock(&self.pending)
            .iter()
            .filter_map(|(cmd, p)| p.matcher.as_ref().map(|m| (*cmd, m.clone())))
            .collect();
        for (cmd, m) in matchers {
            let out = match catch_unwind(AssertUnwindSafe(|| (*relock(&m))(pkt))) {
                Ok(None) => continue,
                Ok(Some(Ok(v))) => Ok(Some(v)),
                Ok(Some(Err(e))) => Err(DepzError::Protocol(format!(
                    "reply to cmd 0x{cmd:02X} does not decode: {e}"
                ))),
                Err(panic) => Err(DepzError::Protocol(format!(
                    "reply to cmd 0x{cmd:02X} does not decode: the matcher panicked: {}",
                    panic_message(&*panic)
                ))),
            };
            // The request may have timed out (and another of its opcode begun) meanwhile.
            let claimed = {
                let mut pending = relock(&self.pending);
                match pending.get(&cmd) {
                    Some(p) if p.matcher.as_ref().is_some_and(|x| Arc::ptr_eq(x, &m)) => {
                        pending.remove(&cmd)
                    }
                    _ => None,
                }
            };
            if let Some(p) = claimed {
                let _ = p.done.try_send(out);
                return;
            }
        }

        // Contract 07 §3: a report longer than its fields is decoded and the
        // tail ignored (a newer firmware may append fields); a shorter one is
        // not a report of that kind and stays visible as hex text below.
        if pkt.cmd == Rpt::SequenceError as u8 && payload.len() >= 2 {
            if let Ok(rep) = SequenceErrorReport::unpack(payload) {
                relock(&self.stats).device_seq_errors += 1;
                self.emit(DeviceEvent::SequenceError {
                    expected_seq: rep.expected_seq,
                    received_seq: rep.received_seq,
                    reported_by_device: true,
                });
                return;
            }
        }
        if pkt.cmd == Rpt::Text as u8 && !payload.is_empty() {
            if let Ok(rep) = TextReport::unpack(payload) {
                self.emit(DeviceEvent::Text {
                    cmd: rep.cmd,
                    text: rep.text,
                });
                return;
            }
        }
        if pkt.cmd == Rpt::Temperature as u8 && payload.len() >= 10 {
            if let Ok(rep) = TemperatureReport::unpack(payload) {
                self.emit(DeviceEvent::Temperature {
                    timestamp_us: rep.timestamp_us,
                    celsius: rep.celsius(),
                });
                return;
            }
        }
        // The report path runs guarded like the matchers: a stream handler that panics drops this
        // one packet, counted — it never ends the reader (contract 07 §3).
        let handler = self
            .handler
            .read()
            .unwrap_or_else(PoisonError::into_inner)
            .clone();
        if let Some(h) = handler {
            match catch_unwind(AssertUnwindSafe(|| h.handle_report(pkt))) {
                Ok(true) => return,
                Ok(false) => {}
                Err(_) => {
                    relock(&self.stats).report_errors += 1;
                    // The panic hook has already printed the panic itself.
                    eprintln!(
                        "depz-sensor-sdk: report 0x{:02X} handler panicked on {}; dropped",
                        pkt.cmd,
                        self.link.name()
                    );
                    return;
                }
            }
        }
        // An unrouted report stays visible, so new firmware is debuggable.
        self.emit(DeviceEvent::Text {
            cmd: pkt.cmd,
            text: hex_encode(payload),
        });
    }
}

fn reader_loop(core: Arc<Core>) {
    let mut parser = PacketParser::new();
    let mut last_rx_seq: Option<u8> = None;
    // Some(cause) when the reader ends on its own; None when close() ended it.
    let outcome = catch_unwind(AssertUnwindSafe(|| {
        while !core.closing.load(Ordering::SeqCst) {
            let data = match core.link.read(READ_SLICE) {
                Ok(d) => d,
                Err(e) => return Some(e.to_string()),
            };
            if data.is_empty() {
                continue;
            }
            relock(&core.stats).rx_bytes += data.len() as u64;
            for ev in parser.feed(&data) {
                match ev {
                    Event::Packet(pkt) => {
                        relock(&core.stats).rx_packets += 1;
                        core.dispatch(&pkt, &mut last_rx_seq);
                    }
                    Event::CrcError { cmd, seq } => {
                        relock(&core.stats).crc_errors += 1;
                        core.emit(DeviceEvent::LinkCrcError { cmd, seq });
                    }
                    Event::Trash(t) => {
                        relock(&core.stats).trash_bytes += t.len() as u64;
                        core.emit(DeviceEvent::Trash(t));
                    }
                }
            }
            relock(&core.stats).header_errors = parser.header_errors;
        }
        None
    }));
    let ended = match outcome {
        Ok(cause) => cause,
        Err(panic) => Some(format!("reader ended: {}", panic_message(&*panic))),
    };
    // A reader that ends on its own must not leave a device that looks open and times every
    // request out: close the link, fail what is pending and every later request, say why
    // (contract 07 §3). Nothing here may panic again, or the queues would never close.
    if let Some(msg) = ended.filter(|_| !core.closing.load(Ordering::SeqCst)) {
        *relock(&core.lost) = Some(msg.clone());
        let _ = catch_unwind(AssertUnwindSafe(|| core.link.close()));
        core.fail_all(|| DepzError::DeviceLost(msg.clone()));
        let _ = catch_unwind(AssertUnwindSafe(|| {
            core.emit(DeviceEvent::Disconnected(msg.clone()))
        }));
    }
    core.fail_all(|| DepzError::LinkClosed("device closed".into()));
    let handler = core
        .handler
        .read()
        .unwrap_or_else(PoisonError::into_inner)
        .clone();
    if let Some(h) = handler {
        let _ = catch_unwind(AssertUnwindSafe(|| h.reader_stopped()));
    }
    let _ = catch_unwind(AssertUnwindSafe(|| core.events.close_all()));
}
