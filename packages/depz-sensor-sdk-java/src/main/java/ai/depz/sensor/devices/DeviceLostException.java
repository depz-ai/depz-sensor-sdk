package ai.depz.sensor.devices;

/** The connection went away: unplugged, closed, or the OS refused I/O. */
public class DeviceLostException extends DepzException {
    public DeviceLostException(String message) {
        super(message);
    }

    public DeviceLostException(String message, Throwable cause) {
        super(message, cause);
    }
}
