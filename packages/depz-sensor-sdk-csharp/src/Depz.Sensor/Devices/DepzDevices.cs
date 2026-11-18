using Depz.Sensor.Protocol;
using Depz.Sensor.Usb;

namespace Depz.Sensor.Devices;

/// <summary>What the firmware name says the board is running (contract 02 §4).</summary>
public enum Mode
{
    App,
    Bootloader,
    Unknown,
}

/// <summary>What the contract 02 §4.4 probe learned about one port.</summary>
public sealed record DeviceInfo(
    string Port,
    Mode Mode,
    SensorType? SensorType,
    string SoftwareName,
    string FwVersion,
    string DeviceName,
    string SerialNumber,
    int? UsbVid = null,
    int? UsbPid = null,
    string? UsbSerial = null)
{
    /// <summary>The model the USB-id table gives the port (contract 02 §4.2), or null.</summary>
    public string? UsbModelHint => UsbIds.UsbModelHint(UsbVid, UsbPid);

    /// <summary>The enum value for a mode string of <see cref="Identity"/>: <c>"app"</c>,
    /// <c>"bootloader"</c> or <c>"unknown"</c>.</summary>
    public static Mode ModeFromLabel(string label) => label switch
    {
        "app" => Mode.App,
        "bootloader" => Mode.Bootloader,
        "unknown" => Mode.Unknown,
        _ => throw new ArgumentException($"no device mode '{label}'", nameof(label)),
    };
}

/// <summary>
/// Opening devices (contract 02 §4, contract 07 §2): the probe, class
/// resolution and continuing a connection as its sensor class. Every
/// <c>OpenDevice…</c> / <c>OpenPort</c> returns the class the probe resolves;
/// cast to the one you expect. Port discovery (<c>OpenDevice</c>,
/// <c>OpenDeviceBySerial</c>, <c>ListDepzDevices</c>) needs .NET 8; in Unity
/// on Windows use <c>DepzDevicesUnity</c> (the <c>unity/</c> module), elsewhere
/// open your own <see cref="ILink"/> and call <see cref="OpenOnLink"/>.
/// </summary>
public static class DepzDevices
{
    /// <summary>
    /// Open a device on a connection that cannot be reopened — a replay or recording link,
    /// a link the host opened (contract 02 §4.4). Runs the probe (GET_NAME_ACTIVE_SOFTWARE,
    /// GET_DEVICE_NAME, GET_SERIAL) on <paramref name="link"/>, resolves the class from the answers
    /// and the port's USB id (null = unknown; a link knows none) and continues as that class on
    /// the same connection. The device owns the link; on failure it is closed.
    /// </summary>
    /// <exception cref="NoDepzDeviceException">No answer to GET_NAME_ACTIVE_SOFTWARE.</exception>
    /// <exception cref="BootloaderModeException">The device runs its bootloader.</exception>
    public static DepzDevice OpenOnLink(ILink link, int? usbVid = null, int? usbPid = null, TimeSpan? timeout = null)
    {
        var dev = new DepzDevice(link, timeout);
        try
        {
            (string software, string deviceName, _) = Probe(dev);
            return AttachSensorClass(dev, Discovery.ResolveSensorClass(software, deviceName, usbVid, usbPid));
        }
        catch (DepzException e) when (!(e is BootloaderModeException) && !(e is NoDepzDeviceException))
        {
            dev.Close();
            throw new NoDepzDeviceException($"no DEPZ device answered on {link.Name} ({e.Message})", e);
        }
        catch
        {
            dev.Close();
            throw;
        }
    }

