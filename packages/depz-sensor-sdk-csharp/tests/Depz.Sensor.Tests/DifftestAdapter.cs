using System.Collections;
using System.Globalization;
using System.Numerics;
using System.Text;
using System.Text.Json;
using Depz.Sensor.Bno055;
using Depz.Sensor.Bno086;
using Depz.Sensor.Protocol;
using Depz.Sensor.Tmf8829;
using Depz.Sensor.Transport;
using Depz.Sensor.Usb;
using Depz.Sensor.Vl53l4;
using Depz.Sensor.Vl53l7;
using Depz.Sensor.Vl53l8;
using Depz.Sensor.Vl53lx;

namespace Depz.Sensor.Tests;

/// <summary>
/// Differential-testing adapter for the C# SDK (contracts/16_DIFFTEST.md). A
/// long-lived process speaking JSON Lines on stdin/stdout:
/// <c>{"id": n, "op": "…", "in": {…}}</c> → <c>{"id": n, "out": …}</c> or
/// <c>{"id": n, "error": "invalid" | "unsupported"}</c>. <c>invalid</c> = the SDK
/// rejected the input (threw); <c>unsupported</c> = the SDK has no such function.
///
/// Lives in the test assembly (the vector tests' shapes, <see cref="VectorShapes"/>,
/// are its output shapes) and runs as its entry point:
/// <c>dotnet tests/Depz.Sensor.Tests/bin/Debug/net8.0/Depz.Sensor.Tests.dll --difftest</c>
/// (the driver uses <c>difftest-adapter.sh</c>, which builds first).
///
/// Integers: every parameter is converted to the API's own C# type with a
/// checked conversion. A value outside that type (−1 for a <c>ushort</c>, 2^32
/// for an <c>int</c>) is one no C# caller can pass at all — the compiler refuses
/// it — so it is answered <c>invalid</c> rather than squeezed into the type;
/// clamping or wrapping would compare a different input against the reference.
/// An <c>int</c>-typed enum (CrcType, SyncPinMode, …) takes any <c>int</c>, as a
/// C# cast does.
/// </summary>
public static class DifftestAdapter
{
    /// <summary>Ops this SDK declares it cannot do, with the reason (never a silent gap).</summary>
    internal static readonly Dictionary<string, string> Unsupported = new();

    // ── input coercion ───────────────────────────────────────────────────────

    private sealed class NotRepresentable : ArgumentException
    {
        public NotRepresentable(JsonElement v, string type) : base($"{v.GetRawText()} is not representable as {type}") { }
    }

    private static JsonElement P(JsonElement o, string key) =>
        o.ValueKind == JsonValueKind.Object && o.TryGetProperty(key, out var v)
            ? v : throw new ArgumentException($"missing {key}");

    private static BigInteger Big(JsonElement e, string type)
    {
        if (e.ValueKind != JsonValueKind.Number)
            throw new ArgumentException($"not a number: {e.GetRawText()}");
        string raw = e.GetRawText();
        if (raw.IndexOfAny(new[] { '.', 'e', 'E' }) < 0)
            return BigInteger.Parse(raw, CultureInfo.InvariantCulture);
        double d = e.GetDouble();
        if (double.IsFinite(d) && Math.Floor(d) == d)
            return new BigInteger(d);
        throw new NotRepresentable(e, type);
    }

    private static BigInteger In(JsonElement e, BigInteger min, BigInteger max, string type)
    {
        var v = Big(e, type);
        return v < min || v > max ? throw new NotRepresentable(e, type) : v;
    }

    private static int I32(JsonElement e) => (int)In(e, int.MinValue, int.MaxValue, "int");
    private static long I64(JsonElement e) => (long)In(e, long.MinValue, long.MaxValue, "long");
    private static byte U8(JsonElement e) => (byte)In(e, 0, byte.MaxValue, "byte");
    private static ushort U16(JsonElement e) => (ushort)In(e, 0, ushort.MaxValue, "ushort");
    private static uint U32(JsonElement e) => (uint)In(e, 0, uint.MaxValue, "uint");
    private static ulong U64(JsonElement e) => (ulong)In(e, 0, ulong.MaxValue, "ulong");
    private static int? NI32(JsonElement e) => e.ValueKind == JsonValueKind.Null ? null : I32(e);

    private static double D(JsonElement e) =>
        e.ValueKind == JsonValueKind.Number ? e.GetDouble() : throw new ArgumentException("not a number");

