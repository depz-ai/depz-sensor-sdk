using System.Text.Json;
using Depz.Sensor.Transport;
using Depz.Sensor.Vl53l7;
using Depz.Sensor.Vl53l8;
using Xunit;

namespace Depz.Sensor.Tests;

// ── VL53L5CX / VL53L7CX / VL53L7CH I2C bridge (contract 11) ─────────────────
//
// Golden vectors: vl53l7.json (encode / decode / model). Full-stack frame decode
// is pinned by four live-board captures under recordings/ (fw APP_VL53L7_v0.53),
// replayed rx-side through framing → reassembly → L5/L7 frame decode.

public class Vl53l7VectorTests
{
    [Fact]
    public void Encode_ByteExact()
    {
        var root = Vectors.Load("vl53l7.json");
        int n = 0;
        foreach (var c in root.GetProperty("encode").EnumerateArray())
        {
            string name = c.GetProperty("name").GetString()!;
            string kind = c.GetProperty("kind").GetString()!;
            byte[] payload = kind switch
            {
                "read_reg" => Vl53l7Wire.PackReadReg(
                    (ushort)c.GetProperty("addr").GetInt32(), c.GetProperty("len").GetInt32()),
                "write_reg" => Vl53l7Wire.PackWriteReg(
                    (ushort)c.GetProperty("addr").GetInt32(), Vectors.Hex(c.GetProperty("data").GetString()!)),
                "pin_ctrl" => Vl53l7Wire.PackPinCtrl((byte)c.GetProperty("action").GetInt32()),
                "set_i2c_speed" => Vl53l7Wire.PackSetI2cSpeed((ushort)c.GetProperty("khz").GetInt32()),
                _ => throw new InvalidOperationException($"{name}: unknown encode kind {kind}"),
            };
            string expected = c.GetProperty("payload").GetString()!;
            string actual = Vectors.ToHex(payload);
            Assert.True(expected == actual, $"{name}: expected {expected} got {actual}");
            n++;
        }
        Assert.True(n > 0);
    }

    [Fact]
    public void Encode_RejectsOverLimitTransfers()
    {
        Assert.Throws<ArgumentOutOfRangeException>(() => Vl53l7Wire.PackReadReg(0, Vl53l7Wire.ReadMaxLen + 1));
        Assert.Throws<ArgumentOutOfRangeException>(() => Vl53l7Wire.PackReadReg(0, 0));
        Assert.Throws<ArgumentOutOfRangeException>(() => Vl53l7Wire.PackReadReg(0xFFFF, 2));
        Assert.Throws<ArgumentOutOfRangeException>(() => Vl53l7Wire.PackWriteReg(0, new byte[Vl53l7Wire.WriteMaxLen + 1]));
        Assert.Equal(2 + Vl53l7Wire.WriteMaxLen, Vl53l7Wire.PackWriteReg(0, new byte[Vl53l7Wire.WriteMaxLen]).Length);
        Assert.Equal(0x34, (int)Vl53l7Cmd.PinCtrl);
        Assert.Equal(0x37, (int)Vl53l7Cmd.GetInfo);
        Assert.Equal(0x38, (int)Vl53l7Cmd.SetI2cSpeed);
        Assert.Equal(0x92, (int)Vl53l7Rpt.Vl53Info);
        Assert.Equal(1536, Vl53l7Wire.StreamChunkMax);
    }

    [Fact]
    public void Decode_InfoFieldExact()
    {
        var root = Vectors.Load("vl53l7.json");
        foreach (var c in root.GetProperty("decode").EnumerateArray())
        {
            string name = c.GetProperty("name").GetString()!;
            Assert.True(c.GetProperty("report").GetInt32() == (int)Vl53l7Rpt.Vl53Info, name);
            Vl53l7Info info = Vl53l7Info.Unpack(Vectors.Hex(c.GetProperty("payload").GetString()!));
            JsonElement e = c.GetProperty("expect");
            Assert.True(e.GetProperty("int_edges").GetUInt32() == info.IntEdges, $"{name}: int_edges");
            Assert.True(e.GetProperty("frames_dropped").GetUInt32() == info.FramesDropped, $"{name}: frames_dropped");
            Assert.True(e.GetProperty("i2c_errors").GetUInt32() == info.I2cErrors, $"{name}: i2c_errors");
            Assert.True(e.GetProperty("last_i2c_error").GetInt32() == info.LastI2cError, $"{name}: last_i2c_error");
            Assert.True(e.GetProperty("lpn_level").GetInt32() == info.LpnLevel, $"{name}: lpn_level");
            Assert.True(e.GetProperty("int_level").GetInt32() == info.IntLevel, $"{name}: int_level");
            Assert.True(e.GetProperty("i2c_khz").GetInt32() == info.I2cKhz, $"{name}: i2c_khz");
            Assert.True(e.GetProperty("frame_size").GetInt32() == info.FrameSize, $"{name}: frame_size");
            Assert.True(e.GetProperty("streaming").GetBoolean() == info.Streaming, $"{name}: streaming");
        }
    }

