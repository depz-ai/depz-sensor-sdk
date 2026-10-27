package ai.depz.sensor.devices;

import java.time.Duration;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.function.BooleanSupplier;
import java.util.function.Consumer;
import java.util.logging.Level;
import java.util.logging.Logger;

/** The subscribers of one stream: callbacks and pull queues. */
final class Fanout<T> {
    private static final Logger LOG = Logger.getLogger("ai.depz.sensor");

    private final List<Consumer<T>> callbacks = new ArrayList<>();
    private final List<Subscription<T>> queues = new ArrayList<>();

    AutoCloseable onItem(Consumer<T> callback) {
        if (callback == null) {
            throw new NullPointerException("callback");
        }
        synchronized (callbacks) {
            callbacks.add(callback);
        }
        AtomicBoolean done = new AtomicBoolean();
        return () -> {
            if (done.compareAndSet(false, true)) {
                synchronized (callbacks) {
                    callbacks.remove(callback);
                }
            }
        };
    }

    Subscription<T> subscribe(int capacity) {
        Subscription<T> sub = new Subscription<>(capacity, s -> {
            synchronized (queues) {
                queues.remove(s);
            }
        });
        synchronized (queues) {
            queues.add(sub);
        }
        return sub;
    }

    List<Long> droppedCounts() {
        synchronized (queues) {
            List<Long> out = new ArrayList<>();
            for (Subscription<T> q : queues) {
                out.add(q.droppedCount());
            }
            return out;
        }
    }

    /**
     * Hand {@code item} to every callback, then to every queue (contract 07 §3); a callback that
     * throws counts in {@code stats}.
     */
    void emit(T item, LinkStats stats) {
        List<Consumer<T>> cbs;
        synchronized (callbacks) {
            cbs = new ArrayList<>(callbacks);
        }
        for (Consumer<T> cb : cbs) {
            call(cb, item, stats);
        }
        List<Subscription<T>> qs;
        synchronized (queues) {
            qs = new ArrayList<>(queues);
        }
        for (Subscription<T> q : qs) {
            q.put(item);
        }
    }

    /**
     * Run a user callback on the reader thread: one that throws is counted in {@link
     * LinkStats#callbackErrors()}, logged and dropped — it never stops the reader, the other
     * callbacks or the queues (contract 07 §3).
     */
    static <T> void call(Consumer<T> cb, T item, LinkStats stats) {
        try {
            cb.accept(item);
        } catch (RuntimeException | Error e) {
            if (e instanceof VirtualMachineError vme) {
                throw vme;
            }
            stats.callbackErrors.incrementAndGet();
            LOG.log(Level.SEVERE, "a callback threw on the reader thread; dropped", e);
        }
    }

    /**
     * The next item on a one-slot queue of its own (contract 07 §3 {@code get_measurement}):
     * {@link DepzTimeoutException} naming {@code reportId} on expiry, a closed error as soon as
     * the device closes (an item already queued still wins).
     */
    T next(Duration timeout, int reportId, BooleanSupplier deviceClosed) {
        if (deviceClosed.getAsBoolean()) {
            throw new LinkClosedException("device is closed");
        }
        try (Subscription<T> sub = subscribe(1)) {
            return sub.take(timeout);
        } catch (DepzTimeoutException e) {
            throw new DepzTimeoutException(reportId, timeout);
        } catch (DeviceLostException e) {
            throw new LinkClosedException("device closed while waiting for a stream item");
        }
    }

    void closeAll() {
        List<Subscription<T>> qs;
        synchronized (queues) {
            qs = new ArrayList<>(queues);
        }
        for (Subscription<T> q : qs) {
            q.markClosed();
        }
    }
}
