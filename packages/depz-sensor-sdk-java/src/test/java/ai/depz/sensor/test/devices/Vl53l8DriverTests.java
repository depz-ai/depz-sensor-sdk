package ai.depz.sensor.test.devices;

import static ai.depz.sensor.test.devices.T.check;
import static ai.depz.sensor.test.devices.T.eq;
import static ai.depz.sensor.test.devices.T.throwsType;

import ai.depz.sensor.sensors.vl53l8.Vl53l8Driver;
import ai.depz.sensor.sensors.vl53l8.Vl53l8Firmware;
import ai.depz.sensor.sensors.vl53l8.Vl53l8Frame;
import ai.depz.sensor.sensors.vl53l8.Vl53l8Motion;
import ai.depz.sensor.sensors.vl53l8.Vl53l8Platform;
import ai.depz.sensor.sensors.vl53l8.Vl53l8Tables;
import ai.depz.sensor.sensors.vl53l8.Vl53l8Uld;
import ai.depz.sensor.sensors.vl53l8.Vl53l8Uld.Vl53l8Error;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.HashMap;
import java.util.HexFormat;
import java.util.List;
import java.util.Map;
import java.util.function.BiFunction;

/** The VL53L8 / L5 / L7 ULD over an emulated register bridge and DCI transport. */
public final class Vl53l8DriverTests {
    static final int DCI_ZONE_CONFIG = 0x5450;
    static final int DCI_FREQ_HZ = 0x5458;
    static final int DCI_INT_TIME = 0x545C;
    static final int DCI_RANGING_MODE = 0xAD30;
    static final int DCI_TARGET_ORDER = 0xAE64;
    static final int DCI_OUTPUT_CONFIG = 0xD968;
    static final int DCI_OUTPUT_ENABLES = 0xD970;
    static final int DCI_OUTPUT_LIST = 0xD980;
    static final int DCI_PIPE_CONTROL = 0xDB80;
    static final int DCI_SINGLE_RANGE = 0xD964;
    static final int DCI_FW_FLAGS = 0xE0C4;
    static final int DCI_CAL_CFG = 0x5470;
    static final int DCI_MOTION_CFG = 0xBFAC;
    static final int DCI_THRESH_START = 0xB6E8;
    static final int DCI_THRESH_VALID = 0xB9F0;
    static final int UI_CMD_STATUS = 0x2C00;
    static final int UI_CMD_START = 0x2C04;
    static final int UI_CMD_END = 0x2FFF;

    record W(int addr, byte[] data) {
        String key() {
            return addr + ":" + HexFormat.of().formatHex(data);
        }
    }

    /**
     * The register bridge and DCI transport, emulated (the Python tests' FakeUldPlatform): DCI
     * reads and writes land in {@code dci}, plain writes in {@code reg}; the command status answers
     * both the NVM poll (byte 0 = 2) and the DCI polls (byte 1 = 3); firmware banks are recorded, not
     * stored; {@code xtalkReply} answers the GET_XTALK command.
     */
    static final class FakePlatform implements Vl53l8Platform {
        final Map<Integer, Byte> reg = new HashMap<>();
        final Map<Integer, byte[]> dci = new HashMap<>();
        final List<W> writes = new ArrayList<>();
        final List<List<Integer>> reads = new ArrayList<>();
        final List<List<Integer>> banks = new ArrayList<>();
        byte[] status = {0x02, 0x03, 0x00, 0x00};
        byte[] xtalkReply;
        private int[] pendingRead;
        private boolean xtalkPending;
        private int page = -1;

        @Override
        public void sleepMs(int ms) {}

        @Override
        public void writeMulti(int addr, byte[] data) {
            data = data.clone();
            if (addr == 0x7FFF && data.length == 1) {
                page = data[0];
            }
            writes.add(new W(addr, data));
            if (data.length >= 0x5000) {
                banks.add(List.of(page, data.length));
                return;
            }
            if (addr == 0x2FB8) {
                xtalkPending = true;
                pendingRead = null;
                return;
            }
            int n = data.length;
            if (addr == UI_CMD_END - 11 && n == 12) {
                xtalkPending = false;
                pendingRead = new int[] {u8(data[0]) << 8 | u8(data[1]), u8(data[2]) << 4 | u8(data[3]) >> 4};
                return;
            }
            if (n >= 12 && data[n - 5] == 0x0F && data[n - 4] == 0x05 && data[n - 3] == 0x01) {
                int index = u8(data[0]) << 8 | u8(data[1]);
                int size = u8(data[2]) << 4 | u8(data[3]) >> 4;
                dci.put(index, Vl53l8Uld.swapBuffer(Arrays.copyOfRange(data, 4, 4 + size)));
                return;
            }
            for (int i = 0; i < n; i++) {
                reg.put(addr + i, data[i]);
            }
        }

