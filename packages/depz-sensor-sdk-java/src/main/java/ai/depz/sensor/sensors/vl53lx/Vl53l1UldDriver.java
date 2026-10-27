package ai.depz.sensor.sensors.vl53lx;

import ai.depz.sensor.sensors.vl53l4.RangeTiming;
import java.util.ArrayList;
import java.util.Collection;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.TreeMap;

/**
 * VL53L1X ULD 3.5.5 ({@code VL53L1X_api.c} + {@code VL53L1X_calibration.c}) for VL53L1CX and VL53L1CB: a
 * tabulated timing budget, short / long distance modes, an init that resets the die first. Crosstalk is
 * in kcps, the register's own unit (ST's wrapper converts to cps).
 */
public final class Vl53l1UldDriver extends Vl53l1Die {
    private static final int DISTANCE_SHORT = 1;
    private static final int DISTANCE_LONG = 2;

    private static final byte[] DEFAULT_CONFIGURATION = Vl53l4UldDriver.bytes(
        0x00, 0x00, 0x00, 0x01, 0x02, 0x00, 0x02, 0x08, 0x00, 0x08, 0x10, 0x01, 0x01, 0x00, 0x00, 0x00,
        0x00, 0xff, 0x00, 0x0F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x0b, 0x00, 0x00, 0x02, 0x0a, 0x21,
        0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x00, 0xc8, 0x00, 0x00, 0x38, 0xff, 0x01, 0x00, 0x08, 0x00,
        0x00, 0x01, 0xcc, 0x0f, 0x01, 0xf1, 0x0d, 0x01, 0x68, 0x00, 0x80, 0x08, 0xb8, 0x00, 0x00, 0x00,
        0x00, 0x0f, 0x89, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x0f, 0x0d, 0x0e, 0x0e, 0x00,
        0x00, 0x02, 0xc7, 0xff, 0x9B, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00);

    // VL53L1X_SetTimingBudgetInMs(): {mode: {budget ms: (MACROP_A_HI, MACROP_B_HI)}}; 15 ms in short only.
    private static final Map<Integer, TreeMap<Integer, int[]>> TIMING_BUDGETS = new LinkedHashMap<>();

    static {
        TreeMap<Integer, int[]> s = new TreeMap<>();
        s.put(15, new int[] {0x001D, 0x0027});
        s.put(20, new int[] {0x0051, 0x006E});
        s.put(33, new int[] {0x00D6, 0x006E});
        s.put(50, new int[] {0x01AE, 0x01E8});
        s.put(100, new int[] {0x02E1, 0x0388});
        s.put(200, new int[] {0x03E1, 0x0496});
        s.put(500, new int[] {0x0591, 0x05C1});
        TreeMap<Integer, int[]> l = new TreeMap<>();
        l.put(20, new int[] {0x001E, 0x0022});
        l.put(33, new int[] {0x0060, 0x006E});
        l.put(50, new int[] {0x00AD, 0x00C6});
        l.put(100, new int[] {0x01CC, 0x01EA});
        l.put(200, new int[] {0x02D9, 0x02F8});
        l.put(500, new int[] {0x048F, 0x04A4});
        TIMING_BUDGETS.put(DISTANCE_SHORT, s);
        TIMING_BUDGETS.put(DISTANCE_LONG, l);
    }

    private static final List<String> MODE_NAMES = List.of("long", "short"); // the blob boots in long

    private static final List<String> GROUPS = List.of(
        "mode", "timing", "offset", "xtalk", "thresholds", "signal_thresh", "sigma_thresh", "temp_update",
        "calib_offset", "calib_xtalk", "roi");

    // The mode last applied: budgetChoices answers from it (PHASECAL reads 0 before init).
    private String appliedMode = "long";

    public Vl53l1UldDriver(Vl53lxPlatform platform, String product) {
        super(platform, product);
    }

    @Override
    protected byte[] configuration() {
        return DEFAULT_CONFIGURATION;
    }

    @Override
    protected int stopMode() {
        return 0x00;
    }

    @Override
    protected int signalAt() {
        return 15;
    }

    @Override
    protected int perSpadK() {
        return 25;
    }

    @Override
    public Collection<String> supports() {
        return GROUPS;
    }

    @Override
    public List<String> modes() {
        return MODE_NAMES;
    }

    @Override
    public int[] budgetMs() {
        return new int[] {15, 500};
    }

