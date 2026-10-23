using Depz.Sensor.Transport;
using Depz.Sensor.Vl53l4;
using Depz.Sensor.Vl53lx;

namespace Depz.Sensor.Devices;

/// <summary>One ranging result of the 1D family, the same shape for every product. On the histogram driver
/// <see cref="Targets"/> holds every return, strongest first, the top level repeats the first and
/// <see cref="Bins"/> carries the raw 24-bin histogram; the light drivers leave both empty.</summary>
public sealed record Vl53lxMeasurement(
    ulong TimestampUs,
    int DistanceMm,
    int Status,
    string StatusText,
    double SignalKcps,
    double AmbientKcps,
    double SigmaMm,
    double Spads,
    IReadOnlyList<Vl53lxTarget> Targets,
    IReadOnlyDictionary<string, object?> Extra,
    object? Bins)
{
    /// <summary>Statuses that mean "this distance is real": a histogram product reports 6 on the first frame
    /// of a stream and 11 for two merged targets. Prefer <see cref="Plottable"/> over <see cref="Valid"/> there.</summary>
    public static readonly IReadOnlyList<int> PlottableStatuses = new[] { 0, 6, 11 };

    public bool Valid => Status == 0;

    public bool Plottable => PlottableStatuses.Contains(Status);

    /// <summary>The distances a chart draws: the plottable targets in driver order, or the single distance.</summary>
    public IReadOnlyList<int> PlotDistances() =>
        Targets.Count > 0
            ? Targets.Where(t => PlottableStatuses.Contains(t.Status)).Select(t => t.DistanceMm).ToList()
            : Plottable ? new[] { DistanceMm } : Array.Empty<int>();

    /// <summary>The one target a single-number readout shows, or null.</summary>
    public Vl53lxTarget? PrimaryTarget() => Targets.FirstOrDefault(t => PlottableStatuses.Contains(t.Status));

    internal static Vl53lxMeasurement From(ulong timestampUs, Vl53lxRange r, object? bins) =>
        new(timestampUs, r.DistanceMm, r.Status, r.StatusText, r.SignalKcps, r.AmbientKcps, r.SigmaMm, r.Spads,
            r.Targets, r.Extra, bins);
}

/// <summary>
/// A board of the VL53L 1D family on the <c>APP_VL53L0_4</c> register bridge (contract 12): VL53L0X,
/// VL53L1CX, VL53L1CB, VL53L3CX, VL53L4CD, VL53L4CX. Every ULD runs here; two axes pick how a board is
/// driven — the product (whose parameter set: the board's own, or a sibling's to borrow its driver) and
/// the driver kind (<c>uld</c>, <c>ulp</c>, <c>histogram</c>). <see cref="Init"/> binds the pair,
/// <see cref="Configure"/> re-initialises and applies a ranging configuration (before every run), then
/// <see cref="StartRanging"/> and read <see cref="Measurements"/>. Ask <see cref="Supports"/> before an
/// optional group; a missing one throws <see cref="NotSupportedException"/>. This generic class reads the
/// product from the board's name; the per-product classes fix it.
/// </summary>
public class Vl53lxDevice : DepzDevice
{
    private static readonly TimeSpan RegTimeout = TimeSpan.FromSeconds(2);
    private const int StreamMinBytes = 12;
    private const int ErrInvalidCmd = 0x02;

    private readonly Fanout<Vl53lxMeasurement> _measurements = new();
    private BridgePlatform? _platform;
    private Vl53lxDriver? _driver;
    private string? _boardName;
    private volatile bool _ranging;
    private long _streamParseErrors;

    public Vl53lxDevice(ILink link, TimeSpan? timeout = null) : base(link, timeout) { }

#if NET
    public Vl53lxDevice(string port, TimeSpan? timeout = null) : base(port, timeout) { }
#endif

    internal Vl53lxDevice(DepzDevice from) : base(from) { }

    /// <summary>The product this class drives; null = read it from the device name.</summary>
    protected virtual string? FixedProduct => null;

    private BridgePlatform Platform => _platform ??= new BridgePlatform(this);

    // ── identity ─────────────────────────────────────────────────────────────

    /// <summary>The board's device name, e.g. <c>DEPZ ToF Sensor VL53L4CX USB v2.1 TOVJALN523</c>.</summary>
    public string BoardName => _boardName ??= GetDeviceName();

