package ai.depz.sensor.devices;

/** I/O on a link that is already closed. */
public final class LinkClosedException extends DeviceLostException {
    public LinkClosedException(String message) {
        super(message);
    }
}
