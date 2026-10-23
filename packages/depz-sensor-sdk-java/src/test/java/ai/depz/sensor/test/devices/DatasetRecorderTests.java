package ai.depz.sensor.test.devices;

import static ai.depz.sensor.test.devices.T.check;
import static ai.depz.sensor.test.devices.T.eq;
import static ai.depz.sensor.test.devices.T.throwsType;
import static ai.depz.sensor.test.devices.T.until;

import ai.depz.sensor.dataset.Dataset;
import ai.depz.sensor.dataset.DatasetPlayer;
import ai.depz.sensor.dataset.DatasetRecords;
import ai.depz.sensor.dataset.DatasetWriter;
import ai.depz.sensor.dataset.PlayerState;
import ai.depz.sensor.dataset.SessionRecorder;
import ai.depz.sensor.devices.DepzDevice;
import ai.depz.sensor.devices.Sr04Device;
import ai.depz.sensor.protocol.Common;
import ai.depz.sensor.protocol.Sr04;
import ai.depz.sensor.transport.Packet;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.time.Duration;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.HashSet;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicBoolean;

/** The dataset recorder, writer, reader and player (contract 09) over fake SR04 boards and temp files. */
public final class DatasetRecorderTests {
    private static final double EPS = 1e-6;

    /**
     * An SR04 board that answers the common commands a recorder needs (sync, serial, name, MCU
     * temperature) on a device clock of its own, and streams a measurement on request.
     */
    static final class FakeSr04Board {
        final FakeBoardLink link = new FakeBoardLink();
        volatile long mcuTimeUs;
        // 50 ms as the Python fake (C#: 1 ms): device time steps must exceed the gap between the two
        // devices' syncs, or the merged order depends on how fast add() ran.
        long samplePeriodUs = 50_000;

        FakeSr04Board(long mcuTimeUs) {
            this.mcuTimeUs = mcuTimeUs;
            link.onPacket = this::onPacket;
        }

        private List<byte[]> onPacket(Packet p) {
            int cmd = p.cmd();
            if (cmd == Common.Cmd.SYNC_TIME.value) {
                ByteBuffer r = ByteBuffer.allocate(24).order(ByteOrder.LITTLE_ENDIAN);
                r.put(p.payload(), 0, 8);
                r.putLong(mcuTimeUs);
                r.putLong(mcuTimeUs);
                return List.of(link.reply(Common.Rpt.SYNC_TIME.value, r.array()));
            }
            if (cmd == Common.Cmd.GET_SERIAL.value) {
                return List.of(link.reply(Common.Rpt.TEXT.value, text(cmd, "SN000042")));
            }
            if (cmd == Common.Cmd.GET_NAME_ACTIVE_SOFTWARE.value) {
                return List.of(link.reply(Common.Rpt.TEXT.value, text(cmd, "APP_usonic_SR04_v0.97")));
            }
            if (cmd == Common.Cmd.GET_MCU_TEMPERATURE.value) {
                ByteBuffer r = ByteBuffer.allocate(10).order(ByteOrder.LITTLE_ENDIAN);
                r.putShort(8, (short) 251);
                return List.of(link.reply(Common.Rpt.TEMPERATURE.value, r.array()));
            }
            return List.of(FakeBoardLink.status(cmd, Common.Status.OK.value));
        }

        private static byte[] text(int cmd, String s) {
            byte[] a = s.getBytes(StandardCharsets.US_ASCII);
            byte[] out = new byte[a.length + 2];
            out[0] = (byte) cmd;
            System.arraycopy(a, 0, out, 1, a.length);
            return out;
        }

        void sendMeasurement(int source) {
            sendMeasurement(source, 5831);
        }

        void sendMeasurement(int source, int echoUs) {
            mcuTimeUs += samplePeriodUs;
            ByteBuffer p = ByteBuffer.allocate(11).order(ByteOrder.LITTLE_ENDIAN);
            p.put((byte) source);
            p.putLong(mcuTimeUs);
            p.putShort((short) echoUs);
            link.inject(link.reply(Sr04.Sr04Rpt.DATA.value, p.array()));
        }
    }

    private static Path tempDir() throws IOException {
        return Files.createTempDirectory("depz-dataset-");
    }

