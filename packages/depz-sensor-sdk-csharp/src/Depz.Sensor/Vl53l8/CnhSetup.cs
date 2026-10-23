using System.Buffers.Binary;

namespace Depz.Sensor.Vl53l8;

/// <summary>
/// The CNH (compact network histogram) configuration of a VL53L8CH / VL53L7CH:
/// a mirror of VL53LMZ_Motion_Configuration with the plugin helpers that fill it
/// (vl53lmz_plugin_cnh.c, ULD 2.0.16), ported from the reference Python
/// <c>CnhConfig</c>. Build it with <see cref="InitConfig"/> and
/// <see cref="CreateAggMap"/>, check <see cref="RequiredMemory"/>, then arm it
/// with the device's <c>ConfigureCnh</c>. The flags are fixed to ping-pong and
/// variance off (Example_12_cnh_data.c), which <see cref="Vl53l8Cnh.DecodeHistogram"/> assumes.
/// Arguments outside their fields throw <see cref="ArgumentException"/>, never wrap.
/// </summary>
public sealed class Vl53l8CnhSetup
{
    public const int MiCfgDevIdx = 0xBFAC;
    public const int CnhMaxDataBytes = 1540 * 4;
    public const int PulseWidthBin = 10;
    public const double BinWidthMm = 37.5348;

    private const int DisablePingPong = 0x01;
    private const int DisableVariance = 0x02;
    private const int EnableAmbientLevel = 0x04;
    private const int EnableXtalkRemoval = 0x08;
    private const int ZeroNonValidBins = 0x10;
    private const int StoreRefResidual = 0x20;
    private const int PerHeaderWords = 5;
    private const int PerBufferHeaderWords = 2;

    public int RefBinOffset { get; private set; }
    public int SumSpan { get; private set; }
    public int FeatureLength { get; private set; }
    public int NbOfAggregates { get; private set; }
    public int CnhCfg { get; private set; }
    public int CnhFlexShift { get; private set; }

    /// <summary>Device zone → CNH aggregate, −1 for a zone in none (64 entries).</summary>
    public sbyte[] MapId { get; } = Enumerable.Repeat((sbyte)-1, 64).ToArray();

    /// <summary>vl53lmz_cnh_init_config. <paramref name="startBin"/>: first device histogram bin;
    /// <paramref name="numBins"/>: CNH bins; <paramref name="subSample"/>: device bins summed per CNH bin.</summary>
    public void InitConfig(int startBin, int numBins, int subSample)
    {
        // ref_bin_offset = start_bin * 2048 is an i32 field.
        if (startBin < -1048576 || startBin > 1048575)
            throw new ArgumentOutOfRangeException(nameof(startBin), startBin, "start_bin out of range -1048576..1048575");
        if (numBins < 0 || numBins > 0xFF)
            throw new ArgumentOutOfRangeException(nameof(numBins), numBins, "num_bins out of range 0..255");
        if (subSample < 0 || subSample > 0xFF)
            throw new ArgumentOutOfRangeException(nameof(subSample), subSample, "sub_sample out of range 0..255");
        RefBinOffset = startBin * 2048;
        FeatureLength = numBins;
        SumSpan = subSample;
        CnhCfg = DisablePingPong | DisableVariance | EnableAmbientLevel | EnableXtalkRemoval
                 | ZeroNonValidBins | StoreRefResidual;
        CnhFlexShift = 1;
    }

    /// <summary>vl53lmz_cnh_create_agg_map: aggregates of <paramref name="mergeX"/>×<paramref name="mergeY"/>
    /// zones, <paramref name="cols"/>×<paramref name="rows"/> of them from (<paramref name="startX"/>,
    /// <paramref name="startY"/>). <paramref name="resolution"/> (16 or 64) must be the sensor's.</summary>
    public void CreateAggMap(int resolution, int startX, int startY, int mergeX, int mergeY, int cols, int rows)
    {
        if (resolution != 16 && resolution != 64)
            throw new ArgumentException($"resolution {resolution}: 16 or 64", nameof(resolution));
        foreach (var (name, v, lo) in new[]
                 {
                     ("start_x", startX, 0), ("start_y", startY, 0), ("merge_x", mergeX, 1),
                     ("merge_y", mergeY, 1), ("cols", cols, 1), ("rows", rows, 1),
                 })
        {
            if (v < lo)
                throw new ArgumentException($"agg map {name} {v} must be an integer >= {lo}");
        }
        int zoneRes = resolution == 16 ? 4 : 8;
        // In 64 bits: int.MaxValue arguments must not wrap past the check.
        if (startX + (long)cols * mergeX > zoneRes || startY + (long)rows * mergeY > zoneRes)
            throw new ArgumentException("agg map exceeds zone grid");
        for (int i = 0; i < 64; i++)
            MapId[i] = -1;
        NbOfAggregates = cols * rows;
        for (int row = startY; row < startY + rows * mergeY; row++)
        {
            for (int col = startX; col < startX + cols * mergeX; col++)
            {
                int aggId = (row - startY) / mergeY * cols + (col - startX) / mergeX;
                if (aggId < 0 || aggId >= 64)
                    throw new ArgumentException("agg id out of range");
                MapId[row * zoneRes + col] = (sbyte)aggId;
            }
        }
    }

