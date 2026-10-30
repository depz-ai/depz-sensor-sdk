using System.Diagnostics;
using System.Globalization;
using System.Text;
using Depz.Sensor.Json;

namespace Depz.Sensor.Devices;

/// <summary>
/// A blocking byte pipe to one device: a serial port, a <c>.depzrec</c>
/// record/replay wrapper, or anything a host supplies (in Unity, a port opened
/// with Unity's own System.IO.Ports). Device classes accept any link, which
/// keeps every layer above the port testable without hardware.
/// </summary>
public interface ILink
{
    /// <summary>The next available chunk (one byte or more), or an empty array when nothing
    /// arrived within <paramref name="timeout"/>.</summary>
    /// <exception cref="DeviceLostException">The link is closed or the device is gone.</exception>
    byte[] Read(TimeSpan timeout);

    /// <exception cref="DeviceLostException">The link is closed or the device is gone.</exception>
    void Write(byte[] data);

    void Close();

    bool IsClosed { get; }

    /// <summary>A name for messages: the port path, or what the link wraps.</summary>
    string Name { get; }
}

/// <summary>
/// Wraps any link and tees both directions into a <c>.depzrec</c> file
/// (contract 08). TX is journaled before the physical write: a fast device reply
/// could otherwise reach the reader and be journaled ahead of its own request,
/// which breaks the causal rx gating replay relies on.
/// </summary>
public sealed class RecordingLink : ILink, IDisposable
{
    public const string Schema = "depz.rec/1";

    private readonly ILink _inner;
    private readonly StreamWriter _file;
    private readonly object _lock = new();
    private readonly Stopwatch _clock = Stopwatch.StartNew();
    private bool _fileClosed;

    /// <param name="inner">The link to record; closing this one closes it.</param>
    /// <param name="path">The <c>.depzrec</c> file to write (overwritten).</param>
    /// <param name="headerExtra">More header fields (contract 08), e.g. <c>"device"</c>.</param>
    public RecordingLink(ILink inner, string path, IDictionary<string, object?>? headerExtra = null)
    {
        _inner = inner ?? throw new ArgumentNullException(nameof(inner));
        _file = new StreamWriter(path, false, new UTF8Encoding(false)) { NewLine = "\n" };
        var header = new Dictionary<string, object?>
        {
            ["schema"] = Schema,
            ["created_utc"] = DateTime.UtcNow.ToString("yyyy-MM-dd'T'HH:mm:ss'Z'", CultureInfo.InvariantCulture),
        };
        if (headerExtra != null)
            foreach (var kv in headerExtra)
                header[kv.Key] = kv.Value;
        _file.WriteLine(JsonText.Serialize(header));
    }

    public string Name => _inner.Name;

    public bool IsClosed => _inner.IsClosed;

    private void Emit(string dir, byte[] data)
    {
        if (data.Length == 0)
            return;
        lock (_lock)
        {
            if (_fileClosed)
                return;
            long t = DepzDevice.TicksToUs(_clock.ElapsedTicks, Stopwatch.Frequency);
            _file.WriteLine(JsonText.Serialize(new Dictionary<string, object?>
            {
                ["t"] = t,
                ["dir"] = dir,
                ["data"] = data,
            }));
        }
    }

    public byte[] Read(TimeSpan timeout)
    {
        byte[] data = _inner.Read(timeout);
        Emit("rx", data);
        return data;
    }

    public void Write(byte[] data)
    {
        Emit("tx", data); // journal first — see the class summary
        _inner.Write(data);
    }

    public void Dispose() => Close();

    public void Close()
    {
        lock (_lock)
        {
            if (!_fileClosed)
            {
                _fileClosed = true;
                _file.Dispose();
            }
        }
        _inner.Close();
    }
}

