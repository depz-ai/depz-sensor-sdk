using System.Buffers.Binary;
using Depz.Sensor.Bno055;
using Depz.Sensor.Bno086;
using Depz.Sensor.Dataset;
using Depz.Sensor.Devices;
using Depz.Sensor.Protocol;
using Depz.Sensor.Vl53l4;
using Depz.Sensor.Vl53l8;
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
                Assert.Throws<InvalidStateException>(() => rec.Add(devA));
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

    /// <summary>Put SYNC_TIME (device clock <paramref name="mcuUs"/>) and GET_SERIAL (null: refused) in front of a
    /// sensor fake that does not answer them; GET_NAME_ACTIVE_SOFTWARE and GET_MCU_TEMPERATURE are refused, as by a
    /// board without them.</summary>
    private static void AnswerSync(FakeBoardLink link, ulong mcuUs, string? serial)
    {
        var inner = link.OnPacket!;
        link.OnPacket = p =>
        {
            switch (p.Cmd)
            {
                case (int)Cmd.SyncTime:
                {
                    var r = new byte[24];
                    p.Payload.AsSpan(0, 8).CopyTo(r);
                    BinaryPrimitives.WriteUInt64LittleEndian(r.AsSpan(8), mcuUs);
                    BinaryPrimitives.WriteUInt64LittleEndian(r.AsSpan(16), mcuUs);
                    return new[] { link.Reply((int)Rpt.SyncTime, r) };
                }
                case (int)Cmd.GetSerial when serial is not null:
                    var t = new[] { (byte)p.Cmd }.Concat(System.Text.Encoding.ASCII.GetBytes(serial)).Concat(new byte[] { 0 });
                    return new[] { link.Reply((int)Rpt.Text, t.ToArray()) };
                case (int)Cmd.GetSerial:
                case (int)Cmd.GetNameActiveSoftware:
                case (int)Cmd.GetMcuTemperature:
                    return new[] { FakeBoardLink.Status(p.Cmd, Status.ErrCmdNotSupported) };
                default:
                    return inner(p);
            }
        };
    }

    [Fact]
    public void EverySensorKindIsRecordedAtItsDeviceTimeMinusTheOffset()
    {
        // Contract 09 vl53l4 / vl53lx / bno055 / bno086: each device's results, as its reader thread hands them
        // out, at the device timestamp minus that device's sync offset.
        var l4 = new FakeVl53l4cdBoard();
        var lx = new FakeVl53lxBridge("DEPZ ToF Sensor VL53L4CD USB v2.1 TESTSERIAL");
        var b55 = new FakeBno055Board();
        var b86 = new FakeBno086Hub();
        AnswerSync(l4.Link, 1_000_000, "SN-L4");
        AnswerSync(b55.Link, 3_000_000, "SN-55");
        AnswerSync(b86.Link, 4_000_000, "SN-86");
        string path = TempPath();
        try
        {
            var timeout = TimeSpan.FromMilliseconds(500);
            using (var l4Dev = new Vl53l4cdDevice(l4.Link, timeout))
            using (var lxDev = new Vl53lxDevice(lx.Link, timeout))
            using (var b55Dev = new Bno055Device(b55.Link, timeout))
            using (var b86Dev = new Bno086Device(b86.Link, timeout))
            using (var rec = new SessionRecorder(path, temperatureHz: null))
            {
                l4Dev.Init();
                lxDev.Init();
                AnswerSync(lx.Link, 2_000_000, "SN-LX"); // after Init: the fake's own name answer picks the driver
                Assert.Equal(new[] { "l4", "lx", "b55", "b86" },
                    new[] { rec.Add(l4Dev, "l4"), rec.Add(lxDev, "lx"), rec.Add(b55Dev, "b55"), rec.Add(b86Dev, "b86") });
                rec.Start();
                rec.Start(); // a no-op: one file, one hook per device
                l4Dev.StartRanging();
                lxDev.StartRanging();
                b55Dev.StartStream(20, (Bno055Regs.AccData, 6));

                l4.McuTimeUs = 1_500_000;
                l4.SendStream(Vl53l4Uld.ResultBlockAddr, FakeVl53l4cdBoard.ResultBlock(291));
                lx.Stream(Vl53lxDriverTests.L4Block, 2_500_000);
                var acc = new byte[16];
                BinaryPrimitives.WriteUInt64LittleEndian(acc, 3_500_000);
                acc[8] = Bno055Regs.AccData;
                acc[9] = 6;
                BinaryPrimitives.WriteInt16LittleEndian(acc.AsSpan(10), 100);
                b55.Link.Inject(b55.Link.Reply((int)Bno055Rpt.Stream, acc));
                // one cargo: the 0xFB base, then an accelerometer report; its time is the frame's capture (1 s)
                b86.Link.Inject(b86.Data((int)ShtpChannel.InputNormal,
                    new byte[] { 0xFB, 0, 0, 0, 0, 0x01, 0, 0, 0, 1, 0, 2, 0, 3, 0 }));
                Assert.True(SpinWait.SpinUntil(() => rec.RecordsWritten == 4, 5000), $"{rec.RecordsWritten} records");
            }
            var reader = new DatasetReader(path);
            Assert.Equal(new (string?, string?)[] { ("SN-L4", "unknown"), ("SN-LX", "unknown"), ("SN-55", "unknown"), ("SN-86", "unknown") },
                new[] { "l4", "lx", "b55", "b86" }.Select(id => (reader.Devices[id].Serial, reader.Devices[id].SensorType)));
            Assert.All(reader.Devices.Values, d => Assert.Null(d.SoftwareName)); // the refused name is left out
            var byId = reader.Records.ToDictionary(r => r.DeviceId);
            long Offset(string id) => reader.Devices[id].TimeSync!.OffsetUs;

            var v4 = byId["l4"];
            Assert.Equal(("vl53l4", 1_500_000 - Offset("l4")), (v4.Kind, v4.THostUs));
            Assert.Equal((291L, 0L), ((long)v4.Value["distance_mm"]!, (long)v4.Value["range_status"]!));
            var vx = byId["lx"];
            Assert.Equal(("vl53lx", 2_500_000 - Offset("lx")), (vx.Kind, vx.THostUs));
            Assert.Equal(("VL53L4CD", 624L), ((string)vx.Value["product"]!, (long)vx.Value["distance_mm"]!));
            var v55 = byId["b55"];
            Assert.Equal(("bno055", 3_500_000 - Offset("b55")), (v55.Kind, v55.THostUs));
            Assert.Equal(new[] { 1.0, 0.0, 0.0 }, ((List<object?>)v55.Value["accel"]!).Select(Convert.ToDouble)); // 100 LSB at 100 LSB per m/s²
            var v86 = byId["b86"];
            Assert.Equal(("bno086", 1_000_000 - Offset("b86")), (v86.Kind, v86.THostUs));
            Assert.Equal(("Acceleration", 0x01L), ((string)v86.Value["type"]!, (long)v86.Value["sensor_id"]!));
        }
        finally
        {
            File.Delete(path);
        }
    }

    [Fact]
    public void Vl53l8FramesAndTheSensorDieTemperatureAreRecorded()
    {
        // As the Python test_recorder_samples_mcu_and_sensor_temperature: the frame at its device time, and once a
        // frame reported the die temperature, "sensor" temperature records from the sampler (this board has no
        // MCU thermometer: those readings are skipped).
        using var r = Vl53l8Rig.Cx();
        r.Ready(16, Vl53l8Rig.L8Frame4x4);
        AnswerSync(r.Board.Link, 1_000_000, "SN-L8");
        string path = TempPath();
        try
        {
            Vl53l8Frame seen;
            using (var q = r.Dev.Frames(4))
            using (var rec = new SessionRecorder(path, temperatureHz: 50))
            {
                string id = rec.Add(r.Dev);
                rec.Start();
                r.Dev.StartRanging();
                r.Board.SendFrame(r.Frame(40), 2_000_000, Vl53l8Wire.StreamChunkMax);
                seen = q.Take(TimeSpan.FromSeconds(2));
                Assert.True(SpinWait.SpinUntil(() => rec.RecordsWritten >= 3, 5000), $"{rec.RecordsWritten} records");
                r.P.SetU32(0x2FFC, 0x4FF, swapped: false); // stopped on its own: no MCU stop handshake
                r.Dev.StopRanging();
            }
            var reader = new DatasetReader(path);
            long offset = reader.Devices["d0"].TimeSync!.OffsetUs;
            var frames = reader.Records.Where(x => x.Kind == "vl53l8").ToList();
            var frame = Assert.Single(frames);
            Assert.Equal(2_000_000 - offset, frame.THostUs);
            Assert.Equal(Enumerable.Repeat(40L, 16), ((List<object?>)frame.Value["distance_mm"]!).Cast<long>());
            Assert.False(frame.Value.ContainsKey("signal_per_spad")); // layers off by default
            var temps = reader.Records.Where(x => x.Kind == "temperature").Select(x => x.Value).ToList();
            Assert.NotEmpty(temps);
            Assert.All(temps, t => Assert.Equal(("sensor", (double)seen.SiliconTempDegc),
                ((string)t["source"]!, Convert.ToDouble(t["celsius"]))));
        }
        finally
        {
            File.Delete(path);
        }
    }

    [Fact]
    public void ADeviceWithoutIdentityOrTemperatureIsStillRecorded()
    {
        // As the Python test_recorder_tolerates_a_device_without_identity_or_temperature: a board that
        // answers SYNC_TIME and refuses everything else — no serial, no name, no MCU thermometer.
        var link = new FakeBoardLink();
        link.OnPacket = p => new[] { FakeBoardLink.Status(p.Cmd, Status.ErrCmdNotSupported) };
        AnswerSync(link, 1_000_000, serial: null);
        string path = TempPath();
        try
        {
            using (var dev = new DepzDevice(link, TimeSpan.FromMilliseconds(500))) // no sensor class: nothing to hook
            using (var rec = new SessionRecorder(path, temperatureHz: 50))
            {
                string id = rec.Add(dev);
                Assert.Equal("d0", id);
                rec.Start();
                // a few sampler periods: every reading is refused
                Assert.True(SpinWait.SpinUntil(
                    () => link.Sent.Count(s => s.Cmd == (int)Cmd.GetMcuTemperature) >= 2, 5000));
            }
            var reader = new DatasetReader(path);
            var meta = reader.Devices["d0"];
            Assert.Equal(("unknown", (string?)null, (string?)null), (meta.SensorType, meta.Serial, meta.SoftwareName));
            Assert.NotNull(meta.TimeSync);
            Assert.Empty(reader.Records);
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
                w.Dispose();
                Assert.Throws<InvalidStateException>(() => w.Write("d0", 400L, "event", new Dictionary<string, object?>()));
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
    [InlineData(double.Epsilon, 0.0)]                      // subnormal: the implicit bit is absent
    [InlineData(9007199254740992.0, 9007199254740992.0)]  // 2^53: an integer, no fraction to round
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

    private static DatasetReader DualSr04() => new(Vectors.RecordingPath("dataset_dual_sr04.depzdata"));

    // Contract 09 §Player: a throwing record / state callback is counted and dropped; the other callbacks still
    // get every record and playback runs to the end (Tick mode).
    [Fact]
    public void AThrowingCallbackIsCountedAndPlaybackGoesOn()
    {
        var ds = DualSr04();
        var p = new DatasetPlayer(ds, 0, new FakeClock().Now, threaded: false);
        var first = ds.Records[0];
        var got = new List<DatasetRecord>();
        var states = new List<PlayerState>();
        p.OnRecord(r =>
        {
            if (ReferenceEquals(r, first))
                throw new InvalidOperationException("consumer failed");
        });
        p.OnRecord(got.Add);
        p.OnState(_ => throw new InvalidOperationException("state consumer failed"));
        p.OnState(states.Add);
        p.Play();
        Assert.Null(p.Tick());
        Assert.Equal(10, got.Count);
        Assert.Equal(ds.Records, got);
        Assert.Equal(PlayerState.Done, p.State);
        Assert.Equal(new[] { PlayerState.Playing, PlayerState.Done }, states);
        Assert.Equal(3, p.CallbackErrors); // the first record + two state changes
    }

    [Fact]
    public void ACallbackMayControlThePlayer()
    {
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
        Assert.Throws<InvalidStateException>(() => q.Wait(TimeSpan.Zero));
        q.Close();
        q.Close();
        Assert.Throws<InvalidStateException>(() => q.Play());
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

        var ds = DualSr04();
        using var e = new DatasetPlayer(ds, 0);
        var got = new List<DatasetRecord>();
        e.OnRecord(r =>
        {
            if (ReferenceEquals(r, ds.Records[0]))
                throw new KeyNotFoundException("x");
        });
        e.OnRecord(r =>
        {
            lock (got)
                got.Add(r);
        });
        e.Play();
        Assert.True(e.Wait(TimeSpan.FromSeconds(5)));
        Assert.Equal(PlayerState.Done, e.State);
        lock (got)
            Assert.Equal(ds.Records, got);
        Assert.Equal(1, e.CallbackErrors);
    }
}
