using System.Buffers.Binary;
using Depz.Sensor.Bno086;
using Depz.Sensor.Devices;
using Depz.Sensor.Protocol;
using Xunit;

namespace Depz.Sensor.Tests;

/// <summary>A SH-2 hub behind the SHTP pass-through bridge: every SEND_SHTP_PACKET is acknowledged,
/// its cargo handed to <c>OnCargo</c>, and the cargos that returns come back as RPT_DATA frames.
/// <c>BusyFirst</c> answers that many sends with ERR_BUSY first.</summary>
internal sealed class FakeBno086Hub
{
    public readonly FakeBoardLink Link = new();
    public readonly List<(int Channel, byte[] Cargo)> Sent = new();
    public Func<int, byte[], IEnumerable<(int Channel, byte[] Cargo)>>? OnCargo;
    public int BusyFirst;
    private readonly int[] _seq = new int[6];

    public FakeBno086Hub() => Link.OnPacket = p =>
    {
        if (p.Cmd != (int)Bno086Cmd.SendShtpPacket)
            return new[] { FakeBoardLink.Status(p.Cmd, Status.Ok) };
        if (BusyFirst > 0)
        {
            BusyFirst--;
            return new[] { FakeBoardLink.Status(p.Cmd, Status.ErrBusy) };
        }
        int channel = p.Payload[2];
        byte[] cargo = p.Payload.AsSpan(4).ToArray();
        lock (Sent)
            Sent.Add((channel, cargo));
        var replies = new List<byte[]> { FakeBoardLink.Status(p.Cmd, Status.Ok) };
        foreach (var (ch, c) in OnCargo?.Invoke(channel, cargo) ?? Array.Empty<(int, byte[])>())
            replies.Add(Data(ch, c));
        return replies;
    };

    public byte[] Data(int channel, byte[] cargo)
    {
        byte[] frame = ShtpLayer.BuildFrame(channel, cargo, _seq[channel]++);
        var payload = new byte[9 + frame.Length];
        BinaryPrimitives.WriteUInt64LittleEndian(payload.AsSpan(1), 1_000_000);
        frame.CopyTo(payload, 9);
        return Link.Reply((int)Bno086Rpt.Data, payload);
    }

    public static byte[] Feature(int sensor, uint intervalUs)
    {
        var r = new byte[17];
        r[0] = (byte)ControlReport.GetFeatureResponse;
        r[1] = (byte)sensor;
        BinaryPrimitives.WriteUInt32LittleEndian(r.AsSpan(5), intervalUs);
        return r;
    }

    public static byte[] FrsRead(int status, int len, int offset, uint d0, uint d1, int type)
    {
        var r = new byte[16];
        r[0] = (byte)ControlReport.FrsReadResponse;
        r[1] = (byte)(len << 4 | status);
        BinaryPrimitives.WriteUInt16LittleEndian(r.AsSpan(2), (ushort)offset);
        BinaryPrimitives.WriteUInt32LittleEndian(r.AsSpan(4), d0);
        BinaryPrimitives.WriteUInt32LittleEndian(r.AsSpan(8), d1);
        BinaryPrimitives.WriteUInt16LittleEndian(r.AsSpan(12), (ushort)type);
        return r;
    }

    public static byte[] FrsWrite(FrsWriteStatus status, int offset) =>
        new[] { (byte)ControlReport.FrsWriteResponse, (byte)status, (byte)offset, (byte)(offset >> 8) };
}

public class Bno086DeviceTests
{
    private const int Control = (int)ShtpChannel.Control;

    private static (Bno086Device, FakeBno086Hub) Make()
    {
        var hub = new FakeBno086Hub();
        var dev = new Bno086Device(hub.Link, TimeSpan.FromMilliseconds(500)) { BusyBackoff = TimeSpan.FromMilliseconds(1) };
        return (dev, hub);
    }

    [Fact]
    public void ABusyBridgeIsRetriedThenGivenUp()
    {
        var (dev, hub) = Make();
        using var _ = dev;
        hub.OnCargo = (ch, c) => new[] { (Control, FakeBno086Hub.Feature(c[1], 10000)) };
        hub.BusyFirst = 2;
        Assert.Equal(10000u, dev.GetFeature(5).IntervalUs);
        hub.BusyFirst = 5;
        Assert.Throws<BusyException>(() => dev.GetFeature(5));
    }

    [Fact]
    public void EnableSkipsAStaleDisabledReadBack()
    {
        var (dev, hub) = Make();
        using var _ = dev;
        int reads = 0;
        hub.OnCargo = (ch, c) => c[0] == (byte)ControlReport.GetFeatureRequest
            ? new[] { (Control, FakeBno086Hub.Feature(c[1], reads++ < 2 ? 0u : 10000u)) }
            : Array.Empty<(int, byte[])>();
        var granted = dev.Enable((int)SensorId.RotationVector, hz: 100);
        Assert.Equal(10000u, granted!.IntervalUs);
        Assert.Equal(3, reads);
        var setFeature = hub.Sent.First(s => s.Cargo[0] == (byte)ControlReport.SetFeatureCommand).Cargo;
        Assert.Equal(10000u, BinaryPrimitives.ReadUInt32LittleEndian(setFeature.AsSpan(5)));
        Assert.Throws<ArgumentException>(() => dev.Enable(5));
        Assert.Throws<ArgumentOutOfRangeException>(() => dev.Enable(5, hz: 0));
    }

