package ai.depz.sensor.sensors.vl53lx;

import ai.depz.sensor.sensors.vl53l4.RangeTiming;
import ai.depz.sensor.sensors.vl53lx.bare.BareDriver;
import ai.depz.sensor.sensors.vl53lx.bare.BareRegs;
import ai.depz.sensor.sensors.vl53lx.bare.FrameHistory;
import ai.depz.sensor.sensors.vl53lx.bare.Hist;
import ai.depz.sensor.sensors.vl53lx.bare.HistogramBinData;
import ai.depz.sensor.sensors.vl53lx.bare.RangeResults;
import java.util.Collection;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;

/**
 * ST's VL53LX BareDriver on the host — the histogram driver of VL53L1CX / L1CB / L3CX / L4CD / L4CX:
 * the die hands over 24 raw photon-count bins per frame and {@link Hist} turns them into up to four
 * targets. What it costs against the light drivers: no calibrations and no detection thresholds, and
 * the first frame of a stream reports status 6 (no wrap check done) because the check needs the frame
 * before.
 */
public final class Vl53lxHistogramDriver extends Vl53lxDriver {
    private static final int GPIO_HV_MUX_CTRL = 0x0030;
    private static final int GPIO_TIO_HV_STATUS = 0x0031;
    private static final int SYSTEM_INTERRUPT_CLEAR = 0x0086;
    private static final int FIRMWARE_SYSTEM_STATUS = 0x00E5;
    private static final int PAD_I2C_HV_CONFIG = 0x002D;
    private static final int I2C_KHZ_BOOT = 400;
    private static final int MAX_KHZ = 1000; // with Fast Mode Plus on the sensor pad

    // Bits 2 and 5, the byte VL53L4CD_I2C_FAST_MODE_PLUS writes here in the C ULD. ST's histogram preset leaves
    // the pad at 0x00; the pad is a property of the die, so it is set here and never cleared.
    private static final int FMP_PAD_CONFIG = 0x12;

    private static final String DEFAULT_MODE = "medium";

    private static final List<String> GROUPS = List.of("mode", "timing");

    private final BareDriver bare;
    private String mode = DEFAULT_MODE;
    private int budgetMs = 33;
    private int interMs;
    private List<String> modes = BareRegs.DISTANCE_MODES;
    private int[] budgetRange = {2, 550};

    // The A/B alternation rides on the ll-driver state, which the polled C loop advances in the call that clears
    // the interrupt. Here the bridge clears it, so the state is advanced when a new frame arrives — told by the
    // device's own result__stream_count, so decoding the same sample twice does not step it twice.
    private Integer lastStreamCount;

    // The phase and event consistency checks read the frame before this one.
    private final FrameHistory history = new FrameHistory();

    public Vl53lxHistogramDriver(Vl53lxPlatform platform, String product) {
        super(platform, product);
        bare = new BareDriver(platform);
    }

    /** The BareDriver underneath: device image, factory data, preset and post-processing config. */
    public BareDriver bare() {
        return bare;
    }

    @Override
    public int addrWidth() {
        return 2;
    }

    /** One write clears the interrupt and lets the next frame run (measured over 40 frames on both dies). */
    @Override
    public int[][] clearSteps() {
        return new int[][] {{SYSTEM_INTERRUPT_CLEAR, BareRegs.CLEAR_RANGE_INT}};
    }

    @Override
    public int maxKhz() {
        return MAX_KHZ;
    }

    @Override
    public Collection<String> supports() {
        return GROUPS;
    }

    /** short / medium / long, narrowed per die by {@link #sensorInit} (an L4 has no short). */
    @Override
    public List<String> modes() {
        return modes;
    }

    /**
     * The guard comes off the budget before it is divided and the range timeout is capped: 2..550 ms,
     * 2..200 on an L4 die.
     */
    @Override
    public int[] budgetMs() {
        return budgetRange.clone();
    }

    @Override
    public boolean histogram() {
        return true;
    }

    @Override
    public int modelId() {
        return p.readWord(0x010F);
    }

    public void waitBoot() {
        waitBoot(1000);
    }

    public void waitBoot(long timeoutMs) {
        long deadline = System.nanoTime() + timeoutMs * 1_000_000L;
        while (p.readByte(FIRMWARE_SYSTEM_STATUS) != 0x03) {
            if (System.nanoTime() > deadline) {
                throw new Vl53lxException("timeout waiting for FIRMWARE__SYSTEM_STATUS == 0x03");
            }
            p.sleepMs(1);
        }
    }

    /**
     * Reset the part (whatever ran before left it ranging under another configuration), configure it
     * for histogram ranging and raise the bus only after the FM+ pad byte has reached the device.
     */
    @Override
    public void sensorInit() {
        p.setAddrWidth(addrWidth());
        p.setI2cSpeed(I2C_KHZ_BOOT);
        p.xshutReset();
        waitBoot();

        bare.dataInit();
        // The die is known once NVM is read: an L4 has no short mode and a 200 ms budget ceiling.
        modes = bare.distanceModesForDie();
        budgetRange = new int[] {2, (int) (bare.fdaMaxTimingBudgetUs() / 1000)};
        setMode(mode);
        p.writeByte(PAD_I2C_HV_CONFIG, FMP_PAD_CONFIG);
        if (maxKhz() != I2C_KHZ_BOOT) {
            p.setI2cSpeed(maxKhz());
        }
    }

