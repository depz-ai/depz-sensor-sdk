package ai.depz.sensor.protocol;

/** SR04 wire codecs (contracts/03_SENSOR_SR04.md). */
public final class Sr04 {
    private Sr04() {}

    public enum Sr04Cmd {
        GET_SAMPLE_PERIOD(0x32),
        SET_SAMPLE_PERIOD(0x33),
        GET_ECHO_DECAY(0x34),
        SET_ECHO_DECAY(0x35),
        MEASURE_ONCE(0x36),
        START_MEASUREMENT_LOOP(0x37),
        STOP_MEASUREMENT_LOOP(0x38);

        public final int value;
        Sr04Cmd(int value) { this.value = value; }
    }

    public enum Sr04Rpt {
        DATA(0x91),
        SAMPLE_PERIOD(0x92),
        ECHO_DECAY(0x93);

        public final int value;
        Sr04Rpt(int value) { this.value = value; }
    }

    /** echo_time_us sentinel: no echo received. */
    public static final int ECHO_TIMEOUT = 0xFFFF;

    public static final long SAMPLE_PERIOD_DEFAULT_US = 50_000L;
    public static final int ECHO_DECAY_DEFAULT_US = 5_000;
    public static final int ECHO_DECAY_MIN_US = 4_000;
    public static final int ECHO_DECAY_MAX_US = 65_000;

    /** {@code sourceCmd}: 0x36 single shot (host or SYNC_IN), 0x37 loop sample. */
    public record Sr04Data(int sourceCmd, long timestampUs, int echoTimeUs) {
        public static Sr04Data unpack(byte[] p) {
            int cmd = p[0] & 0xFF;
            long ts = Common.u64le(p, 1);
            int echo = (p[9] & 0xFF) | ((p[10] & 0xFF) << 8);
            return new Sr04Data(cmd, ts, echo);
        }
    }

    public static byte[] packSamplePeriod(long periodUs) {
        return new byte[] {
            (byte) periodUs, (byte) (periodUs >>> 8), (byte) (periodUs >>> 16), (byte) (periodUs >>> 24)
        };
    }

    /** Reads the u32 sample period as an unsigned value in a long. */
    public static long unpackSamplePeriod(byte[] p) {
        return (p[0] & 0xFFL) | ((p[1] & 0xFFL) << 8) | ((p[2] & 0xFFL) << 16) | ((p[3] & 0xFFL) << 24);
    }

    public static byte[] packEchoDecay(int decayUs) {
        return new byte[] {(byte) decayUs, (byte) (decayUs >>> 8)};
    }

    public static int unpackEchoDecay(byte[] p) {
        return (p[0] & 0xFF) | ((p[1] & 0xFF) << 8);
    }

    /**
     * Round-trip echo time → distance in mm; {@code null} for the timeout
     * sentinel. Default speed of sound 343 m/s; with {@code airTempC} uses
     * c = 331.3 + 0.606·T (m/s).
     */
    public static Double distanceMmFromEcho(int echoTimeUs, Double airTempC) {
        if (echoTimeUs == ECHO_TIMEOUT) {
            return null;
        }
        double cMs = (airTempC == null) ? 343.0 : 331.3 + 0.606 * airTempC;
        return echoTimeUs * cMs / 2000.0;
    }
}
