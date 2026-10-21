package ai.depz.sensor.protocol;

import java.util.Arrays;

/**
 * TMF8829 SPI register-bridge wire codecs, protocol v1.00
 * (contracts/14_SENSOR_TMF8829.md §2).
 *
 * <p>Same command numbering as the VL53 bridges (0x32/0x33/0x35/0x36/0x37) but
 * TMF8829 payloads: an 8-bit register address and self-sized frames, so the
 * host never tells the bridge a frame size. The MCU is a thin bridge; the host
 * runs the sensor ({@link ai.depz.sensor.sensors.tmf8829}). Mirrors
 * {@code depz_sensor_sdk/protocol/tmf8829.py} 1:1.
 */
public final class Tmf8829 {
    private Tmf8829() {}

    public enum Tmf8829Cmd {
        READ_REG(0x32),
        WRITE_REG(0x33),
        START_STREAM(0x35),
        STOP_STREAM(0x36),
        GET_INFO(0x37);

        public final int value;
        Tmf8829Cmd(int value) { this.value = value; }
    }

    public enum Tmf8829Rpt {
        REG_DATA(0x91),
        /** Carries no echoed command byte: a host matching replies by {@code payload[0] == cmd} must exempt it. */
        INFO(0x92),
        FRAME(0x93),
        EVENT(0x94);

        public final int value;
        Tmf8829Rpt(int value) { this.value = value; }
    }

    /** RPT_TMF_EVENT {@code code}. */
    public enum EventCode {
        /** MOTION/PROXIMITY interrupt, no frame; its bits already cleared. */
        INT(0),
        /** Frame header refused, frame not read (expected once on STOP mid histogram dump). */
        BAD_HEADER(1);

        public final int value;
        EventCode(int value) { this.value = value; }
    }

    /** TMF_READ_REG {@code len} 1..1024. */
    public static final int READ_MAX = 1024;
    /** TMF_WRITE_REG N 1..1023 (1024 B buffer minus the address). */
    public static final int WRITE_MAX = 1023;
    /** Frame bytes per RPT_TMF_FRAME. */
    public static final int CHUNK_MAX = 1024;

    // INT_STATUS bits the bridge serves (TMF_START_STREAM int_mask)
    public static final int INT_RESULTS = 0x01;
    public static final int INT_MOTION = 0x02;
    public static final int INT_PROXIMITY = 0x04;
    public static final int INT_HISTOGRAMS = 0x08;
    public static final int INT_MASK_ALL = 0x0F;

    /** TMF_START_STREAM flags: emit the ref-SPAD frame before each results frame. */
    public static final int SF_REF_FRAME = 0x01;

    public static final int REG_DATA_HEADER_SIZE = 9;
    public static final int FRAME_CHUNK_HEADER_SIZE = 21;
    public static final int EVENT_SIZE = 12;
    public static final int INFO_SIZE = 25;

    // ── command payload encoders ─────────────────────────────────────────────
    // TMF_STOP_STREAM and TMF_GET_INFO carry an empty payload.

    /** TMF_READ_REG payload: {@code addr u8, len u16 LE}; {@code len} 1..{@link #READ_MAX}. */
    public static byte[] packReadReg(int addr, int len) {
        if (addr < 0 || addr > 0xFF) {
            throw new IllegalArgumentException("addr must be 0..255");
        }
        if (len < 1 || len > READ_MAX) {
            throw new IllegalArgumentException("length must be 1.." + READ_MAX);
        }
        return new byte[] {(byte) addr, (byte) len, (byte) (len >>> 8)};
    }

    /** TMF_WRITE_REG payload: {@code addr u8, data[1..1023]}. */
    public static byte[] packWriteReg(int addr, byte[] data) {
        if (addr < 0 || addr > 0xFF) {
            throw new IllegalArgumentException("addr must be 0..255");
        }
        if (data.length < 1 || data.length > WRITE_MAX) {
            throw new IllegalArgumentException("data must be 1.." + WRITE_MAX + " bytes");
        }
        byte[] out = new byte[1 + data.length];
        out[0] = (byte) addr;
        System.arraycopy(data, 0, out, 1, data.length);
        return out;
    }

