using System.Diagnostics;
using Depz.Sensor.Devices;
using Depz.Sensor.Usb;
using Depz.Sensor.Vl53l7;
using Depz.Sensor.Vl53l8;
using Xunit;

namespace Depz.Sensor.Tests;

/// <summary>Collects the warnings <see cref="Trace.TraceWarning(string)"/> emits while attached.</summary>
internal sealed class TraceWarnings : TraceListener
{
    private readonly List<string> _messages = new();

    public List<string> Messages
    {
        get { lock (_messages) return _messages.ToList(); }
    }

    public override void Write(string? message) { }

    public override void WriteLine(string? message) { }

    public override void TraceEvent(TraceEventCache? eventCache, string source, TraceEventType eventType, int id, string? message)
    {
        if (eventType <= TraceEventType.Warning && message is not null)
            lock (_messages) _messages.Add(message);
    }

    /// <summary>The module-type warnings <paramref name="body"/> raised.</summary>
    public static List<string> Of(Action body)
    {
        var l = new TraceWarnings();
        Trace.Listeners.Add(l);
        try
        {
            body();
        }
        finally
        {
            Trace.Listeners.Remove(l);
        }
        return l.Messages.Where(m => m.Contains("module type")).ToList();
    }
}

/// <summary>
/// The VL53L5CX / L7CX / L7CH device classes over the wire against <see cref="FakeVl53l8Board"/>
/// (the Python test_vl53l7_device.py): what the L5/L7 family does differently from VL53L8 — init
/// over the L5/L7 ULD branch (silicon check, l7cx without a checksum word, the module type and the
/// class warning), the 1536-byte read split, 1 Hz ranging, the board commands PIN_CTRL / GET_INFO /
/// SET_I2C_SPEED and their refusals, the L5CX/L7CX ULD limits and the xtalk "coverglass too good" flag.
/// </summary>
public class Vl53l7DeviceTests
{
    // ── identity ─────────────────────────────────────────────────────────────

    [Theory]
    [InlineData(0xED48, typeof(Vl53l5cxDevice))]
    [InlineData(0xED49, typeof(Vl53l7cxDevice))]
    [InlineData(0xED4A, typeof(Vl53l7chDevice))]
    public void OpenOnLinkResolvesTheClassByPid(int pid, Type cls)
    {
        var b = Vl53l8Rig.NewBoard(Vl53l8Firmware.L7cx);
        using var dev = DepzDevices.OpenOnLink(b.Link, UsbIds.DepzUsbVid, pid, Vl53l8Rig.Timeout);
        Assert.IsType(cls, dev);
        var l7 = (Vl53l7cxDevice)dev;
        Assert.Equal(FakeVl53l8Board.L7Software, l7.GetSoftwareName());
        Assert.True(l7.IsAlive());
        Assert.Null(l7.ModuleType); // readable only after Init()
        Assert.False(l7.XtalkCalibrationFailed);
    }

    [Fact]
    public void OpenOnLinkFallsBackToThePartInTheDeviceName()
    {
        var b = Vl53l8Rig.NewBoard(Vl53l8Firmware.L7cx);
        using var dev = DepzDevices.OpenOnLink(b.Link, timeout: Vl53l8Rig.Timeout); // a dev board: no production PID
        Assert.IsType<Vl53l7cxDevice>(dev); // "… VL53L7CX USB v2.1 …"
    }

    [Fact]
    public void IsAliveRefusesL8SiliconOnAnL7Board()
    {
        using var r = Vl53l8Rig.L7();
        r.P.Reg[0x01] = 0x0C;
        Assert.False(r.Dev.IsAlive());
        r.P.Reg[0x01] = 0x02;
        Assert.True(r.Dev.IsAlive());
        r.P.Reg[0x01] = 0x01; // cut 1.1 with the L5/L7 device id
        Assert.True(r.Dev.IsAlive());
    }

    // ── init: the L5/L7 ULD branch ───────────────────────────────────────────