    private static bool B(JsonElement e) => e.ValueKind switch
    {
        JsonValueKind.True => true,
        JsonValueKind.False => false,
        _ => throw new ArgumentException("not a boolean"),
    };

    private static string? S(JsonElement e) => e.ValueKind switch
    {
        JsonValueKind.Null => null,
        JsonValueKind.String => e.GetString(),
        _ => throw new ArgumentException("not a string"),
    };

    private static string Str(JsonElement e) => S(e) ?? throw new ArgumentException("null string");

    /// <summary>Lowercase hex string → bytes; anything else is a rejected input.</summary>
    private static byte[] H(JsonElement e)
    {
        string s = Str(e);
        if ((s.Length & 1) != 0 || s.Any(c => !(c is >= '0' and <= '9' or >= 'a' and <= 'f')))
            throw new ArgumentException("not a hex string");
        return Vectors.Hex(s);
    }

    private static List<JsonElement> L(JsonElement e) =>
        e.ValueKind == JsonValueKind.Array ? e.EnumerateArray().ToList() : throw new ArgumentException("not a list");

    private static Dictionary<string, object?> Payload(byte[] p) => new() { ["payload"] = Vectors.ToHex(p) };

    private static Dictionary<string, object?> O(params (string Key, object? Value)[] kv)
    {
        var d = new Dictionary<string, object?>();
        foreach (var (k, v) in kv)
            d[k] = v;
        return d;
    }

    // ── ops ──────────────────────────────────────────────────────────────────

