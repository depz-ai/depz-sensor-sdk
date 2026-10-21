using System.Security.Cryptography;
using System.Text.Json;
using Depz.Sensor.Protocol;
using Depz.Sensor.Tmf8829;
using Depz.Sensor.Usb;
using Xunit;

namespace Depz.Sensor.Tests;

// ── TMF8829 multizone dToF on the APP_TMF8829 SPI register bridge (contract 14, protocol v1.00) ──
//
// Golden vectors: tmf8829.json — encode, encode_errors, decode, reassembly,
// frames, measurements, config (get / set / set_errors), firmware; and
// tmf8829_frame_time.json — the clock model / capture time over 27 recorded
// streams, bit for bit. Mirrors depz-sensor-sdk-python/tests/test_vectors_tmf8829.py.

public class Tmf8829VectorTests
{
    private static readonly JsonElement Root = Vectors.Load("tmf8829.json");

    private static string Sha256(ReadOnlySpan<byte> data) => Vectors.ToHex(SHA256.HashData(data));

    /// <summary>Same key set, and each value equal to the JSON scalar / list / null.</summary>
    private static void AssertFields(string name, JsonElement expect, IReadOnlyDictionary<string, object?> got)
    {
        var expectedKeys = expect.EnumerateObject().Select(p => p.Name).OrderBy(k => k, StringComparer.Ordinal).ToArray();
        var gotKeys = got.Keys.OrderBy(k => k, StringComparer.Ordinal).ToArray();
        Assert.True(expectedKeys.SequenceEqual(gotKeys),
            $"{name}: field set {string.Join(",", expectedKeys)} vs {string.Join(",", gotKeys)}");
        foreach (var (key, value) in got)
            AssertValue($"{name}.{key}", expect.GetProperty(key), value);
    }

    private static void AssertValue(string name, JsonElement e, object? v)
    {
        switch (v)
        {
            case null:
                Assert.True(e.ValueKind == JsonValueKind.Null, $"{name}: expected {e}, got null");
                break;
            case bool b:
                Assert.True(e.ValueKind is JsonValueKind.True or JsonValueKind.False && e.GetBoolean() == b,
                    $"{name}: expected {e}, got {b}");
                break;
            case string s:
                Assert.True(e.GetString() == s, $"{name}: expected {e}, got {s}");
                break;
            case long[] arr:
                Assert.True(e.ValueKind == JsonValueKind.Array, $"{name}: expected {e}, got a list");
                var want = e.EnumerateArray().Select(x => x.GetInt64()).ToArray();
                Assert.True(want.SequenceEqual(arr), $"{name}: list differs (len {want.Length} vs {arr.Length})");
                break;
            default:
                long n = Convert.ToInt64(v);
                Assert.True(e.ValueKind == JsonValueKind.Number && e.GetInt64() == n, $"{name}: expected {e}, got {n}");
                break;
        }
    }

    private static long[]? Flat<T>(T[]? a) where T : struct =>
        a?.Select(x => Convert.ToInt64(x)).ToArray();

    private static Dictionary<string, object?> Arrays(Tmf8829PixelResults r) => new()
    {
        ["peaks"] = r.Peaks,
        ["rows"] = r.Rows,
        ["cols"] = r.Cols,
        ["distance"] = Flat(r.Distance),
        ["snr"] = Flat(r.Snr),
        ["signal"] = Flat(r.Signal),
        ["noise"] = Flat(r.Noise),
        ["xtalk"] = Flat(r.Xtalk),
    };

    private static int Int(JsonElement c, string key) => c.GetProperty(key).GetInt32();

    [Fact]
    public void Encode_ByteExact()
    {
        int n = 0;
        foreach (var c in Root.GetProperty("encode").EnumerateArray())
        {
            string name = c.GetProperty("name").GetString()!;
            byte[] payload = (Tmf8829Cmd)Int(c, "cmd") switch
            {
                Tmf8829Cmd.ReadReg => Tmf8829Wire.PackReadReg(Int(c, "addr"), Int(c, "len")),
                Tmf8829Cmd.WriteReg => Tmf8829Wire.PackWriteReg(Int(c, "addr"), Vectors.Hex(c.GetProperty("data").GetString()!)),
                Tmf8829Cmd.StartStream => Tmf8829Wire.PackStartStream(Int(c, "int_mask"), Int(c, "flags")),
                _ => Array.Empty<byte>(),
            };
            string expected = c.GetProperty("payload").GetString()!;
            Assert.True(expected == Vectors.ToHex(payload), $"{name}: expected {expected} got {Vectors.ToHex(payload)}");
            n++;
        }
        Assert.Equal(13, n);
        Assert.Equal(0x32, (int)Tmf8829Cmd.ReadReg);
        Assert.Equal(0x33, (int)Tmf8829Cmd.WriteReg);
        Assert.Equal(0x35, (int)Tmf8829Cmd.StartStream);
        Assert.Equal(0x36, (int)Tmf8829Cmd.StopStream);
        Assert.Equal(0x37, (int)Tmf8829Cmd.GetInfo);
        Assert.Equal(0x91, (int)Tmf8829Rpt.RegData);
        Assert.Equal(0x92, (int)Tmf8829Rpt.Info);
        Assert.Equal(0x93, (int)Tmf8829Rpt.Frame);
        Assert.Equal(0x94, (int)Tmf8829Rpt.Event);
    }

