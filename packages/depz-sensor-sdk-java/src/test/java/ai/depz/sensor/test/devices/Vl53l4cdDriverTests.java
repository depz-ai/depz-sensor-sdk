package ai.depz.sensor.test.devices;

import static ai.depz.sensor.test.devices.T.check;
import static ai.depz.sensor.test.devices.T.eq;
import static ai.depz.sensor.test.devices.T.throwsType;

import ai.depz.sensor.devices.InvalidStateException;
import ai.depz.sensor.devices.Subscription;
import ai.depz.sensor.devices.Vl53l4Measurement;
import ai.depz.sensor.devices.Vl53l4cdDevice;
import ai.depz.sensor.protocol.Common;
import ai.depz.sensor.protocol.Vl53l4;
import ai.depz.sensor.sensors.vl53l4.DetectionThresholds;
import ai.depz.sensor.sensors.vl53l4.Vl53l4Platform;
import ai.depz.sensor.sensors.vl53l4.Vl53l4Uld;
import ai.depz.sensor.sensors.vl53l4.Vl53l4cdDriver;
import java.time.Duration;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

/** The VL53L4CD ULD over a register map seeded like a booted sensor, and the device class over a fake board. */
public final class Vl53l4cdDriverTests {
    static final int OSC_FREQUENCY_TYPICAL = 0x3980;
    static final int CLOCK_PLL_TYPICAL = 0x0A5C;

    /** A plain register map seeded with what a booted VL53L4CD answers; the config block leaves it "data ready". */
    static final class FakePlatform implements Vl53l4Platform {
        final Map<Integer, Byte> reg = new HashMap<>();
        final List<Object[]> writes = new ArrayList<>();
        final List<Integer> speeds = new ArrayList<>();

        FakePlatform() {
            reg.put(Vl53l4Uld.FIRMWARE_SYSTEM_STATUS, (byte) 0x03);
            reg.put(Vl53l4Uld.GPIO_HV_MUX_CTRL, (byte) 0x11); // INT active low, data ready
            seedWord(Vl53l4Uld.IDENTIFICATION_MODEL_ID, Vl53l4Uld.MODEL_ID);
            seedWord(Vl53l4Uld.OSC_FREQUENCY, OSC_FREQUENCY_TYPICAL);
            seedWord(Vl53l4Uld.RESULT_OSC_CALIBRATE_VAL, CLOCK_PLL_TYPICAL);
        }

        void seedWord(int addr, int value) {
            reg.put(addr, (byte) (value >> 8));
            reg.put(addr + 1, (byte) value);
        }

        @Override
        public byte[] readMulti(int addr, int size) {
            byte[] out = new byte[size];
            for (int i = 0; i < size; i++) {
                out[i] = reg.getOrDefault(addr + i, (byte) 0);
            }
            return out;
        }

        @Override
        public void writeMulti(int addr, byte[] data) {
            writes.add(new Object[] {addr, data.clone()});
            for (int i = 0; i < data.length; i++) {
                reg.put(addr + i, data[i]);
            }
        }

        @Override
        public void sleepMs(int ms) {}

        @Override
        public void setI2cSpeed(int khz) {
            speeds.add(khz);
        }

        List<byte[]> writtenTo(int addr) {
            List<byte[]> out = new ArrayList<>();
            for (Object[] w : writes) {
                if ((int) w[0] == addr) {
                    out.add((byte[]) w[1]);
                }
            }
            return out;
        }

        byte[] last(int addr) {
            List<byte[]> l = writtenTo(addr);
            return l.get(l.size() - 1);
        }

        /** Every result read returns a valid measurement with these values. */
        void seedResult(int distanceMm, int signalRaw, int spadsRaw) {
            reg.put(Vl53l4Uld.RESULT_RANGE_STATUS, (byte) validRaw());
            seedWord(Vl53l4Uld.RESULT_SPAD_NB, spadsRaw);
            seedWord(Vl53l4Uld.RESULT_SIGNAL_RATE, signalRaw);
            seedWord(Vl53l4Uld.RESULT_DISTANCE, distanceMm);
        }
    }

    static int validRaw() {
        for (int i = 0; i < Vl53l4Uld.STATUS_RTN.length; i++) {
            if (Vl53l4Uld.STATUS_RTN[i] == 0) {
                return i;
            }
        }
        throw new AssertionError("no valid raw status");
    }

    private static int word(byte[] b) {
        return ((b[0] & 0xFF) << 8) | (b[1] & 0xFF);
    }

    public static void initRunsAtBootSpeedThenRetimes() {
        FakePlatform p = new FakePlatform();
        new Vl53l4cdDriver(p).sensorInit();
        eq(Vl53l4Uld.I2C_KHZ_BOOT, p.speeds.get(0));
        eq(Vl53l4Uld.I2C_KHZ_DEFAULT, p.speeds.get(p.speeds.size() - 1));
        FakePlatform p2 = new FakePlatform();
        new Vl53l4cdDriver(p2).sensorInit(Vl53l4Uld.I2C_KHZ_BOOT);
        eq(List.of(Vl53l4Uld.I2C_KHZ_BOOT), p2.speeds);
    }

