package ai.depz.sensor.test.devices;

import static ai.depz.sensor.test.devices.FakeBno055.w;
import static ai.depz.sensor.test.devices.T.check;
import static ai.depz.sensor.test.devices.T.eq;
import static ai.depz.sensor.test.devices.T.throwsType;
import static ai.depz.sensor.test.devices.T.until;

import ai.depz.sensor.devices.Bno055Device;
import ai.depz.sensor.devices.Bno055Sample;
import ai.depz.sensor.devices.DepzException;
import ai.depz.sensor.devices.DepzTimeoutException;
import ai.depz.sensor.devices.InvalidStateException;
import ai.depz.sensor.devices.LinkClosedException;
import ai.depz.sensor.devices.StatusException;
import ai.depz.sensor.devices.Subscription;
import ai.depz.sensor.protocol.Bno055;
import ai.depz.sensor.sensors.bno055.Bno055Regs;
import ai.depz.sensor.sensors.bno055.Bno055Regs.AxisRemap;
import ai.depz.sensor.sensors.bno055.Bno055Regs.CalibStatus;
import ai.depz.sensor.sensors.bno055.Bno055Regs.CalibrationProfile;
import ai.depz.sensor.sensors.bno055.Bno055Regs.GyroConfig;
import ai.depz.sensor.sensors.bno055.Bno055Regs.MagConfig;
import ai.depz.sensor.sensors.bno055.Bno055Regs.OprMode;
import ai.depz.sensor.sensors.bno055.Bno055Regs.SystemStatus;
import ai.depz.sensor.sensors.bno055.Bno055Regs.TempSource;
import ai.depz.sensor.sensors.bno055.Bno055Regs.Units;
import java.time.Duration;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.HexFormat;
import java.util.List;

/**
 * {@code Bno055Device} over the register-level model {@link FakeBno055} — the Python tests'
 * test_bno055_device.py where the existing {@link Bno055DeviceTests} do not go: bridge info and
 * isAlive, raw and page-1 register access with its 128-byte split and error paths, axis remap,
 * temperature source, reset, system status, calibration status / profile / SIC matrix, gyro and mag
 * configs, interrupts, polled samples and quaternions, stream start / stop / callbacks / getSample,
 * malformed stream reports.
 */
public final class Bno055ModelTests {
    private Bno055ModelTests() {}

    private static final Units ALL_UNITS = new Units(true, true, true, true, true);
    private static final byte[] FULL_BLOCK_LIVE = HexFormat.of().parseHex(
            "a2037fff73ff17ff4e01d100fffffcff00009f05e50464f7ba2952f9efcf0000e2ff03000400c0037bff6eff1a00");

    private record Rig(Bno055Device dev, FakeBno055 fake) implements AutoCloseable {
        @Override
        public void close() {
            dev.close();
        }
    }

    private static Rig rig() {
        FakeBno055 fake = new FakeBno055();
        return new Rig(new Bno055Device(fake.link, Duration.ofMillis(300)), fake);
    }

    private static byte[] hex(String s) {
        return HexFormat.of().parseHex(s);
    }

    private static void near(double[] want, double[] got, String what) {
        check(got != null && got.length == want.length, what + ": " + Arrays.toString(got));
        for (int i = 0; i < want.length; i++) {
            check(Math.abs(want[i] - got[i]) < 1e-9, what + ": want " + Arrays.toString(want) + ", got " + Arrays.toString(got));
        }
    }

    // ── identity & bridge info ───────────────────────────────────────────────

    public static void bridgeInfoAndIsAlive() {
        try (Rig r = rig()) {
            Bno055.Bno055Info info = r.dev.bridgeInfo();
            eq(Bno055.Bno055Info.unpack(hex(FakeBno055.LIVE_INFO_HEX)), info);
            eq("03.11", info.swRevText());
            eq(List.of(0x37), r.fake.sentCmds());
            check(r.dev.isAlive(), "alive");
            String h = FakeBno055.LIVE_INFO_HEX;
            r.fake.info = hex(h.substring(0, 16) + "00" + h.substring(18)); // initialized = 0
            check(!r.dev.isAlive(), "not initialised");
            r.fake.info = hex("28a1" + h.substring(4)); // wrong chip id
            check(!r.dev.isAlive(), "wrong chip id");
        }
    }

