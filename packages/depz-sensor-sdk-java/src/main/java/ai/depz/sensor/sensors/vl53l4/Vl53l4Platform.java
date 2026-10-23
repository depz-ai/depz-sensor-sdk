package ai.depz.sensor.sensors.vl53l4;

/**
 * What the ULD needs from the board: the register bridge (16-bit addresses, auto-increment), a
 * delay, and the bus speed. {@code Vl53l4cdDevice} maps it onto the firmware bridge; a test can
 * script it.
 */
public interface Vl53l4Platform {
    byte[] readMulti(int addr, int size);

    void writeMulti(int addr, byte[] data);

    void sleepMs(int ms);

    void setI2cSpeed(int khz);
}
