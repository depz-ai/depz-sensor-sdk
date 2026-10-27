package ai.depz.sensor.test.devices;

import static ai.depz.sensor.test.devices.T.check;
import static ai.depz.sensor.test.devices.T.eq;
import static ai.depz.sensor.test.devices.T.same;
import static ai.depz.sensor.test.devices.T.throwsType;

import ai.depz.sensor.devices.Bno055Device;
import ai.depz.sensor.devices.Bno086Device;
import ai.depz.sensor.devices.BootloaderModeException;
import ai.depz.sensor.devices.DepzDevice;
import ai.depz.sensor.devices.DepzDevices;
import ai.depz.sensor.devices.DepzException;
import ai.depz.sensor.devices.DeviceInfo;
import ai.depz.sensor.devices.LinkStats;
import ai.depz.sensor.devices.NoDepzDeviceException;
import ai.depz.sensor.devices.SerialPorts;
import ai.depz.sensor.devices.Sr04Device;
import ai.depz.sensor.devices.Tmf8829Device;
import ai.depz.sensor.devices.Vl53l0xDevice;
import ai.depz.sensor.devices.Vl53l1cbDevice;
import ai.depz.sensor.devices.Vl53l1cxDevice;
import ai.depz.sensor.devices.Vl53l3cxDevice;
import ai.depz.sensor.devices.Vl53l4cdDevice;
import ai.depz.sensor.devices.Vl53l4cxDevice;
import ai.depz.sensor.devices.Vl53l5cxDevice;
import ai.depz.sensor.devices.Vl53l7chDevice;
import ai.depz.sensor.devices.Vl53l7cxDevice;
import ai.depz.sensor.devices.Vl53l8chDevice;
import ai.depz.sensor.devices.Vl53l8cxDevice;
import ai.depz.sensor.devices.Vl53lxDevice;
import ai.depz.sensor.devices.WrongTypeException;
import ai.depz.sensor.protocol.Common;
import ai.depz.sensor.protocol.Identity;
import ai.depz.sensor.transport.Packet;
import ai.depz.sensor.usb.UsbIds;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.time.Duration;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;

/**
 * Opening devices on a connection of the caller's (contract 02 §4.4, contract 07 §2): the probe, class
 * resolution and continuing as the sensor class — the Python {@code test_discovery.py} open_on_link
 * cases. Port enumeration and opening by path need a serial port and are not here.
 */
public final class DepzDevicesTests {
    /** A board that answers the three probe requests; a null answer is a status error, "" is silence. */
    static final class FakeProbeBoard {
        final FakeBoardLink link = new FakeBoardLink();
        final List<Integer> log = new ArrayList<>();
        String softwareName;
        String deviceName = "DEPZ board";
        String serial = "SN000042";

        FakeProbeBoard(String softwareName) {
            this.softwareName = softwareName;
            link.onPacket = this::onPacket;
        }

        private List<byte[]> onPacket(Packet p) {
            int cmd = p.cmd();
            synchronized (log) {
                log.add(cmd);
            }
            String answer;
            if (cmd == Common.Cmd.GET_NAME_ACTIVE_SOFTWARE.value) {
                answer = softwareName;
            } else if (cmd == Common.Cmd.GET_DEVICE_NAME.value) {
                answer = deviceName;
            } else if (cmd == Common.Cmd.GET_SERIAL.value) {
                answer = serial;
            } else {
                return List.of(FakeBoardLink.status(cmd, Common.Status.OK.value));
            }
            if (answer == null) {
                return List.of(FakeBoardLink.status(cmd, Common.Status.ERR_CMD_NOT_SUPPORTED.value));
            }
            if (answer.isEmpty()) {
                return List.of();
            }
            byte[] t = answer.getBytes(StandardCharsets.US_ASCII);
            byte[] out = new byte[t.length + 2];
            out[0] = (byte) cmd;
            System.arraycopy(t, 0, out, 1, t.length);
            return List.of(link.reply(Common.Rpt.TEXT.value, out));
        }

        List<Integer> log() {
            synchronized (log) {
                return new ArrayList<>(log);
            }
        }
    }

