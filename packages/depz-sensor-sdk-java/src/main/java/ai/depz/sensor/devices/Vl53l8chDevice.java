package ai.depz.sensor.devices;

import ai.depz.sensor.sensors.vl53l8.Vl53l8CnhSetup;
import ai.depz.sensor.sensors.vl53l8.Vl53l8Firmware;
import ai.depz.sensor.sensors.vl53l8.Vl53l8Frame;
import java.time.Duration;

/**
 * VL53L8CH: the VL53L8CX superset (every CX method) plus CNH, compact network histograms — each
 * frame can carry a per-aggregate distance histogram. {@link #init()} downloads the CH firmware
 * (VL53LMZ ULD 2.0.16).
 */
public class Vl53l8chDevice extends Vl53l8cxDevice {
    public Vl53l8chDevice(Link link, Duration timeout) {
        super(link, timeout);
    }

    public Vl53l8chDevice(Link link) {
        super(link);
    }

    public Vl53l8chDevice(String port, Duration timeout) {
        super(port, timeout);
    }

    Vl53l8chDevice(DepzDevice from) {
        super(from);
    }

    @Override
    protected Vl53l8Firmware firmwareSet() {
        return Vl53l8Firmware.CH;
    }

    /** The CNH setup armed by {@link #configureCnh}, or null; recorders keep it next to the raw blocks to decode them later. */
    public Vl53l8CnhSetup cnhSetup() {
        return armedCnh();
    }

    /**
     * Arm CNH for the next {@link #startRanging()}: every frame then carries
     * {@link Vl53l8Frame#cnhRaw()}, decoded with {@code Vl53l8Uld.decodeCnh}.
     */
    public void configureCnh(Vl53l8CnhSetup setup) {
        armCnh(setup);
    }
}
