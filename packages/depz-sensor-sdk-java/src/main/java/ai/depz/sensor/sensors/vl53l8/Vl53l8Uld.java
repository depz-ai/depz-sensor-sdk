package ai.depz.sensor.sensors.vl53l8;

import java.util.List;

/**
 * VL53L8CX/CH ULD — the host-verifiable decode layer only.
 *
 * <p>The DEPZ ToF is <b>two</b> sensors: <b>VL53L8CX</b> (the base multizone
 * part, also the dev/unprogrammed default) and <b>VL53L8CH</b> (CX plus CNH
 * compact histograms and its own production USB PID {@code 0xED40}). Their
 * results-frame wire layout is identical, so <b>this decoder serves both</b>:
 * {@link #parseFrame} is shared, and the advanced-feature DCI codecs (motion
 * configuration, detection thresholds, xtalk margin) apply equally to CX and CH.
 * The only variant difference in the decode path is the footer-id offset —
 * see {@link Variant}. Semantics mirror the TS/Python references 1:1.
 *
 * <p>This is a port of the parts of the ST ULD ({@code vl53l8cx_api.c}, ULD
 * 2.1.0) that are pure, hardware-independent and covered by golden vectors.
 *
 * <p>OUT OF SCOPE (extension points, intentionally not ported):
 * <ul>
 *   <li><b>Live ULD init / config register-bridge driver</b> (both variants):
 *       sensor-firmware download, {@code dciReadData}/{@code dciWriteData},
 *       {@code setResolution}, {@code startRanging}, power-mode transitions,
 *       xtalk calibration. Those depend on a live SPI bridge and cannot be
 *       replayed deterministically; see {@link #liveDriverStubbed()}.
 * </ul>
 */
public final class Vl53l8Uld {
    private Vl53l8Uld() {}

    public static final int RESOLUTION_4X4 = 16;
    public static final int RESOLUTION_8X8 = 64;
    public static final int NB_TARGET_PER_ZONE = 1;

    /** Footer-id offset from frame end: cx (ULD 2.1.0) = 12, ch (2.0.16) = 4. */
    public static final int FOOTER_ID_OFF_CX = 12;
    public static final int FOOTER_ID_OFF_CH = 4;

    /**
     * Which VL53L8 part produced a frame. The frame decode is shared; the only
     * per-variant difference is the footer-id offset. CX is the dev-default base
     * part; CH adds CNH histograms and its own production USB PID (0xED40).
     */
    public enum Variant {
        CX(FOOTER_ID_OFF_CX),
        CH(FOOTER_ID_OFF_CH);

        /** Footer-id offset from the frame end for this variant. */
        public final int footerIdOff;

        Variant(int footerIdOff) {
            this.footerIdOff = footerIdOff;
        }
    }

    public static final int STATUS_INVALID_PARAM = 127;
    public static final int STATUS_CORRUPTED_FRAME = 2;

    // Block indices (see uld.ts).
    private static final int METADATA_IDX = 0x54b4;
    private static final int SPAD_COUNT_IDX = 0x55d0;
    private static final int AMBIENT_RATE_IDX = 0x54d0;
    private static final int NB_TARGET_DETECTED_IDX = 0xdb84;
    private static final int SIGNAL_RATE_IDX = 0xdbc4;
    private static final int RANGE_SIGMA_MM_IDX = 0xdec4;
    private static final int DISTANCE_IDX = 0xdf44;
    private static final int REFLECTANCE_EST_PC_IDX = 0xe044;
    private static final int TARGET_STATUS_IDX = 0xe084;

    // Detection-threshold measurement selectors + scale factors (plugin source).
    public static final int DIST_MM = 1;
    public static final int SIGNAL_PER_SPAD_KCPS = 2;
    public static final int RANGE_SIGMA_MM = 4;
    public static final int AMBIENT_PER_SPAD_KCPS = 8;
    public static final int NB_TARGET_DETECTED = 9;
    public static final int TAR_STATUS = 12;
    public static final int NB_SPADS_ENABLED = 13;
    public static final int MOTION_INDICATOR = 19;
    public static final int NB_THRESHOLDS = 64;
    public static final int LAST_THRESHOLD = 128;

