using System.Globalization;
using System.IO.Compression;
using System.Text;
using Depz.Sensor.Devices;
using Depz.Sensor.Json;
using Depz.Sensor.Protocol;

namespace Depz.Sensor.Dataset;

/// <summary>Low-level thread-safe <c>.depzdata</c> record writer (gzip when the path ends in <c>.gz</c>).
/// Prefer <see cref="SessionRecorder"/>.</summary>
public sealed class DatasetWriter : IDisposable
{
    /// <summary>The schema id this SDK writes.</summary>
    public const string Schema = "depz.dataset/2";

    private readonly object _lock = new();
    private readonly Stream _stream;
    private readonly StreamWriter _writer;
    private bool _closed;

    /// <param name="path">The file to create (overwritten).</param>
    /// <param name="devices">Header device entries by device id (<c>serial</c>, <c>sensor_type</c>,
    /// <c>software_name</c>, <c>time_sync</c>, …).</param>
    /// <param name="note">Optional free text for the header.</param>
    public DatasetWriter(string path, IReadOnlyDictionary<string, IReadOnlyDictionary<string, object?>> devices, string note = "")
    {
        if (path is null) throw new ArgumentNullException(nameof(path));
        if (devices is null) throw new ArgumentNullException(nameof(devices));
        Stream file = File.Create(path);
        _stream = path.EndsWith(".gz", StringComparison.OrdinalIgnoreCase) ? new GZipStream(file, CompressionLevel.Optimal) : file;
        _writer = new StreamWriter(_stream, new UTF8Encoding(false)) { NewLine = "\n" };
        var header = new Dictionary<string, object?>
        {
            ["schema"] = Schema,
            ["created_utc"] = DateTime.UtcNow.ToString("yyyy-MM-dd'T'HH:mm:ss'Z'", CultureInfo.InvariantCulture),
            ["devices"] = devices.ToDictionary(kv => kv.Key, kv => (object?)kv.Value.ToDictionary(x => x.Key, x => x.Value)),
        };
        if (note.Length > 0)
            header["note"] = note;
        _writer.WriteLine(JsonText.Serialize(header));
    }

    public long RecordsWritten { get; private set; }

    /// <summary>One record: device id, host µs, kind, value.</summary>
    public void Write(string deviceId, long tHostUs, string kind, IReadOnlyDictionary<string, object?> value)
    {
        string line = JsonText.Serialize(new Dictionary<string, object?>
        {
            ["d"] = deviceId, ["t"] = tHostUs, ["k"] = kind, ["v"] = value,
        });
        lock (_lock)
        {
            if (_closed)
                throw new ObjectDisposedException(nameof(DatasetWriter));
            _writer.WriteLine(line);
            RecordsWritten++;
        }
    }

    public void Close()
    {
        lock (_lock)
        {
            if (_closed)
                return;
            _closed = true;
            _writer.Flush();
            _writer.Dispose();
        }
    }

    public void Dispose() => Close();
}

/// <summary>
/// Record decoded data from several devices onto one host timeline (contract 09). <see cref="Add"/> each open
/// device (it runs <see cref="DepzDevice.SyncTime"/>), then <see cref="Start"/>: the recorder hooks every
/// device's result stream and writes a record per result at device time − offset. It never reconfigures a
/// device — start and stop streaming yourself. <see cref="Stop"/> (or disposing) unhooks and closes the file.
/// </summary>
public sealed class SessionRecorder : IDisposable
{
    private readonly string _path;
    private readonly string _note;
    private readonly bool _vl53l8Layers;
    private readonly double? _temperatureHz;
    private readonly List<(string Id, DepzDevice Device, Dictionary<string, object?> Meta, List<IDisposable> Hooks)> _entries = new();
    private readonly Dictionary<string, double> _lastSilicon = new();
    private DatasetWriter? _writer;
    private Thread? _tempThread;
    private ManualResetEventSlim? _tempStop;

    /// <param name="path">The <c>.depzdata</c> (or <c>.depzdata.gz</c>) to write.</param>
    /// <param name="note">Header note.</param>
    /// <param name="vl53l8Layers">VL53L8 records also carry signal / ambient / sigma / reflectance.</param>
    /// <param name="temperatureHz">About this many <c>temperature</c> records per device and second (the MCU
    /// thermometer, and the sensor die where frames carry one); null for none.</param>
    public SessionRecorder(string path, string note = "", bool vl53l8Layers = false, double? temperatureHz = 1.0)
    {
        _path = path ?? throw new ArgumentNullException(nameof(path));
        _note = note;
        _vl53l8Layers = vl53l8Layers;
        _temperatureHz = temperatureHz;
    }

    /// <summary>Register a device before recording starts; runs <see cref="DepzDevice.SyncTime"/> so its
    /// timestamps land on the shared host timeline. Returns its device id (<c>d0</c>, <c>d1</c>, …).</summary>
    public string Add(DepzDevice device, string? deviceId = null, int syncSamples = 5)
    {
        if (device is null) throw new ArgumentNullException(nameof(device));
        if (_writer is not null)
            throw new InvalidOperationException("Add() every device before recording starts");
        string id = deviceId ?? $"d{_entries.Count}";
        var sync = device.SyncTime(syncSamples);
        var meta = new Dictionary<string, object?>
        {
            ["time_sync"] = new Dictionary<string, object?> { ["offset_us"] = sync.OffsetUs, ["rtt_us"] = sync.RttUs },
        };
        try
        {
            meta["serial"] = device.GetSerialNumber();
            meta["software_name"] = device.GetSoftwareName();
        }
        catch (DepzException)
        {
            // a board that does not answer these is still recorded
        }
        var ident = IdentityParser.ParseSoftwareName(meta.TryGetValue("software_name", out var sw) && sw is string s ? s : "");
        meta["sensor_type"] = ident.SensorType is SensorType st ? st.ToString().ToLowerInvariant() : "unknown";
        _entries.Add((id, device, meta, new List<IDisposable>()));
        return id;
    }

