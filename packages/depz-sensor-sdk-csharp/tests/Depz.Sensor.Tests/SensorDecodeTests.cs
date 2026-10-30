using System.Buffers.Binary;
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
            Vectors.Seen(c);
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
// only in that footer offset (4, VL53LMZ firmware); the two CH captures
// (APP_VL53L8_v0.92) replay the same rx path with that offset, the CNH one
// with cnh_raw byte-exact. The CNH histogram decode (Vl53l8Cnh) itself is
// covered by the vl53l8_cnh.json vector test.

public class Vl53l8ReplayTests
{
    private static byte[] Hex(string s) => Vectors.Hex(s);

    [Theory]
    [InlineData("vl53l8_8x8_15hz_3s", Vl53l8Variant.Cx, false)]
    [InlineData("vl53l8ch_8x8_15hz_3s", Vl53l8Variant.Ch, false)]
    [InlineData("vl53l8ch_cnh_8x8_15hz", Vl53l8Variant.Ch, true)]
    public void FullStackReplay_MatchesExpectedFrames(string stem, Vl53l8Variant variant, bool withCnh)
    {
        string recPath = Vectors.RecordingPath(stem + ".depzrec");
        string expPath = Vectors.RecordingPath(stem + ".expected.json");

        string[] recLines = File.ReadAllLines(recPath);
        using var expDoc = JsonDocument.Parse(File.ReadAllText(expPath));
        JsonElement exp = expDoc.RootElement;
        var expectedFrames = exp.GetProperty("frames").EnumerateArray().ToList();

        // Replay the rx byte stream through framing → reassembly → decode.
        var parser = new PacketParser();
        var reassembler = new FrameReassembler();
        var decoder = Vl53l8FrameDecoder.ForVariant(variant);
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
            if (withCnh)
                Assert.Equal(want.GetProperty("cnh_raw").GetString(), Vectors.ToHex(got.CnhRaw));
            else
                Assert.Null(got.CnhRaw);
        }
        Vectors.Seen($"recordings/{stem}.depzrec");
    }

    // A METADATA block too short to carry the silicon temperature (body byte 8;
    // the real block has 12) leaves it at 0 (bug hunt C-4). The frame is all
    // zero — zero headers walk 4 bytes at a time — with one METADATA header of
    // `size` bytes: at 16, where the byte the temperature would be read from is
    // 0xF6 (-10), or as the last word of the frame, where the read would run
    // past it.
    [Theory]
    [InlineData(4, false)]
    [InlineData(8, false)]
    [InlineData(0, true)]
    public void ShortMetadataBlock_LeavesTheTemperatureAtZero(int size, bool atEnd)
    {
        var buf = new byte[540];
        int at = atEnd ? buf.Length - 4 : 16;
        BinaryPrimitives.WriteUInt32LittleEndian(buf.AsSpan(at), (0x54B4u << 16) | ((uint)size << 4));
        if (!atEnd)
            buf[16 + 12] = 0xF6;
        var f = new Vl53l8FrameDecoder().ParseFrame(0, Vl53l8FrameDecoder.SwapBuffer(buf));
        Assert.Equal(0, f.SiliconTempDegc);
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
            Vectors.Seen(c);
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
            Vectors.Seen(c);
        }
    }

    [Fact]
    public void XtalkMargin_RawByteExact()
    {
        var root = Vectors.Load("vl53l8_advanced.json");
        foreach (var c in root.GetProperty("xtalk_margin").EnumerateArray())
        {
            string name = c.GetProperty("name").GetString()!;
            int kcps = c.GetProperty("kcps").GetInt32();
            uint raw = Vl53l8Advanced.XtalkMarginRaw(kcps);
            Assert.True(c.GetProperty("raw").GetUInt32() == raw, $"{name}: raw mismatch");
            Assert.True(Vl53l8Advanced.XtalkMarginKcps(raw) == kcps, $"{name}: kcps back");
            Vectors.Seen(c);
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
        Vectors.Seen("vl53l8_cnh");
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
            Vectors.Seen(c);
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
            Vectors.Seen(c);
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
            var frames = c.GetProperty("frames").EnumerateArray().ToList();
            var captures = c.GetProperty("captures_us").EnumerateArray().ToList();
            Assert.True(frames.Count == captures.Count, $"{name}: {frames.Count} frames, {captures.Count} captures");
            for (int k = 0; k < frames.Count; k++)
            {
                ShtpCargo? cargo = layer.Feed(Vectors.Hex(frames[k].GetString()!), captures[k].GetInt64());
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
                Assert.Equal(wantCargos[i].GetProperty("capture_us").GetInt64(), cargos[i].CaptureUs);
            }
            Assert.True(expect.GetProperty("discarded").GetInt32() == layer.Discarded, $"{name}: discarded {layer.Discarded}");
            Vectors.Seen(c);
        }
    }

    [Fact]
    public void FragmentCargo_SingleFrameRoundTrips()
    {
        var layer = new ShtpLayer();
        var frames = ShtpLayer.FragmentCargo((int)ShtpChannel.Control, "hello"u8.ToArray(), 0, maxFrame: 64);
        var frame = Assert.Single(frames);
        Assert.Equal(new byte[] { 9, 0, 2, 0 }, frame[..4]); // length 4 + 5, channel 2, seq 0, no continuation
        ShtpCargo? cargo = layer.Feed(frame);
        Assert.NotNull(cargo);
        Assert.Equal("hello"u8.ToArray(), cargo!.Payload);
        Assert.Equal(2, cargo.Channel);
    }

    [Fact]
    public void FragmentCargo_MultiFragmentReassemblesOnTheLastFrame()
    {
        var layer = new ShtpLayer();
        byte[] payload = Enumerable.Range(0, 100).Select(i => (byte)i).ToArray();
        var frames = ShtpLayer.FragmentCargo((int)ShtpChannel.Control, payload, 254, maxFrame: 32);
        // 28 payload bytes per frame: 100 → 28 + 28 + 28 + 16
        Assert.Equal(new[] { 32, 32, 32, 20 }, frames.Select(f => f.Length));
        // the first fragment advertises the whole cargo, each continuation what is left, bit 15 set
        Assert.Equal(new[] { 104, 76 | 0x8000, 48 | 0x8000, 20 | 0x8000 }, frames.Select(f => f[0] | f[1] << 8));
        Assert.Equal(new[] { 254, 255, 0, 1 }, frames.Select(f => (int)f[3])); // seq wraps at 0xFF
        var out_ = frames.Select(f => layer.Feed(f)).ToList();
        Assert.All(out_.Take(3), Assert.Null); // completes only on the last frame
        Assert.Equal(payload, out_[^1]!.Payload);
        Assert.Equal(254, out_[^1]!.Seq); // the first fragment's seq
        Assert.Equal(0, layer.Discarded);
        Assert.Throws<ArgumentException>(() => ShtpLayer.FragmentCargo(2, payload, 0, maxFrame: ShtpHeader.Size));
    }

    [Fact]
    public void Feed_LoneContinuationShortFramesOverrunsAndJunkChannelsAreDropped()
    {
        var layer = new ShtpLayer();
        var frames = ShtpLayer.FragmentCargo((int)ShtpChannel.Control, new byte[50], 0, maxFrame: 16);
        Assert.Null(layer.Feed(frames[1])); // a continuation with no cargo in progress
        Assert.Equal(1, layer.Discarded);
        Assert.Null(layer.Feed(new byte[] { 0x08, 0x00, 0x02 })); // shorter than a header: not a frame, not counted
        Assert.Null(layer.Feed(new byte[] { 0x00, 0x00, 0x02, 0x00 })); // a "no data" header
        Assert.Equal(1, layer.Discarded);
        // a header that promises 3 cargo bytes followed by 5: junk framing, thrown away
        Assert.Null(layer.Feed(new byte[] { 0x07, 0x00, 0x02, 0x00, 1, 2, 3, 4, 5 }));
        Assert.Equal(2, layer.Discarded);
        Assert.Null(layer.Feed(new byte[] { 0x05, 0x00, 0x06, 0x00, 0xAA })); // channel 6 does not exist
        Assert.Equal(3, layer.Discarded);
        // the overrun left the channel clean: the next cargo goes through
        Assert.Equal(new byte[] { 0xAB }, layer.Feed(ShtpLayer.BuildFrame(2, new byte[] { 0xAB }, 1))!.Payload);
        Assert.Throws<ArgumentOutOfRangeException>(() => layer.Feed(frames[0], captureUs: -1));
    }

    [Fact]
    public void Feed_ANewStartDiscardsThePartial()
    {
        var layer = new ShtpLayer();
        var frames = ShtpLayer.FragmentCargo((int)ShtpChannel.Control, new byte[50], 0, maxFrame: 16);
        Assert.Null(layer.Feed(frames[0], captureUs: 100)); // start a partial cargo
        byte[] fresh = ShtpLayer.FragmentCargo((int)ShtpChannel.Control, "x"u8.ToArray(), 5, maxFrame: 64)[0];
        ShtpCargo? cargo = layer.Feed(fresh, captureUs: 200); // a non-continuation frame preempts the partial
        Assert.Equal("x"u8.ToArray(), cargo!.Payload);
        Assert.Equal((5, 200L), (cargo.Seq, cargo.CaptureUs));
        Assert.Equal(1, layer.Discarded);
    }

    [Fact]
    public void NextFrame_RefusesAnOversizeCargoOrAnUnknownChannelAndResetClearsTxAndRx()
    {
        var layer = new ShtpLayer();
        Assert.Throws<ArgumentException>(() => layer.NextFrame(2, new byte[ShtpLayer.MaxTxFrame - ShtpHeader.Size + 1]));
        Assert.Throws<ArgumentOutOfRangeException>(() => layer.NextFrame(ShtpLayer.NumChannels, new byte[1]));
        Assert.Throws<ArgumentOutOfRangeException>(() => layer.NextFrame(-1, new byte[1]));
        Assert.Equal(0, layer.TxSeq(2)); // the refusals consumed no seq
        Assert.Equal(ShtpLayer.MaxTxFrame, layer.NextFrame(2, new byte[ShtpLayer.MaxTxFrame - ShtpHeader.Size]).Length);
        Assert.Equal(1, layer.TxSeq(2));
        Assert.Equal(0, layer.TxSeq(3)); // per channel
        var frames = ShtpLayer.FragmentCargo(2, new byte[50], 0, maxFrame: 16);
        layer.Feed(frames[0]);
        layer.Reset();
        Assert.Equal(0, layer.TxSeq(2));
        Assert.Null(layer.Feed(frames[1])); // the partial is gone: its continuation is an orphan
        Assert.Equal(1, layer.Discarded);
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
            Vectors.Seen(c);
        }
    }

    [Fact]
    public void InteractiveCalibrationAndFlush_ByteExact()
    {
        var root = Vectors.Load("bno086_shtp.json");
        foreach (var c in root.GetProperty("interactive_and_flush").EnumerateArray())
        {
            string name = c.GetProperty("name").GetString()!;
            byte[] actual = c.GetProperty("kind").GetString() switch
            {
                "command_request" => Sh2Control.Command(
                    c.GetProperty("seq").GetInt32(),
                    c.GetProperty("command").GetInt32(),
                    Sh2Hub.InteractiveCalibrationParams(Vectors.Hex(c.GetProperty("params").GetString()!)[0])),
                "force_flush" => Sh2Hub.ForceFlush(c.GetProperty("sensor_id").GetInt32()),
                var k => throw new InvalidOperationException("unknown kind: " + k),
            };
            string expected = c.GetProperty("payload").GetString()!;
            Assert.True(expected == Vectors.ToHex(actual), $"{name}: expected {expected} got {Vectors.ToHex(actual)}");
            Vectors.Seen(c);
        }
    }

    [Fact]
    public void Executable_SleepPowerOn_Frames()
    {
        var root = Vectors.Load("bno086_shtp.json");
        var layer = new ShtpLayer(); // fresh: sleep then power_on → channel-1 seq 0, 1
        int n = 0;
        foreach (var c in root.GetProperty("executable").EnumerateArray())
        {
            string name = c.GetProperty("name").GetString()!;
            (ExecutableCommand command, byte[] payload) = name switch
            {
                "sleep" => (ExecutableCommand.Sleep, Sh2Control.Sleep()),
                "power_on" => (ExecutableCommand.On, Sh2Control.PowerOn()),
                _ => throw new InvalidOperationException("unknown executable command: " + name),
            };
            Assert.Equal(c.GetProperty("command").GetInt32(), (int)command);
            Assert.Equal((int)ShtpChannel.Executable, c.GetProperty("channel").GetInt32());
            Assert.Equal(c.GetProperty("payload").GetString(), Vectors.ToHex(payload));
            byte[] frame = layer.NextFrame((int)ShtpChannel.Executable, payload);
            Assert.Equal(c.GetProperty("shtp_frame").GetString(), Vectors.ToHex(frame));
            byte[] bridge = Framing.BuildPacket(
                c.GetProperty("bridge_cmd").GetInt32(), frame,
                c.GetProperty("bridge_seq").GetInt32(), (CrcType)c.GetProperty("crc_type").GetInt32());
            Assert.Equal(c.GetProperty("bridge_frame").GetString(), Vectors.ToHex(bridge));
            n++;
            Vectors.Seen(c);
        }
        Assert.Equal(2, n);
    }

    [Fact]
    public void FeatureFlags_MatchVectors()
    {
        var flags = Vectors.Load("bno086_shtp.json").GetProperty("feature_flags");
        Assert.Equal(FeatureFlags.Wakeup, flags.GetProperty("wakeup").GetInt32());
        Assert.Equal(FeatureFlags.AlwaysOn, flags.GetProperty("always_on").GetInt32());
        Assert.Equal(2, flags.EnumerateObject().Count());
        Vectors.Seen("bno086_shtp/feature_flags");
    }
}

