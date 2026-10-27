using System.Buffers.Binary;
using Depz.Sensor.Vl53lx;
using Xunit;

namespace Depz.Sensor.Tests;

/// <summary>
/// The register behaviour of a VL53L0X die — the C# twin of the Python fake <c>FakeVl53l0xDie</c>
/// (tests/fake_vl53l0x.py), here as the ULD's platform itself rather than behind the bridge fake. One 256-byte
/// bank per page (register 0xFF selects it), with the registers that behave modelled on top: the soft reset and
/// the NACK after it, the NVM strobe and data, the single-shot start bit, data-ready and interrupt clear, the
/// result block of each shot and the reference-signal measurement the SPAD management converges on. The defaults
/// are what the live board (TB9BGETA6M) answered in vectors/recordings/vl53l0x_uld_long-range_33ms.depzrec.
/// </summary>
internal sealed class FakeVl53l0xDie : Vl53lxPlatform
{
    public const int ModelIdL0x = 0xEE;
    private const int StartSelect = 0xB4;      // GLOBAL_CONFIG_REF_EN_START_SELECT
    private const int FirstApertureSpad = 192; // SPAD indices from here lie in an aperture quadrant

    /// <summary>A flat target at 635 mm (status 0x5A: internal 11, 0x06C1 effective SPADs in 8.8, signal 0x022B
    /// and ambient 0x011B in 9.7).</summary>
    public static readonly byte[] Frame635Mm = Convert.FromHexString("5a06c1040071022b011b027b");

    public static readonly IReadOnlyDictionary<int, uint> NvmDefaults = new Dictionary<int, uint>
    {
        [0x6B] = 0x1D80060B, // reference SPADs: (>>8)&0x7F = 6, (>>15)&1 = 0 (non-aperture)
        [0x24] = 0xFBFFFFDF, // good SPAD map bytes 0..3
        [0x25] = 0xB70F0000, // good SPAD map bytes 4..5
        [0x02] = 0x2A000000, // module id
        [0x7B] = 0x0C345678, // revision / part UID upper
        [0x7C] = 0x9ABCDEF0, // part UID lower
        [0x73] = 0x000000A5, // signal rate at 400 mm, high byte
        [0x74] = 0x3C000000, // ... low byte
        [0x75] = 0x00000019, // distance at 400 mm (1/16 mm), high byte
        [0x76] = 0x40000000, // ... low byte
    };

    public readonly Dictionary<int, byte[]> Banks = new();
    public readonly Dictionary<int, uint> Nvm = new(NvmDefaults);

    // knobs
    public readonly Queue<byte[]> Frames = new(); // result block per ranging shot; the last one repeats
    public byte[] Frame = Frame635Mm;
    public int RefRatePerSpad = 400;          // 9.7 Mcps each enabled non-aperture reference SPAD adds
    public int RefRatePerApertureSpad = 100;  // ... each aperture one
    public bool SpadMapStuck;                 // writes to the SPAD enables are lost

    // what happened
    public readonly List<(int Start, int Sequence)> Shots = new();
    public readonly List<int> RefPeaks = new();

    private int _page;
    private bool _inReset;
    private int _nacks;
    private int? _readyIn; // polls until ready; null = nothing pending

    public FakeVl53l0xDie(IReadOnlyDictionary<int, uint>? nvm = null)
    {
        if (nvm is not null)
            foreach (var kv in nvm)
                Nvm[kv.Key] = kv.Value;
        var b0 = Bank(0);
        b0[0xC0] = ModelIdL0x;
        b0[0x84] = 0x01;          // GPIO_HV_MUX_ACTIVE_HIGH as read on the board
        b0[0xCB] = 0x22;          // VHV settings after the reference calibration
        b0[0xEE] = 0x01;          // phase cal
        b0[0xF9] = 0x28;          // OSC_CALIBRATE_VAL 0x0028
        Frame.CopyTo(b0, 0x14);
        var b1 = Bank(1);
        b1[0x91] = 0x3C;          // the stop variable DataInit reads
        b1[0x84] = 0x97;          // oscillator frequency, FixPoint4.12
        b1[0x85] = 0x5C;
        Bank(6)[0x83] = 0x01;
    }

    public byte[] Bank(int page)
    {
        if (!Banks.TryGetValue(page, out var b))
            Banks[page] = b = new byte[256];
        return b;
    }

    public byte[] SpadEnables() => Bank(0).AsSpan(0xB0, 6).ToArray();

    public int SequenceConfig => Bank(0)[0x01];

