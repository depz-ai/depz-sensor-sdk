package ai.depz.sensor.sensors.vl53lx;

import ai.depz.sensor.sensors.vl53l4.RangeTiming;
import ai.depz.sensor.sensors.vl53l4.Vl53l4Uld;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.function.BiConsumer;

/**
 * The VL53L1 die ({@code uld/vl53l1_die.py}): register map, the 17-byte result block every part of
 * it shares, and the body of the VL53L4CD ULD that {@link Vl53l4UldDriver} and {@link Vl53l3UlpDriver}
 * fill in; {@link Vl53l1UldDriver} (ST's VL53L1X ULD) overrides where that driver differs. A faithful
 * port: register sequences and integer widths as in the C drivers.
 */
public abstract class Vl53l1Die extends Vl53lxDriver {
    // The register map, spelled as the full VL53L1 map spells it.
    public static final int SOFT_RESET = 0x0000;
    public static final int I2C_SLAVE_DEVICE_ADDRESS = 0x0001;
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
    public static final int SYSTEM_INTERRUPT_CONFIG_GPIO = 0x0046;
    public static final int PHASECAL_CONFIG_TIMEOUT_MACROP = 0x004B;
    public static final int RANGE_CONFIG_TIMEOUT_MACROP_A_HI = 0x005E;
    public static final int RANGE_CONFIG_VCSEL_PERIOD_A = 0x0060;
    public static final int RANGE_CONFIG_TIMEOUT_MACROP_B_HI = 0x0061;
    public static final int RANGE_CONFIG_VCSEL_PERIOD_B = 0x0063;
    public static final int RANGE_CONFIG_SIGMA_THRESH = 0x0064;
    public static final int MIN_COUNT_RATE_RTN_LIMIT_MCPS = 0x0066;
    public static final int RANGE_CONFIG_VALID_PHASE_HIGH = 0x0069;
    public static final int INTERMEASUREMENT_MS = 0x006C;
    public static final int THRESH_HIGH = 0x0072;
    public static final int THRESH_LOW = 0x0074;
    public static final int SD_CONFIG_WOI_SD0 = 0x0078;
    public static final int SD_CONFIG_INITIAL_PHASE_SD0 = 0x007A;
    public static final int ROI_CONFIG_USER_ROI_CENTRE_SPAD = 0x007F;
    public static final int ROI_CONFIG_USER_ROI_XY_SIZE = 0x0080;
    public static final int SYSTEM_INTERRUPT_CLEAR = 0x0086;
    public static final int SYSTEM_MODE_START = 0x0087;
    public static final int RESULT_RANGE_STATUS = 0x0089;
    public static final int RESULT_OSC_CALIBRATE_VAL = 0x00DE;
    public static final int FIRMWARE_SYSTEM_STATUS = 0x00E5;
    public static final int IDENTIFICATION_MODEL_ID = 0x010F;
    public static final int ROI_CONFIG_MODE_ROI_CENTRE_SPAD = 0x013E;

    public static final int WINDOW_BELOW = 0;
    public static final int WINDOW_ABOVE = 1;
    public static final int WINDOW_OUT = 2;
    public static final int WINDOW_IN = 3;

    /** The configuration blob spans 0x2D..0x87 on every part; byte 0 is always the Fast Mode Plus byte. */
    public static final int CONFIG_ADDR = 0x002D;
    public static final int CONFIG_END = 0x0087;
    public static final int CONFIG_FMP_BYTE = 0x12;

    public static final int RESULT_BLOCK_ADDR = RESULT_RANGE_STATUS;
    public static final int RESULT_BLOCK_LEN = 17;

    private static final int I2C_KHZ_BOOT = 400;

    // GetResult() raw status → ULD status (status_rtn[24]).
    private static final int[] STATUS_RTN = {
        255, 255, 255, 5, 2, 4, 1, 7, 3, 0, 255, 255, 9, 13, 255, 255, 255, 255, 10, 6, 255, 255, 11, 12,
    };

    protected Vl53l1Die(Vl53lxPlatform platform, String product) {
        super(platform, product);
    }

    @Override
    public int addrWidth() {
        return 2;
    }

