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

    /// <summary>
    /// VL53L5CX / VL53L7CX / VL53L7CH multizone ToF rangers behind the I2C
    /// register bridge (probe string "APP_VL53L7…", contract 11). One firmware
    /// serves all three; the class is resolved separately
    /// (<see cref="Depz.Sensor.Vl53l7.Vl53l7Discovery.ResolveModel"/>); decode layer in
    /// <c>Depz.Sensor.Vl53l7</c>.
    /// </summary>
    Vl53l7,

    /// <summary>
    /// VL53L 1D ToF family — VL53L0X / L1CX / L1CB / L3CX / L4CD / L4CX — behind
    /// the v2.00 register bridge (probe string "APP_VL53L0_4…" or "APP_VL53LX…",
    /// contract 12). One firmware serves all six; the product/class is resolved
    /// separately (<see cref="Depz.Sensor.Vl53lx.Vl53lxProducts.ResolveClass"/>);
    /// decode layer in <c>Depz.Sensor.Vl53lx</c>.
    /// </summary>
    Vl53lx,

    Bno086,

    /// <summary>
    /// BNO055 9-axis IMU behind the thin I2C register bridge (probe string
    /// "APP_BNO055…", contract 13). Fusion runs on the chip; wire codecs and
    /// the register codecs live in <c>Depz.Sensor.Bno055</c>.
    /// </summary>
    Bno055,

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
        // The 1D-family tokens go first: none of the other tokens is a substring
        // of them today, but "APP_VL53L0_4" must never fall through to VL53L4.
        ("VL53L0_4", Protocol.SensorType.Vl53lx),
        ("VL53LX", Protocol.SensorType.Vl53lx),
        ("SR04", Protocol.SensorType.Sr04),
        ("VL53L8", Protocol.SensorType.Vl53l8),
        ("VL53L4", Protocol.SensorType.Vl53l4),
        ("VL53L7", Protocol.SensorType.Vl53l7),
        ("BNO086", Protocol.SensorType.Bno086),
        ("BNO055", Protocol.SensorType.Bno055),
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
