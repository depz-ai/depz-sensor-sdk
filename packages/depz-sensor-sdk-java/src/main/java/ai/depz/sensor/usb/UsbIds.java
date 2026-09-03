package ai.depz.sensor.usb;

import java.util.ArrayList;
import java.util.Comparator;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;

/**
 * DEPZ USB identity table (contracts/02_COMMON_COMMANDS.md §4).
 *
 * <p>Used to pick the right serial port without poking unrelated devices; the
 * protocol probe (GET_NAME_ACTIVE_SOFTWARE) remains the source of truth for
 * what a device actually is. The PID→model map is an informational hint only.
 */
public final class UsbIds {
    private UsbIds() {}

    /** Production VID shared by every DEPZ sensor. */
    public static final int DEPZ_USB_VID = 0x1BCF; // 7119

    public static final int PID_SR04 = 0xEC78; // 60536
    /** VL53L8CH production USB PID (the CH variant). */
    public static final int PID_VL53L8 = 0xED40; // 60736
    /** VL53L8CX production USB PID (hw-verified). */
    public static final int PID_VL53L8CX = 0xED4B; // 60747
    /** VL53L4CD production USB PID. */
    public static final int PID_VL53L4CD = 0xED45; // 60741
    public static final int PID_BNO086 = 0xEE08; // 60936

    /** PID → sensor-model hint. Informational: the protocol probe is authoritative. */
    public static final Map<Integer, String> DEPZ_PID_MODEL;

    static {
        Map<Integer, String> m = new LinkedHashMap<>();
        m.put(PID_SR04, "sr04");
        m.put(PID_VL53L8, "vl53l8ch");
        m.put(0xED41, "vl53l0x"); // 60737
        m.put(0xED42, "vl53l1cb"); // 60738
        m.put(0xED43, "vl53l1cx"); // 60739
        m.put(0xED44, "vl53l3cx"); // 60740
        m.put(PID_VL53L4CD, "vl53l4cd");
        m.put(0xED46, "vl53l4cx"); // 60742
        m.put(0xED47, "vl53l4ed"); // 60743
        m.put(0xED48, "vl53l5cx"); // 60744
        m.put(0xED49, "vl53l7cx"); // 60745
        m.put(0xED4A, "vl53l7ch"); // 60746
        m.put(PID_VL53L8CX, "vl53l8cx");
        m.put(PID_BNO086, "bno086");
        m.put(0xEE09, "bno085"); // 60937
        m.put(0xEE0A, "bno055"); // 60938
        DEPZ_PID_MODEL = java.util.Collections.unmodifiableMap(m);
    }

    /** Whole reserved sensor block (60536..65535 inclusive). */
    public static final int DEPZ_PID_RANGE_LO = 60536;
    public static final int DEPZ_PID_RANGE_HI = 65535;

    /** Dev / unprogrammed default: STMicroelectronics VID/PID. */
    public static final int DEV_USB_VID = 0x0483; // 1155
    public static final int DEV_USB_PID = 0x56DC; // 22236

    /** True when (vid, pid) is a recognized DEPZ (or dev-default) USB id. */
    public static boolean isKnownDepzUsb(Integer vid, Integer pid) {
        if (vid == null || pid == null) {
            return false;
        }
        if (vid == DEV_USB_VID && pid == DEV_USB_PID) {
            return true;
        }
        if (vid != DEPZ_USB_VID) {
            return false;
        }
        if (DEPZ_PID_MODEL.containsKey(pid)) {
            return true;
        }
        return DEPZ_PID_RANGE_LO <= pid && pid <= DEPZ_PID_RANGE_HI;
    }

    /** Best-guess model name for a (vid, pid), or {@code null}. Informational only. */
    public static String usbModelHint(Integer vid, Integer pid) {
        if (vid == null || pid == null) {
            return null;
        }
        if (vid == DEV_USB_VID && pid == DEV_USB_PID) {
            return "dev";
        }
        if (vid == DEPZ_USB_VID) {
            return DEPZ_PID_MODEL.get(pid);
        }
        return null;
    }

    /** One enumerated serial port with its USB iSerial (may be null/empty). */
    public record PortRef(String port, String usbSerial) {}

    private static final Comparator<PortRef> SERIAL_ORDER = (a, b) -> {
        boolean aHas = a.usbSerial() != null && !a.usbSerial().isEmpty();
        boolean bHas = b.usbSerial() != null && !b.usbSerial().isEmpty();
        int ga = aHas ? 0 : 1;
        int gb = bHas ? 0 : 1;
        if (ga != gb) {
            return Integer.compare(ga, gb);
        }
        String sa = aHas ? a.usbSerial() : "";
        String sb = bHas ? b.usbSerial() : "";
        int c = sa.compareTo(sb);
        if (c != 0) {
            return c;
        }
        return a.port().compareTo(b.port());
    };

    /**
     * Order ports by USB iSerial ascending; null/empty serials sort last,
     * tie-broken by port path (contract 02 §4).
     */
    public static List<PortRef> orderBySerial(List<PortRef> ports) {
        List<PortRef> sorted = new ArrayList<>(ports);
        sorted.sort(SERIAL_ORDER);
        return sorted;
    }
}
