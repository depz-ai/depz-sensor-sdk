package ai.depz.sensor.devices;

import ai.depz.sensor.transport.Packet;

/** Claims a packet as the reply to a request: its decoded value, or {@code null} for "not mine". */
@FunctionalInterface
public interface ReplyMatcher<T> {
    T match(Packet packet);
}