    /// <summary>vl53lmz_cnh_calc_required_memory: the on-device CNH buffer, bytes.</summary>
    /// <exception cref="ArgumentException">No aggregate map yet, or more than <see cref="CnhMaxDataBytes"/>.</exception>
    public int RequiredMemory()
    {
        if (NbOfAggregates == 0)
            throw new ArgumentException("agg map not created");
        int size = CalcRequiredMemory(CnhCfg, NbOfAggregates, FeatureLength);
        if (size > CnhMaxDataBytes)
            throw new ArgumentException($"CNH needs {size} B > max {CnhMaxDataBytes} B — reduce aggregates or bins");
        return size;
    }

    /// <summary>(min, max) target distance, mm, the histogram captures whole.</summary>
    public (int MinMm, int MaxMm) MinMaxDistanceMm()
    {
        double pulse = PulseWidthBin / 2.0 * BinWidthMm;
        double start = RefBinOffset / 2048.0;
        double first = (start + SumSpan / 2.0) * BinWidthMm;
        double last = (start + (FeatureLength - 1) * SumSpan + SumSpan / 2.0) * BinWidthMm;
        return ((int)(first + pulse), (int)(last - pulse));
    }

    /// <summary>Distance, mm, at the centre of CNH bin <paramref name="binIdx"/>.</summary>
    public double BinCenterMm(int binIdx) =>
        (RefBinOffset / 2048.0 + binIdx * SumSpan + SumSpan / 2.0) * BinWidthMm;

    /// <summary>What <see cref="Vl53l8Cnh.DecodeHistogram"/> needs to decode this setup's blocks.</summary>
    public Vl53l8CnhConfig ToDecodeConfig() => new(NbOfAggregates, FeatureLength);

    /// <summary>The 156-byte VL53LMZ_Motion_Configuration struct for the device (DCI 0xBFAC).</summary>
    public byte[] Pack()
    {
        var buf = new byte[156];
        BinaryPrimitives.WriteInt32LittleEndian(buf, RefBinOffset);
        // detection threshold, extra noise sigma, null den clip, mem update mode/choice: 0
        buf[18] = (byte)SumSpan;
        buf[19] = (byte)FeatureLength;
        buf[20] = (byte)NbOfAggregates;
        buf[21] = 1; // nb_of_temporal_accumulations
        buf[25] = (byte)CnhCfg;
        buf[26] = (byte)CnhFlexShift;
        for (int k = 0; k < 64; k++)
            buf[28 + k] = (byte)MapId[k];
        return buf;
    }

    private static int CalcRequiredMemory(int flags, int nbAgg, int featLength)
    {
        int aggXFeat = nbAgg * featLength;
        int size = PerBufferHeaderWords * 4;
        size += aggXFeat * 4;                 // FEAT_INT
        size += (3 + aggXFeat) / 4 * 4;       // FEAT_FRAC (8 bits each, padded)
        size += nbAgg * 4;                    // AMBIENT_INT
        size += (3 + nbAgg) / 4 * 4;          // AMBIENT_FRAC
        if ((flags & DisableVariance) == 0)
        {
            size += aggXFeat * 4;             // VARIANCE_INT
            size += (3 + aggXFeat) / 4 * 4;   // VARIANCE_FRAC
        }
        size = size / 4 * 4;
        if ((flags & DisablePingPong) == 0)
            size *= 2;
        return size + PerHeaderWords * 4;
    }
}
