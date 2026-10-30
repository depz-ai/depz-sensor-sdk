using Depz.Sensor.Devices;
using Depz.Sensor.Protocol;
using Depz.Sensor.Vl53l4;
using Depz.Sensor.Vl53lx;
using Xunit;

namespace Depz.Sensor.Tests;

/// <summary>
/// <see cref="Vl53lxDevice"/> (the APP_VL53L0_4 bridge, contract 12) driven through the in-process
/// <see cref="FakeVl53lxBridge"/> with a VL53L1-family register file behind it — no hardware: the Python
/// test_vl53lx_device.py cases. The full-stack captures (ScenarioTests) cover the init / stream path of each
/// pair; this covers what they cannot: refusals, bridge error statuses, timeouts, the capability-gated
/// passthroughs, calibrations, poll mode and the queues.
/// </summary>
public class Vl53lxDeviceTests
{
    private static readonly TimeSpan Short = TimeSpan.FromMilliseconds(500);

    // vectors/vl53lx.json
    private static readonly byte[] L4Block = Vl53lxDriverTests.L4Block;                 // 624 mm, f2
    private static readonly byte[] L4BlockF9 = FakeVl53lxBridge.Hex("090908d320027f008a00092bce026f027f"); // 623 mm, stream 8
    private static readonly byte[] L1Block = Vl53lxDriverTests.L1Block;                 // 576 mm
    private const string StartStreamDie = "890011000001860001";                          // encode start_stream_die_block
    private static readonly byte[] InfoWrapped = FakeVl53lxBridge.Hex("ffffffff0700000003000000020100e80302010c000000");

    private const string L4cdBoard = "DEPZ ToF Sensor VL53L4CD USB v2.1 TESTSERIAL";
    private const string L4cxBoard = "DEPZ ToF Sensor VL53L4CX USB v2.1 TOVJALN523";
    private const string L1cxBoard = "DEPZ ToF Sensor VL53L1CX USB v2.1 SER";
    private const string L3cxBoard = "DEPZ ToF Sensor VL53L3CX USB v2.1 SER";
    private const string Unstamped = "DEPZ ToF Sensor USB v2.1";

    private const int ReadReg = FakeVl53lxBridge.ReadReg, WriteReg = FakeVl53lxBridge.WriteReg;
    private const int Xshut = FakeVl53lxBridge.Xshut, StartStream = FakeVl53lxBridge.StartStream;
    private const int StopStream = FakeVl53lxBridge.StopStream, SetI2cSpeed = FakeVl53lxBridge.SetI2cSpeed;
    private const int SetAddrWidth = FakeVl53lxBridge.SetAddrWidth, ClearI2cErrors = FakeVl53lxBridge.ClearI2cErrors;
    private const int GetDeviceName = FakeVl53lxBridge.GetDeviceName;

    private sealed class Rig : IDisposable
    {
        public readonly FakeVl53lxBridge Fake;
        public readonly Vl53lxDevice Dev;

        public Rig(string board = L4cdBoard, Func<FakeVl53lxBridge, Vl53lxDevice>? make = null,
            TimeSpan? timeout = null, bool noClear = false)
        {
            Fake = new FakeVl53lxBridge(board, noClear);
            Dev = make?.Invoke(Fake) ?? new Vl53lxDevice(Fake.Link, timeout ?? Short);
        }

        /// <summary>Init, then forget the traffic it caused.</summary>
        public Rig Ready(string? product = null)
        {
            Dev.Init(product: product);
            Fake.ClearRx();
            return this;
        }

        /// <summary>Wait until the reader has handled everything the fake sent: a GET_INFO round trip
        /// behind the reports.</summary>
        public void Settle()
        {
            Dev.BridgeInfo();
            lock (Fake.Rx)
                Fake.Rx.RemoveAt(Fake.Rx.Count - 1);
        }

        public void Dispose() => Dev.Dispose();
    }

    private static void Later(Action fn, int delayMs = 50) =>
        new Thread(() => { Thread.Sleep(delayMs); fn(); }) { IsBackground = true }.Start();

    private static string Key(int addr, params int[] data) => FakeVl53lxBridge.Key(addr, data);

    // ── pure helpers ─────────────────────────────────────────────────────────

    private static Vl53lxTarget T(int mm, int status) => new(mm, status, "", 0, 0, 0, mm, mm);

    private static Vl53lxMeasurement M(int mm = 0, int status = 0, params Vl53lxTarget[] targets) =>
        new(0, mm, status, "", 0, 0, 0, 0, targets, new Dictionary<string, object?>(), null);

    [Fact]
    public void PlottableStatusesAre0And6And11()
    {
        Assert.Equal(new[] { 0, 6, 11 }, Vl53lxMeasurement.PlottableStatuses);
        Assert.True(M(status: 6).Plottable);
        Assert.False(M(status: 6).Valid);
        Assert.False(M(status: 4).Plottable);
    }

    [Fact]
    public void PlotDistancesKeepsPlottableTargetsInDriverOrder()
    {
        var m = M(100, 0, T(100, 0), T(250, 4), T(300, 11), T(400, 6));
        Assert.Equal(new[] { 100, 300, 400 }, m.PlotDistances());
        Assert.Equal(100, m.PrimaryTarget()!.DistanceMm);
        Assert.Null(M(1, 0, T(1, 4)).PrimaryTarget());
    }