    [Fact]
    public void FrsReadCollectsEveryWordOfTheRecord()
    {
        var (dev, hub) = Make();
        using var _ = dev;
        hub.OnCargo = (ch, c) => new[]
        {
            (Control, FakeBno086Hub.FrsRead((int)FrsStatus.NoError, 2, 0, 11, 22, FrsRecordId.UserRecord)),
            (Control, FakeBno086Hub.FrsRead((int)FrsStatus.ReadCompleted, 1, 2, 33, 0, FrsRecordId.UserRecord)),
        };
        Assert.Equal(new uint[] { 11, 22, 33 }, dev.FrsRead(FrsRecordId.UserRecord));
        hub.OnCargo = (ch, c) => new[] { (Control, FakeBno086Hub.FrsRead((int)FrsStatus.RecordEmpty, 0, 0, 0, 0, FrsRecordId.SerialNumber)) };
        Assert.Throws<Sh2Exception>(() => dev.FrsRead(FrsRecordId.SerialNumber));
    }

    [Fact]
    public void FrsWriteSendsTheWordsTwoAtATime()
    {
        var (dev, hub) = Make();
        using var _ = dev;
        hub.OnCargo = (ch, c) => c[0] switch
        {
            (byte)ControlReport.FrsWriteRequest => new[] { (Control, FakeBno086Hub.FrsWrite(FrsWriteStatus.WriteModeReady, 0)) },
            (byte)ControlReport.FrsWriteData when c[2] == 2 => new[] { (Control, FakeBno086Hub.FrsWrite(FrsWriteStatus.WriteCompleted, 2)) },
            (byte)ControlReport.FrsWriteData => new[] { (Control, FakeBno086Hub.FrsWrite(FrsWriteStatus.WordsReceived, c[2])) },
            _ => Array.Empty<(int, byte[])>(),
        };
        dev.FrsWrite(FrsRecordId.UserRecord, new uint[] { 1, 2, 3 });
        var data = hub.Sent.Where(s => s.Cargo[0] == (byte)ControlReport.FrsWriteData).Select(s => s.Cargo).ToList();
        Assert.Equal(2, data.Count);
        Assert.Equal(new uint[] { 1, 2 }, new[] { BinaryPrimitives.ReadUInt32LittleEndian(data[0].AsSpan(4)), BinaryPrimitives.ReadUInt32LittleEndian(data[0].AsSpan(8)) });
        Assert.Equal((2, 3u), (data[1][2], BinaryPrimitives.ReadUInt32LittleEndian(data[1].AsSpan(4))));
    }

    [Fact]
    public void FlushReturnsWhenFlushCompletedArrives()
    {
        var (dev, hub) = Make();
        using var _ = dev;
        hub.OnCargo = (ch, c) => c[0] == (byte)ControlReport.ForceSensorFlush
            ? new[] { ((int)ShtpChannel.InputNormal, new byte[] { 0xEF, c[1] }) }
            : Array.Empty<(int, byte[])>();
        dev.Flush((int)SensorId.Accelerometer);
        Assert.Throws<DepzTimeoutException>(() =>
        {
            hub.OnCargo = (_, _) => Array.Empty<(int, byte[])>();
            dev.Flush((int)SensorId.Gyroscope, TimeSpan.FromMilliseconds(50));
        });
    }

    [Fact]
    public void ProductIdsCollectEveryComponent()
    {
        var (dev, hub) = Make();
        using var _ = dev;
        static byte[] Pid(int part) =>
            new byte[] { 0xF8, 4, 3, 12 }.Concat(BitConverter.GetBytes((uint)part)).Concat(new byte[8]).ToArray();
        hub.OnCargo = (ch, c) => new[] { (Control, Pid(10004563).Concat(Pid(10003606)).ToArray()) }; // one cargo, two responses
        var ids = dev.ProductIds(settle: TimeSpan.FromMilliseconds(30));
        Assert.Equal(new uint[] { 10004563, 10003606 }, ids.Select(i => i.SwPartNumber));
        Assert.Equal("3.12.0", ids[0].Version);
    }

