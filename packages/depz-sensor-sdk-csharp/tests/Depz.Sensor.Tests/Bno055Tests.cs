using System.Text.Json;
using Depz.Sensor.Bno055;
using Depz.Sensor.Protocol;
using Xunit;

namespace Depz.Sensor.Tests;

// ── BNO055 9-axis IMU on the APP_BNO055 register bridge (contract 13, protocol v0.10) ──
//
// Golden vectors: bno055.json — encode, decode, units, calib_stat,
// calibration_profile, axis_remap (+ axis_remap_invalid), sensor_config,
// blocks. Identity (APP_BNO055_v0.12 → bno055) is pinned by identity.json in
// IdentityTests.

public class Bno055VectorTests
{
    private static readonly JsonElement Root = Vectors.Load("bno055.json");

    private static void AssertFields(string name, JsonElement expect, IReadOnlyDictionary<string, long> got)
    {
        var expectedKeys = expect.EnumerateObject().Select(p => p.Name).OrderBy(k => k).ToArray();
        Assert.True(expectedKeys.SequenceEqual(got.Keys.OrderBy(k => k)),
            $"{name}: field set {string.Join(",", expectedKeys)} vs {string.Join(",", got.Keys.OrderBy(k => k))}");
        foreach (var (key, value) in got)
            Assert.True(expect.GetProperty(key).GetInt64() == value,
                $"{name}: {key} expected {expect.GetProperty(key).GetInt64()} got {value}");
    }

    /// <summary>A JSON integer list, or a scalar integer as a one-element list.</summary>
    private static int[] Ints(JsonElement e) => e.ValueKind == JsonValueKind.Number
        ? new[] { e.GetInt32() }
        : e.EnumerateArray().Select(x => x.GetInt32()).ToArray();

    /// <summary>A channel is either null in both, or the same integer list.</summary>
    private static void AssertChannel(string name, JsonElement expect, int[]? got)
    {
        if (expect.ValueKind == JsonValueKind.Null)
            Assert.True(got is null, $"{name}: expected null, got [{string.Join(",", got ?? Array.Empty<int>())}]");
        else
            Assert.True(got is not null && Ints(expect).SequenceEqual(got),
                $"{name}: expected {expect}, got {(got is null ? "null" : string.Join(",", got))}");
    }

