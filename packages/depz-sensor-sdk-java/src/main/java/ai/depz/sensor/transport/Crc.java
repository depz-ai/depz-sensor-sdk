package ai.depz.sensor.transport;

/**
 * CRC algorithms of the DEPZ transport (contracts/01_TRANSPORT_FRAMING.md §3).
 *
 * <p>All wire CRCs are reflected table implementations, byte-exact with the
 * firmware and the reference host tool. CRC-8 init is 0x00 for every device
 * (contracts/ERRATA.md E1). {@link #crc16CcittFalse} is used only for the
 * {@code .fwdepz} file header (contract 06), never on the wire.
 *
 * <p>Returned values are unsigned, held in wider signed Java types: crc8 in an
 * {@code int} 0..255, crc16 in an {@code int} 0..65535, crc32 in a {@code long}
 * 0..0xFFFFFFFF.
 */
public final class Crc {
    private Crc() {}

    private static long[] makeTable(long polyReflected, int width) {
        long mask = (width == 64) ? -1L : (1L << width) - 1;
        long[] table = new long[256];
        for (int i = 0; i < 256; i++) {
            long crc = i;
            for (int j = 0; j < 8; j++) {
                crc = ((crc & 1) != 0) ? (crc >>> 1) ^ polyReflected : crc >>> 1;
            }
            table[i] = crc & mask;
        }
        return table;
    }

    private static final long[] CRC8_TABLE = makeTable(0x8CL, 8);
    private static final long[] CRC16_TABLE = makeTable(0xA001L, 16);
    private static final long[] CRC32_TABLE = makeTable(0xEDB88320L, 32);

    /** CRC-8/MAXIM: poly 0x31 reflected, init 0x00, xorout 0x00. */
    public static int crc8Maxim(byte[] data) {
        int crc = 0x00;
        for (byte b : data) {
            crc = (int) CRC8_TABLE[(crc ^ (b & 0xFF)) & 0xFF];
        }
        return crc & 0xFF;
    }

    /** CRC-16/MODBUS: poly 0x8005 reflected, init 0xFFFF, xorout 0x0000. */
    public static int crc16Modbus(byte[] data) {
        int crc = 0xFFFF;
        for (byte b : data) {
            crc = (crc >>> 8) ^ (int) CRC16_TABLE[(crc ^ (b & 0xFF)) & 0xFF];
        }
        return crc & 0xFFFF;
    }

    /** CRC-32/ISO-HDLC: poly 0x04C11DB7 reflected, init/xorout 0xFFFFFFFF. */
    public static long crc32IsoHdlc(byte[] data) {
        long crc = 0xFFFFFFFFL;
        for (byte b : data) {
            crc = (crc >>> 8) ^ CRC32_TABLE[(int) ((crc ^ (b & 0xFF)) & 0xFF)];
        }
        return (crc ^ 0xFFFFFFFFL) & 0xFFFFFFFFL;
    }

    /**
     * CRC-16/CCITT-FALSE: poly 0x1021, init 0xFFFF, not reflected.
     * Used only for the {@code .fwdepz} file header (contract 06).
     */
    public static int crc16CcittFalse(byte[] data) {
        int crc = 0xFFFF;
        for (byte b : data) {
            crc ^= (b & 0xFF) << 8;
            for (int i = 0; i < 8; i++) {
                crc = ((crc & 0x8000) != 0) ? ((crc << 1) ^ 0x1021) & 0xFFFF : (crc << 1) & 0xFFFF;
            }
        }
        return crc & 0xFFFF;
    }
}
