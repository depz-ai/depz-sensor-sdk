package ai.depz.sensor.test;

import java.math.BigInteger;
import java.security.MessageDigest;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;

import ai.depz.sensor.protocol.Bno055;
import ai.depz.sensor.protocol.Tmf8829;
import ai.depz.sensor.protocol.Vl53l4;
import ai.depz.sensor.protocol.Vl53l7;
import ai.depz.sensor.protocol.Vl53lx;
import ai.depz.sensor.sensors.bno055.Bno055Regs;
import ai.depz.sensor.sensors.bno086.Reports;
import ai.depz.sensor.sensors.bno086.Shtp;
import ai.depz.sensor.sensors.tmf8829.CaptureTimer;
import ai.depz.sensor.sensors.tmf8829.Frames;
import ai.depz.sensor.sensors.tmf8829.MeasurementBuilder;
import ai.depz.sensor.sensors.vl53l4.Vl53l4Uld;
import ai.depz.sensor.sensors.vl53lx.Vl53lxDecode;
import ai.depz.sensor.transport.CrcError;
import ai.depz.sensor.transport.Event;
import ai.depz.sensor.transport.Packet;
import ai.depz.sensor.transport.PacketParser;
import ai.depz.sensor.transport.Trash;

/**
 * SDK results -> golden-vector shapes (the JSON each vector group writes under
 * {@code expect}), for the differential-testing adapter
 * ({@link DifftestAdapter}, contracts/16_DIFFTEST.md). Field-for-field the same
 * mapping {@link RunVectors} asserts with (its {@code *Matches} helpers), and
 * the TS reference's test/golden/shapes.ts; the driver's golden self-check
 * holds every builder here against the vectors' {@code expect}.
 *
 * Values: {@code Long}/{@code Integer}, {@code BigInteger} (a u64 printed
 * unsigned), {@code String}, {@code Boolean}, {@code null}, {@code int[]},
 * {@code List}, {@code Map}; bytes are already lowercase hex.
 */
public final class Shapes {
    private Shapes() {}

    // ── helpers ──────────────────────────────────────────────────────────────

    private static final char[] HEXCH = "0123456789abcdef".toCharArray();

    public static String hex(byte[] b) {
        StringBuilder sb = new StringBuilder(b.length * 2);
        for (byte x : b) {
            sb.append(HEXCH[(x >>> 4) & 0xF]).append(HEXCH[x & 0xF]);
        }
        return sb.toString();
    }

    /** A u64 the SDK holds in a {@code long} (two's complement), as its unsigned value. */
    public static BigInteger u64(long v) {
        return new BigInteger(Long.toUnsignedString(v));
    }

    public static String sha256(byte[] b) {
        try {
            return hex(MessageDigest.getInstance("SHA-256").digest(b));
        } catch (java.security.NoSuchAlgorithmException e) {
            throw new IllegalStateException(e);
        }
    }

    /** Ordered map from alternating key/value arguments. */
    public static Map<String, Object> map(Object... kv) {
        Map<String, Object> m = new LinkedHashMap<>();
        for (int i = 0; i < kv.length; i += 2) {
            m.put((String) kv[i], kv[i + 1]);
        }
        return m;
    }

    // ── framing (contract 01 §5) ─────────────────────────────────────────────

    /** Feed {@code chunks} in order to one parser; the framing_decode {@code expect} shape. */
    public static Map<String, Object> framingCollect(PacketParser parser, List<byte[]> chunks) {
        List<Object> events = new ArrayList<>();
        StringBuilder trash = new StringBuilder();
        for (byte[] chunk : chunks) {
            for (Event e : parser.feed(chunk)) {
                if (e instanceof Packet p) {
                    events.add(map("type", "packet", "cmd", p.cmd(), "seq", p.seq(), "payload", hex(p.payload())));
                } else if (e instanceof CrcError ce) {
                    events.add(map("type", "crc_error", "cmd", ce.cmd(), "seq", ce.seq()));
                } else if (e instanceof Trash t) {
                    trash.append(hex(t.data()));
                }
            }
        }
        return map("events", events, "trash", trash.toString(), "residue", hex(parser.residue()),
            "header_errors", parser.headerErrors);
    }

    // ── BNO055 (contract 13) ─────────────────────────────────────────────────