    @Override
    public int[][] clearSteps() {
        return new int[][] {{SYSTEM_INTERRUPT_CLEAR, 0x01}};
    }

    /** With Fast Mode Plus on the sensor pad. */
    @Override
    public int maxKhz() {
        return 1000;
    }

    /** The 91-byte blob written at {@link #CONFIG_ADDR}. */
    protected abstract byte[] configuration();

    /** What stopRanging writes: the L4CD ULD 0x80, the L3CX ULP and the L1X ULD 0x00. */
    protected int stopMode() {
        return 0x80;
    }

    /** Where the signal rate comes from: 0x008E (offset 5) on the L4CD ULD, 0x0098 (15) on the L1X ULD. */
    protected int signalAt() {
        return 5;
    }

    /** Per-SPAD scale: 256 on the L4CD ULD, 25 on the L1X ULD (ST's own difference, kept). */
    protected int perSpadK() {
        return 256;
    }

    /** How far a target can be before the die fails it with status 4, mm. */
    public static long phaseWindowMm(long fastOscFrequency, long validPhaseHigh) {
        long pllPeriod = (1L << 30) / fastOscFrequency;
        return (validPhaseHigh * pllPeriod * (299704 >> 3)) >> 25;
    }

    // ── result block ─────────────────────────────────────────────────────────

    @Override
    public int[] streamBlock() {
        return new int[] {RESULT_BLOCK_ADDR, RESULT_BLOCK_LEN};
    }

    private static int u16be(byte[] b, int off) {
        return (b[off] & 0xFF) << 8 | (b[off + 1] & 0xFF);
    }

    @Override
    public Vl53lxRange decode(byte[] raw) {
        if (raw.length < RESULT_BLOCK_LEN) {
            throw new Vl53lxException("result block too short: " + raw.length + " bytes");
        }
        int status = raw[0] & 0x1F;
        if (status < STATUS_RTN.length) {
            status = STATUS_RTN[status];
        }
        int rawSpads = u16be(raw, 3);                  // 0x008C, 8.8
        int signalKcps = u16be(raw, signalAt()) * 8;
        int ambientKcps = u16be(raw, 7) * 8;           // 0x0090
        int k = perSpadK();
        Map<String, Object> extra = new LinkedHashMap<>();
        extra.put("signal_per_spad_kcps", rawSpads != 0 ? (long) signalKcps * k / rawSpads : 0L);
        extra.put("ambient_per_spad_kcps", rawSpads != 0 ? (long) ambientKcps * k / rawSpads : 0L);
        extra.put("stream_count", (long) (raw[2] & 0xFF));
        return new Vl53lxRange(
                u16be(raw, 13),                        // 0x0096
                status, statusText(status), signalKcps, ambientKcps,
                Math.floorDiv(u16be(raw, 9), 4),       // 0x0092, 14.2: whole mm, as the ULD
                Math.floorDiv(rawSpads, 256), List.of(), extra);   // 8.8: whole SPADs
    }

    public static String statusText(int status) {
        return Vl53l4Uld.RANGE_STATUS_NAMES.getOrDefault(status, "unknown (" + status + ")");
    }

    /** One block read instead of the C driver's six register reads; the decoding is identical. */
    public Vl53lxRange getResult() {
        return decode(p.readMulti(RESULT_BLOCK_ADDR, RESULT_BLOCK_LEN));
    }

    @Override
    public Vl53lxRange readMeasurement() {
        return getResult();
    }

    // ── identity and init ────────────────────────────────────────────────────

    @Override
    public int modelId() {
        return p.readWord(IDENTIFICATION_MODEL_ID);
    }

    public int bootState() {
        return p.readByte(FIRMWARE_SYSTEM_STATUS);
    }

    public void waitBoot(long timeoutMs) {
        long deadline = System.nanoTime() + timeoutMs * 1_000_000L;
        while (true) {
            if (bootState() == 0x03) {
                return;
            }
            if (System.nanoTime() > deadline) {
                throw new Vl53lxException("timeout waiting for FIRMWARE__SYSTEM_STATUS == 0x03");
            }
            p.sleepMs(1);
        }
    }

