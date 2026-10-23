//! A board in-process for the device-layer tests: answers what the host writes with `on_packet`, and
//! takes injected reports.

use std::collections::VecDeque;
use std::sync::{Arc, Condvar, Mutex};
use std::time::{Duration, Instant};

use depz_sensor_sdk::devices::{DepzError, Link};
use depz_sensor_sdk::framing::{build_packet, CrcType, Event, Packet, PacketParser};
use depz_sensor_sdk::protocol::common::Rpt;

type Handler = Arc<dyn Fn(&Packet) -> Vec<Vec<u8>> + Send + Sync>;

struct State {
    rx: VecDeque<Vec<u8>>,
    closed: bool,
    unplugged: bool,
    seq: u8,
}

/// Cloning shares the board (keep one clone in the test, hand the other to the device).
#[derive(Clone)]
pub struct FakeBoard {
    state: Arc<(Mutex<State>, Condvar)>,
    parser: Arc<Mutex<PacketParser>>,
    pub sent: Arc<Mutex<Vec<Packet>>>,
    handler: Arc<Mutex<Option<Handler>>>,
}

impl FakeBoard {
    pub fn new() -> FakeBoard {
        FakeBoard {
            state: Arc::new((Mutex::new(State { rx: VecDeque::new(), closed: false, unplugged: false, seq: 0 }), Condvar::new())),
            parser: Arc::new(Mutex::new(PacketParser::new())),
            sent: Arc::new(Mutex::new(Vec::new())),
            handler: Arc::new(Mutex::new(None)),
        }
    }

    /// Answer every host packet with what `f` returns.
    pub fn on_packet(&self, f: impl Fn(&Packet) -> Vec<Vec<u8>> + Send + Sync + 'static) {
        *self.handler.lock().unwrap() = Some(Arc::new(f));
    }

    pub fn status(cmd: u8, status: u8) -> Vec<u8> {
        build_packet(Rpt::Status as u8, &[cmd, status], 0, CrcType::None).unwrap()
    }

    pub fn reply(&self, cmd: u8, payload: &[u8]) -> Vec<u8> {
        let mut s = self.state.0.lock().unwrap();
        let seq = s.seq;
        s.seq = s.seq.wrapping_add(1);
        build_packet(cmd, payload, seq as u32, CrcType::None).unwrap()
    }

    pub fn inject(&self, bytes: Vec<u8>) {
        let (m, c) = &*self.state;
        m.lock().unwrap().rx.push_back(bytes);
        c.notify_all();
    }

    pub fn unplug(&self) {
        let (m, c) = &*self.state;
        m.lock().unwrap().unplugged = true;
        c.notify_all();
    }

    pub fn sent_count(&self) -> usize {
        self.sent.lock().unwrap().len()
    }

    pub fn is_closed_for_test(&self) -> bool {
        self.state.0.lock().unwrap().closed
    }

    pub fn sent_snapshot(&self) -> Vec<Packet> {
        self.sent.lock().unwrap().clone()
    }
}

impl Default for FakeBoard {
    fn default() -> Self {
        Self::new()
    }
}

impl Link for FakeBoard {
    fn read(&self, timeout: Duration) -> Result<Vec<u8>, DepzError> {
        let (m, c) = &*self.state;
        let deadline = Instant::now() + timeout;
        let mut s = m.lock().unwrap();
        while s.rx.is_empty() && !s.closed && !s.unplugged {
            let now = Instant::now();
            if now >= deadline {
                break;
            }
            s = c.wait_timeout(s, deadline - now).unwrap().0;
        }
        if s.closed {
            return Err(DepzError::LinkClosed("fake closed".into()));
        }
        if s.unplugged {
            return Err(DepzError::DeviceLost("fake unplugged".into()));
        }
        Ok(s.rx.pop_front().unwrap_or_default())
    }

    fn write(&self, data: &[u8]) -> Result<(), DepzError> {
        let evs = self.parser.lock().unwrap().feed(data);
        for ev in evs {
            if let Event::Packet(p) = ev {
                self.sent.lock().unwrap().push(p.clone());
                let h = self.handler.lock().unwrap().clone();
                if let Some(h) = h {
                    for r in h(&p) {
                        self.inject(r);
                    }
                }
            }
        }
        Ok(())
    }

    fn close(&self) {
        let (m, c) = &*self.state;
        m.lock().unwrap().closed = true;
        c.notify_all();
    }

    fn is_closed(&self) -> bool {
        self.state.0.lock().unwrap().closed
    }

    fn name(&self) -> String {
        "fake".into()
    }
}

/// Wait until `cond` holds, up to `ms`.
pub fn until(cond: impl Fn() -> bool, ms: u64) -> bool {
    let end = Instant::now() + Duration::from_millis(ms);
    while Instant::now() < end {
        if cond() {
            return true;
        }
        std::thread::sleep(Duration::from_millis(2));
    }
    cond()
}
