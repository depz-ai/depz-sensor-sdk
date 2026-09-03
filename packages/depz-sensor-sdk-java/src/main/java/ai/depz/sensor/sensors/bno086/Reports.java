package ai.depz.sensor.sensors.bno086;

import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;

/**
 * SH-2 input-report catalog and parsers (contracts/05_SENSOR_BNO086.md §5).
 *
 * <p>Raw wire integers are authoritative (this decode layer surfaces them as
 * {@code *_raw} fields). Q-point scaling is derived downstream. Timestamps:
 * channel-3/4 cargos start with a Base Timestamp Reference (0xFB, i32 base
 * delta in 100 µs ticks, subtracted from the bridge capture time), 0xFA
 * rebases add; each report adds its own 14-bit delay (status bits 7:2 upper,
 * byte 3 lower; 100 µs). timestampUs = capture − baseDelta·100 + delay·100.
 *
 * <p>Mirror of the TS/Python {@code reports} references.
 */
public final class Reports {
    private Reports() {}

    // Sensor (report) IDs.
    public static final int ACCELEROMETER = 0x01;
    public static final int GYROSCOPE = 0x02;
    public static final int MAGNETOMETER = 0x03;
    public static final int LINEAR_ACCELERATION = 0x04;
    public static final int ROTATION_VECTOR = 0x05;
    public static final int GRAVITY = 0x06;
    public static final int UNCAL_GYROSCOPE = 0x07;
    public static final int GAME_ROTATION_VECTOR = 0x08;
    public static final int GEOMAG_ROTATION_VECTOR = 0x09;
    public static final int PRESSURE = 0x0a;
    public static final int AMBIENT_LIGHT = 0x0b;
    public static final int HUMIDITY = 0x0c;
    public static final int PROXIMITY = 0x0d;
    public static final int TEMPERATURE = 0x0e;
    public static final int UNCAL_MAGNETOMETER = 0x0f;
    public static final int TAP_DETECTOR = 0x10;
    public static final int STEP_COUNTER = 0x11;
    public static final int SIGNIFICANT_MOTION = 0x12;
    public static final int STABILITY_CLASSIFIER = 0x13;
    public static final int RAW_ACCELEROMETER = 0x14;
    public static final int RAW_GYROSCOPE = 0x15;
    public static final int RAW_MAGNETOMETER = 0x16;
    public static final int STEP_DETECTOR = 0x18;
    public static final int SHAKE_DETECTOR = 0x19;
    public static final int PERSONAL_ACTIVITY_CLASSIFIER = 0x1e;
    public static final int ARVR_STABILIZED_RV = 0x28;
    public static final int ARVR_STABILIZED_GAME_RV = 0x29;
    public static final int GYRO_INTEGRATED_RV = 0x2a;

    public static final int BASE_TIMESTAMP_REF = 0xfb;
    public static final int TIMESTAMP_REBASE = 0xfa;

    private static final Map<Integer, Integer> REPORT_LENGTHS = new LinkedHashMap<>();