    private static void deleteTree(Path dir) throws IOException {
        if (!Files.exists(dir)) {
            return;
        }
        try (var s = Files.walk(dir)) {
            for (Path p : s.sorted(Comparator.reverseOrder()).toList()) {
                Files.deleteIfExists(p);
            }
        }
    }

    private static Map<String, Object> obj(Object... kv) {
        Map<String, Object> m = new LinkedHashMap<>();
        for (int i = 0; i < kv.length; i += 2) {
            m.put((String) kv[i], kv[i + 1]);
        }
        return m;
    }

    // ── recorder / writer / reader ──────────────────────────────────────────

    public static void twoDevicesLandOnOneHostTimeline() throws Exception {
        FakeSr04Board a = new FakeSr04Board(1_000_000);
        FakeSr04Board b = new FakeSr04Board(9_000_000_000L);
        Path dir = tempDir();
        Path path = dir.resolve("rec.depzdata");
        try {
            try (Sr04Device devA = new Sr04Device(a.link, Duration.ofSeconds(1));
                    Sr04Device devB = new Sr04Device(b.link, Duration.ofSeconds(1));
                    SessionRecorder rec = new SessionRecorder(path, "unit test", false, null)) {
                eq(List.of("d0", "d1"), List.of(rec.add(devA), rec.add(devB)));
                rec.start();
                throwsType(IllegalStateException.class, () -> rec.add(devA));
                a.sendMeasurement(0x37);
                Thread.sleep(20);
                b.sendMeasurement(0x37);
                Thread.sleep(20);
                a.sendMeasurement(0x37);
                Thread.sleep(20);
                b.sendMeasurement(0x36);
                until(() -> rec.recordsWritten() >= 4, 2000);
            }
            Dataset reader = Dataset.read(path);
            eq(List.of("d0", "d1"), reader.devices().keySet().stream().sorted().toList());
            for (Object d : reader.devices().values()) {
                Map<?, ?> m = (Map<?, ?>) d;
                eq(List.of("sr04", "SN000042"), List.of(m.get("sensor_type"), m.get("serial")));
            }
            eq("unit test", reader.note());
            List<Dataset.Record> recs = reader.records();
            eq(4, recs.size());
            eq(List.of("d0", "d1", "d0", "d1"), recs.stream().map(Dataset.Record::deviceId).toList());
            eq("once", recs.get(3).value().get("source"));
            eq(5831L, recs.get(0).value().get("echo_us"));
            // t = device time - offset: both clocks map onto the host clock
            for (Dataset.Record r : recs) {
                check(Math.abs(r.tHostUs() - DepzDevice.hostNowUs()) < 10_000_000, "t on the host clock: " + r.tHostUs());
            }
        } finally {
            deleteTree(dir);
        }
    }

    public static void theTemperatureSamplerWritesMcuRecords() throws Exception {
        FakeSr04Board a = new FakeSr04Board(1_000_000);
        Path dir = tempDir();
        Path path = dir.resolve("rec.depzdata.gz");
        try {
            try (Sr04Device dev = new Sr04Device(a.link, Duration.ofSeconds(1));
                    SessionRecorder rec = new SessionRecorder(path, "", false, 50.0)) {
                rec.add(dev);
                rec.start();
                Thread.sleep(150);
            }
            byte[] raw = Files.readAllBytes(path);
            check((raw[0] & 0xFF) == 0x1F && (raw[1] & 0xFF) == 0x8B, ".gz is written as gzip");
            List<Dataset.Record> temps = Dataset.read(path).records().stream()
                    .filter(r -> r.kind().equals("temperature")).toList();
            check(!temps.isEmpty(), "temperature records written");
            for (Dataset.Record t : temps) {
                eq(List.of(25.1, "mcu"), List.of(t.value().get("celsius"), t.value().get("source")));
            }
        } finally {
            deleteTree(dir);
        }
    }

