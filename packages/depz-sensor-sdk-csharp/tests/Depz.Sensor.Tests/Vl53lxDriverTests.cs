using Depz.Sensor.Vl53l4;
using Depz.Sensor.Vl53lx;
using Xunit;

namespace Depz.Sensor.Tests;

/// <summary>A big-endian register map with a scripted result block; counts the bus traffic. Writes that
/// hit <see cref="NackAt"/> are refused the way a NACKing die surfaces through the bridge.</summary>
internal sealed class FakeVl53lxRegMap : Vl53lxPlatform
{
    public readonly byte[] Regs = new byte[0x10000];
    public readonly List<(int Addr, int Size)> Reads = new();
    public readonly List<(int Addr, int Size)> Writes = new();
    public readonly List<int> Speeds = new();
    public readonly List<int> Widths = new();
    public int Resets;
    public int Slept;
    public int? NackAt;

    public FakeVl53lxRegMap()
    {
        Regs[0x00E5] = 0x03;
        SetWord(0x0006, FakeVl53lxBridge.DieOsc);
        SetWord(0x010F, FakeVl53lxBridge.DieModelId);
        SetWord(0x00DE, FakeVl53lxBridge.DiePll);
        Regs[0x013E] = 0x91;
    }

    public void SetWord(int addr, int v)
    {
        Regs[addr] = (byte)(v >> 8);
        Regs[addr + 1] = (byte)v;
    }

    public int Word(int addr) => Regs[addr] << 8 | Regs[addr + 1];

    public long Dword(int addr) => (long)Word(addr) << 16 | (uint)Word(addr + 2);

    public void SetResultBlock(byte[] raw) => Array.Copy(raw, 0, Regs, 0x0089, raw.Length);

    public int WritesTo(int addr) => Writes.Count(w => w.Addr == addr);

    public override byte[] ReadMulti(int addr, int size)
    {
        Reads.Add((addr, size));
        return Regs.AsSpan(addr, size).ToArray();
    }

    public override void WriteMulti(int addr, byte[] data)
    {
        if (addr == NackAt)
            throw new Vl53lxProtocolException($"WRITE_REG 0x{addr:X4}: ERR_HARDWARE_FAULT");
        Writes.Add((addr, data.Length));
        Array.Copy(data, 0, Regs, addr, data.Length);
    }

    public override void SetI2cSpeed(int khz) => Speeds.Add(khz);

    public override void SetAddrWidth(int width) => Widths.Add(width);

    public override void XshutReset()
    {
        Resets++;
        NackAt = null; // the line reset brings the die back
    }

    public override void SleepMs(int ms) => Slept += ms;
}

/// <summary>The least a driver can be: every optional group left to the base class.</summary>
internal sealed class MinimalVl53lxDriver : Vl53lxDriver
{
    public bool Ready;
    public int Decodes;

    public MinimalVl53lxDriver(Vl53lxPlatform p) : base(p, "VL53L9ZZ") { }

    public override int ModelId() => 0x1234;
    public override void SensorInit() { }
    public override void StartRanging() { }
    public override void StopRanging() { }
    public override bool CheckForDataReady() => Ready;
    public override void ClearInterrupt() { }
    public override (int Addr, int Len) StreamBlock() => (0x0089, 17);

    public override Vl53lxRange Decode(byte[] raw)
    {
        Decodes++;
        return new Vl53lxRange(raw.Length, 0, "valid", 0, 0, 0, 0, Array.Empty<Vl53lxTarget>(), new Dictionary<string, object?>());
    }

    public override void SetRangeTiming(int timingBudgetMs, int interMeasurementMs) { }
    public override (int TimingBudgetMs, int InterMeasurementMs) GetRangeTiming() => (50, 0);
}

/// <summary>
/// The 1D-family driver contract (<see cref="Vl53lxDriver"/>: what a driver gets for free and what an absent
/// optional group throws) and the VL53L4CD / VL53L1X ULD arithmetic over a scripted register map (the Python
/// test_vl53l4cd_uld.py and test_vl53l1_uld.py cases).
/// </summary>
public class Vl53lxDriverTests
{
    // vectors/vl53lx.json die_block vl53l4cx_uld_as_l4cd_50ms_f2: 624 mm, 5160 kcps, 211 SPADs
    internal static readonly byte[] L4Block = FakeVl53lxBridge.Hex("090901d3200285008900082ba702700285");
    // die_block vl53l1cx_uld_long_33ms_f2: 576 mm, 16072 kcps, 67 SPADs
    internal static readonly byte[] L1Block = FakeVl53lxBridge.Hex("090901439007d9013f000d2234024007d9");

