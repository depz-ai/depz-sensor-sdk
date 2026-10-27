package ai.depz.sensor.test.devices;

import static ai.depz.sensor.test.devices.T.check;
import static ai.depz.sensor.test.devices.T.eq;
import static ai.depz.sensor.test.devices.T.throwsType;

import ai.depz.sensor.sensors.vl53l4.RangeTiming;
import ai.depz.sensor.sensors.vl53lx.Vl53l0xUldDriver;
import ai.depz.sensor.sensors.vl53lx.Vl53l0xUldDriver.RefSpads;
import ai.depz.sensor.sensors.vl53lx.Vl53lxException;
import ai.depz.sensor.sensors.vl53lx.Vl53lxRange;
import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.HexFormat;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.function.Consumer;

/**
 * The VL53L0X ULD port over a VL53L0X die model ({@link FakeVl53l0xDie}), no bridge, no hardware: the
 * Python tests' test_vl53l0x_uld.py where the Java driver exposes the path — the init state, every
 * ranging profile, the budget floor and timed mode, the polled shot's start bit / interrupt / range
 * error paths, boot and soft-reset timeouts, the NVM identification words, the reference SPADs from
 * NVM and from the management's convergence cases, the tuning buffer, the PAL range status mapping,
 * sigma / dmax / short blocks, the offset and crosstalk registers and calibrations. Expected values
 * are the Python reference's, worked out there from ST's vl53l0x_api*.c.
 */
public final class Vl53l0xUldTests {
    private Vl53l0xUldTests() {}

    private static final byte[] F635 = FakeVl53l0xDie.FRAME_635_MM;

    private record Rig(Vl53l0xUldDriver drv, FakeVl53l0xDie die) {}

    private static Rig rig(Consumer<FakeVl53l0xDie> setup) {
        FakeVl53l0xDie die = new FakeVl53l0xDie();
        setup.accept(die);
        return new Rig(new Vl53l0xUldDriver(die, "VL53L0X"), die);
    }

    /** An initialised VL53L0X; the die's logs start after init. */
    private static Rig ready(Consumer<FakeVl53l0xDie> setup) {
        Rig r = rig(setup);
        r.drv.sensorInit();
        r.die.writes.clear();
        r.die.reads.clear();
        return r;
    }

    private static Rig ready() {
        return ready(d -> {});
    }

    private static byte[] hex(String s) {
        return HexFormat.of().parseHex(s);
    }

    private static byte[] be16(int v) {
        return new byte[] {(byte) (v >> 8), (byte) v};
    }

    /** A 12-byte result block in the layout of FRAME_635_MM. */
    private static byte[] frame(int deviceStatus, int distance, int signal97, int ambient97, int spads88) {
        return ByteBuffer.allocate(12).put((byte) deviceStatus).put((byte) 0x06).putShort((short) spads88)
                .putShort((short) 0x0071).putShort((short) signal97).putShort((short) ambient97)
                .putShort((short) distance).array();
    }

    private static byte[] frame(int deviceStatus) {
        return frame(deviceStatus, 635, 0x022B, 0x011B, 0xC104);
    }

    private static List<String> writeKeys(FakeVl53l0xDie die) {
        List<String> out = new ArrayList<>();
        for (Object[] w : die.writes) {
            out.add(String.format("%02X:%s", (Integer) w[0], HexFormat.of().formatHex((byte[]) w[1])));
        }
        return out;
    }

    // The undocumented prologue of every StartMeasurement: the stop variable 0x3C goes back on page 1.
    private static final List<String> ARM_STOP_VARIABLE =
            List.of("80:01", "FF:01", "00:00", "91:3c", "00:01", "FF:00", "80:00");

    private static RefSpads spads(Object o) {
        return (RefSpads) o;
    }

    // ── init ─────────────────────────────────────────────────────────────────

