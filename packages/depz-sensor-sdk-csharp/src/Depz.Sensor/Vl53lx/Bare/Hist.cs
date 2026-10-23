namespace Depz.Sensor.Vl53lx.Bare;

/// <summary><c>VL53LX_range_data_t</c>: one target. The event sums and phases are what the algorithm produced;
/// the <c>*RangeMm</c> fields and the rates are what they mean.</summary>
public sealed class RangeData
{
    public long RangeId;
    public long StartBin;            // VL53LX_p_012, window start
    public long FirstBin;            // VL53LX_p_019, first bin over threshold
    public long PeakBin;             // VL53LX_p_023
    public long LastBin;             // VL53LX_p_024
    public long EndBin;              // VL53LX_p_013
    public long WidthBins;           // VL53LX_p_025, bins over threshold
    public long WindowBins;          // VL53LX_p_029, bins in the window
    public long VcselWidth;
    public long FastOscFrequency;
    public long ZeroDistancePhase;
    public long Spads;               // VL53LX_p_004
    public long TotalPeriodsElapsed;
    public long PeakDurationUs;
    public long WoiDurationUs;
    public long AmbientEvents;       // VL53LX_p_016
    public long TotalEvents;         // VL53LX_p_017
    public long SignalEvents;        // VL53LX_p_010
    public long PeakSignalCountRateMcps;
    public long AvgSignalCountRateMcps;
    public long AmbientCountRateMcps;
    public long TotalRatePerSpadMcps;
    public long SignalEventsPerSpadKcps;   // VL53LX_p_009
    public long Sigma;               // VL53LX_p_002, 9.7 fixed point mm
    public long PhaseStart;          // VL53LX_p_026
    public long PhaseMean;           // VL53LX_p_011
    public long PhaseEnd;            // VL53LX_p_027
    public long MinRangeMm;
    public long MedianRangeMm;
    public long MaxRangeMm;
    public long RangeStatus = Hist.DeviceErrorNoUpdate;

    public long SigmaMm => Sigma >> 7;

    /// <summary>The peak signal rate is 9.7 Mcps.</summary>
    public long SignalKcps => PeakSignalCountRateMcps * 1000 / 128;

    public long AmbientKcps => AmbientCountRateMcps * 1000 / 128;

    public override string ToString() =>
        $"<RangeData {MedianRangeMm} mm [{MinRangeMm}..{MaxRangeMm}] sigma {SigmaMm} mm signal {SignalKcps} kcps status {RangeStatus}>";
}

/// <summary><c>VL53LX_range_results_t</c>, reduced to the single-zone case.</summary>
public sealed class RangeResults
{
    public long StreamCount;
    public List<RangeData> Targets { get; } = new();   // VL53LX_p_003, active_results entries
    public long[] AmbientDmaxMm = new long[5];          // VL53LX_p_022
    public long WrapDmaxMm;
}

/// <summary><c>VL53LX_hist_pulse_data_t</c>: one run of bins above the threshold.</summary>
internal sealed class PulseData
{
    public long StartBin;            // VL53LX_p_012
    public long FirstBin;            // VL53LX_p_019
    public long PeakBin = 0xFF;      // VL53LX_p_023
    public long LastBin;             // VL53LX_p_024
    public long EndBin;              // VL53LX_p_013
    public long WidthBins;           // VL53LX_p_025
    public long FilterWoi;           // VL53LX_p_051, filter half width
    public long AmbientEvents;       // VL53LX_p_016
    public long TotalEvents;         // VL53LX_p_017
    public long SignalEvents;        // VL53LX_p_010
    public long PhaseStart;          // VL53LX_p_026
    public long PhaseMean;           // VL53LX_p_011
    public long PhaseEnd;            // VL53LX_p_027
    public long Sigma;               // VL53LX_p_002
}

/// <summary><c>VL53LX_hist_gen3_algo_private_data_t</c>: per-bin thresholds and flags, the pulse list, and the
/// working copies of the histogram.</summary>
internal sealed class Gen3Algo
{
    private const int N = BareRegs.HistogramBufferSize;

    public long FirstBin;            // VL53LX_p_019
    public long BufferSize = N;      // VL53LX_p_020
    public long BinsInData;          // VL53LX_p_021
    public long VcselPeriod;         // VL53LX_p_030, bins per period
    public long BinsOverThreshold;   // VL53LX_p_039
    public long AmbientPerBin;       // VL53LX_p_028
    public long AmbientThreshold;    // VL53LX_p_031, the last one computed
    public readonly long[] OverThreshold = new long[N];   // VL53LX_p_040
    public readonly long[] PulseMask = new long[N];       // VL53LX_p_041
    public readonly long[] PulseNo = new long[N];         // VL53LX_p_042
    public readonly long[] Threshold = new long[N];       // VL53LX_p_052
    public long FirstRisingBin;      // VL53LX_p_044
    public long MaxPulses = Hist.MaxPulses;               // VL53LX_p_045
    public long PulseCount;          // VL53LX_p_046
    public PulseData[] Pulses = Enumerable.Range(0, Hist.MaxPulses).Select(_ => new PulseData()).ToArray();
    public HistogramBinData? Bins;        // VL53LX_p_006, the working frame
    public HistogramBinData? Xtalk;       // VL53LX_p_047
    public HistogramBinData? PulseAmb;    // VL53LX_p_048, pulse padded ambient
    public HistogramBinData? PulseZero;   // VL53LX_p_049, pulse padded zero
    public HistogramBinData? PulseXtalk;  // VL53LX_p_050
}

/// <summary><c>VL53LX_hist_gen4_algo_filtered_data_t</c>: the three-tap window sums and the two edge signals
/// whose sign change locates the peak.</summary>
internal sealed class Filtered
{
    private const int N = BareRegs.HistogramBufferSize;
    public readonly long[] A = new long[N];        // VL53LX_p_007
    public readonly long[] B = new long[N];        // VL53LX_p_032
    public readonly long[] C = new long[N];        // VL53LX_p_001
    public readonly long[] Left = new long[N];     // VL53LX_p_053
    public readonly long[] Right = new long[N];    // VL53LX_p_054
    public readonly long[] IsPeak = new long[N];   // VL53LX_p_040
}

/// <summary>One target of the frame before, reduced to what the consistency checks read.</summary>
internal sealed record PrevTarget(long AmbientEvents, long TotalEvents, long PhaseMean, long RangeStatus);