    // ── the base class ───────────────────────────────────────────────────────

    [Fact]
    public void TheBaseClassDefaultsAreTheDieUldsWithoutOptions()
    {
        var p = new FakeVl53lxRegMap();
        var d = new MinimalVl53lxDriver(p);
        Assert.Equal("VL53L9ZZ", d.Product);
        Assert.Equal(2, d.AddrWidth);
        Assert.Empty(d.ClearSteps);
        Assert.Equal(400, d.MaxKhz);
        Assert.Empty(d.Supports);
        Assert.Empty(d.Modes);
        Assert.Equal((10, 200), d.BudgetMs);
        Assert.False(d.Histogram);
        Assert.Empty(d.BudgetChoices());
        Assert.Null(d.ReachMm());
        Assert.Empty(d.DriverInfo());
        Assert.Throws<ArgumentNullException>(() => new MinimalVl53lxDriver(null!));
    }

    [Fact]
    public void EveryAbsentGroupThrowsNamingTheProductAndTheGroup()
    {
        var d = new MinimalVl53lxDriver(new FakeVl53lxRegMap());
        var calls = new (string Group, Action Call)[]
        {
            ("mode", () => d.SetMode("long")), ("mode", () => d.GetMode()),
            ("offset", () => d.GetOffset()), ("offset", () => d.SetOffset(1)),
            ("xtalk", () => d.GetXtalk()), ("xtalk", () => d.SetXtalk(1)),
            ("calib_offset", () => d.CalibrateOffset(100)), ("calib_xtalk", () => d.CalibrateXtalk(100)),
            ("thresholds", () => d.GetDetectionThresholds()), ("thresholds", () => d.SetDetectionThresholds(1, 2, 3)),
            ("signal_thresh", () => d.GetSignalThreshold()), ("signal_thresh", () => d.SetSignalThreshold(1)),
            ("sigma_thresh", () => d.GetSigmaThreshold()), ("sigma_thresh", () => d.SetSigmaThreshold(1)),
            ("roi", () => d.GetRoi()), ("roi", () => d.SetRoi(4, 4)),
            ("roi", () => d.GetRoiCenter()), ("roi", () => d.SetRoiCenter(199)),
            ("temp_update", () => d.StartTemperatureUpdate()), ("refspad", () => d.PerformRefSpadManagement()),
        };
        foreach (var (group, call) in calls)
        {
            var e = Assert.Throws<NotSupportedException>(call);
            Assert.Equal($"VL53L9ZZ/MinimalVl53lxDriver does not support '{group}'", e.Message);
        }
        Assert.Equal("MinimalVl53lxDriver has no histogram", Assert.Throws<NotSupportedException>(() => d.BinData()).Message);
        Assert.Equal("MinimalVl53lxDriver has no histogram", Assert.Throws<NotSupportedException>(() => d.ToMeasurement(new object())).Message);
    }

    [Fact]
    public void WaitDataReadyPollsThenTimesOutAndReadMeasurementReadsTheStreamBlock()
    {
        var p = new FakeVl53lxRegMap();
        var d = new MinimalVl53lxDriver(p);
        var e = Assert.Throws<Vl53lxException>(() => d.WaitDataReady(TimeSpan.Zero));
        Assert.Contains("timeout waiting for data ready", e.Message);
        Assert.IsAssignableFrom<Devices.DepzException>(e);
        d.Ready = true;
        d.WaitDataReady(TimeSpan.Zero);
        Assert.Equal(17, d.ReadMeasurement().DistanceMm);
        Assert.Equal((0x0089, 17), p.Reads[^1]);
        Assert.Equal(1, d.Decodes);
    }

