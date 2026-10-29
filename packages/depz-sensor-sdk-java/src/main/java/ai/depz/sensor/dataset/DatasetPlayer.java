package ai.depz.sensor.dataset;

import ai.depz.sensor.devices.CallbackGuard;
import ai.depz.sensor.devices.InvalidStateException;
import java.nio.file.Path;
import java.time.Duration;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicLong;
import java.util.concurrent.atomic.AtomicReference;
import java.util.concurrent.locks.Condition;
import java.util.concurrent.locks.ReentrantLock;
import java.util.function.Consumer;
import java.util.function.DoubleSupplier;

/**
 * Timeline player over a dataset: paced delivery with play / pause / seek / speed — the engine behind a
 * viewer's playback mode (contract 09 §Player; reference: the Python {@code DatasetPlayer}).
 *
 * <p>Timing: a media clock anchored at play / resume / seek to (now, t of the last delivered record — of
 * the first when none was). Record i is due at {@code anchor_wall + (t_i − anchor_t) / speed}: the first
 * record goes out at once, each later one after its gap to the previous one. Pacing is against the clock,
 * so a slow consumer gets overdue records back to back and playback does not drift. Speed 0 delivers as
 * fast as possible.
 *
 * <p>Threads: records and the {@link PlayerState#DONE} state are delivered on a player thread (started by
 * the first {@link #play()}); a state change a call makes is reported on the calling thread before it
 * returns. Callbacks run under the player's lock — they see a consistent player and may call its methods,
 * and once {@link #pause()} returns no record callback is running or will start. They must not block. A
 * callback that throws is logged ({@code java.util.logging} logger {@code ai.depz.sensor}), counted in
 * {@link #callbackErrors()} and dropped: the other callbacks still get the record and playback goes on. With
 * {@code threaded = false} no thread starts and the owner calls {@link #tick()} (a GUI timer, or a test
 * with a fake clock).
 */
public final class DatasetPlayer implements AutoCloseable {
    /** The player thread's name. */
    public static final String THREAD_NAME = "depz-dataset-player";

    // Fair, so a control call waiting for the lock gets it between two records even at speed 0.
    private final ReentrantLock lock = new ReentrantLock(true);
    private final Condition changed = lock.newCondition();
    private final Dataset reader;
    private final List<Dataset.Record> recs;
    private final DoubleSupplier clock;
    private final boolean threaded;
    private final List<Consumer<Dataset.Record>> recordCallbacks = new ArrayList<>();
    private final List<Consumer<PlayerState>> stateCallbacks = new ArrayList<>();
    private final AtomicLong callbackErrors = new AtomicLong();
    private PlayerState state = PlayerState.IDLE;
    private double speed;
    private int idx; // the next record to deliver
    private double anchorWall;
    private double anchorT;
    private Thread thread;
    private boolean closed;

    private record Step(boolean delivered, Double waitS) {}

    /**
     * @param reader the dataset
     * @param speed ≥ 0; 0 = as fast as possible
     * @param clock monotonic seconds (null: {@link System#nanoTime()})
     * @param threaded false: no player thread, drive it with {@link #tick()}
     */
    public DatasetPlayer(Dataset reader, double speed, DoubleSupplier clock, boolean threaded) {
        if (reader == null) {
            throw new IllegalArgumentException("reader must not be null");
        }
        this.reader = reader;
        this.recs = reader.records();
        long start = System.nanoTime();
        this.clock = clock != null ? clock : () -> (System.nanoTime() - start) / 1e9;
        this.threaded = threaded;
        this.speed = checkSpeed(speed);
    }

    /** Threaded, on the system clock. */
    public DatasetPlayer(Dataset reader, double speed) {
        this(reader, speed, null, true);
    }

    /** Threaded, real time, on the system clock. */
    public DatasetPlayer(Dataset reader) {
        this(reader, 1.0, null, true);
    }

    /** A player over the {@code .depzdata} file at {@code path}. */
    public DatasetPlayer(Path path, double speed, DoubleSupplier clock, boolean threaded) {
        this(Dataset.read(path), speed, clock, threaded);
    }

    public DatasetPlayer(Path path, double speed) {
        this(Dataset.read(path), speed, null, true);
    }

    public DatasetPlayer(Path path) {
        this(Dataset.read(path), 1.0, null, true);
    }

    public Dataset reader() {
        return reader;
    }

    public PlayerState state() {
        lock.lock();
        try {
            return state;
        } finally {
            lock.unlock();
        }
    }

    /** Span of the timeline: the last record's t minus the first's. */
    public long durationUs() {
        return reader.durationUs();
    }

