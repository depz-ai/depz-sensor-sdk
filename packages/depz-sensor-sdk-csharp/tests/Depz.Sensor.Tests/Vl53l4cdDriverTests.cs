using Depz.Sensor.Devices;
using Depz.Sensor.Transport;
using Depz.Sensor.Vl53l4;
using Xunit;

namespace Depz.Sensor.Tests;

/// <summary>A plain register map seeded with what a booted VL53L4CD answers (model id, boot status,
/// oscillator words), so the driver's real register sequences run end to end. The default
/// configuration block leaves the sensor "data ready".</summary>
internal sealed class FakeVl53l4Platform : IVl53l4Platform
{
    public const int OscFrequencyTypical = 0x3980;
    public const int ClockPllTypical = 0x0A5C;

    public readonly Dictionary<int, byte> Reg = new();
    public readonly List<(int Addr, byte[] Data)> Writes = new();
    public readonly List<int> Speeds = new();

    public FakeVl53l4Platform()
    {
        Reg[Vl53l4Uld.FirmwareSystemStatus] = 0x03;
        Reg[Vl53l4Uld.GpioHvMuxCtrl] = 0x11; // as the configuration block leaves it: INT active low, data ready
        SeedWord(Vl53l4Uld.IdentificationModelId, Vl53l4Uld.ModelId);
        SeedWord(Vl53l4Uld.OscFrequency, OscFrequencyTypical);
        SeedWord(Vl53l4Uld.ResultOscCalibrateVal, ClockPllTypical);
    }

    public void SeedWord(int addr, int value)
    {
        Reg[addr] = (byte)(value >> 8);
        Reg[addr + 1] = (byte)value;
    }

    public byte[] ReadMulti(ushort addr, int size) =>
        Enumerable.Range(0, size).Select(i => Reg.TryGetValue(addr + i, out byte b) ? b : (byte)0).ToArray();

    public void WriteMulti(ushort addr, byte[] data)
    {
        Writes.Add((addr, data.ToArray()));
        for (int i = 0; i < data.Length; i++)
            Reg[addr + i] = data[i];
    }

    public void SleepMs(int ms) { }

    public void SetI2cSpeed(int khz) => Speeds.Add(khz);

    public List<byte[]> WrittenTo(int addr) => Writes.Where(w => w.Addr == addr).Select(w => w.Data).ToList();

    /// <summary>Make every result read return a valid measurement with these values.</summary>
    public void SeedResult(int distanceMm, int signalRaw, int spadsRaw)
    {
        int validRaw = Array.IndexOf(Vl53l4Uld.StatusRtn.ToArray(), (byte)0);
        Reg[Vl53l4Uld.ResultRangeStatus] = (byte)validRaw;
        SeedWord(Vl53l4Uld.ResultSpadNb, spadsRaw);
        SeedWord(Vl53l4Uld.ResultSignalRate, signalRaw);
        SeedWord(Vl53l4Uld.ResultDistance, distanceMm);
    }
}

public class Vl53l4cdDriverTests
{
    private static (Vl53l4cdDriver, FakeVl53l4Platform) Make()
    {
        var p = new FakeVl53l4Platform();
        return (new Vl53l4cdDriver(p), p);
    }

    private static int Word(byte[] b) => (b[0] << 8) | b[1];

    [Fact]
    public void InitRunsAtBootSpeedThenRetimes()
    {
        var (drv, p) = Make();
        drv.SensorInit();
        Assert.Equal(Vl53l4Uld.I2cKhzBoot, p.Speeds[0]);
        Assert.Equal(Vl53l4Uld.I2cKhzDefault, p.Speeds[^1]);

        var (drv2, p2) = Make();
        drv2.SensorInit(Vl53l4Uld.I2cKhzBoot);
        Assert.Equal(new[] { Vl53l4Uld.I2cKhzBoot }, p2.Speeds);
    }