    public static void isAliveIsFalseNotAThrowOnErrors() {
        try (Rig r = rig()) {
            r.fake.fail(0x37, 0x08);
            check(!r.dev.isAlive(), "status error");
            r.fake.silent.add(0x37);
            check(!r.dev.isAlive(), "silent bridge");
        }
    }

    // ── raw register access ──────────────────────────────────────────────────

    public static void readAndWriteRegisters() {
        try (Rig r = rig()) {
            eq(new byte[] {(byte) 0xA0, (byte) 0xFB, 0x32, 0x0F}, r.dev.readRegisters(0x00, 4));
            Object[] last = r.fake.sent.get(r.fake.sent.size() - 1);
            eq(0x32, last[0]);
            eq(new byte[] {0x00, 0x04}, last[1]);
            eq(0xA0, r.dev.readRegister(0x00));
            r.dev.writeRegister(0x40, 0x01);
            List<String> ws = r.fake.writes();
            eq(w(0, 0x40, "01"), ws.get(ws.size() - 1));
            r.dev.writeRegisters(0x55, new byte[] {1, 2});
            eq(new byte[] {1, 2}, Arrays.copyOfRange(r.fake.regs[0], 0x55, 0x57));
            throwsType(IllegalArgumentException.class, () -> r.dev.writeRegister(0x40, 0x101));
        }
    }

    public static void transfersOver128BytesAreSplit() {
        try (Rig r = rig()) {
            byte[] data = new byte[200];
            for (int i = 0; i < 200; i++) {
                data[i] = (byte) i;
            }
            System.arraycopy(data, 0, r.fake.regs[1], 0x10, 200);
            eq(data, r.dev.readRegisters(0x10, 200, 1));
            List<String> reads = new ArrayList<>();
            List<String> writes = new ArrayList<>();
            for (Object[] s : r.fake.sent) {
                byte[] p = (byte[]) s[1];
                if ((Integer) s[0] == 0x32) {
                    reads.add((p[0] & 0xFF) + "+" + (p[1] & 0xFF));
                }
            }
            eq(List.of("16+128", "144+72"), reads);
            r.dev.writeRegisters(0x10, new byte[200], 1);
            for (Object[] s : r.fake.sent) {
                byte[] p = (byte[]) s[1];
                if ((Integer) s[0] == 0x33 && p[0] != 0x07) {
                    writes.add((p[0] & 0xFF) + "+" + (p.length - 1));
                }
            }
            eq(List.of("16+128", "144+72"), writes);
            eq(new byte[200], Arrays.copyOfRange(r.fake.regs[1], 0x10, 0xD8));
        }
    }

    public static void page1IsWrappedInPageIdWritesAndLeftEvenWhenTheAccessFails() {
        try (Rig r = rig()) {
            eq(0xC0, r.dev.readRegister(0x50, 1));
            eq(List.of(w(0, 0x07, "01"), w(1, 0x07, "00")), r.fake.writes());
            eq(0, r.fake.page());
            r.fake.clearLogs();
            r.fake.fail(0x32, 0x09);
            throwsType(StatusException.class, () -> r.dev.readRegister(0x50, 1));
            eq(List.of(w(0, 0x07, "01"), w(1, 0x07, "00")), r.fake.writes());
            eq(0xA0, r.dev.readRegister(0x00)); // the lock is not wedged
        }
    }

