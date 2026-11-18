package ai.depz.sensor.devices;

import ai.depz.sensor.sensors.vl53l8.Vl53l8Driver;
import java.time.Duration;
import org.jspecify.annotations.Nullable;

/** VL53L5CX ToF (8×8 zones, 63° field of view): the board, firmware and API of {@link Vl53l7cxDevice}; the sensor reports module type MZ. */
public class Vl53l5cxDevice extends Vl53l7cxDevice {
    public Vl53l5cxDevice(Link link, @Nullable Duration timeout) {
        super(link, timeout);
    }

    public Vl53l5cxDevice(Link link) {
        super(link);
    }

    Vl53l5cxDevice(DepzDevice from) {
        super(from);
    }

    @Override
    protected int expectedModuleType() {
        return Vl53l8Driver.MODULE_TYPE_MZ;
    }
}