    /// <summary>
    /// Continue the open plain device <paramref name="dev"/> as <paramref name="className"/> (a name
    /// <see cref="Discovery.ResolveSensorClass"/> returns) on the same connection — no reopen; the
    /// reader and the host sequence numbers run on (contract 02 §4.4). Use the result from here on.
    /// "device", and a class this SDK has no device layer for yet, return <paramref name="dev"/> itself.
    /// This is the one place a sensor class is chosen: every <c>OpenDevice…</c> ends here.
    /// </summary>
    /// <exception cref="BootloaderModeException"><paramref name="className"/> is "bootloader".</exception>
    public static DepzDevice AttachSensorClass(DepzDevice dev, string className)
    {
        if (dev is null) throw new ArgumentNullException(nameof(dev));
        if (dev.GetType() != typeof(DepzDevice))
            throw new ArgumentException($"AttachSensorClass needs a plain DepzDevice, got {dev.GetType().Name}", nameof(dev));
        if (!Discovery.SensorClassNames.Contains(className))
            throw new ArgumentException($"no sensor class '{className}'", nameof(className));
        return className switch
        {
            "bootloader" => throw new BootloaderModeException(),
            "sr04" => new Sr04Device(dev),
            "vl53l4cd" => new Vl53l4cdDevice(dev),
            "vl53l8cx" => new Vl53l8cxDevice(dev),
            "vl53l8ch" => new Vl53l8chDevice(dev),
            "vl53l5cx" => new Vl53l5cxDevice(dev),
            "vl53l7cx" => new Vl53l7cxDevice(dev),
            "vl53l7ch" => new Vl53l7chDevice(dev),
            "bno055" => new Bno055Device(dev),
            "bno086" => new Bno086Device(dev),
            "vl53lx" => new Vl53lxDevice(dev),
            "vl53l0x" => new Vl53l0xDevice(dev),
            "vl53l1cx" => new Vl53l1cxDevice(dev),
            "vl53l1cb" => new Vl53l1cbDevice(dev),
            "vl53l3cx" => new Vl53l3cxDevice(dev),
            "vl53l4cx" => new Vl53l4cxDevice(dev),
            "tmf8829" => new Tmf8829Device(dev),
            _ => dev,
        };
    }

    /// <summary>The contract 02 §4.4 probe on an open connection: GET_NAME_ACTIVE_SOFTWARE
    /// (required: its failure throws), then GET_DEVICE_NAME and GET_SERIAL (a timeout or error
    /// leaves ""). Returns (software name, device name, serial number).</summary>
    public static (string SoftwareName, string DeviceName, string SerialNumber) Probe(DepzDevice dev)
    {
        if (dev is null) throw new ArgumentNullException(nameof(dev));
        string software = dev.GetSoftwareName();
        string deviceName;
        try { deviceName = dev.GetDeviceName(); }
        catch (DepzException) { deviceName = ""; }
        string serial;
        try { serial = dev.GetSerialNumber(); }
        catch (DepzException) { serial = ""; }
        return (software, deviceName, serial);
    }

    /// <summary>Probe the device on <paramref name="link"/> (the contract 02 §4.4 requests) and close
    /// the link. Null when nothing DEPZ-shaped answers. <see cref="DeviceInfo.Port"/> is the link's
    /// name; the USB ids stay null (a link knows none). For hosts that open their own ports.</summary>
    internal static DeviceInfo? ProbeLink(ILink link, TimeSpan? timeout = null)
    {
        var dev = new DepzDevice(link, timeout);
        try
        {
            var (software, deviceName, serial) = Probe(dev);
            var id = IdentityParser.ParseSoftwareName(software);
            return new DeviceInfo(link.Name, DeviceInfo.ModeFromLabel(id.Mode), id.SensorType, id.SoftwareName,
                id.Version, deviceName, serial);
        }
        catch (DepzException)
        {
            return null;
        }
        finally
        {
            dev.Close();
        }
    }

#if NET
    /// <summary>Probe one port: open it, run the probe, close. Null when nothing DEPZ-shaped answers.</summary>
    public static DeviceInfo? ProbePort(string port, TimeSpan? timeout = null) => ProbeOpening(port, timeout, out _);

    // ProbePort that also hands back why the port could not be opened (busy, no permission, gone) —
    // so OpenProbed can name it (contract 07 §3).
    private static DeviceInfo? ProbeOpening(string port, TimeSpan? timeout, out Exception? openFailure)
    {
        openFailure = null;
        SerialPortLink link;
        try
        {
            link = new SerialPortLink(port);
        }
        catch (Exception e) when (e is IOException || e is UnauthorizedAccessException || e is DepzException
                                  || e is ArgumentException || e is InvalidOperationException)
        {
            openFailure = e;
            return null;
        }
        return ProbeLink(link, timeout);
    }

    /// <summary>Probe the candidate ports and return every DEPZ device found, ordered by USB serial.
    /// With <paramref name="matchUsb"/> (default) only ports with a known DEPZ USB id are probed;
    /// <paramref name="ports"/> replaces enumeration with an explicit list.</summary>
    public static IReadOnlyList<DeviceInfo> ListDepzDevices(
        IEnumerable<string>? ports = null, bool matchUsb = true, TimeSpan? timeout = null)
    {
        var enumerated = SerialPorts.List();
        IEnumerable<SerialPortInfo> candidates = ports is null
            ? enumerated
            : ports.Select(p => PortId(enumerated, p));
        if (matchUsb)
            candidates = candidates.Where(p => UsbIds.IsKnownDepzUsb(p.Vid, p.Pid));
        var found = new List<DeviceInfo>();
        foreach (var pi in SerialPorts.BySerial(candidates))
        {
            var info = ProbePort(pi.Port, timeout);
            if (info != null)
                found.Add(info with { UsbVid = pi.Vid, UsbPid = pi.Pid, UsbSerial = pi.UsbSerial });
        }
        return found;
    }

