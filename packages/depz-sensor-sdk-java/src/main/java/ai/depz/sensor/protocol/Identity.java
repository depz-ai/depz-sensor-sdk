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
        /** VL53L5CX / VL53L7CX / VL53L7CH board (contracts/11_SENSOR_VL53L7.md). */
        VL53L7("vl53l7"),
        /**
         * VL53L0X / L1CX / L1CB / L3CX / L4CD / L4CX on the 1D-family bridge
         * (contracts/12_SENSOR_VL53LX.md).
         */
        VL53LX("vl53lx"),
        BNO086("bno086"),
        /** BNO055 9-axis IMU on the register bridge (contracts/13_SENSOR_BNO055.md). */
        BNO055("bno055"),
        UNKNOWN("unknown");

        public final String label;
        SensorType(String label) { this.label = label; }
    }

    private static final Pattern VERSION_RE = Pattern.compile("_v(\\d+(?:\\.\\d+)*)$");

    private static final Object[][] PRODUCT_TOKENS = {
        {"SR04", SensorType.SR04},
        {"VL53L4", SensorType.VL53L4},
        {"VL53L8", SensorType.VL53L8},
        {"VL53L7", SensorType.VL53L7},
        // The 1D-family bridge: boards answer APP_VL53L0_4_v*, the protocol spec
        // calls it APP_VL53LX_v* (contract 12).
        {"VL53L0_4", SensorType.VL53LX},
        {"VL53LX", SensorType.VL53LX},
        {"BNO086", SensorType.BNO086},
        {"BNO055", SensorType.BNO055},
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
