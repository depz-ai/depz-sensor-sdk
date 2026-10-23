package ai.depz.sensor.test;

import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.InputStreamReader;
import java.io.OutputStreamWriter;
import java.math.BigInteger;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.Collection;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.function.Function;

import ai.depz.sensor.protocol.Bno055;
import ai.depz.sensor.protocol.Common;
import ai.depz.sensor.protocol.FwDepz;
import ai.depz.sensor.protocol.Identity;
import ai.depz.sensor.protocol.Sr04;
import ai.depz.sensor.protocol.Tmf8829;
import ai.depz.sensor.protocol.Vl53l4;
import ai.depz.sensor.protocol.Vl53l7;
import ai.depz.sensor.protocol.Vl53lx;
import ai.depz.sensor.sensors.bno055.Bno055Regs;
import ai.depz.sensor.sensors.bno086.Reports;
import ai.depz.sensor.sensors.bno086.Sh2;
import ai.depz.sensor.sensors.bno086.Shtp;
import ai.depz.sensor.sensors.tmf8829.CaptureTimer;
import ai.depz.sensor.sensors.tmf8829.SensorClock;
import ai.depz.sensor.sensors.tmf8829.Tmf8829Config;
import ai.depz.sensor.sensors.vl53l4.Vl53l4Uld;
import ai.depz.sensor.sensors.vl53l7.Vl53l7Uld;
import ai.depz.sensor.sensors.vl53l8.Vl53l8Uld;
import ai.depz.sensor.sensors.vl53lx.Vl53lxDecode;
import ai.depz.sensor.sensors.vl53lx.Vl53lxProducts;
import ai.depz.sensor.transport.Crc;
import ai.depz.sensor.transport.CrcType;
import ai.depz.sensor.transport.Framing;
import ai.depz.sensor.transport.PacketParser;
import ai.depz.sensor.usb.Discovery;
import ai.depz.sensor.usb.UsbIds;

import static ai.depz.sensor.test.Shapes.hex;
import static ai.depz.sensor.test.Shapes.map;

/**
 * Differential-testing adapter for the Java SDK (contracts/16_DIFFTEST.md). A
 * long-lived process speaking JSON Lines on stdin/stdout:
 * <pre>
 *   {"id": n, "op": "&lt;op&gt;", "in": {...}}  -&gt;  {"id": n, "out": ...}
 *                                         or  {"id": n, "error": "invalid" | "unsupported"}
 * </pre>
 * {@code invalid} = the SDK rejected the input (threw); {@code unsupported} =
 * this SDK does not implement the op. Run (from the repo root the driver does
 * this): {@code packages/depz-sensor-sdk-java/difftest-adapter.sh}. Output
 * shapes come from {@link Shapes}.
 *
 * Integers: JSON integers beyond the long range arrive as BigInteger. A
 * {@code long} parameter takes any i64 or u64 (a u64 >= 2^63 in Java's
 * two's-complement convention); an {@code int} parameter takes any i32 or u32
 * (same convention). A value outside that is one no Java caller can pass at
 * all, so it is answered {@code invalid} rather than squeezed into the type:
 * clamping or wrapping it would compare a different input against the
 * reference and report a divergence that is the adapter's, not the SDK's.
 */
public final class DifftestAdapter {
    private DifftestAdapter() {}

    /** Ops this SDK declares it cannot do, with the reason (never a silent gap). */
    static final Map<String, String> UNSUPPORTED = new LinkedHashMap<>();


    // ── input coercion ───────────────────────────────────────────────────────

    @SuppressWarnings("unchecked")
    static Map<String, Object> obj(Object o) {
        if (!(o instanceof Map)) {
            throw new IllegalArgumentException("not an object");
        }
        return (Map<String, Object>) o;
    }

    @SuppressWarnings("unchecked")
    static List<Object> arr(Object o) {
        if (!(o instanceof List)) {
            throw new IllegalArgumentException("not a list");
        }
        return (List<Object>) o;
    }

    static final BigInteger U64_MAX = BigInteger.ONE.shiftLeft(64).subtract(BigInteger.ONE);

    /** Thrown for an input no Java caller could pass (outside the parameter type): answered invalid. */
    static final class NotRepresentable extends IllegalArgumentException {
        NotRepresentable(Object v, String type) {
            super(v + " is not representable as a Java " + type);
        }
    }

    static long L(Object o) {
        if (o instanceof Long l) {
            return l;
        }
        if (o instanceof BigInteger b) {
            if (b.signum() > 0 && b.compareTo(U64_MAX) <= 0) {
                return b.longValue(); // a u64 >= 2^63 as Java holds it (two's complement)
            }
            throw new NotRepresentable(b, "long");
        }
        throw new IllegalArgumentException("not an integer: " + o);
    }

