using System.Buffers.Binary;
using Depz.Sensor.Bno055;
using Depz.Sensor.Bno086;
using Depz.Sensor.Dataset;
using Depz.Sensor.Devices;
using Depz.Sensor.Protocol;
using Depz.Sensor.Tmf8829;
using Depz.Sensor.Transport;
using Depz.Sensor.Vl53l8;
using Depz.Sensor.Vl53lx;
using Xunit;

namespace Depz.Sensor.Tests;

/// <summary>The small value types the device layer hands out — events, counters, error names, flag
/// names, plain records — exercised the way a host sees them: through a fake board where one exists
/// (a corrupt packet, trash bytes, an unsolicited temperature), constructed directly where not.</summary>
public class CoverageGapTests
{
    private static readonly TimeSpan Short = TimeSpan.FromMilliseconds(200);

    [Fact]
    public void ACorruptPayloadCrcTrashBytesAndAnUnsolicitedTemperatureBecomeEvents()
    {
        var link = new FakeBoardLink();
        using var dev = new DepzDevice(link, Short);
        using var events = dev.Events();

        // A CRC8 packet whose trailer byte is wrong: the parser reports it, the reader counts it.
        byte[] bad = Framing.BuildPacket(0xC5, new byte[] { 1, 2, 3 }, 9, CrcType.Crc8);
        bad[^1] ^= 0xFF;
        link.Inject(bad);
        Assert.Equal(new LinkCrcErrorEvent(0xC5, 9), events.Take(TimeSpan.FromSeconds(1)));

        // Two bytes before the magic, then a whole packet: the bytes come out as trash.
        var temp = new byte[10];
        BinaryPrimitives.WriteUInt64LittleEndian(temp, 777);
        BinaryPrimitives.WriteInt16LittleEndian(temp.AsSpan(8), -35);
        link.Inject(new byte[] { 0x00, 0x11 }.Concat(link.Reply((int)Rpt.Temperature, temp)).ToArray());
        var trash = Assert.IsType<TrashEvent>(events.Take(TimeSpan.FromSeconds(1)));
        Assert.Equal(new byte[] { 0x00, 0x11 }, trash.Data);
        Assert.Equal(new TemperatureEvent(777, -3.5), events.Take(TimeSpan.FromSeconds(1)));

        Assert.True(SpinWait.SpinUntil(() => dev.Stats.TrashBytes == 2 && dev.Stats.CrcErrors == 1, 1000));
        LinkStats s = dev.Stats;
        // The corrupt packet is a CRC error, not a received packet (as in the Python reader loop).
        Assert.Equal((0L, 1L, 0L), (s.TxPackets, s.RxPackets, s.TxBytes));
        Assert.True(s.RxBytes > 0);
        Assert.Equal((0L, 0L, 0L), (s.HeaderErrors, s.SeqGaps, s.DeviceSeqErrors));
    }

    [Fact]
    public void EventRecordsCompareByValue()
    {
        Assert.Equal(7, new LinkCrcErrorEvent(0x91, 7).Seq);
        Assert.Equal(3, new TrashEvent(new byte[3]).Data.Length);
        Assert.Equal(25.5, new TemperatureEvent(1, 25.5).Celsius);
        Assert.NotEqual(new TemperatureEvent(1, 25.5), new TemperatureEvent(2, 25.5));
        Assert.Contains($"Celsius = {25.5}", new TemperatureEvent(1, 25.5).ToString()); // the current culture's 25.5, as ToString
    }

    [Fact]
    public void SerialPortInfoIsAPlainRecordSortedBySerialThenPath()
    {
        var a = new SerialPortInfo("/dev/ttyACM1", 0x1BCF, 0xEC78, "B");
        var b = new SerialPortInfo("/dev/ttyACM0", 0x1BCF, 0xEC78, "A");
        var none = new SerialPortInfo("/dev/ttyACM2", null, null, null);
        var empty = new SerialPortInfo("COM3", null, null, "");
        // No serial (null or empty) sorts last, then by port path, ordinal: "/dev/…" before "COM3".
        Assert.Equal(new[] { b, a, none, empty }, SerialPorts.BySerial(new[] { none, a, empty, b }).ToArray());
        Assert.Equal(("/dev/ttyACM1", 0x1BCF, 0xEC78, "B"), (a.Port, a.Vid, a.Pid, a.UsbSerial));
        Assert.Equal(a, a with { });
        Assert.Contains("ttyACM1", a.ToString());
    }

