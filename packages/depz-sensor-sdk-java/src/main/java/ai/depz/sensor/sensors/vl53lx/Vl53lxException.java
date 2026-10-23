package ai.depz.sensor.sensors.vl53lx;

import ai.depz.sensor.devices.DepzException;

/**
 * A ULD-level failure: the sensor did not do what the driver needed (a timeout waiting for data
 * ready or boot, a calibration that failed, a mode the driver lacks…).
 */
public class Vl53lxException extends DepzException {
    public Vl53lxException(String message) {
        super(message);
    }
}
