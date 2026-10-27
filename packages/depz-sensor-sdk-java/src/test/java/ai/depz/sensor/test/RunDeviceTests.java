package ai.depz.sensor.test;

import ai.depz.sensor.test.devices.DeviceCoreTests;
import java.io.IOException;
import java.io.UncheckedIOException;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.lang.reflect.Modifier;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.StandardOpenOption;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Comparator;
import java.util.List;

/**
 * The device-layer harness (no JUnit): every public static no-argument method of the listed test
 * classes is a test, and every contract 15 scenario of a ported class runs against its capture.
 * Exits nonzero on any failure. Optional argument: a name filter (substring).
 */
public final class RunDeviceTests {
    private static final List<Class<?>> TEST_CLASSES = List.of(
            DeviceCoreTests.class,
            ai.depz.sensor.test.devices.Vl53l4cdDriverTests.class,
            ai.depz.sensor.test.devices.Vl53l8DriverTests.class,
            ai.depz.sensor.test.devices.Bno055DeviceTests.class,
            ai.depz.sensor.test.devices.Bno086DeviceTests.class,
            ai.depz.sensor.test.devices.Vl53lxHistogramTests.class,
            ai.depz.sensor.test.devices.Tmf8829DeviceTests.class,
            ai.depz.sensor.test.devices.DatasetRecorderTests.class,
            ai.depz.sensor.test.devices.Vl53lxDeviceTests.class,
            ai.depz.sensor.test.devices.Vl53lxDriverTests.class,
            ai.depz.sensor.test.devices.Vl53l7DeviceTests.class,
            ai.depz.sensor.test.devices.DepzDevicesTests.class,
            ai.depz.sensor.test.devices.DatasetRecordsTests.class,
            ai.depz.sensor.test.devices.Vl53lxBareImageTests.class,
            ai.depz.sensor.test.devices.ShtpTests.class,
            ai.depz.sensor.test.devices.Vl53l0xUldTests.class,
            ai.depz.sensor.test.devices.Vl53l4cdDeviceTests.class,
            ai.depz.sensor.test.devices.Bno055ModelTests.class);

    private RunDeviceTests() {}

    public static void main(String[] args) throws Exception {
        Path vectors = Path.of(args.length > 0 ? args[0] : "../../contracts/vectors");
        String filter = args.length > 1 ? args[1] : "";
        List<String> failures = new ArrayList<>();
        int passed = 0;

        for (Class<?> cls : TEST_CLASSES) {
            Method[] methods = cls.getDeclaredMethods();
            Arrays.sort(methods, Comparator.comparing(Method::getName));
            for (Method m : methods) {
                if (!Modifier.isPublic(m.getModifiers()) || !Modifier.isStatic(m.getModifiers()) || m.getParameterCount() != 0) {
                    continue;
                }
                String name = cls.getSimpleName() + "." + m.getName();
                if (!name.contains(filter)) {
                    continue;
                }
                try {
                    m.invoke(null);
                    passed++;
                } catch (InvocationTargetException e) {
                    Throwable c = e.getCause();
                    failures.add(name + ": " + c.getClass().getSimpleName() + ": " + c.getMessage());
                }
            }
        }

        for (Path file : ai.depz.sensor.test.devices.Scenarios.files(vectors)) {
            String name = "scenario " + file.getFileName();
            if (!name.contains(filter)) {
                continue;
            }
            try {
                for (String id : ai.depz.sensor.test.devices.Scenarios.run(file, vectors)) {
                    seen(id);
                }
                passed++;
            } catch (Throwable e) {
                failures.add(name + ": " + e.getMessage());
            }
        }

        System.out.println("device tests: " + passed + " passed, " + failures.size() + " failed");
        for (String f : failures) {
            System.out.println("  FAIL " + f);
        }
        System.exit(failures.isEmpty() ? 0 : 1);
    }

    // Report a case id as asserted (scripts/check_vector_coverage.py); no-op without the env var.
    private static void seen(String id) {
        String path = System.getenv("DEPZ_VECTOR_SEEN");
        if (path == null || path.isEmpty()) {
            return;
        }
        try {
            Files.writeString(Path.of(path), id + "\n", StandardCharsets.UTF_8, StandardOpenOption.CREATE,
                    StandardOpenOption.APPEND);
        } catch (IOException e) {
            throw new UncheckedIOException(e);
        }
    }
}
