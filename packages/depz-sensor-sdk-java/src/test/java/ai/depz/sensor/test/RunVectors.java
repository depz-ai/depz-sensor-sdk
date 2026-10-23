package ai.depz.sensor.test;

import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.Random;

import ai.depz.sensor.protocol.Bno055;
import ai.depz.sensor.protocol.Common;
import ai.depz.sensor.protocol.FwDepz;
import ai.depz.sensor.protocol.Identity;
import ai.depz.sensor.protocol.Sr04;
import ai.depz.sensor.protocol.Tmf8829;
import ai.depz.sensor.protocol.Vl53l4;
import ai.depz.sensor.protocol.Vl53l7;
import ai.depz.sensor.protocol.Vl53lx;
import ai.depz.sensor.transport.CrcError;
import ai.depz.sensor.transport.CrcType;
import ai.depz.sensor.transport.Crc;
import ai.depz.sensor.transport.Event;
import ai.depz.sensor.transport.Framing;
import ai.depz.sensor.transport.Packet;
import ai.depz.sensor.transport.PacketParser;
import ai.depz.sensor.transport.Trash;
import ai.depz.sensor.usb.Discovery;
import ai.depz.sensor.usb.UsbIds;
import ai.depz.sensor.sensors.vl53l4.Vl53l4Uld;
import ai.depz.sensor.sensors.vl53l7.Vl53l7Uld;
import ai.depz.sensor.sensors.vl53lx.Vl53lxDecode;
import ai.depz.sensor.sensors.vl53lx.Vl53lxProducts;
import ai.depz.sensor.sensors.vl53l8.FrameReassembler;
import ai.depz.sensor.sensors.vl53l8.Vl53l8Uld;
import ai.depz.sensor.sensors.bno055.Bno055Regs;
import ai.depz.sensor.sensors.bno086.Shtp;
import ai.depz.sensor.sensors.tmf8829.CaptureTimer;
import ai.depz.sensor.sensors.tmf8829.Frames;
import ai.depz.sensor.sensors.tmf8829.MeasurementBuilder;
import ai.depz.sensor.sensors.tmf8829.SensorClock;
import ai.depz.sensor.sensors.tmf8829.Tmf8829Config;
import ai.depz.sensor.sensors.tmf8829.Tmf8829Firmware;
import ai.depz.sensor.sensors.bno086.Sh2;
import ai.depz.sensor.sensors.bno086.Sh2Hub;
import ai.depz.sensor.sensors.bno086.Reports;
import ai.depz.sensor.dataset.Dataset;
import ai.depz.sensor.dataset.DatasetRecords;
import java.util.Objects;

/**
 * Golden-vector harness. Runs every contract vector this SDK consumes and exits
 * nonzero on any failure. No JUnit; assertions are counted by hand.
 */
public final class RunVectors {
    private static int consumed = 0;
    private static int passed = 0;
    private static final List<String> failures = new ArrayList<>();
    private static Path vectorsDir;

    public static void main(String[] args) throws Exception {
        vectorsDir = Path.of(args.length > 0 ? args[0] : "../../contracts/vectors");
        System.out.println("vectors dir: " + vectorsDir.toAbsolutePath());

        crcVectors();
        framingEncodeVectors();
        framingDecodeVectors();
        usbIdsVectors();
        identityVectors();
        probeVectors();
        commonVectors();
        sr04Vectors();
        fwdepzVectors();
        vl53l4EncodeVectors();
        vl53l4DecodeVectors();
        vl53l4ResultBlockVectors();
        vl53l4TimingVectors();
        vl53l4TuningVectors();
        vl53l4ConfigBlockVector();
        vl53l8AdvancedVectors();
        vl53l8CnhVectors();
        vl53l8ReplayVector();
        vl53l7Vectors();
        vl53l7ReplayVectors();
        vl53lxVectors();
        bno086ShtpVectors();
        bno086ReportsVectors();
        bno055Vectors();
        tmf8829Vectors();
        tmf8829FrameTimeVectors();
        tmf8829FrameTimeRecording();
        datasetVector();
        sr04SessionReplay();
        vl53l4cdSessionReplay();
        bno055ReplayVectors();
        vl53l8chReplayVectors();
        vl53lxDieReplayVectors();
        bno086SessionReplay();
        bno086CanonicalReplay();
        inputRulesVectors();
        difftestRegressions();

        System.out.println();
        System.out.printf("TOTAL: %d/%d vector cases passed%n", passed, consumed);
        if (!failures.isEmpty()) {
            System.out.println("\nFAILURES:");
            for (String f : failures) {
                System.out.println("  - " + f);
            }
            System.exit(1);
        }
        System.out.println("ALL VECTORS PASSED");
    }

    // ── assertion plumbing ────────────────────────────────────────────────────

    private static void check(String label, boolean ok) {
        consumed++;
        if (ok) {
            passed++;
        } else {
            failures.add(label);
        }
    }

    /** Failure count now; pass it to {@link #seen} after a case's checks. */
    private static int mark() {
        return failures.size();
    }

    /**
     * Report a golden-vector case ID as asserted (scripts/check_vector_coverage.py):
     * appended to {@code $DEPZ_VECTOR_SEEN}, only when no check failed since
     * {@code mark}. No-op without the env var. O_APPEND keeps parallel writers whole.
     */
    private static void seen(String id, int mark) {
        String path = System.getenv("DEPZ_VECTOR_SEEN");
        if (path == null || path.isEmpty() || failures.size() != mark) {
            return;
        }
        try {
            Files.writeString(Path.of(path), id + "\n", java.nio.charset.StandardCharsets.UTF_8,
                java.nio.file.StandardOpenOption.CREATE, java.nio.file.StandardOpenOption.APPEND);
        } catch (java.io.IOException e) {
            throw new java.io.UncheckedIOException(e);
        }
    }

    /** {@link #seen(String, int)} for a case object: its {@code id}. */
    private static void seen(Map<String, Object> c, int mark) {
        seen((String) c.get("id"), mark);
    }

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

    private static int intOf(Object o) {
        return (int) ((Long) o).longValue();
    }

    private static Map<String, Object> load(String file) throws Exception {
        String text = Files.readString(vectorsDir.resolve(file));
        return obj(Json.parse(text));
    }

    // ── hex ───────────────────────────────────────────────────────────────────

    private static byte[] hex(String h) {
        int n = h.length() / 2;
        byte[] out = new byte[n];
        for (int i = 0; i < n; i++) {
            out[i] = (byte) Integer.parseInt(h.substring(i * 2, i * 2 + 2), 16);
        }
        return out;
    }

    private static final char[] HEXCH = "0123456789abcdef".toCharArray();

    private static String hex(byte[] b) {
        StringBuilder sb = new StringBuilder(b.length * 2);
        for (byte x : b) {
            sb.append(HEXCH[(x >>> 4) & 0xF]).append(HEXCH[x & 0xF]);
        }
        return sb.toString();
    }

    // ── input_rules (contract 07 §3) ──────────────────────────────────────────

    /**
     * The {@code input_rules} groups: which inputs the SDK rejects and that
     * trailing payload bytes are ignored. Each case is {@code {op, in, expect}}
     * in the differential-test format (contract 16) and runs through the same
     * SDK calls as {@link DifftestAdapter}. A case whose op the adapter declares
     * unsupported is skipped (and so stays unreported).
     */
    private static void inputRulesVectors() throws Exception {
        // every top-level vector file that carries an input_rules group
        List<String> files = new ArrayList<>();
        try (java.util.stream.Stream<Path> s = Files.list(vectorsDir)) {
            s.map(f -> f.getFileName().toString()).filter(n -> n.endsWith(".json")).sorted().forEach(files::add);
        }
        int found = 0;
        for (String file : files) {
            Object root = Json.parse(Files.readString(vectorsDir.resolve(file)));
            if (!(root instanceof Map) || !obj(root).containsKey("input_rules")) {
                continue;
            }
            found++;
            System.out.println("\n[" + file + " input_rules]");
            List<Object> cases = arr(obj(root).get("input_rules"));
            check(file + " input_rules: has cases", cases != null && !cases.isEmpty());
            for (Object co : cases) {
                Map<String, Object> c = obj(co);
                String id = (String) c.get("id");
                if (DifftestAdapter.UNSUPPORTED.containsKey((String) c.get("op"))) {
                    System.out.println("  skip " + id + " (op unsupported in Java)");
                    continue;
                }
                int mk = mark();
                Map<String, Object> req = new java.util.LinkedHashMap<>();
                req.put("id", 0L);
                req.put("op", c.get("op"));
                req.put("in", c.get("in"));
                StringBuilder line = new StringBuilder();
                DifftestAdapter.canon(req, line);
                Map<String, Object> resp = obj(Json.parse(DifftestAdapter.handle(line.toString())));
                Map<String, Object> expect = obj(c.get("expect"));
                boolean ok = expect.containsKey("error")
                    ? "invalid".equals(resp.get("error"))
                    : !resp.containsKey("error") && Objects.equals(resp.get("out"), expect);
                check(id + " -> " + resp, ok);
                seen(c, mk);
            }
        }
        check("input_rules groups found (" + found + ")", found >= 10);
    }

    /** True when {@code r} throws IllegalArgumentException (or a subclass / an SDK error). */
    private static boolean rejects(Runnable r) {
        try {
            r.run();
            return false;
        } catch (RuntimeException e) {
            return true;
        }
    }

    /** Divergences the differential test found (contract 16), pinned. */
    private static void difftestRegressions() {
        System.out.println("\n[difftest regressions]");
        // A short stream payload is rejected, never zero-filled (contract 07 §3).
        check("bno055 stream shorter than its len", rejects(() -> ai.depz.sensor.protocol.Bno055.StreamData.unpack(
            hex("11b90b3100000000082ea2"))));
        check("vl53l4 stream shorter than its len", rejects(() -> ai.depz.sensor.protocol.Vl53l4.StreamData.unpack(
            hex("59cd1e21000000008900110009"))));
        // Encoder arguments are range-checked, never masked.
        check("bno055 write_reg addr -1", rejects(() -> ai.depz.sensor.protocol.Bno055.packWriteReg(-1, new byte[] {1})));
        check("shtp header length 32769", rejects(() -> Shtp.packShtpHeader(new Shtp.ShtpHeader(32769, 3, 0, false))));
        check("framing seq -1", rejects(() -> Framing.buildPacket(4, new byte[0], -1, CrcType.NONE)));
        check("vl53l4 xtalk 128", rejects(() -> Vl53l4Uld.xtalkRaw(128)));
        check("vl53l8 xtalk margin -1", rejects(() -> Vl53l8Uld.xtalkMarginToRaw(-1)));
        check("bno055 block addr -1", rejects(() -> Bno055Regs.decodeBlock(-1, new byte[6])));
        // A one-word FRS write is still 12 bytes.
        check("frs_write_data one word", hex(Sh2.buildFrsWriteData(73, new long[] {0x11223344L}))
            .equals("f60049004433221100000000"));
    }

    // ── crc.json ──────────────────────────────────────────────────────────────

    private static void crcVectors() throws Exception {
        System.out.println("\n[crc.json]");
        for (Object co : arr(load("crc.json").get("cases"))) {
            Map<String, Object> c = obj(co);
            int mk = mark();
            String name = (String) c.get("name");
            byte[] in = hex((String) c.get("input"));
            check("crc8:" + name, Crc.crc8Maxim(in) == lng(c.get("crc8_maxim")));
            check("crc16:" + name, Crc.crc16Modbus(in) == lng(c.get("crc16_modbus")));
            check("crc32:" + name, Crc.crc32IsoHdlc(in) == lng(c.get("crc32_iso_hdlc")));
            check("crc16ccitt:" + name, Crc.crc16CcittFalse(in) == lng(c.get("crc16_ccitt_false")));
            seen(c, mk);
        }
    }

    // ── framing_encode.json ───────────────────────────────────────────────────

    private static void framingEncodeVectors() throws Exception {
        System.out.println("[framing_encode.json]");
        for (Object co : arr(load("framing_encode.json").get("cases"))) {
            Map<String, Object> c = obj(co);
            int mk = mark();
            String name = (String) c.get("name");
            byte[] frame = Framing.buildPacket(
                intOf(c.get("cmd")),
                hex((String) c.get("payload")),
                intOf(c.get("seq")),
                CrcType.fromValue(intOf(c.get("crc_type"))));
            check("encode:" + name, hex(frame).equals(c.get("frame")));
            seen(c, mk);
        }
    }

    // ── framing_decode.json ───────────────────────────────────────────────────

    private static String describe(List<Event> events, String trashHex, String residueHex, int headerErrors) {
        StringBuilder sb = new StringBuilder();
        for (Event e : events) {
            if (e instanceof Packet p) {
                sb.append("P|").append(p.cmd()).append('|').append(p.seq()).append('|').append(hex(p.payload())).append(";;");
            } else if (e instanceof CrcError ce) {
                sb.append("C|").append(ce.cmd()).append('|').append(ce.seq()).append(";;");
            }
        }
        sb.append("#trash=").append(trashHex);
        sb.append("#residue=").append(residueHex);
        sb.append("#hdrerr=").append(headerErrors);
        return sb.toString();
    }

    private static String collect(byte[][] chunks) {
        PacketParser parser = new PacketParser();
        List<Event> events = new ArrayList<>();
        StringBuilder trash = new StringBuilder();
        for (byte[] chunk : chunks) {
            for (Event e : parser.feed(chunk)) {
                if (e instanceof Packet || e instanceof CrcError) {
                    events.add(e);
                } else if (e instanceof Trash t) {
                    trash.append(hex(t.data()));
                }
            }
        }
        return describe(events, trash.toString(), hex(parser.residue()), parser.headerErrors);
    }

    private static String expectedDecode(Map<String, Object> expect) {
        List<Event> events = new ArrayList<>();
        for (Object eo : arr(expect.get("events"))) {
            Map<String, Object> e = obj(eo);
            String type = (String) e.get("type");
            if (type.equals("packet")) {
                events.add(new Packet(intOf(e.get("cmd")), intOf(e.get("seq")), hex((String) e.get("payload"))));
            } else {
                events.add(new CrcError(intOf(e.get("cmd")), intOf(e.get("seq"))));
            }
        }
        return describe(events, (String) expect.get("trash"), (String) expect.get("residue"), intOf(expect.get("header_errors")));
    }

    private static byte[][] wholeChunks(byte[] stream) {
        return new byte[][] {stream};
    }

    private static byte[][] bytewiseChunks(byte[] stream) {
        byte[][] out = new byte[stream.length][];
        for (int i = 0; i < stream.length; i++) {
            out[i] = new byte[] {stream[i]};
        }
        return out;
    }

    private static byte[][] randomChunks(byte[] stream, Random rng) {
        List<byte[]> out = new ArrayList<>();
        int i = 0;
        while (i < stream.length) {
            int n = 1 + rng.nextInt(37);
            int end = Math.min(i + n, stream.length);
            out.add(java.util.Arrays.copyOfRange(stream, i, end));
            i = end;
        }
        return out.toArray(new byte[0][]);
    }

    private static void framingDecodeVectors() throws Exception {
        System.out.println("[framing_decode.json]");
        Random rng = new Random(0xDE92);
        for (Object co : arr(load("framing_decode.json").get("cases"))) {
            Map<String, Object> c = obj(co);
            int mk = mark();
            String name = (String) c.get("name");
            byte[] stream = hex((String) c.get("stream"));
            String expected = expectedDecode(obj(c.get("expect")));

            boolean ok = collect(wholeChunks(stream)).equals(expected);
            ok &= collect(bytewiseChunks(stream)).equals(expected);
            for (int round = 0; round < 5 && ok; round++) {
                ok &= collect(randomChunks(stream, rng)).equals(expected);
            }
            check("decode:" + name, ok);
            seen(c, mk);
        }
    }

    // ── probe.json (class resolution, contract 02 §4.4) ──────────────────────

    private static void probeVectors() throws Exception {
        System.out.println("\n[probe.json]");
        Map<String, Object> v = load("probe.json");
        int mk = mark();
        check("probe.classes", Discovery.SENSOR_CLASS_NAMES.equals(arr(v.get("classes"))));
        seen("probe/classes", mk);
        for (Object co : arr(v.get("resolve"))) {
            Map<String, Object> c = obj(co);
            mk = mark();
            Object vid = c.get("usb_vid");
            Object pid = c.get("usb_pid");
            String got = Discovery.resolveSensorClass((String) c.get("software_name"),
                (String) c.get("device_name"), vid == null ? null : intOf(vid),
                pid == null ? null : intOf(pid));
            check((String) c.get("id") + " -> " + got, got.equals(c.get("class"))
                && Discovery.SENSOR_CLASS_NAMES.contains(got));
            seen(c, mk);
        }
    }

    // ── usb_ids.json ──────────────────────────────────────────────────────────

