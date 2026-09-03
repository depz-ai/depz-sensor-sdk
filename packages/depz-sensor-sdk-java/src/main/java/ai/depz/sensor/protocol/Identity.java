package ai.depz.sensor.protocol;

import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * Firmware-name parsing (contracts/02_COMMON_COMMANDS.md §4).
 *
 * @param mode       "app" | "bootloader" | "unknown"
 * @param sensorType {@code null} in bootloader/unknown mode
 * @param softwareName the classified name
 * @param version    "" when not parseable
 */
public record Identity(String mode, SensorType sensorType, String softwareName, String version) {

    public enum SensorType {
        SR04("sr04"),
        VL53L4("vl53l4"),
        VL53L8("vl53l8"),
        BNO086("bno086"),
        UNKNOWN("unknown");

        public final String label;
        SensorType(String label) { this.label = label; }
    }

    private static final Pattern VERSION_RE = Pattern.compile("_v(\\d+(?:\\.\\d+)*)$");

    private static final Object[][] PRODUCT_TOKENS = {
        {"SR04", SensorType.SR04},
        {"VL53L4", SensorType.VL53L4},
        {"VL53L8", SensorType.VL53L8},
        {"BNO086", SensorType.BNO086},
    };

    /**
     * Classify a GET_NAME_ACTIVE_SOFTWARE string. The string must already be
     * stripped of trailing NUL/0xFF ({@link Common#stripDeviceString}).
     */
    public static Identity parseSoftwareName(String name) {
        Matcher m = VERSION_RE.matcher(name);
        String version = m.find() ? m.group(1) : "";
        if (name.startsWith("BOOTDEPZ")) {
            return new Identity("bootloader", null, name, version);
        }
        if (name.startsWith("APP_")) {
            for (Object[] tok : PRODUCT_TOKENS) {
                if (name.contains((String) tok[0])) {
                    return new Identity("app", (SensorType) tok[1], name, version);
                }
            }
            return new Identity("app", SensorType.UNKNOWN, name, version);
        }
        return new Identity("unknown", null, name, version);
    }
}
