using System.Text.Json;
using Depz.Sensor.Bno086;
using Depz.Sensor.Protocol;
using Depz.Sensor.Usb;
using Xunit;

namespace Depz.Sensor.Tests;

// probe.json: class resolution (contract 02 §4.4, Python reference resolve_sensor_class).
public class ProbeTests
{
    [Fact]
    public void ClassNames_MatchVector()
    {
        var want = Vectors.Load("probe.json").GetProperty("classes").EnumerateArray()
            .Select(e => e.GetString()!).ToList();
        Assert.Equal(want, Discovery.SensorClassNames);
        Vectors.Seen("probe/classes");
    }

    [Fact]
    public void Resolve_MatchesVectors()
    {
        foreach (var c in Vectors.Load("probe.json").GetProperty("resolve").EnumerateArray())
        {
            static int? Id(JsonElement e) => e.ValueKind == JsonValueKind.Null ? null : e.GetInt32();
            string got = Discovery.ResolveSensorClass(
                c.GetProperty("software_name").GetString()!, c.GetProperty("device_name").GetString(),
                Id(c.GetProperty("usb_vid")), Id(c.GetProperty("usb_pid")));
            Assert.True(c.GetProperty("class").GetString() == got, $"{c.GetProperty("id")}: got {got}");
            Assert.Contains(got, Discovery.SensorClassNames);
            Vectors.Seen(c);
        }
    }
}

// recordings/bno086_canonical.depzrec (contract 15 scenario bno086_canonical,
// opened the §4.4 way) decoded rx-side only, like bno086_session: the probe's
// text answers resolve to the sidecar's class, and the 50 reports in its
// `results` (the scenario's collect step) match the first 50 SH-2 reports.
public class Bno086CanonicalRecordingTests
{
    private const int RptData = 0x91; // cmd u8, capture timestamp_us u64, raw SHTP frame

    [Fact]
    public void Canonical_RxDecode_MatchesSidecar()
    {
        var exp = JsonDocument.Parse(File.ReadAllText(Vectors.RecordingPath("bno086_canonical.expected.json"))).RootElement;
        var packets = Vectors.RxPackets("bno086_canonical.depzrec");

        string? Text(Cmd cmd) => packets.Where(p => p.Cmd == (int)Rpt.Text)
            .Select(p => TextReport.Unpack(p.Payload)).FirstOrDefault(t => t.Cmd == (int)cmd)?.Text;
        string? sw = Text(Cmd.GetNameActiveSoftware);
        string? name = Text(Cmd.GetDeviceName);
        Assert.Equal(exp.GetProperty("software_name").GetString(), sw);
        Assert.Equal(exp.GetProperty("device_name").GetString(), name);
        int vid = exp.GetProperty("usb_vid").GetInt32();
        int pid = exp.GetProperty("usb_pid").GetInt32();
        Assert.Equal(exp.GetProperty("usb_model").GetString(), UsbIds.UsbModelHint(vid, pid));
        Assert.Equal(exp.GetProperty("class").GetString(), Discovery.ResolveSensorClass(sw!, name, vid, pid));

        var layer = new ShtpLayer();
        var reports = new List<BnoReport>();
        foreach (var pkt in packets)
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

        var want = exp.GetProperty("results").EnumerateArray()
            .First(r => r.ValueKind == JsonValueKind.Array && r.GetArrayLength() > 0)
            .EnumerateArray().ToList();
        Assert.True(reports.Count >= want.Count, $"decoded {reports.Count} reports, expected >= {want.Count}");
        for (int i = 0; i < want.Count; i++)
        {
            Assert.Equal(want[i].EnumerateObject().Count(), reports[i].Fields().Count);
            Bno086ReportsTests.AssertFields($"report {i}", reports[i].Fields(), want[i]);
        }
        Vectors.Seen("recordings/bno086_canonical.depzrec");
    }
}
