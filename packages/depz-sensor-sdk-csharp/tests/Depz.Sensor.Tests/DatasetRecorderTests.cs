using System.Buffers.Binary;
using Depz.Sensor.Dataset;
using Depz.Sensor.Devices;
using Depz.Sensor.Protocol;
using Xunit;

namespace Depz.Sensor.Tests;

/// <summary>An SR04 board that answers the common commands a recorder needs (sync, serial, name, MCU
/// temperature) on a device clock of its own, and streams a measurement on request.</summary>
internal sealed class FakeSr04Board
{
    public readonly FakeBoardLink Link = new();
    public ulong McuTimeUs;
    // 50 ms as the Python fake: device time steps must exceed the gap between the two devices' syncs,
    // or the merged order depends on how fast Add() ran.
    public ulong SamplePeriodUs = 50_000;

    public FakeSr04Board(ulong mcuTimeUs)
    {
        McuTimeUs = mcuTimeUs;
        Link.OnPacket = p =>
        {
            switch (p.Cmd)
            {
                case (int)Cmd.SyncTime:
                {
                    var r = new byte[24];
                    p.Payload.AsSpan(0, 8).CopyTo(r);
                    BinaryPrimitives.WriteUInt64LittleEndian(r.AsSpan(8), McuTimeUs);
                    BinaryPrimitives.WriteUInt64LittleEndian(r.AsSpan(16), McuTimeUs);
                    return new[] { Link.Reply((int)Rpt.SyncTime, r) };
                }
                case (int)Cmd.GetSerial:
                    return new[] { Link.Reply((int)Rpt.Text, Text(p.Cmd, "SN000042")) };
                case (int)Cmd.GetNameActiveSoftware:
                    return new[] { Link.Reply((int)Rpt.Text, Text(p.Cmd, "APP_usonic_SR04_v0.97")) };
                case (int)Cmd.GetMcuTemperature:
                {
                    var r = new byte[10];
                    BinaryPrimitives.WriteInt16LittleEndian(r.AsSpan(8), 251);
                    return new[] { Link.Reply((int)Rpt.Temperature, r) };
                }
                default:
                    return new[] { FakeBoardLink.Status(p.Cmd, Status.Ok) };
            }
        };
    }

    private static byte[] Text(int cmd, string s) =>
        new[] { (byte)cmd }.Concat(System.Text.Encoding.ASCII.GetBytes(s)).Concat(new byte[] { 0 }).ToArray();

    public void SendMeasurement(int source, int echoUs = 5831)
    {
        McuTimeUs += SamplePeriodUs;
        var p = new byte[11];
        p[0] = (byte)source;
        BinaryPrimitives.WriteUInt64LittleEndian(p.AsSpan(1), McuTimeUs);
        BinaryPrimitives.WriteUInt16LittleEndian(p.AsSpan(9), (ushort)echoUs);
        Link.Inject(Link.Reply((int)Sr04Rpt.Data, p));
    }
}

public class DatasetRecorderTests
{
    private static string TempPath(string suffix = ".depzdata") =>
        Path.Combine(Path.GetTempPath(), $"depz-{Guid.NewGuid():N}{suffix}");

    [Fact]
    public void TwoDevicesLandOnOneHostTimeline()
    {
        var a = new FakeSr04Board(1_000_000);
        var b = new FakeSr04Board(9_000_000_000);
        string path = TempPath();
        try
        {
            using (var devA = new Sr04Device(a.Link, TimeSpan.FromSeconds(1)))
            using (var devB = new Sr04Device(b.Link, TimeSpan.FromSeconds(1)))
            using (var rec = new SessionRecorder(path, note: "unit test", temperatureHz: null))
            {
                Assert.Equal(("d0", "d1"), (rec.Add(devA), rec.Add(devB)));
                rec.Start();
                Assert.Throws<InvalidOperationException>(() => rec.Add(devA));
                a.SendMeasurement(0x37);
                Thread.Sleep(20);
                b.SendMeasurement(0x37);
                Thread.Sleep(20);
                a.SendMeasurement(0x37);
                Thread.Sleep(20);
                b.SendMeasurement(0x36);
                var until = DateTime.UtcNow.AddSeconds(2);
                while (rec.RecordsWritten < 4 && DateTime.UtcNow < until)
                    Thread.Sleep(5);
            }
            var reader = new DatasetReader(path);
            Assert.Equal(new[] { "d0", "d1" }, reader.Devices.Keys.OrderBy(k => k));
            Assert.All(reader.Devices.Values, d => Assert.Equal(("sr04", "SN000042"), (d.SensorType, d.Serial)));
            Assert.Equal("unit test", reader.Note);
            var recs = reader.Records;
            Assert.Equal(4, recs.Count);
            Assert.Equal(new[] { "d0", "d1", "d0", "d1" }, recs.Select(r => r.DeviceId));
            Assert.Equal("once", recs[3].Value["source"]);
            Assert.Equal(5831L, recs[0].Value["echo_us"]);
            // t = device time - offset: both clocks map onto the host clock
            Assert.All(recs, r => Assert.True(Math.Abs(r.THostUs - DepzDevice.HostNowUs()) < 10_000_000));
        }
        finally
        {
            File.Delete(path);
        }
    }

