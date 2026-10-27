using Depz.Sensor.Devices;
using Depz.Sensor.Vl53l8;
using Xunit;

namespace Depz.Sensor.Tests;

/// <summary><see cref="Vl53l8cxDevice"/>'s configuration surface over the register bridge (<see cref="Vl53l8Rig"/>):
/// each getter / setter reaches the ULD's DCI block and reads back what it wrote, every one of them is refused
/// while ranging (the stream owns the register bank), and a corrupt streamed frame is counted, not fatal. The
/// register sequences themselves are held by <see cref="Vl53l8DriverTests"/>.</summary>
public class Vl53l8cxDeviceTests
{
    private const int DciMotionCfg = 0xBFAC, DciPipeControl = 0xDB80, DciSharpener = 0xAED8;

    private static Vl53l8Rig Ready()
    {
        var r = Vl53l8Rig.Cx();
        r.Ready(16, Vl53l8Rig.L8Frame4x4);
        return r;
    }

    // The auto-stop flag set: StopRanging skips the MCU stop handshake the fake never completes (500 polls).
    private static void StopFast(Vl53l8Rig r)
    {
        r.P.SetU32(0x2FFC, 0x4FF, swapped: false);
        r.Dev.StopRanging();
        Assert.False(r.Dev.Ranging);
    }

    [Fact]
    public void EverySettingRoundTripsThroughTheBridge()
    {
        using var r = Ready();
        var dev = r.Dev;
        Assert.Throws<ArgumentException>(() => dev.SetResolution(15));
        dev.SetRangingMode(Vl53l8Driver.RangingModeAutonomous);
        Assert.Equal(Vl53l8Driver.RangingModeAutonomous, dev.GetRangingMode());
        dev.SetIntegrationTimeMs(20);
        Assert.Equal(20, dev.GetIntegrationTimeMs());
        Assert.Throws<Vl53l8Exception>(() => dev.SetIntegrationTimeMs(1)); // 2..1000 ms
        r.P.Dci[DciSharpener] = new byte[16];
        dev.SetSharpenerPercent(25);
        Assert.Equal(25, dev.GetSharpenerPercent());
        dev.SetTargetOrder(Vl53l8Driver.TargetOrderStrongest);
        Assert.Equal(Vl53l8Driver.TargetOrderStrongest, dev.GetTargetOrder());
        dev.SetXtalkMargin(10000);
        Assert.Equal(10000, dev.GetXtalkMargin());

        dev.SetDetectionThresholdsEnable(true);
        Assert.Equal(1, dev.GetDetectionThresholdsEnable());
        dev.SetDetectionThresholdsEnable(false);
        Assert.Equal(0, dev.GetDetectionThresholdsEnable());
        var th = new Vl53l8Advanced.DetectionThreshold(100, 300, Vl53l8Advanced.DistMm, 1, 7, 0);
        dev.SetDetectionThresholds(new[] { th });
        Assert.Equal(th, dev.GetDetectionThresholds()[0]);
        dev.SetDetectionThresholdsAutoStop(true);
        Assert.Equal(1, r.P.Dci[DciPipeControl][3]);

        r.P.Reg[0x09] = 0x04;
        Assert.Equal(Vl53l8Driver.PowerModeWakeup, dev.GetPowerMode());
        dev.SetPowerMode(Vl53l8Driver.PowerModeWakeup); // already awake: only the page dance

        var cfg = dev.ConfigureMotionIndicator(1000, 2500);
        Assert.Equal((int)((1000 / 37.5348 - 4.0) * 2048.5), cfg.RefBinOffset);
        Assert.Equal(cfg.Pack(), r.P.Dci[DciMotionCfg]);
        Assert.Equal(16, dev.GetResolution()); // the indicator was sized for the zones in use
    }

