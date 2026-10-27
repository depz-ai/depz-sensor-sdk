package ai.depz.sensor.test.devices;

import static ai.depz.sensor.test.devices.T.check;
import static ai.depz.sensor.test.devices.T.eq;
import static ai.depz.sensor.test.devices.T.throwsType;

import ai.depz.sensor.sensors.vl53l4.RangeTiming;
import ai.depz.sensor.sensors.vl53lx.Vl53l1Die;
import ai.depz.sensor.sensors.vl53lx.Vl53l1UldDriver;
import ai.depz.sensor.sensors.vl53lx.Vl53l4UldDriver;
import ai.depz.sensor.sensors.vl53lx.Vl53lxDriver;
import ai.depz.sensor.sensors.vl53lx.Vl53lxDrivers;
import ai.depz.sensor.sensors.vl53lx.Vl53lxException;
import ai.depz.sensor.sensors.vl53lx.Vl53lxPlatform;
import ai.depz.sensor.sensors.vl53lx.Vl53lxRange;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;

/**
 * The 1D-family driver contract ({@link Vl53lxDriver}: what a driver gets for free and what an absent
 * optional group throws) and the VL53L4CD ULD's calibration arithmetic over a scripted register map
 * (the Python {@code test_vl53l4cd_uld.py} calibration cases).
 */
public final class Vl53lxDriverTests {
    /** A big-endian register map with a scripted result block; counts the bus traffic. */
    static final class RegMap extends Vl53lxPlatform {
        final byte[] regs = new byte[0x10000];
        final List<int[]> reads = new ArrayList<>();
        final List<int[]> writes = new ArrayList<>();
        final List<Integer> speeds = new ArrayList<>();
        int resets;
        int slept;

        RegMap() {
            regs[0x00E5] = 0x03;
            setWord(0x0006, FakeVl53lxBridge.DIE_OSC);
            setWord(0x010F, FakeVl53lxBridge.DIE_MODEL_ID);
            setWord(0x00DE, FakeVl53lxBridge.DIE_PLL);
        }

        void setWord(int addr, int v) {
            regs[addr] = (byte) (v >> 8);
            regs[addr + 1] = (byte) v;
        }

        int word(int addr) {
            return (regs[addr] & 0xFF) << 8 | (regs[addr + 1] & 0xFF);
        }

        void setResultBlock(byte[] raw) {
            System.arraycopy(raw, 0, regs, 0x0089, raw.length);
        }

        @Override
        public byte[] readMulti(int addr, int size) {
            reads.add(new int[] {addr, size});
            return java.util.Arrays.copyOfRange(regs, addr, addr + size);
        }

        @Override
        public void writeMulti(int addr, byte[] data) {
            writes.add(new int[] {addr, data.length});
            System.arraycopy(data, 0, regs, addr, data.length);
        }

        @Override
        public void setI2cSpeed(int khz) {
            speeds.add(khz);
        }

        @Override
        public void setAddrWidth(int width) {}

        @Override
        public void xshutReset() {
            resets++;
        }

        @Override
        public void sleepMs(int ms) {
            slept += ms;
        }
    }

    /** The least a driver can be: every optional group left to the base class. */
    static final class Bare extends Vl53lxDriver {
        boolean ready;
        int reads;

        Bare(Vl53lxPlatform p) {
            super(p, "VL53L9ZZ");
        }

        @Override
        public int modelId() {
            return 0x1234;
        }

        @Override
        public void sensorInit() {}

        @Override
        public void startRanging() {}

        @Override
        public void stopRanging() {}

        @Override
        public boolean checkForDataReady() {
            return ready;
        }

        @Override
        public void clearInterrupt() {}

        @Override
        public int[] streamBlock() {
            return new int[] {0x0089, 17};
        }

        @Override
        public Vl53lxRange decode(byte[] raw) {
            reads++;
            return new Vl53lxRange(raw.length, 0, "valid", 0, 0, 0, 0, List.of(), Map.of());
        }

        @Override
        public void setRangeTiming(int timingBudgetMs, int interMeasurementMs) {}

        @Override
        public RangeTiming getRangeTiming() {
            return new RangeTiming(50, 0);
        }
    }

    public static void theBaseClassDefaultsAreTheDieUldsWithoutOptions() {
        RegMap p = new RegMap();
        Bare d = new Bare(p);
        eq("VL53L9ZZ", d.product());
        eq(2, d.addrWidth());
        eq(0, d.clearSteps().length);
        eq(400, d.maxKhz());
        eq(List.of(), List.copyOf(d.supports()));
        eq(List.of(), d.modes());
        eq(new int[] {10, 200}, d.budgetMs());
        check(!d.histogram(), "no histogram");
        eq(List.of(), d.budgetChoices());
        eq(null, d.reachMm());
        eq(Map.of(), d.driverInfo());
        throwsType(IllegalArgumentException.class, () -> new Bare(null));
    }

