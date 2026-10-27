using System.Buffers.Binary;
using Depz.Sensor.Devices;
using Depz.Sensor.Protocol;
using Depz.Sensor.Vl53l4;
using Xunit;

namespace Depz.Sensor.Tests;

/// <summary>The VL53L4CD bridge firmware (contract 10) over <see cref="FakeBoardLink"/>, registers backed by
/// <see cref="FakeVl53l4Platform"/> — the twin of the Python fake <c>FakeVl53l4cd</c> (tests/fake_vl53l4.py).
/// <see cref="SendResult"/> emits one RPT_VL53_STREAM as the INT edge would; <see cref="FailNext"/> answers the
/// next command with an error status (null: no answer at all); <see cref="ShortReadNext"/> drops the last byte
/// of the next REG_DATA. XSHUT OFF powers the sensor down (register access answers ERR_HARDWARE_FAULT, §2),
/// RESET wipes the register file back to the boot values.</summary>
internal sealed class FakeVl53l4cdBoard
{
    public readonly FakeBoardLink Link = new();
    public readonly FakeVl53l4Platform Platform = new();
    public readonly List<(int Cmd, byte[] Payload)> Rx = new();
    public ulong McuTimeUs = 5_000_000;
    public bool Streaming;
    public bool Powered = true;
    public int StreamAddr;
    public int StreamLen;
    public uint IntEdges;
    public uint I2cErrors;
    public bool ShortReadNext;
    private readonly Dictionary<int, Queue<Status?>> _failures = new();

    public FakeVl53l4cdBoard() => Link.OnPacket = Handle;

    public void FailNext(Vl53l4Cmd cmd, Status? status)
    {
        if (!_failures.TryGetValue((int)cmd, out var q))
            _failures[(int)cmd] = q = new Queue<Status?>();
        q.Enqueue(status);
    }

    public List<int> Cmds()
    {
        lock (Rx)
            return Rx.Select(r => r.Cmd).ToList();
    }

    public List<byte[]> Sent(Vl53l4Cmd cmd)
    {
        lock (Rx)
            return Rx.Where(r => r.Cmd == (int)cmd).Select(r => r.Payload).ToList();
    }

    public void ClearRx()
    {
        lock (Rx)
            Rx.Clear();
    }

    public int I2cKhz => Platform.Speeds.Count > 0 ? Platform.Speeds[^1] : Vl53l4Uld.I2cKhzBoot;

    /// <summary>A raw 17-byte result block, big-endian registers 0x0089..0x0099 (fake_vl53l4.result_block):
    /// <paramref name="status"/> is the raw RESULT__RANGE_STATUS (9 → ULD status 0 "valid"), <paramref name="spads"/>
    /// the raw RESULT__SPAD_NB word (number_of_spad = spads / 256).</summary>
    public static byte[] ResultBlock(int distanceMm, int status = 9, int sigmaMm = 3, int signalKcps = 1200,
        int spads = 2048, int streamCount = 1, int ambientKcps = 0)
    {
        var raw = new byte[17];
        raw[0] = (byte)status;
        raw[2] = (byte)streamCount;
        BinaryPrimitives.WriteUInt16BigEndian(raw.AsSpan(3), (ushort)spads);
        BinaryPrimitives.WriteUInt16BigEndian(raw.AsSpan(5), (ushort)(signalKcps / 8));
        BinaryPrimitives.WriteUInt16BigEndian(raw.AsSpan(7), (ushort)(ambientKcps / 8));
        BinaryPrimitives.WriteUInt16BigEndian(raw.AsSpan(9), (ushort)(sigmaMm * 4));
        BinaryPrimitives.WriteUInt16BigEndian(raw.AsSpan(13), (ushort)distanceMm);
        return raw;
    }

    public void SetResultBlock(byte[] raw)
    {
        for (int i = 0; i < raw.Length; i++)
            Platform.Reg[Vl53l4Uld.ResultBlockAddr + i] = raw[i];
    }

    /// <summary>One RPT_VL53_STREAM with an explicit header: addr and the length field (default: the data's length).</summary>
    public void SendStream(int addr, byte[] data, int? lenField = null)
    {
        var p = new byte[12 + data.Length];
        BinaryPrimitives.WriteUInt64LittleEndian(p, McuTimeUs);
        BinaryPrimitives.WriteUInt16LittleEndian(p.AsSpan(8), (ushort)addr);
        BinaryPrimitives.WriteUInt16LittleEndian(p.AsSpan(10), (ushort)(lenField ?? data.Length));
        data.CopyTo(p, 12);
        Link.Inject(Link.Reply((int)Vl53l4Rpt.Stream, p));
    }