    private static void usbIdsVectors() throws Exception {
        System.out.println("[usb_ids.json]");
        Map<String, Object> root = load("usb_ids.json");
        int mkScalar = mark();
        check("usb:vid", UsbIds.DEPZ_USB_VID == intOf(root.get("vid")));
        seen("usb_ids/vid", mkScalar);
        mkScalar = mark();
        check("usb:dev_vid", UsbIds.DEV_USB_VID == intOf(root.get("dev_vid")));
        seen("usb_ids/dev_vid", mkScalar);
        mkScalar = mark();
        check("usb:dev_pid", UsbIds.DEV_USB_PID == intOf(root.get("dev_pid")));
        seen("usb_ids/dev_pid", mkScalar);
        mkScalar = mark();
        List<Object> range = arr(root.get("pid_range"));
        check("usb:pid_range", range.size() == 2
            && UsbIds.DEPZ_PID_RANGE_LO == intOf(range.get(0))
            && UsbIds.DEPZ_PID_RANGE_HI == intOf(range.get(1)));
        seen("usb_ids/pid_range", mkScalar);
        for (Object io : arr(root.get("identity"))) {
            Map<String, Object> c = obj(io);
            int mk = mark();
            Integer vid = intOf(c.get("vid"));
            Integer pid = intOf(c.get("pid"));
            boolean known = (Boolean) c.get("known");
            String model = (String) c.get("model");
            String label = "usb:" + vid + ":" + pid;
            check(label + ":known", UsbIds.isKnownDepzUsb(vid, pid) == known);
            String hint = UsbIds.usbModelHint(vid, pid);
            check(label + ":model", java.util.Objects.equals(hint, model));
            seen(c, mk);
        }
        for (Object so : arr(root.get("serial_ordering"))) {
            Map<String, Object> c = obj(so);
            int mk = mark();
            String name = (String) c.get("name");
            List<UsbIds.PortRef> refs = new ArrayList<>();
            for (Object po : arr(c.get("ports"))) {
                Map<String, Object> p = obj(po);
                refs.add(new UsbIds.PortRef((String) p.get("port"), (String) p.get("serial")));
            }
            List<String> got = new ArrayList<>();
            for (UsbIds.PortRef r : UsbIds.orderBySerial(refs)) {
                got.add(r.port());
            }
            List<String> want = new ArrayList<>();
            for (Object o : arr(c.get("order"))) {
                want.add((String) o);
            }
            check("order:" + name, got.equals(want));
            seen(c, mk);
        }
    }

    // ── identity.json ─────────────────────────────────────────────────────────

    private static void identityVectors() throws Exception {
        System.out.println("[identity.json]");
        for (Object co : arr(load("identity.json").get("cases"))) {
            Map<String, Object> c = obj(co);
            int mk = mark();
            String name = (String) c.get("name");
            byte[] raw = hex((String) c.get("raw"));
            Identity ident = Identity.parseSoftwareName(Common.stripDeviceString(raw));
            Map<String, Object> ex = obj(c.get("expect"));
            String sensor = ident.sensorType() == null ? null : ident.sensorType().label;
            boolean ok = ident.mode().equals(ex.get("mode"))
                && java.util.Objects.equals(sensor, ex.get("sensor_type"))
                && ident.softwareName().equals(ex.get("software_name"))
                && ident.version().equals(ex.get("version"));
            check("identity:" + name, ok);
            seen(c, mk);
        }
    }

    // ── common_commands.json ──────────────────────────────────────────────────

    private static void commonVectors() throws Exception {
        System.out.println("[common_commands.json]");
        Map<String, Object> root = load("common_commands.json");
        for (Object eo : arr(root.get("encode"))) {
            Map<String, Object> c = obj(eo);
            int mk = mark();
            String name = (String) c.get("name");
            String kind = (String) c.get("kind");
            byte[] got;
            switch (kind) {
                case "sync_time_request":
                    got = Common.packSyncTime(lng(c.get("pc_timestamp_us")));
                    break;
                case "set_payload_crc_type":
                    got = new byte[] {(byte) intOf(c.get("crc_type"))};
                    break;
                case "sync_pin_config":
                    got = new Common.SyncPinConfig(
                        intOf(c.get("pin")),
                        Common.SyncPinMode.fromValue(intOf(c.get("mode"))),
                        Common.SyncPinPolarity.fromValue(intOf(c.get("polarity")))).pack();
                    break;
                default:
                    throw new RuntimeException("unknown encode kind " + kind);
            }
            check("common.encode:" + name, hex(got).equals(c.get("payload")));
            seen(c, mk);
        }
        for (Object do_ : arr(root.get("decode"))) {
            Map<String, Object> c = obj(do_);
            int mk = mark();
            String name = (String) c.get("name");
            int report = intOf(c.get("report"));
            byte[] payload = hex((String) c.get("payload"));
            Map<String, Object> ex = obj(c.get("expect"));
            boolean ok;
            switch (report) {
                case 0x80: {
                    Common.StatusReport r = Common.StatusReport.unpack(payload);
                    ok = r.cmd() == intOf(ex.get("cmd")) && r.status() == intOf(ex.get("status"));
                    break;
                }
                case 0x81: {
                    Common.TextReport r = Common.TextReport.unpack(payload);
                    ok = r.cmd() == intOf(ex.get("cmd")) && r.text().equals(ex.get("text"));
                    break;
                }
                case 0x83: {
                    Common.TemperatureReport r = Common.TemperatureReport.unpack(payload);
                    ok = r.timestampUs() == lng(ex.get("timestamp_us")) && r.rawDecidegrees() == intOf(ex.get("raw_decidegrees"));
                    break;
                }
                case 0x84: {
                    Common.SequenceErrorReport r = Common.SequenceErrorReport.unpack(payload);
                    ok = r.expectedSeq() == intOf(ex.get("expected_seq")) && r.receivedSeq() == intOf(ex.get("received_seq"));
                    break;
                }
                default:
                    throw new RuntimeException("unknown report " + report);
            }
            check("common.decode:" + name, ok);
            seen(c, mk);
        }
        for (Object mo : arr(root.get("sync_time_math"))) {
            Map<String, Object> c = obj(mo);
            int mk = mark();
            String name = (String) c.get("name");
            long[] r = Common.syncTimeOffsetRtt(lng(c.get("t1")), lng(c.get("t2")), lng(c.get("t3")), lng(c.get("t4")));
            check("common.synctime:" + name, r[0] == lng(c.get("offset_us")) && r[1] == lng(c.get("rtt_us")));
            seen(c, mk);
        }
    }

    // ── sr04.json ─────────────────────────────────────────────────────────────

    private static void sr04Vectors() throws Exception {
        System.out.println("[sr04.json]");
        Map<String, Object> root = load("sr04.json");
        for (Object eo : arr(root.get("encode"))) {
            Map<String, Object> c = obj(eo);
            int mk = mark();
            String name = (String) c.get("name");
            String kind = (String) c.get("kind");
            byte[] got;
            if (kind.equals("set_sample_period")) {
                got = Sr04.packSamplePeriod(lng(c.get("period_us")));
            } else if (kind.equals("set_echo_decay")) {
                got = Sr04.packEchoDecay(intOf(c.get("decay_us")));
            } else {
                throw new RuntimeException("unknown sr04 kind " + kind);
            }
            check("sr04.encode:" + name, hex(got).equals(c.get("payload")));
            seen(c, mk);
        }
        for (Object do_ : arr(root.get("decode"))) {
            Map<String, Object> c = obj(do_);
            int mk = mark();
            String name = (String) c.get("name");
            int report = intOf(c.get("report"));
            byte[] payload = hex((String) c.get("payload"));
            Map<String, Object> ex = obj(c.get("expect"));
            boolean ok;
            switch (report) {
                case 0x91: {
                    Sr04.Sr04Data d = Sr04.Sr04Data.unpack(payload);
                    ok = d.sourceCmd() == intOf(ex.get("source_cmd"))
                        && d.timestampUs() == lng(ex.get("timestamp_us"))
                        && d.echoTimeUs() == intOf(ex.get("echo_time_us"));
                    break;
                }
                case 0x92:
                    ok = Sr04.unpackSamplePeriod(payload) == lng(ex.get("period_us"));
                    break;
                case 0x93:
                    ok = Sr04.unpackEchoDecay(payload) == intOf(ex.get("decay_us"));
                    break;
                default:
                    throw new RuntimeException("unknown sr04 report " + report);
            }
            check("sr04.decode:" + name, ok);
            seen(c, mk);
        }
    }

    // ── fwdepz.json ───────────────────────────────────────────────────────────

    private static void fwdepzVectors() throws Exception {
        System.out.println("[fwdepz.json]");
        for (Object co : arr(load("fwdepz.json").get("cases"))) {
            Map<String, Object> c = obj(co);
            int mk = mark();
            String name = (String) c.get("name");
            byte[] blob = hex((String) c.get("file"));
            if (c.containsKey("error")) {
                boolean threw = false;
                try {
                    FwDepz.FwDepzImage.parse(blob);
                } catch (FwDepz.FwDepzError e) {
                    threw = true;
                }
                check("fwdepz:" + name + ":error", threw);
            } else {
                Map<String, Object> ex = obj(c.get("expect"));
                FwDepz.FwDepzImage img = FwDepz.FwDepzImage.parse(blob);
                boolean ok = img.loadAddr() == lng(ex.get("load_addr"))
                    && img.fwSize() == lng(ex.get("fw_size"))
                    && img.fwCrc32() == lng(ex.get("fw_crc32"))
                    && img.curSec() == intOf(ex.get("cur_sec"))
                    && img.totSec() == intOf(ex.get("tot_sec"))
                    && img.payloadCrcOk() == (Boolean) ex.get("payload_crc_ok");
                check("fwdepz:" + name, ok);
            }
            seen(c, mk);
        }
    }

    // ── vl53l4.json (VL53L4CD wire codecs + host-ULD math) ────────────────────

    private static void vl53l4EncodeVectors() throws Exception {
        System.out.println("[vl53l4.json]");
        Map<String, Object> root = load("vl53l4.json");
        for (Object eo : arr(root.get("encode"))) {
            Map<String, Object> c = obj(eo);
            int mk = mark();
            String name = (String) c.get("name");
            String kind = (String) c.get("kind");
            byte[] got;
            switch (kind) {
                case "read_reg":
                    got = Vl53l4.packReadReg(intOf(c.get("addr")), intOf(c.get("len")));
                    break;
                case "write_reg":
                    got = Vl53l4.packWriteReg(intOf(c.get("addr")), hex((String) c.get("data")));
                    break;
                case "xshut":
                    got = Vl53l4.packXshut(intOf(c.get("action")));
                    break;
                case "start_stream":
                    got = Vl53l4.packStartStream(
                        intOf(c.get("addr")), intOf(c.get("len")), intOf(c.get("flags")));
                    break;
                case "set_i2c_speed":
                    got = Vl53l4.packSetI2cSpeed(intOf(c.get("khz")));
                    break;
                default:
                    throw new RuntimeException("unknown vl53l4 encode kind " + kind);
            }
            check("vl53l4.encode:" + name, hex(got).equals(c.get("payload")));
            seen(c, mk);
        }
    }

    private static void vl53l4DecodeVectors() throws Exception {
        Map<String, Object> root = load("vl53l4.json");
        for (Object do_ : arr(root.get("decode"))) {
            Map<String, Object> c = obj(do_);
            int mk = mark();
            String name = (String) c.get("name");
            int report = intOf(c.get("report"));
            byte[] payload = hex((String) c.get("payload"));
            Map<String, Object> ex = obj(c.get("expect"));
            boolean ok;
            switch (report) {
                case 0x91: {
                    Vl53l4.RegData d = Vl53l4.RegData.unpack(payload);
                    ok = d.cmd() == intOf(ex.get("cmd"))
                        && d.timestampUs() == lng(ex.get("timestamp_us"))
                        && hex(d.data()).equals(ex.get("data"));
                    break;
                }
                case 0x92: {
                    Vl53l4.Vl53l4Info i = Vl53l4.Vl53l4Info.unpack(payload);
                    ok = i.intEdges() == lng(ex.get("int_edges"))
                        && i.slotsSkipped() == lng(ex.get("slots_skipped"))
                        && i.i2cErrors() == lng(ex.get("i2c_errors"))
                        && i.lastI2cError() == intOf(ex.get("last_i2c_error"))
                        && i.modelId() == intOf(ex.get("model_id"))
                        && i.fwStatus() == intOf(ex.get("fw_status"))
                        && i.initialized() == intOf(ex.get("initialized"))
                        && i.xshutLevel() == intOf(ex.get("xshut_level"))
                        && i.intLevel() == intOf(ex.get("int_level"))
                        && i.i2cKhz() == intOf(ex.get("i2c_khz"));
                    break;
                }
                case 0x93: {
                    Vl53l4.StreamData s = Vl53l4.StreamData.unpack(payload);
                    ok = s.timestampUs() == lng(ex.get("timestamp_us"))
                        && s.addr() == intOf(ex.get("addr"))
                        && s.len() == intOf(ex.get("len"))
                        && hex(s.data()).equals(ex.get("data"));
                    break;
                }
                default:
                    throw new RuntimeException("unknown vl53l4 report " + report);
            }
            check("vl53l4.decode:" + name, ok);
            seen(c, mk);
        }
    }

    private static void vl53l4ResultBlockVectors() throws Exception {
        Map<String, Object> root = load("vl53l4.json");
        for (Object ro : arr(root.get("result_block"))) {
            Map<String, Object> c = obj(ro);
            int mk = mark();
            String name = (String) c.get("name");
            Vl53l4Uld.Results r = Vl53l4Uld.parseResultBlock(hex((String) c.get("raw")));
            Map<String, Object> ex = obj(c.get("expect"));
            boolean ok = r.rangeStatus() == intOf(ex.get("range_status"))
                && r.distanceMm() == intOf(ex.get("distance_mm"))
                && r.ambientRateKcps() == intOf(ex.get("ambient_rate_kcps"))
                && r.ambientPerSpadKcps() == intOf(ex.get("ambient_per_spad_kcps"))
                && r.signalRateKcps() == intOf(ex.get("signal_rate_kcps"))
                && r.signalPerSpadKcps() == intOf(ex.get("signal_per_spad_kcps"))
                && r.numberOfSpad() == intOf(ex.get("number_of_spad"))
                && r.sigmaMm() == intOf(ex.get("sigma_mm"))
                && r.streamCount() == intOf(ex.get("stream_count"));
            check("vl53l4.result_block:" + name, ok);
            seen(c, mk);
        }
    }

    private static void vl53l4TimingVectors() throws Exception {
        Map<String, Object> timing = obj(load("vl53l4.json").get("timing"));
        int mk = mark();
        for (Object eo : arr(timing.get("encode"))) {
            Map<String, Object> c = obj(eo);
            String name = (String) c.get("name");
            int[] regs = Vl53l4Uld.rangeTimingRegisters(
                intOf(c.get("timing_budget_ms")),
                intOf(c.get("inter_measurement_ms")),
                intOf(c.get("osc_frequency")),
                intOf(c.get("clock_pll")));
            boolean ok = regs[0] == intOf(c.get("range_config_a"))
                && regs[1] == intOf(c.get("range_config_b"))
                && regs[2] == intOf(c.get("intermeasurement_raw"));
            check("vl53l4.timing.encode:" + name, ok);
        }
        for (Object do_ : arr(timing.get("decode"))) {
            Map<String, Object> c = obj(do_);
            String name = (String) c.get("name");
            int[] got = Vl53l4Uld.decodeRangeTiming(
                lng(c.get("intermeasurement_raw")),
                intOf(c.get("clock_pll")),
                intOf(c.get("osc_frequency")),
                intOf(c.get("range_config_a")));
            boolean ok = got[0] == intOf(c.get("timing_budget_ms"))
                && got[1] == intOf(c.get("inter_measurement_ms"));
            check("vl53l4.timing.decode:" + name, ok);
        }
        seen("vl53l4/timing", mk);
    }

    private static void vl53l4TuningVectors() throws Exception {
        Map<String, Object> root = load("vl53l4.json");
        for (Object to : arr(root.get("tuning"))) {
            Map<String, Object> c = obj(to);
            int mk = mark();
            String name = (String) c.get("name");
            String kind = (String) c.get("kind");
            int value = intOf(c.get("value"));
            int raw = intOf(c.get("raw"));
            boolean ok;
            switch (kind) {
                case "offset":
                    ok = Vl53l4Uld.offsetRaw(value) == raw
                        && Vl53l4Uld.decodeOffset(raw) == value;
                    break;
                case "xtalk":
                    ok = Vl53l4Uld.xtalkRaw(value) == raw
                        && Vl53l4Uld.decodeXtalk(raw) == value;
                    break;
                case "signal_threshold":
                    ok = Vl53l4Uld.signalThresholdRaw(value) == raw
                        && Vl53l4Uld.decodeSignalThreshold(raw) == value;
                    break;
                case "sigma_threshold":
                    ok = Vl53l4Uld.sigmaThresholdRaw(value) == raw
                        && Vl53l4Uld.decodeSigmaThreshold(raw) == value;
                    break;
                default:
                    throw new RuntimeException("unknown vl53l4 tuning kind " + kind);
            }
            check("vl53l4.tuning:" + name, ok);
            seen(c, mk);
        }
        // C round(): an exact .5 goes away from zero (256 -> 1), as ST's GetXtalk
        for (Object to : arr(root.get("xtalk_decode"))) {
            Map<String, Object> c = obj(to);
            int mk = mark();
            int raw = intOf(c.get("raw"));
            check("vl53l4.xtalk_decode:" + raw, Vl53l4Uld.decodeXtalk(raw) == intOf(c.get("kcps")));
            seen(c, mk);
        }
    }

    private static void vl53l4ConfigBlockVector() throws Exception {
        Map<String, Object> cb = obj(load("vl53l4.json").get("config_block"));
        int mk = mark();
        check("vl53l4.config_block.addr", Vl53l4Uld.CONFIG_ADDR == intOf(cb.get("addr")));
        check("vl53l4.config_block.data", hex(Vl53l4Uld.configBlock()).equals(cb.get("data")));
        seen("vl53l4/config_block", mk);
    }

    // ── vl53l8_advanced.json (advanced-DCI codecs, shared by CX and CH) ────────

