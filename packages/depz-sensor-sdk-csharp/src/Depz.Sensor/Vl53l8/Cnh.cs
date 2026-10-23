using System.Buffers.Binary;

namespace Depz.Sensor.Vl53l8;

/// <summary>
/// The <see cref="Vl53l8Cnh"/> input: the two aggregate/histogram dimensions of
/// the on-device CNH buffer (<c>VL53LMZ_Motion_Configuration</c>). These fix the
/// block's internal offsets, so the decode needs them alongside the raw bytes.
/// </summary>
/// <param name="NbOfAggregates">Number of CNH aggregates (device zones merged into
/// one histogram); 1..64.</param>
/// <param name="FeatureLength">CNH bins per aggregate (histogram length).</param>
public sealed record Vl53l8CnhConfig(int NbOfAggregates, int FeatureLength);

/// <summary>
/// One decoded CNH aggregate histogram. The float value of bin <c>i</c> is
/// <c>HistRaw[i] / 2^HistScaler[i]</c> (a per-bin block-floating-point mantissa +
/// shift); both arrays are <see cref="Vl53l8CnhConfig.FeatureLength"/> long. The
/// ambient level of the aggregate is <c>AmbientRaw / 2^AmbientScaler</c> (the
/// value ST's Example_12 prints).
/// </summary>
/// <param name="HistRaw">Per-bin integer mantissa (FEAT_INT, signed int32).</param>
/// <param name="HistScaler">Per-bin base-2 scaler (FEAT_FRAC, signed int8).</param>
/// <param name="AmbientRaw">Ambient mantissa (AMBIENT_INT, signed int32).</param>
/// <param name="AmbientScaler">Ambient base-2 scaler (AMBIENT_FRAC, signed int8).</param>
public sealed record Vl53l8CnhAggregate(int[] HistRaw, sbyte[] HistScaler, int AmbientRaw, sbyte AmbientScaler)
{
    /// <summary>The histogram as floating-point values.</summary>
    public double[] Hist => HistRaw.Select((v, i) => v / Math.Pow(2.0, HistScaler[i])).ToArray();

    /// <summary>The ambient level as a floating-point value.</summary>
    public double Ambient => AmbientRaw / Math.Pow(2.0, AmbientScaler);
}

/// <summary>
/// A decoded CNH data block: the reference-residual word plus one histogram per
/// aggregate (in aggregate-id order).
/// </summary>
/// <param name="RefResidualWord">Raw ref-residual word (the on-device value is
/// <c>RefResidualWord / 2048.0</c>); present because the config enables
/// MI_SFE_STORE_REF_RESIDUAL.</param>
/// <param name="Aggregates">One entry per aggregate, length
/// <see cref="Vl53l8CnhConfig.NbOfAggregates"/>.</param>
public sealed record Vl53l8CnhResult(uint RefResidualWord, IReadOnlyList<Vl53l8CnhAggregate> Aggregates)
{
    /// <summary>The ref residual (11 fractional bits).</summary>
    public double RefResidual => RefResidualWord / 2048.0;
}

/// <summary>
/// VL53L8<b>CH</b>-specific CNH (Compact Network Histogram) decode.
///
/// CNH is what the VL53L8CH firmware adds on top of the CX base: a per-aggregate
/// distance histogram carried alongside the normal ranging frame.
/// The shared results-frame decode (<see cref="Vl53l8FrameDecoder"/>) and the
/// advanced DCI codecs (<see cref="Vl53l8Advanced"/> / <see cref="MotionConfig"/>)
/// already serve both CX and CH; this is the CH-only histogram parse.
///
/// A 1:1 port of the decode path of <c>vl53lmz_plugin_cnh.c</c>
/// (<c>vl53lmz_cnh_get_block_addresses</c> / <c>_cnh_get_mem_block_addresses</c>,
/// VL53LMZ ULD 2.0.16) for the fixed <c>cnh_cfg</c> used by the DEPZ firmware
/// (DISABLE_PING_PONG | DISABLE_VARIANCE | AMBIENT | XTALK | ZERO_INVALID |
/// STORE_REF_RESIDUAL). Verified byte-exact against the live-hardware golden
/// vector <c>contracts/vectors/vl53l8_cnh.json</c>. The offset arithmetic mirrors
/// the C plugin verbatim; do not "simplify" it.
/// </summary>
public static class Vl53l8Cnh
{
    /// <summary>The variant this histogram block belongs to (always CH).</summary>
    public const Vl53l8Variant Variant = Vl53l8Variant.Ch;

    // ---- persistent-data header layout (plugin_cnh.c) ----
    private const int CnhPerHeaderWords = 5;         // CNH_PER_HEADER_BYTES / 4
    private const int CnhPerBufferHeaderWords = 2;   // CNH_PER_BUFFER_HEADER_BYTES / 4
    private const int CnhPerHeaderBufferInfoIdx = 1;
    private const int CnhPerHeaderFlagsIdx = 3;
    private const uint BufferInfoWordsMask = 0xFFFF;
    private const int MiStatePing = 0;

