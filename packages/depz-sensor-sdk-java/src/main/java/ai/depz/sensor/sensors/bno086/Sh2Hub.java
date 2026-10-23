package ai.depz.sensor.sensors.bno086;

import ai.depz.sensor.protocol.Wire;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;

/**
 * SH-2 hub protocol (contract 05 §6): the control-response decoders, the request parameters, the
 * FRS sessions and the control-cargo walk the device layer needs on top of {@link Sh2}. Ported from
 * the reference {@code sh2.py}.
 */
public final class Sh2Hub {
    private Sh2Hub() {}

    /** Host → hub on the control channel; answered by Flush Completed (0xEF) on the input channel. */
    public static final int FORCE_SENSOR_FLUSH = 0xF0;

    // ── SHTP channels ────────────────────────────────────────────────────────
    public static final int CHANNEL_COMMAND = 0;
    public static final int CHANNEL_CONTROL = 2;
    public static final int CHANNEL_INPUT_NORMAL = 3;
    public static final int CHANNEL_INPUT_WAKE = 4;
    public static final int CHANNEL_GYRO_RV = 5;
    /** Inbound on the executable channel: the hub finished a reset. */
    public static final int EXECUTABLE_RESET_COMPLETE = 0x01;

    // ── Command Request / Response commands (0xF2 / 0xF1) ───────────────────
    public static final int CMD_ERRORS = 0x01;
    public static final int CMD_COUNTER = 0x02;
    public static final int CMD_TARE = 0x03;
    public static final int CMD_INITIALIZE = 0x04;
    public static final int CMD_SAVE_DCD = 0x06;
    public static final int CMD_ME_CALIBRATE = 0x07;
    public static final int CMD_PERIODIC_DCD_CONFIG = 0x09;
    public static final int CMD_GET_OSCILLATOR_TYPE = 0x0A;
    public static final int CMD_CLEAR_DCD_AND_RESET = 0x0B;
    /** BNO086 only; no response. */
    public static final int CMD_INTERACTIVE_CALIBRATION = 0x0E;

    // Get Oscillator Type R0.
    public static final int OSC_INTERNAL = 0;
    public static final int OSC_EXT_CRYSTAL = 1;
    public static final int OSC_EXT_CLOCK = 2;

    // The source of an error record (SH-2 §6.4.1).
    public static final int ERROR_SOURCE_MOTION_ENGINE = 1;
    public static final int ERROR_SOURCE_MOTION_HUB = 2;
    public static final int ERROR_SOURCE_SENSOR_HUB = 3;
    public static final int ERROR_SOURCE_CHIP = 4;
    /** The end of the error queue. */
    public static final int ERROR_SOURCE_NO_MORE_ERRORS = 255;

    // Tare axes, a bitmap.
    public static final int TARE_X = 1;
    public static final int TARE_Y = 2;
    public static final int TARE_Z = 4;
    public static final int TARE_ALL = 7;

    /** The rotation vector a tare refers to (Tare Now P2). */
    public enum TareBasis {
        ROTATION_VECTOR(0), GAME_ROTATION_VECTOR(1), GEOMAGNETIC_ROTATION_VECTOR(2), GYRO_INTEGRATED_RV(3),
        ARVR_STABILIZED_RV(4), ARVR_STABILIZED_GAME_RV(5);

        public final int value;
        TareBasis(int value) { this.value = value; }
    }

    /**
     * P0 of Interactive Calibration (0x0E, SH-2 §6.4.12.1): what the host knows about the device's
     * motion. UNKNOWN is the hub's initial state.
     */
    public enum MotionIntent {
        UNKNOWN(0), STATIONARY_NO_VIBRATION(1), STATIONARY_WITH_VIBRATION(2), IN_MOTION(3), IN_MOTION_ACCELERATING(4);

        public final int value;
        MotionIntent(int value) { this.value = value; }
    }

    // ── FRS ──────────────────────────────────────────────────────────────────