    [Fact]
    public void InitWritesTheConfigBlockAndTheVhvSequence()
    {
        var (drv, p) = Make();
        drv.SensorInit();
        var block = p.WrittenTo(Vl53l4Uld.ConfigAddr)[0];
        Assert.Equal(Vl53l4Uld.ConfigBlock(), block);
        Assert.Equal(Vl53l4Uld.ConfigFmpByte, block[0]);
        Assert.Equal(new byte[] { 0x40, 0x80 }, p.WrittenTo(Vl53l4Uld.SystemStart).Take(2).Select(d => d[0]));
        Assert.Equal(new byte[] { 0x01 }, p.WrittenTo(Vl53l4Uld.SystemInterruptClear)[0]);
        Assert.Equal(new byte[] { 0x09 }, p.WrittenTo(Vl53l4Uld.VhvConfigTimeoutMacropLoopBound)[^1]);
        Assert.Equal(new byte[] { 0x00 }, p.WrittenTo(0x000B)[^1]);
        Assert.Equal(new byte[] { 0x05, 0x00 }, p.WrittenTo(0x0024)[^1]);
        Assert.Equal(0, drv.GetRangeTiming().InterMeasurementMs); // 50 ms continuous by default
    }

    [Fact]
    public void RangeTimingWritesWhatDecodesBack()
    {
        var (drv, p) = Make();
        drv.SetRangeTiming(50, 0);
        var (a, b, im) = Vl53l4Uld.RangeTimingRegisters(50, 0, FakeVl53l4Platform.OscFrequencyTypical);
        Assert.Equal(a, Word(p.WrittenTo(Vl53l4Uld.RangeConfigA)[^1]));
        Assert.Equal(b, Word(p.WrittenTo(Vl53l4Uld.RangeConfigB)[^1]));
        Assert.Equal(im, System.Buffers.Binary.BinaryPrimitives.ReadUInt32BigEndian(p.WrittenTo(Vl53l4Uld.IntermeasurementMs)[^1]));
        var (budget, inter) = drv.GetRangeTiming();
        Assert.Equal(0, inter);
        Assert.InRange(budget, 49, 51); // the C math loses up to 1 ms on the way back

        drv.StartRanging();
        Assert.Equal(new byte[] { 0x21 }, p.WrittenTo(Vl53l4Uld.SystemStart)[^1]); // continuous
        drv.StopRanging();
        Assert.Equal(new byte[] { 0x80 }, p.WrittenTo(Vl53l4Uld.SystemStart)[^1]);
    }

    [Fact]
    public void AutonomousModeStartsWith0x40()
    {
        var (drv, p) = Make();
        drv.SetRangeTiming(50, 1000);
        Assert.True(drv.GetRangeTiming().InterMeasurementMs > 0);
        drv.StartRanging();
        Assert.Equal(new byte[] { 0x40 }, p.WrittenTo(Vl53l4Uld.SystemStart)[^1]);
    }

    [Theory]
    [InlineData(9, 0)]
    [InlineData(201, 0)]
    [InlineData(50, 30)] // 0 < inter-measurement <= budget
    public void RangeTimingBounds(int budget, int inter)
    {
        var (drv, _) = Make();
        Assert.Throws<Vl53l4Exception>(() => drv.SetRangeTiming(budget, inter));
    }

    [Fact]
    public void SettingsRoundTrip()
    {
        var (drv, _) = Make();
        foreach (int mm in new[] { 10, -10, 0, 255 })
        {
            drv.SetOffset(mm);
            Assert.Equal(mm, drv.GetOffset());
        }
        drv.SetXtalk(20);
        Assert.Equal(20, drv.GetXtalk());
        drv.SetDetectionThresholds(100, 300, Vl53l4Uld.WindowIn);
        Assert.Equal((100, 300, Vl53l4Uld.WindowIn), drv.GetDetectionThresholds());
        drv.SetSignalThreshold(1024);
        Assert.Equal(1024, drv.GetSignalThreshold());
        drv.SetSigmaThreshold(15);
        Assert.Equal(15, drv.GetSigmaThreshold());
        Assert.Throws<Vl53l4Exception>(() => drv.SetSigmaThreshold(0x4000));
    }

    [Fact]
    public void IsAliveChecksTheModelId()
    {
        var (drv, p) = Make();
        Assert.True(drv.IsAlive());
        p.SeedWord(Vl53l4Uld.IdentificationModelId, 0x1234);
        Assert.False(drv.IsAlive());
    }

    [Fact]
    public void WaitsTimeOutWhenTheSensorNeverAnswers()
    {
        var (drv, p) = Make();
        p.Reg[Vl53l4Uld.FirmwareSystemStatus] = 0;
        Assert.Throws<Vl53l4Exception>(() => drv.WaitBoot(5));
        p.Reg[Vl53l4Uld.GpioTioHvStatus] = 1; // INT high: not at its active (low) level
        Assert.Throws<Vl53l4Exception>(() => drv.WaitDataReady(5));
    }

