package ai.depz.sensor.devices;

import ai.depz.sensor.internal.CallbackGuard;
import java.time.Duration;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicLong;
import java.util.function.Consumer;
import java.util.function.Predicate;
import org.jspecify.annotations.Nullable;

/**
 * One kind of item a device emits, as its consumer sees it (contract 07 §3): what {@code
 * Sr04Device.measurements()}, {@code Vl53l8cxDevice.frames()}, {@code Bno055Device.samples()},
 * {@code Bno086Device.reports()} and {@link DepzDevice#events()} return. {@link #on} subscribes a
 * callback (reader thread: keep it short, never block, never close the device from it) and returns
 * the handle that unsubscribes; {@link #subscribe} makes a {@link Subscription} — a bounded queue
 * that drops the oldest item when full — registered when it returns, so nothing emitted after that
 * is missed; {@link #next} waits for one item on a one-slot queue of its own. {@code where} keeps
 * only the items it accepts (a BNO086 sensor-id filter, for instance). {@link #droppedCounts} and
 * {@link #parseErrors} are its counters. The owner side — emitting, counting, closing — is the
 * device's.
 */
public final class SensorStream<T> {
    /** The default timeout of {@link #next()}, 2 s (contract 07 §3). */
    public static final Duration DEFAULT_NEXT_TIMEOUT = Duration.ofSeconds(2);

    /** The report id of a stream that offers no {@code next()}: {@code events} (contract 07 §3). */
    static final int NO_REPORT = -1;

    /** {@code ended} is set once {@code onEnd} ran or the handle was closed: it runs at most once. */
    private record Callback<T>(Consumer<T> cb, @Nullable Predicate<T> where,
            @Nullable Consumer<@Nullable DeviceLostException> onEnd, AtomicBoolean ended) {}

    /** {@code put} is the queue's {@code put}, bound once here — not per item. */
    private record Queue<T>(Subscription<T> queue, @Nullable Predicate<T> where, Consumer<T> put) {}

    private final DepzDevice.Core core;
    private final int reportId; // what next() names in its timeout; NO_REPORT = no next()
    private final int defaultCapacity;
    // Copy-on-write snapshots, replaced under `lock` by on / subscribe / their closing: emit() (the reader
    // thread, at frame rate) walks the current array without a lock, a copy or a lambda, so an item costs
    // the subscribers nothing on the heap (bug hunt 2026-11-03 KNOWN-1, carried over from C#).
    private final Object lock = new Object();

    @SuppressWarnings("unchecked")
    private volatile Callback<T>[] callbacks = (Callback<T>[]) new Callback<?>[0];

    @SuppressWarnings("unchecked")
    private volatile Queue<T>[] queues = (Queue<T>[]) new Queue<?>[0];

    private final AtomicLong parseErrors = new AtomicLong();
    private boolean closed; // guarded by lock: set by closeAll, never cleared
    private @Nullable DepzException end; // guarded by lock: why the stream ended, for a queue subscribed later

    SensorStream(DepzDevice.Core core, int reportId, int defaultCapacity) {
        this.core = core;
        this.reportId = reportId;
        this.defaultCapacity = defaultCapacity;
    }

    // ── consumer side ────────────────────────────────────────────────────────

    /** Call {@code callback} for every item (reader thread). Close the result to stop; closing it twice is a no-op. */
    public Unsubscribe on(Consumer<T> callback) {
        return on(callback, null);
    }

    /** Call {@code callback} for every item {@code where} accepts ({@code null}: every item). */
    public Unsubscribe on(Consumer<T> callback, @Nullable Predicate<T> where) {
        return on(callback, where, null);
    }

    /**
     * {@link #on(Consumer, Predicate)}, and {@code onEnd} once when the stream ends: with the loss (as
     * {@link DepzDevice#lost}, {@code getCause()} is what the link threw) when the device was lost,
     * with {@code null} after the program's own {@code close()}. It runs on the reader thread after the
     * stream's last item, or at once on the calling thread when the stream has already ended; it never
     * starts once the returned handle was closed, though one the reader had already begun may still be
     * running when {@code close()} returns. Guarded like the callback: what it throws is counted in
     * {@link LinkStats#callbackErrors()}, logged and dropped. The end a Kotlin {@code callbackFlow}
     * needs to complete, or to fail with the loss.
     */
    public Unsubscribe on(Consumer<T> callback, @Nullable Predicate<T> where,
            @Nullable Consumer<@Nullable DeviceLostException> onEnd) {
        if (callback == null) {
            throw new NullPointerException("callback");
        }
        Callback<T> entry = new Callback<>(callback, where, onEnd, new AtomicBoolean());
        boolean ended;
        DepzException why;
        synchronized (lock) {
            callbacks = with(callbacks, entry);
            ended = closed;
            why = end;
        }
        if (ended) {
            end(entry, why);
        }
        AtomicBoolean done = new AtomicBoolean();
        return () -> {
            if (done.compareAndSet(false, true)) {
                entry.ended().set(true);
                synchronized (lock) {
                    callbacks = without(callbacks, entry);
                }
            }
        };
    }

    /** A pull queue of the items (bounded, drop-oldest) at the stream's default capacity; subscribed when this returns. */
    public Subscription<T> subscribe() {
        return subscribe(defaultCapacity, null);
    }

    /** A pull queue of up to {@code capacity} items (bounded, drop-oldest); subscribed when this returns. */
    public Subscription<T> subscribe(int capacity) {
        return subscribe(capacity, null);
    }