    public static void everyAbsentGroupThrowsNamingTheProductAndTheGroup() {
        Bare d = new Bare(new RegMap());
        Object[][] calls = {
            {"mode", (T.Body) () -> d.setMode("long")}, {"mode", (T.Body) d::getMode},
            {"offset", (T.Body) d::getOffset}, {"offset", (T.Body) () -> d.setOffset(1)},
            {"xtalk", (T.Body) d::getXtalk}, {"xtalk", (T.Body) () -> d.setXtalk(1)},
            {"calib_offset", (T.Body) () -> d.calibrateOffset(100, null)},
            {"calib_xtalk", (T.Body) () -> d.calibrateXtalk(100, null)},
            {"thresholds", (T.Body) d::getDetectionThresholds}, {"thresholds", (T.Body) () -> d.setDetectionThresholds(1, 2, 3)},
            {"signal_thresh", (T.Body) d::getSignalThreshold}, {"signal_thresh", (T.Body) () -> d.setSignalThreshold(1)},
            {"sigma_thresh", (T.Body) d::getSigmaThreshold}, {"sigma_thresh", (T.Body) () -> d.setSigmaThreshold(1)},
            {"roi", (T.Body) d::getRoi}, {"roi", (T.Body) () -> d.setRoi(4, 4)},
            {"roi", (T.Body) d::getRoiCenter}, {"roi", (T.Body) () -> d.setRoiCenter(199)},
            {"temp_update", (T.Body) d::startTemperatureUpdate}, {"refspad", (T.Body) d::performRefSpadManagement},
        };
        for (Object[] c : calls) {
            UnsupportedOperationException e = throwsType(UnsupportedOperationException.class, (T.Body) c[1]);
            eq("VL53L9ZZ/Bare does not support '" + c[0] + "'", e.getMessage());
        }
        for (T.Body h : List.<T.Body>of(() -> d.binData(null), () -> d.toMeasurement(null))) {
            eq("Bare has no histogram", throwsType(UnsupportedOperationException.class, h).getMessage());
        }
    }

    public static void waitDataReadyPollsThenTimesOutAndReadMeasurementReadsTheStreamBlock() {
        RegMap p = new RegMap();
        Bare d = new Bare(p);
        Vl53lxException e = throwsType(Vl53lxException.class, () -> d.waitDataReady(0));
        check(e.getMessage().contains("timeout waiting for data ready"), e.getMessage());
        d.ready = true;
        d.waitDataReady(0);
        eq(17, d.readMeasurement().distanceMm());
        eq(List.of(0x0089, 17), List.of(p.reads.get(p.reads.size() - 1)[0], p.reads.get(p.reads.size() - 1)[1]));
        eq(1, d.reads);
    }

    public static void theRegistryBindsAProductToItsDriverAndCaveat() {
        RegMap p = new RegMap();
        Vl53lxDrivers.Created c = Vl53lxDrivers.createDriver("VL53L4CD", "uld", p);
        check(c.driver() instanceof Vl53l4UldDriver, c.driver().getClass().getSimpleName());
        eq("VL53L4CD", c.driver().product());
        eq(null, c.caveat());
        check(Vl53lxDrivers.createDriver("VL53L3CX", "ulp", p).caveat().startsWith("single-target"), "ulp caveat");
        check(Vl53lxDrivers.createDriver("VL53L4CX", "histogram", p).caveat().startsWith("the histogram driver"), "histogram caveat");
        eq(List.of("uld", "histogram"), Vl53lxDrivers.driverKinds("VL53L1CB"));
        UnsupportedOperationException e = throwsType(UnsupportedOperationException.class,
                () -> Vl53lxDrivers.createDriver("VL53L0X", "histogram", p));
        check(e.getMessage().contains("VL53L0X has no 'histogram' driver - it has uld"), e.getMessage());
        e = throwsType(UnsupportedOperationException.class, () -> Vl53lxDrivers.driverKinds("VL53L9ZZ"));
        check(e.getMessage().contains("no such product: VL53L9ZZ"), e.getMessage());
    }

    // ── the VL53L4CD ULD over the register map ───────────────────────────────

