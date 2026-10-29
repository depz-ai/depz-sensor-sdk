using System.Text.Json;
using Depz.Sensor.Devices;
using Depz.Sensor.Protocol;
using Depz.Sensor.Usb;
using Xunit;

namespace Depz.Sensor.Tests;

/// <summary>
/// <see cref="DepzDevices"/> without a serial port: the probe and class resolution over a fake link, the
/// record a probe yields, and every port-side entry point against a path that does not exist — the error
/// each must turn that into, and that no other port is touched on the way (the Python test_discovery.py
/// cases that need no enumeration fake).
/// </summary>
public class DepzDevicesTests
{
    private static readonly TimeSpan Short = TimeSpan.FromMilliseconds(200);
    private const string Software = "APP_usonic_SR04_v0.97";
    private const string Name = "Ultrasonic Sensor SR04 USB v2.1";

    /// <summary>A path no OS has a serial port at.</summary>
    private static readonly string NoPort = Path.Combine(Path.GetTempPath(), "depz-no-such-port-" + Guid.NewGuid().ToString("N"));

    private static byte[] Text(int cmd, string s) =>
        new[] { (byte)cmd }.Concat(System.Text.Encoding.ASCII.GetBytes(s)).Concat(new byte[] { 0, 0xFF }).ToArray();

    /// <summary>A link that answers the three probe requests like an SR04 board.</summary>
    private static FakeBoardLink Sr04Link(string software = Software, string? deviceName = Name, string serial = "SER42")
    {
        var link = new FakeBoardLink();
        link.OnPacket = p => p.Cmd switch
        {
            (int)Cmd.GetNameActiveSoftware => new[] { link.Reply((int)Rpt.Text, Text(p.Cmd, software)) },
            (int)Cmd.GetDeviceName => deviceName is null
                ? new[] { FakeBoardLink.Status(p.Cmd, Protocol.Status.ErrCmdNotSupported) }
                : new[] { link.Reply((int)Rpt.Text, Text(p.Cmd, deviceName)) },
            (int)Cmd.GetSerial => new[] { link.Reply((int)Rpt.Text, Text(p.Cmd, serial)) },
            _ => new[] { FakeBoardLink.Status(p.Cmd, Protocol.Status.ErrInvalidCmd) },
        };
        return link;
    }

    private static DepzDevice Plain() => new(new FakeBoardLink(), Short);

    // ── DeviceInfo ───────────────────────────────────────────────────────────

    [Fact]
    public void DeviceInfoIsAPlainRecordWithTheUsbModelHint()
    {
        var info = new DeviceInfo("/dev/ttyACM0", "app", SensorType.Sr04, "APP_usonic_SR04", "0.97", Name, "SER42");
        Assert.Null(info.UsbVid);
        Assert.Null(info.UsbPid);
        Assert.Null(info.UsbSerial);
        Assert.Null(info.UsbModelHint);
        var stamped = info with { UsbVid = UsbIds.DepzUsbVid, UsbPid = UsbIds.PidVl53l8ch, UsbSerial = "A" };
        Assert.Equal("vl53l8ch", stamped.UsbModelHint);
        Assert.Equal(("/dev/ttyACM0", "app", SensorType.Sr04, "0.97"), (stamped.Port, stamped.Mode, stamped.SensorType, stamped.FwVersion));
        Assert.Equal(("APP_usonic_SR04", Name, "SER42"), (stamped.SoftwareName, stamped.DeviceName, stamped.SerialNumber));
        Assert.NotEqual(info, stamped);
        Assert.Equal(info, stamped with { UsbVid = null, UsbPid = null, UsbSerial = null });
        Assert.Contains("ttyACM0", info.ToString());
    }

    // ── the probe and the class on a link ────────────────────────────────────

