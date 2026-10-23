package ai.depz.sensor.usb;

import ai.depz.sensor.protocol.Identity;
import ai.depz.sensor.sensors.vl53l7.Vl53l7Uld;
import ai.depz.sensor.sensors.vl53lx.Vl53lxProducts;

import java.util.List;
import java.util.Locale;

/**
 * Class resolution (contracts/02_COMMON_COMMANDS.md §4.4, Python reference
 * {@code resolve_sensor_class}): which SDK class a device opens as, from its
 * probe answers and its port's USB (vid, pid).
 *
 * <p>This SDK is decode-only (no device layer), so it carries the pure part of
 * §4.4 only: {@link #resolveSensorClass} and {@link #SENSOR_CLASS_NAMES}.
 */
public final class Discovery {
    private Discovery() {}

    /**
     * Every name {@link #resolveSensorClass} returns: the contract 07 sensor
     * classes, {@code "vl53lx"} (the generic 1D-family class: an unstamped
     * board, or a VL53L4CD on the APP_VL53L0_4 firmware), {@code "tmf8829"},
     * {@code "device"} (the base device: an unknown application, or a name that
     * is neither APP_ nor BOOTDEPZ) and {@code "bootloader"} (use the contract
     * 06 client). BNO085 boards run APP_BNO086 and resolve to {@code "bno086"}.
     */
    public static final List<String> SENSOR_CLASS_NAMES = List.of(
        "sr04", "vl53l4cd", "vl53l8cx", "vl53l8ch", "vl53l5cx", "vl53l7cx", "vl53l7ch",
        "vl53l0x", "vl53l1cx", "vl53l1cb", "vl53l3cx", "vl53l4cx", "vl53lx",
        "bno086", "bno055", "tmf8829", "device", "bootloader");

    /**
     * The canonical class name (one of {@link #SENSOR_CLASS_NAMES}) for a
     * device's probe answers and its port's USB id — contract 02 §4.4, pure.
     *
     * <ul>
     *   <li>BOOTDEPZ_* → {@code "bootloader"};</li>
     *   <li>vl53l8 → {@code "vl53l8ch"} if the USB model is VL53L8CH, else {@code "vl53l8cx"};</li>
     *   <li>vl53l7 → {@link Vl53l7Uld#resolveModel} (USB model, else device name, else CX);</li>
     *   <li>vl53lx → {@link Vl53lxProducts#resolveClass} (USB model, else device name, else
     *       the generic {@code "vl53lx"}; VL53L4CD on this firmware is generic too);</li>
     *   <li>any other known token → its class; anything else → {@code "device"}.</li>
     * </ul>
     *
     * @param softwareName the GET_NAME_ACTIVE_SOFTWARE answer (already stripped)
     * @param deviceName   the GET_DEVICE_NAME answer; {@code ""} or {@code null} when none
     * @param usbVid       the port's USB vendor id; {@code null} when unknown (a replay, a bridge)
     * @param usbPid       the port's USB product id; {@code null} when unknown
     */
    public static String resolveSensorClass(
            String softwareName, String deviceName, Integer usbVid, Integer usbPid) {
        // Each USB id, when known, is a u16 (contract 07 §3) — even if the other is not.
        if (usbVid != null) {
            ai.depz.sensor.protocol.Wire.checkUint("usbVid", usbVid, 16);
        }
        if (usbPid != null) {
            ai.depz.sensor.protocol.Wire.checkUint("usbPid", usbPid, 16);
        }
        Identity ident = Identity.parseSoftwareName(softwareName);
        if ("bootloader".equals(ident.mode())) {
            return "bootloader";
        }
        String name = deviceName == null ? "" : deviceName;
        String model = UsbIds.usbModelHint(usbVid, usbPid);
        Identity.SensorType type = ident.sensorType();
        if (type == null) {
            return "device";
        }
        switch (type) {
            case SR04:
                return "sr04";
            case VL53L8:
                // CX and CH share firmware name and silicon ids: only the production
                // PID tells them apart; the dev default (0483:56DC) falls to the CX base.
                return "vl53l8ch".equals(model) ? "vl53l8ch" : "vl53l8cx";
            case VL53L7:
                return Vl53l7Uld.resolveModel(model, name).label;
            case VL53LX:
                return Vl53lxProducts.resolveClass(model, name).className.toLowerCase(Locale.ROOT);
            case VL53L4:
                return "vl53l4cd";
            case BNO086:
                return "bno086";
            case BNO055:
                return "bno055";
            case TMF8829:
                return "tmf8829";
            default:
                return "device";
        }
    }
}
