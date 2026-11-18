package ai.depz.sensor.protocol;

/**
 * Wire-field checks shared by the codecs (contract 07 §3).
 *
 * <p>Encoding: every argument is checked against its wire width before a byte
 * is written — never truncated, wrapped or masked ({@link IllegalArgumentException},
 * the SDK's {@code invalid_argument}). Decoding: a payload shorter than the
 * fields the contract defines is rejected ({@link IllegalArgumentException});
 * trailing bytes are ignored by the callers.
 */
public final class Wire {
    private Wire() {}

    /** {@code value} as an unsigned integer that fits {@code bits} (≤ 63) bits, else IllegalArgumentException. */
    public static long checkUint(String name, long value, int bits) {
        if (value < 0 || value > (1L << bits) - 1) {
            throw new IllegalArgumentException(name + ": " + value + " out of range for u" + bits);
        }
        return value;
    }

    /** {@link #checkUint} for a field of at most 31 bits, as an {@code int}. */
    public static int checkUintI(String name, long value, int bits) {
        return (int) checkUint(name, value, bits);
    }

    /** {@code value} as a signed integer that fits {@code bits} (≤ 32) bits, else IllegalArgumentException. */
    public static int checkInt(String name, long value, int bits) {
        long lim = 1L << (bits - 1);
        if (value < -lim || value > lim - 1) {
            throw new IllegalArgumentException(name + ": " + value + " out of range for i" + bits);
        }
        return (int) value;
    }

    /** Reject a device payload shorter than the {@code n} bytes its fields need. */
    public static void needBytes(String what, byte[] payload, int n) {
        if (payload.length < n) {
            throw new IllegalArgumentException(what + ": need " + n + " bytes, got " + payload.length);
        }
    }
}