    [Fact]
    public void ProbeLinkReturnsWhatTheThreeRequestsSaidAndClosesTheLink()
    {
        var link = Sr04Link();
        var info = DepzDevices.ProbeLink(link, Short);
        Assert.NotNull(info);
        // the software name stays whole (version included), as the Python Identity keeps it
        Assert.Equal(("fake", "app", SensorType.Sr04, Software, "0.97", Name, "SER42"),
            (info!.Port, info.Mode, info.SensorType, info.SoftwareName, info.FwVersion, info.DeviceName, info.SerialNumber));
        Assert.Null(info.UsbVid); // a link knows no USB id
        Assert.True(link.IsClosed);
        Assert.Equal(new[] { (int)Cmd.GetNameActiveSoftware, (int)Cmd.GetDeviceName, (int)Cmd.GetSerial }, link.Sent.Select(p => p.Cmd));
    }

    [Fact]
    public void ProbeLinkIsNullWhenNothingDepzShapedAnswers()
    {
        var silent = new FakeBoardLink();
        Assert.Null(DepzDevices.ProbeLink(silent, Short));
        Assert.True(silent.IsClosed);
    }

    [Fact]
    public void ProbeLeavesTheOptionalAnswersEmptyWhenTheBoardLacksThem()
    {
        using var dev = new DepzDevice(Sr04Link(deviceName: null), Short);
        var (software, deviceName, serial) = DepzDevices.Probe(dev);
        Assert.Equal((Software, "", "SER42"), (software, deviceName, serial));
        Assert.Throws<ArgumentNullException>(() => DepzDevices.Probe(null!));
    }

    [Fact]
    public void AttachContinuesAPlainDeviceAsTheNamedClassOnTheSameConnection()
    {
        var want = new Dictionary<string, Type>
        {
            ["sr04"] = typeof(Sr04Device), ["vl53l4cd"] = typeof(Vl53l4cdDevice), ["vl53l8cx"] = typeof(Vl53l8cxDevice),
            ["vl53l8ch"] = typeof(Vl53l8chDevice), ["vl53l5cx"] = typeof(Vl53l5cxDevice), ["vl53l7cx"] = typeof(Vl53l7cxDevice),
            ["vl53l7ch"] = typeof(Vl53l7chDevice), ["bno055"] = typeof(Bno055Device), ["bno086"] = typeof(Bno086Device),
            ["vl53lx"] = typeof(Vl53lxDevice), ["vl53l0x"] = typeof(Vl53l0xDevice), ["vl53l1cx"] = typeof(Vl53l1cxDevice),
            ["vl53l1cb"] = typeof(Vl53l1cbDevice), ["vl53l3cx"] = typeof(Vl53l3cxDevice), ["vl53l4cx"] = typeof(Vl53l4cxDevice),
            ["tmf8829"] = typeof(Tmf8829Device), ["device"] = typeof(DepzDevice),
        };
        Assert.Equal(Discovery.SensorClassNames.Where(n => n != "bootloader").OrderBy(n => n), want.Keys.OrderBy(n => n));
        foreach (var (name, type) in want)
        {
            var plain = Plain();
            using var dev = DepzDevices.Attach(plain, name);
            Assert.IsType(type, dev);
            if (name == "device")
                Assert.Same(plain, dev);
        }
    }

    [Fact]
    public void AttachRefusesTheBootloaderANonPlainDeviceAndAnUnknownClass()
    {
        using var plain = Plain();
        Assert.Throws<BootloaderModeException>(() => DepzDevices.Attach(plain, "bootloader"));
        Assert.Contains("no sensor class 'ultrasonic'", Assert.Throws<ArgumentException>(() => DepzDevices.Attach(plain, "ultrasonic")).Message);
        using var sr04 = DepzDevices.Attach(Plain(), "sr04");
        Assert.Contains("plain DepzDevice", Assert.Throws<ArgumentException>(() => DepzDevices.Attach(sr04, "sr04")).Message);
        Assert.Throws<ArgumentNullException>(() => DepzDevices.Attach(null!, "sr04"));
    }

