package ai.depz.sensor.devices;

import com.fazecast.jSerialComm.SerialPort;
import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.InvalidPathException;
import java.nio.file.Path;
import java.nio.file.attribute.BasicFileAttributes;
import java.util.ArrayList;
import java.util.Comparator;
import java.util.List;

/** Serial port enumeration with USB ids (jSerialComm). */
public final class SerialPorts {
    private static final boolean WINDOWS = System.getProperty("os.name", "").startsWith("Windows");

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

    /**
     * The enumerated port {@code port} names, under any alias (contract 02 §4.3, ERRATA A5): the one
     * whose path is the same string, else — on POSIX — the one whose resolved path is the same
     * ({@code /dev/serial/by-id/…} is a symlink to the node), else the one that is the same character
     * device under another name. On Windows a COM name is matched by string only: a COM port is not
     * a file, and resolving one (toRealPath, readAttributes) opens it — a DTR pulse on every board of
     * the bench. Null when the OS does not list the port.
     */
    public static SerialPortInfo find(List<SerialPortInfo> ports, String port) {
        for (SerialPortInfo p : ports) {
            if (p.port().equals(port)) {
                return p;
            }
        }
        if (WINDOWS) {
            return null;
        }
        for (SerialPortInfo p : ports) {
            if (samePort(port, p.port())) {
                return p;
            }
        }
        return null;
    }

    private static boolean samePort(String a, String b) {
        try {
            if (Path.of(a).toRealPath().equals(Path.of(b).toRealPath())) {
                return true;
            }
        } catch (IOException | InvalidPathException e) {
            // not a path the OS resolves (a COM name, a port that is gone)
        }
        Object nodeA = deviceNode(a);
        return nodeA != null && nodeA.equals(deviceNode(b));
    }

    /**
     * The device number of the character device at {@code path}; null for anything else. A zero
     * device number is null too: a platform that stats every port as an empty character device must
     * not make them one device.
     */
    private static Object deviceNode(String path) {
        try {
            Path p = Path.of(path);
            if (!Files.readAttributes(p, BasicFileAttributes.class).isOther()) {
                return null; // a regular file, a directory
            }
            Object rdev = Files.getAttribute(p, "unix:rdev");
            return Long.valueOf(0).equals(rdev) ? null : rdev;
        } catch (IOException | UnsupportedOperationException | IllegalArgumentException e) {
            return null; // missing, or a platform without device nodes
        }
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