    private static void vl53l8AdvancedVectors() throws Exception {
        System.out.println("[vl53l8_advanced.json]");
        Map<String, Object> root = load("vl53l8_advanced.json");
        for (Object mo : arr(root.get("motion"))) {
            Map<String, Object> c = obj(mo);
            int mk = mark();
            String name = (String) c.get("name");
            int resolution = intOf(c.get("resolution"));
            byte[] packed = Vl53l8Uld.defaultMotionConfig(resolution).pack();
            check("vl53l8.motion:" + name, hex(packed).equals(c.get("pack")));
            seen(c, mk);
        }
        for (Object to : arr(root.get("thresholds"))) {
            Map<String, Object> c = obj(to);
            int mk = mark();
            String name = (String) c.get("name");
            List<Vl53l8Uld.DetectionThreshold> ths = new ArrayList<>();
            for (Object t : arr(c.get("thresholds"))) {
                Map<String, Object> th = obj(t);
                ths.add(new Vl53l8Uld.DetectionThreshold(
                    intOf(th.get("low_thresh")),
                    intOf(th.get("high_thresh")),
                    intOf(th.get("measurement")),
                    intOf(th.get("type")),
                    intOf(th.get("zone_num")),
                    intOf(th.get("operation"))));
            }
            Vl53l8Uld.PackedThresholds p = Vl53l8Uld.packDetectionThresholds(ths);
            check("vl53l8.thresholds.start:" + name, hex(p.start()).equals(c.get("start_block")));
            check("vl53l8.thresholds.valid:" + name, hex(p.valid()).equals(c.get("valid_status")));
            seen(c, mk);
        }
        for (Object xo : arr(root.get("xtalk_margin"))) {
            Map<String, Object> c = obj(xo);
            int mk = mark();
            String name = (String) c.get("name");
            long raw = Vl53l8Uld.xtalkMarginToRaw(((Number) c.get("kcps")).doubleValue());
            check("vl53l8.xtalk:" + name, raw == lng(c.get("raw")));
            seen(c, mk);
        }
    }

    // ── vl53l8_cnh.json (VL53L8CH CNH histogram decode) ───────────────────────

    private static void vl53l8CnhVectors() throws Exception {
        System.out.println("[vl53l8_cnh.json]");
        Map<String, Object> root = load("vl53l8_cnh.json");
        int mk = mark();
        Map<String, Object> config = obj(root.get("config"));
        int nbAgg = intOf(config.get("nb_of_aggregates"));
        int feat = intOf(config.get("feature_length"));
        byte[] raw = hex((String) root.get("cnh_raw"));

        Vl53l8Uld.CnhResult res = Vl53l8Uld.decodeCnh(nbAgg, feat, raw);

        // A block shorter than the config implies is refused, not indexed past.
        boolean threw = false;
        try {
            Vl53l8Uld.decodeCnh(nbAgg, feat, java.util.Arrays.copyOf(raw, raw.length - 1));
        } catch (IllegalArgumentException e) {
            threw = true;
        }
        check("vl53l8.cnh.truncated_block_refused", threw);
        threw = false;
        try {
            Vl53l8Uld.decodeCnh(0, feat, raw);
        } catch (IllegalArgumentException e) {
            threw = true;
        }
        check("vl53l8.cnh.zero_aggregates_refused", threw);

        Map<String, Object> expected = obj(root.get("expected"));
        check("vl53l8.cnh.ref_residual_word",
            res.refResidualWord == lng(expected.get("ref_residual_word")));
        check("vl53l8.cnh.nb_aggregates",
            res.aggregates.length == nbAgg
                && res.aggregates.length == intOf(expected.get("nb_aggregates")));

        List<Object> wantAggs = arr(expected.get("aggregates"));
        int aggPass = 0;
        for (int i = 0; i < nbAgg; i++) {
            Map<String, Object> w = obj(wantAggs.get(i));
            Vl53l8Uld.CnhAggregate g = res.aggregates[i];
            boolean ok = intArrEquals(g.histRaw, arr(w.get("hist_raw")))
                && intArrEquals(g.histScaler, arr(w.get("hist_scaler")));
            check("vl53l8.cnh.agg[" + i + "]", ok);
            if (ok) {
                aggPass++;
            }
        }
        System.out.printf("  cnh aggregates: %d/%d exact match%n", aggPass, nbAgg);
        seen("vl53l8_cnh", mk);
    }

    // ── vl53l8 recording replay (framing + reassembler + decoder) ─────────────
    // The capture is from a VL53L8CX (dev-default) device (software_name
    // APP_VL53L8_v0.9); the results-frame path it exercises is shared with
    // VL53L8CH (FrameReassembler + Vl53l8Uld.parseFrame).
    // Its firmware emits the ULD-2.1.0 footer layout (offset 12), so the decode
    // uses that offset. VL53L8CH frames (VL53LMZ firmware) put it at size-4
    // (Variant.CH); their CNH block is covered by vl53l8CnhVectors.

    private static void vl53l8ReplayVector() throws Exception {
        System.out.println("[recordings/vl53l8_8x8_15hz_3s]");
        int mk = mark();
        Map<String, Object> expected =
            obj(Json.parse(Files.readString(vectorsDir.resolve("recordings/vl53l8_8x8_15hz_3s.expected.json"))));
        List<Object> wantFrames = arr(expected.get("frames"));

        String rec = Files.readString(vectorsDir.resolve("recordings/vl53l8_8x8_15hz_3s.depzrec"));
        PacketParser parser = new PacketParser();
        FrameReassembler reasm = new FrameReassembler();
        List<Vl53l8Uld.Results> decoded = new ArrayList<>();
        List<Long> timestamps = new ArrayList<>();

        for (String line : rec.split("\n")) {
            if (line.isBlank()) {
                continue;
            }
            Object parsedLine = Json.parse(line);
            if (!(parsedLine instanceof Map)) {
                continue;
            }
            Map<String, Object> ev = obj(parsedLine);
            if (!"rx".equals(ev.get("dir"))) {
                continue;
            }
            byte[] data = hex((String) ev.get("data"));
            for (Event e : parser.feed(data)) {
                if (!(e instanceof Packet pkt) || pkt.cmd() != 0x93) {
                    continue;
                }
                FrameReassembler.CompletedFrame done =
                    reasm.feed(FrameReassembler.unpackFrameChunk(pkt.payload()));
                if (done == null) {
                    continue;
                }
                Vl53l8Uld.Results r =
                    Vl53l8Uld.parseFrame(done.frame(), done.frame().length, Vl53l8Uld.FOOTER_ID_OFF_CX);
                decoded.add(r);
                timestamps.add(done.timestampUs());
            }
        }

        // The capture streamed one more frame than the hand-checked sidecar; the
        // reference replay consumes exactly `wantFrames.size()` and ignores the
        // trailing frame. Require at least that many, then match the first N.
        check("vl53l8.replay.count", decoded.size() >= wantFrames.size());
        int n = Math.min(decoded.size(), wantFrames.size());
        for (int i = 0; i < n; i++) {
            Map<String, Object> want = obj(wantFrames.get(i));
            Vl53l8Uld.Results got = decoded.get(i);
            boolean ok = timestamps.get(i) == lng(want.get("timestamp_us"))
                && got.resolution() == intOf(want.get("resolution"))
                && got.siliconTempDegc == intOf(want.get("silicon_temp_degc"))
                && intArrEquals(got.distanceMm, arr(want.get("distance_mm")))
                && intArrEquals(got.targetStatus, arr(want.get("target_status")))
                && intArrEquals(got.nbTargetDetected, arr(want.get("nb_target_detected")));
            check("vl53l8.replay.frame[" + i + "]", ok);
        }
        seen("recordings/vl53l8_8x8_15hz_3s.depzrec", mk);
    }

    // ── vl53l7.json (VL53L5CX/L7CX/L7CH I2C bridge, contract 11) ─────────────

    private static void vl53l7Vectors() throws Exception {
        System.out.println("[vl53l7.json]");
        Map<String, Object> root = load("vl53l7.json");
        for (Object eo : arr(root.get("encode"))) {
            Map<String, Object> c = obj(eo);
            int mk = mark();
            String name = (String) c.get("name");
            String kind = (String) c.get("kind");
            byte[] got;
            switch (kind) {
                case "read_reg":
                    got = Vl53l7.packReadReg(intOf(c.get("addr")), intOf(c.get("len")));
                    break;
                case "write_reg":
                    got = Vl53l7.packWriteReg(intOf(c.get("addr")), hex((String) c.get("data")));
                    break;
                case "pin_ctrl":
                    got = Vl53l7.packPinCtrl(intOf(c.get("action")));
                    break;
                case "set_i2c_speed":
                    got = Vl53l7.packSetI2cSpeed(intOf(c.get("khz")));
                    break;
                default:
                    throw new RuntimeException("unknown vl53l7 encode kind " + kind);
            }
            check("vl53l7.encode:" + name, hex(got).equals(c.get("payload")));
            seen(c, mk);
        }
        check("vl53l7.encode:pin_action_enum",
            Vl53l7.PinAction.LPN_OFF.value == 0 && Vl53l7.PinAction.LPN_ON.value == 1
                && Vl53l7.PinAction.I2C_RST.value == 2 && Vl53l7.PinAction.SOFT_CYCLE.value == 3);
        check("vl53l7.limits",
            Vl53l7.READ_MAX_LEN == 1536 && Vl53l7.WRITE_MAX_LEN == 2048
                && Vl53l7.STREAM_CHUNK_MAX == 1536 && Vl53l7.INFO_SIZE == 20);

        for (Object do_ : arr(root.get("decode"))) {
            Map<String, Object> c = obj(do_);
            int mk = mark();
            String name = (String) c.get("name");
            int report = intOf(c.get("report"));
            if (report != Vl53l7.Vl53l7Rpt.VL53_INFO.value) {
                throw new RuntimeException("unknown vl53l7 report " + report);
            }
            Vl53l7.Vl53l7Info i = Vl53l7.Vl53l7Info.unpack(hex((String) c.get("payload")));
            Map<String, Object> ex = obj(c.get("expect"));
            boolean ok = i.intEdges() == lng(ex.get("int_edges"))
                && i.framesDropped() == lng(ex.get("frames_dropped"))
                && i.i2cErrors() == lng(ex.get("i2c_errors"))
                && i.lastI2cError() == intOf(ex.get("last_i2c_error"))
                && i.lpnLevel() == intOf(ex.get("lpn_level"))
                && i.intLevel() == intOf(ex.get("int_level"))
                && i.i2cKhz() == intOf(ex.get("i2c_khz"))
                && i.frameSize() == intOf(ex.get("frame_size"))
                && i.streaming() == (Boolean) ex.get("streaming");
            check("vl53l7.decode:" + name, ok);
            seen(c, mk);
        }
        boolean rejected = false;
        try {
            Vl53l7.Vl53l7Info.unpack(new byte[Vl53l7.INFO_SIZE - 1]);
        } catch (IllegalArgumentException e) {
            rejected = true;
        }
        check("vl53l7.decode:info_rejects_short_payload", rejected);

        for (Object mo : arr(root.get("model"))) {
            Map<String, Object> c = obj(mo);
            int mk = mark();
            String name = (String) c.get("name");
            Vl53l7Uld.Model m = Vl53l7Uld.resolveModel(
                (String) c.get("usb_model"), (String) c.get("device_name"));
            check("vl53l7.model:" + name, m.label.equals(c.get("expect")));
            seen(c, mk);
        }
    }

    // ── vl53lx.json (VL53L 1D family on APP_VL53L0_4, contract 12) ───────────

    private static int[][] pairs(Object o) {
        List<Object> l = arr(o);
        int[][] out = new int[l.size()][];
        for (int i = 0; i < l.size(); i++) {
            List<Object> p = arr(l.get(i));
            out[i] = new int[] {intOf(p.get(0)), intOf(p.get(1))};
        }
        return out;
    }

    private static boolean dieMatches(Vl53l4Uld.Results r, Map<String, Object> ex) {
        return r.rangeStatus() == intOf(ex.get("range_status"))
            && r.distanceMm() == intOf(ex.get("distance_mm"))
            && r.sigmaMm() == intOf(ex.get("sigma_mm"))
            && r.signalRateKcps() == intOf(ex.get("signal_rate_kcps"))
            && r.ambientRateKcps() == intOf(ex.get("ambient_rate_kcps"))
            && r.signalPerSpadKcps() == intOf(ex.get("signal_per_spad_kcps"))
            && r.ambientPerSpadKcps() == intOf(ex.get("ambient_per_spad_kcps"))
            && r.numberOfSpad() == intOf(ex.get("number_of_spad"))
            && r.streamCount() == intOf(ex.get("stream_count"))
            && ex.size() == 9;
    }

    private static void vl53lxVectors() throws Exception {
        System.out.println("[vl53lx.json]");
        Map<String, Object> root = load("vl53lx.json");

        // encode
        for (Object eo : arr(root.get("encode"))) {
            Map<String, Object> c = obj(eo);
            int mk = mark();
            String name = (String) c.get("name");
            String kind = (String) c.get("kind");
            byte[] got;
            switch (kind) {
                case "set_addr_width":
                    got = Vl53lx.packSetAddrWidth(intOf(c.get("width")));
                    break;
                case "read_reg":
                    got = Vl53lx.packReadReg(intOf(c.get("addr")), intOf(c.get("len")));
                    break;
                case "write_reg":
                    got = Vl53lx.packWriteReg(intOf(c.get("addr")), hex((String) c.get("data")));
                    break;
                case "xshut":
                    got = Vl53lx.packXshut(intOf(c.get("action")));
                    break;
                case "set_i2c_speed":
                    got = Vl53lx.packSetI2cSpeed(intOf(c.get("khz")));
                    break;
                case "start_stream":
                    got = Vl53lx.packStartStream(intOf(c.get("addr")), intOf(c.get("len")),
                        pairs(c.get("clear")), intOf(c.get("flags")));
                    break;
                default:
                    throw new RuntimeException("unknown vl53lx encode kind " + kind);
            }
            check("vl53lx.encode:" + name, hex(got).equals(c.get("payload")));
            seen(c, mk);
        }
        boolean fiveRefused = false;
        try {
            Vl53lx.packStartStream(0x0089, 17, new int[][] {
                {0x86, 1}, {0x86, 1}, {0x86, 1}, {0x86, 1}, {0x86, 1}}, 0);
        } catch (IllegalArgumentException e) {
            fiveRefused = true;
        }
        check("vl53lx.encode:start_stream_refuses_five_clear_steps", fiveRefused);
        boolean widthRefused = false;
        try {
            Vl53lx.packSetAddrWidth(3);
        } catch (IllegalArgumentException e) {
            widthRefused = true;
        }
        check("vl53lx.encode:set_addr_width_refuses_3", widthRefused);
        check("vl53lx.cmd_ids",
            Vl53lx.Vl53lxCmd.SET_ADDR_WIDTH.value == 0x39 && Vl53lx.Vl53lxCmd.START_STREAM.value == 0x35
                && Vl53lx.Vl53lxRpt.INFO.value == 0x92 && Vl53lx.INFO_SIZE == 23
                && Vl53lx.CLEAR_STEPS_MAX == 4);

        // decode
        for (Object do_ : arr(root.get("decode"))) {
            Map<String, Object> c = obj(do_);
            int mk = mark();
            String name = (String) c.get("name");
            int report = intOf(c.get("report"));
            if (report != Vl53lx.Vl53lxRpt.INFO.value) {
                throw new RuntimeException("unknown vl53lx report " + report);
            }
            Vl53lx.Vl53lxInfo i = Vl53lx.Vl53lxInfo.unpack(hex((String) c.get("payload")));
            Map<String, Object> ex = obj(c.get("expect"));
            boolean ok = i.intEdges() == lng(ex.get("int_edges"))
                && i.slotsSkipped() == lng(ex.get("slots_skipped"))
                && i.i2cErrors() == lng(ex.get("i2c_errors"))
                && i.lastI2cError() == intOf(ex.get("last_i2c_error"))
                && i.xshutLevel() == intOf(ex.get("xshut_level"))
                && i.intLevel() == intOf(ex.get("int_level"))
                && i.i2cKhz() == intOf(ex.get("i2c_khz"))
                && i.addrWidth() == intOf(ex.get("addr_width"))
                && i.nClear() == intOf(ex.get("n_clear"))
                && i.framesDropped() == lng(ex.get("frames_dropped"))
                && ex.size() == 10;
            check("vl53lx.decode:" + name, ok);
            seen(c, mk);
        }
        boolean shortRejected = false;
        try {
            Vl53lx.Vl53lxInfo.unpack(new byte[Vl53lx.INFO_SIZE - 1]);
        } catch (IllegalArgumentException e) {
            shortRejected = true;
        }
        check("vl53lx.decode:info_rejects_short_payload", shortRejected);

        // products
        List<Object> rows = arr(root.get("products"));
        List<String> order = new ArrayList<>();
        for (Object ro : rows) {
            order.add((String) obj(ro).get("product"));
        }
        check("vl53lx.products:order", order.equals(Vl53lxProducts.PRODUCTS)
            && order.equals(new ArrayList<>(Vl53lxProducts.TABLE.keySet())));
        for (Object ro : rows) {
            Map<String, Object> r = obj(ro);
            int mk = mark();
            String pname = (String) r.get("product");
            Vl53lxProducts.Product p = Vl53lxProducts.product(pname);
            Vl53lxProducts.Bridge b = p.bridge((String) r.get("default_driver"));
            int[][] want = pairs(r.get("clear_steps"));
            boolean ok = p.modelId() == intOf(r.get("model_id"))
                && p.reachMm() == intOf(r.get("reach_mm"))
                && p.driverKinds().equals(arr(r.get("driver_kinds")))
                && p.defaultDriver().equals(r.get("default_driver"))
                && b.addrWidth() == intOf(r.get("addr_width"))
                && b.maxKhz() == intOf(r.get("max_khz"))
                && java.util.Arrays.deepEquals(b.clearSteps(), want);
            check("vl53lx.products:" + pname, ok);
            seen(r, mk);
        }

        // model (class + board-name product)
        for (Object mo : arr(root.get("model"))) {
            Map<String, Object> c = obj(mo);
            int mk = mark();
            String name = (String) c.get("name");
            String dev = (String) c.get("device_name");
            Vl53lxProducts.SensorClass cls = Vl53lxProducts.resolveClass((String) c.get("usb_model"), dev);
            boolean ok = cls.className.equals(c.get("expect_class"))
                && Objects.equals(Vl53lxProducts.productFromBoardName(dev), c.get("expect_product"));
            check("vl53lx.model:" + name, ok);
            seen(c, mk);
        }

        // die_block (l4 variant must also equal contract 10's decode)
        for (Object bo : arr(root.get("die_block"))) {
            Map<String, Object> c = obj(bo);
            int mk = mark();
            String name = (String) c.get("name");
            String variant = (String) c.get("variant");
            byte[] raw = hex((String) c.get("raw"));
            Vl53l4Uld.Results r = Vl53lxDecode.decodeDieBlock(raw, variant);
            boolean ok = dieMatches(r, obj(c.get("expect")));
            if ("l4".equals(variant)) {
                ok = ok && r.equals(Vl53l4Uld.parseResultBlock(raw));
            }
            check("vl53lx.die_block:" + name, ok);
            seen(c, mk);
        }

        // l0x_raw
        for (Object bo : arr(root.get("l0x_raw"))) {
            Map<String, Object> c = obj(bo);
            int mk = mark();
            String name = (String) c.get("name");
            Vl53lxDecode.L0xRaw r = Vl53lxDecode.decodeL0xRaw(hex((String) c.get("raw")));
            Map<String, Object> ex = obj(c.get("expect"));
            boolean ok = r.distanceRaw() == intOf(ex.get("distance_raw"))
                && r.deviceRangeStatus() == intOf(ex.get("device_range_status"))
                && r.signalRateMcps1616() == intOf(ex.get("signal_rate_mcps_1616"))
                && r.ambientRateMcps1616() == intOf(ex.get("ambient_rate_mcps_1616"))
                && r.effectiveSpadCount88() == intOf(ex.get("effective_spad_count_88"))
                && ex.size() == 5;
            check("vl53lx.l0x_raw:" + name, ok);
            seen(c, mk);
        }

        // histogram_raw
        for (Object bo : arr(root.get("histogram_raw"))) {
            Map<String, Object> c = obj(bo);
            int mk = mark();
            String name = (String) c.get("name");
            byte[] raw = hex((String) c.get("raw"));
            byte[] before = raw.clone();
            Vl53lxDecode.HistogramRaw r = Vl53lxDecode.decodeHistogramRaw(raw);
            Map<String, Object> ex = obj(c.get("expect"));
            boolean ok = r.interruptStatus() == intOf(ex.get("interrupt_status"))
                && r.rangeStatus() == intOf(ex.get("range_status"))
                && r.reportStatus() == intOf(ex.get("report_status"))
                && r.streamCount() == intOf(ex.get("stream_count"))
                && r.dssActualEffectiveSpads() == intOf(ex.get("dss_actual_effective_spads"))
                && r.referencePhase() == intOf(ex.get("reference_phase"))
                && r.vcselStart() == intOf(ex.get("vcsel_start"))
                && intArrEquals(r.bins(), arr(ex.get("bins")))
                && ex.size() == 8
                && java.util.Arrays.equals(raw, before);
            check("vl53lx.histogram_raw:" + name, ok);
            seen(c, mk);
        }
    }

