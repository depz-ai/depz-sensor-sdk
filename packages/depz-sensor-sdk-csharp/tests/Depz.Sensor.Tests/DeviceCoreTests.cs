using Depz.Sensor.Devices;
using Depz.Sensor.Protocol;
using Depz.Sensor.Transport;
using Xunit;

namespace Depz.Sensor.Tests;

/// <summary>An in-memory board: every host packet goes to <c>OnPacket</c>, whose reply bytes
/// the device then reads. <c>Inject</c> pushes unsolicited bytes; <c>Unplug</c> makes reads fail.</summary>
internal sealed class FakeBoardLink : ILink
{
    private readonly Queue<byte[]> _rx = new();
    private readonly PacketParser _parser = new();
    private int _seq;
    private bool _closed;
    private bool _unplugged;

    public Func<Packet, IEnumerable<byte[]>>? OnPacket;
    public readonly List<Packet> Sent = new();

    public string Name => "fake";

    public bool IsClosed
    {
        get { lock (_rx) return _closed; }
    }

    public static byte[] Status(int cmd, Status status) =>
        Framing.BuildPacket((int)Rpt.Status, new[] { (byte)cmd, (byte)status });

    public byte[] Reply(int cmd, byte[] payload) => Framing.BuildPacket(cmd, payload, _seq++ & 0xFF);

    public void Inject(byte[] bytes)
    {
        lock (_rx)
        {
            _rx.Enqueue(bytes);
            Monitor.PulseAll(_rx);
        }
    }

    public void Unplug()
    {
        lock (_rx)
        {
            _unplugged = true;
            Monitor.PulseAll(_rx);
        }
    }

    public byte[] Read(TimeSpan timeout)
    {
        lock (_rx)
        {
            if (_rx.Count == 0 && !_closed && !_unplugged)
                Monitor.Wait(_rx, timeout);
            if (_closed)
                throw new LinkClosedException("fake closed");
            if (_unplugged)
                throw new DeviceLostException("fake unplugged");
            return _rx.Count > 0 ? _rx.Dequeue() : Array.Empty<byte>();
        }
    }

    public void Write(byte[] data)
    {
        foreach (var ev in _parser.Feed(data))
        {
            if (ev is not Packet p)
                continue;
            lock (Sent)
                Sent.Add(p);
            foreach (var r in OnPacket?.Invoke(p) ?? Array.Empty<byte[]>())
                Inject(r);
        }
    }

    public void Close()
    {
        lock (_rx)
        {
            _closed = true;
            Monitor.PulseAll(_rx);
        }
    }
}

public class DeviceCoreTests
{
    private static readonly TimeSpan Short = TimeSpan.FromMilliseconds(100);

    private static byte[] Text(int cmd, string s) =>
        new[] { (byte)cmd }.Concat(System.Text.Encoding.ASCII.GetBytes(s)).Concat(new byte[] { 0, 0xFF }).ToArray();

    [Fact]
    public void TextRequestStripsTrailingPadding()
    {
        var link = new FakeBoardLink();
        link.OnPacket = p => new[] { link.Reply((int)Rpt.Text, Text(p.Cmd, "APP_usonic_SR04_v0.97")) };
        using var dev = new DepzDevice(link);
        Assert.Equal("APP_usonic_SR04_v0.97", dev.GetSoftwareName());
        Assert.Equal("sr04", IdentityKind(dev.Identify()));
        Assert.Equal(2, dev.Stats.TxPackets);
    }

    private static string IdentityKind(Identity id) => id.SensorType.ToString()!.ToLowerInvariant();

    [Fact]
    public void NoReplyTimesOutNamingTheOpcode()
    {
        var link = new FakeBoardLink();
        using var dev = new DepzDevice(link, Short);
        var e = Assert.Throws<DepzTimeoutException>(() => dev.GetSerialNumber());
        Assert.Equal((int)Cmd.GetSerial, e.Cmd);
        Assert.Contains("0x05", e.Message);
    }

    [Fact]
    public void NonOkStatusThrowsTypedErrors()
    {
        var link = new FakeBoardLink();
        link.OnPacket = p => new[]
        {
            FakeBoardLink.Status(p.Cmd, p.Cmd == (int)Cmd.SetPayloadCrcType ? Protocol.Status.ErrBusy : Protocol.Status.ErrCmdNotSupported),
        };
        using var dev = new DepzDevice(link, Short);
        Assert.Throws<BusyException>(() => dev.SetReportPayloadCrc(CrcType.Crc8));
        var e = Assert.Throws<StatusException>(() => dev.GetDeviceName());
        Assert.Equal("ERR_CMD_NOT_SUPPORTED", e.StatusName);
    }