    /**
     * TMF_START_STREAM payload: {@code int_mask u8, flags u8}. {@code intMask} a
     * non-zero subset of 0x0F; {@code flags} only {@link #SF_REF_FRAME}.
     */
    public static byte[] packStartStream(int intMask, int flags) {
        if (intMask < 1 || intMask > INT_MASK_ALL) {
            throw new IllegalArgumentException("int_mask must be a non-zero subset of 0x0F");
        }
        if ((flags & ~SF_REF_FRAME) != 0) {
            throw new IllegalArgumentException("flags: only SF_REF_FRAME (0x01) is defined");
        }
        return new byte[] {(byte) intMask, (byte) flags};
    }

    // ── report decoders ──────────────────────────────────────────────────────

    /** RPT_TMF_REG_DATA: register bytes and the MCU time the SPI read completed. */
    public record RegData(long timestampUs, byte[] data) {
        public static RegData unpack(byte[] p) {
            if (p.length < REG_DATA_HEADER_SIZE) {
                throw new IllegalArgumentException(
                    "RPT_TMF_REG_DATA: need >=" + REG_DATA_HEADER_SIZE + " bytes, got " + p.length);
            }
            int cmd = p[0] & 0xFF;
            if (cmd != Tmf8829Cmd.READ_REG.value) {
                throw new IllegalArgumentException(
                    String.format("RPT_TMF_REG_DATA: echoed cmd 0x%02X, expected 0x32", cmd));
            }
            return new RegData(Common.u64le(p, 1), Arrays.copyOfRange(p, REG_DATA_HEADER_SIZE, p.length));
        }
    }

    /**
     * RPT_TMF_INFO (25 B {@code <IIIIIBBBBB}): bridge state only, the sensor is
     * never touched. Carries no echoed command byte. {@code intEdges} runs from
     * MCU power-up; the frame counters restart at TMF_START_STREAM.
     * {@code intLevel} is the PA4 level now, 0 = asserted.
     */
    public record Tmf8829Info(
            long intEdges, long frames, long framesAborted, long frameErrors, long spiTimeouts,
            int intLevel, boolean streaming, int intMask, int lastIntStatus, int lastFifoStatus) {
        public static Tmf8829Info unpack(byte[] p) {
            if (p.length < INFO_SIZE) {
                throw new IllegalArgumentException(
                    "RPT_TMF_INFO: expected " + INFO_SIZE + " bytes, got " + p.length);
            }
            return new Tmf8829Info(
                u32le(p, 0), u32le(p, 4), u32le(p, 8), u32le(p, 12), u32le(p, 16),
                p[20] & 0xFF, p[21] != 0, p[22] & 0xFF, p[23] & 0xFF, p[24] & 0xFF);
        }
    }

    /**
     * One RPT_TMF_FRAME chunk ({@code <QHHBQ} + data). {@code timestampUs} plus
     * the frame's pre-header SYSTICK are a clock pair (not the capture time);
     * {@code edgeUs} is the INT falling edge credited to the frame, 0 = none.
     */
    public record FrameChunk(long timestampUs, int fullSize, int offset, int frameSeq, long edgeUs, byte[] data) {
        public static FrameChunk unpack(byte[] p) {
            if (p.length < FRAME_CHUNK_HEADER_SIZE) {
                throw new IllegalArgumentException(
                    "RPT_TMF_FRAME: need >=" + FRAME_CHUNK_HEADER_SIZE + " bytes, got " + p.length);
            }
            return new FrameChunk(
                Common.u64le(p, 0), u16le(p, 8), u16le(p, 10), p[12] & 0xFF, Common.u64le(p, 13),
                Arrays.copyOfRange(p, FRAME_CHUNK_HEADER_SIZE, p.length));
        }
    }

