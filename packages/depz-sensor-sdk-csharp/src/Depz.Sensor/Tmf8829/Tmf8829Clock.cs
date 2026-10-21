namespace Depz.Sensor.Tmf8829;

/// <summary><see cref="Tmf8829FrameTime.Flags"/> bits (contract 14 §5).</summary>
public static class Tmf8829TimeFlags
{
    /// <summary>Clock ratio not measured yet (nominal 8 µs/tick used).</summary>
    public const int Nominal = 0x01;

    /// <summary>Clock model restarted at this frame (SYSTICK jump).</summary>
    public const int ClockReset = 0x02;

    /// <summary>INT edge &gt; 2 ms later after t1 than usual in this stream.</summary>
    public const int EdgeLate = 0x04;

    /// <summary>Sub-integrations not back to back (histogram FIFO wait).</summary>
    public const int Stalled = 0x08;

    /// <summary>Histograms + 32×32/48×32: T1 may be too long, centre late.</summary>
    public const int T1Unsure = 0x10;

    private static readonly (int Bit, string Name)[] Names =
    {
        (Nominal, "nominal"), (ClockReset, "clock_reset"), (EdgeLate, "edge_late"),
        (Stalled, "stalled"), (T1Unsure, "t1_unsure"),
    };

    /// <summary>Names of the bits set in <paramref name="flags"/>.</summary>
    public static List<string> FlagNames(int flags) =>
        Names.Where(n => (flags & n.Bit) != 0).Select(n => n.Name).ToList();
}

/// <summary>
/// Sensor SYSTICK (125 kHz) → MCU µs, fed one clock pair per frame (the MCU
/// time of the header read and the pre-header SYSTICK sampled by it). The
/// ratio is taken over a sliding 1..2 s window, first after 200 ms; it is also
/// the distance correction (ams tmf8829ClockCorrection). Integer arithmetic
/// exactly as contract 14 §5 — <c>tmf8829_frame_time.json</c> pins it bit for
/// bit: u32 tick differences, signed 32-bit tick deltas, 64-bit µs, floor
/// division, arithmetic shift.
/// </summary>
public sealed class Tmf8829SensorClock
{
    /// <summary>SYSTICK 125 kHz = 8 MCU µs per tick, Q16.</summary>
    public const uint NominalQ16 = 8u << 16;

    /// <summary>First ratio estimate after this span.</summary>
    public const long WindowMinUs = 200_000;

    /// <summary>Then a 1..2 s sliding window.</summary>
    public const long WindowSlideUs = 1_000_000;

    private bool _valid;
    private (long Us, uint Tick) _last;
    private (long Us, uint Tick) _anchor;
    private (long Us, uint Tick)? _mid;

    public Tmf8829SensorClock() => Reset();

    /// <summary>MCU/sensor clock ratio, Q16 × 8 (<see cref="NominalQ16"/> until measured).</summary>
    public uint RatioQ16 { get; private set; }

    /// <summary>True while the ratio is not measured yet.</summary>
    public bool Nominal { get; private set; }

    /// <summary>MCU/sensor clock ratio (≈ 1.015): distances are scaled by it.</summary>
    public double Ratio => (double)RatioQ16 / NominalQ16;

    /// <summary>Restart the model — at every stream start.</summary>
    public void Reset()
    {
        _valid = false;
        _last = (0, 0);
        _anchor = (0, 0);
        _mid = null;
        RatioQ16 = NominalQ16;
        Nominal = true;
    }

    private static bool StepOk(long dUs, uint dTick)
    {
        long expect = (long)dTick * 8;
        long tol = Tmf8829Math.FloorDiv(dUs, 10) + 50;
        return expect + tol >= dUs && expect <= dUs + tol;
    }

    /// <summary>
    /// Add a pair → <see cref="Tmf8829TimeFlags.ClockReset"/> if it does not
    /// fit the model (which then restarts from it), else 0.
    /// </summary>
    public int Add(long us, uint tick)
    {
        int flags = 0;
        if (_valid && !StepOk(us - _last.Us, unchecked(tick - _last.Tick)))
        {
            _valid = false;
            flags = Tmf8829TimeFlags.ClockReset;
        }
        _last = (us, tick);
        if (!_valid)
        {
            _valid = true;
            _anchor = _last;
            _mid = null;
            RatioQ16 = NominalQ16;
            Nominal = true;
            return flags;
        }
        long span = us - _anchor.Us;
        if (span >= WindowMinUs)
        {
            RatioQ16 = unchecked((uint)Tmf8829Math.FloorDiv(span << 16, unchecked(tick - _anchor.Tick)));
            Nominal = false;
        }
        if (_mid is null && span >= WindowSlideUs)
            _mid = _last;
        else if (_mid is { } mid && span >= 2 * WindowSlideUs)
        {
            _anchor = mid;
            _mid = null;
        }
        return flags;
    }