    // Power modes (vl53l8cx_api.h).
    public static final int POWER_MODE_SLEEP = 0;
    public static final int POWER_MODE_WAKEUP = 1;
    public static final int POWER_MODE_DEEP_SLEEP = 2;

    private static int threshScale(int measurement) {
        return switch (measurement) {
            case DIST_MM -> 4;
            case SIGNAL_PER_SPAD_KCPS -> 2048;
            case RANGE_SIGMA_MM -> 128;
            case AMBIENT_PER_SPAD_KCPS -> 2048;
            case NB_SPADS_ENABLED -> 256;
            case MOTION_INDICATOR -> 65535;
            default -> 1;
        };
    }

    /**
     * The live ULD init/config register-bridge driver is intentionally not
     * ported to Java — it is hardware-dependent and out of the decode scope.
     * Applies to both VL53L8CX and VL53L8CH.
     */
    public static boolean liveDriverStubbed() {
        return true;
    }

    // ── CNH (Compact Network Histogram) decode ───────────────────────────────
    // VL53L8CH-specific. Faithful port of vl53lmz_plugin_cnh.c
    // (vl53lmz_cnh_get_block_addresses / _cnh_get_mem_block_addresses) and of the
    // Python reference depz_sensor_sdk/vl53l8/cnh.py, for the fixed cnh_cfg used by
    // Example_12_cnh_data.c (DISABLE_PING_PONG | DISABLE_VARIANCE | ambient | xtalk
    // | zero-invalid | store-ref-residual). Do not "simplify" the offset math.

    /** persistent-data header layout (plugin_cnh.c). */
    private static final int CNH_PER_HEADER_WORDS = 5;          // CNH_PER_HEADER_BYTES / 4
    private static final int CNH_PER_BUFFER_HEADER_WORDS = 2;   // CNH_PER_BUFFER_HEADER_BYTES / 4
    private static final int CNH_PER_HEADER_BUFFER_INFO_IDX = 1;
    private static final int CNH_PER_HEADER_FLAGS_IDX = 3;
    private static final int BUFFER_INFO_WORDS_MASK = 0xFFFF;
    private static final int BUFFER_INFO_FLAGS_SHIFT = 24;
    private static final int MI_STATE_PING = 0;

    /** One decoded CNH aggregate: raw histogram ints + per-bin int8 scalers. */
    public static final class CnhAggregate {
        /** Signed int32 histogram value per CNH bin (len == featureLength). */
        public final int[] histRaw;
        /** int8 scaler per CNH bin (len == featureLength); value = histRaw / 2^scaler. */
        public final int[] histScaler;

        public CnhAggregate(int[] histRaw, int[] histScaler) {
            this.histRaw = histRaw;
            this.histScaler = histScaler;
        }
    }

    /** Decoded CNH data block: reference residual word + per-aggregate histograms. */
    public static final class CnhResult {
        /** Raw uint32 ref-residual word (words[2]); float residual = value / 2048. */
        public final long refResidualWord;
        /** Per-aggregate histograms (len == nbOfAggregates). */
        public final CnhAggregate[] aggregates;

        public CnhResult(long refResidualWord, CnhAggregate[] aggregates) {
            this.refResidualWord = refResidualWord;
            this.aggregates = aggregates;
        }
    }

    /**
     * Decode a captured CNH data block ({@code raw}, byte-swapped exactly like the
     * standard ranging blocks) into per-aggregate histograms, for the fixed
     * cnh_cfg (ping-pong + variance disabled). Faithful port of the ST CNH plugin
     * / Python {@code cnh.decode}. Replaces the former {@code cnhHistogramDecodeStubbed}.
     *
     * @param nbOfAggregates number of CNH aggregates (from the CNH config)
     * @param featureLength  CNH bins per aggregate (from the CNH config)
     * @param raw            captured CNH block bytes
     */
    public static CnhResult decodeCnh(int nbOfAggregates, int featureLength, byte[] raw) {
        // ref_residual_word = uint32 at word offset 2 (byte offset 8).
        long refResidualWord = u32le(raw, 2 * 4);
        CnhAggregate[] aggregates = new CnhAggregate[nbOfAggregates];
        for (int aggId = 0; aggId < nbOfAggregates; aggId++) {
            aggregates[aggId] = decodeCnhAggregate(raw, nbOfAggregates, featureLength, aggId);
        }
        return new CnhResult(refResidualWord, aggregates);
    }

