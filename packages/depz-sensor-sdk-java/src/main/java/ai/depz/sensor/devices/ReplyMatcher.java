package ai.depz.sensor.devices;

import ai.depz.sensor.transport.Packet;
import org.jspecify.annotations.Nullable;

/** Claims a packet as the reply to a request: its decoded value, or {@code null} for "not mine". */
@FunctionalInterface
public interface ReplyMatcher<T> {
    @Nullable T match(Packet packet);
}
