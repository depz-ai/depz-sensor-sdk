using System.IO.Compression;
using Depz.Sensor.Devices;
using Depz.Sensor.Protocol;
using Depz.Sensor.Transport;
using Xunit;

namespace Depz.Sensor.Tests;

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
    public void ToHostTimeBeforeSyncIsTheInvalidStateError()
    {
        // Contract 07 §3 (ERRATA A14): a conversion needs a prior SyncTime — the SDK's own error.
        using var dev = new DepzDevice(new FakeBoardLink(), Short);
        Assert.Null(dev.LastTimeSync);
        Assert.Contains("SyncTime", Assert.Throws<InvalidStateException>(() => dev.ToHostTimeUs(0)).Message);
    }

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
        // The first request must be in flight before the second is tried: a slow CI runner may
        // start the task late, and then the two would swap roles.
        Assert.True(SpinWait.SpinUntil(() => { lock (link.Sent) return link.Sent.Count == 1; }, 10_000));
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
        var e = Assert.Throws<InvalidReplyException>(() => dev.ReadMcuTemperature());
        Assert.Equal((int)Cmd.GetMcuTemperature, e.Cmd);
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
        Assert.Throws<ArgumentOutOfRangeException>(() => dev.SetReportPayloadCrc((CrcType)300)); // contract 02 §2 (ERRATA A2)
        Assert.Empty(link.Sent);
    }

    [Fact]
    public void UnsolicitedReportsBecomeEvents()
    {
        var link = new FakeBoardLink();
        using var dev = new DepzDevice(link);
        using var events = dev.Events();
        // in seq order: a gap would be an event of its own
        link.Inject(link.Reply((int)Rpt.Status, new byte[] { 0, (byte)Protocol.Status.ErrHardwareFault }));
        link.Inject(link.Reply((int)Rpt.SequenceError, new byte[] { 7, 9 }));
        link.Inject(link.Reply(0xC5, new byte[] { 0xAB }));
        var a = Assert.IsType<UnsolicitedStatusEvent>(events.Take(TimeSpan.FromSeconds(1)));
        Assert.True(a.IsHardwareFault);
        Assert.Equal(new SequenceErrorEvent(7, 9, true), events.Take(TimeSpan.FromSeconds(1)));
        Assert.Equal(new TextEvent(0xC5, "ab"), events.Take(TimeSpan.FromSeconds(1)));
        Assert.Equal(1, dev.Stats.DeviceSeqErrors);
    }

    /// <summary>Contract 01 §5 step 5 (ERRATA A1): a gap the host sees in the board's seq is a
    /// SeqGapEvent of its own, ahead of the packet it was seen on; the first packet is no gap.</summary>
    [Fact]
    public void HostObservedSeqGapIsAnEvent()
    {
        var link = new FakeBoardLink();
        using var dev = new DepzDevice(link);
        using var events = dev.Events();
        link.Inject(Framing.BuildPacket(0xC5, new byte[] { 5 }, 5));
        link.Inject(Framing.BuildPacket(0xC5, new byte[] { 9 }, 9));
        Assert.Equal(new TextEvent(0xC5, "05"), events.Take(TimeSpan.FromSeconds(1)));
        Assert.Equal(new SeqGapEvent(6, 9, 3), events.Take(TimeSpan.FromSeconds(1)));
        Assert.Equal(new TextEvent(0xC5, "09"), events.Take(TimeSpan.FromSeconds(1)));
        Assert.Equal(1, dev.Stats.SeqGaps);
        Assert.Equal(0, dev.Stats.DeviceSeqErrors);
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
        // Contract 07 §3: a reader that ended leaves the device closed; a later request fails at once.
        Assert.True(SpinWait.SpinUntil(() => dev.IsClosed, 1000), "the device is open after the loss");
        var sw = System.Diagnostics.Stopwatch.StartNew();
        Assert.Throws<DeviceLostException>(() => dev.GetSamplePeriodUs());
        Assert.True(sw.Elapsed < TimeSpan.FromSeconds(1), $"the request after the loss waited {sw.Elapsed}");
    }

    /// <summary>Contract 07 §3 (bug hunt 2026-10-29 CSHARP-7): a queue subscribed after the device
    /// closed is closed at once — Take fails with the closed error instead of waiting out its timeout.</summary>
    [Fact]
    public void AQueueSubscribedAfterCloseIsClosedAtOnce()
    {
        var dev = new Sr04Device(new FakeBoardLink(), Short);
        dev.Close();
        using var q = dev.Measurements();
        using var events = dev.Events();
        Assert.True(q.IsClosed, "Measurements() after Close is open");
        Assert.True(events.IsClosed, "Events() after Close is open");
        var sw = System.Diagnostics.Stopwatch.StartNew();
        Assert.Throws<DeviceLostException>(() => q.Take(TimeSpan.FromSeconds(5)));
        Assert.Throws<DeviceLostException>(() => events.Take(TimeSpan.FromSeconds(5)));
        Assert.True(sw.Elapsed < TimeSpan.FromSeconds(1), $"Take on a closed device's queue waited {sw.Elapsed}");
    }

    /// <summary>CSHARP-7: the same after a loss (the link failed, nobody called Close).</summary>
    [Fact]
    public void AQueueSubscribedAfterALossIsClosedAtOnce()
    {
        var link = new FakeBoardLink();
        using var dev = new Sr04Device(link, Short);
        link.Unplug();
        Assert.True(SpinWait.SpinUntil(() => dev.IsClosed, 1000), "the device is open after the loss");
        using var q = dev.Measurements();
        var sw = System.Diagnostics.Stopwatch.StartNew();
        Assert.Throws<DeviceLostException>(() => q.Take(TimeSpan.FromSeconds(5)));
        Assert.True(sw.Elapsed < TimeSpan.FromSeconds(1), $"Take after the loss waited {sw.Elapsed}");
    }

    /// <summary>Bug hunt 2026-10-29 CSHARP-6: Take(Timeout.InfiniteTimeSpan) waits without a deadline
    /// (Python get(timeout=None)) — for an item, or until the device closes.</summary>
    [Fact]
    public void TakeWithAnInfiniteTimeoutWaits()
    {
        var link = new FakeBoardLink();
        var dev = new Sr04Device(link, Short);
        using var q = dev.Measurements();
        var item = Task.Run(() => q.Take(Timeout.InfiniteTimeSpan));
        var payload = new byte[11];
        payload[0] = (byte)Sr04Cmd.StartMeasurementLoop;
        payload[9] = 7;
        link.Inject(link.Reply((int)Sr04Rpt.Data, payload));
        Assert.True(item.Wait(TimeSpan.FromSeconds(5)), "Take(InfiniteTimeSpan) did not return the item");
        Assert.Equal(7, item.Result.EchoTimeUs);
        var closed = Task.Run(() => Assert.Throws<DeviceLostException>(() => q.Take(Timeout.InfiniteTimeSpan)));
        dev.Close();
        Assert.True(closed.Wait(TimeSpan.FromSeconds(5)), "Take(InfiniteTimeSpan) did not end at Close");
    }

    /// <summary>Contract 07 §3 (bug hunt 2026-10-29 CSHARP-1): a matcher that throws anything fails
    /// its own request with InvalidReplyException; the reader goes on and the device stays open.</summary>
    [Fact]
    public void AMatcherThatThrowsFailsOnlyItsRequest()
    {
        var link = new FakeBoardLink();
        link.OnPacket = p => new[] { link.Reply((int)Rpt.Text, Text(p.Cmd, "name")) };
        using var dev = new DepzDevice(link, TimeSpan.FromSeconds(2));
        var bugs = new Exception[]
        {
            new IndexOutOfRangeException("decode bug"), new NullReferenceException("decode bug"),
            new InvalidOperationException("decode bug"), new InvalidCastException("decode bug"),
            new OverflowException("decode bug"), new DepzException("decode bug"),
        };
        foreach (var bug in bugs)
        {
            var e = Assert.Throws<InvalidReplyException>(() => dev.Request<string>((int)Cmd.GetDeviceName, null,
                (Packet _, out string r) => throw bug));
            Assert.Equal((int)Cmd.GetDeviceName, e.Cmd);
            Assert.Same(bug, e.InnerException);
        }
        Assert.False(dev.IsClosed);
        Assert.Equal("name", dev.GetDeviceName());
    }

    /// <summary>A device whose stream handler throws on report 0x9E — a decoder bug.</summary>
    private sealed class RaisingReports : DepzDevice
    {
        public RaisingReports(ILink link) : base(link, Short) { }

        private protected override bool HandleReport(Packet packet) =>
            packet.Cmd == 0x9E ? throw new InvalidOperationException("stream decoder bug") : false;
    }

    /// <summary>Contract 07 §3: a report handler that throws drops that packet, counted; the reader goes on.</summary>
    [Fact]
    public void AThrowingReportHandlerDropsThatPacket()
    {
        var link = new FakeBoardLink();
        link.OnPacket = p => new[] { link.Reply((int)Rpt.Text, Text(p.Cmd, "still here")) };
        using var dev = new RaisingReports(link);
        using var events = dev.Events();
        link.Inject(link.Reply(0x9E, new byte[] { 1, 2 }));
        link.Inject(link.Reply(0xC5, new byte[] { 0xAB }));
        Assert.Equal(new TextEvent(0xC5, "ab"), events.Take(TimeSpan.FromSeconds(1)));
        Assert.Equal(1, dev.Stats.ReportErrors);
        Assert.Equal("still here", dev.GetDeviceName());
        Assert.False(dev.IsClosed);
    }

    /// <summary>A link whose read fails with something no link is expected to throw.</summary>
    private sealed class BrokenReadLink : ILink
    {
        public readonly FakeBoardLink Inner = new();
        public string Name => "broken";
        public bool IsClosed => Inner.IsClosed;
        public byte[] Read(TimeSpan timeout) => throw new NotSupportedException("read broke");
        public void Write(byte[] data) => Inner.Write(data);
        public void Close() => Inner.Close();
    }

    /// <summary>Contract 07 §3: a reader that ends for any reason other than Close() closes the device —
    /// IsClosed is true and a later request fails at once with DeviceLostException carrying the cause.</summary>
    [Fact]
    public void AReaderThatEndsForAnyReasonClosesTheDevice()
    {
        var link = new BrokenReadLink();
        using var dev = new DepzDevice(link, TimeSpan.FromSeconds(5));
        Assert.True(SpinWait.SpinUntil(() => dev.IsClosed, 1000), "the device is open after its reader ended");
        Assert.True(link.IsClosed, "the link is open");
        var sw = System.Diagnostics.Stopwatch.StartNew();
        var e = Assert.Throws<DeviceLostException>(() => dev.GetDeviceName());
        Assert.True(sw.Elapsed < TimeSpan.FromSeconds(1), $"the request waited {sw.Elapsed}");
        Assert.IsType<NotSupportedException>(e.InnerException);
        lock (link.Inner.Sent)
            Assert.Empty(link.Inner.Sent);
    }

    /// <summary>Contract 07 §3: a callback that throws is counted and dropped — the other callback
    /// and the queue still get the item, the reader goes on (a request completes, the next report is
    /// read).</summary>
    [Fact]
    public void AThrowingCallbackNeverStopsTheReader()
    {
        var link = new FakeBoardLink();
        link.OnPacket = p => new[] { link.Reply((int)Rpt.Text, Text(p.Cmd, "APP_usonic_SR04_v0.97")) };
        using var dev = new Sr04Device(link);
        using var events = dev.Events();
        using var loop = dev.Measurements(capacity: 4);
        var seen = new List<DeviceEvent>();
        using var boom = dev.OnEvent(_ => throw new InvalidOperationException("consumer bug"));
        using var keep = dev.OnEvent(e => { lock (seen) seen.Add(e); });
        link.Inject(link.Reply(0xC5, new byte[] { 0xAB }));
        Assert.Equal(new TextEvent(0xC5, "ab"), events.Take(TimeSpan.FromSeconds(1)));
        lock (seen)
            Assert.Equal(new DeviceEvent[] { new TextEvent(0xC5, "ab") }, seen);
        Assert.Equal(1, dev.Stats.CallbackErrors);
        Assert.Equal("APP_usonic_SR04_v0.97", dev.GetSoftwareName());

        using var boomM = dev.OnMeasurement(_ => throw new InvalidOperationException("consumer bug"));
        var payload = new byte[11];
        payload[0] = (byte)Sr04Cmd.StartMeasurementLoop;
        payload[9] = 7;
        link.Inject(link.Reply((int)Sr04Rpt.Data, payload));
        Assert.Equal(7, loop.Take(TimeSpan.FromSeconds(1)).EchoTimeUs);
        Assert.Equal(2, dev.Stats.CallbackErrors);
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
        SpinWait.SpinUntil(() => { lock (seen) return seen.Count == 5; }, 10_000);
        Assert.Equal(3, q.DroppedCount);
        Assert.Equal(new long[] { 3 }, dev.StreamDroppedCounts);
        Assert.Equal(new[] { 4, 5 }, q.Drain().Select(m => m.EchoTimeUs));
        Assert.Equal(new[] { 1, 2, 3, 4, 5 }, seen); // callbacks see everything
        Assert.False(q.TryTake(out _));
    }

    [Fact]
    public void ExhaustedReplayWaitsOutTheReadTimeoutAndWakesOnClose()
    {
        // contract 08: an exhausted capture is a quiet port — a read returning at once spins the reader thread
        string path = Path.Combine(Path.GetTempPath(), $"depz-{Guid.NewGuid():N}.depzrec");
        File.WriteAllText(path, "{\"schema\": \"depz.rec/1\"}\n");
        try
        {
            var link = new ReplayLink(path);
            Assert.True(link.RxExhausted);
            var sw = System.Diagnostics.Stopwatch.StartNew();
            Assert.Empty(link.Read(TimeSpan.FromMilliseconds(100)));
            Assert.True(sw.ElapsedMilliseconds >= 80, $"returned after {sw.ElapsedMilliseconds} ms");

            Exception? outcome = null;
            var reader = new Thread(() =>
            {
                try
                {
                    link.Read(TimeSpan.FromSeconds(30));
                }
                catch (Exception e)
                {
                    outcome = e;
                }
            });
            reader.Start();
            Assert.True(SpinWait.SpinUntil(() => reader.ThreadState.HasFlag(ThreadState.WaitSleepJoin), 5000));
            link.Close();
            Assert.True(reader.Join(TimeSpan.FromSeconds(5)));
            Assert.IsType<LinkClosedException>(outcome);
        }
        finally
        {
            File.Delete(path);
        }
    }

    [Fact]
    public void TicksToUsDoesNotOverflowOnALongCapture()
    {
        // 3 h at a 1 GHz Stopwatch (Linux / macOS): ticks * 1e6 overflows a long after 2 h 34 min
        const long Ghz = 1_000_000_000;
        Assert.Equal(10_800_000_000L, DepzDevice.TicksToUs(3L * 3600 * Ghz, Ghz));
        Assert.Equal(1_234_567L, DepzDevice.TicksToUs(12_345_670, 10_000_000)); // Windows: 100 ns ticks
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

    // ERRATA A6: there is no `.depzrec.gz` — a capture is plain text, a gzip
    // container is not inflated but refused like any damaged file. The gzipped
    // bytes (mtime 0) are the same 70 in every SDK's test.
    private static readonly byte[] GzippedCapture = Convert.FromHexString(
        "1f8b0800000000000203ab562a4ece48cd4d54b252504a492da8d22b4a4dd63754aae5aa" +
        "562a018a19e80085338b40b245154a204e6209586d6222500d007e3bc00b3d000000");

    [Fact]
    public void ReplayRefusesAGzippedCapture()
    {
        string path = Path.Combine(Path.GetTempPath(), $"depz-{Guid.NewGuid():N}.depzrec");
        try
        {
            using (var plain = new MemoryStream())
            {
                using (var gz = new GZipStream(new MemoryStream(GzippedCapture), CompressionMode.Decompress))
                    gz.CopyTo(plain);
                File.WriteAllBytes(path, plain.ToArray());
            }
            Assert.Equal("depz.rec/1", new ReplayLink(path).Header["schema"]); // a valid capture
            File.WriteAllBytes(path, GzippedCapture);
            Assert.Throws<ArgumentException>(() => new ReplayLink(path));
        }
        finally
        {
            File.Delete(path);
        }
    }

    // Contract 08 «Line rules»: arrays and objects nest at most 256 deep; a 200 KB
    // line of `[[[[…` is a damaged line naming its number, never a stack overflow.
    [Fact]
    public void ALineNested100000DeepIsADamagedLine()
    {
        string deep = new string('[', 100_000) + new string(']', 100_000);
        string path = Path.Combine(Path.GetTempPath(), $"depz-{Guid.NewGuid():N}.depzdata");
        try
        {
            File.WriteAllText(path, "{\"schema\":\"depz.dataset/2\"}\n{\"d\":\"d0\",\"t\":1,\"k\":\"x\",\"v\":{\"a\":" + deep + "}}\n");
            var e = Assert.Throws<ArgumentException>(() => new Dataset.DatasetReader(path).Records);
            Assert.StartsWith($"{path}:2: not JSON: JSON: nested deeper than 256", e.Message);
            File.WriteAllText(path, "{\"schema\":\"depz.rec/1\",\"x\":" + deep + "}\n");
            e = Assert.Throws<ArgumentException>(() => new ReplayLink(path));
            Assert.StartsWith($"{path}:1: not JSON: JSON: nested deeper than 256", e.Message);
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
