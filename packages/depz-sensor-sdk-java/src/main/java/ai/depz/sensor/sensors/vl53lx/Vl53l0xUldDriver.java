package ai.depz.sensor.sensors.vl53lx;

import ai.depz.sensor.sensors.vl53l4.RangeTiming;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;

/**
 * The VL53L0X API 1.0.4 ranging path (vl53l0x_api.c / _api_core.c / _api_calibration.c), ported from
 * the reference Python driver: DataInit, StaticInit (reference SPADs from NVM, tuning settings), the
 * timing-budget arithmetic, start / stop, GetRangingMeasurementData with its PAL range status (sigma
 * estimate included), the reference calibration, PerformRefSpadManagement, offset and crosstalk
 * calibration and the four ST ranging profiles. Register sequences, integer widths and the deliberate
 * uint32 overflows are the C driver's. There is no ROI on this die.
 */
public final class Vl53l0xUldDriver extends Vl53lxDriver {
    /** The VHV and phase settings the reference calibration found. */
    public record RefCalibration(int vhv, int phase) {}

    /** The reference SPAD set: how many and whether they are aperture SPADs (1) or not (0). */
    public record RefSpads(int count, int isAperture) {}

    // ── registers (vl53l0x_device.h) ─────────────────────────────────────────
    private static final int SYSRANGE_START = 0x00;
    private static final int SYSRANGE_MODE_SINGLESHOT = 0x00;
    private static final int SYSRANGE_MODE_START_STOP = 0x01;
    private static final int SYSRANGE_MODE_BACKTOBACK = 0x02;
    private static final int SYSRANGE_MODE_TIMED = 0x04;
    private static final int SYSTEM_SEQUENCE_CONFIG = 0x01;
    private static final int SYSTEM_INTERMEASUREMENT_PERIOD = 0x04;
    private static final int SYSTEM_RANGE_CONFIG = 0x09;
    private static final int SYSTEM_INTERRUPT_CONFIG_GPIO = 0x0A;
    private static final int SYSTEM_INTERRUPT_CLEAR = 0x0B;
    private static final int RESULT_INTERRUPT_STATUS = 0x13;
    private static final int RESULT_RANGE_STATUS = 0x14;
    private static final int CROSSTALK_COMPENSATION_PEAK_RATE_MCPS = 0x20;
    private static final int ALGO_PART_TO_PART_RANGE_OFFSET_MM = 0x28;
    private static final int ALGO_PHASECAL_CONFIG_TIMEOUT = 0x30;
    private static final int GLOBAL_CONFIG_VCSEL_WIDTH = 0x32;
    private static final int FINAL_RANGE_CONFIG_MIN_COUNT_RATE_RTN_LIMIT = 0x44;
    private static final int MSRC_CONFIG_TIMEOUT_MACROP = 0x46;
    private static final int FINAL_RANGE_CONFIG_VALID_PHASE_LOW = 0x47;
    private static final int FINAL_RANGE_CONFIG_VALID_PHASE_HIGH = 0x48;
    private static final int DYNAMIC_SPAD_NUM_REQUESTED_REF_SPAD = 0x4E;
    private static final int DYNAMIC_SPAD_REF_EN_START_OFFSET = 0x4F;
    private static final int PRE_RANGE_CONFIG_VCSEL_PERIOD = 0x50;
    private static final int PRE_RANGE_CONFIG_TIMEOUT_MACROP_HI = 0x51;
    private static final int PRE_RANGE_CONFIG_VALID_PHASE_LOW = 0x56;
    private static final int PRE_RANGE_CONFIG_VALID_PHASE_HIGH = 0x57;
    private static final int MSRC_CONFIG_CONTROL = 0x60;
    private static final int PRE_RANGE_MIN_COUNT_RATE_RTN_LIMIT = 0x64;
    private static final int FINAL_RANGE_CONFIG_VCSEL_PERIOD = 0x70;
    private static final int FINAL_RANGE_CONFIG_TIMEOUT_MACROP_HI = 0x71;
    private static final int POWER_MANAGEMENT_GO1_POWER_FORCE = 0x80;
    private static final int GPIO_HV_MUX_ACTIVE_HIGH = 0x84;
    private static final int SOFT_RESET_GO2_SOFT_RESET_N = 0xBF;
    private static final int GLOBAL_CONFIG_SPAD_ENABLES_REF_0 = 0xB0;
    private static final int GLOBAL_CONFIG_REF_EN_START_SELECT = 0xB6;
    private static final int IDENTIFICATION_MODEL_ID = 0xC0;
    private static final int OSC_CALIBRATE_VAL = 0xF8;
    // Page 1 (0xFF ← 0x01 first): they share their address with a page-0 register.
    private static final int ALGO_PHASECAL_LIM = 0x30;
    private static final int RESULT_PEAK_SIGNAL_RATE_REF = 0xB6;

    public static final int MODEL_ID_VL53L0X = 0xEE;

    private static final int DEVICE_MODE_SINGLE_RANGING = 0;
    private static final int DEVICE_MODE_CONTINUOUS_RANGING = 1;
    private static final int DEVICE_MODE_CONTINUOUS_TIMED = 3;
    private static final int GPIO_FUNCTIONALITY_NEW_MEASURE_READY = 4;

    private static final int SEQ_TCC = 0, SEQ_DSS = 1, SEQ_MSRC = 2, SEQ_PRE_RANGE = 3, SEQ_FINAL_RANGE = 4;
    private static final int[] SEQ_SET = {0x10, 0x28, 0x04, 0x40, 0x80};
    private static final int[] SEQ_CLEAR = {0xEF, 0xD7, 0xFB, 0xBF, 0x7F};
    private static final int[] SEQ_TEST = {0x10, 0x08, 0x04, 0x40, 0x80};

    private static final int VCSEL_PRE_RANGE = 0, VCSEL_FINAL_RANGE = 1;

    private static final int CHECK_SIGMA_FINAL_RANGE = 0, CHECK_SIGNAL_RATE_FINAL_RANGE = 1, CHECK_SIGNAL_REF_CLIP = 2;
    private static final int CHECK_RANGE_IGNORE_THRESHOLD = 3, CHECK_SIGNAL_RATE_MSRC = 4, CHECK_SIGNAL_RATE_PRE_RANGE = 5;
    private static final int CHECK_NUMBER_OF_CHECKS = 6;

    private static final int DEFAULT_MAX_LOOP = 2000;
    private static final int TARGET_REF_RATE_DEFAULT = 0x0A00;
    private static final long MIN_TIMING_BUDGET_US = 20000; // cMinTimingBudgetMicroSeconds
    // The host-side parameters a tuning buffer may carry in a {0xFF, select, msb, lsb}
    // record, by select value (VL53L0X_load_tuning_settings()).
    private static final int TUNING_PARAM_COUNT = 4;
    private static final long SPEED_OF_LIGHT_IN_AIR = 2997;
    private static final int REF_SPAD_BUFFER_SIZE = 6;

    /** One ST example profile: (signal Mcps, sigma mm, budget µs, pre-range PCLK, final PCLK). */
    private record ModeSetting(String name, double signal, int sigma, int budgetUs, int pre, int fin) {}

    // The four ST examples.
    private static final ModeSetting[] MODE_SETTINGS = {
        new ModeSetting("default", 0.25, 18, 33000, 14, 10),
        new ModeSetting("long-range", 0.10, 60, 33000, 18, 14),
        new ModeSetting("high-speed", 0.25, 32, 30000, 14, 10),
        new ModeSetting("high-accuracy", 0.25, 18, 200000, 14, 10),
    };

    // ST's range for each final-range VCSEL period the profiles use.
    private static final Map<Integer, Integer> REACH_BY_FINAL_PCLKS_MM = Map.of(10, 1200, 14, 2000);

    // DefaultTuningSettings[] (vl53l0x_tuning.h, v36): {count, address, bytes…} records, 0 ends.
    private static final int[] DEFAULT_TUNING_SETTINGS = {
        0x01, 0xFF, 0x01, 0x01, 0x00, 0x00,
        0x01, 0xFF, 0x00, 0x01, 0x09, 0x00, 0x01, 0x10, 0x00, 0x01, 0x11, 0x00,
        0x01, 0x24, 0x01, 0x01, 0x25, 0xff, 0x01, 0x75, 0x00,
        0x01, 0xFF, 0x01, 0x01, 0x4e, 0x2c, 0x01, 0x48, 0x00, 0x01, 0x30, 0x20,
        0x01, 0xFF, 0x00, 0x01, 0x30, 0x09, 0x01, 0x54, 0x00, 0x01, 0x31, 0x04, 0x01, 0x32, 0x03,
        0x01, 0x40, 0x83, 0x01, 0x46, 0x25, 0x01, 0x60, 0x00, 0x01, 0x27, 0x00, 0x01, 0x50, 0x06,
        0x01, 0x51, 0x00, 0x01, 0x52, 0x96, 0x01, 0x56, 0x08, 0x01, 0x57, 0x30, 0x01, 0x61, 0x00,
        0x01, 0x62, 0x00, 0x01, 0x64, 0x00, 0x01, 0x65, 0x00, 0x01, 0x66, 0xa0,
        0x01, 0xFF, 0x01, 0x01, 0x22, 0x32, 0x01, 0x47, 0x14, 0x01, 0x49, 0xff, 0x01, 0x4a, 0x00,
        0x01, 0xFF, 0x00, 0x01, 0x7a, 0x0a, 0x01, 0x7b, 0x00, 0x01, 0x78, 0x21,
        0x01, 0xFF, 0x01, 0x01, 0x23, 0x34, 0x01, 0x42, 0x00, 0x01, 0x44, 0xff, 0x01, 0x45, 0x26,
        0x01, 0x46, 0x05, 0x01, 0x40, 0x40, 0x01, 0x0E, 0x06, 0x01, 0x20, 0x1a, 0x01, 0x43, 0x40,
        0x01, 0xFF, 0x00, 0x01, 0x34, 0x03, 0x01, 0x35, 0x44,
        0x01, 0xFF, 0x01, 0x01, 0x31, 0x04, 0x01, 0x4b, 0x09, 0x01, 0x4c, 0x05, 0x01, 0x4d, 0x04,
        0x01, 0xFF, 0x00, 0x01, 0x44, 0x00, 0x01, 0x45, 0x20, 0x01, 0x47, 0x08, 0x01, 0x48, 0x28,
        0x01, 0x67, 0x00, 0x01, 0x70, 0x04, 0x01, 0x71, 0x01, 0x01, 0x72, 0xfe, 0x01, 0x76, 0x00,
        0x01, 0x77, 0x00,
        0x01, 0xFF, 0x01, 0x01, 0x0d, 0x01,
        0x01, 0xFF, 0x00, 0x01, 0x80, 0x01, 0x01, 0x01, 0xF8,
        0x01, 0xFF, 0x01, 0x01, 0x8e, 0x01, 0x01, 0x00, 0x01, 0x01, 0xFF, 0x00, 0x01, 0x80, 0x00,
        0x00, 0x00, 0x00,
    };

