using System.Buffers.Binary;
using System.Diagnostics;
using Depz.Sensor.Vl53l4;

namespace Depz.Sensor.Vl53lx;

/// <summary>
/// The VL53L1 die (uld/vl53l1_die.py): register map, the 17-byte result block every part of it shares,
/// and the body of the VL53L4CD ULD that <see cref="Vl53l4UldDriver"/> and <see cref="Vl53l3UlpDriver"/>
/// fill in; <see cref="Vl53l1UldDriver"/> (ST's VL53L1X ULD) overrides where that driver differs.
/// A faithful port: register sequences and integer widths as in the C drivers.
/// </summary>
public abstract class Vl53l1Die : Vl53lxDriver
{
    // The register map, spelled as the full VL53L1 map spells it.
    public const int SoftReset = 0x0000;
    public const int I2cSlaveDeviceAddress = 0x0001;
    public const int OscFrequency = 0x0006;
    public const int VhvConfigTimeoutMacropLoopBound = 0x0008;
    public const int XtalkPlaneOffsetKcps = 0x0016;
    public const int XtalkXPlaneGradientKcps = 0x0018;
    public const int XtalkYPlaneGradientKcps = 0x001A;
    public const int RangeOffsetMm = 0x001E;
    public const int InnerOffsetMm = 0x0020;
    public const int OuterOffsetMm = 0x0022;
    public const int GpioHvMuxCtrl = 0x0030;
    public const int GpioTioHvStatus = 0x0031;
    public const int SystemInterruptConfigGpio = 0x0046;
    public const int PhasecalConfigTimeoutMacrop = 0x004B;
    public const int RangeConfigTimeoutMacropAHi = 0x005E;
    public const int RangeConfigVcselPeriodA = 0x0060;
    public const int RangeConfigTimeoutMacropBHi = 0x0061;
    public const int RangeConfigVcselPeriodB = 0x0063;
    public const int RangeConfigSigmaThresh = 0x0064;
    public const int MinCountRateRtnLimitMcps = 0x0066;
    public const int RangeConfigValidPhaseHigh = 0x0069;
    public const int IntermeasurementMs = 0x006C;
    public const int ThreshHigh = 0x0072;
    public const int ThreshLow = 0x0074;
    public const int SdConfigWoiSd0 = 0x0078;
    public const int SdConfigInitialPhaseSd0 = 0x007A;
    public const int RoiConfigUserRoiCentreSpad = 0x007F;
    public const int RoiConfigUserRoiXySize = 0x0080;
    public const int SystemInterruptClear = 0x0086;
    public const int SystemModeStart = 0x0087;
    public const int ResultRangeStatus = 0x0089;
    public const int ResultOscCalibrateVal = 0x00DE;
    public const int FirmwareSystemStatus = 0x00E5;
    public const int IdentificationModelId = 0x010F;
    public const int RoiConfigModeRoiCentreSpad = 0x013E;

    public const int WindowBelow = 0, WindowAbove = 1, WindowOut = 2, WindowIn = 3;

    /// <summary>The configuration blob spans 0x2D..0x87 on every part; byte 0 is always the Fast Mode Plus byte.</summary>
    public const int ConfigAddr = 0x002D;
    public const int ConfigEnd = 0x0087;
    public const byte ConfigFmpByte = 0x12;

    public const int ResultBlockAddr = ResultRangeStatus;
    public const int ResultBlockLen = 17;

    private const int I2cKhzBoot = 400;

    // GetResult() raw status → ULD status (status_rtn[24]).
    private static readonly int[] StatusRtn =
    {
        255, 255, 255, 5, 2, 4, 1, 7, 3, 0, 255, 255, 9, 13, 255, 255, 255, 255, 10, 6, 255, 255, 11, 12,
    };

    private static readonly Vl53lxClearStep[] DieClearSteps = { new(SystemInterruptClear, 0x01) };

    protected Vl53l1Die(Vl53lxPlatform platform, string product) : base(platform, product) { }

    public override int AddrWidth => 2;
    public override IReadOnlyList<Vl53lxClearStep> ClearSteps => DieClearSteps;
    public override int MaxKhz => 1000; // with Fast Mode Plus on the sensor pad

    /// <summary>The 91-byte blob written at <see cref="ConfigAddr"/>.</summary>
    protected abstract byte[] Configuration { get; }

    /// <summary>What StopRanging writes: the L4CD ULD 0x80, the L3CX ULP and the L1X ULD 0x00.</summary>
    protected virtual int StopMode => 0x80;

