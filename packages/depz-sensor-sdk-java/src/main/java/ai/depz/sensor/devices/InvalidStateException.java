package ai.depz.sensor.devices;

/**
 * The call is not valid in the current state of the device or object (contract 07 §3):
 * {@code toHostTimeUs} before {@code syncTime}, a sensor call before {@code init()} /
 * {@code configure()} or while streaming, {@code add()} after recording started, a closed
 * dataset writer or player, {@code await} without the player thread, a write to a read-only
 * register block, a VL53Lx board name without a product number, a packaged firmware blob of
 * the wrong size or missing from the classpath.
 */
public final class InvalidStateException extends DepzException {
    public InvalidStateException(String message) {
        super(message);
    }
}
