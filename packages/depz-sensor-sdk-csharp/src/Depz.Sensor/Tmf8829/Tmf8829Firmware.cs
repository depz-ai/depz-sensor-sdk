using Depz.Sensor.Devices;

namespace Depz.Sensor.Tmf8829;

/// <summary>
/// The TMF8829 sensor firmware the host downloads through the sensor
/// bootloader on every init (contract 14 §4): ams
/// <c>tmf8829_application.hex</c>, application 2.200.0, one contiguous
/// segment, converted to a flat image at build time. MIT-licensed — the
/// license text ships with the package (<c>LICENSES-MIT.TXT</c>, also
/// <see cref="License"/>).
/// </summary>
public static class Tmf8829Firmware
{
    /// <summary>Load address of the image (BL W_FIFO_BOTH <c>addr</c>).</summary>
    public const uint LoadAddr = 0x00010000;

    /// <summary>Image size in bytes (already a multiple of 4: 3611 words).</summary>
    public const int Size = 14444;

    private const string ImageResource = "Depz.Sensor.Tmf8829.tmf8829_application.bin";
    private const string LicenseResource = "Depz.Sensor.Tmf8829.LICENSES-MIT.TXT";

    private static readonly Lazy<byte[]> ImageBytes = new(() => ReadResource(ImageResource));

    /// <summary>A copy of the firmware image (14 444 B).</summary>
    public static byte[] Image() => (byte[])ImageBytes.Value.Clone();

    /// <summary>The image without copying.</summary>
    public static ReadOnlySpan<byte> ImageSpan => ImageBytes.Value;

    /// <summary>The ams MIT license text of the image.</summary>
    public static string License()
    {
        using var s = Open(LicenseResource);
        using var r = new StreamReader(s);
        return r.ReadToEnd();
    }

    private static Stream Open(string name) =>
        typeof(Tmf8829Firmware).Assembly.GetManifestResourceStream(name)
        ?? throw new InvalidStateException($"embedded resource {name} missing");

    private static byte[] ReadResource(string name)
    {
        using var s = Open(name);
        using var ms = new MemoryStream();
        s.CopyTo(ms);
        return ms.ToArray();
    }
}
