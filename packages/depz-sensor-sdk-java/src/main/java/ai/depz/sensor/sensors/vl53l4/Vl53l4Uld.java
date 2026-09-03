package ai.depz.sensor.sensors.vl53l4;

/**
 * VL53L4CD ULD (ST STSW-IMG026 2.2.3) — the host-verifiable decode layer only.
 *
 * <p>Port of the pure codec/math pieces of {@code VL53L4CD_api.c} that are
 * hardware-independent and covered by golden vectors: the 17-byte result-block
 * decode ({@link #parseResultBlock}), the SetRangeTiming/GetRangeTiming
 * register math ({@link #rangeTimingRegisters}, {@link #decodeRangeTiming}),
 * the tuning-register word codecs and the 91-byte init configuration block
 * ({@link #configBlock}). Semantics mirror the Python reference
 * ({@code depz_sensor_sdk/vl53l4/uld.py}) 1:1 — integer widths and 32-bit
 * truncations included; do not "simplify" the math.
 *
 * <p>OUT OF SCOPE (extension point, intentionally not ported): the live ULD
 * init / register-bridge driver (sensor_init, VHV calibration, offset/xtalk
 * calibration loops). Those depend on a live I2C bridge and cannot be replayed
 * deterministically; see {@link #liveDriverStubbed()}.
 */
public final class Vl53l4Uld {
    private Vl53l4Uld() {}

    // ── registers (VL53L4CD_api.h) ───────────────────────────────────────────
    public static final int SOFT_RESET = 0x0000;
    public static final int I2C_SLAVE_DEVICE_ADDRESS = 0x0001;
    /** Unnamed in the C driver; the oscillator-frequency word. */
    public static final int OSC_FREQUENCY = 0x0006;
    public static final int VHV_CONFIG_TIMEOUT_MACROP_LOOP_BOUND = 0x0008;
    public static final int XTALK_PLANE_OFFSET_KCPS = 0x0016;
    public static final int XTALK_X_PLANE_GRADIENT_KCPS = 0x0018;
    public static final int XTALK_Y_PLANE_GRADIENT_KCPS = 0x001A;
    public static final int RANGE_OFFSET_MM = 0x001E;
    public static final int INNER_OFFSET_MM = 0x0020;
    public static final int OUTER_OFFSET_MM = 0x0022;
    public static final int GPIO_HV_MUX_CTRL = 0x0030;
    public static final int GPIO_TIO_HV_STATUS = 0x0031;
    public static final int SYSTEM_INTERRUPT = 0x0046;
    public static final int RANGE_CONFIG_A = 0x005E;
    public static final int RANGE_CONFIG_B = 0x0061;
    public static final int RANGE_CONFIG_SIGMA_THRESH = 0x0064;
    public static final int MIN_COUNT_RATE_RTN_LIMIT_MCPS = 0x0066;
    public static final int INTERMEASUREMENT_MS = 0x006C;
    public static final int THRESH_HIGH = 0x0072;
    public static final int THRESH_LOW = 0x0074;
    public static final int SYSTEM_INTERRUPT_CLEAR = 0x0086;
    public static final int SYSTEM_START = 0x0087;
    public static final int RESULT_RANGE_STATUS = 0x0089;
    public static final int RESULT_SPAD_NB = 0x008C;
    public static final int RESULT_SIGNAL_RATE = 0x008E;
    public static final int RESULT_AMBIENT_RATE = 0x0090;
    public static final int RESULT_SIGMA = 0x0092;
    public static final int RESULT_DISTANCE = 0x0096;
    public static final int RESULT_OSC_CALIBRATE_VAL = 0x00DE;
    public static final int FIRMWARE_SYSTEM_STATUS = 0x00E5;
    public static final int IDENTIFICATION_MODEL_ID = 0x010F;

    /** IDENTIFICATION__MODEL_ID word expected from a live VL53L4CD. */
    public static final int MODEL_ID = 0xEBAA;

