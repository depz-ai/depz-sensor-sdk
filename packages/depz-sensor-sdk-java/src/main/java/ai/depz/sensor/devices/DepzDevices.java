package ai.depz.sensor.devices;

import ai.depz.sensor.protocol.Identity;
import ai.depz.sensor.usb.Discovery;
import ai.depz.sensor.usb.UsbIds;
import java.time.Duration;
import java.util.ArrayList;
import java.util.List;
import java.util.Objects;
import org.jspecify.annotations.Nullable;

/**
 * Opening devices (contract 02 §4, contract 07 §2): the probe, class resolution and continuing a
 * connection as its sensor class.
 */
public final class DepzDevices {
    private DepzDevices() {}

    /** The probe's answers: software name, device name, serial number. */
    record ProbeResult(String softwareName, String deviceName, String serialNumber) {}

    /**
     * Open a device on a connection that cannot be reopened — a replay or recording link, a link
     * the host opened (contract 02 §4.4). Runs the probe on {@code link}, resolves the class from
     * the answers and the port's USB id (null = unknown; a link knows none) and continues as that
     * class on the same connection. The device owns the link; on failure it is closed.
     *
     * @throws NoDepzDeviceException no answer to GET_NAME_ACTIVE_SOFTWARE
     * @throws BootloaderModeException the device runs its bootloader
     */
    public static DepzDevice openOnLink(Link link, @Nullable Integer usbVid, @Nullable Integer usbPid, @Nullable Duration timeout) {
        DepzDevice dev = new DepzDevice(link, timeout);
        try {
            ProbeResult p = probe(dev);
            return attachSensorClass(dev,
                    Discovery.resolveSensorClass(p.softwareName(), p.deviceName(), usbVid, usbPid));
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
     * "device", and a class this SDK has no device layer for, return {@code dev} itself. This is
     * the one place a sensor class is chosen: every {@code openDevice…} ends here.
     *
     * @throws BootloaderModeException {@code className} is "bootloader"
     */
    public static DepzDevice attachSensorClass(DepzDevice dev, String className) {
        if (dev.getClass() != DepzDevice.class) {
            throw new IllegalArgumentException("attachSensorClass needs a plain DepzDevice, got "
                    + dev.getClass().getSimpleName());
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
    static ProbeResult probe(DepzDevice dev) {
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
    public static @Nullable DeviceInfo probePort(String port, @Nullable Duration timeout) {
        try {
            return probeOpening(port, timeout);
        } catch (DepzException | IllegalArgumentException e) {
            return null;
        }
    }

    /**
     * {@link #probePort}, except that a port that cannot be opened (busy, no permission, gone) throws
     * the open failure instead of returning null — so {@link #openPort} can name it (contract 07 §3).
     */
    private static @Nullable DeviceInfo probeOpening(String port, @Nullable Duration timeout) {
        return probeInfo(new DepzDevice(new SerialPortLink(port), timeout), null, null, null);
    }

    /**
     * Probe a connection the caller opened — a link the host supplies (an Android USB connection) —
     * as {@link #probePort} probes a port, without opening a sensor class and <b>without closing
     * {@code link}</b>: it stays the caller's, for {@link #openOnLink} or to close. {@link
     * DeviceInfo#port} is {@code link.name()}; the USB ids the caller knows are carried into the result
     * as given (null = unknown). Null when nothing DEPZ-shaped answers — no answer to
     * GET_NAME_ACTIVE_SOFTWARE, an error reply, a loss. A board in its bootloader is listed with
     * {@link DeviceInfo.Mode#BOOTLOADER}. The probe's reader stops reading {@code link} before this
     * returns; what arrives meanwhile (a late reply) is dropped.
     */
    public static @Nullable DeviceInfo probeOnLink(Link link, @Nullable Integer usbVid, @Nullable Integer usbPid,
            @Nullable String usbSerial, @Nullable Duration timeout) {
        return probeInfo(new DepzDevice(new BorrowedLink(Objects.requireNonNull(link, "link")), timeout),
                usbVid, usbPid, usbSerial);
    }

    /** {@link #probe} on {@code dev}, assembled into a {@link DeviceInfo}; {@code dev} is closed. */
    private static @Nullable DeviceInfo probeInfo(DepzDevice dev, @Nullable Integer usbVid, @Nullable Integer usbPid,
            @Nullable String usbSerial) {
        try {
            ProbeResult p = probe(dev);
            Identity id = Identity.parseSoftwareName(p.softwareName());
            return new DeviceInfo(dev.port(), DeviceInfo.Mode.fromLabel(id.mode()), id.sensorType(), id.softwareName(),
                    id.version(), p.deviceName(), p.serialNumber(), usbVid, usbPid, usbSerial);
        } catch (DepzException e) {
            return null;
        } finally {
            dev.close();
        }
    }

    /**
     * A link lent to a probe: closing it stops the probe's use of it and leaves {@code inner} open. A
     * read that returns after the close drops its bytes and ends the probe's reader.
     */
    private static final class BorrowedLink implements Link {
        private final Link inner;
        private volatile boolean closed;

        BorrowedLink(Link inner) {
            this.inner = inner;
        }

        @Override
        public byte[] read(Duration timeout) {
            if (closed) {
                throw new LinkClosedException(name() + ": the probe is done with it");
            }
            byte[] data = inner.read(timeout);
            if (closed) {
                throw new LinkClosedException(name() + ": the probe is done with it");
            }
            return data;
        }

        @Override
        public void write(byte[] data) {
            if (closed) {
                throw new LinkClosedException(name() + ": the probe is done with it");
            }
            inner.write(data);
        }

        @Override
        public void close() {
            closed = true;
        }

        @Override
        public boolean isClosed() {
            return closed || inner.isClosed();
        }

        @Override
        public String name() {
            return inner.name();
        }
    }

    /**
     * Probe the candidate ports and return every DEPZ device found, ordered by USB serial. With
     * {@code matchUsb} only ports with a known DEPZ USB id are probed; {@code ports} (null:
     * enumerate) replaces enumeration with an explicit list.
     */
    public static List<DeviceInfo> listDepzDevices(@Nullable List<String> ports, boolean matchUsb, @Nullable Duration timeout) {
        List<SerialPorts.SerialPortInfo> enumerated = SerialPorts.list();
        List<SerialPorts.SerialPortInfo> candidates = new ArrayList<>();
        if (ports == null) {
            candidates.addAll(enumerated);
        } else {
            for (String p : ports) {
                candidates.add(portId(enumerated, p));
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
     * Open the DEPZ device whose USB serial is {@code serial} as its sensor class (contract 02 §4.3).
     *
     * @throws NoDepzDeviceException no candidate port has that USB serial
     */
    public static DepzDevice openDeviceBySerial(String serial, @Nullable Duration timeout) {
        Objects.requireNonNull(serial, "serial");
        for (SerialPorts.SerialPortInfo p : depzCandidates()) {
            if (serial.equals(p.usbSerial())) {
                return openProbed(p.port(), timeout);
            }
        }
        throw new NoDepzDeviceException("no DEPZ device with USB serial '" + serial + "'");
    }

    /**
     * Open the {@code index}-th DEPZ candidate as its sensor class, candidates sorted by USB serial
     * (contract 02 §4.3).
     *
     * @throws NoDepzDeviceException no candidate port, or none at {@code index}
     */
    public static DepzDevice openDevice(int index, @Nullable Duration timeout) {
        List<SerialPorts.SerialPortInfo> candidates = depzCandidates();
        if (candidates.isEmpty()) {
            throw new NoDepzDeviceException("no DEPZ device found (no serial port has a known DEPZ USB id). "
                    + "Pass a port path explicitly to open an unprogrammed unit.");
        }
        if (index < 0 || index >= candidates.size()) {
            throw new NoDepzDeviceException("no DEPZ candidate at index " + index + " (" + candidates.size()
                    + " candidate(s) found)");
        }
        return openProbed(candidates.get(index).port(), timeout);
    }

    /** Open the DEPZ device with the smallest USB serial as its sensor class. */
    public static DepzDevice openDevice() {
        return openDevice(0, null);
    }

    /**
     * Open exactly {@code port} as its sensor class (also a port whose USB id is not a DEPZ one). The
     * USB id is that of the enumerated port the path names under any alias ({@code /dev/serial/by-id/…};
     * contract 02 §4.3).
     */
    public static DepzDevice openPort(String port, @Nullable Duration timeout) {
        return openProbed(port, timeout);
    }

    /** Open the port of a {@link #listDepzDevices} result as the class its probe answers resolve to. */
    public static DepzDevice openDevice(DeviceInfo info, @Nullable Duration timeout) {
        return openResolved(info, info.usbVid(), info.usbPid(), timeout);
    }

    private static List<SerialPorts.SerialPortInfo> depzCandidates() {
        List<SerialPorts.SerialPortInfo> all = new ArrayList<>(SerialPorts.list());
        all.removeIf(p -> !UsbIds.isKnownDepzUsb(p.vid(), p.pid()));
        return SerialPorts.bySerial(all);
    }

    /**
     * {@code port} with the USB identity of the enumerated port it names under any alias
     * ({@link SerialPorts#find}); none when the OS does not list it. The port stays {@code port}: an
     * explicit path is opened as given.
     */
    private static SerialPorts.SerialPortInfo portId(List<SerialPorts.SerialPortInfo> enumerated, String port) {
        SerialPorts.SerialPortInfo found = SerialPorts.find(enumerated, port);
        return found == null
                ? new SerialPorts.SerialPortInfo(port, null, null, null)
                : new SerialPorts.SerialPortInfo(port, found.vid(), found.pid(), found.usbSerial());
    }

    private static DepzDevice openProbed(String port, @Nullable Duration timeout) {
        DeviceInfo info;
        try {
            info = probeOpening(port, timeout);
        } catch (DepzException | IllegalArgumentException e) {
            throw new NoDepzDeviceException(
                    "no DEPZ device answered on " + port + ": could not open the port (" + e.getMessage() + ")", e);
        }
        if (info == null) {
            throw new NoDepzDeviceException("no DEPZ device answered on " + port);
        }
        SerialPorts.SerialPortInfo pi = portId(SerialPorts.list(), port);
        return openResolved(info, pi.vid(), pi.pid(), timeout);
    }

    // The probe ran on a connection of its own (closed); the class opens the port afresh.
    private static DepzDevice openResolved(DeviceInfo info, @Nullable Integer vid, @Nullable Integer pid, @Nullable Duration timeout) {
        String name = Discovery.resolveSensorClass(info.softwareName(), info.deviceName(), vid, pid);
        if (name.equals("bootloader")) {
            throw new BootloaderModeException();
        }
        DepzDevice dev = new DepzDevice(new SerialPortLink(info.port()), timeout);
        try {
            return attachSensorClass(dev, name);
        } catch (RuntimeException e) {
            dev.close();
            throw e;
        }
    }
}