    [Fact]
    public void ALightDriverUsesTheSingleDistanceOrNothing()
    {
        Assert.Equal(new[] { 624 }, M(624).PlotDistances());
        Assert.Empty(M(624, 4).PlotDistances());
        Assert.Null(M(624).PrimaryTarget());
    }

    // ── init and the product table ───────────────────────────────────────────

    [Fact]
    public void BeforeInitNoDriverNoProduct()
    {
        using var r = new Rig();
        Assert.False(r.Dev.Initialized);
        Assert.Null(r.Dev.Product);
        Assert.Null(r.Dev.DriverKind);
        Assert.Contains("call Init() first", Assert.Throws<InvalidStateException>(() => r.Dev.Driver).Message);
        Assert.Throws<InvalidStateException>(() => r.Dev.Supports("offset"));
        Assert.Throws<InvalidStateException>(() => r.Dev.GetOffsetMm());
    }

    [Fact]
    public void InitBindsTheProductAndSetsTheAddressWidthFirst()
    {
        using var r = new Rig();
        r.Dev.Init();
        Assert.True(r.Dev.Initialized);
        Assert.Equal(("VL53L4CD", "uld"), (r.Dev.Product, r.Dev.DriverKind));
        Assert.True(r.Dev.Supports("offset"));
        Assert.False(r.Dev.Supports("roi"));
        Assert.Empty(r.Dev.Modes);

        var cmds = r.Fake.Cmds();
        // device name, then SET_ADDR_WIDTH 2 before the first register access
        Assert.Equal(new[] { GetDeviceName, SetAddrWidth }, cmds.Take(2));
        Assert.Equal(new byte[] { 0x02 }, r.Fake.Sent(SetAddrWidth)[0]);
        Assert.True(cmds.IndexOf(SetAddrWidth) < cmds.IndexOf(ReadReg));
        // every init runs at 400 kHz, then the die's 1 MHz ceiling
        Assert.Equal(new[] { "9001", "e803" }, r.Fake.Sent(SetI2cSpeed).Select(FakeVl53lxBridge.ToHex));
        // the blob at 0x2D with byte 0 forced to FM+ (0x12)
        var blob = r.Fake.Writes().First(w => w.Addr == 0x2D).Data;
        Assert.Equal((91, 0x12), (blob.Length, blob[0]));
        // VL53L4CD_SensorInit() closes with ALGO__RANGE_IGNORE_THRESHOLD_MCPS
        Assert.Contains(Key(0x0024, 0x05, 0x00), r.Fake.WriteKeys());
        // CLEAR_I2C_ERRORS closes every init (contract 12 §2)
        Assert.Equal(ClearI2cErrors, cmds[^1]);
        Assert.Equal(new[] { Array.Empty<byte>() }, r.Fake.Sent(ClearI2cErrors));
    }

    [Fact]
    public void TheDeviceNameIsReadOnce()
    {
        using var r = new Rig();
        Assert.Equal("VL53L4CD", r.Dev.Detected);
        r.Dev.Init();
        r.Dev.Notes();
        r.Dev.IdentifyProduct();
        Assert.Single(r.Fake.Sent(GetDeviceName));
        Assert.Equal(L4cdBoard, r.Dev.BoardName);
    }

    [Fact]
    public void IdentifyProductReportsBothReachesAndTheModelIdCheck()
    {
        using var r = new Rig().Ready();
        var info = r.Dev.IdentifyProduct();
        Assert.Equal(L4cdBoard, info["board"]);
        Assert.Equal("VL53L4CD", info["detected"]);
        Assert.Equal("VL53L4CD", info["product"]);
        Assert.Equal("uld", info["driver"]);
        Assert.Equal("Vl53l4UldDriver", info["driver_class"]);
        Assert.Equal(new[] { "uld", "histogram" }, (IEnumerable<string>)info["kinds"]!);
        Assert.Equal(FakeVl53lxBridge.DieModelId, info["model_id"]);
        Assert.Equal(true, info["model_id_ok"]);
        Assert.Empty((IEnumerable<string>)info["modes"]!);
        Assert.Equal(1300, info["reach_mm"]);
        Assert.Equal(new[] { 10, 200 }, (int[])info["budget_ms"]!);
        Assert.Empty((IEnumerable<int>)info["budget_choices"]!);
        Assert.Equal(false, info["histogram"]);
        Assert.Equal(1000, info["max_khz"]);
        Assert.Null(info["caveat"]);
        // §5: the 0x38 window at the die's own oscillator, ~1388 mm
        Assert.Equal(1388, info["driver_reach_mm"]);
        Assert.Equal(1388, r.Dev.DriverReachMm());
        Assert.Contains("calib_xtalk", (IEnumerable<string>)info["supports"]!);
        Assert.Empty(r.Dev.Notes());
        Assert.Equal((10, 200), r.Dev.BudgetRange);
    }

    [Fact]
    public void AWrongModelIdFailsTheCrossCheckNotTheInit()
    {
        using var r = new Rig().Ready();
        r.Fake.SetWord(0x010F, 0xEACC);
        var info = r.Dev.IdentifyProduct();
        Assert.Equal((0xEACC, false), ((int)info["model_id"]!, (bool)info["model_id_ok"]!));
    }