    public static void refusesPagesOtherThan0And1() {
        try (Rig r = rig()) {
            IllegalArgumentException e = throwsType(IllegalArgumentException.class, () -> r.dev.readRegister(0x00, 2));
            check(e.getMessage().contains("0 and 1"), e.getMessage());
            eq(0, r.fake.sent.size());
        }
    }

    public static void shortRegDataIsAnErrorNotPadding() {
        try (Rig r = rig()) {
            r.fake.shortReadBy = 1;
            DepzException e = throwsType(DepzException.class, () -> r.dev.readRegisters(0x00, 4));
            check(e.getMessage().contains("expected 4, got 3"), e.getMessage());
        }
    }

    public static void aSilentBridgeTimesOut() {
        try (Rig r = rig()) {
            r.fake.silent.add(0x32);
            throwsType(DepzTimeoutException.class, () -> r.dev.readRegister(0x00));
        }
    }

    // ── remap, temperature source ────────────────────────────────────────────

    public static void axisRemapByPlacementName() {
        try (Rig r = rig()) {
            r.fake.setMode(OprMode.NDOF.value);
            r.dev.setAxisRemap(AxisRemap.placement("p0"));
            eq(List.of(w(0, 0x3D, "00"), w(0, 0x41, "2104"), w(0, 0x3D, "0c")), r.fake.writes());
            eq(AxisRemap.placement("P0"), r.dev.getAxisRemap());
            r.dev.setAxisRemap(AxisRemap.placement("P1"));
            eq(AxisRemap.placement("P1"), r.dev.getAxisRemap());
        }
    }

    public static void refusesNonPermutationAndUnknownPlacementsBeforeTraffic() {
        try (Rig r = rig()) {
            throwsType(IllegalArgumentException.class, () -> r.dev.setAxisRemap(new AxisRemap(0, 0, 1, false, false, false)));
            throwsType(IllegalArgumentException.class, () -> AxisRemap.placement("P9"));
            eq(0, r.fake.sent.size());
        }
    }

    public static void temperatureSource() {
        try (Rig r = rig()) {
            r.fake.setMode(OprMode.NDOF.value);
            r.dev.setTemperatureSource(TempSource.GYRO);
            eq(List.of(w(0, 0x3D, "00"), w(0, 0x40, "01"), w(0, 0x3D, "0c")), r.fake.writes());
            eq(TempSource.GYRO, r.dev.getTemperatureSource());
        }
    }

    // ── reset, status ────────────────────────────────────────────────────────

    public static void resetSensorStopsTheStreamAndDropsTheUnitsCache() {
        try (Rig r = rig()) {
            r.dev.setUnits(ALL_UNITS);
            r.dev.startStream();
            r.fake.sysStatusScript.addAll(List.of(2, 3, 0));
            r.dev.resetSensor();
            check(r.fake.sentCmds().contains(0x34), "RESET sent");
            check(r.fake.resetCount == 1 && !r.fake.streaming, "the bridge reset and stopped");
            check(!r.dev.streaming(), "the host stopped");
            check(r.fake.sysStatusScript.isEmpty(), "waited for the boot");
            // units cache dropped: power-on 0x80 is read back, not the pre-reset units
            eq(new Units(false, false, false, false, true), r.dev.readSample(new int[] {0x34, 1}).units());
        }
    }

    public static void resetSensorSurfacesAFailure() {
        try (Rig r = rig()) {
            r.fake.fail(0x34, 0x09);
            throwsType(StatusException.class, r.dev::resetSensor);
        }
    }

    public static void systemStatusNeverReadsIntSta() {
        try (Rig r = rig()) {
            r.fake.regs[0][0x37] = 0x40;
            r.fake.regs[0][0x38] = 1;
            r.fake.setMode(OprMode.NDOF.value);
            SystemStatus st = r.dev.systemStatus();
            eq(new SystemStatus(0x0F, 1, 5, 0), st);
            check(st.selfTestPassed(), "self-test passed");
            eq("fusion algorithm running", st.statusText());
            eq(0x40, (int) r.fake.regs[0][0x37]);
            List<String> ops = new ArrayList<>();
            for (FakeBno055.RegOp o : r.fake.ops) {
                ops.add(o.addr() + "+" + o.data().length);
            }
            eq(List.of("54+1", "56+3"), ops);
        }
    }