    public static void writerAndReaderRoundTripAndSortWithinADevice() throws Exception {
        Path dir = tempDir();
        Path path = dir.resolve("rt.depzdata");
        try {
            Map<String, Map<String, Object>> devices = new LinkedHashMap<>();
            devices.put("d0", new LinkedHashMap<>());
            devices.put("d1", new LinkedHashMap<>());
            try (DatasetWriter w = new DatasetWriter(path, devices)) {
                Object[][] events = {{"d0", 300L}, {"d0", 100L}, {"d1", 200L}, {"d0", 250L}, {"d1", 50L}};
                for (Object[] e : events) {
                    w.write((String) e[0], (Long) e[1], "event", obj("arr", List.of(1L, 2.5)));
                }
                eq(5L, w.recordsWritten());
            }
            Dataset r = Dataset.read(path);
            eq(DatasetWriter.SCHEMA, r.header().get("schema"));
            eq(List.of("d1:50", "d0:100", "d1:200", "d0:250", "d0:300"),
                    r.records().stream().map(x -> x.deviceId() + ":" + x.tHostUs()).toList());
            eq(250L, r.durationUs());
            eq(List.of(1L, 2.5), r.records().get(0).value().get("arr"));

            List<Long> got = new ArrayList<>();
            r.play(x -> got.add(x.tHostUs()), 0);
            eq(5, got.size());
            AtomicBoolean cancel = new AtomicBoolean();
            got.clear();
            r.play(x -> {
                got.add(x.tHostUs());
                if (got.size() == 2) {
                    cancel.set(true);
                }
            }, 0.5, null, cancel::get);
            eq(2, got.size());

            // startTUs skips earlier records; speed must be finite and >= 0
            got.clear();
            r.play(x -> got.add(x.tHostUs()), 0, 200L, null);
            eq(List.of(200L, 250L, 300L), got);
            throwsType(IllegalArgumentException.class, () -> r.play(x -> {}, -1));
            throwsType(IllegalArgumentException.class, () -> r.play(x -> {}, Double.NaN));
            throwsType(IllegalArgumentException.class, () -> r.play(x -> {}, Double.POSITIVE_INFINITY));
        } finally {
            deleteTree(dir);
        }
    }

    public static void round1IsPythonsRound() {
        double[][] cases = {
            {0.25, 0.2},
            {0.35, 0.3}, // 0.34999999999999997779… in binary
            {0.05, 0.1}, // 0.05000000000000000277…
            {-0.25, -0.2},
            {1234.56789, 1234.6},
            {0.0, 0.0},
        };
        for (double[] c : cases) {
            eq(c[1], DatasetRecords.round1(c[0]), "round1(" + c[0] + ")");
        }
    }

    // ── player ──────────────────────────────────────────────────────────────

    private static final class FakeClock {
        double t = 100.0;

        double now() {
            return t;
        }
    }

    private record Made(DatasetPlayer p, FakeClock clock, List<Long> seen, List<PlayerState> states) {}

    private static Path write(Path dir, List<Object[]> events) {
        Path path = dir.resolve("p" + System.nanoTime() + ".depzdata");
        Map<String, Map<String, Object>> devices = new LinkedHashMap<>();
        devices.put("d0", obj("sensor_type", "sr04"));
        devices.put("d1", obj("sensor_type", "vl53l8"));
        try (DatasetWriter w = new DatasetWriter(path, devices, "test")) {
            long i = 0;
            for (Object[] e : events) {
                w.write((String) e[0], (Long) e[1], "sr04", obj("n", i++));
            }
        }
        return path;
    }

    private static Path sample(Path dir) {
        return write(dir, List.of(new Object[] {"d0", 1000L}, new Object[] {"d1", 900L},
                new Object[] {"d0", 51000L}, new Object[] {"d1", 30900L}));
    }

    private static Path evenly(Path dir, int n, long gapUs) {
        List<Object[]> events = new ArrayList<>();
        for (int i = 0; i < n; i++) {
            events.add(new Object[] {"d0", 1_000_000 + i * gapUs});
        }
        return write(dir, events);
    }

    private static Path evenly(Path dir) {
        return evenly(dir, 5, 100_000);
    }

    private static Made make(Path path, double speed) {
        FakeClock clock = new FakeClock();
        DatasetPlayer p = new DatasetPlayer(Dataset.read(path), speed, clock::now, false);
        List<Long> seen = new ArrayList<>();
        List<PlayerState> states = new ArrayList<>();
        p.onRecord(r -> seen.add(r.tHostUs()));
        p.onState(states::add);
        return new Made(p, clock, seen, states);
    }

    private static Made make(Path path) {
        return make(path, 1.0);
    }

    private static void near(double expected, Double actual, String what) {
        check(actual != null && Math.abs(expected - actual) < 1e-9, what + ": expected " + expected + ", got " + actual);
    }