    static int I(Object o) {
        long v = L(o);
        if (o instanceof BigInteger) {
            throw new NotRepresentable(o, "int");
        }
        if (v >= Integer.MIN_VALUE && v <= Integer.MAX_VALUE) {
            return (int) v;
        }
        if (v > 0 && v <= 0xFFFFFFFFL) {
            return (int) v; // a u32 in Java's int convention
        }
        throw new NotRepresentable(o, "int");
    }

    /** An {@code int} parameter that is a signed i32 (no u32 reading): outside i32 → invalid. */
    static int I32(Object o) {
        long v = L(o);
        if (o instanceof BigInteger || v < Integer.MIN_VALUE || v > Integer.MAX_VALUE) {
            throw new NotRepresentable(o, "i32 int");
        }
        return (int) v;
    }

    /**
     * A {@code long} parameter that is a u64 (Java's two's-complement convention):
     * a negative JSON value is no u64 at all, so it is invalid, not 2^64 - |v|.
     */
    static long U64(Object o) {
        if (o instanceof Long l && l < 0) {
            throw new NotRepresentable(o, "u64 long");
        }
        return L(o);
    }

    static Integer nI(Object o) {
        return o == null ? null : I(o);
    }

    static double D(Object o) {
        if (o instanceof Number n) {
            return n.doubleValue();
        }
        throw new IllegalArgumentException("not a number: " + o);
    }

    static boolean B(Object o) {
        if (o instanceof Boolean b) {
            return b;
        }
        throw new IllegalArgumentException("not a boolean: " + o);
    }

    static String S(Object o) {
        if (o == null || o instanceof String) {
            return (String) o;
        }
        throw new IllegalArgumentException("not a string: " + o);
    }

    /** Lowercase hex string -> bytes (anything else is a rejected input). */
    static byte[] h(Object o) {
        if (!(o instanceof String s) || (s.length() & 1) != 0) {
            throw new IllegalArgumentException("not a hex string");
        }
        byte[] out = new byte[s.length() / 2];
        for (int i = 0; i < out.length; i++) {
            int hi = Character.digit(s.charAt(2 * i), 16);
            int lo = Character.digit(s.charAt(2 * i + 1), 16);
            if (hi < 0 || lo < 0 || Character.isUpperCase(s.charAt(2 * i)) || Character.isUpperCase(s.charAt(2 * i + 1))) {
                throw new IllegalArgumentException("not a hex string");
            }
            out[i] = (byte) ((hi << 4) | lo);
        }
        return out;
    }

    static List<byte[]> hexList(Object o) {
        List<byte[]> out = new ArrayList<>();
        for (Object x : arr(o)) {
            out.add(h(x));
        }
        return out;
    }

    static Map<String, Object> payload(byte[] p) {
        return map("payload", hex(p));
    }

    // ── ops ──────────────────────────────────────────────────────────────────

    static final Map<String, Function<Map<String, Object>, Object>> OPS = new LinkedHashMap<>();

    static void op(String name, Function<Map<String, Object>, Object> fn) {
        OPS.put(name, fn);
    }