    static Vl53l4UldDriver l4(RegMap p) {
        Vl53l4UldDriver d = new Vl53l4UldDriver(p, "VL53L4CD");
        d.sensorInit();
        p.writes.clear();
        return d;
    }

    public static void l4InitRunsAtBootSpeedThenTheDiesCeilingAndEndsIn50msContinuous() {
        RegMap p = new RegMap();
        Vl53l4UldDriver d = new Vl53l4UldDriver(p, "VL53L4CD");
        eq(List.of("timing", "offset", "xtalk", "thresholds", "signal_thresh", "sigma_thresh", "temp_update",
                "calib_offset", "calib_xtalk"), List.copyOf(d.supports()));
        d.sensorInit();
        eq(List.of(400, 1000), p.speeds);
        eq(0x12, p.regs[0x002D] & 0xFF); // Fast Mode Plus byte on the blob
        eq(0x0500, p.word(0x0024));      // ALGO__RANGE_IGNORE_THRESHOLD_MCPS, the one write SensorInit adds
        RangeTiming t = d.getRangeTiming();
        eq(0, t.interMeasurementMs());
        check(Math.abs(t.timingBudgetMs() - 50) <= 1, "budget " + t.timingBudgetMs());
        eq(0xEBAA, d.modelId());
        eq(1388, d.reachMm());
    }

    public static void l4OffsetCalibrationIsTargetMinusTheMeanTimesFour() {
        RegMap p = new RegMap();
        Vl53l4UldDriver d = l4(p);
        p.setResultBlock(Vl53lxDeviceTests.L4_BLOCK); // 624 mm
        eq(-24, d.calibrateOffset(600, null)); // the default of 20 samples
        eq(-96 & 0xFFFF, p.word(0x001E));
        eq(-24, d.getOffset());
        eq(30, (int) p.writes.stream().filter(w -> w[0] == 0x0086).count()); // 10 heat frames + 20 samples released
        eq(76, d.calibrateOffset(700, 5));
    }

    public static void l4XtalkCalibrationDropsTheFirstAndInvalidFramesThenAverages() {
        RegMap p = new RegMap();
        Vl53l4UldDriver d = l4(p);
        p.setResultBlock(Vl53lxDeviceTests.L4_BLOCK); // 624 mm, 5160 kcps, 211 SPADs
        eq(1, d.calibrateXtalk(650, null));
        eq(500, p.word(0x0016)); // (1 - 624/650) × 5160/211 = 0.978 kcps × 512
        eq(1, d.getXtalk());
        byte[] bad = Vl53lxDeviceTests.L4_BLOCK.clone();
        bad[0] = 0x04; // status 2: no valid sample
        p.setResultBlock(bad);
        check(throwsType(Vl53lxException.class, () -> d.calibrateXtalk(650, 5)).getMessage().contains("no valid samples"), "no valid");
        byte[] hot = Vl53lxDeviceTests.L4_BLOCK.clone();
        hot[3] = 0x01;
        hot[4] = 0x00;
        hot[5] = (byte) 0xFF;
        hot[6] = (byte) 0xFF; // 1 SPAD, 524280 kcps
        p.setResultBlock(hot);
        check(throwsType(Vl53lxException.class, () -> d.calibrateXtalk(5000, 5)).getMessage().contains("> 127"), "> 127");
    }

    public static void l4CalibrationArgumentsAreCheckedBeforeTheBus() {
        RegMap p = new RegMap();
        Vl53l4UldDriver d = l4(p);
        int[][] offset = {{600, 4}, {600, 256}, {9, 20}, {1001, 20}};
        for (int[] a : offset) {
            check(throwsType(Vl53lxException.class, () -> d.calibrateOffset(a[0], a[1])).getMessage().contains("5..255, target 10..1000"), "offset " + a[0] + "/" + a[1]);
        }
        int[][] xtalk = {{600, 4}, {600, 256}, {9, 20}, {5001, 20}};
        for (int[] a : xtalk) {
            check(throwsType(Vl53lxException.class, () -> d.calibrateXtalk(a[0], a[1])).getMessage().contains("5..255, target 10..5000"), "xtalk " + a[0] + "/" + a[1]);
        }
        eq(0, p.writes.size());
    }

    // ── the VL53L1X ULD over the register map ────────────────────────────────

    static Vl53l1UldDriver l1(RegMap p) {
        Vl53l1UldDriver d = new Vl53l1UldDriver(p, "VL53L1CX");
        d.sensorInit();
        p.writes.clear();
        return d;
    }

