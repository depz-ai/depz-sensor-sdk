#if NET
using System.Globalization;
using System.IO.Ports;
using Microsoft.Win32;

namespace Depz.Sensor.Devices;

/// <summary>One serial port and its USB identity, where the OS tells it.</summary>
public sealed record SerialPortInfo(string Port, int? Vid, int? Pid, string? UsbSerial);

/// <summary>
/// Serial port enumeration with USB ids: Linux (sysfs) and Windows (the
/// registry). On macOS the ports are listed without USB ids, so discovery
/// finds no DEPZ candidate there: open a port path explicitly.
/// </summary>
public static class SerialPorts
{
    /// <summary>Every serial port the OS lists.</summary>
    public static IReadOnlyList<SerialPortInfo> List()
    {
        if (OperatingSystem.IsLinux())
            return ListLinux();
        if (OperatingSystem.IsWindows())
            return ListWindows();
        return SerialPort.GetPortNames().Select(p => new SerialPortInfo(p, null, null, null)).ToList();
    }

    /// <summary>Sorted by USB serial (none last), then by port path (contract 02 §4.3).</summary>
    public static IEnumerable<SerialPortInfo> BySerial(IEnumerable<SerialPortInfo> ports) =>
        ports.OrderBy(p => string.IsNullOrEmpty(p.UsbSerial) ? 1 : 0)
            .ThenBy(p => p.UsbSerial ?? "", StringComparer.Ordinal)
            .ThenBy(p => p.Port, StringComparer.Ordinal);

    private static List<SerialPortInfo> ListLinux()
    {
        var result = new List<SerialPortInfo>();
        const string root = "/sys/class/tty";
        if (!Directory.Exists(root))
            return result;
        foreach (string tty in Directory.GetDirectories(root).OrderBy(d => d, StringComparer.Ordinal))
        {
            string name = Path.GetFileName(tty);
            if (!name.StartsWith("ttyACM", StringComparison.Ordinal) && !name.StartsWith("ttyUSB", StringComparison.Ordinal))
                continue;
            int? vid = null, pid = null;
            string? serial = null;
            // device → the USB interface; its parent holds the USB device's descriptors.
            string? dir = ResolveDir(Path.Combine(tty, "device"));
            for (int up = 0; dir != null && up < 3; up++, dir = Path.GetDirectoryName(dir))
            {
                string v = Path.Combine(dir, "idVendor");
                if (!File.Exists(v))
                    continue;
                vid = ParseHex(File.ReadAllText(v));
                pid = ParseHex(File.ReadAllText(Path.Combine(dir, "idProduct")));
                string s = Path.Combine(dir, "serial");
                serial = File.Exists(s) ? File.ReadAllText(s).Trim() : null;
                break;
            }
            result.Add(new SerialPortInfo("/dev/" + name, vid, pid, serial));
        }
        return result;
    }

    private static string? ResolveDir(string path)
    {
        try
        {
            string real = RealPath(path, 0);
            return Directory.Exists(real) ? real : null;
        }
        catch (IOException)
        {
            return null;
        }
    }

    // realpath(3): sysfs links are relative to the directory that really holds them
    // (/sys/class/tty/ttyACM0/device → ../../../1-2:1.0), so resolve one component at a time.
    private static string RealPath(string path, int depth)
    {
        if (depth > 40)
            throw new IOException($"too many levels of symbolic links: {path}");
        string cur = "/";
        foreach (string part in path.Split(new[] { '/' }, StringSplitOptions.RemoveEmptyEntries))
        {
            string next = Path.Combine(cur, part);
            string? target = new FileInfo(next).LinkTarget;
            cur = target is null ? next : RealPath(Path.GetFullPath(Path.Combine(cur, target)), depth + 1);
        }
        return cur;
    }

    private static int? ParseHex(string s) =>
        int.TryParse(s.Trim(), NumberStyles.AllowHexSpecifier, CultureInfo.InvariantCulture, out int v) ? v : null;