    [Fact]
    public void StatusNamesFollowContract02()
    {
        var want = new Dictionary<Status, string>
        {
            [Status.Ok] = "OK", [Status.Error] = "ERROR", [Status.ErrInvalidCmd] = "ERR_INVALID_CMD",
            [Status.ErrPayloadFormat] = "ERR_PAYLOAD_FORMAT", [Status.ErrInvalidParam] = "ERR_INVALID_PARAM",
            [Status.ErrPayloadCrc] = "ERR_PAYLOAD_CRC", [Status.ErrBusy] = "ERR_BUSY",
            [Status.ErrCmdNotSupported] = "ERR_CMD_NOT_SUPPORTED", [Status.ErrNotInitialized] = "ERR_NOT_INITIALIZED",
            [Status.ErrHardwareFault] = "ERR_HARDWARE_FAULT",
        };
        foreach (var (status, name) in want)
        {
            var e = new StatusException(0x12, (int)status);
            Assert.Equal((0x12, (int)status, name), (e.Cmd, e.Status, e.StatusName));
            Assert.Equal($"cmd 0x12 failed: {name} (0x{(int)status:X2})", e.Message);
        }
        Assert.Equal("STATUS_0x7F", new StatusException(1, 0x7F).StatusName);
        var busy = new BusyException(0x05);
        Assert.Equal(("ERR_BUSY", (int)Status.ErrBusy), (busy.StatusName, busy.Status));
        Assert.Contains("already in flight", busy.Message);
        Assert.IsAssignableFrom<DepzException>(busy);
    }

    [Fact]
    public void WrongTypeAndNoDepzDeviceAreDepzExceptions()
    {
        DepzException wrong = new WrongTypeException("the device is not an SR04 (Vl53l8cxDevice)");
        Assert.Contains("not an SR04", wrong.Message);
        var inner = new DepzTimeoutException("silence");
        var none = new NoDepzDeviceException("no DEPZ device answered", inner);
        Assert.Same(inner, none.InnerException);
        Assert.Equal("none", new NoDepzDeviceException("none").Message);
    }

    [Fact]
    public void Tmf8829TimeFlagNamesListTheBitsSet()
    {
        Assert.Empty(Tmf8829TimeFlags.FlagNames(0));
        Assert.Equal(new[] { "nominal", "clock_reset", "edge_late", "stalled", "t1_unsure" }, Tmf8829TimeFlags.FlagNames(0x1F));
        Assert.Equal(new[] { "clock_reset", "stalled" }, Tmf8829TimeFlags.FlagNames(Tmf8829TimeFlags.ClockReset | Tmf8829TimeFlags.Stalled));
        Assert.Equal(new[] { "t1_unsure" }, Tmf8829TimeFlags.FlagNames(0x10 | 0x80)); // an unknown bit has no name
        Assert.Equal((0x01, 0x02, 0x04, 0x08, 0x10),
            (Tmf8829TimeFlags.Nominal, Tmf8829TimeFlags.ClockReset, Tmf8829TimeFlags.EdgeLate, Tmf8829TimeFlags.Stalled, Tmf8829TimeFlags.T1Unsure));
    }