    /**
     * µs from the first record to the last delivered one (after a seek: to the record just before the
     * target); 0 before anything was delivered.
     */
    public long positionUs() {
        lock.lock();
        try {
            if (recs.isEmpty() || idx == 0) {
                return 0;
            }
            return recs.get(Math.min(idx, recs.size()) - 1).tHostUs() - recs.get(0).tHostUs();
        } finally {
            lock.unlock();
        }
    }

    /** Record / state callbacks that threw and were dropped (contract 09). */
    public long callbackErrors() {
        return callbackErrors.get();
    }

    /** Playback speed (finite, ≥ 0; 0 = as fast as possible). */
    public double speed() {
        lock.lock();
        try {
            return speed;
        } finally {
            lock.unlock();
        }
    }

    /**
     * Set the playback speed (finite, ≥ 0, else {@link IllegalArgumentException}). A change while playing
     * applies at once: the rest of the current wait is rescaled.
     */
    public void setSpeed(double value) {
        double s = checkSpeed(value);
        lock.lock();
        try {
            if (state == PlayerState.PLAYING) {
                double now = clock.getAsDouble();
                anchorT = mediaNow(now);
                anchorWall = now;
            }
            speed = s;
            changed.signalAll();
        } finally {
            lock.unlock();
        }
    }

    /** Subscribe to delivered records; close the result to unsubscribe (idempotent). */
    public AutoCloseable onRecord(Consumer<Dataset.Record> callback) {
        return subscribe(recordCallbacks, callback);
    }

    /** Subscribe to state changes, called with the new state on every transition. */
    public AutoCloseable onState(Consumer<PlayerState> callback) {
        return subscribe(stateCallbacks, callback);
    }

    private <T> AutoCloseable subscribe(List<Consumer<T>> list, Consumer<T> callback) {
        if (callback == null) {
            throw new IllegalArgumentException("callback must not be null");
        }
        lock.lock();
        try {
            list.add(callback);
        } finally {
            lock.unlock();
        }
        AtomicReference<Runnable> undo = new AtomicReference<>(() -> {
            lock.lock();
            try {
                list.remove(callback);
            } finally {
                lock.unlock();
            }
        });
        return () -> {
            Runnable r = undo.getAndSet(null);
            if (r != null) {
                r.run();
            }
        };
    }

    /**
     * Start, or resume after {@link #pause()}; no-op while playing. From {@link PlayerState#DONE} (or a
     * position past the end) playback restarts at the first record. An empty dataset goes to done at once.
     */
    public void play() {
        lock.lock();
        try {
            checkOpen();
            if (state == PlayerState.PLAYING) {
                return;
            }
            if (idx >= recs.size()) {
                idx = 0;
            }
            anchor(clock.getAsDouble());
            setState(PlayerState.PLAYING);
            if (recs.isEmpty()) {
                setState(PlayerState.DONE);
                return;
            }
            ensureThread();
            changed.signalAll();
        } finally {
            lock.unlock();
        }
    }

    /** Stop delivering; {@link #play()} resumes from the next record, which then waits its full gap again. No-op unless playing. */
    public void pause() {
        lock.lock();
        try {
            if (state != PlayerState.PLAYING) {
                return;
            }
            setState(PlayerState.PAUSED);
            changed.signalAll();
        } finally {
            lock.unlock();
        }
    }

    /**
     * Move to {@code offsetUs} µs from the first record: the next record delivered is the first with t ≥
     * first t + offset (negative: the start). Records in between are skipped. The state does not change,
     * except that a seek past the end while playing ends playback.
     */
    public void seekUs(double offsetUs) {
        if (Double.isNaN(offsetUs)) {
            throw new IllegalArgumentException("seek_us: offset must be a number");
        }
        lock.lock();
        try {
            checkOpen();
            if (recs.isEmpty()) {
                return;
            }
            double target = recs.get(0).tHostUs() + offsetUs;
            int lo = 0;
            int hi = recs.size();
            while (lo < hi) {
                int mid = (lo + hi) >>> 1;
                if (recs.get(mid).tHostUs() < target) {
                    lo = mid + 1;
                } else {
                    hi = mid;
                }
            }
            idx = lo;
            if (state == PlayerState.PLAYING) {
                if (idx >= recs.size()) {
                    setState(PlayerState.DONE);
                } else {
                    anchor(clock.getAsDouble());
                }
                changed.signalAll();
            }
        } finally {
            lock.unlock();
        }
    }

    /**
     * Deliver every record due now, on the calling thread. Returns the seconds until the next one is due,
     * or null when not playing. For {@code threaded = false}; harmless with a player thread.
     */
    public Double tick() {
        lock.lock();
        try {
            while (true) {
                Step s = stepLocked();
                if (!s.delivered()) {
                    return s.waitS();
                }
            }
        } finally {
            lock.unlock();
        }
    }

