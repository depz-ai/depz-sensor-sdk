namespace Depz.Sensor.Vl53lx.Bare;

/// <summary>One field of a register block: byte offset, width in bytes, signedness and the mask the C codec
/// applies (null: none).</summary>
public sealed record RegField(string Name, int Off, int Width, bool Signed, long? Mask);

/// <summary>One contiguous I2C block of the device image (a <c>VL53LX_*_t</c> struct of the C driver).</summary>
public sealed record RegBlockDef(int Base, int Size, bool Writable, IReadOnlyList<RegField> Fields);

/// <summary>
/// One register block's worth of device image, addressed by the C field name:
/// <c>img.StaticConfig["dss_config__target_total_rate_mcps"]</c>. Bytes no field names (the reserved holes)
/// keep whatever was read, so a read-modify-write round trip is byte-exact.
/// </summary>
public sealed class RegBlock
{
    private readonly Dictionary<string, long> _v = new();
    private byte[] _raw;

    public RegBlock(string name)
    {
        Name = name;
        Def = BareTables.Blocks[name];
        foreach (var f in Def.Fields)
            _v[f.Name] = 0;
        _raw = new byte[Def.Size];
    }

    public string Name { get; }

    public RegBlockDef Def { get; }

    public int Base => Def.Base;

    public int Size => Def.Size;

    public long this[string field]
    {
        get => _v.TryGetValue(field, out long v) ? v : throw new KeyNotFoundException($"{Name} has no field {field}");
        set
        {
            if (!_v.ContainsKey(field))
                throw new KeyNotFoundException($"{Name} has no field {field}");
            _v[field] = value;
        }
    }

    public IEnumerable<KeyValuePair<string, long>> Items() => Def.Fields.Select(f => new KeyValuePair<string, long>(f.Name, _v[f.Name]));

    public RegBlock Decode(byte[] raw)
    {
        if (raw.Length < Size)
            throw new ArgumentException($"{Name}: {raw.Length} bytes, need {Size}", nameof(raw));
        _raw = raw.AsSpan(0, Size).ToArray();
        foreach (var f in Def.Fields)
        {
            long value = 0;
            for (int i = 0; i < f.Width; i++)
                value = (value << 8) | raw[f.Off + i];
            if (f.Mask is long mask)
                value &= mask;
            int bits = f.Width * 8;
            if (f.Signed && value >= 1L << (bits - 1))
                value -= 1L << bits;
            _v[f.Name] = value;
        }
        return this;
    }

    public byte[] Encode()
    {
        var buf = _raw.ToArray();
        foreach (var f in Def.Fields)
        {
            // The field's mask and width, the way the C codec clips.
            long value = _v[f.Name];
            int bits = f.Width * 8;
            long all = bits == 64 ? -1L : (1L << bits) - 1;
            if (f.Signed)
                value &= all;
            if (f.Mask is long mask)
                value &= mask;
            value &= all;
            for (int i = f.Width - 1; i >= 0; i--)
            {
                buf[f.Off + i] = (byte)value;
                value >>= 8;
            }
        }
        return buf;
    }

    public override string ToString() => $"<{Name} 0x{Base:X4}+{Size}>";
}

/// <summary>
/// The device's register image, block by block. The C driver never reads a configuration register on its
/// own: it edits the structs, then pushes whole blocks — ranging start writes 0x0001..0x0087 in a single
/// 135-byte transfer. Every transfer costs a USB round trip, so block moves keep the count where the C
/// driver has it. Editing a field only touches the host copy; <see cref="Push"/> reaches the sensor.
/// </summary>
public sealed class DeviceImage
{
    /// <summary>The blocks <c>init_and_start_range</c> pushes, in address order: one transfer.</summary>
    public static readonly IReadOnlyList<string> RangeStartBlocks = new[]
    {
        "static_nvm_managed", "customer_nvm_managed", "static_config", "general_config", "timing_config",
        "dynamic_config", "system_control",
    };

    private readonly Vl53lxPlatform _p;
    private readonly Dictionary<string, RegBlock> _blocks;

    public DeviceImage(Vl53lxPlatform platform)
    {
        _p = platform;
        _blocks = BareTables.Blocks.Keys.ToDictionary(n => n, n => new RegBlock(n));
    }

    public RegBlock this[string name] => _blocks[name];

    public RegBlock StaticNvm => _blocks["static_nvm_managed"];
    public RegBlock CustomerNvm => _blocks["customer_nvm_managed"];
    public RegBlock StaticConfig => _blocks["static_config"];
    public RegBlock GeneralConfig => _blocks["general_config"];
    public RegBlock TimingConfig => _blocks["timing_config"];
    public RegBlock DynamicConfig => _blocks["dynamic_config"];
    public RegBlock SystemControl => _blocks["system_control"];
    public RegBlock NvmCopyData => _blocks["nvm_copy_data"];