        @Override
        public byte[] readMulti(int addr, int size) {
            reads.add(List.of(addr, size));
            if (addr == UI_CMD_STATUS) {
                return Arrays.copyOf(status, size);
            }
            if (addr == UI_CMD_START && xtalkPending && xtalkReply != null) {
                return Arrays.copyOf(xtalkReply, Math.min(size, xtalkReply.length));
            }
            if (addr == UI_CMD_START && pendingRead != null) {
                byte[] stored = dci.getOrDefault(pendingRead[0], new byte[0]);
                byte[] raw = new byte[pendingRead[1] + 12];
                System.arraycopy(stored, 0, raw, 4, Math.min(pendingRead[1], stored.length));
                return Vl53l8Uld.swapBuffer(raw);
            }
            byte[] out = new byte[size];
            for (int i = 0; i < size; i++) {
                out[i] = reg.getOrDefault(addr + i, (byte) 0);
            }
            return out;
        }

        void setU32(int addr, long value, boolean swapped) {
            byte[] b = {(byte) value, (byte) (value >> 8), (byte) (value >> 16), (byte) (value >> 24)};
            if (swapped) {
                b = Vl53l8Uld.swapBuffer(b);
            }
            for (int i = 0; i < 4; i++) {
                reg.put(addr + i, b[i]);
            }
        }

        void setU32(int addr, long value) {
            setU32(addr, value, true);
        }

        void fwReports(int frameSize) {
            byte[] b = new byte[12];
            b[8] = (byte) frameSize;
            b[9] = (byte) (frameSize >> 8);
            dci.put(0x5440, b);
        }

        void zones(int resolution) {
            byte side = (byte) (resolution == 16 ? 4 : 8);
            dci.put(DCI_ZONE_CONFIG, new byte[] {side, side, 0, 0, 0, 0, 0, 0});
        }

        int count(int addr, int size) {
            return Collections.frequency(reads, List.of(addr, size));
        }

        List<Integer> addrs() {
            List<Integer> out = new ArrayList<>();
            for (W w : writes) {
                out.add(w.addr());
            }
            return out;
        }

        boolean wrote(int addr, int firstByte) {
            return writes.stream().anyMatch(w -> w.addr() == addr && w.data()[0] == (byte) firstByte);
        }
    }

    static int u8(byte b) {
        return b & 0xFF;
    }

    static long[] u32s(byte[] b) {
        long[] out = new long[b.length / 4];
        for (int i = 0; i < out.length; i++) {
            out[i] = (b[4 * i] & 0xFFL) | (b[4 * i + 1] & 0xFFL) << 8 | (b[4 * i + 2] & 0xFFL) << 16
                    | (b[4 * i + 3] & 0xFFL) << 24;
        }
        return out;
    }

    static void putU16(byte[] b, int off, int v) {
        b[off] = (byte) v;
        b[off + 1] = (byte) (v >> 8);
    }

    static void putU32(byte[] b, int off, long v) {
        putU16(b, off, (int) v);
        putU16(b, off + 2, (int) (v >> 16));
    }

    static FakePlatform booting(Vl53l8Firmware fw) {
        FakePlatform p = new FakePlatform();
        boolean i2c = fw == Vl53l8Firmware.L7CX || fw == Vl53l8Firmware.L7CH;
        p.reg.put(0x06, (byte) 0x01); // sensor boot / MCU boot
        p.reg.put(0x21, (byte) (i2c ? 0x10 : 0x04));
        if (fw == Vl53l8Firmware.CX) {
            p.setU32(0x2FFC, 0xCADF7CAFL);
        } else if (fw != Vl53l8Firmware.L7CX) {
            p.setU32(0x2FFC, 0x0C0B6C9EL);
        }
        if (i2c) {
            p.reg.put(0x00, (byte) 0xF0);
            p.reg.put(0x01, (byte) 0x02);
        }
        return p;
    }

    static FakePlatform ranging(int resolution) {
        FakePlatform p = new FakePlatform();
        p.zones(resolution);
        return p;
    }

    static final List<W> UNDO = List.of(new W(0x7FFF, new byte[] {0x00}), new W(0x14, new byte[] {0x00}),
            new W(0x15, new byte[] {0x00}), new W(0x09, new byte[] {0x04}), new W(0x7FFF, new byte[] {0x02}));

    static void sameWrites(List<W> want, List<W> got) {
        eq(want.stream().map(W::key).toList(), got.stream().map(W::key).toList(), "writes");
    }

    static <T> List<T> concat(List<T> a, List<T> b) {
        List<T> out = new ArrayList<>(a);
        out.addAll(b);
        return out;
    }

    static int code(Runnable body) {
        return throwsType(Vl53l8Error.class, body::run).code;
    }

    // ── init ─────────────────────────────────────────────────────────────────