/// <summary>
/// The frame before this one, reduced to what the consistency checks read (<c>VL53LX_zone_hist_info_t</c> and
/// <c>VL53LX_zone_objects_t</c> for one zone). The engine alternates two VCSEL periods, so one frame cannot say
/// whether a target is inside the unambiguous range or an echo wrapped into it; the API compares consecutive
/// frames. The BareDriver keeps no history, so the driver owns one of these and hands it every frame.
/// </summary>
public sealed class FrameHistory
{
    private BareDeviceState? _rdDeviceState;
    private List<PrevTarget> _targets = new();

    internal long TotalPeriodsElapsed { get; private set; }

    internal long Spads { get; private set; }

    /// <summary>Forget the stream so far: the next <see cref="Apply"/> only remembers.</summary>
    public void Reset()
    {
        _rdDeviceState = null;
        TotalPeriodsElapsed = 0;
        Spads = 0;
        _targets = new List<PrevTarget>();
    }

    /// <summary>Check <paramref name="results"/> against the remembered frame, then remember it — C's order:
    /// both checks run before hist_data and range_results are copied into the zone history.</summary>
    public void Apply(RangeResults results, HistogramBinData bins, BareDeviceState rdDeviceState, HistPostProcessConfig hpp)
    {
        PhaseConsistencyCheck(results, hpp);
        _rdDeviceState = rdDeviceState;
        TotalPeriodsElapsed = bins.TotalPeriodsElapsed;
        Spads = bins.ResultDssActualEffectiveSpads;
        _targets = results.Targets.Select(t => new PrevTarget(t.AmbientEvents, t.TotalEvents, t.PhaseMean, t.RangeStatus)).ToList();
    }

    // VL53LX_hist_phase_consistency_check(): every ranged target starts guilty and has to find itself in the
    // previous frame, within the phase tolerance and with consistent events.
    private void PhaseConsistencyCheck(RangeResults results, HistPostProcessConfig hpp)
    {
        long phaseTolerance = hpp.ConsistencyPhaseTolerance << 8; // a whole phase unit; phases are 8.8
        // C also admits RANGING_GATHER_DATA, the multizone path.
        if (_rdDeviceState != BareDeviceState.RangingOutputData)
            return;
        if (phaseTolerance == 0)
            return;
        foreach (var target in results.Targets)
        {
            if (target.RangeStatus != Hist.DeviceErrorRangeComplete && target.RangeStatus != Hist.DeviceErrorRangeCompleteNoWrapCheck)
                continue;
            target.RangeStatus = _targets.Count > 0 ? Hist.DeviceErrorPhaseConsistency : Hist.DeviceErrorPrevRangeNoTargets;
            foreach (var prev in _targets)
            {
                if (Math.Abs(target.PhaseMean - prev.PhaseMean) >= phaseTolerance)
                    continue;
                long status = Hist.EventsConsistencyCheck(hpp.ConsistencyEventSigma, hpp.ConsistencyEventMinSpadCount, this, prev, target);
                if (status == Hist.DeviceErrorRangeComplete)
                    status = Hist.MergedPulseCheck(hpp.ConsistencyMinMaxTolerance, target);
                target.RangeStatus = status;
            }
        }
    }
}

/// <summary>
/// Histogram post-processing: 24 bins → targets, on the host alone, as <c>VL53LX_hist_process_data()</c> does in
/// C. Fold repeated bin codes, estimate the ambient floor, drop the ambient-only bins, threshold each bin, take the
/// runs of marked bins as pulses; each pulse is isolated, filtered with a three-tap window and its zero crossing
/// interpolated to a sub-bin phase, which converts to millimetres. ST's source names its functions
/// <c>VL53LX_f_NNN</c>; the names here are ours, the C name is in the comment. Crosstalk compensation and the
/// ambient dmax are not ported (dmax needs FMT data our boards ship blank, so it would be zero anyway).
/// Arithmetic is the C driver's: truncating division, and saturation where it caps an intermediate.
/// </summary>
public static class Hist
{
    public const int MaxBinSequenceLength = 6;
    public const int MaxBinSequenceCode = 15;
    public const int MaxPulses = 8;                  // VL53LX_D_001
    public const int MaxRangeResults = 4;

    public const long MaxAllowedPhase = 0xFFFF;
    public const long SpadTotalCountMax = (1L << 29) - 1;
    public const long SpadTotalCountResThres = 1L << 24;
    public const long SpeedOfLightInAirDiv8 = 299704 >> 3;

    // vl53lx_sigma_estimate.h — the saturation ceilings of the sigma estimate.
    public const long SigmaInvalid = 0xFFFF;         // VL53LX_D_002
    private const long D003 = 0xFFFFFF;
    private const long D004 = 0xFFFFFFFFFFFFFF;
    private const long D005 = 0x7FFFFFFFFF;
    private const long D006 = 0x7FFFFFFFFFFFFFFF;
    private const long D007 = 0xFFFFFFFF;

    // VL53LX_DeviceError, the ones the histogram path sets.
    public const long DeviceErrorNoUpdate = 0;
    public const long DeviceErrorRangePhaseCheck = 5;
    public const long DeviceErrorSigmaThresholdCheck = 6;
    public const long DeviceErrorPhaseConsistency = 7;
    public const long DeviceErrorRangeComplete = 9;
    public const long DeviceErrorRangeCompleteNoWrapCheck = 19;
    public const long DeviceErrorEventConsistency = 20;
    public const long DeviceErrorRangeCompleteMergedPulse = 22;
    public const long DeviceErrorPrevRangeNoTargets = 23;

    public const long HistTargetOrderStrongestFirst = 1;
    public const long HistTargetOrderClosestFirst = 2;

    // VL53LX_RangeStatus — the user-facing status, the same numbers the rest of the family reports.
    public const int RangeStatusRangeValid = 0;
    public const int RangeStatusSigmaFail = 1;
    public const int RangeStatusOutOfBoundsFail = 4;
    public const int RangeStatusRangeValidNoWrapCheck = 6;
    public const int RangeStatusWrapTargetFail = 7;
    public const int RangeStatusRangeValidMergedPulse = 11;
    public const int RangeStatusTargetPresentLackOfSignal = 12;
    public const int RangeStatusNone = 255;

    /// <summary>UM2931 range status names.</summary>
    public static readonly IReadOnlyDictionary<int, string> StatusNames = new Dictionary<int, string>
    {
        [RangeStatusRangeValid] = "valid",
        [RangeStatusSigmaFail] = "sigma above threshold",
        [RangeStatusOutOfBoundsFail] = "phase out of valid limit",
        [RangeStatusRangeValidNoWrapCheck] = "no wrap-around check done",
        [RangeStatusWrapTargetFail] = "wrapped target",
        [RangeStatusRangeValidMergedPulse] = "valid, merged pulse",
        [RangeStatusTargetPresentLackOfSignal] = "no target in the frame before",
        [RangeStatusNone] = "no target",
    };