    /** FRS record ids (SH-2 figure 28). */
    public static final int FRS_STATIC_CALIBRATION_AGM = 0x7979;
    public static final int FRS_NOMINAL_CALIBRATION = 0x4D4D;
    public static final int FRS_DYNAMIC_CALIBRATION = 0x1F1F;
    public static final int FRS_ME_POWER_MGMT = 0xD3E2;
    /** Mounting quaternion, 4 × Q30 words. */
    public static final int FRS_SYSTEM_ORIENTATION = 0x2D3E;
    public static final int FRS_ACCEL_ORIENTATION = 0x2D41;
    public static final int FRS_GYROSCOPE_ORIENTATION = 0x2D46;
    public static final int FRS_MAGNETOMETER_ORIENTATION = 0x2D4C;
    public static final int FRS_ARVR_STABILIZATION_RV = 0x3E2D;
    public static final int FRS_ARVR_STABILIZATION_GRV = 0x3E2E;
    /** One 32-bit word; empty unless programmed. */
    public static final int FRS_SERIAL_NUMBER = 0x4B4B;
    /** Free for the user, up to 64 words. */
    public static final int FRS_USER_RECORD = 0x74B4;
    public static final int FRS_SIG_MOTION_DETECT_CONFIG = 0xC274;
    public static final int FRS_SHAKE_DETECT_CONFIG = 0x7D7D;
    public static final int FRS_STABILITY_DETECTOR_CONFIG = 0xED85;
    public static final int FRS_ACTIVITY_TRACKER_CONFIG = 0xED88;

    /** Sensor id → its metadata FRS record. */
    public static final Map<Integer, Integer> METADATA_RECORDS = Map.ofEntries(
            Map.entry(0x14, 0xE301), Map.entry(0x01, 0xE302), Map.entry(0x04, 0xE303), Map.entry(0x06, 0xE304),
            Map.entry(0x15, 0xE305), Map.entry(0x02, 0xE306), Map.entry(0x07, 0xE307), Map.entry(0x16, 0xE308),
            Map.entry(0x03, 0xE309), Map.entry(0x0F, 0xE30A), Map.entry(0x05, 0xE30B), Map.entry(0x08, 0xE30C),
            Map.entry(0x09, 0xE30D), Map.entry(0x10, 0xE313), Map.entry(0x18, 0xE314), Map.entry(0x11, 0xE315),
            Map.entry(0x12, 0xE316), Map.entry(0x13, 0xE317), Map.entry(0x19, 0xE318), Map.entry(0x1E, 0xE31C),
            Map.entry(0x28, 0xE322), Map.entry(0x29, 0xE323), Map.entry(0x2A, 0xE324), Map.entry(0x2B, 0xE325));

    // FRS Read Response status.
    public static final int FRS_NO_ERROR = 0;
    public static final int FRS_UNRECOGNIZED_FRS_TYPE = 1;
    public static final int FRS_BUSY = 2;
    public static final int FRS_READ_COMPLETED = 3;
    public static final int FRS_OFFSET_OUT_OF_RANGE = 4;
    public static final int FRS_RECORD_EMPTY = 5;
    public static final int FRS_BLOCK_COMPLETED = 6;
    public static final int FRS_BLOCK_AND_READ_COMPLETED = 7;
    public static final int FRS_DEVICE_ERROR = 8;

    // FRS Write Response status.
    public static final int FRS_WRITE_WORDS_RECEIVED = 0;
    public static final int FRS_WRITE_UNRECOGNIZED_FRS_TYPE = 1;
    public static final int FRS_WRITE_BUSY = 2;
    public static final int FRS_WRITE_COMPLETED = 3;
    public static final int FRS_WRITE_MODE_READY = 4;
    public static final int FRS_WRITE_FAILED = 5;
    public static final int FRS_WRITE_NOT_IN_WRITE_MODE = 6;
    public static final int FRS_WRITE_INVALID_LENGTH = 7;
    public static final int FRS_WRITE_RECORD_VALID = 8;
    public static final int FRS_WRITE_RECORD_INVALID = 9;

    /** A SH-2 level failure: a non-zero command status, an FRS error. */
    public static final class Sh2Exception extends RuntimeException {
        public Sh2Exception(String message) {
            super(message);
        }
    }

    // ── control-response decoders ────────────────────────────────────────────

    /** Get Feature Response (0xFC), 17 bytes: the rates in effect. */
    public record FeatureResponse(int sensorId, int flags, int sensitivity, long intervalUs, long batchUs, long cfgWord) {
        public static FeatureResponse unpack(byte[] p) {
            Wire.needBytes("Get Feature Response", p, 17);
            expectId(p, Sh2.GET_FEATURE_RESPONSE, "get-feature");
            return new FeatureResponse(p[1] & 0xFF, p[2] & 0xFF, u16(p, 3), u32(p, 5), u32(p, 9), u32(p, 13));
        }
    }

