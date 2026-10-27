package ai.depz.sensor.test.devices;

import static ai.depz.sensor.test.devices.FakeVl53lxBridge.CLEAR_I2C_ERRORS;
import static ai.depz.sensor.test.devices.FakeVl53lxBridge.GET_DEVICE_NAME;
import static ai.depz.sensor.test.devices.FakeVl53lxBridge.READ_REG;
import static ai.depz.sensor.test.devices.FakeVl53lxBridge.SET_ADDR_WIDTH;
import static ai.depz.sensor.test.devices.FakeVl53lxBridge.SET_I2C_SPEED;
import static ai.depz.sensor.test.devices.FakeVl53lxBridge.SILENT;
import static ai.depz.sensor.test.devices.FakeVl53lxBridge.START_STREAM;
import static ai.depz.sensor.test.devices.FakeVl53lxBridge.STOP_STREAM;
import static ai.depz.sensor.test.devices.FakeVl53lxBridge.WRITE_REG;
import static ai.depz.sensor.test.devices.FakeVl53lxBridge.XSHUT;
import static ai.depz.sensor.test.devices.FakeVl53lxBridge.hex;
import static ai.depz.sensor.test.devices.FakeVl53lxBridge.key;
import static ai.depz.sensor.test.devices.T.check;
import static ai.depz.sensor.test.devices.T.eq;
import static ai.depz.sensor.test.devices.T.throwsType;

import ai.depz.sensor.devices.DepzException;
import ai.depz.sensor.devices.DepzTimeoutException;
import ai.depz.sensor.devices.InvalidStateException;
import ai.depz.sensor.devices.LinkClosedException;
import ai.depz.sensor.devices.StatusException;
import ai.depz.sensor.devices.Subscription;
import ai.depz.sensor.devices.Vl53l0xDevice;
import ai.depz.sensor.devices.Vl53l1cbDevice;
import ai.depz.sensor.devices.Vl53l3cxDevice;
import ai.depz.sensor.devices.Vl53l1cxDevice;
import ai.depz.sensor.devices.Vl53l4cxDevice;
import ai.depz.sensor.devices.Vl53lxDevice;
import ai.depz.sensor.devices.Vl53lxMeasurement;
import ai.depz.sensor.protocol.Vl53lx;
import ai.depz.sensor.sensors.vl53l4.RangeTiming;
import ai.depz.sensor.sensors.vl53lx.Vl53l1Die;
import ai.depz.sensor.sensors.vl53lx.Vl53lxDriver;
import ai.depz.sensor.sensors.vl53lx.Vl53lxException;
import ai.depz.sensor.sensors.vl53lx.Vl53lxPlatform;
import ai.depz.sensor.sensors.vl53lx.Vl53lxProtocolException;
import ai.depz.sensor.sensors.vl53lx.Vl53lxTarget;
import java.lang.reflect.Field;
import java.time.Duration;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.Map;
import java.util.function.Function;

/**
 * {@link Vl53lxDevice} (the APP_VL53L0_4 bridge, contract 12) over the in-process {@link FakeVl53lxBridge}
 * — the Python {@code test_vl53lx_device.py}. The scenario captures cover the init / stream path of each
 * pair; this covers what they cannot: refusals, bridge error statuses, timeouts, the capability-gated
 * passthroughs, calibrations, poll mode and the queues.
 */
public final class Vl53lxDeviceTests {
    // vectors/vl53lx.json
    static final byte[] L4_BLOCK = hex("090901d3200285008900082ba702700285");     // die_block vl53l4cx_uld_as_l4cd_50ms_f2
    static final byte[] L4_BLOCK_F9 = hex("090908d320027f008a00092bce026f027f");  // ..._f9
    static final byte[] L1_BLOCK = hex("090901439007d9013f000d2234024007d9");     // die_block vl53l1cx_uld_long_33ms_f2
    static final String START_STREAM_DIE = "890011000001860001";                   // encode start_stream_die_block
    static final byte[] INFO_WRAPPED = hex("ffffffff0700000003000000020100e80302010c000000"); // decode info_wrapped_counters

    static final String L4CD_BOARD = "DEPZ ToF Sensor VL53L4CD USB v2.1 TESTSERIAL";
    static final String L4CX_BOARD = "DEPZ ToF Sensor VL53L4CX USB v2.1 TOVJALN523";
    static final String L1CX_BOARD = "DEPZ ToF Sensor VL53L1CX USB v2.1 SER";
    static final String L3CX_BOARD = "DEPZ ToF Sensor VL53L3CX USB v2.1 SER";
    static final String L0X_BOARD = "DEPZ ToF Sensor VL53L0X USB v2.1 SER";
    static final String UNSTAMPED = "DEPZ ToF Sensor USB v2.1";

    private static final Duration TIMEOUT = Duration.ofMillis(500);

    /** A device over its fake; closing closes both. */
    static final class Rig implements AutoCloseable {
        final Vl53lxDevice dev;
        final FakeVl53lxBridge fake;

        Rig(Vl53lxDevice dev, FakeVl53lxBridge fake) {
            this.dev = dev;
            this.fake = fake;
        }

        @Override
        public void close() {
            dev.close();
            fake.link.close();
        }
    }

    static Rig connect(String board, Function<FakeVl53lxBridge, Vl53lxDevice> make, boolean noClear) {
        FakeVl53lxBridge fake = new FakeVl53lxBridge(board, noClear);
        return new Rig(make.apply(fake), fake);
    }

    static Rig connect(String board) {
        return connect(board, f -> new Vl53lxDevice(f.link, TIMEOUT), false);
    }

    static Rig connect() {
        return connect(L4CD_BOARD);
    }

    static Rig ready(String board, String product) {
        Rig r = connect(board);
        r.dev.init(null, product);
        r.fake.clearRx();
        return r;
    }

    static Rig ready() {
        return ready(L4CD_BOARD, null);
    }

    /** Wait until the reader thread has handled everything the fake sent: a GET_INFO round trip behind the reports. */
    static void settle(Rig r) {
        r.dev.bridgeInfo();
        synchronized (r.fake.rx) {
            r.fake.rx.remove(r.fake.rx.size() - 1);
        }
    }

    static Thread later(Runnable fn) {
        Thread t = new Thread(() -> {
            try {
                Thread.sleep(50);
            } catch (InterruptedException e) {
                return;
            }
            fn.run();
        });
        t.setDaemon(true);
        t.start();
        return t;
    }