    [Theory]
    [InlineData("l7", Vl53l8Firmware.L7cx, Vl53l8Driver.ModuleTypeMzevo, false, "MZEVO")] // ULD 2.0.1 publishes no checksum word
    [InlineData("l5", Vl53l8Firmware.L7cx, Vl53l8Driver.ModuleTypeMz, false, "MZ")]
    [InlineData("l7ch", Vl53l8Firmware.L7ch, Vl53l8Driver.ModuleTypeMzevo, true, "MZEVO")] // VL53LMZ 2.0.16: checksum as on VL53L8CH
    public void InitDownloadsTheBlobOfTheClass(string which, Vl53l8Firmware fw, int moduleType, bool checksum, string name)
    {
        using var r = which switch { "l5" => Vl53l8Rig.L5(moduleType), "l7ch" => Vl53l8Rig.L7ch(moduleType), _ => Vl53l8Rig.L7(moduleType) };
        var notes = new List<string>();
        var progress = new List<(int Done, int Total)>();
        var warned = TraceWarnings.Of(() => r.Dev.Init(notes.Add, (done, total) => progress.Add((done, total))));
        Assert.Empty(warned);
        Assert.Equal(fw, r.Dev.Firmware);
        Assert.Equal(checksum, notes.Contains("Sensor FW checksum OK"));
        Assert.Equal(checksum, r.P.Count(0x2FFC, 4) > 0);
        Assert.NotEmpty(progress); // write progress on the firmware banks
        Assert.Equal(progress[^1].Done, progress[^1].Total);
        Assert.Equal(new[] { 0x8000, 0x8000, 0x5000 }, new[] { r.Board.BankBytes[0x09], r.Board.BankBytes[0x0A], r.Board.BankBytes[0x0B] });
        var l7 = (Vl53l7cxDevice)r.Dev;
        Assert.Equal(moduleType, l7.ModuleType);
        Assert.Equal($"Sensor init done (module type {name})", notes[^1]);
        Assert.True(r.Dev.Uld.IsI2c);
        r.P.Zones(Vl53l8Driver.Resolution4x4);
        Assert.Equal(Vl53l8Driver.Resolution4x4, r.Dev.GetResolution());
    }

    [Fact]
    public void InitRefusesNonL5L7SiliconBeforeDownloading()
    {
        using var r = Vl53l8Rig.L7();
        r.P.Reg[0x01] = 0x0C; // an L8 wired to the I2C board
        var e = Assert.Throws<Vl53l8Exception>(() => r.Dev.Init());
        Assert.Equal(Vl53l8Exception.StatusError, e.Code);
        Assert.Contains("not a VL53L5/L7 sensor: device_id=0xF0 revision_id=0x0C", e.Message);
        Assert.Empty(r.Board.BankBytes); // no firmware written
        Assert.Null(((Vl53l7cxDevice)r.Dev).ModuleType);
        Assert.Throws<InvalidStateException>(() => r.Dev.Uld);
    }

    [Theory]
    [InlineData("l7", Vl53l8Driver.ModuleTypeMz, "Vl53l7cxDevice expects module type MZEVO, the sensor reports MZ")]
    [InlineData("l5", Vl53l8Driver.ModuleTypeMzevo, "Vl53l5cxDevice expects module type MZ, the sensor reports MZEVO")]
    [InlineData("l7ch", Vl53l8Driver.ModuleTypeMz, "Vl53l7chDevice expects module type MZEVO, the sensor reports MZ")]
    [InlineData("l7", Vl53l8Driver.ModuleTypeMzplus, "the sensor reports MZPlus")]
    [InlineData("l7", 7, "expects module type MZEVO, the sensor reports 7 ")]
    public void InitWarnsWhenTheModuleTypeContradictsTheClass(string which, int reported, string text)
    {
        using var r = which switch { "l5" => Vl53l8Rig.L5(reported), "l7ch" => Vl53l8Rig.L7ch(reported), _ => Vl53l8Rig.L7(reported) };
        var warned = TraceWarnings.Of(() => r.Dev.Init());
        string w = Assert.Single(warned);
        Assert.Contains(text, w);
        Assert.Contains("check the board's device name", w);
        // a warning, not an error: the blob is shared, ranging works
        Assert.Equal(reported, ((Vl53l7cxDevice)r.Dev).ModuleType);
        r.Dev.SetResolution(Vl53l8Driver.Resolution8x8);
        Assert.Equal(Vl53l8Driver.Resolution8x8, r.Dev.GetResolution());
    }

