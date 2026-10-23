using System.Buffers.Binary;
using System.Diagnostics;
using Depz.Sensor.Devices;

namespace Depz.Sensor.Vl53lx;

/// <summary>A ULD-level failure: the sensor did not do what the driver needed (a timeout waiting for
/// data ready or boot, a calibration that failed, a mode the driver lacks…).</summary>
public class Vl53lxException : DepzException
{
    public Vl53lxException(string message) : base(message) { }
}

/// <summary>The bridge refused a register command or did not answer it. The drivers catch it where a
/// sensor legitimately NACKs for a while (right after a soft reset).</summary>
public sealed class Vl53lxProtocolException : DepzException
{
    public Vl53lxProtocolException(string message, Exception? inner = null) : base(message, inner ?? new Exception(message)) { }
}

/// <summary>
/// ST's platform layer (RdByte / WrWord / WaitMs …) for the 1D family, plus what the bridge owns: bus
/// speed, register-address width and the XSHUT line. Register values are big-endian, as the C drivers
/// assume. <see cref="Devices.Vl53lxDevice"/> implements it over the board; a test can script it.
/// </summary>
public abstract class Vl53lxPlatform
{
    public abstract byte[] ReadMulti(int addr, int size);

    public abstract void WriteMulti(int addr, byte[] data);

    /// <summary>The bridge's bus timing (not part of the C platform layer).</summary>
    public abstract void SetI2cSpeed(int khz);

    /// <summary>How wide a register address the bridge puts on the wire; sticky.</summary>
    public abstract void SetAddrWidth(int width);

    /// <summary>Reset through XSHUT, for a sensor that no longer acknowledges writes.</summary>
    public abstract void XshutReset();

    public virtual void SleepMs(int ms) => Thread.Sleep(ms);

    public int ReadByte(int addr) => ReadMulti(addr, 1)[0];

    public int ReadWord(int addr) => BinaryPrimitives.ReadUInt16BigEndian(ReadMulti(addr, 2));

    public uint ReadDword(int addr) => BinaryPrimitives.ReadUInt32BigEndian(ReadMulti(addr, 4));

    public void WriteByte(int addr, int value) => WriteMulti(addr, new[] { (byte)value });

    public void WriteWord(int addr, int value)
    {
        var b = new byte[2];
        BinaryPrimitives.WriteUInt16BigEndian(b, (ushort)value);
        WriteMulti(addr, b);
    }

    public void WriteDword(int addr, long value)
    {
        var b = new byte[4];
        BinaryPrimitives.WriteUInt32BigEndian(b, (uint)value);
        WriteMulti(addr, b);
    }
}

/// <summary>One return of a multi-target frame. <see cref="MinRangeMm"/> / <see cref="MaxRangeMm"/> are the
/// edges of the target's own pulse, which only the histogram driver knows.</summary>
public sealed record Vl53lxTarget(int DistanceMm, int Status, string StatusText, double SignalKcps, double AmbientKcps,
    double SigmaMm, int MinRangeMm, int MaxRangeMm);

/// <summary>One range result as a driver reports it, the same shape for every product of the family. On the
/// histogram driver <see cref="Targets"/> holds every return and the top level repeats the first; the light
/// drivers leave it empty. <see cref="Extra"/> is driver-specific — show it, don't branch on it.</summary>
public sealed record Vl53lxRange(int DistanceMm, int Status, string StatusText, double SignalKcps, double AmbientKcps,
    double SigmaMm, double Spads, IReadOnlyList<Vl53lxTarget> Targets, IReadOnlyDictionary<string, object?> Extra)
{
    public bool Valid => Status == 0;
}

/// <summary>
/// The contract every driver of the 1D family implements (uld/base.py). The class properties say what
/// the product is as far as the bridge is concerned and which optional groups it serves
/// (<see cref="Supports"/>): mode, timing, offset, xtalk, calib_offset, calib_xtalk, thresholds,
/// signal_thresh, sigma_thresh, roi, temp_update, refspad. A group a driver lacks throws
/// <see cref="NotSupportedException"/>.
/// </summary>
public abstract class Vl53lxDriver
{
    protected Vl53lxDriver(Vl53lxPlatform platform, string product)
    {
        P = platform ?? throw new ArgumentNullException(nameof(platform));
        Product = product;
    }

    protected Vl53lxPlatform P { get; }

    /// <summary>The product this driver was bound for (one driver serves several).</summary>
    public string Product { get; }

    /// <summary>Bytes of register address on the wire.</summary>
    public virtual int AddrWidth => 2;

    /// <summary>The interrupt-release writes handed to START_STREAM.</summary>
    public virtual IReadOnlyList<Vl53lxClearStep> ClearSteps => Array.Empty<Vl53lxClearStep>();

    /// <summary>The bus ceiling; the driver runs the bus there itself.</summary>
    public virtual int MaxKhz => 400;

    public virtual IReadOnlyCollection<string> Supports => Array.Empty<string>();