    /** The bridge platform the ULD port talks to (the Python tests' {@code dev._bridge}). */
    static Vl53lxPlatform bridge(Vl53lxDevice dev) throws Exception {
        Field f = Vl53lxDriver.class.getDeclaredField("p");
        f.setAccessible(true);
        return (Vl53lxPlatform) f.get(dev.driver());
    }

    static List<String> hexes(List<byte[]> payloads) {
        List<String> out = new ArrayList<>();
        for (byte[] p : payloads) {
            out.add(java.util.HexFormat.of().formatHex(p));
        }
        return out;
    }

    static int count(List<Integer> cmds, int cmd) {
        return (int) cmds.stream().filter(c -> c == cmd).count();
    }

    // ── pure helpers ─────────────────────────────────────────────────────────

    static Vl53lxTarget t(int distanceMm, int status) {
        return new Vl53lxTarget(distanceMm, status, "", 0, 0, 0, distanceMm, distanceMm);
    }

    static Vl53lxMeasurement m(int distanceMm, int status, Vl53lxTarget... targets) {
        return new Vl53lxMeasurement(0, distanceMm, status, "", 0, 0, 0, 0, List.of(targets), Map.of(), null);
    }

    public static void plottableStatusesAre0And6And11() {
        eq(List.of(0, 6, 11), Vl53lxMeasurement.PLOTTABLE_STATUSES);
    }

    public static void plotDistancesKeepsPlottableTargetsInDriverOrder() {
        Vl53lxMeasurement m = m(0, 0, t(600, 6), t(900, 4), t(1500, 11), t(30, 0));
        eq(List.of(600, 1500, 30), m.plotDistances());
        eq(t(600, 6), m.primaryTarget());
    }

    public static void lightDriverUsesTheSingleDistanceOrNothing() {
        eq(List.of(624), m(624, 0).plotDistances());
        eq(List.of(), m(624, 4).plotDistances());
        eq(null, m(624, 0).primaryTarget());
        eq(null, m(0, 0, t(1, 7)).primaryTarget());
        Vl53lxMeasurement six = m(624, 6);
        eq(List.of(false, true), List.of(six.valid(), six.plottable()));
    }

    // ── identity and init ────────────────────────────────────────────────────

    public static void beforeInitNoDriverNoProduct() throws Exception {
        try (Rig r = connect()) {
            check(!r.dev.initialized(), "not initialized");
            eq(null, r.dev.product());
            eq(null, r.dev.driverKind());
            InvalidStateException e = throwsType(InvalidStateException.class, r.dev::driver);
            check(e.getMessage().contains("call init() first"), e.getMessage());
            throwsType(InvalidStateException.class, () -> r.dev.supports("offset"));
            throwsType(InvalidStateException.class, r.dev::getOffsetMm);
        }
    }

    public static void initBindsTheProductAndSetsTheAddressWidthFirst() throws Exception {
        try (Rig r = connect()) {
            r.dev.init();
            check(r.dev.initialized(), "initialized");
            eq("VL53L4CD", r.dev.product());
            eq("uld", r.dev.driverKind());
            check(r.dev.supports("offset"), "offset");
            check(!r.dev.supports("roi"), "no roi");
            eq(List.of(), r.dev.modes());

            List<Integer> cmds = r.fake.cmds();
            // device name, then SET_ADDR_WIDTH 2 before the first register access
            eq(List.of(GET_DEVICE_NAME, SET_ADDR_WIDTH), cmds.subList(0, 2));
            eq("02", hexes(r.fake.sent(SET_ADDR_WIDTH)).get(0));
            check(cmds.indexOf(SET_ADDR_WIDTH) < cmds.indexOf(READ_REG), "width before the first read");
            // every init runs at 400 kHz, then the die's 1 MHz ceiling
            eq(List.of("9001", "e803"), hexes(r.fake.sent(SET_I2C_SPEED)));
            // the blob at 0x2D with byte 0 forced to FM+ (0x12)
            byte[] blob = null;
            for (Object[] w : r.fake.writes()) {
                if ((int) w[0] == 0x2D) {
                    blob = (byte[]) w[1];
                    break;
                }
            }
            check(blob != null && blob.length == 91 && blob[0] == 0x12, "config blob");
            // VL53L4CD_SensorInit() closes with ALGO__RANGE_IGNORE_THRESHOLD_MCPS
            check(r.fake.writeKeys().contains(key(0x0024, 0x05, 0x00)), "0x0024 <- 0x0500");
            // CLEAR_I2C_ERRORS closes every init (contract 12 §2)
            eq(CLEAR_I2C_ERRORS, cmds.get(cmds.size() - 1));
            eq(List.of(""), hexes(r.fake.sent(CLEAR_I2C_ERRORS)));
        }
    }

    public static void aClassWithItsOwnProductNeedsNoDeviceName() throws Exception {
        // Contract 12 §1: VL53L0X is the one 1-byte-address part; its full init needs a die model the
        // fake has not got, so this checks what comes before the first register access.
        try (Rig r = connect(L0X_BOARD, f -> new Vl53l0xDevice(f.link, Duration.ofMillis(60)), false)) {
            r.fake.failNext(READ_REG, SILENT);
            // the L0X ULD polls the model id through the bus fault, then gives up as the driver's own error
            Vl53lxException e = throwsType(Vl53lxException.class, r.dev::init);
            check(e.getMessage().contains("MODEL_ID"), e.getMessage());
            // the device sets the width before the first register access, the ULD once more as its init starts
            eq(List.of(SET_ADDR_WIDTH, SET_ADDR_WIDTH, SET_I2C_SPEED, READ_REG), r.fake.cmds().subList(0, 4));
            eq(List.of(), r.fake.sent(GET_DEVICE_NAME));
            eq(List.of("01", "01"), hexes(r.fake.sent(SET_ADDR_WIDTH)));
            eq(List.of("uld"), r.dev.driverKinds());
            check(!r.dev.initialized(), "not initialized");
        }
    }

    public static void deviceNameIsReadOnce() throws Exception {
        try (Rig r = connect()) {
            eq("VL53L4CD", r.dev.detected());
            r.dev.init();
            r.dev.notes();
            r.dev.identifyProduct();
            eq(1, r.fake.sent(GET_DEVICE_NAME).size());
            eq(L4CD_BOARD, r.dev.boardName());
        }
    }

