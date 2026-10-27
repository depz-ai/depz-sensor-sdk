package ai.depz.sensor.test.devices;

import static ai.depz.sensor.test.devices.T.check;
import static ai.depz.sensor.test.devices.T.eq;
import static ai.depz.sensor.test.devices.T.throwsType;

import ai.depz.sensor.devices.DepzException;
import ai.depz.sensor.devices.DepzTimeoutException;
import ai.depz.sensor.devices.InvalidStateException;
import ai.depz.sensor.devices.LinkClosedException;
import ai.depz.sensor.devices.StatusException;
import ai.depz.sensor.devices.Subscription;
import ai.depz.sensor.devices.Vl53l4Measurement;
import ai.depz.sensor.devices.Vl53l4cdDevice;
import ai.depz.sensor.protocol.Vl53l4;
import ai.depz.sensor.sensors.vl53l4.DetectionThresholds;
import ai.depz.sensor.sensors.vl53l4.RangeTiming;
import ai.depz.sensor.sensors.vl53l4.Vl53l4Uld;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.time.Duration;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

/**
 * {@code Vl53l4cdDevice} (contract 10 register bridge) over the in-process bridge {@link FakeVl53l4Bridge}
 * — the Python tests' test_vl53l4cd_device.py: init over the wire, the XSHUT power states, bridge
 * diagnostics, bus re-timing, every configuration getter/setter as register round trips, both
 * calibrations, the INT-driven stream (callbacks, bounded queue, getMeasurement timeouts and close),
 * poll-mode measureOnce, chunked register transfers and the bridge error statuses.
 */
public final class Vl53l4cdDeviceTests {
    private Vl53l4cdDeviceTests() {}

    private static final Duration TIMEOUT = Duration.ofMillis(500);

    private record Rig(Vl53l4cdDevice dev, FakeVl53l4Bridge fake) implements AutoCloseable {
        @Override
        public void close() {
            dev.close();
        }
    }

    private static Rig connect(Duration timeout) {
        FakeVl53l4Bridge fake = new FakeVl53l4Bridge();
        return new Rig(new Vl53l4cdDevice(fake.link, timeout), fake);
    }

    /** Initialised; the fake's logs start after init. */
    private static Rig ready() {
        Rig r = connect(TIMEOUT);
        r.dev.init();
        r.fake.clearRx();
        r.fake.platform.writes.clear();
        return r;
    }

    private static byte[] be16(int v) {
        return new byte[] {(byte) (v >> 8), (byte) v};
    }

    private static byte[] le(int... u16) {
        ByteBuffer b = ByteBuffer.allocate(2 * u16.length).order(ByteOrder.LITTLE_ENDIAN);
        for (int v : u16) {
            b.putShort((short) v);
        }
        return b.array();
    }

    private static byte[] lastWrite(FakeVl53l4Bridge f, int addr) {
        return f.platform.last(addr);
    }

    // ── init / identity ──────────────────────────────────────────────────────

    public static void initRunsTheUldBootSequenceOverTheBridge() {
        try (Rig r = connect(TIMEOUT)) {
            check(!r.dev.initialized(), "not yet");
            r.dev.init();
            check(r.dev.initialized() && !r.dev.ranging(), "initialised, idle");
            // config block at the boot speed, then the bridge is re-timed to 1 MHz
            eq(List.of(Vl53l4Uld.I2C_KHZ_BOOT, Vl53l4Uld.I2C_KHZ_DEFAULT), r.fake.platform.speeds);
            List<byte[]> blocks = r.fake.platform.writtenTo(Vl53l4Uld.CONFIG_ADDR);
            eq(1, blocks.size());
            eq(Vl53l4Uld.configBlock(), blocks.get(0));
            List<Integer> cmds = r.fake.cmds();
            int firstSpeed = cmds.indexOf(FakeVl53l4Bridge.SET_I2C_SPEED);
            int lastSpeed = cmds.lastIndexOf(FakeVl53l4Bridge.SET_I2C_SPEED);
            int configWrite = cmds.indexOf(FakeVl53l4Bridge.WRITE_REG);
            check(firstSpeed < configWrite && configWrite < lastSpeed, "speed, config, speed: " + cmds);
            // init's 50 ms continuous default reads back as 49 (ST's integer math)
            eq(new RangeTiming(49, 0), r.dev.getRangeTiming());
            check(r.dev.uld() != null, "uld");
        }
    }

