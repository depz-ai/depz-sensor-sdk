//! Timeline player over a dataset (contract 09 §Player; reference: the Python `DatasetPlayer`).

use std::panic::{self, AssertUnwindSafe};
use std::sync::atomic::{AtomicU64, Ordering};
use std::sync::{Arc, Condvar, Mutex, MutexGuard};
use std::thread::{JoinHandle, ThreadId};
use std::time::{Duration, Instant};

use super::{Dataset, Record};
use crate::devices::{DepzError, Hook, Result};

/// The thread name of a threaded player.
pub const THREAD_NAME: &str = "depz-dataset-player";

/// [`DatasetPlayer`] state (contract 09 §Player).
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum PlayerState {
    /// Never played.
    Idle,
    /// Delivering records paced by the clock.
    Playing,
    /// Stopped mid-timeline; [`DatasetPlayer::play`] resumes.
    Paused,
    /// Past the last record; [`DatasetPlayer::play`] restarts from the top.
    Done,
}

type RecordCb = Arc<dyn Fn(&Record) + Send + Sync>;
type StateCb = Arc<dyn Fn(PlayerState) + Send + Sync>;
type ErrorCb = Arc<dyn Fn(&str) + Send + Sync>;

struct St {
    state: PlayerState,
    speed: f64,
    idx: usize, // the next record to deliver
    anchor_wall: f64,
    anchor_t: f64,
    closed: bool,
    // Bumped on every change a waiting player thread must see.
    generation: u64,
    // The thread running record callbacks right now, if any.
    delivering: Option<ThreadId>,
    record_cbs: Vec<(u64, RecordCb)>,
    state_cbs: Vec<(u64, StateCb)>,
    error_cbs: Vec<(u64, ErrorCb)>,
    thread: Option<JoinHandle<()>>,
}

struct Inner {
    st: Mutex<St>,
    changed: Condvar,
    recs: Vec<Record>,
    clock: Box<dyn Fn() -> f64 + Send + Sync>,
    threaded: bool,
    next_id: AtomicU64,
}

enum Step {
    Delivered,
    // A record callback panicked: playback paused, the message for whoever drove delivery.
    Failed(Box<dyn std::any::Any + Send>),
    Wait(f64, u64),
    Idle(u64),
}

/// Timeline player over a dataset: paced delivery with play / pause / seek / speed — the engine
/// behind a viewer's playback mode.
///
/// Timing: a media clock anchored at play / resume / seek to (now, `t` of the last delivered
/// record — of the first when none was). Record *i* is due at
/// `anchor_wall + (t_i − anchor_t) / speed`: the first record goes out at once, each later one
/// after its gap to the previous one. Pacing is against the clock, so a slow consumer gets overdue
/// records back to back and playback does not drift. Speed 0 delivers as fast as possible.
///
/// Threads: records and the [`PlayerState::Done`] state are delivered on a player thread (started
/// by the first [`DatasetPlayer::play`]); a state change a call makes is reported on the calling
/// thread before it returns. Callbacks run outside the player's lock, so they may call its
/// methods; once [`DatasetPlayer::pause`] returns no record callback is running or will start
/// (unless `pause` was called from that callback). They must not block. With `threaded = false`
/// no thread starts and the owner calls [`DatasetPlayer::tick`] (a GUI timer, or a test with a
/// fake clock). Dropping the player stops it.
pub struct DatasetPlayer {
    inner: Arc<Inner>,
}

impl DatasetPlayer {
    /// A player over `dataset`. `speed` ≥ 0 (0 = as fast as possible); `clock`: monotonic seconds
    /// (`None`: the system's); `threaded = false`: no player thread, drive it with
    /// [`DatasetPlayer::tick`].
    pub fn new(
        dataset: &Dataset,
        speed: f64,
        clock: Option<Box<dyn Fn() -> f64 + Send + Sync>>,
        threaded: bool,
    ) -> Result<DatasetPlayer> {
        let speed = check_speed(speed)?;
        let clock = clock.unwrap_or_else(|| {
            let start = Instant::now();
            Box::new(move || start.elapsed().as_secs_f64())
        });
        Ok(DatasetPlayer {
            inner: Arc::new(Inner {
                st: Mutex::new(St {
                    state: PlayerState::Idle,
                    speed,
                    idx: 0,
                    anchor_wall: 0.0,
                    anchor_t: 0.0,
                    closed: false,
                    generation: 0,
                    delivering: None,
                    record_cbs: Vec::new(),
                    state_cbs: Vec::new(),
                    error_cbs: Vec::new(),
                    thread: None,
                }),
                changed: Condvar::new(),
                recs: dataset.records.clone(),
                clock,
                threaded,
                next_id: AtomicU64::new(0),
            }),
        })
    }