    public static Map<String, Object> bno055Decode(int report, byte[] raw) {
        if (report == Bno055.Bno055Rpt.INFO.value) {
            Bno055.Bno055Info i = Bno055.Bno055Info.unpack(raw);
            return map("i2c_addr", i.i2cAddr(), "chip_id", i.chipId(), "acc_id", i.accId(), "mag_id", i.magId(),
                "gyr_id", i.gyrId(), "sw_rev", i.swRev(), "bl_rev", i.blRev(), "initialized", i.initialized(),
                "int_level", i.intLevel(), "int_edges", i.intEdges(), "read_min_us", i.readMinUs(),
                "read_max_us", i.readMaxUs(), "read_avg_us", i.readAvgUs(), "tx_dropped", i.txDropped(),
                "i2c_errors", i.i2cErrors(), "slots_skipped", i.slotsSkipped(), "bus_recoveries", i.busRecoveries(),
                "last_i2c_error", i.lastI2cError(), "sensor_resets", i.sensorResets(), "loop_max_us", i.loopMaxUs());
        }
        if (report == Bno055.Bno055Rpt.REG_DATA.value) {
            Bno055.RegData r = Bno055.RegData.unpack(raw);
            return map("cmd", r.cmd(), "timestamp_us", u64(r.timestampUs()), "data", hex(r.data()));
        }
        if (report == Bno055.Bno055Rpt.STREAM.value) {
            Bno055.StreamData s = Bno055.StreamData.unpack(raw);
            return map("timestamp_us", u64(s.timestampUs()), "addr", s.addr(), "len", s.len(), "data", hex(s.data()));
        }
        throw new IllegalArgumentException("unknown bno055 report " + report);
    }

    public static Map<String, Object> bno055Block(int addr, byte[] data) {
        Bno055Regs.RawBlock b = Bno055Regs.decodeBlock(addr, data);
        return map("accel", b.accel(), "mag", b.mag(), "gyro", b.gyro(), "euler", b.euler(),
            "quaternion", b.quaternion(), "linear_accel", b.linearAccel(), "gravity", b.gravity(),
            "temperature", b.temperature(), "calib_stat", b.calibStat());
    }

    // ── BNO086 (contract 05) ─────────────────────────────────────────────────

    public static Map<String, Object> bno086Cargo(Shtp.ShtpCargo c) {
        return map("channel", c.channel(), "seq", c.seq(), "payload", hex(c.payload()), "capture_us", u64(c.captureUs()));
    }

    public static Map<String, Object> bno086Report(Reports.Report r) {
        return map("type", r.type(), "fields", r.fields());
    }

    // ── VL53L4CD / L7 / LX ───────────────────────────────────────────────────

    public static Map<String, Object> vl53l4Decode(int report, byte[] raw) {
        switch (report) {
            case 0x91: {
                Vl53l4.RegData d = Vl53l4.RegData.unpack(raw);
                return map("cmd", d.cmd(), "timestamp_us", u64(d.timestampUs()), "data", hex(d.data()));
            }
            case 0x92: {
                Vl53l4.Vl53l4Info i = Vl53l4.Vl53l4Info.unpack(raw);
                return map("int_edges", i.intEdges(), "slots_skipped", i.slotsSkipped(), "i2c_errors", i.i2cErrors(),
                    "last_i2c_error", i.lastI2cError(), "model_id", i.modelId(), "fw_status", i.fwStatus(),
                    "initialized", i.initialized(), "xshut_level", i.xshutLevel(), "int_level", i.intLevel(),
                    "i2c_khz", i.i2cKhz());
            }
            case 0x93: {
                Vl53l4.StreamData s = Vl53l4.StreamData.unpack(raw);
                return map("timestamp_us", u64(s.timestampUs()), "addr", s.addr(), "len", s.len(), "data", hex(s.data()));
            }
            default:
                throw new IllegalArgumentException("unknown vl53l4 report " + report);
        }
    }

