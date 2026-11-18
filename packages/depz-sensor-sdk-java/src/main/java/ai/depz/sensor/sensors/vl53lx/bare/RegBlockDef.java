package ai.depz.sensor.sensors.vl53lx.bare;

import java.util.List;

/** One contiguous I2C block of the device image (a {@code VL53LX_*_t} struct of the C driver). */
public record RegBlockDef(int base, int size, boolean writable, List<RegField> fields) {}