    private static readonly Dictionary<string, Func<JsonElement, object?>> Ops = new()
    {
        ["crc"] = i =>
        {
            byte[] raw = H(P(i, "input"));
            return O(("crc8_maxim", Crc.Crc8Maxim(raw)), ("crc16_modbus", Crc.Crc16Modbus(raw)),
                ("crc32_iso_hdlc", Crc.Crc32IsoHdlc(raw)), ("crc16_ccitt_false", Crc.Crc16CcittFalse(raw)));
        },
        ["framing_encode"] = i => O(("frame", Vectors.ToHex(Framing.BuildPacket(
            I32(P(i, "cmd")), H(P(i, "payload")), I64(P(i, "seq")), (CrcType)I32(P(i, "crc_type")))))),
        ["framing_decode"] = i => VectorShapes.FramingCollect(new PacketParser(),
            i.TryGetProperty("chunks", out var ch) ? L(ch).Select(H).ToList() : new List<byte[]> { H(P(i, "stream")) }),
        ["identity"] = i =>
        {
            var id = IdentityParser.ParseSoftwareName(Common.StripDeviceString(H(P(i, "raw"))));
            return O(("mode", id.Mode), ("sensor_type", id.SensorType?.ToString().ToLowerInvariant()),
                ("software_name", id.SoftwareName), ("version", id.Version));
        },
        ["usb_identity"] = i => O(("known", UsbIds.IsKnownDepzUsb(NI32(P(i, "vid")), NI32(P(i, "pid")))),
            ("model", UsbIds.UsbModelHint(NI32(P(i, "vid")), NI32(P(i, "pid"))))),
        ["usb_serial_ordering"] = i => O(("order", UsbIds.SerialOrdering(
                L(P(i, "ports")).Select(p => new UsbIds.PortCandidate(Str(P(p, "port")), S(P(p, "serial")))).ToList())
            .Select(p => p.Port).ToList())),
        ["probe_resolve"] = i => O(("class", Discovery.ResolveSensorClass(S(P(i, "software_name"))!,
            S(P(i, "device_name")), NI32(P(i, "usb_vid")), NI32(P(i, "usb_pid"))))),
        ["common_encode"] = i => Str(P(i, "kind")) switch
        {
            "sync_time_request" => Payload(Common.PackSyncTime(U64(P(i, "pc_timestamp_us")))),
            // No codec function: the device layer writes the byte of a CrcType
            // argument, so the op checks the enum like the type does.
            "set_payload_crc_type" => Enum.IsDefined((CrcType)I32(P(i, "crc_type")))
                ? Payload(new[] { (byte)I32(P(i, "crc_type")) })
                : throw new ArgumentException("crc_type not a CrcType"),
            "sync_pin_config" => Payload(new SyncPinConfig(I32(P(i, "pin")), (SyncPinMode)I32(P(i, "mode")),
                (SyncPinPolarity)I32(P(i, "polarity"))).Pack()),
            var k => throw new ArgumentException(k),
        },
        ["common_decode"] = i =>
        {
            byte[] p = H(P(i, "payload"));
            switch (I32(P(i, "report")))
            {
                case 0x80:
                    var s = StatusReport.Unpack(p);
                    return O(("cmd", s.Cmd), ("status", s.Status));
                case 0x81:
                    var t = TextReport.Unpack(p);
                    return O(("cmd", t.Cmd), ("text", t.Text));
                case 0x83:
                    var r = TemperatureReport.Unpack(p);
                    return O(("timestamp_us", r.TimestampUs), ("raw_decidegrees", r.RawDecidegrees));
                case 0x84:
                    var e = SequenceErrorReport.Unpack(p);
                    return O(("expected_seq", e.ExpectedSeq), ("received_seq", e.ReceivedSeq));
                default:
                    throw new ArgumentException("unknown report");
            }
        },
        ["sync_time_math"] = i =>
        {
            var (offset, rtt) = Common.SyncTimeOffsetRtt(I64(P(i, "t1")), I64(P(i, "t2")), I64(P(i, "t3")), I64(P(i, "t4")));
            return O(("offset_us", offset), ("rtt_us", rtt));
        },
        ["sr04_encode"] = i => Str(P(i, "kind")) switch
        {
            "set_sample_period" => Payload(Sr04.PackSamplePeriod(U32(P(i, "period_us")))),
            "set_echo_decay" => Payload(Sr04.PackEchoDecay(U16(P(i, "decay_us")))),
            var k => throw new ArgumentException(k),
        },
        ["sr04_decode"] = i =>
        {
            byte[] p = H(P(i, "payload"));
            switch (I32(P(i, "report")))
            {
                case 0x91:
                    var d = Sr04Data.Unpack(p);
                    return O(("source_cmd", d.SourceCmd), ("timestamp_us", d.TimestampUs), ("echo_time_us", d.EchoTimeUs));
                case 0x92:
                    return O(("period_us", Sr04.UnpackSamplePeriod(p)));
                case 0x93:
                    return O(("decay_us", Sr04.UnpackEchoDecay(p)));
                default:
                    throw new ArgumentException("unknown report");
            }
        },
        ["fwdepz_parse"] = i =>
        {
            var img = FwDepz.FwDepzImage.Parse(H(P(i, "file")));
            return O(("load_addr", img.LoadAddr), ("fw_size", img.FwSize), ("fw_crc32", img.FwCrc32),
                ("cur_sec", img.CurSec), ("tot_sec", img.TotSec), ("payload_crc_ok", img.PayloadCrcOk));
        },

        // ── BNO055 ──
        ["bno055_encode"] = i => Str(P(i, "kind")) switch
        {
            "read_reg" => Payload(Bno055Wire.PackReadReg(U8(P(i, "addr")), U8(P(i, "len")))),
            "write_reg" => Payload(Bno055Wire.PackWriteReg(U8(P(i, "addr")), H(P(i, "data")))),
            "start_stream" => Payload(Bno055Wire.PackStartStream((Bno055Trigger)U8(P(i, "trigger")), U8(P(i, "addr")),
                U8(P(i, "len")), U16(P(i, "period_ms")))),
            "reset" or "stop_stream" or "get_info" => Payload(Array.Empty<byte>()),
            var k => throw new ArgumentException(k),
        },
        ["bno055_decode"] = i => VectorShapes.Bno055Decode(I32(P(i, "report")), H(P(i, "payload"))),
        ["bno055_units"] = i =>
        {
            var u = Bno055Units.Unpack(U8(P(i, "unit_sel")));
            return O(("accel_mg", u.AccelMg), ("gyro_rps", u.GyroRps), ("euler_rad", u.EulerRad), ("temp_f", u.TempF),
                ("android", u.Android), ("repack", u.Pack()));
        },
        ["bno055_calib_stat"] = i =>
        {
            var s = Bno055CalibStatus.Unpack(U8(P(i, "value")));
            return O(("system", s.System), ("gyro", s.Gyro), ("accel", s.Accel), ("mag", s.Mag),
                ("fully_calibrated", s.FullyCalibrated), ("repack", s.Pack()));
        },
        ["bno055_calibration_profile"] = i =>
        {
            var p = Bno055CalibrationProfile.Unpack(H(P(i, "bytes")));
            var o = VectorShapes.Bno055Profile(p);
            o["repack"] = Vectors.ToHex(p.Pack());
            return o;
        },
        ["bno055_axis_remap"] = i =>
        {
            var a = Bno055AxisRemap.Unpack(U8(P(i, "config")), U8(P(i, "sign")));
            var (config, sign) = a.Pack();
            return O(("x", (int)a.X), ("y", (int)a.Y), ("z", (int)a.Z), ("x_negative", a.XNegative),
                ("y_negative", a.YNegative), ("z_negative", a.ZNegative), ("repack", new int[] { config, sign }));
        },
        ["bno055_axis_remap_pack"] = i =>
        {
            var (config, sign) = new Bno055AxisRemap((Bno055Axis)U8(P(i, "x")), (Bno055Axis)U8(P(i, "y")),
                (Bno055Axis)U8(P(i, "z")), B(P(i, "x_negative")), B(P(i, "y_negative")), B(P(i, "z_negative"))).Pack();
            return O(("repack", new int[] { config, sign }));
        },
        ["bno055_sensor_config"] = i =>
        {
            switch (Str(P(i, "sensor")))
            {
                case "accel":
                    var a = Bno055AccelConfig.Unpack(U8(P(i, "value")));
                    return O(("range", a.Range), ("bandwidth", a.Bandwidth), ("power", a.Power), ("repack", a.Pack()));
                case "gyro":
                    var g = Bno055GyroConfig.Unpack(H(P(i, "bytes")));
                    return O(("range", g.Range), ("bandwidth", g.Bandwidth), ("power", g.Power),
                        ("repack", Vectors.ToHex(g.Pack())));
                case "mag":
                    var m = Bno055MagConfig.Unpack(U8(P(i, "value")));
                    return O(("rate", m.Rate), ("mode", m.Mode), ("power", m.Power), ("repack", m.Pack()));
                default:
                    throw new ArgumentException("unknown sensor");
            }
        },
        ["bno055_block"] = i => VectorShapes.Bno055Block(U8(P(i, "addr")), H(P(i, "data"))),

        // ── BNO086 ──
        ["bno086_shtp_header_pack"] = i => O(("bytes", Vectors.ToHex(new ShtpHeader(I32(P(i, "length")),
            I32(P(i, "channel")), I32(P(i, "seq")), B(P(i, "continuation"))).Pack()))),
        ["bno086_shtp_header_unpack"] = i =>
        {
            var h = ShtpHeader.Unpack(H(P(i, "bytes")));
            return O(("length", h.Length), ("channel", h.Channel), ("seq", h.Seq), ("continuation", h.Continuation));
        },
        ["bno086_shtp_tx_seq"] = i =>
        {
            var layer = new ShtpLayer();
            return O(("frames", L(P(i, "packets"))
                .Select(p => Vectors.ToHex(layer.NextFrame(I32(P(p, "channel")), H(P(p, "payload"))))).ToList()));
        },
        ["bno086_shtp_reassembly"] = i =>
        {
            var frames = L(P(i, "frames"));
            var caps = L(P(i, "captures_us"));
            if (frames.Count != caps.Count)
                throw new ArgumentException("frames / captures_us length");
            var layer = new ShtpLayer();
            var cargos = new List<object?>();
            for (int k = 0; k < frames.Count; k++)
                if (layer.Feed(H(frames[k]), I64(caps[k])) is { } c)
                    cargos.Add(VectorShapes.Bno086Cargo(c));
            return O(("cargos", cargos), ("discarded", layer.Discarded));
        },
        ["bno086_control_encode"] = i => Str(P(i, "kind")) switch
        {
            "set_feature" => Payload(Sh2Control.SetFeature(I32(P(i, "sensor_id")), U32(P(i, "interval_us")),
                U32(P(i, "batch_us")), I32(P(i, "sensitivity")), I32(P(i, "flags")), U32(P(i, "cfg_word")))),
            "get_feature_request" => Payload(Sh2Control.GetFeature(I32(P(i, "sensor_id")))),
            "product_id_request" => Payload(Sh2Control.ProductId()),
            "command_request" => Payload(Sh2Control.Command(I32(P(i, "seq")), I32(P(i, "command")), H(P(i, "params")))),
            "frs_read_request" => Payload(Sh2Control.FrsRead(I32(P(i, "frs_type")), I32(P(i, "offset_words")),
                I32(P(i, "block_words")))),
            "frs_write_request" => Payload(Sh2Control.FrsWrite(I32(P(i, "frs_type")), I32(P(i, "length_words")))),
            "frs_write_data" => Payload(Sh2Control.FrsWriteDataReport(I32(P(i, "offset_words")),
                L(P(i, "words")).Select(U32).ToList())),
            var k => throw new ArgumentException(k),
        },
        ["bno086_input_cargo"] = i => O(("reports", Sh2Reports.ParseInputCargo(H(P(i, "cargo")),
            I64(P(i, "capture_timestamp_us"))).Select(VectorShapes.Bno086Report).ToList())),
        ["bno086_record"] = i => O(("records", Sh2Reports.ParseInputCargo(H(P(i, "cargo")),
            I64(P(i, "capture_timestamp_us"))).Select(r => (object?)Dataset.DatasetRecords.Bno086(r)).ToList())),
        ["bno086_gyro_rv"] = i => Sh2Reports.ParseGyroRvCargo(H(P(i, "cargo")), I64(P(i, "capture_timestamp_us"))) is { } r
            ? VectorShapes.Bno086Report(r) : null,

        // ── TMF8829 ──
        ["tmf8829_encode"] = i => (Tmf8829Cmd)I32(P(i, "cmd")) switch
        {
            Tmf8829Cmd.ReadReg => Payload(Tmf8829Wire.PackReadReg(I32(P(i, "addr")), I32(P(i, "len")))),
            Tmf8829Cmd.WriteReg => Payload(Tmf8829Wire.PackWriteReg(I32(P(i, "addr")), H(P(i, "data")))),
            Tmf8829Cmd.StartStream => Payload(Tmf8829Wire.PackStartStream(I32(P(i, "int_mask")), I32(P(i, "flags")))),
            _ => Payload(Array.Empty<byte>()),
        },
        ["tmf8829_decode"] = i => VectorShapes.Tmf8829Decode(I32(P(i, "report")), H(P(i, "payload"))),
        ["tmf8829_reassembly"] = i => VectorShapes.Tmf8829Reassembly(L(P(i, "chunks")).Select(H).ToList()),
        ["tmf8829_frame"] = i => VectorShapes.Tmf8829Frame(H(P(i, "raw"))),
        ["tmf8829_measurements"] = i =>
        {
            var b = new Tmf8829MeasurementBuilder();
            var got = new List<object?>();
            foreach (var f in L(P(i, "frames")))
                if (b.Add(H(P(f, "raw")), I64(P(f, "timestamp_us")), I64(P(f, "edge_us"))) is { } m)
                    got.Add(VectorShapes.Tmf8829Meas(m, b));
            return O(("measurements", got));
        },
        ["tmf8829_config_get"] = i => O(("value", Tmf8829Config.CfgGet(H(P(i, "page")), S(P(i, "field"))!))),
        ["tmf8829_config_set"] = i =>
        {
            byte[] pg = H(P(i, "page"));
            Tmf8829Config.CfgSet(pg, S(P(i, "field"))!, I32(P(i, "value")));
            return O(("page", Vectors.ToHex(pg)));
        },
        ["tmf8829_frame_time"] = i =>
        {
            var clock = new Tmf8829SensorClock();
            var timer = new Tmf8829CaptureTimer(clock);
            var times = new List<object?>();
            foreach (var r in L(P(i, "rows")))
            {
                int fid = U8(P(r, "id")); // the frame-id byte, split as the frame parser does
                int kind = fid & 0xF0;
                if (kind == 0x30)
                    continue;
                uint tick = U32(P(r, "systick"));
                int flags = clock.Add(I64(P(r, "ts")), tick);
                if (kind == 0x20)
                {
                    timer.Histograms();
                    continue;
                }
                var t = timer.Results(fid & 0x0F, tick, U32(P(r, "t0")), U32(P(r, "t1")), I64(P(r, "ts")),
                    I64(P(r, "edge_us")), flags);
                times.Add(O(("timestamp_us", t.TimestampUs), ("t1_len_us", t.T1LenUs), ("flags", t.Flags),
                    ("ratio_q16", clock.RatioQ16)));
            }
            return O(("times", times));
        },

        // ── VL53L4CD ──
        ["vl53l4_encode"] = i => Str(P(i, "kind")) switch
        {
            "read_reg" => Payload(Vl53l4Wire.PackReadReg(U16(P(i, "addr")), U16(P(i, "len")))),
            "write_reg" => Payload(Vl53l4Wire.PackWriteReg(U16(P(i, "addr")), H(P(i, "data")))),
            "xshut" => Payload(Vl53l4Wire.PackXshut(U8(P(i, "action")))),
            "start_stream" => Payload(Vl53l4Wire.PackStartStream(U16(P(i, "addr")), U16(P(i, "len")), U8(P(i, "flags")))),
            "set_i2c_speed" => Payload(Vl53l4Wire.PackSetI2cSpeed(U16(P(i, "khz")))),
            var k => throw new ArgumentException(k),
        },
        ["vl53l4_decode"] = i => VectorShapes.Vl53l4Decode(I32(P(i, "report")), H(P(i, "payload"))),
        ["vl53l4_result_block"] = i => VectorShapes.Vl53l4Result(Vl53l4Uld.ParseResultBlock(H(P(i, "raw")))),
        ["vl53l4_timing_encode"] = i =>
        {
            var r = Vl53l4Uld.RangeTimingRegisters(I32(P(i, "timing_budget_ms")), I64(P(i, "inter_measurement_ms")),
                I32(P(i, "osc_frequency")), I32(P(i, "clock_pll")));
            return O(("range_config_a", r.RangeConfigA), ("range_config_b", r.RangeConfigB),
                ("intermeasurement_raw", r.IntermeasurementRaw));
        },
        ["vl53l4_timing_decode"] = i =>
        {
            var r = Vl53l4Uld.DecodeRangeTiming(U32(P(i, "intermeasurement_raw")), I32(P(i, "clock_pll")),
                I32(P(i, "osc_frequency")), I32(P(i, "range_config_a")));
            return O(("timing_budget_ms", r.TimingBudgetMs), ("inter_measurement_ms", r.InterMeasurementMs));
        },
        ["vl53l4_tuning_encode"] = i => Str(P(i, "kind")) switch
        {
            "offset" => O(("raw", Vl53l4Uld.OffsetRaw(I32(P(i, "value"))))),
            "xtalk" => O(("raw", Vl53l4Uld.XtalkRaw(I32(P(i, "value"))))),
            "signal_threshold" => O(("raw", Vl53l4Uld.SignalThresholdRaw(I32(P(i, "value"))))),
            "sigma_threshold" => O(("raw", Vl53l4Uld.SigmaThresholdRaw(I32(P(i, "value"))))),
            var k => throw new ArgumentException(k),
        },
        ["vl53l4_tuning_decode"] = i => Str(P(i, "kind")) switch
        {
            "offset" => O(("value", Vl53l4Uld.DecodeOffset(U16(P(i, "raw"))))),
            "xtalk" => O(("value", Vl53l4Uld.DecodeXtalk(U16(P(i, "raw"))))),
            "signal_threshold" => O(("value", Vl53l4Uld.DecodeSignalThreshold(I32(P(i, "raw"))))),
            "sigma_threshold" => O(("value", Vl53l4Uld.DecodeSigmaThreshold(I32(P(i, "raw"))))),
            var k => throw new ArgumentException(k),
        },

        // ── VL53L5CX / L7 ──
        ["vl53l7_encode"] = i => Str(P(i, "kind")) switch
        {
            "read_reg" => Payload(Vl53l7Wire.PackReadReg(U16(P(i, "addr")), I32(P(i, "len")))),
            "write_reg" => Payload(Vl53l7Wire.PackWriteReg(U16(P(i, "addr")), H(P(i, "data")))),
            "pin_ctrl" => Payload(Vl53l7Wire.PackPinCtrl(U8(P(i, "action")))),
            "set_i2c_speed" => Payload(Vl53l7Wire.PackSetI2cSpeed(U16(P(i, "khz")))),
            var k => throw new ArgumentException(k),
        },
        ["vl53l7_decode"] = i => I32(P(i, "report")) == 0x92
            ? VectorShapes.Vl53l7InfoShape(H(P(i, "payload"))) : throw new ArgumentException("unknown report"),
        ["vl53l7_model"] = i => O(("class",
            Vl53l7Discovery.ResolveModel(S(P(i, "usb_model")), S(P(i, "device_name"))).ToString().ToLowerInvariant())),

        // ── VL53L 1D family ──
        ["vl53lx_encode"] = i => Str(P(i, "kind")) switch
        {
            "set_addr_width" => Payload(Vl53lxWire.PackSetAddrWidth(I32(P(i, "width")))),
            "read_reg" => Payload(Vl53lxWire.PackReadReg(U16(P(i, "addr")), U16(P(i, "len")))),
            "write_reg" => Payload(Vl53lxWire.PackWriteReg(U16(P(i, "addr")), H(P(i, "data")))),
            "xshut" => Payload(Vl53lxWire.PackXshut(U8(P(i, "action")))),
            "set_i2c_speed" => Payload(Vl53lxWire.PackSetI2cSpeed(U16(P(i, "khz")))),
            "start_stream" => Payload(Vl53lxWire.PackStartStream(U16(P(i, "addr")), U16(P(i, "len")),
                L(P(i, "clear")).Select(s =>
                {
                    var pair = L(s);
                    return pair.Count == 2 ? new Vl53lxClearStep(U16(pair[0]), U8(pair[1]))
                        : throw new ArgumentException("clear step is [addr, value]");
                }).ToList(), U8(P(i, "flags")))),
            var k => throw new ArgumentException(k),
        },
        ["vl53lx_decode"] = i => I32(P(i, "report")) == 0x92
            ? VectorShapes.Vl53lxInfoShape(H(P(i, "payload"))) : throw new ArgumentException("unknown report"),
        ["vl53lx_model"] = i => O(
            ("class", Vl53lxProducts.ResolveClass(S(P(i, "usb_model")), S(P(i, "device_name"))).ToString()),
            ("product", Vl53lxProducts.ProductFromBoardName(S(P(i, "device_name"))))),
        ["vl53lx_die_block"] = i => VectorShapes.Vl53lxDie(
            Vl53lxDecode.DecodeDieBlock(H(P(i, "raw")), Vl53lxDecode.ParseDieVariant(Str(P(i, "variant"))))),
        ["vl53lx_l0x_raw"] = i => VectorShapes.Vl53lxL0x(Vl53lxDecode.DecodeL0xRaw(H(P(i, "raw")))),
        ["vl53lx_histogram_raw"] = i => VectorShapes.Vl53lxHistogram(Vl53lxDecode.DecodeHistogramRaw(H(P(i, "raw")))),

        // ── VL53L8 advanced ──
        // The pure DCI codecs the driver methods call (Vl53l8Driver writes them to the sensor);
        // set_xtalk_margin's own check (0..10000) lives in XtalkMarginRaw.
        ["vl53l8_motion_config"] = i => O(("pack", Vectors.ToHex(MotionConfig.InitDefault(I32(P(i, "resolution"))).Pack()))),
        ["vl53l8_detection_thresholds"] = i => O(
            ("start_block", Vectors.ToHex(Vl53l8Advanced.PackDetectionThresholds(L(P(i, "thresholds")).Select(t =>
                new Vl53l8Advanced.DetectionThreshold(I32(P(t, "low_thresh")), I32(P(t, "high_thresh")),
                    I32(P(t, "measurement")), I32(P(t, "type")), I32(P(t, "zone_num")), I32(P(t, "operation"))))
                .ToList()))),
            ("valid_status", Vectors.ToHex(Vl53l8Advanced.ThresholdValidStatus()))),
        ["vl53l8_xtalk_margin"] = i => O(("raw", Vl53l8Advanced.XtalkMarginRaw(D(P(i, "kcps"))))),
        ["vl53l8_cnh_decode"] = i =>
        {
            var c = P(i, "config");
            var setup = new Vl53l8CnhSetup();
            setup.InitConfig(I32(P(c, "start_bin")), I32(P(c, "num_bins")), I32(P(c, "sub_sample")));
            var m = L(P(c, "agg_map")).Select(I32).ToArray();
            if (m.Length != 6)
                throw new ArgumentException("agg_map: start_x, start_y, merge_x, merge_y, cols, rows");
            setup.CreateAggMap(I32(P(c, "resolution")), m[0], m[1], m[2], m[3], m[4], m[5]);
            var decoded = Vl53l8Cnh.DecodeHistogram(setup.ToDecodeConfig(), H(P(i, "cnh_raw")));
            return O(("aggregates", decoded.Aggregates.Select(a =>
                O(("hist_raw", a.HistRaw), ("hist_scaler", a.HistScaler.Select(x => (int)x).ToList()))).ToList()));
        },
    };