    /// <summary>Where the signal rate comes from: 0x008E (offset 5) on the L4CD ULD, 0x0098 (15) on the L1X ULD.</summary>
    protected virtual int SignalAt => 5;

    /// <summary>Per-SPAD scale: 256 on the L4CD ULD, 25 on the L1X ULD (ST's own difference, kept).</summary>
    protected virtual int PerSpadK => 256;

    /// <summary>How far a target can be before the die fails it with status 4, mm.</summary>
    public static long PhaseWindowMm(long fastOscFrequency, long validPhaseHigh)
    {
        long pllPeriod = (1L << 30) / fastOscFrequency;
        return validPhaseHigh * pllPeriod * (299704 >> 3) >> 25;
    }

    // ── result block ─────────────────────────────────────────────────────────

    public override (int Addr, int Len) StreamBlock() => (ResultBlockAddr, ResultBlockLen);

    public override Vl53lxRange Decode(byte[] raw)
    {
        if (raw.Length < ResultBlockLen)
            throw new Vl53lxException($"result block too short: {raw.Length} bytes");
        int status = raw[0] & 0x1F;
        if (status < StatusRtn.Length)
            status = StatusRtn[status];
        int rawSpads = BinaryPrimitives.ReadUInt16BigEndian(raw.AsSpan(3));      // 0x008C, 8.8
        int signalKcps = BinaryPrimitives.ReadUInt16BigEndian(raw.AsSpan(SignalAt)) * 8;
        int ambientKcps = BinaryPrimitives.ReadUInt16BigEndian(raw.AsSpan(7)) * 8; // 0x0090
        int k = PerSpadK;
        var extra = new Dictionary<string, object?>
        {
            ["signal_per_spad_kcps"] = rawSpads != 0 ? (long)signalKcps * k / rawSpads : 0,
            ["ambient_per_spad_kcps"] = rawSpads != 0 ? (long)ambientKcps * k / rawSpads : 0,
            ["stream_count"] = (long)raw[2],
        };
        return new Vl53lxRange(
            BinaryPrimitives.ReadUInt16BigEndian(raw.AsSpan(13)),                // 0x0096
            status, StatusText(status), signalKcps, ambientKcps,
            BinaryPrimitives.ReadUInt16BigEndian(raw.AsSpan(9)) / 4,             // 0x0092, 14.2
            rawSpads / 256, Array.Empty<Vl53lxTarget>(), extra);
    }

    public static string StatusText(int status) =>
        Vl53l4Uld.RangeStatusNames.TryGetValue(status, out var s) ? s : $"unknown ({status})";

    /// <summary>One block read instead of the C driver's six register reads; the decoding is identical.</summary>
    public Vl53lxRange GetResult() => Decode(P.ReadMulti(ResultBlockAddr, ResultBlockLen));

    public override Vl53lxRange ReadMeasurement() => GetResult();

    // ── identity and init ────────────────────────────────────────────────────

    public override int ModelId() => P.ReadWord(IdentificationModelId);

    public int BootState() => P.ReadByte(FirmwareSystemStatus);

    public void WaitBoot(TimeSpan? timeout = null)
    {
        var limit = timeout ?? TimeSpan.FromSeconds(1);
        var clock = Stopwatch.StartNew();
        while (true)
        {
            if (BootState() == 0x03)
                return;
            if (clock.Elapsed > limit)
                throw new Vl53lxException("timeout waiting for FIRMWARE__SYSTEM_STATUS == 0x03");
            P.SleepMs(1);
        }
    }

    /// <summary>The die's soft reset; a part that no longer acknowledges writes is reset through XSHUT.</summary>
    public void ResetDevice(TimeSpan? timeout = null)
    {
        try
        {
            P.WriteByte(SoftReset, 0x00);
            P.SleepMs(1);
            P.WriteByte(SoftReset, 0x01);
            WaitBoot(timeout);
        }
        catch (Exception e) when (e is Vl53lxException || e is Vl53lxProtocolException)
        {
            P.XshutReset();
            WaitBoot(timeout);
        }
    }