    public static byte[] ResultFrame(int deviceStatus, int distance = 635, int signal97 = 0x022B, int ambient97 = 0x011B,
        int spads88 = 0xC104)
    {
        var f = new byte[12];
        f[0] = (byte)deviceStatus;
        f[1] = 0x06;
        BinaryPrimitives.WriteUInt16BigEndian(f.AsSpan(2), (ushort)spads88);
        BinaryPrimitives.WriteUInt16BigEndian(f.AsSpan(4), 0x0071);
        BinaryPrimitives.WriteUInt16BigEndian(f.AsSpan(6), (ushort)signal97);
        BinaryPrimitives.WriteUInt16BigEndian(f.AsSpan(8), (ushort)ambient97);
        BinaryPrimitives.WriteUInt16BigEndian(f.AsSpan(10), (ushort)distance);
        return f;
    }

    public override byte[] ReadMulti(int addr, int size)
    {
        var bank = Bank(_page);
        if (_page == 0 && addr == 0xC0)
        {
            if (_nacks > 0)
            {
                _nacks--;
                throw new Vl53lxProtocolException("READ_REG 0x00C0: NACK");
            }
            return new[] { _inReset ? (byte)0 : (byte)ModelIdL0x }.Concat(bank.AsSpan(0xC1, size - 1).ToArray()).ToArray();
        }
        if (_page == 7 && addr == 0x83)
            return new byte[] { 0x10 };
        if (_page == 7 && addr == 0x90)
        {
            var w = new byte[4];
            BinaryPrimitives.WriteUInt32BigEndian(w, Nvm.TryGetValue(bank[0x94], out uint v) ? v : 0);
            return w[..size];
        }
        if (_page == 0 && addr == 0x00)
            return new[] { (byte)(bank[0x00] & 0xFE) };
        if (_page == 0 && addr == 0x13)
        {
            bool ready = _readyIn == 0;
            if (_readyIn > 0)
                _readyIn--;
            return new[] { (byte)(0x40 | (ready ? 0x04 : 0)) };
        }
        return bank.AsSpan(addr, size).ToArray();
    }

    public override void WriteMulti(int addr, byte[] data)
    {
        if (addr == 0xFF)
        {
            _page = data[0];
            return;
        }
        var bank = Bank(_page);
        if (_page == 0 && addr == 0xBF)
        {
            if (data[0] == 0x00)
            {
                _inReset = true;
            }
            else
            {
                _inReset = false;
                _nacks = 1;
            }
        }
        else if (_page == 0 && addr == 0x00 && (data[0] & 0x01) != 0)
        {
            Shot(data[0]);
        }
        else if (_page == 0 && addr == 0x0B && data[0] == 0x01)
        {
            _readyIn = null;
        }
        else if (_page == 0 && addr >= 0xB0 && addr <= 0xB5 && SpadMapStuck)
        {
            return;
        }
        data.CopyTo(bank, addr);
    }

    public override void SetI2cSpeed(int khz) { }

    public override void SetAddrWidth(int width) { }

    public override void XshutReset() { }

    public override void SleepMs(int ms) { }

    private void Shot(int value)
    {
        int sequence = Bank(0)[0x01];
        Shots.Add((value, sequence));
        _readyIn = 0;
        if (sequence == 0xC0)
        {
            // Reference-signal measurement: the peak reference rate follows the SPAD enables.
            int peak = RefPeak();
            RefPeaks.Add(peak);
            BinaryPrimitives.WriteUInt16BigEndian(Bank(1).AsSpan(0xB6), (ushort)peak);
        }
        else if (sequence != 0x01 && sequence != 0x02) // VHV / phase shots range nothing
        {
            if (Frames.Count > 0)
                Frame = Frames.Dequeue();
            Frame.CopyTo(Bank(0), 0x14);
        }
    }

    private int RefPeak()
    {
        var enables = SpadEnables();
        int peak = 0;
        for (int index = 0; index < 8 * enables.Length; index++)
            if (((enables[index / 8] >> (index % 8)) & 1) != 0)
                peak += StartSelect + index >= FirstApertureSpad ? RefRatePerApertureSpad : RefRatePerSpad;
        return Math.Min(peak, 0xFFFF);
    }
}

