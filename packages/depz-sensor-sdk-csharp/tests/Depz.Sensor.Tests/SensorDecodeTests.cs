using System.Text.Json;
using Depz.Sensor.Bno086;
using Depz.Sensor.Dataset;
using Depz.Sensor.Transport;
using Depz.Sensor.Usb;
using Depz.Sensor.Vl53l8;
using Xunit;

namespace Depz.Sensor.Tests;

// ── foundation gap: serial ordering ─────────────────────────────────────────

public class SerialOrderingTests
{
    [Fact]
    public void OrdersCandidatesBySerial()
    {
        var root = Vectors.Load("usb_ids.json");
        foreach (var c in root.GetProperty("serial_ordering").EnumerateArray())
        {
            string name = c.GetProperty("name").GetString()!;
            var ports = new List<UsbIds.PortCandidate>();
            foreach (var p in c.GetProperty("ports").EnumerateArray())
            {
                string port = p.GetProperty("port").GetString()!;
                var serEl = p.GetProperty("serial");
                string? serial = serEl.ValueKind == JsonValueKind.Null ? null : serEl.GetString();
                ports.Add(new UsbIds.PortCandidate(port, serial));
            }
            var expected = c.GetProperty("order").EnumerateArray().Select(e => e.GetString()).ToList();
            var got = UsbIds.SerialOrdering(ports).Select(p => p.Port).ToList();
            Assert.True(expected.SequenceEqual(got), $"{name}: expected [{string.Join(",", expected)}] got [{string.Join(",", got)}]");
        }
    }
}

// ── target 1: VL53L8 framing + reassembly + decode via .depzrec replay ──────
//
// The results-frame decode under test is SHARED by both ToF variants —
// VL53L8CX (base) and VL53L8CH (CX + CNH) stream the identical frame layout, so
// this same path serves both (see Vl53l8Variant / Vl53l8FrameDecoder.ForVariant).
// This fixture was captured on CX silicon (dev-default; probe "APP_VL53L8_v0.9",
// ULD 2.1.0 → frame-id footer offset 12, the decoder default). CH would differ
// only in that footer offset; the CH-only CNH histogram block is a separate,
// not-yet-decoded extension point (Vl53l8Cnh) and is not exercised here.

public class Vl53l8ReplayTests
{
    private static byte[] Hex(string s) => Vectors.Hex(s);

    [Fact]
    public void FullStackReplay_MatchesExpectedFrames()
    {
        string recPath = Vectors.RecordingPath("vl53l8_8x8_15hz_3s.depzrec");
        string expPath = Vectors.RecordingPath("vl53l8_8x8_15hz_3s.expected.json");

        string[] recLines = File.ReadAllLines(recPath);
        using var expDoc = JsonDocument.Parse(File.ReadAllText(expPath));
        JsonElement exp = expDoc.RootElement;
        var expectedFrames = exp.GetProperty("frames").EnumerateArray().ToList();

        // Replay the rx byte stream through framing → reassembly → decode.
        var parser = new PacketParser();
        var reassembler = new FrameReassembler();
        var decoder = new Vl53l8FrameDecoder();
        var frames = new List<Vl53l8Frame>();

        for (int i = 1; i < recLines.Length; i++)
        {
            if (recLines[i].Length == 0)
                continue;
            using var evDoc = JsonDocument.Parse(recLines[i]);
            JsonElement ev = evDoc.RootElement;
            if (ev.GetProperty("dir").GetString() != "rx")
                continue;
            byte[] data = Hex(ev.GetProperty("data").GetString()!);
            foreach (ParserEvent pe in parser.Feed(data))
            {
                if (pe is not Packet pkt || pkt.Cmd != (int)Vl53l8Rpt.Vl53Frame || pkt.Payload.Length < 12)
                    continue;
                var chunk = FrameChunk.Unpack(pkt.Payload);
                var done = reassembler.Feed(chunk);
                if (done is null)
                    continue;
                frames.Add(decoder.ParseFrame(done.Value.TimestampUs, done.Value.Frame));
            }
        }

        // The recorded session yields at least the sidecar's frames (a trailing
        // shutdown frame may follow; the reference test consumes only N).
        Assert.True(frames.Count >= expectedFrames.Count,
            $"decoded {frames.Count} frames, expected >= {expectedFrames.Count}");

        for (int i = 0; i < expectedFrames.Count; i++)
        {
            Vl53l8Frame got = frames[i];
            JsonElement want = expectedFrames[i];
            Assert.Equal(want.GetProperty("timestamp_us").GetUInt64(), got.TimestampUs);
            Assert.Equal(want.GetProperty("resolution").GetInt32(), got.Resolution);
            Assert.Equal(want.GetProperty("silicon_temp_degc").GetInt32(), got.SiliconTempDegc);
            Assert.Equal(want.GetProperty("distance_mm").EnumerateArray().Select(e => e.GetInt32()).ToArray(), got.DistanceMm);
            Assert.Equal(want.GetProperty("target_status").EnumerateArray().Select(e => (byte)e.GetInt32()).ToArray(), got.TargetStatus);
            Assert.Equal(want.GetProperty("nb_target_detected").EnumerateArray().Select(e => (byte)e.GetInt32()).ToArray(), got.NbTargetDetected);
        }
    }
}