    public static void initDownloadsThreeBanksAndUploadsTheConfiguration() {
        FakePlatform p = booting(Vl53l8Firmware.CX);
        Vl53l8Driver drv = new Vl53l8Driver(p, Vl53l8Firmware.CX);
        List<String> notes = new ArrayList<>();
        drv.init(notes::add);
        eq(List.of(List.of(0x09, 0x8000), List.of(0x0A, 0x8000), List.of(0x0B, 0x5000)), p.banks, "banks");
        check(p.writes.stream().filter(w -> w.data().length >= 0x5000).allMatch(w -> w.addr() == 0), "banks at 0");
        List<Integer> addrs = p.addrs();
        check(addrs.indexOf(0x2FD8) < addrs.indexOf(0x2E18), "nvm before offset");
        check(addrs.indexOf(0x2E18) < addrs.indexOf(0x2CF8), "offset before xtalk");
        check(addrs.indexOf(0x2CF8) < addrs.indexOf(0x2C34), "xtalk before config");
        byte[] offset = p.writes.stream().filter(w -> w.addr() == 0x2E18).findFirst().orElseThrow().data();
        eq(Vl53l8Driver.OFFSET_BUFFER_SIZE, offset.length);
        eq(new byte[] {0, 0, 0, 0x0F, 0x03, 0x01, 0x01, (byte) 0xE4},
                Arrays.copyOfRange(offset, offset.length - 8, offset.length));
        eq(Vl53l8Driver.XTALK_BUFFER_SIZE,
                p.writes.stream().filter(w -> w.addr() == 0x2CF8).findFirst().orElseThrow().data().length);
        eq(new byte[] {1, 0, 1, 0}, p.dci.get(DCI_PIPE_CONTROL));
        eq(new byte[] {1, 0, 0, 0}, p.dci.get(DCI_SINGLE_RANGE));
        check(notes.contains("Sensor FW checksum OK"), "checksum note");
        eq("Sensor init done", notes.get(notes.size() - 1));
        eq(null, drv.moduleType());
    }

    public static void initRefusesAWrongFirmwareChecksum() {
        FakePlatform p = booting(Vl53l8Firmware.CX);
        p.setU32(0x2FFC, 0xDEADBEEFL);
        eq(Vl53l8Error.STATUS_FW_CHECKSUM_FAIL, code(() -> new Vl53l8Driver(p, Vl53l8Firmware.CX).init()));
    }

    public static void initTimesOutWhenTheSensorNeverBoots() {
        FakePlatform p = booting(Vl53l8Firmware.CX);
        p.reg.put(0x06, (byte) 0x00);
        eq(Vl53l8Error.STATUS_TIMEOUT_ERROR, code(() -> new Vl53l8Driver(p, Vl53l8Firmware.CX).init()));
        eq(201, p.count(0x06, 1), "2 s at 10 ms, then the timeout");
        eq(List.of(), p.banks);
    }

    public static void initStopsOnAnMcuErrorInTheCommandStatus() {
        FakePlatform p = booting(Vl53l8Firmware.CX);
        p.status = new byte[] {0x02, 0x03, 0x7F, 0x00};
        eq(Vl53l8Error.MCU_ERROR, code(() -> new Vl53l8Driver(p, Vl53l8Firmware.CX).init()));
    }

    public static void l7cxInitChecksSiliconSkipsTheChecksumAndReadsTheModuleType() {
        FakePlatform p = booting(Vl53l8Firmware.L7CX);
        p.dci.put(DCI_FW_FLAGS, new byte[] {0, (byte) Vl53l8Driver.MODULE_TYPE_MZEVO, 0, 0, 0, 0, 0, 0});
        Vl53l8Driver drv = new Vl53l8Driver(p, Vl53l8Firmware.L7CX);
        List<String> notes = new ArrayList<>();
        drv.init(notes::add);
        eq(Vl53l8Driver.MODULE_TYPE_MZEVO, drv.moduleType());
        eq("Sensor init done (module type MZEVO)", notes.get(notes.size() - 1));
        check(!notes.contains("Sensor FW checksum OK"), "no checksum on L7CX");
        eq(0, p.count(0x2FFC, 4));
        check(p.wrote(0x20, 0x07) && p.wrote(0x20, 0x06), "L7 boot writes to 0x20");
    }

    public static void l7cxInitRefusesL8Silicon() {
        FakePlatform p = booting(Vl53l8Firmware.L7CX);
        p.reg.put(0x01, (byte) 0x0C);
        Vl53l8Error e = throwsType(Vl53l8Error.class, () -> new Vl53l8Driver(p, Vl53l8Firmware.L7CX).init());
        eq(Vl53l8Error.STATUS_ERROR, e.code);
        check(e.getMessage().contains("not a VL53L5/L7 sensor"), e.getMessage());
        eq(List.of(), p.banks);
    }

    public static void everyFirmwareSetLoadsAndBoots() {
        for (Vl53l8Firmware fw : Vl53l8Firmware.values()) {
            FakePlatform p = booting(fw);
            Vl53l8Driver drv = new Vl53l8Driver(p, fw);
            drv.init();
            eq(3, p.banks.size(), fw.name());
            eq(fw == Vl53l8Firmware.L7CX || fw == Vl53l8Firmware.L7CH, drv.isI2c(), fw.name());
        }
    }

    // ── start / stop ranging ─────────────────────────────────────────────────