    /// <summary>The product the device name carries, or null on an unstamped board.</summary>
    public string? Detected => Vl53lxProducts.ProductFromBoardName(BoardName);

    /// <summary>The product <see cref="Init"/> bound (null before).</summary>
    public string? Product { get; private set; }

    public string? DriverKind { get; private set; }

    public string? Caveat { get; private set; }

    /// <summary>The ULD port in use — escape hatch for product-specific calls.</summary>
    public Vl53lxDriver Driver => _driver ?? throw new InvalidOperationException("call Init() first");

    public bool Initialized => _driver != null;

    public bool Ranging => _ranging;

    /// <summary>The driver kinds a product has (default: this board's).</summary>
    public IReadOnlyList<string> DriverKinds(string? product = null)
    {
        string? name = product ?? FixedProduct ?? Detected;
        return name is null ? Array.Empty<string>() : Vl53lxProducts.DriverKinds(name);
    }

    // ── lifecycle ────────────────────────────────────────────────────────────

    /// <summary>Bind the (product, driver kind) pair and initialise the sensor. <paramref name="product"/>
    /// defaults to this class's, else the board name's; <paramref name="driver"/> to the product's first kind.
    /// A pair the table lacks throws <see cref="NotSupportedException"/> naming what the product has.</summary>
    public void Init(string? driver = null, string? product = null)
    {
        RequireNotRanging();
        string name = product ?? FixedProduct ?? Detected
            ?? throw new InvalidOperationException($"board '{BoardName}' carries no product number - pass product explicitly");
        string kind = driver ?? Vl53lxProducts.DriverKinds(name).FirstOrDefault()
            ?? throw new NotSupportedException($"{name} has no driver this SDK runs yet");
        var (drv, caveat) = Vl53lxProducts.CreateDriver(name, kind, Platform);
        // Sticky on the bridge and 2 after a reset: set before the first register access.
        Platform.SetAddrWidth(drv.AddrWidth);
        SensorInit(drv);
        (_driver, Product, DriverKind, Caveat) = (drv, name, kind, caveat);
    }

    // A reset makes the die NACK for a moment; the driver waits those out, and they are no bus fault.
    private void SensorInit(Vl53lxDriver drv)
    {
        drv.SensorInit();
        Platform.ClearI2cErrors();
    }

    /// <summary>Everything about what is connected (needs <see cref="Init"/>). <c>model_id_ok</c> is a
    /// cross-check only: L1CX/L1CB and L4CD/L4CX share their ids.</summary>
    public IReadOnlyDictionary<string, object?> IdentifyProduct()
    {
        var drv = Driver;
        int modelId = drv.ModelId();
        return new Dictionary<string, object?>
        {
            ["board"] = BoardName,
            ["detected"] = Detected,
            ["product"] = Product,
            ["driver"] = DriverKind,
            ["driver_class"] = drv.GetType().Name,
            ["kinds"] = Vl53lxProducts.DriverKinds(Product!),
            ["model_id"] = modelId,
            ["model_id_ok"] = Vl53lxProducts.ModelIdOk(Product!, modelId),
            ["supports"] = drv.Supports.ToList(),
            ["modes"] = drv.Modes,
            ["reach_mm"] = Vl53lxProducts.Find(Detected ?? Product)?.ReachMm,
            ["driver_reach_mm"] = drv.ReachMm(),
            ["budget_ms"] = new[] { drv.BudgetMs.Min, drv.BudgetMs.Max },
            ["budget_choices"] = BudgetChoices(),
            ["histogram"] = drv.Histogram,
            ["max_khz"] = drv.MaxKhz,
            ["caveat"] = Caveat,
        };
    }

    /// <summary>The lines a UI shows about the chosen pair: a product named by hand, a borrowed driver, a
    /// pair that reaches less than the board is rated for, the driver's caveat.</summary>
    public IReadOnlyList<string> Notes()
    {
        var drv = Driver;
        var output = new List<string>();
        if (Detected is null)
            output.Add($"{Product}: board name carries no product number, named by hand");
        else if (Detected != Product)
            output.Add($"{Product}: borrowing this driver - the board says it is a {Detected}");
        int? rated = Vl53lxProducts.Find(Detected ?? Product)?.ReachMm;
        int? reach = drv.ReachMm();
        if (rated is int r && reach is int d && d < r)
            output.Add($"{Product}/{DriverKind}: this pair reaches {d} mm, the board is rated {r} mm - past {d} mm "
                       + "the phase is out of the valid window and frames come back with status 4");
        if (Caveat != null)
            output.Add($"{Product}/{DriverKind}: {Caveat}");
        return output;
    }

