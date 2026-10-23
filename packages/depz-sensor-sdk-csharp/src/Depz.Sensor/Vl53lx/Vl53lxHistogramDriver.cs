using System.Diagnostics;
using Depz.Sensor.Vl53lx.Bare;

namespace Depz.Sensor.Vl53lx;

/// <summary>
/// ST's VL53LX BareDriver on the host — the histogram driver of VL53L1CX / L1CB / L3CX / L4CD / L4CX: the die
/// hands over 24 raw photon-count bins per frame and <see cref="Hist"/> turns them into up to four targets. What it
/// costs against the light drivers: no calibrations and no detection thresholds, and the first frame of a stream
/// reports status 6 (no wrap check done) because the check needs the frame before.
/// </summary>
public sealed class Vl53lxHistogramDriver : Vl53lxDriver
{
    private const int GpioHvMuxCtrl = 0x0030;
    private const int GpioTioHvStatus = 0x0031;
    private const int SystemInterruptClear = 0x0086;
    private const int FirmwareSystemStatus = 0x00E5;
    private const int PadI2cHvConfig = 0x002D;
    private const int I2cKhzBoot = 400;

    // Bits 2 and 5, the byte VL53L4CD_I2C_FAST_MODE_PLUS writes here in the C ULD. ST's histogram preset leaves
    // the pad at 0x00; the pad is a property of the die, so it is set here and never cleared.
    private const int FmpPadConfig = 0x12;

    private const string DefaultMode = "medium";

    // One write clears the interrupt and lets the next frame run (measured over 40 frames on both dies).
    private static readonly Vl53lxClearStep[] HistClearSteps = { new(SystemInterruptClear, BareRegs.ClearRangeInt) };
    private static readonly string[] Groups = { "mode", "timing" };

    private string _mode = DefaultMode;
    private int _budgetMs = 33;
    private int _interMs;
    private IReadOnlyList<string> _modes = BareRegs.DistanceModes;
    private (int Min, int Max) _budgetRange = (2, 550);

    // The A/B alternation rides on the ll-driver state, which the polled C loop advances in the call that clears
    // the interrupt. Here the bridge clears it, so the state is advanced when a new frame arrives — told by the
    // device's own result__stream_count, so decoding the same sample twice does not step it twice.
    private int? _lastStreamCount;

    // The phase and event consistency checks read the frame before this one.
    private readonly FrameHistory _history = new();

    public Vl53lxHistogramDriver(Vl53lxPlatform platform, string product) : base(platform, product)
    {
        Bare = new BareDriver(platform);
    }

    /// <summary>The BareDriver underneath: device image, factory data, preset and post-processing config.</summary>
    public BareDriver Bare { get; }

    public override int AddrWidth => 2;
    public override IReadOnlyList<Vl53lxClearStep> ClearSteps => HistClearSteps;
    public override int MaxKhz => 1000; // with Fast Mode Plus on the sensor pad
    public override IReadOnlyCollection<string> Supports => Groups;

    /// <summary>short / medium / long, narrowed per die by <see cref="SensorInit"/> (an L4 has no short).</summary>
    public override IReadOnlyList<string> Modes => _modes;

    /// <summary>The guard comes off the budget before it is divided and the range timeout is capped: 2..550 ms,
    /// 2..200 on an L4 die.</summary>
    public override (int Min, int Max) BudgetMs => _budgetRange;

    public override bool Histogram => true;

    public override int ModelId() => P.ReadWord(0x010F);

    public void WaitBoot(TimeSpan? timeout = null)
    {
        var limit = timeout ?? TimeSpan.FromSeconds(1);
        var clock = Stopwatch.StartNew();
        while (P.ReadByte(FirmwareSystemStatus) != 0x03)
        {
            if (clock.Elapsed > limit)
                throw new Vl53lxException("timeout waiting for FIRMWARE__SYSTEM_STATUS == 0x03");
            P.SleepMs(1);
        }
    }

    /// <summary>Reset the part (whatever ran before left it ranging under another configuration), configure it
    /// for histogram ranging and raise the bus only after the FM+ pad byte has reached the device.</summary>
    public override void SensorInit()
    {
        P.SetAddrWidth(AddrWidth);
        P.SetI2cSpeed(I2cKhzBoot);
        P.XshutReset();
        WaitBoot();

        Bare.DataInit();
        // The die is known once NVM is read: an L4 has no short mode and a 200 ms budget ceiling.
        _modes = Bare.DistanceModesForDie();
        _budgetRange = (2, (int)(Bare.FdaMaxTimingBudgetUs() / 1000));
        SetMode(_mode);
        P.WriteByte(PadI2cHvConfig, FmpPadConfig);
        if (MaxKhz != I2cKhzBoot)
            P.SetI2cSpeed(MaxKhz);
    }

