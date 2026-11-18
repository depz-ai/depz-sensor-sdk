package ai.depz.sensor.devices;

import ai.depz.sensor.internal.Waits;
import java.time.Duration;
import java.util.ArrayDeque;
import java.util.ArrayList;
import java.util.List;
import java.util.Locale;
import java.util.Optional;
import java.util.function.Consumer;
import org.jspecify.annotations.Nullable;

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
    private @Nullable DepzException end; // why it closed: a DeviceLostException after a loss; null = this side closed it

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
     * @throws LinkClosedException the queue is empty and the program closed the device or this subscription
     *     (contract 07 §3)
     * @throws DeviceLostException the queue is empty and the device was lost; carries the cause
     * @throws DepzInterruptedException the thread was interrupted (its interrupt flag stays set)
     */
    public synchronized T take(Duration timeout) {
        long deadline = Waits.deadline(timeout);
        while (q.isEmpty()) {
            if (closed) {
                throw endError();
            }
            long left = Waits.left(deadline);
            if (left <= 0) {
                throw new DepzTimeoutException(String.format(Locale.ROOT,
                        "no stream item within %.3fs", Waits.nanos(timeout) / 1e9));
            }
            try {
                wait(Math.max(1, left / 1_000_000), (int) (left % 1_000_000));
            } catch (InterruptedException e) {
                Thread.currentThread().interrupt();
                throw new DepzInterruptedException(e);
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

    /** A fresh error of the kind the queue ended with (the stack belongs to the caller's wait). */
    private DepzException endError() {
        if (end instanceof DeviceLostException lost) {
            return new DeviceLostException(lost.getMessage(), lost.getCause());
        }
        return new LinkClosedException(end != null ? end.getMessage() : "stream closed: this subscription is closed");
    }

    /** No more items, for the reason {@code why} (null: this side closed it); the first reason stays. */
    synchronized void markClosed(@Nullable DepzException why) {
        if (!closed) {
            closed = true;
            end = why;
        }
        notifyAll();
    }

    @Override
    public void close() {
        markClosed(null);
        unregister.accept(this);
    }
}
