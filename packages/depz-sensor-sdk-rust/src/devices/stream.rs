//! The stream object (contract 07 §3): one per kind of item a device emits — `measurements`,
//! `frames`, `samples`, `reports`, `events` — with callbacks, bounded pull queues, a one-shot
//! `next`, and the stream's counters.

use std::collections::VecDeque;
use std::panic::{catch_unwind, AssertUnwindSafe};
use std::sync::atomic::{AtomicBool, AtomicU64, Ordering};
use std::sync::{Arc, Condvar, Mutex, Weak};
use std::time::{Duration, Instant};

use super::error::{DepzError, Result};

/// What [`Stream::next`] waits when given `None`: 2 s (contract 07 §3).
const DEFAULT_NEXT_TIMEOUT: Duration = Duration::from_secs(2);

type Callback<T> = Arc<dyn Fn(&T) + Send + Sync>;
type Where<T> = Arc<dyn Fn(&T) -> bool + Send + Sync>;
/// A registered callback: its id (for the hook), the callback, the items it takes.
type Registered<T> = (u64, Callback<T>, Option<Where<T>>);

struct QueueState<T> {
    items: VecDeque<T>,
    dropped: u64,
    closed: bool,
    /// Why it closed: the loss's message after a loss; `None`: this side closed it.
    lost: Option<String>,
}

struct Queue<T> {
    capacity: usize,
    state: Mutex<QueueState<T>>,
    ready: Condvar,
    /// The items this queue keeps; `None`: all of them.
    accepts: Option<Where<T>>,
}

impl<T> Queue<T> {
    fn put(&self, item: T) {
        let mut s = self.state.lock().unwrap();
        if s.closed {
            return;
        }
        if s.items.len() >= self.capacity {
            s.items.pop_front();
            s.dropped += 1;
        }
        s.items.push_back(item);
        self.ready.notify_all();
    }

    /// No more items, for the reason `lost` (`None`: this side closed it); the first reason stays.
    fn mark_closed(&self, lost: Option<&str>) {
        let mut s = self.state.lock().unwrap();
        if !s.closed {
            s.closed = true;
            s.lost = lost.map(str::to_owned);
        }
        drop(s);
        self.ready.notify_all();
    }
}

struct Inner<T> {
    callbacks: Mutex<Vec<Registered<T>>>,
    queues: Mutex<Vec<Arc<Queue<T>>>>,
    /// Set by `close` under the `queues` lock, never cleared: a later subscription is closed.
    closed: AtomicBool,
    /// Why the stream ended, set with `closed`: the loss's message; `None`: this side closed it.
    lost: Mutex<Option<String>>,
    next_id: Mutex<u64>,
    /// Callbacks that panicked — the device's `LinkStats::callback_errors`.
    callback_errors: Arc<AtomicU64>,
    /// Reports of this stream that did not decode (contract 07 §2).
    parse_errors: AtomicU64,
    /// The report `next` waits for and names on a timeout; `None` on the events stream.
    what: Option<u8>,
}

/// `timeout` from now, or `None` when no `Instant` can hold it (`Duration::MAX`, the idiom for
/// "no timeout"): the wait then has no deadline instead of panicking on `Instant + Duration`.
pub(crate) fn deadline_after(timeout: Duration) -> Option<Instant> {
    Instant::now().checked_add(timeout)
}

/// Run a user callback where its panic must not stop the caller (a device reader, a dataset
/// player — contract 07 §3, 09 §Player): a panic is counted in `errors`, reported on stderr (the
/// panic hook has already printed it) and dropped.
pub(crate) fn call_guarded(f: impl FnOnce(), errors: &AtomicU64) {
    if catch_unwind(AssertUnwindSafe(f)).is_err() {
        errors.fetch_add(1, Ordering::Relaxed);
        eprintln!("depz-sensor-sdk: a callback panicked; dropped");
    }
}