    // ── transfer limits and the 1 Hz floor ───────────────────────────────────

    [Fact]
    public void RegisterReadsSplitAt1536BytesAndWritesAt2048OnTheI2cBoard()
    {
        using var r = Vl53l8Rig.L7();
        r.Dev.Init();
        int n0 = r.P.Reads.Count;
        byte[] data = r.Dev.Uld.DciReadData(Vl53l8Rig.DciFwFlags, 4000); // a 4012-byte read of UI_CMD_START
        Assert.Equal(4000, data.Length);
        Assert.Equal(Vl53l8Driver.ModuleTypeMzevo, data[1]);
        Assert.Equal(new[] { (0x2C04, 1536), (0x2C04 + 1536, 1536), (0x2C04 + 3072, 940) },
            r.P.Reads.Skip(n0).Where(x => x.Addr >= 0x2C04 && x.Size > 4));
        int w0 = r.P.Writes.Count;
        r.Dev.Uld.DciWriteData(0x5450, new byte[3000]); // a 3012-byte write
        Assert.Equal(new[] { 2048, 964 }, r.P.Writes.Skip(w0).Where(w => w.Data.Length > 12).Select(w => w.Data.Length));
    }

    [Fact]
    public void TheI2cBridgeRefusesAReadAbove1536Bytes()
    {
        using var r = Vl53l8Rig.L7();
        var e = Assert.Throws<StatusException>(() => r.Dev.Request((int)Vl53l8Cmd.ReadReg, Vl53l8Wire.PackReadReg(0, 2048),
            DepzDevice.ExpectReport((int)Vl53l8Rpt.RegData, p => Vl53l8RegData.Unpack(p))));
        Assert.Equal("ERR_INVALID_PARAM", e.StatusName);
        Assert.Throws<ArgumentOutOfRangeException>(() => Vl53l7Wire.PackReadReg(0, 2048)); // the L7 codec refuses it first
    }

    [Fact]
    public void RangingAt1HzIsAllowedOnL7()
    {
        using var r = Vl53l8Rig.L7();
        r.Ready(64, Vl53l8Rig.L7Frame8x8);
        r.Dev.SetRangingFrequencyHz(1); // L8 refuses this (contract 04 §4); L5/L7 range at 1 Hz
        Assert.Equal(1, r.Dev.GetRangingFrequencyHz());
        Assert.Equal(new byte[] { 0, 1, 0, 0 }, r.P.Dci[Vl53l8Rig.DciFreqHz]);
        var e = Assert.Throws<ArgumentOutOfRangeException>(() => r.Dev.SetRangingFrequencyHz(0));
        Assert.Contains(">= 1 Hz", e.Message);
    }

    [Theory]
    [InlineData("l7")]
    [InlineData("cx")]
    public void RangingFrequencyCeilingFollowsTheResolution(string which)
    {
        // contract 11 §3 (ERRATA A13): at most 60 Hz at 4x4 and 15 Hz at 8x8, refused host-side — nothing on the bus
        Assert.Equal(60, Vl53l8cxDevice.MaxRangingFrequencyHz(16));
        Assert.Equal(15, Vl53l8cxDevice.MaxRangingFrequencyHz(64));
        using var r = which == "cx" ? Vl53l8Rig.Cx() : Vl53l8Rig.L7();
        r.Ready(16, which == "cx" ? Vl53l8Rig.L8Frame4x4 : Vl53l8Rig.L7Frame4x4);
        r.Dev.SetRangingFrequencyHz(60);
        Assert.Equal(60, r.Dev.GetRangingFrequencyHz());
        int w0 = r.P.Writes.Count;
        var e = Assert.Throws<ArgumentOutOfRangeException>(() => r.Dev.SetRangingFrequencyHz(61));
        Assert.Contains("<= 60 Hz", e.Message);
        Assert.Equal(w0, r.P.Writes.Count);
        r.Dev.SetResolution(64);
        r.Dev.SetRangingFrequencyHz(15);
        w0 = r.P.Writes.Count;
        e = Assert.Throws<ArgumentOutOfRangeException>(() => r.Dev.SetRangingFrequencyHz(16));
        Assert.Contains("<= 15 Hz", e.Message);
        Assert.Equal(w0, r.P.Writes.Count);
        Assert.Equal(15, r.Dev.GetRangingFrequencyHz());
        r.Dev.SetResolution(16);
        r.Dev.SetRangingFrequencyHz(16); // the ceiling moved back with the resolution
    }