    // Dmax lookup table (FixPoint16.16).
    private static final long[] DMAX_LUT_AMB_RATE_MCPS = {0x00000000L, 0x0000B333L, 0x00020000L, 0x0003CCCCL, 0x00074CCCL, 0x000A0000L, 0x000F0000L};
    private static final long[] DMAX_LUT_DMAX_MM = {0x04B00000L, 0x044C0000L, 0x03840000L, 0x02EE0000L, 0x02260000L, 0x01F40000L, 0x01900000L};

    private static final int RESULT_BLOCK_ADDR = RESULT_RANGE_STATUS;
    private static final int RESULT_BLOCK_LEN = 12;

    private static final int[][] L0X_CLEAR_STEPS = {{SYSTEM_INTERRUPT_CLEAR, 0x01}, {SYSTEM_INTERRUPT_CLEAR, 0x00}};

    private static final Map<Integer, String> RANGE_STATUS_NAMES = Map.of(
            0, "Range Valid", 1, "Sigma Fail", 2, "Signal Fail", 3, "Min Range Fail",
            4, "Phase Fail", 5, "Hardware Fail", 255, "No Update");

    private static final int[] REF_ARRAY_QUADRANTS = {10, 5, 0, 5};

    private static final List<String> GROUPS = List.of("mode", "timing", "offset", "xtalk", "calib_offset", "calib_xtalk", "refspad");

    // PALDevData / DeviceSpecificParameters: the state the C driver keeps beside the sensor.
    private int readDataFromDeviceDone;
    private long xtalkCompensationRateMegaCps;
    private int xtalkCompensationEnable;
    private int stopVariable;
    private int sequenceConfig;
    private int rangeFractionalEnable;
    private int deviceMode = DEVICE_MODE_SINGLE_RANGING;
    private Integer continuousMode; // not in the C driver: resumed after a single shot
    private long measurementTimingBudgetUs;
    private int pin0GpioFunctionality = GPIO_FUNCTIONALITY_NEW_MEASURE_READY;
    private int referenceSpadCount;
    private int referenceSpadType;
    private int[] refGoodSpadMap = new int[REF_SPAD_BUFFER_SIZE];
    private int preRangeVcselPulsePeriod;
    private int finalRangeVcselPulsePeriod;
    private long preRangeTimeoutUs;
    private long finalRangeTimeoutUs;
    private int targetRefRate = TARGET_REF_RATE_DEFAULT;
    // Stored for the tuning loader; the 1.0.4 sigma estimate has these three as constants.
    private int sigmaEstRefArray = 100;
    private int sigmaEstEffPulseWidth = 900;
    private int sigmaEstEffAmbWidth = 500;
    private final int[] limitChecksEnable = new int[CHECK_NUMBER_OF_CHECKS];
    private final long[] limitChecksValue = new long[CHECK_NUMBER_OF_CHECKS];
    private String mode = "default";

    private boolean refSpadsFromNvm = true;
    private int moduleId;
    private int revision;
    private String productIdString = "";
    private long partUidUpper;
    private long partUidLower;
    private long signalRateMeasFixed400mm;
    private long part2PartOffsetAdjustmentNvmUm;
    private long oscFrequencyMHz = 618660;

    public Vl53l0xUldDriver(Vl53lxPlatform platform, String product) {
        super(platform, product);
    }

    /** The only part of the family with 8-bit register addresses. */
    @Override
    public int addrWidth() {
        return 1;
    }

    @Override
    public int[][] clearSteps() {
        int[][] copy = new int[L0X_CLEAR_STEPS.length][];
        for (int i = 0; i < copy.length; i++) {
            copy[i] = L0X_CLEAR_STEPS[i].clone();
        }
        return copy;
    }

    /** No Fast Mode Plus on this die. */
    @Override
    public int maxKhz() {
        return 400;
    }

    @Override
    public Collection<String> supports() {
        return GROUPS;
    }

    @Override
    public List<String> modes() {
        List<String> names = new ArrayList<>();
        for (ModeSetting m : MODE_SETTINGS) {
            names.add(m.name());
        }
        return List.copyOf(names);
    }

    @Override
    public int[] budgetMs() {
        return new int[] {20, 200};
    }

    /** False when the NVM reference-SPAD record was out of range and was measured instead. */
    public boolean refSpadsFromNvm() {
        return refSpadsFromNvm;
    }

    public int moduleId() {
        return moduleId;
    }

    public int revision() {
        return revision;
    }

    public String productIdString() {
        return productIdString;
    }

    public long partUidUpper() {
        return partUidUpper;
    }

    public long partUidLower() {
        return partUidLower;
    }

    public long signalRateMeasFixed400mm() {
        return signalRateMeasFixed400mm;
    }

    public long part2PartOffsetAdjustmentNvmUm() {
        return part2PartOffsetAdjustmentNvmUm;
    }

    /** The reference-SPAD target rate (9.7 Mcps) a tuning buffer's {0xFF, 3, msb, lsb} record sets;
     *  the SPAD management converges on it. */
    public int targetRefRate() {
        return targetRefRate;
    }

    /** The three sigma-estimate constants a tuning buffer's {0xFF, 0..2, msb, lsb} records set
     *  (stored only: the 1.0.4 sigma estimate has them as constants). */
    public int sigmaEstRefArray() {
        return sigmaEstRefArray;
    }

    public int sigmaEstEffPulseWidth() {
        return sigmaEstEffPulseWidth;
    }

    public int sigmaEstEffAmbWidth() {
        return sigmaEstEffAmbWidth;
    }

    public long oscFrequencyMHz() {
        return oscFrequencyMHz;
    }

    // ── helpers of the C driver ──────────────────────────────────────────────

    /** C int16 truncation (an assignment to an int16_t). */
    private static long i16(long v) {
        long r = v & 0xFFFF;
        return (r & 0x8000) != 0 ? r - 0x10000 : r;
    }

    private static long u32(long v) {
        return v & 0xFFFFFFFFL;
    }

    /** VL53L0X_isqrt(): the bit-by-bit integer square root. */
    public static long isqrt(long num) {
        long res = 0;
        long bit = 1L << 30;
        while (bit > num) {
            bit >>= 2;
        }
        while (bit != 0) {
            if (num >= res + bit) {
                num -= res + bit;
                res = (res >> 1) + bit;
            } else {
                res >>= 1;
            }
            bit >>= 2;
        }
        return res;
    }

    private static int decodeVcselPeriod(int reg) {
        return (reg + 1) << 1;
    }

    private static int encodeVcselPeriod(int pclks) {
        return (pclks >> 1) - 1;
    }

    private static int encodeTimeout(long mclks) {
        if (mclks <= 0) {
            return 0;
        }
        long ls = mclks - 1;
        int ms = 0;
        while ((ls & 0xFFFFFF00L) != 0) {
            ls >>= 1;
            ms++;
        }
        return (int) (((ms << 8) + (ls & 0xFF)) & 0xFFFF);
    }

    private static long decodeTimeout(int encoded) {
        return ((long) (encoded & 0x00FF) << ((encoded & 0xFF00) >> 8)) + 1;
    }

    private static long macroPeriodPs(int pclks) {
        return 2304L * pclks * 1655;
    }

    private static long timeoutMclks(long timeoutUs, int pclks) {
        long macroPeriodNs = (macroPeriodPs(pclks) + 500) / 1000;
        return (timeoutUs * 1000 + macroPeriodNs / 2) / macroPeriodNs;
    }

    private static long timeoutUs(long mclks, int pclks) {
        long macroPeriodNs = (macroPeriodPs(pclks) + 500) / 1000;
        return (mclks * macroPeriodNs + 500) / 1000;
    }

    private static boolean isAperture(int spadIndex) {
        return REF_ARRAY_QUADRANTS[spadIndex >> 6] != 0;
    }

    private static int nextGoodSpad(int[] good, int size, int curr) {
        int startIndex = curr / 8;
        int fineOffset = curr % 8;
        for (int coarse = startIndex; coarse < size; coarse++) {
            int dataByte = good[coarse];
            int fine = 0;
            if (coarse == startIndex) {
                dataByte >>= fineOffset;
                fine = fineOffset;
            }
            while (fine < 8) {
                if ((dataByte & 1) != 0) {
                    return coarse * 8 + fine;
                }
                dataByte >>= 1;
                fine++;
            }
        }
        return -1;
    }

    private static byte[] toBytes(int[] values) {
        byte[] out = new byte[values.length];
        for (int i = 0; i < values.length; i++) {
            out[i] = (byte) values[i];
        }
        return out;
    }

    // ── contract ─────────────────────────────────────────────────────────────

    @Override
    public int[] streamBlock() {
        return new int[] {RESULT_BLOCK_ADDR, RESULT_BLOCK_LEN};
    }

    @Override
    public Vl53lxRange decode(byte[] raw) {
        return asRange(parseResultBlock(raw));
    }

    @Override
    public Vl53lxRange readMeasurement() {
        return asRange(getResult());
    }

    @Override
    public int modelId() {
        return p.readByte(IDENTIFICATION_MODEL_ID);
    }

    /** No firmware-status register: the model id answering says the die is up (1 s). */
    public void waitBoot() {
        waitBoot(1000);
    }

    /** No firmware-status register: the model id answering says the die is up. */
    public void waitBoot(long timeoutMs) {
        long start = System.nanoTime();
        long limitNs = timeoutMs * 1_000_000L;
        while (true) {
            try {
                if (p.readByte(IDENTIFICATION_MODEL_ID) == MODEL_ID_VL53L0X) {
                    return;
                }
            } catch (Vl53lxProtocolException e) {
                // NACK while the die is still booting
            }
            if (System.nanoTime() - start > limitNs) {
                throw new Vl53lxException("timeout waiting for MODEL_ID 0xEE at 0xC0");
            }
            p.sleepMs(1);
        }
    }

