using Depz.Sensor.Vl53l4;
using Xunit;

namespace Depz.Sensor.Tests;

// ── VL53L4CD golden vectors (contracts/vectors/vl53l4.json, contract 10) ─────

public class Vl53l4EncodeTests
{
    [Fact]
    public void CommandPayloads_ByteExact()
    {
        var root = Vectors.Load("vl53l4.json");
        foreach (var c in root.GetProperty("encode").EnumerateArray())
        {
            string name = c.GetProperty("name").GetString()!;
            string kind = c.GetProperty("kind").GetString()!;
            byte[] actual = kind switch
            {
                "read_reg" => Vl53l4Wire.PackReadReg(
                    (ushort)c.GetProperty("addr").GetInt32(),
                    (ushort)c.GetProperty("len").GetInt32()),
                "write_reg" => Vl53l4Wire.PackWriteReg(
                    (ushort)c.GetProperty("addr").GetInt32(),
                    Vectors.Hex(c.GetProperty("data").GetString()!)),
                "xshut" => Vl53l4Wire.PackXshut((byte)c.GetProperty("action").GetInt32()),
                "start_stream" => Vl53l4Wire.PackStartStream(
                    (ushort)c.GetProperty("addr").GetInt32(),
                    (ushort)c.GetProperty("len").GetInt32(),
                    (byte)c.GetProperty("flags").GetInt32()),
                "set_i2c_speed" => Vl53l4Wire.PackSetI2cSpeed((ushort)c.GetProperty("khz").GetInt32()),
                _ => throw new InvalidOperationException("unknown vl53l4 encode kind: " + kind),
            };
            string expected = c.GetProperty("payload").GetString()!;
            Assert.True(expected == Vectors.ToHex(actual),
                $"{name}: expected {expected} got {Vectors.ToHex(actual)}");
        }
    }
}

public class Vl53l4DecodeTests
{
    [Fact]
    public void Reports_DecodeByteExact()
    {
        var root = Vectors.Load("vl53l4.json");
        foreach (var c in root.GetProperty("decode").EnumerateArray())
        {
            string name = c.GetProperty("name").GetString()!;
            int report = c.GetProperty("report").GetInt32();
            byte[] payload = Vectors.Hex(c.GetProperty("payload").GetString()!);
            var expect = c.GetProperty("expect");
            switch ((Vl53l4Rpt)report)
            {
                case Vl53l4Rpt.RegData:
                    var rd = Vl53l4RegData.Unpack(payload);
                    Assert.Equal(expect.GetProperty("cmd").GetInt32(), rd.Cmd);
                    Assert.Equal(expect.GetProperty("timestamp_us").GetUInt64(), rd.TimestampUs);
                    Assert.Equal(expect.GetProperty("data").GetString(), Vectors.ToHex(rd.Data));
                    break;
                case Vl53l4Rpt.Info:
                    var info = Vl53l4Info.Unpack(payload);
                    Assert.Equal(expect.GetProperty("int_edges").GetUInt32(), info.IntEdges);
                    Assert.Equal(expect.GetProperty("slots_skipped").GetUInt32(), info.SlotsSkipped);
                    Assert.Equal(expect.GetProperty("i2c_errors").GetUInt32(), info.I2cErrors);
                    Assert.Equal(expect.GetProperty("last_i2c_error").GetInt32(), info.LastI2cError);
                    Assert.Equal(expect.GetProperty("model_id").GetInt32(), info.ModelId);
                    Assert.Equal(expect.GetProperty("fw_status").GetInt32(), info.FwStatus);
                    Assert.Equal(expect.GetProperty("initialized").GetInt32(), info.Initialized);
                    Assert.Equal(expect.GetProperty("xshut_level").GetInt32(), info.XshutLevel);
                    Assert.Equal(expect.GetProperty("int_level").GetInt32(), info.IntLevel);
                    Assert.Equal(expect.GetProperty("i2c_khz").GetInt32(), info.I2cKhz);
                    break;
                case Vl53l4Rpt.Stream:
                    var sd = Vl53l4StreamData.Unpack(payload);
                    Assert.Equal(expect.GetProperty("timestamp_us").GetUInt64(), sd.TimestampUs);
                    Assert.Equal(expect.GetProperty("addr").GetInt32(), sd.Addr);
                    Assert.Equal(expect.GetProperty("len").GetInt32(), sd.Len);
                    Assert.Equal(expect.GetProperty("data").GetString(), Vectors.ToHex(sd.Data));
                    break;
                default:
                    throw new InvalidOperationException($"{name}: unknown vl53l4 report {report:x2}");
            }
        }
    }
}