    // ── vl53l5cx_* / vl53l7ch_* recording replay (framing + reassembler + decoder)
    // Full-stack captures from live APP_VL53L7_v0.53 boards (contract 11 §5).
    // Decode-only: the rx side goes through framing -> reassembly -> L5/L7 frame
    // parse (footer at size - 4, per-zone arrays trimmed to the resolution).

    private static void vl53l7ReplayVectors() throws Exception {
        vl53l7ReplayVector("vl53l5cx_8x8_15hz_3s", Vl53l7Uld.RESOLUTION_8X8, false);
        // 4x4 pins the L5/L7 output-list rule: per-target blocks stay 64 entries
        // on the wire and the parser trims them to the 16 real zones.
        vl53l7ReplayVector("vl53l5cx_4x4_15hz", Vl53l7Uld.RESOLUTION_4X4, false);
        vl53l7ReplayVector("vl53l7ch_8x8_15hz_3s", Vl53l7Uld.RESOLUTION_8X8, false);
        // 3156-byte CNH frames arrive chunked (1536-byte RPT_VL53_FRAMEs).
        vl53l7ReplayVector("vl53l7ch_cnh_8x8_15hz", Vl53l7Uld.RESOLUTION_8X8, true);
    }

    private static void vl53l7ReplayVector(String stem, int resolution, boolean withCnh) throws Exception {
        System.out.println("[recordings/" + stem + "]");
        int mk = mark();
        Map<String, Object> expected =
            obj(Json.parse(Files.readString(vectorsDir.resolve("recordings/" + stem + ".expected.json"))));
        List<Object> wantFrames = arr(expected.get("frames"));

        String rec = Files.readString(vectorsDir.resolve("recordings/" + stem + ".depzrec"));
        PacketParser parser = new PacketParser();
        FrameReassembler reasm = new FrameReassembler();
        List<Vl53l8Uld.Results> decoded = new ArrayList<>();
        List<Vl53l8Uld.Results> inferred = new ArrayList<>();
        List<Long> timestamps = new ArrayList<>();
        String softwareName = null;
        String deviceName = null;
        int parseErrors = 0;

        for (String line : rec.split("\n")) {
            if (line.isBlank()) {
                continue;
            }
            Object parsedLine = Json.parse(line);
            if (!(parsedLine instanceof Map)) {
                continue;
            }
            Map<String, Object> ev = obj(parsedLine);
            if (!"rx".equals(ev.get("dir"))) {
                continue;
            }
            for (Event e : parser.feed(hex((String) ev.get("data")))) {
                if (!(e instanceof Packet pkt)) {
                    continue;
                }
                if (pkt.cmd() == Common.Rpt.TEXT.value) {
                    Common.TextReport t = Common.TextReport.unpack(pkt.payload());
                    if (t.cmd() == Common.Cmd.GET_NAME_ACTIVE_SOFTWARE.value && softwareName == null) {
                        softwareName = t.text();
                    } else if (t.cmd() == Common.Cmd.GET_DEVICE_NAME.value && deviceName == null) {
                        deviceName = t.text();
                    }
                    continue;
                }
                if (pkt.cmd() != Vl53l7.Vl53l7Rpt.VL53_FRAME.value) {
                    continue;
                }
                FrameReassembler.CompletedFrame done =
                    reasm.feed(FrameReassembler.unpackFrameChunk(pkt.payload()));
                if (done == null) {
                    continue;
                }
                try {
                    decoded.add(Vl53l7Uld.parseFrame(done.frame(), done.frame().length, resolution));
                    inferred.add(Vl53l7Uld.parseFrame(done.frame(), done.frame().length));
                    timestamps.add(done.timestampUs());
                } catch (Vl53l8Uld.Vl53l8Error err) {
                    parseErrors++;
                }
            }
        }

        String p = "vl53l7.replay." + stem;
        Identity ident = softwareName == null ? null : Identity.parseSoftwareName(softwareName);
        check(p + ".identity", ident != null
            && ident.sensorType() == Identity.SensorType.VL53L7
            && ident.softwareName().equals(expected.get("software_name")));
        // The class comes from the recorded device name: the firmware name alone
        // (APP_VL53L7_*) cannot tell L5CX / L7CX / L7CH apart.
        check(p + ".model", deviceName != null
            && Vl53l7Uld.resolveModel(null, deviceName).label
                .equals(((String) expected.get("class")).toLowerCase()));
        check(p + ".parse_errors", parseErrors == 0);
        // The capture may stream frames past the sidecar; match the first N.
        check(p + ".count", decoded.size() >= wantFrames.size());
        int n = Math.min(decoded.size(), wantFrames.size());
        int framePass = 0;
        for (int i = 0; i < n; i++) {
            Map<String, Object> want = obj(wantFrames.get(i));
            Vl53l8Uld.Results got = decoded.get(i);
            boolean ok = intOf(want.get("resolution")) == resolution
                && vl53l7FrameMatches(got, timestamps.get(i), want, withCnh)
                && vl53l7FrameMatches(inferred.get(i), timestamps.get(i), want, withCnh);
            check(p + ".frame[" + i + "]", ok);
            if (ok) {
                framePass++;
            }
        }
        System.out.printf("  frames: %d/%d exact match%n", framePass, wantFrames.size());
        seen("recordings/" + stem + ".depzrec", mk);
    }

    private static boolean vl53l7FrameMatches(
            Vl53l8Uld.Results got, long timestampUs, Map<String, Object> want, boolean withCnh) {
        boolean ok = timestampUs == lng(want.get("timestamp_us"))
            && got.resolution() == intOf(want.get("resolution"))
            && got.siliconTempDegc == intOf(want.get("silicon_temp_degc"))
            && intArrEquals(got.distanceMm, arr(want.get("distance_mm")))
            && intArrEquals(got.targetStatus, arr(want.get("target_status")))
            && intArrEquals(got.nbTargetDetected, arr(want.get("nb_target_detected")));
        int nt = got.resolution() * Vl53l8Uld.NB_TARGET_PER_ZONE;
        ok &= got.ambientPerSpad.length == got.resolution()
            && got.nbSpadsEnabled.length == got.resolution()
            && got.signalPerSpad.length == nt
            && got.rangeSigmaMm.length == nt
            && got.reflectance.length == nt;
        if (withCnh) {
            ok &= got.cnhRaw != null && hex(got.cnhRaw).equals(want.get("cnh_raw"));
        } else {
            ok &= got.cnhRaw == null && !want.containsKey("cnh_raw");
        }
        return ok;
    }

    // ── bno086_shtp.json ──────────────────────────────────────────────────────

    private static void bno086ShtpVectors() throws Exception {
        System.out.println("[bno086_shtp.json]");
        Map<String, Object> root = load("bno086_shtp.json");
        for (Object ho : arr(root.get("header"))) {
            Map<String, Object> c = obj(ho);
            int mk = mark();
            String name = (String) c.get("name");
            Shtp.ShtpHeader hdr = new Shtp.ShtpHeader(
                intOf(c.get("length")), intOf(c.get("channel")), intOf(c.get("seq")),
                (Boolean) c.get("continuation"));
            boolean ok = hex(Shtp.packShtpHeader(hdr)).equals(c.get("bytes"));
            Shtp.ShtpHeader back = Shtp.unpackShtpHeader(hex((String) c.get("bytes")));
            ok &= back.equals(hdr);
            check("bno.shtp.header:" + name, ok);
            seen(c, mk);
        }
        Shtp.ShtpLayer txLayer = new Shtp.ShtpLayer();
        int txIdx = 0;
        for (Object to : arr(root.get("tx_seq"))) {
            Map<String, Object> c = obj(to);
            int mk = mark();
            byte[] frame = txLayer.nextFrame(intOf(c.get("channel")), hex((String) c.get("payload")));
            check("bno.shtp.tx[" + txIdx++ + "]", hex(frame).equals(c.get("frame")));
            seen(c, mk);
        }
        for (Object ro : arr(root.get("reassembly"))) {
            Map<String, Object> c = obj(ro);
            int mk = mark();
            String name = (String) c.get("name");
            List<Object> frames = arr(c.get("frames"));
            List<Object> captures = arr(c.get("captures_us"));
            Shtp.ShtpLayer rxLayer = new Shtp.ShtpLayer();
            List<Shtp.ShtpCargo> cargos = new ArrayList<>();
            for (int i = 0; i < frames.size(); i++) {
                Shtp.ShtpCargo got = rxLayer.feed(hex((String) frames.get(i)), lng(captures.get(i)));
                if (got != null) {
                    cargos.add(got);
                }
            }
            Map<String, Object> ex = obj(c.get("expect"));
            List<Object> wantCargos = arr(ex.get("cargos"));
            boolean ok = frames.size() == captures.size() && cargos.size() == wantCargos.size()
                && rxLayer.discarded == intOf(ex.get("discarded"));
            for (int i = 0; ok && i < cargos.size(); i++) {
                Map<String, Object> w = obj(wantCargos.get(i));
                Shtp.ShtpCargo g = cargos.get(i);
                ok = g.channel() == intOf(w.get("channel"))
                    && g.seq() == intOf(w.get("seq"))
                    && hex(g.payload()).equals(w.get("payload"))
                    && g.captureUs() == lng(w.get("capture_us"));
            }
            check("bno.shtp.reassembly:" + name, ok);
            seen(c, mk);
        }
        for (Object eo : arr(root.get("control_encode"))) {
            Map<String, Object> c = obj(eo);
            int mk = mark();
            String name = (String) c.get("name");
            String kind = (String) c.get("kind");
            byte[] got;
            switch (kind) {
                case "set_feature":
                    got = Sh2.buildSetFeature(
                        intOf(c.get("sensor_id")), lng(c.get("interval_us")), lng(c.get("batch_us")),
                        intOf(c.get("sensitivity")), intOf(c.get("flags")), lng(c.get("cfg_word")));
                    break;
                case "get_feature_request":
                    got = Sh2.buildGetFeatureRequest(intOf(c.get("sensor_id")));
                    break;
                case "product_id_request":
                    got = Sh2.buildProductIdRequest();
                    break;
                case "command_request":
                    got = Sh2.buildCommandRequest(
                        intOf(c.get("seq")), intOf(c.get("command")), hex((String) c.get("params")));
                    break;
                case "frs_read_request":
                    got = Sh2.buildFrsReadRequest(
                        intOf(c.get("frs_type")), intOf(c.get("offset_words")), intOf(c.get("block_words")));
                    break;
                case "frs_write_request":
                    got = Sh2.buildFrsWriteRequest(intOf(c.get("frs_type")), intOf(c.get("length_words")));
                    break;
                case "frs_write_data": {
                    List<Object> ws = arr(c.get("words"));
                    long[] words = new long[ws.size()];
                    for (int i = 0; i < ws.size(); i++) {
                        words[i] = lng(ws.get(i));
                    }
                    got = Sh2.buildFrsWriteData(intOf(c.get("offset_words")), words);
                    break;
                }
                default:
                    throw new RuntimeException("unknown control_encode kind " + kind);
            }
            check("bno.shtp.control:" + name, hex(got).equals(c.get("payload")));
            seen(c, mk);
        }
        // Executable channel: sleep / power_on (contract 05 §6a); fresh layer → seq 0, 1.
        Shtp.ShtpLayer execLayer = new Shtp.ShtpLayer();
        int execCount = 0;
        for (Object xo : arr(root.get("executable"))) {
            Map<String, Object> c = obj(xo);
            int mk = mark();
            String name = (String) c.get("name");
            int command;
            byte[] payload;
            switch (name) {
                case "sleep":
                    command = Shtp.EXECUTABLE_SLEEP;
                    payload = Sh2.buildSleep();
                    break;
                case "power_on":
                    command = Shtp.EXECUTABLE_ON;
                    payload = Sh2.buildPowerOn();
                    break;
                default:
                    throw new RuntimeException("unknown executable command " + name);
            }
            boolean ok = command == intOf(c.get("command"))
                && Shtp.CHANNEL_EXECUTABLE == intOf(c.get("channel"))
                && hex(payload).equals(c.get("payload"));
            byte[] frame = execLayer.nextFrame(Shtp.CHANNEL_EXECUTABLE, payload);
            ok &= hex(frame).equals(c.get("shtp_frame"));
            byte[] bridge = Framing.buildPacket(
                intOf(c.get("bridge_cmd")), frame, intOf(c.get("bridge_seq")),
                CrcType.fromValue(intOf(c.get("crc_type"))));
            ok &= hex(bridge).equals(c.get("bridge_frame"));
            check("bno.shtp.executable:" + name, ok);
            execCount++;
            seen(c, mk);
        }
        check("bno.shtp.executable:count", execCount == 2);
        Map<String, Object> flags = obj(root.get("feature_flags"));
        int mk = mark();
        check("bno.shtp.feature_flags",
            flags.size() == 2
                && intOf(flags.get("wakeup")) == Sh2.FEATURE_FLAG_WAKEUP
                && intOf(flags.get("always_on")) == Sh2.FEATURE_FLAG_ALWAYS_ON);
        seen("bno086_shtp/feature_flags", mk);
        for (Object io : arr(root.get("interactive_and_flush"))) {
            Map<String, Object> c = obj(io);
            int imk = mark();
            String name = (String) c.get("name");
            byte[] actual = switch ((String) c.get("kind")) {
                case "command_request" -> Sh2.buildCommandRequest(intOf(c.get("seq")), intOf(c.get("command")),
                    Sh2Hub.interactiveCalibrationParams(hex((String) c.get("params"))[0] & 0xFF));
                case "force_flush" -> Sh2Hub.forceFlush(intOf(c.get("sensor_id")));
                default -> throw new RuntimeException("unknown kind " + c.get("kind"));
            };
            check("bno.shtp.interactive_and_flush:" + name, hex(actual).equals(c.get("payload")));
            seen(c, imk);
        }
    }

    // ── bno086_reports.json ───────────────────────────────────────────────────