/// <summary>The VL53L0X ULD port over <see cref="FakeVl53l0xDie"/>, no hardware — the port of the Python
/// test_vl53l0x_uld.py cases the capture cannot reach: the reference-SPAD management and its convergence cases,
/// the offset and crosstalk registers and calibrations, and their error paths. Expected values are worked out
/// from ST's vl53l0x_api*.c in the comments of the Python test.</summary>
public class Vl53l0xUldTests
{
    private static (Vl53l0xUldDriver, FakeVl53l0xDie) Ready(IReadOnlyDictionary<int, uint>? nvm = null)
    {
        var die = new FakeVl53l0xDie(nvm);
        var drv = new Vl53l0xUldDriver(die, "VL53L0X");
        drv.SensorInit();
        return (drv, die);
    }

    // ── init and the reference SPADs ─────────────────────────────────────────

    [Fact]
    public void InitTakesTheReferenceSpadsFromTheNvm()
    {
        var (drv, die) = Ready();
        Assert.True(drv.RefSpadsFromNvm);
        Assert.Equal((6, 0), drv.GetReferenceSpads());
        // the 6 non-aperture SPADs through the good map FB..: indices 0,1,3,4,5,6 (index 2 is bad)
        Assert.Equal(Convert.FromHexString("7b0000000000"), die.SpadEnables());
        Assert.Equal(new[] { (0x41, 0x01), (0x01, 0x02) }, die.Shots); // the VHV / phase shots, in order
        Assert.Equal(0xE8, die.SequenceConfig); // TCC and MSRC off, as StaticInit leaves it
    }

    [Theory]
    [InlineData((0 << 15) | (13 << 8))]
    [InlineData((1 << 15) | (33 << 8))]
    public void AnInvalidNvmRecordMakesInitMeasureTheReferenceSpads(int record)
    {
        var (drv, die) = Ready(new Dictionary<int, uint> { [0x6B] = (uint)record });
        Assert.False(drv.RefSpadsFromNvm);
        // 400 per SPAD against 2560: 3 -> 1200, ..., 6 -> 2400 (160 short), 7 -> 2800 (240 over): the 6-SPAD map
        Assert.Equal(new[] { 1200, 1600, 2000, 2400, 2800 }, die.RefPeaks);
        Assert.Equal((6, 0), drv.GetReferenceSpads());
        Assert.Equal(Convert.FromHexString("7b0000000000"), die.SpadEnables());
        Assert.Equal(0xE8, die.SequenceConfig); // it ran after the rest of StaticInit, the sequence restored
    }

    [Fact]
    public void AReferenceSpadMapThatDoesNotReadBackFailsInit()
    {
        var die = new FakeVl53l0xDie { SpadMapStuck = true };
        var e = Assert.Throws<Vl53lxException>(() => new Vl53l0xUldDriver(die, "VL53L0X").SensorInit());
        Assert.Contains("did not read back", e.Message);
    }

    private static (int, int) Manage(Vl53l0xUldDriver drv, FakeVl53l0xDie die)
    {
        die.RefPeaks.Clear();
        return Assert.IsType<(int, int)>(drv.PerformRefSpadManagement());
    }

    [Fact]
    public void ManagementKeepsTheMapThatOvershootsLess()
    {
        var (drv, die) = Ready();
        die.RefRatePerSpad = 700;
        // 3 -> 2100 (460 short), 4 -> 2800 (240 over, closer): keep 4
        Assert.Equal((4, 0), Manage(drv, die));
        Assert.Equal(new[] { 2100, 2800 }, die.RefPeaks);
        Assert.Equal(Convert.FromHexString("1b0000000000"), die.SpadEnables()); // indices 0, 1, 3, 4
        Assert.Equal((4, 0), drv.GetReferenceSpads());
    }

    [Fact]
    public void ManagementStopsOnTheTargetExactly()
    {
        var (drv, die) = Ready();
        die.RefRatePerSpad = 640;
        Assert.Equal((4, 0), Manage(drv, die));
        Assert.Equal(new[] { 1920, 2560 }, die.RefPeaks);
    }

    [Fact]
    public void ManagementStopsAtTheEdgeOfTheNonApertureQuadrant()
    {
        var (drv, die) = Ready();
        die.RefRatePerSpad = 100;
        // 11 good non-aperture SPADs (index 2 is bad) make 1100; the next good one is an aperture SPAD
        Assert.Equal((11, 0), Manage(drv, die));
        Assert.Equal(1100, die.RefPeaks[^1]);
        Assert.Equal(Convert.FromHexString("fb0f00000000"), die.SpadEnables());
    }

