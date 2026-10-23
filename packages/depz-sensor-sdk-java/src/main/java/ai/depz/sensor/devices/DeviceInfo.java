package ai.depz.sensor.devices;

import ai.depz.sensor.protocol.Identity;
import ai.depz.sensor.usb.UsbIds;

/** What the contract 02 §4.4 probe learned about one port. */
public record DeviceInfo(
        String port,
        String mode,
        Identity.SensorType sensorType,
        String softwareName,
        String fwVersion,
        String deviceName,
        String serialNumber,
        Integer usbVid,
        Integer usbPid,
        String usbSerial) {
    /** The model the USB-id table gives the port (contract 02 §4.2), or null. */
    public String usbModelHint() {
        return UsbIds.usbModelHint(usbVid, usbPid);
    }

    DeviceInfo withUsb(Integer vid, Integer pid, String serial) {
        return new DeviceInfo(port, mode, sensorType, softwareName, fwVersion, deviceName, serialNumber, vid, pid, serial);
    }
}