/// <summary>
/// Replays the rx side of a <c>.depzrec</c> capture causally (contract 08): an
/// rx chunk is served only after the host has written as many tx bytes as
/// preceded it in the capture. With <c>strictTx</c> every written byte must
/// also equal the recorded tx stream (<see cref="ReplayMismatchException"/>).
/// </summary>
public sealed class ReplayLink : ILink
{
    private readonly List<(long TUs, byte[] Data, long TxPrefix)> _rx = new();
    private readonly byte[] _tx;
    private readonly bool _strictTx;
    private readonly bool _realtime;
    private readonly Stopwatch _clock = Stopwatch.StartNew();
    private readonly object _cv = new();
    private int _rxIdx;
    private long _txWritten;
    private bool _closed;

    /// <summary>The capture's header object (contract 08).</summary>
    public IReadOnlyDictionary<string, object?> Header { get; }

    /// <param name="path">A <c>.depzrec</c> file.</param>
    /// <param name="strictTx">Fail a write that differs from the recorded tx stream.</param>
    /// <param name="realtime">Pace rx chunks by their recorded timestamps.</param>
    /// <exception cref="ArgumentException">A damaged capture (contract 08): an empty file, a line
    /// that is not a JSON object, an event without a string <c>dir</c>, a non-hex <c>data</c>, a
    /// <c>t</c> that is not a non-negative integer — the message names the line.</exception>
    public ReplayLink(string path, bool strictTx = false, bool realtime = false)
    {
        Name = $"replay:{Path.GetFileName(path)}";
        _strictTx = strictTx;
        _realtime = realtime;
        // GetString keeps a BOM (ReadAllText would drop it): a BOM is no JSON, as in every SDK
        string text = Encoding.UTF8.GetString(File.ReadAllBytes(path));
        if (text.Length == 0)
            throw new ArgumentException($"{path}: empty, not a .depzrec capture (no header line)");
        string[] lines = text.Split(LineBreaks, StringSplitOptions.None);
        Header = JsonObjectLine(path, 1, lines[0]);
        var tx = new List<byte>();
        for (int i = 1; i < lines.Length; i++)
        {
            if (JsonText.IsBlankLine(lines[i]))
                continue;
            var (tUs, direction, data) = Event(path, i + 1, lines[i]);
            if (direction == "rx")
                _rx.Add((tUs, data, tx.Count));
            else
                tx.AddRange(data);
        }
        _tx = tx.ToArray();
    }

    private static readonly string[] LineBreaks = { "\r\n", "\n" };

    // `line` as the JSON object a capture line must be, else ArgumentException naming the file and
    // line (a damaged capture is the caller's bad argument, contract 07 §3 — never a cast / key error).
    private static Dictionary<string, object?> JsonObjectLine(string path, int lineno, string line)
    {
        object? obj;
        try
        {
            obj = JsonText.Parse(line);
        }
        catch (FormatException bad)
        {
            throw new ArgumentException($"{path}:{lineno}: not JSON: {bad.Message}");
        }
        return obj as Dictionary<string, object?>
            ?? throw new ArgumentException($"{path}:{lineno}: not a JSON object");
    }

    // One capture event line (contract 08) as (t_us, dir, data).
    private static (long TUs, string Dir, byte[] Data) Event(string path, int lineno, string line)
    {
        var ev = JsonObjectLine(path, lineno, line);
        ev.TryGetValue("t", out object? t);
        ev.TryGetValue("dir", out object? direction);
        ev.TryGetValue("data", out object? data);
        if (t is not long tUs || tUs < 0)
            throw new ArgumentException($"{path}:{lineno}: 't' must be a non-negative integer, got {JsonText.Serialize(t)}");
        if (direction is not string dir)
            throw new ArgumentException($"{path}:{lineno}: 'dir' must be a string, got {JsonText.Serialize(direction)}");
        if (data is not string hex)
            throw new ArgumentException($"{path}:{lineno}: 'data' must be a hex string, got {JsonText.Serialize(data)}");
        if (!IsHex(hex))
            throw new ArgumentException($"{path}:{lineno}: 'data' is not hex: {JsonText.Serialize(hex)}");
        return (tUs, dir, FromHex(hex));
    }