    pub fn state(&self) -> PlayerState {
        self.inner.lock().state
    }

    /// Span of the timeline: the last record's `t` minus the first's.
    pub fn duration_us(&self) -> i64 {
        match (self.inner.recs.first(), self.inner.recs.last()) {
            (Some(a), Some(b)) => b.t - a.t,
            _ => 0,
        }
    }

    /// µs from the first record to the last delivered one (after a seek: to the record just before
    /// the target); 0 before anything was delivered.
    pub fn position_us(&self) -> i64 {
        let st = self.inner.lock();
        let recs = &self.inner.recs;
        if recs.is_empty() || st.idx == 0 {
            return 0;
        }
        recs[st.idx.min(recs.len()) - 1].t - recs[0].t
    }

    /// Playback speed (finite, ≥ 0; 0 = as fast as possible).
    pub fn speed(&self) -> f64 {
        self.inner.lock().speed
    }

    /// Set the playback speed (finite, ≥ 0, else [`DepzError::InvalidArgument`]). A change while
    /// playing applies at once: the rest of the current wait is rescaled.
    pub fn set_speed(&self, value: f64) -> Result<()> {
        let s = check_speed(value)?;
        let mut st = self.inner.lock();
        if st.state == PlayerState::Playing {
            let now = (self.inner.clock)();
            st.anchor_t = self.inner.media_now(&st, now);
            st.anchor_wall = now;
        }
        st.speed = s;
        self.inner.bump(&mut st);
        Ok(())
    }

    /// Call `callback` for every delivered record; dropping the hook unsubscribes.
    pub fn on_record(&self, callback: impl Fn(&Record) + Send + Sync + 'static) -> Hook {
        let id = self.inner.next_id.fetch_add(1, Ordering::Relaxed);
        self.inner.lock().record_cbs.push((id, Arc::new(callback)));
        let inner = Arc::downgrade(&self.inner);
        Hook::new(move || {
            if let Some(i) = inner.upgrade() {
                i.lock().record_cbs.retain(|(k, _)| *k != id);
            }
        })
    }

    /// Call `callback` with the new state on every transition.
    pub fn on_state(&self, callback: impl Fn(PlayerState) + Send + Sync + 'static) -> Hook {
        let id = self.inner.next_id.fetch_add(1, Ordering::Relaxed);
        self.inner.lock().state_cbs.push((id, Arc::new(callback)));
        let inner = Arc::downgrade(&self.inner);
        Hook::new(move || {
            if let Some(i) = inner.upgrade() {
                i.lock().state_cbs.retain(|(k, _)| *k != id);
            }
        })
    }

    /// Call `callback` with the message of a record callback that panicked on the player thread:
    /// playback paused there (the record counts as delivered).
    pub fn on_delivery_error(&self, callback: impl Fn(&str) + Send + Sync + 'static) -> Hook {
        let id = self.inner.next_id.fetch_add(1, Ordering::Relaxed);
        self.inner.lock().error_cbs.push((id, Arc::new(callback)));
        let inner = Arc::downgrade(&self.inner);
        Hook::new(move || {
            if let Some(i) = inner.upgrade() {
                i.lock().error_cbs.retain(|(k, _)| *k != id);
            }
        })
    }

    /// Start, or resume after [`DatasetPlayer::pause`]; no-op while playing. From
    /// [`PlayerState::Done`] (or a position past the end) playback restarts at the first record.
    /// An empty dataset goes to done at once.
    pub fn play(&self) -> Result<()> {
        let mut changes = Vec::new();
        {
            let mut st = self.inner.lock();
            if st.closed {
                return Err(DepzError::InvalidState("DatasetPlayer is closed".into()));
            }
            if st.state == PlayerState::Playing {
                return Ok(());
            }
            if st.idx >= self.inner.recs.len() {
                st.idx = 0;
            }
            self.inner.anchor(&mut st, (self.inner.clock)());
            self.inner.set_state(&mut st, PlayerState::Playing, &mut changes);
            if self.inner.recs.is_empty() {
                self.inner.set_state(&mut st, PlayerState::Done, &mut changes);
            } else if self.inner.threaded && st.thread.is_none() {
                let inner = self.inner.clone();
                st.thread = Some(
                    std::thread::Builder::new()
                        .name(THREAD_NAME.into())
                        .spawn(move || inner.run())
                        .map_err(DepzError::Io)?,
                );
            }
            self.inner.bump(&mut st);
            self.inner.notify(st, changes);
        }
        Ok(())
    }

