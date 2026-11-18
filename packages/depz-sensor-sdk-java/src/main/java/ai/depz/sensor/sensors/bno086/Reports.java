package ai.depz.sensor.sensors.bno086;

import ai.depz.sensor.protocol.Wire;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import org.jspecify.annotations.Nullable;

/**
 * SH-2 input-report catalog and parsers (contract 05 §5).
 *
 * <p>Raw wire integers are authoritative (every record keeps them as its
 * {@code *Raw} components); the records derive the physical values with the
 * Q points of {@link #Q_POINTS} ({@code x()}, {@code i()}, {@code value()}, …).
 * Timestamps:
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
    /** Interactive calibration, BNO086 only. */
    public static final int MOTION_REQUEST = 0x2b;

    public static final int BASE_TIMESTAMP_REF = 0xfb;
    public static final int TIMESTAMP_REBASE = 0xfa;
    /** + u8 sensor ID, no SH-2 header; the answer to Force Sensor Flush (0xF0). */
    public static final int FLUSH_COMPLETED = 0xef;

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
        REPORT_LENGTHS.put(MOTION_REQUEST, 6);
    }

    /**
     * Base of every decoded report. {@code timestampUs} is absolute in the MCU clock domain (the
     * bridge capture time corrected by the timebase reference and the report delay); {@code sensorId}
     * is the SH-2 report id (0xEF for a {@link FlushCompleted}). Raw wire integers are authoritative:
     * every record keeps them ({@code xRaw}, {@code iRaw}, …) and derives the physical values with the
     * Q points of {@link #Q_POINTS} ({@code x()}, {@code i()}, …). One record per report kind, the
     * reference SDK's class names; a {@code switch} or {@code instanceof} pattern picks the kind.
     */
    public sealed interface Report {
        int sensorId();

        long timestampUs();

        /** The report kind — the record's simple name, the {@code type} tag of the vectors and datasets. */
        default String type() {
            return getClass().getSimpleName();
        }

        /**
         * The raw wire fields under the vector / dataset names ({@code sensor_id}, {@code timestamp_us},
         * {@code x_raw}, …): a {@link Long}, {@code null} (a missing {@code accuracy_raw}), a
         * {@code List<Long>} ({@code confidences}) or a hex {@link String} ({@code data}).
         */
        Map<String, @Nullable Object> fields();
    }

    /**
     * Channel-3/4 report with the common SH-2 header fields: {@code seq} (8-bit rolling sample counter),
     * {@code accuracy} (status bits 1:0 — 0 unreliable … 3 high) and {@code delayUs} (already folded into
     * {@code timestampUs}).
     */
    public sealed interface InputReport extends Report {
        int seq();

        int accuracy();

        long delayUs();
    }

    /** 0x01 accelerometer / 0x04 linear acceleration / 0x06 gravity (Q8, m/s²). */
    public record Acceleration(int sensorId, long timestampUs, int seq, int accuracy, long delayUs,
                               int xRaw, int yRaw, int zRaw) implements InputReport {
        public double x() {
            return scale(xRaw, 8);
        }

        public double y() {
            return scale(yRaw, 8);
        }

        public double z() {
            return scale(zRaw, 8);
        }

        @Override
        public Map<String, @Nullable Object> fields() {
            return xyz(this, xRaw, yRaw, zRaw);
        }
    }

    /** 0x02 calibrated gyroscope (Q9, rad/s). */
    public record Gyroscope(int sensorId, long timestampUs, int seq, int accuracy, long delayUs,
                            int xRaw, int yRaw, int zRaw) implements InputReport {
        public double x() {
            return scale(xRaw, 9);
        }

        public double y() {
            return scale(yRaw, 9);
        }

        public double z() {
            return scale(zRaw, 9);
        }

        @Override
        public Map<String, @Nullable Object> fields() {
            return xyz(this, xRaw, yRaw, zRaw);
        }
    }

    /** 0x03 calibrated magnetic field (Q4, µT). */
    public record Magnetometer(int sensorId, long timestampUs, int seq, int accuracy, long delayUs,
                               int xRaw, int yRaw, int zRaw) implements InputReport {
        public double x() {
            return scale(xRaw, 4);
        }

        public double y() {
            return scale(yRaw, 4);
        }

        public double z() {
            return scale(zRaw, 4);
        }

        @Override
        public Map<String, @Nullable Object> fields() {
            return xyz(this, xRaw, yRaw, zRaw);
        }
    }

    /** 0x07 uncalibrated gyroscope + bias estimate (all Q9, rad/s). */
    public record UncalibratedGyroscope(int sensorId, long timestampUs, int seq, int accuracy, long delayUs,
                                        int xRaw, int yRaw, int zRaw, int biasXRaw, int biasYRaw, int biasZRaw)
            implements InputReport {
        public double x() {
            return scale(xRaw, 9);
        }

        public double y() {
            return scale(yRaw, 9);
        }

        public double z() {
            return scale(zRaw, 9);
        }

        /** The bias estimate, rad/s. */
        public double[] bias() {
            return new double[] {scale(biasXRaw, 9), scale(biasYRaw, 9), scale(biasZRaw, 9)};
        }

        @Override
        public Map<String, @Nullable Object> fields() {
            return withBias(xyz(this, xRaw, yRaw, zRaw), biasXRaw, biasYRaw, biasZRaw);
        }
    }

    /** 0x0F uncalibrated magnetic field + hard-iron bias (all Q4, µT). */
    public record UncalibratedMagnetometer(int sensorId, long timestampUs, int seq, int accuracy, long delayUs,
                                           int xRaw, int yRaw, int zRaw, int biasXRaw, int biasYRaw, int biasZRaw)
            implements InputReport {
        public double x() {
            return scale(xRaw, 4);
        }

        public double y() {
            return scale(yRaw, 4);
        }

        public double z() {
            return scale(zRaw, 4);
        }

        /** The hard-iron bias, µT. */
        public double[] bias() {
            return new double[] {scale(biasXRaw, 4), scale(biasYRaw, 4), scale(biasZRaw, 4)};
        }

        @Override
        public Map<String, @Nullable Object> fields() {
            return withBias(xyz(this, xRaw, yRaw, zRaw), biasXRaw, biasYRaw, biasZRaw);
        }
    }

    /**
     * Quaternion reports 0x05 / 0x08 / 0x09 / 0x28 / 0x29 (unit quaternion, Q14). {@code accuracyRaw}
     * (Q12, radians) is present only for 0x05 / 0x09 / 0x28 — {@code null} for the game variants.
     */
    public record RotationVector(int sensorId, long timestampUs, int seq, int accuracy, long delayUs,
                                 int iRaw, int jRaw, int kRaw, int realRaw, @Nullable Integer accuracyRaw)
            implements InputReport {
        public double i() {
            return scale(iRaw, 14);
        }

        public double j() {
            return scale(jRaw, 14);
        }

        public double k() {
            return scale(kRaw, 14);
        }

        public double real() {
            return scale(realRaw, 14);
        }

        /** Estimated heading accuracy, radians ({@code null} for the game variants). */
        public @Nullable Double accuracyRad() {
            return accuracyRaw == null ? null : scale(accuracyRaw, RV_ACCURACY_Q);
        }

        @Override
        public Map<String, @Nullable Object> fields() {
            Map<String, Object> f = quaternion(head(this), iRaw, jRaw, kRaw, realRaw);
            f.put("accuracy_raw", accuracyRaw == null ? null : (Long) (long) accuracyRaw);
            return f;
        }
    }

    /** Environment reports 0x0A–0x0E: one value, Q from {@link #Q_POINTS} (hPa, lux, %, cm, °C). */
    public record ScalarReport(int sensorId, long timestampUs, int seq, int accuracy, long delayUs, long valueRaw)
            implements InputReport {
        public double value() {
            return scale(valueRaw, Q_POINTS.get(sensorId));
        }

        @Override
        public Map<String, @Nullable Object> fields() {
            Map<String, Object> f = head(this);
            f.put("value_raw", valueRaw);
            return f;
        }
    }

    /** 0x10 tap detector; {@code flags} bit 6 = double tap, bits 0–5 axis / sign. */
    public record TapDetector(int sensorId, long timestampUs, int seq, int accuracy, long delayUs, int flags)
            implements InputReport {
        public boolean doubleTap() {
            return (flags & 0x40) != 0;
        }

        @Override
        public Map<String, @Nullable Object> fields() {
            Map<String, Object> f = head(this);
            f.put("flags", (long) flags);
            return f;
        }
    }

    /** 0x11 step counter: the latency from the step to the report (µs) and the count. */
    public record StepCounter(int sensorId, long timestampUs, int seq, int accuracy, long delayUs,
                              long latencyUs, int steps) implements InputReport {
        @Override
        public Map<String, @Nullable Object> fields() {
            Map<String, Object> f = head(this);
            f.put("latency_us", latencyUs);
            f.put("steps", (long) steps);
            return f;
        }
    }

    /** 0x18 step detector; the latency from the step event to the report, µs. */
    public record StepDetector(int sensorId, long timestampUs, int seq, int accuracy, long delayUs, long latencyUs)
            implements InputReport {
        @Override
        public Map<String, @Nullable Object> fields() {
            Map<String, Object> f = head(this);
            f.put("latency_us", latencyUs);
            return f;
        }
    }

    /** 0x12 significant motion (1 = motion detected; the sensor disables itself). */
    public record SignificantMotion(int sensorId, long timestampUs, int seq, int accuracy, long delayUs, int motion)
            implements InputReport {
        @Override
        public Map<String, @Nullable Object> fields() {
            Map<String, Object> f = head(this);
            f.put("motion", (long) motion);
            return f;
        }
    }

    /** 0x13 stability classification (0 unknown, 1 on table, 2 stationary, 3 stable, 4 motion). */
    public record StabilityClassifier(int sensorId, long timestampUs, int seq, int accuracy, long delayUs,
                                      int classification) implements InputReport {
        @Override
        public Map<String, @Nullable Object> fields() {
            Map<String, Object> f = head(this);
            f.put("classification", (long) classification);
            return f;
        }
    }

    /**
     * 0x2B motion request (BNO086 interactive calibration): the motion {@code intent} the hub was last
     * told, and what it asks of the device (SH-2 §6.5.45.2; {@code request} 1, 3, 4 = stay still). Sent
     * when the request changes, not periodically, whatever the granted rate.
     */
    public record MotionRequest(int sensorId, long timestampUs, int seq, int accuracy, long delayUs,
                                int intent, int request) implements InputReport {
        /** True when the hub asks the device to stop (the deprecated optional request 2 does not count). */
        public boolean wantsStillness() {
            return request == 1 || request == 3 || request == 4;
        }

        @Override
        public Map<String, @Nullable Object> fields() {
            Map<String, Object> f = head(this);
            f.put("intent", (long) intent);
            f.put("request", (long) request);
            return f;
        }
    }

    /** 0x19 shake detector; {@code flags} bits 0 / 1 / 2 = X / Y / Z shake. */
    public record ShakeDetector(int sensorId, long timestampUs, int seq, int accuracy, long delayUs, int flags)
            implements InputReport {
        @Override
        public Map<String, @Nullable Object> fields() {
            Map<String, Object> f = head(this);
            f.put("flags", (long) flags);
            return f;
        }
    }

    /**
     * The simple u16-value detectors: 0x1A flip, 0x1B pickup, 0x1C stability detector, 0x1F sleep,
     * 0x20 tilt, 0x21 pocket, 0x22 circle, 0x23 heart rate.
     */
    public record GenericEvent(int sensorId, long timestampUs, int seq, int accuracy, long delayUs, int valueRaw)
            implements InputReport {
        @Override
        public Map<String, @Nullable Object> fields() {
            Map<String, Object> f = head(this);
            f.put("value_raw", (long) valueRaw);
            return f;
        }
    }

    /**
     * 0x1E personal activity classifier: {@code confidences} are 0–100 per state, states 0–9 of the
     * current page; {@code endOfSequence} is the page byte's bit 7.
     */
    public record PersonalActivityClassifier(int sensorId, long timestampUs, int seq, int accuracy, long delayUs,
                                             int pageNumber, boolean endOfSequence, int mostLikelyState,
                                             int[] confidences) implements InputReport {
        @Override
        public Map<String, @Nullable Object> fields() {
            Map<String, Object> f = head(this);
            f.put("page_number", (long) pageNumber);
            f.put("end_of_sequence", endOfSequence ? 1L : 0L);
            f.put("most_likely_state", (long) mostLikelyState);
            List<Long> conf = new ArrayList<>(confidences.length);
            for (int c : confidences) {
                conf.add((long) c);
            }
            f.put("confidences", conf);
            return f;
        }
    }

    /**
     * 0x14 / 0x15 / 0x16 raw ADC samples + the sensor-clock timestamp (u32 µs). {@code temperatureRaw}
     * is populated only for the raw gyroscope (0 otherwise).
     */
    public record RawSensor(int sensorId, long timestampUs, int seq, int accuracy, long delayUs,
                            int xRaw, int yRaw, int zRaw, long sensorTimestampUs, int temperatureRaw)
            implements InputReport {
        @Override
        public Map<String, @Nullable Object> fields() {
            Map<String, Object> f = xyz(this, xRaw, yRaw, zRaw);
            f.put("sensor_timestamp_us", sensorTimestampUs);
            f.put("temperature_raw", (long) temperatureRaw);
            return f;
        }
    }

    /**
     * 0x2A gyro-integrated rotation vector (channel 5, dense — no SH-2 header). Quaternion Q14, angular
     * velocity Q10 rad/s.
     */
    public record GyroIntegratedRV(int sensorId, long timestampUs,
                                   int iRaw, int jRaw, int kRaw, int realRaw, int vxRaw, int vyRaw, int vzRaw)
            implements Report {
        public double i() {
            return scale(iRaw, 14);
        }

        public double j() {
            return scale(jRaw, 14);
        }

        public double k() {
            return scale(kRaw, 14);
        }

        public double real() {
            return scale(realRaw, 14);
        }

        /** Angular velocity, rad/s. */
        public double[] angularVelocity() {
            return new double[] {
                scale(vxRaw, GYRO_RV_ANGVEL_Q), scale(vyRaw, GYRO_RV_ANGVEL_Q), scale(vzRaw, GYRO_RV_ANGVEL_Q)};
        }

        @Override
        public Map<String, @Nullable Object> fields() {
            Map<String, Object> f = quaternion(head(this), iRaw, jRaw, kRaw, realRaw);
            f.put("vx_raw", (long) vxRaw);
            f.put("vy_raw", (long) vyRaw);
            f.put("vz_raw", (long) vzRaw);
            return f;
        }
    }

    /**
     * 0xEF Flush Completed: every batched report of {@code flushedSensor} as of the matching Force
     * Sensor Flush has been sent. {@code sensorId} is 0xEF; no SH-2 header, so the timestamp is the
     * cargo's current base.
     */
    public record FlushCompleted(int sensorId, long timestampUs, int flushedSensor) implements Report {
        @Override
        public Map<String, @Nullable Object> fields() {
            Map<String, Object> f = head(this);
            f.put("flushed_sensor", (long) flushedSensor);
            return f;
        }
    }

    /**
     * An unrecognised or truncated report id: the raw bytes from the id to the end of the cargo (the
     * length is unknowable, so parsing stops here).
     */
    public record UnknownReport(int sensorId, long timestampUs, byte[] data) implements Report {
        @Override
        public Map<String, @Nullable Object> fields() {
            Map<String, Object> f = head(this);
            f.put("data", hex(data, 0, data.length));
            return f;
        }
    }

    // ── the fields() maps ────────────────────────────────────────────────────

    private static Map<String, Object> head(Report r) {
        Map<String, Object> f = new LinkedHashMap<>();
        f.put("sensor_id", (long) r.sensorId());
        f.put("timestamp_us", r.timestampUs());
        return f;
    }

    private static Map<String, Object> head(InputReport r) {
        Map<String, Object> f = head((Report) r);
        f.put("seq", (long) r.seq());
        f.put("accuracy", (long) r.accuracy());
        f.put("delay_us", r.delayUs());
        return f;
    }

    private static Map<String, Object> xyz(InputReport r, int x, int y, int z) {
        Map<String, Object> f = head(r);
        f.put("x_raw", (long) x);
        f.put("y_raw", (long) y);
        f.put("z_raw", (long) z);
        return f;
    }

    private static Map<String, Object> withBias(Map<String, Object> f, int x, int y, int z) {
        f.put("bias_x_raw", (long) x);
        f.put("bias_y_raw", (long) y);
        f.put("bias_z_raw", (long) z);
        return f;
    }

    private static Map<String, Object> quaternion(Map<String, Object> f, int i, int j, int k, int real) {
        f.put("i_raw", (long) i);
        f.put("j_raw", (long) j);
        f.put("k_raw", (long) k);
        f.put("real_raw", (long) real);
        return f;
    }

    /** The Q points of the primary fields by sensor id (value = raw / 2^Q), SH-2 reference driver. */
    public static final Map<Integer, Integer> Q_POINTS = Map.ofEntries(
        Map.entry(ACCELEROMETER, 8), Map.entry(LINEAR_ACCELERATION, 8), Map.entry(GRAVITY, 8),
        Map.entry(GYROSCOPE, 9), Map.entry(UNCAL_GYROSCOPE, 9),
        Map.entry(MAGNETOMETER, 4), Map.entry(UNCAL_MAGNETOMETER, 4),
        Map.entry(ROTATION_VECTOR, 14), Map.entry(GAME_ROTATION_VECTOR, 14), Map.entry(GEOMAG_ROTATION_VECTOR, 14),
        Map.entry(ARVR_STABILIZED_RV, 14), Map.entry(ARVR_STABILIZED_GAME_RV, 14), Map.entry(GYRO_INTEGRATED_RV, 14),
        Map.entry(PRESSURE, 20), Map.entry(AMBIENT_LIGHT, 8), Map.entry(HUMIDITY, 8), Map.entry(PROXIMITY, 4),
        Map.entry(TEMPERATURE, 7),
        Map.entry(RAW_ACCELEROMETER, 0), Map.entry(RAW_GYROSCOPE, 0), Map.entry(RAW_MAGNETOMETER, 0));

    /** The rotation-vector accuracy estimate, radians. */
    public static final int RV_ACCURACY_Q = 12;

    /** The gyro-integrated rotation vector's angular velocity, rad/s. */
    public static final int GYRO_RV_ANGVEL_Q = 10;

    /** raw / 2^q (exact in a double). */
    public static double scale(long raw, int q) {
        return raw / (double) (1L << q);
    }

    /**
     * Parse a channel-3/4 cargo into typed reports. {@code captureTimestampUs}
     * is the bridge RPT_DATA capture time (MCU uptime, a u64: a negative one
     * is an IllegalArgumentException).
     */
    public static List<Report> parseInputCargo(byte[] payload, long captureTimestampUs) {
        Wire.checkUint("captureTimestampUs", captureTimestampUs, 63);
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
            if (rid == FLUSH_COMPLETED && pos + 2 <= n) {
                // 0xEF Flush Completed: every batched report of flushed_sensor
                // has been sent. No SH-2 header: stamped with the current base.
                out.add(new FlushCompleted(rid, baseUs, payload[pos + 1] & 0xFF));
                pos += 2;
                continue;
            }
            Integer length = REPORT_LENGTHS.get(rid);
            if (length == null || pos + length > n) {
                out.add(new UnknownReport(rid, baseUs, Arrays.copyOfRange(payload, pos, n)));
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

    private static InputReport decodeReport(int rid, byte[] rep, long ts, int seq, int accuracy, int delayUs) {
        switch (rid) {
            case ACCELEROMETER, LINEAR_ACCELERATION, GRAVITY:
                return new Acceleration(rid, ts, seq, accuracy, delayUs, i16le(rep, 4), i16le(rep, 6), i16le(rep, 8));
            case GYROSCOPE:
                return new Gyroscope(rid, ts, seq, accuracy, delayUs, i16le(rep, 4), i16le(rep, 6), i16le(rep, 8));
            case MAGNETOMETER:
                return new Magnetometer(rid, ts, seq, accuracy, delayUs, i16le(rep, 4), i16le(rep, 6), i16le(rep, 8));
            case UNCAL_GYROSCOPE:
                return new UncalibratedGyroscope(rid, ts, seq, accuracy, delayUs,
                        i16le(rep, 4), i16le(rep, 6), i16le(rep, 8), i16le(rep, 10), i16le(rep, 12), i16le(rep, 14));
            case UNCAL_MAGNETOMETER:
                return new UncalibratedMagnetometer(rid, ts, seq, accuracy, delayUs,
                        i16le(rep, 4), i16le(rep, 6), i16le(rep, 8), i16le(rep, 10), i16le(rep, 12), i16le(rep, 14));
            case ROTATION_VECTOR, GEOMAG_ROTATION_VECTOR, ARVR_STABILIZED_RV:
                return new RotationVector(rid, ts, seq, accuracy, delayUs,
                        i16le(rep, 4), i16le(rep, 6), i16le(rep, 8), i16le(rep, 10), i16le(rep, 12));
            case GAME_ROTATION_VECTOR, ARVR_STABILIZED_GAME_RV:
                return new RotationVector(rid, ts, seq, accuracy, delayUs,
                        i16le(rep, 4), i16le(rep, 6), i16le(rep, 8), i16le(rep, 10), null);
            case PRESSURE, AMBIENT_LIGHT:
                return new ScalarReport(rid, ts, seq, accuracy, delayUs, u32le(rep, 4));
            case HUMIDITY, PROXIMITY:
                return new ScalarReport(rid, ts, seq, accuracy, delayUs, u16le(rep, 4));
            case TEMPERATURE:
                return new ScalarReport(rid, ts, seq, accuracy, delayUs, i16le(rep, 4));
            case TAP_DETECTOR:
                return new TapDetector(rid, ts, seq, accuracy, delayUs, rep[4] & 0xFF);
            case STEP_COUNTER:
                return new StepCounter(rid, ts, seq, accuracy, delayUs, u32le(rep, 4), u16le(rep, 8));
            case STEP_DETECTOR:
                return new StepDetector(rid, ts, seq, accuracy, delayUs, u32le(rep, 4));
            case SIGNIFICANT_MOTION:
                return new SignificantMotion(rid, ts, seq, accuracy, delayUs, u16le(rep, 4));
            case STABILITY_CLASSIFIER:
                return new StabilityClassifier(rid, ts, seq, accuracy, delayUs, rep[4] & 0xFF);
            case MOTION_REQUEST:
                return new MotionRequest(rid, ts, seq, accuracy, delayUs, rep[4] & 0xFF, rep[5] & 0xFF);
            case SHAKE_DETECTOR:
                return new ShakeDetector(rid, ts, seq, accuracy, delayUs, u16le(rep, 4));
            case PERSONAL_ACTIVITY_CLASSIFIER: {
                int page = rep[4] & 0xFF;
                int[] confidences = new int[10];
                for (int k = 0; k < 10; k++) {
                    confidences[k] = rep[6 + k] & 0xFF;
                }
                return new PersonalActivityClassifier(rid, ts, seq, accuracy, delayUs,
                        page & 0x7f, (page & 0x80) != 0, rep[5] & 0xFF, confidences);
            }
            case RAW_ACCELEROMETER, RAW_MAGNETOMETER:
                return new RawSensor(rid, ts, seq, accuracy, delayUs,
                        i16le(rep, 4), i16le(rep, 6), i16le(rep, 8), u32le(rep, 12), 0);
            case RAW_GYROSCOPE:
                return new RawSensor(rid, ts, seq, accuracy, delayUs,
                        i16le(rep, 4), i16le(rep, 6), i16le(rep, 8), u32le(rep, 12), i16le(rep, 10));
            default:
                return new GenericEvent(rid, ts, seq, accuracy, delayUs, u16le(rep, 4));
        }
    }

    /**
     * Parse a channel-5 cargo (gyro-integrated RV, dense format). Two shapes:
     * 7×i16 bare, or prefixed with 0xFB + i32 base delta + u16 delay. Returns
     * {@code null} on a too-short buffer; a negative {@code captureTimestampUs}
     * is an IllegalArgumentException.
     */
    public static @Nullable GyroIntegratedRV parseGyroRvCargo(byte[] payload, long captureTimestampUs) {
        Wire.checkUint("captureTimestampUs", captureTimestampUs, 63);
        long ts = captureTimestampUs;
        int off = 0;
        if (payload.length >= 1 && (payload[0] & 0xFF) == BASE_TIMESTAMP_REF) {
            if (payload.length < 5 + 2 + 14) {
                return null;
            }
            long delta = i32le(payload, 1);
            int delay = u16le(payload, 5);
            ts = captureTimestampUs - delta * 100L + delay * 100L;
            off = 7;
        }
        if (payload.length - off < 14) {
            return null;
        }
        return new GyroIntegratedRV(GYRO_INTEGRATED_RV, ts,
                i16le(payload, off), i16le(payload, off + 2), i16le(payload, off + 4), i16le(payload, off + 6),
                i16le(payload, off + 8), i16le(payload, off + 10), i16le(payload, off + 12));
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
