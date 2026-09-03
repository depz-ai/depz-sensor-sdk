package ai.depz.sensor.sensors.bno086;

import java.io.ByteArrayOutputStream;
import java.util.ArrayList;
import java.util.List;

/**
 * SHTP framing layer for the BNO086 (contracts/05_SENSOR_BNO086.md §3).
 *
 * <p>Header: length u16 LE (bits 14:0 = cargo length incl. the 4-byte header;
 * bit 15 = continuation), channel u8, seq u8. A first fragment advertises the
 * TOTAL cargo length; continuations carry the remaining length with bit 15 set.
 * TX seq counters are per channel. Mirror of the TS/Python references.
 */
public final class Shtp {
    private Shtp() {}

    public static final int SHTP_HEADER_SIZE = 4;
    public static final int LENGTH_MASK = 0x7fff;
    public static final int CONTINUATION_BIT = 0x8000;
    public static final int NUM_CHANNELS = 6;
    /** Host->sensor frames must fit one MCU transmit slot (ERRATA E2). */
    public static final int MAX_TX_FRAME = 64;

    public record ShtpHeader(int length, int channel, int seq, boolean continuation) {}

    public static byte[] packShtpHeader(ShtpHeader hdr) {
        int word = (hdr.length() & LENGTH_MASK) | (hdr.continuation() ? CONTINUATION_BIT : 0);
        return new byte[] {
            (byte) word, (byte) (word >>> 8), (byte) hdr.channel(), (byte) hdr.seq()
        };
    }

    public static ShtpHeader unpackShtpHeader(byte[] data) {
        int word = (data[0] & 0xFF) | ((data[1] & 0xFF) << 8);
        return new ShtpHeader(
                word & LENGTH_MASK, data[2] & 0xFF, data[3] & 0xFF, (word & CONTINUATION_BIT) != 0);
    }

    /** One reassembled cargo: {@code payload} excludes all SHTP headers. */
    public record ShtpCargo(int channel, int seq, byte[] payload) {}

    /** Single-fragment frame: length = header + payload. */
    public static byte[] buildFrame(int channel, byte[] payload, int seq) {
        byte[] hdr =
                packShtpHeader(new ShtpHeader(SHTP_HEADER_SIZE + payload.length, channel, seq & 0xff, false));
        byte[] out = new byte[hdr.length + payload.length];
        System.arraycopy(hdr, 0, out, 0, hdr.length);
        System.arraycopy(payload, 0, out, hdr.length, payload.length);
        return out;
    }

    private static final class ChannelRx {
        final ByteArrayOutputStream chunks = new ByteArrayOutputStream();
        int received = 0;
        int expected = 0;
        int seq = 0;

        void clear() {
            chunks.reset();
            received = 0;
            expected = 0;
        }
    }

    /** Per-channel TX sequence counters + RX cargo reassembly. */
    public static final class ShtpLayer {
        /** Incomplete cargos thrown away. */
        public int discarded = 0;

        private final int[] txSeqs = new int[NUM_CHANNELS];
        private final ChannelRx[] rx = new ChannelRx[NUM_CHANNELS];

        public ShtpLayer() {
            for (int i = 0; i < NUM_CHANNELS; i++) {
                rx[i] = new ChannelRx();
            }
        }

        /** Build a single-fragment frame, consuming the channel's TX seq. */
        public byte[] nextFrame(int channel, byte[] payload) {
            if (SHTP_HEADER_SIZE + payload.length > MAX_TX_FRAME) {
                throw new IllegalArgumentException(
                        "TX cargo " + payload.length + "B exceeds the " + MAX_TX_FRAME + "B MCU slot");
            }
            int seq = txSeqs[channel];
            txSeqs[channel] = (seq + 1) & 0xff;
            return buildFrame(channel, payload, seq);
        }

        public int txSeq(int channel) {
            return txSeqs[channel];
        }

        /** Consume one inbound frame; return the cargo when complete, else null. */
        public ShtpCargo feed(byte[] frame) {
            if (frame.length < SHTP_HEADER_SIZE) {
                return null;
            }
            ShtpHeader hdr = unpackShtpHeader(frame);
            if (hdr.channel() >= NUM_CHANNELS || hdr.length() < SHTP_HEADER_SIZE) {
                return null; // empty/padding header or junk
            }
            int chunkLen = frame.length - SHTP_HEADER_SIZE;
            ChannelRx r = rx[hdr.channel()];
            if (!hdr.continuation()) {
                if (r.expected != 0 && r.received != 0) {
                    discarded += 1;
                }
                r.clear();
                r.chunks.write(frame, SHTP_HEADER_SIZE, chunkLen);
                r.received = chunkLen;
                r.expected = hdr.length() - SHTP_HEADER_SIZE;
                r.seq = hdr.seq();
            } else {
                if (r.expected == 0) {
                    discarded += 1;
                    return null;
                }
                r.chunks.write(frame, SHTP_HEADER_SIZE, chunkLen);
                r.received += chunkLen;
            }
            if (r.received < r.expected) {
                return null;
            }
            if (r.received > r.expected) {
                discarded += 1;
                r.clear();
                return null;
            }
            ShtpCargo cargo = new ShtpCargo(hdr.channel(), r.seq, r.chunks.toByteArray());
            r.clear();
            return cargo;
        }

        /** Forget all TX seq counters and partial cargos (sensor reset). */
        public void reset() {
            for (int i = 0; i < NUM_CHANNELS; i++) {
                txSeqs[i] = 0;
                rx[i].clear();
                rx[i].seq = 0;
            }
            discarded = 0;
        }
    }

    /** Convenience for tests/consumers: reassemble a list of frames. */
    public static List<ShtpCargo> reassemble(List<byte[]> frames, int[] discardedOut) {
        ShtpLayer layer = new ShtpLayer();
        List<ShtpCargo> cargos = new ArrayList<>();
        for (byte[] f : frames) {
            ShtpCargo c = layer.feed(f);
            if (c != null) {
                cargos.add(c);
            }
        }
        if (discardedOut != null && discardedOut.length > 0) {
            discardedOut[0] = layer.discarded;
        }
        return cargos;
    }
}
