using System.Text.Json;
using Depz.Sensor.Protocol;
using Depz.Sensor.Vl53l4;
using Depz.Sensor.Vl53lx;
using Xunit;

namespace Depz.Sensor.Tests;

// ── VL53L 1D family on the APP_VL53L0_4 bridge (contract 12, protocol v2.00) ──
//
// Golden vectors: vl53lx.json — encode, decode, products, model, die_block,
// l0x_raw, histogram_raw (blocks are real frames from recordings/vl53l*_*).
// Identity (APP_VL53L0_4_v0.23 / APP_VL53LX_v0.20 → vl53lx) is pinned by
// identity.json in IdentityTests.

public class Vl53lxVectorTests
{
    private static readonly JsonElement Root = Vectors.Load("vl53lx.json");

    private static void AssertFields(string name, JsonElement expect, IReadOnlyDictionary<string, long> got)
    {
        var expectedKeys = expect.EnumerateObject().Select(p => p.Name).OrderBy(k => k).ToArray();
        Assert.True(expectedKeys.SequenceEqual(got.Keys.OrderBy(k => k)),
            $"{name}: field set {string.Join(",", expectedKeys)} vs {string.Join(",", got.Keys.OrderBy(k => k))}");
        foreach (var (key, value) in got)
            Assert.True(expect.GetProperty(key).GetInt64() == value,
                $"{name}: {key} expected {expect.GetProperty(key).GetInt64()} got {value}");
    }

    [Fact]
    public void Encode_ByteExact()
    {
        int n = 0;
        foreach (var c in Root.GetProperty("encode").EnumerateArray())
        {
            string name = c.GetProperty("name").GetString()!;
            string kind = c.GetProperty("kind").GetString()!;
            byte[] payload = kind switch
            {
                "set_addr_width" => Vl53lxWire.PackSetAddrWidth(c.GetProperty("width").GetInt32()),
                "read_reg" => Vl53lxWire.PackReadReg(
                    (ushort)c.GetProperty("addr").GetInt32(), (ushort)c.GetProperty("len").GetInt32()),
                "write_reg" => Vl53lxWire.PackWriteReg(
                    (ushort)c.GetProperty("addr").GetInt32(), Vectors.Hex(c.GetProperty("data").GetString()!)),
                "xshut" => Vl53lxWire.PackXshut((byte)c.GetProperty("action").GetInt32()),
                "set_i2c_speed" => Vl53lxWire.PackSetI2cSpeed((ushort)c.GetProperty("khz").GetInt32()),
                "start_stream" => Vl53lxWire.PackStartStream(
                    (ushort)c.GetProperty("addr").GetInt32(),
                    (ushort)c.GetProperty("len").GetInt32(),
                    c.GetProperty("clear").EnumerateArray()
                        .Select(s => new Vl53lxClearStep((ushort)s[0].GetInt32(), (byte)s[1].GetInt32()))
                        .ToArray(),
                    (byte)c.GetProperty("flags").GetInt32()),
                _ => throw new InvalidOperationException($"{name}: unknown encode kind {kind}"),
            };
            string expected = c.GetProperty("payload").GetString()!;
            string actual = Vectors.ToHex(payload);
            Assert.True(expected == actual, $"{name}: expected {expected} got {actual}");
            if (kind == "start_stream")
                Assert.Equal(6 + 3 * c.GetProperty("clear").GetArrayLength(), payload.Length);
            n++;
            Vectors.Seen(c);
        }
        Assert.True(n > 0);
    }

    [Fact]
    public void Encode_Refusals()
    {
        var five = Enumerable.Repeat(new Vl53lxClearStep(0x86, 1), 5).ToArray();
        Assert.Throws<ArgumentOutOfRangeException>(() => Vl53lxWire.PackStartStream(0x0089, 17, five));
        Assert.Equal(6 + 3 * 4, Vl53lxWire.PackStartStream(0x0089, 17, five[..4]).Length);
        Assert.Equal(6, Vl53lxWire.PackStartStream(0x0089, 17).Length);
        Assert.Throws<ArgumentOutOfRangeException>(() => Vl53lxWire.PackSetAddrWidth(0));
        Assert.Throws<ArgumentOutOfRangeException>(() => Vl53lxWire.PackSetAddrWidth(3));
        Assert.Equal(0x39, (int)Vl53lxCmd.SetAddrWidth);
        Assert.Equal(0x35, (int)Vl53lxCmd.StartStream);
        Assert.Equal(0x92, (int)Vl53lxRpt.Info);
    }