    [Fact]
    public void AHistogramFrameExposesTheBytesBetweenHeaderAndFooter()
    {
        var info = new Tmf8829FrameInfo(0, 1000, 0x20, 0, 5, 3, new sbyte[] { 25, 0, 0 }, 0, new ushort[2], 0, 0,
            (byte)Tmf8829Frames.FrameValid, 0, Tmf8829Frames.FrameEof, Tmf8829Frames.FrameHead + 5 + Tmf8829Frames.FooterSize);
        var raw = new byte[Tmf8829Frames.FrameHead + 5 + Tmf8829Frames.FooterSize];
        for (int i = 0; i < 5; i++)
            raw[Tmf8829Frames.FrameHead + i] = (byte)(i + 1);
        var frame = new Tmf8829HistogramFrame(info, raw);
        Assert.Equal(new byte[] { 1, 2, 3, 4, 5 }, frame.Data.ToArray());
        Assert.Equal(Tmf8829Frames.FidHistograms, frame.Info.Kind);
        Assert.Same(raw, frame.Raw);
    }

    [Fact]
    public void GyroIntegratedRvScalesQ14AndQ10()
    {
        var g = new GyroIntegratedRV((int)SensorId.GyroIntegratedRv, 123, 0, 8192, -16384, 16384, 1024, -512, 0);
        Assert.Equal("GyroIntegratedRV", g.Kind);
        Assert.Equal((0.0, 0.5, -1.0, 1.0), (g.I, g.J, g.K, g.Real));
        Assert.Equal(new[] { 1.0, -0.5, 0.0 }, g.AngularVelocity);
        var f = g.Fields();
        Assert.Equal(8192L, f["j_raw"]);
        Assert.Equal(-512L, f["vy_raw"]);
        Assert.Equal(0x2AL, f["sensor_id"]);
    }

    [Fact]
    public void SyncPinConfigPacksCheckedFieldsAndUnpacksThreeBytes()
    {
        var cfg = new SyncPinConfig(3, SyncPinMode.OutBoth, SyncPinPolarity.IdleHigh);
        Assert.Equal(new[] { (byte)3, (byte)SyncPinMode.OutBoth, (byte)SyncPinPolarity.IdleHigh }, cfg.Pack());
        Assert.Equal(cfg, SyncPinConfig.Unpack(cfg.Pack()));
        Assert.Equal((1, 5), (SyncPinConfig.PinMin, SyncPinConfig.PinMax));
        Assert.Throws<ArgumentOutOfRangeException>(() => (cfg with { Pin = 0 }).Pack());
        Assert.Throws<ArgumentOutOfRangeException>(() => (cfg with { Pin = 6 }).Pack());
        Assert.Throws<ArgumentOutOfRangeException>(() => (cfg with { Mode = (SyncPinMode)9 }).Pack());
        Assert.Throws<ArgumentOutOfRangeException>(() => (cfg with { Polarity = (SyncPinPolarity)2 }).Pack());
        Assert.Throws<ArgumentException>(() => SyncPinConfig.Unpack(new byte[] { 1, 0 }));
    }

    [Fact]
    public void Sr04MeasurementDerivesDistanceFromTheEcho()
    {
        var echo = new Sr04Measurement(1, 5831, "once");
        Assert.True(echo.Valid);
        Assert.Equal(Protocol.Sr04.DistanceMmFromEcho(5831), echo.DistanceMm);
        Assert.Equal(Protocol.Sr04.DistanceMmFromEcho(5831, 30.0), echo.DistanceMmAt(30.0));
        Assert.True(echo.DistanceMmAt(30.0) > echo.DistanceMm); // warmer air, faster sound
        var none = new Sr04Measurement(1, Protocol.Sr04.EchoTimeout, "loop");
        Assert.False(none.Valid);
        Assert.Null(none.DistanceMm);
        Assert.Null(none.DistanceMmAt(20.0));
    }

    // ── CNH value types and the CH class ─────────────────────────────────────

    [Fact]
    public void CnhAggregatesAndResultsScaleTheirBlockFloatingPointFields()
    {
        var agg = new Vl53l8CnhAggregate(new[] { 12, -6, 5 }, new sbyte[] { 2, 1, 0 }, 40, 3);
        Assert.Equal(new[] { 3.0, -3.0, 5.0 }, agg.Hist);
        Assert.Equal(5.0, agg.Ambient);
        var res = new Vl53l8CnhResult(4096, new[] { agg });
        Assert.Equal(2.0, res.RefResidual);
        Assert.Same(agg, res.Aggregates[0]);
    }