    [Theory]
    [InlineData("l7", 16, Vl53l8Rig.L7Frame4x4)]
    [InlineData("l7", 64, Vl53l8Rig.L7Frame8x8)]
    [InlineData("l7ch", 64, Vl53l8Rig.L7Frame8x8)]
    public void FramesStreamInOneChunkOnTheI2cBoard(string which, int resolution, int size)
    {
        using var r = which == "l7ch" ? Vl53l8Rig.L7ch() : Vl53l8Rig.L7();
        r.Ready(resolution, size);
        r.Dev.SetResolution(resolution);
        r.Dev.SetRangingFrequencyHz(1);
        var got = new List<Vl53l8Frame>();
        using var unsub = r.Dev.OnFrame(f => { lock (got) got.Add(f); });
        using var q = r.Dev.Frames(4);
        r.Dev.StartRanging();
        Assert.True(r.Dev.Ranging);
        Assert.Equal(size, r.Board.FrameSize);
        Assert.Equal(size, r.Dev.Uld.DataReadSize);
        Assert.Null(r.Dev.Uld.FrameSizeMismatch);
        byte[] raw = r.Frame(40);
        r.Board.SendFrame(raw, 777, Vl53l7Wire.StreamChunkMax);
        Assert.Equal(1, r.Board.ChunksSent);
        var f = q.Take(TimeSpan.FromSeconds(2));
        Assert.Equal(resolution, f.Resolution);
        Assert.Equal(resolution, f.DistanceMm.Length);
        Assert.Equal(40, f.DistanceMm[0]);
        Assert.Equal(777UL, f.TimestampUs);
        Assert.True(SpinWait.SpinUntil(() => { lock (got) return got.Count == 1; }, 2000));
        var later = Vl53l8Rig.Later(() => r.Board.SendFrame(r.Frame(41), 778, Vl53l7Wire.StreamChunkMax));
        Assert.Equal(41, r.Dev.GetFrame(TimeSpan.FromSeconds(2)).DistanceMm[0]); // the next frame, contract 07 §3
        later.Wait();
        r.Dev.StopRanging();
        Assert.False(r.Dev.Ranging);
        Assert.False(r.Board.Streaming);
        Assert.Equal(2u, r.Board.IntEdges);
        Assert.Equal(0, r.Dev.FrameParseErrors);
        Assert.Equal(0, r.Dev.ReassemblerDiscards);
    }

    // ── ULD plugins: the L5CX/L7CX limits and the xtalk flag ─────────────────

    [Fact]
    public void L7cxHasSleepButNoDeepSleepAndNoThresholdAutoStop()
    {
        using var r = Vl53l8Rig.L7();
        r.Ready(16, Vl53l8Rig.L7Frame4x4);
        r.P.Reg[0x06] = 0x00;
        r.Dev.SetPowerMode(Vl53l8Driver.PowerModeSleep);
        Assert.Equal(Vl53l8Driver.PowerModeSleep, r.Dev.GetPowerMode());
        Assert.Equal(0, r.P.Count(0x0F, 1)); // ULD 2.0.1 never reads the deep-sleep marker
        r.P.Reg[0x09] = 0x04;
        r.Dev.SetPowerMode(Vl53l8Driver.PowerModeWakeup);
        Assert.Equal(Vl53l8Driver.PowerModeWakeup, r.Dev.GetPowerMode());
        Assert.Equal(Vl53l8Exception.StatusInvalidParam,
            Assert.Throws<Vl53l8Exception>(() => r.Dev.SetPowerMode(Vl53l8Driver.PowerModeDeepSleep)).Code);
        Assert.Equal(Vl53l8Exception.StatusInvalidParam,
            Assert.Throws<Vl53l8Exception>(() => r.Dev.SetDetectionThresholdsAutoStop(true)).Code);
    }