    public byte[] SendResult(int distanceMm, int status = 9, int sigmaMm = 3, int signalKcps = 1200,
        int spads = 2048, int streamCount = 1)
    {
        var raw = ResultBlock(distanceMm, status, sigmaMm, signalKcps, spads, streamCount);
        IntEdges++;
        McuTimeUs += 30_000;
        SendStream(StreamAddr, raw);
        return raw;
    }

    private byte[] Ok(int cmd) => FakeBoardLink.Status(cmd, Status.Ok);

    private IEnumerable<byte[]> Handle(Transport.Packet pkt)
    {
        int cmd = pkt.Cmd;
        byte[] payload = pkt.Payload;
        lock (Rx)
            Rx.Add((cmd, payload.ToArray()));
        if (_failures.TryGetValue(cmd, out var q) && q.Count > 0)
        {
            Status? fail = q.Dequeue();
            return fail is Status s ? new[] { FakeBoardLink.Status(cmd, s) } : Array.Empty<byte[]>();
        }
        if ((cmd == (int)Vl53l4Cmd.ReadReg || cmd == (int)Vl53l4Cmd.WriteReg) && !Powered)
        {
            I2cErrors++; // an unpowered sensor NACKs (contract 10 §2)
            return new[] { FakeBoardLink.Status(cmd, Status.ErrHardwareFault) };
        }
        switch ((Vl53l4Cmd)cmd)
        {
            case Vl53l4Cmd.ReadReg:
            {
                int addr = BinaryPrimitives.ReadUInt16LittleEndian(payload);
                int n = BinaryPrimitives.ReadUInt16LittleEndian(payload.AsSpan(2));
                byte[] data = Platform.ReadMulti((ushort)addr, n);
                if (ShortReadNext)
                {
                    ShortReadNext = false;
                    data = data[..^1];
                }
                var reply = new byte[9 + data.Length];
                reply[0] = (byte)cmd;
                BinaryPrimitives.WriteUInt64LittleEndian(reply.AsSpan(1), McuTimeUs);
                data.CopyTo(reply, 9);
                McuTimeUs += 100;
                return new[] { Link.Reply((int)Vl53l4Rpt.RegData, reply) };
            }
            case Vl53l4Cmd.WriteReg:
                Platform.WriteMulti(BinaryPrimitives.ReadUInt16LittleEndian(payload), payload[2..]);
                return new[] { Ok(cmd) };
            case Vl53l4Cmd.Xshut:
                if (payload[0] == Vl53l4Xshut.Off)
                {
                    Powered = false;
                    Streaming = false;
                }
                else if (payload[0] == Vl53l4Xshut.On)
                {
                    Powered = true;
                }
                else
                {
                    // RESET: boot values only, the ULD configuration is gone.
                    Platform.Reg.Clear();
                    foreach (var kv in new FakeVl53l4Platform().Reg)
                        Platform.Reg[kv.Key] = kv.Value;
                    Platform.Writes.Clear();
                    Powered = true;
                    Streaming = false;
                }
                return new[] { Ok(cmd) };
            case Vl53l4Cmd.StartStream:
                StreamAddr = BinaryPrimitives.ReadUInt16LittleEndian(payload);
                StreamLen = BinaryPrimitives.ReadUInt16LittleEndian(payload.AsSpan(2));
                Streaming = true;
                return new[] { Ok(cmd) };
            case Vl53l4Cmd.StopStream:
                Streaming = false;
                return new[] { Ok(cmd) };
            case Vl53l4Cmd.GetInfo:
            {
                int modelId = Powered ? BinaryPrimitives.ReadUInt16BigEndian(Platform.ReadMulti(Vl53l4Uld.IdentificationModelId, 2)) : 0;
                var info = new byte[21];
                BinaryPrimitives.WriteUInt32LittleEndian(info, IntEdges);
                BinaryPrimitives.WriteUInt32LittleEndian(info.AsSpan(8), I2cErrors);
                info[12] = (byte)(I2cErrors > 0 ? 1 : 0);
                BinaryPrimitives.WriteUInt16LittleEndian(info.AsSpan(13), (ushort)modelId);
                info[15] = Platform.Reg.TryGetValue(Vl53l4Uld.FirmwareSystemStatus, out byte fw) ? fw : (byte)0;
                info[16] = (byte)(modelId == Vl53l4Uld.ModelId ? 1 : 0);
                info[17] = (byte)(Powered ? 1 : 0);
                info[18] = (byte)((Platform.Reg.TryGetValue(Vl53l4Uld.GpioTioHvStatus, out byte lvl) ? lvl : 0) & 1);
                BinaryPrimitives.WriteUInt16LittleEndian(info.AsSpan(19), (ushort)I2cKhz);
                return new[] { Link.Reply((int)Vl53l4Rpt.Info, info) };
            }
            case Vl53l4Cmd.SetI2cSpeed:
                Platform.SetI2cSpeed(BinaryPrimitives.ReadUInt16LittleEndian(payload));
                return new[] { Ok(cmd) };
            default:
                return new[] { FakeBoardLink.Status(cmd, Status.ErrInvalidCmd) };
        }
    }
}