    [Fact]
    public void TheReassemblerDiscardsGapsOrphansRestartsAndOverruns()
    {
        // the Python test_reassembler_rules_directly, chunk for chunk
        var r = new FrameReassembler();
        static FrameChunk Chunk(int off, string data, int full = 8) => new(1, full, off, System.Text.Encoding.ASCII.GetBytes(data));
        static string Text(byte[] b) => System.Text.Encoding.ASCII.GetString(b);
        Assert.Null(r.Feed(Chunk(0, "abcd")));
        var done = r.Feed(Chunk(4, "efgh"));
        Assert.Equal((1UL, "abcdefgh"), (done!.Value.TimestampUs, Text(done.Value.Frame)));
        Assert.Equal((1, 0), (r.Completed, r.Discarded));
        // gap
        Assert.Null(r.Feed(Chunk(0, "abcd")));
        Assert.Null(r.Feed(Chunk(6, "gh")));
        Assert.Equal(1, r.Discarded);
        // an orphan with nothing in progress is dropped without counting a frame
        Assert.Null(r.Feed(Chunk(4, "efgh")));
        Assert.Equal(1, r.Discarded);
        // restart on offset 0 over a partial
        Assert.Null(r.Feed(Chunk(0, "abcd")));
        Assert.Null(r.Feed(Chunk(0, "abcd")));
        Assert.Equal(2, r.Discarded);
        Assert.Equal("abcdefgh", Text(r.Feed(Chunk(4, "efgh"))!.Value.Frame)); // the restarted frame completes
        // over-run (defensive): the single chunk exceeds full_size
        Assert.Null(r.Feed(Chunk(0, "abcdefghij")));
        Assert.Equal(3, r.Discarded);
        Assert.Equal("abcdefgh", Text(r.Feed(Chunk(0, "abcdefgh"))!.Value.Frame));
        // a full_size that changes mid-frame is a gap too
        Assert.Null(r.Feed(Chunk(0, "abcd")));
        Assert.Null(r.Feed(Chunk(4, "efgh", full: 12)));
        Assert.Equal((3, 4), (r.Completed, r.Discarded));
    }

    [Fact]
    public void TheL8chClassStartsWithNoCnhArmed()
    {
        using var r = Vl53l8Rig.Ch();
        var ch = Assert.IsType<Vl53l8chDevice>(r.Dev);
        Assert.Null(ch.CnhSetup);
    }

    // ── contract 09 record values, key for key the Python SessionRecorder ────

    private static Dictionary<string, object?> D(Dictionary<string, object?> v) => v;

    [Fact]
    public void Sr04AndVl53l4RecordValuesCarryEveryFieldAsIntegers()
    {
        Assert.Equal(D(new() { ["echo_us"] = 5831L, ["source"] = "once" }), DatasetRecords.Sr04(new Sr04Measurement(9, 5831, "once")));
        var m = new Vl53l4Measurement(9, 4, 624, 2, 5160, 1096, 24, 5, 211, 7);
        Assert.Equal(D(new()
        {
            ["range_status"] = 4L, ["distance_mm"] = 624L, ["sigma_mm"] = 2L, ["signal_rate_kcps"] = 5160L,
            ["ambient_rate_kcps"] = 1096L, ["number_of_spad"] = 211L, ["stream_count"] = 7L,
        }), DatasetRecords.Vl53l4(m));
    }