    [Fact]
    public void TheRegistryBindsAProductToItsDriverAndCaveat()
    {
        var p = new FakeVl53lxRegMap();
        var (drv, caveat) = Vl53lxProducts.CreateDriver("VL53L4CD", "uld", p);
        Assert.IsType<Vl53l4UldDriver>(drv);
        Assert.Equal("VL53L4CD", drv.Product);
        Assert.Null(caveat);
        Assert.StartsWith("single-target", Vl53lxProducts.CreateDriver("VL53L3CX", "ulp", p).Caveat);
        Assert.StartsWith("the histogram driver", Vl53lxProducts.CreateDriver("VL53L4CX", "histogram", p).Caveat);
        Assert.Equal(new[] { "uld", "histogram" }, Vl53lxProducts.DriverKinds("VL53L1CB"));
        var e = Assert.Throws<NotSupportedException>(() => Vl53lxProducts.CreateDriver("VL53L0X", "histogram", p));
        Assert.Contains("VL53L0X has no 'histogram' driver - it has uld", e.Message);
        e = Assert.Throws<NotSupportedException>(() => Vl53lxProducts.DriverKinds("VL53L9ZZ"));
        Assert.Contains("no such product: VL53L9ZZ", e.Message);
    }

    [Fact]
    public void ARangeIsValidOnlyAtStatusZero()
    {
        var r = new Vl53lxRange(100, 0, "valid", 1, 2, 3, 4, Array.Empty<Vl53lxTarget>(), new Dictionary<string, object?>());
        Assert.True(r.Valid);
        Assert.False((r with { Status = 4 }).Valid);
        Assert.Equal("boom", new Vl53lxException("boom").Message);
        Assert.Null(new Vl53lxProtocolException("no answer").InnerException);
        var cause = new TimeoutException("bus");
        Assert.Same(cause, new Vl53lxProtocolException("no answer", cause).InnerException);
    }

    // ── the VL53L4CD ULD over the register map ───────────────────────────────

    private static Vl53l4UldDriver L4(FakeVl53lxRegMap p)
    {
        var d = new Vl53l4UldDriver(p, "VL53L4CD");
        d.SensorInit();
        p.Writes.Clear();
        return d;
    }

    [Fact]
    public void L4InitRunsAtBootSpeedThenTheDiesCeilingAndEndsIn50msContinuous()
    {
        var p = new FakeVl53lxRegMap();
        var d = new Vl53l4UldDriver(p, "VL53L4CD");
        Assert.Equal(new[] { "timing", "offset", "xtalk", "thresholds", "signal_thresh", "sigma_thresh", "temp_update", "calib_offset", "calib_xtalk" },
            d.Supports);
        Assert.Equal(new[] { new Vl53lxClearStep(0x0086, 0x01) }, d.ClearSteps);
        Assert.Equal(1000, d.MaxKhz);
        Assert.Equal((0x0089, 17), d.StreamBlock());
        d.SensorInit();
        Assert.Equal(new[] { 2 }, p.Widths);
        Assert.Equal(new[] { 400, 1000 }, p.Speeds);
        Assert.Equal(0x12, p.Regs[0x002D]); // Fast Mode Plus byte on the blob
        Assert.Equal(0x0500, p.Word(0x0024)); // ALGO__RANGE_IGNORE_THRESHOLD_MCPS, the one write SensorInit adds
        Assert.Equal(0x09, p.Regs[0x0008]);
        Assert.Equal(0x80, p.Regs[0x0087]); // stopped after VHV
        var (budget, inter) = d.GetRangeTiming();
        Assert.Equal(0, inter);
        Assert.InRange(budget, 49, 51);
        Assert.Equal(0xEBAA, d.ModelId());
        Assert.Equal(0x03, d.BootState());
        Assert.Equal(1388, d.ReachMm());
        Assert.Equal(1388, Vl53l1Die.PhaseWindowMm(FakeVl53lxBridge.DieOsc, 0x38));
        Assert.Equal(4563, Vl53l1Die.PhaseWindowMm(FakeVl53lxBridge.DieOsc, 0xB8));
    }

    [Fact]
    public void L4OffsetCalibrationIsTargetMinusTheMeanTimesFour()
    {
        var p = new FakeVl53lxRegMap();
        var d = L4(p);
        p.SetResultBlock(L4Block); // 624 mm
        Assert.Equal(-24, d.CalibrateOffset(600)); // the default of 20 samples
        Assert.Equal(-96 & 0xFFFF, p.Word(0x001E));
        Assert.Equal(-24, d.GetOffset());
        Assert.Equal(30, p.WritesTo(0x0086)); // 10 heat frames + 20 samples released
        Assert.Equal(76, d.CalibrateOffset(700, 5));
        Assert.Equal(76, d.GetOffset());
    }