    [Fact]
    public void ExpectClosesAndNamesTheClassOfTheWrongDevice()
    {
        var dev = DepzDevices.Attach(Plain(), "vl53l4cd");
        var e = Assert.Throws<WrongTypeException>(() => DepzDevices.Expect<Sr04Device>(dev, "an SR04"));
        Assert.Equal("the device is not an SR04 (Vl53l4cdDevice)", e.Message);
        Assert.True(dev.IsClosed);
        using var right = DepzDevices.Attach(Plain(), "sr04");
        Assert.Same(right, DepzDevices.Expect<Sr04Device>(right, "an SR04"));
    }

    // ── the port-side entry points, with no port to open ─────────────────────

    [Fact]
    public void ProbePortIsNullWhenThePortCannotBeOpened()
    {
        Assert.Null(DepzDevices.ProbePort(NoPort, Short));
    }

    [Fact]
    public void ListDepzDevicesProbesOnlyTheNamedPortsAndSkipsWhatDoesNotAnswer()
    {
        Assert.Empty(DepzDevices.ListDepzDevices(new[] { NoPort }, matchUsb: false, timeout: Short));
        // with the USB filter on, a port the OS does not list has no known id: never probed
        Assert.Empty(DepzDevices.ListDepzDevices(new[] { NoPort }, timeout: Short));
    }

    [Fact]
    public void FindMatchesTheExactPathAndNothingTheOsDoesNotList()
    {
        var ports = new[] { new SerialPortInfo("/dev/ttyACM0", 0x1BCF, 0xED40, "A"), new SerialPortInfo("/dev/ttyACM1", 0x1BCF, 0xED4A, "B") };
        Assert.Same(ports[1], SerialPorts.Find(ports, "/dev/ttyACM1"));
        Assert.Null(SerialPorts.Find(ports, "/dev/depz-unlisted"));
        Assert.Null(SerialPorts.Find(Array.Empty<SerialPortInfo>(), NoPort));
    }

    [Fact]
    public void FindFollowsASymlinkAliasToTheEnumeratedPort()
    {
        // A /dev/serial/by-id/… symlink names the port it points at (ERRATA A5): a regular file stands in
        // for the node, the resolved path matches either way.
        if (OperatingSystem.IsWindows())
            return; // POSIX only: on Windows a COM name is matched by string (SerialPorts.Find)
        string dir = Path.Combine(Path.GetTempPath(), $"depz-a5-{Guid.NewGuid():N}");
        Directory.CreateDirectory(dir);
        try
        {
            string node = Path.Combine(dir, "ttyACM0"), alias = Path.Combine(dir, "usb-DEPZ_ToF-if00");
            File.WriteAllBytes(node, Array.Empty<byte>());
            File.CreateSymbolicLink(alias, node);
            var ports = new[] { new SerialPortInfo("/dev/ttyACM9", 0x1BCF, 0xED4B, "Z"), new SerialPortInfo(node, 0x1BCF, 0xED40, "SN1") };
            Assert.Same(ports[1], SerialPorts.Find(ports, alias));
            Assert.Null(SerialPorts.Find(ports, Path.Combine(dir, "other")));
            // ListDepzDevices keeps the path given and annotates it with the node's USB id — and so would
            // probe the alias with the USB filter on; here nothing answers, so nothing is listed.
            Assert.Empty(DepzDevices.ListDepzDevices(new[] { alias }, timeout: Short));
        }
        finally
        {
            Directory.Delete(dir, true);
        }
    }

    [Fact]
    public void OpenDeviceBySerialOrIndexNamesWhatIsMissing()
    {
        string serial = "\0no-such-serial-" + Guid.NewGuid().ToString("N");
        Assert.Contains("no DEPZ device with USB serial", Assert.Throws<NoDepzDeviceException>(() => DepzDevices.OpenDevice(serial, Short)).Message);
        Assert.Contains("no DEPZ candidate at index -1", Assert.Throws<NoDepzDeviceException>(() => DepzDevices.OpenDevice(-1, Short)).Message);
        Assert.Throws<NoDepzDeviceException>(() => DepzDevices.OpenDevice(int.MaxValue, Short));
        Assert.Contains("no DEPZ device answered on", Assert.Throws<NoDepzDeviceException>(() => DepzDevices.OpenPort(NoPort, Short)).Message); // as on a link (ERRATA A14)
    }

