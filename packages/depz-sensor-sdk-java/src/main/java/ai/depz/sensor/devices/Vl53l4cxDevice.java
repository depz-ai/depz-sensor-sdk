package ai.depz.sensor.devices;

import java.time.Duration;

/** VL53L4CX (6 m): the histogram driver only — to run it light, borrow a sibling's with {@code init("uld", "VL53L4CD")}. */
public final class Vl53l4cxDevice extends Vl53lxDevice {
    public Vl53l4cxDevice(Link link, Duration timeout) {
        super(link, timeout);
    }

    public Vl53l4cxDevice(Link link) {
        super(link);
    }

    public Vl53l4cxDevice(String port, Duration timeout) {
        super(port, timeout);
    }

    Vl53l4cxDevice(DepzDevice from) {
        super(from);
    }

    @Override
    protected String fixedProduct() {
        return "VL53L4CX";
    }
}