    public static void startRangingSizesTheFrameAndProgramsTheOutputs() {
        Object[][] cases = {
            {Vl53l8Firmware.CX, 64, 1452},
            {Vl53l8Firmware.CX, 16, 540},
            {Vl53l8Firmware.CH, 64, 1444},   // 24-byte tail on the 2.0.x firmware
            {Vl53l8Firmware.L7CX, 16, 1060}, // per-target blocks keep 64 entries
            {Vl53l8Firmware.L7CX, 64, 1444},
        };
        for (Object[] c : cases) {
            Vl53l8Firmware fw = (Vl53l8Firmware) c[0];
            int resolution = (int) c[1];
            int size = (int) c[2];
            String at = fw + "/" + resolution;
            FakePlatform p = ranging(resolution);
            Vl53l8Driver drv = new Vl53l8Driver(p, fw);
            p.fwReports(size);
            drv.startRanging();
            eq(size, drv.dataReadSize(), at);
            eq(null, drv.frameSizeMismatch(), at);
            eq(resolution, drv.resolution(), at);
            eq(new long[] {size, 13}, u32s(p.dci.get(DCI_OUTPUT_CONFIG)), at);
            long w3 = fw == Vl53l8Firmware.CH || fw == Vl53l8Firmware.L7CH ? 0 : 0xC0000000L;
            eq(new long[] {0xFFF, 0, 0, w3}, u32s(p.dci.get(DCI_OUTPUT_ENABLES)), at);
            long[] output = u32s(p.dci.get(DCI_OUTPUT_LIST));
            eq(12, output.length, at);
            Map<Integer, Integer> sizes = new HashMap<>();
            for (long bh : output) {
                sizes.put((int) (bh >> 16), (int) ((bh >> 4) & 0xFFF));
            }
            eq(resolution, sizes.get(0x54D0), at + " ambient");
            eq(drv.isI2c() ? 64 : resolution, sizes.get(0xDF44), at + " distance");
            eq(0x8C, sizes.get(0xD858), at + " motion: not zone-scaled");
        }
    }

    public static void startRangingWithCnhAppendsBlock13() {
        FakePlatform p = ranging(64);
        Vl53l8Driver drv = new Vl53l8Driver(p, Vl53l8Firmware.CH);
        int cnh = 1708;
        int size = 1444 + cnh + 4;
        p.fwReports(size);
        drv.startRanging(cnh);
        long[] output = u32s(p.dci.get(DCI_OUTPUT_LIST));
        eq(13, output.length);
        eq((0xC048L << 16) | ((long) (cnh / 4) << 4) | 4, output[12]);
        eq(0x1FFFL, u32s(p.dci.get(DCI_OUTPUT_ENABLES))[0]);
        eq(new long[] {size, 14}, u32s(p.dci.get(DCI_OUTPUT_CONFIG)));
    }

    public static void startRangingTrustsTheFirmwareFrameSize() {
        FakePlatform p = ranging(64);
        Vl53l8Driver drv = new Vl53l8Driver(p);
        p.fwReports(1448);
        drv.startRanging();
        eq(new int[] {1448, 1452}, drv.frameSizeMismatch());
        eq(1448, drv.dataReadSize());
    }

    public static void startRangingWithoutAResolutionIsRefusedBeforeAnyWrite() {
        FakePlatform p = new FakePlatform();
        p.dci.put(DCI_ZONE_CONFIG, new byte[8]);
        eq(Vl53l8Error.STATUS_INVALID_PARAM, code(() -> new Vl53l8Driver(p).startRanging()));
        check(!p.dci.containsKey(DCI_OUTPUT_LIST), "no output list");
    }

    public static void startRangingReportsALaserSafetyFault() {
        FakePlatform p = ranging(64);
        p.fwReports(1452);
        p.dci.put(DCI_FW_FLAGS, new byte[] {0, 0, 0, 0, 0, 0, 1, 0});
        eq(Vl53l8Error.STATUS_LASER_SAFETY, code(() -> new Vl53l8Driver(p).startRanging()));
    }

    public static void stopRangingProvokesAnMcuStopThenUndoesIt() {
        FakePlatform p = new FakePlatform();
        p.reg.put(0x06, (byte) 0x80);
        p.reg.put(0x07, (byte) 0x84);
        new Vl53l8Driver(p).stopRanging();
        sameWrites(concat(List.of(new W(0x7FFF, new byte[] {0}), new W(0x15, new byte[] {0x16}),
                new W(0x14, new byte[] {0x01})), UNDO), p.writes);
        eq(List.of(0x2FFC, 4), p.reads.get(0), "the auto-stop flag, not swapped");
    }

    public static void stopRangingAfterAutoStopSkipsTheMcuStop() {
        FakePlatform p = new FakePlatform();
        p.setU32(0x2FFC, 0x4FF, false);
        new Vl53l8Driver(p).stopRanging();
        sameWrites(UNDO, p.writes);
    }

    public static void stopRangingGivesUpOnAStuckMcuAndStillUndoes() {
        FakePlatform p = new FakePlatform();
        new Vl53l8Driver(p).stopRanging();
        eq(502, p.count(0x06, 1), "501 polls, then the GO2 status read");
        sameWrites(UNDO, p.writes.subList(p.writes.size() - 5, p.writes.size()));
    }

    // ── poll mode and frame parsing ──────────────────────────────────────────

    static void go2(FakePlatform p, int b0, int b1, int b2, int b3) {
        p.reg.put(0, (byte) b0);
        p.reg.put(1, (byte) b1);
        p.reg.put(2, (byte) b2);
        p.reg.put(3, (byte) b3);
    }