/// <summary><see cref="Vl53l4cdDevice"/> over <see cref="FakeVl53l4cdBoard"/>, no hardware — the port of the Python
/// test_vl53l4cd_device.py: init over the wire, the XSHUT power states, bridge diagnostics, bus re-timing, every
/// configuration getter/setter as a register round trip, both calibrations, the INT-driven stream, poll-mode
/// MeasureOnce and the bridge error statuses.</summary>
public class Vl53l4cdDeviceTests
{
    private static readonly TimeSpan Short = TimeSpan.FromMilliseconds(500);

    private static (Vl53l4cdDevice, FakeVl53l4cdBoard) Connect(TimeSpan? timeout = null)
    {
        var fake = new FakeVl53l4cdBoard();
        return (new Vl53l4cdDevice(fake.Link, timeout ?? Short), fake);
    }

    private static (Vl53l4cdDevice, FakeVl53l4cdBoard) Ready()
    {
        var (dev, fake) = Connect();
        dev.Init();
        fake.ClearRx();
        fake.Platform.Writes.Clear();
        return (dev, fake);
    }

    private static byte[] Be16(int v) => new[] { (byte)(v >> 8), (byte)v };

    // ── init / identity ──────────────────────────────────────────────────────

    [Fact]
    public void InitRunsTheUldBootSequenceOverTheBridge()
    {
        var (dev, fake) = Connect();
        using var _ = dev;
        Assert.False(dev.Initialized);
        dev.Init();
        Assert.True(dev.Initialized);
        Assert.False(dev.Ranging);
        // config block at the boot speed, then the bridge is re-timed to 1 MHz
        Assert.Equal(new[] { Vl53l4Uld.I2cKhzBoot, Vl53l4Uld.I2cKhzDefault }, fake.Platform.Speeds);
        Assert.Equal(new[] { Vl53l4Uld.ConfigBlock() }, fake.Platform.WrittenTo(Vl53l4Uld.ConfigAddr));
        var cmds = fake.Cmds();
        var speedAt = cmds.Select((c, i) => (c, i)).Where(t => t.c == (int)Vl53l4Cmd.SetI2cSpeed).Select(t => t.i).ToList();
        int configWrite = cmds.IndexOf((int)Vl53l4Cmd.WriteReg);
        Assert.True(speedAt[0] < configWrite && configWrite < speedAt[1]);
        // init's 50 ms continuous default reads back as 49 (ST's integer math; vectors/vl53l4.json)
        Assert.Equal((49, 0), dev.GetRangeTiming());
    }

    [Fact]
    public void InitAtAChosenBusSpeed()
    {
        var (dev, fake) = Connect();
        using var _ = dev;
        dev.Init(busKhz: 400);
        Assert.Equal(new[] { 400 }, fake.Platform.Speeds);
        Assert.Equal(400, dev.BridgeInfo().I2cKhz);
    }

    [Fact]
    public void IsAliveChecksTheModelIdAndSwallowsBridgeErrors()
    {
        var (dev, fake) = Connect();
        using var _ = dev;
        Assert.True(dev.IsAlive());
        fake.Platform.SeedWord(Vl53l4Uld.IdentificationModelId, 0xEACC);
        Assert.False(dev.IsAlive());
        fake.FailNext(Vl53l4Cmd.ReadReg, Status.ErrHardwareFault);
        Assert.False(dev.IsAlive());
    }