    /// Stop delivering; [`DatasetPlayer::play`] resumes from the next record, which then waits its
    /// full gap again. No-op unless playing.
    pub fn pause(&self) {
        let mut changes = Vec::new();
        let mut st = self.inner.lock();
        if st.state != PlayerState::Playing {
            return;
        }
        self.inner.set_state(&mut st, PlayerState::Paused, &mut changes);
        self.inner.bump(&mut st);
        // Once pause returns no record callback runs: wait out one on another thread.
        let me = std::thread::current().id();
        while st.delivering.is_some_and(|t| t != me) {
            st = self.inner.changed.wait(st).unwrap();
        }
        self.inner.notify(st, changes);
    }

    /// Move to `offset_us` µs from the first record: the next record delivered is the first with
    /// `t ≥ first t + offset` (negative: the start); records in between are skipped. The state does
    /// not change, except that a seek past the end while playing ends playback. NaN is
    /// [`DepzError::InvalidArgument`].
    pub fn seek_us(&self, offset_us: f64) -> Result<()> {
        if offset_us.is_nan() {
            return Err(DepzError::InvalidArgument("seek_us: offset must be a number".into()));
        }
        let mut changes = Vec::new();
        let mut st = self.inner.lock();
        if st.closed {
            return Err(DepzError::InvalidState("DatasetPlayer is closed".into()));
        }
        let recs = &self.inner.recs;
        if recs.is_empty() {
            return Ok(());
        }
        let target = recs[0].t as f64 + offset_us;
        st.idx = recs.partition_point(|r| (r.t as f64) < target);
        if st.state == PlayerState::Playing {
            if st.idx >= recs.len() {
                self.inner.set_state(&mut st, PlayerState::Done, &mut changes);
            } else {
                self.inner.anchor(&mut st, (self.inner.clock)());
            }
        }
        self.inner.bump(&mut st);
        self.inner.notify(st, changes);
        Ok(())
    }

    /// Deliver every record due now, on the calling thread. Returns the seconds until the next one
    /// is due, or `None` when not playing. For `threaded = false`; harmless with a player thread. A
    /// record callback's panic pauses playback and propagates.
    pub fn tick(&self) -> Option<f64> {
        loop {
            match self.inner.step() {
                Step::Delivered => {}
                Step::Failed(p) => panic::resume_unwind(p),
                Step::Wait(w, _) => return Some(w),
                Step::Idle(_) => return None,
            }
        }
    }

    /// Block until the player is not playing (done, or paused by someone else): `true`, or `false`
    /// on timeout (`None`: no timeout). Needs the player thread, else
    /// [`DepzError::InvalidState`].
    pub fn wait(&self, timeout: Option<Duration>) -> Result<bool> {
        if !self.inner.threaded {
            return Err(DepzError::InvalidState("wait() needs threaded = true; drive tick() instead".into()));
        }
        let end = timeout.map(|t| Instant::now() + t);
        let mut st = self.inner.lock();
        while st.state == PlayerState::Playing {
            match end {
                None => st = self.inner.changed.wait(st).unwrap(),
                Some(end) => {
                    let left = end.saturating_duration_since(Instant::now());
                    if left.is_zero() {
                        return Ok(false);
                    }
                    st = self.inner.changed.wait_timeout(st, left).unwrap().0;
                }
            }
        }
        Ok(true)
    }

    /// Stop playback and the player thread. Idempotent.
    pub fn close(&self) {
        let mut changes = Vec::new();
        let thread = {
            let mut st = self.inner.lock();
            if st.closed {
                return;
            }
            if st.state == PlayerState::Playing {
                self.inner.set_state(&mut st, PlayerState::Paused, &mut changes);
            }
            st.closed = true;
            self.inner.bump(&mut st);
            let t = st.thread.take();
            self.inner.notify(st, changes);
            t
        };
        if let Some(t) = thread {
            if t.thread().id() != std::thread::current().id() {
                let _ = t.join();
            }
        }
    }
}

impl Drop for DatasetPlayer {
    fn drop(&mut self) {
        self.close();
    }
}

fn check_speed(value: f64) -> Result<f64> {
    if !value.is_finite() || value < 0.0 {
        return Err(DepzError::InvalidArgument(format!("speed must be a finite number >= 0, got {value}")));
    }
    Ok(value)
}