    [Fact]
    public void XtalkCaldataSavesRestoresAndCalibrates()
    {
        using var r = Ready();
        var dev = r.Dev;
        r.P.XtalkReply = Enumerable.Range(0, 1024).Select(i => (byte)i).ToArray();
        byte[] blob = dev.GetCaldataXtalk();
        Assert.Equal(Vl53l8Driver.XtalkBufferSize, blob.Length);
        Assert.Equal(r.P.XtalkReply[8..Vl53l8Driver.XtalkBufferSize], blob[..^8]);
        var restored = blob.ToArray();
        restored[0] ^= 0xFF;
        dev.SetCaldataXtalk(restored);
        Assert.Equal(restored, dev.Uld.XtalkData);
        Assert.Throws<Vl53l8Exception>(() => dev.SetCaldataXtalk(new byte[10]));

        // the calibration saves and restores these; a fresh fake reads zeros, which the setters refuse
        dev.SetRangingFrequencyHz(15);
        dev.SetIntegrationTimeMs(20);
        dev.SetRangingMode(Vl53l8Driver.RangingModeAutonomous);
        dev.SetTargetOrder(Vl53l8Driver.TargetOrderClosest);
        dev.CalibrateXtalk(16, 4, 600);
        Assert.False(dev.Uld.XtalkCalibrationFailed);
        Assert.Equal(r.P.XtalkReply[8..Vl53l8Driver.XtalkBufferSize], dev.Uld.XtalkData[..^8]);
        // the calibration put the settings back
        Assert.Equal((16, 15, 20), (dev.GetResolution(), dev.GetRangingFrequencyHz(), dev.GetIntegrationTimeMs()));
        Assert.Equal((Vl53l8Driver.RangingModeAutonomous, Vl53l8Driver.TargetOrderClosest), (dev.GetRangingMode(), dev.GetTargetOrder()));
    }

    [Fact]
    public void EveryConfigurationCallRefusesWhileRanging()
    {
        using var r = Ready();
        var dev = r.Dev;
        dev.StartRanging();
        r.Board.ClearSent();
        var th = new[] { new Vl53l8Advanced.DetectionThreshold(100, 300, Vl53l8Advanced.DistMm, 1, 7, 0) };
        var calls = new Action[]
        {
            () => dev.SetResolution(64), () => dev.SetRangingFrequencyHz(10),
            () => dev.SetRangingMode(Vl53l8Driver.RangingModeContinuous), () => dev.SetIntegrationTimeMs(20),
            () => dev.SetSharpenerPercent(5), () => dev.SetTargetOrder(Vl53l8Driver.TargetOrderClosest),
            () => dev.SetPowerMode(Vl53l8Driver.PowerModeSleep), () => dev.SetXtalkMargin(50),
            () => dev.CalibrateXtalk(16, 4, 600), () => dev.GetCaldataXtalk(),
            () => dev.SetCaldataXtalk(new byte[Vl53l8Driver.XtalkBufferSize]),
            () => dev.SetDetectionThresholdsEnable(true), () => dev.SetDetectionThresholds(th),
            () => dev.SetDetectionThresholdsAutoStop(true), () => dev.ConfigureMotionIndicator(),
        };
        foreach (var call in calls)
            Assert.Throws<InvalidStateException>(call);
        Assert.Empty(r.Board.Cmds());
        StopFast(r);
    }

    [Fact]
    public void ACorruptStreamedFrameIsCountedAndTheNextOneGoesThrough()
    {
        using var r = Ready();
        var dev = r.Dev;
        using var q = dev.Frames(4);
        Assert.Equal(new long[] { 0 }, dev.StreamDroppedCounts);
        dev.StartRanging();
        // the footer's id echo no longer matches the header's: the decoder refuses the frame
        var plain = Vl53l8FrameDecoder.SwapBuffer(r.Frame(40));
        plain[plain.Length - r.FooterOffset] ^= 0xFF;
        r.Board.SendFrame(Vl53l8FrameDecoder.SwapBuffer(plain), 100, Vl53l8Wire.StreamChunkMax);
        r.Board.SendFrame(r.Frame(41), 200, Vl53l8Wire.StreamChunkMax);
        var f = q.Take(TimeSpan.FromSeconds(2));
        Assert.Equal((200UL, 41), (f.TimestampUs, f.DistanceMm[0]));
        Assert.Equal(1, dev.FrameParseErrors);
        Assert.Equal(0, dev.ReassemblerDiscards);
        StopFast(r);
    }
}
