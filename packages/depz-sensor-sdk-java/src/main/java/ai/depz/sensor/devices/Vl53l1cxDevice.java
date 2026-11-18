package ai.depz.sensor.devices;

import java.time.Duration;
import org.jspecify.annotations.Nullable;

/** VL53L1CX (4 m): the VL53L1X ULD (short / long) or the histogram driver. */
public final class Vl53l1cxDevice extends Vl53lxDevice {
    public Vl53l1cxDevice(Link link, @Nullable Duration timeout) {
        super(link, timeout);
    }

    public Vl53l1cxDevice(Link link) {
        super(link);
    }

    Vl53l1cxDevice(DepzDevice from) {
        super(from);
    }

    @Override
    protected String fixedProduct() {
        return "VL53L1CX";
    }
}