    public static void checkDataReadyFiresOncePerStreamCount() {
        FakePlatform p = new FakePlatform();
        Vl53l8Driver drv = new Vl53l8Driver(p);
        go2(p, 5, 0x05, 0x05, 0x10);
        check(drv.checkDataReady(), "first");
        check(!drv.checkDataReady(), "same stream count");
        go2(p, 6, 0x05, 0x05, 0x10);
        check(drv.checkDataReady(), "next");
        for (int[] g : new int[][] {{255, 5, 5, 0x10}, {7, 4, 5, 0x10}, {7, 5, 4, 0x10}, {7, 5, 5, 0}}) {
            go2(p, g[0], g[1], g[2], g[3]);
            check(!drv.checkDataReady(), Arrays.toString(g));
        }
        go2(p, 8, 0x00, 0x42, 0x80);
        eq(0x42, code(drv::checkDataReady));
    }

    /** A raw frame (as read from register 0) for the programmed output list. */
    static byte[] frame(Vl53l8Driver drv, long[] output, BiFunction<Integer, Integer, byte[]> fill, int footerOff) {
        byte[] buf = new byte[drv.dataReadSize()];
        buf[8] = 0x12;
        buf[9] = 0x34;
        int i = 16;
        for (long bh : output) {
            int t = (int) (bh & 0xF);
            int s = (int) ((bh >> 4) & 0xFFF);
            int idx = (int) (bh >> 16);
            int msize = t > 1 && t < 0xD ? t * s : s;
            putU32(buf, i, bh);
            byte[] body = fill.apply(idx, msize);
            System.arraycopy(body, 0, buf, i + 4, body.length);
            i += 4 + msize;
        }
        buf[buf.length - footerOff] = 0x12;
        buf[buf.length - footerOff + 1] = 0x34;
        return Vl53l8Uld.swapBuffer(buf);
    }

    static byte[] fill4x4(int idx, int msize) {
        byte[] b = new byte[msize];
        switch (idx) {
            case 0xDF44 -> { // distance ×4: 403 → 400, -5 → 0 (negative clamped, as ST), 8 → 2
                putU16(b, 0, 400 * 4 + 3);
                putU16(b, 2, -5);
                for (int k = 2; k < 16; k++) {
                    putU16(b, 2 * k, 8);
                }
            }
            case 0xDEC4 -> { // sigma ×128
                for (int k = 0; k < 16; k++) {
                    putU16(b, 2 * k, 256);
                }
            }
            case 0xDB84 -> { // nb target detected: zone 1 has none
                for (int k = 0; k < 16; k++) {
                    b[k] = (byte) (k == 1 ? 0 : 1);
                }
            }
            case 0xE084 -> Arrays.fill(b, 0, 16, (byte) 5);
            case 0xDBC4 -> { // signal ×2048: zone k carries k
                for (int k = 0; k < 16; k++) {
                    putU32(b, 4 * k, k);
                }
            }
            case 0x54D0 -> { // ambient ×2048: 3 kcps/SPAD in every zone
                for (int k = 0; 4 * k + 4 <= msize; k++) {
                    putU32(b, 4 * k, 3 * 2048);
                }
            }
            case 0xE044 -> Arrays.fill(b, 0, 16, (byte) 41); // reflectance ×2: 41 → 20.5 %
            case 0x54B4 -> b[8] = (byte) 0xF6; // metadata: −10 °C
            default -> {}
        }
        return b;
    }

    public static void getRangingDataReadsTheFrameAndScales() {
        FakePlatform p = ranging(16);
        Vl53l8Driver drv = new Vl53l8Driver(p);
        p.fwReports(540);
        drv.startRanging();
        byte[] raw = frame(drv, u32s(p.dci.get(DCI_OUTPUT_LIST)), Vl53l8DriverTests::fill4x4, 12);
        for (int i = 0; i < raw.length; i++) {
            p.reg.put(i, raw[i]);
        }
        Vl53l8Frame f = drv.getRangingData(0);
        eq(List.of(0, 540), p.reads.get(p.reads.size() - 1));
        eq(16, f.distanceMm().length);
        eq(new int[] {400, 0, 2}, Arrays.copyOf(f.distanceMm(), 3)); // negative → 0, as ST (contract 04 §5)
        eq(2.0, f.rangeSigmaMm()[0]); // raw ÷128, mm
        double[] signal = new double[16];
        for (int k = 0; k < 16; k++) {
            signal[k] = k / 2048.0;
        }
        eq(signal, f.signalPerSpad()); // kcps/SPAD
        eq(3.0, f.ambientPerSpad()[0]); // raw ÷2048, kcps/SPAD
        eq(20.5, f.reflectance()[0]); // raw ÷2, %
        eq(new int[] {5, 255}, Arrays.copyOf(f.targetStatus(), 2));
        eq(-10, f.siliconTempDegc());
        eq(null, f.cnhRaw());
        eq(null, f.motion());
    }

