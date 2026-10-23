namespace Depz.Sensor.Tmf8829;

/// <summary>
/// One measurement (contract 14 §6): its results frame (8×8, 16×16) or both
/// half-frames (32×32, 48×32) merged, with the histogram / ref-SPAD frames
/// before it. <see cref="DistanceMm"/> is flat <c>[peaks][rows][cols]</c>,
/// clock-corrected, in mm (null when the result format has no peaks);
/// <see cref="Results"/> keeps the raw sensor integers. <see cref="Problems"/>
/// lists what went wrong since the previous measurement.
/// </summary>
/// <param name="TimestampUs">Capture time, MCU µs (floor mean of the half-frame captures).</param>
/// <param name="Flags"><see cref="Tmf8829TimeFlags"/> of all its results frames, OR-ed.</param>
/// <param name="FpMode">Focal-plane mode (id &amp; 0x0F).</param>
/// <param name="FNumber">Header f_number of the (last) results frame.</param>
/// <param name="Results">Raw per-zone integers, half-frames merged.</param>
/// <param name="DistanceMm">Clock-corrected distances in mm, flat [peaks][rows][cols]; null without peaks.</param>
/// <param name="Frames">Its results frame(s), first half first.</param>
/// <param name="Times">Capture time of each results frame.</param>
/// <param name="Histograms">Histogram frames that preceded it.</param>
/// <param name="RefFrames">65 B each, zero pre-header.</param>
/// <param name="Problems">What went wrong since the previous measurement.</param>
public sealed record Tmf8829Measurement(
    long TimestampUs,
    int Flags,
    int FpMode,
    uint FNumber,
    Tmf8829PixelResults Results,
    double[]? DistanceMm,
    IReadOnlyList<Tmf8829FrameInfo> Frames,
    IReadOnlyList<Tmf8829FrameTime> Times,
    IReadOnlyList<Tmf8829HistogramFrame> Histograms,
    IReadOnlyList<byte[]> RefFrames,
    IReadOnlyList<string> Problems)
{
    /// <summary>(rows, cols) of the full focal plane.</summary>
    public (int Rows, int Cols) Shape => Tmf8829Frames.FpGrid[FpMode];

    public int Peaks => Results.Peaks;

    /// <summary>Sensor die temperature (header byte 0), °C.</summary>
    public int TemperatureC => Frames[^1].Temperature[0];

    /// <summary>Raw distances → mm, clock-corrected (× ratio; ÷ 4 for 0.25 mm units).</summary>
    public static double[]? ToMm(Tmf8829PixelResults results, double ratio, bool inMm)
    {
        if (results.Distance is null)
            return null;
        var d = new double[results.Distance.Length];
        for (int i = 0; i < d.Length; i++)
        {
            double v = results.Distance[i] * ratio;
            d[i] = inMm ? v : v / 4.0;
        }
        return d;
    }
}

/// <summary>
/// Frames in stream order → measurements (contract 14 §6). Histogram frames
/// between two results frames belong to the second; a ref-SPAD frame comes
/// right before its results frame (each half-frame has its own). 32×32/48×32:
/// the first half (layout 0x40 clear) holds the even rows; a second half
/// without a first is dropped, two first halves in a row keep the newer one
/// (both logged as problems). Call <see cref="Reset"/> at the start of every
/// stream: the clock model and T1 are per stream. Mirrors
/// <c>depz_sensor_sdk/tmf8829/builder.py</c>.
/// </summary>
public sealed class Tmf8829MeasurementBuilder
{
    private readonly List<(Tmf8829FrameInfo Info, byte[] Raw)> _pending = new();
    private List<Tmf8829HistogramFrame> _hist = new();
    private List<byte[]> _refs = new();
    private (Tmf8829FrameInfo Info, Tmf8829PixelResults Res, Tmf8829FrameTime Time)? _half;
    private List<string> _problems = new();

    public Tmf8829MeasurementBuilder()
    {
        Clock = new Tmf8829SensorClock();
        Timer = new Tmf8829CaptureTimer(Clock);
        Reset();
    }

    public Tmf8829SensorClock Clock { get; }
    public Tmf8829CaptureTimer Timer { get; }

    /// <summary>Frames that could not be parsed (also listed as problems).</summary>
    public int ParseErrors { get; private set; }

    public void Reset()
    {
        Clock.Reset();
        Timer.Reset();
        _pending.Clear();
        _hist = new();
        _refs = new();
        _half = null;
        _problems = new();
        ParseErrors = 0;
    }