    /// <summary>Whether this product / driver serves an optional group.</summary>
    public bool Supports(string group) => Driver.Supports.Contains(group);

    /// <summary>Named ranging modes, the first being what init leaves; empty if none.</summary>
    public IReadOnlyList<string> Modes => Driver.Modes;

    /// <summary>The (min, max) budget the driver offers, ms.</summary>
    public (int Min, int Max) BudgetRange => Driver.BudgetMs;

    /// <summary>How far the loaded configuration can measure, mm, or null.</summary>
    public int? DriverReachMm() => Driver.ReachMm();

    // ── sensor power and the bridge ──────────────────────────────────────────

    /// <summary>Drive XSHUT (<see cref="Vl53l4Xshut"/>). Off and Reset stop the stream; the sensor then
    /// holds none of the configuration — <see cref="Init"/> again.</summary>
    public void Xshut(byte action)
    {
        if (action > Vl53l4Xshut.Reset)
            throw new ArgumentOutOfRangeException(nameof(action), action, "XSHUT action must be 0 (off), 1 (on) or 2 (reset)");
        RequestOk((int)Vl53lxCmd.Xshut, Vl53lxWire.PackXshut(action), TimeSpan.FromSeconds(1));
        _ranging = false;
        _driver = null;
    }

    /// <summary>RPT_VL53_INFO — the bridge's own counters and settings; safe while streaming.</summary>
    public Vl53lxInfo BridgeInfo() =>
        Request((int)Vl53lxCmd.GetInfo, null, ExpectReport((int)Vl53lxRpt.Info, p => Vl53lxInfo.Unpack(p)));

    // ── configuration (Init first; not while ranging) ────────────────────────

    /// <summary>Re-initialise the sensor and apply a ranging configuration. The re-init is deliberate: it is
    /// the only way to know what the configuration registers hold. <paramref name="mode"/> goes on before
    /// the budget; <paramref name="offsetMm"/> / <paramref name="xtalkKcps"/> re-apply a stored calibration;
    /// <paramref name="signalKcps"/> replaces the blob's signal threshold. <paramref name="interMs"/> 0 =
    /// continuous. An unsupported group is refused before the re-init.</summary>
    public void Configure(int budgetMs = 50, int interMs = 0, string? mode = null, int? offsetMm = null,
        int? xtalkKcps = null, int? signalKcps = null)
    {
        RequireNotRanging();
        var drv = Driver;
        if (mode != null) Need("mode");
        if (offsetMm != null) Need("offset");
        if (xtalkKcps != null) Need("xtalk");
        if (signalKcps != null) Need("signal_thresh");
        SensorInit(drv);
        if (mode != null)
            drv.SetMode(mode);
        drv.SetRangeTiming(budgetMs, interMs);
        if (offsetMm is int o)
            drv.SetOffset(o);
        if (xtalkKcps is int x)
            drv.SetXtalk(x);
        if (signalKcps is int s)
            drv.SetSignalThreshold(s);
    }

    /// <summary>(timing budget ms, inter-measurement ms) read back from the sensor; 0 = continuous.</summary>
    public (int TimingBudgetMs, int InterMeasurementMs) GetRangeTiming() => Driver.GetRangeTiming();

    /// <summary>The mode in use, or null on a product without modes.</summary>
    public string? GetMode() => Supports("mode") ? Driver.GetMode() : null;

    /// <summary>The only budgets accepted right now, or empty when any in <see cref="BudgetRange"/> will do.</summary>
    public IReadOnlyList<int> BudgetChoices() => Driver.BudgetChoices();

    /// <summary>The nearest budget this product accepts.</summary>
    public int SnapBudget(int budgetMs)
    {
        var choices = BudgetChoices();
        if (choices.Count > 0)
            return choices.OrderBy(c => Math.Abs(c - budgetMs)).First();
        var (low, high) = Driver.BudgetMs;
        return Math.Max(low, Math.Min(high, budgetMs));
    }

    public int GetOffsetMm() => Need("offset").GetOffset();

    public void SetOffsetMm(int offsetMm)
    {
        RequireNotRanging();
        Need("offset").SetOffset(offsetMm);
    }

    public int GetXtalkKcps() => Need("xtalk").GetXtalk();