    public static void aFrameWhoseFooterIdDiffersIsCorrupted() {
        FakePlatform p = ranging(16);
        Vl53l8Driver drv = new Vl53l8Driver(p);
        p.fwReports(540);
        drv.startRanging();
        byte[] buf = Vl53l8Uld.swapBuffer(frame(drv, u32s(p.dci.get(DCI_OUTPUT_LIST)), Vl53l8DriverTests::fill4x4, 12));
        buf[buf.length - 12] ^= (byte) 0xFF;
        eq(Vl53l8Error.STATUS_CORRUPTED_FRAME, code(() -> drv.parseFrame(0, Vl53l8Uld.swapBuffer(buf))));
    }

    public static void theMotionBlockIsParsedOnceTheIndicatorIsConfigured() {
        FakePlatform p = ranging(16);
        Vl53l8Driver drv = new Vl53l8Driver(p);
        p.fwReports(540);
        drv.motionIndicatorInit(16);
        drv.startRanging();
        BiFunction<Integer, Integer, byte[]> fill = (idx, msize) -> {
            if (idx != 0xD858) {
                return fill4x4(idx, msize);
            }
            byte[] b = new byte[msize];
            putU32(b, 0, 7);
            putU32(b, 4, 9);
            b[8] = 1;
            b[9] = 2;
            b[10] = 16;
            for (int k = 0; k < 32; k++) {
                putU32(b, 12 + 4 * k, 100 + k);
            }
            return b;
        };
        Vl53l8Motion m = drv.parseFrame(0, frame(drv, u32s(p.dci.get(DCI_OUTPUT_LIST)), fill, 12)).motion();
        check(m != null, "motion parsed");
        eq(List.of(7L, 9L, 1, 2, 16),
                List.of(m.globalIndicator1(), m.globalIndicator2(), m.status(), m.nbOfDetectedAggregates(),
                        m.nbOfAggregates()));
        long[] want = new long[32];
        for (int k = 0; k < 32; k++) {
            want[k] = 100 + k;
        }
        eq(want, m.motion());
    }

    public static void l7FramesAreTrimmedToTheZonesInUse() {
        FakePlatform p = ranging(16);
        Vl53l8Driver drv = new Vl53l8Driver(p, Vl53l8Firmware.L7CX);
        p.fwReports(1060);
        drv.startRanging();
        BiFunction<Integer, Integer, byte[]> fill = (idx, msize) -> {
            byte[] b = new byte[msize];
            if (idx == 0xDF44) {
                for (int k = 0; k < 64; k++) {
                    putU16(b, 2 * k, k < 16 ? 40 : 999);
                }
            }
            if (idx == 0xDB84) {
                Arrays.fill(b, 0, 64, (byte) 1);
            }
            return b;
        };
        Vl53l8Frame f = drv.parseFrame(0, frame(drv, u32s(p.dci.get(DCI_OUTPUT_LIST)), fill, 4));
        int[] tens = new int[16];
        Arrays.fill(tens, 10);
        eq(tens, f.distanceMm());
        eq(16, f.targetStatus().length);
        eq(16, f.nbSpadsEnabled().length);
    }

    // ── power modes ──────────────────────────────────────────────────────────

    public static void powerModes() {
        FakePlatform p = new FakePlatform();
        Vl53l8Driver drv = new Vl53l8Driver(p);
        p.reg.put(0x09, (byte) 0x07);
        eq(Vl53l8Error.STATUS_ERROR, code(drv::getPowerMode));
        W last = p.writes.get(p.writes.size() - 1);
        eq(List.of(0x7FFF, 2), List.of(last.addr(), (int) last.data()[0]), "page restored");

        p.writes.clear();
        p.reg.put(0x09, (byte) 0x04);
        drv.setPowerMode(Vl53l8Driver.POWER_MODE_WAKEUP); // already awake: only the page dance
        sameWrites(List.of(new W(0x7FFF, new byte[] {0}), new W(0x7FFF, new byte[] {2})), p.writes);

        p.reg.put(0x06, (byte) 0x00);
        drv.setPowerMode(Vl53l8Driver.POWER_MODE_DEEP_SLEEP);
        sameWrites(List.of(new W(0x7FFF, new byte[] {0}), new W(0x09, new byte[] {2}), new W(0x0F, new byte[] {0x43}),
                new W(0x7FFF, new byte[] {2})), p.writes.subList(p.writes.size() - 4, p.writes.size()));

        p.reg.put(0x09, (byte) 0x04);
        p.reg.put(0x06, (byte) 0x01); // never clears bit 0
        eq(Vl53l8Error.STATUS_TIMEOUT_ERROR, code(() -> drv.setPowerMode(Vl53l8Driver.POWER_MODE_SLEEP)));
        eq(Vl53l8Error.STATUS_INVALID_PARAM, code(() -> drv.setPowerMode(7)));
    }

    public static void l7cxHasNoDeepSleep() {
        FakePlatform p = new FakePlatform();
        Vl53l8Driver drv = new Vl53l8Driver(p, Vl53l8Firmware.L7CX);
        p.reg.put(0x09, (byte) 0x02);
        p.reg.put(0x0F, (byte) 0x43);
        eq(Vl53l8Driver.POWER_MODE_SLEEP, drv.getPowerMode());
        eq(0, p.count(0x0F, 1));
        eq(Vl53l8Error.STATUS_INVALID_PARAM, code(() -> drv.setPowerMode(Vl53l8Driver.POWER_MODE_DEEP_SLEEP)));
        eq(Vl53l8Error.STATUS_INVALID_PARAM, code(() -> drv.setDetectionThresholdsAutoStop(true)));
    }