    public static void initStateMatchesTheBoard() {
        Rig r = ready();
        check(r.drv.refSpadsFromNvm(), "reference SPADs from NVM");
        eq(new RefSpads(6, 0), r.drv.getReferenceSpads());
        // The 6 non-aperture SPADs through the good map FB..: indices 0,1,3,4,5,6 (index 2 is bad).
        eq(hex("7b0000000000"), r.die.spadEnables());
        // The VHV / phase shots of the reference calibration, in order.
        eq(2, r.die.shots.size());
        eq(new int[] {0x41, 0x01}, r.die.shots.get(0));
        eq(new int[] {0x01, 0x02}, r.die.shots.get(1));
        eq(0xE8, r.die.bank(0)[0x01] & 0xFF, "sequence config: TCC and MSRC off, as StaticInit leaves it");
        eq("default", r.drv.getMode());
        eq(0x975CL << 4, r.drv.oscFrequencyMHz());
        // StaticInit's budget read back with TCC off: 33823 us.
        eq(new RangeTiming(33, 0), r.drv.getRangeTiming());
        eq(1200, r.drv.reachMm());
        eq(List.of("default", "long-range", "high-speed", "high-accuracy"), r.drv.modes());
        eq(0xEE, r.drv.modelId());
    }

    // ── ranging profiles ─────────────────────────────────────────────────────

    private static Map<String, Object> info(double signal, double sigma, long budgetUs, int pre, int fin) {
        Map<String, Object> m = new LinkedHashMap<>();
        m.put("signal_mcps", signal);
        m.put("sigma_mm", sigma);
        m.put("budget_us", budgetUs);
        m.put("pre_pclks", pre);
        m.put("final_pclks", fin);
        return m;
    }

    public static void eachRangingProfileReadsBack() {
        Object[][] cases = {
            {"default", info(0.25, 18.0, 32908, 14, 10), 1200},
            {"long-range", info(0.09375, 60.0, 32983, 18, 14), 2000},
            {"high-speed", info(0.25, 32.0, 30010, 14, 10), 1200},
            {"high-accuracy", info(0.25, 18.0, 199769, 14, 10), 1200},
        };
        for (Object[] c : cases) {
            Rig r = ready();
            String mode = (String) c[0];
            r.drv.setMode(mode);
            eq(mode, r.drv.getMode());
            eq(c[1], r.drv.driverInfo(), mode);
            eq(c[2], r.drv.reachMm(), mode);
            @SuppressWarnings("unchecked")
            long budgetUs = (Long) ((Map<String, Object>) c[1]).get("budget_us");
            eq((int) (budgetUs / 1000), r.drv.getRangeTiming().timingBudgetMs(), mode);
        }
    }

    public static void longRangeWritesTheVcselRegistersOfTheCapture() {
        Rig r = ready();
        r.drv.setMode("long-range");
        byte[] b0 = r.die.bank(0);
        byte[] b1 = r.die.bank(1);
        // set_vcsel_pulse_period(): phase limits, period, re-encoded timeouts, the final-range width /
        // phasecal settings — the registers the capture shows being written.
        eq(new int[] {0x50, 0x08, 0x08}, new int[] {b0[0x57], b0[0x56], b0[0x50]});
        eq(hex("0074"), r.die.regs(0, 0x51, 2));
        eq(0x1D, (int) b0[0x46]);
        eq(new int[] {0x48, 0x08, 0x06, 0x03, 0x07, 0x20}, new int[] {b0[0x48], b0[0x47], b0[0x70], b0[0x32], b0[0x30], b1[0x30]});
        eq(hex("01ca"), r.die.regs(0, 0x71, 2));
        // Every VCSEL change ends in a phase calibration (sequence 0x02).
        int[] last = r.die.shots.get(r.die.shots.size() - 1);
        eq(new int[] {0x01, 0x02}, last);
    }

    public static void unknownModeIsRefused() {
        Rig r = ready();
        IllegalArgumentException e = throwsType(IllegalArgumentException.class, () -> r.drv.setMode("fast"));
        check(e.getMessage().startsWith("no such mode: fast"), e.getMessage());
        eq("default", r.drv.getMode());
    }

    // ── timing budget, timed mode ────────────────────────────────────────────

    public static void budgetBelow20msIsRefusedLikeTheCDriver() {
        Rig r = ready();
        Vl53lxException e = throwsType(Vl53lxException.class, () -> r.drv.setRangeTiming(19, 0));
        check(e.getMessage().contains("below the 20000 us minimum"), e.getMessage());
        eq(0, r.die.writes.size(), "refused before a register is touched");
        r.drv.setRangeTiming(20, 0);
        eq(19, r.drv.getRangeTiming().timingBudgetMs()); // quantised down, as read back
    }