    public static void initAtAChosenBusSpeed() {
        try (Rig r = connect(TIMEOUT)) {
            r.dev.init(400);
            eq(List.of(400), r.fake.platform.speeds);
            eq(400, r.dev.bridgeInfo().i2cKhz());
        }
    }

    public static void isAliveChecksTheModelIdAndSwallowsBridgeErrors() {
        try (Rig r = connect(TIMEOUT)) {
            check(r.dev.isAlive(), "alive");
            r.fake.platform.seedWord(Vl53l4Uld.IDENTIFICATION_MODEL_ID, 0xEACC);
            check(!r.dev.isAlive(), "a foreign model id");
            r.fake.platform.seedWord(Vl53l4Uld.IDENTIFICATION_MODEL_ID, Vl53l4Uld.MODEL_ID);
            r.fake.failNext(FakeVl53l4Bridge.READ_REG, FakeVl53l4Bridge.ERR_HARDWARE_FAULT);
            check(!r.dev.isAlive(), "a bridge error is not alive");
            check(r.dev.isAlive(), "and back");
        }
    }

    // ── XSHUT ────────────────────────────────────────────────────────────────

    public static void xshutOffPowersTheSensorDownAndForgetsTheInit() {
        try (Rig r = ready()) {
            r.dev.startRanging();
            r.dev.xshut(Vl53l4.XSHUT_OFF);
            Object[] last = r.fake.lastRx();
            eq(FakeVl53l4Bridge.XSHUT, last[0]);
            eq(new byte[] {(byte) Vl53l4.XSHUT_OFF}, last[1]);
            check(!r.fake.powered && !r.fake.streaming, "powered down, stream stopped");
            check(!r.dev.initialized() && !r.dev.ranging(), "init forgotten");
            // every register access now answers ERR_HARDWARE_FAULT (contract 10 §2)
            check(!r.dev.isAlive(), "not alive");
            throwsType(StatusException.class, r.dev::getOffsetMm);
            throwsType(StatusException.class, () -> r.dev.init());
            Vl53l4.Vl53l4Info info = r.dev.bridgeInfo();
            eq(List.of(0, 0, 0), List.of(info.xshutLevel(), info.initialized(), info.modelId()));
            check(info.i2cErrors() >= 3 && info.lastI2cError() == 1, "i2c errors counted: " + info);
            r.dev.xshut(Vl53l4.XSHUT_ON);
            check(r.fake.powered && r.dev.isAlive(), "back on");
        }
    }

    public static void anXshutActionOutOfRangeIsRefusedBeforeTheWire() {
        try (Rig r = ready()) {
            throwsType(IllegalArgumentException.class, () -> r.dev.xshut(3));
            throwsType(IllegalArgumentException.class, () -> r.dev.xshut(-1));
            eq(0, r.fake.cmds().size());
        }
    }

    public static void resetSensorWipesTheConfigurationAndNeedsInitAgain() {
        try (Rig r = ready()) {
            r.dev.setOffsetMm(7);
            r.dev.resetSensor();
            Object[] last = r.fake.lastRx();
            eq(FakeVl53l4Bridge.XSHUT, last[0]);
            eq(new byte[] {(byte) Vl53l4.XSHUT_RESET}, last[1]);
            check(!r.dev.initialized(), "init forgotten");
            eq(0, r.dev.getOffsetMm()); // back to the boot register file
            eq(0, r.fake.platform.writtenTo(Vl53l4Uld.CONFIG_ADDR).size());
            r.dev.init();
            check(r.dev.initialized(), "again");
            eq(1, r.fake.platform.writtenTo(Vl53l4Uld.CONFIG_ADDR).size());
        }
    }

    // ── bridge diagnostics / bus speed ───────────────────────────────────────