    /// <summary>Open the <paramref name="index"/>-th DEPZ candidate as its sensor class, candidates
    /// sorted by USB serial (contract 02 §4.3); the default is the smallest USB serial.</summary>
    /// <exception cref="NoDepzDeviceException">No candidate port, or none at <paramref name="index"/>.</exception>
    public static DepzDevice OpenDevice(int index = 0, TimeSpan? timeout = null)
    {
        var candidates = DepzCandidates();
        if (candidates.Count == 0)
            throw new NoDepzDeviceException(
                "no DEPZ device found (no serial port has a known DEPZ USB id). "
                + "Pass a port path explicitly to open an unprogrammed unit.");
        if (index < 0 || index >= candidates.Count)
            throw new NoDepzDeviceException($"no DEPZ candidate at index {index} ({candidates.Count} candidate(s) found)");
        return OpenProbed(candidates[index].Port, timeout);
    }

    /// <summary>Open the DEPZ device whose USB serial is <paramref name="serial"/> as its sensor class
    /// (contract 02 §4.3).</summary>
    /// <exception cref="NoDepzDeviceException">No candidate port has that USB serial.</exception>
    public static DepzDevice OpenDeviceBySerial(string serial, TimeSpan? timeout = null)
    {
        if (serial is null) throw new ArgumentNullException(nameof(serial));
        var match = DepzCandidates().FirstOrDefault(p => p.UsbSerial == serial)
            ?? throw new NoDepzDeviceException($"no DEPZ device with USB serial '{serial}'");
        return OpenProbed(match.Port, timeout);
    }

    /// <summary>Open exactly <paramref name="port"/> as its sensor class (also a port whose USB id
    /// is not a DEPZ one: an unprogrammed unit). The USB id is that of the enumerated port the path
    /// names under any alias (<c>/dev/serial/by-id/…</c>; contract 02 §4.3).</summary>
    public static DepzDevice OpenPort(string port, TimeSpan? timeout = null) => OpenProbed(port, timeout);

    /// <summary>Open the port of a <see cref="ListDepzDevices"/> result as the class its probe answers resolve to.</summary>
    public static DepzDevice OpenDevice(DeviceInfo info, TimeSpan? timeout = null)
    {
        if (info is null) throw new ArgumentNullException(nameof(info));
        return OpenResolved(info, info.UsbVid, info.UsbPid, timeout);
    }

    private static List<SerialPortInfo> DepzCandidates() =>
        SerialPorts.BySerial(SerialPorts.List().Where(p => UsbIds.IsKnownDepzUsb(p.Vid, p.Pid))).ToList();

    // `port` with the USB identity of the enumerated port it names under any alias (SerialPorts.Find);
    // none when the OS does not list it. The port stays `port`: an explicit path is opened as given.
    private static SerialPortInfo PortId(IReadOnlyList<SerialPortInfo> enumerated, string port)
    {
        var found = SerialPorts.Find(enumerated, port);
        return new SerialPortInfo(port, found?.Vid, found?.Pid, found?.UsbSerial);
    }

    private static DepzDevice OpenProbed(string port, TimeSpan? timeout)
    {
        var info = ProbeOpening(port, timeout, out var openFailure);
        if (openFailure is not null)
            throw new NoDepzDeviceException(
                $"no DEPZ device answered on {port}: could not open the port ({openFailure.Message})", openFailure);
        if (info is null)
            throw new NoDepzDeviceException($"no DEPZ device answered on {port}");
        var pi = PortId(SerialPorts.List(), port);
        return OpenResolved(info, pi.Vid, pi.Pid, timeout);
    }

    // The probe ran on a connection of its own (closed); the class opens the port afresh and
    // AttachSensorClass chooses it — the bootloader is refused before the port is touched.
    private static DepzDevice OpenResolved(DeviceInfo info, int? vid, int? pid, TimeSpan? timeout)
    {
        string name = Discovery.ResolveSensorClass(info.SoftwareName, info.DeviceName, vid, pid);
        if (name == "bootloader")
            throw new BootloaderModeException();
        var dev = new DepzDevice(new SerialPortLink(info.Port), timeout);
        try
        {
            return AttachSensorClass(dev, name);
        }
        catch
        {
            dev.Close();
            throw;
        }
    }
#endif
}