    /** Detection-threshold window modes (SYSTEM__INTERRUPT). */
    public static final int WINDOW_BELOW = 0;
    public static final int WINDOW_ABOVE = 1;
    public static final int WINDOW_OUT = 2;
    public static final int WINDOW_IN = 3;

    /** First register of the init configuration block (0x2D..0x87). */
    public static final int CONFIG_ADDR = 0x2D;

    /**
     * VL53L4CD_DEFAULT_CONFIGURATION[] — 91 bytes, registers 0x2D..0x87.
     * {@link #configBlock()} always overrides byte 0 (register 0x2D) with
     * {@link #CONFIG_FMP_BYTE} (0x12) to put the sensor's I2C pad in Fast Mode
     * Plus — exactly what VL53L4CD_I2C_FAST_MODE_PLUS does in the C ULD.
     */
    public static final byte[] DEFAULT_CONFIGURATION = {
        0x00, 0x00, 0x00, 0x11, 0x02, 0x00, 0x02, 0x08,          // 0x2D..0x34
        0x00, 0x08, 0x10, 0x01, 0x01, 0x00, 0x00, 0x00,          // 0x35..0x3C
        0x00, (byte) 0xff, 0x00, 0x0F, 0x00, 0x00, 0x00, 0x00,   // 0x3D..0x44
        0x00, 0x20, 0x0b, 0x00, 0x00, 0x02, 0x14, 0x21,          // 0x45..0x4C
        0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x00, (byte) 0xc8,   // 0x4D..0x54
        0x00, 0x00, 0x38, (byte) 0xff, 0x01, 0x00, 0x08, 0x00,   // 0x55..0x5C
        0x00, 0x01, (byte) 0xcc, 0x07, 0x01, (byte) 0xf1, 0x05, 0x00, // 0x5D..0x64
        (byte) 0xa0, 0x00, (byte) 0x80, 0x08, 0x38, 0x00, 0x00, 0x00, // 0x65..0x6C
        0x00, 0x0f, (byte) 0x89, 0x00, 0x00, 0x00, 0x00, 0x00,   // 0x6D..0x74
        0x00, 0x00, 0x01, 0x07, 0x05, 0x06, 0x06, 0x00,          // 0x75..0x7C
        0x00, 0x02, (byte) 0xc7, (byte) 0xff, (byte) 0x9B, 0x00, 0x00, 0x00, // 0x7D..0x84
        0x01, 0x00, 0x00,                                        // 0x85..0x87
    };

    /** Byte forced at register 0x2D: I2C Fast Mode Plus pad, never cleared. */
    public static final int CONFIG_FMP_BYTE = 0x12;

    /** The block the MCU streams: RESULT__RANGE_STATUS .. 0x0099. */
    public static final int RESULT_BLOCK_ADDR = RESULT_RANGE_STATUS;
    public static final int RESULT_BLOCK_LEN = 17;

    /** GetResult() raw status → ULD status (status_rtn[24] in VL53L4CD_api.c). */
    public static final int[] STATUS_RTN = {
        255, 255, 255, 5, 2, 4, 1, 7, 3,
        0, 255, 255, 9, 13, 255, 255, 255, 255, 10, 6,
        255, 255, 11, 12,
    };

    /** ULD error. */
    public static final class Vl53l4Error extends RuntimeException {
        public Vl53l4Error(String message) {
            super(message);
        }
    }

    /** VL53L4CD_ResultsData_t plus the sensor's own frame counter. */
    public record Results(
            int rangeStatus, int distanceMm, int ambientRateKcps,
            int ambientPerSpadKcps, int signalRateKcps, int signalPerSpadKcps,
            int numberOfSpad, int sigmaMm, int streamCount) {}

    /**
     * The 91-byte block sensor_init() writes at {@link #CONFIG_ADDR}: the ST
     * default configuration with byte 0 forced to {@link #CONFIG_FMP_BYTE}
     * (Fast Mode Plus).
     */
    public static byte[] configBlock() {
        byte[] out = DEFAULT_CONFIGURATION.clone();
        out[0] = (byte) CONFIG_FMP_BYTE;
        return out;
    }

