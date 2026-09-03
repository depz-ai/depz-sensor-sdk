package ai.depz.sensor.protocol;

import java.util.Arrays;

import ai.depz.sensor.transport.Crc;

/** {@code .fwdepz} bootloader container parse/validate (contracts/06 §2). */
public final class FwDepz {
    private FwDepz() {}

    public static final byte[] FWDEPZ_MAGIC = "FWDEPZ00".getBytes(java.nio.charset.StandardCharsets.US_ASCII);
    public static final int FWDEPZ_HEADER_SIZE = 64;

    /** Reason codes match the vector {@code error} strings. */
    public static final class FwDepzError extends RuntimeException {
        public final String code;
        public FwDepzError(String code, String message) {
            super(message);
            this.code = code;
        }
    }

    /** Parsed and validated {@code .fwdepz} firmware container. */
    public record FwDepzImage(long loadAddr, long fwSize, long fwCrc32, int curSec, int totSec, byte[] payload) {

        public boolean payloadCrcOk() {
            return Crc.crc32IsoHdlc(payload) == fwCrc32;
        }

        /**
         * Parse and validate. Validation order (contract 06 §2): length, magic,
         * then CRC-16/CCITT-FALSE over bytes [0..61] vs the u16 LE at offset 62,
         * then {@code fw_size == payload length}.
         */
        public static FwDepzImage parse(byte[] blob) {
            if (blob.length < FWDEPZ_HEADER_SIZE) {
                throw new FwDepzError("too_short", "file too short: " + blob.length + " < " + FWDEPZ_HEADER_SIZE);
            }
            if (!Arrays.equals(Arrays.copyOfRange(blob, 0, 8), FWDEPZ_MAGIC)) {
                throw new FwDepzError("magic", "bad magic (not a .fwdepz file)");
            }
            int hdrCrc = (blob[62] & 0xFF) | ((blob[63] & 0xFF) << 8);
            int actual = Crc.crc16CcittFalse(Arrays.copyOfRange(blob, 0, 62));
            if (hdrCrc != actual) {
                throw new FwDepzError("header_crc",
                    String.format("header CRC mismatch: stored=0x%04X actual=0x%04X", hdrCrc, actual));
            }
            long loadAddr = u32le(blob, 8);
            long fwSize = u32le(blob, 12);
            long fwCrc32 = u32le(blob, 16);
            int curSec = blob[20] & 0xFF;
            int totSec = blob[21] & 0xFF;
            byte[] payload = Arrays.copyOfRange(blob, FWDEPZ_HEADER_SIZE, blob.length);
            if (fwSize != payload.length) {
                throw new FwDepzError("size", "fw_size=" + fwSize + " but payload is " + payload.length + " bytes");
            }
            return new FwDepzImage(loadAddr, fwSize, fwCrc32, curSec, totSec, payload);
        }
    }

    private static long u32le(byte[] b, int off) {
        return (b[off] & 0xFFL) | ((b[off + 1] & 0xFFL) << 8) | ((b[off + 2] & 0xFFL) << 16) | ((b[off + 3] & 0xFFL) << 24);
    }
}
