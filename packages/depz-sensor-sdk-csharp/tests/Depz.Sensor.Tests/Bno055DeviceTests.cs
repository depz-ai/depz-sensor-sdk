using System.Buffers.Binary;
using Depz.Sensor.Bno055;
using Depz.Sensor.Devices;
using Depz.Sensor.Protocol;
using Depz.Sensor.Transport;
using Xunit;

namespace Depz.Sensor.Tests;

/// <summary>A BNO055 behind its register bridge: two register pages, a fused quaternion that is
/// non-zero in fusion modes, and every register write recorded (page, register, value).</summary>
internal sealed class FakeBno055Board
{
    public readonly FakeBoardLink Link = new();
    public readonly byte[][] Pages = { new byte[256], new byte[256] };
    public readonly List<(int Page, int Reg, byte Value)> Writes = new();
    public readonly List<(int Page, int Addr, int Length)> Reads = new();
    /// <summary>RPT_BNO_INFO payload GET_INFO answers with (the live board's by default).</summary>
    public byte[] Info = Convert.FromHexString(LiveInfoHex);
    /// <summary>Statuses the next commands answer with instead of their reply (per opcode, in order).</summary>
    public readonly Dictionary<int, Queue<Status>> FailNext = new();
    /// <summary>SYS_STATUS values the next reads return, then 0 (null: the register as written).</summary>
    public Queue<byte>? SysStatusScript;
    /// <summary>A fusion mode never brings the quaternion out of zero.</summary>
    public bool FusionStuck;
    private int _page;

    /// <summary>GET_INFO from the live board (tests/fake_bno055.py LIVE_INFO_HEX): ids A0 FB 32 0F, SW 03.11, initialized.</summary>
    public const string LiveInfoHex = "28a0fb320f1103150100000000006f012b09d105000000000000000000000000000000007d00";

    public FakeBno055Board()
    {
        Pages[0][Bno055Regs.SysStatus] = 0;
        Link.OnPacket = OnPacket;
    }

    private IEnumerable<byte[]> OnPacket(Packet p)
    {
        if (FailNext.TryGetValue(p.Cmd, out var fails) && fails.Count > 0)
            return new[] { FakeBoardLink.Status(p.Cmd, fails.Dequeue()) };
        switch (p.Cmd)
        {
            case (int)Bno055Cmd.GetInfo:
                return new[] { Link.Reply((int)Bno055Rpt.Info, Info) };
            case (int)Bno055Cmd.ReadReg:
            {
                int addr = p.Payload[0], n = p.Payload[1];
                var regs = Pages[_page];
                Reads.Add((_page, addr, n));
                if (_page == 0)
                {
                    bool fusion = Bno055Regs.IsFusion((Bno055OprMode)(regs[Bno055Regs.OprMode] & 0x0F));
                    regs[Bno055Regs.QuaData] = (byte)(fusion && !FusionStuck ? 0x10 : 0);
                    if (SysStatusScript is not null && addr <= Bno055Regs.SysStatus && Bno055Regs.SysStatus < addr + n)
                        regs[Bno055Regs.SysStatus] = SysStatusScript.TryDequeue(out byte st) ? st : (byte)0;
                }
                var reply = new byte[9 + n];
                reply[0] = (byte)Bno055Cmd.ReadReg;
                Array.Copy(regs, addr, reply, 9, n);
                return new[] { Link.Reply((int)Bno055Rpt.RegData, reply) };
            }
            case (int)Bno055Cmd.WriteReg:
            {
                int addr = p.Payload[0];
                for (int i = 1; i < p.Payload.Length; i++)
                {
                    int reg = addr + i - 1;
                    Writes.Add((_page, reg, p.Payload[i]));
                    if (reg == Bno055Regs.PageId)
                        _page = p.Payload[i];
                    else
                        Pages[_page][reg] = p.Payload[i];
                }
                return new[] { FakeBoardLink.Status(p.Cmd, Status.Ok) };
            }
            default:
                return new[] { FakeBoardLink.Status(p.Cmd, Status.Ok) };
        }
    }