    [Fact]
    public void OneRequestPerOpcodeInFlight()
    {
        var link = new FakeBoardLink();
        using var dev = new DepzDevice(link, TimeSpan.FromSeconds(1));
        // Closing the device fails the request still waiting.
        var first = Task.Run(() => Assert.Throws<LinkClosedException>(() => dev.GetSerialNumber()));
        SpinWait.SpinUntil(() => { lock (link.Sent) return link.Sent.Count == 1; }, 1000);
        Assert.Throws<BusyException>(() => dev.Request((int)Cmd.GetSerial, null, DepzDevice.ExpectText((int)Cmd.GetSerial)));
        dev.Close();
        first.Wait();
    }

    [Fact]
    public void OkAckBeforeTheDataReportKeepsWaiting()
    {
        var link = new FakeBoardLink();
        link.OnPacket = p => new[]
        {
            FakeBoardLink.Status(p.Cmd, Protocol.Status.Ok),
            link.Reply((int)Rpt.PayloadCrcType, new byte[] { 2 }),
        };
        using var dev = new DepzDevice(link, Short);
        Assert.Equal(CrcType.Crc16, dev.GetReportPayloadCrc());
    }

    [Fact]
    public void ShortReplyFailsItsRequestAndTheReaderGoesOn()
    {
        var link = new FakeBoardLink();
        link.OnPacket = p => p.Cmd == (int)Cmd.GetMcuTemperature
            ? new[] { link.Reply((int)Rpt.Temperature, new byte[3]) }
            : new[] { link.Reply((int)Rpt.Text, Text(p.Cmd, "x")) };
        using var dev = new DepzDevice(link, Short);
        var e = Assert.Throws<DepzException>(() => dev.ReadMcuTemperature());
        Assert.Contains("does not decode", e.Message);
        Assert.Equal("x", dev.GetDeviceName());
    }

    [Fact]
    public void ArgumentsAreCheckedBeforeAnythingIsSent()
    {
        var link = new FakeBoardLink();
        using var dev = new Sr04Device(link, Short);
        Assert.Throws<ArgumentOutOfRangeException>(() => dev.SetSamplePeriodUs(-1));
        Assert.Throws<ArgumentOutOfRangeException>(() => dev.SetSamplePeriodUs(1L << 32));
        Assert.Throws<ArgumentOutOfRangeException>(() => dev.SetEchoDecayUs(65536));
        Assert.Throws<ArgumentOutOfRangeException>(() => dev.GetSyncPin(0));
        Assert.Throws<ArgumentOutOfRangeException>(() => dev.SetReportPayloadCrc((CrcType)4));
        Assert.Empty(link.Sent);
    }

    [Fact]
    public void UnsolicitedReportsBecomeEvents()
    {
        var link = new FakeBoardLink();
        using var dev = new DepzDevice(link);
        using var events = dev.Events();
        link.Inject(FakeBoardLink.Status(0, Protocol.Status.ErrHardwareFault));
        link.Inject(link.Reply((int)Rpt.SequenceError, new byte[] { 7, 9 }));
        link.Inject(link.Reply(0xC5, new byte[] { 0xAB }));
        var a = Assert.IsType<UnsolicitedStatusEvent>(events.Take(TimeSpan.FromSeconds(1)));
        Assert.True(a.IsHardwareFault);
        Assert.Equal(new SequenceErrorEvent(7, 9, true), events.Take(TimeSpan.FromSeconds(1)));
        Assert.Equal(new TextEvent(0xC5, "ab"), events.Take(TimeSpan.FromSeconds(1)));
        Assert.Equal(1, dev.Stats.DeviceSeqErrors);
    }

    [Fact]
    public void UnplugFailsPendingAndEndsStreams()
    {
        var link = new FakeBoardLink();
        using var dev = new Sr04Device(link, TimeSpan.FromSeconds(5));
        using var events = dev.Events();
        using var loop = dev.Measurements();
        var waiting = Task.Run(() => Assert.Throws<DeviceLostException>(() => dev.GetSamplePeriodUs()));
        // Unplug only once the request is in flight: a slow runner may start the task late, and a
        // request sent after the unplug would wait out its timeout instead.
        Assert.True(SpinWait.SpinUntil(() => { lock (link.Sent) return link.Sent.Count == 1; }, 5000), "the request was never sent");
        link.Unplug();
        Assert.True(waiting.Wait(TimeSpan.FromSeconds(2)), "the pending request did not fail");
        Assert.IsType<DisconnectedEvent>(events.Take(TimeSpan.FromSeconds(1)));
        Assert.Throws<DeviceLostException>(() => loop.Take(TimeSpan.FromSeconds(1)));
        Assert.True(loop.IsClosed);
    }