    private static void bno086ReportsVectors() throws Exception {
        System.out.println("[bno086_reports.json]");
        Map<String, Object> root = load("bno086_reports.json");
        // interactive_and_flush: MotionRequest (0x2B) and the bare 0xEF Flush
        // Completed, in the same input-cargo shape
        bno086QPoints(root);
        List<Object> cargos = new ArrayList<>(arr(root.get("input_cargos")));
        cargos.addAll(arr(root.get("interactive_and_flush")));
        for (Object co : cargos) {
            Map<String, Object> c = obj(co);
            int mk = mark();
            String name = (String) c.get("name");
            byte[] cargo = hex((String) c.get("cargo"));
            long capture = lng(c.get("capture_timestamp_us"));
            List<Reports.Report> got = Reports.parseInputCargo(cargo, capture);
            List<Object> want = arr(c.get("expect"));
            boolean ok = got.size() == want.size();
            for (int i = 0; ok && i < want.size(); i++) {
                Map<String, Object> w = obj(want.get(i));
                Reports.Report g = got.get(i);
                ok = g.type().equals(w.get("type")) && fieldsMatch(g.fields(), obj(w.get("fields")));
                if (ok && w.get("record") != null) {
                    // the contract 09 dataset value, scaled, key for key
                    ok = sameJson(Json.parse(ai.depz.sensor.json.JsonText.serialize(DatasetRecords.bno086(g))), w.get("record"));
                }
            }
            check("bno.report:" + name, ok);
            seen(c, mk);
        }
        for (Object go : arr(root.get("gyro_rv"))) {
            Map<String, Object> c = obj(go);
            int mk = mark();
            String name = (String) c.get("name");
            byte[] cargo = hex((String) c.get("cargo"));
            long capture = lng(c.get("capture_timestamp_us"));
            Reports.Report g = Reports.parseGyroRvCargo(cargo, capture);
            Map<String, Object> ex = obj(c.get("expect"));
            boolean ok = g != null && g.type().equals(ex.get("type"))
                && fieldsMatch(g.fields(), obj(ex.get("fields")));
            if (ok && ex.get("record") != null) {
                ok = sameJson(Json.parse(ai.depz.sensor.json.JsonText.serialize(DatasetRecords.bno086(g))), ex.get("record"));
            }
            check("bno.gyrorv:" + name, ok);
            seen(c, mk);
        }
    }

    // JSON values equal: same keys, same list lengths, numbers by value (an integer equals a real of
    // the same value), everything else exactly.
    private static boolean sameJson(Object got, Object want) {
        if (want instanceof Map<?, ?> w) {
            if (!(got instanceof Map<?, ?> g) || !g.keySet().equals(w.keySet())) {
                return false;
            }
            for (Map.Entry<?, ?> e : w.entrySet()) {
                if (!sameJson(g.get(e.getKey()), e.getValue())) {
                    return false;
                }
            }
            return true;
        }
        if (want instanceof List<?> w) {
            if (!(got instanceof List<?> g) || g.size() != w.size()) {
                return false;
            }
            for (int i = 0; i < w.size(); i++) {
                if (!sameJson(g.get(i), w.get(i))) {
                    return false;
                }
            }
            return true;
        }
        if (want instanceof Number wn && !(want instanceof Boolean)) {
            return got instanceof Number gn && gn.doubleValue() == wn.doubleValue();
        }
        return java.util.Objects.equals(got, want);
    }

    private static void bno086QPoints(Map<String, Object> root) {
        int mk = mark();
        Map<String, Object> want = obj(root.get("q_points"));
        boolean ok = want.size() == Reports.Q_POINTS.size();
        for (Map.Entry<String, Object> e : want.entrySet()) {
            ok &= Integer.valueOf(intOf(e.getValue())).equals(Reports.Q_POINTS.get(Integer.parseInt(e.getKey().substring(2), 16)));
        }
        check("bno.report:q_points", ok);
        seen("bno086_reports/q_points", mk);
        mk = mark();
        check("bno.report:rv_accuracy_q", intOf(root.get("rv_accuracy_q")) == Reports.RV_ACCURACY_Q);
        seen("bno086_reports/rv_accuracy_q", mk);
        mk = mark();
        check("bno.report:gyro_rv_angvel_q", intOf(root.get("gyro_rv_angvel_q")) == Reports.GYRO_RV_ANGVEL_Q);
        seen("bno086_reports/gyro_rv_angvel_q", mk);
    }

    // ── dataset_dual_sr04.depzdata ────────────────────────────────────────────

    private static void datasetVector() throws Exception {
        System.out.println("[recordings/dataset_dual_sr04.depzdata]");
        int mk = mark();
        String content = Files.readString(vectorsDir.resolve("recordings/dataset_dual_sr04.depzdata"));
        Dataset ds = Dataset.parse(content, Json::parse);
        check("dataset.schema", "depz.dataset/1".equals(ds.schema()));
        check("dataset.devices", ds.devices().keySet().equals(new java.util.LinkedHashSet<>(List.of("d0", "d1"))));
        List<Dataset.Record> recs = ds.records();
        check("dataset.count", recs.size() == 10);
        boolean sorted = true;
        boolean allSr04 = true;
        for (int i = 0; i < recs.size(); i++) {
            allSr04 &= "sr04".equals(recs.get(i).kind());
            if (i > 0 && recs.get(i).tHostUs() < recs.get(i - 1).tHostUs()) {
                sorted = false;
            }
        }
        check("dataset.sorted", sorted);
        check("dataset.kinds", allSr04);
        check("dataset.duration", ds.durationUs() == 200256L);
        Dataset.Record first = recs.get(0);
        check("dataset.first", first.deviceId().equals("d0") && first.tHostUs() == 8525788852L
            && ((Number) first.value().get("echo_us")).longValue() == 5831L);
        Dataset.Record last = recs.get(recs.size() - 1);
        check("dataset.last", last.tHostUs() == 8525989108L
            && ((Number) last.value().get("echo_us")).longValue() == 5831L);
        seen("recordings/dataset_dual_sr04.depzdata", mk);
    }

    // ── recording decode (rx side, plus the tx READ_REG each REG_DATA answers) ─
    // Java has no device layer, so these are not strict_tx replays: every rx
    // report goes through framing and the SDK decoders and must match the
    // sidecar. Register read-backs are paired FIFO with the tx READ_REG that
    // asked for them (the bridge answers one read at a time, in order).

    /** One framed packet of a .depzrec capture, in capture order. */
    private record RecPacket(boolean rx, Packet pkt) {}

    /** A REG_DATA reply with the (addr, len) of the request it answers. */
    private record RegRead(int addr, int len, byte[] reply) {}

    private static List<RecPacket> recordingPackets(String file) throws Exception {
        PacketParser txParser = new PacketParser();
        PacketParser rxParser = new PacketParser();
        List<RecPacket> out = new ArrayList<>();
        for (String line : Files.readString(vectorsDir.resolve("recordings/" + file)).split("\n")) {
            if (line.isBlank()) {
                continue;
            }
            Object parsedLine = Json.parse(line);
            if (!(parsedLine instanceof Map)) {
                continue;
            }
            Map<String, Object> ev = obj(parsedLine);
            boolean rx = "rx".equals(ev.get("dir"));
            if (!rx && !"tx".equals(ev.get("dir"))) {
                continue;
            }
            for (Event e : (rx ? rxParser : txParser).feed(hex((String) ev.get("data")))) {
                if (e instanceof Packet p) {
                    out.add(new RecPacket(rx, p));
                }
            }
        }
        return out;
    }

    /** First TEXT reply to {@code cmd} (GET_NAME_ACTIVE_SOFTWARE / GET_DEVICE_NAME). */
    private static String recordedText(List<RecPacket> pkts, Common.Cmd cmd) {
        for (RecPacket rp : pkts) {
            if (rp.rx() && rp.pkt().cmd() == Common.Rpt.TEXT.value) {
                Common.TextReport t = Common.TextReport.unpack(rp.pkt().payload());
                if (t.cmd() == cmd.value) {
                    return t.text();
                }
            }
        }
        return null;
    }

    /**
     * Pair every rx REG_DATA (0x91) with the oldest unanswered tx READ_REG
     * (0x32); an error STATUS for a READ_REG consumes its request. Returns
     * null when a reply has no request left (the pairing lost step).
     */
    private static List<RegRead> regReads(List<RecPacket> pkts, boolean wideAddr) {
        java.util.ArrayDeque<int[]> pending = new java.util.ArrayDeque<>();
        List<RegRead> out = new ArrayList<>();
        for (RecPacket rp : pkts) {
            Packet p = rp.pkt();
            byte[] pl = p.payload();
            if (!rp.rx()) {
                if (p.cmd() == 0x32) {
                    pending.add(wideAddr
                        ? new int[] {(pl[0] & 0xFF) | (pl[1] & 0xFF) << 8, (pl[2] & 0xFF) | (pl[3] & 0xFF) << 8}
                        : new int[] {pl[0] & 0xFF, pl[1] & 0xFF});
                }
            } else if (p.cmd() == 0x91) {
                int[] req = pending.poll();
                if (req == null) {
                    return null;
                }
                out.add(new RegRead(req[0], req[1], pl));
            } else if (p.cmd() == Common.Rpt.STATUS.value) {
                Common.StatusReport s = Common.StatusReport.unpack(pl);
                if (s.cmd() == 0x32 && s.status() != Common.Status.OK.value) {
                    pending.poll();
                }
            }
        }
        return out;
    }

    /** Big-endian register word(s), as the VL53L4CD returns them. */
    private static long be(byte[] b) {
        long v = 0;
        for (byte x : b) {
            v = (v << 8) | (x & 0xFF);
        }
        return v;
    }

    private static void sr04SessionReplay() throws Exception {
        System.out.println("[recordings/sr04_session]");
        int mk = mark();
        Map<String, Object> exp = obj(Json.parse(Files.readString(
            vectorsDir.resolve("recordings/sr04_session.expected.json"))));
        List<RecPacket> pkts = recordingPackets("sr04_session.depzrec");
        String sw = recordedText(pkts, Common.Cmd.GET_NAME_ACTIVE_SOFTWARE);
        Identity ident = sw == null ? null : Identity.parseSoftwareName(sw);
        check("sr04.replay.identity", ident != null && ident.sensorType() == Identity.SensorType.SR04
            && sw.equals(exp.get("software_name")));
        check("sr04.replay.device_name",
            Objects.equals(recordedText(pkts, Common.Cmd.GET_DEVICE_NAME), exp.get("device_name")));
        Long period = null;
        Integer decay = null;
        List<String> once = new ArrayList<>();
        List<String> loop = new ArrayList<>();
        for (RecPacket rp : pkts) {
            if (!rp.rx()) {
                continue;
            }
            int cmd = rp.pkt().cmd();
            byte[] pl = rp.pkt().payload();
            if (cmd == Sr04.Sr04Rpt.SAMPLE_PERIOD.value) {
                period = Sr04.unpackSamplePeriod(pl);
            } else if (cmd == Sr04.Sr04Rpt.ECHO_DECAY.value) {
                decay = Sr04.unpackEchoDecay(pl);
            } else if (cmd == Sr04.Sr04Rpt.DATA.value) {
                Sr04.Sr04Data d = Sr04.Sr04Data.unpack(pl);
                String row = d.timestampUs() + "|" + d.echoTimeUs();
                if (d.sourceCmd() == Sr04.Sr04Cmd.MEASURE_ONCE.value) {
                    once.add(row + "|once");
                } else if (d.sourceCmd() == Sr04.Sr04Cmd.START_MEASUREMENT_LOOP.value) {
                    loop.add(row + "|loop");
                } else {
                    once.add("unknown source " + d.sourceCmd());
                }
            }
        }
        check("sr04.replay.sample_period", period != null && period == lng(exp.get("sample_period_us")));
        check("sr04.replay.echo_decay", decay != null && decay == intOf(exp.get("echo_decay_us")));
        List<String> wantOnce = new ArrayList<>();
        for (Object o : arr(exp.get("once"))) {
            Map<String, Object> w = obj(o);
            wantOnce.add(lng(w.get("timestamp_us")) + "|" + lng(w.get("echo_time_us")) + "|" + w.get("source"));
        }
        List<String> wantLoop = new ArrayList<>();
        for (Object o : arr(exp.get("loop"))) {
            Map<String, Object> w = obj(o);
            wantLoop.add(lng(w.get("timestamp_us")) + "|" + lng(w.get("echo_time_us")) + "|" + w.get("source"));
        }
        check("sr04.replay.once", once.equals(wantOnce));
        // The loop may stream past the sidecar before the stop lands; match the first N.
        check("sr04.replay.loop", loop.size() >= wantLoop.size()
            && loop.subList(0, wantLoop.size()).equals(wantLoop));
        seen("recordings/sr04_session.depzrec", mk);
    }

    /** A VL53L4CD result block vs a sidecar measurement (timestamp + the 9 fields). */
    private static boolean vl53l4MeasurementMatches(Vl53l4Uld.Results r, long timestampUs, Map<String, Object> want) {
        Map<String, Object> fields = new java.util.HashMap<>(want);
        return timestampUs == lng(fields.remove("timestamp_us")) && dieMatches(r, fields);
    }

    private static void vl53l4cdSessionReplay() throws Exception {
        System.out.println("[recordings/vl53l4cd_session]");
        int mk = mark();
        Map<String, Object> exp = obj(Json.parse(Files.readString(
            vectorsDir.resolve("recordings/vl53l4cd_session.expected.json"))));
        List<RecPacket> pkts = recordingPackets("vl53l4cd_session.depzrec");
        String sw = recordedText(pkts, Common.Cmd.GET_NAME_ACTIVE_SOFTWARE);
        Identity ident = sw == null ? null : Identity.parseSoftwareName(sw);
        check("vl53l4cd.replay.identity", ident != null && ident.sensorType() == Identity.SensorType.VL53L4
            && sw.equals(exp.get("software_name")));
        check("vl53l4cd.replay.device_name",
            Objects.equals(recordedText(pkts, Common.Cmd.GET_DEVICE_NAME), exp.get("device_name")));

        Vl53l4.Vl53l4Info info = null;
        List<Vl53l4.StreamData> stream = new ArrayList<>();
        int parseErrors = 0;
        for (RecPacket rp : pkts) {
            if (!rp.rx()) {
                continue;
            }
            if (rp.pkt().cmd() == Vl53l4.Vl53l4Rpt.INFO.value && info == null) {
                info = Vl53l4.Vl53l4Info.unpack(rp.pkt().payload());
            } else if (rp.pkt().cmd() == Vl53l4.Vl53l4Rpt.STREAM.value) {
                Vl53l4.StreamData s = Vl53l4.StreamData.unpack(rp.pkt().payload());
                if (s.addr() == Vl53l4Uld.RESULT_BLOCK_ADDR && s.len() == Vl53l4Uld.RESULT_BLOCK_LEN) {
                    stream.add(s);
                } else {
                    parseErrors++;
                }
            }
        }
        Map<String, Object> wantInfo = obj(exp.get("info"));
        check("vl53l4cd.replay.info", info != null && wantInfo.size() == 3
            && info.modelId() == intOf(wantInfo.get("model_id"))
            && info.fwStatus() == intOf(wantInfo.get("fw_status"))
            && info.i2cKhz() == intOf(wantInfo.get("i2c_khz")));

        // Read-backs: the last read of each register is what the getters saw.
        List<RegRead> reads = regReads(pkts, true);
        check("vl53l4cd.replay.reads_paired", reads != null);
        Map<Integer, byte[]> last = new java.util.HashMap<>();
        List<Vl53l4.RegData> onceReads = new ArrayList<>();
        boolean lengthsOk = true;
        for (RegRead r : reads == null ? List.<RegRead>of() : reads) {
            Vl53l4.RegData d = Vl53l4.RegData.unpack(r.reply());
            lengthsOk &= d.data().length == r.len();
            last.put(r.addr(), d.data());
            if (r.addr() == Vl53l4Uld.RESULT_BLOCK_ADDR && r.len() == Vl53l4Uld.RESULT_BLOCK_LEN) {
                onceReads.add(d);
            }
        }
        check("vl53l4cd.replay.read_lengths", lengthsOk);
        List<Object> timing = arr(exp.get("timing"));
        byte[] im = last.get(Vl53l4Uld.INTERMEASUREMENT_MS);
        byte[] pll = last.get(Vl53l4Uld.RESULT_OSC_CALIBRATE_VAL);
        byte[] osc = last.get(Vl53l4Uld.OSC_FREQUENCY);
        byte[] rca = last.get(Vl53l4Uld.RANGE_CONFIG_A);
        boolean timingOk = im != null && pll != null && osc != null && rca != null;
        if (timingOk) {
            int[] t = Vl53l4Uld.decodeRangeTiming(be(im), (int) be(pll), (int) be(osc), (int) be(rca));
            timingOk = t[0] == intOf(timing.get(0)) && t[1] == intOf(timing.get(1));
        }
        check("vl53l4cd.replay.timing", timingOk);
        byte[] off = last.get(Vl53l4Uld.RANGE_OFFSET_MM);
        check("vl53l4cd.replay.offset",
            off != null && Vl53l4Uld.decodeOffset((int) be(off)) == intOf(exp.get("offset_mm")));
        byte[] xt = last.get(Vl53l4Uld.XTALK_PLANE_OFFSET_KCPS);
        check("vl53l4cd.replay.xtalk",
            xt != null && Vl53l4Uld.decodeXtalk((int) be(xt)) == intOf(exp.get("xtalk_kcps")));

        List<Object> wantOnce = arr(exp.get("once"));
        boolean onceOk = onceReads.size() == wantOnce.size();
        for (int i = 0; onceOk && i < wantOnce.size(); i++) {
            Vl53l4.RegData d = onceReads.get(i);
            onceOk = vl53l4MeasurementMatches(
                Vl53l4Uld.parseResultBlock(d.data()), d.timestampUs(), obj(wantOnce.get(i)));
        }
        check("vl53l4cd.replay.once", onceOk);

        check("vl53l4cd.replay.parse_errors", parseErrors == 0);
        List<Object> wantFrames = arr(exp.get("frames"));
        check("vl53l4cd.replay.count", stream.size() >= wantFrames.size());
        for (int i = 0; i < Math.min(stream.size(), wantFrames.size()); i++) {
            Vl53l4.StreamData s = stream.get(i);
            check("vl53l4cd.replay.frame[" + i + "]", vl53l4MeasurementMatches(
                Vl53l4Uld.parseResultBlock(s.data()), s.timestampUs(), obj(wantFrames.get(i))));
        }
        seen("recordings/vl53l4cd_session.depzrec", mk);
    }

