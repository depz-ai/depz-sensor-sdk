using System.Text;
using System.Text.Json;
using Depz.Sensor.Protocol;
using Depz.Sensor.Transport;
using Depz.Sensor.Usb;
using Xunit;

namespace Depz.Sensor.Tests;

public class CrcTests
{
    [Fact]
    public void Crc_Kat_ByteExact()
    {
        var root = Vectors.Load("crc.json");
        foreach (var c in root.GetProperty("cases").EnumerateArray())
        {
            string name = c.GetProperty("name").GetString()!;
            byte[] input = Vectors.Hex(c.GetProperty("input").GetString()!);
            Assert.Equal(c.GetProperty("crc8_maxim").GetInt32(), Crc.Crc8Maxim(input));
            Assert.Equal((int)c.GetProperty("crc16_modbus").GetUInt32(), Crc.Crc16Modbus(input));
            Assert.Equal(c.GetProperty("crc32_iso_hdlc").GetUInt32(), Crc.Crc32IsoHdlc(input));
            Assert.Equal((int)c.GetProperty("crc16_ccitt_false").GetUInt32(), Crc.Crc16CcittFalse(input));
            _ = name;
        }
    }
}

public class FramingEncodeTests
{
    [Fact]
    public void BuildPacket_ByteExact()
    {
        var root = Vectors.Load("framing_encode.json");
        foreach (var c in root.GetProperty("cases").EnumerateArray())
        {
            string name = c.GetProperty("name").GetString()!;
            int cmd = c.GetProperty("cmd").GetInt32();
            int seq = c.GetProperty("seq").GetInt32();
            var crcType = (CrcType)c.GetProperty("crc_type").GetInt32();
            byte[] payload = Vectors.Hex(c.GetProperty("payload").GetString()!);
            string expected = c.GetProperty("frame").GetString()!;
            string actual = Vectors.ToHex(Framing.BuildPacket(cmd, payload, seq, crcType));
            Assert.True(expected == actual, $"{name}: expected {expected} got {actual}");
        }
    }
}

public class FramingDecodeTests
{
    private static string Describe(List<ParserEvent> events, string trashHex, string residueHex, int headerErrors)
    {
        var sb = new StringBuilder();
        foreach (var e in events)
        {
            switch (e)
            {
                case Packet p:
                    sb.Append($"P:{p.Cmd}:{p.Seq}:{Vectors.ToHex(p.Payload)};");
                    break;
                case CrcError ce:
                    sb.Append($"E:{ce.Cmd}:{ce.Seq};");
                    break;
            }
        }
        sb.Append("#trash=").Append(trashHex);
        sb.Append("#residue=").Append(residueHex);
        sb.Append("#hdrerr=").Append(headerErrors);
        return sb.ToString();
    }

    private static string Collect(IEnumerable<byte[]> chunks)
    {
        var parser = new PacketParser();
        var events = new List<ParserEvent>();
        var trash = new StringBuilder();
        foreach (var chunk in chunks)
        {
            foreach (var e in parser.Feed(chunk))
            {
                if (e is Packet || e is CrcError)
                    events.Add(e);
                else if (e is Trash t)
                    trash.Append(Vectors.ToHex(t.Data));
            }
        }
        return Describe(events, trash.ToString(), Vectors.ToHex(parser.Residue()), parser.HeaderErrors);
    }

    private static string ExpectedDescriptor(JsonElement expect)
    {
        var events = new List<ParserEvent>();
        foreach (var e in expect.GetProperty("events").EnumerateArray())
        {
            string type = e.GetProperty("type").GetString()!;
            if (type == "packet")
                events.Add(new Packet(e.GetProperty("cmd").GetInt32(), e.GetProperty("seq").GetInt32(),
                    Vectors.Hex(e.GetProperty("payload").GetString()!)));
            else
                events.Add(new CrcError(e.GetProperty("cmd").GetInt32(), e.GetProperty("seq").GetInt32()));
        }
        return Describe(events, expect.GetProperty("trash").GetString()!,
            expect.GetProperty("residue").GetString()!, expect.GetProperty("header_errors").GetInt32());
    }

    private static IEnumerable<byte[]> Whole(byte[] stream) => new[] { stream };

    private static IEnumerable<byte[]> Bytewise(byte[] stream)
    {
        foreach (byte b in stream)
            yield return new[] { b };
    }

    private static IEnumerable<byte[]> Random(byte[] stream, Random rng)
    {
        var outChunks = new List<byte[]>();
        int i = 0;
        while (i < stream.Length)
        {
            int n = 1 + rng.Next(37);
            int end = Math.Min(i + n, stream.Length);
            outChunks.Add(stream[i..end]);
            i = end;
        }
        return outChunks;
    }