/// One kind of item a device emits, as a stream (contract 07 §3): `measurements` (SR04,
/// VL53L4CD, the VL53Lx family, TMF8829), `frames` (VL53L8CX/CH, L5CX, L7CX/CH), `samples`
/// (BNO055), `reports` (BNO086) and `events` (every device) are `Stream`s owned by the device.
/// [`Stream::on`] calls back on the reader thread and returns the [`Hook`] that unregisters;
/// [`Stream::subscribe`] is a bounded drop-oldest pull queue ([`Subscription`]);
/// [`Stream::next`] waits for one item; [`Stream::dropped_counts`] and [`Stream::parse_errors`]
/// are its counters. Items reach every callback, then every queue, in arrival order; the stream
/// ends with the device.
pub struct Stream<T> {
    inner: Arc<Inner<T>>,
}

impl<T: Clone + Send + 'static> Stream<T> {
    /// A stream whose panicking callbacks count in `callback_errors` — the device's counter —
    /// and whose `next` waits for the report `what` (`None`: the events stream, which has none).
    pub(crate) fn new(callback_errors: Arc<AtomicU64>, what: Option<u8>) -> Self {
        Stream {
            inner: Arc::new(Inner {
                callbacks: Mutex::new(Vec::new()),
                queues: Mutex::new(Vec::new()),
                closed: AtomicBool::new(false),
                lost: Mutex::new(None),
                next_id: Mutex::new(0),
                callback_errors,
                parse_errors: AtomicU64::new(0),
                what,
            }),
        }
    }

    /// Another handle on the same stream (a sensor class that emits device events).
    pub(crate) fn share(&self) -> Self {
        Stream {
            inner: self.inner.clone(),
        }
    }

    // ── consumer side ────────────────────────────────────────────────────────

    /// Call `callback` for every item (on the reader thread: keep it short, never block, never
    /// close the device from it). Drop the hook to stop.
    pub fn on(&self, callback: impl Fn(&T) + Send + Sync + 'static) -> Hook {
        self.hook(Arc::new(callback), None)
    }

    /// [`Stream::on`] for the items `accept` keeps (a BNO086 sensor-id filter, say).
    pub fn on_where(
        &self,
        callback: impl Fn(&T) + Send + Sync + 'static,
        accept: impl Fn(&T) -> bool + Send + Sync + 'static,
    ) -> Hook {
        self.hook(Arc::new(callback), Some(Arc::new(accept)))
    }

    fn hook(&self, callback: Callback<T>, accept: Option<Where<T>>) -> Hook {
        let id = {
            let mut n = self.inner.next_id.lock().unwrap();
            *n += 1;
            *n
        };
        self.inner
            .callbacks
            .lock()
            .unwrap()
            .push((id, callback, accept));
        let weak: Weak<Inner<T>> = Arc::downgrade(&self.inner);
        Hook::new(move || {
            if let Some(f) = weak.upgrade() {
                f.callbacks.lock().unwrap().retain(|(i, ..)| *i != id);
            }
        })
    }

    /// A pull queue of up to `capacity` items (bounded, drop-oldest), subscribed when this
    /// returns: nothing emitted after it is missed.
    pub fn subscribe(&self, capacity: usize) -> Subscription<T> {
        self.queue(capacity, None)
    }

    /// [`Stream::subscribe`] for the items `accept` keeps.
    pub fn subscribe_where(
        &self,
        capacity: usize,
        accept: impl Fn(&T) -> bool + Send + Sync + 'static,
    ) -> Subscription<T> {
        self.queue(capacity, Some(Arc::new(accept)))
    }

    fn queue(&self, capacity: usize, accept: Option<Where<T>>) -> Subscription<T> {
        let q = Arc::new(Queue {
            capacity: capacity.max(1),
            state: Mutex::new(QueueState {
                items: VecDeque::new(),
                dropped: 0,
                closed: false,
                lost: None,
            }),
            ready: Condvar::new(),
            accepts: accept,
        });
        let mut queues = self.inner.queues.lock().unwrap();
        if self.inner.closed.load(Ordering::SeqCst) {
            // The device already ended: a closed queue, so take() fails at once (contract 07 §3).
            q.mark_closed(self.inner.lost.lock().unwrap().as_deref());
        } else {
            queues.push(q.clone());
        }
        drop(queues);
        Subscription {
            queue: q,
            stream: Arc::downgrade(&self.inner),
        }
    }

    /// The next item, on a one-item queue subscribed for this call (contract 07 §3); `timeout`
    /// `None`: 2 s. [`DepzError::Timeout`] naming the stream's report when nothing arrived; at
    /// once when the device has ended, or as soon as it ends while waiting (an item already
    /// queued still wins), [`DepzError::LinkClosed`] after the program's own `close()` and
    /// [`DepzError::DeviceLost`] with the cause after a loss (contract 07 §3). The events stream has no single report to wait for: there the call is
    /// [`DepzError::InvalidState`].
    pub fn next(&self, timeout: Option<Duration>) -> Result<T> {
        let Some(report_id) = self.inner.what else {
            return Err(DepzError::InvalidState(
                "the events stream offers no next(): no single report to wait for (contract 07 \
                 §3) — subscribe with events().on(cb) or events().subscribe(capacity)"
                    .into(),
            ));
        };
        let timeout = timeout.unwrap_or(DEFAULT_NEXT_TIMEOUT);
        match self.subscribe(1).take(timeout) {
            Err(DepzError::Timeout { .. }) => Err(DepzError::Timeout {
                cmd: Some(report_id),
                message: format!(
                    "no report 0x{report_id:02X} within {:.3}s",
                    timeout.as_secs_f64()
                ),
            }),
            Err(DepzError::LinkClosed(_)) => {
                Err(DepzError::LinkClosed("device closed while waiting".into()))
            }
            other => other,
        }
    }

    /// The drop counter of every live queue ([`Subscription::dropped_count`]); a dropped
    /// subscription leaves the list.
    pub fn dropped_counts(&self) -> Vec<u64> {
        self.inner
            .queues
            .lock()
            .unwrap()
            .iter()
            .map(|q| q.state.lock().unwrap().dropped)
            .collect()
    }

    /// Reports of this stream dropped because they did not decode (contract 07 §2): shorter
    /// than their header, or a value the decoder refused. Monotonic over the device's life, except
    /// where a sensor contract resets it (TMF8829 `start()`).
    pub fn parse_errors(&self) -> u64 {
        self.inner.parse_errors.load(Ordering::Relaxed)
    }

    // ── owner side (the sensor class, on the reader thread) ──────────────────

    /// Hand `item` to every callback, then to every queue (contract 07 §3). A callback (or a
    /// filter) that panics is counted and dropped; it never stops the reader or the others.
    pub(crate) fn emit(&self, item: T) {
        let cbs: Vec<(Callback<T>, Option<Where<T>>)> = self
            .inner
            .callbacks
            .lock()
            .unwrap()
            .iter()
            .map(|(_, c, w)| (c.clone(), w.clone()))
            .collect();
        for (cb, accept) in cbs {
            call_guarded(
                || {
                    if accept.as_ref().is_none_or(|w| w(&item)) {
                        cb(&item);
                    }
                },
                &self.inner.callback_errors,
            );
        }
        let qs: Vec<Arc<Queue<T>>> = self.inner.queues.lock().unwrap().clone();
        for q in qs {
            call_guarded(
                || {
                    if q.accepts.as_ref().is_none_or(|w| w(&item)) {
                        q.put(item.clone());
                    }
                },
                &self.inner.callback_errors,
            );
        }
    }

    /// One more report of this stream that did not decode.
    pub(crate) fn count_parse_error(&self) {
        self.inner.parse_errors.fetch_add(1, Ordering::Relaxed);
    }

    /// Where a sensor contract restarts the counter (TMF8829 `start()`).
    pub(crate) fn reset_parse_errors(&self) {
        self.inner.parse_errors.store(0, Ordering::Relaxed);
    }

    /// The device ended: close every queue, and every one subscribed from now on. `lost` is the
    /// loss's message; `None` after the program's own `close()`.
    pub(crate) fn close(&self, lost: Option<&str>) {
        let queues = self.inner.queues.lock().unwrap();
        *self.inner.lost.lock().unwrap() = lost.map(str::to_owned);
        self.inner.closed.store(true, Ordering::SeqCst);
        for q in queues.iter() {
            q.mark_closed(lost);
        }
    }
}

