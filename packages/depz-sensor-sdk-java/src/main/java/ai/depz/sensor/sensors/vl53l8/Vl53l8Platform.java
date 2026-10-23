package ai.depz.sensor.sensors.vl53l8;

/**
 * What the ULD needs from the board: register access through the bridge (16-bit addresses,
 * auto-increment) and a delay. {@code Vl53l8cxDevice} maps it onto the firmware bridge; a test can
 * script it.
 */
public interface Vl53l8Platform {
    byte[] readMulti(int addr, int size);

    void writeMulti(int addr, byte[] data);

    void sleepMs(int ms);
}