    public static void l1ABudgetTheNewModeLacksSnapsToItsNearestOne() {
        RegMap p = new RegMap();
        Vl53l1UldDriver d = l1(p);
        eq("long", d.getMode());
        d.setMode("short");
        d.setRangeTiming(15, 0); // 15 ms exists in short mode only
        eq(new RangeTiming(15, 15), d.getRangeTiming());
        eq(List.of(15, 20, 33, 50, 100, 200, 500), d.budgetChoices());
        // 15 is not a long-mode budget: the nearest, 20, is applied and returned (contract 12 §5)
        eq(20, d.setMode("long"));
        eq("long", d.getMode());
        eq(20, d.getRangeTiming().timingBudgetMs());
        eq(List.of(20, 33, 50, 100, 200, 500), d.budgetChoices());
        throwsType(IllegalArgumentException.class, () -> d.setMode("medium"));
        Vl53lxException e = throwsType(Vl53lxException.class, () -> d.setRangeTiming(40, 0));
        check(e.getMessage().contains("pick one of 20, 33, 50, 100, 200, 500"), e.getMessage());
        p.regs[0x004B] = 0x00; // PHASECAL_CONFIG__TIMEOUT_MACROP neither 0x14 nor 0x0A
        e = throwsType(Vl53lxException.class, d::getMode);
        check(e.getMessage().contains("neither distance mode"), e.getMessage());
    }

    public static void l1ThresholdsKeepTheBitsOutside0x6FAndCanInterruptOnNoTarget() {
        RegMap p = new RegMap();
        Vl53l1UldDriver d = l1(p);
        p.regs[0x0046] = (byte) 0x90; // bits the ULD must keep
        d.setDetectionThresholds(100, 300, Vl53l1Die.WINDOW_IN);
        eq(0x93, p.regs[0x0046] & 0xFF);
        eq(new Vl53lxDriver.Thresholds(100, 300, Vl53l1Die.WINDOW_IN), d.getDetectionThresholds());
        d.setDetectionThresholds(50, 60, Vl53l1Die.WINDOW_OUT, true);
        eq(0xD2, p.regs[0x0046] & 0xFF);
        eq(List.of(60, 50), List.of(p.word(0x0072), p.word(0x0074)));
    }

    public static void l1CalibrationsUseTheL1Formulas() {
        RegMap p = new RegMap();
        Vl53l1UldDriver d = l1(p);
        p.setResultBlock(Vl53lxDeviceTests.L1_BLOCK); // 576 mm, 16072 kcps, 67 SPADs
        eq(24, d.calibrateOffset(600, 5));
        eq(96, p.word(0x001E));
        eq(24, d.getOffset());
        eq(5, (int) p.writes.stream().filter(w -> w[0] == 0x0086).count()); // no heat loop on the L1X ULD
        check(throwsType(Vl53lxException.class, () -> d.calibrateOffset(600, 4)).getMessage().contains("at least 5"), "offset n");
        // 512 × 16072 × (1 − 576/600) / 67 = 4912.75, truncated → 10 kcps
        eq(10, d.calibrateXtalk(600, 5));
        eq(4912, p.word(0x0016));
        eq(10, d.getXtalk());
        eq(0, d.calibrateXtalk(500, 5)); // target nearer than the mean: clamped at 0
        check(throwsType(Vl53lxException.class, () -> d.calibrateXtalk(600, 4)).getMessage().contains("at least 5"), "xtalk n");
        byte[] none = Vl53lxDeviceTests.L1_BLOCK.clone();
        none[3] = 0;
        none[4] = 0; // 0 SPADs
        p.setResultBlock(none);
        check(throwsType(Vl53lxException.class, () -> d.calibrateXtalk(600, 5)).getMessage().contains("no SPADs enabled"), "no spads");
    }

    public static void l4ResetFallsBackToXshutWhenTheSoftResetIsNotAcknowledged() {
        RegMap p = new RegMap();
        Vl53l4UldDriver d = new Vl53l4UldDriver(p, "VL53L4CD");
        p.regs[0x00E5] = 0x00; // never boots
        p.sleepMs(0);
        Vl53lxException e = throwsType(Vl53lxException.class, () -> d.waitBoot(0));
        check(e.getMessage().contains("FIRMWARE__SYSTEM_STATUS"), e.getMessage());
        eq(0, d.bootState());
        p.regs[0x00E5] = 0x03;
        d.resetDevice(10);
        eq(0, p.resets);
        eq(1, d.getInterruptPolarity()); // GPIO_HV_MUX_CTRL bit 4 clear: active high
        d.setInterruptPolarity(0);
        eq(0, d.getInterruptPolarity());
        eq(0x10, p.regs[0x0030] & 0xFF);
    }
}