    [Fact]
    public void XtalkCalibrationFailedFlag()
    {
        using var r = Vl53l8Rig.L7();
        var l7 = (Vl53l7cxDevice)r.Dev;
        Assert.False(l7.XtalkCalibrationFailed); // before Init(): nothing calibrated
        r.Ready(16, Vl53l8Rig.L7Frame4x4);
        r.P.Dci[Vl53l8Rig.DciFreqHz] = new byte[] { 0, 15, 0, 0 };
        r.P.Dci[Vl53l8Rig.DciIntTime] = new byte[] { 0x20, 0x4E, 0, 0 }.Concat(new byte[16]).ToArray();
        r.P.Dci[Vl53l8Rig.DciRangingMode] = new byte[] { 0, 3, 0, 2, 0, 0, 0, 0 };
        r.P.Dci[Vl53l8Rig.DciTargetOrder] = new byte[] { Vl53l8Driver.TargetOrderClosest, 0, 0, 0 };
        r.P.XtalkReply = Enumerable.Range(0, 1024).Select(i => (byte)i).ToArray();
        r.Dev.CalibrateXtalk(50, 4, 600);
        Assert.False(l7.XtalkCalibrationFailed);
        Assert.Contains(r.P.Writes, w => w.Addr == 0x2C28 && w.Data.SequenceEqual(Vl53l8Tables.CalibrateXtalkL7cx.ToArray()));
        Assert.Equal(Vl53l8Driver.XtalkBufferSize, r.Dev.GetCaldataXtalk().Length);
        (r.P.Reg[0], r.P.Reg[1], r.P.Reg[2], r.P.Reg[3]) = (0x00, 0x00, 0x7F, 0x80); // "coverglass too good"
        r.Dev.CalibrateXtalk(20, 1, 3000);
        Assert.True(l7.XtalkCalibrationFailed);
    }

    // ── board commands (contract 11 §2) ──────────────────────────────────────

    [Fact]
    public void GetBridgeInfoDecodesTheUnechoedReport()
    {
        using var r = Vl53l8Rig.L7();
        var l7 = (Vl53l7cxDevice)r.Dev;
        Assert.Equal(new Vl53l7Info(0, 0, 0, Vl53l7I2cError.Ok, 1, 0, 1000, 0, false), l7.GetBridgeInfo());
        r.Ready(64, Vl53l8Rig.L7Frame8x8);
        r.Dev.SetResolution(64);
        r.Dev.StartRanging();
        var later = Vl53l8Rig.Later(() => r.Board.SendFrame(r.Frame(40), 1, Vl53l7Wire.StreamChunkMax));
        r.Dev.GetFrame(TimeSpan.FromSeconds(2));
        later.Wait();
        var info = l7.GetBridgeInfo();
        Assert.Equal((1u, Vl53l8Rig.L7Frame8x8, true, 1), (info.IntEdges, (int)info.FrameSize, info.Streaming, (int)info.IntLevel));
        Assert.Equal((1, 1000, 0u, 0), ((int)info.LpnLevel, (int)info.I2cKhz, info.I2cErrors, (int)info.LastI2cError));
    }

    [Fact]
    public void SetI2cSpeedSnapsToTheNearestStepAndReturnsIt()
    {
        using var r = Vl53l8Rig.L7();
        var l7 = (Vl53l7cxDevice)r.Dev;
        Assert.Equal(400, l7.SetI2cSpeedKhz(430));
        Assert.Equal(1000, l7.SetI2cSpeedKhz(960));
        Assert.Equal(100, l7.SetI2cSpeedKhz(1));
        Assert.Equal(1000, l7.SetI2cSpeedKhz(65535));
        Assert.Equal(new[] { 430, 960, 1, 65535 }, r.Board.SpeedRequests);
        Assert.Equal(1000, l7.GetBridgeInfo().I2cKhz);
        // the speed is a board setting: Init() afterwards runs at it
        l7.SetI2cSpeedKhz(400);
        r.Dev.Init();
        Assert.Equal(400, l7.GetBridgeInfo().I2cKhz);
        Assert.Equal(Vl53l8Driver.ModuleTypeMzevo, l7.ModuleType);
    }

