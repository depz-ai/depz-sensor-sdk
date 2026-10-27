package ai.depz.sensor.protocol;

/**
 * VL53L8 register-bridge command codecs (contracts/04_SENSOR_VL53L8.md): the SPI bridge of the
 * VL53L8CX / CH boards, which the L5 / L7 I2C boards speak too (contract 11). RPT_VL53_FRAME
 * chunks are reassembled by {@code sensors.vl53l8.FrameReassembler}.
 */
public final class Vl53l8 {
    private Vl53l8() {}

    /** Host→device opcodes. 0x34 is intentionally unused (a gap in the firmware's sequence). */
    public enum Vl53l8Cmd {
        READ_REG(0x32),
        WRITE_REG(0x33),
        START_STREAM(0x35),
        STOP_STREAM(0x36);

        public final int value;

        Vl53l8Cmd(int value) {
            this.value = value;
        }
    }

    /** Device→host report ids. */
    public enum Vl53l8Rpt {
        REG_DATA(0x91),
        VL53_FRAME(0x93);

        public final int value;

        Vl53l8Rpt(int value) {
            this.value = value;
        }
    }

    /** Register bytes one READ_REG / WRITE_REG moves at most (the bridge's limit). */
    public static final int CHUNK_SIZE = 2048;

    /** Bytes of frame data per RPT_VL53_FRAME chunk. */
    public static final int STREAM_CHUNK_MAX = 1528;

    /** Max frame_size accepted by START_STREAM. */
    public static final int STREAM_TOTAL_MAX = 8192;

    /** VL53_READ_REG payload: addr u16 LE, length u16 LE. */
    public static byte[] packReadReg(int addr, int length) {
        Wire.checkUint("addr", addr, 16);
        Wire.checkUint("length", length, 16);
        return new byte[] {(byte) addr, (byte) (addr >> 8), (byte) length, (byte) (length >> 8)};
    }

    /** VL53_WRITE_REG payload: addr u16 LE, then at least one data byte (contract 04 §2). */
    public static byte[] packWriteReg(int addr, byte[] data, int offset, int length) {
        Wire.checkUint("addr", addr, 16);
        if (length == 0) {
            throw new IllegalArgumentException("WRITE_REG data must be at least 1 byte");
        }
        byte[] b = new byte[2 + length];
        b[0] = (byte) addr;
        b[1] = (byte) (addr >> 8);
        System.arraycopy(data, offset, b, 2, length);
        return b;
    }

    public static byte[] packWriteReg(int addr, byte[] data) {
        return packWriteReg(addr, data, 0, data.length);
    }

    /** VL53_START_STREAM payload: the frame size the board reads per INT, u16 LE. */
    public static byte[] packStartStream(int frameSize) {
        Wire.checkUint("frameSize", frameSize, 16);
        // contract 04 §4 rule 4: the firmware refuses 0 and > 8192; so does the host
        if (frameSize < 1 || frameSize > STREAM_TOTAL_MAX) {
            throw new IllegalArgumentException("frameSize: " + frameSize + " outside 1.." + STREAM_TOTAL_MAX);
        }
        return new byte[] {(byte) frameSize, (byte) (frameSize >> 8)};
    }

    /** RPT_VL53_REG_DATA: the echoed READ_REG opcode, the MCU timestamp and the register bytes. */
    public record RegData(int cmd, long timestampUs, byte[] data) {
        public static RegData unpack(byte[] p) {
            Wire.needBytes("RPT_VL53_REG_DATA", p, 9);
            return new RegData(p[0] & 0xFF, Common.u64le(p, 1), java.util.Arrays.copyOfRange(p, 9, p.length));
        }
    }
}