    /** A pull queue of the items {@code where} accepts ({@code null}: every item). */
    public Subscription<T> subscribe(int capacity, @Nullable Predicate<T> where) {
        Subscription<T> sub = new Subscription<>(capacity, s -> {
            synchronized (lock) {
                for (Queue<T> e : queues) {
                    if (e.queue() == s) {
                        queues = without(queues, e);
                        break;
                    }
                }
            }
        });
        synchronized (lock) {
            if (closed) {
                // The device already ended: a closed queue, so take() fails at once (contract 07 §3).
                sub.markClosed(end);
            } else {
                queues = with(queues, new Queue<>(sub, where, sub::put));
            }
        }
        return sub;
    }

    /** The next item, waiting up to {@link #DEFAULT_NEXT_TIMEOUT}. */
    public T next() {
        return next(DEFAULT_NEXT_TIMEOUT);
    }

    /**
     * The next item on a one-slot queue of its own, subscribed for this call only (contract 07 §3):
     * {@link DepzTimeoutException} naming the stream's report on expiry; as soon as the device ends,
     * before or while waiting, {@link LinkClosedException} after the program's own {@code close()} and
     * {@link DeviceLostException} carrying the cause after a loss (an item already queued still wins).
     * The {@code events} stream offers no {@code next()} (no single report to wait for): there the call
     * throws {@link InvalidStateException} before anything is subscribed.
     */
    public T next(@Nullable Duration timeout) {
        if (reportId == NO_REPORT) {
            throw new InvalidStateException("the events stream offers no next(): no single report to wait for "
                    + "(contract 07 §3) — subscribe with events().on(cb) or events().subscribe()");
        }
        Duration wait = timeout == null ? DEFAULT_NEXT_TIMEOUT : timeout;
        if (core.isClosed()) {
            throw core.endError("device is closed");
        }
        try (Subscription<T> sub = subscribe(1, null)) {
            return sub.take(wait);
        } catch (DepzTimeoutException e) {
            throw new DepzTimeoutException(reportId, wait);
        } catch (LinkClosedException e) {
            throw new LinkClosedException("device closed while waiting for a stream item");
        }
    }

    /** The drop counter of each open {@link #subscribe subscription} (a closed one leaves the list). */
    public List<Long> droppedCounts() {
        List<Long> out = new ArrayList<>();
        for (Queue<T> q : queues) {
            out.add(q.queue().droppedCount());
        }
        return out;
    }

    /**
     * Reports of this stream dropped because they did not decode (contract 07 §2): one shorter than
     * its header, or a value the decoder refuses. Monotonic over the device's life, except where a
     * sensor contract resets it (TMF8829 {@code start()}).
     */
    public long parseErrors() {
        return parseErrors.get();
    }

    // ── owner side (the device, reader thread) ───────────────────────────────

    /**
     * Hand {@code item} to every callback, then to every queue, each {@code where} applied (contract
     * 07 §3). A callback (or a predicate) that throws is counted in {@link LinkStats#callbackErrors()},
     * logged and dropped — it never stops the reader, the other callbacks or the queues. Allocation-free
     * per item: the subscriber arrays are read as they are and the guard takes the callback and its
     * predicate as arguments.
     */
    void emit(T item) {
        Callback<T>[] cbs = callbacks;
        for (Callback<T> c : cbs) {
            if (!CallbackGuard.call(c.cb(), c.where(), item)) {
                core.stats.callbackErrors.incrementAndGet();
            }
        }
        Queue<T>[] qs = queues;
        for (Queue<T> q : qs) {
            if (!CallbackGuard.call(q.put(), q.where(), item)) {
                core.stats.callbackErrors.incrementAndGet();
            }
        }
    }

    private static <E> E[] with(E[] arr, E entry) {
        E[] out = Arrays.copyOf(arr, arr.length + 1);
        out[arr.length] = entry;
        return out;
    }

    private static <E> E[] without(E[] arr, E entry) {
        for (int i = 0; i < arr.length; i++) {
            if (entry.equals(arr[i])) {
                E[] out = Arrays.copyOf(arr, arr.length - 1);
                System.arraycopy(arr, i + 1, out, i, arr.length - i - 1);
                return out;
            }
        }
        return arr;
    }

    /** A report of this stream that did not decode: dropped and counted (contract 07 §2). */
    void countParseError() {
        parseErrors.incrementAndGet();
    }

    /** Where a sensor contract restarts the count (TMF8829 {@code start()}). */
    void resetParseErrors() {
        parseErrors.set(0);
    }

    /**
     * Close every queue, and every one subscribed from now on, for the reason {@code why}; then tell
     * every callback's {@code onEnd} (and every one subscribed from now on).
     */
    void closeAll(DepzException why) {
        Queue<T>[] qs;
        Callback<T>[] cbs;
        synchronized (lock) {
            closed = true;
            end = why;
            qs = queues;
            cbs = callbacks;
        }
        for (Queue<T> q : qs) {
            q.queue().markClosed(why);
        }
        for (Callback<T> c : cbs) {
            end(c, why);
        }
    }

    /** Run {@code c}'s {@code onEnd} once, guarded: the loss, or null after the program's own close. */
    private void end(Callback<T> c, @Nullable DepzException why) {
        Consumer<@Nullable DeviceLostException> onEnd = c.onEnd();
        if (onEnd == null || !c.ended().compareAndSet(false, true)) {
            return;
        }
        DeviceLostException loss = why instanceof DeviceLostException d ? d : null;
        if (!CallbackGuard.call(onEnd, loss)) {
            core.stats.callbackErrors.incrementAndGet();
        }
    }
}
