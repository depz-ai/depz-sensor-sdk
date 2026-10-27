package ai.depz.sensor.test.devices;

import static ai.depz.sensor.test.devices.T.check;
import static ai.depz.sensor.test.devices.T.eq;
import static ai.depz.sensor.test.devices.T.throwsType;

import ai.depz.sensor.sensors.bno086.Shtp;
import ai.depz.sensor.sensors.bno086.Shtp.ShtpCargo;
import ai.depz.sensor.sensors.bno086.Shtp.ShtpHeader;
import ai.depz.sensor.sensors.bno086.Shtp.ShtpLayer;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

/**
 * The SHTP layer's reassembly state machine and TX sequence counters (the Python tests'
 * test_bno086_reports.py SHTP block): single and multi-fragment cargos, a continuation without a
 * start, a start that pre-empts a partial, overruns, junk channels, padding headers, reset, the TX
 * slot limit and the first fragment's capture time.
 */
public final class ShtpTests {
    private ShtpTests() {}

    private static final int CONTROL = 2;

    /** A cargo split into frames of at most {@code maxFrame} bytes, as the hub sends it: the first
     *  advertises the total cargo length, each continuation the remaining length with bit 15 set. */
    private static List<byte[]> fragment(int channel, byte[] payload, int seqStart, int maxFrame) {
        int room = maxFrame - Shtp.SHTP_HEADER_SIZE;
        List<byte[]> frames = new ArrayList<>();
        int off = 0;
        int seq = seqStart & 0xFF;
        int total = Shtp.SHTP_HEADER_SIZE + payload.length;
        do {
            int chunk = Math.min(room, payload.length - off);
            byte[] hdr = Shtp.packShtpHeader(new ShtpHeader(total - off, channel, seq, off > 0));
            byte[] f = Arrays.copyOf(hdr, hdr.length + chunk);
            System.arraycopy(payload, off, f, hdr.length, chunk);
            frames.add(f);
            off += chunk;
            seq = (seq + 1) & 0xFF;
        } while (off < payload.length);
        return frames;
    }

    private static byte[] range(int n) {
        byte[] b = new byte[n];
        for (int i = 0; i < n; i++) {
            b[i] = (byte) i;
        }
        return b;
    }

    public static void singleFrameRoundTrip() {
        ShtpLayer layer = new ShtpLayer();
        List<byte[]> frames = fragment(CONTROL, "hello".getBytes(java.nio.charset.StandardCharsets.US_ASCII), 0, 64);
        eq(1, frames.size());
        ShtpCargo cargo = layer.feed(frames.get(0));
        check(cargo != null, "complete");
        eq("hello".getBytes(java.nio.charset.StandardCharsets.US_ASCII), cargo.payload());
        eq(CONTROL, cargo.channel());
        eq(0L, cargo.captureUs());
    }

    public static void multiFragmentReassemblyCarriesTheFirstFragmentsSeqAndCapture() {
        ShtpLayer layer = new ShtpLayer();
        byte[] payload = range(100);
        List<byte[]> frames = fragment(CONTROL, payload, 7, 32);
        check(frames.size() >= 4, "fragments: " + frames.size());
        for (int i = 0; i < frames.size() - 1; i++) {
            check(layer.feed(frames.get(i), 1000 + i) == null, "completes only on the last frame");
        }
        ShtpCargo cargo = layer.feed(frames.get(frames.size() - 1), 9999);
        check(cargo != null, "complete");
        eq(payload, cargo.payload());
        eq(7, cargo.seq());
        eq(1000L, cargo.captureUs()); // the base its 0xFB timestamps refer to
        eq(0, layer.discarded);
    }

    public static void continuationWithoutStartIsDropped() {
        ShtpLayer layer = new ShtpLayer();
        List<byte[]> frames = fragment(CONTROL, new byte[50], 0, 16);
        check(layer.feed(frames.get(1)) == null, "dropped");
        eq(1, layer.discarded);
    }

    public static void newStartDiscardsPartial() {
        ShtpLayer layer = new ShtpLayer();
        List<byte[]> frames = fragment(CONTROL, new byte[50], 0, 16);
        check(layer.feed(frames.get(0)) == null, "a partial cargo");
        ShtpCargo cargo = layer.feed(fragment(CONTROL, new byte[] {'x'}, 5, 64).get(0));
        check(cargo != null, "a non-continuation frame pre-empts the partial");
        eq(new byte[] {'x'}, cargo.payload());
        eq(5, cargo.seq());
        eq(1, layer.discarded);
    }