    public static void playsPacedPausesResumesSeeks() throws Exception {
        Path dir = tempDir();
        try {
            Made m = make(sample(dir));
            DatasetPlayer p = m.p();
            FakeClock clock = m.clock();
            List<Long> seen = m.seen();
            eq(List.of(PlayerState.IDLE, 0L, 50100L), List.of(p.state(), p.positionUs(), p.durationUs()));
            p.play();
            check(seen.isEmpty(), "nothing before the first tick");
            Double wait = p.tick();
            eq(List.of(900L), seen);
            near(100e-6, wait, "wait");
            clock.t += 100e-6 + EPS;
            p.tick();
            eq(List.of(900L, 1000L), seen);
            clock.t += 0.0299 - 2 * EPS;
            p.tick();
            eq(2, seen.size());
            clock.t += 2 * EPS;
            p.tick();
            eq(List.of(900L, 1000L, 30900L), seen);
            eq(30000L, p.positionUs());

            p.pause();
            clock.t += 1.0;
            eq(null, p.tick());
            eq(3, seen.size());

            p.play(); // resume: the next record waits its full gap (20.1 ms) again
            clock.t += 0.0201 - EPS;
            p.tick();
            eq(3, seen.size());
            clock.t += 2 * EPS;
            eq(null, p.tick());
            eq(List.of(900L, 1000L, 30900L, 51000L), seen);
            eq(List.of(PlayerState.DONE, 50100L), List.of(p.state(), p.positionUs()));
            eq(List.of(PlayerState.PLAYING, PlayerState.PAUSED, PlayerState.PLAYING, PlayerState.DONE), m.states());

            seen.clear();
            p.seekUs(30000);
            eq(List.of(PlayerState.DONE, 100L), List.of(p.state(), p.positionUs()));
            p.setSpeed(0);
            p.play();
            p.tick();
            eq(List.of(30900L, 51000L), seen);
        } finally {
            deleteTree(dir);
        }
    }

    public static void speedChangeMidWaitRescalesTheRemainderAndASlowConsumerDoesNotDrift() throws Exception {
        Path dir = tempDir();
        try {
            Made m = make(evenly(dir));
            DatasetPlayer p = m.p();
            p.play();
            p.tick();
            m.clock().t += 0.05;
            p.tick();
            p.setSpeed(0.5); // remaining 50 ms of media → 100 ms of wall time
            near(0.1, p.tick(), "rescaled wait");
            m.clock().t += 0.1 + EPS;
            p.tick();
            eq(2, m.seen().size());

            Made m2 = make(evenly(dir));
            DatasetPlayer q = m2.p();
            q.play();
            q.tick();
            m2.clock().t += 0.35; // records 1..3 overdue
            Double w = q.tick();
            eq(4, m2.seen().size());
            near(0.05, w, "wait after catching up");
            throwsType(IllegalArgumentException.class, () -> q.setSpeed(-1));
            throwsType(IllegalArgumentException.class, () -> q.setSpeed(Double.NaN));
        } finally {
            deleteTree(dir);
        }
    }

    public static void seekWhilePausedSkipsAndPastTheEndEndsPlayback() throws Exception {
        Path dir = tempDir();
        try {
            Made m = make(evenly(dir));
            DatasetPlayer p = m.p();
            p.play();
            p.tick();
            p.pause();
            p.seekUs(250_000);
            eq(List.of(200_000L, PlayerState.PAUSED), List.of(p.positionUs(), p.state()));
            p.play();
            near(0.1, p.tick(), "wait after seek");
            m.clock().t += 0.1;
            p.tick();
            eq(List.of(1_000_000L, 1_300_000L), m.seen());

            Made m2 = make(evenly(dir));
            DatasetPlayer q = m2.p();
            q.play();
            q.tick();
            q.seekUs(1e9);
            eq(List.of(PlayerState.DONE, 400_000L), List.of(q.state(), q.positionUs()));
            eq(List.of(PlayerState.PLAYING, PlayerState.DONE), m2.states());
            throwsType(IllegalArgumentException.class, () -> q.seekUs(Double.NaN));
        } finally {
            deleteTree(dir);
        }
    }

