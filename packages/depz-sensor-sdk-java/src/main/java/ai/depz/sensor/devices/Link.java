package ai.depz.sensor.devices;

import java.time.Duration;

/**
 * A blocking byte pipe to one device: a serial port, a {@code .depzrec} record/replay wrapper,
 * or anything a host supplies. Device classes accept any link, which keeps every layer above the
 * port testable without hardware.
 *
 * <p><b>How the SDK calls a link</b> (contract 07 §3, "The host-supplied link") — what a link
 * given to {@code DepzDevices.openOnLink} or a device constructor has to cope with:
 * <ul>
 * <li><b>{@code read}</b> runs only on the device's own daemon thread
 * {@code depz-reader-<name()>}, one call at a time, back to back, in short slices. The
 * {@code timeout} is 50 ms today; its length is the SDK's choice and may change, so a link must
 * not depend on it.
 * <li><b>{@code write}</b> runs on the thread of the call that sends (any program thread; the
 * reader thread when a callback sends), concurrently with a blocked {@code read}. The SDK
 * serializes the writes of a device: one at a time, one whole frame per call (payload up to
 * {@code Framing.MAX_PAYLOAD} bytes, plus the frame's header).
 * <li><b>{@code write} writes everything or throws, within a bounded time.</b> The SDK looks at
 * no count, does not retry and passes no timeout: the link picks one. A {@code write} that never
 * returns holds the device's write lock, so every later request of that device blocks too,
 * whatever its timeout.
 * <li><b>{@code close}</b> runs on the thread that closes the device, possibly during a blocked
 * {@code read} or a {@code write}, and after a loss on the reader thread and then again from the
 * program's own {@code close()}: it must be idempotent. It must make a blocked {@code read}
 * return or throw promptly, and so must a {@code read} that starts just after it. The device
 * waits up to 2 s for the reader, then returns anyway and leaves that thread in the
 * {@code read}.
 * <li><b>{@code isClosed} and {@code name}</b> are called from any thread.
 * <li><b>What {@code read} throws is a loss</b> unless the program is closing the device: the
 * device closes, every later request fails with {@code DeviceLostException}. Meant for that:
 * {@code DeviceLostException}, {@code LinkClosedException}, {@code UncheckedIOException},
 * {@code IllegalStateException}. Any other {@code RuntimeException} is a loss too, logged at
 * {@code SEVERE} as an SDK bug. <b>An {@code Error} must not escape {@code read}:</b> the device
 * is closed, then the reader rethrows it, and an uncaught throwable on any thread ends an Android
 * app's process.
 * <li><b>What {@code write} throws</b> fails only that call: the device ends when {@code read}
 * reports the loss. {@code LinkClosedException} while the program has not closed the device
 * reaches the caller as {@code DeviceLostException}; anything else as it is.
 * <li><b>What {@code close} throws</b> reaches the program from its own {@code close()} (the
 * device still ends); on the reader's path it is logged and dropped.
 * </ul>
 */
public interface Link extends AutoCloseable {
    /**
     * The next available chunk (one byte or more), or an empty array when nothing arrived within
     * {@code timeout}.
     *
     * @throws LinkClosedException the link is closed
     * @throws DeviceLostException the device is gone
     */
    byte[] read(Duration timeout);

    /**
     * Writes {@code data} to the board.
     *
     * @throws LinkClosedException the link is closed
     * @throws DeviceLostException the device is gone
     */
    void write(byte[] data);

    /**
     * Closes the link; idempotent, throws no checked exception, so {@code try (Link link = ...)} needs
     * no catch. A link handed to a device belongs to the device from then on: closing the device closes
     * it.
     */
    @Override
    void close();

    boolean isClosed();

    /** A name for messages: the port path, or what the link wraps. */
    String name();
}