    [Fact]
    public void L4XtalkCalibrationDropsTheFirstAndInvalidFramesThenAverages()
    {
        var p = new FakeVl53lxRegMap();
        var d = L4(p);
        p.SetResultBlock(L4Block); // 624 mm, 5160 kcps, 211 SPADs
        Assert.Equal(1, d.CalibrateXtalk(650));
        Assert.Equal(500, p.Word(0x0016)); // (1 - 624/650) × 5160/211 = 0.978 kcps × 512
        Assert.Equal(1, d.GetXtalk());
        var bad = (byte[])L4Block.Clone();
        bad[0] = 0x04; // status 2: no valid sample
        p.SetResultBlock(bad);
        Assert.Contains("no valid samples", Assert.Throws<Vl53lxException>(() => d.CalibrateXtalk(650, 5)).Message);
        var hot = (byte[])L4Block.Clone();
        hot[3] = 0x01;
        hot[4] = 0x00;
        hot[5] = 0xFF;
        hot[6] = 0xFF; // 1 SPAD, 524280 kcps
        p.SetResultBlock(hot);
        Assert.Contains("> 127", Assert.Throws<Vl53lxException>(() => d.CalibrateXtalk(5000, 5)).Message);
    }

    [Fact]
    public void L4CalibrationArgumentsAreCheckedBeforeTheBus()
    {
        var p = new FakeVl53lxRegMap();
        var d = L4(p);
        foreach (var (target, n) in new[] { (600, 4), (600, 256), (9, 20), (1001, 20) })
            Assert.Contains("5..255, target 10..1000", Assert.Throws<Vl53lxException>(() => d.CalibrateOffset(target, n)).Message);
        foreach (var (target, n) in new[] { (600, 4), (600, 256), (9, 20), (5001, 20) })
            Assert.Contains("5..255, target 10..5000", Assert.Throws<Vl53lxException>(() => d.CalibrateXtalk(target, n)).Message);
        Assert.Empty(p.Writes);
    }

    [Fact]
    public void L4CollectTimesOutWhenDataReadyNeverComesAndTheResultBlockIsChecked()
    {
        var p = new FakeVl53lxRegMap();
        var d = L4(p);
        p.SetResultBlock(L4Block);
        p.Regs[0x0031] = 0x01; // INT never reaches the active level
        var clock = System.Diagnostics.Stopwatch.StartNew();
        Assert.Contains("timeout waiting for data ready", Assert.Throws<Vl53lxException>(() => d.StartTemperatureUpdate()).Message);
        Assert.True(clock.Elapsed < TimeSpan.FromSeconds(4), "the 1 s data-ready limit, not the 5 s collect one");
        Assert.Contains("too short", Assert.Throws<Vl53lxException>(() => d.Decode(new byte[16])).Message);
        Assert.Equal("unknown (99)", Vl53l1Die.StatusText(99));
        Assert.Equal("other error", Vl53l1Die.StatusText(255));
    }