    public static void timedModeStartsWithTheTimedStartCode() {
        Rig r = ready();
        r.drv.setRangeTiming(50, 100);
        eq(new RangeTiming(49, 100), r.drv.getRangeTiming());
        // the period is scaled by OSC_CALIBRATE_VAL (0x28)
        eq(new byte[] {0, 0, (byte) ((100 * 0x28) >> 8), (byte) (100 * 0x28)}, r.die.regs(0, 0x04, 4));
        r.die.writes.clear();
        r.drv.startRanging();
        List<String> w = writeKeys(r.die);
        eq(ARM_STOP_VARIABLE, w.subList(0, 7));
        eq("00:04", w.get(w.size() - 1)); // SYSRANGE_MODE_TIMED
    }

    public static void backToBackStartAndStopMeasurement() {
        Rig r = ready();
        r.drv.startRanging();
        List<String> w = writeKeys(r.die);
        eq(ARM_STOP_VARIABLE, w.subList(0, 7));
        eq("00:02", w.get(w.size() - 1)); // SYSRANGE_MODE_BACKTOBACK
        r.die.writes.clear();
        r.drv.stopRanging();
        // StopMeasurement zeroes the stop variable on page 1.
        eq(List.of("00:00", "FF:01", "00:00", "91:00", "00:01", "FF:00"), writeKeys(r.die));
    }

    // ── streaming and polling ────────────────────────────────────────────────

    public static void streamedAndPolledFramesDecodeAlike() {
        Rig r = ready();
        Vl53lxRange polled = r.drv.readMeasurement();
        Vl53lxRange streamed = r.drv.decode(F635);
        eq(new int[] {0x14, 12}, r.drv.streamBlock());
        // device status 0x5A -> internal 11 (valid), 0xC104 effective SPADs in 8.8 -> 193 whole, signal
        // 0x022B in 9.7, ambient 0x011B, distance 0x027B = 635 mm.
        eq(635, streamed.distanceMm());
        eq(0, streamed.status());
        eq("Range Valid", streamed.statusText());
        check(streamed.valid(), "valid");
        eq((double) (((0x022BL << 9) * 1000) >> 16), streamed.signalKcps());
        eq((double) (((0x011BL << 9) * 1000) >> 16), streamed.ambientKcps());
        eq(193.0, streamed.spads());
        eq(0x5AL, streamed.extra().get("device_range_status"));
        eq(streamed, polled);
    }

    public static void singleShotWaitsForTheStartBitAndDataReady() {
        Rig r = ready();
        r.die.startBitReads = 3;
        r.die.readyAfterPolls = 4;
        eq(0L, r.drv.performOffsetCalibration(635, 1)); // one shot through the waits, 635 mm
        long startReads = r.die.reads.stream().filter(x -> x[0] == 0 && x[1] == 0x00).count();
        check(startReads >= 4, "SYSRANGE_START polled until the start bit cleared: " + startReads);
        // ... and start_ranging() resumes the continuous mode chosen before.
        r.die.writes.clear();
        r.drv.startRanging();
        List<String> w = writeKeys(r.die);
        eq("00:02", w.get(w.size() - 1));
    }

    public static void singleShotWhoseStartBitNeverClears() {
        Rig r = ready();
        r.die.startBitStuck = true;
        Vl53lxException e = throwsType(Vl53lxException.class, () -> r.drv.performOffsetCalibration(600, 1));
        check(e.getMessage().contains("start bit never cleared"), e.getMessage());
    }

    public static void interruptThatWillNotClear() {
        Rig r = ready();
        r.die.interruptSticky = true;
        Vl53lxException e = throwsType(Vl53lxException.class, () -> r.drv.performOffsetCalibration(600, 1));
        eq("interrupt not cleared after three rounds", e.getMessage());
        eq(3L, writeKeys(r.die).stream().filter("0B:01"::equals).count());
    }

    public static void rangeErrorBitsInTheInterruptStatus() {
        Rig r = ready();
        r.die.interruptIdle = 0x48;
        Vl53lxException e = throwsType(Vl53lxException.class, r.drv::checkForDataReady);
        eq("range error reported in RESULT_INTERRUPT_STATUS 0x48", e.getMessage());
        r.die.interruptIdle = 0x40;
        check(!r.drv.checkForDataReady(), "nothing pending");
    }

    // ── boot and reset ───────────────────────────────────────────────────────