    public static void bridgeInfoDecodesThe21ByteReport() {
        try (Rig r = ready()) {
            Vl53l4.Vl53l4Info info = r.dev.bridgeInfo();
            Object[] last = r.fake.lastRx();
            eq(FakeVl53l4Bridge.GET_INFO, last[0]);
            eq(new byte[0], last[1]);
            eq(List.of(Vl53l4Uld.MODEL_ID, 0x03, 1), List.of(info.modelId(), info.fwStatus(), info.initialized()));
            eq(List.of(1, Vl53l4Uld.I2C_KHZ_DEFAULT), List.of(info.xshutLevel(), info.i2cKhz()));
            eq(0L, info.intEdges());
            // counters move with the INT edges; GET_INFO is safe while streaming
            r.dev.startRanging();
            r.fake.sendResult(FakeVl53l4Bridge.resultBlock(100));
            r.fake.sendResult(FakeVl53l4Bridge.resultBlock(101));
            eq(2L, r.dev.bridgeInfo().intEdges());
        }
    }

    public static void setI2cSpeedKhzRetimesTheBridge() {
        try (Rig r = ready()) {
            r.dev.setI2cSpeedKhz(400);
            Object[] last = r.fake.lastRx();
            eq(FakeVl53l4Bridge.SET_I2C_SPEED, last[0]);
            eq(le(400), last[1]);
            eq(400, r.dev.bridgeInfo().i2cKhz());
            throwsType(IllegalArgumentException.class, () -> r.dev.setI2cSpeedKhz(-1));
            throwsType(IllegalArgumentException.class, () -> r.dev.setI2cSpeedKhz(0x10000));
        }
    }

    // ── configuration round trips (init first; not while ranging) ────────────

    public static void everyConfigurationMethodRoundTripsThroughTheRegisters() {
        try (Rig r = ready()) {
            r.dev.setRangeTiming(100, 250);
            int[] regs = Vl53l4Uld.rangeTimingRegisters(100, 250, Vl53l4cdDriverTests.OSC_FREQUENCY_TYPICAL,
                    Vl53l4cdDriverTests.CLOCK_PLL_TYPICAL);
            eq(regs[0], r.dev.uld().readWord(Vl53l4Uld.RANGE_CONFIG_A));
            eq(Integer.toUnsignedLong(regs[2]), r.dev.uld().readDword(Vl53l4Uld.INTERMEASUREMENT_MS));
            int[] decoded = Vl53l4Uld.decodeRangeTiming(Integer.toUnsignedLong(regs[2]),
                    Vl53l4cdDriverTests.CLOCK_PLL_TYPICAL, Vl53l4cdDriverTests.OSC_FREQUENCY_TYPICAL, regs[0]);
            eq(new RangeTiming(decoded[0], decoded[1]), r.dev.getRangeTiming());
            int inter = r.dev.getRangeTiming().interMeasurementMs();
            check(240 <= inter && inter < 250, "the 1.055 / 1.065 factor pair reads short: " + inter);
            r.dev.setRangeTiming(33);
            eq(0, r.dev.getRangeTiming().interMeasurementMs()); // continuous again
            r.dev.setOffsetMm(-12);
            eq(-12, r.dev.getOffsetMm());
            eq(be16(-48 & 0xFFFF), lastWrite(r.fake, Vl53l4Uld.RANGE_OFFSET_MM));
            r.dev.setXtalkKcps(20);
            eq(20, r.dev.getXtalkKcps());
            eq(be16(20 << 9), lastWrite(r.fake, Vl53l4Uld.XTALK_PLANE_OFFSET_KCPS));
            r.dev.setDetectionThresholds(100, 300, Vl53l4Uld.WINDOW_OUT);
            eq(new DetectionThresholds(100, 300, Vl53l4Uld.WINDOW_OUT), r.dev.getDetectionThresholds());
            eq(new byte[] {(byte) Vl53l4Uld.WINDOW_OUT}, lastWrite(r.fake, Vl53l4Uld.SYSTEM_INTERRUPT));
            r.dev.setSignalThresholdKcps(1024);
            eq(1024, r.dev.getSignalThresholdKcps());
            eq(be16(1024 >> 3), lastWrite(r.fake, Vl53l4Uld.MIN_COUNT_RATE_RTN_LIMIT_MCPS));
            r.dev.setSigmaThresholdMm(15);
            eq(15, r.dev.getSigmaThresholdMm());
            eq(be16(15 << 2), lastWrite(r.fake, Vl53l4Uld.RANGE_CONFIG_SIGMA_THRESH));
        }
    }

