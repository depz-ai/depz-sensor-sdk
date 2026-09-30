package ai.depz.sensor.sensors.vl53lx;

import java.util.LinkedHashMap;
import java.util.List;
import java.util.Locale;
import java.util.Map;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * The VL53L 1D-family product table and class resolution
 * (contracts/12_SENSOR_VL53LX.md §1, §3; Python reference
 * {@code depz_sensor_sdk/vl53lx/uld/registry.py} and
 * {@code discovery.py::_vl53lx_class}).
 *
 * <p>Two independent axes: the <b>product</b> (whose parameter set to load —
 * normally what is soldered on, but naming a neighbour borrows its driver) and
 * the <b>driver kind</b> ({@code uld}, {@code ulp}, {@code histogram}). A missing
 * pair is a refusal, never a fallback. The model id is a cross-check only:
 * L1CX/L1CB share {@code 0xEACC}, L4CD/L4CX share {@code 0xEBAA}.
 *
 * <p>Java carries no live driver (as for every other sensor); each pair is
 * described by the bridge parameters its driver hands the firmware
 * ({@link Bridge}).
 */
public final class Vl53lxProducts {
    private Vl53lxProducts() {}

    /** Driver kinds, in the order a UI should list them. */
    public static final List<String> DRIVER_KINDS = List.of("uld", "ulp", "histogram");

    /** Every product of the family, in the order a UI should list them. */
    public static final List<String> PRODUCTS = List.of(
        "VL53L0X", "VL53L1CX", "VL53L1CB", "VL53L3CX", "VL53L4CD", "VL53L4CX");

    /**
     * What one driver tells the bridge (contract 12 §1, §3): register-address
     * width, the interrupt-release writes {@code {addr, value}} played after each
     * block read, the bus ceiling after init, and the streamed block.
     */
    public record Bridge(int addrWidth, int[][] clearSteps, int maxKhz, int blockAddr, int blockLen) {}

    /** VL53L0X ULD: 12 B at 0x14, width 1, 0x0B←1 then 0x0B←0, 400 kHz (no FM+ pad). */
    public static final Bridge L0X_ULD =
        new Bridge(1, new int[][] {{0x0B, 0x01}, {0x0B, 0x00}}, 400, 0x14, 12);
    /** Die ULD/ULP (L1CX, L1CB, L3CX, L4CD): 17 B at 0x0089, 0x0086←1, 1 MHz. */
    public static final Bridge DIE_ULD =
        new Bridge(2, new int[][] {{0x0086, 0x01}}, 1000, 0x0089, 17);
    /** Histogram (ST Bare Driver): 83 B at 0x0088, 0x0086←1, 1 MHz. */
    public static final Bridge HISTOGRAM =
        new Bridge(2, new int[][] {{0x0086, 0x01}}, 1000, 0x0088, 83);

    /**
     * One row of the table. {@code drivers} maps a kind from {@link #DRIVER_KINDS}
     * to its bridge parameters; an absent kind is absent on purpose.
     * {@code reachMm} is the datasheet rating of the module.
     */
    public record Product(String name, int modelId, int reachMm, String defaultDriver,
                          Map<String, Bridge> drivers) {
        /** Driver kinds this product has, in {@link #DRIVER_KINDS} order. */
        public List<String> driverKinds() {
            return DRIVER_KINDS.stream().filter(drivers::containsKey).toList();
        }

        /** Bridge parameters of one pair; throws {@link UnsupportedOperationException} for a missing pair. */
        public Bridge bridge(String kind) {
            Bridge b = drivers.get(kind);
            if (b == null) {
                throw new UnsupportedOperationException(
                    name + " has no '" + kind + "' driver - it has " + String.join(", ", driverKinds()));
            }
            return b;
        }

        /** Bridge parameters of the default driver. */
        public Bridge defaultBridge() {
            return bridge(defaultDriver);
        }

        /** "Not something else entirely" — pairs sharing an id cannot be told apart. */
        public boolean modelIdOk(int value) {
            return modelId == value;
        }
    }

    private static Product row(String name, int modelId, int reachMm, String def, Object... kv) {
        Map<String, Bridge> m = new LinkedHashMap<>();
        for (int i = 0; i < kv.length; i += 2) {
            m.put((String) kv[i], (Bridge) kv[i + 1]);
        }
        return new Product(name, modelId, reachMm, def, java.util.Collections.unmodifiableMap(m));
    }

