package ai.depz.sensor.devices;

import ai.depz.sensor.protocol.Identity;
import ai.depz.sensor.usb.Discovery;
import ai.depz.sensor.usb.UsbIds;
import java.time.Duration;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

/**
 * Opening devices (contract 02 §4, contract 07 §2): the probe, class resolution and continuing a
 * connection as its sensor class.
 */
public final class DepzDevices {
    private DepzDevices() {}

    /** The probe's answers: software name, device name, serial number. */
    public record ProbeResult(String softwareName, String deviceName, String serialNumber) {}

    /**
     * Open a device on a connection that cannot be reopened — a replay or recording link, a link
     * the host opened (contract 02 §4.4). Runs the probe on {@code link}, resolves the class from
     * the answers and the port's USB id (null = unknown; a link knows none) and continues as that
     * class on the same connection. The device owns the link; on failure it is closed.
     *
     * @throws NoDepzDeviceException no answer to GET_NAME_ACTIVE_SOFTWARE
     * @throws BootloaderModeException the device runs its bootloader
     */
    public static DepzDevice openOnLink(Link link, Integer usbVid, Integer usbPid, Duration timeout) {
        DepzDevice dev = new DepzDevice(link, timeout);
        try {
            ProbeResult p = probe(dev);
            return attach(dev, Discovery.resolveSensorClass(p.softwareName(), p.deviceName(), usbVid, usbPid));
        } catch (BootloaderModeException | NoDepzDeviceException e) {
            dev.close();
            throw e;
        } catch (DepzException e) {
            dev.close();
            throw new NoDepzDeviceException("no DEPZ device answered on " + link.name() + " (" + e.getMessage() + ")", e);
        } catch (RuntimeException e) {
            dev.close();
            throw e;
        }
    }

    public static DepzDevice openOnLink(Link link) {
        return openOnLink(link, null, null, null);
    }

    /**
     * Continue the open plain device {@code dev} as {@code className} (a name
     * {@link Discovery#resolveSensorClass} returns) on the same connection — no reopen; the reader
     * and the host sequence numbers run on (contract 02 §4.4). Use the result from here on.
     * "device", and a class this SDK has no device layer for, return {@code dev} itself.
     *
     * @throws BootloaderModeException {@code className} is "bootloader"
     */
    public static DepzDevice attach(DepzDevice dev, String className) {
        if (dev.getClass() != DepzDevice.class) {
            throw new IllegalArgumentException("attach needs a plain DepzDevice, got " + dev.getClass().getSimpleName());
        }
        if (!Discovery.SENSOR_CLASS_NAMES.contains(className)) {
            throw new IllegalArgumentException("no sensor class '" + className + "'");
        }
        return switch (className) {
            case "bootloader" -> throw new BootloaderModeException();
            case "sr04" -> new Sr04Device(dev);
            case "vl53l4cd" -> new Vl53l4cdDevice(dev);
            case "vl53l8cx" -> new Vl53l8cxDevice(dev);
            case "vl53l8ch" -> new Vl53l8chDevice(dev);
            case "vl53l5cx" -> new Vl53l5cxDevice(dev);
            case "vl53l7cx" -> new Vl53l7cxDevice(dev);
            case "vl53l7ch" -> new Vl53l7chDevice(dev);
            case "bno055" -> new Bno055Device(dev);
            case "bno086" -> new Bno086Device(dev);
            case "vl53lx" -> new Vl53lxDevice(dev);
            case "vl53l0x" -> new Vl53l0xDevice(dev);
            case "vl53l1cx" -> new Vl53l1cxDevice(dev);
            case "vl53l1cb" -> new Vl53l1cbDevice(dev);
            case "vl53l3cx" -> new Vl53l3cxDevice(dev);
            case "vl53l4cx" -> new Vl53l4cxDevice(dev);
            case "tmf8829" -> new Tmf8829Device(dev);
            default -> dev;
        };
    }

    /**
     * The contract 02 §4.4 probe on an open connection: GET_NAME_ACTIVE_SOFTWARE (required: its
     * failure throws), then GET_DEVICE_NAME and GET_SERIAL (a timeout or error leaves "").
     */
    public static ProbeResult probe(DepzDevice dev) {
        String software = dev.getSoftwareName();
        String deviceName;
        try {
            deviceName = dev.getDeviceName();
        } catch (DepzException e) {
            deviceName = "";
        }
        String serial;
        try {
            serial = dev.getSerialNumber();
        } catch (DepzException e) {
            serial = "";
        }
        return new ProbeResult(software, deviceName, serial);
    }

    /** Probe one port: open it, run {@link #probe}, close. Null when nothing DEPZ-shaped answers. */
    public static DeviceInfo probePort(String port, Duration timeout) {
        DepzDevice dev;
        try {
            dev = new DepzDevice(port, timeout);
        } catch (DepzException | IllegalArgumentException e) {
            return null;
        }
        try {
            ProbeResult p = probe(dev);
            Identity id = Identity.parseSoftwareName(p.softwareName());
            return new DeviceInfo(port, id.mode(), id.sensorType(), id.softwareName(), id.version(), p.deviceName(),
                    p.serialNumber(), null, null, null);
        } catch (DepzException e) {
            return null;
        } finally {
            dev.close();
        }
    }