    public static void argumentErrorsPassThroughUnchanged() {
        // ERRATA A16: a refused argument is invalid_argument (IllegalArgumentException), not the
        // driver's Vl53l4Error
        try (Rig r = ready()) {
            throwsType(IllegalArgumentException.class, () -> r.dev.setRangeTiming(50, 30));
            throwsType(IllegalArgumentException.class, () -> r.dev.setRangeTiming(9, 0));
            eq(0, r.fake.cmds().size(), "range timing refuses before any traffic, reads included");
            throwsType(IllegalArgumentException.class, () -> r.dev.setOffsetMm(5000));
            throwsType(IllegalArgumentException.class, () -> r.dev.setXtalkKcps(128));
            throwsType(IllegalArgumentException.class, () -> r.dev.setSigmaThresholdMm(16384));
            // the refused value never reaches its register
            for (int reg : new int[] {Vl53l4Uld.RANGE_CONFIG_A, Vl53l4Uld.INTERMEASUREMENT_MS, Vl53l4Uld.RANGE_OFFSET_MM,
                    Vl53l4Uld.XTALK_PLANE_OFFSET_KCPS, Vl53l4Uld.RANGE_CONFIG_SIGMA_THRESH}) {
                eq(0, r.fake.platform.writtenTo(reg).size(), String.format("0x%04X", reg));
            }
            r.fake.clearRx();
            throwsType(IllegalArgumentException.class, () -> r.dev.calibrateOffset(5));
            throwsType(IllegalArgumentException.class, () -> r.dev.calibrateXtalk(100, 4));
            eq(0, r.fake.cmds().size(), "calibrations refuse before any traffic");
        }
    }

    public static void startTemperatureUpdateRunsTheVhvBurst() {
        try (Rig r = ready()) {
            r.dev.startTemperatureUpdate();
            List<byte[]> loop = r.fake.platform.writtenTo(Vl53l4Uld.VHV_CONFIG_TIMEOUT_MACROP_LOOP_BOUND);
            eq(2, loop.size());
            eq(new byte[] {(byte) 0x81}, loop.get(0));
            eq(new byte[] {0x09}, loop.get(1));
            List<Integer> starts = new ArrayList<>();
            for (byte[] d : r.fake.platform.writtenTo(Vl53l4Uld.SYSTEM_START)) {
                starts.add(d[0] & 0xFF);
            }
            eq(List.of(0x21, 0x80), starts);
            check(!r.dev.ranging(), "not ranging");
        }
    }

    public static void calibrationsRunTheirSampleBurstsOverTheBridge() {
        try (Rig r = ready()) {
            r.fake.setResultBlock(FakeVl53l4Bridge.resultBlock(120, 9, 3, 1200, 2048, 1));
            eq(30, r.dev.calibrateOffset(150, 5));
            eq(30, r.dev.getOffsetMm());
            eq(75, r.dev.calibrateXtalk(240, 5)); // (1 − 120/240) · 1200/8
            eq(75, r.dev.getXtalkKcps());
            check(!r.dev.ranging(), "not ranging");
            // each burst: start, (ready, result, clear) × n, stop — all over READ/WRITE_REG
            eq(List.of(FakeVl53l4Bridge.READ_REG, FakeVl53l4Bridge.WRITE_REG),
                    r.fake.cmds().stream().distinct().sorted().toList());
        }
    }

    public static void configurationCalibrationsAndTemperatureUpdateRefuseWhileRanging() {
        try (Rig r = ready()) {
            r.dev.startRanging();
            r.fake.clearRx();
            List<T.Body> calls = List.of(r.dev::startTemperatureUpdate, () -> r.dev.calibrateOffset(100),
                    () -> r.dev.calibrateXtalk(100), () -> r.dev.init(), () -> r.dev.setRangeTiming(50),
                    () -> r.dev.setXtalkKcps(1), () -> r.dev.setDetectionThresholds(1, 2, 0),
                    () -> r.dev.setSignalThresholdKcps(1), () -> r.dev.setSigmaThresholdMm(1),
                    () -> r.dev.setI2cSpeedKhz(400), () -> r.dev.measureOnce());
            for (T.Body c : calls) {
                throwsType(InvalidStateException.class, c);
            }
            eq(0, r.fake.cmds().size());
        }
    }

    // ── ranging: the INT-driven stream ───────────────────────────────────────