    /** DataInit + StaticInit, the reference calibration, continuous mode; the bus stays at 400 kHz. */
    @Override
    public void sensorInit() {
        p.setAddrWidth(addrWidth());
        p.setI2cSpeed(400);
        waitBoot();
        resetDevice();
        dataInit();
        staticInit();
        if (!refSpadsFromNvm) {
            refSpadManagement();
        }
        performRefCalibration();
        setDeviceMode(DEVICE_MODE_CONTINUOUS_RANGING);
        mode = "default";
    }

    /** VL53L0X_ResetDevice() (1 s): a sensor left configured by a previous session ranges nothing without it. */
    public void resetDevice() {
        resetDevice(1000);
    }

    /** VL53L0X_ResetDevice(): a sensor left configured by a previous session ranges nothing without it. */
    public void resetDevice(long timeoutMs) {
        long start = System.nanoTime();
        long limitNs = timeoutMs * 1_000_000L;
        p.writeByte(SOFT_RESET_GO2_SOFT_RESET_N, 0x00);
        pollModelId(true, start, limitNs);
        p.sleepMs(1);
        p.writeByte(SOFT_RESET_GO2_SOFT_RESET_N, 0x01);
        pollModelId(false, start, limitNs);
        p.sleepMs(1);
    }

    // While the die reboots it NACKs its own address: "not booted yet", not a fault.
    private void pollModelId(boolean wantZero, long start, long limitNs) {
        while (true) {
            Integer b;
            try {
                b = p.readByte(IDENTIFICATION_MODEL_ID);
            } catch (Vl53lxProtocolException e) {
                b = null;
            }
            if (b != null && (b == 0x00) == wantZero) {
                return;
            }
            if (System.nanoTime() - start > limitNs) {
                throw new Vl53lxException("timeout in the soft reset of the sensor");
            }
        }
    }

    private void dataInit() {
        p.writeByte(0x88, 0x00); // I2C standard mode
        readDataFromDeviceDone = 0;
        oscFrequencyMHz = 618660;
        xtalkCompensationRateMegaCps = 0;
        xtalkCompensationEnable = 0;
        deviceMode = DEVICE_MODE_SINGLE_RANGING;

        p.writeByte(0x80, 0x01);
        p.writeByte(0xFF, 0x01);
        p.writeByte(0x00, 0x00);
        stopVariable = p.readByte(0x91);
        p.writeByte(0x00, 0x01);
        p.writeByte(0xFF, 0x00);
        p.writeByte(0x80, 0x00);

        for (int check = 0; check < CHECK_NUMBER_OF_CHECKS; check++) {
            setLimitCheckEnable(check, 1);
        }
        setLimitCheckEnable(CHECK_SIGNAL_REF_CLIP, 0);
        setLimitCheckEnable(CHECK_RANGE_IGNORE_THRESHOLD, 0);
        setLimitCheckEnable(CHECK_SIGNAL_RATE_MSRC, 0);
        setLimitCheckEnable(CHECK_SIGNAL_RATE_PRE_RANGE, 0);

        setLimitCheckValue(CHECK_SIGMA_FINAL_RANGE, 18 * 65536L);
        setLimitCheckValue(CHECK_SIGNAL_RATE_FINAL_RANGE, 25 * 65536L / 100);
        setLimitCheckValue(CHECK_SIGNAL_REF_CLIP, 35 * 65536L);
        setLimitCheckValue(CHECK_RANGE_IGNORE_THRESHOLD, 0);

        sequenceConfig = 0xFF;
        p.writeByte(SYSTEM_SEQUENCE_CONFIG, 0xFF);
    }

    private void staticInit() {
        getInfoFromDevice(1);
        int count = referenceSpadCount;
        int aperture = referenceSpadType;
        refSpadsFromNvm = !(aperture > 1 || (aperture == 1 && count > 32) || (aperture == 0 && count > 12));
        if (refSpadsFromNvm) {
            setReferenceSpads(count, aperture);
        }

        loadTuningSettings(DEFAULT_TUNING_SETTINGS);
        setGpioConfig(GPIO_FUNCTIONALITY_NEW_MEASURE_READY, true);

        p.writeByte(0xFF, 0x01);
        int tempword = p.readWord(0x84);
        p.writeByte(0xFF, 0x00);
        oscFrequencyMHz = (long) tempword << 4; // FixPoint4.12 → 16.16

        measurementTimingBudgetUs = getMeasurementTimingBudget();
        rangeFractionalEnable = p.readByte(SYSTEM_RANGE_CONFIG) & 1;
        sequenceConfig = p.readByte(SYSTEM_SEQUENCE_CONFIG);

        setSequenceStepEnable(SEQ_TCC, 0);
        setSequenceStepEnable(SEQ_MSRC, 0);

        preRangeVcselPulsePeriod = getVcselPulsePeriod(VCSEL_PRE_RANGE);
        finalRangeVcselPulsePeriod = getVcselPulsePeriod(VCSEL_FINAL_RANGE);
        preRangeTimeoutUs = getSequenceStepTimeout(SEQ_PRE_RANGE);
        finalRangeTimeoutUs = getSequenceStepTimeout(SEQ_FINAL_RANGE);
    }

    // ── reference calibration (VHV / phase) ──────────────────────────────────

    private void performSingleRefCalibration(int vhvInitByte) {
        p.writeByte(SYSRANGE_START, SYSRANGE_MODE_START_STOP | vhvInitByte);
        waitDataReady(1000);
        clearInterrupt();
        p.writeByte(SYSRANGE_START, 0x00);
    }

    private RefCalibration refCalibrationIoRead(boolean vhvEnable, boolean phaseEnable) {
        p.writeByte(0xFF, 0x01);
        p.writeByte(0x00, 0x00);
        p.writeByte(0xFF, 0x00);
        int vhv = vhvEnable ? p.readByte(0xCB) : 0;
        int phase = phaseEnable ? p.readByte(0xEE) : 0;
        p.writeByte(0xFF, 0x01);
        p.writeByte(0x00, 0x01);
        p.writeByte(0xFF, 0x00);
        return new RefCalibration(vhv, phase & 0xEF);
    }

    /** VL53L0X_PerformRefCalibration(): VHV, then phase → (vhv, phase). Not optional in practice. */
    public RefCalibration performRefCalibration() {
        int savedSequenceConfig = sequenceConfig;
        p.writeByte(SYSTEM_SEQUENCE_CONFIG, 0x01); // VHV only
        performSingleRefCalibration(0x40);
        int vhv = refCalibrationIoRead(true, false).vhv();
        p.writeByte(SYSTEM_SEQUENCE_CONFIG, 0x02); // phase only
        performSingleRefCalibration(0x00);
        int phase = refCalibrationIoRead(false, true).phase();
        p.writeByte(SYSTEM_SEQUENCE_CONFIG, savedSequenceConfig);
        sequenceConfig = savedSequenceConfig;
        return new RefCalibration(vhv, phase);
    }

    // ── NVM ──────────────────────────────────────────────────────────────────

    private void deviceReadStrobe() {
        p.writeByte(0x83, 0x00);
        boolean done = false;
        for (int i = 0; i < DEFAULT_MAX_LOOP; i++) {
            if (p.readByte(0x83) != 0x00) {
                done = true;
                break;
            }
        }
        if (!done) {
            throw new Vl53lxException("timeout waiting for the NVM read strobe");
        }
        p.writeByte(0x83, 0x01);
    }

    private long nvmDword(int index) {
        p.writeByte(0x94, index);
        deviceReadStrobe();
        return p.readDword(0x90);
    }

