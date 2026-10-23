package ai.depz.sensor.devices;

import java.time.Duration;

/** VL53L1CX (4 m): the VL53L1X ULD (short / long) or the histogram driver. */
public final class Vl53l1cxDevice extends Vl53lxDevice {
    public Vl53l1cxDevice(Link link, Duration timeout) {
        super(link, timeout);
    }

    public Vl53l1cxDevice(Link link) {
        super(link);
    }

    public Vl53l1cxDevice(String port, Duration timeout) {
        super(port, timeout);
    }

    Vl53l1cxDevice(DepzDevice from) {
        super(from);
    }

    @Override
    protected String fixedProduct() {
        return "VL53L1CX";
    }
}
