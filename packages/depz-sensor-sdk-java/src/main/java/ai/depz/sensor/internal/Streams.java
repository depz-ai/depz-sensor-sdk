package ai.depz.sensor.internal;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.stream.Collectors;
import java.util.stream.Stream;

/**
 * The JDK 9–16 stream conveniences the SDK needs, in a form Android 12 (API 31) runs:
 * {@code InputStream.readAllBytes()} and {@code Stream.toList()} are API 33 and 34 there, and only
 * the second is covered by core-library desugaring (docs/plans/android-readiness.md). Same results
 * as the JDK methods. SDK plumbing: the package is not exported by the module.
 */
public final class Streams {
    private Streams() {}

    /** Every remaining byte of {@code in}, as {@code in.readAllBytes()}. */
    public static byte[] readAll(InputStream in) throws IOException {
        ByteArrayOutputStream out = new ByteArrayOutputStream();
        byte[] buf = new byte[8192];
        for (int n; (n = in.read(buf, 0, buf.length)) >= 0; ) {
            out.write(buf, 0, n);
        }
        return out.toByteArray();
    }

    /** The elements of {@code s} as an unmodifiable list (nulls allowed), as {@code s.toList()}. */
    public static <T> List<T> toList(Stream<T> s) {
        List<T> list = s.collect(Collectors.toCollection(ArrayList::new));
        return Collections.unmodifiableList(list);
    }
}
