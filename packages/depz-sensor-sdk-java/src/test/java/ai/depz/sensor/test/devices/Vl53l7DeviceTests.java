package ai.depz.sensor.test.devices;

import static ai.depz.sensor.test.devices.T.check;
import static ai.depz.sensor.test.devices.T.eq;
import static ai.depz.sensor.test.devices.T.throwsType;

import ai.depz.sensor.devices.BusyException;
import ai.depz.sensor.devices.DepzDevice;
import ai.depz.sensor.devices.DepzDevices;
import ai.depz.sensor.devices.DepzTimeoutException;
import ai.depz.sensor.devices.InvalidStateException;
import ai.depz.sensor.devices.LinkClosedException;
import ai.depz.sensor.devices.StatusException;
import ai.depz.sensor.devices.Subscription;
import ai.depz.sensor.devices.Vl53l5cxDevice;
import ai.depz.sensor.devices.Vl53l7chDevice;
import ai.depz.sensor.devices.Vl53l7cxDevice;
import ai.depz.sensor.devices.Vl53l8chDevice;
import ai.depz.sensor.devices.Vl53l8cxDevice;
import ai.depz.sensor.protocol.Vl53l7;
import ai.depz.sensor.protocol.Vl53l8;
import ai.depz.sensor.sensors.vl53l8.Vl53l8CnhSetup;
import ai.depz.sensor.sensors.vl53l8.Vl53l8Driver;
import ai.depz.sensor.sensors.vl53l8.Vl53l8Firmware;
import ai.depz.sensor.sensors.vl53l8.Vl53l8Frame;
import ai.depz.sensor.sensors.vl53l8.Vl53l8Platform;
import ai.depz.sensor.sensors.vl53l8.Vl53l8Uld;
import ai.depz.sensor.test.devices.Vl53l8DriverTests.FakePlatform;
import ai.depz.sensor.usb.UsbIds;
import java.lang.reflect.Field;
import java.time.Duration;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.Map;
import java.util.function.BiFunction;
import java.util.function.Function;
import java.util.logging.Handler;
import java.util.logging.Level;
import java.util.logging.LogRecord;
import java.util.logging.Logger;

/**
 * The VL53L8CX / CH and VL53L5CX / L7CX / L7CH device classes over the wire against {@link FakeVl53l8Board}
 * — the Python {@code test_vl53l7_device.py} and the device-level half of {@code test_vl53l8_device.py}:
 * init over each ULD branch, the transfer splits, the board commands and their refusals, the
 * configuration passthroughs, ranging, frames, the parse / reassembly counters and the CNH arming.
 */
public final class Vl53l7DeviceTests {
    private static final Duration TIMEOUT = Duration.ofMillis(500);
    static final int L7_8X8_FRAME = 1444; // contract 11 §2: one 1536-byte chunk
    static final int L7_4X4_FRAME = 1060;
    static final int L8_8X8_FRAME = 1452;
    static final int L8_4X4_FRAME = 540;

    static final class Rig implements AutoCloseable {
        final Vl53l8cxDevice dev;
        final FakeVl53l8Board board;
        final FakePlatform p;

        Rig(Vl53l8cxDevice dev, FakeVl53l8Board board) {
            this.dev = dev;
            this.board = board;
            this.p = board.p;
        }

        @Override
        public void close() {
            dev.close();
            board.link.close();
        }
    }

    /** A booting sensor of {@code fw} behind the matching board, reporting {@code moduleType} (L5/L7). */
    static FakeVl53l8Board board(Vl53l8Firmware fw, int moduleType) {
        FakePlatform p = Vl53l8DriverTests.booting(fw);
        boolean i2c = fw == Vl53l8Firmware.L7CX || fw == Vl53l8Firmware.L7CH;
        if (i2c) {
            p.dci.put(Vl53l8DriverTests.DCI_FW_FLAGS, new byte[] {0, (byte) moduleType, 0, 0, 0, 0, 0, 0});
        } else {
            p.reg.put(0x00, (byte) 0xF0); // L8 silicon ids
            p.reg.put(0x01, (byte) 0x0C);
        }
        return new FakeVl53l8Board(p, i2c);
    }

    static Rig l7() {
        FakeVl53l8Board b = board(Vl53l8Firmware.L7CX, Vl53l8Driver.MODULE_TYPE_MZEVO);
        return new Rig(new Vl53l7cxDevice(b.link, TIMEOUT), b);
    }

    static Rig l5() {
        FakeVl53l8Board b = board(Vl53l8Firmware.L7CX, Vl53l8Driver.MODULE_TYPE_MZ);
        return new Rig(new Vl53l5cxDevice(b.link, TIMEOUT), b);
    }

    static Rig l7ch() {
        FakeVl53l8Board b = board(Vl53l8Firmware.L7CH, Vl53l8Driver.MODULE_TYPE_MZEVO);
        return new Rig(new Vl53l7chDevice(b.link, TIMEOUT), b);
    }

    static Rig cx() {
        FakeVl53l8Board b = board(Vl53l8Firmware.CX, 0);
        return new Rig(new Vl53l8cxDevice(b.link, TIMEOUT), b);
    }

    static Rig ch() {
        FakeVl53l8Board b = board(Vl53l8Firmware.CH, 0);
        return new Rig(new Vl53l8chDevice(b.link, TIMEOUT), b);
    }

    /** init(), then the zone config and firmware frame size the ULD reads before ranging at {@code resolution}. */
    static void ready(Rig r, int resolution, int frameSize) {
        r.dev.init();
        r.p.zones(resolution);
        r.p.fwReports(frameSize);
        r.board.clearRx();
    }

    static Vl53l8Platform platform(Vl53l8cxDevice dev) throws Exception {
        Field f = Vl53l8Driver.class.getDeclaredField("p");
        f.setAccessible(true);
        return (Vl53l8Platform) f.get(dev.uld());
    }