    private static void bno055ReplayVectors() throws Exception {
        bno055ReplayVector("bno055_imu_quat_units_50hz");
        bno055ReplayVector("bno055_ndof_full_100hz");
    }

    private static void bno055ReplayVector(String stem) throws Exception {
        System.out.println("[recordings/" + stem + "]");
        int mk = mark();
        String p = "bno055.replay." + stem;
        Map<String, Object> exp = obj(Json.parse(Files.readString(
            vectorsDir.resolve("recordings/" + stem + ".expected.json"))));
        List<RecPacket> pkts = recordingPackets(stem + ".depzrec");
        String sw = recordedText(pkts, Common.Cmd.GET_NAME_ACTIVE_SOFTWARE);
        Identity ident = sw == null ? null : Identity.parseSoftwareName(sw);
        check(p + ".identity", ident != null && ident.sensorType() == Identity.SensorType.BNO055
            && sw.equals(exp.get("software_name")));
        check(p + ".device_name",
            Objects.equals(recordedText(pkts, Common.Cmd.GET_DEVICE_NAME), exp.get("device_name")));

        Bno055.Bno055Info info = null;
        List<Bno055.StreamData> frames = new ArrayList<>();
        for (RecPacket rp : pkts) {
            if (!rp.rx()) {
                continue;
            }
            if (rp.pkt().cmd() == Bno055.Bno055Rpt.INFO.value && info == null) {
                info = Bno055.Bno055Info.unpack(rp.pkt().payload());
            } else if (rp.pkt().cmd() == Bno055.Bno055Rpt.STREAM.value) {
                frames.add(Bno055.StreamData.unpack(rp.pkt().payload()));
            }
        }
        check(p + ".info", info != null && bno055InfoMatches(info, obj(exp.get("info"))));

        // Read-backs: calibration profile (0x55, 22 bytes) and system status
        // (ST_RESULT, then SYS_CLK_STATUS..SYS_ERR) — the last read of each.
        List<RegRead> reads = regReads(pkts, false);
        check(p + ".reads_paired", reads != null);
        Map<Integer, byte[]> last = new java.util.HashMap<>();
        boolean lengthsOk = true;
        for (RegRead r : reads == null ? List.<RegRead>of() : reads) {
            Bno055.RegData d = Bno055.RegData.unpack(r.reply());
            lengthsOk &= d.data().length == r.len();
            last.put(r.addr() << 8 | r.len(), d.data());
        }
        check(p + ".read_lengths", lengthsOk);
        byte[] prof = last.get(Bno055Regs.REG_CALIB_PROFILE << 8 | Bno055Regs.CALIB_PROFILE_LEN);
        Map<String, Object> wantProf = obj(exp.get("calibration_profile"));
        boolean profOk = prof != null;
        if (profOk) {
            Bno055Regs.CalibrationProfile cp = Bno055Regs.CalibrationProfile.unpack(prof);
            profOk = intArrEquals(cp.accelOffset(), arr(wantProf.get("accel_offset")))
                && intArrEquals(cp.magOffset(), arr(wantProf.get("mag_offset")))
                && intArrEquals(cp.gyroOffset(), arr(wantProf.get("gyro_offset")))
                && cp.accelRadius() == intOf(wantProf.get("accel_radius"))
                && cp.magRadius() == intOf(wantProf.get("mag_radius"))
                && wantProf.size() == 5;
        }
        check(p + ".calibration_profile", profOk);
        byte[] st = last.get(Bno055Regs.REG_ST_RESULT << 8 | 1);
        byte[] sys = last.get(Bno055Regs.REG_SYS_CLK_STATUS << 8 | 3);
        Map<String, Object> wantSys = obj(exp.get("system_status"));
        check(p + ".system_status", st != null && sys != null && wantSys.size() == 4
            && (st[0] & 0xFF) == intOf(wantSys.get("self_test"))
            && (sys[0] & 0xFF) == intOf(wantSys.get("clk_status"))
            && (sys[1] & 0xFF) == intOf(wantSys.get("status"))
            && (sys[2] & 0xFF) == intOf(wantSys.get("error")));

        List<Object> block = arr(exp.get("block"));
        List<Object> want = arr(exp.get("frames"));
        check(p + ".count", frames.size() >= want.size());
        int n = Math.min(frames.size(), want.size());
        boolean periodOk = true;
        for (int i = 0; i < n; i++) {
            Map<String, Object> w = obj(want.get(i));
            Bno055.StreamData s = frames.get(i);
            Bno055Regs.RawBlock b = Bno055Regs.decodeBlock(s.addr(), s.data());
            check(p + ".frame[" + i + "]", s.timestampUs() == lng(w.get("timestamp_us"))
                && s.addr() == intOf(w.get("addr")) && s.addr() == intOf(block.get(0))
                && s.len() == intOf(block.get(1))
                && hex(s.data()).equals(w.get("raw"))
                && bno055BlockMatches(b, obj(w.get("decoded"))));
            if (i > 0) {
                long dt = s.timestampUs() - frames.get(i - 1).timestampUs();
                periodOk &= Math.abs(dt - 1000L * lng(exp.get("period_ms"))) < 500;
            }
        }
        // Streamed at the requested period on the MCU clock.
        check(p + ".period", periodOk);
        seen("recordings/" + stem + ".depzrec", mk);
    }

    // VL53L8CH captures (APP_VL53L8_v0.92, VL53LMZ firmware): footer at size-4
    // (Variant.CH); the CNH capture carries the raw CNH block in every frame.

    private static void vl53l8chReplayVectors() throws Exception {
        vl53l8chReplayVector("vl53l8ch_8x8_15hz_3s", false);
        vl53l8chReplayVector("vl53l8ch_cnh_8x8_15hz", true);
    }

    private static void vl53l8chReplayVector(String stem, boolean withCnh) throws Exception {
        System.out.println("[recordings/" + stem + "]");
        int mk = mark();
        String p = "vl53l8ch.replay." + stem;
        Map<String, Object> exp = obj(Json.parse(Files.readString(
            vectorsDir.resolve("recordings/" + stem + ".expected.json"))));
        List<RecPacket> pkts = recordingPackets(stem + ".depzrec");
        String sw = recordedText(pkts, Common.Cmd.GET_NAME_ACTIVE_SOFTWARE);
        check(p + ".software_name", Objects.equals(sw, exp.get("software_name")));
        FrameReassembler reasm = new FrameReassembler();
        List<Vl53l8Uld.Results> decoded = new ArrayList<>();
        List<Long> timestamps = new ArrayList<>();
        int parseErrors = 0;
        for (RecPacket rp : pkts) {
            if (!rp.rx() || rp.pkt().cmd() != 0x93) {
                continue;
            }
            FrameReassembler.CompletedFrame done =
                reasm.feed(FrameReassembler.unpackFrameChunk(rp.pkt().payload()));
            if (done == null) {
                continue;
            }
            try {
                decoded.add(Vl53l8Uld.parseFrame(done.frame(), done.frame().length, Vl53l8Uld.Variant.CH));
                timestamps.add(done.timestampUs());
            } catch (Vl53l8Uld.Vl53l8Error err) {
                parseErrors++;
            }
        }
        check(p + ".parse_errors", parseErrors == 0);
        List<Object> want = arr(exp.get("frames"));
        check(p + ".count", decoded.size() >= want.size());
        for (int i = 0; i < Math.min(decoded.size(), want.size()); i++) {
            Map<String, Object> w = obj(want.get(i));
            Vl53l8Uld.Results got = decoded.get(i);
            boolean ok = timestamps.get(i) == lng(w.get("timestamp_us"))
                && got.resolution() == intOf(w.get("resolution"))
                && got.siliconTempDegc == intOf(w.get("silicon_temp_degc"))
                && intArrEquals(got.distanceMm, arr(w.get("distance_mm")))
                && intArrEquals(got.targetStatus, arr(w.get("target_status")))
                && intArrEquals(got.nbTargetDetected, arr(w.get("nb_target_detected")));
            if (withCnh) {
                ok &= got.cnhRaw != null && hex(got.cnhRaw).equals(w.get("cnh_raw"));
            } else {
                ok &= got.cnhRaw == null && !w.containsKey("cnh_raw");
            }
            check(p + ".frame[" + i + "]", ok);
        }
        seen("recordings/" + stem + ".depzrec", mk);
    }

    // VL53L 1D captures whose driver reads the 17-byte 0x0089 die block (ULD on
    // L1CX/L1CB = 'l1', ULD/ULP on an L4-type die = 'l4'): every streamed block
    // decodes to the sidecar's distance and status. Histogram and L0X captures
    // need driver state (histogram algorithm, PAL status) the Java SDK lacks.

    private static void vl53lxDieReplayVectors() throws Exception {
        vl53lxDieReplayVector("vl53l1cb_uld_short_50ms", "l1");
        vl53lxDieReplayVector("vl53l1cx_uld_long_33ms", "l1");
        vl53lxDieReplayVector("vl53l3cx_ulp_33ms", "l4");
        vl53lxDieReplayVector("vl53l4cx_uld_as_l4cd_50ms", "l4");
    }

    private static void vl53lxDieReplayVector(String stem, String variant) throws Exception {
        System.out.println("[recordings/" + stem + "]");
        int mk = mark();
        String p = "vl53lx.replay." + stem;
        Map<String, Object> exp = obj(Json.parse(Files.readString(
            vectorsDir.resolve("recordings/" + stem + ".expected.json"))));
        List<RecPacket> pkts = recordingPackets(stem + ".depzrec");
        String sw = recordedText(pkts, Common.Cmd.GET_NAME_ACTIVE_SOFTWARE);
        Identity ident = sw == null ? null : Identity.parseSoftwareName(sw);
        check(p + ".identity", ident != null && ident.sensorType() == Identity.SensorType.VL53LX
            && sw.equals(exp.get("software_name")));
        String dev = recordedText(pkts, Common.Cmd.GET_DEVICE_NAME);
        check(p + ".class", dev != null
            && Vl53lxProducts.resolveClass(null, dev).className.equals(exp.get("class")));
        List<Vl53l4.StreamData> blocks = new ArrayList<>();
        int parseErrors = 0;
        for (RecPacket rp : pkts) {
            if (rp.rx() && rp.pkt().cmd() == 0x93) {
                Vl53l4.StreamData s = Vl53lx.unpackStreamData(rp.pkt().payload());
                if (s.addr() == Vl53lxDecode.DIE_BLOCK_ADDR && s.len() == Vl53lxDecode.DIE_BLOCK_LEN) {
                    blocks.add(s);
                } else {
                    parseErrors++;
                }
            }
        }
        check(p + ".parse_errors", parseErrors == 0);
        List<Object> want = arr(exp.get("frames"));
        check(p + ".count", blocks.size() >= want.size());
        for (int i = 0; i < Math.min(blocks.size(), want.size()); i++) {
            Map<String, Object> w = obj(want.get(i));
            Vl53l4.StreamData s = blocks.get(i);
            Vl53l4Uld.Results r = Vl53lxDecode.decodeDieBlock(s.data(), variant);
            check(p + ".frame[" + i + "]", s.timestampUs() == lng(w.get("timestamp_us"))
                && hex(s.data()).equals(w.get("raw"))
                && r.distanceMm() == intOf(w.get("distance_mm"))
                && r.rangeStatus() == intOf(w.get("status"))
                && arr(w.get("targets")).isEmpty()
                && !w.containsKey("bins"));
        }
        seen("recordings/" + stem + ".depzrec", mk);
    }

    // bno086_session decoded rx-side only: every RPT_DATA (0x91: cmd u8, capture
    // timestamp_us u64 LE, raw SHTP frame) through the SHTP layer, input-channel
    // cargos (3 normal, 4 wake) and gyro-RV cargos (5) through the SH-2 report
    // parsers, against the sidecar's 150 reports field by field.

    private static void bno086SessionReplay() throws Exception {
        System.out.println("[recordings/bno086_session]");
        int mk = mark();
        Map<String, Object> exp = obj(Json.parse(Files.readString(
            vectorsDir.resolve("recordings/bno086_session.expected.json"))));
        Shtp.ShtpLayer layer = new Shtp.ShtpLayer();
        List<Reports.Report> got = new ArrayList<>();
        for (RecPacket rp : recordingPackets("bno086_session.depzrec")) {
            byte[] pl = rp.pkt().payload();
            if (!rp.rx() || rp.pkt().cmd() != 0x91 || pl.length < 9) {
                continue;
            }
            long capture = 0;
            for (int i = 8; i >= 1; i--) {
                capture = (capture << 8) | (pl[i] & 0xFF);
            }
            Shtp.ShtpCargo cargo = layer.feed(java.util.Arrays.copyOfRange(pl, 9, pl.length), capture);
            if (cargo == null) {
                continue;
            }
            if (cargo.channel() == 3 || cargo.channel() == 4) {
                got.addAll(Reports.parseInputCargo(cargo.payload(), cargo.captureUs()));
            } else if (cargo.channel() == 5) {
                Reports.Report r = Reports.parseGyroRvCargo(cargo.payload(), cargo.captureUs());
                if (r != null) {
                    got.add(r);
                }
            }
        }
        List<Object> want = arr(exp.get("reports"));
        check("bno086.replay.count", got.size() >= want.size());
        for (int i = 0; i < Math.min(got.size(), want.size()); i++) {
            Map<String, Object> w = new java.util.LinkedHashMap<>(obj(want.get(i)));
            Object type = w.remove("type");
            Reports.Report g = got.get(i);
            check("bno086.replay.report[" + i + "]", g.type().equals(type)
                && g.fields().size() == w.size() && fieldsMatch(g.fields(), w));
        }
        seen("recordings/bno086_session.depzrec", mk);
    }

    // bno086_canonical (contract 15 scenario, opened the §4.4 way) decoded
    // rx-side only like bno086_session: the probe's text answers resolve to the
    // sidecar's class, and the 50 reports of its `results` (the scenario's
    // collect step) match the first 50 SH-2 reports of the rx stream.

    private static void bno086CanonicalReplay() throws Exception {
        System.out.println("[recordings/bno086_canonical]");
        int mk = mark();
        Map<String, Object> exp = obj(Json.parse(Files.readString(
            vectorsDir.resolve("recordings/bno086_canonical.expected.json"))));
        List<RecPacket> pkts = recordingPackets("bno086_canonical.depzrec");
        String sw = recordedText(pkts, Common.Cmd.GET_NAME_ACTIVE_SOFTWARE);
        String name = recordedText(pkts, Common.Cmd.GET_DEVICE_NAME);
        check("bno086_canonical.software_name", Objects.equals(sw, exp.get("software_name")));
        check("bno086_canonical.device_name", Objects.equals(name, exp.get("device_name")));
        Integer vid = intOf(exp.get("usb_vid"));
        Integer pid = intOf(exp.get("usb_pid"));
        check("bno086_canonical.usb_model",
            Objects.equals(UsbIds.usbModelHint(vid, pid), exp.get("usb_model")));
        check("bno086_canonical.class", sw != null && Discovery.resolveSensorClass(
            sw, name == null ? "" : name, vid, pid).equals(exp.get("class")));
        Shtp.ShtpLayer layer = new Shtp.ShtpLayer();
        List<Reports.Report> got = new ArrayList<>();
        for (RecPacket rp : pkts) {
            byte[] pl = rp.pkt().payload();
            if (!rp.rx() || rp.pkt().cmd() != 0x91 || pl.length < 9) {
                continue;
            }
            long capture = 0;
            for (int i = 8; i >= 1; i--) {
                capture = (capture << 8) | (pl[i] & 0xFF);
            }
            Shtp.ShtpCargo cargo = layer.feed(java.util.Arrays.copyOfRange(pl, 9, pl.length), capture);
            if (cargo == null) {
                continue;
            }
            if (cargo.channel() == 3 || cargo.channel() == 4) {
                got.addAll(Reports.parseInputCargo(cargo.payload(), cargo.captureUs()));
            } else if (cargo.channel() == 5) {
                Reports.Report r = Reports.parseGyroRvCargo(cargo.payload(), cargo.captureUs());
                if (r != null) {
                    got.add(r);
                }
            }
        }
        List<Object> want = null;
        for (Object r : arr(exp.get("results"))) {
            if (r instanceof List<?> l && !l.isEmpty()) {
                want = arr(r);
                break;
            }
        }
        check("bno086_canonical.reports present", want != null);
        if (want != null) {
            check("bno086_canonical.count", got.size() >= want.size());
            for (int i = 0; i < Math.min(got.size(), want.size()); i++) {
                Map<String, Object> w = obj(want.get(i));
                Reports.Report g = got.get(i);
                check("bno086_canonical.report[" + i + "]",
                    g.fields().size() == w.size() && fieldsMatch(g.fields(), w));
            }
        }
        seen("recordings/bno086_canonical.depzrec", mk);
    }