    [Fact]
    public void Decode_InfoFieldExact()
    {
        int n = 0;
        foreach (var c in Root.GetProperty("decode").EnumerateArray())
        {
            string name = c.GetProperty("name").GetString()!;
            Assert.True(c.GetProperty("report").GetInt32() == (int)Vl53lxRpt.Info, name);
            var info = Vl53lxInfo.Unpack(Vectors.Hex(c.GetProperty("payload").GetString()!));
            AssertFields(name, c.GetProperty("expect"), new Dictionary<string, long>
            {
                ["int_edges"] = info.IntEdges,
                ["slots_skipped"] = info.SlotsSkipped,
                ["i2c_errors"] = info.I2cErrors,
                ["last_i2c_error"] = info.LastI2cError,
                ["xshut_level"] = info.XshutLevel,
                ["int_level"] = info.IntLevel,
                ["i2c_khz"] = info.I2cKhz,
                ["addr_width"] = info.AddrWidth,
                ["n_clear"] = info.NClear,
                ["frames_dropped"] = info.FramesDropped,
            });
            n++;
            Vectors.Seen(c);
        }
        Assert.True(n > 0);
        Assert.Throws<ArgumentException>(() => Vl53lxInfo.Unpack(new byte[Vl53lxWire.InfoSize - 1]));
    }

    [Fact]
    public void ProductTable()
    {
        var rows = Root.GetProperty("products").EnumerateArray().ToArray();
        Assert.Equal(rows.Select(r => r.GetProperty("product").GetString()), Vl53lxProducts.All.Select(p => p.Name));
        foreach (var r in rows)
        {
            string name = r.GetProperty("product").GetString()!;
            Vl53lxProduct p = Vl53lxProducts.Find(name)!;
            Assert.True(r.GetProperty("model_id").GetInt32() == p.ModelId, $"{name}: model_id");
            Assert.True(r.GetProperty("reach_mm").GetInt32() == p.ReachMm, $"{name}: reach_mm");
            Assert.Equal(
                r.GetProperty("driver_kinds").EnumerateArray().Select(k => k.GetString()),
                p.DriverKinds.Select(k => k.ToString().ToLowerInvariant()));
            Assert.Equal(r.GetProperty("default_driver").GetString(), p.DefaultDriver.ToString().ToLowerInvariant());
            Assert.True(r.GetProperty("addr_width").GetInt32() == p.AddrWidth, $"{name}: addr_width");
            Assert.True(r.GetProperty("max_khz").GetInt32() == p.MaxKhz, $"{name}: max_khz");
            Assert.Equal(
                r.GetProperty("clear_steps").EnumerateArray().Select(s => (s[0].GetInt32(), s[1].GetInt32())),
                p.ClearSteps.Select(s => ((int)s.Addr, (int)s.Value)));
            Assert.True(Vl53lxProducts.ModelIdOk(name, p.ModelId), name);
            // The USB PID in the table matches the production PID hint.
            Assert.Equal(name.ToLowerInvariant(), Usb.UsbIds.UsbModelHint(Usb.UsbIds.DepzUsbVid, p.UsbPid));
            Vectors.Seen(r);
        }
        Assert.False(Vl53lxProducts.ModelIdOk("VL53L0X", 0xEACC));
        Assert.False(Vl53lxProducts.ModelIdOk("VL53L4ED", 0xEBAA));
    }

    [Fact]
    public void ModelResolution()
    {
        int n = 0;
        foreach (var c in Root.GetProperty("model").EnumerateArray())
        {
            string name = c.GetProperty("name").GetString()!;
            string? usbModel = c.GetProperty("usb_model").ValueKind == JsonValueKind.Null
                ? null : c.GetProperty("usb_model").GetString();
            string deviceName = c.GetProperty("device_name").GetString()!;
            string? expectProduct = c.GetProperty("expect_product").ValueKind == JsonValueKind.Null
                ? null : c.GetProperty("expect_product").GetString();
            Assert.True(c.GetProperty("expect_class").GetString() == Vl53lxProducts.ResolveClass(usbModel, deviceName).ToString(),
                $"{name}: class");
            Assert.True(expectProduct == Vl53lxProducts.ProductFromBoardName(deviceName), $"{name}: product");
            n++;
            Vectors.Seen(c);
        }
        Assert.True(n > 0);
    }

