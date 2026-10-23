package ai.depz.sensor.test.devices;

import static ai.depz.sensor.test.devices.T.check;
import static ai.depz.sensor.test.devices.T.eq;
import static ai.depz.sensor.test.devices.T.throwsType;

import ai.depz.sensor.devices.Bno055Device;
import ai.depz.sensor.devices.Bno055Sample;
import ai.depz.sensor.devices.Subscription;
import ai.depz.sensor.protocol.Bno055;
import ai.depz.sensor.protocol.Common;
import ai.depz.sensor.sensors.bno055.Bno055Regs;
import ai.depz.sensor.sensors.bno055.Bno055Regs.AccelConfig;
import ai.depz.sensor.sensors.bno055.Bno055Regs.AxisRemap;
import ai.depz.sensor.sensors.bno055.Bno055Regs.CalibrationProfile;
import ai.depz.sensor.sensors.bno055.Bno055Regs.OprMode;
import ai.depz.sensor.sensors.bno055.Bno055Regs.PwrMode;
import ai.depz.sensor.sensors.bno055.Bno055Regs.SystemStatus;
import ai.depz.sensor.sensors.bno055.Bno055Regs.Units;
import ai.depz.sensor.transport.Packet;
import java.time.Duration;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

/** The BNO055 device class over a fake register bridge. */
public final class Bno055DeviceTests {
    /**
     * A BNO055 behind its register bridge: two register pages, a fused quaternion that is non-zero in
     * fusion modes, and every register write recorded (page, register, value).
     */
    static final class FakeBoard {
        final FakeBoardLink link = new FakeBoardLink();
        final byte[][] pages = {new byte[256], new byte[256]};
        final List<int[]> writes = new ArrayList<>();
        private int page;

        FakeBoard() {
            link.onPacket = this::onPacket;
        }

        private synchronized List<byte[]> onPacket(Packet p) {
            byte[] pl = p.payload();
            if (p.cmd() == Bno055.Bno055Cmd.READ_REG.value) {
                int addr = pl[0] & 0xFF;
                int n = pl[1] & 0xFF;
                byte[] regs = pages[page];
                if (page == 0) {
                    int mode = regs[Bno055Regs.REG_OPR_MODE] & 0x0F;
                    regs[Bno055Regs.REG_QUA_DATA] = (byte) (mode >= OprMode.IMU.value ? 0x10 : 0);
                }
                byte[] reply = new byte[9 + n];
                reply[0] = (byte) Bno055.Bno055Cmd.READ_REG.value;
                System.arraycopy(regs, addr, reply, 9, n);
                return List.of(link.reply(Bno055.Bno055Rpt.REG_DATA.value, reply));
            }
            if (p.cmd() == Bno055.Bno055Cmd.WRITE_REG.value) {
                int addr = pl[0] & 0xFF;
                for (int i = 1; i < pl.length; i++) {
                    int reg = addr + i - 1;
                    writes.add(new int[] {page, reg, pl[i] & 0xFF});
                    if (reg == Bno055Regs.REG_PAGE_ID) {
                        page = pl[i];
                    } else {
                        pages[page][reg] = pl[i];
                    }
                }
            }
            return List.of(FakeBoardLink.status(p.cmd(), Common.Status.OK.value));
        }

        synchronized List<Integer> written(int reg, int onPage) {
            List<Integer> out = new ArrayList<>();
            for (int[] w : writes) {
                if (w[1] == reg && w[0] == onPage) {
                    out.add(w[2]);
                }
            }
            return out;
        }

        List<Integer> written(int reg) {
            return written(reg, 0);
        }

        synchronized void clear() {
            writes.clear();
        }
    }

    static FakeBoard board() {
        return new FakeBoard();
    }

    static Bno055Device open(FakeBoard b) {
        return new Bno055Device(b.link, Duration.ofMillis(500));
    }

    public static void aSwitchBetweenTwoOperatingModesGoesThroughConfig() {
        FakeBoard b = board();
        try (Bno055Device dev = open(b)) {
            dev.setOperationMode(OprMode.NDOF);
            dev.setOperationMode(OprMode.AMG);
            eq(List.of(OprMode.NDOF.value, 0, OprMode.AMG.value), b.written(Bno055Regs.REG_OPR_MODE));
            dev.setOperationMode(OprMode.AMG); // already there: nothing written
            eq(3, b.written(Bno055Regs.REG_OPR_MODE).size());
            eq(OprMode.AMG, dev.getOperationMode());
        }
    }

    public static void suspendKeepsTheModeAndWakeUpPutsItBack() {
        FakeBoard b = board();
        try (Bno055Device dev = open(b)) {
            dev.configure(OprMode.NDOF);
            b.clear();
            dev.setPowerMode(PwrMode.SUSPEND);
            eq(List.of(0), b.written(Bno055Regs.REG_OPR_MODE));
            eq(List.of(PwrMode.SUSPEND.value), b.written(Bno055Regs.REG_PWR_MODE));
            b.clear();
            dev.setPowerMode(PwrMode.NORMAL);
            eq(List.of(PwrMode.NORMAL.value), b.written(Bno055Regs.REG_PWR_MODE));
            eq(List.of(OprMode.NDOF.value), b.written(Bno055Regs.REG_OPR_MODE));
        }
    }

