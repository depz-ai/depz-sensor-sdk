package ai.depz.sensor.protocol;

/**
 * BNO055 register-bridge wire codecs, protocol v0.10
 * (contracts/13_SENSOR_BNO055.md §2–§3).
 *
 * <p>The firmware ({@code APP_BNO055}) is a thin register bridge: the MCU owns
 * the I2C bus (sensor at 7-bit 0x28, 400 kHz fixed), the nRESET/INT pins and one
 * streaming loop. Everything else — operating mode, units, axis remap,
 * calibration, decoding — is host logic expressed as register access
 * ({@link ai.depz.sensor.sensors.bno055.Bno055Regs}). Mirrors
 * {@code depz_sensor_sdk/protocol/bno055.py} 1:1.
 */
public final class Bno055 {
    private Bno055() {}

    public enum Bno055Cmd {
        READ_REG(0x32),
        WRITE_REG(0x33),
        /** Pulses nRESET; answered only after the chip-ID handshake (~0.5 s, allow ≥ 1.5 s). */
        RESET(0x34),
        START_STREAM(0x35),
        STOP_STREAM(0x36),
        GET_INFO(0x37);

        public final int value;
        Bno055Cmd(int value) { this.value = value; }
    }

    public enum Bno055Rpt {
        REG_DATA(0x91),
        INFO(0x92),
        STREAM(0x93);

        public final int value;
        Bno055Rpt(int value) { this.value = value; }
    }

    /** Max bytes per READ_REG / WRITE_REG / streamed block; {@code addr + len ≤ 0x100}. */
    public static final int XFER_MAX = 128;
    /** RPT_BNO_INFO payload size. */
    public static final int INFO_SIZE = 38;

    /** BNO_START_STREAM trigger: read every {@code period_ms} (the only data trigger on SW rev 03.11). */
    public static final int TRIGGER_TIMER = 0;
    /** BNO_START_STREAM trigger: read on the INT rising edge; {@code period_ms} is a missed-edge watchdog. */
    public static final int TRIGGER_INT = 1;

    /** BNO_RESET answers after the sensor's ~0.5 s boot handshake. */
    public static final double RESET_TIMEOUT_S = 3.0;

    /** Identity registers 0x00..0x03 of a healthy BNO055. */
    public static final int EXPECTED_CHIP_ID = 0xA0;
    public static final int EXPECTED_ACC_ID = 0xFB;
    public static final int EXPECTED_MAG_ID = 0x32;
    public static final int EXPECTED_GYR_ID = 0x0F;

    /** {@code last_i2c_error} values in RPT_BNO_INFO, indexed by code. */
    public static final String[] I2C_ERROR_NAMES = {"none", "NACK", "TIMEOUT", "BUS_ERROR"};

    // ── command payload encoders ─────────────────────────────────────────────
    // BNO_RESET, BNO_STOP_STREAM and BNO_GET_INFO carry an empty payload.

    /** BNO_READ_REG payload: {@code addr u8, len u8}. */
    public static byte[] packReadReg(int addr, int len) {
        return new byte[] {(byte) addr, (byte) len};
    }

    /** BNO_WRITE_REG payload: {@code addr u8, data[1..128]}. */
    public static byte[] packWriteReg(int addr, byte[] data) {
        byte[] out = new byte[1 + data.length];
        out[0] = (byte) addr;
        System.arraycopy(data, 0, out, 1, data.length);
        return out;
    }

    /**
     * BNO_START_STREAM payload, 5 bytes: {@code trigger u8, addr u8, len u8,
     * period_ms u16 LE}. Replaces any running stream.
     */
    public static byte[] packStartStream(int trigger, int addr, int len, int periodMs) {
        return new byte[] {
            (byte) trigger, (byte) addr, (byte) len, (byte) periodMs, (byte) (periodMs >>> 8)};
    }

    // ── report decoders ──────────────────────────────────────────────────────

    /** RPT_BNO_REG_DATA: {@code cmd u8} (echoed READ_REG opcode), {@code timestamp_us u64}, data. */
    public record RegData(int cmd, long timestampUs, byte[] data) {
        public static RegData unpack(byte[] p) {
            return new RegData(p[0] & 0xFF, Common.u64le(p, 1),
                java.util.Arrays.copyOfRange(p, 9, p.length));
        }
    }

    /**
     * RPT_BNO_REG_STREAM — one streamed register block. {@code addr}/{@code len}
     * echo the stream configuration so each report is self-describing;
     * {@code timestampUs} is the trigger time (timer expiry or INT edge).
     */
    public record StreamData(long timestampUs, int addr, int len, byte[] data) {
        public static StreamData unpack(byte[] p) {
            long ts = Common.u64le(p, 0);
            int addr = p[8] & 0xFF;
            int len = p[9] & 0xFF;
            return new StreamData(ts, addr, len, java.util.Arrays.copyOfRange(p, 10, 10 + len));
        }
    }

    /**
     * RPT_BNO_INFO (38-byte LE payload {@code <BBBBBHBBBIHHHIIIHBBH}) — sensor
     * identity (registers 0x00..0x06) plus bridge diagnostics. Counters are
     * free-running and wrap silently; watch increments. {@code read*Us},
     * {@code slotsSkipped} and {@code loopMaxUs} reset at START_STREAM. A rising
     * {@code sensorResets} means the bridge pulsed nRESET to recover the bus: the
     * sensor is back in CONFIG and the host must restore its configuration.
     * {@code swRev} is BCD ({@code 0x0311} = 03.11).
     */
    public record Bno055Info(
            int i2cAddr, int chipId, int accId, int magId, int gyrId,
            int swRev, int blRev, int initialized, int intLevel, long intEdges,
            int readMinUs, int readMaxUs, int readAvgUs, long txDropped,
            long i2cErrors, long slotsSkipped, int busRecoveries, int lastI2cError,
            int sensorResets, int loopMaxUs) {
        public static Bno055Info unpack(byte[] p) {
            if (p.length < INFO_SIZE) {
                throw new IllegalArgumentException(
                    "RPT_BNO_INFO: expected " + INFO_SIZE + " bytes, got " + p.length);
            }
            return new Bno055Info(
                p[0] & 0xFF,
                p[1] & 0xFF,
                p[2] & 0xFF,
                p[3] & 0xFF,
                p[4] & 0xFF,
                u16le(p, 5),
                p[7] & 0xFF,
                p[8] & 0xFF,
                p[9] & 0xFF,
                u32le(p, 10),
                u16le(p, 14),
                u16le(p, 16),
                u16le(p, 18),
                u32le(p, 20),
                u32le(p, 24),
                u32le(p, 28),
                u16le(p, 32),
                p[34] & 0xFF,
                p[35] & 0xFF,
                u16le(p, 36));
        }

        /** All four identity registers hold the healthy BNO055 values. */
        public boolean idsOk() {
            return chipId == EXPECTED_CHIP_ID && accId == EXPECTED_ACC_ID
                && magId == EXPECTED_MAG_ID && gyrId == EXPECTED_GYR_ID;
        }

        /** Sensor firmware revision as Bosch writes it: 0x0311 → "03.11". */
        public String swRevText() {
            return String.format("%02X.%02X", swRev >> 8, swRev & 0xFF);
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
