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
    private int _page;

    public FakeBno055Board()
    {
        Pages[0][Bno055Regs.SysStatus] = 0;
        Link.OnPacket = OnPacket;
    }

    private IEnumerable<byte[]> OnPacket(Packet p)
    {
        switch (p.Cmd)
        {
            case (int)Bno055Cmd.ReadReg:
            {
                int addr = p.Payload[0], n = p.Payload[1];
                var regs = Pages[_page];
                if (_page == 0)
                {
                    bool fusion = Bno055Regs.IsFusion((Bno055OprMode)(regs[Bno055Regs.OprMode] & 0x0F));
                    regs[Bno055Regs.QuaData] = (byte)(fusion ? 0x10 : 0);
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
        Assert.Throws<InvalidOperationException>(() => dev.UniqueId());
        Assert.Throws<InvalidOperationException>(() => dev.SetInterruptEnable(1));
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
}
