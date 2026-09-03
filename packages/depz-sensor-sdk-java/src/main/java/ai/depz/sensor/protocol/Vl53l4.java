package ai.depz.sensor.protocol;

/** VL53L4CD register-bridge wire codecs (contracts/10_SENSOR_VL53L4.md). */
public final class Vl53l4 {
    private Vl53l4() {}

    public enum Vl53l4Cmd {
        READ_REG(0x32),
        WRITE_REG(0x33),
        XSHUT(0x34),
        START_STREAM(0x35),
        STOP_STREAM(0x36),
        GET_INFO(0x37),
        SET_I2C_SPEED(0x38);

        public final int value;
        Vl53l4Cmd(int value) { this.value = value; }
    }

    public enum Vl53l4Rpt {
        REG_DATA(0x91),
        INFO(0x92),
        STREAM(0x93);

        public final int value;
        Vl53l4Rpt(int value) { this.value = value; }
    }

    /**
     * Max read/write length per transfer. The STM32 I2C NBYTES field is 8 bit
     * and a write spends two bytes on the register address; the firmware
     * applies the same 253 to both directions.
     */
    public static final int XFER_MAX = 253;

    /** VL53_XSHUT action: drive XSHUT low (sensor powered down). */
    public static final int XSHUT_OFF = 0;
    /** VL53_XSHUT action: drive XSHUT high, no boot handshake. */
    public static final int XSHUT_ON = 1;
    /** VL53_XSHUT action: pulse low then wait for the boot handshake (~3 ms). */
    public static final int XSHUT_RESET = 2;

    /**
     * VL53_START_STREAM flags bit 1: interrupt polarity, mirroring bit 4 of
     * GPIO_HV_MUX__CTRL (0x0030). Clear (default): INT active low.
     */
    public static final int SF_INT_ACT_HIGH = 0x02;

    // ── command payload encoders ─────────────────────────────────────────────

    /** VL53_READ_REG payload: {@code addr u16, len u16} little-endian. */
    public static byte[] packReadReg(int addr, int len) {
        return new byte[] {
            (byte) addr, (byte) (addr >>> 8), (byte) len, (byte) (len >>> 8)
        };
    }

    /** VL53_WRITE_REG payload: {@code addr u16, data[1..253]}. */
    public static byte[] packWriteReg(int addr, byte[] data) {
        byte[] out = new byte[2 + data.length];
        out[0] = (byte) addr;
        out[1] = (byte) (addr >>> 8);
        System.arraycopy(data, 0, out, 2, data.length);
        return out;
    }

    /** VL53_XSHUT payload: {@code action u8} (XSHUT_OFF/ON/RESET). */
    public static byte[] packXshut(int action) {
        return new byte[] {(byte) action};
    }

    /** VL53_START_STREAM payload: {@code addr u16, len u16, flags u8}. */
    public static byte[] packStartStream(int addr, int len, int flags) {
        return new byte[] {
            (byte) addr, (byte) (addr >>> 8), (byte) len, (byte) (len >>> 8), (byte) flags
        };
    }

    /** VL53_SET_I2C_SPEED payload: {@code khz u16}. */
    public static byte[] packSetI2cSpeed(int khz) {
        return new byte[] {(byte) khz, (byte) (khz >>> 8)};
    }

    // ── report decoders ──────────────────────────────────────────────────────

    /**
     * RPT_VL53_REG_DATA — one register read. {@code cmd} echoes the READ_REG
     * opcode; {@code timestampUs} is MCU uptime at I2C-read completion.
     */
    public record RegData(int cmd, long timestampUs, byte[] data) {
        public static RegData unpack(byte[] p) {
            int cmd = p[0] & 0xFF;
            long ts = Common.u64le(p, 1);
            byte[] data = java.util.Arrays.copyOfRange(p, 9, p.length);
            return new RegData(cmd, ts, data);
        }
    }

    /**
     * RPT_VL53_INFO — bridge diagnostics (21-byte LE payload). Counters are
     * free-running and wrap silently; watch increments, not absolute values.
     * {@code modelId} expected 0xEBAA, {@code fwStatus} expected 0x03 (booted);
     * {@code lastI2cError}: 0 none, 1 NACK, 2 TIMEOUT, 3 BUS_ERROR.
     */
    public record Vl53l4Info(
            long intEdges, long slotsSkipped, long i2cErrors, int lastI2cError,
            int modelId, int fwStatus, int initialized, int xshutLevel,
            int intLevel, int i2cKhz) {
        public static Vl53l4Info unpack(byte[] p) {
            return new Vl53l4Info(
                u32le(p, 0),
                u32le(p, 4),
                u32le(p, 8),
                p[12] & 0xFF,
                u16le(p, 13),
                p[15] & 0xFF,
                p[16] & 0xFF,
                p[17] & 0xFF,
                p[18] & 0xFF,
                u16le(p, 19));
        }
    }

    /**
     * RPT_VL53_STREAM — one streamed register block. {@code addr}/{@code len}
     * echo the stream configuration so each report is self-describing;
     * {@code timestampUs} is MCU uptime at the INT edge (the sensor event).
     */
    public record StreamData(long timestampUs, int addr, int len, byte[] data) {
        public static StreamData unpack(byte[] p) {
            long ts = Common.u64le(p, 0);
            int addr = u16le(p, 8);
            int len = u16le(p, 10);
            byte[] data = java.util.Arrays.copyOfRange(p, 12, 12 + len);
            return new StreamData(ts, addr, len, data);
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