    public static void startRangingStartsTheSensorThenArmsTheStream() {
        try (Rig r = ready()) {
            r.dev.startRanging();
            check(r.dev.ranging() && r.fake.streaming, "ranging");
            eq(new byte[] {0x21}, lastWrite(r.fake, Vl53l4Uld.SYSTEM_START));
            Object[] last = r.fake.lastRx();
            eq(FakeVl53l4Bridge.START_STREAM, last[0]);
            eq(new byte[] {(byte) 0x89, 0, 17, 0, 0}, last[1]);
            eq(List.of(Vl53l4Uld.RESULT_BLOCK_ADDR, Vl53l4Uld.RESULT_BLOCK_LEN), List.of(r.fake.streamAddr, r.fake.streamLen));
            throwsType(InvalidStateException.class, r.dev::startRanging); // already ranging
            r.dev.stopRanging();
            check(!r.dev.ranging() && !r.fake.streaming, "stopped");
            // the bridge stream is released first, then the sensor is told to stop
            List<Integer> cmds = r.fake.cmds();
            eq(List.of(FakeVl53l4Bridge.STOP_STREAM, FakeVl53l4Bridge.WRITE_REG), cmds.subList(cmds.size() - 2, cmds.size()));
            eq(new byte[] {(byte) 0x80}, lastWrite(r.fake, Vl53l4Uld.SYSTEM_START));
        }
    }

    public static void stopRangingClearsTheStateEvenWhenTheBridgeRefuses() {
        try (Rig r = ready()) {
            r.dev.startRanging();
            r.fake.failNext(FakeVl53l4Bridge.STOP_STREAM, FakeVl53l4Bridge.ERR_HARDWARE_FAULT);
            throwsType(StatusException.class, r.dev::stopRanging);
            check(!r.dev.ranging(), "not ranging");
            // the sensor itself was still told to stop, so reconfiguration is possible
            eq(new byte[] {(byte) 0x80}, lastWrite(r.fake, Vl53l4Uld.SYSTEM_START));
            r.dev.setOffsetMm(3);
            eq(3, r.dev.getOffsetMm());
        }
    }

    public static void streamedBlocksReachCallbacksAndTheMeasurementsQueue() throws Exception {
        try (Rig r = ready()) {
            List<Vl53l4Measurement> got = Collections.synchronizedList(new ArrayList<>());
            AutoCloseable unsubscribe = r.dev.onMeasurement(got::add);
            try (Subscription<Vl53l4Measurement> it = r.dev.measurements(4)) {
                r.dev.startRanging();
                r.fake.sendResult(FakeVl53l4Bridge.resultBlock(291, 9, 3, 1600, 2048, 7));
                Vl53l4Measurement m = it.take(Duration.ofSeconds(2));
                eq(r.fake.mcuTimeUs, m.timestampUs());
                eq(List.of(291, 3, 1600, 8), List.of(m.distanceMm(), m.sigmaMm(), m.signalRateKcps(), m.numberOfSpad()));
                eq(7, m.streamCount());
                check(m.valid(), "valid");
                eq("valid", m.statusText());
                check(T.until(() -> got.size() == 1, 2000), "callback");
                eq(m, got.get(0));
                unsubscribe.close();
                r.fake.sendResult(FakeVl53l4Bridge.resultBlock(77, 4, 3, 1200, 2048, 1));
                Vl53l4Measurement m2 = it.take(Duration.ofSeconds(2));
                eq(List.of(77, 2), List.of(m2.distanceMm(), m2.rangeStatus()));
                check(!m2.valid(), "invalid");
                eq("signal below threshold", m2.statusText());
                eq(1, got.size(), "unsubscribed");
            }
        }
    }

    public static void theMeasurementsQueueIsBoundedDropOldestAndEndsOnClose() {
        Rig r = ready();
        Subscription<Vl53l4Measurement> it = r.dev.measurements(1);
        r.dev.startRanging();
        for (int d : new int[] {10, 20, 30}) {
            r.fake.sendResult(FakeVl53l4Bridge.resultBlock(d));
        }
        r.dev.bridgeInfo(); // a round trip queued behind the reports: they are all handled
        eq(2L, it.droppedCount());
        eq(List.of(2L), r.dev.streamDroppedCounts());
        eq(30, it.take(Duration.ofSeconds(1)).distanceMm());
        r.dev.close();
        check(it.isClosed(), "closed with the device");
        throwsType(ai.depz.sensor.devices.DeviceLostException.class, () -> it.take(Duration.ofSeconds(1)));
    }

