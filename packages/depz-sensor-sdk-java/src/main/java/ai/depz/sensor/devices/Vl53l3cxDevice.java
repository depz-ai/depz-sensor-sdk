package ai.depz.sensor.devices;

import java.time.Duration;

/** VL53L3CX (3 m): ST's ULP driver (single target) or the histogram driver (several per frame). */
public final class Vl53l3cxDevice extends Vl53lxDevice {
    public Vl53l3cxDevice(Link link, Duration timeout) {
        super(link, timeout);
    }

    public Vl53l3cxDevice(Link link) {
        super(link);
    }

    public Vl53l3cxDevice(String port, Duration timeout) {
        super(port, timeout);
    }

    Vl53l3cxDevice(DepzDevice from) {
        super(from);
    }

    @Override
    protected String fixedProduct() {
        return "VL53L3CX";
    }
}