    static {
        REPORT_LENGTHS.put(ACCELEROMETER, 10);
        REPORT_LENGTHS.put(GYROSCOPE, 10);
        REPORT_LENGTHS.put(MAGNETOMETER, 10);
        REPORT_LENGTHS.put(LINEAR_ACCELERATION, 10);
        REPORT_LENGTHS.put(ROTATION_VECTOR, 14);
        REPORT_LENGTHS.put(GRAVITY, 10);
        REPORT_LENGTHS.put(UNCAL_GYROSCOPE, 16);
        REPORT_LENGTHS.put(GAME_ROTATION_VECTOR, 12);
        REPORT_LENGTHS.put(GEOMAG_ROTATION_VECTOR, 14);
        REPORT_LENGTHS.put(PRESSURE, 8);
        REPORT_LENGTHS.put(AMBIENT_LIGHT, 8);
        REPORT_LENGTHS.put(HUMIDITY, 6);
        REPORT_LENGTHS.put(PROXIMITY, 6);
        REPORT_LENGTHS.put(TEMPERATURE, 6);
        REPORT_LENGTHS.put(UNCAL_MAGNETOMETER, 16);
        REPORT_LENGTHS.put(TAP_DETECTOR, 5);
        REPORT_LENGTHS.put(STEP_COUNTER, 12);
        REPORT_LENGTHS.put(SIGNIFICANT_MOTION, 6);
        REPORT_LENGTHS.put(STABILITY_CLASSIFIER, 6);
        REPORT_LENGTHS.put(RAW_ACCELEROMETER, 16);
        REPORT_LENGTHS.put(RAW_GYROSCOPE, 16);
        REPORT_LENGTHS.put(RAW_MAGNETOMETER, 16);
        REPORT_LENGTHS.put(STEP_DETECTOR, 8);
        REPORT_LENGTHS.put(SHAKE_DETECTOR, 6);
        REPORT_LENGTHS.put(0x1a, 6); // flip
        REPORT_LENGTHS.put(0x1b, 8); // pickup
        REPORT_LENGTHS.put(0x1c, 6); // stability detector
        REPORT_LENGTHS.put(PERSONAL_ACTIVITY_CLASSIFIER, 16);
        REPORT_LENGTHS.put(0x1f, 6); // sleep
        REPORT_LENGTHS.put(0x20, 6); // tilt
        REPORT_LENGTHS.put(0x21, 6); // pocket
        REPORT_LENGTHS.put(0x22, 6); // circle
        REPORT_LENGTHS.put(0x23, 6); // heart rate
        REPORT_LENGTHS.put(ARVR_STABILIZED_RV, 14);
        REPORT_LENGTHS.put(ARVR_STABILIZED_GAME_RV, 12);
        REPORT_LENGTHS.put(GYRO_INTEGRATED_RV, 14);
    }

    /** One decoded report: a {@code type} tag plus the field map (raw-integer). */
    public record Report(String type, Map<String, Object> fields) {}

    /**
     * Parse a channel-3/4 cargo into typed reports. {@code captureTimestampUs}
     * is the bridge RPT_DATA capture time (MCU uptime).
     */
    public static List<Report> parseInputCargo(byte[] payload, long captureTimestampUs) {
        List<Report> out = new ArrayList<>();
        long baseUs = captureTimestampUs;
        int pos = 0;
        int n = payload.length;
        while (pos < n) {
            int rid = payload[pos] & 0xFF;
            if (rid == BASE_TIMESTAMP_REF && pos + 5 <= n) {
                long delta = i32le(payload, pos + 1);
                baseUs = captureTimestampUs - delta * 100L;
                pos += 5;
                continue;
            }
            if (rid == TIMESTAMP_REBASE && pos + 5 <= n) {
                long delta = i32le(payload, pos + 1);
                baseUs += delta * 100L;
                pos += 5;
                continue;
            }
            Integer length = REPORT_LENGTHS.get(rid);
            if (length == null || pos + length > n) {
                Map<String, Object> f = new LinkedHashMap<>();
                f.put("sensor_id", (long) rid);
                f.put("timestamp_us", baseUs);
                f.put("data", hex(payload, pos, n - pos));
                out.add(new Report("UnknownReport", f));
                break;
            }
            byte[] rep = new byte[length];
            System.arraycopy(payload, pos, rep, 0, length);
            int seq = rep[1] & 0xFF;
            int status = rep[2] & 0xFF;
            int delayLsb = rep[3] & 0xFF;
            int accuracy = status & 0x03;
            int delayUs = (((status >> 2) << 8) | delayLsb) * 100;
            long ts = baseUs + delayUs;
            out.add(decodeReport(rid, rep, ts, seq, accuracy, delayUs));
            pos += length;
        }
        return out;
    }