    [Fact]
    public void DieBlock_FieldExact()
    {
        var seen = new HashSet<Vl53lxDieVariant>();
        foreach (var c in Root.GetProperty("die_block").EnumerateArray())
        {
            string name = c.GetProperty("name").GetString()!;
            var variant = Vl53lxDecode.ParseDieVariant(c.GetProperty("variant").GetString()!);
            seen.Add(variant);
            var r = Vl53lxDecode.DecodeDieBlock(Vectors.Hex(c.GetProperty("raw").GetString()!), variant);
            AssertFields(name, c.GetProperty("expect"), new Dictionary<string, long>
            {
                ["range_status"] = r.RangeStatus,
                ["distance_mm"] = r.DistanceMm,
                ["sigma_mm"] = r.SigmaMm,
                ["signal_rate_kcps"] = r.SignalRateKcps,
                ["ambient_rate_kcps"] = r.AmbientRateKcps,
                ["signal_per_spad_kcps"] = r.SignalPerSpadKcps,
                ["ambient_per_spad_kcps"] = r.AmbientPerSpadKcps,
                ["number_of_spad"] = r.NumberOfSpad,
                ["stream_count"] = r.StreamCount,
            });
            Vectors.Seen(c);
        }
        Assert.Equal(2, seen.Count);
        Assert.Throws<ArgumentException>(() => Vl53lxDecode.DecodeDieBlock(new byte[Vl53lxDecode.DieBlockLen - 1]));
    }

    [Fact]
    public void L0xRaw_FieldExact()
    {
        int n = 0;
        foreach (var c in Root.GetProperty("l0x_raw").EnumerateArray())
        {
            string name = c.GetProperty("name").GetString()!;
            var r = Vl53lxDecode.DecodeL0xRaw(Vectors.Hex(c.GetProperty("raw").GetString()!));
            AssertFields(name, c.GetProperty("expect"), new Dictionary<string, long>
            {
                ["distance_raw"] = r.DistanceRaw,
                ["device_range_status"] = r.DeviceRangeStatus,
                ["signal_rate_mcps_1616"] = r.SignalRateMcps1616,
                ["ambient_rate_mcps_1616"] = r.AmbientRateMcps1616,
                ["effective_spad_count_88"] = r.EffectiveSpadCount88,
            });
            n++;
            Vectors.Seen(c);
        }
        Assert.True(n > 0);
        Assert.Throws<ArgumentException>(() => Vl53lxDecode.DecodeL0xRaw(new byte[Vl53lxDecode.L0xBlockLen - 1]));
    }

    [Fact]
    public void HistogramRaw_FieldExact_InputUntouched()
    {
        int n = 0;
        foreach (var c in Root.GetProperty("histogram_raw").EnumerateArray())
        {
            string name = c.GetProperty("name").GetString()!;
            byte[] raw = Vectors.Hex(c.GetProperty("raw").GetString()!);
            byte[] before = (byte[])raw.Clone();
            var r = Vl53lxDecode.DecodeHistogramRaw(raw);
            Assert.True(before.AsSpan().SequenceEqual(raw), $"{name}: input mutated");
            JsonElement e = c.GetProperty("expect");
            var bins = e.GetProperty("bins").EnumerateArray().Select(b => b.GetInt32()).ToArray();
            Assert.Equal(Vl53lxDecode.HistogramBins, bins.Length);
            Assert.True(bins.SequenceEqual(r.Bins), $"{name}: bins");
            var fields = e.EnumerateObject().Where(p => p.Name != "bins").ToArray();
            var got = new Dictionary<string, long>
            {
                ["interrupt_status"] = r.InterruptStatus,
                ["range_status"] = r.RangeStatus,
                ["report_status"] = r.ReportStatus,
                ["stream_count"] = r.StreamCount,
                ["dss_actual_effective_spads"] = r.DssActualEffectiveSpads,
                ["reference_phase"] = r.ReferencePhase,
                ["vcsel_start"] = r.VcselStart,
            };
            Assert.Equal(fields.Length, got.Count);
            foreach (var f in fields)
                Assert.True(f.Value.GetInt64() == got[f.Name], $"{name}: {f.Name}");
            n++;
            Vectors.Seen(c);
        }
        Assert.True(n > 0);
        Assert.Throws<ArgumentException>(() => Vl53lxDecode.DecodeHistogramRaw(new byte[Vl53lxDecode.HistogramBlockLen - 1]));
    }

    /// <summary>The scripted platform of the l0x_uld cases: every register op the driver issues must be the
    /// next one of the vector's trace, a read answered with its recorded bytes.</summary>
    private sealed class TracePlatform : Vl53lxPlatform
    {
        private readonly List<JsonElement> _ops;
        public int Next;

        public TracePlatform(JsonElement trace) => _ops = trace.EnumerateArray().ToList();

        public int Count => _ops.Count;