    [Fact]
    public void BorrowingASiblingsDriver()
    {
        using var r = new Rig(L4cxBoard).Ready("VL53L4CD");
        var info = r.Dev.IdentifyProduct();
        Assert.Equal(("VL53L4CX", "VL53L4CD"), ((string)info["detected"]!, (string)info["product"]!));
        Assert.Equal((6000, 1388), ((int)info["reach_mm"]!, (int)info["driver_reach_mm"]!));
        var notes = r.Dev.Notes();
        Assert.Equal(2, notes.Count);
        Assert.Contains("borrowing this driver - the board says it is a VL53L4CX", notes[0]);
        Assert.Contains("reaches 1388 mm, the board is rated 6000 mm", notes[1]);
    }

    [Fact]
    public void AnUnstampedBoardNeedsTheProductNamed()
    {
        using var r = new Rig(Unstamped);
        Assert.Null(r.Dev.Detected);
        Assert.Empty(r.Dev.DriverKinds());
        Assert.Contains("carries no product number", Assert.Throws<InvalidStateException>(() => r.Dev.Init()).Message);
        Assert.Empty(r.Fake.Sent(SetAddrWidth));
        r.Dev.Init(product: "VL53L4CD");
        Assert.Equal("VL53L4CD", r.Dev.Product);
        Assert.Equal(new[] { "VL53L4CD: board name carries no product number, named by hand" }, r.Dev.Notes());
    }

    [Fact]
    public void APairTheTableHasNoRowForIsRefusedBeforeAnyTraffic()
    {
        using var r = new Rig(L4cxBoard, f => new Vl53l4cxDevice(f.Link, Short));
        Assert.Equal(new[] { "histogram" }, r.Dev.DriverKinds());
        Assert.Contains("VL53L4CX has no 'uld' driver - it has histogram",
            Assert.Throws<NotSupportedException>(() => r.Dev.Init("uld")).Message);
        Assert.Contains("no such product", Assert.Throws<NotSupportedException>(() => r.Dev.Init(product: "VL53L9ZZ")).Message);
        Assert.Empty(r.Fake.Cmds().Where(c => c != GetDeviceName));
        Assert.False(r.Dev.Initialized);
    }

    [Fact]
    public void DriverKindsOfANamedProductAndOfTheClassProduct()
    {
        using var r = new Rig(Unstamped, f => new Vl53l1cxDevice(f.Link, Short));
        Assert.Equal(new[] { "uld", "histogram" }, r.Dev.DriverKinds());
        Assert.Equal(new[] { "ulp", "histogram" }, r.Dev.DriverKinds("VL53L3CX"));
    }

    [Fact]
    public void EveryProductClassFixesItsProductWhateverTheBoardSays()
    {
        var want = new (Func<FakeVl53lxBridge, Vl53lxDevice> Make, string Product, string[] Kinds)[]
        {
            (f => new Vl53l0xDevice(f.Link, Short), "VL53L0X", new[] { "uld" }),
            (f => new Vl53l1cxDevice(f.Link, Short), "VL53L1CX", new[] { "uld", "histogram" }),
            (f => new Vl53l1cbDevice(f.Link, Short), "VL53L1CB", new[] { "uld", "histogram" }),
            (f => new Vl53l3cxDevice(f.Link, Short), "VL53L3CX", new[] { "ulp", "histogram" }),
            (f => new Vl53l4cxDevice(f.Link, Short), "VL53L4CX", new[] { "histogram" }),
        };
        foreach (var (make, product, kinds) in want)
        {
            using var r = new Rig(Unstamped, make);
            Assert.Equal(kinds, r.Dev.DriverKinds());
            Assert.Null(r.Dev.Detected);
            if (product == "VL53L1CB" || product == "VL53L3CX")
            {
                // the L1 die's light drivers run on the register file: the class name is the product
                r.Dev.Init();
                Assert.Equal((product, kinds[0]), (r.Dev.Product, r.Dev.DriverKind));
                Assert.Contains($"{product}: board name carries no product number", r.Dev.Notes()[0]);
            }
        }
    }

    [Fact]
    public void AV200BridgeIsToldToReflash()
    {
        using var r = new Rig(noClear: true);
        var e = Assert.Throws<DepzException>(() => r.Dev.Init());
        Assert.Matches("predates APP_VL53L0_4_v0.24.*reflash", e.Message);
        Assert.False(r.Dev.Initialized);
    }

    [Fact]
    public void AnyOtherClearI2cErrorsFailureIsAProtocolError()
    {
        using var r = new Rig();
        r.Fake.FailNext(ClearI2cErrors, (int)Status.ErrHardwareFault);
        Assert.Throws<Vl53lxProtocolException>(() => r.Dev.Init());
    }

    [Fact]
    public void ASilentClearI2cErrorsIsAProtocolErrorToo()
    {
        using var r = new Rig(timeout: TimeSpan.FromMilliseconds(100));
        r.Fake.FailNext(ClearI2cErrors, FakeVl53lxBridge.Silent);
        Assert.Throws<Vl53lxProtocolException>(() => r.Dev.Init());
    }