    public static void identifyProductReportsBothReachesAndTheModelIdCheck() throws Exception {
        try (Rig r = ready()) {
            Map<String, Object> info = r.dev.identifyProduct();
            eq(L4CD_BOARD, info.get("board"));
            eq("VL53L4CD", info.get("detected"));
            eq("VL53L4CD", info.get("product"));
            eq("uld", info.get("driver"));
            eq("Vl53l4UldDriver", info.get("driver_class"));
            eq(List.of("uld", "histogram"), info.get("kinds"));
            eq(0xEBAA, info.get("model_id"));
            eq(true, info.get("model_id_ok"));
            eq(List.of(), info.get("modes"));
            eq(1300, info.get("reach_mm"));
            eq(new int[] {10, 200}, info.get("budget_ms"));
            eq(List.of(), info.get("budget_choices"));
            eq(false, info.get("histogram"));
            eq(1000, info.get("max_khz"));
            eq(null, info.get("caveat"));
            // §5: the 0x38 window at the die's own oscillator, ~1388 mm
            eq(1388L, Vl53l1Die.phaseWindowMm(FakeVl53lxBridge.DIE_OSC, 0x38));
            eq(1388, info.get("driver_reach_mm"));
            eq(1388, r.dev.driverReachMm());
            check(((List<?>) info.get("supports")).contains("calib_xtalk"), "supports calib_xtalk");
            eq(List.of(), r.dev.notes());
            eq(new int[] {10, 200}, r.dev.budgetRange());
        }
    }

    public static void aWrongModelIdFailsTheCrossCheckNotTheInit() throws Exception {
        try (Rig r = ready()) {
            r.fake.setWord(0x010F, 0xEACC);
            Map<String, Object> info = r.dev.identifyProduct();
            eq(0xEACC, info.get("model_id"));
            eq(false, info.get("model_id_ok"));
        }
    }

    public static void borrowingASiblingsDriver() throws Exception {
        try (Rig r = ready(L4CX_BOARD, "VL53L4CD")) {
            Map<String, Object> info = r.dev.identifyProduct();
            eq("VL53L4CX", info.get("detected"));
            eq("VL53L4CD", info.get("product"));
            eq(6000, info.get("reach_mm"));
            eq(1388, info.get("driver_reach_mm"));
            List<String> notes = r.dev.notes();
            eq(2, notes.size());
            check(notes.get(0).contains("borrowing this driver - the board says it is a VL53L4CX"), notes.get(0));
            check(notes.get(1).contains("reaches 1388 mm, the board is rated 6000 mm"), notes.get(1));
        }
    }

    public static void anUnstampedBoardNeedsTheProductNamed() throws Exception {
        try (Rig r = connect(UNSTAMPED)) {
            eq(null, r.dev.detected());
            eq(List.of(), r.dev.driverKinds());
            InvalidStateException e = throwsType(InvalidStateException.class, r.dev::init);
            check(e.getMessage().contains("carries no product number"), e.getMessage());
            eq(List.of(), r.fake.sent(SET_ADDR_WIDTH));
            r.dev.init(null, "VL53L4CD");
            eq("VL53L4CD", r.dev.product());
            eq(List.of("VL53L4CD: board name carries no product number, named by hand"), r.dev.notes());
        }
    }

    public static void aPairTheTableHasNoRowForIsRefusedBeforeAnyTraffic() throws Exception {
        try (Rig r = connect(L4CX_BOARD, f -> new Vl53l4cxDevice(f.link, TIMEOUT), false)) {
            eq(List.of("histogram"), r.dev.driverKinds());
            UnsupportedOperationException e = throwsType(UnsupportedOperationException.class, () -> r.dev.init("uld"));
            check(e.getMessage().contains("VL53L4CX has no 'uld' driver - it has histogram"), e.getMessage());
            e = throwsType(UnsupportedOperationException.class, () -> r.dev.init(null, "VL53L9ZZ"));
            check(e.getMessage().contains("no such product"), e.getMessage());
            eq(List.of(), r.fake.cmds().stream().filter(c -> c != GET_DEVICE_NAME).toList());
            check(!r.dev.initialized(), "not initialized");
        }
    }

    public static void driverKindsOfANamedProductAndOfTheClassProduct() throws Exception {
        try (Rig r = connect(UNSTAMPED, f -> new Vl53l1cxDevice(f.link, TIMEOUT), false)) {
            eq(List.of("uld", "histogram"), r.dev.driverKinds());
            eq(List.of("ulp", "histogram"), r.dev.driverKinds("VL53L3CX"));
        }
    }

    public static void aV200BridgeIsToldToReflash() throws Exception {
        try (Rig r = connect(L4CD_BOARD, f -> new Vl53lxDevice(f.link, TIMEOUT), true)) {
            DepzException e = throwsType(DepzException.class, r.dev::init);
            check(!(e instanceof Vl53lxProtocolException), "a plain DepzException");
            check(e.getMessage().contains("predates APP_VL53L0_4_v0.24") && e.getMessage().contains("reflash"), e.getMessage());
            check(!r.dev.initialized(), "not initialized");
        }
    }

    public static void anyOtherClearI2cErrorsFailureIsAProtocolError() throws Exception {
        try (Rig r = connect()) {
            r.fake.failNext(CLEAR_I2C_ERRORS, 0x09);
            throwsType(Vl53lxProtocolException.class, r.dev::init);
        }
    }

    public static void aSilentClearI2cErrorsIsAProtocolError() throws Exception {
        try (Rig r = connect(L4CD_BOARD, f -> new Vl53lxDevice(f.link, Duration.ofMillis(60)), false)) {
            r.fake.failNext(CLEAR_I2C_ERRORS, SILENT);
            throwsType(Vl53lxProtocolException.class, r.dev::init);
        }
    }

    public static void aRefusedSetAddrWidthFailsInitAsProtocolError() throws Exception {
        try (Rig r = connect()) {
            r.fake.failNext(SET_ADDR_WIDTH, 0x04);
            throwsType(Vl53lxProtocolException.class, r.dev::init);
            check(!r.dev.initialized(), "not initialized");
        }
    }

    // ── the bridge platform the ULD ports talk to ────────────────────────────

    public static void readsPast253BytesInChunks() throws Exception {
        try (Rig r = ready()) {
            byte[] want = new byte[300];
            for (int i = 0; i < 300; i++) {
                want[i] = (byte) i;
            }
            System.arraycopy(want, 0, r.fake.regs, 0, 300);
            eq(want, bridge(r.dev).readMulti(0, 300));
            eq(List.of("0000fd00", "fd002f00"), hexes(r.fake.sent(READ_REG)));
        }
    }