    /**
     * Product ID Response (0xF8), 16 bytes, one per software component (the SH-2 application
     * first); the reset cause (SH-2 §6.4.5.2) is set on the first only.
     */
    public record ProductId(int resetCause, int swVersionMajor, int swVersionMinor, long swPartNumber,
            long swBuildNumber, int swVersionPatch) {
        public String version() {
            return swVersionMajor + "." + swVersionMinor + "." + swVersionPatch;
        }

        public static ProductId unpack(byte[] p) {
            Wire.needBytes("Product ID Response", p, 14);
            expectId(p, Sh2.PRODUCT_ID_RESPONSE, "product-id");
            return new ProductId(p[1] & 0xFF, p[2] & 0xFF, p[3] & 0xFF, u32(p, 4), u32(p, 8), u16(p, 12));
        }
    }

    /**
     * Command Response (0xF1), 16 bytes. {@code commandSeq} echoes the request (correlate on it plus
     * {@code command}); {@code responseSeq} counts responses to one request; R0 is the status of most
     * commands (0 = success).
     */
    public record CommandResponse(int seq, int command, int commandSeq, int responseSeq, byte[] r) {
        public int status() {
            return r[0] & 0xFF;
        }

        /** R{@code i} as an unsigned byte. */
        public int r(int i) {
            return r[i] & 0xFF;
        }

        public static CommandResponse unpack(byte[] p) {
            Wire.needBytes("Command Response", p, 16);
            expectId(p, Sh2.COMMAND_RESPONSE, "command");
            return new CommandResponse(p[1] & 0xFF, p[2] & 0xFF, p[3] & 0xFF, p[4] & 0xFF, Arrays.copyOfRange(p, 5, 16));
        }
    }

    /** FRS Read Response (0xF3), 16 bytes, up to two data words. */
    public record FrsReadResponse(int status, int dataLength, int offsetWords, long data0, long data1, int frsType) {
        public static FrsReadResponse unpack(byte[] p) {
            Wire.needBytes("FRS Read Response", p, 16);
            expectId(p, Sh2.FRS_READ_RESPONSE, "FRS read");
            return new FrsReadResponse(p[1] & 0x0F, (p[1] & 0xFF) >> 4, u16(p, 2), u32(p, 4), u32(p, 8), u16(p, 12));
        }
    }

    /** FRS Write Response (0xF5), 4 bytes. */
    public record FrsWriteResponse(int status, int offsetWords) {
        public static FrsWriteResponse unpack(byte[] p) {
            Wire.needBytes("FRS Write Response", p, 4);
            expectId(p, Sh2.FRS_WRITE_RESPONSE, "FRS write");
            return new FrsWriteResponse(p[1] & 0xFF, u16(p, 2));
        }
    }

    /** One error-queue entry (command 0x01 response, R0..R5). */
    public record ErrorRecord(int severity, int seq, int source, int error, int module, int code) {
        public static ErrorRecord from(CommandResponse r) {
            return new ErrorRecord(r.r(0), r.r(1), r.r(2), r.r(3), r.r(4), r.r(5));
        }
    }

    /** Per-sensor event counts (command 0x02, two responses). */
    public record Counts(int sensorId, long offered, long accepted, long on, long attempted) {}

    /** ME calibration enables as the sensor reports them. */
    public record CalibrationConfig(boolean accel, boolean gyro, boolean mag, boolean planar) {}

    /**
     * A sensor's metadata FRS record (revision-gated fields are 0 on older records);
     * {@code rawWords} is authoritative.
     */
    public record SensorMetadata(int meVersion, int mhVersion, int shVersion, long rangeRaw, long resolutionRaw,
            int revision, int powerMaQ10, long minPeriodUs, long maxPeriodUs, int fifoMax, int fifoReserved,
            int batchBufferBytes, int qPoint1, int qPoint2, int qPoint3, long[] rawWords) {

        public static SensorMetadata fromWords(long[] words) {
            long[] w = Arrays.copyOf(words, Math.max(10, words.length));
            // Word 3 is power_mA u16 (Q10) | revision u16 << 16.
            int revision = (int) (w[3] >>> 16);
            return new SensorMetadata(
                    (int) (w[0] & 0xFF), (int) (w[0] >>> 8 & 0xFF), (int) (w[0] >>> 16 & 0xFF),
                    w[1], w[2], revision, (int) (w[3] & 0xFFFF), w[4], revision >= 4 ? w[9] : 0,
                    (int) (w[5] & 0xFFFF), (int) (w[5] >>> 16), (int) (w[6] & 0xFFFF),
                    (int) (w[7] & 0xFFFF), (int) (w[7] >>> 16), revision >= 3 ? (int) (w[8] >>> 16) : 0,
                    words.clone());
        }

        /** The fields under the reference SDK's names. */
        public Map<String, Object> fields() {
            Map<String, Object> f = new LinkedHashMap<>();
            f.put("me_version", meVersion);
            f.put("mh_version", mhVersion);
            f.put("sh_version", shVersion);
            f.put("range_raw", rangeRaw);
            f.put("resolution_raw", resolutionRaw);
            f.put("revision", revision);
            f.put("power_ma_q10", powerMaQ10);
            f.put("min_period_us", minPeriodUs);
            f.put("max_period_us", maxPeriodUs);
            f.put("fifo_max", fifoMax);
            f.put("fifo_reserved", fifoReserved);
            f.put("batch_buffer_bytes", batchBufferBytes);
            f.put("q_point_1", qPoint1);
            f.put("q_point_2", qPoint2);
            f.put("q_point_3", qPoint3);
            f.put("raw_words", rawWords.clone());
            return f;
        }
    }