    private static CnhAggregate decodeCnhAggregate(byte[] raw, int nbAgg, int feat, int aggId) {
        int aggXFeat = nbAgg * feat;
        int aggOff = aggId * feat;

        int state = i32le(raw, 0 * 4);                                  // words[0]
        long info = u32le(raw, CNH_PER_HEADER_BUFFER_INFO_IDX * 4);     // words[1] unsigned
        int ppSize = (int) (info & BUFFER_INFO_WORDS_MASK);
        int flagsWord = i32le(raw, CNH_PER_HEADER_FLAGS_IDX * 4);       // words[3]

        // Select ping or pong buffer exactly as the C code does. With ping-pong
        // disabled the device reports a single buffer and this resolves to ping.
        int localPp = 1;
        if ((flagsWord & 0x10) == 0x10) {
            localPp = 1;
        }
        if (state == MI_STATE_PING) {
            localPp = 1 - localPp;
        }

        int base = CNH_PER_HEADER_WORDS;
        if (localPp == 1) {
            base += ppSize;
        }
        // buffer header is 2 words (state, nb_accumulated); data starts after it.
        int blk = (base + CNH_PER_BUFFER_HEADER_WORDS) * 4;            // byte offset of p[2]

        // FEAT_INT: int32 per (agg, feat).
        int[] featInt = new int[feat];
        for (int k = 0; k < feat; k++) {
            featInt[k] = i32le(raw, blk + (aggOff + k) * 4);
        }
        blk += aggXFeat * 4;
        // FEAT_FRAC: int8 scaler per (agg, feat), padded to a word boundary.
        int[] featScaler = new int[feat];
        for (int k = 0; k < feat; k++) {
            featScaler[k] = raw[blk + aggOff + k];                     // signed byte
        }
        blk += ((3 + aggXFeat) / 4) * 4;
        // AMBIENT_INT (int32 per agg) then AMBIENT_FRAC (int8 per agg) follow; the
        // decode does not surface ambient, but the offsets are part of the layout.
        return new CnhAggregate(featInt, featScaler);
    }

    // ── results-frame decoder ────────────────────────────────────────────────

    /** Parsed raw results frame (raw-integer per-zone arrays). */
    public static final class Results {
        public int[] distanceMm = new int[RESOLUTION_8X8 * NB_TARGET_PER_ZONE];
        public int[] targetStatus = new int[RESOLUTION_8X8 * NB_TARGET_PER_ZONE];
        public int[] nbTargetDetected = new int[RESOLUTION_8X8];
        public long[] signalPerSpad = new long[RESOLUTION_8X8 * NB_TARGET_PER_ZONE];
        public long[] ambientPerSpad = new long[RESOLUTION_8X8];
        public long[] nbSpadsEnabled = new long[RESOLUTION_8X8];
        public double[] rangeSigmaMm = new double[RESOLUTION_8X8 * NB_TARGET_PER_ZONE];
        public int[] reflectance = new int[RESOLUTION_8X8 * NB_TARGET_PER_ZONE];
        public int siliconTempDegc = 0;
        /**
         * Raw CNH (compact-histogram) block bytes, VL53L8CH only; {@code null}
         * when absent (always on CX). Decode with {@link #decodeCnh}.
         */
        public byte[] cnhRaw = null;

        /** Zones actually present this frame (16 for 4x4, 64 for 8x8). */
        public int resolution() {
            return nbTargetDetected.length;
        }
    }

    /** VL53L8CX_SwapBuffer: byte-reverse every 32-bit word (tail untouched). */
    public static byte[] swapBuffer(byte[] data) {
        byte[] out = data.clone();
        int n4 = (data.length >>> 2) << 2;
        for (int i = 0; i < n4; i += 4) {
            out[i] = data[i + 3];
            out[i + 1] = data[i + 2];
            out[i + 2] = data[i + 1];
            out[i + 3] = data[i];
        }
        return out;
    }