    /**
     * Decode the streamed 0x0089..0x0099 block exactly as VL53L4CD_GetResult()
     * decodes the same registers read one by one. Register contents are
     * big-endian words (the bridge passes them through untouched). Throws
     * {@link Vl53l4Error} when {@code raw} is shorter than 15 bytes.
     */
    public static Results parseResultBlock(byte[] raw) {
        if (raw.length < 15) {
            throw new Vl53l4Error("result block too short: " + raw.length + " bytes");
        }

        int status = raw[0] & 0x1F;
        if (status < STATUS_RTN.length) {
            status = STATUS_RTN[status];
        }

        int rawSpads = be16(raw, 3);                // 0x008C
        int signalKcps = be16(raw, 5) * 8;          // 0x008E
        int ambientKcps = be16(raw, 7) * 8;         // 0x0090

        return new Results(
            status,
            be16(raw, 13),                          // 0x0096
            ambientKcps,
            rawSpads != 0 ? ambientKcps * 256 / rawSpads : 0,
            signalKcps,
            rawSpads != 0 ? signalKcps * 256 / rawSpads : 0,
            rawSpads / 256,
            be16(raw, 9) / 4,                       // 0x0092
            raw[2] & 0xFF);
    }

    // ── timing math (SetRangeTiming / GetRangeTiming) ────────────────────────

    /**
     * SetRangeTiming register math → {@code {RANGE_CONFIG_A, RANGE_CONFIG_B,
     * INTERMEASUREMENT_MS raw dword}}.
     *
     * <p>{@code oscFrequency} is the word read from 0x0006; {@code clockPll}
     * is the word read from RESULT__OSC_CALIBRATE_VAL (used only in autonomous
     * mode, i.e. when {@code interMeasurementMs > 0}). Budget 10..200 ms;
     * {@code interMeasurementMs} 0 (continuous) or greater than the budget
     * (autonomous low power) — anything else throws {@link Vl53l4Error}.
     */
    public static int[] rangeTimingRegisters(
            int timingBudgetMs, int interMeasurementMs, int oscFrequency, int clockPll) {
        if (oscFrequency == 0) {
            throw new Vl53l4Error("osc_frequency reads 0");
        }
        if (timingBudgetMs < 10 || timingBudgetMs > 200) {
            throw new Vl53l4Error("timing_budget_ms must be 10..200");
        }

        long timingBudgetUs = timingBudgetMs * 1000L;
        long macroPeriodUs = ((2304L * (0x40000000L / oscFrequency)) & 0xFFFFFFFFL) >> 6;

        long intermeasurementRaw;
        if (interMeasurementMs == 0) { // continuous
            intermeasurementRaw = 0;
            timingBudgetUs -= 2500;
        } else if (interMeasurementMs > timingBudgetMs) { // autonomous low power
            double factor = 1.055 * interMeasurementMs * (clockPll & 0x3FF);
            intermeasurementRaw = (long) factor; // truncate toward zero (non-negative)
            timingBudgetUs = (timingBudgetUs - 4300) / 2;
        } else {
            throw new Vl53l4Error("inter_measurement_ms must be 0 or > timing_budget_ms");
        }

        timingBudgetUs = (timingBudgetUs << 12) & 0xFFFFFFFFL;
        long[] words = new long[2];
        int[] mults = {16, 12}; // RANGE_CONFIG_A, RANGE_CONFIG_B
        for (int w = 0; w < 2; w++) {
            long tmp = ((macroPeriodUs * mults[w]) & 0xFFFFFFFFL) >> 6;
            long lsByte = ((timingBudgetUs + (tmp >> 1)) / tmp) - 1;
            long msByte = 0;
            while ((lsByte & 0xFFFFFF00L) != 0) {
                lsByte >>= 1;
                msByte += 1;
            }
            words[w] = ((msByte << 8) + (lsByte & 0xFF)) & 0xFFFF;
        }
        return new int[] {(int) words[0], (int) words[1], (int) intermeasurementRaw};
    }