    public IEnumerable<byte> Written(int reg, int page = 0) => Writes.Where(w => w.Reg == reg && w.Page == page).Select(w => w.Value);
}

public class Bno055DeviceTests
{
    private static (Bno055Device, FakeBno055Board) Make()
    {
        var board = new FakeBno055Board();
        return (new Bno055Device(board.Link, TimeSpan.FromMilliseconds(500)), board);
    }

    [Fact]
    public void ASwitchBetweenTwoOperatingModesGoesThroughConfig()
    {
        var (dev, b) = Make();
        using var _ = dev;
        dev.SetOperationMode(Bno055OprMode.Ndof);
        dev.SetOperationMode(Bno055OprMode.Amg);
        Assert.Equal(new byte[] { (byte)Bno055OprMode.Ndof, 0, (byte)Bno055OprMode.Amg }, b.Written(Bno055Regs.OprMode));
        dev.SetOperationMode(Bno055OprMode.Amg); // already there: nothing written
        Assert.Equal(3, b.Written(Bno055Regs.OprMode).Count());
        Assert.Equal(Bno055OprMode.Amg, dev.GetOperationMode());
    }

    [Fact]
    public void SuspendKeepsTheModeAndWakeUpPutsItBack()
    {
        var (dev, b) = Make();
        using var _ = dev;
        dev.Configure(Bno055OprMode.Ndof);
        b.Writes.Clear();
        dev.SetPowerMode(Bno055PwrMode.Suspend);
        Assert.Equal(new byte[] { 0 }, b.Written(Bno055Regs.OprMode));
        Assert.Equal(new byte[] { (byte)Bno055PwrMode.Suspend }, b.Written(Bno055Regs.PwrMode));
        b.Writes.Clear();
        dev.SetPowerMode(Bno055PwrMode.Normal);
        Assert.Equal(new byte[] { (byte)Bno055PwrMode.Normal }, b.Written(Bno055Regs.PwrMode));
        Assert.Equal(new byte[] { (byte)Bno055OprMode.Ndof }, b.Written(Bno055Regs.OprMode));
    }

    [Fact]
    public void AChipFoundAsleepWakesIntoNdof()
    {
        var (dev, b) = Make();
        using var _ = dev;
        b.Pages[0][Bno055Regs.PwrMode] = (byte)Bno055PwrMode.Suspend; // CONFIG + suspended at connect
        dev.SetPowerMode(Bno055PwrMode.Normal);
        Assert.Equal(new byte[] { (byte)Bno055OprMode.Ndof }, b.Written(Bno055Regs.OprMode));
    }

    [Fact]
    public void RestoreConfigurationBeforeConfigureIsInvalidState()
    {
        var (dev, _) = Make();
        using var _d = dev;
        Assert.Throws<InvalidStateException>(() => dev.RestoreConfiguration());
    }

    [Fact]
    public void ConfigureWritesUnitsRemapAndProfileInConfig()
    {
        var (dev, b) = Make();
        using var _ = dev;
        var units = new Bno055Units(AccelMg: true, TempF: true);
        var profile = new Bno055CalibrationProfile(new Bno055Vec3(1, 2, 3), new Bno055Vec3(4, 5, 6), new Bno055Vec3(7, 8, 9), 1000, 480);
        dev.Configure(Bno055OprMode.Ndof, units, Bno055AxisRemap.Placement("P2"), profile);
        Assert.Equal(new[] { 0, (int)Bno055OprMode.Ndof }, b.Written(Bno055Regs.OprMode).Select(x => (int)x));
        Assert.Equal(new[] { units.Pack() }, b.Written(Bno055Regs.UnitSel));
        Assert.Equal(profile.Pack(), b.Pages[0].Skip(Bno055Regs.CalibProfile).Take(Bno055Regs.CalibProfileLen));
        Assert.Equal(units, dev.GetUnits());
        b.Writes.Clear();
        dev.RestoreConfiguration();
        Assert.Equal(new[] { units.Pack() }, b.Written(Bno055Regs.UnitSel));
    }