    [Fact]
    public void EncodeErrors_Refused()
    {
        int n = 0;
        foreach (var c in Root.GetProperty("encode_errors").EnumerateArray())
        {
            string kind = c.GetProperty("kind").GetString()!;
            Action act = kind switch
            {
                "read_reg" => () => Tmf8829Wire.PackReadReg(Int(c, "addr"), Int(c, "len")),
                "write_reg" => () => Tmf8829Wire.PackWriteReg(Int(c, "addr"), Vectors.Hex(c.GetProperty("data").GetString()!)),
                _ => () => Tmf8829Wire.PackStartStream(Int(c, "int_mask"), Int(c, "flags")),
            };
            Assert.ThrowsAny<ArgumentException>(act);
            n++;
        }
        Assert.Equal(7, n);
    }

    [Fact]
    public void Decode_Reports()
    {
        int n = 0;
        foreach (var c in Root.GetProperty("decode").EnumerateArray())
        {
            string name = c.GetProperty("name").GetString()!;
            byte[] payload = Vectors.Hex(c.GetProperty("payload").GetString()!);
            Dictionary<string, object?> got;
            switch ((Tmf8829Rpt)Int(c, "report"))
            {
                case Tmf8829Rpt.RegData:
                    var r = Tmf8829RegData.Unpack(payload);
                    got = new() { ["timestamp_us"] = r.TimestampUs, ["data"] = Vectors.ToHex(r.Data) };
                    break;
                case Tmf8829Rpt.Info:
                    var i = Tmf8829Info.Unpack(payload);
                    got = new()
                    {
                        ["int_edges"] = i.IntEdges, ["frames"] = i.Frames, ["frames_aborted"] = i.FramesAborted,
                        ["frame_errors"] = i.FrameErrors, ["spi_timeouts"] = i.SpiTimeouts, ["int_level"] = i.IntLevel,
                        ["streaming"] = i.Streaming, ["int_mask"] = i.IntMask, ["last_int_status"] = i.LastIntStatus,
                        ["last_fifo_status"] = i.LastFifoStatus,
                    };
                    break;
                case Tmf8829Rpt.Frame:
                    var f = Tmf8829FrameChunk.Unpack(payload);
                    got = new()
                    {
                        ["timestamp_us"] = f.TimestampUs, ["full_size"] = f.FullSize, ["offset"] = f.Offset,
                        ["frame_seq"] = f.FrameSeq, ["edge_us"] = f.EdgeUs, ["data"] = Vectors.ToHex(f.Data),
                    };
                    break;
                default:
                    var e = Tmf8829Event.Unpack(payload);
                    got = new()
                    {
                        ["timestamp_us"] = e.TimestampUs, ["int_status"] = e.IntStatus, ["code"] = e.Code,
                        ["frame_id"] = e.FrameId, ["fifo_status"] = e.FifoStatus,
                    };
                    break;
            }
            AssertFields(name, c.GetProperty("expect"), got);
            n++;
        }
        Assert.Equal(6, n);
        Assert.Throws<ArgumentException>(() => Tmf8829Info.Unpack(new byte[Tmf8829Wire.InfoSize - 1]));
        Assert.Throws<ArgumentException>(() => Tmf8829RegData.Unpack(Vectors.Hex("330000000000000000")));
    }