    public void waitBoot() {
        waitBoot(1000);
    }

    /** The die's soft reset; a part that no longer acknowledges writes is reset through XSHUT. */
    public void resetDevice(long timeoutMs) {
        try {
            p.writeByte(SOFT_RESET, 0x00);
            p.sleepMs(1);
            p.writeByte(SOFT_RESET, 0x01);
            waitBoot(timeoutMs);
        } catch (Vl53lxException | Vl53lxProtocolException e) {
            p.xshutReset();
            waitBoot(timeoutMs);
        }
    }

    public void resetDevice() {
        resetDevice(1000);
    }

    /**
     * Configuration at 400 kHz (the only speed an unconfigured sensor is specified for), Fast Mode Plus
     * byte first, then the bus at {@link #maxKhz()}, VHV, and 50 ms continuous.
     */
    @Override
    public void sensorInit() {
        p.setAddrWidth(addrWidth());
        p.setI2cSpeed(I2C_KHZ_BOOT);
        initBoot();
        byte[] config = configuration().clone();
        config[0] = (byte) CONFIG_FMP_BYTE;
        // The C driver writes the 91 bytes one at a time; auto-increment makes it one write.
        p.writeMulti(CONFIG_ADDR, config);
        if (maxKhz() != I2C_KHZ_BOOT) {
            p.setI2cSpeed(maxKhz());
        }
        initAfterConfig();

        p.writeByte(SYSTEM_MODE_START, 0x40); // start VHV
        waitDataReady(1000);
        clearInterrupt();
        stopRanging();
        p.writeByte(VHV_CONFIG_TIMEOUT_MACROP_LOOP_BOUND, 0x09);
        p.writeByte(0x000B, 0x00);
        initExtra();

        setRangeTiming(50, 0);
    }

    /** How a part gets to a booted die (the L1X ULD resets first). */
    protected void initBoot() {
        waitBoot();
    }

    /** Writes right after the configuration blob, before VHV. */
    protected void initAfterConfig() {}

    /** Writes at the end of init, before the timing is set. */
    protected void initExtra() {}

    // ── ranging and the interrupt ────────────────────────────────────────────

    @Override
    public void clearInterrupt() {
        p.writeByte(SYSTEM_INTERRUPT_CLEAR, 0x01);
    }

    @Override
    public void startRanging() {
        // 0 = continuous, anything else = autonomous low power.
        int mode = p.readDword(INTERMEASUREMENT_MS) == 0 ? 0x21 : 0x40;
        p.writeByte(SYSTEM_MODE_START, mode);
    }

    @Override
    public void stopRanging() {
        p.writeByte(SYSTEM_MODE_START, stopMode());
    }

    public int getInterruptPolarity() {
        return (p.readByte(GPIO_HV_MUX_CTRL) & 0x10) != 0 ? 0 : 1;
    }

    public void setInterruptPolarity(int polarity) {
        int temp = p.readByte(GPIO_HV_MUX_CTRL) & 0xEF;
        p.writeByte(GPIO_HV_MUX_CTRL, temp | ((polarity & 1) == 0 ? 1 : 0) << 4);
    }

    @Override
    public boolean checkForDataReady() {
        int intPol = getInterruptPolarity(); // polarity first, as both C drivers read it
        return (p.readByte(GPIO_TIO_HV_STATUS) & 1) == intPol;
    }

    // ── timing ───────────────────────────────────────────────────────────────

    private static long macroPeriodUs(long osc) {
        return ((2304 * (0x40000000L / osc)) & 0xFFFFFFFFL) >> 6;
    }

