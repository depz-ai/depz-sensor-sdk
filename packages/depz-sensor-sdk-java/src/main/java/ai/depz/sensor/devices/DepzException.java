package ai.depz.sensor.devices;

/**
 * Base of every live-layer error (contract 07). Unchecked. Arguments the SDK refuses before
 * sending anything throw {@link IllegalArgumentException} instead.
 */
public class DepzException extends RuntimeException {
    public DepzException(String message) {
        super(message);
    }

    public DepzException(String message, Throwable cause) {
        super(message, cause);
    }
}