    public static void writesPast253BytesInChunks() throws Exception {
        try (Rig r = ready()) {
            byte[] data = new byte[300];
            for (int i = 0; i < 300; i++) {
                data[i] = (byte) (i * 7);
            }
            bridge(r.dev).writeMulti(0x1000, data);
            List<Object[]> w = r.fake.writes();
            eq(2, w.size());
            eq(List.of(0x1000, 253), List.of((int) w.get(0)[0], ((byte[]) w.get(0)[1]).length));
            eq(List.of(0x1000 + 253, 47), List.of((int) w.get(1)[0], ((byte[]) w.get(1)[1]).length));
            eq(data, Arrays.copyOfRange(r.fake.regs, 0x1000, 0x1000 + 300));
        }
    }

    public static void anErrorStatusOnARegisterAccessIsAProtocolError() throws Exception {
        try (Rig r = ready()) {
            Vl53lxPlatform b = bridge(r.dev);
            r.fake.failNext(READ_REG, 0x09);
            throwsType(Vl53lxProtocolException.class, () -> b.readMulti(0x010F, 2));
            r.fake.failNext(WRITE_REG, 0x09);
            throwsType(Vl53lxProtocolException.class, () -> b.writeMulti(0x0087, new byte[] {0x40}));
            r.fake.failNext(SET_I2C_SPEED, 0x04);
            throwsType(Vl53lxProtocolException.class, () -> b.setI2cSpeed(1000));
            r.fake.failNext(XSHUT, 0x09);
            throwsType(Vl53lxProtocolException.class, b::xshutReset);
            r.fake.failNext(SET_ADDR_WIDTH, 0x04);
            throwsType(Vl53lxProtocolException.class, () -> b.setAddrWidth(2));
        }
    }

    public static void aShortRegisterReadIsAProtocolError() throws Exception {
        try (Rig r = ready()) {
            r.fake.truncateNextRead = 1; // the bridge answers one byte of two
            Vl53lxProtocolException e = throwsType(Vl53lxProtocolException.class, () -> bridge(r.dev).readMulti(0x0089, 2));
            check(e.getMessage().contains("expected 2, got 1"), e.getMessage());
        }
    }

    public static void aReadPast0xFFFFIsRefusedBeforeTheWire() throws Exception {
        // contract 12 §2 / 10 §1: addr + len <= 0x10000 is the host's check (ERRATA A13)
        try (Rig r = ready()) {
            int reads = r.fake.sent(FakeVl53lxBridge.READ_REG).size();
            throwsType(IllegalArgumentException.class, () -> bridge(r.dev).readMulti(0xFFFF, 2));
            eq(reads, r.fake.sent(FakeVl53lxBridge.READ_REG).size());
        }
    }

    public static void noAnswerIsAProtocolErrorToo() throws Exception {
        try (Rig r = connect(L4CD_BOARD, f -> new Vl53lxDevice(f.link, Duration.ofMillis(30)), false)) {
            r.dev.init();
            r.fake.failNext(SET_I2C_SPEED, SILENT);
            throwsType(Vl53lxProtocolException.class, () -> bridge(r.dev).setI2cSpeed(400));
        }
    }

    public static void argumentErrorsPassThroughUnchanged() throws Exception {
        try (Rig r = ready()) {
            Vl53lxPlatform b = bridge(r.dev);
            throwsType(IllegalArgumentException.class, () -> b.setI2cSpeed(70000));
            throwsType(IllegalArgumentException.class, () -> b.setAddrWidth(3));
            eq(List.of(), r.fake.sent(SET_I2C_SPEED));
            eq(List.of(), r.fake.sent(SET_ADDR_WIDTH));
        }
    }

    public static void bridgeInfoDecodesRptVl53Info() throws Exception {
        try (Rig r = connect()) {
            r.fake.info = INFO_WRAPPED;
            Vl53lx.Vl53lxInfo info = r.dev.bridgeInfo();
            eq(new Vl53lx.Vl53lxInfo(4294967295L, 7, 3, 2, 1, 0, 1000, 2, 1, 12), info);
        }
    }

    public static void xshutDropsTheDriverAndTheStream() throws Exception {
        try (Rig r = ready()) {
            r.dev.startRanging();
            r.dev.xshut(Vl53lx.XSHUT_OFF);
            eq(List.of("00"), hexes(r.fake.sent(XSHUT)));
            check(!r.dev.ranging(), "not ranging");
            check(!r.dev.initialized(), "not initialized");
            throwsType(InvalidStateException.class, r.dev::driver);
            throwsType(IllegalArgumentException.class, () -> r.dev.xshut(5));
        }
    }

    // ── configuration and the capability-gated passthroughs ──────────────────

    public static void configureReinitsThenTimingOffsetXtalkSignal() throws Exception {
        try (Rig r = ready()) {
            r.dev.configure(33, 0, null, -10, 20, 1024);
            eq(1, r.fake.sent(CLEAR_I2C_ERRORS).size());
            // vectors/vl53l4.json tuning: offset -10 → 65496, xtalk 20 → 10240, signal 1024 → 128
            List<String> w = r.fake.writeKeys();
            eq(List.of(key(0x001E, 0xFF, 0xD8), key(0x0020, 0, 0), key(0x0022, 0, 0), key(0x0018, 0, 0),
                    key(0x001A, 0, 0), key(0x0016, 0x28, 0x00), key(0x0066, 0x00, 0x80)), w.subList(w.size() - 7, w.size()));
            eq(-10, r.dev.getOffsetMm());
            eq(20, r.dev.getXtalkKcps());
            eq(1024, r.dev.getSignalThresholdKcps());
            RangeTiming t = r.dev.getRangeTiming();
            eq(0, t.interMeasurementMs());
            check(Math.abs(t.timingBudgetMs() - 33) <= 1, "budget " + t.timingBudgetMs());
            eq(null, r.dev.getMode());
        }
    }

    public static void anInterMeasurementPeriodSelectsAutonomousMode() throws Exception {
        try (Rig r = ready()) {
            r.dev.configure(50, 100);
            eq((long) (1.055 * 100 * (FakeVl53lxBridge.DIE_PLL & 0x3FF)), r.fake.dword(0x006C));
            r.fake.clearRx();
            r.dev.startRanging();
            eq(List.of(key(0x0087, 0x40)), r.fake.writeKeys());
        }
    }

