package ai.depz.sensor.test.devices;

import static ai.depz.sensor.test.devices.T.check;
import static ai.depz.sensor.test.devices.T.eq;
import static ai.depz.sensor.test.devices.T.same;
import static ai.depz.sensor.test.devices.T.throwsType;

import ai.depz.sensor.devices.DepzException;
import ai.depz.sensor.devices.DepzTimeoutException;
import ai.depz.sensor.devices.InvalidStateException;
import ai.depz.sensor.devices.StatusException;
import ai.depz.sensor.devices.Subscription;
import ai.depz.sensor.devices.Tmf8829AppliedConfig;
import ai.depz.sensor.devices.Tmf8829Device;
import ai.depz.sensor.devices.Tmf8829Exception;
import ai.depz.sensor.devices.Tmf8829InterruptEvent;
import ai.depz.sensor.protocol.Common;
import ai.depz.sensor.protocol.Tmf8829;
import ai.depz.sensor.sensors.tmf8829.CaptureTimer;
import ai.depz.sensor.sensors.tmf8829.Frames;
import ai.depz.sensor.sensors.tmf8829.MeasurementBuilder;
import ai.depz.sensor.sensors.tmf8829.Tmf8829Config;
import ai.depz.sensor.sensors.tmf8829.Tmf8829Firmware;
import ai.depz.sensor.test.Json;
import ai.depz.sensor.transport.Packet;
import java.io.ByteArrayOutputStream;
import java.nio.file.Files;
import java.nio.file.Path;
import java.time.Duration;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.HashSet;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.TreeSet;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.function.IntFunction;

/**
 * The TMF8829 device class over a fake SPI register bridge + sensor (the C# Tmf8829DeviceTests.cs, itself
 * the Python tests/test_tmf8829_device.py over fake_tmf8829.py).
 */
public final class Tmf8829DeviceTests {
    private static final int W = 0x33;
    private static final int R = 0x32;

    /**
     * Fake TMF8829 SPI register bridge + sensor: the bridge answers READ_REG / WRITE_REG / START_STREAM /
     * STOP_STREAM / GET_INFO; the sensor is a 256-byte register file with the side effects the host sequence
     * relies on — soft reset → bootloader, bootloader commands at CMD_STAT, the firmware download through FIFO
     * 0xFF, application commands (presets, LOAD_CONFIG_PAGE, WRITE_PAGE, MEASURE, STOP) and the config page
     * with the "preset does not reset everything" behaviour.
     */
    static final class FakeTmf8829 {
        private static final int REG_APP_ID = 0x00, REG_CMD_STAT = 0x08, REG_SERIAL = 0x1C, REG_CID_RID = 0x20,
                REG_CFG = 0x22, CFG_SIZE = 190;
        private static final int REG_INT_STATUS = 0xE1, REG_ID = 0xE3, REG_RESET = 0xF7, REG_ENABLE = 0xF8,
                REG_FIFO_STATUS = 0xFA, REG_FIFO = 0xFF;

        // fp_mode each preset command loads
        private static final Map<Integer, Integer> PRESET_FP_MODE = Map.of(
                0x40, 0, 0x41, 0, 0x42, 0, 0x43, 2, 0x44, 2, 0x45, 3, 0x46, 3, 0x47, 5, 0x48, 5, 0x49, 0);

        final FakeBoardLink link = new FakeBoardLink();
        final byte[] regs = new byte[256];
        final byte[] cfg = new byte[CFG_SIZE];
        final ByteArrayOutputStream fifo = new ByteArrayOutputStream();
        byte[] wFifoParams;
        String mode = "app-old";
        boolean i2cOff;

        // fault injection
        volatile int deviceId = 0x9E;
        final Map<Integer, Integer> blStatus = new HashMap<>();
        final Map<Integer, Integer> appStatus = new HashMap<>();
        final AtomicInteger busyPolls = new AtomicInteger();
        volatile boolean stuckBeforeBootloader;
        volatile Integer cidOverride;
        final Map<Integer, Integer> stuckBytes = new HashMap<>();
        final Map<Integer, Integer> bridgeStatus = new HashMap<>();
        final Set<Integer> silent = new HashSet<>();
        volatile int shortRead;
        /** Packets sent after a CMD_STAT write is applied, before its WRITE_REG reply. */
        volatile IntFunction<List<byte[]>> onCommand;
        volatile boolean streaming;
        volatile int intMask;
        volatile int flags;

        private int pendingCmd;
        private int frameSeq;

        FakeTmf8829() {
            regs[REG_APP_ID] = 0x01; // an application left running by an earlier session
            cfg[0] = 33;
            cfg[2] = (byte) (586 & 0xFF);
            cfg[3] = (byte) (586 >> 8);
            cfg[0x2A - REG_CFG] = 1;
            link.onPacket = this::handle;
        }

        List<byte[]> sent(int cmd) {
            List<byte[]> out = new ArrayList<>();
            for (Packet p : link.sentSnapshot()) {
                if (p.cmd() == cmd) {
                    out.add(p.payload().clone());
                }
            }
            return out;
        }

