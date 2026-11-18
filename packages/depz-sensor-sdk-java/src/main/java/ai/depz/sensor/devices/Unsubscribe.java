package ai.depz.sensor.devices;

/**
 * What {@link SensorStream#on} and the {@code DatasetPlayer} subscriptions return: close it to stop
 * the callback. {@code close()} throws nothing
 * and closing twice is a no-op, so {@code try (Unsubscribe h = stream.on(cb)) { ... }} needs no catch.
 */
@FunctionalInterface
public interface Unsubscribe extends AutoCloseable {
    @Override
    void close();
}