    // ── request parameters ───────────────────────────────────────────────────

    public static final int COUNTS_GET = 0;
    public static final int COUNTS_CLEAR = 1;
    /** ME Calibration subcommand: report the current configuration. */
    public static final int ME_CAL_GET = 0x01;

    // Fixed lengths of the sensor → host control reports; one cargo may hold several.
    private static final Map<Integer, Integer> CONTROL_RESPONSE_LENGTHS = Map.of(
            Sh2.COMMAND_RESPONSE, 16,
            Sh2.FRS_READ_RESPONSE, 16,
            Sh2.FRS_WRITE_RESPONSE, 4,
            Sh2.PRODUCT_ID_RESPONSE, 16,
            Sh2.GET_FEATURE_RESPONSE, 17);

    /**
     * Split a control cargo into its reports; an unknown id or a short tail ends the walk (the rest
     * is one piece: its length is unknowable).
     */
    public static List<byte[]> splitControlCargo(byte[] payload) {
        List<byte[]> out = new ArrayList<>();
        int pos = 0;
        while (pos < payload.length) {
            Integer n = CONTROL_RESPONSE_LENGTHS.get(payload[pos] & 0xFF);
            if (n == null || pos + n > payload.length) {
                out.add(Arrays.copyOfRange(payload, pos, payload.length));
                break;
            }
            out.add(Arrays.copyOfRange(payload, pos, pos + n));
            pos += n;
        }
        return out;
    }

    public static byte[] countsGetParams(int sensorId) {
        return new byte[] {(byte) COUNTS_GET, u8(sensorId, "sensorId")};
    }

    public static byte[] countsClearParams(int sensorId) {
        return new byte[] {(byte) COUNTS_CLEAR, u8(sensorId, "sensorId")};
    }

    /** Errors of {@code severity} or greater (0 = all). */
    public static byte[] errorsParams(int severity) {
        return new byte[] {u8(severity, "severity")};
    }

    /** {@code axes}: a {@code TARE_*} bitmap. */
    public static byte[] tareNowParams(int axes, TareBasis basis) {
        return new byte[] {0x00, u8(axes, "axes"), (byte) basis.value};
    }

    public static byte[] persistTareParams() {
        return new byte[] {0x01};
    }

    /** Tare subcommand 2: the reorientation quaternion as four Q14 int16 (all zero clears). */
    public static byte[] setReorientationParams(double x, double y, double z, double w) {
        byte[] p = new byte[9];
        p[0] = 0x02;
        double[] q = {x, y, z, w};
        for (int i = 0; i < 4; i++) {
            double v = Math.rint(q[i] * (1 << 14)); // Python round(): half to even
            if (Double.isNaN(v) || v < Short.MIN_VALUE || v > Short.MAX_VALUE) {
                throw new IllegalArgumentException("quaternion component " + q[i] + " out of Q14 int16 range");
            }
            int s = (int) v;
            p[1 + 2 * i] = (byte) s;
            p[2 + 2 * i] = (byte) (s >> 8);
        }
        return p;
    }

    /** ME Calibration (0x07): subcommand 0 configures, 1 ({@link #ME_CAL_GET}) reads back. */
    public static byte[] meCalibrationParams(boolean accel, boolean gyro, boolean mag, boolean planar, int subcommand) {
        return new byte[] {b(accel), b(gyro), b(mag), u8(subcommand, "subcommand"), b(planar)};
    }

    /** Interactive Calibration (0x0E, BNO086 only): P0 = motion intent; no response. */
    public static byte[] interactiveCalibrationParams(int intent) {
        return new byte[] {u8(intent, "intent")};
    }

    /** Force Sensor Flush (0xF0, SH-2 §6.5.6); the hub answers with Flush Completed (0xEF). */
    public static byte[] forceFlush(int sensorId) {
        return new byte[] {(byte) FORCE_SENSOR_FLUSH, u8(sensorId, "sensorId")};
    }