// ── target 2: VL53L8 advanced DCI codecs (shared by CX and CH) ───────────────

public class Vl53l8AdvancedTests
{
    [Fact]
    public void MotionInit_PacksByteExact()
    {
        var root = Vectors.Load("vl53l8_advanced.json");
        foreach (var c in root.GetProperty("motion").EnumerateArray())
        {
            string name = c.GetProperty("name").GetString()!;
            int resolution = c.GetProperty("resolution").GetInt32();
            string expected = c.GetProperty("pack").GetString()!;
            string actual = Vectors.ToHex(MotionConfig.InitDefault(resolution).Pack());
            Assert.True(expected == actual, $"{name}: expected {expected} got {actual}");
        }
    }

    [Fact]
    public void DetectionThresholds_PackByteExact()
    {
        var root = Vectors.Load("vl53l8_advanced.json");
        foreach (var c in root.GetProperty("thresholds").EnumerateArray())
        {
            string name = c.GetProperty("name").GetString()!;
            var thresholds = new List<Vl53l8Advanced.DetectionThreshold>();
            foreach (var t in c.GetProperty("thresholds").EnumerateArray())
            {
                thresholds.Add(new Vl53l8Advanced.DetectionThreshold(
                    t.GetProperty("low_thresh").GetInt32(),
                    t.GetProperty("high_thresh").GetInt32(),
                    t.GetProperty("measurement").GetInt32(),
                    t.GetProperty("type").GetInt32(),
                    t.GetProperty("zone_num").GetInt32(),
                    t.GetProperty("operation").GetInt32()));
            }
            string expected = c.GetProperty("start_block").GetString()!;
            string actual = Vectors.ToHex(Vl53l8Advanced.PackDetectionThresholds(thresholds));
            Assert.True(expected == actual, $"{name}: start_block mismatch");
            Assert.Equal(c.GetProperty("valid_status").GetString(), Vectors.ToHex(Vl53l8Advanced.ThresholdValidStatus()));
        }
    }

    [Fact]
    public void XtalkMargin_RawByteExact()
    {
        var root = Vectors.Load("vl53l8_advanced.json");
        foreach (var c in root.GetProperty("xtalk_margin").EnumerateArray())
        {
            string name = c.GetProperty("name").GetString()!;
            double kcps = c.GetProperty("kcps").GetDouble();
            Assert.True(c.GetProperty("raw").GetUInt32() == Vl53l8Advanced.XtalkMarginRaw(kcps), $"{name}: raw mismatch");
        }
    }
}