    public static void aPeriodNotAboveTheBudgetIsRefused() throws Exception {
        try (Rig r = ready()) {
            throwsType(Vl53lxException.class, () -> r.dev.configure(50, 20));
            Vl53lxException e = throwsType(Vl53lxException.class, () -> r.dev.configure(300, 0));
            check(e.getMessage().contains("10..200"), e.getMessage());
        }
    }

    public static void aModeOnAProductWithoutModesIsRefusedBeforeTheReinit() throws Exception {
        try (Rig r = ready()) {
            UnsupportedOperationException e = throwsType(UnsupportedOperationException.class, () -> r.dev.configure(50, 0, "long"));
            check(e.getMessage().contains("VL53L4CD/uld does not support 'mode'"), e.getMessage());
            eq(0, r.fake.rxCount());
        }
    }

    public static void snapBudgetClampsToTheDriversRangeWhenAnyBudgetGoes() throws Exception {
        try (Rig r = ready()) {
            eq(List.of(), r.dev.budgetChoices());
            eq(List.of(10, 77, 200), List.of(r.dev.snapBudget(5), r.dev.snapBudget(77), r.dev.snapBudget(500)));
        }
    }

    public static void offsetXtalkThresholdsSignalSigmaRoundTrip() throws Exception {
        try (Rig r = ready()) {
            r.dev.setOffsetMm(10);
            eq(40, r.fake.word(0x001E));
            eq(10, r.dev.getOffsetMm());
            r.dev.setXtalkKcps(3);
            eq(1536, r.fake.word(0x0016));
            eq(3, r.dev.getXtalkKcps());
            r.dev.setDetectionThresholds(100, 300, 3);
            eq(3, r.fake.regs[0x0046] & 0xFF);
            eq(List.of(300, 100), List.of(r.fake.word(0x0072), r.fake.word(0x0074)));
            eq(new Vl53lxDriver.Thresholds(100, 300, 3), r.dev.getDetectionThresholds());
            r.dev.setSignalThresholdKcps(2048);
            eq(256, r.fake.word(0x0066));
            eq(2048, r.dev.getSignalThresholdKcps());
            r.dev.setSigmaThresholdMm(15);
            eq(60, r.fake.word(0x0064));
            eq(15, r.dev.getSigmaThresholdMm());
            Vl53lxException e = throwsType(Vl53lxException.class, () -> r.dev.setSigmaThresholdMm(16384));
            check(e.getMessage().contains("16383"), e.getMessage());
        }
    }

    public static void anUnsupportedGroupIsRefusedWithNoTraffic() throws Exception {
        try (Rig r = ready()) {
            List<T.Body> calls = List.of(r.dev::getRoi, () -> r.dev.setRoi(4, 4), r.dev::getRoiCenter,
                    () -> r.dev.setRoiCenter(199), r.dev::performRefSpadManagement);
            for (T.Body call : calls) {
                UnsupportedOperationException e = throwsType(UnsupportedOperationException.class, call);
                check(e.getMessage().contains("VL53L4CD/uld does not support"), e.getMessage());
            }
            eq(0, r.fake.rxCount());
        }
    }

    public static void theRefusalNamesTheCaveatAndComesBeforeTheReinit() throws Exception {
        try (Rig r = ready(L3CX_BOARD, null)) {
            eq("ulp", r.dev.driverKind());
            UnsupportedOperationException e = throwsType(UnsupportedOperationException.class,
                    () -> r.dev.configure(50, 0, null, 5, null, null));
            check(e.getMessage().contains("VL53L3CX/ulp does not support 'offset' (single-target ranging only"), e.getMessage());
            e = throwsType(UnsupportedOperationException.class, () -> r.dev.calibrateXtalk(600));
            check(e.getMessage().contains("'calib_xtalk'"), e.getMessage());
            eq(0, r.fake.rxCount());
            check(r.dev.notes().stream().anyMatch(n -> n.startsWith("VL53L3CX/ulp: single-target")), r.dev.notes().toString());
        }
    }

    public static void temperatureUpdateRerunsVhvAndRestoresTheLoopBound() throws Exception {
        try (Rig r = ready()) {
            r.dev.startTemperatureUpdate();
            eq(List.of(key(0x0008, 0x81), key(0x000B, 0x92), key(0x0087, 0x21), key(0x0086, 0x01), key(0x0087, 0x80),
                    key(0x0008, 0x09), key(0x000B, 0x00)), r.fake.writeKeys());
        }
    }

    // ── calibrations (VL53L4CD_calibration.c) ────────────────────────────────

    public static void offsetCalibrationTargetMinusMeanProgrammedX4() throws Exception {
        try (Rig r = ready()) {
            r.fake.setResultBlock(L4_BLOCK); // 624 mm
            eq(-24, r.dev.calibrateOffset(600, 5));
            eq(-96 & 0xFFFF, r.fake.word(0x001E));
            eq(-24, r.dev.getOffsetMm());
            // offsets zeroed first, 10 heat frames + 5 samples each released
            List<String> w = r.fake.writeKeys();
            eq(List.of(key(0x001E, 0, 0), key(0x0020, 0, 0), key(0x0022, 0, 0)), w.subList(0, 3));
            eq(15, (int) r.fake.writes().stream().filter(x -> (int) x[0] == 0x0086).count());
        }
    }

    public static void xtalkCalibrationFromTheValidSamplesAfterTheFirst() throws Exception {
        try (Rig r = ready()) {
            r.fake.setResultBlock(L4_BLOCK); // 624 mm, 5160 kcps, 211 SPADs
            eq(1, r.dev.calibrateXtalk(650, 5));
            // (1 - 624/650) × 5160/211 = 0.978 kcps → ×512
            eq(500, r.fake.word(0x0016));
        }
    }

    public static void xtalkCalibrationFailsWithNoValidSample() throws Exception {
        try (Rig r = ready()) {
            byte[] bad = L4_BLOCK.clone();
            bad[0] = 0x04; // raw 4 → status 2, signal below threshold
            r.fake.setResultBlock(bad);
            Vl53lxException e = throwsType(Vl53lxException.class, () -> r.dev.calibrateXtalk(650, 5));
            check(e.getMessage().contains("no valid samples"), e.getMessage());
        }
    }