    /// <summary>Configuration at 400 kHz (the only speed an unconfigured sensor is specified for), Fast Mode
    /// Plus byte first, then the bus at <see cref="MaxKhz"/>, VHV, and 50 ms continuous.</summary>
    public override void SensorInit()
    {
        P.SetAddrWidth(AddrWidth);
        P.SetI2cSpeed(I2cKhzBoot);
        InitBoot();
        byte[] config = Configuration.ToArray();
        config[0] = ConfigFmpByte;
        // The C driver writes the 91 bytes one at a time; auto-increment makes it one write.
        P.WriteMulti(ConfigAddr, config);
        if (MaxKhz != I2cKhzBoot)
            P.SetI2cSpeed(MaxKhz);
        InitAfterConfig();

        P.WriteByte(SystemModeStart, 0x40); // start VHV
        WaitDataReady(TimeSpan.FromSeconds(1));
        ClearInterrupt();
        StopRanging();
        P.WriteByte(VhvConfigTimeoutMacropLoopBound, 0x09);
        P.WriteByte(0x000B, 0x00);
        InitExtra();

        SetRangeTiming(50, 0);
    }

    /// <summary>How a part gets to a booted die (the L1X ULD resets first).</summary>
    protected virtual void InitBoot() => WaitBoot();

    /// <summary>Writes right after the configuration blob, before VHV.</summary>
    protected virtual void InitAfterConfig() { }

    /// <summary>Writes at the end of init, before the timing is set.</summary>
    protected virtual void InitExtra() { }

    // ── ranging and the interrupt ────────────────────────────────────────────

    public override void ClearInterrupt() => P.WriteByte(SystemInterruptClear, 0x01);

    public override void StartRanging()
    {
        // 0 = continuous, anything else = autonomous low power.
        int mode = P.ReadDword(IntermeasurementMs) == 0 ? 0x21 : 0x40;
        P.WriteByte(SystemModeStart, mode);
    }

    public override void StopRanging() => P.WriteByte(SystemModeStart, StopMode);

    public int GetInterruptPolarity() => (P.ReadByte(GpioHvMuxCtrl) & 0x10) != 0 ? 0 : 1;

    public void SetInterruptPolarity(int polarity)
    {
        int temp = P.ReadByte(GpioHvMuxCtrl) & 0xEF;
        P.WriteByte(GpioHvMuxCtrl, temp | ((polarity & 1) == 0 ? 1 : 0) << 4);
    }

    public override bool CheckForDataReady()
    {
        int intPol = GetInterruptPolarity(); // polarity first, as both C drivers read it
        return (P.ReadByte(GpioTioHvStatus) & 1) == intPol;
    }

    // ── timing ───────────────────────────────────────────────────────────────

    private static long MacroPeriodUs(long osc) => ((2304 * (0x40000000L / osc)) & 0xFFFFFFFFL) >> 6;

    public override void SetRangeTiming(int timingBudgetMs, int interMeasurementMs)
    {
        int osc = P.ReadWord(OscFrequency);
        if (osc == 0)
            throw new Vl53lxException("osc_frequency reads 0");
        if (timingBudgetMs < 10 || timingBudgetMs > 200)
            throw new Vl53lxException("timing_budget_ms must be 10..200");

        long budgetUs = timingBudgetMs * 1000L;
        long macroPeriodUs = MacroPeriodUs(osc);
        if (interMeasurementMs == 0) // continuous
        {
            P.WriteDword(IntermeasurementMs, 0);
            budgetUs -= 2500;
        }
        else if (interMeasurementMs > timingBudgetMs) // autonomous low power
        {
            int clockPll = P.ReadWord(ResultOscCalibrateVal) & 0x3FF;
            double factor = 1.055 * interMeasurementMs * clockPll;
            P.WriteDword(IntermeasurementMs, (long)factor);
            budgetUs = (budgetUs - 4300) / 2;
        }
        else
        {
            throw new Vl53lxException("inter_measurement_ms must be 0 or > timing_budget_ms");
        }

        budgetUs = (budgetUs << 12) & 0xFFFFFFFFL;
        foreach (var (reg, mult) in new[] { (RangeConfigTimeoutMacropAHi, 16), (RangeConfigTimeoutMacropBHi, 12) })
        {
            long tmp = ((macroPeriodUs * mult) & 0xFFFFFFFFL) >> 6;
            long ls = (budgetUs + (tmp >> 1)) / tmp - 1;
            int ms = 0;
            while ((ls & 0xFFFFFF00L) != 0)
            {
                ls >>= 1;
                ms++;
            }
            P.WriteWord(reg, (int)(((ms << 8) + (ls & 0xFF)) & 0xFFFF));
        }
    }