    [Fact]
    public void ManagementSwitchesToApertureSpadsWhenThreeAreTooBright()
    {
        var (drv, die) = Ready();
        die.RefRatePerSpad = 1000;
        die.RefRatePerApertureSpad = 100;
        // 3 non-aperture -> 3000 > 2560; restart at index 12 on aperture SPADs:
        // 3 -> 300 ... 25 -> 2500 (60 short), 26 -> 2600 (40 over, closer): 26
        Assert.Equal((26, 1), Manage(drv, die));
        Assert.Equal(new[] { 3000, 300 }, die.RefPeaks.Take(2));
        Assert.Equal(new[] { 2500, 2600 }, die.RefPeaks.Skip(die.RefPeaks.Count - 2));
        var enables = die.SpadEnables();
        Assert.Equal(0, enables[0]);
        Assert.Equal(0, enables[1] & 0x0F);
        Assert.Equal(26, enables.Sum(b => System.Numerics.BitOperations.PopCount(b)));
        Assert.Equal((26, 1), drv.GetReferenceSpads());
    }

    [Fact]
    public void ManagementSettlesForThreeApertureSpadsWhenEvenThoseAreTooBright()
    {
        var (drv, die) = Ready();
        die.RefRatePerSpad = 1000;
        die.RefRatePerApertureSpad = 1000;
        Assert.Equal((3, 1), Manage(drv, die));
        Assert.Equal(new[] { 3000, 3000 }, die.RefPeaks);
        Assert.Equal(Convert.FromHexString("007000000000"), die.SpadEnables()); // indices 12, 13, 14
    }

    [Theory]
    [InlineData(0x0B000000u, "ran out of good reference SPADs")]       // indices 0, 1, 3 only
    [InlineData(0x03000000u, "ran out of good reference SPADs")]       // fewer than the minimum of three
    [InlineData(0x0000FF00u, "leaves the requested quadrant")]         // aperture SPADs only
    public void ManagementNeedsTheMinimumFromTheGoodMap(uint goodMap03, string message)
    {
        // An invalid NVM SPAD record: init itself runs the management over this good map.
        var die = new FakeVl53l0xDie(new Dictionary<int, uint> { [0x6B] = 13 << 8, [0x24] = goodMap03, [0x25] = 0 });
        var e = Assert.Throws<Vl53lxException>(() => new Vl53l0xUldDriver(die, "VL53L0X").SensorInit());
        Assert.Contains(message, e.Message);
    }

    [Fact]
    public void ManagementWhoseMapDoesNotReadBack()
    {
        var (drv, die) = Ready();
        die.SpadMapStuck = true;
        var e = Assert.Throws<Vl53lxException>(() => drv.PerformRefSpadManagement());
        Assert.Contains("did not read back", e.Message);
    }

    [Fact]
    public void ThePublicRemeasureRearmsTheStopVariableAndRestoresTheSequence()
    {
        var (drv, die) = Ready();
        drv.StartRanging();
        drv.StopRanging();
        die.Shots.Clear();
        Assert.Equal((6, 0), Manage(drv, die));
        // the reference measurements run on the reference steps alone (0xC0), after a VHV / phase calibration
        Assert.Equal(new[] { (0x41, 0x01), (0x01, 0x02) }.Concat(Enumerable.Repeat((0x01, 0xC0), 5)), die.Shots);
        Assert.Equal(0xE8, die.SequenceConfig);
        Assert.Equal(Convert.FromHexString("7b0000000000"), die.SpadEnables()); // the 6-SPAD map back after the 7th
        Assert.Equal(0x3C, die.Bank(1)[0x91]);
    }

    // ── offset ───────────────────────────────────────────────────────────────

    [Theory]
    [InlineData(0, 0x0000, 0)]
    [InlineData(35_000, 140, 35_000)]
    [InlineData(-35_000, 4096 - 140, -35_000)]
    [InlineData(511_000, 2044, 511_000)]
    [InlineData(600_000, 2044, 511_000)]       // clamped to 511000
    [InlineData(-512_000, 2048, -512_000)]
    [InlineData(-700_000, 2048, -512_000)]     // clamped to -512000
    [InlineData(-1_249, 4096 - 4, -1_000)]     // truncated toward zero
    public void TheOffsetRegisterIsQuarterMillimetresInTwosComplement(long um, int reg, long back)
    {
        var (drv, die) = Ready();
        drv.SetOffsetUm(um);
        Assert.Equal(reg, BinaryPrimitives.ReadUInt16BigEndian(die.Bank(0).AsSpan(0x28)));
        Assert.Equal(back, drv.GetOffsetUm());
    }

    [Fact]
    public void OffsetInWholeMillimetres()
    {
        var (drv, die) = Ready();
        drv.SetOffset(-7);
        Assert.Equal(-7, drv.GetOffset());
        Assert.Equal(4096 - 28, BinaryPrimitives.ReadUInt16BigEndian(die.Bank(0).AsSpan(0x28)));
    }