// ── target 2b: VL53L8CH CNH histogram decode (CH-only) ───────────────────────
//
// Ports vl53lmz_plugin_cnh.c's decode path (Vl53l8Cnh); verified byte-exact
// against a live-VL53L8CH golden vector (config + cnh_raw → per-aggregate
// integer histograms + ref_residual_word).

public class Vl53l8CnhTests
{
    [Fact]
    public void DecodeHistogram_MatchesGoldenVector()
    {
        var root = Vectors.Load("vl53l8_cnh.json");
        JsonElement cfg = root.GetProperty("config");
        var config = new Vl53l8CnhConfig(
            cfg.GetProperty("nb_of_aggregates").GetInt32(),
            cfg.GetProperty("feature_length").GetInt32());

        byte[] raw = Vectors.Hex(root.GetProperty("cnh_raw").GetString()!);
        Vl53l8CnhResult got = Vl53l8Cnh.DecodeHistogram(config, raw);

        JsonElement expected = root.GetProperty("expected");
        Assert.Equal(expected.GetProperty("ref_residual_word").GetUInt32(), got.RefResidualWord);

        var wantAggs = expected.GetProperty("aggregates").EnumerateArray().ToList();
        Assert.Equal(wantAggs.Count, got.Aggregates.Count);
        Assert.Equal(config.NbOfAggregates, got.Aggregates.Count);

        for (int a = 0; a < wantAggs.Count; a++)
        {
            int[] wantRaw = wantAggs[a].GetProperty("hist_raw").EnumerateArray().Select(e => e.GetInt32()).ToArray();
            sbyte[] wantScaler = wantAggs[a].GetProperty("hist_scaler").EnumerateArray().Select(e => (sbyte)e.GetInt32()).ToArray();
            Assert.Equal(config.FeatureLength, got.Aggregates[a].HistRaw.Length);
            Assert.Equal(wantRaw, got.Aggregates[a].HistRaw);
            Assert.Equal(wantScaler, got.Aggregates[a].HistScaler);
        }
    }
}

// ── target 3: BNO086 SHTP framing + reassembly + control encodes ────────────

public class Bno086ShtpTests
{
    [Fact]
    public void Header_PackUnpack_ByteExact()
    {
        var root = Vectors.Load("bno086_shtp.json");
        foreach (var c in root.GetProperty("header").EnumerateArray())
        {
            string name = c.GetProperty("name").GetString()!;
            var hdr = new ShtpHeader(
                c.GetProperty("length").GetInt32(),
                c.GetProperty("channel").GetInt32(),
                c.GetProperty("seq").GetInt32(),
                c.GetProperty("continuation").GetBoolean());
            string expected = c.GetProperty("bytes").GetString()!;
            Assert.True(expected == Vectors.ToHex(hdr.Pack()), $"{name}: pack mismatch");
            var round = ShtpHeader.Unpack(Vectors.Hex(expected));
            Assert.Equal(hdr, round);
        }
    }

    [Fact]
    public void TxSeq_PerChannelCounters()
    {
        var root = Vectors.Load("bno086_shtp.json");
        var layer = new ShtpLayer();
        foreach (var c in root.GetProperty("tx_seq").EnumerateArray())
        {
            int channel = c.GetProperty("channel").GetInt32();
            byte[] payload = Vectors.Hex(c.GetProperty("payload").GetString()!);
            string expected = c.GetProperty("frame").GetString()!;
            Assert.True(expected == Vectors.ToHex(layer.NextFrame(channel, payload)), "tx_seq frame mismatch");
        }
    }