    public static void bootTimeoutWhenTheModelIdNeverAnswers() {
        Rig r = rig(d -> d.nacksAfterReset = 1_000_000);
        r.die.writeMulti(0xBF, new byte[] {0x01}); // arm the NACKs
        Vl53lxException e = throwsType(Vl53lxException.class, () -> r.drv.waitBoot(50));
        eq("timeout waiting for MODEL_ID 0xEE at 0xC0", e.getMessage());
    }

    public static void softResetTimeouts() {
        Rig r = rig(d -> d.resetIgnored = true); // the model id never drops to 0
        Vl53lxException e = throwsType(Vl53lxException.class, () -> r.drv.resetDevice(50));
        eq("timeout in the soft reset of the sensor", e.getMessage());
        r.die.resetIgnored = false;
        r.die.nacksAfterReset = 1_000_000;      // ... and never comes back
        e = throwsType(Vl53lxException.class, () -> r.drv.resetDevice(50));
        eq("timeout in the soft reset of the sensor", e.getMessage());
    }

    // ── NVM ──────────────────────────────────────────────────────────────────

    public static void identificationWordsFromTheNvm() {
        Rig r = ready();
        r.drv.getInfoFromDevice(7);
        eq(FakeVl53l0xDie.PRODUCT_ID, r.drv.productIdString());
        eq(0x2A, r.drv.moduleId());
        eq(0x0C, r.drv.revision());
        eq(0x0C345678L, r.drv.partUidUpper());
        eq(0x9ABCDEF0L, r.drv.partUidLower());
        eq(0xA53CL << 9, r.drv.signalRateMeasFixed400mm());
        // 0x1940 = 6464 sixteenths = 404 mm at the 400 mm target: -4 mm.
        eq(-4000L, r.drv.part2PartOffsetAdjustmentNvmUm());
        // The reference-SPAD record is read once (option 1 was done by init).
        List<Integer> strobes = new ArrayList<>();
        for (Object[] w : r.die.writes) {
            if ((Integer) w[0] == 0x94) {
                strobes.add(((byte[]) w[1])[0] & 0xFF);
            }
        }
        eq(List.of(0x02, 0x7B, 0x77, 0x78, 0x79, 0x7A, 0x7B, 0x7C, 0x73, 0x74, 0x75, 0x76), strobes);
        r.die.writes.clear();
        r.die.reads.clear();
        r.drv.getInfoFromDevice(7);
        eq(0, r.die.writes.size() + r.die.reads.size(), "everything read once");
    }

    public static void nvmOffsetOfATargetMeasuredNearerThan400mm() {
        // 0x18C0 = 6336 sixteenths = 396 mm: the C driver's uint32 difference wraps and its int16 result
        // wraps back, which is what makes the offset come out +4 mm.
        Rig r = ready(d -> {
            d.nvm.put(0x75, 0x00000018L);
            d.nvm.put(0x76, 0xC0000000L);
        });
        r.drv.getInfoFromDevice(4);
        eq(4000L, r.drv.part2PartOffsetAdjustmentNvmUm());
    }

    public static void nvmWithoutA400mmMeasurementHasNoOffset() {
        Rig r = ready(d -> {
            d.nvm.put(0x75, 0L);
            d.nvm.put(0x76, 0L);
        });
        r.drv.getInfoFromDevice(4);
        eq(0L, r.drv.part2PartOffsetAdjustmentNvmUm());
    }

    public static void nvmReadStrobeTimeout() {
        Rig r = rig(d -> d.strobeStuck = true);
        Vl53lxException e = throwsType(Vl53lxException.class, r.drv::sensorInit);
        eq("timeout waiting for the NVM read strobe", e.getMessage());
    }

    // ── reference SPADs from NVM ─────────────────────────────────────────────

    public static void nvmApertureRecordIsAppliedFromTheApertureQuadrant() {
        // 0x8000 bit set: aperture; count 5 -> indices 12..16
        Rig r = ready(d -> d.nvm.put(0x6B, (1L << 15) | (5L << 8)));
        eq(new RefSpads(5, 1), r.drv.getReferenceSpads());
        eq(hex("00f001000000"), r.die.spadEnables());
    }

    public static void nvmRecordThatTheGoodMapCannotServe() {
        Object[][] cases = {
            {0L, "ran out of good reference SPADs"},
            {0x0000FFFFL, "the good SPAD map leaves the requested quadrant"},
        };
        for (Object[] c : cases) {
            Rig r = rig(d -> {
                d.nvm.put(0x24, (Long) c[0]);
                d.nvm.put(0x25, 0L);
            });
            eq(c[1], throwsType(Vl53lxException.class, r.drv::sensorInit).getMessage());
        }
    }