    // ── calibration ──────────────────────────────────────────────────────────

    public static void calibrationStatus() {
        try (Rig r = rig()) {
            r.fake.regs[0][0x35] = (byte) 0xFF;
            check(r.dev.calibrationStatus().fullyCalibrated(), "fully calibrated");
            r.fake.regs[0][0x35] = (byte) 0xC4;
            eq(new CalibStatus(3, 0, 1, 0), r.dev.calibrationStatus());
        }
    }

    public static void calibrationProfileRoundTripInConfig() {
        try (Rig r = rig()) {
            CalibrationProfile extremes = new CalibrationProfile(new int[] {-32768, 32767, 0}, new int[] {1, -1, 2},
                    new int[] {-2, 3, -4}, 1000, 65535);
            System.arraycopy(extremes.pack(), 0, r.fake.regs[0], 0x55, 22);
            r.fake.setMode(OprMode.NDOF.value);
            CalibrationProfile got = r.dev.readCalibrationProfile();
            eq(extremes.pack(), got.pack());
            eq(List.of(w(0, 0x3D, "00"), w(0, 0x3D, "0c")), r.fake.writes());
            List<String> kinds = new ArrayList<>();
            for (FakeBno055.RegOp o : r.fake.ops) {
                kinds.add(o.read() ? String.format("r%x+%d", o.addr(), o.data().length)
                        : String.format("w%x=%s", o.addr(), HexFormat.of().formatHex(o.data())));
            }
            check(kinds.indexOf("w3d=00") < kinds.indexOf("r55+22") && kinds.indexOf("r55+22") < kinds.indexOf("w3d=0c"),
                    "read in CONFIG: " + kinds);
            r.fake.clearLogs();
            CalibrationProfile typical = new CalibrationProfile(new int[] {-20, 13, -31}, new int[] {120, -340, 55},
                    new int[] {-1, 0, 2}, 1000, 712);
            r.dev.writeCalibrationProfile(typical);
            eq(List.of(w(0, 0x3D, "00"), w(0, 0x55, HexFormat.of().formatHex(typical.pack())), w(0, 0x3D, "0c")),
                    r.fake.writes());
            eq(typical.pack(), Arrays.copyOfRange(r.fake.regs[0], 0x55, 0x6B));
        }
    }

    public static void sicMatrix() {
        try (Rig r = rig()) {
            r.fake.setMode(OprMode.NDOF.value);
            r.dev.setSicMatrix(null);
            eq(List.of(w(0, 0x3D, "00"), w(0, 0x43, "0040" + "0000".repeat(3) + "0040" + "0000".repeat(3) + "0040"),
                    w(0, 0x3D, "0c")), r.fake.writes());
            eq(Bno055Regs.SIC_IDENTITY, r.dev.getSicMatrix());
        }
    }

    // ── page-1 configuration and interrupts ──────────────────────────────────

    public static void gyroAndMagConfigs() {
        try (Rig r = rig()) {
            eq(new GyroConfig(0, 7, 0), r.dev.getGyroConfig());
            eq(new MagConfig(5, 1, 3), r.dev.getMagConfig());
            r.dev.setGyroConfig(new GyroConfig(2, 3, 1));
            r.dev.setMagConfig(new MagConfig(7, 3, 0));
            List<String> page1 = new ArrayList<>();
            for (String s : r.fake.writes()) {
                if (s.startsWith("1:0a") || s.startsWith("1:09")) {
                    page1.add(s);
                }
            }
            eq(List.of(w(1, 0x0A, "1a01"), w(1, 0x09, "1f")), page1);
            eq(new GyroConfig(2, 3, 1), r.dev.getGyroConfig());
            eq(new MagConfig(7, 3, 0), r.dev.getMagConfig());
        }
    }

