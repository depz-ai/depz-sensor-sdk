using System.IO.Compression;
using System.Text;
using System.Text.Json;

namespace Depz.Sensor.Dataset;

/// <summary>Per-device time-sync (contract 02 §5): device_clock − host_clock, and RTT.</summary>
public sealed record TimeSync(long OffsetUs, long RttUs);

/// <summary>Header metadata for one device in a dataset (contract 09).</summary>
public sealed record DatasetDevice(
    string Id, string? Serial, string? SoftwareName, string? SensorType, TimeSync? TimeSync);

/// <summary>
/// One decoded record on the shared host timeline. <see cref="Value"/> is the
/// kind-specific payload object (players use the fields present for the kind).
/// </summary>
public sealed record DatasetRecord(string DeviceId, long THostUs, string Kind, JsonElement Value);

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

    public DatasetReader(string path)
        : this(ReadAllLines(path))
    {
    }

    private DatasetReader(IReadOnlyList<string> lines)
    {
        if (lines.Count == 0)
            throw new InvalidDataException("empty dataset (no header line)");

        using JsonDocument headerDoc = JsonDocument.Parse(lines[0]);
        JsonElement header = headerDoc.RootElement;

        var devices = new Dictionary<string, DatasetDevice>();
        if (header.TryGetProperty("devices", out JsonElement devs) && devs.ValueKind == JsonValueKind.Object)
        {
            foreach (JsonProperty dev in devs.EnumerateObject())
            {
                JsonElement m = dev.Value;
                TimeSync? sync = null;
                if (m.TryGetProperty("time_sync", out JsonElement ts) && ts.ValueKind == JsonValueKind.Object)
                {
                    sync = new TimeSync(
                        ts.GetProperty("offset_us").GetInt64(),
                        ts.TryGetProperty("rtt_us", out JsonElement rtt) ? rtt.GetInt64() : 0);
                }
                devices[dev.Name] = new DatasetDevice(
                    dev.Name,
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
            using JsonDocument doc = JsonDocument.Parse(line);
            JsonElement r = doc.RootElement;
            long t = r.GetProperty("t").GetInt64();
            var rec = new DatasetRecord(
                r.GetProperty("d").GetString()!,
                t,
                r.GetProperty("k").GetString()!,
                r.TryGetProperty("v", out JsonElement v) ? v.Clone() : default);
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

    private static string? GetStringOrNull(JsonElement obj, string name) =>
        obj.TryGetProperty(name, out JsonElement e) && e.ValueKind == JsonValueKind.String
            ? e.GetString()
            : null;

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