        List<int[]> writes() {
            List<int[]> out = new ArrayList<>();
            for (byte[] p : sent(W)) {
                out.add(ints(p));
            }
            return out;
        }

        int logCount() {
            return link.sentCount();
        }

        List<Packet> logFrom(int n) {
            List<Packet> all = link.sentSnapshot();
            return new ArrayList<>(all.subList(n, all.size()));
        }

        synchronized int emitFrame(byte[] raw, long timestampUs, long edgeUs, Integer seq, int[] chunks) {
            int s = seq != null ? seq : frameSeq;
            frameSeq = (s + 1) & 0xFF;
            int[] offsets = chunks;
            if (offsets == null) {
                offsets = new int[(raw.length + 1023) / 1024];
                for (int i = 0; i < offsets.length; i++) {
                    offsets[i] = i * 1024;
                }
            }
            for (int i = 0; i < offsets.length; i++) {
                int off = offsets[i];
                int end = i + 1 < offsets.length ? offsets[i + 1] : raw.length;
                int len = Math.min(end, off + 1024) - off;
                byte[] p = new byte[21 + len];
                putLe(p, 0, timestampUs, 8);
                putLe(p, 8, raw.length, 2);
                putLe(p, 10, off, 2);
                p[12] = (byte) s;
                putLe(p, 13, edgeUs, 8);
                System.arraycopy(raw, off, p, 21, len);
                link.inject(link.reply(0x93, p));
            }
            return s;
        }

        int emitFrame(byte[] raw, long timestampUs, long edgeUs) {
            return emitFrame(raw, timestampUs, edgeUs, null, null);
        }

        byte[] event(int code, int intStatus, int frameId, int fifoStatus) {
            byte[] p = new byte[12];
            putLe(p, 0, 987654321L, 8);
            p[8] = (byte) intStatus;
            p[9] = (byte) code;
            p[10] = (byte) frameId;
            p[11] = (byte) fifoStatus;
            return link.reply(0x94, p);
        }

        void emitEvent(int code, int intStatus, int frameId) {
            link.inject(event(code, intStatus, frameId, 0));
        }

        private synchronized List<byte[]> handle(Packet pkt) {
            int cmd = pkt.cmd();
            byte[] payload = pkt.payload();
            if (silent.contains(cmd)) {
                return List.of();
            }
            Integer bs = bridgeStatus.get(cmd);
            if (bs != null) {
                return List.of(FakeBoardLink.status(cmd, bs));
            }
            switch (cmd) {
                case 0x32: {
                    int addr = payload[0] & 0xFF;
                    int n = (payload[1] & 0xFF) | (payload[2] & 0xFF) << 8;
                    int m = Math.max(0, n - shortRead);
                    byte[] p = new byte[9 + m];
                    p[0] = 0x32;
                    putLe(p, 1, 5_000_000L, 8);
                    for (int i = 0; i < m; i++) {
                        p[9 + i] = (byte) read8(Math.min(addr + i, REG_FIFO));
                    }
                    return List.of(link.reply(0x91, p));
                }
                case 0x33: {
                    writeRegs(payload[0] & 0xFF, Arrays.copyOfRange(payload, 1, payload.length));
                    List<byte[]> out = new ArrayList<>();
                    IntFunction<List<byte[]>> oc = onCommand;
                    if ((payload[0] & 0xFF) == REG_CMD_STAT && oc != null) {
                        out.addAll(oc.apply(payload[1] & 0xFF));
                    }
                    out.add(FakeBoardLink.status(cmd, Common.Status.OK.value));
                    return out;
                }
                case 0x35:
                    streaming = true;
                    intMask = payload[0] & 0xFF;
                    flags = payload[1] & 0xFF;
                    frameSeq = 0;
                    return List.of(FakeBoardLink.status(cmd, Common.Status.OK.value));
                case 0x36:
                    streaming = false;
                    return List.of(FakeBoardLink.status(cmd, Common.Status.OK.value));
                case 0x37: {
                    byte[] p = new byte[25];
                    putLe(p, 0, 7, 4);
                    putLe(p, 4, 6, 4);
                    putLe(p, 8, 1, 4);
                    p[20] = 1;
                    p[21] = (byte) (streaming ? 1 : 0);
                    p[22] = (byte) intMask;
                    p[23] = 0x01;
                    p[24] = 0x00;
                    return List.of(link.reply(0x92, p));
                }
                default:
                    return List.of(FakeBoardLink.status(cmd, Common.Status.ERR_INVALID_CMD.value));
            }
        }

        private int read8(int a) {
            if (a == REG_ID) {
                return deviceId;
            }
            if (a == REG_ENABLE) {
                // 0 over SPI until the sensor's I2C is off
                return mode.equals("bootloader") && !i2cOff ? 0 : (regs[a] & 0xFF) | 0x80;
            }
            if (a == REG_CMD_STAT && busyPolls.getAndUpdate(n -> n > 0 ? n - 1 : 0) > 0) {
                return pendingCmd;
            }
            return regs[a] & 0xFF;
        }