    [Fact]
    public void TheTemperatureSamplerWritesMcuRecords()
    {
        var a = new FakeSr04Board(1_000_000);
        string path = TempPath(".depzdata.gz");
        try
        {
            using (var dev = new Sr04Device(a.Link, TimeSpan.FromSeconds(1)))
            using (var rec = new SessionRecorder(path, temperatureHz: 50))
            {
                rec.Add(dev);
                rec.Start();
                Thread.Sleep(150);
            }
            var temps = new DatasetReader(path).Records.Where(r => r.Kind == "temperature").ToList();
            Assert.NotEmpty(temps);
            Assert.All(temps, t => Assert.Equal((25.1, "mcu"), ((double)t.Value["celsius"]!, (string)t.Value["source"]!)));
        }
        finally
        {
            File.Delete(path);
        }
    }

    [Fact]
    public void WriterAndReaderRoundTripAndSortWithinADevice()
    {
        string path = TempPath();
        try
        {
            var devices = new Dictionary<string, IReadOnlyDictionary<string, object?>>
            {
                ["d0"] = new Dictionary<string, object?>(), ["d1"] = new Dictionary<string, object?>(),
            };
            using (var w = new DatasetWriter(path, devices))
            {
                foreach (var (d, t) in new[] { ("d0", 300L), ("d0", 100L), ("d1", 200L), ("d0", 250L), ("d1", 50L) })
                    w.Write(d, t, "event", new Dictionary<string, object?> { ["arr"] = new List<object?> { 1L, 2.5 } });
                Assert.Equal(5, w.RecordsWritten);
            }
            var r = new DatasetReader(path);
            Assert.Equal(DatasetWriter.Schema, r.Header["schema"]);
            Assert.Equal(new[] { ("d1", 50L), ("d0", 100L), ("d1", 200L), ("d0", 250L), ("d0", 300L) },
                r.Records.Select(x => (x.DeviceId, x.THostUs)));
            Assert.Equal(250, r.DurationUs);
            Assert.Equal(new List<object?> { 1L, 2.5 }, r.Records[0].Value["arr"]);

            var got = new List<long>();
            r.Play(x => got.Add(x.THostUs), speed: 0);
            Assert.Equal(5, got.Count);
            using var cancel = new CancellationTokenSource();
            got.Clear();
            r.Play(x =>
            {
                got.Add(x.THostUs);
                if (got.Count == 2)
                    cancel.Cancel();
            }, speed: 0.5, cancel: cancel.Token);
            Assert.Equal(2, got.Count);
        }
        finally
        {
            File.Delete(path);
        }
    }

    [Theory]
    [InlineData(0.25, 0.2)]
    [InlineData(0.35, 0.3)]   // 0.34999999999999997779… in binary
    [InlineData(0.05, 0.1)]   // 0.05000000000000000277…
    [InlineData(-0.25, -0.2)]
    [InlineData(1234.56789, 1234.6)]
    [InlineData(0.0, 0.0)]
    public void Round1IsPythonsRound(double x, double want) => Assert.Equal(want, DatasetRecords.Round1(x));
}

public class DatasetPlayerTests
{
    private const double Eps = 1e-6;

    private sealed class FakeClock
    {
        public double T = 100.0;
        public double Now() => T;
    }

    private static string Write(IEnumerable<(string D, long T)> events)
    {
        string path = Path.Combine(Path.GetTempPath(), $"depz-{Guid.NewGuid():N}.depzdata");
        var devices = new Dictionary<string, IReadOnlyDictionary<string, object?>>
        {
            ["d0"] = new Dictionary<string, object?> { ["sensor_type"] = "sr04" },
            ["d1"] = new Dictionary<string, object?> { ["sensor_type"] = "vl53l8" },
        };
        using var w = new DatasetWriter(path, devices, "test");
        int i = 0;
        foreach (var (d, t) in events)
            w.Write(d, t, "sr04", new Dictionary<string, object?> { ["n"] = (long)i++ });
        return path;
    }

