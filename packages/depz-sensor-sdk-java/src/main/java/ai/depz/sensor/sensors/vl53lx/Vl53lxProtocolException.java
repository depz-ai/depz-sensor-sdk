package ai.depz.sensor.sensors.vl53lx;

import ai.depz.sensor.devices.DepzException;

/**
 * The bridge refused a register command or did not answer it. The drivers catch it where a sensor
 * legitimately NACKs for a while (right after a soft reset).
 */
public final class Vl53lxProtocolException extends DepzException {
    public Vl53lxProtocolException(String message) {
        super(message);
    }

    public Vl53lxProtocolException(String message, Throwable cause) {
        super(message, cause);
    }
}