    public static void xtalkCalibrationFailsPast127Kcps() throws Exception {
        try (Rig r = ready()) {
            byte[] hot = L4_BLOCK.clone();
            hot[3] = 0x01; // 1 SPAD, 524280 kcps
            hot[4] = 0x00;
            hot[5] = (byte) 0xFF;
            hot[6] = (byte) 0xFF;
            r.fake.setResultBlock(hot);
            Vl53lxException e = throwsType(Vl53lxException.class, () -> r.dev.calibrateXtalk(5000, 5));
            check(e.getMessage().contains("> 127"), e.getMessage());
        }
    }

    public static void calibrationArgumentsOutOfRangeAreRefusedBeforeAnyTraffic() throws Exception {
        try (Rig r = ready()) {
            Vl53lxException e = throwsType(Vl53lxException.class, () -> r.dev.calibrateOffset(600, 4));
            check(e.getMessage().contains("5..255"), e.getMessage());
            e = throwsType(Vl53lxException.class, () -> r.dev.calibrateOffset(1001));
            check(e.getMessage().contains("10..1000"), e.getMessage());
            e = throwsType(Vl53lxException.class, () -> r.dev.calibrateXtalk(5001));
            check(e.getMessage().contains("10..5000"), e.getMessage());
            throwsType(Vl53lxException.class, () -> r.dev.calibrateXtalk(600, 256));
            eq(0, r.fake.rxCount());
        }
    }

    // ── VL53L1X ULD: modes and tabulated budgets ─────────────────────────────

    public static void l1ModesBudgetChoicesThatMoveWithTheModeAndSnap() throws Exception {
        try (Rig r = ready(L1CX_BOARD, null)) {
            eq(List.of("long", "short"), r.dev.modes());
            eq("long", r.dev.getMode());
            eq(List.of(20, 33, 50, 100, 200, 500), r.dev.budgetChoices());
            eq(List.of(33, 500), List.of(r.dev.snapBudget(40), r.dev.snapBudget(1000)));
            r.dev.configure(15, 0, "short");
            eq("short", r.dev.getMode());
            eq(15, r.dev.budgetChoices().get(0));
            eq(new RangeTiming(15, 15), r.dev.getRangeTiming());
            eq((int) Vl53l1Die.phaseWindowMm(FakeVl53lxBridge.DIE_OSC, 0x38), r.dev.identifyProduct().get("driver_reach_mm"));
        }
    }

    public static void l1ABudgetTheModeDoesNotHaveIsRefusedNotRounded() throws Exception {
        try (Rig r = ready(L1CX_BOARD, null)) {
            Vl53lxException e = throwsType(Vl53lxException.class, () -> r.dev.configure(40, 0));
            check(e.getMessage().contains("pick one of 20, 33, 50, 100, 200, 500"), e.getMessage());
            IllegalArgumentException m = throwsType(IllegalArgumentException.class, () -> r.dev.configure(50, 0, "medium"));
            check(m.getMessage().contains("no such mode"), m.getMessage());
        }
    }

    public static void l1RoiSizeAndCentre() throws Exception {
        try (Rig r = ready(L1CX_BOARD, null)) {
            r.dev.setRoi(8, 6);
            eq(0x91, r.fake.regs[0x007F] & 0xFF); // the optical centre the die reports
            eq(0x57, r.fake.regs[0x0080] & 0xFF);
            eq(new int[] {8, 6}, r.dev.getRoi());
            r.dev.setRoi(16, 16); // over 10: the centre moves to 199
            eq(199, r.dev.getRoiCenter());
            r.dev.setRoiCenter(0x88);
            eq(0x88, r.dev.getRoiCenter());
        }
    }

    public static void l1StreamsDecodeWithTheL1Variant() throws Exception {
        try (Rig r = ready(L1CX_BOARD, null)) {
            r.dev.startRanging();
            later(() -> r.fake.stream(L1_BLOCK, 1));
            Vl53lxMeasurement m = r.dev.getMeasurement(Duration.ofSeconds(1));
            eq(List.of(576, 0, 3.0, 16072.0, 2552.0, 67.0),
                    List.of(m.distanceMm(), m.status(), m.sigmaMm(), m.signalKcps(), m.ambientKcps(), m.spads()));
            eq(Map.of("signal_per_spad_kcps", 23L, "ambient_per_spad_kcps", 3L, "stream_count", 1L), m.extra());
        }
    }

    // ── ranging ──────────────────────────────────────────────────────────────

    public static void startArmsTheStreamWithTheDieBlockAndItsRelease() throws Exception {
        try (Rig r = ready()) {
            r.dev.startRanging();
            eq(List.of(key(0x0087, 0x21)), r.fake.writeKeys()); // continuous
            eq(List.of(START_STREAM_DIE), hexes(r.fake.sent(START_STREAM)));
            check(r.dev.ranging(), "ranging");
        }
    }

    public static void eachStreamedBlockIsDecodedOnceToEverySubscriber() throws Exception {
        try (Rig r = ready()) {
            List<Vl53lxMeasurement> seen = new ArrayList<>();
            AutoCloseable unsub = r.dev.onMeasurement(seen::add);
            Subscription<Vl53lxMeasurement> it = r.dev.measurements();
            r.dev.startRanging();
            r.fake.stream(L4_BLOCK, 123_456);
            r.fake.stream(L4_BLOCK_F9, 123_789);
            settle(r);
            eq(List.of(123_456L, 123_789L), seen.stream().map(Vl53lxMeasurement::timestampUs).toList());
            eq(new Vl53lxMeasurement(123_456, 624, 0, "valid", 5160, 1096, 2, 211, List.of(),
                    Map.of("signal_per_spad_kcps", 24L, "ambient_per_spad_kcps", 5L, "stream_count", 1L), null), seen.get(0));
            check(seen.get(0).valid() && seen.get(0).plottable(), "valid and plottable");
            eq(623, seen.get(1).distanceMm());
            eq(8L, seen.get(1).extra().get("stream_count"));
            eq(List.of(624, 623), List.of(it.take(Duration.ofSeconds(1)).distanceMm(), it.take(Duration.ofSeconds(1)).distanceMm()));
            unsub.close();
            r.fake.stream(L4_BLOCK, 1);
            settle(r);
            eq(2, seen.size());
        }
    }

    public static void aNonzeroStatusIsNotValidBut6IsPlottable() throws Exception {
        try (Rig r = ready()) {
            r.dev.startRanging();
            byte[] blk = L4_BLOCK.clone();
            blk[0] = 0x13; // raw 19 → status 6
            later(() -> r.fake.stream(blk, 1));
            Vl53lxMeasurement m = r.dev.getMeasurement(Duration.ofSeconds(1));
            eq(List.of(6, false, true), List.of(m.status(), m.valid(), m.plottable()));
        }
    }