    /**
     * The device preset mode: the VCSEL period, which sets where the phase wraps (1.6 / 2.4 / 4.0 m),
     * and the valid phase window inside it ({@link #reachMm}). The bin is 199 mm wide in all three.
     */
    @Override
    public Integer setMode(String name) {
        if (!modes.contains(name)) {
            throw new IllegalArgumentException("no such mode: " + name + " (have " + String.join(", ", modes) + ")");
        }
        mode = name;
        bare.setDistanceMode(name);
        // The preset rewrites the static config, pad byte included, and start_ranging() puts that image on the
        // wire: put FM+ back or that write would switch the pad off under a 1 MHz bus.
        bare.img().staticConfig().set("pad_i2c_hv__config", FMP_PAD_CONFIG);
        setRangeTiming(budgetMs, interMs);
        return null;
    }

    @Override
    public String getMode() {
        return mode;
    }

    /**
     * The valid phase window the post-processing applies: 992 / 1785 / 3373 mm for short / medium /
     * long at the usual oscillator. Past it the target is status 4.
     */
    @Override
    public Integer reachMm() {
        return (int) Vl53l1Die.phaseWindowMm(bare.img().staticNvm().get("osc_measured__fast_osc__frequency"),
                bare.hpp().validPhaseHigh);
    }

    /** How long one range actually runs, and the MM1/MM2 offset the post-processing applies. */
    @Override
    public Map<String, Object> driverInfo() {
        Map<String, Object> m = new LinkedHashMap<>();
        m.put("range timeout us", bare.rangeConfigTimeoutUs());
        m.put("MM1/MM2 offset mm", bare.hpp().rangeOffsetMm / 4.0);
        return m;
    }

    @Override
    public void startRanging() {
        lastStreamCount = null;
        history.reset();
        bare.loadPatch();
        bare.initAndStartRange();
        // The interrupt line comes out of reset asserted, so the first data-ready would read the untouched
        // register file as a frame; one clear deasserts it (ST's ClearInterruptAndStartMeasurement).
        bare.clearInterruptAndStartNextRange();
    }

    @Override
    public void stopRanging() {
        bare.stopRange();
        bare.unloadPatch();
    }

    @Override
    public int[] streamBlock() {
        return new int[] {BareRegs.HISTOGRAM_BIN_DATA_I2C_INDEX, BareRegs.HISTOGRAM_BIN_DATA_I2C_SIZE_BYTES};
    }

    @Override
    public Vl53lxRange decode(byte[] raw) {
        return toMeasurement(binData(raw));
    }

    @Override
    public Vl53lxRange readMeasurement() {
        return toMeasurement(binData(null));
    }

    /** The raw frame as a {@link HistogramBinData}; read here when {@code raw} is null. */
    @Override
    public Object binData(byte[] raw) {
        if (raw == null) {
            int[] b = streamBlock();
            raw = p.readMulti(b[0], b[1]);
        }
        int streamCount = raw[3] & 0xFF; // result__stream_count
        if (lastStreamCount == null || streamCount != lastStreamCount) {
            // What clear_interrupt_and_start_next_range() does to the state machine, without the register writes:
            // one frame on, which flips the A/B timing status and steps the bin sequence.
            if (lastStreamCount != null) {
                bare.state().updateRd(bare.img().systemControl().get("system__mode_start"),
                        bare.img().dynamicConfig().get("system__grouped_parameter_hold"));
            }
            lastStreamCount = streamCount;
        }
        return bare.getHistogramBinData(raw);
    }

    /** A frame → its range. Once per frame: the history is a state machine and would step twice. */
    @Override
    public Vl53lxRange toMeasurement(Object bins) {
        HistogramBinData b = (HistogramBinData) bins;
        RangeResults results = Hist.processData(b, bare.hpp());
        history.apply(results, b, bare.state().rdDeviceState, bare.hpp());
        return Hist.toRange(results, b);
    }

    @Override
    public boolean checkForDataReady() {
        int intPol = ((p.readByte(GPIO_HV_MUX_CTRL) & 0x10) >> 4) == 1 ? 0 : 1;
        return (p.readByte(GPIO_TIO_HV_STATUS) & 1) == intPol;
    }

    @Override
    public void clearInterrupt() {
        p.writeByte(SYSTEM_INTERRUPT_CLEAR, BareRegs.CLEAR_RANGE_INT);
    }

    /** The budget covers the whole measurement; the range timeout is one sixth of it after a fixed guard. */
    @Override
    public void setRangeTiming(int timingBudgetMs, int interMeasurementMs) {
        int low = budgetRange[0];
        int high = budgetRange[1];
        if (timingBudgetMs < low || timingBudgetMs > high) {
            throw new Vl53lxException("timing_budget_ms must be " + low + ".." + high);
        }
        budgetMs = timingBudgetMs;
        interMs = interMeasurementMs;
        bare.setMeasurementTimingBudgetUs(timingBudgetMs * 1000L);
        // 0 means continuous; the device wants a period, and back-to-back is a period of zero.
        bare.setInterMeasurementPeriodMs(interMeasurementMs);
    }

    @Override
    public RangeTiming getRangeTiming() {
        long budgetUs = bare.rangeConfigTimeoutUs() * BareRegs.TIMING_DIVISOR + BareRegs.TIMING_GUARD_US;
        return new RangeTiming((int) (budgetUs / 1000), (int) bare.interMeasurementPeriodMs());
    }
}