    static {
        op("crc", i -> {
            byte[] raw = h(i.get("input"));
            return map("crc8_maxim", Crc.crc8Maxim(raw), "crc16_modbus", Crc.crc16Modbus(raw),
                "crc32_iso_hdlc", Crc.crc32IsoHdlc(raw), "crc16_ccitt_false", Crc.crc16CcittFalse(raw));
        });
        op("framing_encode", i -> map("frame", hex(Framing.buildPacket(I(i.get("cmd")), h(i.get("payload")),
            L(i.get("seq")), CrcType.fromValue(I(i.get("crc_type")))))));
        op("framing_decode", i -> Shapes.framingCollect(new PacketParser(),
            i.containsKey("chunks") ? hexList(i.get("chunks")) : List.of(h(i.get("stream")))));
        op("identity", i -> {
            Identity id = Identity.parseSoftwareName(Common.stripDeviceString(h(i.get("raw"))));
            return map("mode", id.mode(), "sensor_type", id.sensorType() == null ? null : id.sensorType().label,
                "software_name", id.softwareName(), "version", id.version());
        });
        op("usb_identity", i -> map("known", UsbIds.isKnownDepzUsb(nI(i.get("vid")), nI(i.get("pid"))),
            "model", UsbIds.usbModelHint(nI(i.get("vid")), nI(i.get("pid")))));
        op("usb_serial_ordering", i -> {
            List<UsbIds.PortRef> refs = new ArrayList<>();
            for (Object po : arr(i.get("ports"))) {
                Map<String, Object> p = obj(po);
                refs.add(new UsbIds.PortRef(S(p.get("port")), S(p.get("serial"))));
            }
            List<Object> order = new ArrayList<>();
            for (UsbIds.PortRef r : UsbIds.orderBySerial(refs)) {
                order.add(r.port());
            }
            return map("order", order);
        });
        op("probe_resolve", i -> map("class", Discovery.resolveSensorClass(S(i.get("software_name")),
            S(i.get("device_name")), nI(i.get("usb_vid")), nI(i.get("usb_pid")))));
        op("common_encode", i -> {
            String kind = S(i.get("kind"));
            switch (kind) {
                case "sync_time_request":
                    return payload(Common.packSyncTime(U64(i.get("pc_timestamp_us"))));
                case "set_payload_crc_type": {
                    // No codec function: the device layer writes the byte of a
                    // CrcType argument, so the op checks the enum like the type does.
                    CrcType t = CrcType.fromValue(I(i.get("crc_type")));
                    return payload(new byte[] {(byte) t.value});
                }
                case "sync_pin_config":
                    return payload(new Common.SyncPinConfig(I(i.get("pin")),
                        Common.SyncPinMode.fromValue(I(i.get("mode"))),
                        Common.SyncPinPolarity.fromValue(I(i.get("polarity")))).pack());
                default:
                    throw new IllegalArgumentException(kind);
            }
        });
        op("common_decode", i -> {
            byte[] p = h(i.get("payload"));
            switch (I(i.get("report"))) {
                case 0x80: {
                    Common.StatusReport r = Common.StatusReport.unpack(p);
                    return map("cmd", r.cmd(), "status", r.status());
                }
                case 0x81: {
                    Common.TextReport r = Common.TextReport.unpack(p);
                    return map("cmd", r.cmd(), "text", r.text());
                }
                case 0x83: {
                    Common.TemperatureReport r = Common.TemperatureReport.unpack(p);
                    return map("timestamp_us", Shapes.u64(r.timestampUs()), "raw_decidegrees", r.rawDecidegrees());
                }
                case 0x84: {
                    Common.SequenceErrorReport r = Common.SequenceErrorReport.unpack(p);
                    return map("expected_seq", r.expectedSeq(), "received_seq", r.receivedSeq());
                }
                default:
                    throw new IllegalArgumentException("report " + i.get("report"));
            }
        });
        op("sync_time_math", i -> {
            long[] r = Common.syncTimeOffsetRtt(L(i.get("t1")), L(i.get("t2")), L(i.get("t3")), L(i.get("t4")));
            return map("offset_us", r[0], "rtt_us", r[1]);
        });
        op("sr04_encode", i -> {
            String kind = S(i.get("kind"));
            if (kind.equals("set_sample_period")) {
                return payload(Sr04.packSamplePeriod(L(i.get("period_us"))));
            }
            if (kind.equals("set_echo_decay")) {
                return payload(Sr04.packEchoDecay(I(i.get("decay_us"))));
            }
            throw new IllegalArgumentException(kind);
        });
        op("sr04_decode", i -> {
            byte[] p = h(i.get("payload"));
            switch (I(i.get("report"))) {
                case 0x91: {
                    Sr04.Sr04Data d = Sr04.Sr04Data.unpack(p);
                    return map("source_cmd", d.sourceCmd(), "timestamp_us", Shapes.u64(d.timestampUs()),
                        "echo_time_us", d.echoTimeUs());
                }
                case 0x92:
                    return map("period_us", Sr04.unpackSamplePeriod(p));
                case 0x93:
                    return map("decay_us", Sr04.unpackEchoDecay(p));
                default:
                    throw new IllegalArgumentException("report " + i.get("report"));
            }
        });
        op("fwdepz_parse", i -> {
            FwDepz.FwDepzImage img = FwDepz.FwDepzImage.parse(h(i.get("file")));
            return map("load_addr", img.loadAddr(), "fw_size", img.fwSize(), "fw_crc32", img.fwCrc32(),
                "cur_sec", img.curSec(), "tot_sec", img.totSec(), "payload_crc_ok", img.payloadCrcOk());
        });

        // ── BNO055 ──
        op("bno055_encode", i -> {
            String kind = S(i.get("kind"));
            switch (kind) {
                case "read_reg":
                    return payload(Bno055.packReadReg(I(i.get("addr")), I(i.get("len"))));
                case "write_reg":
                    return payload(Bno055.packWriteReg(I(i.get("addr")), h(i.get("data"))));
                case "start_stream":
                    return payload(Bno055.packStartStream(I(i.get("trigger")), I(i.get("addr")), I(i.get("len")),
                        I(i.get("period_ms"))));
                case "reset":
                case "stop_stream":
                case "get_info":
                    return payload(new byte[0]);
                default:
                    throw new IllegalArgumentException(kind);
            }
        });
        op("bno055_decode", i -> Shapes.bno055Decode(I(i.get("report")), h(i.get("payload"))));
        op("bno055_units", i -> {
            Bno055Regs.Units u = Bno055Regs.Units.unpack(I(i.get("unit_sel")));
            return map("accel_mg", u.accelMg(), "gyro_rps", u.gyroRps(), "euler_rad", u.eulerRad(),
                "temp_f", u.tempF(), "android", u.android(), "repack", u.pack());
        });
        op("bno055_calib_stat", i -> {
            Bno055Regs.CalibStatus s = Bno055Regs.CalibStatus.unpack(I(i.get("value")));
            return map("system", s.system(), "gyro", s.gyro(), "accel", s.accel(), "mag", s.mag(),
                "fully_calibrated", s.fullyCalibrated(), "repack", s.pack());
        });
        op("bno055_calibration_profile", i -> {
            Bno055Regs.CalibrationProfile p = Bno055Regs.CalibrationProfile.unpack(h(i.get("bytes")));
            return map("accel_offset", p.accelOffset(), "mag_offset", p.magOffset(), "gyro_offset", p.gyroOffset(),
                "accel_radius", p.accelRadius(), "mag_radius", p.magRadius(), "repack", hex(p.pack()));
        });
        op("bno055_axis_remap", i -> {
            Bno055Regs.AxisRemap a = Bno055Regs.AxisRemap.unpack(I(i.get("config")), I(i.get("sign")));
            return map("x", a.x(), "y", a.y(), "z", a.z(), "x_negative", a.xNegative(),
                "y_negative", a.yNegative(), "z_negative", a.zNegative(), "repack", a.pack());
        });
        op("bno055_axis_remap_pack", i -> map("repack", new Bno055Regs.AxisRemap(I(i.get("x")), I(i.get("y")),
            I(i.get("z")), B(i.get("x_negative")), B(i.get("y_negative")), B(i.get("z_negative"))).pack()));
        op("bno055_sensor_config", i -> {
            String sensor = S(i.get("sensor"));
            switch (sensor) {
                case "accel": {
                    Bno055Regs.AccelConfig a = Bno055Regs.AccelConfig.unpack(I(i.get("value")));
                    return map("range", a.range(), "bandwidth", a.bandwidth(), "power", a.power(), "repack", a.pack());
                }
                case "gyro": {
                    Bno055Regs.GyroConfig g = Bno055Regs.GyroConfig.unpack(h(i.get("bytes")));
                    return map("range", g.range(), "bandwidth", g.bandwidth(), "power", g.power(),
                        "repack", hex(g.pack()));
                }
                case "mag": {
                    Bno055Regs.MagConfig m = Bno055Regs.MagConfig.unpack(I(i.get("value")));
                    return map("rate", m.rate(), "mode", m.mode(), "power", m.power(), "repack", m.pack());
                }
                default:
                    throw new IllegalArgumentException(sensor);
            }
        });
        op("bno055_block", i -> Shapes.bno055Block(I(i.get("addr")), h(i.get("data"))));

        // ── BNO086 ──
        op("bno086_shtp_header_pack", i -> map("bytes", hex(Shtp.packShtpHeader(new Shtp.ShtpHeader(
            I(i.get("length")), I(i.get("channel")), I(i.get("seq")), B(i.get("continuation")))))));
        op("bno086_shtp_header_unpack", i -> {
            Shtp.ShtpHeader r = Shtp.unpackShtpHeader(h(i.get("bytes")));
            return map("length", r.length(), "channel", r.channel(), "seq", r.seq(), "continuation", r.continuation());
        });
        op("bno086_shtp_tx_seq", i -> {
            Shtp.ShtpLayer layer = new Shtp.ShtpLayer();
            List<Object> frames = new ArrayList<>();
            for (Object po : arr(i.get("packets"))) {
                Map<String, Object> p = obj(po);
                frames.add(hex(layer.nextFrame(I(p.get("channel")), h(p.get("payload")))));
            }
            return map("frames", frames);
        });
        op("bno086_shtp_reassembly", i -> {
            List<Object> frames = arr(i.get("frames"));
            List<Object> caps = arr(i.get("captures_us"));
            if (frames.size() != caps.size()) {
                throw new IllegalArgumentException("frames / captures_us length");
            }
            Shtp.ShtpLayer layer = new Shtp.ShtpLayer();
            List<Object> cargos = new ArrayList<>();
            for (int k = 0; k < frames.size(); k++) {
                Shtp.ShtpCargo c = layer.feed(h(frames.get(k)), U64(caps.get(k)));
                if (c != null) {
                    cargos.add(Shapes.bno086Cargo(c));
                }
            }
            return map("cargos", cargos, "discarded", layer.discarded);
        });
        op("bno086_control_encode", i -> {
            String kind = S(i.get("kind"));
            switch (kind) {
                case "set_feature":
                    return payload(Sh2.buildSetFeature(I(i.get("sensor_id")), L(i.get("interval_us")),
                        L(i.get("batch_us")), I(i.get("sensitivity")), I(i.get("flags")), L(i.get("cfg_word"))));
                case "get_feature_request":
                    return payload(Sh2.buildGetFeatureRequest(I(i.get("sensor_id"))));
                case "product_id_request":
                    return payload(Sh2.buildProductIdRequest());
                case "command_request":
                    return payload(Sh2.buildCommandRequest(I(i.get("seq")), I(i.get("command")), h(i.get("params"))));
                case "frs_read_request":
                    return payload(Sh2.buildFrsReadRequest(I(i.get("frs_type")), I(i.get("offset_words")),
                        I(i.get("block_words"))));
                case "frs_write_request":
                    return payload(Sh2.buildFrsWriteRequest(I(i.get("frs_type")), I(i.get("length_words"))));
                case "frs_write_data": {
                    List<Object> ws = arr(i.get("words"));
                    long[] words = new long[ws.size()];
                    for (int k = 0; k < words.length; k++) {
                        words[k] = L(ws.get(k));
                    }
                    return payload(Sh2.buildFrsWriteData(I(i.get("offset_words")), words));
                }
                default:
                    throw new IllegalArgumentException(kind);
            }
        });
        op("bno086_input_cargo", i -> {
            List<Object> reports = new ArrayList<>();
            for (Reports.Report r : Reports.parseInputCargo(h(i.get("cargo")), L(i.get("capture_timestamp_us")))) {
                reports.add(Shapes.bno086Report(r));
            }
            return map("reports", reports);
        });
        op("bno086_record", i -> {
            List<Object> records = new ArrayList<>();
            for (Reports.Report r : Reports.parseInputCargo(h(i.get("cargo")), L(i.get("capture_timestamp_us")))) {
                records.add(ai.depz.sensor.dataset.DatasetRecords.bno086(r));
            }
            return map("records", records);
        });
        op("bno086_gyro_rv", i -> {
            Reports.Report r = Reports.parseGyroRvCargo(h(i.get("cargo")), L(i.get("capture_timestamp_us")));
            return r == null ? null : Shapes.bno086Report(r);
        });

        // ── TMF8829 ──
        op("tmf8829_encode", i -> {
            int cmd = I(i.get("cmd"));
            if (cmd == Tmf8829.Tmf8829Cmd.READ_REG.value) {
                return payload(Tmf8829.packReadReg(I(i.get("addr")), I(i.get("len"))));
            }
            if (cmd == Tmf8829.Tmf8829Cmd.WRITE_REG.value) {
                return payload(Tmf8829.packWriteReg(I(i.get("addr")), h(i.get("data"))));
            }
            if (cmd == Tmf8829.Tmf8829Cmd.START_STREAM.value) {
                return payload(Tmf8829.packStartStream(I(i.get("int_mask")), I(i.get("flags"))));
            }
            return payload(new byte[0]);
        });
        op("tmf8829_decode", i -> Shapes.tmf8829Decode(I(i.get("report")), h(i.get("payload"))));
        op("tmf8829_reassembly", i -> Shapes.tmf8829Reassembly(hexList(i.get("chunks"))));
        op("tmf8829_frame", i -> Shapes.tmf8829Frame(h(i.get("raw"))));
        op("tmf8829_measurements", i -> {
            List<Shapes.MeasFrame> frames = new ArrayList<>();
            for (Object fo : arr(i.get("frames"))) {
                Map<String, Object> f = obj(fo);
                frames.add(new Shapes.MeasFrame(h(f.get("raw")), U64(f.get("timestamp_us")), U64(f.get("edge_us"))));
            }
            return map("measurements", Shapes.tmf8829Measurements(frames));
        });
        op("tmf8829_config_get", i -> map("value", Tmf8829Config.cfgGet(h(i.get("page")), S(i.get("field")))));
        op("tmf8829_config_set", i -> {
            byte[] pg = h(i.get("page"));
            Tmf8829Config.cfgSet(pg, S(i.get("field")), I(i.get("value")));
            return map("page", hex(pg));
        });
        op("tmf8829_frame_time", i -> {
            SensorClock clock = new SensorClock();
            CaptureTimer timer = new CaptureTimer(clock);
            List<Object> times = new ArrayList<>();
            for (Object ro : arr(i.get("rows"))) {
                Map<String, Object> r = obj(ro);
                int fid = I(r.get("id"));
                if (fid < 0 || fid > 0xFF) {
                    throw new NotRepresentable(r.get("id"), "frame-id byte"); // split as the frame parser does
                }
                int kind = fid & 0xF0;
                if (kind == 0x30) {
                    continue;
                }
                long tick = L(r.get("systick"));
                int flags = clock.add(U64(r.get("ts")), tick);
                if (kind == 0x20) {
                    timer.histograms();
                    continue;
                }
                CaptureTimer.FrameTime t = timer.results(fid & 0x0F, tick, L(r.get("t0")), L(r.get("t1")),
                    U64(r.get("ts")), U64(r.get("edge_us")), flags);
                times.add(map("timestamp_us", t.timestampUs(), "t1_len_us", t.t1LenUs(), "flags", t.flags(),
                    "ratio_q16", clock.ratioQ16));
            }
            return map("times", times);
        });

        // ── VL53L4CD ──
        op("vl53l4_encode", i -> {
            String kind = S(i.get("kind"));
            switch (kind) {
                case "read_reg":
                    return payload(Vl53l4.packReadReg(I(i.get("addr")), I(i.get("len"))));
                case "write_reg":
                    return payload(Vl53l4.packWriteReg(I(i.get("addr")), h(i.get("data"))));
                case "xshut":
                    return payload(Vl53l4.packXshut(I(i.get("action"))));
                case "start_stream":
                    return payload(Vl53l4.packStartStream(I(i.get("addr")), I(i.get("len")), I(i.get("flags"))));
                case "set_i2c_speed":
                    return payload(Vl53l4.packSetI2cSpeed(I(i.get("khz"))));
                default:
                    throw new IllegalArgumentException(kind);
            }
        });
        op("vl53l4_decode", i -> Shapes.vl53l4Decode(I(i.get("report")), h(i.get("payload"))));
        op("vl53l4_result_block", i -> Shapes.vl53l4Result(Vl53l4Uld.parseResultBlock(h(i.get("raw")))));
        op("vl53l4_timing_encode", i -> {
            int[] r = Vl53l4Uld.rangeTimingRegisters(I(i.get("timing_budget_ms")), L(i.get("inter_measurement_ms")),
                I(i.get("osc_frequency")), I(i.get("clock_pll")));
            return map("range_config_a", r[0], "range_config_b", r[1],
                "intermeasurement_raw", Integer.toUnsignedLong(r[2]));
        });
        op("vl53l4_timing_decode", i -> {
            int[] r = Vl53l4Uld.decodeRangeTiming(L(i.get("intermeasurement_raw")), I(i.get("clock_pll")),
                I(i.get("osc_frequency")), I(i.get("range_config_a")));
            return map("timing_budget_ms", r[0], "inter_measurement_ms", r[1]);
        });
        op("vl53l4_tuning_encode", i -> {
            int v = I32(i.get("value")); // offset is signed: no u32 reading
            String kind = S(i.get("kind"));
            switch (kind) {
                case "offset": return map("raw", Vl53l4Uld.offsetRaw(v));
                case "xtalk": return map("raw", Vl53l4Uld.xtalkRaw(v));
                case "signal_threshold": return map("raw", Vl53l4Uld.signalThresholdRaw(v));
                case "sigma_threshold": return map("raw", Vl53l4Uld.sigmaThresholdRaw(v));
                default: throw new IllegalArgumentException(kind);
            }
        });
        op("vl53l4_tuning_decode", i -> {
            int raw = I(i.get("raw"));
            String kind = S(i.get("kind"));
            switch (kind) {
                case "offset": return map("value", Vl53l4Uld.decodeOffset(raw));
                case "xtalk": return map("value", Vl53l4Uld.decodeXtalk(raw));
                case "signal_threshold": return map("value", Vl53l4Uld.decodeSignalThreshold(raw));
                case "sigma_threshold": return map("value", Vl53l4Uld.decodeSigmaThreshold(raw));
                default: throw new IllegalArgumentException(kind);
            }
        });

        // ── VL53L5CX / L7 ──
        op("vl53l7_encode", i -> {
            String kind = S(i.get("kind"));
            switch (kind) {
                case "read_reg":
                    return payload(Vl53l7.packReadReg(I(i.get("addr")), I(i.get("len"))));
                case "write_reg":
                    return payload(Vl53l7.packWriteReg(I(i.get("addr")), h(i.get("data"))));
                case "pin_ctrl":
                    return payload(Vl53l7.packPinCtrl(I(i.get("action"))));
                case "set_i2c_speed":
                    return payload(Vl53l7.packSetI2cSpeed(I(i.get("khz"))));
                default:
                    throw new IllegalArgumentException(kind);
            }
        });
        op("vl53l7_decode", i -> {
            if (I(i.get("report")) != Vl53l7.Vl53l7Rpt.VL53_INFO.value) {
                throw new IllegalArgumentException("report " + i.get("report"));
            }
            return Shapes.vl53l7Info(h(i.get("payload")));
        });
        op("vl53l7_model", i -> map("class",
            Vl53l7Uld.resolveModel(S(i.get("usb_model")), S(i.get("device_name"))).label));

        // ── VL53L 1D family ──
        op("vl53lx_encode", i -> {
            String kind = S(i.get("kind"));
            switch (kind) {
                case "set_addr_width":
                    return payload(Vl53lx.packSetAddrWidth(I(i.get("width"))));
                case "read_reg":
                    return payload(Vl53lx.packReadReg(I(i.get("addr")), I(i.get("len"))));
                case "write_reg":
                    return payload(Vl53lx.packWriteReg(I(i.get("addr")), h(i.get("data"))));
                case "xshut":
                    return payload(Vl53lx.packXshut(I(i.get("action"))));
                case "set_i2c_speed":
                    return payload(Vl53lx.packSetI2cSpeed(I(i.get("khz"))));
                case "start_stream": {
                    List<Object> cl = arr(i.get("clear"));
                    int[][] clear = new int[cl.size()][];
                    for (int k = 0; k < clear.length; k++) {
                        List<Object> p = arr(cl.get(k));
                        clear[k] = new int[] {I(p.get(0)), I(p.get(1))};
                    }
                    return payload(Vl53lx.packStartStream(I(i.get("addr")), I(i.get("len")), clear,
                        I(i.get("flags"))));
                }
                default:
                    throw new IllegalArgumentException(kind);
            }
        });
        op("vl53lx_decode", i -> {
            if (I(i.get("report")) != Vl53lx.Vl53lxRpt.INFO.value) {
                throw new IllegalArgumentException("report " + i.get("report"));
            }
            return Shapes.vl53lxInfo(h(i.get("payload")));
        });
        op("vl53lx_model", i -> {
            String dev = S(i.get("device_name"));
            return map("class", Vl53lxProducts.resolveClass(S(i.get("usb_model")), dev).className,
                "product", Vl53lxProducts.productFromBoardName(dev));
        });
        op("vl53lx_die_block", i -> Shapes.vl53l4Result(Vl53lxDecode.decodeDieBlock(h(i.get("raw")),
            S(i.get("variant")))));
        op("vl53lx_l0x_raw", i -> Shapes.vl53lxL0xRaw(h(i.get("raw"))));
        op("vl53lx_histogram_raw", i -> Shapes.vl53lxHistogramRaw(h(i.get("raw"))));

        // ── VL53L8 advanced ──
        // The pure DCI codecs the driver methods call.
        // set_xtalk_margin's own check (0..10000) lives in Vl53l8Uld.xtalkMarginToRaw.
        op("vl53l8_motion_config", i -> map("pack", hex(Vl53l8Uld.defaultMotionConfig(I(i.get("resolution"))).pack())));
        op("vl53l8_detection_thresholds", i -> {
            List<Vl53l8Uld.DetectionThreshold> ths = new ArrayList<>();
            for (Object to : arr(i.get("thresholds"))) {
                Map<String, Object> t = obj(to);
                ths.add(new Vl53l8Uld.DetectionThreshold(I32(t.get("low_thresh")), I32(t.get("high_thresh")),
                    I(t.get("measurement")), I(t.get("type")), I(t.get("zone_num")), I(t.get("operation"))));
            }
            Vl53l8Uld.PackedThresholds p = Vl53l8Uld.packDetectionThresholds(ths);
            return map("start_block", hex(p.start()), "valid_status", hex(p.valid()));
        });
        op("vl53l8_xtalk_margin", i -> map("raw", Vl53l8Uld.xtalkMarginToRaw(D(i.get("kcps")))));
        op("vl53l8_cnh_decode", i -> {
            Map<String, Object> c = obj(i.get("config"));
            ai.depz.sensor.sensors.vl53l8.Vl53l8CnhSetup setup = new ai.depz.sensor.sensors.vl53l8.Vl53l8CnhSetup();
            setup.initConfig(I32(c.get("start_bin")), I32(c.get("num_bins")), I32(c.get("sub_sample")));
            List<Object> m = arr(c.get("agg_map"));
            if (m.size() != 6) {
                throw new IllegalArgumentException("agg_map: start_x, start_y, merge_x, merge_y, cols, rows");
            }
            setup.createAggMap(I32(c.get("resolution")), I32(m.get(0)), I32(m.get(1)), I32(m.get(2)), I32(m.get(3)),
                I32(m.get(4)), I32(m.get(5)));
            Vl53l8Uld.CnhResult r = Vl53l8Uld.decodeCnh(setup.nbOfAggregates(), setup.featureLength(), h(i.get("cnh_raw")));
            List<Object> aggs = new ArrayList<>();
            for (Vl53l8Uld.CnhAggregate a : r.aggregates) {
                List<Object> raw = new ArrayList<>();
                for (int x : a.histRaw) {
                    raw.add((long) x);
                }
                List<Object> sc = new ArrayList<>();
                for (int x : a.histScaler) {
                    sc.add((long) x);
                }
                aggs.add(map("hist_raw", raw, "hist_scaler", sc));
            }
            return map("aggregates", aggs);
        });
    }