    // ── xtalk, thresholds, motion ────────────────────────────────────────────

    public static void xtalkMarginOutOfRangeIsRefused() {
        for (double bad : new double[] {-0.5, 10000.5, Double.NaN}) {
            FakePlatform p = new FakePlatform();
            throwsType(Vl53l8Error.class, () -> new Vl53l8Driver(p).setXtalkMargin(bad));
            check(!p.dci.containsKey(0xAD94), "nothing written for " + bad);
        }
    }

    static byte[] xtalkReply() {
        byte[] b = new byte[1024];
        for (int i = 0; i < b.length; i++) {
            b[i] = (byte) i;
        }
        return b;
    }

    public static void caldataXtalkReadsTheBufferAndRestoresTheResolution() {
        FakePlatform p = ranging(16);
        Vl53l8Driver drv = new Vl53l8Driver(p);
        p.xtalkReply = xtalkReply();
        byte[] blob = drv.getCaldataXtalk();
        check(p.writes.stream().anyMatch(w -> w.addr() == 0x2FB8 && Arrays.equals(w.data(), Vl53l8Tables.getXtalkCmd())),
                "GET_XTALK sent");
        int n = Vl53l8Driver.XTALK_BUFFER_SIZE;
        eq(n, blob.length);
        eq(Arrays.copyOfRange(p.xtalkReply, 8, n), Arrays.copyOf(blob, n - 8));
        eq(new byte[] {0, 0, 0, 0x0F, 0, 0x01, 0x03, 0x04}, Arrays.copyOfRange(blob, n - 8, n));
        eq(blob, drv.xtalkData());
        eq(16, drv.getResolution());
        throwsType(Vl53l8Error.class, () -> drv.setCaldataXtalk(new byte[10]));
    }

    public static void detectionThresholds() {
        FakePlatform p = new FakePlatform();
        Vl53l8Driver drv = new Vl53l8Driver(p);
        throwsType(IllegalArgumentException.class, () -> drv.setDetectionThresholds(
                Collections.nCopies(65, new Vl53l8Uld.DetectionThreshold(0, 0, 1, 0, 0, 0))));
        throwsType(IllegalArgumentException.class, () -> drv.setDetectionThresholds(List.of(
                new Vl53l8Uld.DetectionThreshold(0, Integer.MAX_VALUE / 4 + 1, Vl53l8Uld.DIST_MM, 0, 0, 0))));
        check(!p.dci.containsKey(DCI_THRESH_START), "nothing written");

        drv.setDetectionThresholds(List.of());
        byte[] fives = new byte[8];
        Arrays.fill(fives, (byte) 5);
        eq(fives, p.dci.get(DCI_THRESH_VALID));
        eq(new byte[64 * 12], p.dci.get(DCI_THRESH_START));

        Vl53l8Uld.DetectionThreshold t = new Vl53l8Uld.DetectionThreshold(100, 300, Vl53l8Uld.DIST_MM, 1, 7, 0);
        drv.setDetectionThresholds(List.of(t));
        eq(t, drv.getDetectionThresholds().get(0));
    }

    public static void motionIndicatorWindow() {
        FakePlatform p = new FakePlatform();
        Vl53l8Driver drv = new Vl53l8Driver(p);
        Vl53l8Uld.MotionConfig cfg = drv.motionIndicatorInit(16);
        int[] map = new int[64];
        for (int i = 0; i < 64; i++) {
            map[i] = i < 16 ? i : -1;
        }
        eq(map, cfg.mapId);
        throwsType(Vl53l8Error.class, () -> drv.motionIndicatorSetResolution(cfg, 32));
        for (int[] w : new int[][] {{400, 1901}, {300, 1000}, {3000, 4001}}) {
            throwsType(Vl53l8Error.class, () -> drv.motionIndicatorSetDistanceMotion(cfg, w[0], w[1]));
        }
        drv.motionIndicatorSetDistanceMotion(cfg, 1000, 2500);
        eq((int) ((1000 / 37.5348 - 4.0) * 2048.5), cfg.refBinOffset);
        eq((int) ((150.0 + 30.02784) / 15.01392 + 0.5), cfg.featureLength);
        eq(cfg.pack(), p.dci.get(DCI_MOTION_CFG));
    }

    // ── xtalk calibration ────────────────────────────────────────────────────

    public static void calibrationArgumentsAreCheckedFirst() {
        for (int[] a : new int[][] {{0, 4, 600}, {100, 4, 600}, {16, 0, 600}, {16, 17, 600}, {16, 4, 599}, {16, 4, 3001}}) {
            FakePlatform p = new FakePlatform();
            eq(Vl53l8Error.STATUS_INVALID_PARAM, code(() -> new Vl53l8Driver(p).calibrateXtalk(a[0], a[1], a[2])),
                    Arrays.toString(a));
            eq(List.of(), p.writes);
        }
    }