        private void writeRegs(int addr, byte[] data) {
            if (addr == REG_FIFO) {
                fifo.writeBytes(data);
                return;
            }
            if (addr == REG_CMD_STAT) {
                command(data);
                return;
            }
            for (int i = 0; i < data.length; i++) {
                int a = addr + i;
                byte b = data[i];
                if (a == REG_RESET && (b & 0x40) != 0) {
                    softReset();
                } else if (a == REG_INT_STATUS) {
                    regs[a] &= (byte) ~b; // write-1-to-clear
                } else if (a == REG_FIFO_STATUS) {
                    regs[a] &= (byte) ~(b & 0x30);
                } else {
                    regs[a] = b;
                }
            }
        }

        private void softReset() {
            mode = "bootloader";
            i2cOff = false;
            regs[REG_APP_ID] = (byte) (stuckBeforeBootloader ? 0x00 : 0x80);
            regs[REG_CMD_STAT] = 0;
            fifo.reset();
            cfg[0] = 33;
            cfg[1] = 0;
            cfg[0x2A - REG_CFG] = 1;
        }

        private void command(byte[] data) {
            int cmd = data[0] & 0xFF;
            pendingCmd = cmd;
            int st;
            if (mode.equals("bootloader")) {
                st = blStatus.getOrDefault(cmd, 0);
                if (st == 0) {
                    if (cmd == 0x22) {
                        i2cOff = true;
                    } else if (cmd == 0x45) {
                        wFifoParams = Arrays.copyOfRange(data, 2, 2 + (data[1] & 0xFF));
                    } else if (cmd == 0x16) {
                        mode = "app";
                        regs[REG_APP_ID] = 0x01;
                        regs[1] = 2;
                        regs[2] = (byte) 200;
                        regs[3] = 0;
                        putLe(regs, REG_SERIAL, 0x12345678L, 4);
                    }
                }
            } else {
                st = appStatus.getOrDefault(cmd, cmd == 0x10 ? 0x01 : 0x00);
                if (st <= 0x01) {
                    appCommand(cmd);
                }
            }
            regs[REG_CMD_STAT] = (byte) st;
        }

        private void appCommand(int cmd) {
            Integer fp = PRESET_FP_MODE.get(cmd);
            if (fp != null) {
                cfg[0x26 - REG_CFG] = (byte) (int) fp;
                regs[REG_CID_RID] = (byte) cmd;
            } else if (cmd == 0x16) {
                System.arraycopy(cfg, 0, regs, REG_CFG, CFG_SIZE);
                regs[REG_CID_RID] = (byte) (cidOverride != null ? (int) cidOverride : 0x16);
            } else if (cmd == 0x15) {
                System.arraycopy(regs, REG_CFG, cfg, 0, CFG_SIZE);
                for (Map.Entry<Integer, Integer> kv : stuckBytes.entrySet()) {
                    cfg[kv.getKey()] = (byte) (int) kv.getValue();
                }
            }
        }

        private static void putLe(byte[] b, int at, long v, int n) {
            for (int i = 0; i < n; i++) {
                b[at + i] = (byte) (v >>> (8 * i));
            }
        }
    }

    // ── helpers ──────────────────────────────────────────────────────────────

    private static int[] ints(byte[] b) {
        int[] out = new int[b.length];
        for (int i = 0; i < b.length; i++) {
            out[i] = b[i] & 0xFF;
        }
        return out;
    }

    private static byte[] bytes(int... v) {
        byte[] out = new byte[v.length];
        for (int i = 0; i < v.length; i++) {
            out[i] = (byte) v[i];
        }
        return out;
    }

    private static int[][] grid(List<int[]> l) {
        return l.toArray(new int[0][]);
    }

    private static byte[][] grid2(List<byte[]> l) {
        return l.toArray(new byte[0][]);
    }

    private static boolean containsWrite(List<int[]> writes, int... want) {
        return writes.stream().anyMatch(x -> Arrays.equals(x, want));
    }

    private record Made(FakeTmf8829 fake, Tmf8829Device dev) {}

    private static Made make() {
        FakeTmf8829 fake = new FakeTmf8829();
        return new Made(fake, new Tmf8829Device(fake.link, Duration.ofMillis(500)));
    }

    private static <X> void waitFor(List<X> list, int n) {
        T.until(() -> {
            synchronized (list) {
                return list.size() >= n;
            }
        }, 2000);
        synchronized (list) {
            eq(n, list.size(), "items");
        }
    }

    private static void sleep(long ms) {
        try {
            Thread.sleep(ms);
        } catch (InterruptedException e) {
            Thread.currentThread().interrupt();
        }
    }

    // ── vectors: contracts/vectors/tmf8829.json ──────────────────────────────

