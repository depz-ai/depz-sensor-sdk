package ai.depz.sensor.devices;

import ai.depz.sensor.sensors.vl53l8.Vl53l8CnhSetup;
import ai.depz.sensor.sensors.vl53l8.Vl53l8Firmware;
import java.time.Duration;

/**
 * VL53L7CH: the VL53L7CX superset with CNH histograms. {@link #init()} downloads the CH firmware
 * (VL53LMZ ULD 2.0.16, the VL53L8CH one); CNH frames up to ~7.6 KB stream in chunks.
 */
public class Vl53l7chDevice extends Vl53l7cxDevice {
    public Vl53l7chDevice(Link link, Duration timeout) {
        super(link, timeout);
    }

    public Vl53l7chDevice(Link link) {
        super(link);
    }

    public Vl53l7chDevice(String port, Duration timeout) {
        super(port, timeout);
    }

    Vl53l7chDevice(DepzDevice from) {
        super(from);
    }

    @Override
    protected Vl53l8Firmware firmwareSet() {
        return Vl53l8Firmware.L7CH;
    }

    /** The CNH setup armed by {@link #configureCnh}, or null. */
    public Vl53l8CnhSetup cnhSetup() {
        return armedCnh();
    }

    /** Arm CNH for the next start (as {@link Vl53l8chDevice#configureCnh}). */
    public void configureCnh(Vl53l8CnhSetup setup) {
        armCnh(setup);
    }
}