    [Fact]
    public void Decode_InfoRejectsShortPayload()
    {
        Assert.Throws<ArgumentException>(() => Vl53l7Info.Unpack(new byte[Vl53l7Wire.InfoSize - 1]));
    }

    [Fact]
    public void Model_ResolvesClass()
    {
        var root = Vectors.Load("vl53l7.json");
        foreach (var c in root.GetProperty("model").EnumerateArray())
        {
            string name = c.GetProperty("name").GetString()!;
            JsonElement um = c.GetProperty("usb_model");
            string? usbModel = um.ValueKind == JsonValueKind.Null ? null : um.GetString();
            Vl53l7Model got = Vl53l7Discovery.ResolveModel(usbModel, c.GetProperty("device_name").GetString());
            Assert.True(c.GetProperty("expect").GetString() == got.ToString().ToLowerInvariant(),
                $"{name}: expected {c.GetProperty("expect").GetString()} got {got}");
        }
    }
}

public class Vl53l7ReplayTests
{
    [Theory]
    [InlineData("vl53l5cx_8x8_15hz_3s", 64, false)]
    // 4x4 pins the trim: per-target blocks arrive with 64 entries, 16 are real.
    [InlineData("vl53l5cx_4x4_15hz", 16, false)]
    [InlineData("vl53l7ch_8x8_15hz_3s", 64, false)]
    // CNH: 3156 B frames over the chunked stream; cnh_raw byte-exact.
    [InlineData("vl53l7ch_cnh_8x8_15hz", 64, true)]
    public void FullStackReplay_MatchesExpectedFrames(string stem, int resolution, bool withCnh)
    {
        string[] recLines = File.ReadAllLines(Vectors.RecordingPath(stem + ".depzrec"));
        using var expDoc = JsonDocument.Parse(File.ReadAllText(Vectors.RecordingPath(stem + ".expected.json")));
        JsonElement exp = expDoc.RootElement;
        var expectedFrames = exp.GetProperty("frames").EnumerateArray().ToList();
        Assert.StartsWith("APP_VL53L7_", exp.GetProperty("software_name").GetString());
        Assert.NotEmpty(expectedFrames);

        var parser = new PacketParser();
        var reassembler = new FrameReassembler();
        var decoder = Vl53l7Frames.CreateDecoder();
        var frames = new List<Vl53l8Frame>();

        for (int i = 1; i < recLines.Length; i++)
        {
            if (recLines[i].Length == 0)
                continue;
            using var evDoc = JsonDocument.Parse(recLines[i]);
            JsonElement ev = evDoc.RootElement;
            if (ev.GetProperty("dir").GetString() != "rx")
                continue;
            foreach (ParserEvent pe in parser.Feed(Vectors.Hex(ev.GetProperty("data").GetString()!)))
            {
                if (pe is not Packet pkt || pkt.Cmd != (int)Vl53l8Rpt.Vl53Frame || pkt.Payload.Length < 12)
                    continue;
                var chunk = FrameChunk.Unpack(pkt.Payload);
                Assert.True(chunk.Data.Length <= Vl53l7Wire.StreamChunkMax, $"{stem}: chunk over {Vl53l7Wire.StreamChunkMax} B");
                var done = reassembler.Feed(chunk);
                if (done is null)
                    continue;
                frames.Add(decoder.ParseFrame(done.Value.TimestampUs, done.Value.Frame));
            }
        }

        Assert.True(frames.Count >= expectedFrames.Count,
            $"{stem}: decoded {frames.Count} frames, expected >= {expectedFrames.Count}");

        for (int i = 0; i < expectedFrames.Count; i++)
        {
            Vl53l8Frame got = frames[i];
            JsonElement want = expectedFrames[i];
            Assert.Equal(want.GetProperty("timestamp_us").GetUInt64(), got.TimestampUs);
            Assert.Equal(resolution, want.GetProperty("resolution").GetInt32());
            Assert.Equal(resolution, got.Resolution);
            Assert.Equal(want.GetProperty("silicon_temp_degc").GetInt32(), got.SiliconTempDegc);
            Assert.Equal(want.GetProperty("distance_mm").EnumerateArray().Select(e => e.GetInt32()).ToArray(), got.DistanceMm);
            Assert.Equal(want.GetProperty("target_status").EnumerateArray().Select(e => (byte)e.GetInt32()).ToArray(), got.TargetStatus);
            Assert.Equal(want.GetProperty("nb_target_detected").EnumerateArray().Select(e => (byte)e.GetInt32()).ToArray(), got.NbTargetDetected);
            // Every per-zone array is trimmed to the resolution.
            Assert.Equal(resolution, got.DistanceMm.Length);
            Assert.Equal(resolution, got.SignalPerSpad.Length);
            Assert.Equal(resolution, got.AmbientPerSpad.Length);
            Assert.Equal(resolution, got.NbSpadsEnabled.Length);
            Assert.Equal(resolution, got.RangeSigmaMm.Length);
            Assert.Equal(resolution, got.Reflectance.Length);
            if (withCnh)
            {
                Assert.NotNull(got.CnhRaw);
                Assert.Equal(want.GetProperty("cnh_raw").GetString(), Vectors.ToHex(got.CnhRaw));
            }
            else
            {
                Assert.Null(got.CnhRaw);
                Assert.False(want.TryGetProperty("cnh_raw", out JsonElement c) && c.ValueKind != JsonValueKind.Null);
            }
        }
    }

