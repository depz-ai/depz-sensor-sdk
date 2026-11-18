package ai.depz.sensor.devices;

import org.jspecify.annotations.Nullable;

/** The connection went away: unplugged, closed, or the OS refused I/O. */
public class DeviceLostException extends DepzException {
    public DeviceLostException(@Nullable String message) {
        super(message);
    }

    public DeviceLostException(@Nullable String message, @Nullable Throwable cause) {
        super(message, cause);
    }
}