    [Fact]
    public void Page1IsEnteredAndLeftAndRefusedWhileStreaming()
    {
        var (dev, b) = Make();
        using var _ = dev;
        dev.SetAccelConfig(new Bno055AccelConfig(2, 3, 0));
        Assert.Equal(new byte[] { 1, 0 }, b.Writes.Where(w => w.Reg == Bno055Regs.PageId).Select(w => w.Value));
        Assert.Equal(new Bno055AccelConfig(2, 3, 0).Pack(), b.Pages[1][Bno055Regs.P1AccConfig]);
        Assert.Equal(new Bno055AccelConfig(2, 3, 0), dev.GetAccelConfig());
        dev.StartStream();
        Assert.Throws<InvalidStateException>(() => dev.UniqueId());
        Assert.Throws<InvalidStateException>(() => dev.SetInterruptEnable(1));
        dev.StopStream();
        Assert.Equal(16, dev.UniqueId().Length);
    }

    [Fact]
    public void SelfTestRunsInConfigAndRestoresTheMode()
    {
        var (dev, b) = Make();
        using var _ = dev;
        dev.SetOperationMode(Bno055OprMode.Ndof);
        b.Pages[0][Bno055Regs.StResult] = Bno055Regs.ExpectedSelfTest;
        b.Writes.Clear();
        var status = dev.SelfTest();
        Assert.True(status.SelfTestPassed);
        Assert.Equal(new byte[] { Bno055Regs.SysTriggerSelfTest }, b.Written(Bno055Regs.SysTrigger));
        Assert.Equal(new byte[] { 0, (byte)Bno055OprMode.Ndof }, b.Written(Bno055Regs.OprMode));
    }

    [Fact]
    public void ArgumentsOutsideTheirFieldsAreRefusedBeforeSending()
    {
        var (dev, b) = Make();
        using var _ = dev;
        Assert.Throws<ArgumentOutOfRangeException>(() => dev.SetOperationMode((Bno055OprMode)0x0D));
        Assert.Throws<ArgumentOutOfRangeException>(() => dev.SetPowerMode((Bno055PwrMode)3));
        Assert.Throws<ArgumentOutOfRangeException>(() => dev.SetInterruptSetting(0x10, 1));
        Assert.Throws<ArgumentOutOfRangeException>(() => dev.SetInterruptMask(256));
        Assert.Throws<ArgumentException>(() => dev.StartStream(10, (0xF0, 0x20)));
        Assert.Throws<ArgumentOutOfRangeException>(() => dev.StartStream(-1));
        // contract 13 §2 (ERRATA A13): a TIMER period outside 1..60000 ms is refused before any traffic
        Assert.Throws<ArgumentOutOfRangeException>(() => dev.StartStream(0));
        Assert.Throws<ArgumentOutOfRangeException>(() => dev.StartStream(60001));
        Assert.Throws<ArgumentOutOfRangeException>(() => dev.ReadRegisters(0, 1, page: 2));
        Assert.Empty(b.Link.Sent);
    }

    [Fact]
    public void SamplesDecodeWithTheUnitsOfTheStreamStart()
    {
        var (dev, b) = Make();
        using var _ = dev;
        dev.SetUnits(new Bno055Units(AccelMg: true));
        using var q = dev.Samples();
        dev.StartStream(20, (Bno055Regs.AccData, 6));
        var payload = new byte[16];
        BitConverter.GetBytes(77UL).CopyTo(payload, 0);
        payload[8] = Bno055Regs.AccData;
        payload[9] = 6;
        BitConverter.GetBytes((short)1000).CopyTo(payload, 10);
        b.Link.Inject(b.Link.Reply((int)Bno055Rpt.Stream, payload));
        var s = q.Take(TimeSpan.FromSeconds(1));
        Assert.Equal(1000.0, s.Accel![0]); // mg: LSB 1
        Assert.Null(s.Quaternion);
        Assert.Equal(77UL, s.TimestampUs);
    }