    private static final Duration TIMEOUT = Duration.ofMillis(300);
    private static final List<Integer> PROBE = List.of(Common.Cmd.GET_NAME_ACTIVE_SOFTWARE.value,
            Common.Cmd.GET_DEVICE_NAME.value, Common.Cmd.GET_SERIAL.value);

    public static void findPortMatchesTheExactPathAndNothingTheOsDoesNotList() {
        List<SerialPorts.SerialPortInfo> ports = List.of(
                new SerialPorts.SerialPortInfo("/dev/ttyACM0", UsbIds.DEPZ_USB_VID, UsbIds.PID_VL53L8, "A"),
                new SerialPorts.SerialPortInfo("/dev/ttyACM1", UsbIds.DEPZ_USB_VID, 0xED4A /* VL53L7CH */, "B"));
        eq(ports.get(1), SerialPorts.find(ports, "/dev/ttyACM1"));
        eq(null, SerialPorts.find(ports, "/dev/depz-unlisted"));
        eq(null, SerialPorts.find(List.of(), "/dev/depz-no-such-port"));
    }

    /**
     * A {@code /dev/serial/by-id/…} symlink names the port it points at (ERRATA A5): a regular file
     * stands in for the node, the resolved path matches either way.
     */
    public static void findPortFollowsASymlinkAliasToTheEnumeratedPort() throws Exception {
        Path dir = Files.createTempDirectory("depz-a5-");
        try {
            Path node = dir.resolve("ttyACM0");
            Path alias = dir.resolve("usb-DEPZ_ToF-if00");
            Files.write(node, new byte[0]);
            Files.createSymbolicLink(alias, node);
            List<SerialPorts.SerialPortInfo> ports = List.of(
                    new SerialPorts.SerialPortInfo("/dev/ttyACM9", UsbIds.DEPZ_USB_VID, UsbIds.PID_VL53L8CX, "Z"),
                    new SerialPorts.SerialPortInfo(node.toString(), UsbIds.DEPZ_USB_VID, UsbIds.PID_VL53L8, "SN1"));
            eq(ports.get(1), SerialPorts.find(ports, alias.toString()));
            eq(null, SerialPorts.find(ports, dir.resolve("other").toString()));
            // listDepzDevices keeps the path given and annotates it with the node's USB id — and so
            // would probe the alias with the USB filter on; here nothing answers, so nothing is listed.
            eq(List.of(), DepzDevices.listDepzDevices(List.of(alias.toString()), true, TIMEOUT));
        } finally {
            try (var walk = Files.walk(dir)) {
                for (Path p : walk.sorted(java.util.Comparator.reverseOrder()).toList()) {
                    Files.deleteIfExists(p);
                }
            }
        }
    }

    public static void openOnLinkProbesThreeRequestsThenAttaches() throws Exception {
        FakeProbeBoard fake = new FakeProbeBoard("APP_usonic_SR04_v0.97");
        try (DepzDevice dev = DepzDevices.openOnLink(fake.link, UsbIds.DEPZ_USB_VID, UsbIds.PID_SR04, TIMEOUT)) {
            eq(Sr04Device.class, dev.getClass());
            eq(PROBE, fake.log()); // name, device name, serial — nothing else
            eq("DEPZ board", dev.getDeviceName()); // the same connection goes on
            eq("fake", dev.port());
            eq(TIMEOUT, dev.timeout());
            LinkStats s = dev.stats();
            eq(4L, s.txPackets());
            eq(4L, s.rxPackets());
            check(s.txBytes() > 0 && s.rxBytes() > 0, "bytes counted");
            eq(List.of(0L, 0L, 0L, 0L, 0L), List.of(s.crcErrors(), s.headerErrors(), s.trashBytes(), s.seqGaps(), s.deviceSeqErrors()));
        }
        check(fake.link.isClosed(), "the device owns the link");
    }

    public static void openOnLinkUnknownAppStaysBaseDevice() throws Exception {
        FakeProbeBoard fake = new FakeProbeBoard("APP_NEWSENSOR_v1.2");
        try (DepzDevice dev = DepzDevices.openOnLink(fake.link)) {
            eq(DepzDevice.class, dev.getClass());
            eq(PROBE, fake.log());
            eq(DepzDevice.DEFAULT_TIMEOUT, dev.timeout());
        }
    }