    [Fact]
    public void Reassembly()
    {
        int n = 0;
        foreach (var c in Root.GetProperty("reassembly").EnumerateArray())
        {
            string name = c.GetProperty("name").GetString()!;
            var r = new Tmf8829FrameReassembler();
            var expect = c.GetProperty("expect");
            var frames = expect.GetProperty("frames").EnumerateArray().ToArray();
            int done = 0;
            foreach (var ch in c.GetProperty("chunks").EnumerateArray())
            {
                var f = r.Feed(Tmf8829FrameChunk.Unpack(Vectors.Hex(ch.GetString()!)));
                if (f is null)
                    continue;
                Assert.True(done < frames.Length, $"{name}: more frames than expected");
                AssertFields($"{name}#{done}", frames[done], new Dictionary<string, object?>
                {
                    ["frame_seq"] = f.FrameSeq, ["timestamp_us"] = f.TimestampUs, ["edge_us"] = f.EdgeUs,
                    ["size"] = f.Data.Length, ["sha256"] = Sha256(f.Data),
                });
                done++;
            }
            Assert.True(done == frames.Length, $"{name}: {done} frames, expected {frames.Length}");
            Assert.True(expect.GetProperty("completed").GetInt32() == r.Completed, $"{name}: completed");
            Assert.True(expect.GetProperty("discarded").GetInt32() == r.Discarded, $"{name}: discarded");
            Assert.True(expect.GetProperty("seq_gaps").GetInt32() == r.SeqGaps, $"{name}: seq_gaps");
            n++;
        }
        Assert.Equal(7, n);
    }

    [Fact]
    public void Frames_HeaderFooterAndPixels()
    {
        int n = 0;
        foreach (var c in Root.GetProperty("frames").EnumerateArray())
        {
            string name = c.GetProperty("name").GetString()!;
            byte[] raw = Vectors.Hex(c.GetProperty("raw").GetString()!);
            var info = Tmf8829Frames.ParseFrame(raw);
            AssertFields(name, c.GetProperty("header"), new Dictionary<string, object?>
            {
                ["fifo_status"] = info.FifoStatus, ["systick"] = info.Systick, ["id"] = info.Id,
                ["kind"] = info.Kind, ["fp_mode"] = info.FpMode, ["layout"] = info.Layout,
                ["payload"] = info.Payload, ["f_number"] = info.FNumber,
                ["temperature"] = info.Temperature.Select(x => (long)x).ToArray(), ["bdv"] = info.Bdv,
                ["ref_pos"] = info.RefPos.Select(x => (long)x).ToArray(),
                ["t0_integration"] = info.T0Integration, ["t1_integration"] = info.T1Integration,
                ["frame_status"] = info.FrameStatus, ["reserved"] = info.Reserved, ["eof"] = info.Eof,
                ["size"] = info.Size,
            });
            if (info.Kind != Tmf8829Frames.FidRefSpad)
                Assert.True(Tmf8829Frames.FrameSize(raw) == raw.Length, $"{name}: full_size from the header");
            var problems = c.GetProperty("problems").EnumerateArray().Select(x => x.GetString()!).ToArray();
            Assert.True(problems.SequenceEqual(info.Problems()), $"{name}: problems");
            if (info.Kind == Tmf8829Frames.FidResults)
                AssertFields(name + ".results", c.GetProperty("results"), Arrays(Tmf8829Frames.ParseResults(raw, info)));
            else
                Assert.True(c.GetProperty("data_sha256").GetString() == Sha256(raw.AsSpan(21, raw.Length - 33)),
                    $"{name}: data sha256");
            n++;
        }
        Assert.Equal(18, n);
    }

    [Fact]
    public void Measurements()
    {
        var rawByCase = Root.GetProperty("frames").EnumerateArray()
            .ToDictionary(c => c.GetProperty("name").GetString()!, c => Vectors.Hex(c.GetProperty("raw").GetString()!));
        int n = 0;
        foreach (var s in Root.GetProperty("measurements").EnumerateArray())
        {
            string name = s.GetProperty("name").GetString()!;
            var b = new Tmf8829MeasurementBuilder();
            var expect = s.GetProperty("expect").EnumerateArray().ToArray();
            int k = 0;
            foreach (var f in s.GetProperty("frames").EnumerateArray())
            {
                var m = b.Add(rawByCase[f.GetProperty("raw_case").GetString()!],
                    f.GetProperty("timestamp_us").GetInt64(), f.GetProperty("edge_us").GetInt64());
                if (m is null)
                    continue;
                Assert.True(k < expect.Length, $"{name}: more measurements than expected");
                var got = new Dictionary<string, object?>
                {
                    ["timestamp_us"] = m.TimestampUs, ["flags"] = m.Flags, ["fp_mode"] = m.FpMode,
                    ["f_number"] = m.FNumber,
                    ["frame_times_us"] = m.Times.Select(t => t.TimestampUs).ToArray(),
                    ["t1_len_us"] = m.Times.Select(t => (long)t.T1LenUs).ToArray(),
                    ["ratio_q16"] = b.Clock.RatioQ16,
                    ["n_histograms"] = m.Histograms.Count, ["n_ref_frames"] = m.RefFrames.Count,
                    ["n_problems"] = m.Problems.Count, ["distance_in_mm"] = m.Frames[^1].DistanceInMm,
                };
                foreach (var (key, value) in Arrays(m.Results))
                    got[key] = value;
                AssertFields($"{name}#{k}", expect[k], got);
                k++;
            }
            Assert.True(k == expect.Length, $"{name}: {k} measurements, expected {expect.Length}");
            n++;
        }
        Assert.Equal(9, n);
    }

