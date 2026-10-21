package ai.depz.sensor.sensors.tmf8829;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

/**
 * TMF8829 frame layout and pixel decoding (contracts/14_SENSOR_TMF8829.md §3).
 *
 * <p>A frame is the sensor bytes from register 0xFA on: 5-byte pre-header
 * (FIFOSTATUS, SYSTICK), 16-byte header, data (pixel results or histograms),
 * 12-byte footer. Mirrors {@code depz_sensor_sdk/tmf8829/frames.py}.
 */
public final class Frames {
    private Frames() {}

    public static final int PRE_HEADER_SIZE = 5;
    public static final int HEADER_SIZE = 16;
    public static final int FOOTER_SIZE = 12;
    /** 21 B the bridge reads @0xFA. */
    public static final int FRAME_HEAD = PRE_HEADER_SIZE + HEADER_SIZE;
    public static final int FRAME_EOF = 0xE0F7;

    public static final int FID_MASK = 0xF0;
    public static final int FID_RESULTS = 0x10;
    public static final int FID_HISTOGRAMS = 0x20;
    public static final int FID_REF_SPAD = 0x30;

    public static final int LAYOUT_PEAKS = 0x07;
    public static final int LAYOUT_SIGNAL = 0x08;
    public static final int LAYOUT_NOISE = 0x10;
    public static final int LAYOUT_XTALK = 0x20;
    /** 32x32/48x32: set on the second (odd-row) half-frame. */
    public static final int LAYOUT_SUBFRAME = 0x40;

    public static final int FRAME_VALID = 0x01;
    public static final int FRAME_ABORTED = 0xC0;
    /** frame_status warning bits, in report order. */
    public static final int[] FRAME_WARNING_BITS = {0x08, 0x10, 0x20};
    public static final String[] FRAME_WARNING_NAMES = {
        "HV_CP_OVERLOAD", "VCDRV_OVERLOAD", "VCDRV_BURST_EXCEEDED"};
    /** footer.reserved b0: distances in mm, else 0.25 mm. */
    public static final int FOOTER_DIST_IN_MM = 0x01;

    public static final int FIFO_TX_UNDERRUN = 0x10;
    public static final int FIFO_RX_OVERRUN = 0x20;

    /** fp_mode → {rows, cols} of the full focal plane (index = fp_mode 0..5). */
    public static final int[][] FP_GRID = {{8, 8}, {8, 8}, {16, 16}, {32, 32}, {32, 32}, {32, 48}};
    public static final String[] FP_NAMES = {"8x8A", "8x8B", "16x16", "32x32", "32x32s", "48x32"};

    /** True when {@code fpMode} is one of the six known focal-plane modes. */
    public static boolean knownFpMode(int fpMode) {
        return fpMode >= 0 && fpMode < FP_GRID.length;
    }

    /** A frame whose bytes do not match its own header. */
    public static final class FrameError extends IllegalArgumentException {
        public FrameError(String message) { super(message); }
    }

    /**
     * Pre-header, header and footer of one frame (any kind). {@code temperature}
     * is three signed bytes, {@code refPos} two u16; u32 fields are {@code long}.
     */
    public record FrameInfo(
            int fifoStatus, long systick, int id, int layout, int payload, long fNumber,
            int[] temperature, int bdv, int[] refPos,
            long t0Integration, long t1Integration, int frameStatus, int reserved, int eof, int size) {

        /** FID_RESULTS, FID_HISTOGRAMS or FID_REF_SPAD. */
        public int kind() { return id & FID_MASK; }

        public int fpMode() { return id & 0x0F; }

        public boolean isSecondHalf() { return (layout & LAYOUT_SUBFRAME) != 0; }

        public boolean distanceInMm() { return (reserved & FOOTER_DIST_IN_MM) != 0; }

        /** What is wrong with the frame, human-readable; empty when fine. */
        public List<String> problems() {
            List<String> out = new ArrayList<>();
            if (eof != FRAME_EOF) {
                out.add(String.format("EOF 0x%04X", eof));
            }
            if ((frameStatus & FRAME_VALID) == 0) {
                out.add(String.format("status 0x%02X not valid", frameStatus));
            }
            if ((frameStatus & FRAME_ABORTED) != 0) {
                out.add("aborted");
            }
            for (int i = 0; i < FRAME_WARNING_BITS.length; i++) {
                if ((frameStatus & FRAME_WARNING_BITS[i]) != 0) {
                    out.add(FRAME_WARNING_NAMES[i]);
                }
            }
            if ((fifoStatus & FIFO_TX_UNDERRUN) != 0) {
                out.add("txfifo_underrun");
            }
            if ((fifoStatus & FIFO_RX_OVERRUN) != 0) {
                out.add("rxfifo_overrun");
            }
            return out;
        }
    }