    /**
     * The vectors dir: the {@code depz.vectors} system property, else {@code contracts/vectors} found walking
     * up from the working directory (../../contracts/vectors from packages/depz-sensor-sdk-java).
     */
    private static Path vectorsDir() {
        String prop = System.getProperty("depz.vectors");
        if (prop != null && !prop.isEmpty()) {
            return Path.of(prop);
        }
        for (Path d = Path.of("").toAbsolutePath(); d != null; d = d.getParent()) {
            Path v = d.resolve("contracts/vectors");
            if (Files.isRegularFile(v.resolve("tmf8829.json"))) {
                return v;
            }
        }
        return Path.of("../../contracts/vectors");
    }

    private static Map<String, Object> vectors;
    private static Map<String, byte[]> raw;
    private static Map<String, Map<String, Object>> meas;
    private static Set<String> corrupt; // results frames whose pixel decode fails

    @SuppressWarnings("unchecked")
    private static Map<String, Object> obj(Object o) {
        return (Map<String, Object>) o;
    }

    @SuppressWarnings("unchecked")
    private static List<Object> arr(Object o) {
        return (List<Object>) o;
    }

    private static long lng(Object o) {
        return ((Long) o).longValue();
    }

    private static byte[] hex(String h) {
        int n = h.length() / 2;
        byte[] out = new byte[n];
        for (int i = 0; i < n; i++) {
            out[i] = (byte) Integer.parseInt(h.substring(i * 2, i * 2 + 2), 16);
        }
        return out;
    }

    private static synchronized void loadVectors() {
        if (vectors != null) {
            return;
        }
        try {
            vectors = obj(Json.parse(Files.readString(vectorsDir().resolve("tmf8829.json"))));
        } catch (java.io.IOException e) {
            throw new IllegalStateException("tmf8829.json: " + e.getMessage(), e);
        }
        raw = new LinkedHashMap<>();
        corrupt = new HashSet<>();
        for (Object o : arr(vectors.get("frames"))) {
            Map<String, Object> c = obj(o);
            raw.put((String) c.get("name"), hex((String) c.get("raw")));
            if (c.containsKey("error")) corrupt.add((String) c.get("name"));
        }
        meas = new LinkedHashMap<>();
        for (Object o : arr(vectors.get("measurements"))) {
            Map<String, Object> m = obj(o);
            meas.put((String) m.get("name"), m);
        }
    }

    private static byte[] raw(String name) {
        loadVectors();
        return raw.get(name);
    }

    private static void play(FakeTmf8829 fake, String name) {
        loadVectors();
        for (Object o : arr(meas.get(name).get("frames"))) {
            Map<String, Object> f = obj(o);
            fake.emitFrame(raw.get((String) f.get("raw_case")), lng(f.get("timestamp_us")), lng(f.get("edge_us")));
        }
    }

    private static long[] flat(int[] a) {
        return a == null ? null : Arrays.stream(a).asLongStream().toArray();
    }

    /**
     * A measurement in the vector's field shape (the C# VectorShapes.Tmf8829Meas; the same fields RunVectors
     * compares for tmf8829.json "measurements", replicated here as RunVectors keeps them inline).
     */
    private static Map<String, Object> measFields(MeasurementBuilder.Measurement m) {
        Map<String, Object> got = new LinkedHashMap<>();
        got.put("timestamp_us", m.timestampUs());
        got.put("flags", (long) m.flags());
        got.put("fp_mode", (long) m.fpMode());
        got.put("f_number", m.fNumber());
        got.put("frame_times_us", m.times().stream().mapToLong(CaptureTimer.FrameTime::timestampUs).toArray());
        got.put("t1_len_us", m.times().stream().mapToLong(CaptureTimer.FrameTime::t1LenUs).toArray());
        // the device test takes the ratio the measurement carries (no shadow builder's clock)
        got.put("ratio_q16", m.times().get(m.times().size() - 1).ratioQ16());
        got.put("n_histograms", (long) m.histograms().size());
        got.put("n_ref_frames", (long) m.refFrames().size());
        got.put("n_problems", (long) m.problems().size());
        got.put("distance_in_mm", m.frames().get(m.frames().size() - 1).distanceInMm());
        Frames.PixelResults r = m.results();
        int[] shape = r.shape();
        got.put("peaks", (long) r.peaks());
        got.put("rows", (long) shape[0]);
        got.put("cols", (long) shape[1]);
        got.put("distance", flat(r.distance()));
        got.put("snr", flat(r.snr()));
        got.put("signal", flat(r.signal()));
        got.put("noise", flat(r.noise()));
        got.put("xtalk", flat(r.xtalk()));
        return got;
    }

    /** The C# Tmf8829Tests.AssertFields: the same field set, every value equal. */
    private static void assertFields(String name, Map<String, Object> expect, Map<String, Object> got) {
        eq(new TreeSet<>(expect.keySet()).toString(), new TreeSet<>(got.keySet()).toString(), name + ": field set");
        for (Map.Entry<String, Object> kv : got.entrySet()) {
            String what = name + "." + kv.getKey();
            Object e = expect.get(kv.getKey());
            Object v = kv.getValue();
            if (v == null) {
                check(e == null, what + ": expected " + e + ", got null");
            } else if (v instanceof Boolean b) {
                eq(e, b, what);
            } else if (v instanceof long[] a) {
                check(e instanceof List, what + ": expected " + e + ", got a list");
                long[] want = arr(e).stream().mapToLong(x -> lng(x)).toArray();
                check(Arrays.equals(want, a), what + ": list differs (len " + want.length + " vs " + a.length + ")");
            } else {
                eq(e, ((Number) v).longValue(), what);
            }
        }
    }

