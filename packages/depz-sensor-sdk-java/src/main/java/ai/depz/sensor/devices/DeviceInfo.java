package ai.depz.sensor.devices;

import ai.depz.sensor.protocol.Identity;
import ai.depz.sensor.usb.UsbIds;
import org.jspecify.annotations.Nullable;

/** What the contract 02 §4.4 probe learned about one port. */
public record DeviceInfo(
        String port,
        Mode mode,
        Identity.@Nullable SensorType sensorType,
        String softwareName,
        String fwVersion,
        String deviceName,
        String serialNumber,
        @Nullable Integer usbVid,
        @Nullable Integer usbPid,
        @Nullable String usbSerial) {
    /** What the firmware name says the board is running (contract 02 §4). */
    public enum Mode {
        APP("app"),
        BOOTLOADER("bootloader"),
        UNKNOWN("unknown");

        /** The name {@link Identity#mode} uses. */
        public final String label;

        Mode(String label) {
            this.label = label;
        }

        /** The mode {@link Identity#mode} names. */
        public static Mode fromLabel(String label) {
            for (Mode m : values()) {
                if (m.label.equals(label)) {
                    return m;
                }
            }
            throw new IllegalArgumentException("no device mode '" + label + "'");
        }
    }

    /** The model the USB-id table gives the port (contract 02 §4.2), or null. */
    public @Nullable String usbModelHint() {
        return UsbIds.usbModelHint(usbVid, usbPid);
    }

    DeviceInfo withUsb(@Nullable Integer vid, @Nullable Integer pid, @Nullable String serial) {
        return new DeviceInfo(port, mode, sensorType, softwareName, fwVersion, deviceName, serialNumber, vid, pid, serial);
    }
}