    @Override
    public void setRangeTiming(int timingBudgetMs, int interMeasurementMs) {
        int osc = p.readWord(OSC_FREQUENCY);
        if (osc == 0) {
            throw new Vl53lxException("osc_frequency reads 0");
        }
        if (timingBudgetMs < 10 || timingBudgetMs > 200) {
            throw new Vl53lxException("timing_budget_ms must be 10..200");
        }

        long budgetUs = timingBudgetMs * 1000L;
        long macroPeriodUs = macroPeriodUs(osc);
        if (interMeasurementMs == 0) { // continuous
            p.writeDword(INTERMEASUREMENT_MS, 0);
            budgetUs -= 2500;
        } else if (interMeasurementMs > timingBudgetMs) { // autonomous low power
            int clockPll = p.readWord(RESULT_OSC_CALIBRATE_VAL) & 0x3FF;
            double factor = 1.055 * interMeasurementMs * clockPll;
            p.writeDword(INTERMEASUREMENT_MS, (long) factor);
            budgetUs = (budgetUs - 4300) / 2;
        } else {
            throw new Vl53lxException("inter_measurement_ms must be 0 or > timing_budget_ms");
        }

        budgetUs = (budgetUs << 12) & 0xFFFFFFFFL;
        int[][] regs = {{RANGE_CONFIG_TIMEOUT_MACROP_A_HI, 16}, {RANGE_CONFIG_TIMEOUT_MACROP_B_HI, 12}};
        for (int[] rm : regs) {
            long tmp = ((macroPeriodUs * rm[1]) & 0xFFFFFFFFL) >> 6;
            long ls = (budgetUs + (tmp >> 1)) / tmp - 1;
            int ms = 0;
            while ((ls & 0xFFFFFF00L) != 0) {
                ls >>= 1;
                ms++;
            }
            p.writeWord(rm[0], (int) (((ms << 8) + (ls & 0xFF)) & 0xFFFF));
        }
    }

    @Override
    public RangeTiming getRangeTiming() {
        long tmp = p.readDword(INTERMEASUREMENT_MS);
        int clockPll = p.readWord(RESULT_OSC_CALIBRATE_VAL) & 0x3FF;
        clockPll = (int) (1.065 * clockPll) & 0xFFFF;
        int interMs = clockPll != 0 ? (int) (tmp / clockPll & 0xFFFF) : 0;

        int osc = p.readWord(OSC_FREQUENCY);
        if (osc == 0) {
            throw new Vl53lxException("osc_frequency reads 0");
        }
        int macropHigh = p.readWord(RANGE_CONFIG_TIMEOUT_MACROP_A_HI);

        long macroPeriodUs = macroPeriodUs(osc);
        long ls = (long) (macropHigh & 0x00FF) << 4;
        long ms = (macropHigh & 0xFF00) >> 8;
        ms = (0x04 - (ms - 1) - 1) & 0xFFFFFFFFL;
        macroPeriodUs = (macroPeriodUs * 16) & 0xFFFFFFFFL;
        long budget = ((((ls + 1) * (macroPeriodUs >> 6)) - ((macroPeriodUs >> 6) >> 1)) & 0xFFFFFFFFL) >> 12;
        if (ms < 12) {
            budget >>= (int) ms;
        }
        budget = tmp == 0 ? budget + 2500 : budget * 2 + 4300;
        return new RangeTiming((int) (budget / 1000), interMs);
    }

    /** Read off the loaded configuration: 1388 mm for the 0x38 window, 4563 mm for 0xB8. */
    @Override
    public Integer reachMm() {
        int osc = p.readWord(OSC_FREQUENCY);
        if (osc == 0) {
            throw new Vl53lxException("osc_frequency reads 0");
        }
        return (int) phaseWindowMm(osc, p.readByte(RANGE_CONFIG_VALID_PHASE_HIGH));
    }

    // ── thresholds, offset, xtalk, ROI, temperature ──────────────────────────

    @Override
    public void setDetectionThresholds(int distanceLowMm, int distanceHighMm, int window) {
        p.writeByte(SYSTEM_INTERRUPT_CONFIG_GPIO, window);
        p.writeWord(THRESH_HIGH, distanceHighMm);
        p.writeWord(THRESH_LOW, distanceLowMm);
    }

    @Override
    public Thresholds getDetectionThresholds() {
        int high = p.readWord(THRESH_HIGH);
        int low = p.readWord(THRESH_LOW);
        return new Thresholds(low, high, p.readByte(SYSTEM_INTERRUPT_CONFIG_GPIO) & 0x07);
    }

    @Override
    public void setSignalThreshold(int signalKcps) {
        p.writeWord(MIN_COUNT_RATE_RTN_LIMIT_MCPS, signalKcps >> 3);
    }

