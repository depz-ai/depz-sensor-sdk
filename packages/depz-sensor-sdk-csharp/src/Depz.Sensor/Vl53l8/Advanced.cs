using System.Buffers.Binary;

namespace Depz.Sensor.Vl53l8;

/// <summary>
/// VL53L8 advanced-feature DCI codecs — pure, verifiable ports of the ST ULD
/// (BSD-3-Clause) plugin encoders: motion-indicator configuration,
/// detection-threshold block, and xtalk-margin scaling. Shared across both ToF
/// variants (VL53L8CX and VL53L8CH; see <see cref="Vl53l8Variant"/>). The live
/// DCI read/write transport that carries these to the sensor is
/// hardware-dependent and out of scope (see <see cref="Vl53l8Uld"/>).
/// </summary>
public static class Vl53l8Advanced
{
    public const int Resolution4x4 = 16;
    public const int Resolution8x8 = 64;

    // Detection-threshold measurement selectors + scale factors (plugin source):
    // get divides, set multiplies by these.
    public const int DistMm = 1;
    public const int SignalPerSpadKcps = 2;
    public const int RangeSigmaMm = 4;
    public const int AmbientPerSpadKcps = 8;
    public const int NbSpadsEnabled = 13;
    public const int MotionIndicator = 19;

    private static readonly IReadOnlyDictionary<int, int> ThreshScale = new Dictionary<int, int>
    {
        [DistMm] = 4,
        [SignalPerSpadKcps] = 2048,
        [RangeSigmaMm] = 128,
        [AmbientPerSpadKcps] = 2048,
        [NbSpadsEnabled] = 256,
        [MotionIndicator] = 65535,
    };

    public const int NbThresholds = 64;

    /// <summary>Xtalk margin raw DCI value: round(kcps × 2048).</summary>
    public static uint XtalkMarginRaw(double marginKcps) =>
        (uint)(long)Math.Round(marginKcps * 2048.0, MidpointRounding.AwayFromZero);

    /// <summary>One detection threshold (raw real-unit low/high, before scaling).</summary>
    public sealed record DetectionThreshold(
        int LowThresh, int HighThresh, int Measurement, int Type, int ZoneNum, int Operation);

    /// <summary>The 8-byte DCI_DET_THRESH_VALID_STATUS payload written before the block.</summary>
    public static byte[] ThresholdValidStatus() => new byte[] { 5, 5, 5, 5, 5, 5, 5, 5 };

    /// <summary>
    /// Encode the 64×12-byte DCI_DET_THRESH_START block. Each entry is packed as
    /// (i32 low, i32 high, u8 measurement, u8 type, u8 zone, u8 op) with low/high
    /// multiplied by the measurement's scale factor; missing entries are zeros.
    /// </summary>
    public static byte[] PackDetectionThresholds(IReadOnlyList<DetectionThreshold> thresholds)
    {
        var packed = new byte[NbThresholds * 12];
        for (int k = 0; k < NbThresholds; k++)
        {
            if (k >= thresholds.Count)
                continue;
            DetectionThreshold t = thresholds[k];
            int scale = ThreshScale.TryGetValue(t.Measurement, out int sc) ? sc : 1;
            int lo = t.LowThresh * scale;
            int hi = t.HighThresh * scale;
            int b = k * 12;
            BinaryPrimitives.WriteInt32LittleEndian(packed.AsSpan(b), lo);
            BinaryPrimitives.WriteInt32LittleEndian(packed.AsSpan(b + 4), hi);
            packed[b + 8] = (byte)t.Measurement;
            packed[b + 9] = (byte)t.Type;
            packed[b + 10] = (byte)t.ZoneNum;
            packed[b + 11] = (byte)t.Operation;
        }
        return packed;
    }
}