    // ── tests ────────────────────────────────────────────────────────────────

    public static void initRunsTheContractSequenceAndDownloadsTheFirmware() {
        Made mk = make();
        FakeTmf8829 fake = mk.fake();
        try (Tmf8829Device dev = mk.dev()) {
            List<int[]> progress = new ArrayList<>();
            Tmf8829Device.InitResult res = dev.init((d, t) -> progress.add(new int[] {d, t}));
            eq("2.200.0", res.appVersion());
            eq(0x12345678L, res.sensorSerial());
            eq(null, dev.config());

            List<int[]> w = fake.writes();
            eq(new int[] {0xF8, 0x14}, w.get(0)); // PON | bootmonitor
            eq(new int[] {0xF7, 0x40}, w.get(1)); // soft reset
            eq(bytes(0xE3, 1, 0), fake.sent(R).get(0));
            eq(new int[] {0x08, 0x22}, w.get(2)); // BL I2C_OFF
            eq(new int[] {0x08, 0x45, 6, 0x00, 0x00, 0x01, 0x00, 3611 & 0xFF, 3611 >> 8}, w.get(3));
            List<int[]> fifo = w.stream().filter(x -> x[0] == 0xFF).toList();
            eq((14444 + 1022) / 1023, fifo.size(), "fifo writes");
            eq(Tmf8829Firmware.image(), fake.fifo.toByteArray(), "downloaded image");
            int last = w.indexOf(fifo.get(fifo.size() - 1));
            List<int[]> after = w.subList(last + 1, w.size());
            eq(new int[] {0x08, 0x16}, after.get(0)); // START_RAM_APP
            eq(new int[] {0xF8, 0xA4}, after.get(1)); // powerup_select = RAM
            eq(fifo.size(), progress.size(), "progress calls");
            eq(new int[] {14444, 14444}, progress.get(progress.size() - 1));
            List<byte[]> reads = fake.sent(R);
            eq(new byte[][] {bytes(0x00, 4, 0), bytes(0x1C, 4, 0)}, grid2(reads.subList(reads.size() - 2, reads.size())));
            eq("app", fake.mode);
        }
    }

    public static void initSurvivesABusyCmdStatAndRefusesAWrongSensor() {
        Made mk = make();
        FakeTmf8829 fake = mk.fake();
        try (Tmf8829Device dev = mk.dev()) {
            fake.busyPolls.set(3);
            eq("2.200.0", dev.init().appVersion());
            fake.deviceId = 0x00;
            Tmf8829Exception e = throwsType(Tmf8829Exception.class, dev::init);
            check(e.getMessage().contains("sensor ID 0x00, expected 0x9E"), e.getMessage());
            fake.deviceId = 0x9E;
            fake.blStatus.put(0x45, 2);
            eq("bootloader cmd 0x45 -> ERR_ADDR", throwsType(Tmf8829Exception.class, dev::init).getMessage());
            fake.blStatus.clear();
            fake.blStatus.put(0x16, 7);
            eq("bootloader cmd 0x16 -> 0x7", throwsType(Tmf8829Exception.class, dev::init).getMessage());
            fake.blStatus.clear();
            fake.stuckBeforeBootloader = true;
            String msg = throwsType(Tmf8829Exception.class, dev::init).getMessage();
            check(msg.contains("sensor bootloader: timeout, reg 0x00 = 0x00"), msg);
        }
    }

    public static void configureWritesEveryFieldAPresetDoesNotReset() {
        Made mk = make();
        FakeTmf8829 fake = mk.fake();
        try (Tmf8829Device dev = mk.dev()) {
            fake.cfg[0x2B - 0x22] = 1;
            fake.cfg[0x2C - 0x22] = 1;
            fake.cfg[0x2A - 0x22] = 0x3C;
            fake.cfg[0] = 100;
            Tmf8829AppliedConfig cfg = dev.configure();
            List<int[]> w = fake.writes();
            eq(new int[] {0x08, 0x40}, w.get(0));
            eq(new int[] {0x08, 0x16}, w.get(1));
            eq(0x22, w.get(2)[0]);
            eq(191, w.get(2).length);
            eq(new int[] {0x08, 0x15}, w.get(3));
            eq(new int[] {0x08, 0x16}, w.get(4));
            eq("8x8", cfg.preset());
            eq(new int[] {8, 8}, cfg.grid());
            eq(false, cfg.histograms());
            eq(false, cfg.refFrame());
            Map<String, Integer> want = new LinkedHashMap<>();
            want.put("period_ms", 33);
            want.put("kilo_iterations", 586);
            want.put("fp_mode", 0);
            want.put("nr_peaks", 1);
            want.put("signal_strength", 0);
            want.put("noise_strength", 0);
            want.put("xtalk", 0);
            want.put("dump_histograms", 0);
            want.put("ref_spad_frame", 0);
            for (Map.Entry<String, Integer> kv : want.entrySet()) {
                eq(kv.getValue(), cfg.fields().get(kv.getKey()), kv.getKey());
            }
            check(same(cfg, dev.config()), "config() is the configure() result");
        }
    }