    /**
     * VL53L0X_get_info_from_device(): the reference SPAD record (option 1), the product
     * identification (2), the part UID and factory offset (4).
     */
    public void getInfoFromDevice(int option) {
        int done = readDataFromDeviceDone;
        if (done == 7) {
            return;
        }
        p.writeByte(0x80, 0x01);
        p.writeByte(0xFF, 0x01);
        p.writeByte(0x00, 0x00);
        p.writeByte(0xFF, 0x06);
        p.writeByte(0x83, p.readByte(0x83) | 4);
        p.writeByte(0xFF, 0x07);
        p.writeByte(0x81, 0x01);
        p.sleepMs(1); // VL53L0X_PollingDelay()
        p.writeByte(0x80, 0x01);

        int[] goodSpadMap = refGoodSpadMap.clone();
        int refSpadCount = 0, refSpadType = 0;
        String productId = "";
        int newModuleId = 0, newRevision = 0;
        long uidUpper = 0, uidLower = 0, signalRate400 = 0, dist400 = 0;

        if ((option & 1) == 1 && (done & 1) == 0) {
            long tmp = nvmDword(0x6B);
            refSpadCount = (int) (tmp >> 8 & 0x7F);
            refSpadType = (int) (tmp >> 15 & 0x01);
            tmp = nvmDword(0x24);
            goodSpadMap[0] = (int) (tmp >> 24 & 0xFF);
            goodSpadMap[1] = (int) (tmp >> 16 & 0xFF);
            goodSpadMap[2] = (int) (tmp >> 8 & 0xFF);
            goodSpadMap[3] = (int) (tmp & 0xFF);
            tmp = nvmDword(0x25);
            goodSpadMap[4] = (int) (tmp >> 24 & 0xFF);
            goodSpadMap[5] = (int) (tmp >> 16 & 0xFF);
        }

        if ((option & 2) == 2 && (done & 2) == 0) {
            p.writeByte(0x94, 0x02);
            deviceReadStrobe();
            newModuleId = p.readByte(0x90);
            p.writeByte(0x94, 0x7B);
            deviceReadStrobe();
            newRevision = p.readByte(0x90);

            List<Long> chars = new ArrayList<>();
            long tmp = nvmDword(0x77);
            chars.addAll(List.of(tmp >> 25 & 0x7F, tmp >> 18 & 0x7F, tmp >> 11 & 0x7F, tmp >> 4 & 0x7F));
            long b = (tmp & 0x00F) << 3;
            tmp = nvmDword(0x78);
            chars.addAll(List.of(b + (tmp >> 29 & 0x7F), tmp >> 22 & 0x7F, tmp >> 15 & 0x7F, tmp >> 8 & 0x7F, tmp >> 1 & 0x7F));
            b = (tmp & 0x001) << 6;
            tmp = nvmDword(0x79);
            chars.addAll(List.of(b + (tmp >> 26 & 0x7F), tmp >> 19 & 0x7F, tmp >> 12 & 0x7F, tmp >> 5 & 0x7F));
            b = (tmp & 0x01F) << 2;
            tmp = nvmDword(0x7A);
            chars.addAll(List.of(b + (tmp >> 30 & 0x7F), tmp >> 23 & 0x7F, tmp >> 16 & 0x7F, tmp >> 9 & 0x7F, tmp >> 2 & 0x7F));
            StringBuilder sb = new StringBuilder();
            for (long c : chars) {
                sb.append((char) (c & 0x7F));
            }
            productId = sb.toString();
        }

        if ((option & 4) == 4 && (done & 4) == 0) {
            uidUpper = nvmDword(0x7B);
            uidLower = nvmDword(0x7C);
            signalRate400 = (nvmDword(0x73) & 0xFF) << 8;
            signalRate400 |= (nvmDword(0x74) & 0xFF000000L) >> 24;
            dist400 = (nvmDword(0x75) & 0xFF) << 8;
            dist400 |= (nvmDword(0x76) & 0xFF000000L) >> 24;
        }

        p.writeByte(0x81, 0x00);
        p.writeByte(0xFF, 0x06);
        p.writeByte(0x83, p.readByte(0x83) & 0xFB);
        p.writeByte(0xFF, 0x01);
        p.writeByte(0x00, 0x01);
        p.writeByte(0xFF, 0x00);
        p.writeByte(0x80, 0x00);

        if ((option & 1) == 1 && (done & 1) == 0) {
            referenceSpadCount = refSpadCount;
            referenceSpadType = refSpadType;
            refGoodSpadMap = goodSpadMap;
        }
        if ((option & 2) == 2 && (done & 2) == 0) {
            moduleId = newModuleId;
            revision = newRevision;
            productIdString = productId;
        }
        if ((option & 4) == 4 && (done & 4) == 0) {
            partUidUpper = uidUpper;
            partUidLower = uidLower;
            signalRateMeasFixed400mm = signalRate400 << 9;
            long offsetUm = 0;
            if (dist400 != 0) {
                // uint32 difference, uint32 product, then an int16_t: the C driver's types,
                // kept because they are what makes the sign come out right. A target measured
                // nearer than 400 mm wraps the uint32 difference, and only the int16
                // truncation turns that back into the positive offset it stands for.
                long offset1104Mm = u32(dist400 - (400 << 4));
                offsetUm = i16(-i16(u32(offset1104Mm * 1000) >> 4));
            }
            part2PartOffsetAdjustmentNvmUm = offsetUm;
        }
        readDataFromDeviceDone = done | option;
    }

    // ── reference SPADs ──────────────────────────────────────────────────────

    private void setReferenceSpads(int count, int isApertureSpads) {
        final int startSelect = 0xB4;
        final int maxSpadCount = 44;
        p.writeByte(0xFF, 0x01);
        p.writeByte(DYNAMIC_SPAD_REF_EN_START_OFFSET, 0x00);
        p.writeByte(DYNAMIC_SPAD_NUM_REQUESTED_REF_SPAD, 0x2C);
        p.writeByte(0xFF, 0x00);
        p.writeByte(GLOBAL_CONFIG_REF_EN_START_SELECT, startSelect);

        int[] spadArray = new int[REF_SPAD_BUFFER_SIZE];
        int current = 0;
        if (isApertureSpads != 0) {
            while (!isAperture(startSelect + current) && current < maxSpadCount) {
                current++;
            }
        }
        for (int i = 0; i < count; i++) {
            int next = nextGoodSpad(refGoodSpadMap, REF_SPAD_BUFFER_SIZE, current);
            if (next == -1) {
                throw new Vl53lxException("ran out of good reference SPADs");
            }
            if (isAperture(startSelect + next) != (isApertureSpads != 0)) {
                throw new Vl53lxException("the good SPAD map leaves the requested quadrant");
            }
            current = next;
            spadArray[current / 8] |= 1 << (current % 8);
            current++;
        }
        byte[] bytes = toBytes(spadArray);
        p.writeMulti(GLOBAL_CONFIG_SPAD_ENABLES_REF_0, bytes);
        if (!Arrays.equals(bytes, p.readMulti(GLOBAL_CONFIG_SPAD_ENABLES_REF_0, REF_SPAD_BUFFER_SIZE))) {
            throw new Vl53lxException("reference SPAD map did not read back");
        }
        referenceSpadCount = count;
        referenceSpadType = isApertureSpads;
    }

    // ── tuning settings ──────────────────────────────────────────────────────

    /** VL53L0X_load_tuning_settings(): {count, address, bytes...} records, with count 0xFF carrying a
     *  host-side parameter instead; a zero count ends the buffer. */
    public void loadTuningSettings(int[] buffer) {
        int index = 0;
        while (buffer[index] != 0) {
            int writes = buffer[index++];
            if (writes == 0xFF) {
                // A host-side parameter: {0xFF, select, msb, lsb}. The three sigma parameters
                // are constants in the 1.0.4 sigma estimate and only get stored; the fourth is
                // the rate the reference SPAD management converges on.
                int select = buffer[index];
                if (select >= TUNING_PARAM_COUNT) {
                    throw new Vl53lxException("bad tuning parameter " + select + " at offset " + (index - 1));
                }
                int value = (buffer[index + 1] << 8) | buffer[index + 2];
                switch (select) {
                    case 0 -> sigmaEstRefArray = value;
                    case 1 -> sigmaEstEffPulseWidth = value;
                    case 2 -> sigmaEstEffAmbWidth = value;
                    default -> targetRefRate = value;
                }
                index += 3;
            } else if (writes <= 4) {
                int address = buffer[index++];
                p.writeMulti(address, toBytes(Arrays.copyOfRange(buffer, index, index + writes)));
                index += writes;
            } else {
                throw new Vl53lxException("bad tuning record at offset " + index);
            }
        }
    }

    // ── GPIO / interrupt ─────────────────────────────────────────────────────

    private void setGpioConfig(int functionality, boolean polarityLow) {
        p.writeByte(SYSTEM_INTERRUPT_CONFIG_GPIO, functionality);
        int data = polarityLow ? 0x00 : 0x10;
        int current = p.readByte(GPIO_HV_MUX_ACTIVE_HIGH);
        p.writeByte(GPIO_HV_MUX_ACTIVE_HIGH, (current & 0xEF) | data);
        pin0GpioFunctionality = functionality;
        clearInterrupt();
    }

    /** VL53L0X_ClearInterruptMask(): two writes, then confirm — up to three rounds. */
    @Override
    public void clearInterrupt() {
        for (int i = 0; i < 3; i++) {
            p.writeByte(SYSTEM_INTERRUPT_CLEAR, 0x01);
            p.writeByte(SYSTEM_INTERRUPT_CLEAR, 0x00);
            if ((p.readByte(RESULT_INTERRUPT_STATUS) & 0x07) == 0x00) {
                return;
            }
        }
        throw new Vl53lxException("interrupt not cleared after three rounds");
    }

    private int getInterruptMaskStatus() {
        int b = p.readByte(RESULT_INTERRUPT_STATUS);
        if ((b & 0x18) != 0) {
            throw new Vl53lxException(String.format("range error reported in RESULT_INTERRUPT_STATUS 0x%02X", b));
        }
        return b & 0x07;
    }

    @Override
    public boolean checkForDataReady() {
        return pin0GpioFunctionality == GPIO_FUNCTIONALITY_NEW_MEASURE_READY
                ? getInterruptMaskStatus() == GPIO_FUNCTIONALITY_NEW_MEASURE_READY
                : (p.readByte(RESULT_RANGE_STATUS) & 0x01) != 0;
    }

    // ── sequence steps ───────────────────────────────────────────────────────

    private boolean[] getSequenceStepEnables() {
        int config = p.readByte(SYSTEM_SEQUENCE_CONFIG);
        boolean[] steps = new boolean[SEQ_TEST.length];
        for (int i = 0; i < steps.length; i++) {
            steps[i] = (config & SEQ_TEST[i]) != 0;
        }
        return steps;
    }

    private void setSequenceStepEnable(int step, int enabled) {
        int config = p.readByte(SYSTEM_SEQUENCE_CONFIG);
        int next = enabled != 0 ? config | SEQ_SET[step] : config & SEQ_CLEAR[step];
        if (next == config) {
            return;
        }
        p.writeByte(SYSTEM_SEQUENCE_CONFIG, next);
        sequenceConfig = next;
        // The budget is spread over the enabled steps, so it is re-applied.
        setMeasurementTimingBudget(measurementTimingBudgetUs);
    }

    private int getVcselPulsePeriod(int type) {
        return decodeVcselPeriod(p.readByte(type == VCSEL_PRE_RANGE ? PRE_RANGE_CONFIG_VCSEL_PERIOD : FINAL_RANGE_CONFIG_VCSEL_PERIOD));
    }

    private long getSequenceStepTimeout(int step) {
        if (step == SEQ_TCC || step == SEQ_DSS || step == SEQ_MSRC) {
            int vcsel = getVcselPulsePeriod(VCSEL_PRE_RANGE);
            long mclks = decodeTimeout(p.readByte(MSRC_CONFIG_TIMEOUT_MACROP));
            return timeoutUs(mclks, vcsel);
        }
        if (step == SEQ_PRE_RANGE) {
            int vcsel = getVcselPulsePeriod(VCSEL_PRE_RANGE);
            long mclks = decodeTimeout(p.readWord(PRE_RANGE_CONFIG_TIMEOUT_MACROP_HI));
            return timeoutUs(mclks, vcsel);
        }
        if (step == SEQ_FINAL_RANGE) {
            boolean[] steps = getSequenceStepEnables();
            long pre = 0;
            if (steps[SEQ_PRE_RANGE]) {
                pre = decodeTimeout(p.readWord(PRE_RANGE_CONFIG_TIMEOUT_MACROP_HI));
            }
            int vcsel = getVcselPulsePeriod(VCSEL_FINAL_RANGE);
            long fin = decodeTimeout(p.readWord(FINAL_RANGE_CONFIG_TIMEOUT_MACROP_HI));
            return timeoutUs((fin - pre) & 0xFFFF, vcsel);
        }
        throw new Vl53lxException("no timeout for sequence step " + step);
    }