    public override (int TimingBudgetMs, int InterMeasurementMs) GetRangeTiming()
    {
        long tmp = P.ReadDword(IntermeasurementMs);
        int clockPll = P.ReadWord(ResultOscCalibrateVal) & 0x3FF;
        clockPll = (int)(1.065 * clockPll) & 0xFFFF;
        int interMs = clockPll != 0 ? (int)(tmp / clockPll & 0xFFFF) : 0;

        int osc = P.ReadWord(OscFrequency);
        if (osc == 0)
            throw new Vl53lxException("osc_frequency reads 0");
        int macropHigh = P.ReadWord(RangeConfigTimeoutMacropAHi);

        long macroPeriodUs = MacroPeriodUs(osc);
        long ls = (macropHigh & 0x00FF) << 4;
        long ms = (macropHigh & 0xFF00) >> 8;
        ms = (0x04 - (ms - 1) - 1) & 0xFFFFFFFFL;
        macroPeriodUs = (macroPeriodUs * 16) & 0xFFFFFFFFL;
        long budget = ((((ls + 1) * (macroPeriodUs >> 6)) - ((macroPeriodUs >> 6) >> 1)) & 0xFFFFFFFFL) >> 12;
        if (ms < 12)
            budget >>= (int)ms;
        budget = tmp == 0 ? budget + 2500 : budget * 2 + 4300;
        return ((int)(budget / 1000), interMs);
    }

    /// <summary>Read off the loaded configuration: 1388 mm for the 0x38 window, 4563 mm for 0xB8.</summary>
    public override int? ReachMm()
    {
        int osc = P.ReadWord(OscFrequency);
        if (osc == 0)
            throw new Vl53lxException("osc_frequency reads 0");
        return (int)PhaseWindowMm(osc, P.ReadByte(RangeConfigValidPhaseHigh));
    }

    // ── thresholds, offset, xtalk, ROI, temperature ──────────────────────────

    public override void SetDetectionThresholds(int distanceLowMm, int distanceHighMm, int window)
    {
        P.WriteByte(SystemInterruptConfigGpio, window);
        P.WriteWord(ThreshHigh, distanceHighMm);
        P.WriteWord(ThreshLow, distanceLowMm);
    }

    public override (int DistanceLowMm, int DistanceHighMm, int Window) GetDetectionThresholds()
    {
        int high = P.ReadWord(ThreshHigh);
        int low = P.ReadWord(ThreshLow);
        return (low, high, P.ReadByte(SystemInterruptConfigGpio) & 0x07);
    }

    public override void SetSignalThreshold(int signalKcps) => P.WriteWord(MinCountRateRtnLimitMcps, signalKcps >> 3);

    public override int GetSignalThreshold() => (P.ReadWord(MinCountRateRtnLimitMcps) << 3) & 0xFFFF;

    public override void SetSigmaThreshold(int sigmaMm)
    {
        if (sigmaMm > 0xFFFF >> 2)
            throw new Vl53lxException("sigma_mm must be <= 16383");
        P.WriteWord(RangeConfigSigmaThresh, sigmaMm << 2);
    }

    public override int GetSigmaThreshold() => P.ReadWord(RangeConfigSigmaThresh) >> 2;

    public override void SetOffset(int offsetMm)
    {
        P.WriteWord(RangeOffsetMm, (offsetMm * 4) & 0xFFFF);
        P.WriteWord(InnerOffsetMm, 0);
        P.WriteWord(OuterOffsetMm, 0);
    }

    public override int GetOffset()
    {
        int temp = ((P.ReadWord(RangeOffsetMm) << 3) & 0xFFFF) >> 5;
        return temp > 1024 ? temp - 2048 : temp;
    }

    public override void SetXtalk(int xtalkKcps)
    {
        P.WriteWord(XtalkXPlaneGradientKcps, 0x0000);
        P.WriteWord(XtalkYPlaneGradientKcps, 0x0000);
        P.WriteWord(XtalkPlaneOffsetKcps, (xtalkKcps << 9) & 0xFFFF);
    }

    public override int GetXtalk() => (int)Math.Round(P.ReadWord(XtalkPlaneOffsetKcps) / 512.0, MidpointRounding.AwayFromZero);

    /// <summary>An X by Y window of SPADs, 4..16 each way.</summary>
    public override void SetRoi(int x, int y)
    {
        int opticalCenter = P.ReadByte(RoiConfigModeRoiCentreSpad);
        x = Math.Min(x, 16);
        y = Math.Min(y, 16);
        if (x > 10 || y > 10)
            opticalCenter = 199;
        P.WriteByte(RoiConfigUserRoiCentreSpad, opticalCenter);
        P.WriteByte(RoiConfigUserRoiXySize, (y - 1) << 4 | (x - 1));
    }

    public override (int X, int Y) GetRoi()
    {
        int temp = P.ReadByte(RoiConfigUserRoiXySize);
        return ((temp & 0x0F) + 1, ((temp & 0xF0) >> 4) + 1);
    }