    [Fact]
    public void Reassembly_CargosAndDiscards()
    {
        var root = Vectors.Load("bno086_shtp.json");
        foreach (var c in root.GetProperty("reassembly").EnumerateArray())
        {
            string name = c.GetProperty("name").GetString()!;
            var layer = new ShtpLayer();
            var cargos = new List<ShtpCargo>();
            foreach (var f in c.GetProperty("frames").EnumerateArray())
            {
                ShtpCargo? cargo = layer.Feed(Vectors.Hex(f.GetString()!));
                if (cargo is not null)
                    cargos.Add(cargo);
            }
            var expect = c.GetProperty("expect");
            var wantCargos = expect.GetProperty("cargos").EnumerateArray().ToList();
            Assert.True(wantCargos.Count == cargos.Count, $"{name}: cargo count {cargos.Count} != {wantCargos.Count}");
            for (int i = 0; i < wantCargos.Count; i++)
            {
                Assert.Equal(wantCargos[i].GetProperty("channel").GetInt32(), cargos[i].Channel);
                Assert.Equal(wantCargos[i].GetProperty("seq").GetInt32(), cargos[i].Seq);
                Assert.Equal(wantCargos[i].GetProperty("payload").GetString(), Vectors.ToHex(cargos[i].Payload));
            }
            Assert.True(expect.GetProperty("discarded").GetInt32() == layer.Discarded, $"{name}: discarded {layer.Discarded}");
        }
    }

    [Fact]
    public void ControlEncode_ByteExact()
    {
        var root = Vectors.Load("bno086_shtp.json");
        foreach (var c in root.GetProperty("control_encode").EnumerateArray())
        {
            string kind = c.GetProperty("kind").GetString()!;
            string name = c.GetProperty("name").GetString()!;
            byte[] actual = kind switch
            {
                "set_feature" => Sh2Control.SetFeature(
                    c.GetProperty("sensor_id").GetInt32(),
                    (uint)c.GetProperty("interval_us").GetInt64(),
                    (uint)c.GetProperty("batch_us").GetInt64(),
                    c.GetProperty("sensitivity").GetInt32(),
                    c.GetProperty("flags").GetInt32(),
                    (uint)c.GetProperty("cfg_word").GetInt64()),
                "get_feature_request" => Sh2Control.GetFeature(c.GetProperty("sensor_id").GetInt32()),
                "product_id_request" => Sh2Control.ProductId(),
                "command_request" => Sh2Control.Command(
                    c.GetProperty("seq").GetInt32(),
                    c.GetProperty("command").GetInt32(),
                    Vectors.Hex(c.GetProperty("params").GetString()!)),
                "frs_read_request" => Sh2Control.FrsRead(
                    c.GetProperty("frs_type").GetInt32(),
                    c.GetProperty("offset_words").GetInt32(),
                    c.GetProperty("block_words").GetInt32()),
                "frs_write_request" => Sh2Control.FrsWrite(
                    c.GetProperty("frs_type").GetInt32(),
                    c.GetProperty("length_words").GetInt32()),
                "frs_write_data" => Sh2Control.FrsWriteDataReport(
                    c.GetProperty("offset_words").GetInt32(),
                    c.GetProperty("words").EnumerateArray().Select(e => (uint)e.GetInt64()).ToList()),
                _ => throw new InvalidOperationException("unknown control kind: " + kind),
            };
            string expected = c.GetProperty("payload").GetString()!;
            Assert.True(expected == Vectors.ToHex(actual), $"{name} ({kind}): expected {expected} got {Vectors.ToHex(actual)}");
        }
    }
}

// ── target 4: BNO086 report parsers ─────────────────────────────────────────

public class Bno086ReportsTests
{
    private static void AssertFields(string name, IReadOnlyDictionary<string, object?> got, JsonElement wantFields)
    {
        foreach (JsonProperty prop in wantFields.EnumerateObject())
        {
            Assert.True(got.ContainsKey(prop.Name), $"{name}: missing field {prop.Name}");
            object? gv = got[prop.Name];
            JsonElement wv = prop.Value;
            switch (wv.ValueKind)
            {
                case JsonValueKind.Null:
                    Assert.True(gv is null, $"{name}.{prop.Name}: expected null got {gv}");
                    break;
                case JsonValueKind.Array:
                    var wantArr = wv.EnumerateArray().Select(e => e.GetInt32()).ToArray();
                    Assert.Equal(wantArr, (int[])gv!);
                    break;
                case JsonValueKind.String:
                    Assert.Equal(wv.GetString(), (string)gv!);
                    break;
                default:
                    Assert.True(Convert.ToInt64(gv) == wv.GetInt64(), $"{name}.{prop.Name}: expected {wv.GetInt64()} got {gv}");
                    break;
            }
        }
    }