    private void setSequenceStepTimeout(int step, long timeoutUs) {
        if (step == SEQ_TCC || step == SEQ_DSS || step == SEQ_MSRC) {
            int vcsel = getVcselPulsePeriod(VCSEL_PRE_RANGE);
            long mclks = timeoutMclks(timeoutUs, vcsel);
            int encoded = mclks > 256 ? 255 : (int) ((mclks - 1) & 0xFF);
            p.writeByte(MSRC_CONFIG_TIMEOUT_MACROP, encoded);
        } else if (step == SEQ_PRE_RANGE) {
            int vcsel = getVcselPulsePeriod(VCSEL_PRE_RANGE);
            p.writeWord(PRE_RANGE_CONFIG_TIMEOUT_MACROP_HI, encodeTimeout(timeoutMclks(timeoutUs, vcsel)));
            preRangeTimeoutUs = timeoutUs;
        } else if (step == SEQ_FINAL_RANGE) {
            // The final-range register carries pre-range + final range, in macro periods.
            boolean[] steps = getSequenceStepEnables();
            long pre = 0;
            if (steps[SEQ_PRE_RANGE]) {
                pre = decodeTimeout(p.readWord(PRE_RANGE_CONFIG_TIMEOUT_MACROP_HI));
            }
            int vcsel = getVcselPulsePeriod(VCSEL_FINAL_RANGE);
            p.writeWord(FINAL_RANGE_CONFIG_TIMEOUT_MACROP_HI, encodeTimeout(timeoutMclks(timeoutUs, vcsel) + pre));
            finalRangeTimeoutUs = timeoutUs;
        } else {
            throw new Vl53lxException("no timeout for sequence step " + step);
        }
    }

    // ── timing budget ────────────────────────────────────────────────────────

    private static final int START_OVERHEAD_US = 1910, END_OVERHEAD_US = 960, MSRC_OVERHEAD_US = 660, TCC_OVERHEAD_US = 590;
    private static final int DSS_OVERHEAD_US = 690, PRE_RANGE_OVERHEAD_US = 660, FINAL_RANGE_OVERHEAD_US = 550;

    private void setMeasurementTimingBudget(long budgetUs) {
        if (budgetUs < MIN_TIMING_BUDGET_US) {
            throw new Vl53lxException("timing budget " + budgetUs + " us is below the " + MIN_TIMING_BUDGET_US + " us minimum");
        }
        boolean[] steps = getSequenceStepEnables();
        long[] finalBudgetUs = {budgetUs - (START_OVERHEAD_US + END_OVERHEAD_US)};
        if (steps[SEQ_TCC] || steps[SEQ_MSRC] || steps[SEQ_DSS]) {
            long msrcUs = getSequenceStepTimeout(SEQ_MSRC); // TCC, MSRC and DSS share one timeout
            if (steps[SEQ_TCC]) {
                take(finalBudgetUs, msrcUs + TCC_OVERHEAD_US, budgetUs);
            }
            if (steps[SEQ_DSS]) {
                take(finalBudgetUs, 2 * (msrcUs + DSS_OVERHEAD_US), budgetUs);
            } else if (steps[SEQ_MSRC]) {
                take(finalBudgetUs, msrcUs + MSRC_OVERHEAD_US, budgetUs);
            }
        }
        if (steps[SEQ_PRE_RANGE]) {
            take(finalBudgetUs, getSequenceStepTimeout(SEQ_PRE_RANGE) + PRE_RANGE_OVERHEAD_US, budgetUs);
        }
        if (steps[SEQ_FINAL_RANGE]) {
            finalBudgetUs[0] -= FINAL_RANGE_OVERHEAD_US; // whatever is left goes to the final range
            setSequenceStepTimeout(SEQ_FINAL_RANGE, finalBudgetUs[0]);
            measurementTimingBudgetUs = budgetUs;
        }
    }

    private static void take(long[] finalBudgetUs, long sub, long budgetUs) {
        if (sub >= finalBudgetUs[0]) {
            throw new Vl53lxException("timing budget " + budgetUs + " us is too small for the enabled sequence steps");
        }
        finalBudgetUs[0] -= sub;
    }

    private long getMeasurementTimingBudget() {
        boolean[] steps = getSequenceStepEnables();
        long budgetUs = START_OVERHEAD_US + END_OVERHEAD_US;
        if (steps[SEQ_TCC] || steps[SEQ_MSRC] || steps[SEQ_DSS]) {
            long msrcUs = getSequenceStepTimeout(SEQ_MSRC);
            if (steps[SEQ_TCC]) {
                budgetUs += msrcUs + TCC_OVERHEAD_US;
            }
            if (steps[SEQ_DSS]) {
                budgetUs += 2 * (msrcUs + DSS_OVERHEAD_US);
            } else if (steps[SEQ_MSRC]) {
                budgetUs += msrcUs + MSRC_OVERHEAD_US;
            }
        }
        if (steps[SEQ_PRE_RANGE]) {
            budgetUs += getSequenceStepTimeout(SEQ_PRE_RANGE) + PRE_RANGE_OVERHEAD_US;
        }
        if (steps[SEQ_FINAL_RANGE]) {
            budgetUs += getSequenceStepTimeout(SEQ_FINAL_RANGE) + FINAL_RANGE_OVERHEAD_US;
        }
        measurementTimingBudgetUs = budgetUs;
        return budgetUs;
    }

    private void setInterMeasurementPeriod(long periodMs) {
        int oscCalibrateVal = p.readWord(OSC_CALIBRATE_VAL);
        p.writeDword(SYSTEM_INTERMEASUREMENT_PERIOD, oscCalibrateVal != 0 ? periodMs * oscCalibrateVal : periodMs);
    }

    private long getInterMeasurementPeriod() {
        int oscCalibrateVal = p.readWord(OSC_CALIBRATE_VAL);
        long value = p.readDword(SYSTEM_INTERMEASUREMENT_PERIOD);
        return oscCalibrateVal != 0 ? value / oscCalibrateVal : value;
    }

    /**
     * Budget and period as the family asks for them: a period of 0 means back-to-back continuous,
     * anything else the timed continuous mode.
     */
    @Override
    public void setRangeTiming(int timingBudgetMs, int interMeasurementMs) {
        setMeasurementTimingBudget(timingBudgetMs * 1000L);
        setInterMeasurementPeriod(interMeasurementMs);
        setDeviceMode(interMeasurementMs == 0 ? DEVICE_MODE_CONTINUOUS_RANGING : DEVICE_MODE_CONTINUOUS_TIMED);
    }

    @Override
    public RangeTiming getRangeTiming() {
        int budgetMs = (int) (getMeasurementTimingBudget() / 1000);
        return new RangeTiming(budgetMs, (int) getInterMeasurementPeriod());
    }

    // ── ranging ──────────────────────────────────────────────────────────────

    private void setDeviceMode(int newMode) {
        if (newMode != DEVICE_MODE_SINGLE_RANGING && newMode != DEVICE_MODE_CONTINUOUS_RANGING
                && newMode != DEVICE_MODE_CONTINUOUS_TIMED) {
            throw new Vl53lxException("device mode " + newMode + " is not supported");
        }
        deviceMode = newMode;
        if (newMode != DEVICE_MODE_SINGLE_RANGING) {
            continuousMode = newMode;
        }
    }

    // The undocumented prologue of every VL53L0X_StartMeasurement(): the stop variable goes back.
    private void armStopVariable() {
        p.writeByte(0x80, 0x01);
        p.writeByte(0xFF, 0x01);
        p.writeByte(0x00, 0x00);
        p.writeByte(0x91, stopVariable);
        p.writeByte(0x00, 0x01);
        p.writeByte(0xFF, 0x00);
        p.writeByte(0x80, 0x00);
    }

    /** VL53L0X_StartMeasurement() for the continuous modes; a single shot in between is resumed from. */
    @Override
    public void startRanging() {
        if (deviceMode == DEVICE_MODE_SINGLE_RANGING && continuousMode != null) {
            setDeviceMode(continuousMode);
        }
        armStopVariable();
        if (deviceMode == DEVICE_MODE_CONTINUOUS_RANGING) {
            p.writeByte(SYSRANGE_START, SYSRANGE_MODE_BACKTOBACK);
        } else if (deviceMode == DEVICE_MODE_CONTINUOUS_TIMED) {
            p.writeByte(SYSRANGE_START, SYSRANGE_MODE_TIMED);
        } else {
            throw new Vl53lxException("start_ranging() needs a continuous device mode");
        }
    }

    /** VL53L0X_StopMeasurement(). */
    @Override
    public void stopRanging() {
        p.writeByte(SYSRANGE_START, SYSRANGE_MODE_SINGLESHOT);
        p.writeByte(0xFF, 0x01);
        p.writeByte(0x00, 0x00);
        p.writeByte(0x91, 0x00);
        p.writeByte(0x00, 0x01);
        p.writeByte(0xFF, 0x00);
    }

    private RangingData performSingleRangingMeasurement() {
        setDeviceMode(DEVICE_MODE_SINGLE_RANGING);
        armStopVariable();
        p.writeByte(SYSRANGE_START, 0x01);
        boolean cleared = false;
        for (int i = 0; i < DEFAULT_MAX_LOOP; i++) {
            if ((p.readByte(SYSRANGE_START) & SYSRANGE_MODE_START_STOP) == 0) {
                cleared = true;
                break;
            }
        }
        if (!cleared) {
            throw new Vl53lxException("the single-shot start bit never cleared");
        }
        waitDataReady(1000);
        RangingData data = getResult();
        clearInterrupt();
        return data;
    }

    private RangingData getResult() {
        return parseResultBlock(p.readMulti(RESULT_BLOCK_ADDR, RESULT_BLOCK_LEN));
    }

    // ── result decoding ──────────────────────────────────────────────────────

    // VL53L0X_RangingMeasurementData_t, the fields this port fills.
    private static final class RangingData {
        int rangeStatus;
        int distanceMm;
        long signalRateMcps;
        long ambientRateMcps;
        long effectiveSpadRtnCount;
        long dmaxMm;
        long sigmaMm;
        int deviceRangeStatus;
    }

