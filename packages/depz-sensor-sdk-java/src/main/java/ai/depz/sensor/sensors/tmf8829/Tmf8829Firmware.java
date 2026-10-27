package ai.depz.sensor.sensors.tmf8829;

import ai.depz.sensor.devices.InvalidStateException;
import java.io.IOException;
import java.io.InputStream;
import java.io.UncheckedIOException;

/**
 * The embedded TMF8829 sensor firmware (contracts/14_SENSOR_TMF8829.md §4):
 * ams {@code tmf8829_application.hex}, application 2.200.0, <b>MIT</b> (see
 * {@code LICENSES-MIT.TXT} next to the image on the classpath). One contiguous
 * segment, 14 444 B (already a multiple of 4: 3611 words), downloaded by the
 * host through the sensor bootloader to {@link #LOAD_ADDR}.
 */
public final class Tmf8829Firmware {
    private Tmf8829Firmware() {}

    /** Load address of the image in sensor RAM. */
    public static final long LOAD_ADDR = 0x00010000L;

    /** Classpath resource of the image. */
    public static final String RESOURCE = "/ai/depz/sensor/sensors/tmf8829/tmf8829_application.bin";
    /** Classpath resource of the image's MIT license. */
    public static final String LICENSE_RESOURCE = "/ai/depz/sensor/sensors/tmf8829/LICENSES-MIT.TXT";

    private static byte[] cached;

    /** The sensor firmware image (14 444 B); a fresh copy on every call. */
    public static synchronized byte[] image() {
        if (cached == null) {
            cached = readResource(RESOURCE);
        }
        return cached.clone();
    }

    /** The MIT license text that ships with the image. */
    public static String license() {
        return new String(readResource(LICENSE_RESOURCE), java.nio.charset.StandardCharsets.UTF_8);
    }

    private static byte[] readResource(String name) {
        try (InputStream in = Tmf8829Firmware.class.getResourceAsStream(name)) {
            if (in == null) {
                throw new InvalidStateException("resource " + name + " missing from the classpath");
            }
            return in.readAllBytes();
        } catch (IOException e) {
            throw new UncheckedIOException(e);
        }
    }
}
