package ai.depz.sensor.protocol;

/**
 * VL53L5CX / VL53L7CX / VL53L7CH I2C register-bridge wire codecs
 * (contracts/11_SENSOR_VL53L7.md, a delta against contracts/04_SENSOR_VL53L8.md).
 *
 * <p>Commands 0x32/0x33/0x35/0x36 and reports 0x91/0x93 are bit-for-bit the
 * VL53L8 bridge; RPT_VL53_FRAME (0x93) chunks are decoded and reassembled by
 * {@link ai.depz.sensor.sensors.vl53l8.FrameReassembler}. This class adds what
 * the I2C board brings: PIN_CTRL, GET_INFO, SET_I2C_SPEED, RPT_VL53_INFO and its
 * tighter transfer limits. The register encoders are included here because the
 * Java SDK has no separate VL53L8 protocol class (its VL53L8 support is
 * decode-only).
 */
public final class Vl53l7 {
    private Vl53l7() {}

    public enum Vl53l7Cmd {
        READ_REG(0x32),
        WRITE_REG(0x33),
        PIN_CTRL(0x34),
        START_STREAM(0x35),
        STOP_STREAM(0x36),
        GET_INFO(0x37),
        SET_I2C_SPEED(0x38);

        public final int value;
        Vl53l7Cmd(int value) { this.value = value; }
    }

    public enum Vl53l7Rpt {
        REG_DATA(0x91),
        /** Bridge state; carries <b>no</b> echoed command byte. */
        VL53_INFO(0x92),
        VL53_FRAME(0x93);

        public final int value;
        Vl53l7Rpt(int value) { this.value = value; }
    }

    /**
     * VL53_PIN_CTRL actions. None is a true sensor reset (the board has no power
     * GPIO): after LPN_OFF or SOFT_CYCLE the host must re-run init().
     */
    public enum PinAction {
        /** Stop streaming, drive LPn low: sensor I2C interface off. */
        LPN_OFF(0),
        /** Drive LPn high: interface on (power-up default). */
        LPN_ON(1),
        /** Pulse I2C_RST. */
        I2C_RST(2),
        /** Stop streaming, LPn low 1 ms, high, I2C_RST pulse; clears I2C counters. */
        SOFT_CYCLE(3);

        public final int value;
        PinAction(int value) { this.value = value; }
    }

    /** RPT_VL53_INFO.lastI2cError values. */
    public static final int I2C_ERR_OK = 0;
    public static final int I2C_ERR_NACK = 1;
    public static final int I2C_ERR_TIMEOUT = 2;
    public static final int I2C_ERR_BUS_ERROR = 3;

    /** VL53LMZ_READ_MAX: READ_REG len 1..1536 (the VL53L8 host's 2048 fails here). */
    public static final int READ_MAX_LEN = 1536;
    /** VL53LMZ_XFER_MAX: WRITE_REG N 1..2048. */
    public static final int WRITE_MAX_LEN = 2048;
    /** Bytes of frame data per RPT_VL53_FRAME chunk (VL53L8: 1528). */
    public static final int STREAM_CHUNK_MAX = 1536;
    /** RPT_VL53_INFO payload size. */
    public static final int INFO_SIZE = 20;

    /** Nominal SCL steps the firmware carries a timing for; others snap to nearest. */
    public static final int[] I2C_SPEED_STEPS_KHZ = {100, 200, 400, 500, 600, 700, 800, 900, 1000};

    // ── command payload encoders ─────────────────────────────────────────────

    /** VL53_READ_REG payload: {@code addr u16, len u16} little-endian. */
    public static byte[] packReadReg(int addr, int len) {
        return new byte[] {
            (byte) addr, (byte) (addr >>> 8), (byte) len, (byte) (len >>> 8)
        };
    }

    /** VL53_WRITE_REG payload: {@code addr u16, data[1..2048]}. */
    public static byte[] packWriteReg(int addr, byte[] data) {
        byte[] out = new byte[2 + data.length];
        out[0] = (byte) addr;
        out[1] = (byte) (addr >>> 8);
        System.arraycopy(data, 0, out, 2, data.length);
        return out;
    }

    /** VL53_START_STREAM payload: {@code frame_size u16}. */
    public static byte[] packStartStream(int frameSize) {
        return new byte[] {(byte) frameSize, (byte) (frameSize >>> 8)};
    }

    /** VL53_PIN_CTRL payload: {@code action u8} (see {@link PinAction}). */
    public static byte[] packPinCtrl(int action) {
        return new byte[] {(byte) action};
    }

    /** VL53_PIN_CTRL payload for a {@link PinAction}. */
    public static byte[] packPinCtrl(PinAction action) {
        return packPinCtrl(action.value);
    }

    /** VL53_SET_I2C_SPEED payload: {@code khz u16}. */
    public static byte[] packSetI2cSpeed(int khz) {
        return new byte[] {(byte) khz, (byte) (khz >>> 8)};
    }

    // ── report decoders ──────────────────────────────────────────────────────

    /**
     * RPT_VL53_INFO — bridge state only (the sensor is never probed), 20-byte LE
     * payload {@code <IIIBBBHHB}. All counters run from power-up / DEVICE_RESET;
     * SOFT_CYCLE clears the I2C ones. The report carries no echoed command byte.
     */
    public record Vl53l7Info(
            long intEdges, long framesDropped, long i2cErrors, int lastI2cError,
            int lpnLevel, int intLevel, int i2cKhz, int frameSize, boolean streaming) {
        public static Vl53l7Info unpack(byte[] p) {
            if (p.length < INFO_SIZE) {
                throw new IllegalArgumentException(
                    "RPT_VL53_INFO: expected " + INFO_SIZE + " bytes, got " + p.length);
            }
            return new Vl53l7Info(
                u32le(p, 0),
                u32le(p, 4),
                u32le(p, 8),
                p[12] & 0xFF,
                p[13] & 0xFF,
                p[14] & 0xFF,
                u16le(p, 15),
                u16le(p, 17),
                (p[19] & 0xFF) != 0);
        }
    }

    // ── LE helpers ───────────────────────────────────────────────────────────

    private static int u16le(byte[] b, int off) {
        return (b[off] & 0xFF) | ((b[off + 1] & 0xFF) << 8);
    }

    private static long u32le(byte[] b, int off) {
        return (b[off] & 0xFFL)
                | ((b[off + 1] & 0xFFL) << 8)
                | ((b[off + 2] & 0xFFL) << 16)
                | ((b[off + 3] & 0xFFL) << 24);
    }
}