    /** The product table (contract 12 §1), keyed by product name, in {@link #PRODUCTS} order. */
    public static final Map<String, Product> TABLE;
    static {
        Map<String, Product> t = new LinkedHashMap<>();
        t.put("VL53L0X", row("VL53L0X", 0x00EE, 2000, "uld", "uld", L0X_ULD));
        t.put("VL53L1CX", row("VL53L1CX", 0xEACC, 4000, "uld", "uld", DIE_ULD, "histogram", HISTOGRAM));
        t.put("VL53L1CB", row("VL53L1CB", 0xEACC, 8000, "uld", "uld", DIE_ULD, "histogram", HISTOGRAM));
        t.put("VL53L3CX", row("VL53L3CX", 0xEAAA, 3000, "ulp", "ulp", DIE_ULD, "histogram", HISTOGRAM));
        t.put("VL53L4CD", row("VL53L4CD", 0xEBAA, 1200, "uld", "uld", DIE_ULD, "histogram", HISTOGRAM));
        t.put("VL53L4CX", row("VL53L4CX", 0xEBAA, 6000, "histogram", "histogram", HISTOGRAM));
        TABLE = java.util.Collections.unmodifiableMap(t);
    }

    /** The table row; throws {@link UnsupportedOperationException} for a product nobody serves. */
    public static Product product(String name) {
        Product p = TABLE.get(name);
        if (p == null) {
            throw new UnsupportedOperationException(
                "no such product: " + name + " - served: " + String.join(", ", PRODUCTS));
        }
        return p;
    }

    private static final Pattern NAME_RE = Pattern.compile("VL53L(\\d[A-Z0-9]*)");

    /**
     * {@code "ToF Sensor VL53L4CD USB v2.1"} → {@code "VL53L4CD"}: the first
     * {@code VL53L<digit><part>} match, case-insensitive. {@code null} if the name
     * carries no product this table serves (an unstamped board, an unknown part).
     */
    public static String productFromBoardName(String name) {
        if (name == null || name.isEmpty()) {
            return null;
        }
        Matcher m = NAME_RE.matcher(name.toUpperCase(Locale.ROOT));
        if (!m.find()) {
            return null;
        }
        String product = "VL53L" + m.group(1);
        return TABLE.containsKey(product) ? product : null;
    }

    /**
     * The sensor class an {@code APP_VL53L0_4} board opens as (the Python/TS
     * class names). {@link #VL53LX} is the generic class that takes the product
     * at init — also what a VL53L4CD board on this firmware uses.
     */
    public enum SensorClass {
        VL53L0X("Vl53l0x", "VL53L0X"),
        VL53L1CX("Vl53l1cx", "VL53L1CX"),
        VL53L1CB("Vl53l1cb", "VL53L1CB"),
        VL53L3CX("Vl53l3cx", "VL53L3CX"),
        VL53L4CX("Vl53l4cx", "VL53L4CX"),
        VL53LX("Vl53lx", null);

        public final String className;
        /** The product the class is fixed to, {@code null} for the generic class. */
        public final String product;
        SensorClass(String className, String product) {
            this.className = className;
            this.product = product;
        }

        static SensorClass forProduct(String product) {
            if (product != null) {
                for (SensorClass c : values()) {
                    if (product.equals(c.product)) {
                        return c;
                    }
                }
            }
            return VL53LX;
        }
    }

    /**
     * Class resolution (contract 12 §1, pinned by {@code vl53lx.json model}): the
     * production PID model ({@code usbModel}, e.g. {@code "vl53l0x"}; may be
     * {@code null}) if it is a family product, else the product the device name
     * carries, else the generic class.
     */
    public static SensorClass resolveClass(String usbModel, String deviceName) {
        String product = usbModel == null || usbModel.isEmpty()
            ? null : usbModel.toUpperCase(Locale.ROOT);
        if (product == null || !PRODUCTS.contains(product)) {
            product = productFromBoardName(deviceName == null ? "" : deviceName);
        }
        return SensorClass.forProduct(product);
    }
}