    [Fact]
    public void DieTimingAutonomousModeAndItsRefusals()
    {
        var p = new FakeVl53lxRegMap();
        var d = L4(p);
        d.SetRangeTiming(50, 100);
        Assert.Equal((long)(1.055 * 100 * FakeVl53lxBridge.DiePll), p.Dword(0x006C));
        var (budget, inter) = d.GetRangeTiming();
        Assert.InRange(inter, 99, 100); // ST writes ×1.055 and reads ÷1.065
        Assert.InRange(budget, 48, 52);
        p.Writes.Clear();
        d.StartRanging();
        Assert.Equal(0x40, p.Regs[0x0087]); // autonomous low power
        d.SetRangeTiming(200, 0);
        Assert.InRange(d.GetRangeTiming().TimingBudgetMs, 199, 201);
        d.SetRangeTiming(10, 0);
        Assert.InRange(d.GetRangeTiming().TimingBudgetMs, 9, 11);

        Assert.Contains("10..200", Assert.Throws<Vl53lxException>(() => d.SetRangeTiming(9, 0)).Message);
        Assert.Contains("10..200", Assert.Throws<Vl53lxException>(() => d.SetRangeTiming(201, 0)).Message);
        Assert.Contains("inter_measurement_ms must be 0 or >", Assert.Throws<Vl53lxException>(() => d.SetRangeTiming(50, 50)).Message);
        p.SetWord(0x00DE, 0); // no PLL: the period reads 0
        Assert.Equal(0, d.GetRangeTiming().InterMeasurementMs);
        p.SetWord(0x0006, 0);
        Assert.Contains("osc_frequency reads 0", Assert.Throws<Vl53lxException>(() => d.SetRangeTiming(50, 0)).Message);
        Assert.Contains("osc_frequency reads 0", Assert.Throws<Vl53lxException>(() => d.GetRangeTiming()).Message);
        Assert.Contains("osc_frequency reads 0", Assert.Throws<Vl53lxException>(() => d.ReachMm()).Message);
    }

    [Fact]
    public void DieThresholdsOffsetXtalkSigmaSignalRoundTrip()
    {
        var p = new FakeVl53lxRegMap();
        var d = L4(p);
        d.SetDetectionThresholds(100, 300, Vl53l4Uld.WindowIn);
        Assert.Equal((100, 300, Vl53l4Uld.WindowIn), d.GetDetectionThresholds());
        Assert.Equal((300, 100), (p.Word(0x0072), p.Word(0x0074)));
        d.SetOffset(-10);
        Assert.Equal(0xFFD8, p.Word(0x001E));
        Assert.Equal(-10, d.GetOffset());
        d.SetOffset(7);
        Assert.Equal(7, d.GetOffset());
        d.SetXtalk(20);
        Assert.Equal(10240, p.Word(0x0016));
        Assert.Equal(20, d.GetXtalk());
        d.SetSignalThreshold(1024);
        Assert.Equal(128, p.Word(0x0066));
        Assert.Equal(1024, d.GetSignalThreshold());
        d.SetSigmaThreshold(15);
        Assert.Equal(60, p.Word(0x0064));
        Assert.Equal(15, d.GetSigmaThreshold());
        Assert.Contains("16383", Assert.Throws<Vl53lxException>(() => d.SetSigmaThreshold(16384)).Message);
        d.SetSigmaThreshold(16383);
        Assert.Equal(16383, d.GetSigmaThreshold());
    }

    [Fact]
    public void L4ResetFallsBackToXshutWhenTheSoftResetIsNotAcknowledged()
    {
        var p = new FakeVl53lxRegMap();
        var d = new Vl53l4UldDriver(p, "VL53L4CD");
        p.Regs[0x00E5] = 0x00; // never boots
        var e = Assert.Throws<Vl53lxException>(() => d.WaitBoot(TimeSpan.Zero));
        Assert.Contains("FIRMWARE__SYSTEM_STATUS", e.Message);
        Assert.Equal(0, d.BootState());
        p.Regs[0x00E5] = 0x03;
        d.ResetDevice(TimeSpan.FromMilliseconds(10));
        Assert.Equal(0, p.Resets);
        Assert.Equal(1, p.Slept);
        Assert.Equal(1, d.GetInterruptPolarity()); // GPIO_HV_MUX_CTRL bit 4 clear: active high
        d.SetInterruptPolarity(0);
        Assert.Equal(0, d.GetInterruptPolarity());
        Assert.Equal(0x10, p.Regs[0x0030]);
        d.SetInterruptPolarity(1);
        Assert.Equal(1, d.GetInterruptPolarity());
        Assert.Equal(0x00, p.Regs[0x0030]);

        // a die that no longer ACKs its soft reset is reset through the XSHUT line instead
        p.NackAt = 0x0000;
        d.ResetDevice(TimeSpan.FromMilliseconds(10));
        Assert.Equal(1, p.Resets);
        Assert.Null(p.NackAt);

        // ... and one that acknowledges but never boots is reset through it too
        var slow = new FakeVl53lxRegMap();
        var ds = new Vl53l4UldDriver(slow, "VL53L4CD");
        slow.Regs[0x00E5] = 0x00;
        Assert.Throws<Vl53lxException>(() => ds.ResetDevice(TimeSpan.FromMilliseconds(5)));
        Assert.Equal(1, slow.Resets);
    }