    /// <summary>Read one block off the sensor into the image.</summary>
    public RegBlock Pull(string name)
    {
        var blk = _blocks[name];
        return blk.Decode(_p.ReadMulti(blk.Base, blk.Size));
    }

    /// <summary>Write one block from the image to the sensor.</summary>
    public void Push(string name)
    {
        var blk = _blocks[name];
        if (!blk.Def.Writable)
            throw new InvalidOperationException($"{name} is read-only");
        _p.WriteMulti(blk.Base, blk.Encode());
    }

    // (base, total size) covering the named blocks; reserved bytes between them are part of the span.
    private static (int Base, int Size) Span(IReadOnlyList<string> names)
    {
        int lo = names.Min(n => BareTables.Blocks[n].Base);
        int hi = names.Max(n => BareTables.Blocks[n].Base + BareTables.Blocks[n].Size);
        return (lo, hi - lo);
    }

    public void PullRange(IReadOnlyList<string> names)
    {
        var (b, size) = Span(names);
        byte[] raw = _p.ReadMulti(b, size);
        foreach (string name in names)
        {
            var blk = _blocks[name];
            blk.Decode(raw.AsSpan(blk.Base - b, blk.Size).ToArray());
        }
    }

    /// <summary>The named blocks as one transfer; a hole between them goes out as 0x00, as in the C driver.</summary>
    public void PushRange(IReadOnlyList<string> names)
    {
        var (b, size) = Span(names);
        var buf = new byte[size];
        foreach (string name in names)
        {
            var blk = _blocks[name];
            if (!blk.Def.Writable)
                throw new InvalidOperationException($"{name} is read-only");
            blk.Encode().CopyTo(buf, blk.Base - b);
        }
        _p.WriteMulti(b, buf);
    }

    /// <summary>Every field of the named blocks (default: all), one per line.</summary>
    public string Dump(IEnumerable<string>? names = null)
    {
        var lines = new List<string>();
        foreach (string name in names ?? BareTables.Blocks.Keys)
        {
            var blk = _blocks[name];
            lines.Add($"--- {name} @ 0x{blk.Base:X4} ({blk.Size} B)");
            foreach (var kv in blk.Items())
                lines.Add($"  {kv.Key,-52} {kv.Value}");
        }
        return string.Join("\n", lines);
    }
}

/// <summary><c>VL53LX_optical_centre_t</c>.</summary>
public sealed record OpticalCentre(int XCentre, int YCentre);

/// <summary><c>VL53LX_cal_peak_rate_map_t</c>.</summary>
public sealed record CalPeakRateMap(int CalDistanceMm, int CalReflectancePc, int MaxSamples, int Width, int Height,
    IReadOnlyList<int> PeakRateMcps);

/// <summary><c>VL53LX_additional_offset_cal_data_t</c>.</summary>
public sealed record AdditionalOffsetCalData(long MmInnerActualEffectiveSpads, long MmOuterActualEffectiveSpads,
    long MmInnerPeakSignalCountRtnMcps, long MmOuterPeakSignalCountRtnMcps);

/// <summary><c>VL53LX_decoded_nvm_fmt_range_data_t</c>.</summary>
public sealed record FmtRangeData(int ActualEffectiveRtnSpads, int NumRequestedRefSpads, int RefLocation,
    int PeakSignalCountRateRtnMcps, int AmbientCountRateRtnMcps, int PeakSignalCountRateRefMcps,
    int AmbientCountRateRefMcps, int MeasuredDistanceMm, int MeasuredDistanceStdevMm);

/// <summary>
/// The raw NVM, which the register blocks do not cover: the optical centre, the 5x5 peak-rate map, the
/// mode-mitigation offset calibration and the FMT range results. Each word costs four writes and a read,
/// so only the regions the driver wants are read, each inside its own enable/disable pair so the sensor is
/// never left with its firmware stopped (vl53lx_nvm.c).
/// </summary>
public sealed class NvmReader
{
    private const int PowerManagementGo1PowerForce = 0x0083;
    private const int FirmwareEnable = 0x0085;
    private const int RangingCoreClkCtrl1 = 0x0683;
    private const int NvmCtrlMode = 0x0780;
    private const int NvmCtrlPdn = 0x0781;
    private const int NvmCtrlReadn = 0x0783;
    private const int NvmCtrlPulseWidthMsb = 0x0784;
    private const int NvmCtrlDataoutMmm = 0x0790;
    private const int NvmCtrlAddr = 0x0794;
    private const int NvmCtrlPulseWidth = 0x0004;

