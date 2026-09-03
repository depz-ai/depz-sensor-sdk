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