    private static bool IsHex(string s)
    {
        if ((s.Length & 1) != 0)
            return false;
        foreach (char c in s)
            if (!(c is >= '0' and <= '9' or >= 'a' and <= 'f' or >= 'A' and <= 'F'))
                return false;
        return true;
    }

    public string Name { get; }

    public bool IsClosed
    {
        get { lock (_cv) return _closed; }
    }

    /// <summary>Every rx chunk was delivered.</summary>
    public bool RxExhausted
    {
        get { lock (_cv) return _rxIdx >= _rx.Count; }
    }

    /// <summary>Recorded rx chunks not delivered yet.</summary>
    public int RxRemaining
    {
        get { lock (_cv) return _rx.Count - _rxIdx; }
    }

    /// <summary>Recorded tx bytes the host has not written yet.</summary>
    public long TxRemaining
    {
        get { lock (_cv) return _tx.Length - _txWritten; }
    }

    public byte[] Read(TimeSpan timeout)
    {
        var deadline = _clock.Elapsed + timeout;
        long tEv;
        byte[] data;
        lock (_cv)
        {
            while (true)
            {
                if (_closed)
                    throw new LinkClosedException("replay link closed");
                // exhausted: wait like a quiet port (until the timeout or Close()), else the reader spins a core
                if (_rxIdx < _rx.Count && _txWritten >= _rx[_rxIdx].TxPrefix)
                    break;
                var remaining = deadline - _clock.Elapsed;
                if (remaining <= TimeSpan.Zero)
                    return Array.Empty<byte>();
                Monitor.Wait(_cv, remaining);
            }
            (tEv, data, _) = _rx[_rxIdx];
        }
        if (_realtime)
        {
            var due = TimeSpan.FromTicks(tEv * 10);
            var wait = due - _clock.Elapsed;
            if (wait > TimeSpan.Zero)
            {
                var budget = deadline - _clock.Elapsed;
                if (wait > budget)
                {
                    if (budget > TimeSpan.Zero)
                        Thread.Sleep(budget);
                    return Array.Empty<byte>();
                }
                Thread.Sleep(wait);
            }
        }
        lock (_cv)
            _rxIdx++;
        return data;
    }

    public void Write(byte[] data)
    {
        lock (_cv)
        {
            if (_closed)
                throw new LinkClosedException("replay link closed");
            if (_strictTx)
            {
                for (int i = 0; i < data.Length; i++)
                {
                    long at = _txWritten + i;
                    if (at >= _tx.Length || _tx[at] != data[i])
                    {
                        int n = (int)Math.Max(0, Math.Min(data.Length, _tx.Length - _txWritten));
                        string expected = ToHex(_tx, (int)Math.Min(_txWritten, _tx.Length), n);
                        throw new ReplayMismatchException(
                            $"replay strict_tx mismatch at offset {_txWritten}: expected {expected}, got {ToHex(data, 0, data.Length)}");
                    }
                }
            }
            _txWritten += data.Length;
            Monitor.PulseAll(_cv);
        }
    }

    public void Close()
    {
        lock (_cv)
        {
            _closed = true;
            Monitor.PulseAll(_cv);
        }
    }

    private static string ToHex(byte[] b, int offset, int count) =>
        count == 0 ? "" : BitConverter.ToString(b, offset, count).Replace("-", "").ToLowerInvariant();

    private static byte[] FromHex(string hex)
    {
        var b = new byte[hex.Length / 2];
        for (int i = 0; i < b.Length; i++)
            b[i] = byte.Parse(hex.AsSpan(2 * i, 2), NumberStyles.AllowHexSpecifier, CultureInfo.InvariantCulture);
        return b;
    }
}