    private static (int, int, byte)[] W(params (int Page, int Reg, int Value)[] w) =>
        w.Select(x => (x.Page, x.Reg, (byte)x.Value)).ToArray();

    [Fact]
    public void BridgeInfoAndIsAlive()
    {
        var (dev, b) = Make();
        using var _ = dev;
        var info = dev.BridgeInfo();
        Assert.Equal(Bno055Info.Unpack(Convert.FromHexString(FakeBno055Board.LiveInfoHex)), info);
        Assert.Equal("03.11", info.SwRevText);
        Assert.True(dev.IsAlive());
        string hex = FakeBno055Board.LiveInfoHex;
        b.Info = Convert.FromHexString(hex[..16] + "00" + hex[18..]); // initialized = 0
        Assert.False(dev.IsAlive());
        b.Info = Convert.FromHexString("28a1" + hex[4..]); // wrong chip id
        Assert.False(dev.IsAlive());
        // an error status is "not alive", not a throw
        b.Info = Convert.FromHexString(hex);
        b.FailNext[(int)Bno055Cmd.GetInfo] = new Queue<Status>(new[] { Status.ErrNotInitialized });
        Assert.False(dev.IsAlive());
        Assert.True(dev.IsAlive());
    }

    [Fact]
    public void APage1RegisterWriteIsWrappedInPageIdWrites()
    {
        var (dev, b) = Make();
        using var _ = dev;
        dev.WriteRegisters(Bno055Regs.P1IntEn, new byte[] { 0x40 }, page: 1);
        Assert.Equal(W((0, 0x07, 1), (1, 0x10, 0x40), (1, 0x07, 0)), b.Writes);
        Assert.Equal(0x40, dev.ReadRegister(Bno055Regs.P1IntEn, page: 1));
    }

    [Fact]
    public void LowPowerInConfigStaysInConfig()
    {
        var (dev, b) = Make();
        using var _ = dev;
        dev.SetPowerMode(Bno055PwrMode.LowPower);
        Assert.Equal(W((0, 0x3E, 1)), b.Writes);
        Assert.Equal(Bno055PwrMode.LowPower, dev.GetPowerMode());
        Assert.Equal(Bno055OprMode.Config, dev.GetOperationMode());
    }

    [Fact]
    public void AxisRemapAndTemperatureSourceAreWrittenInsideAConfigRoundTrip()
    {
        var (dev, b) = Make();
        using var _ = dev;
        dev.SetOperationMode(Bno055OprMode.Ndof);
        b.Writes.Clear();
        dev.SetAxisRemap(Bno055AxisRemap.Placement("p0"));
        Assert.Equal(W((0, 0x3D, 0), (0, 0x41, 0x21), (0, 0x42, 0x04), (0, 0x3D, 0x0C)), b.Writes);
        Assert.Equal(Bno055AxisRemap.Placement("P0"), dev.GetAxisRemap());
        dev.SetAxisRemap(new Bno055AxisRemap());
        Assert.Equal(Bno055AxisRemap.Placement("P1"), dev.GetAxisRemap());
        b.Writes.Clear();
        dev.SetTemperatureSource(Bno055TempSource.Gyro);
        Assert.Equal(W((0, 0x3D, 0), (0, 0x40, 1), (0, 0x3D, 0x0C)), b.Writes);
        Assert.Equal(Bno055TempSource.Gyro, dev.GetTemperatureSource());
        Assert.Throws<ArgumentNullException>(() => dev.SetAxisRemap(null!));
    }