/// A pull subscription to one stream (contract 07 §3): a bounded queue that drops the oldest item
/// when full and counts the drops. It is registered when created, so nothing produced after that is
/// missed. Dropping it unsubscribes.
pub struct Subscription<T> {
    queue: Arc<Queue<T>>,
    stream: Weak<Inner<T>>,
}

impl<T> Subscription<T> {
    /// Items dropped because the queue was full (monotonic).
    pub fn dropped_count(&self) -> u64 {
        self.queue.state.lock().unwrap().dropped
    }

    /// Items waiting.
    pub fn len(&self) -> usize {
        self.queue.state.lock().unwrap().items.len()
    }

    pub fn is_empty(&self) -> bool {
        self.len() == 0
    }

    /// The device closed: no more items will arrive.
    pub fn is_closed(&self) -> bool {
        self.queue.state.lock().unwrap().closed
    }

    /// The oldest waiting item, without blocking.
    pub fn try_take(&self) -> Option<T> {
        self.queue.state.lock().unwrap().items.pop_front()
    }

    /// The oldest item, waiting up to `timeout` for one: [`DepzError::Timeout`] when nothing
    /// arrived; with the queue empty and the device ended, [`DepzError::LinkClosed`] after the
    /// program's own `close()` and [`DepzError::DeviceLost`] with the cause after a loss
    /// (contract 07 §3).
    pub fn take(&self, timeout: Duration) -> Result<T> {
        let deadline = deadline_after(timeout);
        let mut s = self.queue.state.lock().unwrap();
        loop {
            if let Some(item) = s.items.pop_front() {
                return Ok(item);
            }
            if s.closed {
                return Err(match &s.lost {
                    Some(m) => DepzError::DeviceLost(m.clone()),
                    None => DepzError::LinkClosed("stream closed: the device is closed".into()),
                });
            }
            let Some(deadline) = deadline else {
                s = self.queue.ready.wait(s).unwrap();
                continue;
            };
            let now = Instant::now();
            if now >= deadline {
                return Err(DepzError::Timeout {
                    cmd: None,
                    message: format!("no stream item within {:.3}s", timeout.as_secs_f64()),
                });
            }
            s = self.queue.ready.wait_timeout(s, deadline - now).unwrap().0;
        }
    }