    @Override
    public int getSignalThreshold() {
        return (p.readWord(MIN_COUNT_RATE_RTN_LIMIT_MCPS) << 3) & 0xFFFF;
    }

    @Override
    public void setSigmaThreshold(int sigmaMm) {
        if (sigmaMm > 0xFFFF >> 2) {
            throw new Vl53lxException("sigma_mm must be <= 16383");
        }
        p.writeWord(RANGE_CONFIG_SIGMA_THRESH, sigmaMm << 2);
    }

    @Override
    public int getSigmaThreshold() {
        return p.readWord(RANGE_CONFIG_SIGMA_THRESH) >> 2;
    }

    @Override
    public void setOffset(int offsetMm) {
        p.writeWord(RANGE_OFFSET_MM, (offsetMm * 4) & 0xFFFF);
        p.writeWord(INNER_OFFSET_MM, 0);
        p.writeWord(OUTER_OFFSET_MM, 0);
    }

    @Override
    public int getOffset() {
        int temp = ((p.readWord(RANGE_OFFSET_MM) << 3) & 0xFFFF) >> 5;
        return temp > 1024 ? temp - 2048 : temp;
    }

    @Override
    public void setXtalk(int xtalkKcps) {
        p.writeWord(XTALK_X_PLANE_GRADIENT_KCPS, 0x0000);
        p.writeWord(XTALK_Y_PLANE_GRADIENT_KCPS, 0x0000);
        p.writeWord(XTALK_PLANE_OFFSET_KCPS, (xtalkKcps << 9) & 0xFFFF);
    }

    @Override
    public int getXtalk() {
        return (int) roundHalfAway(p.readWord(XTALK_PLANE_OFFSET_KCPS) / 512.0);
    }

    /** An X by Y window of SPADs, 4..16 each way. */
    @Override
    public void setRoi(int x, int y) {
        int opticalCenter = p.readByte(ROI_CONFIG_MODE_ROI_CENTRE_SPAD);
        x = Math.min(x, 16);
        y = Math.min(y, 16);
        if (x > 10 || y > 10) {
            opticalCenter = 199;
        }
        p.writeByte(ROI_CONFIG_USER_ROI_CENTRE_SPAD, opticalCenter);
        p.writeByte(ROI_CONFIG_USER_ROI_XY_SIZE, (y - 1) << 4 | (x - 1));
    }

    @Override
    public int[] getRoi() {
        int temp = p.readByte(ROI_CONFIG_USER_ROI_XY_SIZE);
        return new int[] {(temp & 0x0F) + 1, ((temp & 0xF0) >> 4) + 1};
    }

    @Override
    public void setRoiCenter(int centerSpad) {
        p.writeByte(ROI_CONFIG_USER_ROI_CENTRE_SPAD, centerSpad);
    }

    @Override
    public int getRoiCenter() {
        return p.readByte(ROI_CONFIG_USER_ROI_CENTRE_SPAD);
    }

    /** Recommended after a >8 °C ambient change. */
    @Override
    public void startTemperatureUpdate() {
        p.writeByte(VHV_CONFIG_TIMEOUT_MACROP_LOOP_BOUND, 0x81);
        p.writeByte(0x000B, 0x92);
        startRanging();
        waitDataReady(1000);
        clearInterrupt();
        stopRanging();
        p.writeByte(VHV_CONFIG_TIMEOUT_MACROP_LOOP_BOUND, 0x09);
        p.writeByte(0x000B, 0x00);
    }

    // The ranging loop every calibration shares: data ready, result, clear, n times.
    protected void collect(int nbSamples, BiConsumer<Integer, Vl53lxRange> onSample) {
        startRanging();
        for (int i = 0; i < nbSamples; i++) {
            waitDataReady(5000);
            Vl53lxRange r = getResult();
            clearInterrupt();
            onSample.accept(i, r);
        }
        stopRanging();
    }

    /** C's round(): half away from zero. */
    protected static long roundHalfAway(double v) {
        return (long) (v < 0 ? -Math.floor(-v + 0.5) : Math.floor(v + 0.5));
    }
}