    public static void configureWritesTheCallerOptionsAndRefusesBadOnes() {
        Made mk = make();
        FakeTmf8829 fake = mk.fake();
        try (Tmf8829Device dev = mk.dev()) {
            Tmf8829AppliedConfig cfg = dev.configure("32x32", 300, null, 2, true, true, true, true, true,
                    Map.of("power_modes", 3));
            eq(new int[] {0x08, 0x45}, fake.writes().get(0));
            int[] w2 = fake.writes().get(2);
            int[] page = Arrays.copyOfRange(w2, 1, w2.length);
            eq(300, page[0] | page[1] << 8);
            eq(2344, page[2] | page[3] << 8);
            eq(2 | 0x08 | 0x10 | 0x20, page[0x2A - 0x22]);
            eq(new int[] {1, 1, 3}, new int[] {page[0x2B - 0x22], page[0x2C - 0x22], page[0x2E - 0x22]});
            eq(new int[] {32, 32}, cfg.grid());
            eq(true, cfg.histograms());
            eq(true, cfg.refFrame());
            eq(3, cfg.fields().get("fp_mode"));

            int n = fake.logCount();
            throwsType(IllegalArgumentException.class, () -> dev.configure("9x9"));
            throwsType(IllegalArgumentException.class, () -> dev.configure("8x8", Tmf8829Config.DEFAULT_PERIOD_MS, null,
                    0, false, false, false, false, false, null));
            throwsType(IllegalArgumentException.class, () -> dev.configure("8x8", Tmf8829Config.DEFAULT_PERIOD_MS, null,
                    5, false, false, false, false, false, null));
            eq(n, fake.logCount(), "nothing sent for refused arguments");

            fake.appStatus.put(0x15, 0x04);
            eq("sensor cmd 0x15 -> ERR_CONFIG_RESULT_SIZE", throwsType(Tmf8829Exception.class,
                    () -> dev.configure("48x32", Tmf8829Config.DEFAULT_PERIOD_MS, null, 4, false, false, false, false,
                            false, null)).getMessage());
            fake.appStatus.clear();
            fake.stuckBytes.put(0x78 - 0x22, 5);
            eq("config fields did not stick: fov_correction wrote 3, reads 5", throwsType(Tmf8829Exception.class,
                    () -> dev.configure("8x8", Tmf8829Config.DEFAULT_PERIOD_MS, null, 1, false, false, false, false,
                            false, Map.of("fov_correction", 3))).getMessage());
            fake.stuckBytes.clear();
            fake.cidOverride = 0x40;
            eq("CID_RID 0x40 after LOAD_CONFIG_PAGE", throwsType(Tmf8829Exception.class, dev::configPage).getMessage());
        }
    }

    public static void startArmsTheStreamThenMeasuresAndStopUndoesIt() {
        Made mk = make();
        FakeTmf8829 fake = mk.fake();
        try (Tmf8829Device dev = mk.dev()) {
            dev.start();
            check(dev.streaming(), "streaming");
            eq("8x8", dev.config().preset());
            List<int[]> w = fake.writes();
            int i = 0;
            while (w.get(i)[0] != 0xFA) {
                i++;
            }
            eq(new int[][] {{0xFA, 0x30}, {0xE1, 0xFF}, {0xE2, 0x01}}, grid(w.subList(i, i + 3)));
            eq(new byte[][] {bytes(0x01, 0x00)}, grid2(fake.sent(0x35)));
            eq(new int[] {0x08, 0x10}, w.get(w.size() - 1)); // MEASURE after the stream is armed
            throwsType(InvalidStateException.class, dev::configure);
            dev.start(); // no-op while streaming
            eq(1, fake.sent(0x35).size(), "one START_STREAM");

            int n = fake.logCount();
            dev.stop();
            List<int[]> tail = new ArrayList<>();
            for (Packet p : fake.logFrom(n)) {
                if (p.cmd() == R) {
                    continue;
                }
                int[] row = new int[1 + p.payload().length];
                row[0] = p.cmd();
                System.arraycopy(ints(p.payload()), 0, row, 1, p.payload().length);
                tail.add(row);
            }
            eq(new int[][] {{W, 0x08, 0xFF}, {W, 0xE2, 0x00}, {W, 0xE1, 0xFF}, {0x36}}, grid(tail));
            check(!dev.streaming(), "stopped");
            dev.stop(); // the sensor is stopped again, the bridge left alone
            eq(1, fake.sent(0x36).size(), "one STOP_STREAM");

            dev.configure("8x8", Tmf8829Config.DEFAULT_PERIOD_MS, null, 1, false, false, false, true, true, null);
            dev.start();
            check(containsWrite(fake.writes(), 0xE2, 0x09), "INT_ENAB 0x09 written");
            List<byte[]> ss = fake.sent(0x35);
            eq(bytes(0x09, 0x01), ss.get(ss.size() - 1));
            dev.stop();

            fake.appStatus.put(0x10, 0x03);
            eq("sensor cmd 0x10 -> ERR_APPLICATION", throwsType(Tmf8829Exception.class, dev::start).getMessage());
            check(!dev.streaming(), "not streaming after a refused MEASURE");
            eq(3, fake.sent(0x36).size(), "STOP_STREAM count");
        }
    }

