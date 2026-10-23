package ai.depz.sensor.devices;

import java.time.Duration;

/** VL53L1CB (8 m, cover-glass module): the VL53L1CX die and drivers; usually needs a crosstalk calibration. */
public final class Vl53l1cbDevice extends Vl53lxDevice {
    public Vl53l1cbDevice(Link link, Duration timeout) {
        super(link, timeout);
    }

    public Vl53l1cbDevice(Link link) {
        super(link);
    }

    public Vl53l1cbDevice(String port, Duration timeout) {
        super(port, timeout);
    }

    Vl53l1cbDevice(DepzDevice from) {
        super(from);
    }

    @Override
    protected String fixedProduct() {
        return "VL53L1CB";
    }
}