    private static string Sample() => Write(new[] { ("d0", 1000L), ("d1", 900L), ("d0", 51000L), ("d1", 30900L) });

    private static string Evenly(int n = 5, long gapUs = 100_000) =>
        Write(Enumerable.Range(0, n).Select(i => ("d0", 1_000_000 + i * gapUs)));

    private static (DatasetPlayer P, FakeClock Clock, List<long> Seen, List<PlayerState> States) Make(string path, double speed = 1.0)
    {
        var clock = new FakeClock();
        var p = new DatasetPlayer(new DatasetReader(path), speed, clock.Now, threaded: false);
        var seen = new List<long>();
        var states = new List<PlayerState>();
        p.OnRecord(r => seen.Add(r.THostUs));
        p.OnState(states.Add);
        return (p, clock, seen, states);
    }

    [Fact]
    public void PlaysPacedPausesResumesSeeks()
    {
        var (p, clock, seen, states) = Make(Sample());
        Assert.Equal((PlayerState.Idle, 0L, 50100L), (p.State, p.PositionUs, p.DurationUs));
        p.Play();
        Assert.Empty(seen);
        double? wait = p.Tick();
        Assert.Equal(new[] { 900L }, seen);
        Assert.Equal(100e-6, wait!.Value, 9);
        clock.T += 100e-6 + Eps;
        p.Tick();
        Assert.Equal(new[] { 900L, 1000L }, seen);
        clock.T += 0.0299 - 2 * Eps;
        p.Tick();
        Assert.Equal(2, seen.Count);
        clock.T += 2 * Eps;
        p.Tick();
        Assert.Equal(new[] { 900L, 1000L, 30900L }, seen);
        Assert.Equal(30000, p.PositionUs);

        p.Pause();
        clock.T += 1.0;
        Assert.Null(p.Tick());
        Assert.Equal(3, seen.Count);

        p.Play(); // resume: the next record waits its full gap (20.1 ms) again
        clock.T += 0.0201 - Eps;
        p.Tick();
        Assert.Equal(3, seen.Count);
        clock.T += 2 * Eps;
        Assert.Null(p.Tick());
        Assert.Equal(new[] { 900L, 1000L, 30900L, 51000L }, seen);
        Assert.Equal((PlayerState.Done, 50100L), (p.State, p.PositionUs));
        Assert.Equal(new[] { PlayerState.Playing, PlayerState.Paused, PlayerState.Playing, PlayerState.Done }, states);

        seen.Clear();
        p.SeekUs(30000);
        Assert.Equal((PlayerState.Done, 100L), (p.State, p.PositionUs));
        p.Speed = 0;
        p.Play();
        p.Tick();
        Assert.Equal(new[] { 30900L, 51000L }, seen);
    }

    [Fact]
    public void SpeedChangeMidWaitRescalesTheRemainderAndASlowConsumerDoesNotDrift()
    {
        var (p, clock, seen, _) = Make(Evenly());
        p.Play();
        p.Tick();
        clock.T += 0.05;
        p.Tick();
        p.Speed = 0.5; // remaining 50 ms of media → 100 ms of wall time
        Assert.Equal(0.1, p.Tick()!.Value, 9);
        clock.T += 0.1 + Eps;
        p.Tick();
        Assert.Equal(2, seen.Count);

        var (q, c2, seen2, _) = Make(Evenly());
        q.Play();
        q.Tick();
        c2.T += 0.35; // records 1..3 overdue
        double? w = q.Tick();
        Assert.Equal(4, seen2.Count);
        Assert.Equal(0.05, w!.Value, 9);
        Assert.Throws<ArgumentOutOfRangeException>(() => q.Speed = -1);
        Assert.Throws<ArgumentOutOfRangeException>(() => q.Speed = double.NaN);
    }

    [Fact]
    public void SeekWhilePausedSkipsAndPastTheEndEndsPlayback()
    {
        var (p, clock, seen, states) = Make(Evenly());
        p.Play();
        p.Tick();
        p.Pause();
        p.SeekUs(250_000);
        Assert.Equal((200_000L, PlayerState.Paused), (p.PositionUs, p.State));
        p.Play();
        Assert.Equal(0.1, p.Tick()!.Value, 9);
        clock.T += 0.1;
        p.Tick();
        Assert.Equal(new[] { 1_000_000L, 1_300_000L }, seen);

        var (q, _, seen2, states2) = Make(Evenly());
        q.Play();
        q.Tick();
        q.SeekUs(1e9);
        Assert.Equal((PlayerState.Done, 400_000L), (q.State, q.PositionUs));
        Assert.Equal(new[] { PlayerState.Playing, PlayerState.Done }, states2);
        Assert.Throws<ArgumentException>(() => q.SeekUs(double.NaN));
    }