    public static void uniqueId() {
        try (Rig r = rig()) {
            byte[] want = new byte[16];
            for (int i = 0; i < 16; i++) {
                want[i] = (byte) (0xC0 + i);
            }
            eq(want, r.dev.uniqueId());
        }
    }

    public static void interruptEnableAndMaskSkipTheConfigRoundTrip() {
        try (Rig r = rig()) {
            r.fake.setMode(OprMode.NDOF.value);
            r.dev.setInterruptEnable(0x40);
            r.dev.setInterruptMask(0xC0);
            eq(List.of(w(0, 0x07, "01"), w(1, 0x10, "40"), w(1, 0x07, "00"),
                    w(0, 0x07, "01"), w(1, 0x0F, "c0"), w(1, 0x07, "00")), r.fake.writes());
            eq(0x40, r.dev.getInterruptEnable());
            eq(0xC0, r.dev.getInterruptMask());
        }
    }

    public static void interruptSettingAcceptsOnlyPage1From0x11To0x1f() {
        try (Rig r = rig()) {
            for (int bad : new int[] {0x10, 0x20}) {
                IllegalArgumentException e = throwsType(IllegalArgumentException.class, () -> r.dev.setInterruptSetting(bad, 1));
                check(e.getMessage().contains("interrupt setting"), e.getMessage());
            }
            eq(0, r.fake.sent.size());
            r.fake.setMode(OprMode.AMG.value);
            r.dev.setInterruptSetting(0x11, 0x0A);
            eq(List.of(w(0, 0x3D, "00"), w(0, 0x07, "01"), w(1, 0x11, "0a"), w(1, 0x07, "00"), w(0, 0x3D, "07")),
                    r.fake.writes());
        }
    }

    public static void intStaClearsOnReadAndClearInterruptKeepsClkSel() {
        try (Rig r = rig()) {
            r.fake.regs[0][0x37] = 0x40;
            r.fake.regs[0][0x3F] = (byte) 0x80;
            eq(0x40, r.dev.readInterruptStatus());
            eq(0, r.dev.readInterruptStatus());
            r.dev.clearInterrupt();
            eq(List.of(w(0, 0x3F, "c0")), r.fake.writes());
            eq(0x80, r.fake.regs[0][0x3F] & 0xFF);
        }
    }

    // ── polling ──────────────────────────────────────────────────────────────

    public static void readSampleFullBlockWithTheUnitsCached() {
        try (Rig r = rig()) {
            System.arraycopy(FULL_BLOCK_LIVE, 0, r.fake.regs[0], 0x08, FULL_BLOCK_LIVE.length);
            Bno055Sample s = r.dev.readSample();
            eq(new Units(false, false, false, false, true), s.units());
            eq(Bno055Regs.FULL_BLOCK_ADDR, s.addr());
            eq(FULL_BLOCK_LIVE, s.raw());
            eq(r.fake.mcuTimeUs, s.timestampUs());
            near(new double[] {9.30, -1.29, -1.41}, s.accel(), "accel m/s²"); // 930, -129, -141 / 100
            eq(26.0, s.temperature());
            eq(0, s.calibration().pack());
            r.dev.readSample();
            eq(1, r.fake.reads(0x3B), "UNIT_SEL read once, then cached");
        }
    }

    public static void readSampleScalesByEveryUnitBit() {
        try (Rig r = rig()) {
            System.arraycopy(FULL_BLOCK_LIVE, 0, r.fake.regs[0], 0x08, FULL_BLOCK_LIVE.length);
            r.fake.regs[0][0x3B] = (byte) 0x97;
            Bno055Sample s = r.dev.readSample();
            eq(ALL_UNITS, s.units());
            near(new double[] {930, -129, -141}, s.accel(), "mg: 1 LSB");
            eq(52.0, s.temperature()); // °F: 1 LSB = 2 °F
        }
    }

