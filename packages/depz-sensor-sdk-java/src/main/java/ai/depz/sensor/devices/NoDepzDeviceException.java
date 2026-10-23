package ai.depz.sensor.devices;

/** Discovery found no DEPZ device to open. */
public final class NoDepzDeviceException extends DepzException {
    public NoDepzDeviceException(String message) {
        super(message);
    }

    public NoDepzDeviceException(String message, Throwable cause) {
        super(message, cause);
    }
}