    private static Report decodeReport(int rid, byte[] rep, long ts, int seq, int accuracy, int delayUs) {
        Map<String, Object> f = new LinkedHashMap<>();
        f.put("sensor_id", (long) rid);
        f.put("timestamp_us", ts);
        f.put("seq", (long) seq);
        f.put("accuracy", (long) accuracy);
        f.put("delay_us", (long) delayUs);

        switch (rid) {
            case ACCELEROMETER, LINEAR_ACCELERATION, GRAVITY -> {
                f.put("x_raw", (long) i16le(rep, 4));
                f.put("y_raw", (long) i16le(rep, 6));
                f.put("z_raw", (long) i16le(rep, 8));
                return new Report("Acceleration", f);
            }
            case GYROSCOPE -> {
                f.put("x_raw", (long) i16le(rep, 4));
                f.put("y_raw", (long) i16le(rep, 6));
                f.put("z_raw", (long) i16le(rep, 8));
                return new Report("Gyroscope", f);
            }
            case MAGNETOMETER -> {
                f.put("x_raw", (long) i16le(rep, 4));
                f.put("y_raw", (long) i16le(rep, 6));
                f.put("z_raw", (long) i16le(rep, 8));
                return new Report("Magnetometer", f);
            }
            case UNCAL_GYROSCOPE, UNCAL_MAGNETOMETER -> {
                f.put("x_raw", (long) i16le(rep, 4));
                f.put("y_raw", (long) i16le(rep, 6));
                f.put("z_raw", (long) i16le(rep, 8));
                f.put("bias_x_raw", (long) i16le(rep, 10));
                f.put("bias_y_raw", (long) i16le(rep, 12));
                f.put("bias_z_raw", (long) i16le(rep, 14));
                return new Report(
                        rid == UNCAL_GYROSCOPE ? "UncalibratedGyroscope" : "UncalibratedMagnetometer", f);
            }
            case ROTATION_VECTOR, GEOMAG_ROTATION_VECTOR, ARVR_STABILIZED_RV,
                    GAME_ROTATION_VECTOR, ARVR_STABILIZED_GAME_RV -> {
                f.put("i_raw", (long) i16le(rep, 4));
                f.put("j_raw", (long) i16le(rep, 6));
                f.put("k_raw", (long) i16le(rep, 8));
                f.put("real_raw", (long) i16le(rep, 10));
                boolean hasAccuracy = rid != GAME_ROTATION_VECTOR && rid != ARVR_STABILIZED_GAME_RV;
                f.put("accuracy_raw", hasAccuracy ? (Long) (long) i16le(rep, 12) : null);
                return new Report("RotationVector", f);
            }
            case PRESSURE, AMBIENT_LIGHT -> {
                f.put("value_raw", u32le(rep, 4));
                return new Report("ScalarReport", f);
            }
            case HUMIDITY, PROXIMITY -> {
                f.put("value_raw", (long) u16le(rep, 4));
                return new Report("ScalarReport", f);
            }
            case TEMPERATURE -> {
                f.put("value_raw", (long) i16le(rep, 4));
                return new Report("ScalarReport", f);
            }
            case TAP_DETECTOR -> {
                f.put("flags", (long) (rep[4] & 0xFF));
                return new Report("TapDetector", f);
            }
            case STEP_COUNTER -> {
                f.put("latency_us", u32le(rep, 4));
                f.put("steps", (long) u16le(rep, 8));
                return new Report("StepCounter", f);
            }
            case STEP_DETECTOR -> {
                f.put("latency_us", u32le(rep, 4));
                return new Report("StepDetector", f);
            }
            case SIGNIFICANT_MOTION -> {
                f.put("motion", (long) u16le(rep, 4));
                return new Report("SignificantMotion", f);
            }
            case STABILITY_CLASSIFIER -> {
                f.put("classification", (long) (rep[4] & 0xFF));
                return new Report("StabilityClassifier", f);
            }
            case SHAKE_DETECTOR -> {
                f.put("flags", (long) u16le(rep, 4));
                return new Report("ShakeDetector", f);
            }
            case PERSONAL_ACTIVITY_CLASSIFIER -> {
                int page = rep[4] & 0xFF;
                f.put("page_number", (long) (page & 0x7f));
                f.put("end_of_sequence", (long) ((page & 0x80) != 0 ? 1 : 0));
                f.put("most_likely_state", (long) (rep[5] & 0xFF));
                List<Long> conf = new ArrayList<>();
                for (int k = 6; k < 16; k++) {
                    conf.add((long) (rep[k] & 0xFF));
                }
                f.put("confidences", conf);
                return new Report("PersonalActivityClassifier", f);
            }
            case RAW_ACCELEROMETER, RAW_MAGNETOMETER -> {
                f.put("x_raw", (long) i16le(rep, 4));
                f.put("y_raw", (long) i16le(rep, 6));
                f.put("z_raw", (long) i16le(rep, 8));
                f.put("sensor_timestamp_us", u32le(rep, 12));
                f.put("temperature_raw", 0L);
                return new Report("RawSensor", f);
            }
            case RAW_GYROSCOPE -> {
                f.put("x_raw", (long) i16le(rep, 4));
                f.put("y_raw", (long) i16le(rep, 6));
                f.put("z_raw", (long) i16le(rep, 8));
                f.put("sensor_timestamp_us", u32le(rep, 12));
                f.put("temperature_raw", (long) i16le(rep, 10));
                return new Report("RawSensor", f);
            }
            default -> {
                f.put("value_raw", (long) u16le(rep, 4));
                return new Report("GenericEvent", f);
            }
        }
    }