    /**
     * VL53L0X_GetRangingMeasurementData() on the 12 bytes at 0x14: pure arithmetic over the block and
     * the cached device data, so a streamed frame decodes exactly like a polled one.
     */
    private RangingData parseResultBlock(byte[] raw) {
        if (raw.length < RESULT_BLOCK_LEN) {
            throw new Vl53lxException("result block too short: " + raw.length + " bytes");
        }
        int distance = ((raw[10] & 0xFF) << 8) + (raw[11] & 0xFF);
        RangingData data = new RangingData();
        data.signalRateMcps = (long) (((raw[6] & 0xFF) << 8) + (raw[7] & 0xFF)) << 9;   // 9.7 → 16.16
        data.ambientRateMcps = (long) (((raw[8] & 0xFF) << 8) + (raw[9] & 0xFF)) << 9;
        data.effectiveSpadRtnCount = ((long) (raw[2] & 0xFF) << 8) + (raw[3] & 0xFF);          // 8.8
        data.deviceRangeStatus = raw[0] & 0xFF;
        if (rangeFractionalEnable != 0) {
            data.distanceMm = distance >> 2;
        } else {
            data.distanceMm = distance;
        }
        getPalRangeStatus(data);
        return data;
    }

    private long getTotalXtalkRate(RangingData data) {
        if (xtalkCompensationEnable == 0) {
            return 0;
        }
        long total = data.effectiveSpadRtnCount * xtalkCompensationRateMegaCps;
        return (total + 0x80) >> 8;
    }

    /** VL53L0X_calc_sigma_estimate(), FixPoint16.16 in and out. */
    private long calcSigmaEstimate(RangingData data) {
        final long cPulseEffectiveWidthCentiNs = 800;
        final long cAmbientEffectiveWidthCentiNs = 600;
        final long cDfltFinalRangeIntegrationTimeMs = 0x00190000; // 25 ms
        final long cVcselPulseWidthPs = 4700;
        final long cSigmaEstMax = 0x028F87AE;
        final long cSigmaEstRtnMax = 0xF000;
        final long cAmbToSignalRatioMax = 0xF0000000L / cAmbientEffectiveWidthCentiNs;
        final long cTofPerMmPs = 0x0006999A;
        final long c16BitRoundingParam = 0x00008000;
        final long cMaxXtalkKcps = 0x00320000;
        final long cPllPeriodPs = 1655;

        long ambientRateKcps = (data.ambientRateMcps * 1000) >> 16;
        long xtalkCompRateMcps = getTotalXtalkRate(data);
        long totalSignalRateMcps = data.signalRateMcps + xtalkCompRateMcps;
        long peakSignalRateKcps = ((totalSignalRateMcps * 1000) + 0x8000) >> 16;
        long xtalkCompRateKcps = Math.min(xtalkCompRateMcps * 1000, cMaxXtalkKcps);

        long finalRangeTimeoutUsCached = finalRangeTimeoutUs;
        int finalRangeVcselPclks = finalRangeVcselPulsePeriod;
        long finalRangeMacroPclks = timeoutMclks(finalRangeTimeoutUsCached, finalRangeVcselPclks);
        long preRangeTimeoutUsCached = preRangeTimeoutUs;
        long preRangeMacroPclks = timeoutMclks(preRangeTimeoutUsCached, preRangeVcselPulsePeriod);

        long vcselWidth = finalRangeVcselPclks == 8 ? 2 : 3;
        long peakVcselDurationUs = vcselWidth * 2048 * (preRangeMacroPclks + finalRangeMacroPclks);
        peakVcselDurationUs = (peakVcselDurationUs + 500) / 1000;
        peakVcselDurationUs *= cPllPeriodPs;
        peakVcselDurationUs = (peakVcselDurationUs + 500) / 1000;

        long totalSignalRate2408 = (totalSignalRateMcps + 0x80) >> 8;
        long vcselTotalEventsRtn = ((totalSignalRate2408 * peakVcselDurationUs) + 0x80) >> 8;

        if (peakSignalRateKcps == 0) {
            return cSigmaEstMax;
        }
        vcselTotalEventsRtn = Math.max(1, vcselTotalEventsRtn);

        long p1 = cPulseEffectiveWidthCentiNs;
        long p2 = (ambientRateKcps << 16) / peakSignalRateKcps;
        p2 = Math.min(p2, cAmbToSignalRatioMax);
        p2 *= cAmbientEffectiveWidthCentiNs;
        long p3 = 2 * isqrt(vcselTotalEventsRtn * 12);

        long deltaTPs = data.distanceMm * cTofPerMmPs;
        long diff1Mcps = u32(u32((peakSignalRateKcps << 16) - 2 * xtalkCompRateKcps) + 500) / 1000;
        long diff2Mcps = ((peakSignalRateKcps << 16) + 500) / 1000;
        diff1Mcps <<= 8;
        long xtalkCorrection = Math.abs(diff1Mcps / diff2Mcps) << 8;

        long pwMult = deltaTPs / cVcselPulseWidthPs;
        pwMult = u32(pwMult * u32((1L << 16) - xtalkCorrection));
        pwMult = (pwMult + c16BitRoundingParam) >> 16;
        pwMult += 1L << 16;
        pwMult >>= 1; // squaring 1.xx would leave 32 bits, so the C driver halves it first
        pwMult = (pwMult * pwMult) >> 14;

        long sqr1 = ((pwMult * p1) + 0x8000) >> 16;
        sqr1 *= sqr1;
        long sqr2 = (p2 + 0x8000) >> 16;
        sqr2 *= sqr2;

        long sqrtResultCentiNs = isqrt(sqr1 + sqr2) << 16;
        long sigmaEstRtn = ((sqrtResultCentiNs + 50) / 100) / p3;
        sigmaEstRtn *= SPEED_OF_LIGHT_IN_AIR;
        sigmaEstRtn = (sigmaEstRtn + 5000) / 10000;
        sigmaEstRtn = Math.min(sigmaEstRtn, cSigmaEstRtnMax);

        long finalRangeIntegrationTimeMs = (finalRangeTimeoutUsCached + preRangeTimeoutUsCached + 500) / 1000;
        long sigmaEstRef = isqrt((cDfltFinalRangeIntegrationTimeMs + finalRangeIntegrationTimeMs / 2)
                / finalRangeIntegrationTimeMs) << 8;
        sigmaEstRef = (sigmaEstRef + 500) / 1000;

        long sigmaEstimate = 1000 * isqrt(sigmaEstRtn * sigmaEstRtn + sigmaEstRef * sigmaEstRef);
        if (peakSignalRateKcps < 1 || vcselTotalEventsRtn < 1 || sigmaEstimate > cSigmaEstMax) {
            sigmaEstimate = cSigmaEstMax;
        }
        return sigmaEstimate;
    }

    /** VL53L0X_calc_dmax(): ambient rate → max range, linear between the table's points. */
    private static long calcDmax(long ambRateMeas) {
        long[] amb = DMAX_LUT_AMB_RATE_MCPS;
        long[] dmax = DMAX_LUT_DMAX_MM;
        if (ambRateMeas <= amb[0]) {
            return dmax[0] >> 16;
        }
        if (ambRateMeas >= amb[amb.length - 1]) {
            return dmax[dmax.length - 1] >> 16;
        }
        int index1 = -1;
        for (int i = 0; i < amb.length; i++) {
            if (ambRateMeas <= amb[i]) {
                index1 = i;
                break;
            }
        }
        int index0 = index1 != 0 ? index1 - 1 : 0;
        if (index0 == index1) {
            return dmax[index0] >> 16;
        }
        long amb0 = amb[index0], amb1 = amb[index1], dmax0 = dmax[index0], dmax1 = dmax[index1];
        if (amb1 == amb0) {
            return dmax0 >> 16;
        }
        long linearSlope = u32(dmax0 - dmax1) / ((amb1 - amb0) >> 8);
        return (((amb1 - ambRateMeas) >> 8) * linearSlope + dmax1) >> 16;
    }

    /**
     * VL53L0X_get_pal_range_status(): the device status and the enabled limit checks mapped onto the
     * PAL range status. DataInit leaves the reference clip and the range-ignore check off, so only the
     * sigma check runs — arithmetic over cached data.
     */
    private void getPalRangeStatus(RangingData data) {
        int internalStatus = (data.deviceRangeStatus & 0x78) >> 3;
        boolean noneFlag = internalStatus == 0 || internalStatus == 5 || internalStatus == 7
                || internalStatus == 12 || internalStatus == 13 || internalStatus == 14 || internalStatus == 15;
        boolean sigmaLimitFlag = false;
        if (limitChecksEnable[CHECK_SIGMA_FINAL_RANGE] != 0) {
            long sigmaEstimate = calcSigmaEstimate(data);
            data.sigmaMm = sigmaEstimate >> 16;
            data.dmaxMm = calcDmax(data.ambientRateMcps);
            long sigmaLimitValue = limitChecksValue[CHECK_SIGMA_FINAL_RANGE];
            sigmaLimitFlag = sigmaLimitValue > 0 && sigmaEstimate > sigmaLimitValue;
        }
        if (noneFlag) {
            data.rangeStatus = 255;
        } else if (internalStatus == 1 || internalStatus == 2 || internalStatus == 3) {
            data.rangeStatus = 5;   // hardware fail
        } else if (internalStatus == 6 || internalStatus == 9) {
            data.rangeStatus = 4;   // phase fail
        } else if (internalStatus == 8 || internalStatus == 10) {
            data.rangeStatus = 3;   // min range
        } else if (internalStatus == 4) {
            data.rangeStatus = 2;   // signal fail
        } else if (sigmaLimitFlag) {
            data.rangeStatus = 1;   // sigma fail
        } else {
            data.rangeStatus = 0;
        }
    }

    private static Vl53lxRange asRange(RangingData r) {
        String statusText = RANGE_STATUS_NAMES.get(r.rangeStatus);
        if (statusText == null) {
            statusText = "unknown (" + r.rangeStatus + ")";
        }
        Map<String, Object> extra = new LinkedHashMap<>();
        extra.put("dmax_mm", r.dmaxMm);
        extra.put("device_range_status", (long) r.deviceRangeStatus);
        return new Vl53lxRange(r.distanceMm, r.rangeStatus, statusText,
                (r.signalRateMcps * 1000) >> 16, (r.ambientRateMcps * 1000) >> 16, r.sigmaMm,
                r.effectiveSpadRtnCount >> 8, List.of(), extra);
    }