    public static void stopSwallowsTheBadHeaderAStopRaisesAndOtherEventsPass() throws Exception {
        Made mk = make();
        FakeTmf8829 fake = mk.fake();
        try (Tmf8829Device dev = mk.dev()) {
            List<Tmf8829InterruptEvent> events = new ArrayList<>();
            AutoCloseable sub = dev.onEvent(e -> {
                if (e instanceof Tmf8829InterruptEvent t) {
                    synchronized (events) {
                        events.add(t);
                    }
                }
            });
            dev.start();
            int badHeader = Tmf8829.EventCode.BAD_HEADER.value;
            fake.onCommand = cmd -> cmd == 0xFF ? List.of(fake.event(badHeader, 0x08, 0xFF, 0x04)) : List.of();
            dev.stop();
            sleep(50);
            synchronized (events) {
                eq(0, events.size(), "events after stop");
            }

            fake.link.inject(fake.link.reply(0x94, new byte[5])); // truncated: ignored
            fake.emitEvent(Tmf8829.EventCode.INT.value, 0x02, 0);
            fake.emitEvent(badHeader, 0x01, 0x55);
            waitFor(events, 2);
            eq(new Tmf8829.Tmf8829Event(987654321L, 0x02, 0, 0, 0), events.get(0).event());
            eq(1, events.get(1).event().code());
            eq(0x55, events.get(1).event().frameId());
            sub.close();
        }
    }

    public static void closeStopsARunningStreamEvenWhenTheStopIsRefused() {
        Made mk = make();
        FakeTmf8829 fake = mk.fake();
        Tmf8829Device dev = mk.dev();
        dev.start();
        dev.close();
        check(containsWrite(fake.writes(), 0x08, 0xFF), "STOP written");
        eq(1, fake.sent(0x36).size(), "one STOP_STREAM");

        Made mk2 = make();
        Tmf8829Device dev2 = mk2.dev();
        dev2.start();
        mk2.fake().appStatus.put(0xFF, 0x0A);
        dev2.close();
        check(dev2.isClosed(), "closed");
    }

    public static void streamedFramesMatchTheGoldenMeasurements() {
        loadVectors();
        for (String name : meas.keySet()) {
            Made mk = make();
            FakeTmf8829 fake = mk.fake();
            try (Tmf8829Device dev = mk.dev()) {
                List<MeasurementBuilder.Measurement> got = new ArrayList<>();
                dev.onMeasurement(m -> {
                    synchronized (got) {
                        got.add(m);
                    }
                });
                dev.start();
                play(fake, name);
                List<Object> want = arr(meas.get(name).get("expect"));
                waitFor(got, want.size());
                for (int k = 0; k < want.size(); k++) {
                    assertFields(name + "#" + k, obj(want.get(k)), measFields(got.get(k)));
                }
                int nCorrupt = 0;
                for (Object fo : arr(meas.get(name).get("frames"))) {
                    if (corrupt.contains((String) obj(fo).get("raw_case"))) nCorrupt++;
                }
                eq(new int[] {0, 0, nCorrupt},
                        new int[] {dev.framesDropped(), dev.framesDiscarded(), dev.frameParseErrors()},
                        name + ": dropped/discarded/parse errors");
                eq(lng(obj(want.get(want.size() - 1)).get("ratio_q16")), dev.clockRatioQ16(), name + ": ratio_q16");
            }
        }
    }

    public static void histogramFramesAreReassembledFrom1024ByteChunks() {
        Made mk = make();
        FakeTmf8829 fake = mk.fake();
        try (Tmf8829Device dev = mk.dev()) {
            List<MeasurementBuilder.Measurement> got = new ArrayList<>();
            dev.onMeasurement(m -> {
                synchronized (got) {
                    got.add(m);
                }
            });
            dev.start();
            play(fake, "8x8_hist");
            waitFor(got, 1);
            eq(new int[] {25377, 25377},
                    got.get(0).histograms().stream().mapToInt(h -> h.raw().length).toArray());
            eq(raw("8x8_hist#0"), got.get(0).histograms().get(0).raw());
        }
    }