    [Fact]
    public void Vl53l8RecordValuesCutToTheResolutionAndAddLayersMotionAndCnh()
    {
        // a 4x4 frame inside 64-long arrays: only the first 16 of each go out
        var f = new Vl53l8Frame(9, 16, 31,
            Enumerable.Range(0, 64).ToArray(), Enumerable.Repeat((byte)5, 64).ToArray(), Enumerable.Repeat((byte)1, 64).ToArray(),
            Enumerable.Repeat(1.5, 64).ToArray(), Enumerable.Repeat(0.25, 64).ToArray(), new uint[64],
            Enumerable.Repeat(2.0, 64).ToArray(), Enumerable.Repeat(12.5, 64).ToArray());
        var v = DatasetRecords.Vl53l8(f);
        Assert.Equal(new[] { "resolution", "silicon_temp_degc", "distance_mm", "target_status", "nb_target_detected" }, v.Keys);
        Assert.Equal((16L, 31L), (v["resolution"], v["silicon_temp_degc"]));
        Assert.Equal(Enumerable.Range(0, 16).Select(i => (object?)(long)i), (List<object?>)v["distance_mm"]!);
        Assert.Equal(Enumerable.Repeat((object?)5L, 16), (List<object?>)v["target_status"]!);
        Assert.Equal(Enumerable.Repeat((object?)1L, 16), (List<object?>)v["nb_target_detected"]!);

        var motion = new Vl53l8Motion(7, 8, 1, 2, 16, Enumerable.Range(0, 32).Select(i => (uint)i).ToArray());
        var layered = DatasetRecords.Vl53l8(f with { Motion = motion }, layers: true);
        Assert.Equal(Enumerable.Repeat((object?)1.5, 16), (List<object?>)layered["signal_per_spad"]!);
        Assert.Equal(Enumerable.Repeat((object?)0.25, 16), (List<object?>)layered["ambient_per_spad"]!);
        Assert.Equal(Enumerable.Repeat((object?)2.0, 16), (List<object?>)layered["range_sigma_mm"]!);
        Assert.Equal(Enumerable.Repeat((object?)12.5, 16), (List<object?>)layered["reflectance"]!);
        var mo = (Dictionary<string, object?>)layered["motion"]!;
        Assert.Equal(new[] { "global_indicator_1", "global_indicator_2", "status", "nb_of_detected_aggregates", "nb_of_aggregates", "motion" }, mo.Keys);
        Assert.Equal((7L, 8L, 1L, 2L, 16L), (mo["global_indicator_1"], mo["global_indicator_2"], mo["status"], mo["nb_of_detected_aggregates"], mo["nb_of_aggregates"]));
        Assert.Equal(Enumerable.Range(0, 32).Select(i => (object?)(long)i), (List<object?>)mo["motion"]!);

        var setup = new Vl53l8CnhSetup();
        setup.InitConfig(10, 24, 1);
        setup.CreateAggMap(64, 0, 0, 2, 2, 4, 4);
        var cnh = DatasetRecords.Vl53l8Cnh(new byte[] { 1, 2, 3 }, setup);
        Assert.Equal(D(new()
        {
            ["nb_aggregates"] = 16L, ["feature_length"] = 24L, ["sum_span"] = 1L, ["start_bin"] = 10L, ["cnh_raw_b64"] = "AQID",
        }), cnh);
    }

    [Fact]
    public void Vl53lxRecordValuesNameThePairAndListTargetsOnlyWhenThereAreSome()
    {
        var extra = new Dictionary<string, object?> { ["stream_count"] = 1L };
        var single = new Vl53lxMeasurement(9, 624, 0, "valid", 5160, 1096, 2, 211, Array.Empty<Vl53lxTarget>(), extra, null);
        Assert.Equal(D(new()
        {
            ["product"] = "VL53L4CD", ["driver"] = "uld", ["status"] = 0L, ["distance_mm"] = 624L,
            ["sigma_mm"] = 2.0, ["signal_kcps"] = 5160.0, ["ambient_kcps"] = 1096.0, ["spads"] = 211.0,
        }), DatasetRecords.Vl53lx(single, "VL53L4CD", "uld"));
        var targets = new[] { new Vl53lxTarget(300, 0, "valid", 1200, 10, 3, 280, 320), new Vl53lxTarget(900, 4, "phase", 80, 10, 9, 850, 950) };
        var multi = single with { Targets = targets };
        var v = DatasetRecords.Vl53lx(multi, "VL53L4CX", "histogram");
        Assert.Equal("histogram", v["driver"]);
        var got = ((List<object?>)v["targets"]!).Cast<Dictionary<string, object?>>().ToList();
        Assert.Equal(2, got.Count);
        Assert.Equal(D(new() { ["distance_mm"] = 300L, ["status"] = 0L, ["signal_kcps"] = 1200.0 }), got[0]);
        Assert.Equal(D(new() { ["distance_mm"] = 900L, ["status"] = 4L, ["signal_kcps"] = 80.0 }), got[1]);
    }