public class Vl53l4ResultBlockTests
{
    [Fact]
    public void ResultBlock_DecodeByteExact()
    {
        var root = Vectors.Load("vl53l4.json");
        foreach (var c in root.GetProperty("result_block").EnumerateArray())
        {
            string name = c.GetProperty("name").GetString()!;
            Vl53l4Results r = Vl53l4Uld.ParseResultBlock(Vectors.Hex(c.GetProperty("raw").GetString()!));
            var expect = c.GetProperty("expect");
            Assert.True(expect.GetProperty("range_status").GetInt32() == r.RangeStatus, $"{name}: range_status");
            Assert.True(expect.GetProperty("distance_mm").GetInt32() == r.DistanceMm, $"{name}: distance_mm");
            Assert.True(expect.GetProperty("ambient_rate_kcps").GetInt32() == r.AmbientRateKcps, $"{name}: ambient_rate_kcps");
            Assert.True(expect.GetProperty("ambient_per_spad_kcps").GetInt32() == r.AmbientPerSpadKcps, $"{name}: ambient_per_spad_kcps");
            Assert.True(expect.GetProperty("signal_rate_kcps").GetInt32() == r.SignalRateKcps, $"{name}: signal_rate_kcps");
            Assert.True(expect.GetProperty("signal_per_spad_kcps").GetInt32() == r.SignalPerSpadKcps, $"{name}: signal_per_spad_kcps");
            Assert.True(expect.GetProperty("number_of_spad").GetInt32() == r.NumberOfSpad, $"{name}: number_of_spad");
            Assert.True(expect.GetProperty("sigma_mm").GetInt32() == r.SigmaMm, $"{name}: sigma_mm");
            Assert.True(expect.GetProperty("stream_count").GetInt32() == r.StreamCount, $"{name}: stream_count");
        }
    }

    [Fact]
    public void ShortBlock_Throws()
    {
        Assert.Throws<Vl53l4Exception>(() => Vl53l4Uld.ParseResultBlock(new byte[14]));
    }
}

public class Vl53l4TimingTests
{
    [Fact]
    public void RangeTimingRegisters_ByteExact()
    {
        var root = Vectors.Load("vl53l4.json");
        foreach (var c in root.GetProperty("timing").GetProperty("encode").EnumerateArray())
        {
            string name = c.GetProperty("name").GetString()!;
            var (a, b, imRaw) = Vl53l4Uld.RangeTimingRegisters(
                c.GetProperty("timing_budget_ms").GetInt32(),
                c.GetProperty("inter_measurement_ms").GetInt32(),
                c.GetProperty("osc_frequency").GetInt32(),
                c.GetProperty("clock_pll").GetInt32());
            Assert.True(c.GetProperty("range_config_a").GetInt32() == a, $"{name}: range_config_a got {a}");
            Assert.True(c.GetProperty("range_config_b").GetInt32() == b, $"{name}: range_config_b got {b}");
            Assert.True(c.GetProperty("intermeasurement_raw").GetUInt32() == imRaw, $"{name}: intermeasurement_raw got {imRaw}");
        }
    }