    [System.Runtime.Versioning.SupportedOSPlatform("windows")]
    private static List<SerialPortInfo> ListWindows()
    {
        // HKLM\SYSTEM\CurrentControlSet\Enum\USB\VID_xxxx&PID_xxxx[&MI_nn]\<instance>\Device Parameters\PortName.
        // The instance id of a plain device is its USB serial; an interface of a composite device
        // (…&MI_nn) carries a generated id, so the serial is the parent device's instance id.
        var byPort = new Dictionary<string, SerialPortInfo>(StringComparer.OrdinalIgnoreCase);
        try
        {
            using var usb = Registry.LocalMachine.OpenSubKey(@"SYSTEM\CurrentControlSet\Enum\USB");
            if (usb != null)
            {
                foreach (string hw in usb.GetSubKeyNames())
                {
                    int? vid = HwField(hw, "VID_"), pid = HwField(hw, "PID_");
                    bool iface = hw.IndexOf("&MI_", StringComparison.OrdinalIgnoreCase) >= 0;
                    using var hwKey = usb.OpenSubKey(hw);
                    if (hwKey is null)
                        continue;
                    foreach (string inst in hwKey.GetSubKeyNames())
                    {
                        using var p = hwKey.OpenSubKey(inst + @"\Device Parameters");
                        if (p?.GetValue("PortName") is not string port)
                            continue;
                        string? serial = iface ? ParentSerial(usb, vid, pid) : inst;
                        byPort[port] = new SerialPortInfo(port, vid, pid, serial);
                    }
                }
            }
        }
        catch (Exception e) when (e is System.Security.SecurityException || e is UnauthorizedAccessException || e is IOException)
        {
            // Without registry access the ports are still listed, without USB ids.
        }
        foreach (string port in SerialPort.GetPortNames())
            if (!byPort.ContainsKey(port))
                byPort[port] = new SerialPortInfo(port, null, null, null);
        return byPort.Values.ToList();
    }

    [System.Runtime.Versioning.SupportedOSPlatform("windows")]
    private static string? ParentSerial(RegistryKey usb, int? vid, int? pid)
    {
        if (vid is null || pid is null)
            return null;
        using var parent = usb.OpenSubKey($"VID_{vid:X4}&PID_{pid:X4}");
        return parent?.GetSubKeyNames().FirstOrDefault();
    }

    private static int? HwField(string hw, string tag)
    {
        int i = hw.IndexOf(tag, StringComparison.OrdinalIgnoreCase);
        return i >= 0 && hw.Length >= i + tag.Length + 4 ? ParseHex(hw.Substring(i + tag.Length, 4)) : null;
    }
}

/// <summary>
/// A serial port as an <see cref="ILink"/> (System.IO.Ports). DTR and RTS are
/// raised on open, as the reference SDK's pyserial does: a CDC device may hold
/// its output until the host asserts DTR.
/// </summary>
public sealed class SerialPortLink : ILink
{
    private readonly SerialPort _port;
    private readonly byte[] _buf = new byte[16384];
    private volatile bool _closed;

    public SerialPortLink(string port, int baudRate = 115200)
    {
        _port = new SerialPort(port, baudRate) { DtrEnable = true, RtsEnable = true, ReadTimeout = 50, WriteTimeout = 1000 };
        try
        {
            _port.Open();
        }
        catch (Exception e) when (e is IOException || e is UnauthorizedAccessException || e is InvalidOperationException)
        {
            throw new DeviceLostException($"cannot open {port}: {e.Message}", e);
        }
        Name = port;
    }

    public string Name { get; }

    public bool IsClosed => _closed || !_port.IsOpen;

    public byte[] Read(TimeSpan timeout)
    {
        if (_closed)
            throw new LinkClosedException($"{Name} closed");
        try
        {
            _port.ReadTimeout = Math.Max(1, (int)timeout.TotalMilliseconds);
            int n = _port.BaseStream.Read(_buf, 0, _buf.Length);
            if (n <= 0)
                return Array.Empty<byte>();
            var data = new byte[n];
            Array.Copy(_buf, data, n);
            return data;
        }
        catch (TimeoutException)
        {
            return Array.Empty<byte>();
        }
        catch (Exception e) when (e is IOException || e is InvalidOperationException || e is ObjectDisposedException)
        {
            if (_closed)
                throw new LinkClosedException($"{Name} closed");
            throw new DeviceLostException($"{Name}: {e.Message}", e);
        }
    }

    public void Write(byte[] data)
    {
        if (_closed)
            throw new LinkClosedException($"{Name} closed");
        try
        {
            _port.BaseStream.Write(data, 0, data.Length);
        }
        catch (Exception e) when (e is IOException || e is InvalidOperationException || e is ObjectDisposedException
                                  || e is TimeoutException)
        {
            throw new DeviceLostException($"{Name}: {e.Message}", e);
        }
    }

    public void Close()
    {
        _closed = true;
        try
        {
            _port.Close();
        }
        catch (IOException)
        {
            // already gone (unplugged)
        }
    }
}
#endif