    public static void openOnLinkRefusesBootloader() throws Exception {
        FakeProbeBoard fake = new FakeProbeBoard("BOOTDEPZ_v1.1");
        throwsType(BootloaderModeException.class, () -> DepzDevices.openOnLink(fake.link, null, null, TIMEOUT));
        check(fake.link.isClosed(), "closed on failure");
    }

    public static void openOnLinkWithNoAnswerIsNoDepzDevice() throws Exception {
        FakeProbeBoard silent = new FakeProbeBoard("");
        NoDepzDeviceException e = throwsType(NoDepzDeviceException.class,
                () -> DepzDevices.openOnLink(silent.link, null, null, Duration.ofMillis(50)));
        check(e.getMessage().contains("no DEPZ device answered on fake"), e.getMessage());
        check(silent.link.isClosed(), "closed on failure");

        FakeProbeBoard refusing = new FakeProbeBoard(null);
        throwsType(NoDepzDeviceException.class, () -> DepzDevices.openOnLink(refusing.link, null, null, TIMEOUT));
        check(refusing.link.isClosed(), "closed on failure");
    }

    public static void openOnLinkClosesTheLinkOnAnArgumentError() throws Exception {
        FakeProbeBoard fake = new FakeProbeBoard("APP_usonic_SR04_v0.97");
        throwsType(IllegalArgumentException.class, () -> DepzDevices.openOnLink(fake.link, 0x10000, UsbIds.PID_SR04, TIMEOUT));
        check(fake.link.isClosed(), "closed on failure");
    }

    public static void theProbeTakesTheOptionalAnswersAsEmptyWhenTheyFail() throws Exception {
        FakeProbeBoard fake = new FakeProbeBoard("APP_usonic_SR04_v0.97");
        fake.deviceName = null; // a status error
        fake.serial = "";       // no answer
        try (DepzDevice dev = new DepzDevice(fake.link, Duration.ofMillis(50))) {
            eq(new DepzDevices.ProbeResult("APP_usonic_SR04_v0.97", "", ""), DepzDevices.probe(dev));
        }
    }

    public static void openOnLinkResolvesEveryClassFromTheNameTheDeviceNameAndThePid() throws Exception {
        Object[][] cases = {
            {"APP_VL53L4_v0.83", "x", null, Vl53l4cdDevice.class},
            {"APP_VL53L8_v0.92", "x", null, Vl53l8cxDevice.class},
            {"APP_VL53L8_v0.92", "x", UsbIds.PID_VL53L8, Vl53l8chDevice.class},
            {"APP_VL53L7_v0.53", "DEPZ ToF Sensor VL53L7CX USB v2.1 TXK5KAX6X4", null, Vl53l7cxDevice.class},
            {"APP_VL53L7_v0.53", "x", 0xED48, Vl53l5cxDevice.class},
            {"APP_VL53L7_v0.53", "x", 0xED4A, Vl53l7chDevice.class},
            {"APP_VL53L0_4_v0.25", Vl53lxDeviceTests.L1CX_BOARD, null, Vl53l1cxDevice.class},
            {"APP_VL53L0_4_v0.25", Vl53lxDeviceTests.UNSTAMPED, null, Vl53lxDevice.class},
            {"APP_VL53L0_4_v0.25", Vl53lxDeviceTests.L4CD_BOARD, null, Vl53lxDevice.class},
            {"APP_VL53L0_4_v0.25", "x", 0xED41, Vl53l0xDevice.class},
            {"APP_VL53L0_4_v0.25", "x", 0xED42, Vl53l1cbDevice.class},
            {"APP_VL53L0_4_v0.25", "x", 0xED44, Vl53l3cxDevice.class},
            {"APP_VL53L0_4_v0.25", "x", 0xED46, Vl53l4cxDevice.class},
            {"APP_BNO055_v0.12", "x", null, Bno055Device.class},
            {"APP_BNO086_v1.06", "x", null, Bno086Device.class},
            {"APP_TMF8829_v0.10", "x", null, Tmf8829Device.class},
        };
        for (Object[] c : cases) {
            FakeProbeBoard fake = new FakeProbeBoard((String) c[0]);
            fake.deviceName = (String) c[1];
            Integer pid = (Integer) c[2];
            try (DepzDevice dev = DepzDevices.openOnLink(fake.link, pid == null ? null : UsbIds.DEPZ_USB_VID, pid, TIMEOUT)) {
                eq(c[3], dev.getClass(), c[0] + " / " + c[1] + " / " + pid);
                eq(PROBE, fake.log(), (String) c[0]);
            }
        }
    }