    public static void aBlockTheDriverCannotDecodeIsCountedAndTheReaderSurvives() throws Exception {
        try (Rig r = ready()) {
            r.dev.startRanging();
            r.fake.stream(Arrays.copyOf(L4_BLOCK, 5), 1);
            settle(r);
            eq(1L, r.dev.streamParseErrors());
            later(() -> r.fake.stream(L4_BLOCK, 2));
            eq(624, r.dev.getMeasurement(Duration.ofSeconds(1)).distanceMm());
        }
    }

    public static void aMalformedStreamReportIsCountedAndDropped() throws Exception {
        // The length field says 17, only 5 bytes follow (contract 07 §3).
        try (Rig r = ready()) {
            r.dev.startRanging();
            r.fake.stream(Arrays.copyOf(L4_BLOCK, 5), 1, 0x0089, 17);
            settle(r);
            eq(1L, r.dev.streamParseErrors());
            later(() -> r.fake.stream(L4_BLOCK, 2));
            eq(624, r.dev.getMeasurement(Duration.ofSeconds(1)).distanceMm());
            eq(0, r.dev.bridgeInfo().addrWidth()); // requests still answered
        }
    }

    public static void aStreamReportShorterThanItsHeaderIsNotAMeasurement() throws Exception {
        try (Rig r = ready()) {
            List<Vl53lxMeasurement> seen = new ArrayList<>();
            r.dev.onMeasurement(seen::add);
            r.dev.startRanging();
            r.fake.send(Vl53lx.Vl53lxRpt.STREAM.value, new byte[11]);
            settle(r);
            eq(0, seen.size());
            eq(0L, r.dev.streamParseErrors());
        }
    }

    public static void aStreamReportBeforeInitIsSwallowedNotDecoded() throws Exception {
        try (Rig r = connect()) {
            List<Vl53lxMeasurement> seen = new ArrayList<>();
            r.dev.onMeasurement(seen::add);
            r.fake.stream(L4_BLOCK, 1);
            settle(r);
            eq(0, seen.size());
            eq(0L, r.dev.streamParseErrors());
        }
    }

    public static void measurementsIsBoundedDropOldestAndDeregistersOnClose() throws Exception {
        try (Rig r = ready()) {
            Subscription<Vl53lxMeasurement> it = r.dev.measurements(1);
            eq(List.of(0L), r.dev.streamDroppedCounts());
            r.dev.startRanging();
            r.fake.stream(L4_BLOCK, 1);
            r.fake.stream(L4_BLOCK, 2);
            r.fake.stream(L4_BLOCK_F9, 3);
            settle(r);
            eq(List.of(2L), r.dev.streamDroppedCounts());
            eq(3L, it.take(Duration.ofSeconds(1)).timestampUs());
            it.close();
            eq(List.of(), r.dev.streamDroppedCounts());
        }
    }

    public static void getMeasurementTimesOutAndLeavesNoQueue() throws Exception {
        try (Rig r = ready()) {
            throwsType(DepzTimeoutException.class, () -> r.dev.getMeasurement(Duration.ofMillis(20)));
            eq(List.of(), r.dev.streamDroppedCounts());
        }
    }

    public static void getMeasurementEndsWhenTheDeviceCloses() throws Exception {
        try (Rig r = ready()) {
            later(r.dev::close);
            throwsType(LinkClosedException.class, () -> r.dev.getMeasurement(Duration.ofSeconds(2)));
            throwsType(LinkClosedException.class, () -> r.dev.getMeasurement(Duration.ofSeconds(2)));
        }
    }

    public static void configurationIsRefusedWhileTheStreamOwnsTheBus() throws Exception {
        try (Rig r = ready()) {
            r.dev.startRanging();
            r.fake.clearRx();
            List<T.Body> calls = List.of(r.dev::init, r.dev::configure, () -> r.dev.setOffsetMm(1), () -> r.dev.setXtalkKcps(1),
                    () -> r.dev.calibrateOffset(100), () -> r.dev.calibrateXtalk(100), () -> r.dev.setDetectionThresholds(1, 2, 3),
                    () -> r.dev.setSignalThresholdKcps(1), () -> r.dev.setSigmaThresholdMm(1), () -> r.dev.setRoi(4, 4),
                    () -> r.dev.setRoiCenter(1), r.dev::startTemperatureUpdate, r.dev::performRefSpadManagement,
                    r.dev::startRanging, r.dev::measureOnce);
            for (T.Body call : calls) {
                InvalidStateException e = throwsType(InvalidStateException.class, call);
                check(e.getMessage().contains("stopRanging() first"), e.getMessage());
            }
            eq(0, r.fake.rxCount());
            r.dev.bridgeInfo(); // touches no sensor register: allowed while streaming
        }
    }

    public static void stopRangingStopsTheStreamThenTheSensor() throws Exception {
        try (Rig r = ready()) {
            r.dev.stopRanging(); // not ranging: no-op
            eq(0, r.fake.rxCount());
            r.dev.startRanging();
            r.fake.clearRx();
            r.dev.stopRanging();
            eq(List.of(STOP_STREAM, WRITE_REG), r.fake.cmds());
            eq(List.of(key(0x0087, 0x80)), r.fake.writeKeys());
            check(!r.dev.ranging(), "stopped");
        }
    }

    public static void aFailedStopStreamStillStopsTheSensorAndClearsTheFlag() throws Exception {
        try (Rig r = ready()) {
            r.dev.startRanging();
            r.fake.clearRx();
            r.fake.failNext(STOP_STREAM, 0x01);
            throwsType(StatusException.class, r.dev::stopRanging);
            check(!r.dev.ranging(), "not ranging");
            eq(List.of(key(0x0087, 0x80)), r.fake.writeKeys());
            r.dev.setOffsetMm(0); // configuration allowed again
        }
    }

    public static void aRefusedStartStreamLeavesTheDeviceNotRanging() throws Exception {
        try (Rig r = ready()) {
            r.fake.failNext(START_STREAM, 0x03);
            throwsType(StatusException.class, r.dev::startRanging);
            check(!r.dev.ranging(), "not ranging");
        }
    }

    // ── measureOnce (poll) ───────────────────────────────────────────────────