// ── target 4: BNO086 report parsers ─────────────────────────────────────────

public class Bno086ReportsTests
{
    internal static void AssertFields(string name, IReadOnlyDictionary<string, object?> got, JsonElement wantFields)
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
        // interactive_and_flush: MotionRequest (0x2B) and the bare 0xEF Flush
        // Completed, in the same input-cargo shape
        var cargos = root.GetProperty("input_cargos").EnumerateArray()
            .Concat(root.GetProperty("interactive_and_flush").EnumerateArray());
        foreach (var c in cargos)
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
            // `record`: the contract 09 dataset value, scaled, key for key.
            for (int i = 0; i < wantList.Count; i++)
            {
                if (!wantList[i].TryGetProperty("record", out var wantRecord))
                    continue;
                var got = (Dictionary<string, object?>)Json.JsonText.Parse(Json.JsonText.Serialize(DatasetRecords.Bno086(reports[i])))!;
                var want = (Dictionary<string, object?>)Json.JsonText.Parse(wantRecord.GetRawText())!;
                Assert.Equal(want.Keys.OrderBy(k => k, StringComparer.Ordinal), got.Keys.OrderBy(k => k, StringComparer.Ordinal));
                Assert.Null(ScenarioTests.Mismatch(got, want, $"{name}#{i}.record"));
            }
            Vectors.Seen(c);
        }
    }

    [Fact]
    public void QPoints()
    {
        var root = Vectors.Load("bno086_reports.json");
        var want = root.GetProperty("q_points").EnumerateObject()
            .ToDictionary(p => Convert.ToInt32(p.Name, 16), p => p.Value.GetInt32());
        Assert.Equal(want.OrderBy(kv => kv.Key), Bno086Q.Points.OrderBy(kv => kv.Key));
        Vectors.Seen("bno086_reports/q_points");
        Assert.Equal(root.GetProperty("rv_accuracy_q").GetInt32(), Bno086Q.RvAccuracyQ);
        Vectors.Seen("bno086_reports/rv_accuracy_q");
        Assert.Equal(root.GetProperty("gyro_rv_angvel_q").GetInt32(), Bno086Q.GyroRvAngvelQ);
        Vectors.Seen("bno086_reports/gyro_rv_angvel_q");
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
            Vectors.Seen(c); // `record` not checked, as in InputCargos_DecodeByteExact
        }
    }
}