    /**
     * Probe the candidate ports and return every DEPZ device found, ordered by USB serial. With
     * {@code matchUsb} only ports with a known DEPZ USB id are probed; {@code ports} (null:
     * enumerate) replaces enumeration with an explicit list.
     */
    public static List<DeviceInfo> listDepzDevices(List<String> ports, boolean matchUsb, Duration timeout) {
        Map<String, SerialPorts.SerialPortInfo> enumerated = new HashMap<>();
        for (SerialPorts.SerialPortInfo p : SerialPorts.list()) {
            enumerated.put(p.port(), p);
        }
        List<SerialPorts.SerialPortInfo> candidates = new ArrayList<>();
        if (ports == null) {
            candidates.addAll(enumerated.values());
        } else {
            for (String p : ports) {
                candidates.add(enumerated.getOrDefault(p, new SerialPorts.SerialPortInfo(p, null, null, null)));
            }
        }
        if (matchUsb) {
            candidates.removeIf(p -> !UsbIds.isKnownDepzUsb(p.vid(), p.pid()));
        }
        List<DeviceInfo> found = new ArrayList<>();
        for (SerialPorts.SerialPortInfo pi : SerialPorts.bySerial(candidates)) {
            DeviceInfo info = probePort(pi.port(), timeout);
            if (info != null) {
                found.add(info.withUsb(pi.vid(), pi.pid(), pi.usbSerial()));
            }
        }
        return found;
    }

    public static List<DeviceInfo> listDepzDevices() {
        return listDepzDevices(null, true, null);
    }

    /**
     * Open the DEPZ device with the smallest USB serial, or the one whose USB serial is
     * {@code serial}, as its sensor class (contract 02 §4.3).
     *
     * @throws NoDepzDeviceException no candidate port
     */
    public static DepzDevice openDevice(String serial, Duration timeout) {
        List<SerialPorts.SerialPortInfo> candidates = depzCandidates();
        if (serial != null) {
            for (SerialPorts.SerialPortInfo p : candidates) {
                if (serial.equals(p.usbSerial())) {
                    return openProbed(p.port(), timeout);
                }
            }
            throw new NoDepzDeviceException("no DEPZ device with USB serial '" + serial + "'");
        }
        if (candidates.isEmpty()) {
            throw new NoDepzDeviceException("no DEPZ device found (no serial port has a known DEPZ USB id). "
                    + "Pass a port path explicitly to open an unprogrammed unit.");
        }
        return openProbed(candidates.get(0).port(), timeout);
    }

    public static DepzDevice openDevice() {
        return openDevice((String) null, null);
    }

    /** Open the {@code index}-th DEPZ candidate, candidates sorted by USB serial. */
    public static DepzDevice openDevice(int index, Duration timeout) {
        List<SerialPorts.SerialPortInfo> candidates = depzCandidates();
        if (index < 0 || index >= candidates.size()) {
            throw new NoDepzDeviceException("no DEPZ candidate at index " + index + " (" + candidates.size()
                    + " candidate(s) found)");
        }
        return openProbed(candidates.get(index).port(), timeout);
    }

    /** Open exactly {@code port} as its sensor class (also a port whose USB id is not a DEPZ one). */
    public static DepzDevice openPort(String port, Duration timeout) {
        return openProbed(port, timeout);
    }

    /** Open the port of a {@link #listDepzDevices} result as the class its probe answers resolve to. */
    public static DepzDevice openDevice(DeviceInfo info, Duration timeout) {
        return openResolved(info, info.usbVid(), info.usbPid(), timeout);
    }

    /** {@link #openDevice(String, Duration)}, and {@link WrongTypeException} unless it is an SR04. */
    public static Sr04Device openSr04(String serial, Duration timeout) {
        return as(Sr04Device.class, openDevice(serial, timeout), "an SR04");
    }

    public static Sr04Device openSr04() {
        return openSr04(null, null);
    }

    /** {@link #openDevice(String, Duration)}, and {@link WrongTypeException} unless it is a VL53L4CD. */
    public static Vl53l4cdDevice openVl53l4cd(String serial, Duration timeout) {
        return as(Vl53l4cdDevice.class, openDevice(serial, timeout), "a VL53L4CD");
    }

    public static Vl53l4cdDevice openVl53l4cd() {
        return openVl53l4cd(null, null);
    }

    /**
     * {@link #openDevice(String, Duration)}, and {@link WrongTypeException} unless it is a VL53L8CX or
     * VL53L8CH ({@link Vl53l8chDevice} extends {@link Vl53l8cxDevice}).
     */
    public static Vl53l8cxDevice openVl53l8(String serial, Duration timeout) {
        return as(Vl53l8cxDevice.class, openDevice(serial, timeout), "a VL53L8CX / VL53L8CH");
    }

    public static Vl53l8cxDevice openVl53l8() {
        return openVl53l8(null, null);
    }

