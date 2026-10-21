namespace Depz.Sensor.Tmf8829;

/// <summary>
/// TMF8829 presets and config-page fields (contracts/14_SENSOR_TMF8829.md §4).
/// Mirrors <c>depz_sensor_sdk/tmf8829/config.py</c>. A preset load is not a
/// clean slate: hosts write <c>period_ms</c>, <c>kilo_iterations</c>,
/// <c>nr_peaks</c>, the three strengths, <c>dump_histograms</c> and
/// <c>ref_spad_frame</c> on every configure.
/// </summary>
public static class Tmf8829Config
{
    /// <summary>Config page register 0x22..0xDF.</summary>
    public const int RegCfg = 0x22;

    /// <summary>190 B.</summary>
    public const int CfgSize = 0xDF - RegCfg + 1;

    /// <summary>The sensor's own period after init().</summary>
    public const int DefaultPeriodMs = 33;

    /// <summary>Preset name → load command (each loads its config page).</summary>
    public static readonly IReadOnlyDictionary<string, byte> Presets = new Dictionary<string, byte>
    {
        ["8x8"] = 0x40,
        ["8x8-lr"] = 0x41,   // long range
        ["8x8-ha"] = 0x42,   // high accuracy
        ["16x16"] = 0x43,
        ["16x16-ha"] = 0x44,
        ["32x32"] = 0x45,
        ["32x32-ha"] = 0x46,
        ["48x32"] = 0x47,
        ["48x32-ha"] = 0x48,
        ["8x8-xr"] = 0x49,   // extended range, distances in mm
    };

    /// <summary>The preset's own kilo_iterations (a preset rescales a stale value, it does not reset it).</summary>
    public static readonly IReadOnlyDictionary<string, int> PresetKiloIterations = new Dictionary<string, int>
    {
        ["8x8"] = 586,
        ["8x8-lr"] = 586,
        ["8x8-ha"] = 586,
        ["8x8-xr"] = 586,
        ["16x16"] = 586,
        ["16x16-ha"] = 586,
        ["32x32"] = 2344,
        ["32x32-ha"] = 2344,
        ["48x32"] = 3516,
        ["48x32-ha"] = 3516,
    };

    /// <summary>Field → (register, bit shift, width in bits); width 16 = little-endian register pair.</summary>
    public static readonly IReadOnlyDictionary<string, (int Reg, int Shift, int Width)> CfgFields =
        new Dictionary<string, (int, int, int)>
        {
            ["period_ms"] = (0x22, 0, 16),
            ["kilo_iterations"] = (0x24, 0, 16),
            ["fp_mode"] = (0x26, 0, 3),
            ["nr_peaks"] = (0x2A, 0, 3),
            ["signal_strength"] = (0x2A, 3, 1),
            ["noise_strength"] = (0x2A, 4, 1),
            ["xtalk"] = (0x2A, 5, 1),
            ["dump_histograms"] = (0x2B, 0, 1),
            ["ref_spad_frame"] = (0x2C, 0, 1),
            ["power_modes"] = (0x2E, 0, 8),
            ["ha_kilo_iterations"] = (0x4A, 0, 16),
            ["dual_mode"] = (0x4C, 0, 2),
            ["fov_correction"] = (0x78, 0, 4),
            ["distance_in_mm"] = (0x8F, 0, 1),
        };

    /// <summary>Read a named field from a config page (page byte 0 = register 0x22).</summary>
    /// <exception cref="KeyNotFoundException">Unknown field name.</exception>
    public static int CfgGet(ReadOnlySpan<byte> page, string name)
    {
        var (reg, shift, width) = CfgFields[name];
        int i = reg - RegCfg;
        if (width == 16)
            return page[i] | (page[i + 1] << 8);
        return (page[i] >> shift) & ((1 << width) - 1);
    }

    /// <summary>Write a named field into a config page in place.</summary>
    /// <exception cref="ArgumentOutOfRangeException">Value does not fit the field width.</exception>
    /// <exception cref="KeyNotFoundException">Unknown field name.</exception>
    public static void CfgSet(Span<byte> page, string name, int value)
    {
        var (reg, shift, width) = CfgFields[name];
        int i = reg - RegCfg;
        if (value < 0 || value >= (1 << width))
            throw new ArgumentOutOfRangeException(nameof(value), value, $"{name}: {value} out of range for {width} bit");
        if (width == 16)
        {
            page[i] = (byte)(value & 0xFF);
            page[i + 1] = (byte)(value >> 8);
        }
        else
        {
            int mask = ((1 << width) - 1) << shift;
            page[i] = (byte)((page[i] & ~mask) | (value << shift));
        }
    }

    /// <summary>Every named field of a config page.</summary>
    public static Dictionary<string, int> ConfigFields(ReadOnlySpan<byte> page)
    {
        var o = new Dictionary<string, int>();
        foreach (var name in CfgFields.Keys)
            o[name] = CfgGet(page, name);
        return o;
    }
}