    // ── VCSEL period and the ranging profiles ────────────────────────────────

    private void setVcselPulsePeriod(int type, int pclks) {
        Map<Integer, Integer> limits = type == VCSEL_PRE_RANGE
                ? Map.of(12, 0x18, 14, 0x30, 16, 0x40, 18, 0x50)
                : Map.of(8, 0x10, 10, 0x28, 12, 0x38, 14, 0x48);
        if (pclks % 2 != 0 || !limits.containsKey(pclks)) {
            throw new Vl53lxException("VCSEL period " + pclks + " PCLK is out of range for the "
                    + (type == VCSEL_PRE_RANGE ? "pre-range" : "final range"));
        }
        if (type == VCSEL_PRE_RANGE) {
            p.writeByte(PRE_RANGE_CONFIG_VALID_PHASE_HIGH, limits.get(pclks));
            p.writeByte(PRE_RANGE_CONFIG_VALID_PHASE_LOW, 0x08);
        } else {
            // The final range also retunes the VCSEL width and the phase-cal limits.
            int width, timeout, phasecalLim;
            switch (pclks) {
                case 8 -> {
                    width = 0x02;
                    timeout = 0x0C;
                    phasecalLim = 0x30;
                }
                case 10 -> {
                    width = 0x03;
                    timeout = 0x09;
                    phasecalLim = 0x20;
                }
                case 12 -> {
                    width = 0x03;
                    timeout = 0x08;
                    phasecalLim = 0x20;
                }
                default -> {
                    width = 0x03;
                    timeout = 0x07;
                    phasecalLim = 0x20;
                }
            }
            p.writeByte(FINAL_RANGE_CONFIG_VALID_PHASE_HIGH, limits.get(pclks));
            p.writeByte(FINAL_RANGE_CONFIG_VALID_PHASE_LOW, 0x08);
            p.writeByte(GLOBAL_CONFIG_VCSEL_WIDTH, width);
            p.writeByte(ALGO_PHASECAL_CONFIG_TIMEOUT, timeout);
            p.writeByte(0xFF, 0x01);
            p.writeByte(ALGO_PHASECAL_LIM, phasecalLim);
            p.writeByte(0xFF, 0x00);
        }
        int reg = encodeVcselPeriod(pclks);
        if (type == VCSEL_PRE_RANGE) {
            long preTimeout = getSequenceStepTimeout(SEQ_PRE_RANGE);
            long msrcTimeout = getSequenceStepTimeout(SEQ_MSRC);
            p.writeByte(PRE_RANGE_CONFIG_VCSEL_PERIOD, reg);
            setSequenceStepTimeout(SEQ_PRE_RANGE, preTimeout);
            setSequenceStepTimeout(SEQ_MSRC, msrcTimeout);
            preRangeVcselPulsePeriod = pclks;
        } else {
            long finalTimeout = getSequenceStepTimeout(SEQ_FINAL_RANGE);
            p.writeByte(FINAL_RANGE_CONFIG_VCSEL_PERIOD, reg);
            setSequenceStepTimeout(SEQ_FINAL_RANGE, finalTimeout);
            finalRangeVcselPulsePeriod = pclks;
        }
        setMeasurementTimingBudget(measurementTimingBudgetUs);
        performPhaseCalibration();
    }

    private void performPhaseCalibration() {
        int savedSequenceConfig = sequenceConfig;
        p.writeByte(SYSTEM_SEQUENCE_CONFIG, 0x02);
        performSingleRefCalibration(0x00);
        p.writeByte(SYSTEM_SEQUENCE_CONFIG, savedSequenceConfig);
        sequenceConfig = savedSequenceConfig;
    }

    /** Apply one of {@link #modes()}: the sigma and signal limits, the budget and both VCSEL periods. */
    @Override
    public Integer setMode(String name) {
        ModeSetting settings = null;
        for (ModeSetting m : MODE_SETTINGS) {
            if (m.name().equals(name)) {
                settings = m;
                break;
            }
        }
        if (settings == null) {
            throw new IllegalArgumentException("no such mode: " + name + " (have " + String.join(", ", modes()) + ")");
        }
        setLimitCheckEnable(CHECK_SIGMA_FINAL_RANGE, 1);
        setLimitCheckEnable(CHECK_SIGNAL_RATE_FINAL_RANGE, 1);
        setLimitCheckValue(CHECK_SIGNAL_RATE_FINAL_RANGE, (long) (settings.signal() * 65536));
        setLimitCheckValue(CHECK_SIGMA_FINAL_RANGE, settings.sigma() * 65536L);
        setMeasurementTimingBudget(settings.budgetUs());
        setVcselPulsePeriod(VCSEL_PRE_RANGE, settings.pre());
        setVcselPulsePeriod(VCSEL_FINAL_RANGE, settings.fin());
        mode = name;
        return null;
    }

    @Override
    public String getMode() {
        return mode;
    }

    /** By the final-range VCSEL period read from the sensor; null for a period no profile sets. */
    @Override
    public Integer reachMm() {
        return REACH_BY_FINAL_PCLKS_MM.get(getVcselPulsePeriod(VCSEL_FINAL_RANGE));
    }

    @Override
    public Map<String, Object> driverInfo() {
        Map<String, Object> info = new LinkedHashMap<>();
        info.put("signal_mcps", getLimitCheckValue(CHECK_SIGNAL_RATE_FINAL_RANGE) / 65536.0);
        info.put("sigma_mm", getLimitCheckValue(CHECK_SIGMA_FINAL_RANGE) / 65536.0);
        info.put("budget_us", getMeasurementTimingBudget());
        info.put("pre_pclks", getVcselPulsePeriod(VCSEL_PRE_RANGE));
        info.put("final_pclks", getVcselPulsePeriod(VCSEL_FINAL_RANGE));
        return info;
    }

    // ── offset ───────────────────────────────────────────────────────────────

    /** The register is 10.2 format in mm: steps of 250 µm. */
    public void setOffsetUm(long offsetUm) {
        offsetUm = Math.max(-512000, Math.min(511000, offsetUm));
        long steps = Math.abs(offsetUm) / 250;
        long encoded = offsetUm >= 0 ? steps : 4096 - steps;
        p.writeWord(ALGO_PART_TO_PART_RANGE_OFFSET_MM, (int) (encoded & 0xFFFF));
    }

    public long getOffsetUm() {
        int reg = p.readWord(ALGO_PART_TO_PART_RANGE_OFFSET_MM) & 0x0FFF;
        return reg > 2047 ? (reg - 4096) * 250L : reg * 250L;
    }

    @Override
    public void setOffset(int offsetMm) {
        setOffsetUm(offsetMm * 1000L);
    }

    @Override
    public int getOffset() {
        return (int) Math.rint(getOffsetUm() / 1000.0);
    }

    /** VL53L0X_perform_offset_calibration() with 50 samples → µm. */
    public long performOffsetCalibration(int calDistanceMm) {
        return performOffsetCalibration(calDistanceMm, 50);
    }

    /** VL53L0X_perform_offset_calibration() → µm. TCC and the range-ignore threshold are off for the run. */
    public long performOffsetCalibration(int calDistanceMm, int nbSamples) {
        if (calDistanceMm <= 0) {
            throw new Vl53lxException("the calibration distance must be positive");
        }
        setOffsetUm(0);
        boolean tccWasOn = getSequenceStepEnables()[SEQ_TCC];
        setSequenceStepEnable(SEQ_TCC, 0);
        setLimitCheckEnable(CHECK_RANGE_IGNORE_THRESHOLD, 0);
        long sum = 0;
        int count = 0;
        for (int i = 0; i < nbSamples; i++) {
            RangingData data = performSingleRangingMeasurement();
            if (data.rangeStatus == 0) {
                sum = (sum + data.distanceMm) & 0xFFFF;
                count++;
            }
        }
        if (count == 0) {
            throw new Vl53lxException("offset calibration got no valid measurement - check that a target is in front of the sensor");
        }
        long meanMm = (2 * sum + count) / (2L * count); // round half up
        if (meanMm == 0) {
            throw new Vl53lxException("the mean range came out 0 mm - the target is closer than the part-to-part offset; "
                    + "move it out to 100..400 mm and calibrate again");
        }
        long offsetUm = (calDistanceMm - meanMm) * 1000;
        setOffsetUm(offsetUm);
        if (tccWasOn) {
            setSequenceStepEnable(SEQ_TCC, 1);
        }
        return offsetUm;
    }

    @Override
    public int calibrateOffset(int targetDistMm, Integer nbSamples) {
        return (int) Math.rint(performOffsetCalibration(targetDistMm, nbSamples != null ? nbSamples : 50) / 1000.0);
    }

    // ── crosstalk ────────────────────────────────────────────────────────────

    private void setXtalkEnable(int enable) {
        long rate = enable != 0 ? xtalkCompensationRateMegaCps : 0;
        p.writeWord(CROSSTALK_COMPENSATION_PEAK_RATE_MCPS, (int) ((rate >> 3) & 0xFFFF)); // 16.16 → 3.13
        xtalkCompensationEnable = enable != 0 ? 1 : 0;
    }

    private void setXtalkRateMcps(long rateMcps) {
        if (xtalkCompensationEnable != 0) {
            p.writeWord(CROSSTALK_COMPENSATION_PEAK_RATE_MCPS, (int) ((rateMcps >> 3) & 0xFFFF));
        }
        xtalkCompensationRateMegaCps = rateMcps;
    }

    private long getXtalkRateMcps() {
        return (long) p.readWord(CROSSTALK_COMPENSATION_PEAK_RATE_MCPS) << 3;
    }

    @Override
    public void setXtalk(int xtalkKcps) {
        setXtalkEnable(1);
        setXtalkRateMcps(((long) xtalkKcps << 16) / 1000);
    }

    @Override
    public int getXtalk() {
        return (int) ((getXtalkRateMcps() * 1000) >> 16);
    }

    /** VL53L0X_perform_xtalk_calibration() with 50 samples → the rate, Mcps 16.16. */
    public long performXtalkCalibration(int calDistanceMm) {
        return performXtalkCalibration(calDistanceMm, 50);
    }

