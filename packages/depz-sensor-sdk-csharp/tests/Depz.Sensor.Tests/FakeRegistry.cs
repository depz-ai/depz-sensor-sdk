using Depz.Sensor.Devices;

namespace Depz.Sensor.Tests;

/// <summary>An in-memory HKEY_LOCAL_MACHINE: keys by path (case-insensitive, parents implied),
/// string values, and "locked" keys that read as unreadable (null), like a key the user may not open.</summary>
internal sealed class FakeRegistry : IRegistry
{
    private readonly Dictionary<string, Dictionary<string, string>> _keys = new(StringComparer.OrdinalIgnoreCase);
    private readonly HashSet<string> _locked = new(StringComparer.OrdinalIgnoreCase);

    public FakeRegistry Key(string path, params (string Name, string Value)[] values)
    {
        for (string p = path; p.Length > 0; p = p.Contains('\\') ? p[..p.LastIndexOf('\\')] : "")
            if (!_keys.ContainsKey(p))
                _keys[p] = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
        foreach (var (name, value) in values)
            _keys[path][name] = value;
        return this;
    }

    /// <summary>The key exists but cannot be read.</summary>
    public FakeRegistry Lock(string path)
    {
        Key(path);
        _locked.Add(path);
        return this;
    }

    /// <summary>The ports present now: SERIALCOMM values <c>\Device\USBSER00n</c> → <c>COMn</c>.</summary>
    public FakeRegistry Present(params string[] ports)
    {
        Key(WindowsSerialPorts.SerialCommKey);
        int n = _keys[WindowsSerialPorts.SerialCommKey].Count;
        foreach (string port in ports)
            Key(WindowsSerialPorts.SerialCommKey, ($@"\Device\USBSER{n++:000}", port));
        return this;
    }

    /// <summary>A USB device instance <c>Enum\USB\{hw}\{inst}</c> with a PortName; <paramref name="attached"/>
    /// gives it the volatile Control subkey a present device has.</summary>
    public FakeRegistry UsbPort(string hw, string inst, string port, bool attached = true)
    {
        string p = $@"{WindowsSerialPorts.UsbEnumKey}\{hw}\{inst}";
        Key(p + @"\Device Parameters", ("PortName", port));
        if (attached)
            Key(p + @"\Control");
        return this;
    }

    /// <summary>A composite device's parent instance, whose interfaces (…&amp;MI_nn) carry <paramref name="parentIdPrefix"/>.</summary>
    public FakeRegistry UsbParent(string hw, string inst, string parentIdPrefix) =>
        Key($@"{WindowsSerialPorts.UsbEnumKey}\{hw}\{inst}", ("ParentIdPrefix", parentIdPrefix));

    public IReadOnlyList<string>? SubKeyNames(string path) =>
        Readable(path)
            ? _keys.Keys.Where(k => k.Length > path.Length + 1 && k.StartsWith(path + "\\", StringComparison.OrdinalIgnoreCase)
                                    && k.IndexOf('\\', path.Length + 1) < 0)
                .Select(k => k[(path.Length + 1)..]).OrderBy(k => k, StringComparer.Ordinal).ToList()
            : null;

    public IReadOnlyList<string>? ValueNames(string path) => Readable(path) ? _keys[path].Keys.ToList() : null;

    public string? GetString(string path, string name) =>
        Readable(path) && _keys[path].TryGetValue(name, out string? v) ? v : null;

    private bool Readable(string path) => _keys.ContainsKey(path) && !_locked.Contains(path);
}