    public static void referenceSpadMapThatDoesNotReadBack() {
        Rig r = rig(d -> d.spadMapStuck = true);
        eq("reference SPAD map did not read back", throwsType(Vl53lxException.class, r.drv::sensorInit).getMessage());
    }

    // ── reference SPAD management ────────────────────────────────────────────

    public static void invalidNvmRecordMakesInitMeasureTheReferenceSpads() {
        for (long record : new long[] {13L << 8, (1L << 15) | (33L << 8)}) {
            Rig r = ready(d -> d.nvm.put(0x6B, record));
            check(!r.drv.refSpadsFromNvm(), "measured, not from NVM");
            // 400 per SPAD against 2560: 3 -> 1200, ..., 6 -> 2400 (160 short), 7 -> 2800 (240 over).
            eq(List.of(1200, 1600, 2000, 2400, 2800), r.die.refPeaks);
            eq(new RefSpads(6, 0), r.drv.getReferenceSpads());
            eq(hex("7b0000000000"), r.die.spadEnables());
            // It ran after the rest of StaticInit, with the sequence config restored.
            eq(0xE8, r.die.bank(0)[0x01] & 0xFF);
            eq(new RangeTiming(33, 0), r.drv.getRangeTiming());
        }
    }

    public static void managementKeepsTheMapThatOvershootsLess() {
        Rig r = ready();
        r.die.refRatePerSpad = 700;
        // 3 -> 2100 (460 short), 4 -> 2800 (240 over, closer): keep 4.
        eq(new RefSpads(4, 0), spads(r.drv.performRefSpadManagement()));
        eq(List.of(2100, 2800), r.die.refPeaks);
        eq(hex("1b0000000000"), r.die.spadEnables()); // indices 0, 1, 3, 4
    }

    public static void managementGoesBackOneWhenTheLastSpadOvershootsMore() {
        Rig r = ready();
        // 400 per SPAD: 6 -> 2400 (160 short), 7 -> 2800 (240 over): the 6-SPAD map goes back.
        eq(new RefSpads(6, 0), spads(r.drv.performRefSpadManagement()));
        eq(hex("7b0000000000"), r.die.spadEnables());
    }

    public static void managementStopsOnTheTargetExactly() {
        Rig r = ready();
        r.die.refRatePerSpad = 640;
        eq(new RefSpads(4, 0), spads(r.drv.performRefSpadManagement()));
        eq(List.of(1920, 2560), r.die.refPeaks);
    }

    public static void managementStopsAtTheEdgeOfTheNonApertureQuadrant() {
        Rig r = ready();
        r.die.refRatePerSpad = 100;
        // 11 good non-aperture SPADs (index 2 is bad) make 1100; the next good SPAD is an aperture one.
        eq(new RefSpads(11, 0), spads(r.drv.performRefSpadManagement()));
        eq(1100, r.die.refPeaks.get(r.die.refPeaks.size() - 1));
        eq(hex("fb0f00000000"), r.die.spadEnables());
    }

    public static void managementSwitchesToApertureSpadsWhenThreeAreTooBright() {
        Rig r = ready();
        r.die.refRatePerSpad = 1000;
        r.die.refRatePerApertureSpad = 100;
        // 3 non-aperture -> 3000 > 2560; restart at index 12 on aperture SPADs:
        // 3 -> 300 ... 25 -> 2500 (60 short), 26 -> 2600 (40 over, closer): 26.
        eq(new RefSpads(26, 1), spads(r.drv.performRefSpadManagement()));
        List<Integer> peaks = r.die.refPeaks;
        eq(List.of(3000, 300), peaks.subList(0, 2));
        eq(List.of(2500, 2600), peaks.subList(peaks.size() - 2, peaks.size()));
        byte[] en = r.die.spadEnables();
        eq(0, en[0] | (en[1] & 0x0F));
        int bits = 0;
        for (byte b : en) {
            bits += Integer.bitCount(b & 0xFF);
        }
        eq(26, bits);
        eq(new RefSpads(26, 1), r.drv.getReferenceSpads());
    }