    public void SetXtalkKcps(int xtalkKcps)
    {
        RequireNotRanging();
        Need("xtalk").SetXtalk(xtalkKcps);
    }

    /// <summary>Offset calibration against a flat target; returns the offset programmed (store it on the host).</summary>
    public int CalibrateOffset(int targetDistMm, int? nbSamples = null)
    {
        RequireNotRanging();
        return Need("calib_offset").CalibrateOffset(targetDistMm, nbSamples);
    }

    /// <summary>Crosstalk calibration; returns the xtalk programmed, kcps (store it on the host).</summary>
    public int CalibrateXtalk(int targetDistMm, int? nbSamples = null)
    {
        RequireNotRanging();
        return Need("calib_xtalk").CalibrateXtalk(targetDistMm, nbSamples);
    }

    public (int DistanceLowMm, int DistanceHighMm, int Window) GetDetectionThresholds() =>
        Need("thresholds").GetDetectionThresholds();

    /// <summary>The distance-window interrupt; stays armed until the next init / configure.</summary>
    public void SetDetectionThresholds(int distanceLowMm, int distanceHighMm, int window)
    {
        RequireNotRanging();
        Need("thresholds").SetDetectionThresholds(distanceLowMm, distanceHighMm, window);
    }

    public int GetSignalThresholdKcps() => Need("signal_thresh").GetSignalThreshold();

    public void SetSignalThresholdKcps(int signalKcps)
    {
        RequireNotRanging();
        Need("signal_thresh").SetSignalThreshold(signalKcps);
    }

    public int GetSigmaThresholdMm() => Need("sigma_thresh").GetSigmaThreshold();

    public void SetSigmaThresholdMm(int sigmaMm)
    {
        RequireNotRanging();
        Need("sigma_thresh").SetSigmaThreshold(sigmaMm);
    }

    public (int X, int Y) GetRoi() => Need("roi").GetRoi();

    public void SetRoi(int x, int y)
    {
        RequireNotRanging();
        Need("roi").SetRoi(x, y);
    }

    public int GetRoiCenter() => Need("roi").GetRoiCenter();

    public void SetRoiCenter(int centerSpad)
    {
        RequireNotRanging();
        Need("roi").SetRoiCenter(centerSpad);
    }

    /// <summary>Re-run VHV after an ambient change over 8 °C.</summary>
    public void StartTemperatureUpdate()
    {
        RequireNotRanging();
        Need("temp_update").StartTemperatureUpdate();
    }

    /// <summary>VL53L0X: re-measure the reference SPADs.</summary>
    public object PerformRefSpadManagement()
    {
        RequireNotRanging();
        return Need("refspad").PerformRefSpadManagement();
    }

    // ── ranging ──────────────────────────────────────────────────────────────

    /// <summary>Start the sensor and arm the board's stream: one report per INT edge, followed on the MCU by
    /// the driver's interrupt-release writes.</summary>
    public void StartRanging()
    {
        RequireNotRanging();
        var drv = Driver;
        drv.StartRanging();
        var (addr, len) = drv.StreamBlock();
        RequestOk((int)Vl53lxCmd.StartStream, Vl53lxWire.PackStartStream((ushort)addr, (ushort)len, drv.ClearSteps));
        _ranging = true;
    }

    public void StopRanging()
    {
        if (!_ranging)
            return;
        try
        {
            RequestOk((int)Vl53lxCmd.StopStream);
        }
        finally
        {
            _ranging = false;
            Driver.StopRanging();
        }
    }

    /// <summary>One polled measurement: start, data ready, read, release the interrupt, stop. Not while streaming.</summary>
    public Vl53lxMeasurement MeasureOnce(TimeSpan? timeout = null)
    {
        RequireNotRanging();
        var drv = Driver;
        drv.StartRanging();
        Vl53lxRange m;
        object? bins;
        try
        {
            drv.WaitDataReady(timeout ?? TimeSpan.FromSeconds(1));
            if (drv.Histogram)
            {
                bins = drv.BinData();
                m = drv.ToMeasurement(bins);
            }
            else
            {
                m = drv.ReadMeasurement();
                bins = null;
            }
            drv.ClearInterrupt();
        }
        finally
        {
            drv.StopRanging();
        }
        return Vl53lxMeasurement.From(Platform.LastTimestampUs, m, bins);
    }

    public IDisposable OnMeasurement(Action<Vl53lxMeasurement> callback) => _measurements.OnItem(callback);