    // ── arithmetic ───────────────────────────────────────────────────────────

    /// <summary>The modulo of the reference (sign of the divisor).</summary>
    public static long PyMod(long a, long b)
    {
        long r = a % b;
        return r != 0 && (r < 0) != (b < 0) ? r + b : r;
    }

    // The floor division of the reference (C truncation is plain '/').
    private static long FloorDiv(long a, long b)
    {
        long q = a / b;
        return (a % b != 0) && ((a < 0) != (b < 0)) ? q - 1 : q;
    }

    /// <summary>VL53LX_isqrt(): floor of the square root of a uint32 argument.</summary>
    public static long Isqrt(long num)
    {
        ulong n = (ulong)(num & 0xFFFFFFFFL);
        ulong r = (ulong)Math.Sqrt(n);
        while (r * r > n)
            r--;
        while ((r + 1) * (r + 1) <= n)
            r++;
        return (long)r;
    }

    // a * b for non-negative operands, held at cap when the true product would pass it.
    private static long SatMul(long a, long b, long cap) => b != 0 && a > cap / b ? cap : Math.Min(a * b, cap);

    /// <summary>VL53LX_calc_pll_period_mm().</summary>
    public static long CalcPllPeriodMm(long fastOscFrequency)
    {
        long pllPeriodUs = BareMath.CalcPllPeriodUs(fastOscFrequency);
        long pllPeriodMm = SpeedOfLightInAirDiv8 * (pllPeriodUs >> 2);
        return (pllPeriodMm + (1 << 15)) >> 16;
    }

    /// <summary>VL53LX_rate_maths(): events over a duration → count rate in 9.7 Mcps.</summary>
    public static long RateMaths(long events, long timeUs)
    {
        long tmp = 0;
        if (events > SpadTotalCountMax)
            tmp = SpadTotalCountMax;
        else if (events > 0)
            tmp = events;
        int fracBits = events > SpadTotalCountResThres ? 3 : 7;
        if (timeUs > 0)
            tmp = ((tmp << fracBits) + timeUs / 2) / timeUs;
        if (events > SpadTotalCountResThres)
            tmp <<= 4;
        return Math.Min(tmp, 0xFFFF);
    }

    /// <summary>VL53LX_rate_per_spad_maths().</summary>
    public static long RatePerSpadMaths(int fracBits, long peakCountRate, long numSpads, long maxOutputValue)
    {
        long tmp;
        if (numSpads > 0)
        {
            tmp = (peakCountRate << 8) << fracBits;
            tmp = (tmp + numSpads / 2) / numSpads;
        }
        else
        {
            tmp = peakCountRate << fracBits;
        }
        return Math.Min(tmp, maxOutputValue);
    }

    /// <summary>VL53LX_events_per_spad_maths() → kcps per SPAD.</summary>
    public static long EventsPerSpadMaths(long events, long numSpads, long duration)
    {
        long total = numSpads != 0 ? FloorDiv(events * 1000 * 256, numSpads) : 0;
        long perSpad = duration > 0 ? FloorDiv((total << 11) + duration / 2, duration) : total << 11;
        return perSpad & 0xFFFFFFFFL;
    }

    /// <summary>VL53LX_range_maths(): phase → millimetres.</summary>
    public static long RangeMaths(long fastOscFrequency, long phase, long zeroDistancePhase, int fractionalBits,
        long gainFactor, long rangeOffsetMm)
    {
        long pllPeriodUs = BareMath.CalcPllPeriodUs(fastOscFrequency);
        long tmp = phase - zeroDistancePhase;
        tmp *= pllPeriodUs;
        tmp /= 1 << 9;
        tmp *= SpeedOfLightInAirDiv8;
        tmp /= 1 << 22;

        long rangeMm = tmp + rangeOffsetMm;
        rangeMm *= gainFactor;
        rangeMm += 0x0400;
        rangeMm /= 0x0800;

        if (fractionalBits == 0)
        {
            long rangeMm10 = rangeMm * 10 / (1 << 2);
            rangeMm = Math.Abs(rangeMm10 - rangeMm10 / 10 * 10) < 5 ? rangeMm10 / 10 : rangeMm10 / 10 + 1;
        }
        else if (fractionalBits == 1)
        {
            rangeMm /= 1 << 1;
        }
        return rangeMm;
    }

    // ── bin housekeeping ─────────────────────────────────────────────────────

    /// <summary>VL53LX_f_031: fold the six bin-sequence codes together. Repeats of a code land on the same four
    /// output bins and are averaged, so what comes out is one bin per distinct code.</summary>
    public static HistogramBinData AverageRepeatedBins(HistogramBinData src)
    {
        var dst = src.Clone();
        dst.BinsInData = 0;
        dst.BinSeq = Enumerable.Repeat(MaxBinSequenceCode + 1, MaxBinSequenceLength).ToArray();
        dst.BinData = new long[dst.NumberOfBins];

        var initialIndex = new int[MaxBinSequenceCode + 2];
        var repeatCount = new long[MaxBinSequenceCode + 2];
        int seqLength = 0;
        for (int lc = 0; lc < MaxBinSequenceLength; lc++)
        {
            int binCfg = src.BinSeq[lc];
            if (repeatCount[binCfg] == 0)
            {
                initialIndex[binCfg] = seqLength * 4;
                dst.BinSeq[seqLength] = binCfg;
                seqLength++;
            }
            repeatCount[binCfg]++;
            int b = initialIndex[binCfg];
            for (int i = 0; i < 4; i++)
                dst.BinData[b + i] += src.BinData[lc * 4 + i];
        }

        dst.BinRep = dst.BinSeq.Select(c => c <= MaxBinSequenceCode ? repeatCount[c] : 0).ToArray();
        dst.BinsInData = seqLength * 4;

        for (int code = 0; code <= MaxBinSequenceCode; code++)
        {
            long reps = repeatCount[code];
            if (reps <= 0)
                continue;
            int b = initialIndex[code];
            for (int i = 0; i < 4; i++)
                dst.BinData[b + i] = (dst.BinData[b + i] + reps / 2) / reps;
        }

        // Codes 7 and 15 are the ambient-only sequence entries.
        dst.NumberOfAmbientBins = repeatCount[7] != 0 || repeatCount[15] != 0 ? 4 : 0;
        return dst;
    }