    /**
     * Parse a channel-5 cargo (gyro-integrated RV, dense format). Two shapes:
     * 7×i16 bare, or prefixed with 0xFB + i32 base delta + u16 delay. Returns
     * {@code null} on a too-short buffer.
     */
    public static Report parseGyroRvCargo(byte[] payload, long captureTimestampUs) {
        long ts = captureTimestampUs;
        byte[] body = payload;
        int bodyOff = 0;
        if (payload.length >= 1 && (payload[0] & 0xFF) == BASE_TIMESTAMP_REF) {
            if (payload.length < 5 + 2 + 14) {
                return null;
            }
            long delta = i32le(payload, 1);
            int delay = u16le(payload, 5);
            ts = captureTimestampUs - delta * 100L + delay * 100L;
            bodyOff = 7;
        }
        if (body.length - bodyOff < 14) {
            return null;
        }
        Map<String, Object> f = new LinkedHashMap<>();
        f.put("sensor_id", (long) GYRO_INTEGRATED_RV);
        f.put("timestamp_us", ts);
        f.put("i_raw", (long) i16le(body, bodyOff));
        f.put("j_raw", (long) i16le(body, bodyOff + 2));
        f.put("k_raw", (long) i16le(body, bodyOff + 4));
        f.put("real_raw", (long) i16le(body, bodyOff + 6));
        f.put("vx_raw", (long) i16le(body, bodyOff + 8));
        f.put("vy_raw", (long) i16le(body, bodyOff + 10));
        f.put("vz_raw", (long) i16le(body, bodyOff + 12));
        return new Report("GyroIntegratedRV", f);
    }

    // ── LE helpers ───────────────────────────────────────────────────────────

    private static int i16le(byte[] b, int off) {
        return (short) ((b[off] & 0xFF) | ((b[off + 1] & 0xFF) << 8));
    }

    private static int u16le(byte[] b, int off) {
        return (b[off] & 0xFF) | ((b[off + 1] & 0xFF) << 8);
    }

    private static long u32le(byte[] b, int off) {
        return (b[off] & 0xFFL)
                | ((b[off + 1] & 0xFFL) << 8)
                | ((b[off + 2] & 0xFFL) << 16)
                | ((b[off + 3] & 0xFFL) << 24);
    }

    private static long i32le(byte[] b, int off) {
        return (int) u32le(b, off);
    }

    private static final char[] HEXCH = "0123456789abcdef".toCharArray();

    private static String hex(byte[] b, int off, int len) {
        StringBuilder sb = new StringBuilder(len * 2);
        for (int i = 0; i < len; i++) {
            int x = b[off + i] & 0xFF;
            sb.append(HEXCH[x >>> 4]).append(HEXCH[x & 0xF]);
        }
        return sb.toString();
    }
}