    public static void readSampleWindowLeavesUncoveredChannelsNull() {
        try (Rig r = rig()) {
            r.fake.regs[0][0x34] = (byte) 0xE7; // -25 °C
            r.fake.regs[0][0x35] = (byte) 0xFF;
            Bno055Sample s = r.dev.readSample(new int[] {0x34, 2});
            eq(-25.0, s.temperature());
            check(s.calibration().fullyCalibrated(), "calibrated");
            eq(Collections.nCopies(7, null), Arrays.asList(s.accel(), s.mag(), s.gyro(), s.euler(), s.quaternion(),
                    s.linearAccel(), s.gravity()));
        }
    }

    public static void readQuaternionReadsOnlyThe8ByteBlock() {
        try (Rig r = rig()) {
            r.fake.setMode(OprMode.NDOF.value);
            near(new double[] {10685 / 16384.0, -1693 / 16384.0, -12305 / 16384.0, 0.0}, r.dev.readQuaternion(), "quaternion");
            List<String> reads = new ArrayList<>();
            for (FakeBno055.RegOp o : r.fake.ops) {
                if (o.read() && o.addr() != 0x3B) {
                    reads.add(o.addr() + "+" + o.data().length);
                }
            }
            eq(List.of("32+8"), reads);
        }
    }

    // ── streaming ────────────────────────────────────────────────────────────

    public static void startStreamWireBytesAndTheUnitsLatchedBeforeIt() {
        try (Rig r = rig()) {
            check(!r.dev.streaming(), "idle");
            r.fake.regs[0][0x3B] = (byte) 0x97;
            r.dev.startStream();
            // UNIT_SEL was read before START_STREAM went out
            eq(List.of(0x32, 0x35), r.fake.sentCmds());
            eq(Bno055.packStartStream(Bno055.TRIGGER_TIMER, 0x08, 46, 10), r.fake.streams.get(0));
            check(r.dev.streaming() && r.fake.streaming, "streaming");
            List<Bno055Sample> got = Collections.synchronizedList(new ArrayList<>());
            r.dev.onSample(got::add);
            r.dev.setUnits(Units.DEFAULT); // later units do not apply to the running stream
            r.fake.pushStream(0x08, hex("a2037fff73ff"), 1);
            check(until(() -> got.size() == 1, 2000), "a sample");
            eq(ALL_UNITS, got.get(0).units());
            near(new double[] {930, -129, -141}, got.get(0).accel(), "accel mg");
        }
    }

    public static void aFailedStartStreamLeavesTheStreamOff() {
        try (Rig r = rig()) {
            r.fake.fail(0x35, 0x08);
            throwsType(StatusException.class, r.dev::startStream);
            check(!r.dev.streaming(), "off");
        }
    }

    public static void stopStream() {
        try (Rig r = rig()) {
            r.dev.stopStream(); // idle: nothing sent
            eq(0, r.fake.sent.size());
            r.dev.startStream();
            r.dev.stopStream();
            eq(0x36, r.fake.sentCmds().get(r.fake.sentCmds().size() - 1));
            check(!r.dev.streaming() && !r.fake.streaming, "stopped");
            r.dev.startStream();
            r.fake.fail(0x36, 0x01);
            throwsType(StatusException.class, r.dev::stopStream);
            check(!r.dev.streaming(), "the flag is cleared even on failure");
        }
    }