    [Theory]
    [InlineData(0)]
    [InlineData(65536)]
    [InlineData(-1)]
    public void SetI2cSpeedRefusesOutOfRangeBeforeSending(int khz)
    {
        using var r = Vl53l8Rig.L7();
        var e = Assert.Throws<ArgumentOutOfRangeException>(() => ((Vl53l7cxDevice)r.Dev).SetI2cSpeedKhz(khz));
        Assert.Contains("khz must be 1..65535", e.Message);
        Assert.Empty(r.Board.SpeedRequests);
    }

    [Fact]
    public void SetI2cSpeedIsBusyWhileStreaming()
    {
        using var r = Vl53l8Rig.L7();
        var l7 = (Vl53l7cxDevice)r.Dev;
        r.Ready(16, Vl53l8Rig.L7Frame4x4);
        r.Dev.StartRanging();
        Assert.Throws<BusyException>(() => l7.SetI2cSpeedKhz(400));
        Assert.Throws<InvalidStateException>(() => r.Dev.SetResolution(64)); // the stream owns the register bank
        Assert.Equal(1000, r.Board.I2cKhz);
        r.Dev.StopRanging();
        Assert.Equal(400, l7.SetI2cSpeedKhz(400));
    }

    [Fact]
    public void LpnOffDropsTheSensorStateAndNeedsInit()
    {
        using var r = Vl53l8Rig.L7();
        var l7 = (Vl53l7cxDevice)r.Dev;
        r.Ready(16, Vl53l8Rig.L7Frame4x4);
        r.Dev.StartRanging();
        l7.PinCtrl(Vl53l7PinAction.LpnOff);
        Assert.Equal(new[] { (int)Vl53l7PinAction.LpnOff }, r.Board.PinActions);
        Assert.False(r.Board.Streaming); // stream stopped by the board
        Assert.False(r.Dev.Ranging);
        Assert.Null(l7.ModuleType);
        var e = Assert.Throws<InvalidStateException>(() => r.Dev.GetResolution());
        Assert.Contains("call Init", e.Message); // nothing goes to the bus: the host knows the sensor is gone
        // LPn low: the sensor NACKs — the bridge reports a hardware fault and counts it
        Assert.False(r.Dev.IsAlive());
        var info = l7.GetBridgeInfo();
        Assert.Equal((0, 1u, Vl53l7I2cError.Nack), ((int)info.LpnLevel, info.I2cErrors, info.LastI2cError));
        l7.PinCtrl(Vl53l7PinAction.LpnOn);
        Assert.True(r.Dev.IsAlive());
        Assert.Null(l7.ModuleType); // LPN_ON restores the bus, not the firmware
        r.Dev.Init();
        Assert.Equal(Vl53l8Driver.ModuleTypeMzevo, l7.ModuleType);
        r.Dev.StopRanging(); // not ranging: a no-op
    }

    [Fact]
    public void I2cRstKeepsTheSensorState()
    {
        using var r = Vl53l8Rig.L7();
        var l7 = (Vl53l7cxDevice)r.Dev;
        r.Ready(16, Vl53l8Rig.L7Frame4x4);
        r.Dev.SetResolution(64);
        l7.PinCtrl(Vl53l7PinAction.I2cRst);
        Assert.Equal(new[] { (int)Vl53l7PinAction.I2cRst }, r.Board.PinActions);
        Assert.Equal(Vl53l8Driver.ModuleTypeMzevo, l7.ModuleType);
        Assert.Equal(64, r.Dev.GetResolution()); // the ULD is still valid
    }

    [Fact]
    public void SoftCycleStopsTheStreamClearsTheI2cCountersAndNeedsInit()
    {
        using var r = Vl53l8Rig.L7();
        var l7 = (Vl53l7cxDevice)r.Dev;
        r.Ready(16, Vl53l8Rig.L7Frame4x4);
        r.Dev.StartRanging();
        r.Board.I2cErrors = 3;
        r.Board.LastI2cError = Vl53l7I2cError.Timeout;
        l7.PinCtrl(Vl53l7PinAction.SoftCycle);
        Assert.Equal(new[] { (int)Vl53l7PinAction.SoftCycle }, r.Board.PinActions);
        Assert.False(r.Board.Streaming);
        Assert.False(r.Dev.Ranging);
        Assert.Null(l7.ModuleType);
        var info = l7.GetBridgeInfo();
        Assert.Equal((false, 1, 0u, Vl53l7I2cError.Ok), (info.Streaming, (int)info.LpnLevel, info.I2cErrors, info.LastI2cError));
        r.Dev.Init();
        Assert.Equal(Vl53l8Driver.ModuleTypeMzevo, l7.ModuleType);
    }