    /** A raw frame for the programmed output list: every zone's distance {@code mm}. */
    static byte[] frame(Rig r, int mm) {
        BiFunction<Integer, Integer, byte[]> fill = (idx, msize) -> {
            byte[] b = new byte[msize];
            if (idx == 0xDF44) {
                for (int k = 0; 2 * k + 2 <= msize; k++) {
                    Vl53l8DriverTests.putU16(b, 2 * k, mm * 4);
                }
            }
            if (idx == 0xDB84) {
                Arrays.fill(b, 0, Math.min(64, msize), (byte) 1);
            }
            return b;
        };
        int footer = r.dev.firmware() == Vl53l8Firmware.CX ? Vl53l8Uld.FOOTER_ID_OFF_CX : Vl53l8Uld.FOOTER_ID_OFF_CH;
        return Vl53l8DriverTests.frame(r.dev.uld(), Vl53l8DriverTests.u32s(r.p.dci.get(Vl53l8DriverTests.DCI_OUTPUT_LIST)), fill, footer);
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

    /** Run {@code body} with the L7 class logger captured; returns the warnings logged. */
    static List<String> warnings(T.Body body) throws Exception {
        Logger log = Logger.getLogger(Vl53l7cxDevice.class.getName());
        List<String> out = new ArrayList<>();
        Handler h = new Handler() {
            @Override
            public void publish(LogRecord record) {
                if (record.getLevel().intValue() >= Level.WARNING.intValue()) {
                    out.add(record.getMessage());
                }
            }

            @Override
            public void flush() {}

            @Override
            public void close() {}
        };
        boolean parent = log.getUseParentHandlers();
        log.addHandler(h);
        log.setUseParentHandlers(false);
        try {
            body.run();
        } finally {
            log.removeHandler(h);
            log.setUseParentHandlers(parent);
        }
        return out;
    }

    // ── identity ─────────────────────────────────────────────────────────────

    public static void openOnLinkResolvesTheClassByPid() throws Exception {
        Object[][] cases = {{0xED48, Vl53l5cxDevice.class}, {0xED49, Vl53l7cxDevice.class}, {0xED4A, Vl53l7chDevice.class}};
        for (Object[] c : cases) {
            FakeVl53l8Board b = board(Vl53l8Firmware.L7CX, Vl53l8Driver.MODULE_TYPE_MZEVO);
            try (DepzDevice dev = DepzDevices.openOnLink(b.link, UsbIds.DEPZ_USB_VID, (int) c[0], TIMEOUT)) {
                eq(c[1], dev.getClass());
                Vl53l7cxDevice l7 = (Vl53l7cxDevice) dev;
                eq(FakeVl53l8Board.L7_SOFTWARE, l7.getSoftwareName());
                check(l7.isAlive(), "alive");
                eq(null, l7.moduleType()); // readable only after init()
                check(!l7.xtalkCalibrationFailed(), "nothing calibrated");
            } finally {
                b.link.close();
            }
        }
    }

    public static void openOnLinkFallsBackToThePartInTheDeviceName() throws Exception {
        FakeVl53l8Board b = board(Vl53l8Firmware.L7CX, Vl53l8Driver.MODULE_TYPE_MZEVO);
        try (DepzDevice dev = DepzDevices.openOnLink(b.link)) { // a dev board: no production PID
            eq(Vl53l7cxDevice.class, dev.getClass()); // "… VL53L7CX USB v2.1 …"
        }
    }

    public static void isAliveRefusesL8SiliconOnAnL7BoardAndForeignOrSilentSiliconOnAnL8() throws Exception {
        try (Rig r = l7()) {
            r.p.reg.put(0x01, (byte) 0x0C);
            check(!r.dev.isAlive(), "L8 silicon on an L7 board");
            r.p.reg.put(0x01, (byte) 0x02);
            check(r.dev.isAlive(), "L7 silicon");
        }
        try (Rig r = cx()) {
            check(r.dev.isAlive(), "L8 silicon");
            r.p.reg.put(0x00, (byte) 0x12);
            check(!r.dev.isAlive(), "foreign silicon");
            r.p.reg.put(0x00, (byte) 0xF0);
            r.board.failNext(FakeVl53l8Board.READ_REG, 0x09);
            check(!r.dev.isAlive(), "a bus error");
        }
    }

    public static void uldAndFirmwareNeedInit() throws Exception {
        try (Rig r = cx()) {
            InvalidStateException e = throwsType(InvalidStateException.class, r.dev::uld);
            check(e.getMessage().contains("call init() first"), e.getMessage());
            throwsType(InvalidStateException.class, r.dev::firmware);
            throwsType(InvalidStateException.class, r.dev::getResolution);
            check(!r.dev.ranging(), "not ranging");
            eq(0L, r.dev.frameParseErrors());
            eq(0, r.dev.reassemblerDiscards());
        }
    }

    public static void everyClassOpensOnAPlainLinkWithTheDefaultTimeout() throws Exception {
        Object[][] cases = {
            {Vl53l8Firmware.L7CX, (Function<FakeVl53l8Board, Vl53l8cxDevice>) b -> new Vl53l7cxDevice(b.link)},
            {Vl53l8Firmware.L7CX, (Function<FakeVl53l8Board, Vl53l8cxDevice>) b -> new Vl53l5cxDevice(b.link)},
            {Vl53l8Firmware.L7CH, (Function<FakeVl53l8Board, Vl53l8cxDevice>) b -> new Vl53l7chDevice(b.link)},
            {Vl53l8Firmware.CX, (Function<FakeVl53l8Board, Vl53l8cxDevice>) b -> new Vl53l8cxDevice(b.link)},
            {Vl53l8Firmware.CH, (Function<FakeVl53l8Board, Vl53l8cxDevice>) b -> new Vl53l8chDevice(b.link)},
        };
        for (Object[] c : cases) {
            FakeVl53l8Board b = board((Vl53l8Firmware) c[0], Vl53l8Driver.MODULE_TYPE_MZEVO);
            @SuppressWarnings("unchecked")
            Function<FakeVl53l8Board, Vl53l8cxDevice> make = (Function<FakeVl53l8Board, Vl53l8cxDevice>) c[1];
            try (Rig r = new Rig(make.apply(b), b)) {
                eq(DepzDevice.DEFAULT_TIMEOUT, r.dev.timeout(), c[0].toString());
                check(r.dev.isAlive(), c[0] + " alive");
                check(!r.dev.ranging(), c[0] + " idle");
            }
        }
    }

    // ── init: the L5/L7 ULD branch and the L8 one ────────────────────────────

    public static void initDownloadsTheBlobOfTheClass() throws Exception {
        Object[][] cases = {
            {Vl53l8Firmware.L7CX, Vl53l8Driver.MODULE_TYPE_MZEVO, (Function<FakeVl53l8Board, Vl53l8cxDevice>) b -> new Vl53l7cxDevice(b.link, TIMEOUT), false, "MZEVO"},
            {Vl53l8Firmware.L7CX, Vl53l8Driver.MODULE_TYPE_MZ, (Function<FakeVl53l8Board, Vl53l8cxDevice>) b -> new Vl53l5cxDevice(b.link, TIMEOUT), false, "MZ"},
            {Vl53l8Firmware.L7CH, Vl53l8Driver.MODULE_TYPE_MZEVO, (Function<FakeVl53l8Board, Vl53l8cxDevice>) b -> new Vl53l7chDevice(b.link, TIMEOUT), true, "MZEVO"},
            {Vl53l8Firmware.CX, 0, (Function<FakeVl53l8Board, Vl53l8cxDevice>) b -> new Vl53l8cxDevice(b.link, TIMEOUT), true, null},
            {Vl53l8Firmware.CH, 0, (Function<FakeVl53l8Board, Vl53l8cxDevice>) b -> new Vl53l8chDevice(b.link, TIMEOUT), true, null},
        };
        for (Object[] c : cases) {
            Vl53l8Firmware fw = (Vl53l8Firmware) c[0];
            FakeVl53l8Board b = board(fw, (int) c[1]);
            @SuppressWarnings("unchecked")
            Function<FakeVl53l8Board, Vl53l8cxDevice> make = (Function<FakeVl53l8Board, Vl53l8cxDevice>) c[2];
            List<String> notes = new ArrayList<>();
            List<int[]> progress = new ArrayList<>();
            try (Rig r = new Rig(make.apply(b), b)) {
                List<String> warned = warnings(() -> r.dev.init(notes::add, (done, total) -> progress.add(new int[] {done, total})));
                eq(List.of(), warned, fw.name());
                eq(fw, r.dev.firmware(), fw.name());
                eq((boolean) c[3], notes.contains("Sensor FW checksum OK"), fw.name() + " checksum");
                check(!progress.isEmpty(), fw.name() + " write progress on the firmware banks");
                eq(List.of(0x8000, 0x8000, 0x5000), List.of(r.board.bankBytes.get(0x09), r.board.bankBytes.get(0x0A), r.board.bankBytes.get(0x0B)), fw.name());
                eq(progress.get(progress.size() - 1)[0], progress.get(progress.size() - 1)[1], fw.name() + " progress completes");
                if (c[4] != null) {
                    Vl53l7cxDevice l7 = (Vl53l7cxDevice) r.dev;
                    eq((int) c[1], l7.moduleType(), fw.name());
                    eq("Sensor init done (module type " + c[4] + ")", notes.get(notes.size() - 1), fw.name());
                }
                r.p.zones(Vl53l8Driver.RESOLUTION_4X4);
                eq(Vl53l8Driver.RESOLUTION_4X4, r.dev.getResolution(), fw.name());
            }
        }
    }

    public static void initRefusesNonL5L7SiliconBeforeDownloading() throws Exception {
        try (Rig r = l7()) {
            r.p.reg.put(0x01, (byte) 0x0C); // an L8 wired to the I2C board
            Vl53l8Uld.Vl53l8Error e = throwsType(Vl53l8Uld.Vl53l8Error.class, r.dev::init);
            check(e.getMessage().contains("not a VL53L5/L7 sensor: device_id=0xF0 revision_id=0x0C"), e.getMessage());
            eq(Map.of(), r.board.bankBytes); // no firmware written
            eq(null, ((Vl53l7cxDevice) r.dev).moduleType());
            throwsType(InvalidStateException.class, r.dev::uld);
        }
    }

    public static void initWarnsWhenTheModuleTypeContradictsTheClass() throws Exception {
        Object[][] cases = {
            {Vl53l8Driver.MODULE_TYPE_MZ, (Function<FakeVl53l8Board, Vl53l8cxDevice>) b -> new Vl53l7cxDevice(b.link, TIMEOUT), "Vl53l7cxDevice expects module type MZEVO, the sensor reports MZ", Vl53l8Firmware.L7CX},
            {Vl53l8Driver.MODULE_TYPE_MZEVO, (Function<FakeVl53l8Board, Vl53l8cxDevice>) b -> new Vl53l5cxDevice(b.link, TIMEOUT), "Vl53l5cxDevice expects module type MZ, the sensor reports MZEVO", Vl53l8Firmware.L7CX},
            {Vl53l8Driver.MODULE_TYPE_MZ, (Function<FakeVl53l8Board, Vl53l8cxDevice>) b -> new Vl53l7chDevice(b.link, TIMEOUT), "Vl53l7chDevice expects module type MZEVO, the sensor reports MZ", Vl53l8Firmware.L7CH},
            {Vl53l8Driver.MODULE_TYPE_MZPLUS, (Function<FakeVl53l8Board, Vl53l8cxDevice>) b -> new Vl53l7cxDevice(b.link, TIMEOUT), "the sensor reports MZPlus", Vl53l8Firmware.L7CX},
            {7, (Function<FakeVl53l8Board, Vl53l8cxDevice>) b -> new Vl53l7cxDevice(b.link, TIMEOUT), "expects module type MZEVO, the sensor reports 7 ", Vl53l8Firmware.L7CX},
        };
        for (Object[] c : cases) {
            int reported = (int) c[0];
            @SuppressWarnings("unchecked")
            Function<FakeVl53l8Board, Vl53l8cxDevice> make = (Function<FakeVl53l8Board, Vl53l8cxDevice>) c[1];
            FakeVl53l8Board b = board((Vl53l8Firmware) c[3], reported);
            try (Rig r = new Rig(make.apply(b), b)) {
                List<String> warned = warnings(r.dev::init);
                eq(1, warned.size(), (String) c[2]);
                check(warned.get(0).contains((String) c[2]) && warned.get(0).contains("check the board's device name"), warned.get(0));
                // a warning, not an error: the blob is shared, ranging works
                eq(reported, ((Vl53l7cxDevice) r.dev).moduleType());
                r.dev.setResolution(Vl53l8Driver.RESOLUTION_8X8);
                eq(Vl53l8Driver.RESOLUTION_8X8, r.dev.getResolution());
            }
        }
    }

    // ── transfer limits and the 1 Hz floor ───────────────────────────────────

    public static void registerReadsSplitAt1536BytesOnTheI2cBoardAnd2048OnTheSpiOne() throws Exception {
        try (Rig r = l7()) {
            r.dev.init();
            int n0 = r.p.reads.size();
            byte[] data = platform(r.dev).readMulti(0x0000, 4000);
            eq(4000, data.length);
            eq(List.of(List.of(0x0000, 1536), List.of(0x0600, 1536), List.of(0x0C00, 928)), r.p.reads.subList(n0, r.p.reads.size()));
            int w0 = r.p.writes.size();
            platform(r.dev).writeMulti(0x1000, new byte[3000]);
            eq(List.of(2048, 952), r.p.writes.subList(w0, r.p.writes.size()).stream().map(w -> w.data().length).toList());
        }
        try (Rig r = cx()) {
            r.dev.init();
            int n0 = r.p.reads.size();
            platform(r.dev).readMulti(0x0000, 4000);
            eq(List.of(List.of(0x0000, 2048), List.of(0x0800, 1952)), r.p.reads.subList(n0, r.p.reads.size()));
        }
    }

    public static void theI2cBridgeRefusesAReadAbove1536Bytes() throws Exception {
        try (Rig r = l7()) {
            StatusException e = throwsType(StatusException.class, () -> r.dev.request(Vl53l8.Vl53l8Cmd.READ_REG.value,
                    Vl53l8.packReadReg(0x0000, 2048), DepzDevice.expectReport(Vl53l8.Vl53l8Rpt.REG_DATA.value, Vl53l8.RegData::unpack)));
            eq("ERR_INVALID_PARAM", e.statusName());
        }
    }

    public static void aShortRegisterAnswerIsAnError() throws Exception {
        try (Rig r = cx()) {
            r.dev.init();
            r.p.xtalkReply = new byte[4]; // 4 bytes where the 776-byte buffer is expected
            r.p.zones(16);
            // the fake answers the UI_CMD_START read with what it has: shorter than asked
            try {
                r.dev.getCaldataXtalk();
                throw new AssertionError("a short read must not pass");
            } catch (RuntimeException e) {
                check(e.getMessage().contains("expected") && e.getMessage().contains("got"), e.getMessage());
            }
        }
    }

    public static void rangingAt1HzIsAllowedOnL7AndRefusedOnL8() throws Exception {
        try (Rig r = l7()) {
            ready(r, 64, L7_8X8_FRAME);
            r.dev.setRangingFrequencyHz(1); // L8 refuses this (contract 04 §4); L5/L7 range at 1 Hz
            eq(1, r.dev.getRangingFrequencyHz());
            throwsType(IllegalArgumentException.class, () -> r.dev.setRangingFrequencyHz(0));
        }
        try (Rig r = cx()) {
            ready(r, 64, L8_8X8_FRAME);
            IllegalArgumentException e = throwsType(IllegalArgumentException.class, () -> r.dev.setRangingFrequencyHz(1));
            check(e.getMessage().contains(">= 2 Hz"), e.getMessage());
            r.dev.setRangingFrequencyHz(15);
            eq(15, r.dev.getRangingFrequencyHz());
        }
    }

    public static void rangingFrequencyCeilingFollowsTheResolution() throws Exception {
        // contract 11 §3 (ERRATA A13): at most 60 Hz at 4x4 and 15 Hz at 8x8, refused host-side — nothing on the bus
        eq(60, Vl53l8cxDevice.maxRangingFrequencyHz(16));
        eq(15, Vl53l8cxDevice.maxRangingFrequencyHz(64));
        for (String which : new String[] {"l7", "cx"}) {
            try (Rig r = "cx".equals(which) ? cx() : l7()) {
                ready(r, 16, "cx".equals(which) ? L8_4X4_FRAME : L7_4X4_FRAME);
                r.dev.setRangingFrequencyHz(60);
                eq(60, r.dev.getRangingFrequencyHz(), which);
                int w0 = r.p.writes.size();
                IllegalArgumentException e = throwsType(IllegalArgumentException.class, () -> r.dev.setRangingFrequencyHz(61));
                check(e.getMessage().contains("<= 60 Hz"), e.getMessage());
                eq(w0, r.p.writes.size(), which + ": refused before the bus");
                r.dev.setResolution(64);
                r.dev.setRangingFrequencyHz(15);
                int w1 = r.p.writes.size();
                e = throwsType(IllegalArgumentException.class, () -> r.dev.setRangingFrequencyHz(16));
                check(e.getMessage().contains("<= 15 Hz"), e.getMessage());
                eq(w1, r.p.writes.size(), which + ": refused before the bus");
                eq(15, r.dev.getRangingFrequencyHz(), which);
                r.dev.setResolution(16);
                r.dev.setRangingFrequencyHz(16); // the ceiling moved back with the resolution
            }
        }
    }

    public static void framesStreamInOneChunkOnTheI2cBoard() throws Exception {
        Object[][] cases = {{"l7", 16, L7_4X4_FRAME}, {"l7", 64, L7_8X8_FRAME}, {"l7ch", 64, L7_8X8_FRAME}};
        for (Object[] c : cases) {
            int resolution = (int) c[1];
            int size = (int) c[2];
            try (Rig r = "l7".equals(c[0]) ? l7() : l7ch()) {
                String at = c[0] + "/" + resolution;
                ready(r, resolution, size);
                r.dev.setResolution(resolution);
                r.dev.setRangingFrequencyHz(1);
                List<Vl53l8Frame> got = new ArrayList<>();
                AutoCloseable unsub = r.dev.onFrame(got::add);
                Subscription<Vl53l8Frame> q = r.dev.frames(4);
                r.dev.startRanging();
                check(r.dev.ranging(), at);
                eq(size, r.board.frameSize, at);
                eq(size, r.dev.uld().dataReadSize(), at);
                byte[] raw = frame(r, 40);
                r.board.sendFrame(raw, 777, Vl53l7.STREAM_CHUNK_MAX);
                eq(1, r.board.chunksSent.get(), at + " raw " + raw.length + " chunk " + Vl53l7.STREAM_CHUNK_MAX);
                Vl53l8Frame f = q.take(Duration.ofSeconds(2));
                eq(resolution, f.resolution(), at);
                eq(resolution, f.distanceMm().length, at);
                eq(40, f.distanceMm()[0], at);
                eq(777L, f.timestampUs(), at);
                T.until(() -> got.size() == 1, 1000);
                eq(1, got.size(), at + " " + got.stream().map(x -> x.timestampUs() + "/" + x.distanceMm()[0]).toList());
                later(() -> r.board.sendFrame(frame(r, 41), 778, Vl53l7.STREAM_CHUNK_MAX));
                eq(41, r.dev.getFrame(Duration.ofSeconds(2)).distanceMm()[0], at); // the next frame, contract 07 §3
                q.close();
                unsub.close();
                r.dev.stopRanging();
                check(!r.dev.ranging() && !r.board.streaming, at + " stopped");
                eq(2L, r.board.intEdges.get(), at); // the two frames the board pushed
                eq(0L, r.dev.frameParseErrors(), at);
                eq(0, r.dev.reassemblerDiscards(), at);
                r.board.chunksSent.set(0);
            }
        }
    }

    public static void l8FramesStreamIn1528ByteChunksAndSubscribersGetThem() throws Exception {
        try (Rig r = cx()) {
            ready(r, 64, L8_8X8_FRAME);
            r.dev.setResolution(64);
            try (Subscription<Vl53l8Frame> q = r.dev.frames()) {
                eq(List.of(0L), r.dev.streamDroppedCounts());
                r.dev.startRanging();
                eq(L8_8X8_FRAME, r.board.frameSize);
                r.board.sendFrame(frame(r, 123), 5, Vl53l8.STREAM_CHUNK_MAX);
                eq(1, r.board.chunksSent.get()); // 1452 bytes fit one chunk
                Vl53l8Frame f = q.take(Duration.ofSeconds(2));
                eq(64, f.resolution());
                eq(123, f.distanceMm()[63]);
                // a frame split in two chunks reassembles
                r.board.sendFrame(frame(r, 50), 6, 1000);
                eq(3, r.board.chunksSent.get());
                eq(50, q.take(Duration.ofSeconds(2)).distanceMm()[0]);
                eq(0, r.dev.reassemblerDiscards());
            }
            eq(List.of(), r.dev.streamDroppedCounts());
            r.dev.stopRanging();
        }
    }

    public static void aCorruptedFooterCountsAsAParseErrorAndAGapAsADiscard() throws Exception {
        try (Rig r = cx()) {
            ready(r, 16, L8_4X4_FRAME);
            Subscription<Vl53l8Frame> q = r.dev.frames(4);
            r.dev.startRanging();
            byte[] raw = frame(r, 10);
            byte[] bad = Vl53l8Uld.swapBuffer(raw);
            bad[bad.length - Vl53l8Uld.FOOTER_ID_OFF_CX] ^= (byte) 0xFF;
            r.board.sendFrame(Vl53l8Uld.swapBuffer(bad), 1, Vl53l8.STREAM_CHUNK_MAX);
            T.until(() -> r.dev.frameParseErrors() == 1, 1000);
            eq(1L, r.dev.frameParseErrors());
            // the first chunk of a frame, then a fresh frame: the half one is discarded
            r.board.sendChunk(raw, 2, 0, 300, raw.length);
            r.board.sendFrame(raw, 3, Vl53l8.STREAM_CHUNK_MAX);
            eq(10, q.take(Duration.ofSeconds(2)).distanceMm()[0]);
            eq(1, r.dev.reassemblerDiscards());
            // shorter than a chunk header: not a frame at all
            r.board.link.inject(r.board.link.reply(Vl53l8.Vl53l8Rpt.VL53_FRAME.value, new byte[11]));
            r.dev.getSoftwareName(); // a round trip behind it
            eq(0, q.size());
            eq(1L, r.dev.frameParseErrors());
        }
    }

    public static void framesBeforeInitAreDroppedSilently() throws Exception {
        try (Rig r = cx()) {
            List<Vl53l8Frame> got = new ArrayList<>();
            r.dev.onFrame(got::add);
            byte[] raw = new byte[L8_4X4_FRAME];
            r.board.sendFrame(raw, 1, Vl53l8.STREAM_CHUNK_MAX);
            r.dev.getSoftwareName(); // a round trip behind the frame
            eq(0, got.size());
            eq(0L, r.dev.frameParseErrors());
        }
    }

    public static void getFrameTimesOutAndEndsWhenTheDeviceCloses() throws Exception {
        try (Rig r = cx()) {
            throwsType(DepzTimeoutException.class, () -> r.dev.getFrame(Duration.ofMillis(20)));
            later(r.dev::close);
            throwsType(LinkClosedException.class, () -> r.dev.getFrame(Duration.ofSeconds(2)));
        }
    }

    public static void startStreamRejectedByTheBridgeStopsTheSensorAgainAndStopKeepsItsPromise() throws Exception {
        try (Rig r = cx()) {
            ready(r, 16, L8_4X4_FRAME);
            // what the ULD stop sends on the bus: a normal stop past its STOP_STREAM
            r.dev.startRanging();
            r.board.clearRx();
            r.dev.stopRanging();
            List<Integer> stop = r.board.cmds();
            eq(FakeVl53l8Board.STOP_STREAM, stop.get(0));
            List<Integer> uldStop = stop.subList(1, stop.size());
            check(!uldStop.isEmpty(), "the ULD stop talks to the sensor");
            r.board.clearRx();
            r.board.failNext(FakeVl53l8Board.START_STREAM, 0x03);
            StatusException e = throwsType(StatusException.class, r.dev::startRanging);
            eq(0x03, e.status());
            check(!r.dev.ranging(), "not ranging");
            List<Integer> cmds = r.board.cmds();
            int start = cmds.lastIndexOf(FakeVl53l8Board.START_STREAM);
            // the ULD started the sensor, so it is stopped again
            eq(uldStop, cmds.subList(start + 1, cmds.size()));
            r.dev.stopRanging(); // not ranging: a no-op
            r.dev.startRanging();
            r.board.failNext(FakeVl53l8Board.STOP_STREAM, 0x01);
            throwsType(StatusException.class, r.dev::stopRanging);
            check(!r.dev.ranging(), "the flag clears even when the ack fails");
            r.dev.setResolution(16); // configuration allowed again
        }
    }

    public static void configurationIsRefusedWhileRanging() throws Exception {
        try (Rig r = ch()) {
            ready(r, 16, L8_4X4_FRAME);
            r.dev.startRanging();
            Vl53l8chDevice ch = (Vl53l8chDevice) r.dev;
            List<T.Body> calls = List.of(r.dev::init, () -> r.dev.setResolution(64), () -> r.dev.setRangingFrequencyHz(15),
                    () -> r.dev.setRangingMode(Vl53l8Driver.RANGING_MODE_AUTONOMOUS), () -> r.dev.setIntegrationTimeMs(20),
                    () -> r.dev.setSharpenerPercent(5), () -> r.dev.setTargetOrder(Vl53l8Driver.TARGET_ORDER_STRONGEST),
                    () -> r.dev.setPowerMode(Vl53l8Driver.POWER_MODE_SLEEP), () -> r.dev.setXtalkMargin(50),
                    () -> r.dev.calibrateXtalk(16, 4, 600), r.dev::getCaldataXtalk, () -> r.dev.setCaldataXtalk(new byte[776]),
                    () -> r.dev.setDetectionThresholdsEnable(true), () -> r.dev.setDetectionThresholds(List.of()),
                    () -> r.dev.setDetectionThresholdsAutoStop(true), () -> r.dev.configureMotionIndicator(),
                    () -> ch.configureCnh(new Vl53l8CnhSetup()), r.dev::startRanging);
            int before = r.board.cmds().size();
            for (T.Body call : calls) {
                InvalidStateException e = throwsType(InvalidStateException.class, call);
                check(e.getMessage().contains("stopRanging() first"), e.getMessage());
            }
            eq(before, r.board.cmds().size()); // nothing reached the bus
            r.dev.stopRanging();
        }
    }

    public static void theConfigurationPassthroughsReadBackWhatWasSet() throws Exception {
        try (Rig r = cx()) {
            ready(r, 16, L8_4X4_FRAME);
            throwsType(IllegalArgumentException.class, () -> r.dev.setResolution(32));
            r.dev.setResolution(Vl53l8Driver.RESOLUTION_8X8);
            eq(64, r.dev.getResolution());
            r.dev.setRangingMode(Vl53l8Driver.RANGING_MODE_AUTONOMOUS);
            eq(Vl53l8Driver.RANGING_MODE_AUTONOMOUS, r.dev.getRangingMode());
            r.dev.setIntegrationTimeMs(20);
            eq(20, r.dev.getIntegrationTimeMs());
            r.p.dci.put(0xAED8, new byte[16]);
            r.dev.setSharpenerPercent(25);
            eq(25, r.dev.getSharpenerPercent());
            r.dev.setTargetOrder(Vl53l8Driver.TARGET_ORDER_STRONGEST);
            eq(Vl53l8Driver.TARGET_ORDER_STRONGEST, r.dev.getTargetOrder());
            r.dev.setXtalkMargin(50);
            eq(50, r.dev.getXtalkMargin());
            r.dev.setDetectionThresholdsEnable(true);
            eq(1, r.dev.getDetectionThresholdsEnable());
            Vl53l8Uld.DetectionThreshold t = new Vl53l8Uld.DetectionThreshold(100, 300, Vl53l8Uld.DIST_MM, 1, 7, 0);
            r.dev.setDetectionThresholds(List.of(t));
            eq(t, r.dev.getDetectionThresholds().get(0));
            r.dev.setDetectionThresholdsAutoStop(true);
            Vl53l8Uld.MotionConfig cfg = r.dev.configureMotionIndicator();
            eq(cfg.pack(), r.p.dci.get(Vl53l8DriverTests.DCI_MOTION_CFG));
            eq((int) ((400 / 37.5348 - 4.0) * 2048.5), cfg.refBinOffset);
            r.p.reg.put(0x09, (byte) 0x04);
            eq(Vl53l8Driver.POWER_MODE_WAKEUP, r.dev.getPowerMode());
            r.dev.setPowerMode(Vl53l8Driver.POWER_MODE_WAKEUP); // already awake
            r.p.xtalkReply = Vl53l8DriverTests.xtalkReply();
            byte[] blob = r.dev.getCaldataXtalk();
            eq(Vl53l8Driver.XTALK_BUFFER_SIZE, blob.length);
            r.dev.setCaldataXtalk(blob);
            throwsType(Vl53l8Uld.Vl53l8Error.class, () -> r.dev.setCaldataXtalk(new byte[10]));
        }
    }

    public static void l7cxHasSleepButNoDeepSleepAndNoThresholdAutoStop() throws Exception {
        try (Rig r = l7()) {
            ready(r, 16, L7_4X4_FRAME);
            r.p.reg.put(0x06, (byte) 0x00);
            r.dev.setPowerMode(Vl53l8Driver.POWER_MODE_SLEEP);
            eq(Vl53l8Driver.POWER_MODE_SLEEP, r.dev.getPowerMode());
            check(!r.p.reads.contains(List.of(0x000F, 1)), "ULD 2.0.1 never reads the deep-sleep marker");
            eq(Vl53l8Uld.Vl53l8Error.STATUS_INVALID_PARAM, throwsType(Vl53l8Uld.Vl53l8Error.class,
                    () -> r.dev.setPowerMode(Vl53l8Driver.POWER_MODE_DEEP_SLEEP)).code);
            eq(Vl53l8Uld.Vl53l8Error.STATUS_INVALID_PARAM, throwsType(Vl53l8Uld.Vl53l8Error.class,
                    () -> r.dev.setDetectionThresholdsAutoStop(true)).code);
        }
    }

    public static void xtalkCalibrationFailedFlag() throws Exception {
        try (Rig r = l7()) {
            Vl53l7cxDevice l7 = (Vl53l7cxDevice) r.dev;
            check(!l7.xtalkCalibrationFailed(), "before init(): nothing calibrated");
            ready(r, 16, L7_4X4_FRAME);
            r.p.dci.put(Vl53l8DriverTests.DCI_FREQ_HZ, new byte[] {0, 15, 0, 0});
            r.p.dci.put(Vl53l8DriverTests.DCI_INT_TIME, Arrays.copyOf(new byte[] {0x20, 0x4E, 0, 0}, 20));
            r.p.dci.put(Vl53l8DriverTests.DCI_RANGING_MODE, new byte[] {0, 3, 0, 2, 0, 0, 0, 0});
            r.p.dci.put(Vl53l8DriverTests.DCI_TARGET_ORDER, new byte[] {Vl53l8Driver.TARGET_ORDER_CLOSEST, 0, 0, 0});
            r.p.xtalkReply = Vl53l8DriverTests.xtalkReply();
            r.dev.calibrateXtalk(50, 4, 600);
            check(!l7.xtalkCalibrationFailed(), "calibrated");
            eq(Vl53l8Driver.XTALK_BUFFER_SIZE, r.dev.getCaldataXtalk().length);
            Vl53l8DriverTests.go2(r.p, 0x00, 0x00, 0x7F, 0x80); // "coverglass too good"
            r.dev.calibrateXtalk(20, 1, 3000);
            check(l7.xtalkCalibrationFailed(), "flagged, not thrown");
        }
    }

    // ── board commands (contract 11 §2) ──────────────────────────────────────

    public static void getBridgeInfoDecodesTheUnechoedReport() throws Exception {
        try (Rig r = l7()) {
            Vl53l7cxDevice l7 = (Vl53l7cxDevice) r.dev;
            eq(new Vl53l7.Vl53l7Info(0, 0, 0, Vl53l7.I2C_ERR_OK, 1, 0, 1000, 0, false), l7.getBridgeInfo());
            ready(r, 64, L7_8X8_FRAME);
            r.dev.setResolution(64);
            r.dev.startRanging();
            later(() -> r.board.sendFrame(frame(r, 40), 1, Vl53l7.STREAM_CHUNK_MAX));
            r.dev.getFrame(Duration.ofSeconds(2));
            Vl53l7.Vl53l7Info info = l7.getBridgeInfo();
            eq(List.of(1L, L7_8X8_FRAME, true, 1), List.of(info.intEdges(), info.frameSize(), info.streaming(), info.intLevel()));
            eq(List.of(1, 1000, 0L, 0), List.of(info.lpnLevel(), info.i2cKhz(), info.i2cErrors(), info.lastI2cError()));
        }
    }

    public static void setI2cSpeedSnapsToTheNearestStepAndReturnsIt() throws Exception {
        try (Rig r = l7()) {
            Vl53l7cxDevice l7 = (Vl53l7cxDevice) r.dev;
            eq(400, l7.setI2cSpeedKhz(430));
            eq(1000, l7.setI2cSpeedKhz(960));
            eq(100, l7.setI2cSpeedKhz(1));
            eq(1000, l7.setI2cSpeedKhz(65535));
            eq(List.of(430, 960, 1, 65535), r.board.speedRequests);
            eq(1000, l7.getBridgeInfo().i2cKhz());
            for (int khz : new int[] {0, 65536, -1}) {
                IllegalArgumentException e = throwsType(IllegalArgumentException.class, () -> l7.setI2cSpeedKhz(khz));
                check(e.getMessage().contains("khz must be 1..65535"), e.getMessage());
            }
            eq(4, r.board.speedRequests.size());
            // the speed is a board setting: init() afterwards runs at it
            l7.setI2cSpeedKhz(400);
            r.dev.init();
            eq(400, l7.getBridgeInfo().i2cKhz());
        }
    }

    public static void setI2cSpeedIsBusyWhileStreaming() throws Exception {
        try (Rig r = l7()) {
            Vl53l7cxDevice l7 = (Vl53l7cxDevice) r.dev;
            ready(r, 16, L7_4X4_FRAME);
            r.dev.startRanging();
            throwsType(BusyException.class, () -> l7.setI2cSpeedKhz(400));
            eq(1000, r.board.i2cKhz);
            r.dev.stopRanging();
            eq(400, l7.setI2cSpeedKhz(400));
        }
    }

    public static void lpnOffDropsTheSensorStateAndNeedsInit() throws Exception {
        try (Rig r = l7()) {
            Vl53l7cxDevice l7 = (Vl53l7cxDevice) r.dev;
            ready(r, 16, L7_4X4_FRAME);
            r.dev.startRanging();
            l7.pinCtrl(Vl53l7.PinAction.LPN_OFF);
            eq(List.of(Vl53l7.PinAction.LPN_OFF.value), r.board.pinActions);
            check(!r.board.streaming, "stream stopped by the board");
            check(!r.dev.ranging(), "not ranging");
            eq(null, l7.moduleType());
            InvalidStateException e = throwsType(InvalidStateException.class, r.dev::getResolution);
            check(e.getMessage().contains("call init"), e.getMessage()); // nothing goes to the bus: the host knows the sensor is gone
            // LPn low: the sensor NACKs — the bridge reports a hardware fault and counts it
            check(!r.dev.isAlive(), "not alive");
            Vl53l7.Vl53l7Info info = l7.getBridgeInfo();
            eq(List.of(0, 1L, Vl53l7.I2C_ERR_NACK), List.of(info.lpnLevel(), info.i2cErrors(), info.lastI2cError()));
            l7.pinCtrl(Vl53l7.PinAction.LPN_ON);
            check(r.dev.isAlive(), "alive again");
            eq(null, l7.moduleType()); // LPN_ON restores the bus, not the firmware
            r.dev.init();
            eq(Vl53l8Driver.MODULE_TYPE_MZEVO, l7.moduleType());
        }
    }

    public static void i2cRstKeepsTheSensorState() throws Exception {
        try (Rig r = l7()) {
            Vl53l7cxDevice l7 = (Vl53l7cxDevice) r.dev;
            ready(r, 16, L7_4X4_FRAME);
            r.dev.setResolution(64);
            l7.pinCtrl(Vl53l7.PinAction.I2C_RST);
            eq(List.of(Vl53l7.PinAction.I2C_RST.value), r.board.pinActions);
            eq(Vl53l8Driver.MODULE_TYPE_MZEVO, l7.moduleType());
            eq(64, r.dev.getResolution()); // the ULD is still valid
        }
    }

    public static void softCycleStopsTheStreamClearsTheI2cCountersAndNeedsInit() throws Exception {
        try (Rig r = l7()) {
            Vl53l7cxDevice l7 = (Vl53l7cxDevice) r.dev;
            ready(r, 16, L7_4X4_FRAME);
            r.dev.startRanging();
            r.board.i2cErrors = 3;
            r.board.lastI2cError = Vl53l7.I2C_ERR_TIMEOUT;
            l7.pinCtrl(Vl53l7.PinAction.SOFT_CYCLE);
            eq(List.of(Vl53l7.PinAction.SOFT_CYCLE.value), r.board.pinActions);
            check(!r.board.streaming && !r.dev.ranging(), "stopped");
            eq(null, l7.moduleType());
            Vl53l7.Vl53l7Info info = l7.getBridgeInfo();
            eq(List.of(false, 1, 0L, Vl53l7.I2C_ERR_OK), List.of(info.streaming(), info.lpnLevel(), info.i2cErrors(), info.lastI2cError()));
            r.dev.init();
            eq(Vl53l8Driver.MODULE_TYPE_MZEVO, l7.moduleType());
        }
    }

    public static void aMalformedPinActionIsRefusedByTheBoard() throws Exception {
        try (Rig r = l7()) {
            StatusException e = throwsType(StatusException.class, () -> r.dev.requestOk(FakeVl53l8Board.PIN_CTRL, new byte[] {7}));
            eq("ERR_INVALID_PARAM", e.statusName());
            e = throwsType(StatusException.class, () -> r.dev.requestOk(FakeVl53l8Board.PIN_CTRL, new byte[2]));
            eq("ERR_PAYLOAD_FORMAT", e.statusName());
            eq(List.of(), r.board.pinActions);
        }
    }

    // ── CNH (the CH classes) ─────────────────────────────────────────────────

    public static void theChClassesArmCnhForTheNextStart() throws Exception {
        try (Rig r = ch()) {
            Vl53l8chDevice ch = (Vl53l8chDevice) r.dev;
            eq(null, ch.cnhSetup());
            ready(r, 64, L8_8X8_FRAME);
            throwsType(NullPointerException.class, () -> ch.configureCnh(null));
            throwsType(RuntimeException.class, () -> ch.configureCnh(new Vl53l8CnhSetup())); // blank: refused before anything is sent
            eq(null, ch.cnhSetup());
            Vl53l8CnhSetup setup = new Vl53l8CnhSetup();
            setup.initConfig(10, 24, 1);
            setup.createAggMap(64, 0, 0, 2, 2, 4, 4);
            ch.configureCnh(setup);
            eq(setup, ch.cnhSetup());
            eq(setup.pack(), r.p.dci.get(Vl53l8CnhSetup.MI_CFG_DEV_IDX));
            int size = 1444 + setup.requiredMemory() + 4;
            r.p.fwReports(size);
            r.dev.startRanging();
            eq(size, r.board.frameSize);
            eq(13, Vl53l8DriverTests.u32s(r.p.dci.get(Vl53l8DriverTests.DCI_OUTPUT_LIST)).length);
            r.dev.stopRanging();
        }
        try (Rig r = l7ch()) {
            Vl53l7chDevice ch = (Vl53l7chDevice) r.dev;
            eq(null, ch.cnhSetup());
            ready(r, 64, L7_8X8_FRAME);
            Vl53l8CnhSetup setup = new Vl53l8CnhSetup();
            setup.initConfig(10, 24, 1);
            setup.createAggMap(64, 0, 0, 2, 2, 4, 4);
            ch.configureCnh(setup);
            eq(setup, ch.cnhSetup());
        }
    }

    /** True when the programmed output list carries the CNH block (index 0xC048). */
    static boolean cnhInOutputList(Rig r) {
        for (long bh : Vl53l8DriverTests.u32s(r.p.dci.get(Vl53l8DriverTests.DCI_OUTPUT_LIST))) {
            if ((bh >> 16) == 0xC048) {
                return true;
            }
        }
        return false;
    }

    /** One frame of the running stream. */
    static Vl53l8Frame take(Rig r) throws Exception {
        try (Subscription<Vl53l8Frame> q = r.dev.frames()) {
            r.board.sendFrame(frame(r, 500), 1, Vl53l8.STREAM_CHUNK_MAX);
            return q.take(Duration.ofSeconds(2));
        }
    }

    // CNH and the motion indicator share DCI 0xBFAC (contract 04 §7, ERRATA B4): configuring one
    // disarms the other, init() disarms both.
    public static void cnhAndTheMotionIndicatorDisarmEachOther() throws Exception {
        try (Rig r = ch()) {
            Vl53l8chDevice ch = (Vl53l8chDevice) r.dev;
            ready(r, 64, L8_8X8_FRAME);
            Vl53l8CnhSetup setup = new Vl53l8CnhSetup();
            setup.initConfig(10, 20, 2);
            setup.createAggMap(64, 0, 0, 2, 2, 4, 4);
            int cnhFrame = L8_8X8_FRAME - 8 + setup.requiredMemory() + 4; // the CH tail is 8 bytes shorter

            // motion, then CNH: 0xBFAC holds the CNH setup, frames carry no motion
            r.dev.configureMotionIndicator();
            ch.configureCnh(setup);
            eq(setup.pack(), r.p.dci.get(Vl53l8CnhSetup.MI_CFG_DEV_IDX));
            r.p.fwReports(cnhFrame);
            r.dev.startRanging();
            check(cnhInOutputList(r), "CNH block programmed");
            Vl53l8Frame f = take(r);
            eq(null, f.motion());
            eq(setup.requiredMemory(), f.cnhRaw().length);
            r.dev.stopRanging();

            // CNH, then motion: 0xBFAC holds the motion config, the CNH block is gone
            r.dev.configureMotionIndicator();
            eq(null, ch.cnhSetup());
            r.p.fwReports(L8_8X8_FRAME - 8);
            r.dev.startRanging();
            check(!cnhInOutputList(r), "no CNH block after the motion indicator");
            f = take(r);
            check(f.motion() != null, "motion parsed");
            eq(null, f.cnhRaw());
            r.dev.stopRanging();

            // init() disarms both
            ch.configureCnh(setup);
            // The platform fake has one flat register map (no pages): the first session wrote over
            // the boot status (0x06) and the firmware checksum (0x2FFC). Re-seed them as a booting
            // sensor, then init again.
            r.p.reg.putAll(Vl53l8DriverTests.booting(Vl53l8Firmware.CH).reg);
            r.dev.init();
            eq(null, ch.cnhSetup());
            r.dev.startRanging();
            check(!cnhInOutputList(r), "no CNH block after init()");
            f = take(r);
            eq(null, f.motion());
            eq(null, f.cnhRaw());
            r.dev.stopRanging();
            eq(0L, r.dev.frameParseErrors());
        }
    }
}
