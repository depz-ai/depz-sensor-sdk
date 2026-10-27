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

    [Fact]
    public void PullRangeIsOneReadSpanningTheBlocks()
    {
        var p = new FakeVl53lxRegMap();
        var img = new DeviceImage(p);
        p.SetWord(0x0018, 0xFFFE); // algo__crosstalk_compensation_x_plane_gradient_kcps
        p.Regs[0x001E] = 0xE0;     // algo__part_to_part_range_offset_mm, high byte
        p.Regs[0x001F] = 0x04;     // → 0xE004 & 0x1FFF = 0x0004
        img.PullRange(new[] { "static_nvm_managed", "customer_nvm_managed" });
        // 0x0001..0x0023: static_nvm (11) + the reserved 0x000C + customer_nvm (23)
        Assert.Equal(new[] { (0x0001, 35) }, p.Reads);
        Assert.Equal(FakeVl53lxBridge.DieOsc, img.StaticNvm["osc_measured__fast_osc__frequency"]);
        Assert.Equal(-2, img.CustomerNvm["algo__crosstalk_compensation_x_plane_gradient_kcps"]);
        Assert.Equal(4, img["customer_nvm_managed"]["algo__part_to_part_range_offset_mm"]);
    }

    [Fact]
    public void PushRangeGoesOutAsOneWriteWithTheReservedByteZeroed()
    {
        var p = new FakeVl53lxRegMap();
        var img = new DeviceImage(p);
        p.Regs[0x000C] = 0x77; // the hole: the C driver zeroes it, it does not read-modify-write it
        img.StaticNvm["vhv_config__init"] = 0x5A;
        img.CustomerNvm["mm_config__outer_offset_mm"] = -1;
        img.PushRange(new[] { "static_nvm_managed", "customer_nvm_managed" });
        Assert.Equal(new[] { (0x0001, 35) }, p.Writes);
        Assert.Equal(0x5A, p.Regs[0x000B]); // last byte of static_nvm
        Assert.Equal(0x00, p.Regs[0x000C]); // the hole at 0x000C
        Assert.Equal(0xFFFF, p.Word(0x0022));
    }

    [Fact]
    public void AnUnknownBlockIsAKeyError()
    {
        var img = new DeviceImage(new NoBus());
        Assert.Throws<KeyNotFoundException>(() => img["nonsense"]);
        Assert.Throws<KeyNotFoundException>(() => img.Pull("nonsense"));
        Assert.Throws<KeyNotFoundException>(() => img.Push("nonsense"));
    }

    [Fact]
    public void DumpListsEveryFieldOfTheNamedBlocks()
    {
        var img = new DeviceImage(new NoBus());
        img.SystemControl["system__mode_start"] = 0x42;
        string[] lines = img.Dump(new[] { "system_control" }).Split('\n');
        Assert.Equal("--- system_control @ 0x0083 (5 B)", lines[0]);
        Assert.Equal(1 + BareTables.Blocks["system_control"].Fields.Count, lines.Length);
        Assert.Equal(new[] { "system__mode_start", "66" }, lines[^1].Split(' ', StringSplitOptions.RemoveEmptyEntries));
        // default: all blocks, in table order
        var headers = img.Dump().Split('\n').Where(l => l.StartsWith("--- ", StringComparison.Ordinal));
        Assert.Equal(BareTables.Blocks.Keys, headers.Select(h => h.Split(' ')[1]));
    }

    [Fact]
    public void LlStateRdFollowsTheGphHandshakeAndFlipsTheTimingStatus()
    {
        const long Mode = 0x40; // a ranging mode bit inside DeviceMeasurementModeModeMask
        var s = new LlState();
        // SwStandby with the GPH id set: wait for the sync, stream count restarts
        s.UpdateRd(Mode, BareRegs.GroupedParameterHoldIdMask);
        Assert.Equal((BareDeviceState.RangingWaitGphSync, 0L, 0L, 0L), (s.RdDeviceState, s.RdStreamCount, s.RdGphId, s.RdTimingStatus));
        s.UpdateRd(Mode, 0);
        Assert.Equal((BareDeviceState.RangingOutputData, 0L, 2L), (s.RdDeviceState, s.RdStreamCount, s.RdGphId));
        s.UpdateRd(Mode, 0);
        Assert.Equal((1L, 0L, 1L), (s.RdStreamCount, s.RdGphId, s.RdTimingStatus));
        s.UpdateRd(Mode, 0);
        Assert.Equal((2L, 0L), (s.RdStreamCount, s.RdTimingStatus));
        // the stream count wraps 0xFF → 0x80, not to 0
        s.RdStreamCount = 0xFF;
        s.UpdateRd(Mode, 0);
        Assert.Equal(0x80L, s.RdStreamCount);
        // without the GPH id SwStandby goes straight to output data
        var t = new LlState();
        t.UpdateRd(Mode, 0);
        Assert.Equal(BareDeviceState.RangingOutputData, t.RdDeviceState);
        // mode start 0 (stop): the rd side is back at its reset values, the cfg side untouched
        s.CfgStreamCount = 9;
        s.UpdateRd(0x0F, 0);
        Assert.Equal((BareDeviceState.SwStandby, 0L, (long)BareRegs.GroupedParameterHoldIdMask, 0L),
            (s.RdDeviceState, s.RdStreamCount, s.RdGphId, s.RdTimingStatus));
        Assert.Equal(9L, s.CfgStreamCount);
        // a state the rd side never enters (dss_auto belongs to cfg) resets both sides
        s.RdDeviceState = BareDeviceState.RangingDssAuto;
        s.UpdateRd(Mode, 0);
        Assert.Equal((BareDeviceState.SwStandby, BareDeviceState.SwStandby, 0L, 0L),
            (s.RdDeviceState, s.CfgDeviceState, s.RdStreamCount, s.CfgStreamCount));
    }

    [Fact]
    public void LlStateCfgEntersDssAutoAndFlipsTheTimingStatus()
    {
        const long Mode = 0x40;
        var s = new LlState();
        s.UpdateCfg(Mode);
        Assert.Equal((BareDeviceState.RangingDssAuto, 1L, 0L, 1L), (s.CfgDeviceState, s.CfgStreamCount, s.CfgGphId, s.CfgTimingStatus));
        s.UpdateCfg(Mode);
        Assert.Equal((BareDeviceState.RangingDssAuto, 2L, 2L, 0L), (s.CfgDeviceState, s.CfgStreamCount, s.CfgGphId, s.CfgTimingStatus));
        s.CfgStreamCount = 0xFF;
        s.UpdateCfg(Mode);
        Assert.Equal(0x80L, s.CfgStreamCount);
        // mode start 0: the cfg side resets, the rd side untouched
        s.RdStreamCount = 5;
        s.UpdateCfg(0);
        Assert.Equal((BareDeviceState.SwStandby, 0L, (long)BareRegs.GroupedParameterHoldIdMask, 0L),
            (s.CfgDeviceState, s.CfgStreamCount, s.CfgGphId, s.CfgTimingStatus));
        Assert.Equal(5L, s.RdStreamCount);
        // a state the cfg side never enters resets both sides
        s.CfgDeviceState = BareDeviceState.RangingOutputData;
        s.UpdateCfg(Mode);
        Assert.Equal((BareDeviceState.SwStandby, 0L, 0L), (s.CfgDeviceState, s.CfgStreamCount, s.RdStreamCount));
    }
}