        private byte[] Take(string op, int addr)
        {
            Assert.True(Next < _ops.Count, $"op #{Next} beyond the trace: {op} 0x{addr:X2}");
            var want = _ops[Next++];
            string wantOp = want.GetProperty("op").GetString()!;
            int wantAddr = want.GetProperty("addr").GetInt32();
            Assert.True(wantOp == op && wantAddr == addr,
                $"op #{Next - 1}: expected {wantOp} 0x{wantAddr:X2}, got {op} 0x{addr:X2}");
            return Vectors.Hex(want.GetProperty("data").GetString()!);
        }

        public override byte[] ReadMulti(int addr, int size)
        {
            byte[] data = Take("r", addr);
            Assert.True(data.Length == size, $"read 0x{addr:X2}: {size} B asked, {data.Length} recorded");
            return data;
        }

        public override void WriteMulti(int addr, byte[] data) =>
            Assert.True(Take("w", addr).SequenceEqual(data), $"write 0x{addr:X2}: {Convert.ToHexString(data)}");

        public override void SetI2cSpeed(int khz) => throw new InvalidOperationException("SET_I2C_SPEED is not in the trace");

        public override void SetAddrWidth(int width) => throw new InvalidOperationException("SET_ADDR_WIDTH is not in the trace");

        public override void XshutReset() => throw new InvalidOperationException("XSHUT is not in the trace");

        public override void SleepMs(int ms) { }
    }

    /// <summary>The VL53L0X ULD calls over the recorded register traces (the three ST divergences of 2fa4be5), on
    /// a fresh driver: the ops must come out in the recorded order, the trace must be used up, the host state /
    /// error match.</summary>
    [Fact]
    public void L0xUld_RegisterTrace()
    {
        int n = 0;
        foreach (var c in Root.GetProperty("l0x_uld").EnumerateArray())
        {
            string name = c.GetProperty("name").GetString()!;
            string kind = c.GetProperty("kind").GetString()!;
            var trace = new TracePlatform(c.GetProperty("trace"));
            var drv = new Vl53l0xUldDriver(trace, "VL53L0X");
            Action call = kind switch
            {
                "nvm_offset" => () => drv.GetInfoFromDevice(c.GetProperty("option").GetInt32()),
                "timing_budget" => () => drv.SetRangeTiming(
                    c.GetProperty("timing_budget_ms").GetInt32(), c.GetProperty("inter_measurement_ms").GetInt32()),
                "tuning" => () => drv.LoadTuningSettings(Vectors.Hex(c.GetProperty("buffer").GetString()!)),
                _ => throw new InvalidOperationException($"unknown l0x_uld kind {kind}"),
            };
            if (c.TryGetProperty("error", out var error))
            {
                var ex = Assert.Throws<Vl53lxException>(call);
                Assert.Contains(error.GetString()!, ex.Message);
            }
            else
            {
                call();
                c.TryGetProperty("expect", out var expect); // the budget case has none: its trace is the expectation
                if (kind == "nvm_offset")
                    Assert.True(expect.GetProperty("part2part_offset_adjustment_nvm_um").GetInt64() == drv.Part2PartOffsetAdjustmentNvmUm,
                        $"{name}: offset {drv.Part2PartOffsetAdjustmentNvmUm}");
                else if (kind == "tuning")
                    Assert.Equal(
                        new[]
                        {
                            expect.GetProperty("sigma_est_ref_array").GetInt32(),
                            expect.GetProperty("sigma_est_eff_pulse_width").GetInt32(),
                            expect.GetProperty("sigma_est_eff_amb_width").GetInt32(),
                            expect.GetProperty("target_ref_rate").GetInt32(),
                        },
                        new[] { drv.SigmaEstRefArray, drv.SigmaEstEffPulseWidth, drv.SigmaEstEffAmbWidth, drv.TargetRefRate });
            }
            Assert.True(trace.Next == trace.Count, $"{name}: {trace.Next} of {trace.Count} ops issued");
            Vectors.Seen(c);
            n++;
        }
        Assert.True(n >= 8, $"l0x_uld cases: {n}");
    }