    [Fact]
    public void DecodeRangeTiming_ByteExact()
    {
        var root = Vectors.Load("vl53l4.json");
        foreach (var c in root.GetProperty("timing").GetProperty("decode").EnumerateArray())
        {
            string name = c.GetProperty("name").GetString()!;
            var (budgetMs, interMs) = Vl53l4Uld.DecodeRangeTiming(
                c.GetProperty("intermeasurement_raw").GetUInt32(),
                c.GetProperty("clock_pll").GetInt32(),
                c.GetProperty("osc_frequency").GetInt32(),
                c.GetProperty("range_config_a").GetInt32());
            Assert.True(c.GetProperty("timing_budget_ms").GetInt32() == budgetMs, $"{name}: timing_budget_ms got {budgetMs}");
            Assert.True(c.GetProperty("inter_measurement_ms").GetInt32() == interMs, $"{name}: inter_measurement_ms got {interMs}");
        }
    }

    [Fact]
    public void InvalidInputs_Throw()
    {
        Assert.Throws<Vl53l4Exception>(() => Vl53l4Uld.RangeTimingRegisters(50, 0, 0));
        Assert.Throws<Vl53l4Exception>(() => Vl53l4Uld.RangeTimingRegisters(9, 0, 14720));
        Assert.Throws<Vl53l4Exception>(() => Vl53l4Uld.RangeTimingRegisters(201, 0, 14720));
        Assert.Throws<Vl53l4Exception>(() => Vl53l4Uld.RangeTimingRegisters(50, 40, 14720)); // 0 < inter <= budget
        Assert.Throws<Vl53l4Exception>(() => Vl53l4Uld.DecodeRangeTiming(0, 0, 0, 403));
        Assert.Throws<Vl53l4Exception>(() => Vl53l4Uld.SigmaThresholdRaw(16384));
    }
}

public class Vl53l4TuningTests
{
    [Fact]
    public void TuningWords_RoundTripByteExact()
    {
        var root = Vectors.Load("vl53l4.json");
        foreach (var c in root.GetProperty("tuning").EnumerateArray())
        {
            string name = c.GetProperty("name").GetString()!;
            string kind = c.GetProperty("kind").GetString()!;
            int value = c.GetProperty("value").GetInt32();
            int raw = c.GetProperty("raw").GetInt32();
            (int encoded, int decoded) = kind switch
            {
                "offset" => ((int)Vl53l4Uld.OffsetRaw(value), Vl53l4Uld.DecodeOffset((ushort)raw)),
                "xtalk" => ((int)Vl53l4Uld.XtalkRaw(value), Vl53l4Uld.DecodeXtalk((ushort)raw)),
                "signal_threshold" => (Vl53l4Uld.SignalThresholdRaw(value), Vl53l4Uld.DecodeSignalThreshold(raw)),
                "sigma_threshold" => (Vl53l4Uld.SigmaThresholdRaw(value), Vl53l4Uld.DecodeSigmaThreshold(raw)),
                _ => throw new InvalidOperationException("unknown vl53l4 tuning kind: " + kind),
            };
            Assert.True(raw == encoded, $"{name}: raw expected {raw} got {encoded}");
            Assert.True(value == decoded, $"{name}: value expected {value} got {decoded}");
        }
    }
}

public class Vl53l4ConfigBlockTests
{
    [Fact]
    public void ConfigBlock_ByteExact()
    {
        var root = Vectors.Load("vl53l4.json");
        var block = root.GetProperty("config_block");
        Assert.Equal(block.GetProperty("addr").GetInt32(), Vl53l4Uld.ConfigAddr);

        byte[] cfg = Vl53l4Uld.ConfigBlock();
        Assert.Equal(block.GetProperty("data").GetString(), Vectors.ToHex(cfg));

        // Byte 0 is the FM+ pad override; the tail matches ST's stock table.
        Assert.Equal(Vl53l4Uld.ConfigFmpByte, cfg[0]);
        Assert.Equal(Vl53l4Uld.DefaultConfiguration[1..].ToArray(), cfg[1..]);
        Assert.Equal(Vl53l4Uld.ConfigEnd - Vl53l4Uld.ConfigAddr + 1, cfg.Length);
    }
}