    /// <summary>
    /// One frame with the MCU time of its header read and its INT edge
    /// (0 = none) → the measurement it completes, or null.
    /// </summary>
    /// <exception cref="ArgumentOutOfRangeException">a negative µs time (a u64, contract 07 §3),
    /// checked before the frame is parsed.</exception>
    public Tmf8829Measurement? Add(byte[] raw, long timestampUs, long edgeUs = 0)
    {
        Protocol.Wire.CheckUint("timestampUs", timestampUs, 63);
        Protocol.Wire.CheckUint("edgeUs", edgeUs, 63);
        Tmf8829FrameInfo f;
        try
        {
            f = Tmf8829Frames.ParseFrame(raw);
        }
        catch (Tmf8829FrameException e)
        {
            ParseErrors++;
            _problems.Add(e.Message);
            return null;
        }
        if (f.Kind == Tmf8829Frames.FidRefSpad)
        {
            _pending.Add((f, raw)); // zero pre-header: no clock pair
            return null;
        }
        if (f.Kind != Tmf8829Frames.FidResults && f.Kind != Tmf8829Frames.FidHistograms)
        {
            ParseErrors++;
            _problems.Add($"frame id 0x{f.Id:X2}: unknown kind");
            return null;
        }
        int flags = Clock.Add(timestampUs, f.Systick);
        var bad = f.Problems();
        if (bad.Count > 0)
            _problems.Add($"frame id=0x{f.Id:X2} fN={f.FNumber} {f.Size} B: {string.Join(", ", bad)}");
        if (f.Kind == Tmf8829Frames.FidHistograms)
        {
            Timer.Histograms();
            _pending.Add((f, raw));
            return null;
        }

        var t = Timer.ResultsFrame(f, timestampUs, edgeUs, flags);
        foreach (var (pf, praw) in _pending)
        {
            if (pf.Kind == Tmf8829Frames.FidHistograms)
                _hist.Add(new Tmf8829HistogramFrame(pf, praw));
            else
                _refs.Add(praw);
        }
        _pending.Clear();

        Tmf8829PixelResults res;
        try
        {
            res = Tmf8829Frames.ParseResults(raw, f);
        }
        catch (Tmf8829FrameException e)
        {
            ParseErrors++;
            _problems.Add($"fN={f.FNumber}: {e.Message}");
            _half = null;
            return null;
        }

        Tmf8829FrameInfo[] frames;
        Tmf8829FrameTime[] times;
        if (Tmf8829Frames.FpGrid.TryGetValue(f.FpMode, out var grid) && grid.Rows == 32)
        {
            if (_half is not { } half)
            {
                if (f.IsSecondHalf)
                {
                    _problems.Add($"second half fN={f.FNumber} without a first one - dropped");
                    _hist = new();
                    _refs = new();
                    return null;
                }
                _half = (f, res, t);
                return null;
            }
            if (!f.IsSecondHalf)
            {
                _problems.Add($"two first halves in a row (fN {half.Info.FNumber}, {f.FNumber})");
                _half = (f, res, t);
                return null;
            }
            _half = null;
            if (half.Info.FpMode != f.FpMode
                || (half.Info.Layout & ~Tmf8829Frames.LayoutSubframe) != (f.Layout & ~Tmf8829Frames.LayoutSubframe))
            {
                // Impossible within one stream (configure() needs a stop); never throw on a malformed one.
                _problems.Add($"half-frames of different formats (fN {half.Info.FNumber}, {f.FNumber}) - dropped");
                _hist = new();
                _refs = new();
                return null;
            }
            frames = new[] { half.Info, f };
            times = new[] { half.Time, t };
            res = Tmf8829Frames.MergeHalves(half.Res, res);
        }
        else
        {
            frames = new[] { f };
            times = new[] { t };
        }

        long sum = 0;
        int orFlags = 0;
        foreach (var x in times)
        {
            sum += x.TimestampUs;
            orFlags |= x.Flags;
        }
        var m = new Tmf8829Measurement(
            Tmf8829Math.FloorDiv(sum, times.Length),
            orFlags,
            f.FpMode,
            f.FNumber,
            res,
            Tmf8829Measurement.ToMm(res, Clock.Ratio, f.DistanceInMm),
            frames,
            times,
            _hist,
            _refs,
            _problems);
        _hist = new();
        _refs = new();
        _problems = new();
        return m;
    }
}