    public static void aChipFoundAsleepWakesIntoNdof() {
        FakeBoard b = board();
        try (Bno055Device dev = open(b)) {
            b.pages[0][Bno055Regs.REG_PWR_MODE] = (byte) PwrMode.SUSPEND.value; // CONFIG + suspended at connect
            dev.setPowerMode(PwrMode.NORMAL);
            eq(List.of(OprMode.NDOF.value), b.written(Bno055Regs.REG_OPR_MODE));
        }
    }

    public static void configureWritesUnitsRemapAndProfileInConfig() {
        FakeBoard b = board();
        try (Bno055Device dev = open(b)) {
            Units units = new Units(true, false, false, true, false);
            CalibrationProfile profile = new CalibrationProfile(
                    new int[] {1, 2, 3}, new int[] {4, 5, 6}, new int[] {7, 8, 9}, 1000, 480);
            dev.configure(OprMode.NDOF, units, AxisRemap.placement("P2"), profile);
            eq(List.of(0, OprMode.NDOF.value), b.written(Bno055Regs.REG_OPR_MODE));
            eq(List.of(units.pack()), b.written(Bno055Regs.REG_UNIT_SEL));
            eq(profile.pack(), Arrays.copyOfRange(b.pages[0], Bno055Regs.REG_CALIB_PROFILE,
                    Bno055Regs.REG_CALIB_PROFILE + Bno055Regs.CALIB_PROFILE_LEN));
            eq(units, dev.getUnits());
            b.clear();
            dev.restoreConfiguration();
            eq(List.of(units.pack()), b.written(Bno055Regs.REG_UNIT_SEL));
        }
    }

    public static void page1IsEnteredAndLeftAndRefusedWhileStreaming() {
        FakeBoard b = board();
        try (Bno055Device dev = open(b)) {
            AccelConfig cfg = new AccelConfig(2, 3, 0);
            dev.setAccelConfig(cfg);
            List<Integer> pageWrites = new ArrayList<>();
            synchronized (b) {
                for (int[] w : b.writes) {
                    if (w[1] == Bno055Regs.REG_PAGE_ID) {
                        pageWrites.add(w[2]);
                    }
                }
            }
            eq(List.of(1, 0), pageWrites);
            eq(cfg.pack(), b.pages[1][Bno055Regs.REG1_ACC_CONFIG] & 0xFF);
            eq(cfg, dev.getAccelConfig());
            dev.startStream();
            throwsType(IllegalStateException.class, dev::uniqueId);
            throwsType(IllegalStateException.class, () -> dev.setInterruptEnable(1));
            dev.stopStream();
            eq(16, dev.uniqueId().length);
        }
    }

    public static void selfTestRunsInConfigAndRestoresTheMode() {
        FakeBoard b = board();
        try (Bno055Device dev = open(b)) {
            dev.setOperationMode(OprMode.NDOF);
            b.pages[0][Bno055Regs.REG_ST_RESULT] = (byte) Bno055Regs.EXPECTED_SELF_TEST;
            b.clear();
            SystemStatus status = dev.selfTest();
            check(status.selfTestPassed(), "self-test passed");
            eq(List.of(Bno055Regs.SYS_TRIGGER_SELF_TEST), b.written(Bno055Regs.REG_SYS_TRIGGER));
            eq(List.of(0, OprMode.NDOF.value), b.written(Bno055Regs.REG_OPR_MODE));
        }
    }

    public static void argumentsOutsideTheirFieldsAreRefusedBeforeSending() {
        FakeBoard b = board();
        try (Bno055Device dev = open(b)) {
            throwsType(IllegalArgumentException.class, () -> dev.setOperationMode(null));
            throwsType(IllegalArgumentException.class, () -> dev.setInterruptSetting(0x10, 1));
            throwsType(IllegalArgumentException.class, () -> dev.setInterruptMask(256));
            throwsType(IllegalArgumentException.class, () -> dev.startStream(10, new int[] {0xF0, 0x20}));
            throwsType(IllegalArgumentException.class, () -> dev.startStream(-1));
            throwsType(IllegalArgumentException.class, () -> dev.startStream(10, null, 2));
            throwsType(IllegalArgumentException.class, () -> dev.readRegisters(0, 1, 2));
            eq(0, b.link.sentCount());
        }
    }

    public static void samplesDecodeWithTheUnitsOfTheStreamStart() {
        FakeBoard b = board();
        try (Bno055Device dev = open(b); Subscription<Bno055Sample> q = dev.samples()) {
            dev.setUnits(new Units(true, false, false, false, false));
            dev.startStream(20, new int[] {Bno055Regs.REG_ACC_DATA, 6});
            byte[] payload = new byte[16];
            payload[0] = 77;
            payload[8] = (byte) Bno055Regs.REG_ACC_DATA;
            payload[9] = 6;
            payload[10] = (byte) 1000;
            payload[11] = (byte) (1000 >> 8);
            b.link.inject(b.link.reply(Bno055.Bno055Rpt.STREAM.value, payload));
            Bno055Sample s = q.take(Duration.ofSeconds(1));
            eq(1000.0, s.accel()[0], "mg: LSB 1");
            eq(null, s.quaternion());
            eq(77L, s.timestampUs());
        }
    }
}