    [Fact]
    public void L4SensorInitFailsOnASensorThatNeverBoots()
    {
        var p = new FakeVl53lxRegMap();
        p.Regs[0x00E5] = 0x00;
        var d = new Vl53l4UldDriver(p, "VL53L4CD");
        Assert.Contains("FIRMWARE__SYSTEM_STATUS", Assert.Throws<Vl53lxException>(() => d.SensorInit()).Message);
        Assert.Equal(new[] { 400 }, p.Speeds); // never raised to the ceiling
    }

    [Fact]
    public void DataReadyFollowsTheInterruptPolarityBit()
    {
        var p = new FakeVl53lxRegMap();
        var d = new Vl53l4UldDriver(p, "VL53L4CD");
        p.Regs[0x0030] = 0x10; // active low
        p.Regs[0x0031] = 0x00;
        Assert.True(d.CheckForDataReady());
        p.Regs[0x0031] = 0x01;
        Assert.False(d.CheckForDataReady());
        p.Regs[0x0030] = 0x00; // active high
        Assert.True(d.CheckForDataReady());
        p.Regs[0x0031] = 0x00;
        Assert.False(d.CheckForDataReady());
        d.ClearInterrupt();
        Assert.Equal(0x01, p.Regs[0x0086]);
        d.StopRanging();
        Assert.Equal(0x80, p.Regs[0x0087]);
    }

    // ── the VL53L3CX ULP: the die's body with its own blob and hooks ─────────

    [Fact]
    public void L3UlpInitAddsItsOwnWritesAndStopsWithZero()
    {
        var p = new FakeVl53lxRegMap();
        var d = new Vl53l3UlpDriver(p, "VL53L3CX");
        Assert.Equal(new[] { "timing", "thresholds", "signal_thresh", "sigma_thresh", "roi" }, d.Supports);
        d.SensorInit();
        Assert.Equal(0x12, p.Regs[0x002D]);
        Assert.Equal(0x0500, p.Word(0x0024));
        Assert.Equal(0x8A, p.Regs[0x0081]);
        Assert.Equal(0x03, p.Regs[0x004B]);
        Assert.Equal(0x00, p.Regs[0x0087]); // STOP_MODE 0x00
        p.Writes.Clear();
        d.SetRoi(8, 6);
        Assert.Equal(0x91, p.Regs[0x007F]); // the optical centre the die reports
        Assert.Equal(0x57, p.Regs[0x0080]);
        Assert.Equal((8, 6), d.GetRoi());
        d.SetRoi(16, 16); // over 10: the centre moves to 199
        Assert.Equal(199, d.GetRoiCenter());
        Assert.Equal((16, 16), d.GetRoi());
        d.SetRoi(20, 4); // clipped to 16
        Assert.Equal((16, 4), d.GetRoi());
        d.SetRoiCenter(0x88);
        Assert.Equal(0x88, d.GetRoiCenter());
        // The die reads the offset register either way; the "offset" group is gated one layer up,
        // in Vl53lxDevice.Need, exactly as the Python _need — so it is only absent from Supports.
        Assert.DoesNotContain("offset", d.Supports);
        p.SetWord(0x001E, 10 * 4); // RANGE_OFFSET_MM holds mm × 4
        Assert.Equal(10, d.GetOffset());
    }

    // ── the VL53L1X ULD over the register map ────────────────────────────────

    private static Vl53l1UldDriver L1(FakeVl53lxRegMap p)
    {
        var d = new Vl53l1UldDriver(p, "VL53L1CX");
        d.SensorInit();
        p.Writes.Clear();
        return d;
    }