    /** VL53L0X_perform_xtalk_calibration() → the rate, Mcps 16.16. */
    public long performXtalkCalibration(int calDistanceMm, int nbSamples) {
        if (calDistanceMm <= 0) {
            throw new Vl53lxException("the calibration distance must be positive");
        }
        setXtalkEnable(0);
        setLimitCheckEnable(CHECK_RANGE_IGNORE_THRESHOLD, 0);
        long sumRanging = 0, sumSignal = 0, sumSpads = 0;
        int count = 0;
        for (int i = 0; i < nbSamples; i++) {
            RangingData data = performSingleRangingMeasurement();
            if (data.rangeStatus == 0) {
                // The C sums are uint16 / uint32; kept that way on purpose.
                sumRanging = (sumRanging + data.distanceMm) & 0xFFFF;
                sumSignal = u32(sumSignal + data.signalRateMcps);
                sumSpads = (sumSpads + data.effectiveSpadRtnCount / 256) & 0xFFFF;
                count++;
            }
        }
        if (count == 0) {
            throw new Vl53lxException("crosstalk calibration got no valid measurement - check that a target is in front of the sensor");
        }
        long meanSignal = sumSignal / count;
        long meanRange = u32(sumRanging << 16) / count;
        long meanSpads = u32(sumSpads << 16) / count;
        long meanSpadsInt = (meanSpads + 0x8000) >> 16;
        long calDistance = (long) calDistanceMm << 16;
        long rate;
        if (meanSpadsInt == 0 || meanRange >= calDistance) {
            rate = 0;
        } else {
            long perSpad = meanSignal / meanSpadsInt;
            perSpad = u32(perSpad * ((1L << 16) - meanRange / calDistanceMm));
            rate = (perSpad + 0x8000) >> 16;
        }
        setXtalkEnable(1);
        setXtalkRateMcps(rate);
        return rate;
    }

    @Override
    public int calibrateXtalk(int targetDistMm, Integer nbSamples) {
        return (int) ((performXtalkCalibration(targetDistMm, nbSamples != null ? nbSamples : 50) * 1000) >> 16);
    }

    // ── reference SPAD management ────────────────────────────────────────────

    public RefSpads getReferenceSpads() {
        return new RefSpads(referenceSpadCount, referenceSpadType);
    }

    private void setRefSpadMap(int[] spadArray) {
        p.writeMulti(GLOBAL_CONFIG_SPAD_ENABLES_REF_0, toBytes(spadArray));
    }

    private int[] getRefSpadMap() {
        byte[] raw = p.readMulti(GLOBAL_CONFIG_SPAD_ENABLES_REF_0, REF_SPAD_BUFFER_SIZE);
        int[] out = new int[raw.length];
        for (int i = 0; i < raw.length; i++) {
            out[i] = raw[i] & 0xFF;
        }
        return out;
    }

    private int enableRefSpads(int apertureSpads, int[] spadArray, int start, int offset, int spadCount) {
        int current = offset;
        for (int i = 0; i < spadCount; i++) {
            int next = nextGoodSpad(refGoodSpadMap, REF_SPAD_BUFFER_SIZE, current);
            if (next == -1) {
                throw new Vl53lxException("ran out of good reference SPADs");
            }
            if (isAperture(start + next) != (apertureSpads != 0)) {
                throw new Vl53lxException("the good SPAD map leaves the requested quadrant");
            }
            current = next;
            spadArray[current / 8] |= 1 << (current % 8);
            current++;
        }
        setRefSpadMap(spadArray);
        if (!Arrays.equals(getRefSpadMap(), spadArray)) {
            throw new Vl53lxException("reference SPAD map did not read back");
        }
        return current;
    }

    private int performRefSignalMeasurement() {
        int savedSequenceConfig = sequenceConfig;
        p.writeByte(SYSTEM_SEQUENCE_CONFIG, 0xC0);
        performSingleRangingMeasurement();
        p.writeByte(0xFF, 0x01);
        int peak = p.readWord(RESULT_PEAK_SIGNAL_RATE_REF);
        p.writeByte(0xFF, 0x00);
        p.writeByte(SYSTEM_SEQUENCE_CONFIG, savedSequenceConfig);
        sequenceConfig = savedSequenceConfig;
        return peak;
    }

    /**
     * Re-measure the reference SPADs on a live session → {@link RefSpads} (count, is aperture). The stop
     * variable goes back first (a stopped session leaves register 0x91 at 0, and the rate then reads far
     * too high).
     */
    @Override
    public Object performRefSpadManagement() {
        armStopVariable();
        return refSpadManagement();
    }

    // VL53L0X_perform_ref_spad_management(): the minimum non-aperture set, then one SPAD at a time until the
    // reference rate lands nearest 20 Mcps; aperture SPADs when even the minimum overshoots.
    private RefSpads refSpadManagement() {
        final int startSelect = 0xB4;
        final int minimumSpadCount = 3;
        final int maxSpadCount = 44;
        int[] spadArray = new int[REF_SPAD_BUFFER_SIZE];
        p.writeByte(0xFF, 0x01);
        p.writeByte(DYNAMIC_SPAD_REF_EN_START_OFFSET, 0x00);
        p.writeByte(DYNAMIC_SPAD_NUM_REQUESTED_REF_SPAD, 0x2C);
        p.writeByte(0xFF, 0x00);
        p.writeByte(GLOBAL_CONFIG_REF_EN_START_SELECT, startSelect);
        p.writeByte(POWER_MANAGEMENT_GO1_POWER_FORCE, 0x00);
        performRefCalibration();

        int needApt = 0;
        int current = enableRefSpads(needApt, spadArray, startSelect, 0, minimumSpadCount);
        int peak = performRefSignalMeasurement();
        if (peak > targetRefRate) {
            spadArray = new int[REF_SPAD_BUFFER_SIZE];
            while (!isAperture(startSelect + current) && current < maxSpadCount) {
                current++;
            }
            needApt = 1;
            current = enableRefSpads(needApt, spadArray, startSelect, current, minimumSpadCount);
            peak = performRefSignalMeasurement();
            if (peak > targetRefRate) {
                storeRefSpads(minimumSpadCount, 1);
                return new RefSpads(minimumSpadCount, 1);
            }
        }
        int refSpadCount = minimumSpadCount;
        int[] lastSpadArray = spadArray.clone();
        int lastDiff = Math.abs(peak - targetRefRate);
        while (peak < targetRefRate) {
            int next = nextGoodSpad(refGoodSpadMap, REF_SPAD_BUFFER_SIZE, current);
            if (next == -1) {
                throw new Vl53lxException("ran out of good reference SPADs");
            }
            if (isAperture(startSelect + next) != (needApt != 0)) {
                break; // the quadrant is exhausted
            }
            refSpadCount++;
            current = next;
            if (current / 8 >= REF_SPAD_BUFFER_SIZE) {
                throw new Vl53lxException("reference SPAD index out of range");
            }
            spadArray[current / 8] |= 1 << (current % 8);
            current++;
            setRefSpadMap(spadArray);
            peak = performRefSignalMeasurement();
            int diff = Math.abs(peak - targetRefRate);
            if (peak > targetRefRate) {
                if (diff > lastDiff) {
                    setRefSpadMap(lastSpadArray); // the previous map came closer
                    spadArray = lastSpadArray.clone();
                    refSpadCount--;
                }
                break;
            }
            lastDiff = diff;
            lastSpadArray = spadArray.clone();
        }
        storeRefSpads(refSpadCount, needApt);
        return new RefSpads(refSpadCount, needApt);
    }

    private void storeRefSpads(int count, int isApertureSpads) {
        referenceSpadCount = count;
        referenceSpadType = isApertureSpads;
    }

    // ── limit checks ─────────────────────────────────────────────────────────

    private void setLimitCheckEnable(int check, int enable) {
        if (check >= CHECK_NUMBER_OF_CHECKS) {
            throw new Vl53lxException("no limit check " + check);
        }
        long value = enable == 0 ? 0 : limitChecksValue[check];
        int disable = enable == 0 ? 1 : 0;
        if (check == CHECK_SIGNAL_RATE_FINAL_RANGE) {
            p.writeWord(FINAL_RANGE_CONFIG_MIN_COUNT_RATE_RTN_LIMIT, (int) ((value >> 9) & 0xFFFF));
        } else if (check == CHECK_SIGNAL_RATE_MSRC) {
            int current = p.readByte(MSRC_CONFIG_CONTROL);
            p.writeByte(MSRC_CONFIG_CONTROL, (current & 0xFE) | (disable << 1));
        } else if (check == CHECK_SIGNAL_RATE_PRE_RANGE) {
            int current = p.readByte(MSRC_CONFIG_CONTROL);
            p.writeByte(MSRC_CONFIG_CONTROL, (current & 0xEF) | (disable << 4));
        }
        limitChecksEnable[check] = enable == 0 ? 0 : 1; // the other three checks are host-side only
    }

    private void setLimitCheckValue(int check, long value) {
        if (limitChecksEnable[check] == 0) {
            limitChecksValue[check] = value; // disabled: kept here
            return;
        }
        if (check == CHECK_SIGNAL_RATE_FINAL_RANGE) {
            p.writeWord(FINAL_RANGE_CONFIG_MIN_COUNT_RATE_RTN_LIMIT, (int) ((value >> 9) & 0xFFFF));
        } else if (check == CHECK_SIGNAL_RATE_MSRC || check == CHECK_SIGNAL_RATE_PRE_RANGE) {
            p.writeWord(PRE_RANGE_MIN_COUNT_RATE_RTN_LIMIT, (int) ((value >> 9) & 0xFFFF));
        }
        limitChecksValue[check] = value;
    }

    private long getLimitCheckValue(int check) {
        if (check == CHECK_SIGNAL_RATE_FINAL_RANGE) {
            return (long) p.readWord(FINAL_RANGE_CONFIG_MIN_COUNT_RATE_RTN_LIMIT) << 9;
        }
        if (check == CHECK_SIGNAL_RATE_MSRC || check == CHECK_SIGNAL_RATE_PRE_RANGE) {
            return (long) p.readWord(PRE_RANGE_MIN_COUNT_RATE_RTN_LIMIT) << 9;
        }
        return limitChecksValue[check];
    }
}
