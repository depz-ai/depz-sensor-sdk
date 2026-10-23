using System.Text.Json;
using System.Text.Json.Nodes;
using Xunit;

namespace Depz.Sensor.Tests;

/// <summary>
/// <c>input_rules</c> groups (contract 07 §3): which inputs the SDK rejects and
/// that trailing payload bytes are ignored. Each case is <c>{op, in, expect}</c>
/// in the differential-test format (contract 16) and runs through the same SDK
/// calls as <see cref="DifftestAdapter"/>. A case whose op the adapter declares
/// unsupported is skipped (so it stays unreported to the coverage gate).
/// </summary>
public class InputRulesTests
{
    /// <summary>Every vector file that carries an <c>input_rules</c> group.</summary>
    public static IEnumerable<object[]> FilesWithInputRules() =>
        Vectors.Files()
            .Where(f => Vectors.Load(f) is { ValueKind: JsonValueKind.Object } r && r.TryGetProperty("input_rules", out _))
            .Select(f => new object[] { f });

    [Fact]
    public void InputRules_Found() => Assert.True(FilesWithInputRules().Count() >= 10);

    [Theory]
    [MemberData(nameof(FilesWithInputRules))]
    public void InputRules(string file)
    {
        var cases = Vectors.Load(file).GetProperty("input_rules").EnumerateArray().ToList();
        Assert.NotEmpty(cases);
        foreach (var c in cases)
        {
            string id = c.GetProperty("id").GetString()!;
            string op = c.GetProperty("op").GetString()!;
            if (DifftestAdapter.Unsupported.ContainsKey(op))
                continue;
            string line = $"{{\"id\":0,\"op\":{JsonSerializer.Serialize(op)},\"in\":{c.GetProperty("in").GetRawText()}}}";
            string resp = DifftestAdapter.Handle(line);
            var got = JsonNode.Parse(resp)!.AsObject();
            var expect = JsonNode.Parse(c.GetProperty("expect").GetRawText())!.AsObject();
            if (expect.ContainsKey("error"))
                Assert.True(got["error"]?.GetValue<string>() == "invalid", $"{id}: expected invalid, got {resp}");
            else
                Assert.True(!got.ContainsKey("error") && JsonNode.DeepEquals(got["out"], expect), $"{id}: got {resp}");
            Vectors.Seen(c);
        }
    }

    /// <summary>Divergences the differential test found (contract 16), pinned.</summary>
    [Fact]
    public void Difftest_Regressions()
    {
        // FRS Write Data is always 12 bytes (contract 05 §6): one word leaves slot 2 zero.
        Assert.Equal("f60049004433221100000000",
            Vectors.ToHex(Bno086.Sh2Control.FrsWriteDataReport(73, new uint[] { 0x11223344 })));
        // Device strings: bytes >= 0x80 decode to U+FFFD, never '?' (contract 02 §4.1).
        Assert.Equal("AP\uFFFDP", Protocol.Common.StripDeviceString(new byte[] { 0x41, 0x50, 0xE8, 0x50, 0x00, 0xFF }));
        // Product tokens in the contract's order, first match wins: VL53L4 before VL53L0_4.
        Assert.Equal(Protocol.SensorType.Vl53l4,
            Protocol.IdentityParser.ParseSoftwareName("APP_VL53L0_4_v0.2APP_VL53L4_v0.803").SensorType);
        Assert.Equal(Protocol.SensorType.Sr04,
            Protocol.IdentityParser.ParseSoftwareName("APP_VL53L0_4_v0.23APP_usonic_SR04_v0.95").SensorType);
        // Encoder arguments are range-checked, never masked (contract 07 §3).
        Assert.Throws<ArgumentOutOfRangeException>(() => new Bno086.ShtpHeader(32769, 3, 0).Pack());
        Assert.Throws<ArgumentOutOfRangeException>(() => Transport.Framing.BuildPacket(1, default, 0, (Transport.CrcType)6));
        Assert.Throws<Vl53l4.Vl53l4Exception>(() => Vl53l4.Vl53l4Uld.XtalkRaw(128));
    }
}
