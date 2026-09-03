package ai.depz.sensor.transport;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

/**
 * Incremental frame parser. Feed arbitrary byte chunks; get events.
 *
 * <p>Event order is invariant to chunking (contract 01 §5) except Trash event
 * boundaries — concatenate Trash data when comparing streams. Byte-exact with
 * the reference Python {@code PacketParser}, including the empty-payload-no-CRC
 * rule (ERRATA E6) and single-byte resync on a corrupt header.
 */
public final class PacketParser {
    private byte[] buf = new byte[64];
    private int len = 0;

    public int packets = 0;
    public int crcErrors = 0;
    public int headerErrors = 0;
    public int trashBytes = 0;

    private void ensureCapacity(int extra) {
        if (len + extra > buf.length) {
            int cap = buf.length;
            while (cap < len + extra) {
                cap *= 2;
            }
            buf = Arrays.copyOf(buf, cap);
        }
    }

    private void delFront(int n) {
        System.arraycopy(buf, n, buf, 0, len - n);
        len -= n;
    }

    private int findMagic() {
        for (int i = 0; i + 1 < len; i++) {
            if (buf[i] == Framing.MAGIC[0] && buf[i + 1] == Framing.MAGIC[1]) {
                return i;
            }
        }
        return -1;
    }

    /** Current unconsumed bytes (residue). */
    public byte[] residue() {
        return Arrays.copyOf(buf, len);
    }

    public List<Event> feed(byte[] data) {
        ensureCapacity(data.length);
        System.arraycopy(data, 0, buf, len, data.length);
        len += data.length;
        List<Event> out = new ArrayList<>();
        while (true) {
            Event ev = parseOne();
            if (ev == null) {
                break;
            }
            out.add(ev);
        }
        return out;
    }

    private Trash emitTrash(int count) {
        byte[] data = Arrays.copyOf(buf, count);
        delFront(count);
        trashBytes += count;
        return new Trash(data);
    }

    private static int u16le(byte[] b, int off) {
        return (b[off] & 0xFF) | ((b[off + 1] & 0xFF) << 8);
    }

    private Event parseOne() {
        int pos = findMagic();
        if (pos == -1) {
            // Keep the last byte: it may be a split 0xA5.
            if (len > 1) {
                return emitTrash(len - 1);
            }
            return null;
        }
        if (pos > 0) {
            return emitTrash(pos);
        }
        if (len < Framing.HEADER_SIZE) {
            return null;
        }
        int dataSize = u16le(buf, 2);
        int payloadSize = dataSize & Framing.MAX_PAYLOAD;
        CrcType crcType = CrcType.fromValue((dataSize >>> 14) & 0x03);
        byte[] hdrInput = {buf[2], buf[3], buf[4], buf[5]};
        if (Crc.crc8Maxim(hdrInput) != (buf[6] & 0xFF)) {
            // Header corrupt: advance one byte past the magic start and let the
            // magic hunt resync (firmware: rb_skip(off + 1)).
            headerErrors++;
            return emitTrash(1);
        }
        // ERRATA E6: empty payload never carries CRC bytes.
        int crcSize = payloadSize > 0 ? crcType.size : 0;
        int total = Framing.HEADER_SIZE + payloadSize + crcSize;
        if (len < total) {
            return null;
        }
        int cmd = buf[4] & 0xFF;
        int seq = buf[5] & 0xFF;
        byte[] payload = Arrays.copyOfRange(buf, Framing.HEADER_SIZE, Framing.HEADER_SIZE + payloadSize);
        byte[] trailer = Arrays.copyOfRange(buf, Framing.HEADER_SIZE + payloadSize, total);
        delFront(total);
        if (crcSize > 0 && !Arrays.equals(Framing.payloadCrcBytes(crcType, payload), trailer)) {
            crcErrors++;
            return new CrcError(cmd, seq);
        }
        packets++;
        return new Packet(cmd, seq, payload);
    }
}