    [Fact]
    public void ConfigFields_GetSet()
    {
        var cfg = Root.GetProperty("config");
        byte[] page = Vectors.Hex(cfg.GetProperty("page").GetString()!);
        Assert.Equal(Tmf8829Config.CfgSize, page.Length);
        foreach (var p in cfg.GetProperty("get").EnumerateObject())
            Assert.True(Tmf8829Config.CfgGet(page, p.Name) == p.Value.GetInt32(), $"get {p.Name}");
        Assert.Equal(cfg.GetProperty("get").EnumerateObject().Count(), Tmf8829Config.ConfigFields(page).Count);
        foreach (var c in cfg.GetProperty("set").EnumerateArray())
        {
            var pg = (byte[])page.Clone();
            string field = c.GetProperty("field").GetString()!;
            Tmf8829Config.CfgSet(pg, field, c.GetProperty("value").GetInt32());
            Assert.True(c.GetProperty("page").GetString() == Vectors.ToHex(pg), $"set {field}");
        }
        int errors = 0;
        foreach (var c in cfg.GetProperty("set_errors").EnumerateArray())
        {
            var pg = (byte[])page.Clone();
            Assert.Throws<ArgumentOutOfRangeException>(() =>
                Tmf8829Config.CfgSet(pg, c.GetProperty("field").GetString()!, c.GetProperty("value").GetInt32()));
            errors++;
        }
        Assert.True(errors > 0);
        Assert.Equal(10, Tmf8829Config.Presets.Count);
        Assert.True(Tmf8829Config.Presets.Keys.OrderBy(x => x).SequenceEqual(Tmf8829Config.PresetKiloIterations.Keys.OrderBy(x => x)));
    }

    [Fact]
    public void FirmwareImage()
    {
        var fw = Root.GetProperty("firmware");
        byte[] img = Tmf8829Firmware.Image();
        Assert.Equal((long)Tmf8829Firmware.LoadAddr, fw.GetProperty("load_addr").GetInt64());
        Assert.Equal(fw.GetProperty("size").GetInt32(), img.Length);
        Assert.Equal(Tmf8829Firmware.Size, img.Length);
        Assert.Equal(fw.GetProperty("words").GetInt32(), img.Length / 4);
        Assert.Equal(fw.GetProperty("sha256").GetString(), Sha256(img));
        Assert.Contains("MIT License", Tmf8829Firmware.License());
    }

    [Fact]
    public void Identity_AndUsbPid()
    {
        Assert.Equal(SensorType.Tmf8829, IdentityParser.ParseSoftwareName("APP_TMF8829_v0.52").SensorType);
        Assert.Equal("tmf8829", UsbIds.UsbModelHint(UsbIds.DepzUsbVid, 0xED4C));
    }
}

public class Tmf8829FrameTimeVectorTests
{
    [Fact]
    public void FrameTime_BitExact()
    {
        var v = Vectors.Load("tmf8829_frame_time.json");
        var col = v.GetProperty("columns").EnumerateArray()
            .Select((e, i) => (e.GetString()!, i)).ToDictionary(x => x.Item1, x => x.i);
        int checkedFrames = 0;
        foreach (var s in v.GetProperty("streams").EnumerateArray())
        {
            string scenario = s.GetProperty("scenario").GetString()!;
            var clock = new Tmf8829SensorClock();
            var timer = new Tmf8829CaptureTimer(clock);
            foreach (var rowEl in s.GetProperty("frames").EnumerateArray())
            {
                var row = rowEl.EnumerateArray().ToArray();
                long Get(string name) => row[col[name]].GetInt64();
                int fid = (int)Get("id");
                long ts = Get("ts");
                uint tick = (uint)Get("systick");
                int kind = fid & 0xF0;
                if (kind == 0x30)
                    continue;
                int flags = clock.Add(ts, tick);
                if (kind == 0x20)
                {
                    timer.Histograms();
                    continue;
                }
                var t = timer.Results(fid & 0x0F, tick, (uint)Get("t0"), (uint)Get("t1"), ts, Get("edge_us"), flags);
                var got = (t.TimestampUs, (long)t.T1LenUs, (long)t.Flags, (long)clock.RatioQ16);
                var want = (Get("exp_timestamp_us"), Get("exp_t1_len_us"), Get("exp_flags"), Get("exp_ratio_q16"));
                Assert.True(got == want, $"{scenario} f_number {Get("f_number")}: got {got}, expected {want}");
                checkedFrames++;
            }
        }
        Assert.Equal(970, checkedFrames);
    }
}