    /** VL53L4CD_ResultsData_t shape (vl53l4 result_block, vl53lx die_block). */
    public static Map<String, Object> vl53l4Result(Vl53l4Uld.Results r) {
        return map("range_status", r.rangeStatus(), "distance_mm", r.distanceMm(),
            "ambient_rate_kcps", r.ambientRateKcps(), "ambient_per_spad_kcps", r.ambientPerSpadKcps(),
            "signal_rate_kcps", r.signalRateKcps(), "signal_per_spad_kcps", r.signalPerSpadKcps(),
            "number_of_spad", r.numberOfSpad(), "sigma_mm", r.sigmaMm(), "stream_count", r.streamCount());
    }

    public static Map<String, Object> vl53l7Info(byte[] raw) {
        Vl53l7.Vl53l7Info i = Vl53l7.Vl53l7Info.unpack(raw);
        return map("int_edges", i.intEdges(), "frames_dropped", i.framesDropped(), "i2c_errors", i.i2cErrors(),
            "last_i2c_error", i.lastI2cError(), "lpn_level", i.lpnLevel(), "int_level", i.intLevel(),
            "i2c_khz", i.i2cKhz(), "frame_size", i.frameSize(), "streaming", i.streaming());
    }

    public static Map<String, Object> vl53lxInfo(byte[] raw) {
        Vl53lx.Vl53lxInfo i = Vl53lx.Vl53lxInfo.unpack(raw);
        return map("int_edges", i.intEdges(), "slots_skipped", i.slotsSkipped(), "i2c_errors", i.i2cErrors(),
            "last_i2c_error", i.lastI2cError(), "xshut_level", i.xshutLevel(), "int_level", i.intLevel(),
            "i2c_khz", i.i2cKhz(), "addr_width", i.addrWidth(), "n_clear", i.nClear(),
            "frames_dropped", i.framesDropped());
    }

    public static Map<String, Object> vl53lxL0xRaw(byte[] raw) {
        Vl53lxDecode.L0xRaw r = Vl53lxDecode.decodeL0xRaw(raw);
        return map("distance_raw", r.distanceRaw(), "device_range_status", r.deviceRangeStatus(),
            "signal_rate_mcps_1616", r.signalRateMcps1616(), "ambient_rate_mcps_1616", r.ambientRateMcps1616(),
            "effective_spad_count_88", r.effectiveSpadCount88());
    }

    public static Map<String, Object> vl53lxHistogramRaw(byte[] raw) {
        Vl53lxDecode.HistogramRaw r = Vl53lxDecode.decodeHistogramRaw(raw);
        return map("interrupt_status", r.interruptStatus(), "range_status", r.rangeStatus(),
            "report_status", r.reportStatus(), "stream_count", r.streamCount(),
            "dss_actual_effective_spads", r.dssActualEffectiveSpads(), "reference_phase", r.referencePhase(),
            "vcsel_start", r.vcselStart(), "bins", r.bins());
    }

    // ── TMF8829 (contract 14) ────────────────────────────────────────────────

    /** {@code _arrays(res)}: peaks, rows/cols of the arrays present, then the nullable arrays. */
    public static Map<String, Object> tmf8829Arrays(Frames.PixelResults r) {
        int[] shape = r.shape();
        return map("peaks", r.peaks(), "rows", shape[0], "cols", shape[1], "distance", r.distance(),
            "snr", r.snr(), "signal", r.signal(), "noise", r.noise(), "xtalk", r.xtalk());
    }

    public static Map<String, Object> tmf8829Decode(int report, byte[] p) {
        if (report == Tmf8829.Tmf8829Rpt.REG_DATA.value) {
            Tmf8829.RegData r = Tmf8829.RegData.unpack(p);
            return map("timestamp_us", u64(r.timestampUs()), "data", hex(r.data()));
        }
        if (report == Tmf8829.Tmf8829Rpt.INFO.value) {
            Tmf8829.Tmf8829Info i = Tmf8829.Tmf8829Info.unpack(p);
            return map("int_edges", i.intEdges(), "frames", i.frames(), "frames_aborted", i.framesAborted(),
                "frame_errors", i.frameErrors(), "spi_timeouts", i.spiTimeouts(), "int_level", i.intLevel(),
                "streaming", i.streaming(), "int_mask", i.intMask(), "last_int_status", i.lastIntStatus(),
                "last_fifo_status", i.lastFifoStatus());
        }
        if (report == Tmf8829.Tmf8829Rpt.FRAME.value) {
            Tmf8829.FrameChunk f = Tmf8829.FrameChunk.unpack(p);
            return map("timestamp_us", u64(f.timestampUs()), "full_size", f.fullSize(), "offset", f.offset(),
                "frame_seq", f.frameSeq(), "edge_us", u64(f.edgeUs()), "data", hex(f.data()));
        }
        if (report == Tmf8829.Tmf8829Rpt.EVENT.value) {
            Tmf8829.Tmf8829Event e = Tmf8829.Tmf8829Event.unpack(p);
            return map("timestamp_us", u64(e.timestampUs()), "int_status", e.intStatus(), "code", e.code(),
                "frame_id", e.frameId(), "fifo_status", e.fifoStatus());
        }
        throw new IllegalArgumentException("unknown TMF8829 report " + report);
    }

