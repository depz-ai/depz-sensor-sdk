package ai.depz.sensor.devices;

import java.time.Duration;
import java.util.ArrayDeque;
import java.util.ArrayList;
import java.util.List;
import java.util.Locale;
import java.util.Optional;
import java.util.function.Consumer;

/**
 * A pull subscription to one stream (contract 07 §3): a bounded queue that drops the oldest
 * item when full and counts the drops. It is registered when created, so nothing produced after
 * that is missed. Poll it with {@link #tryTake} (never blocks) or wait with {@link #take};
 * {@link #close} unsubscribes.
 */
public final class Subscription<T> implements AutoCloseable {
    private final ArrayDeque<T> q = new ArrayDeque<>();
    private final int capacity;
    private final Consumer<Subscription<T>> unregister;
    private long droppedCount;
    private boolean closed;

    Subscription(int capacity, Consumer<Subscription<T>> unregister) {
        if (capacity < 1) {
            throw new IllegalArgumentException("capacity must be at least 1, got " + capacity);
        }
        this.capacity = capacity;
        this.unregister = unregister;
    }

    /** Items dropped because the queue was full (monotonic). */
    public synchronized long droppedCount() {
        return droppedCount;
    }

    /** Items waiting. */
    public synchronized int size() {
        return q.size();
    }

    /** The device closed (or this was closed): no more items will arrive. */
    public synchronized boolean isClosed() {
        return closed;
    }

    /** The oldest waiting item, without blocking. */
    public synchronized Optional<T> tryTake() {
        return Optional.ofNullable(q.pollFirst());
    }

    /**
     * The oldest item, waiting up to {@code timeout} for one.
     *
     * @throws DepzTimeoutException nothing arrived in time
     * @throws DeviceLostException the device closed with the queue empty
     */
    public synchronized T take(Duration timeout) {
        long deadline = System.nanoTime() + timeout.toNanos();
        while (q.isEmpty()) {
            if (closed) {
                throw new DeviceLostException("stream closed: the device is closed");
            }
            long left = deadline - System.nanoTime();
            if (left <= 0) {
                throw new DepzTimeoutException(String.format(Locale.ROOT,
                        "no stream item within %.3fs", timeout.toNanos() / 1e9));
            }
            try {
                wait(Math.max(1, left / 1_000_000), (int) (left % 1_000_000));
            } catch (InterruptedException e) {
                Thread.currentThread().interrupt();
                throw new DepzTimeoutException("interrupted while waiting for a stream item");
            }
        }
        return q.pollFirst();
    }

    /** Everything waiting now, oldest first, without blocking. */
    public synchronized List<T> drain() {
        List<T> all = new ArrayList<>(q);
        q.clear();
        return all;
    }

    synchronized void put(T item) {
        if (closed) {
            return;
        }
        if (q.size() >= capacity) {
            q.pollFirst();
            droppedCount++;
        }
        q.addLast(item);
        notifyAll();
    }

    synchronized void markClosed() {
        closed = true;
        notifyAll();
    }

    @Override
    public void close() {
        markClosed();
        unregister.accept(this);
    }
}