    public static void attachContinuesAPlainDeviceAsTheNamedClass() throws Exception {
        Map<String, Class<?>> want = Map.ofEntries(
            Map.entry("sr04", Sr04Device.class), Map.entry("vl53l4cd", Vl53l4cdDevice.class),
            Map.entry("vl53l8cx", Vl53l8cxDevice.class), Map.entry("vl53l8ch", Vl53l8chDevice.class),
            Map.entry("vl53l5cx", Vl53l5cxDevice.class), Map.entry("vl53l7cx", Vl53l7cxDevice.class),
            Map.entry("vl53l7ch", Vl53l7chDevice.class), Map.entry("bno055", Bno055Device.class),
            Map.entry("bno086", Bno086Device.class), Map.entry("vl53lx", Vl53lxDevice.class),
            Map.entry("vl53l0x", Vl53l0xDevice.class), Map.entry("vl53l1cx", Vl53l1cxDevice.class),
            Map.entry("vl53l1cb", Vl53l1cbDevice.class), Map.entry("vl53l3cx", Vl53l3cxDevice.class),
            Map.entry("vl53l4cx", Vl53l4cxDevice.class), Map.entry("tmf8829", Tmf8829Device.class),
            Map.entry("device", DepzDevice.class));
        for (Map.Entry<String, Class<?>> kv : want.entrySet()) {
            FakeProbeBoard fake = new FakeProbeBoard("APP_usonic_SR04_v0.97");
            DepzDevice plain = new DepzDevice(fake.link, TIMEOUT);
            try (DepzDevice dev = DepzDevices.attach(plain, kv.getKey())) {
                eq(kv.getValue(), dev.getClass(), kv.getKey());
                eq("SN000042", dev.getSerialNumber(), kv.getKey()); // the reader and the sequence numbers run on
                check(kv.getKey().equals("device") == same(dev, plain), kv.getKey());
            }
        }
    }

    public static void attachRefusesTheBootloaderUnknownNamesAndASensorClass() throws Exception {
        FakeProbeBoard fake = new FakeProbeBoard("APP_usonic_SR04_v0.97");
        try (DepzDevice plain = new DepzDevice(fake.link, TIMEOUT)) {
            throwsType(BootloaderModeException.class, () -> DepzDevices.attach(plain, "bootloader"));
            IllegalArgumentException e = throwsType(IllegalArgumentException.class, () -> DepzDevices.attach(plain, "vl53l9zz"));
            check(e.getMessage().contains("no sensor class 'vl53l9zz'"), e.getMessage());
            DepzDevice sr04 = DepzDevices.attach(plain, "sr04");
            e = throwsType(IllegalArgumentException.class, () -> DepzDevices.attach(sr04, "sr04"));
            check(e.getMessage().contains("attach needs a plain DepzDevice, got Sr04Device"), e.getMessage());
        }
    }

    public static void deviceInfoCarriesTheUsbModelHint() {
        DeviceInfo info = new DeviceInfo("COM3", "app", Identity.SensorType.SR04, "APP_usonic_SR04_v0.97", "0.97",
                "DEPZ board", "SN000042", UsbIds.DEPZ_USB_VID, UsbIds.PID_SR04, "USB42");
        eq("sr04", info.usbModelHint());
        eq("COM3", info.port());
        eq(null, new DeviceInfo("COM3", "app", null, "", "", "", "", null, null, null).usbModelHint());
    }

    public static void wrongTypeIsADepzException() {
        DepzException e = new WrongTypeException("the device is not an SR04 (Vl53l8cxDevice)");
        check(e.getMessage().contains("not an SR04"), e.getMessage());
    }
}