    [Fact]
    public void QueuesDropTheOldestAndCount()
    {
        var link = new FakeBoardLink();
        using var dev = new Sr04Device(link);
        var seen = new List<int>();
        using var cb = dev.OnMeasurement(m => { lock (seen) seen.Add(m.EchoTimeUs); });
        using var q = dev.Measurements(capacity: 2);
        for (int i = 1; i <= 5; i++)
        {
            var payload = new byte[11];
            payload[0] = (byte)Sr04Cmd.StartMeasurementLoop;
            payload[9] = (byte)i;
            link.Inject(link.Reply((int)Sr04Rpt.Data, payload));
        }
        SpinWait.SpinUntil(() => { lock (seen) return seen.Count == 5; }, 2000);
        Assert.Equal(3, q.DroppedCount);
        Assert.Equal(new long[] { 3 }, dev.StreamDroppedCounts);
        Assert.Equal(new[] { 4, 5 }, q.Drain().Select(m => m.EchoTimeUs));
        Assert.Equal(new[] { 1, 2, 3, 4, 5 }, seen); // callbacks see everything
        Assert.False(q.TryTake(out _));
    }

    [Fact]
    public void RecordingReplaysByteForByte()
    {
        string path = Path.Combine(Path.GetTempPath(), $"depz-{Guid.NewGuid():N}.depzrec");
        try
        {
            var board = new FakeBoardLink();
            board.OnPacket = p => new[] { board.Reply((int)Rpt.Text, Text(p.Cmd, "APP_usonic_SR04_v0.97")) };
            using (var rec = new DepzDevice(new RecordingLink(board, path, new Dictionary<string, object?> { ["note"] = "test" })))
                Assert.Equal("APP_usonic_SR04_v0.97", rec.GetSoftwareName());

            var replay = new ReplayLink(path, strictTx: true);
            Assert.Equal("test", replay.Header["note"]);
            using var dev = new DepzDevice(replay);
            Assert.Equal("APP_usonic_SR04_v0.97", dev.GetSoftwareName());
            Assert.True(replay.RxExhausted);
            Assert.Equal(0, replay.TxRemaining);
            Assert.Throws<ReplayMismatchException>(() => dev.Send((int)Cmd.GetSerial));
        }
        finally
        {
            File.Delete(path);
        }
    }

    [Fact]
    public void OpenOnLinkResolvesAndAttachesInPlace()
    {
        var link = new FakeBoardLink();
        link.OnPacket = p => p.Cmd switch
        {
            (int)Cmd.GetNameActiveSoftware => new[] { link.Reply((int)Rpt.Text, Text(p.Cmd, "APP_usonic_SR04_v0.97")) },
            (int)Cmd.GetDeviceName => new[] { FakeBoardLink.Status(p.Cmd, Protocol.Status.ErrCmdNotSupported) },
            (int)Sr04Cmd.GetEchoDecay => new[] { link.Reply((int)Sr04Rpt.EchoDecay, new byte[] { 0x88, 0x13 }) },
            _ => Array.Empty<byte[]>(),
        };
        using var dev = DepzDevices.OpenOnLink(link, timeout: Short);
        var sr04 = Assert.IsType<Sr04Device>(dev);
        Assert.Equal(5000, sr04.GetEchoDecayUs());
        // The probe's three requests and this one ran on one connection: seq 0..3.
        Assert.Equal(new[] { 0, 1, 2, 3 }, link.Sent.Select(p => p.Seq));
    }

    [Fact]
    public void OpenOnLinkRefusesBootloaderAndSilence()
    {
        var boot = new FakeBoardLink();
        boot.OnPacket = p => new[] { boot.Reply((int)Rpt.Text, Text(p.Cmd, "BOOTDEPZ_v1.11")) };
        Assert.Throws<BootloaderModeException>(() => DepzDevices.OpenOnLink(boot, timeout: Short));
        Assert.True(boot.IsClosed);

        var silent = new FakeBoardLink();
        Assert.Throws<NoDepzDeviceException>(() => DepzDevices.OpenOnLink(silent, timeout: Short));
        Assert.True(silent.IsClosed);
    }
}