    [Fact]
    public void AnUnknownPinActionIsRefusedBeforeSendingAndAMalformedOneByTheBoard()
    {
        using var r = Vl53l8Rig.L7();
        var l7 = (Vl53l7cxDevice)r.Dev;
        Assert.Throws<ArgumentOutOfRangeException>(() => l7.PinCtrl(4));
        Assert.Empty(r.Board.Cmds());
        var e = Assert.Throws<StatusException>(() => r.Dev.RequestOk((int)Vl53l7Cmd.PinCtrl, new byte[] { 7 }));
        Assert.Equal("ERR_INVALID_PARAM", e.StatusName);
        e = Assert.Throws<StatusException>(() => r.Dev.RequestOk((int)Vl53l7Cmd.PinCtrl, new byte[2]));
        Assert.Equal("ERR_PAYLOAD_FORMAT", e.StatusName);
        e = Assert.Throws<StatusException>(() => r.Dev.RequestOk((int)Vl53l7Cmd.SetI2cSpeed, new byte[3]));
        Assert.Equal("ERR_PAYLOAD_FORMAT", e.StatusName);
        Assert.Empty(r.Board.PinActions);
    }

    [Fact]
    public void TheL8BoardKnowsNoneOfTheI2cCommands()
    {
        using var r = Vl53l8Rig.Cx();
        var e = Assert.Throws<StatusException>(() => r.Dev.RequestOk((int)Vl53l7Cmd.PinCtrl, new byte[] { 1 }));
        Assert.Equal("ERR_INVALID_CMD", e.StatusName);
    }

    // ── CNH (VL53L7CH) ───────────────────────────────────────────────────────

    [Fact]
    public void TheL7chClassArmsCnhForTheNextStart()
    {
        using var r = Vl53l8Rig.L7ch();
        var ch = (Vl53l7chDevice)r.Dev;
        Assert.Null(ch.CnhSetup);
        r.Ready(64, Vl53l8Rig.L7Frame8x8);
        var setup = new Vl53l8CnhSetup();
        setup.InitConfig(10, 24, 1);
        setup.CreateAggMap(64, 0, 0, 2, 2, 4, 4);
        ch.ConfigureCnh(setup);
        Assert.Same(setup, ch.CnhSetup);
        Assert.Equal(setup.Pack(), r.P.Dci[Vl53l8CnhSetup.MiCfgDevIdx]);
        int size = Vl53l8Rig.L7Frame8x8 + setup.RequiredMemory() + 4;
        r.P.FwReports(size);
        using var q = r.Dev.Frames();
        r.Dev.StartRanging();
        Assert.Equal(size, r.Board.FrameSize);
        Assert.Equal(13, FakeVl53l8Platform.U32s(r.P.Dci[Vl53l8Rig.DciOutputList]).Length);
        r.Board.SendFrame(r.Frame(30), 9, Vl53l7Wire.StreamChunkMax);
        int chunks = (size + Vl53l7Wire.StreamChunkMax - 1) / Vl53l7Wire.StreamChunkMax;
        Assert.True(chunks >= 2); // the CNH frame spans several 1536-byte chunks
        Assert.Equal(chunks, r.Board.ChunksSent);
        var f = q.Take(TimeSpan.FromSeconds(2));
        Assert.Equal(64, f.Resolution);
        Assert.Equal(setup.RequiredMemory(), f.CnhRaw!.Length);
        Assert.Equal(0, r.Dev.ReassemblerDiscards);
        r.Dev.StopRanging();
    }

    // ── CNH ↔ motion indicator: one DCI block (ERRATA B4) ───────────────────

    private static bool CnhInOutputList(Vl53l8Rig r) =>
        FakeVl53l8Platform.U32s(r.P.Dci[Vl53l8Rig.DciOutputList]).Any(bh => bh >> 16 == 0xC048);