    // The board's own frame-time capture that tmf8829_frame_time.json is built
    // from: replayed straight from the JSONL against each row's `exp`.
    private static void tmf8829FrameTimeRecording() throws Exception {
        System.out.println("[recordings/tmf8829_frame_time.jsonl]");
        int mk = mark();
        SensorClock clock = null;
        CaptureTimer timer = null;
        int streams = 0;
        int checked = 0;
        int lineNo = 0;
        for (String line : Files.readString(vectorsDir.resolve("recordings/tmf8829_frame_time.jsonl")).split("\n")) {
            lineNo++;
            if (line.isBlank()) {
                continue;
            }
            Map<String, Object> v = obj(Json.parse(line));
            if (v.containsKey("scenario")) {
                clock = new SensorClock();
                timer = new CaptureTimer(clock);
                streams++;
                continue;
            }
            int fid = intOf(v.get("id"));
            int kind = fid & 0xF0;
            if (kind == 0x30) {
                continue;
            }
            long tick = lng(v.get("systick"));
            int flags = clock.add(lng(v.get("ts")), tick);
            if (kind == 0x20) {
                timer.histograms();
                continue;
            }
            CaptureTimer.FrameTime t = timer.results(fid & 0x0F, tick, lng(v.get("t0")), lng(v.get("t1")),
                lng(v.get("ts")), lng(v.get("edge_us")), flags);
            Map<String, Object> e = obj(v.get("exp"));
            if (!(t.timestampUs() == lng(e.get("timestamp_us")) && t.t1LenUs() == lng(e.get("t1_len_us"))
                    && t.flags() == intOf(e.get("flags")) && clock.ratioQ16 == lng(e.get("ratio_q16")))) {
                check("tmf8829_frame_time.jsonl:line " + lineNo, false);
            }
            checked++;
        }
        check("tmf8829_frame_time.jsonl:streams=27 results=970 (got " + streams + ", " + checked + ")",
            streams == 27 && checked == 970);
        seen("recordings/tmf8829_frame_time.jsonl", mk);
    }

    // ── comparison helpers ────────────────────────────────────────────────────

    // ── tmf8829.json + tmf8829_frame_time.json (contract 14) ─────────────────

    private static String sha256(byte[] b) throws Exception {
        return hex(java.security.MessageDigest.getInstance("SHA-256").digest(b));
    }

    /** Raw int array vs a JSON list of integers or null. */
    private static boolean intsEq(int[] got, Object expected) {
        if (expected == null || got == null) {
            return expected == null && got == null;
        }
        List<Object> ex = arr(expected);
        if (ex.size() != got.length) {
            return false;
        }
        for (int i = 0; i < got.length; i++) {
            if (lng(ex.get(i)) != got[i]) {
                return false;
            }
        }
        return true;
    }

    /** {@code _arrays(res)} of the Python test vs its expected dict (extra keys ignored). */
    private static boolean pixelResultsEq(Frames.PixelResults r, Map<String, Object> ex) {
        int[] shape = r.shape();
        return r.peaks() == intOf(ex.get("peaks"))
            && shape[0] == intOf(ex.get("rows"))
            && shape[1] == intOf(ex.get("cols"))
            && intsEq(r.distance(), ex.get("distance"))
            && intsEq(r.snr(), ex.get("snr"))
            && intsEq(r.signal(), ex.get("signal"))
            && intsEq(r.noise(), ex.get("noise"))
            && intsEq(r.xtalk(), ex.get("xtalk"));
    }

    private static boolean throwsIae(Runnable r) {
        try {
            r.run();
            return false;
        } catch (IllegalArgumentException e) {
            return true;
        }
    }

    private static void tmf8829Vectors() throws Exception {
        System.out.println("[tmf8829.json]");
        Map<String, Object> root = load("tmf8829.json");

        // encode
        for (Object eo : arr(root.get("encode"))) {
            Map<String, Object> c = obj(eo);
            int mk = mark();
            String name = (String) c.get("name");
            int cmd = intOf(c.get("cmd"));
            byte[] got;
            if (cmd == Tmf8829.Tmf8829Cmd.READ_REG.value) {
                got = Tmf8829.packReadReg(intOf(c.get("addr")), intOf(c.get("len")));
            } else if (cmd == Tmf8829.Tmf8829Cmd.WRITE_REG.value) {
                got = Tmf8829.packWriteReg(intOf(c.get("addr")), hex((String) c.get("data")));
            } else if (cmd == Tmf8829.Tmf8829Cmd.START_STREAM.value) {
                got = Tmf8829.packStartStream(intOf(c.get("int_mask")), intOf(c.get("flags")));
            } else {
                got = new byte[0];
            }
            check("tmf8829.encode:" + name, hex(got).equals(c.get("payload")));
            seen(c, mk);
        }
        check("tmf8829.cmd_ids",
            Tmf8829.Tmf8829Cmd.READ_REG.value == 0x32 && Tmf8829.Tmf8829Cmd.WRITE_REG.value == 0x33
                && Tmf8829.Tmf8829Cmd.START_STREAM.value == 0x35 && Tmf8829.Tmf8829Cmd.STOP_STREAM.value == 0x36
                && Tmf8829.Tmf8829Cmd.GET_INFO.value == 0x37
                && Tmf8829.Tmf8829Rpt.REG_DATA.value == 0x91 && Tmf8829.Tmf8829Rpt.INFO.value == 0x92
                && Tmf8829.Tmf8829Rpt.FRAME.value == 0x93 && Tmf8829.Tmf8829Rpt.EVENT.value == 0x94);

        // encode_errors
        for (Object eo : arr(root.get("encode_errors"))) {
            Map<String, Object> c = obj(eo);
            int mk = mark();
            String name = (String) c.get("name");
            String kind = (String) c.get("kind");
            boolean refused;
            switch (kind) {
                case "read_reg":
                    refused = throwsIae(() -> Tmf8829.packReadReg(intOf(c.get("addr")), intOf(c.get("len"))));
                    break;
                case "write_reg":
                    refused = throwsIae(() -> Tmf8829.packWriteReg(intOf(c.get("addr")), hex((String) c.get("data"))));
                    break;
                default:
                    refused = throwsIae(() -> Tmf8829.packStartStream(intOf(c.get("int_mask")), intOf(c.get("flags"))));
            }
            check("tmf8829.encode_error:" + name, refused);
            seen(c, mk);
        }

        // decode
        for (Object do_ : arr(root.get("decode"))) {
            Map<String, Object> c = obj(do_);
            int mk = mark();
            String name = (String) c.get("name");
            int report = intOf(c.get("report"));
            byte[] p = hex((String) c.get("payload"));
            Map<String, Object> ex = obj(c.get("expect"));
            boolean ok;
            if (report == Tmf8829.Tmf8829Rpt.REG_DATA.value) {
                Tmf8829.RegData r = Tmf8829.RegData.unpack(p);
                ok = ex.size() == 2 && r.timestampUs() == lng(ex.get("timestamp_us"))
                    && hex(r.data()).equals(ex.get("data"));
            } else if (report == Tmf8829.Tmf8829Rpt.INFO.value) {
                Tmf8829.Tmf8829Info i = Tmf8829.Tmf8829Info.unpack(p);
                ok = ex.size() == 10
                    && i.intEdges() == lng(ex.get("int_edges"))
                    && i.frames() == lng(ex.get("frames"))
                    && i.framesAborted() == lng(ex.get("frames_aborted"))
                    && i.frameErrors() == lng(ex.get("frame_errors"))
                    && i.spiTimeouts() == lng(ex.get("spi_timeouts"))
                    && i.intLevel() == intOf(ex.get("int_level"))
                    && i.streaming() == (Boolean) ex.get("streaming")
                    && i.intMask() == intOf(ex.get("int_mask"))
                    && i.lastIntStatus() == intOf(ex.get("last_int_status"))
                    && i.lastFifoStatus() == intOf(ex.get("last_fifo_status"));
            } else if (report == Tmf8829.Tmf8829Rpt.FRAME.value) {
                Tmf8829.FrameChunk f = Tmf8829.FrameChunk.unpack(p);
                ok = ex.size() == 6
                    && f.timestampUs() == lng(ex.get("timestamp_us"))
                    && f.fullSize() == intOf(ex.get("full_size"))
                    && f.offset() == intOf(ex.get("offset"))
                    && f.frameSeq() == intOf(ex.get("frame_seq"))
                    && f.edgeUs() == lng(ex.get("edge_us"))
                    && hex(f.data()).equals(ex.get("data"));
            } else {
                Tmf8829.Tmf8829Event e = Tmf8829.Tmf8829Event.unpack(p);
                ok = ex.size() == 5
                    && e.timestampUs() == lng(ex.get("timestamp_us"))
                    && e.intStatus() == intOf(ex.get("int_status"))
                    && e.code() == intOf(ex.get("code"))
                    && e.frameId() == intOf(ex.get("frame_id"))
                    && e.fifoStatus() == intOf(ex.get("fifo_status"));
            }
            check("tmf8829.decode:" + name, ok);
            seen(c, mk);
        }

        // reassembly
        for (Object ro : arr(root.get("reassembly"))) {
            Map<String, Object> c = obj(ro);
            int mk = mark();
            String name = (String) c.get("name");
            Tmf8829.FrameReassembler r = new Tmf8829.FrameReassembler();
            List<String> done = new ArrayList<>();
            for (Object ch : arr(c.get("chunks"))) {
                Tmf8829.RawFrame f = r.feed(Tmf8829.FrameChunk.unpack(hex((String) ch)));
                if (f != null) {
                    done.add(f.frameSeq() + "|" + f.timestampUs() + "|" + f.edgeUs() + "|"
                        + f.data().length + "|" + sha256(f.data()));
                }
            }
            Map<String, Object> ex = obj(c.get("expect"));
            List<String> want = new ArrayList<>();
            for (Object fo : arr(ex.get("frames"))) {
                Map<String, Object> f = obj(fo);
                want.add(lng(f.get("frame_seq")) + "|" + lng(f.get("timestamp_us")) + "|" + lng(f.get("edge_us"))
                    + "|" + lng(f.get("size")) + "|" + f.get("sha256"));
            }
            check("tmf8829.reassembly:" + name, done.equals(want)
                && r.completed == intOf(ex.get("completed"))
                && r.discarded == intOf(ex.get("discarded"))
                && r.seqGaps == intOf(ex.get("seq_gaps")));
            seen(c, mk);
        }

        // frames
        Map<String, byte[]> rawByCase = new java.util.HashMap<>();
        for (Object fo : arr(root.get("frames"))) {
            Map<String, Object> c = obj(fo);
            int mk = mark();
            String name = (String) c.get("name");
            byte[] raw = hex((String) c.get("raw"));
            rawByCase.put(name, raw);
            Frames.FrameInfo info = Frames.parseFrame(raw);
            Map<String, Object> h = obj(c.get("header"));
            List<Object> temp = arr(h.get("temperature"));
            List<Object> ref = arr(h.get("ref_pos"));
            boolean hdr = info.fifoStatus() == intOf(h.get("fifo_status"))
                && info.systick() == lng(h.get("systick"))
                && info.id() == intOf(h.get("id"))
                && info.kind() == intOf(h.get("kind"))
                && info.fpMode() == intOf(h.get("fp_mode"))
                && info.layout() == intOf(h.get("layout"))
                && info.payload() == intOf(h.get("payload"))
                && info.fNumber() == lng(h.get("f_number"))
                && temp.size() == 3
                && info.temperature()[0] == intOf(temp.get(0))
                && info.temperature()[1] == intOf(temp.get(1))
                && info.temperature()[2] == intOf(temp.get(2))
                && info.bdv() == intOf(h.get("bdv"))
                && ref.size() == 2
                && info.refPos()[0] == intOf(ref.get(0))
                && info.refPos()[1] == intOf(ref.get(1))
                && info.t0Integration() == lng(h.get("t0_integration"))
                && info.t1Integration() == lng(h.get("t1_integration"))
                && info.frameStatus() == intOf(h.get("frame_status"))
                && info.reserved() == intOf(h.get("reserved"))
                && info.eof() == intOf(h.get("eof"))
                && info.size() == intOf(h.get("size"));
            check("tmf8829.frame.header:" + name, hdr);
            check("tmf8829.frame.problems:" + name, info.problems().equals(arr(c.get("problems"))));
            if (info.kind() == Frames.FID_RESULTS) {
                check("tmf8829.frame.results:" + name,
                    pixelResultsEq(Frames.parseResults(raw, info), obj(c.get("results"))));
            } else {
                check("tmf8829.frame.data:" + name, sha256(java.util.Arrays.copyOfRange(
                    raw, Frames.FRAME_HEAD, raw.length - Frames.FOOTER_SIZE)).equals(c.get("data_sha256")));
            }
            seen(c, mk);
        }

        // measurements
        for (Object so : arr(root.get("measurements"))) {
            Map<String, Object> s = obj(so);
            int mk = mark();
            String name = (String) s.get("name");
            MeasurementBuilder b = new MeasurementBuilder();
            List<Object> want = arr(s.get("expect"));
            int k = 0;
            boolean ok = true;
            for (Object fo : arr(s.get("frames"))) {
                Map<String, Object> f = obj(fo);
                MeasurementBuilder.Measurement m = b.add(
                    rawByCase.get((String) f.get("raw_case")), lng(f.get("timestamp_us")), lng(f.get("edge_us")));
                if (m == null) {
                    continue;
                }
                if (k >= want.size()) {
                    ok = false;
                    k++;
                    continue;
                }
                Map<String, Object> ex = obj(want.get(k++));
                List<Object> ft = arr(ex.get("frame_times_us"));
                List<Object> tl = arr(ex.get("t1_len_us"));
                boolean times = ft.size() == m.times().size() && tl.size() == m.times().size();
                for (int i = 0; times && i < ft.size(); i++) {
                    times = m.times().get(i).timestampUs() == lng(ft.get(i))
                        && m.times().get(i).t1LenUs() == lng(tl.get(i));
                }
                ok &= times
                    && m.timestampUs() == lng(ex.get("timestamp_us"))
                    && m.flags() == intOf(ex.get("flags"))
                    && m.fpMode() == intOf(ex.get("fp_mode"))
                    && m.fNumber() == lng(ex.get("f_number"))
                    && b.clock.ratioQ16 == lng(ex.get("ratio_q16"))
                    && m.histograms().size() == intOf(ex.get("n_histograms"))
                    && m.refFrames().size() == intOf(ex.get("n_ref_frames"))
                    && m.problems().size() == intOf(ex.get("n_problems"))
                    && m.frames().get(m.frames().size() - 1).distanceInMm() == (Boolean) ex.get("distance_in_mm")
                    && pixelResultsEq(m.results(), ex);
            }
            check("tmf8829.measurements:" + name, ok && k == want.size());
            seen(s, mk);
        }

        // config
        Map<String, Object> cfg = obj(root.get("config"));
        int mk = mark();
        byte[] page = hex((String) cfg.get("page"));
        check("tmf8829.config.page_size", page.length == Tmf8829Config.CFG_SIZE);
        // field table: name -> [reg, shift, width], same names in the same order
        Map<String, Object> fields = obj(cfg.get("fields"));
        boolean fieldsOk = new ArrayList<>(fields.keySet())
            .equals(new ArrayList<>(Tmf8829Config.CFG_FIELDS.keySet()));
        for (Map.Entry<String, Object> e : fields.entrySet()) {
            List<Object> loc = arr(e.getValue());
            Tmf8829Config.CfgField f = Tmf8829Config.CFG_FIELDS.get(e.getKey());
            fieldsOk &= f != null && loc.size() == 3 && f.reg() == intOf(loc.get(0))
                && f.shift() == intOf(loc.get(1)) && f.width() == intOf(loc.get(2));
        }
        check("tmf8829.config.fields", fieldsOk);
        for (Map.Entry<String, Object> e : obj(cfg.get("get")).entrySet()) {
            check("tmf8829.config.get:" + e.getKey(),
                Tmf8829Config.cfgGet(page, e.getKey()) == lng(e.getValue()));
        }
        for (Object co : arr(cfg.get("set"))) {
            Map<String, Object> c = obj(co);
            String field = (String) c.get("field");
            byte[] pg = page.clone();
            Tmf8829Config.cfgSet(pg, field, intOf(c.get("value")));
            check("tmf8829.config.set:" + field, hex(pg).equals(c.get("page")));
        }
        for (Object co : arr(cfg.get("set_errors"))) {
            Map<String, Object> c = obj(co);
            String field = (String) c.get("field");
            check("tmf8829.config.set_error:" + field,
                throwsIae(() -> Tmf8829Config.cfgSet(page.clone(), field, intOf(c.get("value")))));
        }
        seen("tmf8829/config", mk);

        // firmware
        mk = mark();
        Map<String, Object> fw = obj(root.get("firmware"));
        byte[] img = Tmf8829Firmware.image();
        check("tmf8829.firmware.load_addr", Tmf8829Firmware.LOAD_ADDR == lng(fw.get("load_addr")));
        check("tmf8829.firmware.size", img.length == intOf(fw.get("size")));
        check("tmf8829.firmware.words", img.length / 4 == intOf(fw.get("words")));
        check("tmf8829.firmware.sha256", sha256(img).equals(fw.get("sha256")));
        check("tmf8829.firmware.license", Tmf8829Firmware.license().contains("MIT"));
        seen("tmf8829/firmware", mk);
    }