    [Fact]
    public void Vl53l8Decoder_DoesNotTrimByDefault()
    {
        // The 4x4 L5 capture decoded with the untrimmed L8 path keeps the
        // 64-entry per-target blocks — the trim is opt-in (L8 unchanged).
        string[] recLines = File.ReadAllLines(Vectors.RecordingPath("vl53l5cx_4x4_15hz.depzrec"));
        var parser = new PacketParser();
        var reassembler = new FrameReassembler();
        var l8 = Vl53l8FrameDecoder.ForVariant(Vl53l8Variant.Ch);
        var l7 = Vl53l7Frames.CreateDecoder();
        for (int i = 1; i < recLines.Length; i++)
        {
            if (recLines[i].Length == 0)
                continue;
            using var evDoc = JsonDocument.Parse(recLines[i]);
            if (evDoc.RootElement.GetProperty("dir").GetString() != "rx")
                continue;
            foreach (ParserEvent pe in parser.Feed(Vectors.Hex(evDoc.RootElement.GetProperty("data").GetString()!)))
            {
                if (pe is not Packet pkt || pkt.Cmd != (int)Vl53l8Rpt.Vl53Frame || pkt.Payload.Length < 12)
                    continue;
                var done = reassembler.Feed(FrameChunk.Unpack(pkt.Payload));
                if (done is null)
                    continue;
                Vl53l8Frame untrimmed = l8.ParseFrame(done.Value.TimestampUs, done.Value.Frame);
                Assert.Equal(64, untrimmed.DistanceMm.Length);
                Vl53l8Frame inferred = l7.ParseFrame(done.Value.TimestampUs, done.Value.Frame);
                Vl53l8Frame explicitRes = l8.ParseFrame(done.Value.TimestampUs, done.Value.Frame, 16);
                Assert.Equal(16, inferred.Resolution);
                Assert.Equal(inferred.DistanceMm, explicitRes.DistanceMm);
                Assert.Equal(inferred.TargetStatus, explicitRes.TargetStatus);
                Assert.Equal(inferred.NbTargetDetected, explicitRes.NbTargetDetected);
                return;
            }
        }
        Assert.Fail("no frame in the 4x4 capture");
    }
}
