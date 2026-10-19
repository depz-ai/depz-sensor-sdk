package ai.depz.sensor.sensors.bno086;

/**
 * SH-2 control-channel encoders (contracts/05_SENSOR_BNO086.md §6). Pure
 * codecs; mirror of the TS/Python {@code sh2} references. Report/response
 * decoders that require a live hub are out of the verifiable-vector scope.
 */
public final class Sh2 {
    private Sh2() {}

    // Control report IDs.
    public static final int SET_FEATURE_COMMAND = 0xfd;
    public static final int GET_FEATURE_REQUEST = 0xfe;
    public static final int GET_FEATURE_RESPONSE = 0xfc;
    public static final int PRODUCT_ID_REQUEST = 0xf9;
    public static final int PRODUCT_ID_RESPONSE = 0xf8;
    public static final int COMMAND_REQUEST = 0xf2;
    public static final int COMMAND_RESPONSE = 0xf1;
    public static final int FRS_READ_REQUEST = 0xf4;
    public static final int FRS_READ_RESPONSE = 0xf3;
    public static final int FRS_WRITE_REQUEST = 0xf7;
    public static final int FRS_WRITE_DATA = 0xf6;
    public static final int FRS_WRITE_RESPONSE = 0xf5;

    /**
     * Set Feature {@code flags} bit 2 (SH-2 §6.5.4): wake-up enabled — the
     * sensor reports on SHTP channel 4 (wake input).
     */
    public static final int FEATURE_FLAG_WAKEUP = 0x04;

    /**
     * Set Feature {@code flags} bit 3 (SH-2 §6.5.4): always-on — the sensor
     * keeps running while the hub sleeps. With {@link #FEATURE_FLAG_WAKEUP} a
     * report kept its full rate through {@link #buildSleep()} on a DEPZ BNO086
     * (SH-2 3.12.6, 2026-10-16), on channel 4, while every other report stopped.
     */
    public static final int FEATURE_FLAG_ALWAYS_ON = 0x08;

    /** Set Feature Command (0xFD), 17 bytes. intervalUs = 0 disables. */
    public static byte[] buildSetFeature(
            int sensorId, long intervalUs, long batchUs, int sensitivity, int flags, long cfgWord) {
        byte[] out = new byte[17];
        out[0] = (byte) SET_FEATURE_COMMAND;
        out[1] = (byte) sensorId;
        out[2] = (byte) flags;
        putU16le(out, 3, sensitivity);
        putU32le(out, 5, intervalUs);
        putU32le(out, 9, batchUs);
        putU32le(out, 13, cfgWord);
        return out;
    }

    /** Get Feature Request (0xFE), 2 bytes. */
    public static byte[] buildGetFeatureRequest(int sensorId) {
        return new byte[] {(byte) GET_FEATURE_REQUEST, (byte) sensorId};
    }

    /** Product ID Request (0xF9), 2 bytes. */
    public static byte[] buildProductIdRequest() {
        return new byte[] {(byte) PRODUCT_ID_REQUEST, 0x00};
    }

    /**
     * SH-2 "Sleep" for the <b>executable</b> channel (channel 1, not control):
     * {@code [0x03]}. Frame it with
     * {@code nextFrame(Shtp.CHANNEL_EXECUTABLE, Sh2.buildSleep())} and send it
     * through the bridge's SEND_SHTP_PACKET (0x34); the hub sends no response.
     * Every report stops, except those enabled with
     * {@link #FEATURE_FLAG_WAKEUP} | {@link #FEATURE_FLAG_ALWAYS_ON}, which keep
     * their rate; the configuration (enabled sensors, rates) is kept.
     * {@link #buildPowerOn()} ends it. On a DEPZ BNO086 (SH-2 3.12.6,
     * 2026-10-16) all reports stopped within one sample; a WAKEUP|ALWAYS_ON
     * report kept streaming at full rate on channel 4.
     */
    public static byte[] buildSleep() {
        return new byte[] {(byte) Shtp.EXECUTABLE_SLEEP};
    }

    /**
     * SH-2 "On" for the <b>executable</b> channel (channel 1): {@code [0x02]} —
     * ends {@link #buildSleep()}'s sleep. Every report that was enabled comes
     * back at its rate, without re-enabling; the hub sends no response. Not the
     * bridge's SENSOR_WAKE_UP (0x33), which pulses the WAKE/PS0 pin — a hardware
     * signal, not an SH-2 command.
     */
    public static byte[] buildPowerOn() {
        return new byte[] {(byte) Shtp.EXECUTABLE_ON};
    }

    /** Command Request (0xF2), 12 bytes: id, seq, command, P0..P8. */
    public static byte[] buildCommandRequest(int seq, int command, byte[] params) {
        if (params.length > 9) {
            throw new IllegalArgumentException("command request carries at most 9 parameter bytes");
        }
        byte[] out = new byte[12];
        out[0] = (byte) COMMAND_REQUEST;
        out[1] = (byte) seq;
        out[2] = (byte) command;
        System.arraycopy(params, 0, out, 3, params.length);
        return out;
    }

    /** FRS Read Request (0xF4), 8 bytes. blockWords = 0 reads the record. */
    public static byte[] buildFrsReadRequest(int frsType, int offsetWords, int blockWords) {
        byte[] out = new byte[8];
        out[0] = (byte) FRS_READ_REQUEST;
        out[1] = 0;
        putU16le(out, 2, offsetWords);
        putU16le(out, 4, frsType);
        putU16le(out, 6, blockWords);
        return out;
    }

    /** FRS Write Request (0xF7), 6 bytes. lengthWords = 0 erases the record. */
    public static byte[] buildFrsWriteRequest(int frsType, int lengthWords) {
        byte[] out = new byte[6];
        out[0] = (byte) FRS_WRITE_REQUEST;
        out[1] = 0;
        putU16le(out, 2, lengthWords);
        putU16le(out, 4, frsType);
        return out;
    }

    /** FRS Write Data (0xF6), 12 bytes; 1 or 2 words per packet. */
    public static byte[] buildFrsWriteData(int offsetWords, long[] words) {
        if (words.length < 1 || words.length > 2) {
            throw new IllegalArgumentException("FRS write data carries 1 or 2 words");
        }
        byte[] out = new byte[12];
        out[0] = (byte) FRS_WRITE_DATA;
        out[1] = 0;
        putU16le(out, 2, offsetWords);
        putU32le(out, 4, words[0]);
        putU32le(out, 8, words.length > 1 ? words[1] : 0);
        return out;
    }

    private static void putU16le(byte[] b, int off, int v) {
        b[off] = (byte) v;
        b[off + 1] = (byte) (v >>> 8);
    }

    private static void putU32le(byte[] b, int off, long v) {
        b[off] = (byte) v;
        b[off + 1] = (byte) (v >>> 8);
        b[off + 2] = (byte) (v >>> 16);
        b[off + 3] = (byte) (v >>> 24);
    }
}
