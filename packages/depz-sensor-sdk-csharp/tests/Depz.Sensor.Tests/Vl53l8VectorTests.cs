using System.Text.Json;
using Depz.Sensor.Vl53l8;
using Xunit;

namespace Depz.Sensor.Tests;

// ── VL53L8 SPI register bridge, the streaming side (contract 04 §2–§4) ──────
//
// Golden vectors: vl53l8.json — START_STREAM encode, RPT_VL53_FRAME chunk
// decode and the FrameReassembler rules over chunks a live board emitted.

public class Vl53l8VectorTests
{
    private static readonly JsonElement Root = Vectors.Load("vl53l8.json");

    [Fact]
    public void Encode_StartStream_ByteExact()
    {
        int n = 0;
        foreach (var c in Root.GetProperty("encode").EnumerateArray())
        {
            string name = c.GetProperty("name").GetString()!;
            Assert.True(c.GetProperty("kind").GetString() == "start_stream", $"{name}: kind");
            string expected = c.GetProperty("payload").GetString()!;
            string actual = Vectors.ToHex(Vl53l8Wire.PackStartStream(c.GetProperty("frame_size").GetInt32()));
            Assert.True(expected == actual, $"{name}: expected {expected} got {actual}");
            n++;
            Vectors.Seen(c);
        }
        Assert.True(n > 0);
    }

    [Fact]
    public void Decode_FrameChunk_FieldExact()
    {
        int n = 0;
        foreach (var c in Root.GetProperty("decode").EnumerateArray())
        {
            string name = c.GetProperty("name").GetString()!;
            int report = c.GetProperty("report").GetInt32();
            Assert.True(report == (int)Vl53l8Rpt.Vl53Frame, $"{name}: report");
            Tmf8829VectorTests.AssertFields(name, c.GetProperty("expect"),
                VectorShapes.Vl53l8Decode(report, Vectors.Hex(c.GetProperty("payload").GetString()!)));
            n++;
            Vectors.Seen(c);
        }
        Assert.True(n > 0);
    }

    [Fact]
    public void Decode_FrameChunkRejectsShortPayload()
    {
        Assert.Throws<ArgumentException>(() => FrameChunk.Unpack(new byte[11]));
        Assert.Empty(FrameChunk.Unpack(new byte[12]).Data);
    }

    [Fact]
    public void Reassembly_RulesOverLiveChunks()
    {
        int n = 0;
        foreach (var c in Root.GetProperty("reassembly").EnumerateArray())
        {
            string name = c.GetProperty("name").GetString()!;
            var chunks = c.GetProperty("chunks").EnumerateArray().Select(ch => Vectors.Hex(ch.GetString()!)).ToList();
            var got = VectorShapes.Vl53l8Reassembly(chunks);
            var expect = c.GetProperty("expect");
            var wantFrames = expect.GetProperty("frames").EnumerateArray().ToArray();
            var gotFrames = (List<object?>)got["frames"]!;
            Assert.True(wantFrames.Length == gotFrames.Count, $"{name}: {gotFrames.Count} frames, expected {wantFrames.Length}");
            for (int i = 0; i < wantFrames.Length; i++)
                Tmf8829VectorTests.AssertFields($"{name}#{i}", wantFrames[i], (Dictionary<string, object?>)gotFrames[i]!);
            Assert.True(expect.GetProperty("completed").GetInt32() == (int)got["completed"]!, $"{name}: completed");
            Assert.True(expect.GetProperty("discarded").GetInt32() == (int)got["discarded"]!, $"{name}: discarded");
            n++;
            Vectors.Seen(c);
        }
        Assert.True(n > 0);
    }
}