    /// <summary>VL53LX_hist_calc_zero_distance_phase.</summary>
    public static void CalcZeroDistancePhase(HistogramBinData bins)
    {
        long period = 2048 * BareMath.DecodeVcselPeriod(bins.VcselPeriod);
        long phase = period + bins.PhasecalResultReferencePhase + 2048 * bins.PhasecalResultVcselStart - 2048 * bins.CalConfigVcselStart;
        bins.ZeroDistancePhase = period != 0 ? PyMod(phase, period) : 0;
    }

    /// <summary>VL53LX_hist_estimate_ambient_from_thresholded_bins(): the floor from every bin near the quietest.</summary>
    public static void EstimateAmbientFromThresholdedBins(long ambientThresholdSigma, HistogramBinData bins)
    {
        var used = bins.BinData.Take((int)bins.BinsInData).ToArray();
        bins.MinBinValue = used.Length > 0 ? used.Min() : 0;
        bins.MaxBinValue = used.Length > 0 ? used.Max() : 0;

        long threshold = Isqrt(bins.MinBinValue);
        threshold *= ambientThresholdSigma;
        threshold += 0x07;
        threshold >>= 4;
        threshold += bins.MinBinValue;

        bins.NumberOfAmbientSamples = 0;
        bins.AmbientEventsSum = 0;
        foreach (long value in used)
        {
            if (value < threshold)
            {
                bins.AmbientEventsSum += value;
                bins.NumberOfAmbientSamples++;
            }
        }
        if (bins.NumberOfAmbientSamples > 0)
            bins.AmbientPerBin = (bins.AmbientEventsSum + bins.NumberOfAmbientSamples / 2) / bins.NumberOfAmbientSamples;
    }

    /// <summary>VL53LX_hist_estimate_ambient_from_ambient_bins(): the better estimate, from the VCSEL-off bins.</summary>
    public static void EstimateAmbientFromAmbientBins(HistogramBinData bins)
    {
        if (bins.NumberOfAmbientBins <= 0)
            return;
        bins.NumberOfAmbientSamples = bins.NumberOfAmbientBins;
        bins.AmbientEventsSum = bins.BinData.Take((int)bins.NumberOfAmbientBins).Sum();
        bins.AmbientPerBin = (bins.AmbientEventsSum + bins.NumberOfAmbientBins / 2) / bins.NumberOfAmbientBins;
    }

    /// <summary>VL53LX_hist_remove_ambient_bins: drop the ambient-only bins off the front, so bin 0 is phase zero.</summary>
    public static void RemoveAmbientBins(HistogramBinData bins)
    {
        if ((bins.BinSeq[0] & 0x07) == 0x07)
        {
            var keptSeq = new List<int>();
            var keptRep = new List<long>();
            for (int lc = 0; lc < MaxBinSequenceLength; lc++)
            {
                if ((bins.BinSeq[lc] & 0x07) != 0x07)
                {
                    keptSeq.Add(bins.BinSeq[lc]);
                    keptRep.Add(bins.BinRep[lc]);
                }
            }
            int pad = MaxBinSequenceLength - keptSeq.Count;
            bins.BinSeq = keptSeq.Concat(Enumerable.Repeat(MaxBinSequenceCode + 1, pad)).ToArray();
            bins.BinRep = keptRep.Concat(Enumerable.Repeat(0L, pad)).ToArray();
        }
        int n = (int)bins.NumberOfAmbientBins;
        if (n > 0)
        {
            bins.BinData = bins.BinData.Skip(n).Concat(new long[n]).ToArray();
            bins.BinsInData -= n;
            bins.NumberOfAmbientBins = 0;
        }
    }

    // VL53LX_f_022: the three-tap window around binIndex over a half width of filterWoi bins.
    private static (long A, long B, long C) WoiSums(long binIndex, long filterWoi, HistogramBinData bins)
    {
        long a = 0, b = bins.BinData[binIndex], c = 0;
        for (long w = 0; w < (filterWoi << 1) + 1; w++)
        {
            long j = PyMod(binIndex + w + bins.BinsInData - filterWoi, bins.BinsInData);
            if (w < filterWoi)
                a += bins.BinData[j];
            else if (w > filterWoi)
                c += bins.BinData[j];
        }
        return (a, b, c);
    }

    // VL53LX_f_018: event sums → count rates, over the durations the pulse actually ran.
    private static bool CalcRates(RangeData target, long vcselWidth, long fastOscFrequency, long totalPeriodsElapsed,
        long spads, long mergeNb)
    {
        target.VcselWidth = vcselWidth;
        target.FastOscFrequency = fastOscFrequency;
        target.TotalPeriodsElapsed = totalPeriodsElapsed;
        target.Spads = spads;
        if (fastOscFrequency == 0 || totalPeriodsElapsed == 0)
            return false;

        long pllPeriodUs = BareMath.CalcPllPeriodUs(fastOscFrequency);
        long periodsElapsed = totalPeriodsElapsed + 1;
        target.PeakDurationUs = BareMath.DurationMaths(pllPeriodUs, vcselWidth, BareRegs.RangingWindowVcselPeriods, periodsElapsed);
        target.WoiDurationUs = BareMath.DurationMaths(pllPeriodUs, target.WindowBins << 4, BareRegs.RangingWindowVcselPeriods, periodsElapsed);

        target.PeakSignalCountRateMcps = RateMaths(target.SignalEvents, target.PeakDurationUs);
        target.AvgSignalCountRateMcps = RateMaths(target.SignalEvents, target.WoiDurationUs);
        target.AmbientCountRateMcps = RateMaths(target.AmbientEvents, target.WoiDurationUs);

        long countRateTotal = target.PeakSignalCountRateMcps + target.AmbientCountRateMcps;
        if (mergeNb > 1)
            countRateTotal = FloorDiv(countRateTotal, mergeNb);
        target.TotalRatePerSpadMcps = RatePerSpadMaths(0x06, countRateTotal, spads, 0xFFFF);
        target.SignalEventsPerSpadKcps = EventsPerSpadMaths(target.SignalEvents, spads, target.PeakDurationUs);
        return true;
    }

    // VL53LX_f_019
    private static void CalcRanges(long gainFactor, long rangeOffsetMm, RangeData target)
    {
        target.MinRangeMm = RangeMaths(target.FastOscFrequency, target.PhaseStart, target.ZeroDistancePhase, 0, gainFactor, rangeOffsetMm);
        target.MedianRangeMm = RangeMaths(target.FastOscFrequency, target.PhaseMean, target.ZeroDistancePhase, 0, gainFactor, rangeOffsetMm);
        target.MaxRangeMm = RangeMaths(target.FastOscFrequency, target.PhaseEnd, target.ZeroDistancePhase, 0, gainFactor, rangeOffsetMm);
    }