    /// <summary>VL53L1X SetMode over the recorded register traces, on a fresh driver (ERRATA A9): the ops come
    /// out in the recorded order and use the trace up, and the call returns the budget in effect — one the new
    /// mode does not tabulate moved to the nearest one — or fails as the case says.</summary>
    [Fact]
    public void L1Uld_RegisterTrace()
    {
        int n = 0;
        foreach (var c in Root.GetProperty("l1_uld").EnumerateArray())
        {
            string name = c.GetProperty("name").GetString()!;
            Assert.Equal("set_mode", c.GetProperty("kind").GetString());
            var trace = new TracePlatform(c.GetProperty("trace"));
            var drv = new Vl53l1UldDriver(trace, "VL53L1CX");
            string mode = c.GetProperty("mode").GetString()!;
            if (c.TryGetProperty("error", out var error))
            {
                var ex = Assert.Throws<ArgumentException>(() => drv.SetMode(mode));
                Assert.Contains(error.GetString()!, ex.Message);
            }
            else
            {
                Assert.True(c.GetProperty("expect").GetProperty("timing_budget_ms").GetInt32() == drv.SetMode(mode),
                    $"{name}: budget returned");
            }
            Assert.True(trace.Next == trace.Count, $"{name}: {trace.Next} of {trace.Count} ops issued");
            Vectors.Seen(c);
            n++;
        }
        Assert.True(n >= 3, $"l1_uld cases: {n}");
    }

    [Fact]
    public void HistogramRaw_Bin23Patch()
    {
        // Bin 23 low byte = ((MSB << 2) + LSB) & 0xFF, from offsets 81/82 into 77.
        var raw = new byte[Vl53lxDecode.HistogramBlockLen];
        raw[75] = 0x01; raw[76] = 0x02; raw[77] = 0xEE; // stale low byte, must be replaced
        raw[81] = 0x41; raw[82] = 0x03;                  // (0x41 << 2) + 3 = 0x107 → 0x07
        var r = Vl53lxDecode.DecodeHistogramRaw(raw);
        Assert.Equal(0x010207, r.Bins[23]);
        Assert.Equal(0xEE, raw[77]);
    }

    [Fact]
    public void Identity_BothFirmwareNames()
    {
        Assert.Equal(SensorType.Vl53lx, IdentityParser.ParseSoftwareName("APP_VL53L0_4_v0.23").SensorType);
        Assert.Equal(SensorType.Vl53lx, IdentityParser.ParseSoftwareName("APP_VL53LX_v0.20").SensorType);
        Assert.Equal(SensorType.Vl53l4, IdentityParser.ParseSoftwareName("APP_VL53L4_v1.00").SensorType);
    }
}

// recordings/vl53l*_{uld,ulp}_*.depzrec decoded rx-side only (the C# SDK has no
// device layer to re-issue the tx half): every streamed 0x0089 die block
// through DecodeDieBlock, against the sidecar's distance and status. The
// histogram and VL53L0X captures need the ST ranging drivers (not in this SDK).
public class Vl53lxRecordingTests
{
    [Theory]
    [InlineData("vl53l1cb_uld_short_50ms", "l1")]
    [InlineData("vl53l1cx_uld_long_33ms", "l1")]
    [InlineData("vl53l3cx_ulp_33ms", "l4")]
    [InlineData("vl53l4cx_uld_as_l4cd_50ms", "l4")]
    public void DieBlockStream_RxDecode_MatchesSidecar(string stem, string variantName)
    {
        var exp = JsonDocument.Parse(File.ReadAllText(Vectors.RecordingPath(stem + ".expected.json"))).RootElement;
        var variant = Vl53lxDecode.ParseDieVariant(variantName);
        var frames = Vectors.RxPackets(stem + ".depzrec").Where(p => p.Cmd == (int)Vl53lxRpt.Stream)
            .Select(p => Vl53l4StreamData.Unpack(p.Payload)).ToList();
        var want = exp.GetProperty("frames").EnumerateArray().ToList();
        Assert.NotEmpty(want);
        Assert.True(frames.Count >= want.Count, $"{stem}: {frames.Count} frames, expected >= {want.Count}");
        for (int i = 0; i < want.Count; i++)
        {
            var w = want[i];
            Assert.Equal(5, w.EnumerateObject().Count());
            Assert.True(w.GetProperty("timestamp_us").GetUInt64() == frames[i].TimestampUs, $"{stem}[{i}]: timestamp_us");
            Assert.Equal(Vl53lxDecode.DieBlockAddr, frames[i].Addr);
            Assert.True(w.GetProperty("raw").GetString() == Vectors.ToHex(frames[i].Data), $"{stem}[{i}]: raw");
            var r = Vl53lxDecode.DecodeDieBlock(frames[i].Data, variant);
            Assert.True(w.GetProperty("distance_mm").GetInt32() == r.DistanceMm, $"{stem}[{i}]: distance_mm got {r.DistanceMm}");
            Assert.True(w.GetProperty("status").GetInt32() == r.RangeStatus, $"{stem}[{i}]: status got {r.RangeStatus}");
            Assert.Equal(0, w.GetProperty("targets").GetArrayLength()); // single-target drivers
        }
        Vectors.Seen($"recordings/{stem}.depzrec");
    }
}