    // ── XSHUT ────────────────────────────────────────────────────────────────

    [Fact]
    public void XshutOffPowersTheSensorDownAndForgetsTheInit()
    {
        var (dev, fake) = Ready();
        using var _ = dev;
        dev.StartRanging();
        dev.Xshut(Vl53l4Xshut.Off);
        Assert.Equal(new[] { Vl53l4Xshut.Off }, fake.Sent(Vl53l4Cmd.Xshut)[^1]);
        Assert.False(fake.Powered);
        Assert.False(fake.Streaming);
        Assert.False(dev.Initialized);
        Assert.False(dev.Ranging);
        // every register access now answers ERR_HARDWARE_FAULT (contract 10 §2)
        Assert.False(dev.IsAlive());
        Assert.Throws<StatusException>(() => dev.GetOffsetMm());
        Assert.Throws<StatusException>(() => dev.Init());
        var info = dev.BridgeInfo();
        Assert.Equal((0, 0, 0), (info.XshutLevel, info.Initialized, info.ModelId));
        Assert.True(info.I2cErrors >= 3);
        Assert.Equal(1, info.LastI2cError);
        dev.Xshut(Vl53l4Xshut.On);
        Assert.True(fake.Powered);
        Assert.True(dev.IsAlive());
    }

    [Fact]
    public void AnXshutActionPastResetIsRefusedBeforeTheWire()
    {
        var (dev, fake) = Ready();
        using var _ = dev;
        Assert.Throws<ArgumentOutOfRangeException>(() => dev.Xshut(3));
        Assert.Empty(fake.Cmds());
    }

    [Fact]
    public void ResetSensorWipesTheConfigurationAndNeedsInitAgain()
    {
        var (dev, fake) = Ready();
        using var _ = dev;
        dev.SetOffsetMm(7);
        dev.ResetSensor();
        Assert.Equal(new[] { Vl53l4Xshut.Reset }, fake.Sent(Vl53l4Cmd.Xshut)[^1]);
        Assert.False(dev.Initialized);
        Assert.Equal(0, dev.GetOffsetMm()); // back to the boot register file
        Assert.Empty(fake.Platform.WrittenTo(Vl53l4Uld.ConfigAddr));
        dev.Init();
        Assert.True(dev.Initialized);
        Assert.Equal(new[] { Vl53l4Uld.ConfigBlock() }, fake.Platform.WrittenTo(Vl53l4Uld.ConfigAddr));
    }

    // ── bridge diagnostics / bus speed ───────────────────────────────────────

    [Fact]
    public void BridgeInfoDecodesThe21ByteReport()
    {
        var (dev, fake) = Ready();
        using var _ = dev;
        var info = dev.BridgeInfo();
        Assert.Equal(Array.Empty<byte>(), fake.Sent(Vl53l4Cmd.GetInfo)[^1]);
        Assert.Equal((Vl53l4Uld.ModelId, 0x03, 1), (info.ModelId, info.FwStatus, info.Initialized));
        Assert.Equal((1, Vl53l4Uld.I2cKhzDefault, 0u), (info.XshutLevel, info.I2cKhz, info.IntEdges));
        // counters move with the INT edges; GET_INFO is safe while streaming
        dev.StartRanging();
        fake.SendResult(100);
        fake.SendResult(101);
        Assert.Equal(2u, dev.BridgeInfo().IntEdges);
    }

    [Fact]
    public void SetI2cSpeedKhzRetimesTheBridge()
    {
        var (dev, fake) = Ready();
        using var _ = dev;
        dev.SetI2cSpeedKhz(400);
        Assert.Equal(new byte[] { 0x90, 0x01 }, fake.Sent(Vl53l4Cmd.SetI2cSpeed)[^1]);
        Assert.Equal(400, dev.BridgeInfo().I2cKhz);
        fake.ClearRx();
        Assert.Throws<ArgumentOutOfRangeException>(() => dev.SetI2cSpeedKhz(0x10000)); // does not fit u16 kHz
        Assert.Throws<ArgumentOutOfRangeException>(() => dev.SetI2cSpeedKhz(-1));
        Assert.Empty(fake.Cmds());
    }

    // ── configuration round trips (init first; not while ranging) ────────────