    public static void measureOnceStartsWaitsReadsReleasesStops() throws Exception {
        try (Rig r = ready()) {
            r.fake.setResultBlock(L4_BLOCK);
            Vl53lxMeasurement m = r.dev.measureOnce();
            eq(List.of(624, true), List.of(m.distanceMm(), m.valid()));
            eq(null, m.bins());
            // the timestamp of the last register read: the block itself
            eq(r.fake.mcuTimeUs - 100, m.timestampUs());
            check(hexes(r.fake.sent(READ_REG)).contains("89001100"), "the block read");
            List<String> w = r.fake.writeKeys();
            eq(key(0x0087, 0x21), w.get(0));
            eq(List.of(key(0x0086, 0x01), key(0x0087, 0x80)), w.subList(w.size() - 2, w.size()));
            eq(List.of(), r.fake.sent(START_STREAM));
        }
    }

    public static void aDataReadyTimeoutStillStopsTheSensor() throws Exception {
        try (Rig r = ready()) {
            r.fake.regs[0x0031] = 0x01; // INT never reaches the active level
            Vl53lxException e = throwsType(Vl53lxException.class, () -> r.dev.measureOnce(Duration.ofMillis(30)));
            check(e.getMessage().contains("timeout waiting for data ready"), e.getMessage());
            List<String> w = r.fake.writeKeys();
            eq(key(0x0087, 0x80), w.get(w.size() - 1));
            check(!r.dev.ranging(), "not ranging");
        }
    }

    public static void everyProductClassFixesItsProductAndOpensOnAPlainLink() throws Exception {
        Object[][] cases = {
            {(Function<FakeVl53lxBridge, Vl53lxDevice>) f -> new Vl53l0xDevice(f.link), (Function<FakeVl53lxBridge, Vl53lxDevice>) f -> new Vl53l0xDevice(f.link, TIMEOUT), List.of("uld")},
            {(Function<FakeVl53lxBridge, Vl53lxDevice>) f -> new Vl53l1cxDevice(f.link), (Function<FakeVl53lxBridge, Vl53lxDevice>) f -> new Vl53l1cxDevice(f.link, TIMEOUT), List.of("uld", "histogram")},
            {(Function<FakeVl53lxBridge, Vl53lxDevice>) f -> new Vl53l1cbDevice(f.link), (Function<FakeVl53lxBridge, Vl53lxDevice>) f -> new Vl53l1cbDevice(f.link, TIMEOUT), List.of("uld", "histogram")},
            {(Function<FakeVl53lxBridge, Vl53lxDevice>) f -> new Vl53l3cxDevice(f.link), (Function<FakeVl53lxBridge, Vl53lxDevice>) f -> new Vl53l3cxDevice(f.link, TIMEOUT), List.of("ulp", "histogram")},
            {(Function<FakeVl53lxBridge, Vl53lxDevice>) f -> new Vl53l4cxDevice(f.link), (Function<FakeVl53lxBridge, Vl53lxDevice>) f -> new Vl53l4cxDevice(f.link, TIMEOUT), List.of("histogram")},
            {(Function<FakeVl53lxBridge, Vl53lxDevice>) f -> new Vl53lxDevice(f.link), (Function<FakeVl53lxBridge, Vl53lxDevice>) f -> new Vl53lxDevice(f.link, TIMEOUT), List.of("uld", "histogram")},
        };
        for (Object[] c : cases) {
            for (int i = 0; i < 2; i++) {
                @SuppressWarnings("unchecked")
                Function<FakeVl53lxBridge, Vl53lxDevice> make = (Function<FakeVl53lxBridge, Vl53lxDevice>) c[i];
                try (Rig r = connect(L4CD_BOARD, make, false)) { // an unrelated board name: the class's product wins
                    eq(c[2], r.dev.driverKinds(), r.dev.getClass().getSimpleName());
                    eq(i == 0 ? ai.depz.sensor.devices.DepzDevice.DEFAULT_TIMEOUT : TIMEOUT, r.dev.timeout());
                    check(!r.dev.initialized(), "not initialized");
                }
            }
        }
        // the L1CB runs the L1X ULD and the L3CX the ULP against the fake die, as their siblings do
        try (Rig r = connect(UNSTAMPED, f -> new Vl53l1cbDevice(f.link, TIMEOUT), false)) {
            r.dev.init();
            eq(List.of("VL53L1CB", "uld"), List.of(r.dev.product(), r.dev.driverKind()));
            eq(null, r.dev.caveat());
            eq(8000, r.dev.identifyProduct().get("reach_mm"));
        }
        try (Rig r = connect(UNSTAMPED, f -> new Vl53l3cxDevice(f.link, TIMEOUT), false)) {
            r.dev.init();
            eq(List.of("VL53L3CX", "ulp"), List.of(r.dev.product(), r.dev.driverKind()));
            check(r.dev.caveat().startsWith("single-target"), r.dev.caveat());
        }
    }

    public static void theShortFormsAre50msContinuousAndTheDriversDefaultSampleCount() throws Exception {
        try (Rig r = ready()) {
            r.dev.configure(); // 50 ms, continuous
            eq(0, r.dev.getRangeTiming().interMeasurementMs());
            check(Math.abs(r.dev.getRangeTiming().timingBudgetMs() - 50) <= 1, "50 ms");
            r.fake.clearRx();
            r.fake.setResultBlock(L4_BLOCK); // 624 mm
            eq(-24, r.dev.calibrateOffset(600));
            eq(30, (int) r.fake.writes().stream().filter(x -> (int) x[0] == 0x0086).count()); // 10 heat frames + the default 20
            r.fake.clearRx();
            eq(1, r.dev.calibrateXtalk(650));
            eq(30, (int) r.fake.writes().stream().filter(x -> (int) x[0] == 0x0086).count());
        }
    }

    // ── the histogram driver on the same bridge ──────────────────────────────

    public static void theHistogramPairStreamsThe83ByteBlock() throws Exception {
        // VL53L4CX has only the histogram driver (ST's Bare Driver); its init needs the die's NVM and
        // the fake has a blank one, so the pair is bound through the table and refused where it must be.
        try (Rig r = connect(L4CX_BOARD, f -> new Vl53l4cxDevice(f.link, Duration.ofMillis(60)), false)) {
            eq(List.of("histogram"), r.dev.driverKinds());
            r.fake.failNext(READ_REG, SILENT);
            throwsType(Vl53lxProtocolException.class, r.dev::init);
            check(!r.dev.initialized(), "not initialized");
            eq("02", hexes(r.fake.sent(SET_ADDR_WIDTH)).get(0)); // the device's, before the first register access
        }
    }
}
