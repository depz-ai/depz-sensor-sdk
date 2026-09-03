package ai.depz.sensor.protocol;

import java.nio.charset.StandardCharsets;

/**
 * Common command/report IDs and payload codecs (contracts/02_COMMON_COMMANDS.md).
 *
 * <p>Payload codecs return raw integers exactly as on the wire; unit
 * conversions (0.1 °C, µs) happen in the device layer.
 */
public final class Common {
    private Common() {}

    /** Host→device command opcodes. */
    public enum Cmd {
        BOOTLOADER(0x01),
        DEVICE_RESET(0x02),
        GET_DEVICE_NAME(0x03),
        GET_NAME_ACTIVE_SOFTWARE(0x04),
        GET_SERIAL(0x05),
        SYNC_TIME(0x06),
        GET_MCU_TEMPERATURE(0x07),
        GET_PAYLOAD_CRC_TYPE(0x08),
        SET_PAYLOAD_CRC_TYPE(0x09),
        THROUGHPUT_TX_START(0x1C),
        THROUGHPUT_TX_STOP(0x1D),
        THROUGHPUT_RX_DATA(0x1E),
        GET_SYNC_PIN_CONFIG(0x30),
        SET_SYNC_PIN_CONFIG(0x31);

        public final int value;
        Cmd(int value) { this.value = value; }
    }

    /** Device→host report IDs. */
    public enum Rpt {
        STATUS(0x80),
        TEXT(0x81),
        SYNC_TIME(0x82),
        TEMPERATURE(0x83),
        SEQUENCE_ERROR(0x84),
        PAYLOAD_CRC_TYPE(0x87),
        THROUGHPUT_DATA(0x88),
        SYNC_PIN_CONFIG(0x90);

        public final int value;
        Rpt(int value) { this.value = value; }
    }

    public enum Status {
        OK(0x00),
        ERROR(0x01),
        ERR_INVALID_CMD(0x02),
        ERR_PAYLOAD_FORMAT(0x03),
        ERR_INVALID_PARAM(0x04),
        ERR_PAYLOAD_CRC(0x05),
        ERR_BUSY(0x06),
        ERR_CMD_NOT_SUPPORTED(0x07),
        ERR_NOT_INITIALIZED(0x08),
        ERR_HARDWARE_FAULT(0x09);

        public final int value;
        Status(int value) { this.value = value; }
    }

    public enum SyncPinMode {
        DISABLE(0x00),
        IN(0x01),
        OUT_START(0x02),
        OUT_END(0x03),
        OUT_BOTH(0x04);

        public final int value;
        SyncPinMode(int value) { this.value = value; }

        public static SyncPinMode fromValue(int v) {
            for (SyncPinMode m : values()) if (m.value == v) return m;
            throw new IllegalArgumentException("invalid sync-pin mode: " + v);
        }
    }

    public enum SyncPinPolarity {
        IDLE_LOW(0x00),
        IDLE_HIGH(0x01);

        public final int value;
        SyncPinPolarity(int value) { this.value = value; }

        public static SyncPinPolarity fromValue(int v) {
            for (SyncPinPolarity p : values()) if (p.value == v) return p;
            throw new IllegalArgumentException("invalid sync-pin polarity: " + v);
        }
    }

    /** Value of the echoed-cmd byte in unsolicited reports. */
    public static final int UNSOLICITED = 0x00;

    // ── report records ───────────────────────────────────────────────────────

    /** {@code cmd}: echoed request opcode; 0x00 = unsolicited. */
    public record StatusReport(int cmd, int status) {
        public static StatusReport unpack(byte[] p) {
            return new StatusReport(p[0] & 0xFF, p[1] & 0xFF);
        }
    }

    public record TextReport(int cmd, String text) {
        public static TextReport unpack(byte[] p) {
            byte[] rest = new byte[p.length - 1];
            System.arraycopy(p, 1, rest, 0, rest.length);
            return new TextReport(p[0] & 0xFF, stripDeviceString(rest));
        }
    }

    /** {@code pcTimestampUs}=T1 echoed, {@code mcuRxUs}=T2, {@code mcuTxUs}=T3. */
    public record SyncTimeReport(long pcTimestampUs, long mcuRxUs, long mcuTxUs) {
        public static SyncTimeReport unpack(byte[] p) {
            return new SyncTimeReport(u64le(p, 0), u64le(p, 8), u64le(p, 16));
        }
    }

    /** {@code rawDecidegrees}: int16, units of 0.1 °C. */
    public record TemperatureReport(long timestampUs, int rawDecidegrees) {
        public static TemperatureReport unpack(byte[] p) {
            return new TemperatureReport(u64le(p, 0), i16le(p, 8));
        }

        public double celsius() {
            return rawDecidegrees / 10.0;
        }
    }

    public record SequenceErrorReport(int expectedSeq, int receivedSeq) {
        public static SequenceErrorReport unpack(byte[] p) {
            return new SequenceErrorReport(p[0] & 0xFF, p[1] & 0xFF);
        }
    }

    /** {@code pin}: 1..5. */
    public record SyncPinConfig(int pin, SyncPinMode mode, SyncPinPolarity polarity) {
        public byte[] pack() {
            return new byte[] {(byte) pin, (byte) mode.value, (byte) polarity.value};
        }

        public static SyncPinConfig unpack(byte[] p) {
            return new SyncPinConfig(p[0] & 0xFF, SyncPinMode.fromValue(p[1] & 0xFF), SyncPinPolarity.fromValue(p[2] & 0xFF));
        }
    }

    // ── codecs ───────────────────────────────────────────────────────────────

    public static byte[] packSyncTime(long pcTimestampUs) {
        return u64leBytes(pcTimestampUs);
    }

    /**
     * NTP-style clock math, all µs (contract 02 §5). Returns
     * {@code {offset_us, rtt_us}} where offset = device_clock - host_clock,
     * computed as {@code ((T2-T1)+(T3-T4)) / 2} truncated toward zero (Java
     * {@code long} division truncates toward zero, matching all SDKs), and
     * {@code rtt = (T4-T1)-(T3-T2)}.
     */
    public static long[] syncTimeOffsetRtt(long t1, long t2, long t3, long t4) {
        long num = (t2 - t1) + (t3 - t4);
        long offset = num / 2;
        long rtt = (t4 - t1) - (t3 - t2);
        return new long[] {offset, rtt};
    }

    /** Decode an ASCII device string, dropping trailing NUL/0xFF filler. */
    public static String stripDeviceString(byte[] raw) {
        int end = raw.length;
        while (end > 0 && (raw[end - 1] == 0x00 || (raw[end - 1] & 0xFF) == 0xFF)) {
            end--;
        }
        return new String(raw, 0, end, StandardCharsets.US_ASCII);
    }

    // ── little-endian helpers ─────────────────────────────────────────────────

    static long u64le(byte[] b, int off) {
        long v = 0;
        for (int i = 0; i < 8; i++) {
            v |= (long) (b[off + i] & 0xFF) << (8 * i);
        }
        return v;
    }

    static int i16le(byte[] b, int off) {
        return (short) ((b[off] & 0xFF) | ((b[off + 1] & 0xFF) << 8));
    }

    static byte[] u64leBytes(long v) {
        byte[] out = new byte[8];
        for (int i = 0; i < 8; i++) {
            out[i] = (byte) (v >>> (8 * i));
        }
        return out;
    }
}