    [Fact]
    public void ARefusedSetAddrWidthFailsInitAsAProtocolError()
    {
        using var r = new Rig();
        r.Fake.FailNext(SetAddrWidth, (int)Status.ErrInvalidParam);
        Assert.Throws<Vl53lxProtocolException>(() => r.Dev.Init());
        Assert.False(r.Dev.Initialized);
    }

    // ── the bridge platform the ULD ports talk to ────────────────────────────

    [Fact]
    public void AnErrorStatusOnARegisterAccessIsAProtocolError()
    {
        using var r = new Rig().Ready();
        r.Fake.FailNext(ReadReg, (int)Status.ErrHardwareFault);
        Assert.Throws<Vl53lxProtocolException>(() => r.Dev.GetOffsetMm());
        r.Fake.FailNext(WriteReg, (int)Status.ErrHardwareFault);
        Assert.Throws<Vl53lxProtocolException>(() => r.Dev.SetOffsetMm(1));
        // the traffic after the failure is answered again
        Assert.Equal(0, r.Dev.GetOffsetMm());
    }

    [Fact]
    public void NoAnswerToARegisterAccessIsAProtocolErrorToo()
    {
        using var r = new Rig(timeout: TimeSpan.FromMilliseconds(100)).Ready();
        r.Fake.FailNext(ReadReg, FakeVl53lxBridge.Silent);
        Assert.Throws<Vl53lxProtocolException>(() => r.Dev.GetXtalkKcps());
    }

    [Fact]
    public void BridgeInfoDecodesRptVl53Info()
    {
        using var r = new Rig();
        r.Fake.Info = InfoWrapped;
        var info = r.Dev.BridgeInfo();
        Assert.Equal(new Vl53lxInfo(4294967295, 7, 3, 2, 1, 0, 1000, 2, 1, 12), info);
    }

    [Fact]
    public void XshutDropsTheDriverAndTheStream()
    {
        using var r = new Rig().Ready();
        r.Dev.StartRanging();
        r.Dev.Xshut(Vl53l4Xshut.Off);
        Assert.Equal(new[] { new byte[] { 0x00 } }, r.Fake.Sent(Xshut));
        Assert.False(r.Dev.Ranging);
        Assert.False(r.Dev.Initialized);
        Assert.Throws<InvalidStateException>(() => r.Dev.Driver);
        Assert.Throws<ArgumentOutOfRangeException>(() => r.Dev.Xshut(3));
    }

    // ── configuration and the capability-gated passthroughs ──────────────────

    [Fact]
    public void ConfigureReinitsThenTimingOffsetXtalkSignal()
    {
        using var r = new Rig().Ready();
        r.Dev.Configure(budgetMs: 33, offsetMm: -10, xtalkKcps: 20, signalKcps: 1024);
        Assert.Single(r.Fake.Sent(ClearI2cErrors));
        // vectors/vl53l4.json tuning: offset -10 → 65496, xtalk 20 → 10240, signal 1024 → 128
        Assert.Equal(new[]
        {
            Key(0x001E, 0xFF, 0xD8), Key(0x0020, 0, 0), Key(0x0022, 0, 0), Key(0x0018, 0, 0), Key(0x001A, 0, 0),
            Key(0x0016, 0x28, 0x00), Key(0x0066, 0x00, 0x80),
        }, r.Fake.WriteKeys().TakeLast(7));
        Assert.Equal(-10, r.Dev.GetOffsetMm());
        Assert.Equal(20, r.Dev.GetXtalkKcps());
        Assert.Equal(1024, r.Dev.GetSignalThresholdKcps());
        var (budget, inter) = r.Dev.GetRangeTiming();
        Assert.Equal(0, inter);
        Assert.InRange(budget, 32, 34);
        Assert.Null(r.Dev.GetMode());
    }

    [Fact]
    public void AnInterMeasurementPeriodSelectsAutonomousMode()
    {
        using var r = new Rig().Ready();
        r.Dev.Configure(budgetMs: 50, interMs: 100);
        Assert.Equal((long)(1.055 * 100 * (FakeVl53lxBridge.DiePll & 0x3FF)), r.Fake.Dword(0x006C));
        r.Fake.ClearRx();
        r.Dev.StartRanging();
        Assert.Equal(new[] { Key(0x0087, 0x40) }, r.Fake.WriteKeys());
    }

    [Fact]
    public void APeriodNotAboveTheBudgetIsRefused()
    {
        using var r = new Rig().Ready();
        Assert.Throws<Vl53lxException>(() => r.Dev.Configure(budgetMs: 50, interMs: 20));
        Assert.Contains("10..200", Assert.Throws<Vl53lxException>(() => r.Dev.Configure(budgetMs: 300)).Message);
    }

    [Fact]
    public void AModeOnAProductWithoutModesIsRefusedBeforeTheReinit()
    {
        using var r = new Rig().Ready();
        Assert.Contains("VL53L4CD/uld does not support 'mode'",
            Assert.Throws<NotSupportedException>(() => r.Dev.Configure(mode: "long")).Message);
        Assert.Equal(0, r.Fake.RxCount());
    }

    [Fact]
    public void SnapBudgetClampsToTheDriversRangeWhenAnyBudgetGoes()
    {
        using var r = new Rig().Ready();
        Assert.Empty(r.Dev.BudgetChoices());
        Assert.Equal((10, 77, 200), (r.Dev.SnapBudget(5), r.Dev.SnapBudget(77), r.Dev.SnapBudget(500)));
    }

