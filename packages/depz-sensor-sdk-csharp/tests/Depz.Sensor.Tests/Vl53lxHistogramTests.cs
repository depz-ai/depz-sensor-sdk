using Depz.Sensor.Vl53lx;
using Depz.Sensor.Vl53lx.Bare;
using Xunit;

namespace Depz.Sensor.Tests;

public class Vl53lxHistogramTests
{
    // The BareDriver touches the bus only where a test does not reach.
    private sealed class NoBus : Vl53lxPlatform
    {
        public override byte[] ReadMulti(int addr, int size) => throw new InvalidOperationException("no bus");
        public override void WriteMulti(int addr, byte[] data) => throw new InvalidOperationException("no bus");
        public override void SetI2cSpeed(int khz) { }
        public override void SetAddrWidth(int width) { }
        public override void XshutReset() { }
    }

    [Fact]
    public void RegBlockRoundTripsByteExactWithSignedMaskedFields()
    {
        var raw = Enumerable.Range(0, 23).Select(i => (byte)(0xA0 + i)).ToArray();
        raw[17] = 0xFF; // algo__part_to_part_range_offset_mm: signed, mask 0x1FFF
        raw[18] = 0xF0;
        var blk = new RegBlock("customer_nvm_managed").Decode(raw);
        Assert.Equal(0x1FF0, blk["algo__part_to_part_range_offset_mm"]); // masked first, so it reads positive
        Assert.Equal(raw[0], blk["global_config__spad_enables_ref_0"]);
        blk["mm_config__inner_offset_mm"] = -5;
        byte[] enc = blk.Encode();
        Assert.Equal(new byte[] { 0xFF, 0xFB }, enc.AsSpan(19, 2).ToArray());
        Assert.Equal(-5, new RegBlock("customer_nvm_managed").Decode(enc)["mm_config__inner_offset_mm"]);
        Assert.Throws<KeyNotFoundException>(() => blk["no_such_field"]);
    }

    [Fact]
    public void RepeatedBinCodesAreAveragedIntoOneBinEach()
    {
        var src = new HistogramBinData { BinSeq = new[] { 7, 0, 1, 1, 2, 2 } };
        for (int i = 0; i < 24; i++)
            src.BinData[i] = i < 4 ? 100 : 1000 + i;
        var dst = Hist.AverageRepeatedBins(src);
        Assert.Equal(new[] { 7, 0, 1, 2, 16, 16 }, dst.BinSeq);
        Assert.Equal(new long[] { 1, 1, 2, 2, 0, 0 }, dst.BinRep);
        Assert.Equal(16, dst.BinsInData);
        Assert.Equal(4, dst.NumberOfAmbientBins);
        Assert.Equal((1008 + 1012 + 1) / 2, dst.BinData[8]); // codes 1 at bins 8 and 12, rounded half up
    }

    [Fact]
    public void AnL4DieRefusesTheShortPreset()
    {
        var bare = new BareDriver(new NoBus());
        bare.Img.NvmCopyData["identification__module_type"] = 0xAA;
        bare.Img.NvmCopyData["identification__model_id"] = 0xEB;
        Assert.True(bare.IsL4());
        Assert.Equal(new[] { "medium", "long" }, bare.DistanceModesForDie());
        Assert.Equal(200000, bare.FdaMaxTimingBudgetUs());
        Assert.Throws<ArgumentException>(() => bare.SetDistanceMode("short"));
    }

    [Fact]
    public void IsqrtIsTheFloorOfAUint32Root()
    {
        Assert.Equal(65535, Hist.Isqrt(0xFFFFFFFFL));
        Assert.Equal(3, Hist.Isqrt(15));
        Assert.Equal(4, Hist.Isqrt(16));
        Assert.Equal(0, Hist.Isqrt(1L << 32)); // the argument is uint32 in C
    }
}
