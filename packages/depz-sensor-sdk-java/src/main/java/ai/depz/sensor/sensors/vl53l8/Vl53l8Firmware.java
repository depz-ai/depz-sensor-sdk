package ai.depz.sensor.sensors.vl53l8;

/** The sensor firmware a {@link Vl53l8Driver} downloads. */
public enum Vl53l8Firmware {
    /** VL53L8CX, ST ULD 2.1.0. */
    CX("cx"),
    /** VL53L8CH, VL53LMZ ULD 2.0.16 (CNH). */
    CH("ch"),
    /** VL53L5CX / VL53L7CX over I2C, ULD 2.0.1. */
    L7CX("l7cx"),
    /** VL53L7CH over I2C, VL53LMZ ULD 2.0.16 with the VL53L7 configuration. */
    L7CH("l7ch");

    /** The blob set name under {@code vl53l8/data}. */
    public final String setName;

    Vl53l8Firmware(String setName) {
        this.setName = setName;
    }
}
