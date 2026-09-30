package ai.depz.sensor.protocol;

/**
 * VL53L 1D-family register-bridge wire codecs, protocol v2.00
 * (contracts/12_SENSOR_VL53LX.md).
 *
 * <p>One firmware ({@code APP_VL53L0_4}) serves VL53L0X, VL53L1CX, VL53L1CB,
 * VL53L3CX, VL53L4CD and VL53L4CX. It is the VL53L4CD bridge of contract 10 with
 * the three sensor-specific facts moved to the host: the register-address width
 * (VL53_SET_ADDR_WIDTH, new), the interrupt-release writes (carried by
 * VL53_START_STREAM) and the boot handshake (no longer inside VL53_XSHUT).
 * READ_REG, WRITE_REG, XSHUT, STOP_STREAM, SET_I2C_SPEED and the REG_DATA /
 * STREAM reports are the contract-10 codecs of {@link Vl53l4}, re-exposed here.
 */
public final class Vl53lx {
    private Vl53lx() {}

    public enum Vl53lxCmd {
        READ_REG(0x32),
        WRITE_REG(0x33),
        XSHUT(0x34),
        START_STREAM(0x35),
        STOP_STREAM(0x36),
        GET_INFO(0x37),
        SET_I2C_SPEED(0x38),
        /** New in v2.00: register-address width, 1 or 2 bytes (sticky, 2 after reset). */
        SET_ADDR_WIDTH(0x39),
        /** New in v2.01 (firmware v0.24), no payload: zero the I2C error counter; sent after every sensor init. */
        CLEAR_I2C_ERRORS(0x3A);

        public final int value;
        Vl53lxCmd(int value) { this.value = value; }
    }

    public enum Vl53lxRpt {
        REG_DATA(0x91),
        INFO(0x92),
        STREAM(0x93);

        public final int value;
        Vl53lxRpt(int value) { this.value = value; }
    }

    /** Interrupt-release steps a stream may carry (VL53_CLEAR_STEPS_WIRE_MAX). */
    public static final int CLEAR_STEPS_MAX = 4;
    /** RPT_VL53_INFO payload size (v2.00). */
    public static final int INFO_SIZE = 23;

    /** Same as contract 10. */
    public static final int XFER_MAX = Vl53l4.XFER_MAX;
    public static final int XSHUT_OFF = Vl53l4.XSHUT_OFF;
    public static final int XSHUT_ON = Vl53l4.XSHUT_ON;
    /** 1 ms low + a fixed 5 ms wait; v2.00 has <b>no</b> boot handshake — the host polls. */
    public static final int XSHUT_RESET = Vl53l4.XSHUT_RESET;
    public static final int SF_INT_ACT_HIGH = Vl53l4.SF_INT_ACT_HIGH;

    // ── command payload encoders ─────────────────────────────────────────────

    /** VL53_READ_REG payload (contract 10). At width 1 only the low byte of addr goes on the bus. */
    public static byte[] packReadReg(int addr, int len) {
        return Vl53l4.packReadReg(addr, len);
    }

    /** VL53_WRITE_REG payload (contract 10). */
    public static byte[] packWriteReg(int addr, byte[] data) {
        return Vl53l4.packWriteReg(addr, data);
    }

    /** VL53_XSHUT payload (contract 10 codec; RESET semantics changed, see {@link #XSHUT_RESET}). */
    public static byte[] packXshut(int action) {
        return Vl53l4.packXshut(action);
    }

    /** VL53_SET_I2C_SPEED payload (contract 10). */
    public static byte[] packSetI2cSpeed(int khz) {
        return Vl53l4.packSetI2cSpeed(khz);
    }

    /**
     * VL53_START_STREAM payload, {@code 6 + 3n} bytes: {@code addr u16, len u16,
     * flags u8, n_clear u8, clear[n] {addr u16, value u8}}. {@code clear} holds
     * {@code {addr, value}} pairs the bridge writes after every block read
     * (0..{@value #CLEAR_STEPS_MAX} steps); more throws
     * {@link IllegalArgumentException}.
     */
    public static byte[] packStartStream(int addr, int len, int[][] clear, int flags) {
        if (clear == null) {
            clear = new int[0][];
        }
        if (clear.length > CLEAR_STEPS_MAX) {
            throw new IllegalArgumentException(
                "at most " + CLEAR_STEPS_MAX + " interrupt-release steps, got " + clear.length);
        }
        byte[] out = new byte[6 + 3 * clear.length];
        out[0] = (byte) addr;
        out[1] = (byte) (addr >>> 8);
        out[2] = (byte) len;
        out[3] = (byte) (len >>> 8);
        out[4] = (byte) flags;
        out[5] = (byte) clear.length;
        for (int i = 0; i < clear.length; i++) {
            int o = 6 + 3 * i;
            out[o] = (byte) clear[i][0];
            out[o + 1] = (byte) (clear[i][0] >>> 8);
            out[o + 2] = (byte) clear[i][1];
        }
        return out;
    }

    /** VL53_SET_ADDR_WIDTH payload: {@code width u8}, 1 or 2 (else {@link IllegalArgumentException}). */
    public static byte[] packSetAddrWidth(int width) {
        if (width != 1 && width != 2) {
            throw new IllegalArgumentException("register address width is 1 or 2 bytes, got " + width);
        }
        return new byte[] {(byte) width};
    }

    // ── report decoders ──────────────────────────────────────────────────────

    /** RPT_VL53_REG_DATA — contract 10 unchanged. */
    public static Vl53l4.RegData unpackRegData(byte[] p) {
        return Vl53l4.RegData.unpack(p);
    }

    /** RPT_VL53_STREAM — contract 10 unchanged. */
    public static Vl53l4.StreamData unpackStreamData(byte[] p) {
        return Vl53l4.StreamData.unpack(p);
    }

    /**
     * RPT_VL53_INFO (v2.00, 23-byte LE payload {@code <IIIBBBHBBI}) — bridge
     * state only; the bridge reads no sensor register. Counters are free-running
     * (wrap silently): watch increments. {@code slotsSkipped}, {@code framesDropped}
     * and the fault latch reset at START_STREAM; {@code i2cErrors} is free-running.
     * {@code lastI2cError}: 0 none, 1 NACK, 2 TIMEOUT, 3 BUS_ERROR.
     */
    public record Vl53lxInfo(
            long intEdges, long slotsSkipped, long i2cErrors, int lastI2cError,
            int xshutLevel, int intLevel, int i2cKhz, int addrWidth, int nClear,
            long framesDropped) {
        public static Vl53lxInfo unpack(byte[] p) {
            if (p.length < INFO_SIZE) {
                throw new IllegalArgumentException(
                    "RPT_VL53_INFO: expected " + INFO_SIZE + " bytes, got " + p.length);
            }
            return new Vl53lxInfo(
                u32le(p, 0),
                u32le(p, 4),
                u32le(p, 8),
                p[12] & 0xFF,
                p[13] & 0xFF,
                p[14] & 0xFF,
                u16le(p, 15),
                p[17] & 0xFF,
                p[18] & 0xFF,
                u32le(p, 19));
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