    public override void SetRoiCenter(int centerSpad) => P.WriteByte(RoiConfigUserRoiCentreSpad, centerSpad);

    public override int GetRoiCenter() => P.ReadByte(RoiConfigUserRoiCentreSpad);

    /// <summary>Recommended after a &gt;8 °C ambient change.</summary>
    public override void StartTemperatureUpdate()
    {
        P.WriteByte(VhvConfigTimeoutMacropLoopBound, 0x81);
        P.WriteByte(0x000B, 0x92);
        StartRanging();
        WaitDataReady(TimeSpan.FromSeconds(1));
        ClearInterrupt();
        StopRanging();
        P.WriteByte(VhvConfigTimeoutMacropLoopBound, 0x09);
        P.WriteByte(0x000B, 0x00);
    }

    // The ranging loop every calibration shares: data ready, result, clear, n times.
    protected void Collect(int nbSamples, Action<int, Vl53lxRange> onSample)
    {
        StartRanging();
        for (int i = 0; i < nbSamples; i++)
        {
            WaitDataReady(TimeSpan.FromSeconds(5));
            var r = GetResult();
            ClearInterrupt();
            onSample(i, r);
        }
        StopRanging();
    }

    protected static long FloorDiv(long a, long b) => a / b - ((a % b != 0) && ((a < 0) != (b < 0)) ? 1 : 0);
}

/// <summary>VL53L4CD ULD 2.2.3 (VL53L4CD_api.c + VL53L4CD_calibration.c) for VL53L4CD and VL53L4CX alike.</summary>
public sealed class Vl53l4UldDriver : Vl53l1Die
{
    private static readonly byte[] DefaultConfiguration =
    {
        0x00, 0x00, 0x00, 0x11, 0x02, 0x00, 0x02, 0x08, 0x00, 0x08, 0x10, 0x01, 0x01, 0x00, 0x00, 0x00,
        0x00, 0xff, 0x00, 0x0F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x0b, 0x00, 0x00, 0x02, 0x14, 0x21,
        0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x00, 0xc8, 0x00, 0x00, 0x38, 0xff, 0x01, 0x00, 0x08, 0x00,
        0x00, 0x01, 0xcc, 0x07, 0x01, 0xf1, 0x05, 0x00, 0xa0, 0x00, 0x80, 0x08, 0x38, 0x00, 0x00, 0x00,
        0x00, 0x0f, 0x89, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x07, 0x05, 0x06, 0x06, 0x00,
        0x00, 0x02, 0xc7, 0xff, 0x9B, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00,
    };

    private static readonly string[] Groups =
        { "timing", "offset", "xtalk", "thresholds", "signal_thresh", "sigma_thresh", "temp_update", "calib_offset", "calib_xtalk" };

    public Vl53l4UldDriver(Vl53lxPlatform platform, string product) : base(platform, product) { }

    protected override byte[] Configuration => DefaultConfiguration;

    public override IReadOnlyCollection<string> Supports => Groups;

    // ALGO__RANGE_IGNORE_THRESHOLD_MCPS, the one write VL53L4CD_SensorInit() adds at the end.
    protected override void InitExtra() => P.WriteWord(0x0024, 0x0500);

    public override int CalibrateOffset(int targetDistMm, int? nbSamples = null)
    {
        int n = nbSamples ?? 20;
        if (n < 5 || n > 255 || targetDistMm < 10 || targetDistMm > 1000)
            throw new Vl53lxException("nb_samples must be 5..255, target 10..1000 mm");
        P.WriteWord(RangeOffsetMm, 0);
        P.WriteWord(InnerOffsetMm, 0);
        P.WriteWord(OuterOffsetMm, 0);
        Collect(10, (_, _) => { }); // device heat loop
        long sum = 0;
        Collect(n, (_, r) => sum += r.DistanceMm);
        int offsetMm = targetDistMm - (int)FloorDiv(sum, n);
        P.WriteWord(RangeOffsetMm, (offsetMm * 4) & 0xFFFF);
        return offsetMm;
    }