    public static void managementSettlesForThreeApertureSpadsWhenEvenThoseAreTooBright() {
        Rig r = ready();
        r.die.refRatePerSpad = 1000;
        r.die.refRatePerApertureSpad = 1000;
        eq(new RefSpads(3, 1), spads(r.drv.performRefSpadManagement()));
        eq(List.of(3000, 3000), r.die.refPeaks);
        eq(hex("007000000000"), r.die.spadEnables()); // indices 12, 13, 14
    }

    public static void managementRunsOutOfGoodSpads() {
        // An invalid NVM record makes init measure; the good map holds too few SPADs.
        Object[][] cases = {
            {0x0B000000L, "ran out of good reference SPADs"},           // indices 0, 1, 3: the minimum, then none
            {0x03000000L, "ran out of good reference SPADs"},           // not even the minimum
            {0x0000FF00L, "the good SPAD map leaves the requested quadrant"}, // aperture SPADs only
        };
        for (Object[] c : cases) {
            Rig r = rig(d -> {
                d.nvm.put(0x6B, 13L << 8);
                d.nvm.put(0x24, (Long) c[0]);
                d.nvm.put(0x25, 0L);
            });
            eq(c[1], throwsType(Vl53lxException.class, r.drv::sensorInit).getMessage());
        }
    }

    public static void managementMapThatDoesNotReadBack() {
        Rig r = ready();
        r.die.spadMapStuck = true;
        eq("reference SPAD map did not read back",
                throwsType(Vl53lxException.class, r.drv::performRefSpadManagement).getMessage());
    }

    public static void publicRemeasureRearmsTheStopVariableAndRestoresTheSequence() {
        Rig r = ready();
        r.drv.startRanging();
        r.drv.stopRanging();
        r.die.writes.clear();
        int shotsBefore = r.die.shots.size();
        eq(new RefSpads(6, 0), spads(r.drv.performRefSpadManagement()));
        List<String> w = writeKeys(r.die);
        eq(ARM_STOP_VARIABLE, w.subList(0, 7));
        // The reference measurements run on the reference steps alone (0xC0), after a VHV / phase
        // calibration, and leave the sequence as it was.
        List<int[]> shots = r.die.shots.subList(shotsBefore, r.die.shots.size());
        eq(7, shots.size());
        eq(new int[] {0x41, 0x01}, shots.get(0));
        eq(new int[] {0x01, 0x02}, shots.get(1));
        for (int[] s : shots.subList(2, 7)) {
            eq(new int[] {0x01, 0xC0}, s);
        }
        eq(0xE8, r.die.bank(0)[0x01] & 0xFF);
        // The last write puts the 6-SPAD map back after the 7th overshot.
        eq("B0:7b0000000000", w.get(w.size() - 1));
        eq(0, (int) r.die.bank(0)[0x80]);
    }

    // ── tuning settings ──────────────────────────────────────────────────────

    public static void tuningParametersInTheBuffer() {
        Rig r = ready();
        r.drv.loadTuningSettings(new int[] {
            0xFF, 0x00, 0x00, 0x64,  // SigmaEstRefArray 100
            0xFF, 0x01, 0x03, 0x84,  // SigmaEstEffPulseWidth 900
            0xFF, 0x02, 0x01, 0xF4,  // SigmaEstEffAmbWidth 500
            0xFF, 0x03, 0x0B, 0x00,  // targetRefRate 22 Mcps in 9.7
            0x02, 0x51, 0x00, 0x96,  // a two-byte register write
            0x00,
        });
        eq(List.of("51:0096"), writeKeys(r.die));
        eq(new int[] {100, 900, 500}, new int[] {r.drv.sigmaEstRefArray(), r.drv.sigmaEstEffPulseWidth(), r.drv.sigmaEstEffAmbWidth()});
        eq(0x0B00, r.drv.targetRefRate());
        // ... and the SPAD management converges on the new target: 400 per SPAD, 7 -> 2800 (16 short
        // of 2816), 8 -> 3200 (384 over): 7.
        eq(new RefSpads(7, 0), spads(r.drv.performRefSpadManagement()));
    }

    public static void tuningBufferErrors() {
        Rig r = ready();
        eq("bad tuning parameter 4 at offset 0", throwsType(Vl53lxException.class,
                () -> r.drv.loadTuningSettings(new int[] {0xFF, 0x04, 0x00, 0x00, 0x00})).getMessage());
        eq("bad tuning record at offset 4", throwsType(Vl53lxException.class,
                () -> r.drv.loadTuningSettings(new int[] {0x01, 0x80, 0x01, 0x05, 0x00, 0, 0, 0, 0, 0, 0})).getMessage());
    }

