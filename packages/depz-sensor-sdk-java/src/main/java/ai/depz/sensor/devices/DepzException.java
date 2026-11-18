package ai.depz.sensor.devices;

import org.jspecify.annotations.Nullable;

/**
 * Base of every live-layer error (contract 07). Unchecked. Arguments the SDK refuses before
 * sending anything throw {@link IllegalArgumentException} instead.
 */
public class DepzException extends RuntimeException {
    public DepzException(@Nullable String message) {
        super(message);
    }

    public DepzException(@Nullable String message, @Nullable Throwable cause) {
        super(message, cause);
    }
}