    [Fact]
    public void OffsetXtalkThresholdsSignalSigmaRoundTrip()
    {
        using var r = new Rig().Ready();
        r.Dev.SetOffsetMm(10);
        Assert.Equal(40, r.Fake.Word(0x001E));
        Assert.Equal(10, r.Dev.GetOffsetMm());
        r.Dev.SetXtalkKcps(3);
        Assert.Equal(1536, r.Fake.Word(0x0016));
        Assert.Equal(3, r.Dev.GetXtalkKcps());
        r.Dev.SetDetectionThresholds(100, 300, 3);
        Assert.Equal(3, r.Fake.Regs[0x0046]);
        Assert.Equal((300, 100), (r.Fake.Word(0x0072), r.Fake.Word(0x0074)));
        Assert.Equal((100, 300, 3), r.Dev.GetDetectionThresholds());
        r.Dev.SetSignalThresholdKcps(2048);
        Assert.Equal(256, r.Fake.Word(0x0066));
        Assert.Equal(2048, r.Dev.GetSignalThresholdKcps());
        r.Dev.SetSigmaThresholdMm(15);
        Assert.Equal(60, r.Fake.Word(0x0064));
        Assert.Equal(15, r.Dev.GetSigmaThresholdMm());
        Assert.Contains("16383", Assert.Throws<Vl53lxException>(() => r.Dev.SetSigmaThresholdMm(16384)).Message);
    }

    [Fact]
    public void AnUnsupportedGroupIsRefusedWithNoTraffic()
    {
        using var r = new Rig().Ready();
        var calls = new Action[]
        {
            () => r.Dev.GetRoi(), () => r.Dev.SetRoi(4, 4), () => r.Dev.GetRoiCenter(), () => r.Dev.SetRoiCenter(199),
            () => r.Dev.PerformRefSpadManagement(),
        };
        foreach (var call in calls)
            Assert.Contains("VL53L4CD/uld does not support", Assert.Throws<NotSupportedException>(call).Message);
        Assert.Equal(0, r.Fake.RxCount());
    }

    [Fact]
    public void TheRefusalNamesTheCaveatAndComesBeforeTheReinit()
    {
        using var r = new Rig(L3cxBoard).Ready();
        Assert.Equal("ulp", r.Dev.DriverKind);
        var e = Assert.Throws<NotSupportedException>(() => r.Dev.Configure(offsetMm: 5));
        Assert.Contains("VL53L3CX/ulp does not support 'offset' (single-target ranging only", e.Message);
        Assert.Contains("'calib_xtalk'", Assert.Throws<NotSupportedException>(() => r.Dev.CalibrateXtalk(600)).Message);
        Assert.Equal(0, r.Fake.RxCount());
        Assert.Contains(r.Dev.Notes(), n => n.StartsWith("VL53L3CX/ulp: single-target", StringComparison.Ordinal));
    }

    [Fact]
    public void TemperatureUpdateRerunsVhvAndRestoresTheLoopBound()
    {
        using var r = new Rig().Ready();
        r.Dev.StartTemperatureUpdate();
        Assert.Equal(new[]
        {
            Key(0x0008, 0x81), Key(0x000B, 0x92), Key(0x0087, 0x21), Key(0x0086, 0x01), Key(0x0087, 0x80),
            Key(0x0008, 0x09), Key(0x000B, 0x00),
        }, r.Fake.WriteKeys());
    }

    // ── calibrations (VL53L4CD_calibration.c) ────────────────────────────────

    [Fact]
    public void OffsetCalibrationIsTargetMinusTheMeanProgrammedX4()
    {
        using var r = new Rig().Ready();
        r.Fake.SetResultBlock(L4Block); // 624 mm
        Assert.Equal(-24, r.Dev.CalibrateOffset(600, 5));
        Assert.Equal(-96 & 0xFFFF, r.Fake.Word(0x001E));
        Assert.Equal(-24, r.Dev.GetOffsetMm());
        // offsets zeroed first, 10 heat frames + 5 samples each released
        var keys = r.Fake.WriteKeys();
        Assert.Equal(new[] { Key(0x001E, 0, 0), Key(0x0020, 0, 0), Key(0x0022, 0, 0) }, keys.Take(3));
        Assert.Equal(15, r.Fake.Writes().Count(w => w.Addr == 0x0086));
    }

    [Fact]
    public void XtalkCalibrationFromTheValidSamplesAfterTheFirst()
    {
        using var r = new Rig().Ready();
        r.Fake.SetResultBlock(L4Block); // 624 mm, 5160 kcps, 211 SPADs
        Assert.Equal(1, r.Dev.CalibrateXtalk(650, 5));
        // (1 - 624/650) × 5160/211 = 0.978 kcps → ×512
        Assert.Equal(500, r.Fake.Word(0x0016));
    }