    [Fact]
    public void InputCargos_DecodeByteExact()
    {
        var root = Vectors.Load("bno086_reports.json");
        foreach (var c in root.GetProperty("input_cargos").EnumerateArray())
        {
            string name = c.GetProperty("name").GetString()!;
            byte[] cargo = Vectors.Hex(c.GetProperty("cargo").GetString()!);
            long capture = c.GetProperty("capture_timestamp_us").GetInt64();
            List<BnoReport> reports = Sh2Reports.ParseInputCargo(cargo, capture);
            var wantList = c.GetProperty("expect").EnumerateArray().ToList();
            Assert.True(wantList.Count == reports.Count, $"{name}: report count {reports.Count} != {wantList.Count}");
            for (int i = 0; i < wantList.Count; i++)
            {
                Assert.Equal(wantList[i].GetProperty("type").GetString(), reports[i].Kind);
                AssertFields(name, reports[i].Fields(), wantList[i].GetProperty("fields"));
            }
        }
    }

    [Fact]
    public void GyroRv_DecodeByteExact()
    {
        var root = Vectors.Load("bno086_reports.json");
        foreach (var c in root.GetProperty("gyro_rv").EnumerateArray())
        {
            string name = c.GetProperty("name").GetString()!;
            byte[] cargo = Vectors.Hex(c.GetProperty("cargo").GetString()!);
            long capture = c.GetProperty("capture_timestamp_us").GetInt64();
            GyroIntegratedRV? rep = Sh2Reports.ParseGyroRvCargo(cargo, capture);
            Assert.NotNull(rep);
            var expect = c.GetProperty("expect");
            Assert.Equal(expect.GetProperty("type").GetString(), rep!.Kind);
            AssertFields(name, rep.Fields(), expect.GetProperty("fields"));
        }
    }
}

// ── target 5: dataset (.depzdata) read ──────────────────────────────────────

public class DatasetReadTests
{
    [Fact]
    public void ReadsDualSr04Dataset()
    {
        var reader = new DatasetReader(Vectors.RecordingPath("dataset_dual_sr04.depzdata"));

        // Header: two devices, both SR04, with time-sync offsets.
        Assert.Equal(new[] { "d0", "d1" }, reader.Devices.Keys.OrderBy(k => k).ToArray());
        foreach (var dev in reader.Devices.Values)
        {
            Assert.Equal("sr04", dev.SensorType);
            Assert.NotNull(dev.TimeSync);
        }
        Assert.Equal(-8524738852, reader.Devices["d0"].TimeSync!.OffsetUs);
        Assert.Equal(-3525739108, reader.Devices["d1"].TimeSync!.OffsetUs);

        // 10 records, all sr04, merged in ascending host time.
        Assert.Equal(10, reader.Records.Count);
        Assert.All(reader.Records, r => Assert.Equal("sr04", r.Kind));
        var times = reader.Records.Select(r => r.THostUs).ToList();
        Assert.Equal(times.OrderBy(t => t).ToList(), times);
        Assert.Equal(5, reader.Records.Count(r => r.DeviceId == "d0"));
        Assert.Equal(5, reader.Records.Count(r => r.DeviceId == "d1"));

        // Decoded payloads (contract 09 sr04 kind): echo_us + source once|loop.
        foreach (var r in reader.Records)
        {
            Assert.Equal(5831, r.Value.GetProperty("echo_us").GetInt32());
            Assert.Contains(r.Value.GetProperty("source").GetString(), new[] { "once", "loop" });
        }
        Assert.Equal(8, reader.Records.Count(r => r.Value.GetProperty("source").GetString() == "loop"));
        Assert.Equal(2, reader.Records.Count(r => r.Value.GetProperty("source").GetString() == "once"));
    }
}