    /** Full frame size (pre-header included) from its first 21 bytes. */
    public static int frameSize(byte[] head) {
        return PRE_HEADER_SIZE + 4 + u16(head, 7);
    }

    public static FrameInfo parseFrame(byte[] raw) {
        if (raw.length < FRAME_HEAD + FOOTER_SIZE) {
            throw new FrameError("frame of " + raw.length + " B is shorter than header + footer");
        }
        int h = PRE_HEADER_SIZE;
        int f = raw.length - FOOTER_SIZE;
        return new FrameInfo(
            raw[0] & 0xFF, u32(raw, 1),
            raw[h] & 0xFF, raw[h + 1] & 0xFF, u16(raw, h + 2), u32(raw, h + 4),
            new int[] {raw[h + 8], raw[h + 9], raw[h + 10]},
            raw[h + 11] & 0xFF,
            new int[] {u16(raw, h + 12), u16(raw, h + 14)},
            u32(raw, f), u32(raw, f + 4), raw[f + 8] & 0xFF, raw[f + 9] & 0xFF, u16(raw, f + 10),
            raw.length);
    }

    /**
     * Decoded results of one frame or of a merged measurement, flat row-major
     * arrays. {@code distance}, {@code snr}, {@code signal} are
     * {@code [peaks][rows][cols]} (index {@code (p * rows + r) * cols + c});
     * {@code noise}, {@code xtalk} {@code [rows][cols]}. Raw sensor integers:
     * distance in 0.25 mm (or mm with the footer flag), not yet clock-corrected.
     * Absent fields are {@code null}.
     */
    public record PixelResults(
            int peaks, int rows, int cols,
            int[] distance, int[] snr, int[] signal, int[] noise, int[] xtalk) {
        /** {rows, cols} of the arrays present; {0, 0} when the format carries none (as Python). */
        public int[] shape() {
            return distance != null || noise != null || xtalk != null
                ? new int[] {rows, cols} : new int[] {0, 0};
        }
    }

    /** Size in bytes of one results pixel for this layout. */
    public static int pixelSize(int layout) {
        int n = 0;
        if ((layout & LAYOUT_NOISE) != 0) n += 2;
        if ((layout & LAYOUT_XTALK) != 0) n += 2;
        int perPeak = 3 + ((layout & LAYOUT_SIGNAL) != 0 ? 2 : 0);
        return n + (layout & LAYOUT_PEAKS) * perPeak;
    }

    /** {rows, cols} one results frame carries: half the rows for 32x32/48x32. */
    public static int[] frameRows(int fpMode) {
        int rows = FP_GRID[fpMode][0];
        int cols = FP_GRID[fpMode][1];
        return rows == 32 ? new int[] {rows / 2, cols} : new int[] {rows, cols};
    }

    public static PixelResults parseResults(byte[] raw) {
        return parseResults(raw, parseFrame(raw));
    }