    [Fact]
    public void XtalkCalibrationFailsWithNoValidSampleAndPast127Kcps()
    {
        using var r = new Rig().Ready();
        var bad = (byte[])L4Block.Clone();
        bad[0] = 0x04; // raw 4 → status 2, signal below threshold
        r.Fake.SetResultBlock(bad);
        Assert.Contains("no valid samples", Assert.Throws<Vl53lxException>(() => r.Dev.CalibrateXtalk(650, 5)).Message);
        var hot = (byte[])L4Block.Clone();
        hot[3] = 0x01; hot[4] = 0x00; hot[5] = 0xFF; hot[6] = 0xFF; // 1 SPAD, 524280 kcps
        r.Fake.SetResultBlock(hot);
        Assert.Contains("> 127", Assert.Throws<Vl53lxException>(() => r.Dev.CalibrateXtalk(5000, 5)).Message);
    }

    [Fact]
    public void CalibrationArgumentsOutOfRangeAreRefusedBeforeAnyTraffic()
    {
        using var r = new Rig().Ready();
        Assert.Contains("5..255", Assert.Throws<Vl53lxException>(() => r.Dev.CalibrateOffset(600, 4)).Message);
        Assert.Contains("10..1000", Assert.Throws<Vl53lxException>(() => r.Dev.CalibrateOffset(1001)).Message);
        Assert.Contains("10..5000", Assert.Throws<Vl53lxException>(() => r.Dev.CalibrateXtalk(5001)).Message);
        Assert.Throws<Vl53lxException>(() => r.Dev.CalibrateXtalk(600, 256));
        Assert.Equal(0, r.Fake.RxCount());
    }

    // ── VL53L1X ULD: modes and tabulated budgets ─────────────────────────────

    [Fact]
    public void L1ModesBudgetChoicesThatMoveWithTheModeAndSnap()
    {
        using var r = new Rig(L1cxBoard).Ready();
        Assert.Equal(new[] { "long", "short" }, r.Dev.Modes);
        Assert.Equal("long", r.Dev.GetMode());
        Assert.Equal(new[] { 20, 33, 50, 100, 200, 500 }, r.Dev.BudgetChoices());
        Assert.Equal((33, 500), (r.Dev.SnapBudget(40), r.Dev.SnapBudget(1000)));
        r.Dev.Configure(mode: "short", budgetMs: 15);
        Assert.Equal("short", r.Dev.GetMode());
        Assert.Equal(15, r.Dev.BudgetChoices()[0]);
        Assert.Equal((15, 15), r.Dev.GetRangeTiming());
        Assert.Equal(1388, r.Dev.IdentifyProduct()["driver_reach_mm"]);
    }

    [Fact]
    public void L1ABudgetTheModeDoesNotHaveIsRefusedNotRounded()
    {
        using var r = new Rig(L1cxBoard).Ready();
        Assert.Contains("pick one of 20, 33, 50, 100, 200, 500",
            Assert.Throws<Vl53lxException>(() => r.Dev.Configure(budgetMs: 40)).Message);
        // a mode name is an argument: refused as ArgumentException (the Python Vl53Error), nothing written
        Assert.Contains("no such mode", Assert.Throws<ArgumentException>(() => r.Dev.Configure(mode: "medium")).Message);
    }

    [Fact]
    public void L1RoiSizeAndCentre()
    {
        using var r = new Rig(L1cxBoard).Ready();
        r.Dev.SetRoi(8, 6);
        Assert.Equal(0x91, r.Fake.Regs[0x007F]); // the optical centre the die reports
        Assert.Equal(0x57, r.Fake.Regs[0x0080]);
        Assert.Equal((8, 6), r.Dev.GetRoi());
        r.Dev.SetRoi(16, 16); // over 10: the centre moves to 199
        Assert.Equal(199, r.Dev.GetRoiCenter());
        r.Dev.SetRoiCenter(0x88);
        Assert.Equal(0x88, r.Dev.GetRoiCenter());
    }

    [Fact]
    public void L1StreamsDecodeWithTheL1Variant()
    {
        using var r = new Rig(L1cxBoard).Ready();
        r.Dev.StartRanging();
        Later(() => r.Fake.Stream(L1Block, 1));
        var m = r.Dev.GetMeasurement(TimeSpan.FromSeconds(1));
        Assert.Equal((576, 0, 3.0, 16072.0, 2552.0, 67.0), (m.DistanceMm, m.Status, m.SigmaMm, m.SignalKcps, m.AmbientKcps, m.Spads));
        Assert.Equal(new Dictionary<string, object?>
        {
            ["signal_per_spad_kcps"] = 23L, ["ambient_per_spad_kcps"] = 3L, ["stream_count"] = 1L,
        }, m.Extra);
    }

    // ── ranging ──────────────────────────────────────────────────────────────

    [Fact]
    public void StartArmsTheStreamWithTheDieBlockAndItsRelease()
    {
        using var r = new Rig().Ready();
        r.Dev.StartRanging();
        Assert.Equal(new[] { Key(0x0087, 0x21) }, r.Fake.WriteKeys()); // continuous
        Assert.Equal(new[] { StartStreamDie }, r.Fake.Sent(StartStream).Select(FakeVl53lxBridge.ToHex));
        Assert.True(r.Dev.Ranging);
    }