    // ── sigma (vl53lx_sigma_estimate.c) ──────────────────────────────────────

    // VL53LX_f_023: the width of the pulse, 9.7 fixed point mm — how far the window sums lean to one side, over
    // how much signal there is above ambient. The ceilings are ST's.
    private static long SigmaEstimate(long sigmaRefMm, long a, long b, long c, long aZp, long cZp, long bx, long axZp,
        long cxZp, long ambientPerBin, long fastOscFrequency)
    {
        if (fastOscFrequency == 0)
            return SigmaInvalid;
        long pllPeriodMm = CalcPllPeriodMm(fastOscFrequency);
        long bMinusAmb = Math.Abs(ambientPerBin - b);
        long aMinusC = Math.Abs(a - c);
        if (bMinusAmb == 0)
            return SigmaInvalid;

        long tmp0 = Math.Min(b + bx + ambientPerBin, D003);

        // (a - c)^2 << 8, capped at D004 — which it passes exactly when |a - c| >= 2^24.
        long tmp1 = aMinusC >= 1L << 24 ? D004 : Math.Min((aMinusC * aMinusC) << 8, D004);
        tmp1 /= bMinusAmb;
        tmp1 /= bMinusAmb;
        tmp1 = Math.Min(tmp1, D005);

        tmp0 = tmp1 * tmp0; // < 2^39 * 2^24: no overflow

        tmp1 = Math.Min(cZp + cxZp + aZp + axZp, D003) << 8;

        tmp0 = Math.Min(tmp1 + tmp0, D006);

        tmp0 = tmp0 > D007 ? SatMul(tmp0 / bMinusAmb, pllPeriodMm, D006) : tmp0 * pllPeriodMm / bMinusAmb;
        tmp0 = Math.Min(tmp0, D006);

        tmp0 = tmp0 > D007 ? SatMul(tmp0 / bMinusAmb / 4, pllPeriodMm, D006) : tmp0 * pllPeriodMm / bMinusAmb / 4;
        tmp0 = Math.Min(tmp0, D006);

        tmp0 = Math.Min(tmp0 >> 2, D007);

        tmp1 = sigmaRefMm << 7;
        tmp0 = Math.Min(tmp0 + tmp1 * tmp1, D007);
        return Isqrt(tmp0);
    }

    // VL53LX_f_014: the sigma of one pulse, from its two padded copies (and its crosstalk copy).
    private static long PulseSigma(PulseData pulse, long sigmaRefMm, Gen3Algo algo, bool xtalkEnable)
    {
        if (algo.VcselPeriod == 0)
            return SigmaInvalid;
        long i = pulse.PeakBin % algo.VcselPeriod;
        var (aZp, _, cZp) = WoiSums(i, pulse.FilterWoi, algo.PulseZero!);
        var (a, b, c) = WoiSums(i, pulse.FilterWoi, algo.PulseAmb!);
        var (ax, bx, cx) = xtalkEnable ? WoiSums(i, pulse.FilterWoi, algo.PulseXtalk!) : (0L, 0L, 0L);
        return SigmaEstimate(sigmaRefMm, a, b, c, aZp, cZp, bx, ax, cx, algo.PulseAmb!.AmbientPerBin, algo.PulseAmb.FastOscFrequency);
    }

    // ── pulse detection (vl53lx_hist_algos_gen3.c) ───────────────────────────

    // VL53LX_f_006: a detection threshold per bin — the ambient floor plus a sigma margin that shrinks with the
    // number of samples in the bin — and the flags saying which bins clear it.
    private static void AmbientThresholds(long scaler, long thresholdSigma, long minThresholdEvents, bool xtalkEnable,
        HistogramBinData bins, Gen3Algo algo)
    {
        algo.BufferSize = bins.NumberOfBins;
        algo.FirstBin = bins.FirstBin;
        algo.BinsInData = bins.BinsInData;
        algo.AmbientPerBin = bins.AmbientPerBin;
        algo.VcselPeriod = BareMath.DecodeVcselPeriod(bins.VcselPeriod);

        long ambEvents = (bins.AmbientPerBin * scaler + 2048) / 4096;

        for (long lb = 0; lb < bins.BinsInData; lb++)
        {
            long samples = bins.BinRep[lb >> 2];
            if (samples <= 0)
                continue;
            long value = lb < algo.Xtalk!.BinsInData && xtalkEnable
                ? samples * (ambEvents + algo.Xtalk.BinData[lb])
                : samples * ambEvents;
            value = Isqrt(value);
            value += samples / 2;
            value = FloorDiv(value, samples);
            value *= thresholdSigma;
            value += 8;
            value = FloorDiv(value, 16);
            value += ambEvents;
            value = Math.Max(value, minThresholdEvents);
            algo.Threshold[lb] = value;
            algo.AmbientThreshold = value;
        }

        algo.BinsOverThreshold = 0;
        for (long lb = bins.FirstBin; lb < bins.BinsInData; lb++)
        {
            long over = bins.BinData[lb] > algo.Threshold[lb] ? 1 : 0;
            algo.OverThreshold[lb] = over;
            algo.PulseMask[lb] = over;
            algo.BinsOverThreshold += over;
        }
    }

    // VL53LX_f_007: the first quiet-to-loud step, so a pulse wrapped across the end of the period stays whole.
    private static void FindFirstRisingEdge(Gen3Algo algo)
    {
        algo.FirstRisingBin = 0;
        bool found = false;
        for (long i = 0; i < algo.VcselPeriod; i++)
        {
            long j = (i + 1) % algo.VcselPeriod;
            if (i < algo.BinsInData && j < algo.BinsInData && algo.PulseMask[i] == 0 && algo.PulseMask[j] == 1 && !found)
            {
                algo.FirstRisingBin = i;
                found = true;
            }
        }
    }

    // VL53LX_f_008: number the runs of over-threshold bins, from the first rising edge.
    private static void AssignPulseNumbers(Gen3Algo algo)
    {
        for (long lb = algo.FirstRisingBin; lb < algo.FirstRisingBin + algo.VcselPeriod; lb++)
        {
            long i = lb % algo.VcselPeriod;
            long j = (lb + 1) % algo.VcselPeriod;
            if (!(i < algo.BinsInData && j < algo.BinsInData))
                continue;
            if (algo.PulseMask[i] == 0 && algo.PulseMask[j] == 1)
                algo.PulseCount++;
            algo.PulseCount = Math.Min(algo.PulseCount, algo.MaxPulses);
            algo.PulseNo[i] = algo.PulseMask[i] > 0 ? algo.PulseCount : 0;
        }
    }