    private static Vl53l8Frame TakeFrame(Vl53l8Rig r)
    {
        using var q = r.Dev.Frames();
        r.Board.SendFrame(r.Frame(500), 1, Vl53l8Wire.StreamChunkMax);
        return q.Take(TimeSpan.FromSeconds(2));
    }

    /// <summary>CNH and the motion indicator share DCI 0xBFAC (contract 04 §7, ERRATA B4):
    /// configuring one disarms the other, Init() disarms both.</summary>
    [Fact]
    public void CnhAndTheMotionIndicatorDisarmEachOther()
    {
        using var r = Vl53l8Rig.Ch();
        var ch = (Vl53l8chDevice)r.Dev;
        r.Ready(64, Vl53l8Rig.L8Frame8x8);
        var setup = new Vl53l8CnhSetup();
        setup.InitConfig(10, 20, 2);
        setup.CreateAggMap(64, 0, 0, 2, 2, 4, 4);
        const int chFrame8x8 = Vl53l8Rig.L8Frame8x8 - 8; // the CH tail is 8 bytes shorter

        // motion, then CNH: 0xBFAC holds the CNH setup, frames carry no motion
        r.Dev.ConfigureMotionIndicator();
        ch.ConfigureCnh(setup);
        Assert.Equal(setup.Pack(), r.P.Dci[Vl53l8CnhSetup.MiCfgDevIdx]);
        r.P.FwReports(chFrame8x8 + setup.RequiredMemory() + 4);
        r.Dev.StartRanging();
        Assert.True(CnhInOutputList(r));
        var f = TakeFrame(r);
        Assert.Null(f.Motion);
        Assert.Equal(setup.RequiredMemory(), f.CnhRaw!.Length);
        r.Dev.StopRanging();

        // CNH, then motion: 0xBFAC holds the motion config, the CNH block is gone
        r.Dev.ConfigureMotionIndicator();
        Assert.Null(ch.CnhSetup);
        r.P.FwReports(chFrame8x8);
        r.Dev.StartRanging();
        Assert.False(CnhInOutputList(r));
        f = TakeFrame(r);
        Assert.NotNull(f.Motion);
        Assert.Null(f.CnhRaw);
        r.Dev.StopRanging();

        // Init() disarms both. The platform fake has one flat register map (no pages): the first
        // session wrote over the boot status (0x06) and the firmware checksum (0x2FFC) — re-seed
        // them as a booting sensor first.
        ch.ConfigureCnh(setup);
        r.P.Reg[0x06] = 0x01;
        r.P.SetU32(0x2FFC, 0x0C0B6C9E);
        r.Dev.Init();
        Assert.Null(ch.CnhSetup);
        r.Dev.StartRanging();
        Assert.False(CnhInOutputList(r));
        f = TakeFrame(r);
        Assert.Null(f.Motion);
        Assert.Null(f.CnhRaw);
        r.Dev.StopRanging();
        Assert.Equal(0L, r.Dev.FrameParseErrors);
    }

    // ── the L7 wire codec pieces the device does not reach ───────────────────

    [Fact]
    public void WireStartStreamAndSpeedSteps()
    {
        Assert.Equal(new byte[] { 0xA4, 0x05 }, Vl53l7Wire.PackStartStream(Vl53l8Rig.L7Frame8x8));
        Assert.Throws<ArgumentOutOfRangeException>(() => Vl53l7Wire.PackStartStream(0));
        Assert.Throws<ArgumentOutOfRangeException>(() => Vl53l7Wire.PackStartStream(Vl53l7Wire.StreamTotalMax + 1));
        Assert.Equal(new[] { 100, 200, 400, 500, 600, 700, 800, 900, 1000 }, Vl53l7Wire.I2cKhzSteps.ToArray());
        Assert.Equal(new byte[] { 0xE8, 0x03 }, Vl53l7Wire.PackSetI2cSpeed(1000));
        Assert.Equal(new byte[] { 3 }, Vl53l7Wire.PackPinCtrl(Vl53l7PinAction.SoftCycle));
        Assert.Throws<ArgumentOutOfRangeException>(() => Vl53l7Wire.PackWriteReg(0xFFFF, new byte[2]));
        Assert.Throws<ArgumentOutOfRangeException>(() => Vl53l7Wire.PackWriteReg(0, ReadOnlySpan<byte>.Empty));
    }
}