    [Fact]
    public void OffsetCalibrationAgainstATarget()
    {
        var (drv, die) = Ready();
        drv.SetOffsetUm(12_000);
        foreach (var f in new[] { FakeVl53l0xDie.ResultFrame(0x22), FakeVl53l0xDie.ResultFrame(0x22),
                     FakeVl53l0xDie.Frame635Mm, FakeVl53l0xDie.ResultFrame(0x5A, distance: 636) })
            die.Frames.Enqueue(f);
        // three valid of five samples (the last one repeats): mean (635+636+636)/3 = 635.67 -> 636
        Assert.Equal(-36, drv.CalibrateOffset(600, nbSamples: 5));
        Assert.Equal(-36_000, drv.GetOffsetUm());
        Assert.Equal(0xE8, die.SequenceConfig); // TCC was off: nothing to put back
    }

    [Fact]
    public void OffsetCalibrationErrors()
    {
        var (drv, die) = Ready();
        Assert.Equal(0, drv.PerformOffsetCalibration(635, nbSamples: 2));
        Assert.Contains("distance must be positive", Assert.Throws<Vl53lxException>(() => drv.PerformOffsetCalibration(0)).Message);
        die.Frame = FakeVl53l0xDie.ResultFrame(0x22);
        Assert.Contains("no valid measurement",
            Assert.Throws<Vl53lxException>(() => drv.PerformOffsetCalibration(400, nbSamples: 3)).Message);
        die.Frame = FakeVl53l0xDie.ResultFrame(0x5A, distance: 0);
        Assert.Contains("mean range came out 0 mm",
            Assert.Throws<Vl53lxException>(() => drv.PerformOffsetCalibration(400, nbSamples: 3)).Message);
    }

    // ── crosstalk ────────────────────────────────────────────────────────────

    [Fact]
    public void TheXtalkRegisterIs3Point13()
    {
        var (drv, die) = Ready();
        drv.SetXtalk(297);
        long rate = (297L << 16) / 1000;
        Assert.Equal(rate >> 3, BinaryPrimitives.ReadUInt16BigEndian(die.Bank(0).AsSpan(0x20)));
        Assert.Equal((int)((((rate >> 3) << 3) * 1000) >> 16), drv.GetXtalk());
    }

    [Fact]
    public void XtalkCalibrationAgainstATargetBehindTheGlass()
    {
        var (drv, die) = Ready();
        die.Frames.Enqueue(FakeVl53l0xDie.ResultFrame(0x22));
        die.Frames.Enqueue(FakeVl53l0xDie.Frame635Mm);
        // four valid samples of 635 mm, 284160 (16.16) signal, 193 SPADs at a 1000 mm target:
        // per SPAD 1472 * (65536 - 41615) = 35211712, rounded >> 16 = 537 (0.0082 Mcps) -> 8 kcps
        Assert.Equal(8, drv.CalibrateXtalk(1000, nbSamples: 5));
        Assert.Equal(537 >> 3, BinaryPrimitives.ReadUInt16BigEndian(die.Bank(0).AsSpan(0x20)));
        Assert.Equal(0xE8, die.SequenceConfig);
    }

    [Fact]
    public void XtalkCalibrationWithNothingToCompensate()
    {
        var (drv, die) = Ready();
        // target at the measured distance (or nearer): no crosstalk
        Assert.Equal(0, drv.PerformXtalkCalibration(635, nbSamples: 2));
        Assert.Equal(0, drv.PerformXtalkCalibration(600, nbSamples: 2));
        // no effective SPADs at all: the per-SPAD division is skipped
        die.Frame = FakeVl53l0xDie.ResultFrame(0x5A, spads88: 0x0010);
        Assert.Equal(0, drv.PerformXtalkCalibration(1000, nbSamples: 2));
        Assert.Equal(0, BinaryPrimitives.ReadUInt16BigEndian(die.Bank(0).AsSpan(0x20)));
    }

    [Fact]
    public void XtalkCalibrationErrors()
    {
        var (drv, die) = Ready();
        Assert.Contains("distance must be positive", Assert.Throws<Vl53lxException>(() => drv.PerformXtalkCalibration(-1)).Message);
        die.Frame = FakeVl53l0xDie.ResultFrame(0x22);
        Assert.Contains("no valid measurement",
            Assert.Throws<Vl53lxException>(() => drv.PerformXtalkCalibration(1000, nbSamples: 2)).Message);
    }
}