    // VL53LX_f_009: each numbered run → its window, width and filter half width.
    private static void PulseExtents(Gen3Algo algo)
    {
        long maxFilterHalfWidth = (algo.VcselPeriod - 1) >> 1;
        for (long blb = algo.FirstRisingBin; blb < algo.FirstRisingBin + algo.VcselPeriod; blb++)
        {
            long i = blb % algo.VcselPeriod;
            long j = (blb + 1) % algo.VcselPeriod;
            if (!(i < algo.BinsInData && j < algo.BinsInData))
                continue;
            if (algo.PulseNo[i] == 0 && algo.PulseNo[j] > 0)
            {
                long no = algo.PulseNo[j] - 1;
                if (no < algo.MaxPulses)
                {
                    var pulse = algo.Pulses[no];
                    pulse.StartBin = blb;
                    pulse.FirstBin = blb + 1;
                    pulse.PeakBin = 0xFF;
                    pulse.LastBin = 0;
                    pulse.EndBin = 0;
                }
            }
            if (algo.PulseNo[i] > 0 && algo.PulseNo[j] == 0)
            {
                long no = algo.PulseNo[i] - 1;
                if (no < algo.MaxPulses)
                {
                    var pulse = algo.Pulses[no];
                    pulse.LastBin = blb;
                    pulse.EndBin = blb + 1;
                    pulse.WidthBins = pulse.LastBin + 1 - pulse.FirstBin;
                    pulse.FilterWoi = Math.Min(pulse.EndBin + 1 - pulse.StartBin, maxFilterHalfWidth);
                }
            }
        }
    }

    // VL53LX_f_010: total and ambient events inside the pulse window.
    private static void PulseEventSums(int pulseNo, HistogramBinData bins, Gen3Algo algo)
    {
        var pulse = algo.Pulses[pulseNo];
        pulse.TotalEvents = 0;
        pulse.AmbientEvents = 0;
        for (long lb = pulse.StartBin; lb <= pulse.EndBin; lb++)
        {
            long i = lb % algo.VcselPeriod;
            pulse.TotalEvents += bins.BinData[i];
            pulse.AmbientEvents += algo.AmbientPerBin;
        }
        pulse.SignalEvents = pulse.TotalEvents - pulse.AmbientEvents;
    }

    // VL53LX_f_011: a copy of the frame with everything outside the pulse replaced by padValue.
    private static HistogramBinData IsolatePulse(int pulseNo, HistogramBinData bins, Gen3Algo algo, long padValue)
    {
        var pulse = algo.Pulses[pulseNo];
        var output = bins.Clone();
        for (long lb = algo.FirstRisingBin; lb < algo.FirstRisingBin + algo.VcselPeriod; lb++)
        {
            if (lb < pulse.StartBin || lb > pulse.EndBin)
            {
                long i = lb % algo.VcselPeriod;
                if (i < output.BinsInData)
                    output.BinData[i] = padValue;
            }
        }
        return output;
    }

    // VL53LX_f_020: the centre of mass of the bins in [start, end], in 1/2048ths of a bin.
    private static long WeightedPhase(long start, long end, long vcselPeriod, bool clipEvents, HistogramBinData bins)
    {
        long eventSum = 0, weightedSum = 0;
        if (vcselPeriod == 0)
            return MaxAllowedPhase;
        for (long lb = start; lb <= end; lb++)
        {
            long i = lb < 0 ? lb + vcselPeriod : lb % vcselPeriod;
            if (i >= 0 && i < BareRegs.HistogramBufferSize)
            {
                long value = bins.BinData[i] - bins.AmbientPerBin;
                if (clipEvents && value < 0)
                    value = 0;
                eventSum += value;
                weightedSum += value * (1024 + 2048 * lb);
            }
        }
        if (eventSum <= 0)
            return MaxAllowedPhase;
        weightedSum += eventSum / 2;
        weightedSum /= eventSum;
        return Math.Max(weightedSum, 0);
    }

    // VL53LX_f_015: the near and far phase of the pulse — the centres of mass of its leading and trailing bins.
    private static void PulsePhaseLimits(int pulseNo, bool clipEvents, HistogramBinData bins, Gen3Algo algo)
    {
        var pulse = algo.Pulses[pulseNo];
        if (pulse.PeakBin == 0xFF)
            pulse.PeakBin = 1;
        long i = pulse.PeakBin % algo.VcselPeriod;
        long start = i + pulse.StartBin - pulse.PeakBin;
        long end = i + pulse.EndBin - pulse.PeakBin;
        long windowWidth = Math.Min(end - start, 3);

        pulse.PhaseStart = WeightedPhase(start, start + windowWidth, algo.VcselPeriod, clipEvents, bins);
        pulse.PhaseEnd = WeightedPhase(end - windowWidth, end, algo.VcselPeriod, clipEvents, bins);
        if (pulse.PhaseStart > pulse.PhaseEnd)
            (pulse.PhaseStart, pulse.PhaseEnd) = (pulse.PhaseEnd, pulse.PhaseStart);
        pulse.PhaseStart = Math.Min(pulse.PhaseStart, pulse.PhaseMean);
        pulse.PhaseEnd = Math.Max(pulse.PhaseEnd, pulse.PhaseMean);
    }

    // VL53LX_f_016: strongest first or closest first. Stable, as the reference's sort is.
    private static void SortPulses(long targetOrder, Gen3Algo algo)
    {
        if (algo.PulseCount <= 1)
            return;
        int n = (int)algo.PulseCount;
        var head = algo.Pulses.Take(n);
        var sorted = (targetOrder == HistTargetOrderStrongestFirst
            ? head.OrderByDescending(p => p.SignalEvents)
            : head.OrderBy(p => p.PhaseMean)).ToArray();
        sorted.CopyTo(algo.Pulses, 0);
    }

