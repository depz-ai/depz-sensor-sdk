package ai.depz.sensor.protocol;

/**
 * BNO085 / BNO086 bridge wire codecs (contracts/05_SENSOR_BNO086.md §2): the MCU only shuttles
 * raw SHTP frames; the SH-2 hub protocol runs on the host
 * ({@link ai.depz.sensor.sensors.bno086.Sh2Hub}, {@link ai.depz.sensor.devices.Bno086Device}).
 */
public final class Bno086 {
    private Bno086() {}

    public enum Bno086Cmd {
        /** Hardware reset through nRST; RPT_STATUS OK. */
        SENSOR_RESET(0x32),
        /** 1 ms WAKE (PS0) pulse; RPT_STATUS OK. */
        SENSOR_WAKE_UP(0x33),
        /** Payload = one raw SHTP frame; RPT_STATUS OK or ERR_BUSY (both TX slots full). */
        SEND_SHTP_PACKET(0x34);

        public final int value;
        Bno086Cmd(int value) { this.value = value; }
    }

    public enum Bno086Rpt {
        /** rpt_data_t: cmd u8 (always 0x00, ERRATA E2), timestamp_us u64, raw SHTP frame. */
        DATA(0x91);

        public final int value;
        Bno086Rpt(int value) { this.value = value; }
    }

    /**
     * Two 64-byte SHTP transmit slots on the MCU (ERRATA E2); a third send gets ERR_BUSY, to back
     * off at least this long.
     */
    public static final int BUSY_BACKOFF_MS = 200;

    /** One RPT_DATA report: an SHTP frame captured from the sensor bus, with the MCU uptime. */
    public record Data(int cmd, long timestampUs, byte[] shtp) {
        public static Data unpack(byte[] p) {
            Wire.needBytes("RPT_DATA", p, 9);
            return new Data(p[0] & 0xFF, Common.u64le(p, 1), java.util.Arrays.copyOfRange(p, 9, p.length));
        }
    }
}