    // ── result decoding and the PAL range status ─────────────────────────────

    public static void deviceStatusMapsOntoThePalRangeStatus() {
        Object[][] cases = {
            {11, 0, "Range Valid"},
            {1, 5, "Hardware Fail"}, {2, 5, "Hardware Fail"}, {3, 5, "Hardware Fail"},
            {6, 4, "Phase Fail"}, {9, 4, "Phase Fail"},
            {8, 3, "Min Range Fail"}, {10, 3, "Min Range Fail"},
            {4, 2, "Signal Fail"},
            {0, 255, "No Update"}, {5, 255, "No Update"}, {7, 255, "No Update"},
            {12, 255, "No Update"}, {13, 255, "No Update"}, {14, 255, "No Update"}, {15, 255, "No Update"},
        };
        Rig r = ready();
        for (Object[] c : cases) {
            int internal = (Integer) c[0];
            Vl53lxRange m = r.drv.decode(frame((internal << 3) | 0x02));
            eq(c[1], m.status(), "internal " + internal);
            eq(c[2], m.statusText(), "internal " + internal);
            eq((long) ((internal << 3) | 0x02), m.extra().get("device_range_status"));
        }
    }

    public static void sigmaFailAndTheSigmaEstimate() {
        Rig r = ready();
        eq(F635, frame(0x5A));
        // The sigma estimate on this frame with the post-init timeouts: 166000 in 16.16 = 2 mm; dmax
        // from the 2.21 Mcps ambient between the 2.0 Mcps (900 mm) and 3.8 Mcps (750 mm) knees -> 882 mm.
        Vl53lxRange good = r.drv.decode(F635);
        eq(0, good.status());
        eq(2.0, good.sigmaMm());
        eq(882L, good.extra().get("dmax_mm"));
        // No signal at all: the estimate pins at its maximum (0x028F87AE >> 16 = 655 mm).
        Vl53lxRange none = r.drv.decode(frame(0x5A, 635, 0, 0x011B, 0xC104));
        eq(1, none.status());
        eq("Sigma Fail", none.statusText());
        eq(655.0, none.sigmaMm());
        // A faint signal under heavy ambient over a long integration: sigma over 18 mm.
        Vl53lxRange faint = r.drv.decode(frame(0x5A, 1800, 4, 0x0800, 0xC104));
        eq(1, faint.status());
        check(faint.sigmaMm() > 18, "sigma " + faint.sigmaMm());
    }

    public static void dmaxOfABadFrameFollowsItsAmbient() {
        Rig r = ready();
        Vl53lxRange bad = r.drv.decode(frame(0x22, 635, 0x022B, 0x1000, 0xC104)); // 32 Mcps ambient
        eq(2, bad.status());
        eq(400L, bad.extra().get("dmax_mm"));
    }

    public static void shortResultBlockIsRefused() {
        Rig r = ready();
        byte[] eleven = java.util.Arrays.copyOf(F635, 11);
        eq("result block too short: 11 bytes", throwsType(Vl53lxException.class, () -> r.drv.decode(eleven)).getMessage());
    }

    // ── offset ───────────────────────────────────────────────────────────────

    public static void offsetRegisterIsQuarterMillimetresInTwosComplement() {
        long[][] cases = {
            {0, 0x0000, 0}, {35_000, 140, 35_000}, {-35_000, 4096 - 140, -35_000},
            {511_000, 2044, 511_000}, {600_000, 2044, 511_000},       // clamped to 511000
            {-512_000, 2048, -512_000}, {-700_000, 2048, -512_000},   // clamped to -512000
            {-1_249, 4096 - 4, -1_000},                               // truncated toward zero
        };
        Rig r = ready();
        for (long[] c : cases) {
            r.drv.setOffsetUm(c[0]);
            eq(be16((int) c[1]), r.die.regs(0, 0x28, 2), "offset " + c[0]);
            eq(c[2], r.drv.getOffsetUm(), "offset " + c[0]);
        }
    }

    public static void offsetInWholeMillimetres() {
        Rig r = ready();
        r.drv.setOffset(-7);
        eq(-7, r.drv.getOffset());
        eq(be16(4096 - 28), r.die.regs(0, 0x28, 2));
    }

