using Depz.Sensor.Json;
using Xunit;

namespace Depz.Sensor.Tests;

// The SDK's own JSON (netstandard2.1 / Unity has no System.Text.Json).
public class JsonTextTests
{
    [Fact]
    public void ParsesEveryValueKind()
    {
        var v = (Dictionary<string, object?>)JsonText.Parse(
            " {\"a\":1,\"b\":-2.5e1,\"c\":\"x\\\"\\u00e9\\n\",\"d\":[true,false,null],\"e\":{}} ")!;
        Assert.Equal(new[] { "a", "b", "c", "d", "e" }, v.Keys.ToArray());
        Assert.Equal(1L, v["a"]);
        Assert.Equal(-25.0, v["b"]);
        Assert.Equal("x\"é\n", v["c"]);
        Assert.Equal(new object?[] { true, false, null }, (List<object?>)v["d"]!);
        Assert.Empty((Dictionary<string, object?>)v["e"]!);
    }

    [Fact]
    public void KeepsWideIntegersExact()
    {
        Assert.Equal(long.MinValue, JsonText.Parse("-9223372036854775808"));
        Assert.Equal(ulong.MaxValue, JsonText.Parse("18446744073709551615"));
        Assert.Equal("18446744073709551615", JsonText.Serialize(ulong.MaxValue));
    }

    [Fact]
    public void SerializesCompactAndRoundTrips()
    {
        var tree = new Dictionary<string, object?>
        {
            ["t"] = 1234567L,
            ["k"] = "sr04",
            ["v"] = new Dictionary<string, object?> { ["echo_us"] = 5831, ["f"] = 0.1, ["raw"] = new byte[] { 0xAB, 0x01 } },
            ["l"] = new[] { 1, 2 },
            ["s"] = "tab\there \u0001",
        };
        string text = JsonText.Serialize(tree);
        Assert.Equal(
            "{\"t\":1234567,\"k\":\"sr04\",\"v\":{\"echo_us\":5831,\"f\":0.1,\"raw\":\"ab01\"},\"l\":[1,2],\"s\":\"tab\\there \\u0001\"}",
            text);
        Assert.Equal(text, JsonText.Serialize(JsonText.Parse(text)));
    }

    [Theory]
    [InlineData("")]
    [InlineData("{")]
    [InlineData("[1,]")]
    [InlineData("{\"a\" 1}")]
    [InlineData("01")]
    [InlineData("1.")]
    [InlineData("\"\\x\"")]
    [InlineData("tru")]
    [InlineData("1 2")]
    public void RejectsInvalidText(string text) =>
        Assert.Throws<FormatException>(() => JsonText.Parse(text));
}
