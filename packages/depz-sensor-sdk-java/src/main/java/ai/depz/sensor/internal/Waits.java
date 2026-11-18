package ai.depz.sensor.internal;

import java.time.Duration;

/**
 * A caller's timeout as a deadline on {@link System#nanoTime()}. A {@link Duration} the clock cannot
 * hold — {@code ChronoUnit.FOREVER}'s 292 years, {@code Duration.ofSeconds(Long.MAX_VALUE)}: the JDK's
 * own "forever" — waits without a deadline instead of failing with the bare {@link ArithmeticException}
 * of {@code toNanos()} or timing out at once on an overflowed sum (bug hunt 2026-11-03 JAVA-10). A
 * negative duration is an expired wait. SDK plumbing: the package is not exported by the module.
 */
public final class Waits {
    /** The deadline of a wait that never expires. */
    public static final long FOREVER = Long.MAX_VALUE;

    private static final Duration MAX_NANOS = Duration.ofNanos(Long.MAX_VALUE);
    private static final Duration MAX_MILLIS = Duration.ofMillis(Long.MAX_VALUE);

    private Waits() {}

    /** {@code timeout} in nanoseconds, saturating at {@link Long#MAX_VALUE}; 0 for a negative one. */
    public static long nanos(Duration timeout) {
        if (timeout.isNegative()) {
            return 0;
        }
        return timeout.compareTo(MAX_NANOS) >= 0 ? Long.MAX_VALUE : timeout.toNanos();
    }

    /** {@code timeout} in milliseconds, saturating at {@link Long#MAX_VALUE}; 0 for a negative one. */
    public static long millis(Duration timeout) {
        if (timeout.isNegative()) {
            return 0;
        }
        return timeout.compareTo(MAX_MILLIS) >= 0 ? Long.MAX_VALUE : timeout.toMillis();
    }

    /** The deadline {@code timeout} from now; {@link #FOREVER} when the sum would overflow the clock. */
    public static long deadline(Duration timeout) {
        long n = nanos(timeout);
        long now = System.nanoTime();
        long end = now + n;
        return end < now ? FOREVER : end; // n >= 0: a sum below now has wrapped
    }

    /** Nanoseconds left until {@code deadline} (≤ 0: expired); {@link Long#MAX_VALUE} for {@link #FOREVER}. */
    public static long left(long deadline) {
        return deadline == FOREVER ? Long.MAX_VALUE : deadline - System.nanoTime();
    }
}