    /// <summary>
    /// Decode a captured CNH data block (<paramref name="block"/>, byte-swapped
    /// exactly like the standard ranging blocks) into per-aggregate histograms.
    ///
    /// Layout (little-endian words) — a faithful port of
    /// <c>_cnh_get_mem_block_addresses</c> for the fixed cnh_cfg (ping-pong +
    /// variance disabled):
    /// <list type="number">
    ///   <item><c>ref_residual_word = word[2]</c> (byte offset 8).</item>
    ///   <item>Per-buffer base = <c>CNH_PER_HEADER_WORDS (5)</c> + the ping-pong
    ///   buffer size (from the header buffer-info word); with ping-pong disabled
    ///   the device reports one buffer and this resolves to the single buffer.</item>
    ///   <item>Data starts after the 2-word buffer header, then:
    ///   FEAT_INT <c>int32[nb_agg*feat]</c>, FEAT_FRAC <c>sbyte[nb_agg*feat]</c>
    ///   (4-byte padded), AMBIENT_INT <c>int32[nb_agg]</c>, AMBIENT_FRAC
    ///   <c>sbyte[nb_agg]</c>. Aggregate <c>a</c>'s slice starts at
    ///   <c>a*feat</c>.</item>
    /// </list>
    /// </summary>
    /// <param name="config">Aggregate/bin dimensions of the CNH buffer.</param>
    /// <param name="block">Raw CNH data block bytes.</param>
    /// <returns>The ref-residual word and per-aggregate histograms.</returns>
    /// <exception cref="ArgumentOutOfRangeException">A negative dimension.</exception>
    /// <exception cref="ArgumentException">If <paramref name="block"/> is too short for the
    /// described layout (every read of the reference decoder must fit, AMBIENT_FRAC last).</exception>
    public static Vl53l8CnhResult DecodeHistogram(Vl53l8CnhConfig config, ReadOnlySpan<byte> block)
    {
        if (config is null) throw new ArgumentNullException(nameof(config));
        int nbAgg = config.NbOfAggregates;
        int feat = config.FeatureLength;
        if (nbAgg < 0)
            throw new ArgumentOutOfRangeException(nameof(config), nbAgg, "nb_of_aggregates must not be negative");
        if (feat < 0)
            throw new ArgumentOutOfRangeException(nameof(config), feat, "feature_length must not be negative");

        // vl53lmz_cnh_get_ref_residual: raw word[2] (device value is this / 2048).
        if (block.Length < 3 * 4)
            throw new ArgumentException($"CNH block too short for the ref residual: {block.Length} B < 12 B", nameof(block));
        uint refResidualWord = (uint)Word(block, 2);
        if (nbAgg == 0)
            return new Vl53l8CnhResult(refResidualWord, Array.Empty<Vl53l8CnhAggregate>());

        // The buffer selection reads header words 0, 1 and 3.
        if (block.Length < (CnhPerHeaderFlagsIdx + 1) * 4)
            throw new ArgumentException(
                $"CNH block too short for its header: {block.Length} B < {(CnhPerHeaderFlagsIdx + 1) * 4} B", nameof(block));
        int state = Word(block, 0);
        uint info = (uint)Word(block, CnhPerHeaderBufferInfoIdx);
        uint headerFlags = (uint)Word(block, CnhPerHeaderFlagsIdx);
        uint ppSize = info & BufferInfoWordsMask;

        // Select ping or pong buffer exactly as the C code does. With ping-pong
        // disabled the device reports a single buffer and this resolves to ping.
        int localPp = 1;
        if ((headerFlags & 0x10) == 0x10)
            localPp = 1;
        if (state == MiStatePing)
            localPp = 1 - localPp;

        long baseWords = CnhPerHeaderWords;
        if (localPp == 1)
            baseWords += ppSize;
        // Buffer header is 2 words (state, nb_accumulated); data starts after it.
        long dataByte = (baseWords + CnhPerBufferHeaderWords) * 4;

        long aggXFeat = (long)nbAgg * feat;
        long featIntBase = dataByte;                              // FEAT_INT: int32 per (agg, feat)
        long featFracBase = featIntBase + aggXFeat * 4;           // FEAT_FRAC: sbyte per (agg, feat), padded
        long ambIntBase = featFracBase + (3 + aggXFeat) / 4 * 4;  // AMBIENT_INT: int32 per agg
        long ambFracBase = ambIntBase + (long)nbAgg * 4;          // AMBIENT_FRAC: sbyte per agg

        // The deepest read is AMBIENT_FRAC of the last aggregate.
        long need = ambFracBase + nbAgg;
        if (block.Length < need)
            throw new ArgumentException(
                $"CNH block too short for {nbAgg} aggregates x {feat} bins: {block.Length} B < {need} B",
                nameof(block));

        var aggregates = new Vl53l8CnhAggregate[nbAgg];
        for (int aggId = 0; aggId < nbAgg; aggId++)
        {
            long aggOff = (long)aggId * feat;
            var histRaw = new int[feat];
            var histScaler = new sbyte[feat];
            int intAt = checked((int)(featIntBase + aggOff * 4));
            int fracAt = checked((int)(featFracBase + aggOff));
            for (int i = 0; i < feat; i++)
            {
                histRaw[i] = BinaryPrimitives.ReadInt32LittleEndian(block.Slice(intAt + i * 4, 4));
                histScaler[i] = (sbyte)block[fracAt + i];
            }
            int ambient = BinaryPrimitives.ReadInt32LittleEndian(block.Slice(checked((int)(ambIntBase + aggId * 4L)), 4));
            var ambientScaler = (sbyte)block[checked((int)(ambFracBase + aggId))];
            aggregates[aggId] = new Vl53l8CnhAggregate(histRaw, histScaler, ambient, ambientScaler);
        }

        return new Vl53l8CnhResult(refResidualWord, aggregates);
    }

    /// <summary>Read the little-endian signed int32 word at <paramref name="wordIndex"/>.</summary>
    private static int Word(ReadOnlySpan<byte> block, int wordIndex) =>
        BinaryPrimitives.ReadInt32LittleEndian(block.Slice(wordIndex * 4, 4));
}