    /** union Block_header: type[3:0], size[15:4], idx[31:16]. */
    public static int[] bhFields(long bh) {
        int type = (int) (bh & 0xf);
        int size = (int) ((bh >>> 4) & 0xfff);
        int idx = (int) ((bh >>> 16) & 0xffff);
        return new int[] {type, size, idx};
    }

    /**
     * Parse one raw results frame ({@code dataReadSize} bytes) for the given
     * variant (CX or CH). Convenience overload of
     * {@link #parseFrame(byte[], int, int)} that picks the variant-specific
     * footer-id offset. The decode body is shared across CX and CH.
     */
    public static Results parseFrame(byte[] raw, int dataReadSize, Variant variant) {
        return parseFrame(raw, dataReadSize, variant.footerIdOff);
    }

    /**
     * Parse one raw results frame ({@code dataReadSize} bytes). {@code footerIdOff}
     * is variant-specific (cx = 12, ch = 4); prefer the
     * {@link #parseFrame(byte[], int, Variant)} overload. Shared by VL53L8CX and
     * VL53L8CH. Throws on a header/footer id mismatch.
     */
    public static Results parseFrame(byte[] raw, int dataReadSize, int footerIdOff) {
        byte[] buf = swapBuffer(raw);
        Results r = new Results();

        int i = 16;
        while (i + 4 <= dataReadSize) {
            long bh = u32le(buf, i);
            int[] f = bhFields(bh);
            int bhType = f[0];
            int bhSize = f[1];
            int bhIdx = f[2];
            int msize = (bhType > 0x1 && bhType < 0xd) ? bhType * bhSize : bhSize;
            // Exact-sized buffer: stop once a block would run past the end (the
            // footer follows all data blocks). Mirrors the C oversized-buffer walk.
            if (i + 4 + msize > dataReadSize) {
                break;
            }
            if (bhIdx == METADATA_IDX) {
                r.siliconTempDegc = buf[i + 12]; // signed byte
            } else if (bhIdx == DISTANCE_IDX) {
                int[] out = new int[msize / 2];
                for (int k = 0; k < out.length; k++) {
                    out[k] = i16le(buf, i + 4 + 2 * k);
                }
                r.distanceMm = out;
            } else if (bhIdx == TARGET_STATUS_IDX) {
                r.targetStatus = u8slice(buf, i + 4, msize);
            } else if (bhIdx == NB_TARGET_DETECTED_IDX) {
                r.nbTargetDetected = u8slice(buf, i + 4, msize);
            } else if (bhIdx == SIGNAL_RATE_IDX) {
                r.signalPerSpad = u32slice(buf, i + 4, msize);
            } else if (bhIdx == AMBIENT_RATE_IDX) {
                r.ambientPerSpad = u32slice(buf, i + 4, msize);
            } else if (bhIdx == SPAD_COUNT_IDX) {
                r.nbSpadsEnabled = u32slice(buf, i + 4, msize);
            } else if (bhIdx == RANGE_SIGMA_MM_IDX) {
                double[] out = new double[msize / 2];
                for (int k = 0; k < out.length; k++) {
                    out[k] = u16le(buf, i + 4 + 2 * k);
                }
                r.rangeSigmaMm = out;
            } else if (bhIdx == REFLECTANCE_EST_PC_IDX) {
                r.reflectance = u8slice(buf, i + 4, msize);
            }
            i += msize + 4;
        }

        // Fixed-point scaling (per ST GetRangingData). Floor division for signed mm.
        for (int k = 0; k < r.distanceMm.length; k++) {
            r.distanceMm[k] = Math.floorDiv(r.distanceMm[k], 4);
        }
        for (int k = 0; k < r.rangeSigmaMm.length; k++) {
            r.rangeSigmaMm[k] = r.rangeSigmaMm[k] / 128.0;
        }

        // No target detected -> status 255.
        int nzones = r.nbTargetDetected.length;
        for (int z = 0; z < nzones; z++) {
            if (r.nbTargetDetected[z] == 0) {
                for (int t = 0; t < NB_TARGET_PER_ZONE; t++) {
                    int idx = NB_TARGET_PER_ZONE * z + t;
                    if (idx < r.targetStatus.length) {
                        r.targetStatus[idx] = 255;
                    }
                }
            }
        }

        // Header/footer id match check.
        int foff = footerIdOff;
        if ((buf[0x8] & 0xFF) != (buf[dataReadSize - foff] & 0xFF)
                || (buf[0x9] & 0xFF) != (buf[dataReadSize - foff + 1] & 0xFF)) {
            throw new Vl53l8Error(STATUS_CORRUPTED_FRAME, "parseFrame");
        }
        return r;
    }