    public static void getMeasurementTimesOutWhenNothingStreams() {
        try (Rig r = ready()) {
            r.dev.startRanging();
            long t0 = System.nanoTime();
            throwsType(DepzTimeoutException.class, () -> r.dev.getMeasurement(Duration.ofMillis(150)));
            double s = (System.nanoTime() - t0) / 1e9;
            check(s >= 0.1 && s < 1.0, "waited " + s + " s");
            eq(List.of(), r.dev.streamDroppedCounts(), "the one-shot queue is gone");
        }
    }

    public static void getMeasurementEndsWithLinkClosedOnAClosedDevice() throws Exception {
        Rig r = ready();
        r.dev.startRanging();
        // close once the waiter has subscribed (its one-shot queue appears)
        Thread closer = new Thread(() -> {
            T.until(() -> r.dev.streamDroppedCounts().size() == 1, 5000);
            r.dev.close();
        });
        closer.start();
        LinkClosedException e = throwsType(LinkClosedException.class, () -> r.dev.getMeasurement(Duration.ofSeconds(5)));
        check(e.getMessage().contains("while waiting"), e.getMessage());
        closer.join(5000);
        e = throwsType(LinkClosedException.class, () -> r.dev.getMeasurement(Duration.ofSeconds(5)));
        check(e.getMessage().contains("is closed"), e.getMessage());
    }

    public static void undecodableStreamBlocksAreCountedNotFatal() {
        try (Rig r = ready(); Subscription<Vl53l4Measurement> it = r.dev.measurements(4)) {
            r.dev.startRanging();
            // somebody re-armed the stream on a different block: 4 bytes where 17 decode
            r.fake.sendStream(new byte[4], Vl53l4Uld.RESULT_BLOCK_ADDR, 4);
            r.fake.sendResult(FakeVl53l4Bridge.resultBlock(624));
            eq(624, it.take(Duration.ofSeconds(2)).distanceMm());
            eq(1L, r.dev.streamParseErrors());
        }
    }

    // ── poll mode ────────────────────────────────────────────────────────────

    public static void measureOncePollsOneResultAndStops() {
        try (Rig r = ready()) {
            r.fake.setResultBlock(FakeVl53l4Bridge.resultBlock(333, 9, 3, 1200, 2048, 5));
            Vl53l4Measurement m = r.dev.measureOnce();
            eq(List.of(333, 5), List.of(m.distanceMm(), m.streamCount()));
            check(m.valid(), "valid");
            eq(r.fake.mcuTimeUs - 100, m.timestampUs()); // the block read's MCU stamp
            List<Integer> starts = new ArrayList<>();
            for (byte[] d : r.fake.platform.writtenTo(Vl53l4Uld.SYSTEM_START)) {
                starts.add(d[0] & 0xFF);
            }
            eq(List.of(0x21, 0x80), starts);
            List<byte[]> clears = r.fake.platform.writtenTo(Vl53l4Uld.SYSTEM_INTERRUPT_CLEAR);
            eq(1, clears.size());
            eq(new byte[] {0x01}, clears.get(0));
            check(!r.fake.cmds().contains(FakeVl53l4Bridge.START_STREAM) && !r.dev.ranging(), "no stream");
        }
    }

    public static void measureOnceTimesOutAndStillStopsTheSensor() {
        try (Rig r = ready()) {
            r.fake.platform.reg.put(Vl53l4Uld.GPIO_TIO_HV_STATUS, (byte) 0x01); // INT never asserted
            Vl53l4Uld.Vl53l4Error e = throwsType(Vl53l4Uld.Vl53l4Error.class, () -> r.dev.measureOnce(Duration.ofMillis(10)));
            check(e.getMessage().contains("data ready"), e.getMessage());
            eq(new byte[] {(byte) 0x80}, lastWrite(r.fake, Vl53l4Uld.SYSTEM_START));
        }
    }

    // ── register transfers and bridge errors ─────────────────────────────────