    private static void tmf8829FrameTimeVectors() throws Exception {
        System.out.println("[tmf8829_frame_time.json]");
        Map<String, Object> v = load("tmf8829_frame_time.json");
        Map<String, Integer> col = new java.util.HashMap<>();
        List<Object> columns = arr(v.get("columns"));
        for (int i = 0; i < columns.size(); i++) {
            col.put((String) columns.get(i), i);
        }
        int checked = 0;
        for (Object so : arr(v.get("streams"))) {
            Map<String, Object> s = obj(so);
            int mk = mark();
            String scenario = (String) s.get("scenario");
            SensorClock clock = new SensorClock();
            CaptureTimer timer = new CaptureTimer(clock);
            int idx = 0;
            for (Object ro : arr(s.get("frames"))) {
                List<Object> row = arr(ro);
                int fid = intOf(row.get(col.get("id")));
                long ts = lng(row.get(col.get("ts")));
                long tick = lng(row.get(col.get("systick")));
                int kind = fid & 0xF0;
                idx++;
                if (kind == 0x30) {
                    continue;
                }
                int flags = clock.add(ts, tick);
                if (kind == 0x20) {
                    timer.histograms();
                    continue;
                }
                CaptureTimer.FrameTime t = timer.results(
                    fid & 0x0F, tick, lng(row.get(col.get("t0"))), lng(row.get(col.get("t1"))),
                    ts, lng(row.get(col.get("edge_us"))), flags);
                check("tmf8829_frame_time:" + scenario + "#" + (idx - 1),
                    t.timestampUs() == lng(row.get(col.get("exp_timestamp_us")))
                        && t.t1LenUs() == lng(row.get(col.get("exp_t1_len_us")))
                        && t.flags() == intOf(row.get(col.get("exp_flags")))
                        && clock.ratioQ16 == lng(row.get(col.get("exp_ratio_q16"))));
                checked++;
            }
            seen(s, mk);
        }
        check("tmf8829_frame_time:results_frames_checked=970 (got " + checked + ")", checked == 970);
    }

    // ── bno055.json (BNO055 register bridge, contract 13) ────────────────────

    /** Nullable channel vs JSON: both null, or equal integer lists. */
    private static boolean chanEquals(int[] got, Object want) {
        if (got == null || want == null) {
            return got == null && want == null;
        }
        return intArrEquals(got, arr(want));
    }

    private static boolean nullableIntEquals(Integer got, Object want) {
        if (got == null || want == null) {
            return got == null && want == null;
        }
        return got == intOf(want);
    }

    private static boolean bno055InfoMatches(Bno055.Bno055Info i, Map<String, Object> ex) {
        return i.i2cAddr() == intOf(ex.get("i2c_addr"))
            && i.chipId() == intOf(ex.get("chip_id"))
            && i.accId() == intOf(ex.get("acc_id"))
            && i.magId() == intOf(ex.get("mag_id"))
            && i.gyrId() == intOf(ex.get("gyr_id"))
            && i.swRev() == intOf(ex.get("sw_rev"))
            && i.blRev() == intOf(ex.get("bl_rev"))
            && i.initialized() == intOf(ex.get("initialized"))
            && i.intLevel() == intOf(ex.get("int_level"))
            && i.intEdges() == lng(ex.get("int_edges"))
            && i.readMinUs() == intOf(ex.get("read_min_us"))
            && i.readMaxUs() == intOf(ex.get("read_max_us"))
            && i.readAvgUs() == intOf(ex.get("read_avg_us"))
            && i.txDropped() == lng(ex.get("tx_dropped"))
            && i.i2cErrors() == lng(ex.get("i2c_errors"))
            && i.slotsSkipped() == lng(ex.get("slots_skipped"))
            && i.busRecoveries() == intOf(ex.get("bus_recoveries"))
            && i.lastI2cError() == intOf(ex.get("last_i2c_error"))
            && i.sensorResets() == intOf(ex.get("sensor_resets"))
            && i.loopMaxUs() == intOf(ex.get("loop_max_us"))
            && ex.size() == 20;
    }

    private static boolean bno055BlockMatches(Bno055Regs.RawBlock b, Map<String, Object> ex) {
        return chanEquals(b.accel(), ex.get("accel"))
            && chanEquals(b.mag(), ex.get("mag"))
            && chanEquals(b.gyro(), ex.get("gyro"))
            && chanEquals(b.euler(), ex.get("euler"))
            && chanEquals(b.quaternion(), ex.get("quaternion"))
            && chanEquals(b.linearAccel(), ex.get("linear_accel"))
            && chanEquals(b.gravity(), ex.get("gravity"))
            && nullableIntEquals(b.temperature(), ex.get("temperature"))
            && nullableIntEquals(b.calibStat(), ex.get("calib_stat"))
            && ex.size() == 9;
    }

    private static void bno055Vectors() throws Exception {
        System.out.println("[bno055.json]");
        Map<String, Object> root = load("bno055.json");

        // encode
        for (Object eo : arr(root.get("encode"))) {
            Map<String, Object> c = obj(eo);
            int mk = mark();
            String name = (String) c.get("name");
            String kind = (String) c.get("kind");
            byte[] got;
            switch (kind) {
                case "read_reg":
                    got = Bno055.packReadReg(intOf(c.get("addr")), intOf(c.get("len")));
                    break;
                case "write_reg":
                    got = Bno055.packWriteReg(intOf(c.get("addr")), hex((String) c.get("data")));
                    break;
                case "start_stream":
                    got = Bno055.packStartStream(intOf(c.get("trigger")), intOf(c.get("addr")),
                        intOf(c.get("len")), intOf(c.get("period_ms")));
                    break;
                case "reset":
                case "stop_stream":
                case "get_info":
                    got = new byte[0];
                    break;
                default:
                    throw new RuntimeException("unknown bno055 encode kind " + kind);
            }
            check("bno055.encode:" + name, hex(got).equals(c.get("payload")));
            seen(c, mk);
        }
        check("bno055.cmd_ids",
            Bno055.Bno055Cmd.READ_REG.value == 0x32 && Bno055.Bno055Cmd.WRITE_REG.value == 0x33
                && Bno055.Bno055Cmd.RESET.value == 0x34 && Bno055.Bno055Cmd.START_STREAM.value == 0x35
                && Bno055.Bno055Cmd.STOP_STREAM.value == 0x36 && Bno055.Bno055Cmd.GET_INFO.value == 0x37
                && Bno055.Bno055Rpt.REG_DATA.value == 0x91 && Bno055.Bno055Rpt.INFO.value == 0x92
                && Bno055.Bno055Rpt.STREAM.value == 0x93 && Bno055.INFO_SIZE == 38);

        // decode
        for (Object do_ : arr(root.get("decode"))) {
            Map<String, Object> c = obj(do_);
            int mk = mark();
            String name = (String) c.get("name");
            int report = intOf(c.get("report"));
            byte[] raw = hex((String) c.get("payload"));
            Map<String, Object> ex = obj(c.get("expect"));
            boolean ok;
            if (report == Bno055.Bno055Rpt.INFO.value) {
                Bno055.Bno055Info i = Bno055.Bno055Info.unpack(raw);
                    ok = bno055InfoMatches(i, ex);
            } else if (report == Bno055.Bno055Rpt.REG_DATA.value) {
                Bno055.RegData r = Bno055.RegData.unpack(raw);
                ok = r.cmd() == intOf(ex.get("cmd"))
                    && r.timestampUs() == lng(ex.get("timestamp_us"))
                    && hex(r.data()).equals(ex.get("data"))
                    && ex.size() == 3;
            } else if (report == Bno055.Bno055Rpt.STREAM.value) {
                Bno055.StreamData s = Bno055.StreamData.unpack(raw);
                ok = s.timestampUs() == lng(ex.get("timestamp_us"))
                    && s.addr() == intOf(ex.get("addr"))
                    && s.len() == intOf(ex.get("len"))
                    && hex(s.data()).equals(ex.get("data"))
                    && ex.size() == 4;
            } else {
                throw new RuntimeException("unknown bno055 report " + report);
            }
            check("bno055.decode:" + name, ok);
            seen(c, mk);
        }
        boolean shortRejected = false;
        try {
            Bno055.Bno055Info.unpack(new byte[Bno055.INFO_SIZE - 1]);
        } catch (IllegalArgumentException e) {
            shortRejected = true;
        }
        check("bno055.decode:info_rejects_short_payload", shortRejected);

        // units
        for (Object uo : arr(root.get("units"))) {
            Map<String, Object> c = obj(uo);
            int mk = mark();
            int sel = intOf(c.get("unit_sel"));
            Bno055Regs.Units u = Bno055Regs.Units.unpack(sel);
            Map<String, Object> ex = obj(c.get("expect"));
            boolean ok = u.accelMg() == (Boolean) ex.get("accel_mg")
                && u.gyroRps() == (Boolean) ex.get("gyro_rps")
                && u.eulerRad() == (Boolean) ex.get("euler_rad")
                && u.tempF() == (Boolean) ex.get("temp_f")
                && u.android() == (Boolean) ex.get("android")
                && ex.size() == 5
                && u.pack() == intOf(c.get("repack"));
            check(String.format("bno055.units:0x%02x", sel), ok);
            seen(c, mk);
        }
        Bno055Regs.Units si = Bno055Regs.Units.DEFAULT;
        Bno055Regs.Units all = Bno055Regs.Units.unpack(0x97);
        check("bno055.units:lsb",
            si.pack() == 0 && si.accelLsb() == 100.0 && si.gyroLsb() == 16.0
                && si.eulerLsb() == 16.0 && si.tempLsb() == 1.0
                && all.accelLsb() == 1.0 && all.gyroLsb() == 900.0
                && all.eulerLsb() == 900.0 && all.tempLsb() == 0.5
                && Bno055Regs.MAG_LSB == 16.0 && Bno055Regs.QUAT_LSB == 16384.0
                && Bno055Regs.FUSION_ACCEL_LSB == 100.0);

        // calib_stat
        for (Object co : arr(root.get("calib_stat"))) {
            Map<String, Object> c = obj(co);
            int mk = mark();
            int value = intOf(c.get("value"));
            Bno055Regs.CalibStatus s = Bno055Regs.CalibStatus.unpack(value);
            Map<String, Object> ex = obj(c.get("expect"));
            boolean ok = s.system() == intOf(ex.get("system"))
                && s.gyro() == intOf(ex.get("gyro"))
                && s.accel() == intOf(ex.get("accel"))
                && s.mag() == intOf(ex.get("mag"))
                && ex.size() == 4
                && s.fullyCalibrated() == (Boolean) c.get("fully_calibrated")
                && s.pack() == value;
            check(String.format("bno055.calib_stat:0x%02x", value), ok);
            seen(c, mk);
        }

        // calibration_profile
        for (Object po : arr(root.get("calibration_profile"))) {
            Map<String, Object> c = obj(po);
            int mk = mark();
            String name = (String) c.get("name");
            byte[] bytes = hex((String) c.get("bytes"));
            Bno055Regs.CalibrationProfile p = Bno055Regs.CalibrationProfile.unpack(bytes);
            Map<String, Object> ex = obj(c.get("expect"));
            boolean ok = intArrEquals(p.accelOffset(), arr(ex.get("accel_offset")))
                && intArrEquals(p.magOffset(), arr(ex.get("mag_offset")))
                && intArrEquals(p.gyroOffset(), arr(ex.get("gyro_offset")))
                && p.accelRadius() == intOf(ex.get("accel_radius"))
                && p.magRadius() == intOf(ex.get("mag_radius"))
                && ex.size() == 5
                && hex(p.pack()).equals(c.get("bytes"));
            check("bno055.calibration_profile:" + name, ok);
            seen(c, mk);
        }
        boolean profileShort = false;
        try {
            Bno055Regs.CalibrationProfile.unpack(new byte[Bno055Regs.CALIB_PROFILE_LEN - 1]);
        } catch (IllegalArgumentException e) {
            profileShort = true;
        }
        check("bno055.calibration_profile:rejects_21_bytes", profileShort);

        // axis_remap (+ placements P0..P7)
        List<String> names = new ArrayList<>();
        for (Object ao : arr(root.get("axis_remap"))) {
            Map<String, Object> c = obj(ao);
            int mk = mark();
            String name = (String) c.get("name");
            names.add(name);
            Bno055Regs.AxisRemap a = Bno055Regs.AxisRemap.unpack(intOf(c.get("config")), intOf(c.get("sign")));
            Map<String, Object> ex = obj(c.get("expect"));
            boolean ok = a.x() == intOf(ex.get("x"))
                && a.y() == intOf(ex.get("y"))
                && a.z() == intOf(ex.get("z"))
                && a.xNegative() == (Boolean) ex.get("x_negative")
                && a.yNegative() == (Boolean) ex.get("y_negative")
                && a.zNegative() == (Boolean) ex.get("z_negative")
                && ex.size() == 6
                && intArrEquals(a.pack(), arr(c.get("repack")))
                && Bno055Regs.AxisRemap.placement(name).equals(a)
                && Bno055Regs.AxisRemap.placement(name.toLowerCase(java.util.Locale.ROOT)).equals(a);
            check("bno055.axis_remap:" + name, ok);
            seen(c, mk);
        }
        check("bno055.axis_remap:placement_table",
            names.equals(new ArrayList<>(Bno055Regs.PLACEMENTS.keySet()))
                && Bno055Regs.AxisRemap.placement("P1").equals(Bno055Regs.AxisRemap.DEFAULT));
        boolean unknownPlacement = false;
        try {
            Bno055Regs.AxisRemap.placement("P8");
        } catch (IllegalArgumentException e) {
            unknownPlacement = true;
        }
        check("bno055.axis_remap:refuses_P8", unknownPlacement);
        for (Object ao : arr(root.get("axis_remap_invalid"))) {
            Map<String, Object> c = obj(ao);
            int mk = mark();
            boolean refused = false;
            try {
                new Bno055Regs.AxisRemap(intOf(c.get("x")), intOf(c.get("y")), intOf(c.get("z")),
                    false, false, false).pack();
            } catch (IllegalArgumentException e) {
                refused = true;
            }
            check("bno055.axis_remap_invalid:" + c.get("name"), refused);
            seen(c, mk);
        }

        // sensor_config (mag bit 7 is not a field: repack is value & 0x7F)
        Map<String, Object> sc = obj(root.get("sensor_config"));
        int mkCfg = mark();
        for (Object ao : arr(sc.get("accel"))) {
            Map<String, Object> c = obj(ao);
            int value = intOf(c.get("value"));
            Map<String, Object> ex = obj(c.get("expect"));
            Bno055Regs.AccelConfig a = Bno055Regs.AccelConfig.unpack(value);
            Bno055Regs.AccelConfig want = new Bno055Regs.AccelConfig(
                intOf(ex.get("range")), intOf(ex.get("bandwidth")), intOf(ex.get("power")));
            check(String.format("bno055.sensor_config.accel:0x%02x", value),
                a.equals(want) && ex.size() == 3 && want.pack() == value);
        }
        for (Object go : arr(sc.get("gyro"))) {
            Map<String, Object> c = obj(go);
            String bytes = (String) c.get("bytes");
            Map<String, Object> ex = obj(c.get("expect"));
            Bno055Regs.GyroConfig g = Bno055Regs.GyroConfig.unpack(hex(bytes));
            Bno055Regs.GyroConfig want = new Bno055Regs.GyroConfig(
                intOf(ex.get("range")), intOf(ex.get("bandwidth")), intOf(ex.get("power")));
            check("bno055.sensor_config.gyro:" + bytes,
                g.equals(want) && ex.size() == 3 && hex(want.pack()).equals(bytes));
        }
        for (Object mo : arr(sc.get("mag"))) {
            Map<String, Object> c = obj(mo);
            int value = intOf(c.get("value"));
            Map<String, Object> ex = obj(c.get("expect"));
            Bno055Regs.MagConfig m = Bno055Regs.MagConfig.unpack(value);
            Bno055Regs.MagConfig want = new Bno055Regs.MagConfig(
                intOf(ex.get("rate")), intOf(ex.get("mode")), intOf(ex.get("power")));
            check(String.format("bno055.sensor_config.mag:0x%02x", value),
                m.equals(want) && ex.size() == 3 && want.pack() == (value & 0x7F));
        }
        seen("bno055/sensor_config", mkCfg);

        // blocks
        for (Object bo : arr(root.get("blocks"))) {
            Map<String, Object> c = obj(bo);
            int mk = mark();
            String name = (String) c.get("name");
            Bno055Regs.RawBlock b = Bno055Regs.decodeBlock(intOf(c.get("addr")), hex((String) c.get("data")));
            Map<String, Object> ex = obj(c.get("expect"));
            boolean ok = bno055BlockMatches(b, ex);
            check("bno055.blocks:" + name, ok);
            seen(c, mk);
        }
        check("bno055.blocks:full_block_is_46",
            Bno055Regs.FULL_BLOCK_ADDR == 0x08 && Bno055Regs.FULL_BLOCK_LEN == 46
                && Bno055Regs.QUAT_BLOCK_ADDR == 0x20 && Bno055Regs.QUAT_BLOCK_LEN == 8);
    }

    private static boolean intArrEquals(int[] got, List<Object> want) {
        if (got.length != want.size()) {
            return false;
        }
        for (int i = 0; i < got.length; i++) {
            if (got[i] != (int) lng(want.get(i))) {
                return false;
            }
        }
        return true;
    }

    private static boolean fieldsMatch(Map<String, Object> got, Map<String, Object> want) {
        for (Map.Entry<String, Object> e : want.entrySet()) {
            if (!Objects.equals(got.get(e.getKey()), e.getValue())) {
                return false;
            }
        }
        return true;
    }
}
