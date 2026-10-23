using System.Text.Json;

namespace Depz.Sensor.Tests;

/// <summary>Locates and loads the shared golden vectors under contracts/vectors.</summary>
public static class Vectors
{
    private static readonly string Dir = LocateVectorsDir();

    private static string LocateVectorsDir()
    {
        // Allow an explicit override, else walk up from the test binary directory.
        string? env = Environment.GetEnvironmentVariable("DEPZ_VECTORS_DIR");
        if (!string.IsNullOrEmpty(env) && Directory.Exists(env))
            return env;

        var dir = new DirectoryInfo(AppContext.BaseDirectory);
        while (dir is not null)
        {
            string candidate = Path.Combine(dir.FullName, "contracts", "vectors");
            if (Directory.Exists(candidate))
                return candidate;
            dir = dir.Parent;
        }
        throw new DirectoryNotFoundException(
            "Could not locate contracts/vectors above " + AppContext.BaseDirectory);
    }

    /// <summary>The top-level vector files (<c>*.json</c>), by file name, sorted.</summary>
    public static IEnumerable<string> Files() =>
        Directory.GetFiles(Dir, "*.json").Select(Path.GetFileName).OfType<string>().Order(StringComparer.Ordinal);

    public static JsonElement Load(string fileName)
    {
        string path = Path.Combine(Dir, fileName);
        using var fs = File.OpenRead(path);
        using var doc = JsonDocument.Parse(fs);
        // Clone so the element survives disposal of the document.
        return doc.RootElement.Clone();
    }

    /// <summary>Absolute path to a committed fixture under contracts/vectors/recordings.</summary>
    public static string RecordingPath(string fileName) => Path.Combine(Dir, "recordings", fileName);

    /// <summary>
    /// Every packet the device sent in a .depzrec capture, in order: the rx byte
    /// stream through the framing parser (decode-only SDK — no tx replay).
    /// </summary>
    public static List<Transport.Packet> RxPackets(string fileName)
    {
        var parser = new Transport.PacketParser();
        var packets = new List<Transport.Packet>();
        string[] lines = File.ReadAllLines(RecordingPath(fileName));
        for (int i = 1; i < lines.Length; i++) // line 0 is the header
        {
            if (lines[i].Length == 0)
                continue;
            using var ev = JsonDocument.Parse(lines[i]);
            if (ev.RootElement.GetProperty("dir").GetString() != "rx")
                continue;
            foreach (var e in parser.Feed(Hex(ev.RootElement.GetProperty("data").GetString()!)))
                if (e is Transport.Packet p)
                    packets.Add(p);
        }
        return packets;
    }

    private static readonly object SeenLock = new();

    /// <summary>
    /// Report a golden-vector case as asserted (scripts/check_vector_coverage.py):
    /// appends its id to $DEPZ_VECTOR_SEEN, no-op when unset. Call it only after
    /// the case's assertions passed.
    /// </summary>
    public static void Seen(string id)
    {
        string? path = Environment.GetEnvironmentVariable("DEPZ_VECTOR_SEEN");
        if (string.IsNullOrEmpty(path))
            return;
        byte[] line = System.Text.Encoding.UTF8.GetBytes(id + "\n");
        lock (SeenLock)
        {
            // FileShare.None takes an exclusive lock, so appends from parallel
            // test processes never interleave; retry while another holds it.
            for (int attempt = 0; ; attempt++)
            {
                try
                {
                    using var fs = new FileStream(path, FileMode.Append, FileAccess.Write, FileShare.None);
                    fs.Write(line);
                    return;
                }
                catch (IOException) when (attempt < 200)
                {
                    Thread.Sleep(5);
                }
            }
        }
    }

    /// <summary>A case object: report its <c>id</c>.</summary>
    public static void Seen(JsonElement testCase) => Seen(testCase.GetProperty("id").GetString()!);

    /// <summary>Parse a lowercase hex string into bytes (empty string → empty array).</summary>
    public static byte[] Hex(string s)
    {
        if (s.Length == 0)
            return Array.Empty<byte>();
        if ((s.Length & 1) != 0)
            throw new ArgumentException("odd-length hex string: " + s);
        var outBytes = new byte[s.Length / 2];
        for (int i = 0; i < outBytes.Length; i++)
            outBytes[i] = Convert.ToByte(s.Substring(i * 2, 2), 16);
        return outBytes;
    }

    public static string ToHex(ReadOnlySpan<byte> data)
    {
        var sb = new System.Text.StringBuilder(data.Length * 2);
        foreach (byte b in data)
            sb.Append(b.ToString("x2"));
        return sb.ToString();
    }
}