// recordings/bno086_session.depzrec decoded rx-side only (the C# SDK has no
// device layer to re-issue the tx half): every RPT_DATA SHTP frame through the
// SHTP layer, input-channel cargos through the SH-2 report parsers, against
// the sidecar's 150 reports.
public class Bno086RecordingTests
{
    private const int RptData = 0x91; // cmd u8, capture timestamp_us u64, raw SHTP frame

    [Fact]
    public void Session_RxDecode_MatchesSidecar()
    {
        var exp = JsonDocument.Parse(File.ReadAllText(Vectors.RecordingPath("bno086_session.expected.json"))).RootElement;
        var layer = new ShtpLayer();
        var reports = new List<BnoReport>();
        foreach (var pkt in Vectors.RxPackets("bno086_session.depzrec"))
        {
            if (pkt.Cmd != RptData || pkt.Payload.Length < 9)
                continue;
            long capture = (long)System.Buffers.Binary.BinaryPrimitives.ReadUInt64LittleEndian(pkt.Payload.AsSpan(1));
            ShtpCargo? cargo = layer.Feed(pkt.Payload[9..], capture);
            if (cargo is null)
                continue;
            if (cargo.Channel is (int)ShtpChannel.InputNormal or (int)ShtpChannel.InputWake)
                reports.AddRange(Sh2Reports.ParseInputCargo(cargo.Payload, cargo.CaptureUs));
            else if (cargo.Channel == (int)ShtpChannel.GyroRv && Sh2Reports.ParseGyroRvCargo(cargo.Payload, cargo.CaptureUs) is { } rv)
                reports.Add(rv);
        }

        var want = exp.GetProperty("reports").EnumerateArray().ToList();
        Assert.True(reports.Count >= want.Count, $"decoded {reports.Count} reports, expected >= {want.Count}");
        for (int i = 0; i < want.Count; i++)
        {
            Assert.Equal(want[i].GetProperty("type").GetString(), reports[i].Kind);
            var fields = new Dictionary<string, JsonElement>();
            foreach (var p in want[i].EnumerateObject())
                if (p.Name != "type")
                    fields[p.Name] = p.Value;
            var wantFields = JsonSerializer.SerializeToElement(fields);
            Assert.Equal(fields.Count, reports[i].Fields().Count);
            Bno086ReportsTests.AssertFields($"report {i}", reports[i].Fields(), wantFields);
        }
        Vectors.Seen("recordings/bno086_session.depzrec");
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
            Assert.Equal(5831L, r.Value["echo_us"]);
            Assert.Contains((string)r.Value["source"]!, new[] { "once", "loop" });
        }
        Assert.Equal(8, reader.Records.Count(r => (string?)r.Value["source"] == "loop"));
        Assert.Equal(2, reader.Records.Count(r => (string?)r.Value["source"] == "once"));
        Vectors.Seen("recordings/dataset_dual_sr04.depzdata");
    }
}
