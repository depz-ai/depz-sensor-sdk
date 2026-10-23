package ai.depz.sensor.sensors.vl53lx;

/**
 * ST's platform layer (RdByte / WrWord / WaitMs …) for the 1D family, plus what the bridge owns: bus
 * speed, register-address width and the XSHUT line. Register values are big-endian, as the C drivers
 * assume. {@code Vl53lxDevice} implements it over the board; a test can script it.
 */
public abstract class Vl53lxPlatform {
    public abstract byte[] readMulti(int addr, int size);

    public abstract void writeMulti(int addr, byte[] data);

    /** The bridge's bus timing (not part of the C platform layer). */
    public abstract void setI2cSpeed(int khz);

    /** How wide a register address the bridge puts on the wire; sticky. */
    public abstract void setAddrWidth(int width);

    /** Reset through XSHUT, for a sensor that no longer acknowledges writes. */
    public abstract void xshutReset();

    public void sleepMs(int ms) {
        try {
            Thread.sleep(ms);
        } catch (InterruptedException e) {
            Thread.currentThread().interrupt();
            throw new Vl53lxException("interrupted");
        }
    }

    public int readByte(int addr) {
        return readMulti(addr, 1)[0] & 0xFF;
    }

    public int readWord(int addr) {
        byte[] b = readMulti(addr, 2);
        return (b[0] & 0xFF) << 8 | (b[1] & 0xFF);
    }

    public long readDword(int addr) {
        byte[] b = readMulti(addr, 4);
        return (b[0] & 0xFFL) << 24 | (b[1] & 0xFFL) << 16 | (b[2] & 0xFFL) << 8 | (b[3] & 0xFFL);
    }

    public void writeByte(int addr, int value) {
        writeMulti(addr, new byte[] {(byte) value});
    }

    public void writeWord(int addr, int value) {
        writeMulti(addr, new byte[] {(byte) (value >> 8), (byte) value});
    }

    public void writeDword(int addr, long value) {
        writeMulti(addr, new byte[] {(byte) (value >> 24), (byte) (value >> 16), (byte) (value >> 8), (byte) value});
    }
}