    public Subscription<Vl53lxMeasurement> Measurements(int capacity = 64) => _measurements.Subscribe(capacity);

    public Vl53lxMeasurement GetMeasurement(TimeSpan? timeout = null) =>
        _measurements.Next(timeout ?? TimeSpan.FromSeconds(2), (int)Vl53lxRpt.Stream, () => IsClosed);

    /// <summary>Stream reports dropped because the driver did not decode them.</summary>
    public long StreamParseErrors => Interlocked.Read(ref _streamParseErrors);

    public IReadOnlyList<long> StreamDroppedCounts => _measurements.DroppedCounts;

    // ── internal ─────────────────────────────────────────────────────────────

    private Vl53lxDriver Need(string group)
    {
        var drv = Driver;
        if (!drv.Supports.Contains(group))
            throw new NotSupportedException($"{Product}/{DriverKind} does not support '{group}'" + (Caveat != null ? $" ({Caveat})" : ""));
        return drv;
    }

    private void RequireNotRanging()
    {
        if (_ranging)
            throw new InvalidOperationException("StopRanging() first — the stream owns the bus");
    }

    private protected override bool HandleReport(Packet packet)
    {
        if (packet.Cmd != (int)Vl53lxRpt.Stream || packet.Payload.Length < StreamMinBytes)
            return false;
        var drv = _driver;
        if (drv is null)
            return true;
        Vl53lxMeasurement m;
        try
        {
            var sample = Vl53l4StreamData.Unpack(packet.Payload); // shorter than its length field: refused
            // Decoded exactly once: the histogram driver steps its own frame-pair state on every frame.
            if (drv.Histogram)
            {
                object bins = drv.BinData(sample.Data);
                m = Vl53lxMeasurement.From(sample.TimestampUs, drv.ToMeasurement(bins), bins);
            }
            else
            {
                m = Vl53lxMeasurement.From(sample.TimestampUs, drv.Decode(sample.Data), null);
            }
        }
        catch (Exception e) when (!(e is OutOfMemoryException))
        {
            Interlocked.Increment(ref _streamParseErrors); // a bad sample must not kill the reader
            return true;
        }
        _measurements.Emit(m);
        return true;
    }

    private protected override void OnReaderStopped()
    {
        _measurements.CloseAll();
        base.OnReaderStopped();
    }

    // The platform the ULD ports expect, over the register bridge: transfers split at 253 bytes; a non-OK
    // status or a missing answer surfaces as Vl53lxProtocolException, which the drivers catch.
    private sealed class BridgePlatform : Vl53lxPlatform
    {
        private readonly Vl53lxDevice _dev;

        public BridgePlatform(Vl53lxDevice dev) => _dev = dev;

        public ulong LastTimestampUs;

        private static T Call<T>(Func<T> fn)
        {
            try
            {
                return fn();
            }
            catch (Exception e) when (e is StatusException || e is DepzTimeoutException)
            {
                throw new Vl53lxProtocolException(e.Message, e);
            }
        }

        public override byte[] ReadMulti(int addr, int size)
        {
            var output = new byte[size];
            int done = 0;
            while (done < size)
            {
                int n = Math.Min(size - done, Vl53lxWire.XferMax);
                int a = addr + done;
                var rep = Call(() => _dev.Request((int)Vl53lxCmd.ReadReg, Vl53lxWire.PackReadReg((ushort)a, (ushort)n),
                    ExpectReport((int)Vl53lxRpt.RegData, p => Vl53l4RegData.Unpack(p)), RegTimeout));
                if (rep.Data.Length != n)
                    throw new Vl53lxProtocolException($"READ_REG 0x{a:X4}: expected {n}, got {rep.Data.Length}");
                LastTimestampUs = rep.TimestampUs;
                Array.Copy(rep.Data, 0, output, done, n);
                done += n;
            }
            return output;
        }

        public override void WriteMulti(int addr, byte[] data)
        {
            int done = 0;
            while (done < data.Length)
            {
                int n = Math.Min(data.Length - done, Vl53lxWire.XferMax);
                byte[] payload = Vl53lxWire.PackWriteReg((ushort)(addr + done), data.AsSpan(done, n));
                Call(() =>
                {
                    _dev.RequestOk((int)Vl53lxCmd.WriteReg, payload, RegTimeout);
                    return 0;
                });
                done += n;
            }
        }