    [Fact]
    public void EveryConfigurationMethodRoundTripsThroughTheRegisters()
    {
        var (dev, fake) = Ready();
        using var _ = dev;
        dev.SetRangeTiming(100, 250);
        // readback is ST's integer math over the programmed words: compare with the pure decode
        var (a, _b, inter) = Vl53l4Uld.RangeTimingRegisters(100, 250, FakeVl53l4Platform.OscFrequencyTypical,
            FakeVl53l4Platform.ClockPllTypical);
        Assert.Equal(a, dev.Uld.ReadWord(Vl53l4Uld.RangeConfigA));
        Assert.Equal(inter, dev.Uld.ReadDword(Vl53l4Uld.IntermeasurementMs));
        var timing = dev.GetRangeTiming();
        Assert.Equal(Vl53l4Uld.DecodeRangeTiming(inter, FakeVl53l4Platform.ClockPllTypical,
            FakeVl53l4Platform.OscFrequencyTypical, a), timing);
        Assert.InRange(timing.InterMeasurementMs, 240, 249); // the 1.055 / 1.065 factor pair reads short
        dev.SetRangeTiming(33);
        Assert.Equal(0, dev.GetRangeTiming().InterMeasurementMs); // continuous again

        dev.SetOffsetMm(-12);
        Assert.Equal(-12, dev.GetOffsetMm());
        Assert.Equal(Be16(-48 & 0xFFFF), fake.Platform.WrittenTo(Vl53l4Uld.RangeOffsetMm)[^1]);
        dev.SetXtalkKcps(20);
        Assert.Equal(20, dev.GetXtalkKcps());
        Assert.Equal(Be16(20 << 9), fake.Platform.WrittenTo(Vl53l4Uld.XtalkPlaneOffsetKcps)[^1]);
        dev.SetDetectionThresholds(100, 300, Vl53l4Uld.WindowOut);
        Assert.Equal((100, 300, Vl53l4Uld.WindowOut), dev.GetDetectionThresholds());
        Assert.Equal(new[] { (byte)Vl53l4Uld.WindowOut }, fake.Platform.WrittenTo(Vl53l4Uld.SystemInterrupt)[^1]);
        dev.SetSignalThresholdKcps(1024);
        Assert.Equal(1024, dev.GetSignalThresholdKcps());
        Assert.Equal(Be16(1024 >> 3), fake.Platform.WrittenTo(Vl53l4Uld.MinCountRateRtnLimitMcps)[^1]);
        dev.SetSigmaThresholdMm(15);
        Assert.Equal(15, dev.GetSigmaThresholdMm());
        Assert.Equal(Be16(15 << 2), fake.Platform.WrittenTo(Vl53l4Uld.RangeConfigSigmaThresh)[^1]);
    }

    [Fact]
    public void ArgumentErrorsPassThroughUnchanged()
    {
        // ERRATA A16: a refused argument is invalid_argument, not the driver's Vl53l4Exception
        var (dev, fake) = Ready();
        using var _ = dev;
        Assert.Throws<ArgumentOutOfRangeException>(() => dev.SetRangeTiming(50, 30));
        Assert.Throws<ArgumentOutOfRangeException>(() => dev.SetRangeTiming(9, 0));
        Assert.Empty(fake.Cmds()); // range timing refuses before any traffic, reads included
        Assert.ThrowsAny<ArgumentException>(() => dev.SetOffsetMm(5000));
        Assert.ThrowsAny<ArgumentException>(() => dev.SetXtalkKcps(128));
        Assert.ThrowsAny<ArgumentException>(() => dev.SetSigmaThresholdMm(16384));
        // the refused value never reaches its register
        foreach (ushort reg in new[] { Vl53l4Uld.RangeConfigA, Vl53l4Uld.IntermeasurementMs, Vl53l4Uld.RangeOffsetMm,
                     Vl53l4Uld.XtalkPlaneOffsetKcps, Vl53l4Uld.RangeConfigSigmaThresh })
            Assert.Empty(fake.Platform.WrittenTo(reg));
        fake.ClearRx();
        Assert.ThrowsAny<ArgumentException>(() => dev.CalibrateOffset(5));
        Assert.ThrowsAny<ArgumentException>(() => dev.CalibrateXtalk(100, nbSamples: 4));
        Assert.Empty(fake.Cmds()); // calibrations refuse before any traffic
    }