    [Fact]
    public void Parser_ChunkingInvariant_ByteExact()
    {
        var root = Vectors.Load("framing_decode.json");
        var rng = new Random(0xDE92);
        foreach (var c in root.GetProperty("cases").EnumerateArray())
        {
            string name = c.GetProperty("name").GetString()!;
            byte[] stream = Vectors.Hex(c.GetProperty("stream").GetString()!);
            string expected = ExpectedDescriptor(c.GetProperty("expect"));

            Assert.True(expected == Collect(Whole(stream)), $"{name} (whole): expected [{expected}] got [{Collect(Whole(stream))}]");
            Assert.True(expected == Collect(Bytewise(stream)), $"{name} (bytewise)");
            for (int round = 0; round < 5; round++)
                Assert.True(expected == Collect(Random(stream, rng)), $"{name} (random round {round})");
        }
    }
}

public class UsbIdsTests
{
    [Fact]
    public void IdentityTable_And_ModelHint()
    {
        var root = Vectors.Load("usb_ids.json");
        Assert.Equal(UsbIds.DepzUsbVid, root.GetProperty("vid").GetInt32());
        Assert.Equal(UsbIds.DevUsbVid, root.GetProperty("dev_vid").GetInt32());
        Assert.Equal(UsbIds.DevUsbPid, root.GetProperty("dev_pid").GetInt32());
        var range = root.GetProperty("pid_range");
        Assert.Equal(UsbIds.DepzPidRangeLo, range[0].GetInt32());
        Assert.Equal(UsbIds.DepzPidRangeHi, range[1].GetInt32());

        foreach (var e in root.GetProperty("identity").EnumerateArray())
        {
            int vid = e.GetProperty("vid").GetInt32();
            int pid = e.GetProperty("pid").GetInt32();
            bool known = e.GetProperty("known").GetBoolean();
            string? model = e.GetProperty("model").ValueKind == JsonValueKind.Null
                ? null : e.GetProperty("model").GetString();
            Assert.True(known == UsbIds.IsKnownDepzUsb(vid, pid), $"known mismatch for {vid:X}:{pid:X}");
            Assert.True(model == UsbIds.UsbModelHint(vid, pid), $"model mismatch for {vid:X}:{pid:X}");
        }
    }
}

public class IdentityTests
{
    [Fact]
    public void ParseSoftwareName_ByteExact()
    {
        var root = Vectors.Load("identity.json");
        foreach (var c in root.GetProperty("cases").EnumerateArray())
        {
            string name = c.GetProperty("name").GetString()!;
            byte[] raw = Vectors.Hex(c.GetProperty("raw").GetString()!);
            string stripped = Common.StripDeviceString(raw);
            Identity id = IdentityParser.ParseSoftwareName(stripped);
            var expect = c.GetProperty("expect");

            Assert.Equal(expect.GetProperty("mode").GetString(), id.Mode);
            string? sensor = expect.GetProperty("sensor_type").ValueKind == JsonValueKind.Null
                ? null : expect.GetProperty("sensor_type").GetString();
            Assert.Equal(sensor, id.SensorType is null ? null : id.SensorType.ToString()!.ToLowerInvariant());
            Assert.Equal(expect.GetProperty("software_name").GetString(), id.SoftwareName);
            Assert.Equal(expect.GetProperty("version").GetString(), id.Version);
            _ = name;
        }
    }
}

public class CommonCommandsTests
{
    [Fact]
    public void Encode_ByteExact()
    {
        var root = Vectors.Load("common_commands.json");
        foreach (var c in root.GetProperty("encode").EnumerateArray())
        {
            string kind = c.GetProperty("kind").GetString()!;
            string expected = c.GetProperty("payload").GetString()!;
            string actual = kind switch
            {
                "sync_time_request" => Vectors.ToHex(Common.PackSyncTime(c.GetProperty("pc_timestamp_us").GetUInt64())),
                "set_payload_crc_type" => Vectors.ToHex(new[] { (byte)c.GetProperty("crc_type").GetInt32() }),
                "sync_pin_config" => Vectors.ToHex(new SyncPinConfig(
                    c.GetProperty("pin").GetInt32(),
                    (SyncPinMode)c.GetProperty("mode").GetInt32(),
                    (SyncPinPolarity)c.GetProperty("polarity").GetInt32()).Pack()),
                _ => throw new InvalidOperationException("unknown encode kind: " + kind),
            };
            Assert.True(expected == actual, $"{kind}: expected {expected} got {actual}");
        }
    }

    [Fact]
    public void Decode_ByteExact()
    {
        var root = Vectors.Load("common_commands.json");
        foreach (var c in root.GetProperty("decode").EnumerateArray())
        {
            int report = c.GetProperty("report").GetInt32();
            byte[] payload = Vectors.Hex(c.GetProperty("payload").GetString()!);
            var expect = c.GetProperty("expect");
            switch ((Rpt)report)
            {
                case Rpt.Status:
                    var st = StatusReport.Unpack(payload);
                    Assert.Equal(expect.GetProperty("cmd").GetInt32(), st.Cmd);
                    Assert.Equal(expect.GetProperty("status").GetInt32(), st.Status);
                    break;
                case Rpt.Text:
                    var tx = TextReport.Unpack(payload);
                    Assert.Equal(expect.GetProperty("cmd").GetInt32(), tx.Cmd);
                    Assert.Equal(expect.GetProperty("text").GetString(), tx.Text);
                    break;
                case Rpt.Temperature:
                    var tp = TemperatureReport.Unpack(payload);
                    Assert.Equal(expect.GetProperty("timestamp_us").GetUInt64(), tp.TimestampUs);
                    Assert.Equal(expect.GetProperty("raw_decidegrees").GetInt32(), tp.RawDecidegrees);
                    break;
                case Rpt.SequenceError:
                    var se = SequenceErrorReport.Unpack(payload);
                    Assert.Equal(expect.GetProperty("expected_seq").GetInt32(), se.ExpectedSeq);
                    Assert.Equal(expect.GetProperty("received_seq").GetInt32(), se.ReceivedSeq);
                    break;
                default:
                    throw new InvalidOperationException("unhandled report: " + report);
            }
        }
    }