    [Fact]
    public void EachStreamedBlockIsDecodedOnceToEverySubscriber()
    {
        using var r = new Rig().Ready();
        var seen = new List<Vl53lxMeasurement>();
        var unsub = r.Dev.OnMeasurement(seen.Add);
        using var q = r.Dev.Measurements();
        r.Dev.StartRanging();
        r.Fake.Stream(L4Block, 123_456);
        r.Fake.Stream(L4BlockF9, 123_789);
        r.Settle();
        Assert.Equal(new ulong[] { 123_456, 123_789 }, seen.Select(m => m.TimestampUs));
        var m = seen[0];
        Assert.Equal((624, 0, "valid"), (m.DistanceMm, m.Status, m.StatusText));
        Assert.Equal((5160.0, 1096.0, 2.0, 211.0), (m.SignalKcps, m.AmbientKcps, m.SigmaMm, m.Spads));
        Assert.Empty(m.Targets);
        Assert.Null(m.Bins);
        Assert.Equal(new Dictionary<string, object?>
        {
            ["signal_per_spad_kcps"] = 24L, ["ambient_per_spad_kcps"] = 5L, ["stream_count"] = 1L,
        }, m.Extra);
        Assert.True(m.Valid);
        Assert.True(m.Plottable);
        Assert.Equal(623, seen[1].DistanceMm);
        Assert.Equal(8L, seen[1].Extra["stream_count"]);
        Assert.Equal(new[] { 624, 623 }, new[] { q.Take(Short).DistanceMm, q.Take(Short).DistanceMm });
        unsub.Dispose();
        r.Fake.Stream(L4Block, 1);
        r.Settle();
        Assert.Equal(2, seen.Count);
    }

    [Fact]
    public void ANonzeroStatusIsNotValidBut6IsPlottable()
    {
        using var r = new Rig().Ready();
        r.Dev.StartRanging();
        var blk = (byte[])L4Block.Clone();
        blk[0] = 0x13; // raw 19 → status 6
        Later(() => r.Fake.Stream(blk, 1));
        var m = r.Dev.GetMeasurement(TimeSpan.FromSeconds(1));
        Assert.Equal((6, false, true), (m.Status, m.Valid, m.Plottable));
    }

    [Fact]
    public void ABlockTheDriverCannotDecodeIsCountedAndTheReaderSurvives()
    {
        using var r = new Rig().Ready();
        r.Dev.StartRanging();
        r.Fake.Stream(L4Block.Take(5).ToArray(), 1);
        r.Settle();
        Assert.Equal(1, r.Dev.StreamParseErrors);
        Later(() => r.Fake.Stream(L4Block, 2));
        Assert.Equal(624, r.Dev.GetMeasurement(TimeSpan.FromSeconds(1)).DistanceMm);
    }

    [Fact]
    public void AMalformedStreamReportIsCountedAndDropped()
    {
        // The length field says 17, only 5 bytes follow (contract 07 §3).
        using var r = new Rig().Ready();
        r.Dev.StartRanging();
        r.Fake.Stream(L4Block.Take(5).ToArray(), 1, 0x0089, 17);
        r.Settle();
        Assert.Equal(1, r.Dev.StreamParseErrors);
        Later(() => r.Fake.Stream(L4Block, 2));
        Assert.Equal(624, r.Dev.GetMeasurement(TimeSpan.FromSeconds(1)).DistanceMm);
        Assert.Equal(0, r.Dev.BridgeInfo().AddrWidth); // requests still answered
    }

    [Fact]
    public void AStreamReportShorterThanItsHeaderIsNotAMeasurement()
    {
        using var r = new Rig().Ready();
        var seen = new List<Vl53lxMeasurement>();
        using var unsub = r.Dev.OnMeasurement(seen.Add);
        r.Dev.StartRanging();
        r.Fake.Send((int)Vl53lxRpt.Stream, new byte[11]);
        r.Settle();
        Assert.Empty(seen);
        Assert.Equal(0, r.Dev.StreamParseErrors);
    }

    [Fact]
    public void AStreamReportBeforeInitIsSwallowedNotDecoded()
    {
        using var r = new Rig();
        var seen = new List<Vl53lxMeasurement>();
        using var unsub = r.Dev.OnMeasurement(seen.Add);
        r.Fake.Stream(L4Block, 1);
        r.Settle();
        Assert.Empty(seen);
        Assert.Equal(0, r.Dev.StreamParseErrors);
    }

    [Fact]
    public void MeasurementsIsBoundedDropOldestAndDeregistersOnClose()
    {
        using var r = new Rig().Ready();
        var q = r.Dev.Measurements(capacity: 1);
        Assert.Equal(new long[] { 0 }, r.Dev.StreamDroppedCounts);
        r.Dev.StartRanging();
        r.Fake.Stream(L4Block, 1);
        r.Fake.Stream(L4Block, 2);
        r.Fake.Stream(L4BlockF9, 3);
        r.Settle();
        Assert.Equal(new long[] { 2 }, r.Dev.StreamDroppedCounts);
        Assert.Equal(3UL, q.Take(Short).TimestampUs);
        q.Dispose();
        Assert.Empty(r.Dev.StreamDroppedCounts);
    }

    [Fact]
    public void GetMeasurementTimesOutAndLeavesNoQueue()
    {
        using var r = new Rig().Ready();
        Assert.Throws<DepzTimeoutException>(() => r.Dev.GetMeasurement(TimeSpan.FromMilliseconds(20)));
        Assert.Empty(r.Dev.StreamDroppedCounts);
    }