    // ── canonical JSON (contract 15 §3 + 16 §3) ──────────────────────────────

    private static void Canon(object? v, StringBuilder sb)
    {
        switch (v)
        {
            case null:
                sb.Append("null");
                break;
            case bool b:
                sb.Append(b ? "true" : "false");
                break;
            case string s:
                Str(s, sb);
                break;
            case sbyte or byte or short or ushort or int or uint or long or ulong or BigInteger:
                sb.Append(Convert.ToString(v, CultureInfo.InvariantCulture));
                break;
            case double or float:
                double d = Convert.ToDouble(v, CultureInfo.InvariantCulture);
                if (double.IsNaN(d))
                    sb.Append("\"NaN\"");
                else if (double.IsInfinity(d))
                    sb.Append(d > 0 ? "\"Infinity\"" : "\"-Infinity\"");
                else
                    sb.Append(d.ToString("R", CultureInfo.InvariantCulture));
                break;
            case byte[] bytes:
                Str(Vectors.ToHex(bytes), sb);
                break;
            case IEnumerable<KeyValuePair<string, object?>> map:
                sb.Append('{');
                bool firstKey = true;
                foreach (var (k, x) in map)
                {
                    if (!firstKey)
                        sb.Append(',');
                    firstKey = false;
                    Str(k, sb);
                    sb.Append(':');
                    Canon(x, sb);
                }
                sb.Append('}');
                break;
            case IEnumerable list:
                sb.Append('[');
                bool first = true;
                foreach (var x in list)
                {
                    if (!first)
                        sb.Append(',');
                    first = false;
                    Canon(x, sb);
                }
                sb.Append(']');
                break;
            default:
                throw new InvalidOperationException($"cannot serialize {v.GetType()}");
        }
    }