    [Fact]
    public void OffsetCalibrationProgramsTargetMinusMean()
    {
        var (drv, p) = Make();
        p.SeedResult(distanceMm: 93, signalRaw: 200, spadsRaw: 256 * 40);
        Assert.Equal(7, drv.CalibrateOffset(100, nbSamples: 5));
        Assert.Equal(Vl53l4Uld.OffsetRaw(7), Word(p.WrittenTo(Vl53l4Uld.RangeOffsetMm)[^1]));
        Assert.Throws<Vl53l4Exception>(() => drv.CalibrateOffset(5));
        Assert.Throws<Vl53l4Exception>(() => drv.CalibrateOffset(100, nbSamples: 4));
    }

    [Fact]
    public void XtalkCalibrationFollowsTheStFormula()
    {
        var (drv, p) = Make();
        // signal 200*8 = 1600 kcps over 40 SPADs; distance 90 at a 100 mm target:
        // (1 - 90/100) * (1600 / 40) = 4.0 kcps.
        p.SeedResult(distanceMm: 90, signalRaw: 200, spadsRaw: 256 * 40);
        Assert.Equal(4, drv.CalibrateXtalk(100, nbSamples: 5));
        // In binary floating point the product is 3.9999…, and the register takes the truncation (as Python's int()).
        Assert.Equal(2047, Word(p.WrittenTo(Vl53l4Uld.XtalkPlaneOffsetKcps)[^1]));

        p.SeedResult(distanceMm: 10, signalRaw: 0xFFFF, spadsRaw: 256);
        Assert.Throws<Vl53l4Exception>(() => drv.CalibrateXtalk(1000, nbSamples: 5)); // > 127 kcps
    }

    [Fact]
    public void ConfigurationWhileRangingIsRefused()
    {
        var board = new FakeBoardLink();
        board.OnPacket = pkt =>
        {
            if (pkt.Cmd != (int)Vl53l4Cmd.ReadReg)
                return new[] { FakeBoardLink.Status(pkt.Cmd, Protocol.Status.Ok) };
            int len = pkt.Payload[2] | (pkt.Payload[3] << 8);
            var reply = new byte[9 + len]; // echoed cmd, timestamp, data (all zero: continuous mode)
            reply[0] = (byte)Vl53l4Cmd.ReadReg;
            return new[] { board.Reply((int)Vl53l4Rpt.RegData, reply) };
        };
        using var dev = new Vl53l4cdDevice(board, TimeSpan.FromMilliseconds(500));
        dev.StartRanging();
        Assert.True(dev.Ranging);
        Assert.Throws<InvalidOperationException>(() => dev.SetOffsetMm(3));
        Assert.Throws<InvalidOperationException>(() => dev.Init());
        Assert.Throws<InvalidOperationException>(() => dev.MeasureOnce());
        dev.StopRanging();
        Assert.False(dev.Ranging);
        dev.StopRanging(); // a no-op once stopped
    }

    [Fact]
    public void StreamReportsDecodeAndShortOnesAreCounted()
    {
        var board = new FakeBoardLink();
        using var dev = new Vl53l4cdDevice(board);
        using var q = dev.Measurements();
        var block = new byte[17];
        block[0] = (byte)Array.IndexOf(Vl53l4Uld.StatusRtn.ToArray(), (byte)0);
        block[13] = 0x01; // distance 0x0102 = 258 mm
        block[14] = 0x02;
        byte[] Payload(byte[] data, int lenField)
        {
            var p = new byte[12 + data.Length];
            BitConverter.GetBytes(123456UL).CopyTo(p, 0);
            p[8] = (byte)Vl53l4Uld.ResultBlockAddr;
            p[10] = (byte)lenField;
            data.CopyTo(p, 12);
            return p;
        }
        board.Inject(board.Reply((int)Vl53l4Rpt.Stream, Payload(block, 17)));
        board.Inject(board.Reply((int)Vl53l4Rpt.Stream, Payload(block.Take(8).ToArray(), 17))); // shorter than its length field
        var m = q.Take(TimeSpan.FromSeconds(1));
        Assert.Equal((123456UL, 258, 0), (m.TimestampUs, m.DistanceMm, m.RangeStatus));
        Assert.True(m.Valid);
        SpinWait.SpinUntil(() => dev.StreamParseErrors == 1, 1000);
        Assert.Equal(1, dev.StreamParseErrors);
        Assert.Equal(0, q.Count);
    }
}