    [Fact]
    public void StartTemperatureUpdateRunsTheVhvBurst()
    {
        var (dev, fake) = Ready();
        using var _ = dev;
        dev.StartTemperatureUpdate();
        var p = fake.Platform;
        Assert.Equal(new[] { new byte[] { 0x81 }, new byte[] { 0x09 } }, p.WrittenTo(Vl53l4Uld.VhvConfigTimeoutMacropLoopBound));
        Assert.Equal(new byte[] { 0x21, 0x80 }, p.WrittenTo(Vl53l4Uld.SystemStart).Select(d => d[0]));
        Assert.False(dev.Ranging);
    }

    [Fact]
    public void CalibrationsRunTheirSampleBurstsOverTheBridge()
    {
        var (dev, fake) = Ready();
        using var _ = dev;
        fake.SetResultBlock(FakeVl53l4cdBoard.ResultBlock(120, signalKcps: 1200, spads: 2048));
        Assert.Equal(30, dev.CalibrateOffset(150, nbSamples: 5));
        Assert.Equal(30, dev.GetOffsetMm());
        Assert.Equal(75, dev.CalibrateXtalk(240, nbSamples: 5)); // (1 − 120/240) · 1200/8
        Assert.Equal(75, dev.GetXtalkKcps());
        Assert.False(dev.Ranging);
        // each burst: start, (ready, result, clear) × n, stop — all over READ/WRITE_REG
        Assert.Equal(new[] { (int)Vl53l4Cmd.ReadReg, (int)Vl53l4Cmd.WriteReg }, fake.Cmds().Distinct().OrderBy(c => c));
    }

    [Fact]
    public void EveryConfigurationCallRefusesWhileRanging()
    {
        var (dev, fake) = Ready();
        using var _ = dev;
        dev.StartRanging();
        fake.ClearRx();
        var calls = new Action[]
        {
            () => dev.Init(), () => dev.SetI2cSpeedKhz(400), () => dev.SetRangeTiming(50), () => dev.SetOffsetMm(1),
            () => dev.SetXtalkKcps(1), () => dev.SetDetectionThresholds(100, 300, Vl53l4Uld.WindowIn),
            () => dev.SetSignalThresholdKcps(1024), () => dev.SetSigmaThresholdMm(15), () => dev.StartTemperatureUpdate(),
            () => dev.CalibrateOffset(100), () => dev.CalibrateXtalk(100), () => dev.MeasureOnce(), () => dev.StartRanging(),
        };
        foreach (var call in calls)
            Assert.Throws<InvalidStateException>(call);
        Assert.Empty(fake.Cmds());
    }

    // ── ranging: the INT-driven stream ───────────────────────────────────────

    [Fact]
    public void StartRangingStartsTheSensorThenArmsTheStream()
    {
        var (dev, fake) = Ready();
        using var _ = dev;
        dev.StartRanging();
        Assert.True(dev.Ranging);
        Assert.True(fake.Streaming);
        Assert.Equal(new byte[] { 0x21 }, fake.Platform.WrittenTo(Vl53l4Uld.SystemStart)[^1]);
        Assert.Equal(new byte[] { 0x89, 0x00, 17, 0x00, 0x00 }, fake.Sent(Vl53l4Cmd.StartStream)[^1]);
        Assert.Equal((Vl53l4Uld.ResultBlockAddr, Vl53l4Uld.ResultBlockLen), (fake.StreamAddr, fake.StreamLen));
        dev.StopRanging();
        Assert.False(dev.Ranging);
        Assert.False(fake.Streaming);
        // the bridge stream is released first, then the sensor is told to stop
        var cmds = fake.Cmds();
        Assert.Equal(new[] { (int)Vl53l4Cmd.StopStream, (int)Vl53l4Cmd.WriteReg }, cmds.Skip(cmds.Count - 2));
        Assert.Equal(new byte[] { 0x87, 0x00, 0x80 }, fake.Sent(Vl53l4Cmd.WriteReg)[^1]);
    }