    /// <summary>JSON string, ASCII-only (everything else as a UTF-16 \u escape).</summary>
    private static void Str(string s, StringBuilder sb)
    {
        sb.Append('"');
        foreach (char c in s)
        {
            if (c is '"' or '\\')
                sb.Append('\\').Append(c);
            else if (c < 0x20 || c > 0x7e)
                sb.Append("\\u").Append(((int)c).ToString("x4", CultureInfo.InvariantCulture));
            else
                sb.Append(c);
        }
        sb.Append('"');
    }

    /// <summary>One request line → its response line (also the input_rules vector tests' runner).</summary>
    internal static string Handle(string line)
    {
        using var doc = JsonDocument.Parse(line);
        var req = doc.RootElement;
        var sb = new StringBuilder("{\"id\":");
        sb.Append(req.GetProperty("id").GetRawText());
        string op = req.TryGetProperty("op", out var o) && o.ValueKind == JsonValueKind.String ? o.GetString()! : "";
        if (Unsupported.TryGetValue(op, out var why) || !Ops.TryGetValue(op, out var fn))
        {
            sb.Append(",\"error\":\"unsupported\",\"detail\":");
            Str(why ?? "unknown op", sb);
            return sb.Append('}').ToString();
        }
        var outSb = new StringBuilder();
        try
        {
            Canon(fn(req.GetProperty("in")), outSb);
        }
        catch (Exception e)
        {
            // every rejection the SDK can raise (ArgumentException, its own
            // exception types, index/overflow faults on a malformed input) is `invalid`
            string detail = $"{e.GetType().Name}: {e.Message}";
            sb.Append(",\"error\":\"invalid\",\"detail\":");
            Str(detail.Length > 300 ? detail[..300] : detail, sb);
            return sb.Append('}').ToString();
        }
        return sb.Append(",\"out\":").Append(outSb).Append('}').ToString();
    }

    /// <summary>
    /// Entry point of the test assembly (GenerateProgramFile is off in the
    /// csproj; xunit / dotnet test never call it). <c>--difftest</c> runs the adapter.
    /// </summary>
    public static int Main(string[] args)
    {
        if (!args.Contains("--difftest"))
        {
            Console.Error.WriteLine("usage: Depz.Sensor.Tests --difftest   (the contract 16 adapter; tests run via dotnet test)");
            return 2;
        }
        using var stdin = new StreamReader(Console.OpenStandardInput(), new UTF8Encoding(false));
        using var stdout = new StreamWriter(Console.OpenStandardOutput(), new UTF8Encoding(false)) { AutoFlush = false };
        string? line;
        while ((line = stdin.ReadLine()) is not null)
        {
            if (string.IsNullOrWhiteSpace(line))
                continue;
            stdout.Write(Handle(line));
            stdout.Write('\n');
            stdout.Flush();
        }
        return 0;
    }
}