    public static void anOverrunIsDiscardedAndTheChannelStartsOver() {
        ShtpLayer layer = new ShtpLayer();
        List<byte[]> frames = fragment(CONTROL, new byte[20], 0, 16); // 12 + 8 bytes
        check(layer.feed(frames.get(0)) == null, "partial");
        byte[] longer = Arrays.copyOf(frames.get(1), frames.get(1).length + 3); // 3 bytes past the length
        check(layer.feed(longer) == null, "overrun");
        eq(1, layer.discarded);
        // the channel is clean again: a continuation now has no start
        check(layer.feed(frames.get(1)) == null, "no start");
        eq(2, layer.discarded);
        ShtpCargo ok = layer.feed(fragment(CONTROL, new byte[] {1, 2}, 0, 64).get(0));
        eq(new byte[] {1, 2}, ok.payload());
    }

    public static void junkChannelsShortFramesAndPaddingHeaders() {
        ShtpLayer layer = new ShtpLayer();
        check(layer.feed(new byte[] {1, 2, 3}) == null, "shorter than a header");
        check(layer.feed(new byte[] {0, 0, 0, 0}) == null, "empty / padding header (no data read)");
        eq(0, layer.discarded);
        check(layer.feed(Shtp.buildFrame(Shtp.NUM_CHANNELS, new byte[] {1}, 0)) == null, "channel 6 is junk");
        eq(1, layer.discarded);
    }

    public static void channelsReassembleIndependently() {
        ShtpLayer layer = new ShtpLayer();
        List<byte[]> a = fragment(CONTROL, range(30), 0, 20);
        List<byte[]> b = fragment(3, range(10), 0, 64);
        check(layer.feed(a.get(0)) == null, "partial on 2");
        ShtpCargo cb = layer.feed(b.get(0));
        eq(3, cb.channel());
        ShtpCargo ca = layer.feed(a.get(1));
        eq(range(30), ca.payload());
        eq(0, layer.discarded);
    }

    public static void txSeqCountsPerChannelWrapsAndResets() {
        ShtpLayer layer = new ShtpLayer();
        byte[] f0 = layer.nextFrame(CONTROL, new byte[] {'a'});
        eq(new ShtpHeader(5, CONTROL, 0, false), Shtp.unpackShtpHeader(f0));
        eq(1, layer.txSeq(CONTROL));
        eq(0, layer.txSeq(Shtp.CHANNEL_EXECUTABLE));
        for (int i = 1; i < 256; i++) {
            layer.nextFrame(CONTROL, new byte[] {'a'});
        }
        eq(0, layer.txSeq(CONTROL), "u8 wrap");
        layer.nextFrame(Shtp.CHANNEL_EXECUTABLE, new byte[] {(byte) Shtp.EXECUTABLE_RESET});
        check(layer.feed(fragment(CONTROL, new byte[40], 0, 16).get(0)) == null, "partial");
        check(layer.feed(new byte[] {1, 2, 3, 4, 5}) == null, "a partial on channel 3 (513 B advertised)");
        layer.reset();
        eq(0, layer.txSeq(Shtp.CHANNEL_EXECUTABLE));
        eq(0, layer.discarded);
        // the partial is gone too: a continuation has no start after reset
        check(layer.feed(fragment(CONTROL, new byte[40], 0, 16).get(1)) == null, "no start");
        eq(1, layer.discarded);
    }

    public static void txCargoTooLargeOrABadChannelRaises() {
        ShtpLayer layer = new ShtpLayer();
        IllegalArgumentException e = throwsType(IllegalArgumentException.class,
                () -> layer.nextFrame(CONTROL, new byte[200])); // exceeds the 64 B MCU slot
        eq("TX cargo 200B exceeds the 64B MCU slot", e.getMessage());
        layer.nextFrame(CONTROL, new byte[Shtp.MAX_TX_FRAME - Shtp.SHTP_HEADER_SIZE]); // exactly fits
        throwsType(IllegalArgumentException.class, () -> layer.nextFrame(Shtp.NUM_CHANNELS, new byte[1]));
        throwsType(IllegalArgumentException.class, () -> layer.nextFrame(-1, new byte[1]));
        eq(1, layer.txSeq(CONTROL), "a refused frame consumes no seq");
    }

    public static void reassembleReportsTheDiscards() {
        List<byte[]> frames = new ArrayList<>(fragment(CONTROL, range(50), 0, 16));
        frames.add(0, fragment(CONTROL, new byte[50], 0, 16).get(2)); // a lone continuation first
        int[] discarded = new int[1];
        List<ShtpCargo> cargos = Shtp.reassemble(frames, discarded);
        eq(1, cargos.size());
        eq(range(50), cargos.get(0).payload());
        eq(1, discarded[0]);
        eq(1, Shtp.reassemble(frames, null).size());
    }
}