    // ── canonical JSON (contract 15 §3 + 16 §3) ──────────────────────────────

    static void canon(Object v, StringBuilder sb) {
        if (v == null) {
            sb.append("null");
        } else if (v instanceof Boolean || v instanceof Long || v instanceof Integer || v instanceof BigInteger
                || v instanceof Short || v instanceof Byte) {
            sb.append(v);
        } else if (v instanceof Double || v instanceof Float) {
            double d = ((Number) v).doubleValue();
            if (Double.isNaN(d)) {
                sb.append("\"NaN\"");
            } else if (Double.isInfinite(d)) {
                sb.append(d > 0 ? "\"Infinity\"" : "\"-Infinity\"");
            } else {
                sb.append(d);
            }
        } else if (v instanceof String s) {
            str(s, sb);
        } else if (v instanceof byte[] b) {
            str(hex(b), sb);
        } else if (v instanceof int[] a) {
            sb.append('[');
            for (int k = 0; k < a.length; k++) {
                sb.append(k == 0 ? "" : ",").append(a[k]);
            }
            sb.append(']');
        } else if (v instanceof long[] a) {
            sb.append('[');
            for (int k = 0; k < a.length; k++) {
                sb.append(k == 0 ? "" : ",").append(a[k]);
            }
            sb.append(']');
        } else if (v instanceof double[] a) {
            List<Object> l = new ArrayList<>();
            for (double d : a) {
                l.add(d);
            }
            canon(l, sb);
        } else if (v instanceof Collection<?> c) {
            sb.append('[');
            boolean first = true;
            for (Object x : c) {
                sb.append(first ? "" : ",");
                first = false;
                canon(x, sb);
            }
            sb.append(']');
        } else if (v instanceof Map<?, ?> m) {
            sb.append('{');
            boolean first = true;
            for (Map.Entry<?, ?> e : m.entrySet()) {
                sb.append(first ? "" : ",");
                first = false;
                str((String) e.getKey(), sb);
                sb.append(':');
                canon(e.getValue(), sb);
            }
            sb.append('}');
        } else {
            throw new IllegalStateException("cannot serialize " + v.getClass());
        }
    }