    [Fact]
    public void ShortControlResponseFailsThatRequestAsInvalidReply()
    {
        // A response shorter than its fields (contract 05 §4) fails the request with the
        // invalid_reply kind (contract 07 §3): never a timeout, never a bare ArgumentException.
        var (dev, hub) = Make();
        using var _ = dev;
        hub.OnCargo = (ch, c) => c[0] switch
        {
            (byte)ControlReport.ProductIdRequest => new[] { (Control, new byte[] { 0xF8, 4, 3, 12 }.Concat(new byte[11]).ToArray()) }, // 15 of 16
            (byte)ControlReport.GetFeatureRequest => new[] { (Control, new byte[] { 0xFC, c[1], 0, 0, 0, 0x10, 0x27, 0, 0 }) }, // 9 of 17
            _ => Array.Empty<(int, byte[])>(),
        };
        var pid = Assert.Throws<InvalidReplyException>(() => dev.ProductIds(settle: TimeSpan.FromMilliseconds(30)));
        Assert.Equal((int)ControlReport.ProductIdResponse, pid.Cmd);
        Assert.IsType<ArgumentException>(pid.InnerException);
        var feat = Assert.Throws<InvalidReplyException>(() => dev.GetFeature((int)SensorId.Accelerometer, TimeSpan.FromMilliseconds(200)));
        Assert.Equal((int)ControlReport.GetFeatureResponse, feat.Cmd);
    }

    [Fact]
    public void ErrorsAndCountsCollectTheirResponses()
    {
        var (dev, hub) = Make();
        using var _ = dev;
        static byte[] Resp(int command, int cmdSeq, int respSeq, params byte[] r) =>
            new byte[] { 0xF1, 0, (byte)command, (byte)cmdSeq, (byte)respSeq }.Concat(r).Concat(new byte[11 - r.Length]).ToArray();
        hub.OnCargo = (ch, c) => c[2] switch
        {
            (byte)Sh2Command.Errors => new[]
            {
                (Control, Resp(1, c[1], 0, 1, 7, 3, 9, 2, 4)),
                (Control, Resp(1, c[1], 1, 0, 0, 255)),
            },
            (byte)Sh2Command.Counter => new[]
            {
                (Control, Resp(2, c[1], 0, 0, 0, 0, 5, 0, 0, 0, 6, 0, 0, 0)),
                (Control, Resp(2, c[1], 1, 0, 0, 0, 7, 0, 0, 0, 8, 0, 0, 0)),
            },
            _ => Array.Empty<(int, byte[])>(),
        };
        Assert.Equal(new[] { new ErrorRecord(1, 7, 3, 9, 2, 4) }, dev.GetErrors());
        Assert.Equal(new Counts(5, 5, 6, 7, 8), dev.GetCounts(5));
    }

    [Fact]
    public void ReportsAreFilteredBySensor()
    {
        var (dev, hub) = Make();
        using var _ = dev;
        using var accel = dev.Reports(new[] { (int)SensorId.Accelerometer });
        using var all = dev.Reports();
        var cargo = new byte[] { 0xFB, 0, 0, 0, 0 }
            .Concat(new byte[] { 0x01, 0, 0, 0, 1, 0, 2, 0, 3, 0 })   // accelerometer
            .Concat(new byte[] { 0x02, 0, 0, 0, 1, 0, 2, 0, 3, 0 })   // gyroscope
            .ToArray();
        hub.Link.Inject(hub.Data((int)ShtpChannel.InputNormal, cargo));
        Assert.IsType<Acceleration>(accel.Take(TimeSpan.FromSeconds(1)));
        Assert.IsType<Acceleration>(all.Take(TimeSpan.FromSeconds(1)));
        Assert.IsType<Gyroscope>(all.Take(TimeSpan.FromSeconds(1)));
        Assert.False(accel.TryTake(out var none), $"unexpected {none}");
    }

    [Fact]
    public void ReorientationParamsAreQ14AndChecked()
    {
        byte[] p = Sh2Hub.SetReorientationParams(0, 0, 0.5, 1);
        Assert.Equal(new byte[] { 2, 0, 0, 0, 0, 0x00, 0x20, 0x00, 0x40 }, p);
        Assert.Throws<ArgumentOutOfRangeException>(() => Sh2Hub.SetReorientationParams(2, 0, 0, 0));
        Assert.Throws<ArgumentOutOfRangeException>(() => Sh2Hub.TareNowParams((TareAxis)256));
    }

    /// <summary>Contract 07 §3 (bug hunt 2026-10-29 CSHARP-7): Reports() after Close is a closed
    /// queue, so Take fails at once.</summary>
    [Fact]
    public void ReportsSubscribedAfterCloseAreClosedAtOnce()
    {
        var (dev, _) = Make();
        dev.Close();
        using var q = dev.Reports();
        Assert.True(q.IsClosed, "Reports() after Close is open");
        var sw = System.Diagnostics.Stopwatch.StartNew();
        Assert.Throws<DeviceLostException>(() => q.Take(TimeSpan.FromSeconds(5)));
        Assert.True(sw.Elapsed < TimeSpan.FromSeconds(1), $"Take on a closed device's reports waited {sw.Elapsed}");
    }
}