        public override void SetI2cSpeed(int khz) =>
            Call(() =>
            {
                _dev.RequestOk((int)Vl53lxCmd.SetI2cSpeed, Vl53lxWire.PackSetI2cSpeed((ushort)khz));
                return 0;
            });

        public override void SetAddrWidth(int width) =>
            Call(() =>
            {
                _dev.RequestOk((int)Vl53lxCmd.SetAddrWidth, Vl53lxWire.PackSetAddrWidth(width));
                return 0;
            });

        public override void XshutReset() =>
            Call(() =>
            {
                _dev.RequestOk((int)Vl53lxCmd.Xshut, Vl53lxWire.PackXshut(Vl53l4Xshut.Reset), TimeSpan.FromSeconds(1));
                return 0;
            });

        /// <summary>Zero the bridge's I2C error counters once an init is through (a resetting die NACKs for
        /// a moment, expectedly). A v2.00 bridge (firmware older than v0.24) lacks the command.</summary>
        public void ClearI2cErrors()
        {
            try
            {
                _dev.RequestOk((int)Vl53lxCmd.ClearI2cErrors);
            }
            catch (StatusException e) when (e.Status == ErrInvalidCmd)
            {
                throw new DepzException("the board's firmware predates APP_VL53L0_4_v0.24 (protocol v2.01) and "
                                        + "cannot clear its I2C error counter — reflash it", e);
            }
            catch (Exception e) when (e is StatusException || e is DepzTimeoutException)
            {
                throw new Vl53lxProtocolException(e.Message, e);
            }
        }
    }
}

/// <summary>VL53L0X (2 m, 1-byte register addresses, 400 kHz): ULD API 1.0.4 with ranging profiles.</summary>
public sealed class Vl53l0xDevice : Vl53lxDevice
{
    public Vl53l0xDevice(ILink link, TimeSpan? timeout = null) : base(link, timeout) { }
#if NET
    public Vl53l0xDevice(string port, TimeSpan? timeout = null) : base(port, timeout) { }
#endif
    internal Vl53l0xDevice(DepzDevice from) : base(from) { }
    protected override string? FixedProduct => "VL53L0X";
}

/// <summary>VL53L1CX (4 m): the VL53L1X ULD (short / long) or the histogram driver.</summary>
public sealed class Vl53l1cxDevice : Vl53lxDevice
{
    public Vl53l1cxDevice(ILink link, TimeSpan? timeout = null) : base(link, timeout) { }
#if NET
    public Vl53l1cxDevice(string port, TimeSpan? timeout = null) : base(port, timeout) { }
#endif
    internal Vl53l1cxDevice(DepzDevice from) : base(from) { }
    protected override string? FixedProduct => "VL53L1CX";
}

/// <summary>VL53L1CB (8 m, cover-glass module): the VL53L1CX die and drivers; usually needs a crosstalk calibration.</summary>
public sealed class Vl53l1cbDevice : Vl53lxDevice
{
    public Vl53l1cbDevice(ILink link, TimeSpan? timeout = null) : base(link, timeout) { }
#if NET
    public Vl53l1cbDevice(string port, TimeSpan? timeout = null) : base(port, timeout) { }
#endif
    internal Vl53l1cbDevice(DepzDevice from) : base(from) { }
    protected override string? FixedProduct => "VL53L1CB";
}

/// <summary>VL53L3CX (3 m): ST's ULP driver (single target) or the histogram driver (several per frame).</summary>
public sealed class Vl53l3cxDevice : Vl53lxDevice
{
    public Vl53l3cxDevice(ILink link, TimeSpan? timeout = null) : base(link, timeout) { }
#if NET
    public Vl53l3cxDevice(string port, TimeSpan? timeout = null) : base(port, timeout) { }
#endif
    internal Vl53l3cxDevice(DepzDevice from) : base(from) { }
    protected override string? FixedProduct => "VL53L3CX";
}

/// <summary>VL53L4CX (6 m): the histogram driver only — to run it light, borrow a sibling's with
/// <c>Init(product: "VL53L4CD")</c>.</summary>
public sealed class Vl53l4cxDevice : Vl53lxDevice
{
    public Vl53l4cxDevice(ILink link, TimeSpan? timeout = null) : base(link, timeout) { }
#if NET
    public Vl53l4cxDevice(string port, TimeSpan? timeout = null) : base(port, timeout) { }
#endif
    internal Vl53l4cxDevice(DepzDevice from) : base(from) { }
    protected override string? FixedProduct => "VL53L4CX";
}