    /**
     * Block until the player is not playing (done, or paused by someone else): true, or false on timeout
     * (null: no timeout). Needs the player thread, else {@link InvalidStateException}. (Python's
     * {@code wait}; {@code wait} is taken by {@link Object} in Java.)
     */
    public boolean await(Duration timeout) {
        if (!threaded) {
            throw new InvalidStateException("await() needs threaded = true; drive tick() instead");
        }
        long end = timeout == null ? 0 : System.nanoTime() + timeout.toNanos();
        lock.lock();
        try {
            while (state == PlayerState.PLAYING) {
                if (timeout != null) {
                    long left = end - System.nanoTime();
                    if (left <= 0 || !changed.await(left, TimeUnit.NANOSECONDS)) {
                        return state != PlayerState.PLAYING;
                    }
                } else {
                    changed.await();
                }
            }
            return true;
        } catch (InterruptedException e) {
            Thread.currentThread().interrupt();
            return state != PlayerState.PLAYING;
        } finally {
            lock.unlock();
        }
    }

    /** {@link #await(Duration)} without a timeout. */
    public boolean await() {
        return await(null);
    }

    /** Stop playback and the player thread. Idempotent. */
    @Override
    public void close() {
        lock.lock();
        try {
            if (closed) {
                return;
            }
            if (state == PlayerState.PLAYING) {
                setState(PlayerState.PAUSED);
            }
            closed = true;
            changed.signalAll();
        } finally {
            lock.unlock();
        }
        Thread t = thread;
        if (t != null && !t.equals(Thread.currentThread())) {
            try {
                t.join(2000);
            } catch (InterruptedException e) {
                Thread.currentThread().interrupt();
            }
        }
        thread = null;
    }

    // ── internals (under lock) ──────────────────────────────────────────────

    private static double checkSpeed(double value) {
        if (Double.isNaN(value) || Double.isInfinite(value) || value < 0) {
            throw new IllegalArgumentException("speed must be a finite number >= 0, got " + value);
        }
        return value;
    }

    private void checkOpen() {
        if (closed) {
            throw new InvalidStateException("DatasetPlayer is closed");
        }
    }

    // The media clock restarts at `now` from the last delivered record's t (the first record's when none was).
    private void anchor(double now) {
        anchorWall = now;
        anchorT = recs.isEmpty() ? 0 : recs.get(Math.max(idx - 1, 0)).tHostUs();
    }

    private double mediaNow(double now) {
        return speed <= 0 ? recs.get(Math.max(idx - 1, 0)).tHostUs() : anchorT + (now - anchorWall) * 1e6 * speed;
    }

    private void setState(PlayerState s) {
        state = s;
        changed.signalAll();
        deliver(stateCallbacks, s);
    }

    // Hand `item` to every callback; one that throws is counted, logged and dropped (contract 09).
    private <T> void deliver(List<Consumer<T>> callbacks, T item) {
        for (Consumer<T> cb : new ArrayList<>(callbacks)) {
            if (!CallbackGuard.call(cb, item)) {
                callbackErrors.incrementAndGet();
            }
        }
    }

    // Deliver the next record if it is due → (true, _); else (false, seconds to wait) or (false, null) when not playing.
    private Step stepLocked() {
        if (state != PlayerState.PLAYING) {
            return new Step(false, null);
        }
        if (idx >= recs.size()) {
            setState(PlayerState.DONE);
            return new Step(false, null);
        }
        Dataset.Record rec = recs.get(idx);
        if (speed > 0) {
            double now = clock.getAsDouble();
            double due = anchorWall + (rec.tHostUs() - anchorT) / 1e6 / speed;
            if (due > now) {
                return new Step(false, due - now);
            }
        }
        idx++;
        deliver(recordCallbacks, rec);
        if (state == PlayerState.PLAYING && idx >= recs.size()) {
            setState(PlayerState.DONE);
        }
        return new Step(true, null);
    }

    private void ensureThread() {
        if (!threaded || (thread != null && thread.isAlive())) {
            return;
        }
        thread = new Thread(this::run, THREAD_NAME);
        thread.setDaemon(true);
        thread.start();
    }

    /** Releases and retakes the held lock, so a waiting control call gets it in between. */
    private void letOthersIn() {
        lock.unlock();
        lock.lock();
    }

    private void run() {
        lock.lock();
        try {
            while (!closed) {
                Step r = stepLocked();
                if (r.delivered()) {
                    // let control calls (pause / seek from other threads) in between records, even at speed 0
                    letOthersIn();
                    continue;
                }
                if (r.waitS() != null) {
                    changed.awaitNanos((long) (r.waitS() * 1e9));
                } else {
                    changed.await();
                }
            }
        } catch (InterruptedException e) {
            // the thread is a daemon; an interrupt just ends it
        } finally {
            lock.unlock();
        }
    }
}