    [Fact]
    public void OpenPortThatCannotBeOpenedNamesTheOpenFailure()
    {
        // NoPort fails to open as a port another process holds does (contract 07 §3).
        var e = Assert.Throws<NoDepzDeviceException>(() => DepzDevices.OpenPort(NoPort, Short));
        Assert.NotNull(e.InnerException);
        Assert.Equal($"no DEPZ device answered on {NoPort}: could not open the port ({e.InnerException!.Message})", e.Message);
        Assert.Null(DepzDevices.ProbePort(NoPort, Short));
    }

    [Fact]
    public void WithNoDepzBoardPluggedInOpenDeviceSaysSoWithoutTouchingAPort()
    {
        // Enumeration reads sysfs only; whether a DEPZ board is plugged in decides what can be asserted.
        // With one present, OpenDevice() would open it — a live board is off limits here.
        bool anyDepz = SerialPorts.List().Any(p => UsbIds.IsKnownDepzUsb(p.Vid, p.Pid));
        if (anyDepz)
            return;
        Assert.Contains("no DEPZ device found", Assert.Throws<NoDepzDeviceException>(() => DepzDevices.OpenDevice(timeout: Short)).Message);
        Assert.Contains("(0 candidate(s) found)", Assert.Throws<NoDepzDeviceException>(() => DepzDevices.OpenDevice(0, Short)).Message);
    }

    [Fact]
    public void EveryTypedOpenGoesThroughTheSerialLookup()
    {
        string serial = "\0no-such-serial-" + Guid.NewGuid().ToString("N");
        var opens = new Func<DepzDevice>[]
        {
            () => DepzDevices.OpenSr04(serial, Short), () => DepzDevices.OpenVl53l4cd(serial, Short),
            () => DepzDevices.OpenVl53l8(serial, Short), () => DepzDevices.OpenVl53l7(serial, Short),
            () => DepzDevices.OpenBno055(serial, Short), () => DepzDevices.OpenBno086(serial, Short),
            () => DepzDevices.OpenVl53lx(serial, Short), () => DepzDevices.OpenTmf8829(serial, Short),
        };
        foreach (var open in opens)
            Assert.Contains(serial, Assert.Throws<NoDepzDeviceException>(() => open()).Message);
    }

    [Fact]
    public void OpenDeviceOfAProbeResultOpensThePortAsTheResolvedClass()
    {
        Assert.Throws<ArgumentNullException>(() => DepzDevices.OpenDevice((DeviceInfo)null!, Short));
        // One probe answer per class (vectors/probe.json resolve): the class opens the port afresh, and the
        // port is not there — the bootloader is refused before any port is touched.
        var perClass = new Dictionary<string, DeviceInfo>();
        foreach (var c in Vectors.Load("probe.json").GetProperty("resolve").EnumerateArray())
        {
            static int? Id(JsonElement e) => e.ValueKind == JsonValueKind.Null ? null : e.GetInt32();
            string cls = c.GetProperty("class").GetString()!;
            if (perClass.ContainsKey(cls))
                continue;
            var id = IdentityParser.ParseSoftwareName(c.GetProperty("software_name").GetString()!);
            perClass[cls] = new DeviceInfo(NoPort, id.Mode, id.SensorType, id.SoftwareName, id.Version,
                c.GetProperty("device_name").GetString() ?? "", "", Id(c.GetProperty("usb_vid")), Id(c.GetProperty("usb_pid")));
        }
        Assert.Equal(Discovery.SensorClassNames.OrderBy(n => n), perClass.Keys.OrderBy(n => n));
        foreach (var (cls, info) in perClass)
        {
            if (cls == "bootloader")
                Assert.Throws<BootloaderModeException>(() => DepzDevices.OpenDevice(info, Short));
            else
                Assert.Contains("cannot open", Assert.Throws<DeviceLostException>(() => DepzDevices.OpenDevice(info, Short)).Message);
        }
    }
}
