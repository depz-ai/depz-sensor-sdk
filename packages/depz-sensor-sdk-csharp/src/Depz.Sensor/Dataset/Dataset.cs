using System.IO.Compression;
using System.Text;
using Depz.Sensor.Json;

namespace Depz.Sensor.Dataset;

/// <summary>Per-device time-sync (contract 02 §5): device_clock − host_clock, and RTT.</summary>
public sealed record TimeSync(long OffsetUs, long RttUs);

/// <summary>Header metadata for one device in a dataset (contract 09).</summary>
public sealed record DatasetDevice(
    string Id, string? Serial, string? SoftwareName, string? SensorType, TimeSync? TimeSync);

/// <summary>
/// One decoded record on the shared host timeline. <see cref="Value"/> is the
/// kind-specific payload object (players use the fields present for the kind),
/// as <see cref="JsonText.Parse"/> returns it: nested values are <c>long</c>,
/// <c>double</c>, <c>string</c>, <c>bool</c>, <c>null</c>, lists and dictionaries.
/// </summary>
public sealed record DatasetRecord(string DeviceId, long THostUs, string Kind, IReadOnlyDictionary<string, object?> Value);

/// <summary>
/// Reader for the <c>.depzdata</c> decoded, multi-device, time-synced dataset
/// format (contracts/09_DATASET_FORMAT.md). Plain or gzip (<c>.depzdata.gz</c>).
/// Records are exposed merged by host time <c>t</c>; unknown kinds are kept.
/// </summary>
public sealed class DatasetReader
{
    /// <summary>Device metadata from the header, keyed by device id (d0, d1, …).</summary>
    public IReadOnlyDictionary<string, DatasetDevice> Devices { get; }

    /// <summary>All records, stably merged by ascending host time.</summary>
    public IReadOnlyList<DatasetRecord> Records { get; }

    public string? Note { get; }

    /// <summary>The header line as parsed (unknown fields included).</summary>
    public IReadOnlyDictionary<string, object?> Header { get; }

    /// <summary>Host-time span from the first record to the last (0 with fewer than two records).</summary>
    public long DurationUs => Records.Count < 2 ? 0 : Records[Records.Count - 1].THostUs - Records[0].THostUs;

    public DatasetReader(string path)
        : this(ReadAllLines(path))
    {
    }

    private DatasetReader(IReadOnlyList<string> lines)
    {
        if (lines.Count == 0)
            throw new InvalidDataException("empty dataset (no header line)");

        var header = AsObject(JsonText.Parse(lines[0]), "header");
        if (!(GetStringOrNull(header, "schema") ?? "").StartsWith("depz.dataset/", StringComparison.Ordinal))
            throw new InvalidDataException("not a depz.dataset file");
        Header = header;

        var devices = new Dictionary<string, DatasetDevice>();
        if (header.TryGetValue("devices", out object? devs) && devs is Dictionary<string, object?> devMap)
        {
            foreach (var dev in devMap)
            {
                if (dev.Value is not Dictionary<string, object?> m)
                    continue;
                TimeSync? sync = null;
                if (m.TryGetValue("time_sync", out object? ts) && ts is Dictionary<string, object?> tsMap)
                {
                    sync = new TimeSync(
                        GetLong(tsMap, "offset_us"),
                        tsMap.ContainsKey("rtt_us") ? GetLong(tsMap, "rtt_us") : 0);
                }
                devices[dev.Key] = new DatasetDevice(
                    dev.Key,
                    GetStringOrNull(m, "serial"),
                    GetStringOrNull(m, "software_name"),
                    GetStringOrNull(m, "sensor_type"),
                    sync);
            }
        }
        Devices = devices;
        Note = GetStringOrNull(header, "note");

        var records = new List<(long T, int Order, DatasetRecord Rec)>();
        int order = 0;
        for (int i = 1; i < lines.Count; i++)
        {
            string line = lines[i];
            if (line.Length == 0)
                continue;
            var r = AsObject(JsonText.Parse(line), $"line {i + 1}");
            long t = GetLong(r, "t");
            var rec = new DatasetRecord(
                GetString(r, "d"),
                t,
                GetString(r, "k"),
                r.TryGetValue("v", out object? v) && v is Dictionary<string, object?> value
                    ? value
                    : new Dictionary<string, object?>());
            records.Add((t, order++, rec));
        }
        // Merge by host time; stable within equal t (append order preserved).
        records.Sort((a, b) =>
        {
            int c = a.T.CompareTo(b.T);
            return c != 0 ? c : a.Order.CompareTo(b.Order);
        });
        Records = records.Select(x => x.Rec).ToList();
    }

    /// <summary>
    /// Deliver the records paced by their timestamps (<paramref name="speed"/> 2 = twice as fast, 0 = as fast as
    /// possible), blocking until the end or <paramref name="cancel"/>. Records before <paramref name="startTUs"/>
    /// are skipped. For pause / seek / speed changes use <see cref="DatasetPlayer"/>.
    /// </summary>
    public void Play(Action<DatasetRecord> callback, double speed = 1.0, long? startTUs = null, CancellationToken cancel = default)
    {
        if (callback is null) throw new ArgumentNullException(nameof(callback));
        if (double.IsNaN(speed) || double.IsInfinity(speed) || speed < 0)
            throw new ArgumentOutOfRangeException(nameof(speed), speed, "speed must be a finite number >= 0");
        long? firstT = null;
        var wall = System.Diagnostics.Stopwatch.StartNew();
        foreach (var rec in Records)
        {
            if (startTUs is long s && rec.THostUs < s)
                continue;
            if (cancel.IsCancellationRequested)
                return;
            if (speed > 0)
            {
                firstT ??= rec.THostUs;
                double delay = (rec.THostUs - firstT.Value) / 1e6 / speed - wall.Elapsed.TotalSeconds;
                if (delay > 0 && cancel.WaitHandle.WaitOne(TimeSpan.FromSeconds(delay)))
                    return;
            }
            callback(rec);
        }
    }

    private static Dictionary<string, object?> AsObject(object? v, string where) =>
        v as Dictionary<string, object?> ?? throw new InvalidDataException($"dataset {where}: not a JSON object");

    private static long GetLong(Dictionary<string, object?> obj, string name) =>
        obj.TryGetValue(name, out object? e) && e is long l
            ? l
            : throw new InvalidDataException($"dataset: '{name}' is missing or not an integer");

    private static string GetString(Dictionary<string, object?> obj, string name) =>
        GetStringOrNull(obj, name) ?? throw new InvalidDataException($"dataset: '{name}' is missing or not a string");

    private static string? GetStringOrNull(Dictionary<string, object?> obj, string name) =>
        obj.TryGetValue(name, out object? e) ? e as string : null;

    private static IReadOnlyList<string> ReadAllLines(string path)
    {
        byte[] raw = File.ReadAllBytes(path);
        // gzip magic (also covers .depzdata.gz files without extension check).
        if (raw.Length >= 2 && raw[0] == 0x1F && raw[1] == 0x8B)
        {
            using var ms = new MemoryStream(raw);
            using var gz = new GZipStream(ms, CompressionMode.Decompress);
            using var reader = new StreamReader(gz, Encoding.UTF8);
            raw = Encoding.UTF8.GetBytes(reader.ReadToEnd());
        }
        string text = Encoding.UTF8.GetString(raw);
        return text.Split('\n').Where(l => l.Trim().Length > 0).ToArray();
    }
}