    public static void aCallbackErrorPausesAndPropagatesAndCallbacksMayControlThePlayer() throws Exception {
        Path dir = tempDir();
        try {
            Made m = make(evenly(dir), 0);
            DatasetPlayer p = m.p();
            p.onRecord(r -> {
                if (r.tHostUs() == 1_200_000) {
                    throw new IllegalStateException("consumer failed");
                }
            });
            p.play();
            eq("consumer failed", throwsType(IllegalStateException.class, p::tick).getMessage());
            eq(List.of(PlayerState.PAUSED, 200_000L), List.of(p.state(), p.positionUs()));
            p.play();
            p.tick();
            eq(5, m.seen().size());
            eq(List.of(PlayerState.PLAYING, PlayerState.PAUSED, PlayerState.PLAYING, PlayerState.DONE), m.states());

            Made m2 = make(evenly(dir), 0);
            DatasetPlayer q = m2.p();
            q.onRecord(r -> {
                if (r.tHostUs() == 1_100_000) {
                    q.pause();
                }
            });
            q.play();
            eq(null, q.tick());
            eq(List.of(1_000_000L, 1_100_000L), m2.seen());
            eq(List.of(PlayerState.PLAYING, PlayerState.PAUSED), m2.states());
        } finally {
            deleteTree(dir);
        }
    }

    public static void emptyDatasetAndUnsubscribeAndClose() throws Exception {
        Path dir = tempDir();
        try {
            Made m = make(write(dir, List.of()));
            DatasetPlayer p = m.p();
            p.play();
            eq(List.of(PlayerState.PLAYING, PlayerState.DONE), m.states());
            p.seekUs(100);
            eq(0L, p.positionUs());

            DatasetPlayer q = new DatasetPlayer(evenly(dir), 0, new FakeClock()::now, false);
            int[] got = {0};
            AutoCloseable sub = q.onRecord(r -> got[0]++);
            sub.close();
            sub.close();
            q.play();
            q.tick();
            eq(0, got[0]);
            throwsType(IllegalStateException.class, () -> q.await(Duration.ZERO));
            q.close();
            q.close();
            throwsType(IllegalStateException.class, q::play);
        } finally {
            deleteTree(dir);
        }
    }

    public static void threadedDeliversOnThePlayerThreadAndPausesFromAnother() throws Exception {
        Path dir = tempDir();
        try {
            try (DatasetPlayer p = new DatasetPlayer(evenly(dir, 4, 20_000))) {
                Set<String> threads = Collections.synchronizedSet(new HashSet<>());
                List<Long> seen = Collections.synchronizedList(new ArrayList<>());
                p.onRecord(r -> {
                    threads.add(Thread.currentThread().getName());
                    seen.add(r.tHostUs());
                });
                p.play();
                check(p.await(Duration.ofSeconds(5)), "done in time");
                eq(List.of(PlayerState.DONE, 4), List.of(p.state(), seen.size()));
                eq(Set.of(DatasetPlayer.THREAD_NAME), threads);
            }

            try (DatasetPlayer q = new DatasetPlayer(evenly(dir, 50, 100_000))) {
                CountDownLatch first = new CountDownLatch(1);
                List<Long> seen2 = Collections.synchronizedList(new ArrayList<>());
                q.onRecord(r -> {
                    seen2.add(r.tHostUs());
                    first.countDown();
                });
                q.play();
                check(first.await(5, TimeUnit.SECONDS), "a first record");
                q.pause();
                int n = seen2.size();
                check(q.await(Duration.ZERO), "not playing after pause");
                q.seekUs(4_000_000);
                eq(n, seen2.size());
                q.setSpeed(0);
                q.play();
                check(q.await(Duration.ofSeconds(5)), "done in time");
                List<Long> want = new ArrayList<>();
                for (int i = 40; i < 50; i++) {
                    want.add(1_000_000L + i * 100_000L);
                }
                eq(want, new ArrayList<>(seen2.subList(n, seen2.size())));
            }

            try (DatasetPlayer e = new DatasetPlayer(evenly(dir), 0)) {
                List<Exception> errors = Collections.synchronizedList(new ArrayList<>());
                e.onDeliveryError(errors::add);
                e.onRecord(r -> {
                    throw new java.util.NoSuchElementException("x");
                });
                e.play();
                check(e.await(Duration.ofSeconds(5)), "stopped in time");
                eq(PlayerState.PAUSED, e.state());
                eq(1, errors.size());
                check(errors.get(0) instanceof java.util.NoSuchElementException, "the callback's exception: " + errors);
            }
        } finally {
            deleteTree(dir);
        }
    }
}
