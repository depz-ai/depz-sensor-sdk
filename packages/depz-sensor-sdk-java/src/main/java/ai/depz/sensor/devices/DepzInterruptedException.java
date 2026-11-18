package ai.depz.sensor.devices;

/**
 * A wait of the SDK — a request, {@link Subscription#take}, a sensor class's own wait or sleep — ended
 * because its thread was interrupted. The thread's interrupt flag stays set. A {@code catch
 * (DepzException e)} still catches it; this type tells a cancellation (a Kotlin coroutine, an executor's
 * {@code cancel(true)}) from a failure without reading the message.
 */
public class DepzInterruptedException extends DepzException {
    /** @param cause the {@link InterruptedException} the wait caught */
    public DepzInterruptedException(InterruptedException cause) {
        super("interrupted", cause);
    }
}
