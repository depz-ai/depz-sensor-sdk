package ai.depz.sensor.test.devices;

import java.util.Objects;

/** The assertions of the device tests (no JUnit). */
public final class T {
    private T() {}

    /** {@code a} and {@code b} are the one object (identity, not equality). */
    @SuppressWarnings("ReferenceEquality") // identity is what this asks
    public static boolean same(Object a, Object b) {
        return a == b;
    }

    public static void check(boolean ok, String what) {
        if (!ok) {
            throw new AssertionError(what);
        }
    }

    public static void eq(Object expected, Object actual) {
        eq(expected, actual, "");
    }

    public static void eq(Object expected, Object actual, String what) {
        if (!Objects.deepEquals(expected, actual)) {
            throw new AssertionError((what.isEmpty() ? "" : what + ": ") + "expected <" + str(expected) + ">, got <"
                    + str(actual) + ">");
        }
    }

    private static String str(Object o) {
        if (o instanceof byte[] b) {
            return java.util.Arrays.toString(b);
        }
        if (o instanceof int[] a) {
            return java.util.Arrays.toString(a);
        }
        if (o instanceof long[] a) {
            return java.util.Arrays.toString(a);
        }
        if (o instanceof double[] a) {
            return java.util.Arrays.toString(a);
        }
        return String.valueOf(o);
    }

    @FunctionalInterface
    public interface Body {
        void run() throws Exception;
    }

    /** {@code body} throws a {@code type} (or a subclass); returns it. */
    public static <E extends Throwable> E throwsType(Class<E> type, Body body) {
        try {
            body.run();
        } catch (Throwable e) {
            if (type.isInstance(e)) {
                return type.cast(e);
            }
            throw new AssertionError("expected " + type.getSimpleName() + ", got " + e.getClass().getSimpleName() + ": "
                    + e.getMessage(), e);
        }
        throw new AssertionError("expected " + type.getSimpleName() + ", nothing thrown");
    }

    /** Wait until {@code cond} holds, up to {@code ms}. */
    public static boolean until(java.util.function.BooleanSupplier cond, long ms) {
        long end = System.nanoTime() + ms * 1_000_000;
        while (System.nanoTime() < end) {
            if (cond.getAsBoolean()) {
                return true;
            }
            try {
                Thread.sleep(2);
            } catch (InterruptedException e) {
                Thread.currentThread().interrupt();
                return false;
            }
        }
        return cond.getAsBoolean();
    }
}