    /// <summary>The named ranging modes, the first being what init leaves; empty when there are none.</summary>
    public virtual IReadOnlyList<string> Modes => Array.Empty<string>();

    /// <summary>The (min, max) budget a UI should offer, ms; see <see cref="BudgetChoices"/>.</summary>
    public virtual (int Min, int Max) BudgetMs => (10, 200);

    /// <summary>The frame is a raw histogram (the histogram driver).</summary>
    public virtual bool Histogram => false;

    public abstract int ModelId();

    /// <summary>Reset and configure the sensor; leaves the bus at <see cref="MaxKhz"/>. Safe to re-run.</summary>
    public abstract void SensorInit();

    public abstract void StartRanging();

    public abstract void StopRanging();

    public abstract bool CheckForDataReady();

    public virtual void WaitDataReady(TimeSpan timeout)
    {
        var clock = Stopwatch.StartNew();
        while (!CheckForDataReady())
        {
            if (clock.Elapsed > timeout)
                throw new Vl53lxException("timeout waiting for data ready");
            P.SleepMs(1);
        }
    }

    public abstract void ClearInterrupt();

    /// <summary>(addr, len) — the register block the bridge streams on each INT.</summary>
    public abstract (int Addr, int Len) StreamBlock();

    /// <summary>Streamed block bytes → a range.</summary>
    public abstract Vl53lxRange Decode(byte[] raw);

    /// <summary>One polled result: the stream block read and decoded.</summary>
    public virtual Vl53lxRange ReadMeasurement()
    {
        var (addr, len) = StreamBlock();
        return Decode(P.ReadMulti(addr, len));
    }

    /// <summary>Budget of one measurement and the period between two; a period of 0 means continuous.</summary>
    public abstract void SetRangeTiming(int timingBudgetMs, int interMeasurementMs);

    public abstract (int TimingBudgetMs, int InterMeasurementMs) GetRangeTiming();

    /// <summary>The only budgets accepted right now (ascending), or empty when any integer of
    /// <see cref="BudgetMs"/> will do.</summary>
    public virtual IReadOnlyList<int> BudgetChoices() => Array.Empty<int>();

    public virtual void SetMode(string name) => throw Unsupported("mode");

    public virtual string GetMode() => throw Unsupported("mode");

    /// <summary>How far the loaded configuration can measure, mm, or null when not characterised.</summary>
    public virtual int? ReachMm() => null;

    /// <summary>Numbers this driver shows that the contract has no field for.</summary>
    public virtual IReadOnlyDictionary<string, object?> DriverInfo() => new Dictionary<string, object?>();

    // ── optional groups ──────────────────────────────────────────────────────

    public virtual int GetOffset() => throw Unsupported("offset");
    public virtual void SetOffset(int offsetMm) => throw Unsupported("offset");
    public virtual int GetXtalk() => throw Unsupported("xtalk");
    public virtual void SetXtalk(int xtalkKcps) => throw Unsupported("xtalk");
    public virtual int CalibrateOffset(int targetDistMm, int? nbSamples = null) => throw Unsupported("calib_offset");
    public virtual int CalibrateXtalk(int targetDistMm, int? nbSamples = null) => throw Unsupported("calib_xtalk");
    public virtual (int DistanceLowMm, int DistanceHighMm, int Window) GetDetectionThresholds() => throw Unsupported("thresholds");
    public virtual void SetDetectionThresholds(int distanceLowMm, int distanceHighMm, int window) => throw Unsupported("thresholds");
    public virtual int GetSignalThreshold() => throw Unsupported("signal_thresh");
    public virtual void SetSignalThreshold(int signalKcps) => throw Unsupported("signal_thresh");
    public virtual int GetSigmaThreshold() => throw Unsupported("sigma_thresh");
    public virtual void SetSigmaThreshold(int sigmaMm) => throw Unsupported("sigma_thresh");
    public virtual (int X, int Y) GetRoi() => throw Unsupported("roi");
    public virtual void SetRoi(int x, int y) => throw Unsupported("roi");
    public virtual int GetRoiCenter() => throw Unsupported("roi");
    public virtual void SetRoiCenter(int centerSpad) => throw Unsupported("roi");
    public virtual void StartTemperatureUpdate() => throw Unsupported("temp_update");
    public virtual object PerformRefSpadManagement() => throw Unsupported("refspad");

    // ── histogram driver ─────────────────────────────────────────────────────

    /// <summary>The raw histogram of a frame (read from the sensor when <paramref name="raw"/> is null).</summary>
    public virtual object BinData(byte[]? raw = null) => throw new NotSupportedException($"{GetType().Name} has no histogram");

    /// <summary>A histogram frame → a range (the host-side ranging of the histogram driver).</summary>
    public virtual Vl53lxRange ToMeasurement(object bins) => throw new NotSupportedException($"{GetType().Name} has no histogram");

    protected NotSupportedException Unsupported(string group) =>
        new($"{Product}/{GetType().Name} does not support '{group}'");
}