    /** Periodic DCD save (0x09): P0 0 = enable, 1 = disable; no response. */
    public static byte[] periodicDcdParams(boolean enable) {
        return new byte[] {(byte) (enable ? 0x00 : 0x01)};
    }

    // ── FRS sessions ─────────────────────────────────────────────────────────

    /**
     * A multi-packet FRS read: send {@link #request()}, {@link #feed} every 0xF3 of the record until
     * it returns true; {@link #words()} holds the record. Error statuses throw {@link Sh2Exception}.
     */
    public static final class FrsReadSession {
        private final int frsType;
        private final List<Long> words = new ArrayList<>();
        private boolean done;

        public FrsReadSession(int frsType) {
            this.frsType = frsType;
        }

        public int frsType() {
            return frsType;
        }

        public boolean done() {
            return done;
        }

        public long[] words() {
            return words.stream().mapToLong(Long::longValue).toArray();
        }

        public byte[] request() {
            return Sh2.buildFrsReadRequest(frsType, 0, 0);
        }

        public boolean feed(FrsReadResponse r) {
            if (r.frsType() != frsType) {
                return done; // another record's traffic
            }
            int s = r.status();
            if (s == FRS_UNRECOGNIZED_FRS_TYPE || s == FRS_BUSY || s == FRS_OFFSET_OUT_OF_RANGE
                    || s == FRS_RECORD_EMPTY || s == FRS_DEVICE_ERROR) {
                throw new Sh2Exception(String.format("FRS read 0x%04X failed: status %d", frsType, s));
            }
            if (r.dataLength() > 0) {
                words.add(r.data0());
            }
            if (r.dataLength() > 1) {
                words.add(r.data1());
            }
            if (s == FRS_READ_COMPLETED || s == FRS_BLOCK_AND_READ_COMPLETED) {
                done = true;
            }
            return done;
        }
    }

    /**
     * A multi-packet FRS write: send {@link #request()}; for every 0xF5 {@link #feed} returns the
     * next Write Data payload, or null; {@link #done()} on WRITE_COMPLETED. Error statuses throw
     * {@link Sh2Exception}.
     */
    public static final class FrsWriteSession {
        private final int frsType;
        private final long[] words;
        private int offset;
        private boolean done;

        public FrsWriteSession(int frsType, long[] words) {
            this.frsType = frsType;
            this.words = words.clone();
        }

        public int frsType() {
            return frsType;
        }

        public boolean done() {
            return done;
        }

        public byte[] request() {
            return Sh2.buildFrsWriteRequest(frsType, words.length);
        }

        public byte[] feed(FrsWriteResponse r) {
            int s = r.status();
            if (s == FRS_WRITE_UNRECOGNIZED_FRS_TYPE || s == FRS_WRITE_BUSY || s == FRS_WRITE_FAILED
                    || s == FRS_WRITE_NOT_IN_WRITE_MODE || s == FRS_WRITE_INVALID_LENGTH || s == FRS_WRITE_RECORD_INVALID) {
                throw new Sh2Exception(String.format("FRS write 0x%04X failed: status %d", frsType, s));
            }
            if (s == FRS_WRITE_COMPLETED) {
                done = true;
                return null;
            }
            if (s == FRS_WRITE_MODE_READY || s == FRS_WRITE_WORDS_RECEIVED) {
                if (offset >= words.length) {
                    return null;
                }
                long[] chunk = Arrays.copyOfRange(words, offset, Math.min(words.length, offset + 2));
                byte[] data = Sh2.buildFrsWriteData(offset, chunk);
                offset += chunk.length;
                return data;
            }
            return null; // RECORD_VALID and friends: informational
        }
    }

    // ── helpers ──────────────────────────────────────────────────────────────

    private static void expectId(byte[] p, int id, String what) {
        if ((p[0] & 0xFF) != id) {
            throw new Sh2Exception(String.format("not a %s response: 0x%02X", what, p[0] & 0xFF));
        }
    }

    private static byte b(boolean v) {
        return (byte) (v ? 1 : 0);
    }

    private static byte u8(int v, String name) {
        return (byte) Wire.checkUintI(name, v, 8);
    }

    private static int u16(byte[] b, int off) {
        return (b[off] & 0xFF) | (b[off + 1] & 0xFF) << 8;
    }

    private static long u32(byte[] b, int off) {
        return (b[off] & 0xFFL) | (b[off + 1] & 0xFFL) << 8 | (b[off + 2] & 0xFFL) << 16 | (b[off + 3] & 0xFFL) << 24;
    }
}