    /// <summary>The device preset mode: the VCSEL period, which sets where the phase wraps (1.6 / 2.4 / 4.0 m),
    /// and the valid phase window inside it (<see cref="ReachMm"/>). The bin is 199 mm wide in all three.</summary>
    public override void SetMode(string name)
    {
        if (!_modes.Contains(name))
            throw new ArgumentException($"no such mode: {name} (have {string.Join(", ", _modes)})", nameof(name));
        _mode = name;
        Bare.SetDistanceMode(name);
        // The preset rewrites the static config, pad byte included, and start_ranging() puts that image on the
        // wire: put FM+ back or that write would switch the pad off under a 1 MHz bus.
        Bare.Img.StaticConfig["pad_i2c_hv__config"] = FmpPadConfig;
        SetRangeTiming(_budgetMs, _interMs);
    }

    public override string GetMode() => _mode;

    /// <summary>The valid phase window the post-processing applies: 992 / 1785 / 3373 mm for short / medium /
    /// long at the usual oscillator. Past it the target is status 4.</summary>
    public override int? ReachMm() =>
        (int)Vl53l1Die.PhaseWindowMm(Bare.Img.StaticNvm["osc_measured__fast_osc__frequency"], Bare.Hpp.ValidPhaseHigh);

    /// <summary>How long one range actually runs, and the MM1/MM2 offset the post-processing applies.</summary>
    public override IReadOnlyDictionary<string, object?> DriverInfo() => new Dictionary<string, object?>
    {
        ["range timeout us"] = Bare.RangeConfigTimeoutUs,
        ["MM1/MM2 offset mm"] = Bare.Hpp.RangeOffsetMm / 4.0,
    };

    public override void StartRanging()
    {
        _lastStreamCount = null;
        _history.Reset();
        Bare.LoadPatch();
        Bare.InitAndStartRange();
        // The interrupt line comes out of reset asserted, so the first data-ready would read the untouched
        // register file as a frame; one clear deasserts it (ST's ClearInterruptAndStartMeasurement).
        Bare.ClearInterruptAndStartNextRange();
    }

    public override void StopRanging()
    {
        Bare.StopRange();
        Bare.UnloadPatch();
    }

    public override (int Addr, int Len) StreamBlock() => (BareRegs.HistogramBinDataI2cIndex, BareRegs.HistogramBinDataI2cSizeBytes);

    public override Vl53lxRange Decode(byte[] raw) => ToMeasurement(BinData(raw));

    public override Vl53lxRange ReadMeasurement() => ToMeasurement(BinData());

    /// <summary>The raw frame as a <see cref="HistogramBinData"/>; read here when <paramref name="raw"/> is null.</summary>
    public override object BinData(byte[]? raw = null)
    {
        if (raw is null)
        {
            var (addr, len) = StreamBlock();
            raw = P.ReadMulti(addr, len);
        }
        int streamCount = raw[3]; // result__stream_count
        if (streamCount != _lastStreamCount)
        {
            // What clear_interrupt_and_start_next_range() does to the state machine, without the register writes:
            // one frame on, which flips the A/B timing status and steps the bin sequence.
            if (_lastStreamCount is not null)
                Bare.State.UpdateRd(Bare.Img.SystemControl["system__mode_start"], Bare.Img.DynamicConfig["system__grouped_parameter_hold"]);
            _lastStreamCount = streamCount;
        }
        return Bare.GetHistogramBinData(raw);
    }

    /// <summary>A frame → its range. Once per frame: the history is a state machine and would step twice.</summary>
    public override Vl53lxRange ToMeasurement(object bins)
    {
        var b = (HistogramBinData)bins;
        var results = Hist.ProcessData(b, Bare.Hpp);
        _history.Apply(results, b, Bare.State.RdDeviceState, Bare.Hpp);
        return Hist.ToRange(results, b);
    }

    public override bool CheckForDataReady()
    {
        int intPol = ((P.ReadByte(GpioHvMuxCtrl) & 0x10) >> 4) == 1 ? 0 : 1;
        return (P.ReadByte(GpioTioHvStatus) & 1) == intPol;
    }

    public override void ClearInterrupt() => P.WriteByte(SystemInterruptClear, BareRegs.ClearRangeInt);

    /// <summary>The budget covers the whole measurement; the range timeout is one sixth of it after a fixed guard.</summary>
    public override void SetRangeTiming(int timingBudgetMs, int interMeasurementMs)
    {
        var (low, high) = _budgetRange;
        if (timingBudgetMs < low || timingBudgetMs > high)
            throw new Vl53lxException($"timing_budget_ms must be {low}..{high}");
        _budgetMs = timingBudgetMs;
        _interMs = interMeasurementMs;
        Bare.SetMeasurementTimingBudgetUs(timingBudgetMs * 1000L);
        // 0 means continuous; the device wants a period, and back-to-back is a period of zero.
        Bare.SetInterMeasurementPeriodMs(interMeasurementMs);
    }

    public override (int TimingBudgetMs, int InterMeasurementMs) GetRangeTiming()
    {
        long budgetUs = Bare.RangeConfigTimeoutUs * BareRegs.TimingDivisor + BareRegs.TimingGuardUs;
        return ((int)(budgetUs / 1000), (int)Bare.InterMeasurementPeriodMs);
    }
}