    [Fact]
    public void SyncTimeMath_ByteExact()
    {
        var root = Vectors.Load("common_commands.json");
        foreach (var c in root.GetProperty("sync_time_math").EnumerateArray())
        {
            long t1 = c.GetProperty("t1").GetInt64();
            long t2 = c.GetProperty("t2").GetInt64();
            long t3 = c.GetProperty("t3").GetInt64();
            long t4 = c.GetProperty("t4").GetInt64();
            var (offset, rtt) = Common.SyncTimeOffsetRtt(t1, t2, t3, t4);
            Assert.Equal(c.GetProperty("offset_us").GetInt64(), offset);
            Assert.Equal(c.GetProperty("rtt_us").GetInt64(), rtt);
        }
    }
}

public class Sr04Tests
{
    [Fact]
    public void Encode_ByteExact()
    {
        var root = Vectors.Load("sr04.json");
        foreach (var c in root.GetProperty("encode").EnumerateArray())
        {
            string kind = c.GetProperty("kind").GetString()!;
            string expected = c.GetProperty("payload").GetString()!;
            string actual = kind switch
            {
                "set_sample_period" => Vectors.ToHex(Sr04.PackSamplePeriod((uint)c.GetProperty("period_us").GetInt64())),
                "set_echo_decay" => Vectors.ToHex(Sr04.PackEchoDecay((ushort)c.GetProperty("decay_us").GetInt32())),
                _ => throw new InvalidOperationException("unknown sr04 encode kind: " + kind),
            };
            Assert.True(expected == actual, $"{kind}: expected {expected} got {actual}");
        }
    }

    [Fact]
    public void Decode_ByteExact()
    {
        var root = Vectors.Load("sr04.json");
        foreach (var c in root.GetProperty("decode").EnumerateArray())
        {
            int report = c.GetProperty("report").GetInt32();
            byte[] payload = Vectors.Hex(c.GetProperty("payload").GetString()!);
            var expect = c.GetProperty("expect");
            switch ((Sr04Rpt)report)
            {
                case Sr04Rpt.Data:
                    var d = Sr04Data.Unpack(payload);
                    Assert.Equal(expect.GetProperty("source_cmd").GetInt32(), d.SourceCmd);
                    Assert.Equal(expect.GetProperty("timestamp_us").GetUInt64(), d.TimestampUs);
                    Assert.Equal(expect.GetProperty("echo_time_us").GetInt32(), d.EchoTimeUs);
                    break;
                case Sr04Rpt.SamplePeriod:
                    Assert.Equal(expect.GetProperty("period_us").GetUInt32(), Sr04.UnpackSamplePeriod(payload));
                    break;
                case Sr04Rpt.EchoDecay:
                    Assert.Equal((ushort)expect.GetProperty("decay_us").GetInt32(), Sr04.UnpackEchoDecay(payload));
                    break;
                default:
                    throw new InvalidOperationException("unhandled sr04 report: " + report);
            }
        }
    }
}

public class FwDepzTests
{
    [Fact]
    public void Parse_ByteExact()
    {
        var root = Vectors.Load("fwdepz.json");
        foreach (var c in root.GetProperty("cases").EnumerateArray())
        {
            string name = c.GetProperty("name").GetString()!;
            byte[] blob = Vectors.Hex(c.GetProperty("file").GetString()!);
            if (c.TryGetProperty("error", out var errEl) && errEl.ValueKind == JsonValueKind.String)
            {
                string code = errEl.GetString()!;
                var ex = Assert.Throws<FwDepz.FwDepzException>(() => FwDepz.FwDepzImage.Parse(blob));
                Assert.True(code == ex.Code, $"{name}: expected error {code} got {ex.Code}");
            }
            else
            {
                var img = FwDepz.FwDepzImage.Parse(blob);
                var expect = c.GetProperty("expect");
                Assert.Equal(expect.GetProperty("load_addr").GetUInt32(), img.LoadAddr);
                Assert.Equal(expect.GetProperty("fw_size").GetUInt32(), img.FwSize);
                Assert.Equal(expect.GetProperty("fw_crc32").GetUInt32(), img.FwCrc32);
                Assert.Equal(expect.GetProperty("cur_sec").GetInt32(), img.CurSec);
                Assert.Equal(expect.GetProperty("tot_sec").GetInt32(), img.TotSec);
                Assert.Equal(expect.GetProperty("payload_crc_ok").GetBoolean(), img.PayloadCrcOk);
            }
        }
    }
}
