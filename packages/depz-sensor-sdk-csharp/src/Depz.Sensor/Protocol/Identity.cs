using System.Text.RegularExpressions;

namespace Depz.Sensor.Protocol;

public enum SensorType
{
    Sr04,

    /// <summary>
    /// ToF ranger. Covers both silicon variants — VL53L8CX (base) and VL53L8CH
    /// (CX + CNH). The GET_NAME_ACTIVE_SOFTWARE probe string ("APP_VL53L8…") does
    /// not distinguish them; the CX/CH split surfaces in the decode layer as
    /// <see cref="Vl53l8.Vl53l8Variant"/> and in the USB PID hint (CH = 0xED40).
    /// </summary>
    Vl53l8,

    /// <summary>
    /// VL53L4CD single-zone ToF ranger behind the thin I2C register bridge
    /// (probe string "APP_VL53L4…"; decode layer in <c>Depz.Sensor.Vl53l4</c>).
    /// </summary>
    Vl53l4,

    Bno086,
    Unknown,
}

/// <summary>
/// Classified GET_NAME_ACTIVE_SOFTWARE string (contracts/02 §4).
/// <see cref="SensorType"/> is null in bootloader mode.
/// </summary>
public sealed record Identity(string Mode, SensorType? SensorType, string SoftwareName, string Version);

public static class IdentityParser
{
    private static readonly Regex VersionRe = new(@"_v(\d+(?:\.\d+)*)$", RegexOptions.Compiled);

    private static readonly (string Token, SensorType Sensor)[] ProductTokens =
    {
        ("SR04", Protocol.SensorType.Sr04),
        ("VL53L8", Protocol.SensorType.Vl53l8),
        ("VL53L4", Protocol.SensorType.Vl53l4),
        ("BNO086", Protocol.SensorType.Bno086),
    };

    /// <summary>
    /// Classify a GET_NAME_ACTIVE_SOFTWARE string. The string must already be
    /// stripped of trailing NUL/0xFF (<see cref="Common.StripDeviceString"/>).
    /// </summary>
    public static Identity ParseSoftwareName(string name)
    {
        Match m = VersionRe.Match(name);
        string version = m.Success ? m.Groups[1].Value : "";

        if (name.StartsWith("BOOTDEPZ", StringComparison.Ordinal))
            return new Identity("bootloader", null, name, version);

        if (name.StartsWith("APP_", StringComparison.Ordinal))
        {
            foreach (var (token, sensor) in ProductTokens)
            {
                if (name.Contains(token, StringComparison.Ordinal))
                    return new Identity("app", sensor, name, version);
            }
            return new Identity("app", Protocol.SensorType.Unknown, name, version);
        }

        return new Identity("unknown", null, name, version);
    }
}