    [Fact]
    public void StopRangingClearsTheStateEvenWhenTheBridgeRefuses()
    {
        var (dev, fake) = Ready();
        using var _ = dev;
        dev.StartRanging();
        fake.FailNext(Vl53l4Cmd.StopStream, Status.ErrHardwareFault);
        Assert.Throws<StatusException>(() => dev.StopRanging());
        Assert.False(dev.Ranging);
        // the sensor itself was still told to stop, so reconfiguration is possible
        Assert.Equal(new byte[] { 0x80 }, fake.Platform.WrittenTo(Vl53l4Uld.SystemStart)[^1]);
        dev.SetOffsetMm(3);
        Assert.Equal(3, dev.GetOffsetMm());
    }

    [Fact]
    public void StreamedBlocksReachCallbacksAndTheMeasurementsQueue()
    {
        var (dev, fake) = Ready();
        using var _ = dev;
        var got = new List<Vl53l4Measurement>();
        var unsubscribe = dev.OnMeasurement(m => { lock (got) got.Add(m); });
        using var q = dev.Measurements(capacity: 4);
        dev.StartRanging();
        byte[] raw = fake.SendResult(291, sigmaMm: 3, signalKcps: 1600, spads: 2048, streamCount: 7);
        var m = q.Take(TimeSpan.FromSeconds(2));
        var r = Vl53l4Uld.ParseResultBlock(raw);
        Assert.Equal(new Vl53l4Measurement(fake.McuTimeUs, r.RangeStatus, r.DistanceMm, r.SigmaMm, r.SignalRateKcps,
            r.AmbientRateKcps, r.SignalPerSpadKcps, r.AmbientPerSpadKcps, r.NumberOfSpad, r.StreamCount), m);
        Assert.Equal((291, 3, 1600, 8), (m.DistanceMm, m.SigmaMm, m.SignalRateKcps, m.NumberOfSpad));
        Assert.Equal((7, true, "valid"), (m.StreamCount, m.Valid, m.StatusText));
        Assert.True(SpinWait.SpinUntil(() => { lock (got) return got.Count == 1; }, 2000));
        unsubscribe.Dispose();
        fake.SendResult(77, status: 4);
        var m2 = q.Take(TimeSpan.FromSeconds(2));
        Assert.Equal((77, 2, false), (m2.DistanceMm, m2.RangeStatus, m2.Valid));
        Assert.Equal("signal below threshold", m2.StatusText);
        lock (got)
            Assert.Equal(new[] { m }, got); // unsubscribed
    }

    [Fact]
    public void TheMeasurementsQueueIsBoundedDropOldestAndEndsWithTheDevice()
    {
        var (dev, fake) = Ready();
        var q = dev.Measurements(capacity: 1);
        dev.StartRanging();
        foreach (int d in new[] { 10, 20, 30 })
            fake.SendResult(d);
        dev.BridgeInfo(); // queues behind the three reports: the reader has handled them
        Assert.Equal(2, q.DroppedCount);
        Assert.Equal(new[] { 2L }, dev.StreamDroppedCounts);
        Assert.Equal(30, q.Take(TimeSpan.FromSeconds(1)).DistanceMm);
        dev.Dispose();
        Assert.True(q.IsClosed); // closing the device ends the queue
        Assert.Throws<DeviceLostException>(() => q.Take(TimeSpan.FromSeconds(1)));
        q.Dispose();
        Assert.Empty(dev.StreamDroppedCounts); // deregistered by its own Dispose
    }

    [Fact]
    public void GetMeasurementTimesOutWhenNothingStreams()
    {
        var (dev, _) = Ready();
        using var __ = dev;
        dev.StartRanging();
        var e = Assert.Throws<DepzTimeoutException>(() => dev.GetMeasurement(TimeSpan.FromMilliseconds(150)));
        Assert.Equal((int)Vl53l4Rpt.Stream, e.Cmd);
        Assert.Empty(dev.StreamDroppedCounts); // the one-shot queue is gone
    }

    [Fact]
    public async Task GetMeasurementReturnsTheNextStreamedSampleAndRefusesAClosedDevice()
    {
        var (dev, fake) = Ready();
        dev.StartRanging();
        var waiter = Task.Run(() => dev.GetMeasurement(TimeSpan.FromSeconds(5)));
        // only a sample sent after GetMeasurement subscribed reaches it: wait for its one-shot queue
        Assert.True(SpinWait.SpinUntil(() => dev.StreamDroppedCounts.Count == 1, 5000));
        fake.SendResult(625);
        Assert.Equal(625, (await waiter.WaitAsync(TimeSpan.FromSeconds(5))).DistanceMm);
        dev.Dispose();
        var e = Assert.Throws<LinkClosedException>(() => dev.GetMeasurement(TimeSpan.FromSeconds(5)));
        Assert.Contains("is closed", e.Message);
    }