    public static readonly (int Index, int Size) FmtOpticalCentre = (0x00B8, 4);
    public static readonly (int Index, int Size) FmtCalPeakRateMap = (0x015C, 56);
    public static readonly (int Index, int Size) FmtAdditionalOffsetCal = (0x0194, 8);
    public static readonly (int Index, int Size) FmtRangeResults140mmDark = (0x01AC, 16);

    private const int PeakRateMapWidth = 5, PeakRateMapHeight = 5, PeakRateMapSamples = 25;

    private readonly Vl53lxPlatform _p;
    private readonly DeviceImage _img;

    public NvmReader(Vl53lxPlatform platform, DeviceImage image)
    {
        _p = platform;
        _img = image;
    }

    private static int U16(byte[] buf, int off) => (buf[off] << 8) | buf[off + 1];

    private void Enable()
    {
        // disable_firmware / enable_powerforce, through system_control so the image stays truthful.
        _img.SystemControl["firmware__enable"] = 0;
        _p.WriteByte(FirmwareEnable, 0);
        _img.SystemControl["power_management__go1_power_force"] = 1;
        _p.WriteByte(PowerManagementGo1PowerForce, 1);
        _p.SleepMs(1); // 250 us settling, rounded up

        _p.WriteByte(NvmCtrlPdn, 0x01);
        _p.WriteByte(RangingCoreClkCtrl1, 0x05);
        _p.SleepMs(1); // 50 us power-up, rounded up
        _p.WriteByte(NvmCtrlMode, 0x01);
        _p.WriteWord(NvmCtrlPulseWidthMsb, NvmCtrlPulseWidth);
    }

    private void Disable()
    {
        _p.WriteByte(NvmCtrlReadn, 0x01);
        _p.WriteByte(NvmCtrlPdn, 0x00);
        _img.SystemControl["power_management__go1_power_force"] = 0;
        _p.WriteByte(PowerManagementGo1PowerForce, 0);
        _img.SystemControl["firmware__enable"] = 1;
        _p.WriteByte(FirmwareEnable, 1);
    }

    private byte[] ReadWords(int startWord, int count)
    {
        var output = new List<byte>();
        for (int addr = startWord; addr < startWord + count; addr++)
        {
            _p.WriteByte(NvmCtrlAddr, addr);
            _p.WriteByte(NvmCtrlReadn, 0x00);
            // 5 us trigger delay: the next USB round trip is the wait.
            _p.WriteByte(NvmCtrlReadn, 0x01);
            output.AddRange(_p.ReadMulti(NvmCtrlDataoutMmm, 4));
        }
        return output.ToArray();
    }

    /// <summary>VL53LX_read_nvm_raw_data(): (byte index, byte size), both word-aligned → the bytes.</summary>
    public byte[] ReadRegion((int Index, int Size) region)
    {
        var (index, size) = region;
        if ((index & 3) != 0 || (size & 3) != 0)
            throw new ArgumentException($"NVM region 0x{index:X4}+{size} is not word-aligned", nameof(region));
        Enable();
        try
        {
            return ReadWords(index >> 2, size >> 2);
        }
        finally
        {
            Disable();
        }
    }

    public OpticalCentre ReadOpticalCentre()
    {
        byte[] buf = ReadRegion(FmtOpticalCentre);
        // Stored as a distance down from 0x0100; on a never-programmed part the driver zeroes it, which is
        // what makes read_p2p_data fall back to the mm ROI centre.
        int x = 0x0100 - buf[2];
        return new OpticalCentre(x <= 0xFF ? x : 0, buf[3]);
    }

    public CalPeakRateMap ReadCalPeakRateMap()
    {
        byte[] buf = ReadRegion(FmtCalPeakRateMap);
        return new CalPeakRateMap(U16(buf, 0), U16(buf, 2) >> 6, PeakRateMapSamples, PeakRateMapWidth, PeakRateMapHeight,
            Enumerable.Range(0, PeakRateMapSamples).Select(i => U16(buf, 4 + 2 * i)).ToArray());
    }

    public AdditionalOffsetCalData ReadAdditionalOffsetCalData()
    {
        byte[] buf = ReadRegion(FmtAdditionalOffsetCal);
        return new AdditionalOffsetCalData(U16(buf, 0), U16(buf, 2), U16(buf, 4), U16(buf, 6));
    }

    public FmtRangeData ReadFmtRangeResults((int Index, int Size)? region = null)
    {
        byte[] buf = ReadRegion(region ?? FmtRangeResults140mmDark);
        return new FmtRangeData(U16(buf, 0), buf[2], buf[3], U16(buf, 4), U16(buf, 6), U16(buf, 8), U16(buf, 10),
            U16(buf, 12), U16(buf, 14));
    }
}