    public override int CalibrateXtalk(int targetDistMm, int? nbSamples = null)
    {
        int n = nbSamples ?? 20;
        if (n < 5 || n > 255 || targetDistMm < 10 || targetDistMm > 5000)
            throw new Vl53lxException("nb_samples must be 5..255, target 10..5000 mm");
        P.WriteWord(XtalkPlaneOffsetKcps, 0); // compensation off
        Collect(10, (_, _) => { });
        var samples = new List<Vl53lxRange>();
        Collect(n, (i, r) =>
        {
            if (r.Status == 0 && i > 0) // invalid measurements and the first frame are dropped
                samples.Add(r);
        });
        if (samples.Count == 0)
            throw new Vl53lxException("xtalk calibration failed: no valid samples");
        double count = samples.Count;
        double avgDistance = samples.Sum(s => (double)s.DistanceMm) / count;
        double avgSpadNb = samples.Sum(s => s.Spads) / count;
        double avgSignal = samples.Sum(s => s.SignalKcps) / count;
        double tmp = (1.0 - avgDistance / targetDistMm) * (avgSignal / avgSpadNb);
        if (tmp > 127) // 65536 / 512
            throw new Vl53lxException($"xtalk calibration failed: {tmp:0.0} kcps > 127");
        P.WriteWord(XtalkPlaneOffsetKcps, (int)(tmp * 512.0) & 0xFFFF);
        return (int)Math.Round(tmp, MidpointRounding.AwayFromZero);
    }
}

/// <summary>VL53L3CX ULP 1.0.0: plain single-target ranging on the histogram part.</summary>
public sealed class Vl53l3UlpDriver : Vl53l1Die
{
    private static readonly byte[] DefaultConfiguration =
    {
        0x00, 0x00, 0x00, 0x11, 0x02, 0x00, 0x02, 0x08, 0x00, 0x08, 0x10, 0x01, 0x01, 0x00, 0x00, 0x00,
        0x00, 0xff, 0x00, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x0b, 0x00, 0x00, 0x02, 0x14, 0x21,
        0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x00, 0xc8, 0x00, 0x00, 0x38, 0xff, 0x01, 0x00, 0x08, 0x00,
        0x00, 0x00, 0x01, 0x07, 0x00, 0x02, 0x05, 0x00, 0xb4, 0x00, 0xbb, 0x08, 0x38, 0x00, 0x00, 0x00,
        0x00, 0x0f, 0x89, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x07, 0x05, 0x06, 0x06, 0x00,
        0x00, 0x02, 0xc7, 0xff, 0x9b, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00,
    };

    private static readonly string[] Groups = { "timing", "thresholds", "signal_thresh", "sigma_thresh", "roi" };

    public Vl53l3UlpDriver(Vl53lxPlatform platform, string product) : base(platform, product) { }

    protected override byte[] Configuration => DefaultConfiguration;

    protected override int StopMode => 0x00;

    public override IReadOnlyCollection<string> Supports => Groups;

    protected override void InitExtra()
    {
        P.WriteWord(0x0024, 0x0500);
        P.WriteByte(0x0081, 0x8A); // ULP only
        P.WriteByte(PhasecalConfigTimeoutMacrop, 0x03);
    }
}

/// <summary>VL53L1X ULD 3.5.5 (VL53L1X_api.c + VL53L1X_calibration.c) for VL53L1CX and VL53L1CB: a tabulated
/// timing budget, short / long distance modes, an init that resets the die first. Crosstalk is in kcps,
/// the register's own unit (ST's wrapper converts to cps).</summary>
public sealed class Vl53l1UldDriver : Vl53l1Die
{
    private const int DistanceShort = 1, DistanceLong = 2;

    private static readonly byte[] DefaultConfiguration =
    {
        0x00, 0x00, 0x00, 0x01, 0x02, 0x00, 0x02, 0x08, 0x00, 0x08, 0x10, 0x01, 0x01, 0x00, 0x00, 0x00,
        0x00, 0xff, 0x00, 0x0F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x0b, 0x00, 0x00, 0x02, 0x0a, 0x21,
        0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x00, 0xc8, 0x00, 0x00, 0x38, 0xff, 0x01, 0x00, 0x08, 0x00,
        0x00, 0x01, 0xcc, 0x0f, 0x01, 0xf1, 0x0d, 0x01, 0x68, 0x00, 0x80, 0x08, 0xb8, 0x00, 0x00, 0x00,
        0x00, 0x0f, 0x89, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x0f, 0x0d, 0x0e, 0x0e, 0x00,
        0x00, 0x02, 0xc7, 0xff, 0x9B, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00,
    };

