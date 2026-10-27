package ai.depz.sensor.sensors.vl53l8;

import java.util.Arrays;

/**
 * Rebuilds full VL53L8 sensor frames from chunked RPT_VL53_FRAME reports
 * (contracts/04_SENSOR_VL53L8.md). Shared by both VL53L8CX and VL53L8CH — the
 * chunk transport is identical across the two parts.
 *
 * Rules: reset on {@code offset == 0}; chunks must be contiguous — a gap
 * discards the frame in progress; a frame completes when the accumulated bytes
 * equal {@code fullSize}. A chunk out of the rule-4 bounds (more than
 * {@code chunkMax} bytes, {@code fullSize} over {@link #STREAM_TOTAL_MAX}, or
 * running past {@code fullSize}) is damaged: dropped and counted as a lost
 * frame like the other damaged input, never thrown. Mirror of the TS/Python
 * {@code FrameReassembler}.
 */
public final class FrameReassembler {
    /** Bytes of frame data per RPT_VL53_FRAME chunk. */
    public static final int STREAM_CHUNK_MAX = 1528;
    /** Largest frame_size START_STREAM accepts (contract 04 §4 rule 4). */
    public static final int STREAM_TOTAL_MAX = 8192;

    public int completed = 0;
    public int discarded = 0;

    private final int chunkMax;

    private byte[] buf = new byte[0];
    private int fullSize = 0;
    private long timestampUs = 0L;

    /** A reassembler for the VL53L8 board's {@link #STREAM_CHUNK_MAX} chunks. */
    public FrameReassembler() {
        this(STREAM_CHUNK_MAX);
    }

    /**
     * A reassembler for a board whose VL53_STREAM_FRAME_MAX is {@code chunkMax}
     * ({@code Vl53l7.STREAM_CHUNK_MAX} = 1536 on the I2C board, contract 11 §3).
     */
    public FrameReassembler(int chunkMax) {
        this.chunkMax = chunkMax;
    }

    /** One RPT_VL53_FRAME chunk (payload of a cmd-0x93 packet). */
    public record FrameChunk(long timestampUs, int fullSize, int offset, byte[] data) {}

    /** Result of a completed frame. */
    public record CompletedFrame(long timestampUs, byte[] frame) {}

    /** Decode a RPT_VL53_FRAME payload: ts u64 LE, fullSize u16, offset u16, data. */
    public static FrameChunk unpackFrameChunk(byte[] payload) {
        ai.depz.sensor.protocol.Wire.needBytes("RPT_VL53_FRAME", payload, 12);
        long ts = u64le(payload, 0);
        int fullSize = u16le(payload, 8);
        int offset = u16le(payload, 10);
        byte[] data = Arrays.copyOfRange(payload, 12, payload.length);
        return new FrameChunk(ts, fullSize, offset, data);
    }

    /** Returns a completed frame or {@code null}. */
    public CompletedFrame feed(FrameChunk chunk) {
        if (chunk.data().length > chunkMax
                || chunk.fullSize() > STREAM_TOTAL_MAX
                || chunk.offset() + chunk.data().length > chunk.fullSize()) {
            // rule 4: damaged — the frame in progress is lost (counted, rules
            // 1–2) and so is the frame an offset-0 chunk starts (counted); a
            // damaged orphan with nothing in progress is dropped like any orphan
            if (buf.length > 0) {
                discarded += 1;
            }
            if (chunk.offset() == 0) {
                discarded += 1;
            }
            buf = new byte[0];
            fullSize = 0;
            return null;
        }
        if (chunk.offset() == 0) {
            if (buf.length > 0 && buf.length != fullSize) {
                discarded += 1;
            }
            buf = chunk.data().clone();
            fullSize = chunk.fullSize();
            timestampUs = chunk.timestampUs();
        } else if (chunk.offset() == buf.length
                && fullSize == chunk.fullSize()
                && buf.length > 0) {
            byte[] next = Arrays.copyOf(buf, buf.length + chunk.data().length);
            System.arraycopy(chunk.data(), 0, next, buf.length, chunk.data().length);
            buf = next;
        } else {
            if (buf.length > 0) {
                discarded += 1;
            }
            buf = new byte[0];
            fullSize = 0;
            return null;
        }
        if (buf.length == fullSize && fullSize > 0) {
            CompletedFrame done = new CompletedFrame(timestampUs, buf);
            buf = new byte[0];
            fullSize = 0;
            completed += 1;
            return done;
        }
        return null;
    }

    private static long u64le(byte[] b, int off) {
        long v = 0;
        for (int i = 0; i < 8; i++) {
            v |= (b[off + i] & 0xFFL) << (8 * i);
        }
        return v;
    }

    private static int u16le(byte[] b, int off) {
        return (b[off] & 0xFF) | ((b[off + 1] & 0xFF) << 8);
    }
}