    public static void offsetCalibrationAgainstATarget() {
        Rig r = ready();
        r.drv.setOffsetUm(12_000);
        r.die.frames.add(frame(0x22));
        r.die.frames.add(frame(0x22));
        r.die.frames.add(F635);
        r.die.frames.add(frame(0x5A, 636, 0x022B, 0x011B, 0xC104));
        int shotsBefore = r.die.shots.size();
        // Three valid of five samples (the last one repeats): mean (635+636+636)/3 = 635.67 -> 636; the
        // offset becomes (600 - 636) mm.
        eq(-36, r.drv.calibrateOffset(600, 5));
        eq(-36_000L, r.drv.getOffsetUm());
        long ranged = r.die.shots.subList(shotsBefore, r.die.shots.size()).stream().filter(s -> s[1] == 0xE8).count();
        eq(5L, ranged, "five shots with TCC off");
        eq(0xE8, r.die.bank(0)[0x01] & 0xFF, "TCC was off before: nothing to put back");
    }

    public static void offsetCalibrationErrors() {
        Rig r = ready();
        eq(0L, r.drv.performOffsetCalibration(635, 2));
        eq("the calibration distance must be positive",
                throwsType(Vl53lxException.class, () -> r.drv.performOffsetCalibration(0)).getMessage());
        r.die.frame = frame(0x22);
        Vl53lxException e = throwsType(Vl53lxException.class, () -> r.drv.performOffsetCalibration(400, 3));
        check(e.getMessage().contains("no valid measurement"), e.getMessage());
        r.die.frame = frame(0x5A, 0, 0x022B, 0x011B, 0xC104);
        e = throwsType(Vl53lxException.class, () -> r.drv.performOffsetCalibration(400, 3));
        check(e.getMessage().contains("mean range came out 0 mm"), e.getMessage());
    }

    // ── crosstalk ────────────────────────────────────────────────────────────

    public static void xtalkRegisterIs3Dot13() {
        Rig r = ready();
        r.drv.setXtalk(297);
        long rate = (297L << 16) / 1000;
        eq(be16((int) (rate >> 3)), r.die.regs(0, 0x20, 2));
        eq((int) ((((rate >> 3) << 3) * 1000) >> 16), r.drv.getXtalk());
    }

    public static void xtalkCalibrationAgainstATargetBehindTheGlass() {
        Rig r = ready();
        r.die.frames.add(frame(0x22));
        r.die.frames.add(F635);
        // Four valid samples of 635 mm, 284160 (16.16) signal, 193 SPADs at a 1000 mm target: per SPAD
        // 1472 * (65536 - 41615) = 35211712, rounded >> 16 = 537 (0.0082 Mcps) -> 8 kcps.
        eq(8, r.drv.calibrateXtalk(1000, 5));
        eq(be16(537 >> 3), r.die.regs(0, 0x20, 2));
        eq(0xE8, r.die.bank(0)[0x01] & 0xFF);
    }

    public static void xtalkCalibrationWithNothingToCompensate() {
        Rig r = ready();
        // Target at the measured distance (or nearer): no crosstalk.
        eq(0L, r.drv.performXtalkCalibration(635, 2));
        eq(0L, r.drv.performXtalkCalibration(600, 2));
        // No effective SPADs at all: the per-SPAD division is skipped.
        r.die.frame = frame(0x5A, 635, 0x022B, 0x011B, 0x0010);
        eq(0L, r.drv.performXtalkCalibration(1000, 2));
        eq(be16(0), r.die.regs(0, 0x20, 2));
    }

    public static void xtalkCalibrationErrors() {
        Rig r = ready();
        eq("the calibration distance must be positive",
                throwsType(Vl53lxException.class, () -> r.drv.performXtalkCalibration(-1)).getMessage());
        r.die.frame = frame(0x22);
        Vl53lxException e = throwsType(Vl53lxException.class, () -> r.drv.performXtalkCalibration(1000, 2));
        check(e.getMessage().contains("no valid measurement"), e.getMessage());
    }

    public static void isqrtIsTheFloorOfTheRoot() {
        eq(0L, Vl53l0xUldDriver.isqrt(0));
        eq(3L, Vl53l0xUldDriver.isqrt(15));
        eq(4L, Vl53l0xUldDriver.isqrt(16));
        eq(65535L, Vl53l0xUldDriver.isqrt(0xFFFFFFFFL));
    }
}