    // VL53L1X_SetTimingBudgetInMs(): {mode: {budget ms: (MACROP_A_HI, MACROP_B_HI)}}; 15 ms in short only.
    private static readonly Dictionary<int, SortedDictionary<int, (int A, int B)>> TimingBudgets = new()
    {
        [DistanceShort] = new()
        {
            [15] = (0x001D, 0x0027), [20] = (0x0051, 0x006E), [33] = (0x00D6, 0x006E), [50] = (0x01AE, 0x01E8),
            [100] = (0x02E1, 0x0388), [200] = (0x03E1, 0x0496), [500] = (0x0591, 0x05C1),
        },
        [DistanceLong] = new()
        {
            [20] = (0x001E, 0x0022), [33] = (0x0060, 0x006E), [50] = (0x00AD, 0x00C6), [100] = (0x01CC, 0x01EA),
            [200] = (0x02D9, 0x02F8), [500] = (0x048F, 0x04A4),
        },
    };

    private static readonly string[] ModeNames = { "long", "short" }; // the blob boots in long

    private static readonly string[] Groups =
    {
        "mode", "timing", "offset", "xtalk", "thresholds", "signal_thresh", "sigma_thresh", "temp_update",
        "calib_offset", "calib_xtalk", "roi",
    };

    // The mode last applied: BudgetChoices answers from it (PHASECAL reads 0 before init).
    private string _appliedMode = "long";

    public Vl53l1UldDriver(Vl53lxPlatform platform, string product) : base(platform, product) { }

    protected override byte[] Configuration => DefaultConfiguration;
    protected override int StopMode => 0x00;
    protected override int SignalAt => 15;
    protected override int PerSpadK => 25;
    public override IReadOnlyCollection<string> Supports => Groups;
    public override IReadOnlyList<string> Modes => ModeNames;
    public override (int Min, int Max) BudgetMs => (15, 500);

    private static int Code(string name) => name switch
    {
        "short" => DistanceShort,
        "long" => DistanceLong,
        _ => 0,
    };

    // No plain wait: a sensor left hung by a previous session answers reads with zeros.
    protected override void InitBoot() => ResetDevice();

    // ST ships this blob with an active-HIGH interrupt; the bridge watches the falling edge.
    protected override void InitAfterConfig() => SetInterruptPolarity(0);

    protected override void InitExtra() => _appliedMode = ModeNames[0];

    /// <summary>Timed mode only: this part has no free-running mode under the ULD configuration.</summary>
    public override void StartRanging() => P.WriteByte(SystemModeStart, 0x40);

    /// <summary>"short" or "long"; the budget is re-applied (the VCSEL periods change what it means).</summary>
    public override void SetMode(string name)
    {
        int mode = Code(name);
        if (mode == 0)
            throw new ArgumentException($"no such mode: {name} (have {string.Join(", ", ModeNames)})", nameof(name));
        var (budgetMs, interMs) = GetRangeTiming();
        _appliedMode = name;
        if (mode == DistanceShort)
        {
            P.WriteByte(PhasecalConfigTimeoutMacrop, 0x14);
            P.WriteByte(RangeConfigVcselPeriodA, 0x07);
            P.WriteByte(RangeConfigVcselPeriodB, 0x05);
            P.WriteByte(RangeConfigValidPhaseHigh, 0x38);
            P.WriteWord(SdConfigWoiSd0, 0x0705);
            P.WriteWord(SdConfigInitialPhaseSd0, 0x0606);
        }
        else
        {
            P.WriteByte(PhasecalConfigTimeoutMacrop, 0x0A);
            P.WriteByte(RangeConfigVcselPeriodA, 0x0F);
            P.WriteByte(RangeConfigVcselPeriodB, 0x0D);
            P.WriteByte(RangeConfigValidPhaseHigh, 0xB8);
            P.WriteWord(SdConfigWoiSd0, 0x0F0D);
            P.WriteWord(SdConfigInitialPhaseSd0, 0x0E0E);
        }
        var table = TimingBudgets[mode];
        if (!table.ContainsKey(budgetMs)) // 15 ms exists in short mode only
            budgetMs = table.Keys.OrderBy(ms => Math.Abs(ms - budgetMs)).First();
        SetRangeTiming(budgetMs, interMs);
    }

    public override string GetMode() => ModeCode() == DistanceShort ? "short" : "long";

    private int ModeCode()
    {
        int temp = P.ReadByte(PhasecalConfigTimeoutMacrop);
        if (temp == 0x14)
            return DistanceShort;
        if (temp == 0x0A)
            return DistanceLong;
        throw new Vl53lxException($"PHASECAL_CONFIG__TIMEOUT_MACROP reads 0x{temp:X2}, which is neither distance mode");
    }