    /** Pixel results of one results frame, row-major. */
    public static PixelResults parseResults(byte[] raw, FrameInfo f) {
        if (f.kind() != FID_RESULTS) {
            throw new FrameError(String.format("frame id 0x%02X is not a results frame", f.id()));
        }
        if (!knownFpMode(f.fpMode())) {
            throw new FrameError("unknown fp_mode " + f.fpMode());
        }
        int[] rc = frameRows(f.fpMode());
        int rows = rc[0];
        int cols = rc[1];
        int layout = f.layout();
        int itemsize = pixelSize(layout);
        int n = rows * cols;
        int dataSize = f.size() - FRAME_HEAD - FOOTER_SIZE;
        if (itemsize * n != dataSize) {
            throw new FrameError(String.format(
                "result data %d B, expected %d px × %d B (layout 0x%02X)", dataSize, n, itemsize, layout));
        }
        int peaks = layout & LAYOUT_PEAKS;
        boolean hasSignal = (layout & LAYOUT_SIGNAL) != 0;
        int[] noise = (layout & LAYOUT_NOISE) != 0 ? new int[n] : null;
        int[] xtalk = (layout & LAYOUT_XTALK) != 0 ? new int[n] : null;
        int[] distance = peaks > 0 ? new int[peaks * n] : null;
        int[] snr = peaks > 0 ? new int[peaks * n] : null;
        int[] signal = peaks > 0 && hasSignal ? new int[peaks * n] : null;
        int off = FRAME_HEAD;
        for (int i = 0; i < n; i++) {
            if (noise != null) {
                noise[i] = u16(raw, off);
                off += 2;
            }
            if (xtalk != null) {
                xtalk[i] = u16(raw, off);
                off += 2;
            }
            for (int p = 0; p < peaks; p++) {
                distance[p * n + i] = u16(raw, off);
                snr[p * n + i] = raw[off + 2] & 0xFF;
                off += 3;
                if (signal != null) {
                    signal[p * n + i] = u16(raw, off);
                    off += 2;
                }
            }
        }
        return new PixelResults(peaks, rows, cols, distance, snr, signal, noise, xtalk);
    }

    /** 32x32/48x32: the first half-frame holds the even rows, the second the odd ones. */
    public static PixelResults mergeHalves(PixelResults first, PixelResults second) {
        int rows = first.rows();
        int cols = first.cols();
        return new PixelResults(
            first.peaks(), rows * 2, cols,
            merge(first.distance(), second.distance(), rows, cols),
            merge(first.snr(), second.snr(), rows, cols),
            merge(first.signal(), second.signal(), rows, cols),
            merge(first.noise(), second.noise(), rows, cols),
            merge(first.xtalk(), second.xtalk(), rows, cols));
    }

    private static int[] merge(int[] a, int[] b, int rows, int cols) {
        if (a == null || b == null) {
            return null;
        }
        int planes = a.length / (rows * cols);
        int[] m = new int[a.length * 2];
        for (int p = 0; p < planes; p++) {
            for (int r = 0; r < rows; r++) {
                int src = (p * rows + r) * cols;
                System.arraycopy(a, src, m, (p * 2 * rows + 2 * r) * cols, cols);
                System.arraycopy(b, src, m, (p * 2 * rows + 2 * r + 1) * cols, cols);
            }
        }
        return m;
    }

    /**
     * A raw histogram frame (25 377 B); decoding the bins is left to the caller
     * (ams histogram layout).
     */
    public record HistogramFrame(FrameInfo info, byte[] raw) {
        public byte[] data() {
            return Arrays.copyOfRange(raw, FRAME_HEAD, raw.length - FOOTER_SIZE);
        }
    }

    // ── little-endian helpers ────────────────────────────────────────────────

    static int u16(byte[] b, int off) {
        return (b[off] & 0xFF) | ((b[off + 1] & 0xFF) << 8);
    }

    static long u32(byte[] b, int off) {
        return (b[off] & 0xFFL) | ((b[off + 1] & 0xFFL) << 8)
            | ((b[off + 2] & 0xFFL) << 16) | ((b[off + 3] & 0xFFL) << 24);
    }
}