    private static int code(String name) {
        if ("short".equals(name)) {
            return DISTANCE_SHORT;
        }
        if ("long".equals(name)) {
            return DISTANCE_LONG;
        }
        return 0;
    }

    // No plain wait: a sensor left hung by a previous session answers reads with zeros.
    @Override
    protected void initBoot() {
        resetDevice();
    }

    // ST ships this blob with an active-HIGH interrupt; the bridge watches the falling edge.
    @Override
    protected void initAfterConfig() {
        setInterruptPolarity(0);
    }

    @Override
    protected void initExtra() {
        appliedMode = MODE_NAMES.get(0);
    }

    /** Timed mode only: this part has no free-running mode under the ULD configuration. */
    @Override
    public void startRanging() {
        p.writeByte(SYSTEM_MODE_START, 0x40);
    }

    /**
     * "short" or "long"; the budget is re-applied (the VCSEL periods change what it means). Returns the budget in
     * effect after the change, ms: one the new mode does not tabulate moves to the nearest one it does (contract 12
     * §5, the one exception to "refuse, don't round"), so the caller sees it.
     */
    @Override
    public Integer setMode(String name) {
        int mode = code(name);
        if (mode == 0) {
            throw new IllegalArgumentException("no such mode: " + name + " (have " + String.join(", ", MODE_NAMES) + ")");
        }
        RangeTiming t = getRangeTiming();
        int budgetMs = t.timingBudgetMs();
        appliedMode = name;
        if (mode == DISTANCE_SHORT) {
            p.writeByte(PHASECAL_CONFIG_TIMEOUT_MACROP, 0x14);
            p.writeByte(RANGE_CONFIG_VCSEL_PERIOD_A, 0x07);
            p.writeByte(RANGE_CONFIG_VCSEL_PERIOD_B, 0x05);
            p.writeByte(RANGE_CONFIG_VALID_PHASE_HIGH, 0x38);
            p.writeWord(SD_CONFIG_WOI_SD0, 0x0705);
            p.writeWord(SD_CONFIG_INITIAL_PHASE_SD0, 0x0606);
        } else {
            p.writeByte(PHASECAL_CONFIG_TIMEOUT_MACROP, 0x0A);
            p.writeByte(RANGE_CONFIG_VCSEL_PERIOD_A, 0x0F);
            p.writeByte(RANGE_CONFIG_VCSEL_PERIOD_B, 0x0D);
            p.writeByte(RANGE_CONFIG_VALID_PHASE_HIGH, 0xB8);
            p.writeWord(SD_CONFIG_WOI_SD0, 0x0F0D);
            p.writeWord(SD_CONFIG_INITIAL_PHASE_SD0, 0x0E0E);
        }
        TreeMap<Integer, int[]> table = TIMING_BUDGETS.get(mode);
        if (!table.containsKey(budgetMs)) { // 15 ms exists in short mode only
            int best = table.firstKey();
            for (int ms : table.keySet()) {
                if (Math.abs(ms - budgetMs) < Math.abs(best - budgetMs)) {
                    best = ms;
                }
            }
            budgetMs = best;
        }
        setRangeTiming(budgetMs, t.interMeasurementMs());
        return budgetMs;
    }

    @Override
    public String getMode() {
        return modeCode() == DISTANCE_SHORT ? "short" : "long";
    }

    private int modeCode() {
        int temp = p.readByte(PHASECAL_CONFIG_TIMEOUT_MACROP);
        if (temp == 0x14) {
            return DISTANCE_SHORT;
        }
        if (temp == 0x0A) {
            return DISTANCE_LONG;
        }
        throw new Vl53lxException(String.format(
                "PHASECAL_CONFIG__TIMEOUT_MACROP reads 0x%02X, which is neither distance mode", temp));
    }

    /**
     * Only the tabulated budgets of the mode exist; a period of 0 is written as the budget itself (this
     * part has no free-running mode).
     */
    @Override
    public void setRangeTiming(int timingBudgetMs, int interMeasurementMs) {
        int mode = modeCode();
        TreeMap<Integer, int[]> table = TIMING_BUDGETS.get(mode);
        int[] macrop = table.get(timingBudgetMs);
        if (macrop == null) {
            throw new Vl53lxException("timing budget " + timingBudgetMs + " ms is not one the "
                    + (mode == DISTANCE_SHORT ? "short" : "long") + " mode has - pick one of "
                    + String.join(", ", table.keySet().stream().map(String::valueOf).toList()) + " ms");
        }
        p.writeWord(RANGE_CONFIG_TIMEOUT_MACROP_A_HI, macrop[0]);
        p.writeWord(RANGE_CONFIG_TIMEOUT_MACROP_B_HI, macrop[1]);
        if (interMeasurementMs == 0) {
            interMeasurementMs = timingBudgetMs;
        }
        int clockPll = p.readWord(RESULT_OSC_CALIBRATE_VAL) & 0x3FF;
        p.writeDword(INTERMEASUREMENT_MS, (long) ((double) (clockPll * interMeasurementMs) * 1.075));
    }

