package ai.depz.sensor.devices;

/** A {@link ReplayLink} in strict mode saw the SDK write bytes the capture does not hold. */
public final class ReplayMismatchException extends DepzException {
    public ReplayMismatchException(String message) {
        super(message);
    }
}