    // VL53LX_f_017: pulse → target, plus the checks that can fail it: sigma over the threshold, and a phase
    // outside the window the preset mode is valid for.
    private static void FillTarget(long rangeId, long validPhaseLow, long validPhaseHigh, long sigmaThresh,
        HistogramBinData bins, PulseData pulse, RangeData target)
    {
        target.RangeId = rangeId;
        target.StartBin = pulse.StartBin;
        target.FirstBin = pulse.FirstBin;
        target.PeakBin = pulse.PeakBin;
        target.LastBin = pulse.LastBin;
        target.EndBin = pulse.EndBin;
        target.WidthBins = pulse.WidthBins;
        target.WindowBins = pulse.EndBin + 1 - pulse.StartBin;

        target.ZeroDistancePhase = bins.ZeroDistancePhase;
        target.Sigma = pulse.Sigma;
        target.PhaseStart = pulse.PhaseStart & 0xFFFF;
        target.PhaseMean = pulse.PhaseMean & 0xFFFF;
        target.PhaseEnd = pulse.PhaseEnd & 0xFFFF;
        target.TotalEvents = pulse.TotalEvents;
        target.SignalEvents = pulse.SignalEvents;
        target.AmbientEvents = pulse.AmbientEvents;
        target.TotalPeriodsElapsed = bins.TotalPeriodsElapsed;

        target.RangeStatus = DeviceErrorRangeCompleteNoWrapCheck;
        if (sigmaThresh > 0 && pulse.Sigma > sigmaThresh << 5)
            target.RangeStatus = DeviceErrorSigmaThresholdCheck;

        long lower = (validPhaseLow << 8) & 0xFFFF;
        lower = lower < target.ZeroDistancePhase ? target.ZeroDistancePhase - lower : 0;
        long upper = ((validPhaseHigh << 8) + bins.ZeroDistancePhase) & 0xFFFF;
        if (target.PhaseMean < lower || target.PhaseMean > upper)
            target.RangeStatus = DeviceErrorRangePhaseCheck;
    }

    // ── peak location (vl53lx_hist_algos_gen4.c) ─────────────────────────────

    // VL53LX_f_026: the three-tap window over the pulse; where left and right are both positive the centre of
    // mass is inside this bin.
    private static void FilterPulse(int pulseNo, HistogramBinData pulseBins, Gen3Algo algo, Filtered filtered)
    {
        var pulse = algo.Pulses[pulseNo];
        for (long lb = pulse.StartBin; lb <= pulse.EndBin; lb++)
        {
            long i = lb % algo.VcselPeriod;
            var (a, b, c) = WoiSums(i, pulse.FilterWoi, pulseBins);
            filtered.A[i] = a;
            filtered.B[i] = b;
            filtered.C[i] = c;
            filtered.Left[i] = a + b - (c + algo.AmbientPerBin);
            filtered.Right[i] = b + c - (a + algo.AmbientPerBin);
        }
    }

    // VL53LX_f_028: the sub-bin phase where (c - a) against (b - ambient) crosses zero; null when the pulse has
    // no height (the division by zero C turns into "not a peak after all").
    private static long? InterpolatePhase(long binIndex, long a, long b, long c, long ambientPerBin, long vcselPeriod)
    {
        long numerator = 4096 * (c - a);
        long halfBMinusAmb = 4096 * (b - ambientPerBin);
        if (halfBMinusAmb == 0)
            return null;
        long meanPhase = 4096 * numerator + halfBMinusAmb;
        meanPhase /= halfBMinusAmb * 2;
        meanPhase += 2048;
        meanPhase += 4096 * binIndex;
        meanPhase = (meanPhase + 1) / 2;
        meanPhase = Math.Min(Math.Max(meanPhase, 0), MaxAllowedPhase);
        return PyMod(meanPhase, vcselPeriod * 2048);
    }

    // VL53LX_f_027: keep the last bin where the filter puts the centre of mass; that bin and its phase are the target.
    private static void FindPeakBin(int pulseNo, Filtered filtered, Gen3Algo algo)
    {
        var pulse = algo.Pulses[pulseNo];
        for (long lb = pulse.StartBin; lb < pulse.EndBin; lb++)
        {
            long i = lb % algo.VcselPeriod;
            long j = (lb + 1) % algo.VcselPeriod;
            if (!(i < algo.BinsInData && j < algo.BinsInData))
                continue;
            long isPeak;
            if (filtered.Left[i] == 0 && filtered.Right[i] == 0)
                isPeak = 0;
            else if (filtered.Left[i] >= 0 && filtered.Right[i] >= 0)
                isPeak = 1;
            else if (filtered.Left[i] < 0 && filtered.Right[i] >= 0 && filtered.Left[j] >= 0 && filtered.Right[j] < 0)
                isPeak = 1;
            else
                isPeak = 0;
            filtered.IsPeak[i] = isPeak;
            if (isPeak == 0)
                continue;
            pulse.PeakBin = lb;
            long? phase = InterpolatePhase(lb, filtered.A[i], filtered.B[i], filtered.C[i], algo.AmbientPerBin, algo.VcselPeriod);
            if (phase is long ph)
                pulse.PhaseMean = ph;
            else
                filtered.IsPeak[i] = 0;
        }
    }

    // ── entry point ──────────────────────────────────────────────────────────

    /// <summary><c>VL53LX_hist_process_data()</c> + <c>VL53LX_f_025</c>, crosstalk off.</summary>
    public static RangeResults ProcessData(HistogramBinData binsInput, HistPostProcessConfig hpp, long mergeNb = 1)
    {
        if (hpp.CrosstalkCompensationEnable != 0)
            throw new NotSupportedException("crosstalk compensation is not ported");

        var algo = new Gen3Algo();
        var filtered = new Filtered();
        var results = new RangeResults();

        var bins = AverageRepeatedBins(binsInput);
        algo.Bins = bins;
        // The crosstalk histogram: zero everywhere while compensation is off, full width so window sums stay defined.
        algo.Xtalk = bins.Clone();
        algo.Xtalk.BinData = new long[bins.NumberOfBins];

        results.StreamCount = binsInput.ResultStreamCount;

        CalcZeroDistancePhase(bins);
        EstimateAmbientFromThresholdedBins(hpp.AmbientThreshSigma0, bins);
        EstimateAmbientFromAmbientBins(bins);
        RemoveAmbientBins(bins);

        results.AmbientDmaxMm = new long[5]; // VL53LX_f_001: zero without the FMT dmax calibration

        AmbientThresholds(hpp.AmbientThreshEventsScaler, hpp.AmbientThreshSigma1, hpp.MinAmbientThreshEvents, false, bins, algo);
        FindFirstRisingEdge(algo);
        AssignPulseNumbers(algo);
        PulseExtents(algo);

        for (int p = 0; p < algo.PulseCount; p++)
        {
            var pulse = algo.Pulses[p];
            PulseEventSums(p, bins, algo);
            algo.PulseAmb = IsolatePulse(p, bins, algo, bins.AmbientPerBin);
            algo.PulseZero = IsolatePulse(p, bins, algo, 0);
            algo.PulseXtalk = IsolatePulse(p, algo.Xtalk, algo, 0);

            FilterPulse(p, algo.PulseAmb, algo, filtered);
            FindPeakBin(p, filtered, algo);
            pulse.Sigma = PulseSigma(pulse, hpp.SigmaEstimatorSigmaRefMm, algo, xtalkEnable: false);
            PulsePhaseLimits(p, true, bins, algo);
        }

        SortPulses(hpp.HistTargetOrder, algo);

        for (int p = 0; p < algo.PulseCount; p++)
        {
            if (results.Targets.Count >= MaxRangeResults)
                break;
            var pulse = algo.Pulses[p];
            if (!(pulse.SignalEvents > hpp.SignalTotalEventsLimit && pulse.PeakBin < 0xFF))
                continue;
            var target = new RangeData();
            FillTarget(results.Targets.Count, hpp.ValidPhaseLow, hpp.ValidPhaseHigh, hpp.SigmaThresh, bins, pulse, target);
            CalcRates(target, bins.VcselWidth, bins.FastOscFrequency, bins.TotalPeriodsElapsed, bins.ResultDssActualEffectiveSpads, mergeNb);
            CalcRanges(hpp.GainFactor, hpp.RangeOffsetMm, target);
            results.Targets.Add(target);
        }
        return results;
    }