    public static void callbacksUnsubscribeQueueAndDropOldest() throws Exception {
        try (Rig r = rig()) {
            r.dev.startStream(20, new int[] {Bno055Regs.QUAT_BLOCK_ADDR, Bno055Regs.QUAT_BLOCK_LEN});
            List<Bno055Sample> got = Collections.synchronizedList(new ArrayList<>());
            AutoCloseable off = r.dev.onSample(got::add);
            Subscription<Bno055Sample> it = r.dev.samples(2);
            for (int i = 0; i < 5; i++) {
                r.fake.pushStream(0x20, FakeBno055.LIVE_QUAT, i);
            }
            check(until(() -> got.size() == 5, 2000), "five samples");
            check(got.get(0).quaternion() != null && got.get(0).accel() == null, "quaternion block only");
            eq(List.of(3L), r.dev.streamDroppedCounts());
            eq(3L, it.droppedCount());
            eq(3L, it.take(Duration.ofSeconds(1)).timestampUs());
            off.close();
            it.close();
            eq(List.of(), r.dev.streamDroppedCounts());
            r.fake.pushStream(0x20, FakeBno055.LIVE_QUAT, 9);
            r.dev.bridgeInfo(); // a round trip queued behind the report: it was handled
            eq(5, got.size());
        }
    }

    public static void getSampleNextTimeoutAndClose() throws Exception {
        Rig r = rig();
        r.dev.startStream();
        List<Object> result = Collections.synchronizedList(new ArrayList<>());
        Thread t = new Thread(() -> result.add(r.dev.getSample(Duration.ofSeconds(1))));
        t.start();
        check(until(() -> r.dev.streamDroppedCounts().equals(List.of(0L)), 2000), "the waiter subscribed");
        r.fake.pushStream(0x34, hex("1a00"), 77);
        t.join(2000);
        Bno055Sample s = (Bno055Sample) result.get(0);
        eq(77L, s.timestampUs());
        eq(26.0, s.temperature());
        eq(List.of(), r.dev.streamDroppedCounts(), "its one-shot queue is gone");
        throwsType(DepzTimeoutException.class, () -> r.dev.getSample(Duration.ofMillis(50)));
        Thread pending = new Thread(() -> {
            try {
                r.dev.getSample(Duration.ofSeconds(5));
            } catch (RuntimeException e) {
                result.add(e);
            }
        });
        pending.start();
        check(until(() -> r.dev.streamDroppedCounts().equals(List.of(0L)), 2000), "the waiter subscribed");
        r.dev.close();
        pending.join(2000);
        LinkClosedException e = (LinkClosedException) result.get(1);
        check(e.getMessage().contains("device closed"), e.getMessage());
        e = throwsType(LinkClosedException.class, () -> r.dev.getSample(Duration.ofMillis(100)));
        check(e.getMessage().contains("device is closed"), e.getMessage());
    }

    public static void malformedStreamReportsAreCountedAndDropped() {
        try (Rig r = rig()) {
            r.dev.startStream();
            List<Bno055Sample> got = Collections.synchronizedList(new ArrayList<>());
            r.dev.onSample(got::add);
            r.fake.pushRaw(0x93, new byte[9]); // shorter than the 10-byte header
            r.fake.pushStream(0x08, hex("a2037fff73ff"), 1, 46); // truncated block
            check(until(() -> r.dev.streamParseErrors() == 2, 2000), "two parse errors: " + r.dev.streamParseErrors());
            eq(0, got.size());
            r.fake.pushStream(0x08, hex("a2037fff73ff"), 2);
            check(until(() -> got.size() == 1, 2000), "a good one");
            near(new double[] {9.3, -1.29, -1.41}, got.get(0).accel(), "accel m/s²");
            eq(0xA0, r.dev.readRegister(0x00)); // the link keeps working
        }
    }

    public static void configurationOnPage1IsRefusedWhileStreaming() {
        try (Rig r = rig()) {
            r.dev.startStream();
            int n = r.fake.sent.size();
            throwsType(InvalidStateException.class, () -> r.dev.setGyroConfig(new GyroConfig(0, 0, 0)));
            throwsType(InvalidStateException.class, () -> r.dev.setMagConfig(new MagConfig(0, 0, 0)));
            throwsType(InvalidStateException.class, () -> r.dev.setInterruptSetting(0x11, 1));
            eq(n, r.fake.sent.size());
        }
    }
}