    [Fact]
    public void GetMeasurementEndsWhenTheDeviceCloses()
    {
        using var r = new Rig().Ready();
        Later(() => r.Dev.Close());
        Assert.Contains("device closed", Assert.Throws<LinkClosedException>(() => r.Dev.GetMeasurement(TimeSpan.FromSeconds(2))).Message);
        Assert.Contains("device is closed", Assert.Throws<LinkClosedException>(() => r.Dev.GetMeasurement(TimeSpan.FromSeconds(2))).Message);
    }

    [Fact]
    public void ConfigurationIsRefusedWhileTheStreamOwnsTheBus()
    {
        using var r = new Rig().Ready();
        r.Dev.StartRanging();
        r.Fake.ClearRx();
        var calls = new Action[]
        {
            () => r.Dev.Init(), () => r.Dev.Configure(), () => r.Dev.SetOffsetMm(1), () => r.Dev.SetXtalkKcps(1),
            () => r.Dev.CalibrateOffset(100), () => r.Dev.CalibrateXtalk(100), () => r.Dev.SetDetectionThresholds(1, 2, 3),
            () => r.Dev.SetSignalThresholdKcps(1), () => r.Dev.SetSigmaThresholdMm(1), () => r.Dev.SetRoi(4, 4),
            () => r.Dev.SetRoiCenter(1), () => r.Dev.StartTemperatureUpdate(), () => r.Dev.PerformRefSpadManagement(),
            () => r.Dev.StartRanging(), () => r.Dev.MeasureOnce(),
        };
        foreach (var call in calls)
            Assert.Contains("StopRanging() first", Assert.Throws<InvalidStateException>(call).Message);
        Assert.Equal(0, r.Fake.RxCount());
        r.Dev.BridgeInfo(); // touches no sensor register: allowed while streaming
    }

    [Fact]
    public void StopRangingStopsTheStreamThenTheSensor()
    {
        using var r = new Rig().Ready();
        r.Dev.StopRanging(); // not ranging: no-op
        Assert.Equal(0, r.Fake.RxCount());
        r.Dev.StartRanging();
        r.Fake.ClearRx();
        r.Dev.StopRanging();
        Assert.Equal(new[] { StopStream, WriteReg }, r.Fake.Cmds());
        Assert.Equal(new[] { Key(0x0087, 0x80) }, r.Fake.WriteKeys());
        Assert.False(r.Dev.Ranging);
    }

    [Fact]
    public void AFailedStopStreamStillStopsTheSensorAndClearsTheFlag()
    {
        using var r = new Rig().Ready();
        r.Dev.StartRanging();
        r.Fake.ClearRx();
        r.Fake.FailNext(StopStream, (int)Status.Error);
        Assert.Throws<StatusException>(() => r.Dev.StopRanging());
        Assert.False(r.Dev.Ranging);
        Assert.Equal(new[] { Key(0x0087, 0x80) }, r.Fake.WriteKeys());
        r.Dev.SetOffsetMm(0); // configuration allowed again
    }

    [Fact]
    public void ARefusedStartStreamStopsTheSensorAgain()
    {
        using var r = new Rig().Ready();
        r.Fake.FailNext(StartStream, (int)Status.ErrPayloadFormat);
        var e = Assert.Throws<StatusException>(() => r.Dev.StartRanging());
        Assert.Equal((int)Status.ErrPayloadFormat, e.Status); // the original error, not one from the stop
        Assert.False(r.Dev.Ranging);
        // the driver started the die, so it is stopped again
        var keys = r.Fake.WriteKeys();
        Assert.Equal(Key(0x0087, 0x21), keys[0]);
        Assert.Equal(Key(0x0087, 0x80), keys[^1]);
        r.Dev.SetOffsetMm(0); // configuration allowed again
    }

    // ── MeasureOnce (poll) ───────────────────────────────────────────────────

    [Fact]
    public void MeasureOnceStartsWaitsReadsReleasesStops()
    {
        using var r = new Rig().Ready();
        r.Fake.SetResultBlock(L4Block);
        var m = r.Dev.MeasureOnce();
        Assert.Equal((624, true), (m.DistanceMm, m.Valid));
        Assert.Null(m.Bins);
        // the timestamp of the last register read: the block itself
        Assert.Equal((ulong)(r.Fake.McuTimeUs - 100), m.TimestampUs);
        Assert.Contains("89001100", r.Fake.Sent(ReadReg).Select(FakeVl53lxBridge.ToHex));
        var keys = r.Fake.WriteKeys();
        Assert.Equal(Key(0x0087, 0x21), keys[0]);
        Assert.Equal(new[] { Key(0x0086, 0x01), Key(0x0087, 0x80) }, keys.TakeLast(2));
        Assert.Empty(r.Fake.Sent(StartStream));
    }

    [Fact]
    public void ADataReadyTimeoutStillStopsTheSensor()
    {
        using var r = new Rig().Ready();
        r.Fake.Regs[0x0031] = 0x01; // INT never reaches the active level
        var e = Assert.Throws<Vl53lxException>(() => r.Dev.MeasureOnce(TimeSpan.FromMilliseconds(30)));
        Assert.Contains("timeout waiting for data ready", e.Message);
        Assert.Equal(Key(0x0087, 0x80), r.Fake.WriteKeys()[^1]);
        Assert.False(r.Dev.Ranging);
    }
}