    public static void initWritesTheConfigBlockAndTheVhvSequence() {
        FakePlatform p = new FakePlatform();
        Vl53l4cdDriver drv = new Vl53l4cdDriver(p);
        drv.sensorInit();
        byte[] block = p.writtenTo(Vl53l4Uld.CONFIG_ADDR).get(0);
        check(Arrays.equals(Vl53l4Uld.configBlock(), block), "config block");
        eq(Vl53l4Uld.CONFIG_FMP_BYTE, block[0] & 0xFF);
        List<byte[]> starts = p.writtenTo(Vl53l4Uld.SYSTEM_START);
        eq(0x40, starts.get(0)[0] & 0xFF);
        eq(0x80, starts.get(1)[0] & 0xFF);
        eq(1, p.writtenTo(Vl53l4Uld.SYSTEM_INTERRUPT_CLEAR).get(0)[0] & 0xFF);
        eq(0x09, p.last(Vl53l4Uld.VHV_CONFIG_TIMEOUT_MACROP_LOOP_BOUND)[0] & 0xFF);
        eq(0x00, p.last(0x000B)[0] & 0xFF);
        eq(0x0500, word(p.last(0x0024)));
        eq(0, drv.getRangeTiming().interMeasurementMs()); // 50 ms continuous by default
    }

    public static void rangeTimingWritesWhatDecodesBack() {
        FakePlatform p = new FakePlatform();
        Vl53l4cdDriver drv = new Vl53l4cdDriver(p);
        drv.setRangeTiming(50, 0);
        int[] r = Vl53l4Uld.rangeTimingRegisters(50, 0, OSC_FREQUENCY_TYPICAL, 0);
        eq(r[0], word(p.last(Vl53l4Uld.RANGE_CONFIG_A)));
        eq(r[1], word(p.last(Vl53l4Uld.RANGE_CONFIG_B)));
        eq(0, drv.getRangeTiming().interMeasurementMs());
        int budget = drv.getRangeTiming().timingBudgetMs();
        check(budget >= 49 && budget <= 51, "budget " + budget); // the C math loses up to 1 ms on the way back
        drv.startRanging();
        eq(0x21, p.last(Vl53l4Uld.SYSTEM_START)[0] & 0xFF); // continuous
        drv.stopRanging();
        eq(0x80, p.last(Vl53l4Uld.SYSTEM_START)[0] & 0xFF);
    }

    public static void autonomousModeStartsWith0x40() {
        FakePlatform p = new FakePlatform();
        Vl53l4cdDriver drv = new Vl53l4cdDriver(p);
        drv.setRangeTiming(50, 1000);
        check(drv.getRangeTiming().interMeasurementMs() > 0, "autonomous");
        drv.startRanging();
        eq(0x40, p.last(Vl53l4Uld.SYSTEM_START)[0] & 0xFF);
    }

    public static void rangeTimingBoundsAndSettingsRoundTrip() {
        Vl53l4cdDriver drv = new Vl53l4cdDriver(new FakePlatform());
        // ERRATA A16: a refused argument is invalid_argument, not the driver's Vl53l4Error
        throwsType(IllegalArgumentException.class, () -> drv.setRangeTiming(9, 0));
        throwsType(IllegalArgumentException.class, () -> drv.setRangeTiming(201, 0));
        throwsType(IllegalArgumentException.class, () -> drv.setRangeTiming(50, 30)); // 0 < inter <= budget
        for (int mm : new int[] {10, -10, 0, 255}) {
            drv.setOffset(mm);
            eq(mm, drv.getOffset());
        }
        drv.setXtalk(20);
        eq(20, drv.getXtalk());
        drv.setDetectionThresholds(100, 300, Vl53l4Uld.WINDOW_IN);
        eq(new DetectionThresholds(100, 300, Vl53l4Uld.WINDOW_IN), drv.getDetectionThresholds());
        drv.setSignalThreshold(1024);
        eq(1024, drv.getSignalThreshold());
        drv.setSigmaThreshold(15);
        eq(15, drv.getSigmaThreshold());
        throwsType(IllegalArgumentException.class, () -> drv.setSigmaThreshold(0x4000));
    }

    public static void isAliveAndWaitsTimeOut() {
        FakePlatform p = new FakePlatform();
        Vl53l4cdDriver drv = new Vl53l4cdDriver(p);
        check(drv.isAlive(), "alive");
        p.seedWord(Vl53l4Uld.IDENTIFICATION_MODEL_ID, 0x1234);
        check(!drv.isAlive(), "not alive");
        p.reg.put(Vl53l4Uld.FIRMWARE_SYSTEM_STATUS, (byte) 0);
        throwsType(Vl53l4Uld.Vl53l4Error.class, () -> drv.waitBoot(5));
        p.reg.put(Vl53l4Uld.GPIO_TIO_HV_STATUS, (byte) 1); // INT high: not at its active (low) level
        throwsType(Vl53l4Uld.Vl53l4Error.class, () -> drv.waitDataReady(5));
    }