    /**
     * RPT_TMF_EVENT ({@code <QBBBB}). {@code intStatus} is INT_STATUS as read
     * (unmasked); {@code frameId} / {@code fifoStatus} are the header id byte and
     * the pre-header byte for BAD_HEADER, else 0.
     */
    public record Tmf8829Event(long timestampUs, int intStatus, int code, int frameId, int fifoStatus) {
        public static Tmf8829Event unpack(byte[] p) {
            if (p.length < EVENT_SIZE) {
                throw new IllegalArgumentException(
                    "RPT_TMF_EVENT: expected " + EVENT_SIZE + " bytes, got " + p.length);
            }
            return new Tmf8829Event(Common.u64le(p, 0), p[8] & 0xFF, p[9] & 0xFF, p[10] & 0xFF, p[11] & 0xFF);
        }
    }

    /** A reassembled frame: the sensor bytes from register 0xFA on. */
    public record RawFrame(long timestampUs, long edgeUs, int frameSeq, byte[] data) {}

    /**
     * Rebuilds frames from RPT_TMF_FRAME chunks (contract 14 §2): reset on
     * offset 0; append only a chunk of the same frame_seq at the expected
     * offset; complete at full_size. Never stitches chunks of different
     * frame_seq. {@code seqGaps} counts frames the bridge dropped (frame_seq jumps).
     */
    public static final class FrameReassembler {
        public int completed;
        public int discarded;
        public int seqGaps;

        private byte[] buf;
        private int held;
        private FrameChunk head;
        private int nextSeq;
        private boolean haveNextSeq;

        public FrameReassembler() {
            reset();
        }

        /** Forget everything — call at every stream start (frame_seq restarts at 0). */
        public void reset() {
            clearBuffer();
            haveNextSeq = false;
            nextSeq = 0;
            completed = 0;
            discarded = 0;
            seqGaps = 0;
        }

        private void clearBuffer() {
            buf = new byte[0];
            held = 0;
            head = null;
        }

        private void append(byte[] d) {
            if (held + d.length > buf.length) {
                buf = Arrays.copyOf(buf, Math.max(held + d.length, buf.length * 2));
            }
            System.arraycopy(d, 0, buf, held, d.length);
            held += d.length;
        }

        /** One chunk → the frame it completes, or {@code null}. */
        public RawFrame feed(FrameChunk chunk) {
            if (chunk.offset() == 0) {
                if (head != null) {
                    discarded += 1; // the previous frame never completed
                }
                if (haveNextSeq && chunk.frameSeq() != nextSeq) {
                    seqGaps += (chunk.frameSeq() - nextSeq) & 0xFF;
                }
                nextSeq = (chunk.frameSeq() + 1) & 0xFF;
                haveNextSeq = true;
                buf = new byte[Math.max(chunk.fullSize(), chunk.data().length)];
                held = 0;
                head = chunk;
                append(chunk.data());
            } else if (head != null
                    && chunk.frameSeq() == head.frameSeq()
                    && chunk.fullSize() == head.fullSize()
                    && chunk.offset() == held) {
                append(chunk.data());
            } else {
                if (head != null) {
                    discarded += 1;
                }
                clearBuffer();
                return null;
            }
            FrameChunk h = head;
            if (held == h.fullSize()) {
                RawFrame frame = new RawFrame(h.timestampUs(), h.edgeUs(), h.frameSeq(), Arrays.copyOf(buf, held));
                clearBuffer();
                completed += 1;
                return frame;
            }
            if (held > h.fullSize()) {
                discarded += 1;
                clearBuffer();
            }
            return null;
        }
    }

    // ── little-endian helpers ────────────────────────────────────────────────

    static int u16le(byte[] b, int off) {
        return (b[off] & 0xFF) | ((b[off + 1] & 0xFF) << 8);
    }

    static long u32le(byte[] b, int off) {
        return (b[off] & 0xFFL) | ((b[off + 1] & 0xFFL) << 8)
            | ((b[off + 2] & 0xFFL) << 16) | ((b[off + 3] & 0xFFL) << 24);
    }
}
