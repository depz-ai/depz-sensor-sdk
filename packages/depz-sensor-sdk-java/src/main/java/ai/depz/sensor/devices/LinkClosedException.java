package ai.depz.sensor.devices;

import org.jspecify.annotations.Nullable;

/** I/O on a link this side already closed (contract 07 §3 {@code closed}; not a lost device). */
public final class LinkClosedException extends DepzException {
    public LinkClosedException(@Nullable String message) {
        super(message);
    }
}