    /// <summary>SYSTICK <paramref name="tick"/> → MCU µs through the pair (<paramref name="pairUs"/>, <paramref name="pairTick"/>).</summary>
    public long ToUs(long pairUs, uint pairTick, uint tick) =>
        pairUs + (((long)unchecked((int)(tick - pairTick)) * RatioQ16) >> 16);
}

/// <summary>Capture time of one results frame, MCU µs.</summary>
/// <param name="TimestampUs">Centre of the capture window.</param>
/// <param name="Flags"><see cref="Tmf8829TimeFlags"/> bits.</param>
/// <param name="T1LenUs">Sub-integration length used.</param>
/// <param name="T0Us">Start of the first sub-integration.</param>
/// <param name="T1Us">Start of the last one.</param>
/// <param name="EdgeUs">INT edge credited to the frame, 0 = none.</param>
/// <param name="PairUs">MCU time of the header read (the clock pair).</param>
/// <param name="RatioQ16">Clock ratio after this frame.</param>
public sealed record Tmf8829FrameTime(
    long TimestampUs, int Flags, uint T1LenUs, long T0Us, long T1Us, long EdgeUs, long PairUs, uint RatioQ16);

/// <summary>
/// When a results frame was captured. A results frame is N equal
/// sub-integrations back to back; the footer has the SYSTICK of the start of
/// the first (t0) and of the last (t1). The window is [t0, t1 + T1], T1 the
/// stream's shortest (t1 − t0)/(N − 1); its centre is the frame time.
/// </summary>
public sealed class Tmf8829CaptureTimer
{
    /// <summary>EDGE_LATE threshold over the stream's smallest edge lag.</summary>
    public const long EdgeLateUs = 2000;

    /// <summary>Sub-integrations per results frame by fp_mode.</summary>
    public static readonly IReadOnlyDictionary<int, int> FpSubints = new Dictionary<int, int>
    {
        [0] = 2, [1] = 2, [2] = 2, [3] = 4, [4] = 4, [5] = 6,
    };

    private long? _lagMin;
    private bool _histSeen;
    private uint _subMin;

    public Tmf8829CaptureTimer(Tmf8829SensorClock clock)
    {
        Clock = clock;
        Reset();
    }

    public Tmf8829SensorClock Clock { get; }

    /// <summary>Forget the per-stream state (lag, T1, histogram mode).</summary>
    public void Reset()
    {
        _lagMin = null;
        _histSeen = false;
        _subMin = 0;
    }

    /// <summary>A histogram frame was seen in the stream.</summary>
    public void Histograms() => _histSeen = true;

    /// <summary>
    /// Capture time of a results frame. Call <see cref="Tmf8829SensorClock.Add"/>
    /// with the same pair first and pass its flags in <paramref name="flags"/>.
    /// </summary>
    public Tmf8829FrameTime Results(
        int fpMode, uint systick, uint t0Tick, uint t1Tick, long pairUs, long edgeUs, int flags)
    {
        var c = Clock;
        if (c.Nominal)
            flags |= Tmf8829TimeFlags.Nominal;
        long t0 = c.ToUs(pairUs, systick, t0Tick);
        long t1 = c.ToUs(pairUs, systick, t1Tick);

        if (edgeUs != 0 && !_histSeen)
        {
            long lag = edgeUs - t1;
            if (lag <= 0 || (_lagMin is long lm && lag > lm + EdgeLateUs))
                flags |= Tmf8829TimeFlags.EdgeLate;
            else if (_lagMin is null || lag < _lagMin)
                _lagMin = lag;
        }

        uint n1 = (uint)((FpSubints.TryGetValue(fpMode, out int n) ? n : 2) - 1);
        uint sub = unchecked(t1Tick - t0Tick) / n1;
        if (_subMin == 0 || sub < _subMin)
            _subMin = sub;
        if (sub > (ulong)_subMin + 1)
            flags |= Tmf8829TimeFlags.Stalled;
        if (_histSeen && n1 > 1)
            flags |= Tmf8829TimeFlags.T1Unsure;

        uint t1Len = unchecked((uint)(((ulong)_subMin * c.RatioQ16) >> 16));
        return new Tmf8829FrameTime(
            Tmf8829Math.FloorDiv(t0 + t1 + t1Len, 2), flags, t1Len, t0, t1, edgeUs, pairUs, c.RatioQ16);
    }

    /// <summary><see cref="Results"/> with the times taken from a parsed frame.</summary>
    public Tmf8829FrameTime ResultsFrame(Tmf8829FrameInfo f, long pairUs, long edgeUs, int flags) =>
        Results(f.FpMode, f.Systick, f.T0Integration, f.T1Integration, pairUs, edgeUs, flags);
}

/// <summary>Integer helpers with Python semantics.</summary>
internal static class Tmf8829Math
{
    /// <summary>Floor division (C# <c>/</c> truncates toward zero). <paramref name="b"/> &gt; 0.</summary>
    public static long FloorDiv(long a, long b)
    {
        long q = a / b;
        if ((a % b != 0) && ((a < 0) != (b < 0)))
            q--;
        return q;
    }
}
