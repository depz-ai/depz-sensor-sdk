//! Stream subscribers (contract 07 §3): callbacks and bounded pull queues.

use std::collections::VecDeque;
use std::panic::{catch_unwind, AssertUnwindSafe};
use std::sync::atomic::{AtomicU64, Ordering};
use std::sync::{Arc, Condvar, Mutex, Weak};
use std::time::{Duration, Instant};

use super::error::{DepzError, Result};

type Callback<T> = Arc<dyn Fn(&T) + Send + Sync>;

struct QueueState<T> {
    items: VecDeque<T>,
    dropped: u64,
    closed: bool,
}

struct Queue<T> {
    capacity: usize,
    state: Mutex<QueueState<T>>,
    ready: Condvar,
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

    fn mark_closed(&self) {
        self.state.lock().unwrap().closed = true;
        self.ready.notify_all();
    }
}

struct FanoutInner<T> {
    callbacks: Mutex<Vec<(u64, Callback<T>)>>,
    queues: Mutex<Vec<Arc<Queue<T>>>>,
    next_id: Mutex<u64>,
    /// Callbacks that panicked — the device's `LinkStats::callback_errors`.
    callback_errors: Arc<AtomicU64>,
}

/// The subscribers of one stream. Cloning shares it.
pub(crate) struct Fanout<T> {
    inner: Arc<FanoutInner<T>>,
}

impl<T> Clone for Fanout<T> {
    fn clone(&self) -> Self {
        Fanout {
            inner: self.inner.clone(),
        }
    }
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

impl<T: Clone + Send + 'static> Fanout<T> {
    /// A stream whose panicking callbacks count in `callback_errors` — the device's counter
    /// ([`super::device::DepzDevice::callback_errors_counter`]).
    pub(crate) fn new(callback_errors: Arc<AtomicU64>) -> Self {
        Fanout {
            inner: Arc::new(FanoutInner {
                callbacks: Mutex::new(Vec::new()),
                queues: Mutex::new(Vec::new()),
                next_id: Mutex::new(0),
                callback_errors,
            }),
        }
    }

    pub(crate) fn on_item(&self, callback: impl Fn(&T) + Send + Sync + 'static) -> Hook {
        let id = {
            let mut n = self.inner.next_id.lock().unwrap();
            *n += 1;
            *n
        };
        self.inner
            .callbacks
            .lock()
            .unwrap()
            .push((id, Arc::new(callback)));
        let weak: Weak<FanoutInner<T>> = Arc::downgrade(&self.inner);
        Hook::new(move || {
            if let Some(f) = weak.upgrade() {
                f.callbacks.lock().unwrap().retain(|(i, _)| *i != id);
            }
        })
    }

    pub(crate) fn subscribe(&self, capacity: usize) -> Subscription<T> {
        let q = Arc::new(Queue {
            capacity: capacity.max(1),
            state: Mutex::new(QueueState {
                items: VecDeque::new(),
                dropped: 0,
                closed: false,
            }),
            ready: Condvar::new(),
        });
        self.inner.queues.lock().unwrap().push(q.clone());
        Subscription {
            queue: q,
            fanout: Arc::downgrade(&self.inner),
        }
    }

    pub(crate) fn dropped_counts(&self) -> Vec<u64> {
        self.inner
            .queues
            .lock()
            .unwrap()
            .iter()
            .map(|q| q.state.lock().unwrap().dropped)
            .collect()
    }

    pub(crate) fn emit(&self, item: T) {
        let cbs: Vec<Callback<T>> = self
            .inner
            .callbacks
            .lock()
            .unwrap()
            .iter()
            .map(|(_, c)| c.clone())
            .collect();
        for cb in cbs {
            call_guarded(|| cb(&item), &self.inner.callback_errors);
        }
        let qs: Vec<Arc<Queue<T>>> = self.inner.queues.lock().unwrap().clone();
        for q in qs {
            q.put(item.clone());
        }
    }

    /// The next item of this stream (contract 07 §3): a one-item subscription taken at once.
    pub(crate) fn next(&self, timeout: Duration, report_id: u8, device_closed: bool) -> Result<T> {
        if device_closed {
            return Err(DepzError::DeviceLost("the device is closed".into()));
        }
        let sub = self.subscribe(1);
        match sub.take(timeout) {
            Err(DepzError::Timeout { .. }) => Err(DepzError::Timeout {
                cmd: Some(report_id),
                message: format!(
                    "no report 0x{report_id:02X} within {:.3}s",
                    timeout.as_secs_f64()
                ),
            }),
            other => other,
        }
    }

    pub(crate) fn close_all(&self) {
        for q in self.inner.queues.lock().unwrap().iter() {
            q.mark_closed();
        }
    }
}

/// A pull subscription to one stream (contract 07 §3): a bounded queue that drops the oldest item
/// when full and counts the drops. It is registered when created, so nothing produced after that is
/// missed. Dropping it unsubscribes.
pub struct Subscription<T> {
    queue: Arc<Queue<T>>,
    fanout: Weak<FanoutInner<T>>,
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
    /// arrived, [`DepzError::DeviceLost`] when the device closed with the queue empty.
    pub fn take(&self, timeout: Duration) -> Result<T> {
        let deadline = Instant::now() + timeout;
        let mut s = self.queue.state.lock().unwrap();
        loop {
            if let Some(item) = s.items.pop_front() {
                return Ok(item);
            }
            if s.closed {
                return Err(DepzError::DeviceLost(
                    "stream closed: the device is closed".into(),
                ));
            }
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
        self.queue.mark_closed();
        if let Some(f) = self.fanout.upgrade() {
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