    [Fact]
    public void ACallbackErrorPausesAndPropagatesAndCallbacksMayControlThePlayer()
    {
        var (p, _, seen, states) = Make(Evenly(), speed: 0);
        p.OnRecord(r =>
        {
            if (r.THostUs == 1_200_000)
                throw new InvalidOperationException("consumer failed");
        });
        p.Play();
        Assert.Equal("consumer failed", Assert.Throws<InvalidOperationException>(() => p.Tick()).Message);
        Assert.Equal((PlayerState.Paused, 200_000L), (p.State, p.PositionUs));
        p.Play();
        p.Tick();
        Assert.Equal(5, seen.Count);
        Assert.Equal(new[] { PlayerState.Playing, PlayerState.Paused, PlayerState.Playing, PlayerState.Done }, states);

        var (q, _, seen2, states2) = Make(Evenly(), speed: 0);
        q.OnRecord(r =>
        {
            if (r.THostUs == 1_100_000)
                q.Pause();
        });
        q.Play();
        Assert.Null(q.Tick());
        Assert.Equal(new[] { 1_000_000L, 1_100_000L }, seen2);
        Assert.Equal(new[] { PlayerState.Playing, PlayerState.Paused }, states2);
    }

    [Fact]
    public void EmptyDatasetAndUnsubscribeAndClose()
    {
        var (p, _, seen, states) = Make(Write(Array.Empty<(string, long)>()));
        p.Play();
        Assert.Equal(new[] { PlayerState.Playing, PlayerState.Done }, states);
        p.SeekUs(100);
        Assert.Equal(0, p.PositionUs);

        var q = new DatasetPlayer(Evenly(), 0, new FakeClock().Now, threaded: false);
        int got = 0;
        var sub = q.OnRecord(_ => got++);
        sub.Dispose();
        sub.Dispose();
        q.Play();
        q.Tick();
        Assert.Equal(0, got);
        Assert.Throws<InvalidOperationException>(() => q.Wait(TimeSpan.Zero));
        q.Close();
        q.Close();
        Assert.Throws<ObjectDisposedException>(() => q.Play());
    }

    [Fact]
    public void ThreadedDeliversOnThePlayerThreadAndPausesFromAnother()
    {
        using var p = new DatasetPlayer(Evenly(4, 20_000));
        var threads = new HashSet<string?>();
        var seen = new List<long>();
        p.OnRecord(r =>
        {
            threads.Add(Thread.CurrentThread.Name);
            seen.Add(r.THostUs);
        });
        p.Play();
        Assert.True(p.Wait(TimeSpan.FromSeconds(5)));
        Assert.Equal((PlayerState.Done, 4), (p.State, seen.Count));
        Assert.Equal(new[] { "depz-dataset-player" }, threads);

        using var q = new DatasetPlayer(Evenly(50, 100_000));
        var first = new ManualResetEventSlim();
        var seen2 = new List<long>();
        q.OnRecord(r =>
        {
            seen2.Add(r.THostUs);
            first.Set();
        });
        q.Play();
        Assert.True(first.Wait(TimeSpan.FromSeconds(5)));
        q.Pause();
        int n = seen2.Count;
        Assert.True(q.Wait(TimeSpan.Zero));
        q.SeekUs(4_000_000);
        Assert.Equal(n, seen2.Count);
        q.Speed = 0;
        q.Play();
        Assert.True(q.Wait(TimeSpan.FromSeconds(5)));
        Assert.Equal(Enumerable.Range(40, 10).Select(i => 1_000_000L + i * 100_000), seen2.Skip(n));

        using var e = new DatasetPlayer(Evenly(), 0);
        var errors = new List<Exception>();
        e.DeliveryError += errors.Add;
        e.OnRecord(_ => throw new KeyNotFoundException("x"));
        e.Play();
        Assert.True(e.Wait(TimeSpan.FromSeconds(5)));
        Assert.Equal(PlayerState.Paused, e.State);
        Assert.IsType<KeyNotFoundException>(Assert.Single(errors));
    }
}