/// <summary>
/// Mirror of VL53L8CX_Motion_Configuration (156 bytes, ST ULD motion plugin).
/// <see cref="Pack"/> is byte-exact with the C struct
/// <c>&lt;i 3I 12B 64b 32B 32B&gt;</c>.
/// </summary>
public sealed class MotionConfig
{
    public int RefBinOffset;
    public uint DetectionThreshold;
    public uint ExtraNoiseSigma;
    public uint NullDenClipValue;
    public byte MemUpdateMode;
    public byte MemUpdateChoice;
    public byte SumSpan;
    public byte FeatureLength;
    public byte NbOfAggregates;
    public byte NbOfTemporalAccumulations;
    public byte MinNbForGlobalDetection;
    public byte GlobalIndicatorFormat1;
    public byte GlobalIndicatorFormat2;
    public byte Spare1;
    public byte Spare2;
    public byte Spare3;
    public readonly sbyte[] MapId = new sbyte[64];
    public readonly byte[] IndicatorFormat1 = new byte[32];
    public readonly byte[] IndicatorFormat2 = new byte[32];

    /// <summary>Serialize to the 156-byte VL53L8CX_Motion_Configuration wire form.</summary>
    public byte[] Pack()
    {
        var buf = new byte[156];
        BinaryPrimitives.WriteInt32LittleEndian(buf.AsSpan(0), RefBinOffset);
        BinaryPrimitives.WriteUInt32LittleEndian(buf.AsSpan(4), DetectionThreshold);
        BinaryPrimitives.WriteUInt32LittleEndian(buf.AsSpan(8), ExtraNoiseSigma);
        BinaryPrimitives.WriteUInt32LittleEndian(buf.AsSpan(12), NullDenClipValue);
        buf[16] = MemUpdateMode;
        buf[17] = MemUpdateChoice;
        buf[18] = SumSpan;
        buf[19] = FeatureLength;
        buf[20] = NbOfAggregates;
        buf[21] = NbOfTemporalAccumulations;
        buf[22] = MinNbForGlobalDetection;
        buf[23] = GlobalIndicatorFormat1;
        buf[24] = GlobalIndicatorFormat2;
        buf[25] = Spare1;
        buf[26] = Spare2;
        buf[27] = Spare3;
        for (int k = 0; k < 64; k++)
            buf[28 + k] = (byte)MapId[k];
        for (int k = 0; k < 32; k++)
            buf[92 + k] = IndicatorFormat1[k];
        for (int k = 0; k < 32; k++)
            buf[124 + k] = IndicatorFormat2[k];
        return buf;
    }

    /// <summary>
    /// The default motion configuration produced by vl53l8cx_motion_indicator_init
    /// for the given resolution (distance-window preset + zone map).
    /// </summary>
    public static MotionConfig InitDefault(int resolution)
    {
        var cfg = new MotionConfig
        {
            RefBinOffset = 13633,
            DetectionThreshold = 2883584,
            ExtraNoiseSigma = 0,
            NullDenClipValue = 0,
            MemUpdateMode = 6,
            MemUpdateChoice = 2,
            SumSpan = 4,
            FeatureLength = 9,
            NbOfAggregates = 16,
            NbOfTemporalAccumulations = 16,
            MinNbForGlobalDetection = 1,
            GlobalIndicatorFormat1 = 8,
            GlobalIndicatorFormat2 = 0,
        };
        cfg.SetResolution(resolution);
        return cfg;
    }

    /// <summary>vl53l8cx_motion_indicator_set_resolution: fill the per-zone map id.</summary>
    public void SetResolution(int resolution)
    {
        if (resolution == Vl53l8Advanced.Resolution4x4)
        {
            for (int i = 0; i < 16; i++)
                MapId[i] = (sbyte)i;
            for (int i = 16; i < 64; i++)
                MapId[i] = -1;
        }
        else if (resolution == Vl53l8Advanced.Resolution8x8)
        {
            for (int i = 0; i < 64; i++)
                MapId[i] = (sbyte)(((i % 8) / 2) + (4 * (i / 16)));
        }
        else
        {
            throw new ArgumentException($"resolution must be 16 or 64, got {resolution}", nameof(resolution));
        }
    }
}