    // ── frame-to-frame consistency ───────────────────────────────────────────

    // VL53LX_hist_events_consistency_check(): scale the previous frame's events onto this exposure and ask
    // whether the difference fits inside the shot noise of the pair, eventSigma sixty-fourths wide.
    internal static long EventsConsistencyCheck(long eventSigma, long minEffectiveSpadCount, FrameHistory prevFrame,
        PrevTarget prev, RangeData target)
    {
        if (eventSigma == 0)
            return DeviceErrorRangeComplete;

        long tmpp = (1 + prevFrame.TotalPeriodsElapsed) * prevFrame.Spads;
        long tmpc = (1 + target.TotalPeriodsElapsed) * target.Spads;

        long eventsScaler = tmpp * 4096;
        if (tmpc != 0)
            eventsScaler = (eventsScaler + FloorDiv(tmpc, 2)) / tmpc;
        long eventsScalerSq = (eventsScaler * eventsScaler + 2048) / 4096;

        long cSignalEvents = ((target.TotalEvents - target.AmbientEvents) * eventsScaler + 2048) / 4096;
        long cSigNoiseSq = (eventsScalerSq * target.TotalEvents + 2048) / 4096;
        long cAmbNoiseSq = (eventsScalerSq * target.AmbientEvents + 2048) / 4096;
        // Ambient is subtracted from both frames, so only its noise survives, at a quarter weight.
        cAmbNoiseSq = (cAmbNoiseSq + 2) / 4;
        long pAmbNoiseSq = (prev.AmbientEvents + 2) / 4;

        long noiseSqSum = (prev.TotalEvents + cSigNoiseSq + pAmbNoiseSq + cAmbNoiseSq) & 0xFFFFFFFFL;
        long tolerance = Isqrt(noiseSqSum * 16);
        tolerance = (tolerance * eventSigma + 32) / 64;

        long pSignalEvents = prev.TotalEvents - prev.AmbientEvents;
        long delta = Math.Abs(cSignalEvents - pSignalEvents);
        return delta > tolerance && target.Spads > minEffectiveSpadCount ? DeviceErrorEventConsistency : DeviceErrorRangeComplete;
    }

    // VL53LX_hist_merged_pulse_check(): a pulse far wider than one target should be is two targets in one.
    internal static long MergedPulseCheck(long minMaxToleranceMm, RangeData target)
    {
        long deltaMm = Math.Abs(target.MaxRangeMm - target.MinRangeMm);
        return minMaxToleranceMm > 0 && deltaMm > minMaxToleranceMm ? DeviceErrorRangeCompleteMergedPulse : DeviceErrorRangeComplete;
    }

    // ── the family-wide result shape ─────────────────────────────────────────

    /// <summary>ConvertStatusHisto() (vl53lx_api.c), the cases the histogram path produces here.</summary>
    public static int ConvertStatus(long deviceError) => deviceError switch
    {
        DeviceErrorRangePhaseCheck => RangeStatusOutOfBoundsFail,
        DeviceErrorSigmaThresholdCheck => RangeStatusSigmaFail,
        DeviceErrorRangeCompleteNoWrapCheck => RangeStatusRangeValidNoWrapCheck,
        DeviceErrorPhaseConsistency => RangeStatusWrapTargetFail,
        DeviceErrorEventConsistency => RangeStatusWrapTargetFail,
        DeviceErrorPrevRangeNoTargets => RangeStatusTargetPresentLackOfSignal,
        DeviceErrorRangeCompleteMergedPulse => RangeStatusRangeValidMergedPulse,
        DeviceErrorRangeComplete => RangeStatusRangeValid,
        _ => RangeStatusNone,
    };

    private static string StatusText(int status) => StatusNames.TryGetValue(status, out var s) ? s : $"unknown ({status})";

    private static Vl53lxTarget ToTarget(RangeData d)
    {
        int status = ConvertStatus(d.RangeStatus);
        return new Vl53lxTarget((int)d.MedianRangeMm, status, StatusText(status), d.SignalKcps, d.AmbientKcps, d.SigmaMm,
            (int)d.MinRangeMm, (int)d.MaxRangeMm);
    }

    /// <summary>The frame's results as the family-wide range: every return in <c>Targets</c>, the first one also
    /// the range itself. Effective SPADs come in 1/256 and are reported whole, as the rest of the family does.</summary>
    public static Vl53lxRange ToRange(RangeResults results, HistogramBinData bins)
    {
        if (results.Targets.Count == 0)
        {
            return new Vl53lxRange(8191, RangeStatusNone, StatusNames[RangeStatusNone], 0, 0, 0,
                bins.ResultDssActualEffectiveSpads >> 8, Array.Empty<Vl53lxTarget>(),
                new Dictionary<string, object?> { ["stream_count"] = results.StreamCount, ["ambient_per_bin"] = bins.AmbientPerBin });
        }
        var first = results.Targets[0];
        var t0 = ToTarget(first);
        return new Vl53lxRange(t0.DistanceMm, t0.Status, t0.StatusText, t0.SignalKcps, t0.AmbientKcps, t0.SigmaMm,
            first.Spads >> 8, results.Targets.Select(ToTarget).ToArray(),
            new Dictionary<string, object?>
            {
                ["stream_count"] = results.StreamCount,
                ["min_range_mm"] = first.MinRangeMm,
                ["max_range_mm"] = first.MaxRangeMm,
                ["peak_bin"] = first.PeakBin,
                ["ambient_per_bin"] = bins.AmbientPerBin,
            });
    }
}