    [Fact]
    public void L1InitResetsTheDieFirstAndFlipsTheInterruptToActiveLow()
    {
        var p = new FakeVl53lxRegMap();
        var d = new Vl53l1UldDriver(p, "VL53L1CX");
        Assert.Equal(new[] { "long", "short" }, d.Modes);
        Assert.Equal((15, 500), d.BudgetMs);
        Assert.Equal(new[] { 20, 33, 50, 100, 200, 500 }, d.BudgetChoices()); // before init: the blob's long mode
        d.SensorInit();
        Assert.Equal((0x0000, 1), p.Writes[0]); // soft reset low ...
        Assert.Equal((0x0000, 1), p.Writes[1]); // ... then high
        Assert.Equal(0x01, p.Regs[0x0000]);
        Assert.Equal(0x10, p.Regs[0x0030] & 0x10); // INT active low for the bridge
        Assert.Equal(0x00, p.Regs[0x0087]); // STOP_MODE 0x00
        Assert.Equal("long", d.GetMode());
        Assert.Equal((50, 50), d.GetRangeTiming()); // no free-running mode: a period of 0 is the budget
        Assert.Equal(0x00AD, p.Word(0x005E));
        Assert.Equal(0x00C6, p.Word(0x0061));
        Assert.Equal(4563, d.ReachMm());
        p.Writes.Clear();
        d.StartRanging();
        Assert.Equal(new[] { (0x0087, 1) }, p.Writes);
        Assert.Equal(0x40, p.Regs[0x0087]); // timed mode only
    }

    [Fact]
    public void L1ABudgetTheNewModeLacksSnapsToItsNearestOne()
    {
        var p = new FakeVl53lxRegMap();
        var d = L1(p);
        Assert.Equal("long", d.GetMode());
        d.SetMode("short");
        d.SetRangeTiming(15, 0); // 15 ms exists in short mode only
        Assert.Equal((15, 15), d.GetRangeTiming());
        Assert.Equal(new[] { 15, 20, 33, 50, 100, 200, 500 }, d.BudgetChoices());
        Assert.Equal(1388, d.ReachMm());
        p.Writes.Clear();
        // 15 is not a long-mode budget: the nearest, 20, is applied and returned (contract 12 §5)
        Assert.Equal(20, d.SetMode("long"));
        Assert.Equal("long", d.GetMode());
        Assert.Equal((20, 15), d.GetRangeTiming());
        Assert.Equal(new[] { 20, 33, 50, 100, 200, 500 }, d.BudgetChoices());
        // the long-mode VCSEL / phase registers, then the 20 ms MACROP pair
        Assert.Equal(new[] { (0x004B, 1), (0x0060, 1), (0x0063, 1), (0x0069, 1), (0x0078, 2), (0x007A, 2), (0x005E, 2), (0x0061, 2) },
            p.Writes.Take(8));
        Assert.Equal((0x001E, 0x0022), (p.Word(0x005E), p.Word(0x0061)));
        var a = Assert.Throws<ArgumentException>(() => d.SetMode("medium"));
        Assert.Contains("no such mode: medium (have long, short)", a.Message);
        var e = Assert.Throws<Vl53lxException>(() => d.SetRangeTiming(40, 0));
        Assert.Contains("pick one of 20, 33, 50, 100, 200, 500", e.Message);
        d.SetRangeTiming(500, 1000);
        Assert.Equal(500, d.GetRangeTiming().TimingBudgetMs);
        Assert.InRange(d.GetRangeTiming().InterMeasurementMs, 1000, 1010); // ST writes ×1.075 and reads ÷1.065
        p.SetWord(0x005E, 0x0001); // a MACROP no table row has
        Assert.Equal(0, d.GetRangeTiming().TimingBudgetMs);
        p.SetWord(0x00DE, 0); // no PLL
        Assert.Equal(0, d.GetRangeTiming().InterMeasurementMs);
        p.Regs[0x004B] = 0x00; // PHASECAL_CONFIG__TIMEOUT_MACROP neither 0x14 nor 0x0A
        e = Assert.Throws<Vl53lxException>(() => d.GetMode());
        Assert.Contains("neither distance mode", e.Message);
        Assert.Throws<Vl53lxException>(() => d.SetRangeTiming(50, 0));
    }