    /**
     * {@link #openDevice(String, Duration)}, and {@link WrongTypeException} unless it is a VL53L5CX,
     * VL53L7CX or VL53L7CH (all extend {@link Vl53l7cxDevice}).
     */
    public static Vl53l7cxDevice openVl53l7(String serial, Duration timeout) {
        return as(Vl53l7cxDevice.class, openDevice(serial, timeout), "a VL53L5CX / VL53L7CX / VL53L7CH");
    }

    public static Vl53l7cxDevice openVl53l7() {
        return openVl53l7(null, null);
    }

    /** {@link #openDevice(String, Duration)}, and {@link WrongTypeException} unless it is a BNO055. */
    public static Bno055Device openBno055(String serial, Duration timeout) {
        return as(Bno055Device.class, openDevice(serial, timeout), "a BNO055");
    }

    public static Bno055Device openBno055() {
        return openBno055(null, null);
    }

    /** {@link #openDevice(String, Duration)}, and {@link WrongTypeException} unless it is a BNO085 / BNO086. */
    public static Bno086Device openBno086(String serial, Duration timeout) {
        return as(Bno086Device.class, openDevice(serial, timeout), "a BNO085 / BNO086");
    }

    public static Bno086Device openBno086() {
        return openBno086(null, null);
    }

    /**
     * {@link #openDevice(String, Duration)}, and {@link WrongTypeException} unless it is a board of the
     * VL53L 1D family (every class extends {@link Vl53lxDevice}).
     */
    public static Vl53lxDevice openVl53lx(String serial, Duration timeout) {
        return as(Vl53lxDevice.class, openDevice(serial, timeout), "a VL53L 1D-family board");
    }

    public static Vl53lxDevice openVl53lx() {
        return openVl53lx(null, null);
    }

    /** {@link #openDevice(String, Duration)}, and {@link WrongTypeException} unless it is a TMF8829. */
    public static Tmf8829Device openTmf8829(String serial, Duration timeout) {
        return as(Tmf8829Device.class, openDevice(serial, timeout), "a TMF8829");
    }

    public static Tmf8829Device openTmf8829() {
        return openTmf8829(null, null);
    }

    static <T extends DepzDevice> T as(Class<T> type, DepzDevice dev, String what) {
        if (type.isInstance(dev)) {
            return type.cast(dev);
        }
        String name = dev.getClass().getSimpleName();
        dev.close();
        throw new WrongTypeException("the device is not " + what + " (" + name + ")");
    }

    private static List<SerialPorts.SerialPortInfo> depzCandidates() {
        List<SerialPorts.SerialPortInfo> all = new ArrayList<>(SerialPorts.list());
        all.removeIf(p -> !UsbIds.isKnownDepzUsb(p.vid(), p.pid()));
        return SerialPorts.bySerial(all);
    }

    private static DepzDevice openProbed(String port, Duration timeout) {
        DeviceInfo info = probePort(port, timeout);
        if (info == null) {
            throw new DeviceLostException("no DEPZ device answered on " + port);
        }
        SerialPorts.SerialPortInfo pi = null;
        for (SerialPorts.SerialPortInfo p : SerialPorts.list()) {
            if (p.port().equals(port)) {
                pi = p;
            }
        }
        return openResolved(info, pi == null ? null : pi.vid(), pi == null ? null : pi.pid(), timeout);
    }

    // The probe ran on a connection of its own (closed); the class opens the port afresh.
    private static DepzDevice openResolved(DeviceInfo info, Integer vid, Integer pid, Duration timeout) {
        String name = Discovery.resolveSensorClass(info.softwareName(), info.deviceName(), vid, pid);
        return switch (name) {
            case "bootloader" -> throw new BootloaderModeException();
            case "sr04" -> new Sr04Device(info.port(), timeout);
            case "vl53l4cd" -> new Vl53l4cdDevice(info.port(), timeout);
            case "vl53l8cx" -> new Vl53l8cxDevice(info.port(), timeout);
            case "vl53l8ch" -> new Vl53l8chDevice(info.port(), timeout);
            case "vl53l5cx" -> new Vl53l5cxDevice(info.port(), timeout);
            case "vl53l7cx" -> new Vl53l7cxDevice(info.port(), timeout);
            case "vl53l7ch" -> new Vl53l7chDevice(info.port(), timeout);
            case "bno055" -> new Bno055Device(info.port(), timeout);
            case "bno086" -> new Bno086Device(info.port(), timeout);
            case "vl53lx" -> new Vl53lxDevice(info.port(), timeout);
            case "vl53l0x" -> new Vl53l0xDevice(info.port(), timeout);
            case "vl53l1cx" -> new Vl53l1cxDevice(info.port(), timeout);
            case "vl53l1cb" -> new Vl53l1cbDevice(info.port(), timeout);
            case "vl53l3cx" -> new Vl53l3cxDevice(info.port(), timeout);
            case "vl53l4cx" -> new Vl53l4cxDevice(info.port(), timeout);
            case "tmf8829" -> new Tmf8829Device(info.port(), timeout);
            default -> new DepzDevice(info.port(), timeout);
        };
    }
}
