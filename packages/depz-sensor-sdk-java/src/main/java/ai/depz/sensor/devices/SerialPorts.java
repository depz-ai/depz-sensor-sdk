package ai.depz.sensor.devices;

import com.fazecast.jSerialComm.SerialPort;
import java.util.ArrayList;
import java.util.Comparator;
import java.util.List;

/** Serial port enumeration with USB ids (jSerialComm). */
public final class SerialPorts {
    private SerialPorts() {}

    /** One serial port: its path, and its USB ids and iSerial when it is a USB device (else null). */
    public record SerialPortInfo(String port, Integer vid, Integer pid, String usbSerial) {}

    /** Every serial port the OS lists. */
    public static List<SerialPortInfo> list() {
        List<SerialPortInfo> out = new ArrayList<>();
        for (SerialPort p : SerialPort.getCommPorts()) {
            String path = p.getSystemPortPath();
            // Windows paths come as \\.\COM5: a user knows the port as COM5.
            String port = path.startsWith("\\\\.\\") ? p.getSystemPortName() : path;
            int vid = p.getVendorID();
            int pid = p.getProductID();
            String serial = p.getSerialNumber();
            out.add(new SerialPortInfo(port,
                    vid >= 0 ? vid : null,
                    pid >= 0 ? pid : null,
                    serial == null || serial.isEmpty() || "Unknown".equals(serial) ? null : serial));
        }
        return out;
    }

    /** Ports by USB iSerial ascending, those without one last, ties by path (contract 02 §4). */
    public static List<SerialPortInfo> bySerial(List<SerialPortInfo> ports) {
        List<SerialPortInfo> sorted = new ArrayList<>(ports);
        sorted.sort(Comparator
                .comparingInt((SerialPortInfo p) -> p.usbSerial() == null || p.usbSerial().isEmpty() ? 1 : 0)
                .thenComparing(p -> p.usbSerial() == null ? "" : p.usbSerial())
                .thenComparing(SerialPortInfo::port));
        return sorted;
    }
}