    [Fact]
    public void L1ThresholdsKeepTheBitsOutside0x6FAndCanInterruptOnNoTarget()
    {
        var p = new FakeVl53lxRegMap();
        var d = L1(p);
        p.Regs[0x0046] = 0x90; // bits the ULD must keep
        d.SetDetectionThresholds(100, 300, Vl53l4Uld.WindowIn);
        Assert.Equal(0x93, p.Regs[0x0046]);
        Assert.Equal((100, 300, Vl53l4Uld.WindowIn), d.GetDetectionThresholds());
        d.SetDetectionThresholds(50, 60, Vl53l4Uld.WindowOut, intOnNoTarget: true);
        Assert.Equal(0xD2, p.Regs[0x0046]);
        Assert.Equal((60, 50), (p.Word(0x0072), p.Word(0x0074)));
        Assert.Equal((50, 60, Vl53l4Uld.WindowOut), d.GetDetectionThresholds());
    }

    [Fact]
    public void L1CalibrationsUseTheL1Formulas()
    {
        var p = new FakeVl53lxRegMap();
        var d = L1(p);
        p.SetResultBlock(L1Block); // 576 mm, 16072 kcps, 67 SPADs
        Assert.Equal(24, d.CalibrateOffset(600, 5));
        Assert.Equal(96, p.Word(0x001E));
        Assert.Equal(24, d.GetOffset());
        Assert.Equal(5, p.WritesTo(0x0086)); // no heat loop on the L1X ULD
        Assert.Contains("at least 5", Assert.Throws<Vl53lxException>(() => d.CalibrateOffset(600, 4)).Message);
        Assert.Equal(-26, d.CalibrateOffset(550)); // the default of 50 samples, negative
        Assert.Equal(55, p.WritesTo(0x0086));
        // 512 × 16072 × (1 − 576/600) / 67 = 4912.75, truncated → 10 kcps
        Assert.Equal(10, d.CalibrateXtalk(600, 5));
        Assert.Equal(4912, p.Word(0x0016));
        Assert.Equal(10, d.GetXtalk());
        Assert.Equal(0, d.CalibrateXtalk(500, 5)); // target nearer than the mean: clamped at 0
        Assert.Equal(0, p.Word(0x0016));
        Assert.Contains("at least 5", Assert.Throws<Vl53lxException>(() => d.CalibrateXtalk(600, 4)).Message);
        var none = (byte[])L1Block.Clone();
        none[3] = 0;
        none[4] = 0; // 0 SPADs
        p.SetResultBlock(none);
        Assert.Contains("no SPADs enabled", Assert.Throws<Vl53lxException>(() => d.CalibrateXtalk(600, 5)).Message);
        var hot = (byte[])L1Block.Clone();
        hot[3] = 0x01;
        hot[4] = 0x00;
        hot[15] = 0xFF;
        hot[16] = 0xFF; // 1 SPAD, 524280 kcps: the register clamps at 0xFFFF
        p.SetResultBlock(hot);
        Assert.Equal(128, d.CalibrateXtalk(5000, 5));
        Assert.Equal(0xFFFF, p.Word(0x0016));
    }

    [Fact]
    public void L1DecodesWithItsOwnSignalOffsetAndPerSpadScale()
    {
        var p = new FakeVl53lxRegMap();
        var d = L1(p);
        p.SetResultBlock(L1Block);
        var m = d.ReadMeasurement();
        Assert.Equal((576, 0, "valid", 3.0, 16072.0, 2552.0, 67.0), (m.DistanceMm, m.Status, m.StatusText, m.SigmaMm, m.SignalKcps, m.AmbientKcps, m.Spads));
        Assert.Equal(23L, m.Extra["signal_per_spad_kcps"]);
        Assert.Equal(3L, m.Extra["ambient_per_spad_kcps"]);
        Assert.Equal(1L, m.Extra["stream_count"]);
        Assert.Empty(m.Targets);
        var l4 = new Vl53l4UldDriver(p, "VL53L4CD").Decode(L4Block);
        Assert.Equal((624, 5160.0, 211.0), (l4.DistanceMm, l4.SignalKcps, l4.Spads));
        Assert.Equal(24L, l4.Extra["signal_per_spad_kcps"]);
        var blank = new byte[17];
        blank[0] = 0x13; // raw 19 → 6
        var six = d.Decode(blank);
        Assert.Equal((6, "no wrap-around check done"), (six.Status, six.StatusText));
        Assert.Equal(0L, six.Extra["signal_per_spad_kcps"]);
        blank[0] = 0x1F; // raw 31: past the table, kept as is
        Assert.Equal(31, d.Decode(blank).Status);
    }
}