    @Override
    public List<Integer> budgetChoices() {
        return new ArrayList<>(TIMING_BUDGETS.get(code(appliedMode)).keySet());
    }

    @Override
    public RangeTiming getRangeTiming() {
        int macropA = p.readWord(RANGE_CONFIG_TIMEOUT_MACROP_A_HI);
        int budgetMs = 0;
        for (TreeMap<Integer, int[]> t : TIMING_BUDGETS.values()) {
            for (Map.Entry<Integer, int[]> kv : t.entrySet()) {
                if (kv.getValue()[0] == macropA) {
                    budgetMs = kv.getKey();
                }
            }
        }
        long tmp = p.readDword(INTERMEASUREMENT_MS);
        int clockPll = p.readWord(RESULT_OSC_CALIBRATE_VAL) & 0x3FF;
        int interMs = clockPll != 0 ? (int) (tmp / (clockPll * 1.065)) : 0;
        return new RangeTiming(budgetMs, interMs);
    }

    /** VL53L1X_SetDistanceThreshold(): a read-modify-write keeping the bits outside 0x6F. */
    @Override
    public void setDetectionThresholds(int distanceLowMm, int distanceHighMm, int window) {
        setDetectionThresholds(distanceLowMm, distanceHighMm, window, false);
    }

    public void setDetectionThresholds(int distanceLowMm, int distanceHighMm, int window, boolean intOnNoTarget) {
        int temp = p.readByte(SYSTEM_INTERRUPT_CONFIG_GPIO) & ~0x6F & 0xFF;
        temp |= window;
        if (intOnNoTarget) {
            temp |= 0x40;
        }
        p.writeByte(SYSTEM_INTERRUPT_CONFIG_GPIO, temp);
        p.writeWord(THRESH_HIGH, distanceHighMm);
        p.writeWord(THRESH_LOW, distanceLowMm);
    }

    @Override
    public int calibrateOffset(int targetDistMm, Integer nbSamples) {
        int n = nbSamples != null ? nbSamples : 50;
        if (n < 5) {
            throw new Vl53lxException("nb_samples must be at least 5");
        }
        p.writeWord(RANGE_OFFSET_MM, 0);
        p.writeWord(INNER_OFFSET_MM, 0);
        p.writeWord(OUTER_OFFSET_MM, 0);
        long[] sum = {0};
        collect(n, (i, r) -> sum[0] += r.distanceMm());
        int offsetMm = targetDistMm - (int) Math.floorDiv(sum[0], n);
        p.writeWord(RANGE_OFFSET_MM, (offsetMm * 4) & 0xFFFF);
        return offsetMm;
    }

    /** VL53L1X_CalibrateXtalk(); returns kcps, not the C driver's cps. */
    @Override
    public int calibrateXtalk(int targetDistMm, Integer nbSamples) {
        int n = nbSamples != null ? nbSamples : 50;
        if (n < 5) {
            throw new Vl53lxException("nb_samples must be at least 5");
        }
        p.writeWord(XTALK_PLANE_OFFSET_KCPS, 0);
        List<Vl53lxRange> samples = new ArrayList<>();
        collect(n, (i, r) -> samples.add(r));
        double count = samples.size();
        double avgDistance = samples.stream().mapToDouble(s -> s.distanceMm()).sum() / count;
        double avgSpadNb = samples.stream().mapToDouble(Vl53lxRange::spads).sum() / count;
        double avgSignal = samples.stream().mapToDouble(Vl53lxRange::signalKcps).sum() / count;
        if (avgSpadNb == 0) {
            throw new Vl53lxException("xtalk calibration failed: no SPADs enabled");
        }
        long cal = (long) (512 * (avgSignal * (1 - avgDistance / targetDistMm)) / avgSpadNb);
        cal = Math.min(Math.max(cal, 0), 0xFFFF);
        p.writeWord(XTALK_PLANE_OFFSET_KCPS, (int) cal);
        return (int) roundHalfAway(cal / 512.0);
    }
}