    [Fact]
    public void CalibrationStatusProfileAndSicMatrix()
    {
        var (dev, b) = Make();
        using var _ = dev;
        b.Pages[0][Bno055Regs.CalibStat] = 0xFF;
        Assert.True(dev.CalibrationStatus().FullyCalibrated);
        b.Pages[0][Bno055Regs.CalibStat] = 0xC4;
        Assert.Equal(new Bno055CalibStatus(3, 0, 1, 0), dev.CalibrationStatus());

        dev.SetOperationMode(Bno055OprMode.Ndof);
        var profile = new Bno055CalibrationProfile(new Bno055Vec3(-1, 2, -3), new Bno055Vec3(400, -500, 600),
            new Bno055Vec3(7, -8, 9), 1000, -32768);
        b.Writes.Clear();
        dev.WriteCalibrationProfile(profile);
        var writes = b.Writes.ToList();
        Assert.Equal((0, 0x3D, (byte)0), writes[0]);
        Assert.Equal(profile.Pack(), writes.Skip(1).Take(22).Select(w => w.Value));
        Assert.Equal(Enumerable.Range(0x55, 22), writes.Skip(1).Take(22).Select(w => w.Reg));
        Assert.Equal((0, 0x3D, (byte)0x0C), writes[^1]);
        Assert.Equal(profile, dev.ReadCalibrationProfile()); // read in CONFIG, NDOF again after
        Assert.Equal(Bno055OprMode.Ndof, dev.GetOperationMode());
        Assert.Throws<ArgumentNullException>(() => dev.WriteCalibrationProfile(null!));

        dev.SetSicMatrix();
        Assert.Equal(Bno055Regs.SicIdentity, dev.GetSicMatrix());
        Assert.Equal(new byte[] { 0x00, 0x40, 0, 0 }, b.Pages[0].AsSpan(Bno055Regs.SicMatrix, 4).ToArray());
    }

    [Fact]
    public void GyroAndMagConfigsRoundTripOnPage1()
    {
        var (dev, b) = Make();
        using var _ = dev;
        dev.SetGyroConfig(new Bno055GyroConfig(Range: 2, Bandwidth: 3, Power: 1));
        dev.SetMagConfig(new Bno055MagConfig(Rate: 7, Mode: 3, Power: 0));
        Assert.Equal(W((1, 0x0A, 0x1A), (1, 0x0B, 0x01), (1, 0x09, 0x1F)), b.Writes.Where(w => w.Page == 1 && w.Reg != Bno055Regs.PageId));
        Assert.Equal(new Bno055GyroConfig(2, 3, 1), dev.GetGyroConfig());
        Assert.Equal(new Bno055MagConfig(7, 3, 0), dev.GetMagConfig());
    }

    [Fact]
    public void InterruptEnableAndMaskSkipTheConfigRoundTrip()
    {
        var (dev, b) = Make();
        using var _ = dev;
        dev.SetOperationMode(Bno055OprMode.Ndof);
        b.Writes.Clear();
        dev.SetInterruptEnable(0x40);
        dev.SetInterruptMask(0xC0);
        Assert.Equal(W((0, 0x07, 1), (1, 0x10, 0x40), (1, 0x07, 0), (0, 0x07, 1), (1, 0x0F, 0xC0), (1, 0x07, 0)), b.Writes);
        Assert.Equal(0x40, dev.GetInterruptEnable());
        Assert.Equal(0xC0, dev.GetInterruptMask());
        Assert.Throws<ArgumentOutOfRangeException>(() => dev.SetInterruptMask(0x100));
    }

