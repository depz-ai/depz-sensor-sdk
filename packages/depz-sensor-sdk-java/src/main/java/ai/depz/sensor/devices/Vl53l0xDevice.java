package ai.depz.sensor.devices;

import java.time.Duration;
import org.jspecify.annotations.Nullable;

/** VL53L0X (2 m, 1-byte register addresses, 400 kHz): ULD API 1.0.4 with ranging profiles. */
public final class Vl53l0xDevice extends Vl53lxDevice {
    public Vl53l0xDevice(Link link, @Nullable Duration timeout) {
        super(link, timeout);
    }

    public Vl53l0xDevice(Link link) {
        super(link);
    }

    Vl53l0xDevice(DepzDevice from) {
        super(from);
    }

    @Override
    protected String fixedProduct() {
        return "VL53L0X";
    }
}