    private static int[]? Arr(Bno055Vec3? v) => v is { } x ? new int[] { x.X, x.Y, x.Z } : null;

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
                "read_reg" => Bno055Wire.PackReadReg(
                    (byte)c.GetProperty("addr").GetInt32(), (byte)c.GetProperty("len").GetInt32()),
                "write_reg" => Bno055Wire.PackWriteReg(
                    (byte)c.GetProperty("addr").GetInt32(), Vectors.Hex(c.GetProperty("data").GetString()!)),
                "start_stream" => Bno055Wire.PackStartStream(
                    (Bno055Trigger)c.GetProperty("trigger").GetInt32(),
                    (byte)c.GetProperty("addr").GetInt32(),
                    (byte)c.GetProperty("len").GetInt32(),
                    (ushort)c.GetProperty("period_ms").GetInt32()),
                "reset" or "stop_stream" or "get_info" => Array.Empty<byte>(),
                _ => throw new InvalidOperationException($"{name}: unknown encode kind {kind}"),
            };
            string expected = c.GetProperty("payload").GetString()!;
            string actual = Vectors.ToHex(payload);
            Assert.True(expected == actual, $"{name}: expected {expected} got {actual}");
            n++;
        }
        Assert.True(n > 0);
        Assert.Equal(0x32, (int)Bno055Cmd.ReadReg);
        Assert.Equal(0x33, (int)Bno055Cmd.WriteReg);
        Assert.Equal(0x34, (int)Bno055Cmd.Reset);
        Assert.Equal(0x35, (int)Bno055Cmd.StartStream);
        Assert.Equal(0x36, (int)Bno055Cmd.StopStream);
        Assert.Equal(0x37, (int)Bno055Cmd.GetInfo);
        Assert.Equal(0x91, (int)Bno055Rpt.RegData);
        Assert.Equal(0x92, (int)Bno055Rpt.Info);
        Assert.Equal(0x93, (int)Bno055Rpt.Stream);
    }

    [Fact]
    public void Decode_FieldExact()
    {
        var seen = new HashSet<int>();
        foreach (var c in Root.GetProperty("decode").EnumerateArray())
        {
            string name = c.GetProperty("name").GetString()!;
            int report = c.GetProperty("report").GetInt32();
            seen.Add(report);
            byte[] raw = Vectors.Hex(c.GetProperty("payload").GetString()!);
            JsonElement e = c.GetProperty("expect");
            switch ((Bno055Rpt)report)
            {
                case Bno055Rpt.Info:
                {
                    var i = Bno055Info.Unpack(raw);
                    AssertFields(name, e, new Dictionary<string, long>
                    {
                        ["i2c_addr"] = i.I2cAddr,
                        ["chip_id"] = i.ChipId,
                        ["acc_id"] = i.AccId,
                        ["mag_id"] = i.MagId,
                        ["gyr_id"] = i.GyrId,
                        ["sw_rev"] = i.SwRev,
                        ["bl_rev"] = i.BlRev,
                        ["initialized"] = i.Initialized,
                        ["int_level"] = i.IntLevel,
                        ["int_edges"] = i.IntEdges,
                        ["read_min_us"] = i.ReadMinUs,
                        ["read_max_us"] = i.ReadMaxUs,
                        ["read_avg_us"] = i.ReadAvgUs,
                        ["tx_dropped"] = i.TxDropped,
                        ["i2c_errors"] = i.I2cErrors,
                        ["slots_skipped"] = i.SlotsSkipped,
                        ["bus_recoveries"] = i.BusRecoveries,
                        ["last_i2c_error"] = i.LastI2cError,
                        ["sensor_resets"] = i.SensorResets,
                        ["loop_max_us"] = i.LoopMaxUs,
                    });
                    Assert.True(i.IdsOk, name);
                    Assert.Equal("03.11", i.SwRevText);
                    break;
                }
                case Bno055Rpt.RegData:
                {
                    var r = Bno055RegData.Unpack(raw);
                    Assert.Equal(3, e.EnumerateObject().Count());
                    Assert.Equal(e.GetProperty("cmd").GetInt32(), r.Cmd);
                    Assert.Equal(e.GetProperty("timestamp_us").GetUInt64(), r.TimestampUs);
                    Assert.Equal(e.GetProperty("data").GetString(), Vectors.ToHex(r.Data));
                    break;
                }
                case Bno055Rpt.Stream:
                {
                    var s = Bno055StreamData.Unpack(raw);
                    Assert.Equal(4, e.EnumerateObject().Count());
                    Assert.Equal(e.GetProperty("timestamp_us").GetUInt64(), s.TimestampUs);
                    Assert.Equal(e.GetProperty("addr").GetInt32(), s.Addr);
                    Assert.Equal(e.GetProperty("len").GetInt32(), s.Length);
                    Assert.Equal(e.GetProperty("data").GetString(), Vectors.ToHex(s.Data));
                    break;
                }
                default:
                    throw new InvalidOperationException($"{name}: unknown report {report}");
            }
        }
        Assert.Equal(3, seen.Count);
        Assert.Throws<ArgumentException>(() => Bno055Info.Unpack(new byte[Bno055Wire.InfoSize - 1]));
    }

    [Fact]
    public void Units_FlagsAndRepack()
    {
        int n = 0;
        foreach (var c in Root.GetProperty("units").EnumerateArray())
        {
            byte sel = (byte)c.GetProperty("unit_sel").GetInt32();
            var u = Bno055Units.Unpack(sel);
            JsonElement e = c.GetProperty("expect");
            Assert.Equal(5, e.EnumerateObject().Count());
            Assert.True(e.GetProperty("accel_mg").GetBoolean() == u.AccelMg, $"0x{sel:x2} accel_mg");
            Assert.True(e.GetProperty("gyro_rps").GetBoolean() == u.GyroRps, $"0x{sel:x2} gyro_rps");
            Assert.True(e.GetProperty("euler_rad").GetBoolean() == u.EulerRad, $"0x{sel:x2} euler_rad");
            Assert.True(e.GetProperty("temp_f").GetBoolean() == u.TempF, $"0x{sel:x2} temp_f");
            Assert.True(e.GetProperty("android").GetBoolean() == u.Android, $"0x{sel:x2} android");
            Assert.True(c.GetProperty("repack").GetInt32() == u.Pack(), $"0x{sel:x2} repack");
            n++;
        }
        Assert.True(n > 0);
    }

    [Fact]
    public void Units_LsbConstants()
    {
        var si = Bno055Units.Default;
        Assert.Equal(0, si.Pack());
        Assert.Equal((100.0, 16.0, 16.0, 1.0), (si.AccelLsb, si.GyroLsb, si.EulerLsb, si.TempLsb));
        var all = Bno055Units.Unpack(0x97);
        Assert.Equal((1.0, 900.0, 900.0, 0.5), (all.AccelLsb, all.GyroLsb, all.EulerLsb, all.TempLsb));
        Assert.Equal(16.0, Bno055Regs.MagLsb);
        Assert.Equal(16384.0, Bno055Regs.QuatLsb);
        Assert.Equal(100.0, Bno055Regs.FusionAccelLsb);
    }

    [Fact]
    public void CalibStat_FieldsAndRepack()
    {
        int n = 0;
        foreach (var c in Root.GetProperty("calib_stat").EnumerateArray())
        {
            byte value = (byte)c.GetProperty("value").GetInt32();
            var s = Bno055CalibStatus.Unpack(value);
            AssertFields($"0x{value:x2}", c.GetProperty("expect"), new Dictionary<string, long>
            {
                ["system"] = s.System,
                ["gyro"] = s.Gyro,
                ["accel"] = s.Accel,
                ["mag"] = s.Mag,
            });
            Assert.True(c.GetProperty("fully_calibrated").GetBoolean() == s.FullyCalibrated, $"0x{value:x2}");
            Assert.Equal(value, s.Pack());
            n++;
        }
        Assert.True(n > 0);
    }

    [Fact]
    public void CalibrationProfile_RoundTrip()
    {
        int n = 0;
        foreach (var c in Root.GetProperty("calibration_profile").EnumerateArray())
        {
            string name = c.GetProperty("name").GetString()!;
            string bytes = c.GetProperty("bytes").GetString()!;
            var p = Bno055CalibrationProfile.Unpack(Vectors.Hex(bytes));
            JsonElement e = c.GetProperty("expect");
            Assert.Equal(5, e.EnumerateObject().Count());
            AssertChannel($"{name}.accel_offset", e.GetProperty("accel_offset"), Arr(p.AccelOffset));
            AssertChannel($"{name}.mag_offset", e.GetProperty("mag_offset"), Arr(p.MagOffset));
            AssertChannel($"{name}.gyro_offset", e.GetProperty("gyro_offset"), Arr(p.GyroOffset));
            Assert.Equal(e.GetProperty("accel_radius").GetInt32(), p.AccelRadius);
            Assert.Equal(e.GetProperty("mag_radius").GetInt32(), p.MagRadius);
            Assert.True(bytes == Vectors.ToHex(p.Pack()), $"{name}: repack");
            n++;
        }
        Assert.True(n > 0);
        Assert.Throws<ArgumentException>(() => Bno055CalibrationProfile.Unpack(new byte[Bno055Regs.CalibProfileLen - 1]));
    }

    [Fact]
    public void AxisRemap_Placements()
    {
        var names = new List<string>();
        foreach (var c in Root.GetProperty("axis_remap").EnumerateArray())
        {
            string name = c.GetProperty("name").GetString()!;
            names.Add(name);
            var a = Bno055AxisRemap.Unpack((byte)c.GetProperty("config").GetInt32(), (byte)c.GetProperty("sign").GetInt32());
            JsonElement e = c.GetProperty("expect");
            Assert.Equal(6, e.EnumerateObject().Count());
            Assert.True(e.GetProperty("x").GetInt32() == (int)a.X, $"{name}: x");
            Assert.True(e.GetProperty("y").GetInt32() == (int)a.Y, $"{name}: y");
            Assert.True(e.GetProperty("z").GetInt32() == (int)a.Z, $"{name}: z");
            Assert.True(e.GetProperty("x_negative").GetBoolean() == a.XNegative, $"{name}: x_negative");
            Assert.True(e.GetProperty("y_negative").GetBoolean() == a.YNegative, $"{name}: y_negative");
            Assert.True(e.GetProperty("z_negative").GetBoolean() == a.ZNegative, $"{name}: z_negative");
            var (config, sign) = a.Pack();
            Assert.True(Ints(c.GetProperty("repack")).SequenceEqual(new int[] { config, sign }), $"{name}: repack");
            Assert.Equal(a, Bno055AxisRemap.Placement(name));
            Assert.Equal(a, Bno055AxisRemap.Placement(name.ToLowerInvariant()));
        }
        Assert.Equal(names, Bno055AxisRemap.Placements.Select(p => p.Name));
        Assert.Equal(new Bno055AxisRemap(), Bno055AxisRemap.Placement("P1"));
        Assert.Throws<ArgumentException>(() => Bno055AxisRemap.Placement("P8"));
    }

    [Fact]
    public void AxisRemap_InvalidRefused()
    {
        int n = 0;
        foreach (var c in Root.GetProperty("axis_remap_invalid").EnumerateArray())
        {
            var a = new Bno055AxisRemap(
                (Bno055Axis)c.GetProperty("x").GetInt32(),
                (Bno055Axis)c.GetProperty("y").GetInt32(),
                (Bno055Axis)c.GetProperty("z").GetInt32());
            Assert.Throws<ArgumentException>(() => a.Pack());
            n++;
        }
        Assert.True(n > 0);
    }

    [Fact]
    public void SensorConfig_Page1()
    {
        JsonElement sc = Root.GetProperty("sensor_config");
        foreach (var c in sc.GetProperty("accel").EnumerateArray())
        {
            byte value = (byte)c.GetProperty("value").GetInt32();
            var a = Bno055AccelConfig.Unpack(value);
            AssertFields($"accel 0x{value:x2}", c.GetProperty("expect"), new Dictionary<string, long>
            {
                ["range"] = a.Range, ["bandwidth"] = a.Bandwidth, ["power"] = a.Power,
            });
            Assert.Equal(value, a.Pack());
        }
        foreach (var c in sc.GetProperty("gyro").EnumerateArray())
        {
            string bytes = c.GetProperty("bytes").GetString()!;
            var g = Bno055GyroConfig.Unpack(Vectors.Hex(bytes));
            AssertFields($"gyro {bytes}", c.GetProperty("expect"), new Dictionary<string, long>
            {
                ["range"] = g.Range, ["bandwidth"] = g.Bandwidth, ["power"] = g.Power,
            });
            Assert.Equal(bytes, Vectors.ToHex(g.Pack()));
        }
        foreach (var c in sc.GetProperty("mag").EnumerateArray())
        {
            byte value = (byte)c.GetProperty("value").GetInt32();
            var m = Bno055MagConfig.Unpack(value);
            AssertFields($"mag 0x{value:x2}", c.GetProperty("expect"), new Dictionary<string, long>
            {
                ["rate"] = m.Rate, ["mode"] = m.Mode, ["power"] = m.Power,
            });
            // Bit 7 is not a field: the repack drops it.
            Assert.Equal(value & 0x7F, m.Pack());
        }
        Assert.Equal(Bno055AccelConfig.Default, Bno055AccelConfig.Unpack(0x0D));
        Assert.Equal(Bno055MagConfig.Default, Bno055MagConfig.Unpack(0x0B));
        Assert.Equal(Bno055GyroConfig.Default, Bno055GyroConfig.Unpack(new byte[] { 0x38, 0x00 }));
    }

    [Fact]
    public void Blocks_WindowDecode()
    {
        int n = 0;
        foreach (var c in Root.GetProperty("blocks").EnumerateArray())
        {
            string name = c.GetProperty("name").GetString()!;
            var b = Bno055Regs.DecodeBlock((byte)c.GetProperty("addr").GetInt32(), Vectors.Hex(c.GetProperty("data").GetString()!));
            JsonElement e = c.GetProperty("expect");
            Assert.Equal(9, e.EnumerateObject().Count());
            AssertChannel($"{name}.accel", e.GetProperty("accel"), Arr(b.Accel));
            AssertChannel($"{name}.mag", e.GetProperty("mag"), Arr(b.Mag));
            AssertChannel($"{name}.gyro", e.GetProperty("gyro"), Arr(b.Gyro));
            AssertChannel($"{name}.euler", e.GetProperty("euler"),
                b.Euler is { } eu ? new int[] { eu.Heading, eu.Roll, eu.Pitch } : null);
            AssertChannel($"{name}.quaternion", e.GetProperty("quaternion"),
                b.Quaternion is { } q ? new int[] { q.W, q.X, q.Y, q.Z } : null);
            AssertChannel($"{name}.linear_accel", e.GetProperty("linear_accel"), Arr(b.LinearAccel));
            AssertChannel($"{name}.gravity", e.GetProperty("gravity"), Arr(b.Gravity));
            AssertChannel($"{name}.temperature", e.GetProperty("temperature"),
                b.Temperature is { } t ? new int[] { t } : null);
            AssertChannel($"{name}.calib_stat", e.GetProperty("calib_stat"),
                b.CalibStat is { } cs ? new int[] { cs } : null);
            n++;
        }
        Assert.True(n > 0);
        Assert.Equal((0x08, 46), (Bno055Regs.FullBlockAddr, Bno055Regs.FullBlockLen));
        Assert.Equal((0x20, 8), (Bno055Regs.QuatBlockAddr, Bno055Regs.QuatBlockLen));
    }

    [Fact]
    public void Identity_FirmwareName()
    {
        Assert.Equal(SensorType.Bno055, IdentityParser.ParseSoftwareName("APP_BNO055_v0.12").SensorType);
        Assert.Equal(SensorType.Bno086, IdentityParser.ParseSoftwareName("APP_BNO086_v0.95").SensorType);
    }
}