    /// <summary>Only the tabulated budgets of the mode exist; a period of 0 is written as the budget itself
    /// (this part has no free-running mode).</summary>
    public override void SetRangeTiming(int timingBudgetMs, int interMeasurementMs)
    {
        int mode = ModeCode();
        var table = TimingBudgets[mode];
        if (!table.TryGetValue(timingBudgetMs, out var macrop))
            throw new Vl53lxException(
                $"timing budget {timingBudgetMs} ms is not one the {(mode == DistanceShort ? "short" : "long")} mode has - pick one of {string.Join(", ", table.Keys)} ms");
        P.WriteWord(RangeConfigTimeoutMacropAHi, macrop.A);
        P.WriteWord(RangeConfigTimeoutMacropBHi, macrop.B);
        if (interMeasurementMs == 0)
            interMeasurementMs = timingBudgetMs;
        int clockPll = P.ReadWord(ResultOscCalibrateVal) & 0x3FF;
        P.WriteDword(IntermeasurementMs, (long)((double)(clockPll * interMeasurementMs) * 1.075));
    }

    public override IReadOnlyList<int> BudgetChoices() => TimingBudgets[Code(_appliedMode)].Keys.ToList();

    public override (int TimingBudgetMs, int InterMeasurementMs) GetRangeTiming()
    {
        int macropA = P.ReadWord(RangeConfigTimeoutMacropAHi);
        int budgetMs = 0;
        foreach (var t in TimingBudgets.Values)
            foreach (var kv in t)
                if (kv.Value.A == macropA)
                    budgetMs = kv.Key;
        long tmp = P.ReadDword(IntermeasurementMs);
        int clockPll = P.ReadWord(ResultOscCalibrateVal) & 0x3FF;
        int interMs = clockPll != 0 ? (int)(tmp / (clockPll * 1.065)) : 0;
        return (budgetMs, interMs);
    }

    /// <summary>VL53L1X_SetDistanceThreshold(): a read-modify-write keeping the bits outside 0x6F.</summary>
    public override void SetDetectionThresholds(int distanceLowMm, int distanceHighMm, int window) =>
        SetDetectionThresholds(distanceLowMm, distanceHighMm, window, intOnNoTarget: false);

    public void SetDetectionThresholds(int distanceLowMm, int distanceHighMm, int window, bool intOnNoTarget)
    {
        int temp = P.ReadByte(SystemInterruptConfigGpio) & ~0x6F & 0xFF;
        temp |= window;
        if (intOnNoTarget)
            temp |= 0x40;
        P.WriteByte(SystemInterruptConfigGpio, temp);
        P.WriteWord(ThreshHigh, distanceHighMm);
        P.WriteWord(ThreshLow, distanceLowMm);
    }

    public override int CalibrateOffset(int targetDistMm, int? nbSamples = null)
    {
        int n = nbSamples ?? 50;
        if (n < 5)
            throw new Vl53lxException("nb_samples must be at least 5");
        P.WriteWord(RangeOffsetMm, 0);
        P.WriteWord(InnerOffsetMm, 0);
        P.WriteWord(OuterOffsetMm, 0);
        long sum = 0;
        Collect(n, (_, r) => sum += r.DistanceMm);
        int offsetMm = targetDistMm - (int)FloorDiv(sum, n);
        P.WriteWord(RangeOffsetMm, (offsetMm * 4) & 0xFFFF);
        return offsetMm;
    }

    /// <summary>VL53L1X_CalibrateXtalk(); returns kcps, not the C driver's cps.</summary>
    public override int CalibrateXtalk(int targetDistMm, int? nbSamples = null)
    {
        int n = nbSamples ?? 50;
        if (n < 5)
            throw new Vl53lxException("nb_samples must be at least 5");
        P.WriteWord(XtalkPlaneOffsetKcps, 0);
        var samples = new List<Vl53lxRange>();
        Collect(n, (_, r) => samples.Add(r));
        double count = samples.Count;
        double avgDistance = samples.Sum(s => (double)s.DistanceMm) / count;
        double avgSpadNb = samples.Sum(s => s.Spads) / count;
        double avgSignal = samples.Sum(s => s.SignalKcps) / count;
        if (avgSpadNb == 0)
            throw new Vl53lxException("xtalk calibration failed: no SPADs enabled");
        long cal = (long)(512 * (avgSignal * (1 - avgDistance / targetDistMm)) / avgSpadNb);
        cal = Math.Min(Math.Max(cal, 0), 0xFFFF);
        P.WriteWord(XtalkPlaneOffsetKcps, (int)cal);
        return (int)Math.Round(cal / 512.0, MidpointRounding.AwayFromZero);
    }
}