    public static void getMeasurementTakesTheNextAndTimesOut() throws Exception {
        Made mk = make();
        FakeTmf8829 fake = mk.fake();
        byte[] frame = raw("8x8#0");
        try (Tmf8829Device dev = mk.dev();
                Subscription<MeasurementBuilder.Measurement> all = dev.measurements(4)) {
            dev.start();
            Thread t = new Thread(() -> {
                sleep(50);
                fake.emitFrame(frame, 2125575099L, 2125575071L);
            });
            t.start();
            eq(1L, dev.getMeasurement(Duration.ofSeconds(2)).fNumber());
            t.join();
            eq(1L, all.take(Duration.ofSeconds(1)).fNumber());
            throwsType(DepzTimeoutException.class, () -> dev.getMeasurement(Duration.ofMillis(100)));
        }
    }

    public static void lostAndBrokenFramesAreCountedNotRaised() {
        Made mk = make();
        FakeTmf8829 fake = mk.fake();
        try (Tmf8829Device dev = mk.dev()) {
            List<MeasurementBuilder.Measurement> got = new ArrayList<>();
            dev.onMeasurement(m -> {
                synchronized (got) {
                    got.add(m);
                }
            });
            dev.start();
            fake.emitFrame(raw("8x8#0"), 2125575099L, 2125575071L, 0, null);
            fake.emitFrame(raw("8x8#1"), 2125672180L, 2125672161L, 3, null); // two frames dropped by the bridge
            fake.emitFrame(raw("8x8_full#0"), 2125937131L, 0, 4, new int[] {0}); // cut short
            fake.emitFrame(Arrays.copyOf(raw("8x8#1"), 40), 1, 0, 5, null); // parse error
            fake.link.inject(fake.link.reply(0x93, new byte[10])); // truncated chunk header
            waitFor(got, 2);
            sleep(50);
            eq(new int[] {2, 1, 1}, new int[] {dev.framesDropped(), dev.framesDiscarded(), dev.frameParseErrors()});
            dev.stop();
            dev.start();
            eq(new int[] {0, 0, 0}, new int[] {dev.framesDropped(), dev.framesDiscarded(), dev.frameParseErrors()});
        }
    }

    public static void registerAccessChunksAndChecksItsReplies() {
        Made mk = make();
        FakeTmf8829 fake = mk.fake();
        try (Tmf8829Device dev = mk.dev()) {
            eq(2000, dev.readRegisters(0xF0, 2000).length);
            eq(new byte[][] {bytes(0xF0, 0x00, 0x04), bytes(0xFF, 976 & 0xFF, 976 >> 8)}, grid2(fake.sent(R)));

            List<int[]> progress = new ArrayList<>();
            byte[] seven = new byte[2100];
            Arrays.fill(seven, (byte) 7);
            dev.writeRegisters(0xFF, seven, (d, t) -> progress.add(new int[] {d, t}));
            List<int[]> shape = new ArrayList<>();
            for (byte[] p : fake.sent(W)) {
                shape.add(new int[] {p[0] & 0xFF, p.length - 1});
            }
            eq(new int[][] {{0xFF, 1023}, {0xFF, 1023}, {0xFF, 54}}, grid(shape));
            eq(new int[][] {{1023, 2100}, {2046, 2100}, {2100, 2100}}, grid(progress));

            dev.writeRegister(0xE2, 0x109);
            List<byte[]> ws = fake.sent(W);
            eq(bytes(0xE2, 0x09), ws.get(ws.size() - 1));

            int n = fake.logCount();
            throwsType(IllegalArgumentException.class, () -> dev.readRegisters(0x100, 1));
            throwsType(IllegalArgumentException.class, () -> dev.writeRegisters(0x100, new byte[1]));
            eq(n, fake.logCount(), "nothing sent for refused addresses");

            fake.shortRead = 1;
            eq("READ_REG 0x00: expected 4, got 3",
                    throwsType(DepzException.class, () -> dev.readRegisters(0x00, 4)).getMessage());
            fake.shortRead = 0;
            fake.bridgeStatus.put(W, 0x03);
            throwsType(StatusException.class, () -> dev.writeRegisters(0xE2, new byte[] {1}));
            fake.bridgeStatus.clear();
            dev.writeRegisters(0xE2, new byte[] {1});
            eq(1, (int) fake.regs[0xE2]);
        }
    }

    public static void sensorStateAndBridgeInfo() {
        Made mk = make();
        try (Tmf8829Device dev = mk.dev()) {
            dev.init();
            Map<String, Integer> st = dev.sensorState();
            eq(List.of("ID", "REVID", "ENABLE", "RESET", "APP_ID", "CMD_STAT", "INT_STATUS", "INT_ENAB", "FIFOSTATUS"),
                    new ArrayList<>(st.keySet()));
            eq(0x9E, st.get("ID"));
            eq(0x01, st.get("APP_ID"));
            eq(0x80, st.get("ENABLE") & 0x80);
            dev.start();
            Tmf8829.Tmf8829Info info = dev.getBridgeInfo();
            eq(new long[] {7, 6, 1}, new long[] {info.intEdges(), info.frames(), info.framesAborted()});
            eq(true, info.streaming());
            eq(0x01, info.intMask());
        }
    }
}