fn panic_text(p: &(dyn std::any::Any + Send)) -> String {
    p.downcast_ref::<&str>().map(|s| s.to_string()).or_else(|| p.downcast_ref::<String>().cloned()).unwrap_or_else(|| "a record callback panicked".into())
}

impl Inner {
    fn lock(&self) -> MutexGuard<'_, St> {
        self.st.lock().unwrap_or_else(|e| e.into_inner())
    }

    fn bump(&self, st: &mut St) {
        st.generation += 1;
        self.changed.notify_all();
    }

    // The media clock restarts at `now` from the last delivered record's t (the first's when none was).
    fn anchor(&self, st: &mut St, now: f64) {
        st.anchor_wall = now;
        st.anchor_t = self.recs.get(st.idx.saturating_sub(1)).map_or(0.0, |r| r.t as f64);
    }

    fn media_now(&self, st: &St, now: f64) -> f64 {
        if st.speed <= 0.0 {
            self.recs[st.idx.saturating_sub(1)].t as f64
        } else {
            st.anchor_t + (now - st.anchor_wall) * 1e6 * st.speed
        }
    }

    fn set_state(&self, st: &mut St, s: PlayerState, changes: &mut Vec<PlayerState>) {
        st.state = s;
        changes.push(s);
    }

    // Unlock, then report the state changes on this thread.
    fn notify(&self, st: MutexGuard<'_, St>, changes: Vec<PlayerState>) {
        if changes.is_empty() {
            return;
        }
        let cbs: Vec<StateCb> = st.state_cbs.iter().map(|(_, c)| c.clone()).collect();
        drop(st);
        for s in changes {
            for cb in &cbs {
                cb(s);
            }
        }
    }

    // Deliver the next record if it is due.
    fn step(&self) -> Step {
        let mut changes = Vec::new();
        let mut st = self.lock();
        let gen = st.generation;
        if st.state != PlayerState::Playing {
            return Step::Idle(gen);
        }
        if st.idx >= self.recs.len() {
            self.set_state(&mut st, PlayerState::Done, &mut changes);
            self.bump(&mut st);
            self.notify(st, changes);
            return Step::Idle(gen + 1);
        }
        let rec = &self.recs[st.idx];
        if st.speed > 0.0 {
            let now = (self.clock)();
            let due = st.anchor_wall + (rec.t as f64 - st.anchor_t) / 1e6 / st.speed;
            if due > now {
                return Step::Wait(due - now, gen);
            }
        }
        st.idx += 1;
        st.delivering = Some(std::thread::current().id());
        let cbs: Vec<RecordCb> = st.record_cbs.iter().map(|(_, c)| c.clone()).collect();
        drop(st);
        let r = panic::catch_unwind(AssertUnwindSafe(|| {
            for cb in &cbs {
                cb(rec);
            }
        }));
        let mut st = self.lock();
        st.delivering = None;
        self.changed.notify_all();
        let out = match r {
            Ok(()) => Step::Delivered,
            // A failing consumer stops playback where it failed; the error goes to whoever drove delivery.
            Err(p) => {
                if st.state == PlayerState::Playing {
                    self.set_state(&mut st, PlayerState::Paused, &mut changes);
                    self.bump(&mut st);
                }
                Step::Failed(p)
            }
        };
        if matches!(out, Step::Delivered) && st.state == PlayerState::Playing && st.idx >= self.recs.len() {
            self.set_state(&mut st, PlayerState::Done, &mut changes);
            self.bump(&mut st);
        }
        self.notify(st, changes);
        out
    }

    fn run(&self) {
        loop {
            match self.step() {
                Step::Delivered => {}
                Step::Failed(p) => {
                    let text = panic_text(&*p);
                    let cbs: Vec<ErrorCb> = self.lock().error_cbs.iter().map(|(_, c)| c.clone()).collect();
                    for cb in cbs {
                        cb(&text);
                    }
                }
                Step::Wait(w, gen) => {
                    let st = self.lock();
                    if st.closed {
                        return;
                    }
                    if st.generation == gen {
                        drop(self.changed.wait_timeout(st, Duration::from_secs_f64(w)).unwrap());
                    }
                }
                Step::Idle(gen) => {
                    let mut st = self.lock();
                    while !st.closed && st.generation == gen {
                        st = self.changed.wait(st).unwrap();
                    }
                    if st.closed {
                        return;
                    }
                }
            }
        }
    }
}
