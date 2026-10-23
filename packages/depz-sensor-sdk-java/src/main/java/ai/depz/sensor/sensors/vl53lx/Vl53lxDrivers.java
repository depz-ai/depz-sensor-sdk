package ai.depz.sensor.sensors.vl53lx;

import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.function.BiFunction;

/**
 * The drivers behind the product table ({@code uld/registry.py}): which (product, kind) pairs exist,
 * the driver each one runs and what to tell the user about it. A missing pair is absent on purpose.
 */
public final class Vl53lxDrivers {
    private Vl53lxDrivers() {}

    /** A driver bound to a product, and the caveat a UI shows about the pair (or null). */
    public record Created(Vl53lxDriver driver, String caveat) {}

    private record Entry(BiFunction<Vl53lxPlatform, String, Vl53lxDriver> create, String caveat) {}

    private static final String HIST_CAVEAT =
            "the histogram driver has no calibrations and no detection thresholds - the light drivers are the ones with those";

    private static final Map<String, Map<String, Entry>> DRIVERS = new LinkedHashMap<>();

    private static void put(String product, String kind, BiFunction<Vl53lxPlatform, String, Vl53lxDriver> create, String caveat) {
        DRIVERS.computeIfAbsent(product, k -> new LinkedHashMap<>()).put(kind, new Entry(create, caveat));
    }

    static {
        put("VL53L0X", "uld", Vl53l0xUldDriver::new, "no detection thresholds; there is no ROI on this die at all");
        put("VL53L1CX", "uld", Vl53l1UldDriver::new, null);
        put("VL53L1CB", "uld", Vl53l1UldDriver::new, null);
        put("VL53L3CX", "ulp", Vl53l3UlpDriver::new,
                "single-target ranging only - the histogram driver gives several targets instead");
        put("VL53L4CD", "uld", Vl53l4UldDriver::new, null);
        for (String product : List.of("VL53L1CX", "VL53L1CB", "VL53L3CX", "VL53L4CD", "VL53L4CX")) {
            put(product, "histogram", Vl53lxHistogramDriver::new, HIST_CAVEAT);
        }
    }

    /**
     * The driver kinds {@code product} has, in {@link Vl53lxProducts#DRIVER_KINDS} order.
     *
     * @throws UnsupportedOperationException no such product
     */
    public static List<String> driverKinds(String product) {
        Map<String, Entry> have = row(product);
        return Vl53lxProducts.DRIVER_KINDS.stream().filter(have::containsKey).toList();
    }

    /**
     * The driver for one (product, kind) pair, and its caveat or null.
     *
     * @throws UnsupportedOperationException a pair the table has no row for
     */
    public static Created createDriver(String product, String kind, Vl53lxPlatform platform) {
        Entry e = row(product).get(kind);
        if (e == null) {
            throw new UnsupportedOperationException(product + " has no '" + kind + "' driver - it has "
                    + String.join(", ", driverKinds(product)));
        }
        return new Created(e.create().apply(platform, product), e.caveat());
    }

    private static Map<String, Entry> row(String product) {
        Map<String, Entry> row = DRIVERS.get(product);
        if (row == null) {
            throw new UnsupportedOperationException("no such product: " + product + " - served: "
                    + String.join(", ", Vl53lxProducts.PRODUCTS));
        }
        return row;
    }
}