    [Fact]
    public void Bno055RecordValuesHoldUnitSelAndOnlyTheChannelsTheBlockCovered()
    {
        var units = new Bno055Units(AccelMg: true, EulerRad: true);
        var bare = new Bno055Sample(9, 0x08, new byte[6], units, new[] { 1.0, 2.0, 3.0 }, null, null, null, null, null, null, null, null);
        Assert.Equal(D(new() { ["unit_sel"] = (long)units.Pack(), ["accel"] = new List<object?> { 1.0, 2.0, 3.0 } }), DatasetRecords.Bno055(bare));

        var full = new Bno055Sample(9, 0x08, new byte[46], new Bno055Units(),
            new[] { 1.0, 2.0, 3.0 }, new[] { 4.0, 5.0, 6.0 }, new[] { 7.0, 8.0, 9.0 }, new[] { 10.0, 11.0, 12.0 },
            new[] { 1.0, 0.0, 0.0, 0.0 }, new[] { 0.1, 0.2, 0.3 }, new[] { 0.0, 0.0, 9.8 }, 27.0, new Bno055CalibStatus(3, 2, 1, 0));
        var v = DatasetRecords.Bno055(full);
        Assert.Equal(new[] { "unit_sel", "quaternion", "euler", "accel", "gyro", "mag", "linear_accel", "gravity", "temperature", "calib" }, v.Keys);
        Assert.Equal(new List<object?> { 10.0, 11.0, 12.0 }, (List<object?>)v["euler"]!);
        Assert.Equal(new List<object?> { 0.0, 0.0, 9.8 }, (List<object?>)v["gravity"]!);
        Assert.Equal(27.0, v["temperature"]);
        Assert.Equal(new List<object?> { 3L, 2L, 1L, 0L }, (List<object?>)v["calib"]!);
    }

    [Fact]
    public void Tmf8829RecordValuesRoundDistancesToATenthAndAddTheOptionalLayers()
    {
        var info = new Tmf8829FrameInfo(0, 1000, 0x20, 0, 5, 3, new sbyte[] { 25, 0, 0 }, 0, new ushort[2], 0, 0,
            (byte)Tmf8829Frames.FrameValid, 0, Tmf8829Frames.FrameEof, Tmf8829Frames.FrameHead + 5 + Tmf8829Frames.FooterSize);
        Tmf8829Measurement Make(Tmf8829PixelResults r, double[]? mm) => new(9, 0x03, 0, 42, r, mm,
            new[] { info }, Array.Empty<Tmf8829FrameTime>(), Array.Empty<Tmf8829HistogramFrame>(), Array.Empty<byte[]>(), Array.Empty<string>());

        var empty = Make(new Tmf8829PixelResults(1, 8, 8, null, null, null, null, null), null);
        Assert.Throws<InvalidStateException>(() => empty.Results.DistanceAt(0, 0, 0)); // no peaks in this format
        var v = DatasetRecords.Tmf8829(empty);
        Assert.Equal(new[] { "fp_mode", "rows", "cols", "peaks", "f_number", "flags", "temperature_c", "distance_mm", "snr" }, v.Keys);
        Assert.Equal((0L, 8L, 8L, 1L, 42L, 3L, 25L), (v["fp_mode"], v["rows"], v["cols"], v["peaks"], v["f_number"], v["flags"], v["temperature_c"]));
        Assert.Empty((List<object?>)v["distance_mm"]!);
        Assert.Empty((List<object?>)v["snr"]!);

        var full = Make(new Tmf8829PixelResults(1, 8, 8, new ushort[] { 100, 200 }, new byte[] { 7, 9 },
            new ushort[] { 11, 12 }, new ushort[] { 1, 2 }, new ushort[] { 3, 4 }), new[] { 123.456, 0.25 });
        var w = DatasetRecords.Tmf8829(full);
        Assert.Equal(new List<object?> { 123.5, 0.2 }, (List<object?>)w["distance_mm"]!); // Python round(x, 1): ties to even
        Assert.Equal(new List<object?> { 7L, 9L }, (List<object?>)w["snr"]!);
        Assert.Equal(new List<object?> { 11L, 12L }, (List<object?>)w["signal"]!);
        Assert.Equal(new List<object?> { 1L, 2L }, (List<object?>)w["noise"]!);
        Assert.Equal(new List<object?> { 3L, 4L }, (List<object?>)w["xtalk"]!);
    }

