package ai.depz.sensor.transport;

import java.io.ByteArrayOutputStream;

/**
 * Packet framing (contracts/01_TRANSPORT_FRAMING.md). Byte-exact with the
 * firmware transport. Per contracts/ERRATA.md E6, a packet whose header
 * advertises a payload CRC type but carries an empty payload has <b>no</b> CRC
 * bytes on the wire.
 */
public final class Framing {
    private Framing() {}

    public static final byte[] MAGIC = {(byte) 0xA5, (byte) 0xC3};
    public static final int HEADER_SIZE = 7;
    public static final int MAX_PAYLOAD = 0x3FFF;

    /** CRC trailer for a payload; empty payloads never carry CRC bytes (E6). */
    public static byte[] payloadCrcBytes(CrcType crcType, byte[] payload) {
        if (crcType == CrcType.NONE || payload.length == 0) {
            return new byte[0];
        }
        switch (crcType) {
            case CRC8:
                return new byte[] {(byte) Crc.crc8Maxim(payload)};
            case CRC16: {
                int c = Crc.crc16Modbus(payload);
                return new byte[] {(byte) c, (byte) (c >>> 8)};
            }
            default: { // CRC32
                long c = Crc.crc32IsoHdlc(payload);
                return new byte[] {(byte) c, (byte) (c >>> 8), (byte) (c >>> 16), (byte) (c >>> 24)};
            }
        }
    }

    /**
     * Frame one packet. {@code crcType} bits are set in the header even for an
     * empty payload (matching device TX), but CRC bytes are only appended for
     * non-empty payloads. {@code seq} is taken modulo 256.
     */
    public static byte[] buildPacket(int cmd, byte[] payload, int seq, CrcType crcType) {
        if (payload.length > MAX_PAYLOAD) {
            throw new IllegalArgumentException("payload too long: " + payload.length + " > " + MAX_PAYLOAD);
        }
        int dataSize = payload.length | (crcType.value << 14);
        byte[] ds = {(byte) dataSize, (byte) (dataSize >>> 8)};
        byte[] hdrInput = {ds[0], ds[1], (byte) cmd, (byte) (seq & 0xFF)};
        int hdrCrc = Crc.crc8Maxim(hdrInput);

        ByteArrayOutputStream out = new ByteArrayOutputStream();
        out.write(MAGIC, 0, MAGIC.length);
        out.write(ds, 0, ds.length);
        out.write(cmd & 0xFF);
        out.write(seq & 0xFF);
        out.write(hdrCrc & 0xFF);
        out.write(payload, 0, payload.length);
        byte[] trailer = payloadCrcBytes(crcType, payload);
        out.write(trailer, 0, trailer.length);
        return out.toByteArray();
    }

    public static byte[] buildPacket(int cmd) {
        return buildPacket(cmd, new byte[0], 0, CrcType.NONE);
    }
}