    [Fact]
    public void InterruptSettingAcceptsOnlyPage1Registers0x11To0x1F()
    {
        var (dev, b) = Make();
        using var _ = dev;
        foreach (int bad in new[] { 0x10, 0x20 })
            Assert.Throws<ArgumentOutOfRangeException>(() => dev.SetInterruptSetting(bad, 1));
        Assert.Throws<ArgumentOutOfRangeException>(() => dev.SetInterruptSetting(0x11, 0x10A)); // a byte, not masked
        Assert.Empty(b.Link.Sent);
        dev.SetOperationMode(Bno055OprMode.Amg);
        b.Writes.Clear();
        dev.SetInterruptSetting(0x11, 0x0A);
        Assert.Equal(W((0, 0x3D, 0), (0, 0x07, 1), (1, 0x11, 0x0A), (1, 0x07, 0), (0, 0x3D, 0x07)), b.Writes);
    }

    [Fact]
    public void IntStaReadsAndClearInterruptKeepsClkSel()
    {
        var (dev, b) = Make();
        using var _ = dev;
        b.Pages[0][Bno055Regs.IntSta] = 0x40;
        b.Pages[0][Bno055Regs.SysTrigger] = 0x80;
        Assert.Equal(0x40, dev.ReadInterruptStatus());
        dev.ClearInterrupt();
        Assert.Equal(W((0, 0x3F, 0xC0)), b.Writes);
    }

    [Fact]
    public void ReadSampleWindowAndQuaternion()
    {
        var (dev, b) = Make();
        using var _ = dev;
        b.Pages[0][0x34] = 0xE7; // -25 °C
        b.Pages[0][0x35] = 0xFF;
        var s = dev.ReadSample((0x34, 2));
        Assert.Equal(-25.0, s.Temperature);
        Assert.True(s.Calibration!.Value.FullyCalibrated);
        Assert.Equal(new double[]?[] { null, null, null, null, null, null, null },
            new[] { s.Accel, s.Mag, s.Gyro, s.Euler, s.Quaternion, s.LinearAccel, s.Gravity });
        // w = 0x4000, x = -0x2000 (Q14); 0x20 itself reads 0 in CONFIG on this fake
        b.Pages[0][0x21] = 0x40;
        b.Pages[0][0x22] = 0x00;
        b.Pages[0][0x23] = 0xE0;
        b.Reads.Clear();
        Assert.Equal(new[] { 1.0, -0.5, 0.0, 0.0 }, dev.ReadQuaternion());
        Assert.Equal(new[] { (0, 0x20, 8) }, b.Reads); // units are cached: only the 8-byte block
    }

    [Fact]
    public void StopStreamIsANoOpWhenIdleAndClearsTheFlagOnFailure()
    {
        var (dev, b) = Make();
        using var _ = dev;
        dev.StopStream();
        Assert.Empty(b.Link.Sent);
        dev.StartStream();
        Assert.True(dev.Streaming);
        dev.StopStream();
        Assert.Equal((int)Bno055Cmd.StopStream, b.Link.Sent[^1].Cmd);
        Assert.False(dev.Streaming);
        dev.StartStream();
        b.FailNext[(int)Bno055Cmd.StopStream] = new Queue<Status>(new[] { Status.Error });
        Assert.Throws<StatusException>(() => dev.StopStream());
        Assert.False(dev.Streaming); // cleared even on failure
    }

    private static void PushStream(FakeBno055Board b, int addr, byte[] data, ulong ts, int? lenField = null)
    {
        var payload = new byte[10 + data.Length];
        BinaryPrimitives.WriteUInt64LittleEndian(payload, ts);
        payload[8] = (byte)addr;
        payload[9] = (byte)(lenField ?? data.Length);
        data.CopyTo(payload, 10);
        b.Link.Inject(b.Link.Reply((int)Bno055Rpt.Stream, payload));
    }