    // ── advanced-feature DCI codecs (pure) ───────────────────────────────────

    /** Xtalk margin (kcps/spad) -> raw DCI value (round(kcps * 2048)). */
    public static long xtalkMarginToRaw(double marginKcps) {
        return (long) Math.round(marginKcps * 2048) & 0xFFFFFFFFL;
    }

    /** One detection-threshold entry (real units). */
    public record DetectionThreshold(
            int lowThresh, int highThresh, int measurement, int type, int zoneNum, int operation) {}

    /** {@code DCI_DET_THRESH_START} payload (64x12 B) + 8-B valid-status block. */
    public record PackedThresholds(byte[] start, byte[] valid) {}

    /**
     * Pack up to 64 detection thresholds into the DCI_DET_THRESH_START payload
     * plus the 8-byte valid-status block. Each threshold's low/high are scaled
     * by its measurement selector. Mirror of set_detection_thresholds.
     */
    public static PackedThresholds packDetectionThresholds(List<DetectionThreshold> thresholds) {
        byte[] valid = new byte[8];
        java.util.Arrays.fill(valid, (byte) 0x05);
        byte[] start = new byte[NB_THRESHOLDS * 12];
        for (int k = 0; k < NB_THRESHOLDS; k++) {
            DetectionThreshold t = k < thresholds.size() ? thresholds.get(k) : null;
            int meas = t == null ? 0 : t.measurement();
            int scale = threshScale(meas);
            int low = (t == null ? 0 : t.lowThresh()) * scale;
            int high = (t == null ? 0 : t.highThresh()) * scale;
            int off = k * 12;
            putI32le(start, off, low);
            putI32le(start, off + 4, high);
            start[off + 8] = (byte) meas;
            start[off + 9] = (byte) (t == null ? 0 : t.type());
            start[off + 10] = (byte) (t == null ? 0 : t.zoneNum());
            start[off + 11] = (byte) (t == null ? 0 : t.operation());
        }
        return new PackedThresholds(start, valid);
    }

    /** Mirror of VL53L8CX_Motion_Configuration (156 bytes, {@code <i3I12B64b32B32B}). */
    public static final class MotionConfig {
        public int refBinOffset = 0;
        public long detectionThreshold = 0;
        public long extraNoiseSigma = 0;
        public long nullDenClipValue = 0;
        public int memUpdateMode = 0;
        public int memUpdateChoice = 0;
        public int sumSpan = 0;
        public int featureLength = 0;
        public int nbOfAggregates = 0;
        public int nbOfTemporalAccumulations = 0;
        public int minNbForGlobalDetection = 0;
        public int globalIndicatorFormat1 = 0;
        public int globalIndicatorFormat2 = 0;
        public int spare1 = 0;
        public int spare2 = 0;
        public int spare3 = 0;
        public final int[] mapId = new int[64];
        public final int[] indicatorFormat1 = new int[32];
        public final int[] indicatorFormat2 = new int[32];

        public byte[] pack() {
            byte[] out = new byte[156];
            int o = 0;
            putI32le(out, o, refBinOffset);
            o += 4;
            putU32le(out, o, detectionThreshold);
            o += 4;
            putU32le(out, o, extraNoiseSigma);
            o += 4;
            putU32le(out, o, nullDenClipValue);
            o += 4;
            int[] bytes12 = {
                memUpdateMode, memUpdateChoice, sumSpan, featureLength,
                nbOfAggregates, nbOfTemporalAccumulations, minNbForGlobalDetection,
                globalIndicatorFormat1, globalIndicatorFormat2, spare1, spare2, spare3
            };
            for (int b : bytes12) {
                out[o++] = (byte) b;
            }
            for (int k = 0; k < 64; k++) {
                out[o++] = (byte) mapId[k]; // signed
            }
            for (int k = 0; k < 32; k++) {
                out[o++] = (byte) indicatorFormat1[k];
            }
            for (int k = 0; k < 32; k++) {
                out[o++] = (byte) indicatorFormat2[k];
            }
            return out;
        }
    }