    /**
     * GetRangeTiming register math → {@code {timing_budget_ms,
     * inter_measurement_ms}}.
     *
     * <p>Inputs are the raw register reads: INTERMEASUREMENT_MS dword, the
     * RESULT__OSC_CALIBRATE_VAL word, the 0x0006 word and the RANGE_CONFIG_A
     * word.
     */
    public static int[] decodeRangeTiming(
            long intermeasurementRaw, int clockPll, int oscFrequency, int rangeConfigA) {
        if (oscFrequency == 0) {
            throw new Vl53l4Error("osc_frequency reads 0");
        }

        long pll = ((long) (1.065 * (clockPll & 0x3FF))) & 0xFFFF;
        long interMeasurementMs = pll != 0 ? (intermeasurementRaw / pll) & 0xFFFF : 0;

        long macroPeriodUs = ((2304L * (0x40000000L / oscFrequency)) & 0xFFFFFFFFL) >> 6;
        long lsByte = (rangeConfigA & 0x00FF) << 4;
        long msByte = (rangeConfigA & 0xFF00) >> 8;
        msByte = (0x04 - (msByte - 1) - 1) & 0xFFFFFFFFL; // wraps: only shift when < 12
        macroPeriodUs = (macroPeriodUs * 16) & 0xFFFFFFFFL;

        long budget = ((((lsByte + 1) * (macroPeriodUs >> 6))
                - ((macroPeriodUs >> 6) >> 1)) & 0xFFFFFFFFL) >> 12;
        if (msByte < 12) {
            budget >>= msByte;
        }
        budget = intermeasurementRaw == 0 ? budget + 2500 : budget * 2 + 4300;
        return new int[] {(int) (budget / 1000), (int) interMeasurementMs};
    }

    // ── tuning codecs (register word ↔ user units) ───────────────────────────

    /** RANGE_OFFSET_MM word for SetOffset (INNER/OUTER are zeroed alongside). */
    public static int offsetRaw(int offsetMm) {
        return (offsetMm * 4) & 0xFFFF;
    }

    /** GetOffset: RANGE_OFFSET_MM word → signed millimetres. */
    public static int decodeOffset(int rawWord) {
        int temp = ((rawWord << 3) & 0xFFFF) >> 5;
        return temp > 1024 ? temp - 2048 : temp;
    }

    /** XTALK_PLANE_OFFSET_KCPS word for SetXtalk. */
    public static int xtalkRaw(int xtalkKcps) {
        return (xtalkKcps << 9) & 0xFFFF;
    }

    /** GetXtalk: XTALK_PLANE_OFFSET_KCPS word → kcps. */
    public static int decodeXtalk(int rawWord) {
        return (int) Math.round(rawWord / 512.0);
    }

    /** MIN_COUNT_RATE_RTN_LIMIT_MCPS word for SetSignalThreshold. */
    public static int signalThresholdRaw(int signalKcps) {
        return signalKcps >> 3;
    }

    /** GetSignalThreshold: word → kcps. */
    public static int decodeSignalThreshold(int rawWord) {
        return (rawWord << 3) & 0xFFFF;
    }

    /** RANGE_CONFIG__SIGMA_THRESH word for SetSigmaThreshold (mm ≤ 16383). */
    public static int sigmaThresholdRaw(int sigmaMm) {
        if (sigmaMm > (0xFFFF >> 2)) {
            throw new Vl53l4Error("sigma_mm must be <= 16383");
        }
        return sigmaMm << 2;
    }

    /** GetSigmaThreshold: word → mm. */
    public static int decodeSigmaThreshold(int rawWord) {
        return rawWord >> 2;
    }

    /**
     * The live ULD init / register-bridge driver (sensor_init, VHV/offset/xtalk
     * calibration) is intentionally not ported to Java — it is
     * hardware-dependent and out of the decode scope.
     */
    public static boolean liveDriverStubbed() {
        return true;
    }

    // ── BE helpers ───────────────────────────────────────────────────────────

    private static int be16(byte[] b, int off) {
        return ((b[off] & 0xFF) << 8) | (b[off + 1] & 0xFF);
    }
}