    public long RecordsWritten => _writer?.RecordsWritten ?? 0;

    /// <summary>Open the file and hook every device's stream.</summary>
    public void Start()
    {
        if (_writer is not null)
            return;
        _writer = new DatasetWriter(_path,
            _entries.ToDictionary(e => e.Id, e => (IReadOnlyDictionary<string, object?>)e.Meta), _note);
        foreach (var e in _entries)
            Hook(e.Id, e.Device, (long)((Dictionary<string, object?>)e.Meta["time_sync"]!)["offset_us"]!, e.Hooks);
        if (_temperatureHz is double hz && hz > 0)
        {
            _tempStop = new ManualResetEventSlim(false);
            var stop = _tempStop;
            var period = TimeSpan.FromSeconds(1.0 / hz);
            _tempThread = new Thread(() => TemperatureLoop(period, stop)) { IsBackground = true, Name = "depz-rec-temp" };
            _tempThread.Start();
        }
    }

    private void TemperatureLoop(TimeSpan period, ManualResetEventSlim stop)
    {
        while (!stop.Wait(period))
        {
            var writer = _writer;
            if (writer is null)
                return;
            foreach (var e in _entries)
            {
                double? silicon;
                lock (_lastSilicon)
                    silicon = _lastSilicon.TryGetValue(e.Id, out double c) ? c : null;
                if (silicon is double sc)
                    writer.Write(e.Id, DepzDevice.HostNowUs(), "temperature", new Dictionary<string, object?> { ["celsius"] = sc, ["source"] = "sensor" });
                double celsius;
                try
                {
                    celsius = e.Device.ReadMcuTemperature();
                }
                catch (Exception ex) when (ex is DepzException || ex is ObjectDisposedException)
                {
                    continue;
                }
                writer.Write(e.Id, DepzDevice.HostNowUs(), "temperature", new Dictionary<string, object?> { ["celsius"] = celsius, ["source"] = "mcu" });
            }
        }
    }

    private void Hook(string id, DepzDevice dev, long offset, List<IDisposable> hooks)
    {
        var w = _writer!;
        long T(ulong deviceUs) => (long)deviceUs - offset;
        switch (dev)
        {
            case Sr04Device sr04:
                hooks.Add(sr04.OnMeasurement(m => w.Write(id, T(m.TimestampUs), "sr04", DatasetRecords.Sr04(m))));
                break;
            case Vl53l8cxDevice l8:
                hooks.Add(l8.OnFrame(f =>
                {
                    long t = T(f.TimestampUs);
                    lock (_lastSilicon)
                        _lastSilicon[id] = f.SiliconTempDegc;
                    w.Write(id, t, "vl53l8", DatasetRecords.Vl53l8(f, _vl53l8Layers));
                    // CH: the raw CNH block riding this frame, with the armed setup's decode parameters.
                    if (f.CnhRaw is { Length: > 0 } cnh && l8.CnhSetupArmed is { } setup)
                        w.Write(id, t, "vl53l8_cnh", DatasetRecords.Vl53l8Cnh(cnh, setup));
                }));
                break;
            case Vl53l4cdDevice l4:
                hooks.Add(l4.OnMeasurement(m => w.Write(id, T(m.TimestampUs), "vl53l4", DatasetRecords.Vl53l4(m))));
                break;
            case Vl53lxDevice lx:
                hooks.Add(lx.OnMeasurement(m => w.Write(id, T(m.TimestampUs), "vl53lx", DatasetRecords.Vl53lx(m, lx.Product, lx.DriverKind))));
                break;
            case Bno055Device b55:
                hooks.Add(b55.OnSample(s => w.Write(id, T(s.TimestampUs), "bno055", DatasetRecords.Bno055(s))));
                break;
            case Tmf8829Device tmf:
                hooks.Add(tmf.OnMeasurement(m => w.Write(id, m.TimestampUs - offset, "tmf8829", DatasetRecords.Tmf8829(m))));
                break;
            case Bno086Device b86:
                // Every enabled sensor's report at its measurement time; reports of one cargo carry out-of-order
                // device times, so t may step back within a device — readers sort (contract 09).
                hooks.Add(b86.OnReport(r => w.Write(id, r.TimestampUs - offset, "bno086", DatasetRecords.Bno086(r))));
                break;
        }
    }

    /// <summary>Unhook every device and close the file.</summary>
    public void Stop()
    {
        _tempStop?.Set();
        _tempThread?.Join(TimeSpan.FromSeconds(3));
        _tempThread = null;
        foreach (var e in _entries)
        {
            foreach (var h in e.Hooks)
                h.Dispose();
            e.Hooks.Clear();
        }
        _writer?.Close();
        _writer = null;
    }

    public void Dispose() => Stop();
}