    /** Set MotionConfig.mapId for the given resolution (pure — no I/O). */
    public static void motionConfigSetResolution(MotionConfig cfg, int resolution) {
        if (resolution == RESOLUTION_4X4) {
            for (int i = 0; i < 16; i++) {
                cfg.mapId[i] = i;
            }
            for (int i = 16; i < 64; i++) {
                cfg.mapId[i] = -1;
            }
        } else if (resolution == RESOLUTION_8X8) {
            for (int i = 0; i < 64; i++) {
                cfg.mapId[i] = ((i % 8) >> 1) + 4 * (i / 16);
            }
        } else {
            throw new Vl53l8Error(STATUS_INVALID_PARAM, "motion set_resolution");
        }
    }

    /** Default motion-indicator configuration for a resolution (pure). */
    public static MotionConfig defaultMotionConfig(int resolution) {
        MotionConfig cfg = new MotionConfig();
        cfg.refBinOffset = 13633;
        cfg.detectionThreshold = 2883584;
        cfg.extraNoiseSigma = 0;
        cfg.nullDenClipValue = 0;
        cfg.memUpdateMode = 6;
        cfg.memUpdateChoice = 2;
        cfg.sumSpan = 4;
        cfg.featureLength = 9;
        cfg.nbOfAggregates = 16;
        cfg.nbOfTemporalAccumulations = 16;
        cfg.minNbForGlobalDetection = 1;
        cfg.globalIndicatorFormat1 = 8;
        cfg.globalIndicatorFormat2 = 0;
        motionConfigSetResolution(cfg, resolution);
        return cfg;
    }

    /** ULD status error. */
    public static final class Vl53l8Error extends RuntimeException {
        public final int code;

        public Vl53l8Error(int code, String where) {
            super("VL53L8CX code " + code + " " + where);
            this.code = code;
        }
    }

    // ── LE helpers ───────────────────────────────────────────────────────────

    private static long u32le(byte[] b, int off) {
        return (b[off] & 0xFFL)
                | ((b[off + 1] & 0xFFL) << 8)
                | ((b[off + 2] & 0xFFL) << 16)
                | ((b[off + 3] & 0xFFL) << 24);
    }

    private static int i32le(byte[] b, int off) {
        return (b[off] & 0xFF)
                | ((b[off + 1] & 0xFF) << 8)
                | ((b[off + 2] & 0xFF) << 16)
                | ((b[off + 3] & 0xFF) << 24);
    }

    private static int u16le(byte[] b, int off) {
        return (b[off] & 0xFF) | ((b[off + 1] & 0xFF) << 8);
    }

    private static int i16le(byte[] b, int off) {
        return (short) ((b[off] & 0xFF) | ((b[off + 1] & 0xFF) << 8));
    }

    private static int[] u8slice(byte[] b, int off, int len) {
        int[] out = new int[len];
        for (int k = 0; k < len; k++) {
            out[k] = b[off + k] & 0xFF;
        }
        return out;
    }

    private static long[] u32slice(byte[] b, int off, int msize) {
        long[] out = new long[msize / 4];
        for (int k = 0; k < out.length; k++) {
            out[k] = u32le(b, off + 4 * k);
        }
        return out;
    }

    private static void putI32le(byte[] b, int off, int v) {
        b[off] = (byte) v;
        b[off + 1] = (byte) (v >>> 8);
        b[off + 2] = (byte) (v >>> 16);
        b[off + 3] = (byte) (v >>> 24);
    }

    private static void putU32le(byte[] b, int off, long v) {
        b[off] = (byte) v;
        b[off + 1] = (byte) (v >>> 8);
        b[off + 2] = (byte) (v >>> 16);
        b[off + 3] = (byte) (v >>> 24);
    }
}