    /** FRAME-chunk payloads fed to one reassembler; the reassembly shape. */
    public static Map<String, Object> tmf8829Reassembly(List<byte[]> chunks) {
        Tmf8829.FrameReassembler r = new Tmf8829.FrameReassembler();
        List<Object> frames = new ArrayList<>();
        for (byte[] ch : chunks) {
            Tmf8829.RawFrame f = r.feed(Tmf8829.FrameChunk.unpack(ch));
            if (f != null) {
                frames.add(map("frame_seq", f.frameSeq(), "timestamp_us", u64(f.timestampUs()),
                    "edge_us", u64(f.edgeUs()), "size", f.data().length, "sha256", sha256(f.data())));
            }
        }
        return map("frames", frames, "completed", r.completed, "discarded", r.discarded, "seq_gaps", r.seqGaps);
    }

    /** tmf8829.json frames shape: header, problems, then results or data_sha256. */
    public static Map<String, Object> tmf8829Frame(byte[] raw) {
        Frames.FrameInfo f = Frames.parseFrame(raw);
        Map<String, Object> out = map(
            "header", map("fifo_status", f.fifoStatus(), "systick", f.systick(), "id", f.id(), "kind", f.kind(),
                "fp_mode", f.fpMode(), "layout", f.layout(), "payload", f.payload(), "f_number", f.fNumber(),
                "temperature", f.temperature(), "bdv", f.bdv(), "ref_pos", f.refPos(),
                "t0_integration", f.t0Integration(), "t1_integration", f.t1Integration(),
                "frame_status", f.frameStatus(), "reserved", f.reserved(), "eof", f.eof(), "size", f.size()),
            "problems", f.problems());
        if (f.kind() == Frames.FID_RESULTS) {
            out.put("results", tmf8829Arrays(Frames.parseResults(raw, f)));
        } else {
            out.put("data_sha256", sha256(Arrays.copyOfRange(raw, Frames.FRAME_HEAD, raw.length - Frames.FOOTER_SIZE)));
        }
        return out;
    }

    /** One frame of a measurements stream: raw bytes, timestamp_us, edge_us. */
    public record MeasFrame(byte[] raw, long timestampUs, long edgeUs) {}

    /** Frames through one MeasurementBuilder; the measurements {@code expect} list. */
    public static List<Object> tmf8829Measurements(List<MeasFrame> frames) {
        MeasurementBuilder b = new MeasurementBuilder();
        List<Object> got = new ArrayList<>();
        for (MeasFrame fr : frames) {
            MeasurementBuilder.Measurement m = b.add(fr.raw(), fr.timestampUs(), fr.edgeUs());
            if (m == null) {
                continue;
            }
            List<Object> ft = new ArrayList<>();
            List<Object> tl = new ArrayList<>();
            for (CaptureTimer.FrameTime t : m.times()) {
                ft.add(t.timestampUs());
                tl.add(t.t1LenUs());
            }
            Map<String, Object> o = map("timestamp_us", m.timestampUs(), "flags", m.flags(), "fp_mode", m.fpMode(),
                "f_number", m.fNumber(), "frame_times_us", ft, "t1_len_us", tl, "ratio_q16", b.clock.ratioQ16,
                "n_histograms", m.histograms().size(), "n_ref_frames", m.refFrames().size(),
                "n_problems", m.problems().size(),
                "distance_in_mm", m.frames().get(m.frames().size() - 1).distanceInMm());
            o.putAll(tmf8829Arrays(m.results()));
            got.add(o);
        }
        return got;
    }
}