    [Fact]
    public void Bno086RecordValuesOfTheReportsTheVectorsLack()
    {
        // the gyro-integrated rotation vector: quaternion plus angular velocity, no accuracy (not an input report)
        var g = DatasetRecords.Bno086(new GyroIntegratedRV((int)SensorId.GyroIntegratedRv, 123, 0, 8192, -16384, 16384, 1024, -512, 0));
        Assert.Equal(new[] { "type", "sensor_id", "i", "j", "k", "real", "angular_velocity", "timestamp_us" }, g.Keys);
        Assert.Equal(("GyroIntegratedRV", 0x2AL, 123L), (g["type"], g["sensor_id"], g["timestamp_us"]));
        Assert.Equal((0.0, 0.5, -1.0, 1.0), (g["i"], g["j"], g["k"], g["real"]));
        Assert.Equal(new List<object?> { 1.0, -0.5, 0.0 }, (List<object?>)g["angular_velocity"]!);

        var f = DatasetRecords.Bno086(new FlushCompleted(0xEF, 5, 0x01));
        Assert.Equal(D(new() { ["type"] = "FlushCompleted", ["sensor_id"] = 0xEFL, ["flushed_sensor"] = 1L, ["timestamp_us"] = 5L }), f);

        var um = DatasetRecords.Bno086(new UncalibratedMagnetometer(0x0F, 7, 3, 2, 0, 16, 32, -16, 1, 2, 3));
        Assert.Equal(new[] { "type", "sensor_id", "x", "y", "z", "accuracy", "bias", "timestamp_us" }, um.Keys);
        Assert.Equal((1.0, 2.0, -1.0, 2L), (um["x"], um["y"], um["z"], um["accuracy"]));
        Assert.Equal(new List<object?> { 0.0625, 0.125, 0.1875 }, (List<object?>)um["bias"]!);

        var pa = DatasetRecords.Bno086(new PersonalActivityClassifier(0x1E, 7, 1, 0, 0, 0, 1, 4, new[] { 10, 20, 70 }));
        Assert.Equal(4L, pa["most_likely_state"]);
        Assert.Equal(new List<object?> { 10L, 20L, 70L }, (List<object?>)pa["confidences"]!);

        var ge = DatasetRecords.Bno086(new GenericEvent(0x37, 7, 1, 0, 0, 99));
        Assert.Equal(D(new() { ["type"] = "GenericEvent", ["sensor_id"] = 0x37L, ["accuracy"] = 0L, ["value_raw"] = 99L, ["timestamp_us"] = 7L }), ge);

        var raw = DatasetRecords.Bno086(new RawSensor(0x14, 7, 1, 0, 0, 100, -200, 300, 4567, 23));
        Assert.Equal(new[] { "type", "sensor_id", "accuracy", "x_raw", "y_raw", "z_raw", "temperature_raw", "sensor_timestamp_us", "timestamp_us" }, raw.Keys);
        Assert.Equal((100L, -200L, 300L, 23L, 4567L), (raw["x_raw"], raw["y_raw"], raw["z_raw"], raw["temperature_raw"], raw["sensor_timestamp_us"]));
        // the same shape under a non-raw id carries no raw counts
        Assert.DoesNotContain("x_raw", DatasetRecords.Bno086(new RawSensor(0x01, 7, 1, 0, 0, 100, -200, 300, 4567, 23)).Keys);
    }
}