    static FakePlatform calibration() {
        FakePlatform p = ranging(16);
        p.dci.put(DCI_FREQ_HZ, new byte[] {0, 15, 0, 0});
        p.dci.put(DCI_INT_TIME, Arrays.copyOf(new byte[] {0x20, 0x4E, 0, 0}, 20)); // 20000 µs
        p.dci.put(DCI_RANGING_MODE, new byte[] {0, 3, 0, 2, 0, 0, 0, 0});           // autonomous
        p.dci.put(DCI_TARGET_ORDER, new byte[] {Vl53l8Driver.TARGET_ORDER_CLOSEST, 0, 0, 0});
        p.xtalkReply = xtalkReply();
        return p;
    }

    public static void calibrationRunsTheFirmwareTableAndRestoresTheSettings() {
        for (Vl53l8Firmware fw : new Vl53l8Firmware[] {Vl53l8Firmware.CX, Vl53l8Firmware.CH, Vl53l8Firmware.L7CX}) {
            String at = fw.name();
            FakePlatform p = calibration();
            Vl53l8Driver drv = new Vl53l8Driver(p, fw);
            drv.calibrateXtalk(16, 4, 600);
            byte[] table = switch (fw) {
                case CX -> Vl53l8Tables.calibrateXtalkCx();
                case L7CX -> Vl53l8Tables.calibrateXtalkL7cx();
                default -> Vl53l8Tables.calibrateXtalkLmz();
            };
            List<Integer> addrs = p.addrs();
            check(p.writes.stream().anyMatch(w -> w.addr() == 0x2C28 && Arrays.equals(w.data(), table)), at + " table");
            check(addrs.indexOf(0x2C28) < addrs.indexOf(UI_CMD_END - 3), at + " table before start");
            check(addrs.indexOf(UI_CMD_END - 3) < addrs.indexOf(0x2FB8), at + " start before readout");
            byte[] cal = p.dci.get(DCI_CAL_CFG);
            eq(List.of(2400, 256, 4), List.of(u8(cal[0]) | u8(cal[1]) << 8, u8(cal[2]) | u8(cal[3]) << 8, u8(cal[4])), at);
            int[] want = Vl53l8Tables.xtalkCalOutput();
            long[] got = u32s(p.dci.get(DCI_OUTPUT_LIST));
            eq(want.length, got.length, at);
            for (int k = 0; k < want.length; k++) {
                eq(Integer.toUnsignedLong(want[k]) & ~0xFFF0L, got[k] & ~0xFFF0L, at + " output " + k);
            }
            eq(new long[] {1204, 18}, u32s(p.dci.get(DCI_OUTPUT_CONFIG)), at);
            check(!drv.xtalkCalibrationFailed(), at + " not failed");
            int n = Vl53l8Driver.XTALK_BUFFER_SIZE;
            eq(Arrays.copyOfRange(p.xtalkReply, 8, n), Arrays.copyOf(drv.xtalkData(), n - 8), at);
            check(addrs.indexOf(0x2FB8) < addrs.indexOf(0x2C34), at + " readout before restore");
            eq(16, drv.getResolution(), at);
            eq(15, drv.getRangingFrequencyHz(), at);
            eq(20, drv.getIntegrationTimeMs(), at);
            eq(Vl53l8Driver.RANGING_MODE_AUTONOMOUS, drv.getRangingMode(), at);
            eq(Vl53l8Driver.TARGET_ORDER_CLOSEST, drv.getTargetOrder(), at);
        }
    }

    public static void coverglassTooGoodIsFlaggedNotThrown() {
        FakePlatform p = calibration();
        Vl53l8Driver drv = new Vl53l8Driver(p);
        go2(p, 0x00, 0x00, 0x7F, 0x80);
        drv.calibrateXtalk(16, 4, 600);
        check(drv.xtalkCalibrationFailed(), "flagged");
    }

    public static void calibrationWaitsThenTimesOut() {
        FakePlatform p = calibration();
        p.reg.put(0, (byte) Vl53l8Error.STATUS_ERROR); // still calibrating
        Vl53l8Error e = throwsType(Vl53l8Error.class, () -> new Vl53l8Driver(p).calibrateXtalk(16, 4, 600));
        eq(Vl53l8Error.STATUS_ERROR, e.code);
        check(e.getMessage().contains("xtalk calibration"), e.getMessage());
        eq(401, p.count(0x0, 4), "400 waits of 50 ms, 401 reads");
    }

    public static void sharpenerReadsBackRoundedToNearest() {
        FakePlatform p = new FakePlatform();
        Vl53l8Driver drv = new Vl53l8Driver(p);
        p.dci.put(0xAED8, new byte[16]);
        drv.setSharpenerPercent(25);
        eq(25, drv.getSharpenerPercent());
        throwsType(Vl53l8Error.class, () -> drv.setSharpenerPercent(100));
        throwsType(IllegalArgumentException.class, () -> drv.setSharpenerPercent(-1));
        throwsType(IllegalArgumentException.class, () -> drv.setRangingFrequencyHz(256));
    }
}