    public static void offsetCalibrationProgramsTargetMinusMean() {
        FakePlatform p = new FakePlatform();
        Vl53l4cdDriver drv = new Vl53l4cdDriver(p);
        p.seedResult(93, 200, 256 * 40);
        eq(7, drv.calibrateOffset(100, 5));
        eq(Vl53l4Uld.offsetRaw(7), word(p.last(Vl53l4Uld.RANGE_OFFSET_MM)));
        throwsType(IllegalArgumentException.class, () -> drv.calibrateOffset(5, 20));
        throwsType(IllegalArgumentException.class, () -> drv.calibrateOffset(100, 4));
    }

    public static void xtalkCalibrationFollowsTheStFormula() {
        FakePlatform p = new FakePlatform();
        Vl53l4cdDriver drv = new Vl53l4cdDriver(p);
        // signal 200*8 = 1600 kcps over 40 SPADs; distance 90 at a 100 mm target: (1 - 90/100) * (1600 / 40) = 4.0 kcps.
        p.seedResult(90, 200, 256 * 40);
        eq(4, drv.calibrateXtalk(100, 5));
        // In binary floating point the product is 3.9999…, and the register takes the truncation.
        eq(2047, word(p.last(Vl53l4Uld.XTALK_PLANE_OFFSET_KCPS)));
        p.seedResult(10, 0xFFFF, 256);
        throwsType(Vl53l4Uld.Vl53l4Error.class, () -> drv.calibrateXtalk(1000, 5)); // > 127 kcps
        eq(-3, Vl53l4Uld.roundHalfAway(-2.5));
        eq(3, Vl53l4Uld.roundHalfAway(2.5));
    }

    public static void configurationWhileRangingIsRefused() throws Exception {
        FakeBoardLink board = new FakeBoardLink();
        board.onPacket = pkt -> {
            if (pkt.cmd() != Vl53l4.Vl53l4Cmd.READ_REG.value) {
                return List.of(FakeBoardLink.status(pkt.cmd(), Common.Status.OK.value));
            }
            int len = (pkt.payload()[2] & 0xFF) | ((pkt.payload()[3] & 0xFF) << 8);
            byte[] reply = new byte[9 + len]; // echoed cmd, timestamp, data (all zero: continuous mode)
            reply[0] = (byte) Vl53l4.Vl53l4Cmd.READ_REG.value;
            return List.of(board.reply(Vl53l4.Vl53l4Rpt.REG_DATA.value, reply));
        };
        try (Vl53l4cdDevice dev = new Vl53l4cdDevice(board, Duration.ofMillis(500))) {
            dev.startRanging();
            check(dev.ranging(), "ranging");
            throwsType(InvalidStateException.class, () -> dev.setOffsetMm(3));
            throwsType(InvalidStateException.class, () -> dev.init());
            throwsType(InvalidStateException.class, () -> dev.measureOnce());
            dev.stopRanging();
            check(!dev.ranging(), "stopped");
            dev.stopRanging(); // a no-op once stopped
        }
    }

    public static void streamReportsDecodeAndShortOnesAreCounted() throws Exception {
        FakeBoardLink board = new FakeBoardLink();
        try (Vl53l4cdDevice dev = new Vl53l4cdDevice(board); Subscription<Vl53l4Measurement> q = dev.measurements()) {
            byte[] block = new byte[17];
            block[0] = (byte) validRaw();
            block[13] = 0x01; // distance 0x0102 = 258 mm
            block[14] = 0x02;
            board.inject(board.reply(Vl53l4.Vl53l4Rpt.STREAM.value, streamPayload(block, 17)));
            board.inject(board.reply(Vl53l4.Vl53l4Rpt.STREAM.value, streamPayload(Arrays.copyOf(block, 8), 17)));
            Vl53l4Measurement m = q.take(Duration.ofSeconds(1));
            eq(List.of(123456L, 258, 0), List.of(m.timestampUs(), m.distanceMm(), m.rangeStatus()));
            check(m.valid(), "valid");
            T.until(() -> dev.streamParseErrors() == 1, 1000);
            eq(1L, dev.streamParseErrors());
            eq(0, q.size());
        }
    }

    private static byte[] streamPayload(byte[] data, int lenField) {
        byte[] p = new byte[12 + data.length];
        long ts = 123456L;
        for (int i = 0; i < 8; i++) {
            p[i] = (byte) (ts >>> (8 * i));
        }
        p[8] = (byte) Vl53l4Uld.RESULT_BLOCK_ADDR;
        p[10] = (byte) lenField;
        System.arraycopy(data, 0, p, 12, data.length);
        return p;
    }
}
