package ai.depz.sensor.devices;

/** The opened device is not the sensor class the caller asked for. */
public final class WrongTypeException extends DepzException {
    public WrongTypeException(String message) {
        super(message);
    }
}