    [Fact]
    public void UndecodableStreamBlocksAreCountedNotFatal()
    {
        var (dev, fake) = Ready();
        using var _ = dev;
        using var q = dev.Measurements(capacity: 4);
        dev.StartRanging();
        // somebody re-armed the stream on a four-byte block: the decoder needs the whole result block
        fake.SendStream(Vl53l4Uld.ResultBlockAddr, new byte[4]);
        fake.SendResult(624);
        Assert.Equal(624, q.Take(TimeSpan.FromSeconds(2)).DistanceMm);
        Assert.Equal(1, dev.StreamParseErrors);
        Assert.Equal(0, q.Count);
    }

    // ── poll mode ────────────────────────────────────────────────────────────

    [Fact]
    public void MeasureOncePollsOneResultAndStops()
    {
        var (dev, fake) = Ready();
        using var _ = dev;
        fake.SetResultBlock(FakeVl53l4cdBoard.ResultBlock(333, streamCount: 5));
        var m = dev.MeasureOnce();
        Assert.Equal((333, 5, true), (m.DistanceMm, m.StreamCount, m.Valid));
        Assert.Equal(fake.McuTimeUs - 100, m.TimestampUs); // the block read's MCU stamp
        Assert.Equal(new byte[] { 0x21, 0x80 }, fake.Platform.WrittenTo(Vl53l4Uld.SystemStart).Select(d => d[0]));
        Assert.Equal(new[] { new byte[] { 0x01 } }, fake.Platform.WrittenTo(Vl53l4Uld.SystemInterruptClear));
        Assert.DoesNotContain((int)Vl53l4Cmd.StartStream, fake.Cmds());
        Assert.False(dev.Ranging);
    }

    [Fact]
    public void MeasureOnceTimesOutAndStillStopsTheSensor()
    {
        var (dev, fake) = Ready();
        using var _ = dev;
        fake.Platform.Reg[Vl53l4Uld.GpioTioHvStatus] = 0x01; // INT never asserted
        var e = Assert.Throws<Vl53l4Exception>(() => dev.MeasureOnce(TimeSpan.FromMilliseconds(10)));
        Assert.Contains("data ready", e.Message);
        Assert.Equal(new byte[] { 0x80 }, fake.Platform.WrittenTo(Vl53l4Uld.SystemStart)[^1]);
    }

    // ── register transfers and bridge errors ─────────────────────────────────

    [Fact]
    public void AShortRegisterReadIsAnError()
    {
        var (dev, fake) = Ready();
        using var _ = dev;
        fake.ShortReadNext = true;
        var e = Assert.Throws<DepzException>(() => dev.GetOffsetMm());
        Assert.Contains("expected 2, got 1", e.Message);
    }

    [Fact]
    public void BridgeErrorStatusesRaiseStatusException()
    {
        var (dev, fake) = Ready();
        using var _ = dev;
        fake.FailNext(Vl53l4Cmd.WriteReg, Status.ErrHardwareFault);
        Assert.Throws<StatusException>(() => dev.SetOffsetMm(1));
        fake.FailNext(Vl53l4Cmd.ReadReg, Status.ErrHardwareFault);
        Assert.Throws<StatusException>(() => dev.GetRangeTiming());
        fake.FailNext(Vl53l4Cmd.Xshut, Status.ErrHardwareFault); // RESET boot handshake timed out
        Assert.Throws<StatusException>(() => dev.ResetSensor());
        fake.FailNext(Vl53l4Cmd.StartStream, Status.ErrHardwareFault);
        Assert.Throws<StatusException>(() => dev.StartRanging());
        Assert.False(dev.Ranging);
    }

    [Fact]
    public void NoAnswerIsATimeout()
    {
        var (dev, fake) = Connect(TimeSpan.FromMilliseconds(100));
        using var _ = dev;
        fake.FailNext(Vl53l4Cmd.GetInfo, null);
        Assert.Throws<DepzTimeoutException>(() => dev.BridgeInfo());
        fake.FailNext(Vl53l4Cmd.StartStream, null);
        dev.Init();
        Assert.Throws<DepzTimeoutException>(() => dev.StartRanging());
        Assert.False(dev.Ranging);
    }
}