    public static void registerTransfersPast253BytesGoInChunks() {
        try (Rig r = ready()) {
            byte[] data = new byte[300];
            for (int i = 0; i < 256; i++) {
                data[i] = (byte) i;
            }
            // the ULD never moves more than a block at once: reach the chunking through the bridge
            // platform the device hands the ULD (Python: dev.uld.p)
            bridgeWrite(r.dev, 0x0100, data);
            List<byte[]> writes = r.fake.sent(FakeVl53l4Bridge.WRITE_REG);
            eq(2, writes.size());
            eq(List.of(0x0100, 0x0100 + Vl53l4.XFER_MAX), List.of(u16(writes.get(0)), u16(writes.get(1))));
            eq(List.of(Vl53l4.XFER_MAX, 300 - Vl53l4.XFER_MAX), List.of(writes.get(0).length - 2, writes.get(1).length - 2));
            r.fake.clearRx();
            eq(data, bridgeRead(r.dev, 0x0100, 300));
            List<byte[]> reads = r.fake.sent(FakeVl53l4Bridge.READ_REG);
            eq(le(0x0100, Vl53l4.XFER_MAX), reads.get(0));
            eq(le(0x0100 + Vl53l4.XFER_MAX, 300 - Vl53l4.XFER_MAX), reads.get(1));
        }
    }

    private static int u16(byte[] p) {
        return (p[0] & 0xFF) | (p[1] & 0xFF) << 8;
    }

    // The device's bridge platform is private; the ULD reaches it as its platform field.
    private static ai.depz.sensor.sensors.vl53l4.Vl53l4Platform platformOf(Vl53l4cdDevice dev) {
        try {
            java.lang.reflect.Field f = ai.depz.sensor.sensors.vl53l4.Vl53l4cdDriver.class.getDeclaredField("p");
            f.setAccessible(true);
            return (ai.depz.sensor.sensors.vl53l4.Vl53l4Platform) f.get(dev.uld());
        } catch (ReflectiveOperationException e) {
            throw new LinkageError(e.getMessage(), e);
        }
    }

    private static void bridgeWrite(Vl53l4cdDevice dev, int addr, byte[] data) {
        platformOf(dev).writeMulti(addr, data);
    }

    private static byte[] bridgeRead(Vl53l4cdDevice dev, int addr, int n) {
        return platformOf(dev).readMulti(addr, n);
    }

    public static void aShortRegisterReadIsAnError() {
        try (Rig r = ready()) {
            r.fake.shortReadNext = true;
            DepzException e = throwsType(DepzException.class, r.dev::getOffsetMm);
            check(e.getMessage().contains("expected 2, got 1"), e.getMessage());
        }
    }

    public static void bridgeErrorStatusesRaiseStatusException() {
        try (Rig r = ready()) {
            r.fake.failNext(FakeVl53l4Bridge.WRITE_REG, FakeVl53l4Bridge.ERR_HARDWARE_FAULT);
            throwsType(StatusException.class, () -> r.dev.setOffsetMm(1));
            r.fake.failNext(FakeVl53l4Bridge.READ_REG, FakeVl53l4Bridge.ERR_HARDWARE_FAULT);
            throwsType(StatusException.class, r.dev::getRangeTiming);
            r.fake.failNext(FakeVl53l4Bridge.XSHUT, FakeVl53l4Bridge.ERR_HARDWARE_FAULT); // RESET boot handshake timed out
            throwsType(StatusException.class, r.dev::resetSensor);
            r.fake.failNext(FakeVl53l4Bridge.START_STREAM, FakeVl53l4Bridge.ERR_HARDWARE_FAULT);
            throwsType(StatusException.class, r.dev::startRanging);
            check(!r.dev.ranging(), "not ranging");
        }
    }

    public static void noAnswerIsATimeout() {
        try (Rig r = connect(Duration.ofMillis(100))) {
            r.fake.failNext(FakeVl53l4Bridge.GET_INFO, FakeVl53l4Bridge.SILENT);
            throwsType(DepzTimeoutException.class, r.dev::bridgeInfo);
            r.fake.failNext(FakeVl53l4Bridge.START_STREAM, FakeVl53l4Bridge.SILENT);
            r.dev.init();
            throwsType(DepzTimeoutException.class, r.dev::startRanging);
            check(!r.dev.ranging(), "not ranging");
        }
    }
}
