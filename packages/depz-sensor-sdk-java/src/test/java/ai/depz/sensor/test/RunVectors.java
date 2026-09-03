package ai.depz.sensor.test;

import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.Random;

import ai.depz.sensor.protocol.Common;
import ai.depz.sensor.protocol.FwDepz;
import ai.depz.sensor.protocol.Identity;
import ai.depz.sensor.protocol.Sr04;
import ai.depz.sensor.protocol.Vl53l4;
import ai.depz.sensor.transport.CrcError;
import ai.depz.sensor.transport.CrcType;
import ai.depz.sensor.transport.Crc;
import ai.depz.sensor.transport.Event;
import ai.depz.sensor.transport.Framing;
import ai.depz.sensor.transport.Packet;
import ai.depz.sensor.transport.PacketParser;
import ai.depz.sensor.transport.Trash;
import ai.depz.sensor.usb.UsbIds;
import ai.depz.sensor.sensors.vl53l4.Vl53l4Uld;
import ai.depz.sensor.sensors.vl53l8.FrameReassembler;
import ai.depz.sensor.sensors.vl53l8.Vl53l8Uld;
import ai.depz.sensor.sensors.bno086.Shtp;
import ai.depz.sensor.sensors.bno086.Sh2;
import ai.depz.sensor.sensors.bno086.Reports;
import ai.depz.sensor.dataset.Dataset;
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
        bno086ShtpVectors();
        bno086ReportsVectors();
        datasetVector();

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

    // ── crc.json ──────────────────────────────────────────────────────────────

    private static void crcVectors() throws Exception {
        System.out.println("\n[crc.json]");
        for (Object co : arr(load("crc.json").get("cases"))) {
            Map<String, Object> c = obj(co);
            String name = (String) c.get("name");
            byte[] in = hex((String) c.get("input"));
            check("crc8:" + name, Crc.crc8Maxim(in) == lng(c.get("crc8_maxim")));
            check("crc16:" + name, Crc.crc16Modbus(in) == lng(c.get("crc16_modbus")));
            check("crc32:" + name, Crc.crc32IsoHdlc(in) == lng(c.get("crc32_iso_hdlc")));
            check("crc16ccitt:" + name, Crc.crc16CcittFalse(in) == lng(c.get("crc16_ccitt_false")));
        }
    }

    // ── framing_encode.json ───────────────────────────────────────────────────

    private static void framingEncodeVectors() throws Exception {
        System.out.println("[framing_encode.json]");
        for (Object co : arr(load("framing_encode.json").get("cases"))) {
            Map<String, Object> c = obj(co);
            String name = (String) c.get("name");
            byte[] frame = Framing.buildPacket(
                intOf(c.get("cmd")),
                hex((String) c.get("payload")),
                intOf(c.get("seq")),
                CrcType.fromValue(intOf(c.get("crc_type"))));
            check("encode:" + name, hex(frame).equals(c.get("frame")));
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
            String name = (String) c.get("name");
            byte[] stream = hex((String) c.get("stream"));
            String expected = expectedDecode(obj(c.get("expect")));

            boolean ok = collect(wholeChunks(stream)).equals(expected);
            ok &= collect(bytewiseChunks(stream)).equals(expected);
            for (int round = 0; round < 5 && ok; round++) {
                ok &= collect(randomChunks(stream, rng)).equals(expected);
            }
            check("decode:" + name, ok);
        }
    }

    // ── usb_ids.json ──────────────────────────────────────────────────────────

    private static void usbIdsVectors() throws Exception {
        System.out.println("[usb_ids.json]");
        Map<String, Object> root = load("usb_ids.json");
        for (Object io : arr(root.get("identity"))) {
            Map<String, Object> c = obj(io);
            Integer vid = intOf(c.get("vid"));
            Integer pid = intOf(c.get("pid"));
            boolean known = (Boolean) c.get("known");
            String model = (String) c.get("model");
            String label = "usb:" + vid + ":" + pid;
            check(label + ":known", UsbIds.isKnownDepzUsb(vid, pid) == known);
            String hint = UsbIds.usbModelHint(vid, pid);
            check(label + ":model", java.util.Objects.equals(hint, model));
        }
        for (Object so : arr(root.get("serial_ordering"))) {
            Map<String, Object> c = obj(so);
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
        }
    }

    // ── identity.json ─────────────────────────────────────────────────────────

    private static void identityVectors() throws Exception {
        System.out.println("[identity.json]");
        for (Object co : arr(load("identity.json").get("cases"))) {
            Map<String, Object> c = obj(co);
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
        }
    }

    // ── common_commands.json ──────────────────────────────────────────────────

    private static void commonVectors() throws Exception {
        System.out.println("[common_commands.json]");
        Map<String, Object> root = load("common_commands.json");
        for (Object eo : arr(root.get("encode"))) {
            Map<String, Object> c = obj(eo);
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
        }
        for (Object do_ : arr(root.get("decode"))) {
            Map<String, Object> c = obj(do_);
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
        }
        for (Object mo : arr(root.get("sync_time_math"))) {
            Map<String, Object> c = obj(mo);
            String name = (String) c.get("name");
            long[] r = Common.syncTimeOffsetRtt(lng(c.get("t1")), lng(c.get("t2")), lng(c.get("t3")), lng(c.get("t4")));
            check("common.synctime:" + name, r[0] == lng(c.get("offset_us")) && r[1] == lng(c.get("rtt_us")));
        }
    }

    // ── sr04.json ─────────────────────────────────────────────────────────────

    private static void sr04Vectors() throws Exception {
        System.out.println("[sr04.json]");
        Map<String, Object> root = load("sr04.json");
        for (Object eo : arr(root.get("encode"))) {
            Map<String, Object> c = obj(eo);
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
        }
        for (Object do_ : arr(root.get("decode"))) {
            Map<String, Object> c = obj(do_);
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
        }
    }

    // ── fwdepz.json ───────────────────────────────────────────────────────────

    private static void fwdepzVectors() throws Exception {
        System.out.println("[fwdepz.json]");
        for (Object co : arr(load("fwdepz.json").get("cases"))) {
            Map<String, Object> c = obj(co);
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
        }
    }

    // ── vl53l4.json (VL53L4CD wire codecs + host-ULD math) ────────────────────

    private static void vl53l4EncodeVectors() throws Exception {
        System.out.println("[vl53l4.json]");
        Map<String, Object> root = load("vl53l4.json");
        for (Object eo : arr(root.get("encode"))) {
            Map<String, Object> c = obj(eo);
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
        }
    }

    private static void vl53l4DecodeVectors() throws Exception {
        Map<String, Object> root = load("vl53l4.json");
        for (Object do_ : arr(root.get("decode"))) {
            Map<String, Object> c = obj(do_);
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
        }
    }

    private static void vl53l4ResultBlockVectors() throws Exception {
        Map<String, Object> root = load("vl53l4.json");
        for (Object ro : arr(root.get("result_block"))) {
            Map<String, Object> c = obj(ro);
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
        }
    }

    private static void vl53l4TimingVectors() throws Exception {
        Map<String, Object> timing = obj(load("vl53l4.json").get("timing"));
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
    }

    private static void vl53l4TuningVectors() throws Exception {
        Map<String, Object> root = load("vl53l4.json");
        for (Object to : arr(root.get("tuning"))) {
            Map<String, Object> c = obj(to);
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
        }
    }

    private static void vl53l4ConfigBlockVector() throws Exception {
        Map<String, Object> cb = obj(load("vl53l4.json").get("config_block"));
        check("vl53l4.config_block.addr", Vl53l4Uld.CONFIG_ADDR == intOf(cb.get("addr")));
        check("vl53l4.config_block.data", hex(Vl53l4Uld.configBlock()).equals(cb.get("data")));
    }

    // ── vl53l8_advanced.json (advanced-DCI codecs, shared by CX and CH) ────────

    private static void vl53l8AdvancedVectors() throws Exception {
        System.out.println("[vl53l8_advanced.json]");
        Map<String, Object> root = load("vl53l8_advanced.json");
        for (Object mo : arr(root.get("motion"))) {
            Map<String, Object> c = obj(mo);
            String name = (String) c.get("name");
            int resolution = intOf(c.get("resolution"));
            byte[] packed = Vl53l8Uld.defaultMotionConfig(resolution).pack();
            check("vl53l8.motion:" + name, hex(packed).equals(c.get("pack")));
        }
        for (Object to : arr(root.get("thresholds"))) {
            Map<String, Object> c = obj(to);
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
        }
        for (Object xo : arr(root.get("xtalk_margin"))) {
            Map<String, Object> c = obj(xo);
            String name = (String) c.get("name");
            long raw = Vl53l8Uld.xtalkMarginToRaw(((Number) c.get("kcps")).doubleValue());
            check("vl53l8.xtalk:" + name, raw == lng(c.get("raw")));
        }
    }

    // ── vl53l8_cnh.json (VL53L8CH CNH histogram decode) ───────────────────────

    private static void vl53l8CnhVectors() throws Exception {
        System.out.println("[vl53l8_cnh.json]");
        Map<String, Object> root = load("vl53l8_cnh.json");
        Map<String, Object> config = obj(root.get("config"));
        int nbAgg = intOf(config.get("nb_of_aggregates"));
        int feat = intOf(config.get("feature_length"));
        byte[] raw = hex((String) root.get("cnh_raw"));

        Vl53l8Uld.CnhResult res = Vl53l8Uld.decodeCnh(nbAgg, feat, raw);

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
    }

    // ── vl53l8 recording replay (framing + reassembler + decoder) ─────────────
    // The capture is from a VL53L8CX (dev-default) device (software_name
    // APP_VL53L8_v0.9); the results-frame path it exercises is shared with
    // VL53L8CH (FrameReassembler + Vl53l8Uld.parseFrame).
    // Its firmware emits the ULD-2.1.0 footer layout (offset 12), so the decode
    // uses that offset; only CNH (CH-only, not decoded here) would differ.

    private static void vl53l8ReplayVector() throws Exception {
        System.out.println("[recordings/vl53l8_8x8_15hz_3s]");
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
    }

    // ── bno086_shtp.json ──────────────────────────────────────────────────────

    private static void bno086ShtpVectors() throws Exception {
        System.out.println("[bno086_shtp.json]");
        Map<String, Object> root = load("bno086_shtp.json");
        for (Object ho : arr(root.get("header"))) {
            Map<String, Object> c = obj(ho);
            String name = (String) c.get("name");
            Shtp.ShtpHeader hdr = new Shtp.ShtpHeader(
                intOf(c.get("length")), intOf(c.get("channel")), intOf(c.get("seq")),
                (Boolean) c.get("continuation"));
            boolean ok = hex(Shtp.packShtpHeader(hdr)).equals(c.get("bytes"));
            Shtp.ShtpHeader back = Shtp.unpackShtpHeader(hex((String) c.get("bytes")));
            ok &= back.equals(hdr);
            check("bno.shtp.header:" + name, ok);
        }
        Shtp.ShtpLayer txLayer = new Shtp.ShtpLayer();
        int txIdx = 0;
        for (Object to : arr(root.get("tx_seq"))) {
            Map<String, Object> c = obj(to);
            byte[] frame = txLayer.nextFrame(intOf(c.get("channel")), hex((String) c.get("payload")));
            check("bno.shtp.tx[" + txIdx++ + "]", hex(frame).equals(c.get("frame")));
        }
        for (Object ro : arr(root.get("reassembly"))) {
            Map<String, Object> c = obj(ro);
            String name = (String) c.get("name");
            List<byte[]> frames = new ArrayList<>();
            for (Object fo : arr(c.get("frames"))) {
                frames.add(hex((String) fo));
            }
            int[] disc = new int[1];
            List<Shtp.ShtpCargo> cargos = Shtp.reassemble(frames, disc);
            Map<String, Object> ex = obj(c.get("expect"));
            List<Object> wantCargos = arr(ex.get("cargos"));
            boolean ok = cargos.size() == wantCargos.size() && disc[0] == intOf(ex.get("discarded"));
            for (int i = 0; ok && i < cargos.size(); i++) {
                Map<String, Object> w = obj(wantCargos.get(i));
                Shtp.ShtpCargo g = cargos.get(i);
                ok = g.channel() == intOf(w.get("channel"))
                    && g.seq() == intOf(w.get("seq"))
                    && hex(g.payload()).equals(w.get("payload"));
            }
            check("bno.shtp.reassembly:" + name, ok);
        }
        for (Object eo : arr(root.get("control_encode"))) {
            Map<String, Object> c = obj(eo);
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
        }
    }

    // ── bno086_reports.json ───────────────────────────────────────────────────

    private static void bno086ReportsVectors() throws Exception {
        System.out.println("[bno086_reports.json]");
        Map<String, Object> root = load("bno086_reports.json");
        for (Object co : arr(root.get("input_cargos"))) {
            Map<String, Object> c = obj(co);
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
            }
            check("bno.report:" + name, ok);
        }
        for (Object go : arr(root.get("gyro_rv"))) {
            Map<String, Object> c = obj(go);
            String name = (String) c.get("name");
            byte[] cargo = hex((String) c.get("cargo"));
            long capture = lng(c.get("capture_timestamp_us"));
            Reports.Report g = Reports.parseGyroRvCargo(cargo, capture);
            Map<String, Object> ex = obj(c.get("expect"));
            boolean ok = g != null && g.type().equals(ex.get("type"))
                && fieldsMatch(g.fields(), obj(ex.get("fields")));
            check("bno.gyrorv:" + name, ok);
        }
    }

    // ── dataset_dual_sr04.depzdata ────────────────────────────────────────────

    private static void datasetVector() throws Exception {
        System.out.println("[recordings/dataset_dual_sr04.depzdata]");
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
    }

    // ── comparison helpers ────────────────────────────────────────────────────

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
