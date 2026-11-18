using System.Text.RegularExpressions;
using Depz.Sensor.Vl53l8;

namespace Depz.Sensor.Vl53l7;

/// <summary>
/// The three sensors one <c>APP_VL53L7</c> firmware serves (contract 11). They
/// stream the VL53L8 results-frame layout with the L5/L7 geometry (footer id at
/// size − 4, per-zone trim — see <see cref="Vl53l7Frames.CreateDecoder"/>);
/// only <see cref="Vl53l7ch"/> adds CNH (decoded by <see cref="Vl53l8Cnh"/>).
/// </summary>
public enum Vl53l7Model
{
    /// <summary>VL53L5CX — 63° FoV, production PID 0xED48, blob set l7cx.</summary>
    Vl53l5cx,

    /// <summary>VL53L7CX — 90° FoV, production PID 0xED49, blob set l7cx (base class).</summary>
    Vl53l7cx,

    /// <summary>VL53L7CH — VL53L7CX plus CNH, production PID 0xED4A, blob set l7ch.</summary>
    Vl53l7ch,
}

/// <summary>Class resolution for an <c>APP_VL53L7</c> board (contract 11 §1).</summary>
public static class Vl53l7Discovery
{
    private static readonly Regex PartRe = new(@"VL53L([57])(CX|CH)", RegexOptions.Compiled);

    /// <summary>
    /// Pick the sensor class. The firmware name cannot tell the three apart and
    /// the silicon never tells CX from CH, so (normative order): the production
    /// USB PID model (<see cref="Usb.UsbIds.UsbModelHint"/>), else the first
    /// <c>VL53L([57])(CX|CH)</c> match in GET_DEVICE_NAME, else
    /// <see cref="Vl53l7Model.Vl53l7cx"/> (its blob runs on every L5/L7 part).
    /// </summary>
    /// <param name="usbModel">PID model hint ("vl53l5cx" / "vl53l7cx" / "vl53l7ch"), or null.</param>
    /// <param name="deviceName">GET_DEVICE_NAME string (may be empty or null).</param>
    public static Vl53l7Model ResolveModel(string? usbModel, string? deviceName)
    {
        Vl53l7Model? byPid = FromName(usbModel);
        if (byPid is not null)
            return byPid.Value;
        Match m = PartRe.Match(deviceName ?? "");
        if (m.Success)
            return FromName($"vl53l{m.Groups[1].Value}{m.Groups[2].Value}".ToLowerInvariant()) ?? Vl53l7Model.Vl53l7cx;
        return Vl53l7Model.Vl53l7cx;
    }

    /// <summary>True for the model that carries CNH histograms (VL53L7CH).</summary>
    public static bool HasCnh(Vl53l7Model model) => model == Vl53l7Model.Vl53l7ch;

    private static Vl53l7Model? FromName(string? name) => name switch
    {
        "vl53l5cx" => Vl53l7Model.Vl53l5cx,
        "vl53l7cx" => Vl53l7Model.Vl53l7cx,
        "vl53l7ch" => Vl53l7Model.Vl53l7ch,
        _ => null,
    };
}

/// <summary>
/// L5/L7 results-frame decoding (contract 11 §3). The frame layout, scaling and
/// CNH block are VL53L8's (<see cref="Vl53l8FrameDecoder"/>); two things differ:
/// <list type="bullet">
///   <item>the frame-id footer sits at size − 4 for every L5/L7 class (both
///   ULD 2.0.1 and VL53LMZ 2.0.16);</item>
///   <item>per-target blocks keep 64 entries even in 4×4 — the decoder trims
///   every per-zone array to the frame's resolution, inferred from the
///   zone-scaled ambient block (index 0x54D0).</item>
/// </list>
/// Chunks are reassembled with the shared <see cref="FrameReassembler"/>
/// (chunk size <see cref="Vl53l7Wire.StreamChunkMax"/> does not affect it).
/// </summary>
public static class Vl53l7Frames
{
    /// <summary>Frame-id footer offset from the frame end, for every L5/L7 class.</summary>
    public const int FooterIdOffset = 4;

    /// <summary>Minimum ranging frequency: L5/L7 range at 1 Hz (VL53L8: 2 Hz).</summary>
    public const int MinRangingFrequencyHz = 1;

    /// <summary>A decoder for L5/L7 frames (any of the three classes).</summary>
    public static Vl53l8FrameDecoder CreateDecoder() => new(FooterIdOffset, trimToZones: true);
}