    [Fact]
    public async Task GetSampleReturnsTheNextOneTimesOutAndEndsOnClose()
    {
        var (dev, b) = Make();
        dev.StartStream();
        var got = Task.Run(() => dev.GetSample(TimeSpan.FromSeconds(5)));
        Assert.True(SpinWait.SpinUntil(() => dev.StreamDroppedCounts.Count == 1, 5000));
        PushStream(b, 0x34, new byte[] { 0x1A, 0x00 }, 77);
        var sample = await got.WaitAsync(TimeSpan.FromSeconds(5));
        Assert.Equal((77UL, 26.0), (sample.TimestampUs, sample.Temperature));
        Assert.Empty(dev.StreamDroppedCounts); // its one-shot queue is gone
        var t = Assert.Throws<DepzTimeoutException>(() => dev.GetSample(TimeSpan.FromMilliseconds(50)));
        Assert.Equal((int)Bno055Rpt.Stream, t.Cmd);
        var pending = Task.Run(() => dev.GetSample(TimeSpan.FromSeconds(5)));
        Assert.True(SpinWait.SpinUntil(() => dev.StreamDroppedCounts.Count == 1, 5000));
        dev.Dispose();
        var e = await Assert.ThrowsAsync<LinkClosedException>(() => pending.WaitAsync(TimeSpan.FromSeconds(5)));
        Assert.Contains("device closed", e.Message);
        Assert.Contains("device is closed", Assert.Throws<LinkClosedException>(() => dev.GetSample(TimeSpan.FromMilliseconds(100))).Message);
    }

    [Fact]
    public void MalformedStreamReportsAreCountedAndDropped()
    {
        var (dev, b) = Make();
        using var _ = dev;
        dev.StartStream();
        var got = new List<Bno055Sample>();
        using var hook = dev.OnSample(x => { lock (got) got.Add(x); });
        b.Link.Inject(b.Link.Reply((int)Bno055Rpt.Stream, new byte[9])); // shorter than the 10-byte header
        PushStream(b, 0x08, Convert.FromHexString("a2037fff73ff"), 1, lenField: 46); // truncated block
        Assert.True(SpinWait.SpinUntil(() => dev.StreamParseErrors == 2, 5000));
        PushStream(b, 0x08, Convert.FromHexString("a2037fff73ff"), 2);
        Assert.True(SpinWait.SpinUntil(() => { lock (got) return got.Count == 1; }, 5000));
        Assert.Equal(new[] { 9.3, -1.29, -1.41 }, got[0].Accel!.Select(v => Math.Round(v, 2)));
        Assert.Equal(2, dev.StreamParseErrors);
    }

    [Fact]
    public void ASelfTestInConfigClearsTheLeftover()
    {
        var (dev, b) = Make();
        using var _ = dev;
        dev.SelfTest();
        Assert.Equal(W((0, 0x3F, 0x01), (0, 0x3D, 0x01), (0, 0x3D, 0x00)), b.Writes);
    }

    [Fact]
    public void ConfigureFailsWhenTheSensorNeverLeavesBoot()
    {
        var (dev, b) = Make();
        using var _ = dev;
        b.SysStatusScript = new Queue<byte>(Enumerable.Repeat((byte)3, 100_000));
        var e = Assert.Throws<DepzException>(() => dev.Configure());
        Assert.Contains("did not finish booting", e.Message);
        Assert.Empty(b.Writes);
    }

    [Fact]
    public void SysStatus4For20PollsIsASelfTestLeftover()
    {
        var (dev, b) = Make();
        using var _ = dev;
        b.SysStatusScript = new Queue<byte>(Enumerable.Repeat((byte)4, 20));
        dev.Configure(Bno055OprMode.Amg);
        Assert.Equal(W((0, 0x3D, 0x01), (0, 0x3D, 0x00)), b.Writes.Take(2));
        Assert.Equal(21, b.Reads.Count(r => r.Addr == Bno055Regs.SysStatus));
    }

    [Fact]
    public void ConfigureFailsWhenFusionNeverStarts()
    {
        var (dev, b) = Make();
        using var _ = dev;
        b.FusionStuck = true;
        var e = Assert.Throws<DepzException>(() => dev.Configure());
        Assert.Contains("fusion did not start", e.Message);
    }
}
