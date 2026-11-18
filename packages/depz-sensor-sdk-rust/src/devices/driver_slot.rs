//! A host-side ULD driver shared by the caller's thread (configuration, polling) and the reader
//! thread (stream decode), where driver calls go over the bus whose answers the reader thread
//! delivers. So the reader never waits for the driver: a frame that arrives while a call holds it
//! goes to the backlog, and that call decodes it on its way out ([`DriverSlot::release`]). Frames
//! keep their order: every batch of decoded items takes a ticket under the backlog lock and goes
//! out in ticket order — with no lock held while the callbacks run, so a callback may take the
//! driver for what it knows locally (bug hunt 2026-11-03 RUST-3). A callback's own release is
//! nested in the batch's turn: it draws no ticket (that would wait for its own thread) and leaves
//! the frames that came in meanwhile to the batch, which decodes and emits them in its turn.
//! Over the bus a callback on the reader thread gets nothing: the reader is in the callback.

use std::collections::VecDeque;
use std::sync::{Condvar, Mutex, MutexGuard, PoisonError, TryLockError};
use std::thread::{self, ThreadId};

/// The driver `D` (`None` before init) and the frames `F` that streamed in while it was held.
pub(crate) struct DriverSlot<D, F> {
    driver: Mutex<Option<D>>,
    backlog: Mutex<VecDeque<F>>,
    /// Tickets of the batches decoded so far, and how many of them went out.
    turns: Mutex<Turns>,
    served: Condvar,
}

struct Turns {
    issued: u64,
    served: u64,
    /// The thread of the batch whose turn it is, while it emits.
    owner: Option<ThreadId>,
}

/// A batch's turn to emit: taken in ticket order, given back when the batch is out — by drop,
/// so a batch that panics on its way out never blocks the next.
struct Turn<'a> {
    turns: &'a Mutex<Turns>,
    served: &'a Condvar,
}

impl Drop for Turn<'_> {
    fn drop(&mut self) {
        let mut t = self.turns.lock().unwrap_or_else(PoisonError::into_inner);
        t.served += 1;
        t.owner = None;
        drop(t);
        self.served.notify_all();
    }
}

impl<D, F> DriverSlot<D, F> {
    pub(crate) fn new() -> DriverSlot<D, F> {
        DriverSlot {
            driver: Mutex::new(None),
            backlog: Mutex::new(VecDeque::new()),
            turns: Mutex::new(Turns {
                issued: 0,
                served: 0,
                owner: None,
            }),
            served: Condvar::new(),
        }
    }

    /// Take the driver for a call; give it back with [`DriverSlot::release`].
    pub(crate) fn lock(&self) -> MutexGuard<'_, Option<D>> {
        self.driver.lock().unwrap_or_else(|p| p.into_inner())
    }

    pub(crate) fn is_some(&self) -> bool {
        self.lock().is_some()
    }

    /// Give the driver back, decoding what streamed in meanwhile: `decode` turns a frame into an
    /// item (`None`: nothing to emit — the caller counts its parse failures), `emit` sends the
    /// items on once the driver is free, in stream order. From a callback of a batch going out
    /// on this thread the backlog is left to that batch (see [`DriverSlot::emit_in_turn`]).
    pub(crate) fn release<T>(
        &self,
        mut drv: MutexGuard<'_, Option<D>>,
        mut decode: impl FnMut(&mut Option<D>, F) -> Option<T>,
        emit: impl FnMut(T),
    ) {
        if self.emitting_on_this_thread() {
            drop(drv);
            return;
        }
        let mut backlog = self.backlog.lock().unwrap();
        let out: Vec<T> = backlog
            .drain(..)
            .filter_map(|f| decode(&mut drv, f))
            .collect();
        drop(drv);
        self.emit_in_turn(backlog, out, decode, emit);
    }

    /// Whether a batch is going out on this thread — the caller is one of its callbacks.
    fn emitting_on_this_thread(&self) -> bool {
        let t = self.turns.lock().unwrap_or_else(PoisonError::into_inner);
        t.owner == Some(thread::current().id())
    }

    /// A frame from the reader thread: decoded now (after the backlog) when the driver is free,
    /// else queued for the holder to decode on release.
    pub(crate) fn feed<T>(
        &self,
        frame: F,
        mut decode: impl FnMut(&mut Option<D>, F) -> Option<T>,
        emit: impl FnMut(T),
    ) {
        let mut backlog = self.backlog.lock().unwrap();
        let mut drv = match self.driver.try_lock() {
            Ok(g) => g,
            Err(TryLockError::Poisoned(p)) => p.into_inner(),
            Err(TryLockError::WouldBlock) => {
                backlog.push_back(frame);
                return;
            }
        };
        let mut out: Vec<T> = backlog
            .drain(..)
            .filter_map(|f| decode(&mut drv, f))
            .collect();
        out.extend(decode(&mut drv, frame));
        drop(drv);
        self.emit_in_turn(backlog, out, decode, emit);
    }

    /// Send `out` on in decode order: its ticket is drawn while the backlog is still held (so
    /// batches are numbered as they were decoded), the backlog is let go, and the batch waits
    /// for its turn with nothing locked — a callback may take the driver and release it. An
    /// empty batch draws no ticket. Frames that came in while a callback held the driver are
    /// left by its (nested) release: the batch decodes and emits them in the same turn, until the
    /// backlog is empty or another thread's call holds the driver (that call decodes them on
    /// its release, after this turn).
    fn emit_in_turn<T>(
        &self,
        backlog: MutexGuard<'_, VecDeque<F>>,
        mut out: Vec<T>,
        mut decode: impl FnMut(&mut Option<D>, F) -> Option<T>,
        mut emit: impl FnMut(T),
    ) {
        if out.is_empty() {
            return;
        }
        let ticket = {
            let mut t = self.turns.lock().unwrap_or_else(PoisonError::into_inner);
            t.issued += 1;
            t.issued - 1
        };
        drop(backlog);
        let turn = {
            let mut t = self.turns.lock().unwrap_or_else(PoisonError::into_inner);
            while t.served != ticket {
                t = self.served.wait(t).unwrap_or_else(PoisonError::into_inner);
            }
            t.owner = Some(thread::current().id());
            Turn {
                turns: &self.turns,
                served: &self.served,
            }
        };
        loop {
            out.into_iter().for_each(&mut emit);
            let mut backlog = self.backlog.lock().unwrap();
            if backlog.is_empty() {
                break;
            }
            let mut drv = match self.driver.try_lock() {
                Ok(g) => g,
                Err(TryLockError::Poisoned(p)) => p.into_inner(),
                Err(TryLockError::WouldBlock) => break,
            };
            out = backlog
                .drain(..)
                .filter_map(|f| decode(&mut drv, f))
                .collect();
        }
        drop(turn);
    }
}