    /// Everything waiting now, oldest first, without blocking.
    pub fn drain(&self) -> Vec<T> {
        self.queue.state.lock().unwrap().items.drain(..).collect()
    }
}

impl<T> Drop for Subscription<T> {
    fn drop(&mut self) {
        self.queue.mark_closed(None);
        if let Some(f) = self.stream.upgrade() {
            f.queues
                .lock()
                .unwrap()
                .retain(|q| !Arc::ptr_eq(q, &self.queue));
        }
    }
}

/// A registered callback; dropping it (or [`Hook::cancel`]) unregisters. [`Hook::forget`] keeps the
/// callback for the device's lifetime.
#[must_use = "dropping the hook unregisters the callback; call forget() to keep it"]
pub struct Hook {
    undo: Option<Box<dyn FnOnce() + Send>>,
}

impl Hook {
    pub(crate) fn new(undo: impl FnOnce() + Send + 'static) -> Hook {
        Hook {
            undo: Some(Box::new(undo)),
        }
    }

    /// Unregister now.
    pub fn cancel(mut self) {
        if let Some(u) = self.undo.take() {
            u();
        }
    }

    /// Keep the callback registered for as long as the device lives.
    pub fn forget(mut self) {
        self.undo = None;
    }
}

impl Drop for Hook {
    fn drop(&mut self) {
        if let Some(u) = self.undo.take() {
            u();
        }
    }
}