    /** JSON string, ASCII-only (everything else as a UTF-16 \\u escape). */
    static void str(String s, StringBuilder sb) {
        sb.append('"');
        for (int k = 0; k < s.length(); k++) {
            char c = s.charAt(k);
            if (c == '"' || c == '\\') {
                sb.append('\\').append(c);
            } else if (c < 0x20 || c > 0x7e) {
                sb.append(String.format("\\u%04x", (int) c));
            } else {
                sb.append(c);
            }
        }
        sb.append('"');
    }

    static String handle(String line) {
        Map<String, Object> req = obj(Json.parse(line));
        StringBuilder sb = new StringBuilder("{\"id\":");
        canon(req.get("id"), sb);
        Object opName = req.get("op");
        Function<Map<String, Object>, Object> fn = OPS.get(opName);
        if (UNSUPPORTED.containsKey(opName) || fn == null) {
            sb.append(",\"error\":\"unsupported\",\"detail\":");
            str(UNSUPPORTED.getOrDefault(opName, "unknown op"), sb);
            return sb.append('}').toString();
        }
        StringBuilder out = new StringBuilder();
        try {
            canon(fn.apply(obj(req.get("in"))), out);
        } catch (RuntimeException e) {
            // every rejection the SDK can raise (IllegalArgumentException, its own
            // *Error types, index/arith faults on a malformed input) is `invalid`
            String detail = e.getClass().getSimpleName() + ": " + e.getMessage();
            sb.append(",\"error\":\"invalid\",\"detail\":");
            str(detail.length() > 300 ? detail.substring(0, 300) : detail, sb);
            return sb.append('}').toString();
        }
        return sb.append(",\"out\":").append(out).append('}').toString();
    }

    public static void main(String[] args) throws Exception {
        BufferedReader in = new BufferedReader(new InputStreamReader(System.in, StandardCharsets.UTF_8));
        BufferedWriter out = new BufferedWriter(new OutputStreamWriter(System.out, StandardCharsets.UTF_8));
        String line;
        while ((line = in.readLine()) != null) {
            if (line.isBlank()) {
                continue;
            }
            out.write(handle(line));
            out.write('\n');
            out.flush();
        }
    }
}
