using System.Security.Cryptography;
using Depz.Sensor.Bno055;
using Depz.Sensor.Bno086;
using Depz.Sensor.Tmf8829;
using Depz.Sensor.Transport;
using Depz.Sensor.Vl53l4;
using Depz.Sensor.Vl53l7;
using Depz.Sensor.Vl53l8;
using Depz.Sensor.Vl53lx;

namespace Depz.Sensor.Tests;

/// <summary>
/// SDK results → golden-vector shapes: the JSON each vector group writes under
/// <c>expect</c>, as a field → value dictionary. Shared by the vector tests
/// (<see cref="Tmf8829VectorTests"/>) and the differential-testing adapter
/// (<see cref="DifftestAdapter"/>, contracts/16_DIFFTEST.md), so the adapter
/// cannot drift from what the vector tests accept; the TS reference's twin is
/// test/golden/shapes.ts.
///
/// Values: integers in the SDK's own width (u64 stays <c>ulong</c>), bool,
/// string (bytes already lowercase hex), null, <c>long[]</c> / <c>int[]</c> lists,
/// nested dictionaries and lists.
/// </summary>
public static class VectorShapes
{
    public static string Sha256(ReadOnlySpan<byte> data) => Vectors.ToHex(SHA256.HashData(data));

    public static long[]? Flat<T>(T[]? a) where T : struct => a?.Select(x => Convert.ToInt64(x)).ToArray();

    // ── framing (contract 01 §5) ─────────────────────────────────────────────

    /// <summary>Feed <paramref name="chunks"/> in order to one parser; the framing_decode <c>expect</c> shape.</summary>
    public static Dictionary<string, object?> FramingCollect(PacketParser parser, IEnumerable<byte[]> chunks)
    {
        var events = new List<object?>();
        var trash = new System.Text.StringBuilder();
        foreach (var chunk in chunks)
            foreach (var e in parser.Feed(chunk))
                switch (e)
                {
                    case Packet p:
                        events.Add(new Dictionary<string, object?>
                        {
                            ["type"] = "packet", ["cmd"] = p.Cmd, ["seq"] = p.Seq, ["payload"] = Vectors.ToHex(p.Payload),
                        });
                        break;
                    case CrcError ce:
                        events.Add(new Dictionary<string, object?> { ["type"] = "crc_error", ["cmd"] = ce.Cmd, ["seq"] = ce.Seq });
                        break;
                    case Trash t:
                        trash.Append(Vectors.ToHex(t.Data));
                        break;
                }
        return new()
        {
            ["events"] = events, ["trash"] = trash.ToString(), ["residue"] = Vectors.ToHex(parser.Residue()),
            ["header_errors"] = parser.HeaderErrors,
        };
    }

    // ── BNO055 (contract 13) ─────────────────────────────────────────────────

    public static Dictionary<string, object?> Bno055Decode(int report, byte[] raw)
    {
        switch ((Bno055Rpt)report)
        {
            case Bno055Rpt.Info:
                var i = Bno055Info.Unpack(raw);
                return new()
                {
                    ["i2c_addr"] = i.I2cAddr, ["chip_id"] = i.ChipId, ["acc_id"] = i.AccId, ["mag_id"] = i.MagId,
                    ["gyr_id"] = i.GyrId, ["sw_rev"] = i.SwRev, ["bl_rev"] = i.BlRev, ["initialized"] = i.Initialized,
                    ["int_level"] = i.IntLevel, ["int_edges"] = i.IntEdges, ["read_min_us"] = i.ReadMinUs,
                    ["read_max_us"] = i.ReadMaxUs, ["read_avg_us"] = i.ReadAvgUs, ["tx_dropped"] = i.TxDropped,
                    ["i2c_errors"] = i.I2cErrors, ["slots_skipped"] = i.SlotsSkipped,
                    ["bus_recoveries"] = i.BusRecoveries, ["last_i2c_error"] = i.LastI2cError,
                    ["sensor_resets"] = i.SensorResets, ["loop_max_us"] = i.LoopMaxUs,
                };
            case Bno055Rpt.RegData:
                var r = Bno055RegData.Unpack(raw);
                return new() { ["cmd"] = r.Cmd, ["timestamp_us"] = r.TimestampUs, ["data"] = Vectors.ToHex(r.Data) };
            case Bno055Rpt.Stream:
                var s = Bno055StreamData.Unpack(raw);
                return new()
                {
                    ["timestamp_us"] = s.TimestampUs, ["addr"] = s.Addr, ["len"] = s.Length, ["data"] = Vectors.ToHex(s.Data),
                };
            default:
                throw new ArgumentException($"unknown BNO055 report {report}");
        }
    }

    private static int[]? Vec(Bno055Vec3? v) => v is { } x ? new int[] { x.X, x.Y, x.Z } : null;

    public static Dictionary<string, object?> Bno055Block(byte addr, byte[] data)
    {
        var b = Bno055Regs.DecodeBlock(addr, data);
        return new()
        {
            ["accel"] = Vec(b.Accel), ["mag"] = Vec(b.Mag), ["gyro"] = Vec(b.Gyro),
            ["euler"] = b.Euler is { } e ? new int[] { e.Heading, e.Roll, e.Pitch } : null,
            ["quaternion"] = b.Quaternion is { } q ? new int[] { q.W, q.X, q.Y, q.Z } : null,
            ["linear_accel"] = Vec(b.LinearAccel), ["gravity"] = Vec(b.Gravity),
            ["temperature"] = b.Temperature is { } t ? (int)t : null,
            ["calib_stat"] = b.CalibStat is { } c ? (int)c : null,
        };
    }

    public static Dictionary<string, object?> Bno055Profile(Bno055CalibrationProfile p) => new()
    {
        ["accel_offset"] = Vec(p.AccelOffset), ["mag_offset"] = Vec(p.MagOffset), ["gyro_offset"] = Vec(p.GyroOffset),
        ["accel_radius"] = p.AccelRadius, ["mag_radius"] = p.MagRadius,
    };

    // ── BNO086 (contract 05) ─────────────────────────────────────────────────

    public static Dictionary<string, object?> Bno086Cargo(ShtpCargo c) => new()
    {
        ["channel"] = c.Channel, ["seq"] = c.Seq, ["payload"] = Vectors.ToHex(c.Payload), ["capture_us"] = c.CaptureUs,
    };

    public static Dictionary<string, object?> Bno086Report(BnoReport r) => new()
    {
        ["type"] = r.Kind, ["fields"] = r.Fields(),
    };

    // ── VL53L4CD / L7 / LX ───────────────────────────────────────────────────

    public static Dictionary<string, object?> Vl53l4Decode(int report, byte[] raw)
    {
        switch (report)
        {
            case 0x91:
                var d = Vl53l4RegData.Unpack(raw);
                return new() { ["cmd"] = d.Cmd, ["timestamp_us"] = d.TimestampUs, ["data"] = Vectors.ToHex(d.Data) };
            case 0x92:
                var i = Vl53l4Info.Unpack(raw);
                return new()
                {
                    ["int_edges"] = i.IntEdges, ["slots_skipped"] = i.SlotsSkipped, ["i2c_errors"] = i.I2cErrors,
                    ["last_i2c_error"] = i.LastI2cError, ["model_id"] = i.ModelId, ["fw_status"] = i.FwStatus,
                    ["initialized"] = i.Initialized, ["xshut_level"] = i.XshutLevel, ["int_level"] = i.IntLevel,
                    ["i2c_khz"] = i.I2cKhz,
                };
            case 0x93:
                var s = Vl53l4StreamData.Unpack(raw);
                return new()
                {
                    ["timestamp_us"] = s.TimestampUs, ["addr"] = s.Addr, ["len"] = s.Len, ["data"] = Vectors.ToHex(s.Data),
                };
            default:
                throw new ArgumentException($"unknown VL53L4CD report {report}");
        }
    }

    public static Dictionary<string, object?> Vl53l4Result(Vl53l4Results r) => new()
    {
        ["range_status"] = r.RangeStatus, ["distance_mm"] = r.DistanceMm, ["ambient_rate_kcps"] = r.AmbientRateKcps,
        ["ambient_per_spad_kcps"] = r.AmbientPerSpadKcps, ["signal_rate_kcps"] = r.SignalRateKcps,
        ["signal_per_spad_kcps"] = r.SignalPerSpadKcps, ["number_of_spad"] = r.NumberOfSpad, ["sigma_mm"] = r.SigmaMm,
        ["stream_count"] = r.StreamCount,
    };

    public static Dictionary<string, object?> Vl53l7InfoShape(byte[] raw)
    {
        var i = Depz.Sensor.Vl53l7.Vl53l7Info.Unpack(raw);
        return new()
        {
            ["int_edges"] = i.IntEdges, ["frames_dropped"] = i.FramesDropped, ["i2c_errors"] = i.I2cErrors,
            ["last_i2c_error"] = i.LastI2cError, ["lpn_level"] = i.LpnLevel, ["int_level"] = i.IntLevel,
            ["i2c_khz"] = i.I2cKhz, ["frame_size"] = i.FrameSize, ["streaming"] = i.Streaming,
        };
    }

    public static Dictionary<string, object?> Vl53lxInfoShape(byte[] raw)
    {
        var i = Depz.Sensor.Vl53lx.Vl53lxInfo.Unpack(raw);
        return new()
        {
            ["int_edges"] = i.IntEdges, ["slots_skipped"] = i.SlotsSkipped, ["i2c_errors"] = i.I2cErrors,
            ["last_i2c_error"] = i.LastI2cError, ["xshut_level"] = i.XshutLevel, ["int_level"] = i.IntLevel,
            ["i2c_khz"] = i.I2cKhz, ["addr_width"] = i.AddrWidth, ["n_clear"] = i.NClear,
            ["frames_dropped"] = i.FramesDropped,
        };
    }

    public static Dictionary<string, object?> Vl53lxDie(Vl53lxDieResult r) => new()
    {
        ["range_status"] = r.RangeStatus, ["distance_mm"] = r.DistanceMm, ["sigma_mm"] = r.SigmaMm,
        ["signal_rate_kcps"] = r.SignalRateKcps, ["ambient_rate_kcps"] = r.AmbientRateKcps,
        ["signal_per_spad_kcps"] = r.SignalPerSpadKcps, ["ambient_per_spad_kcps"] = r.AmbientPerSpadKcps,
        ["number_of_spad"] = r.NumberOfSpad, ["stream_count"] = r.StreamCount,
    };

    public static Dictionary<string, object?> Vl53lxL0x(Vl53lxL0xRaw r) => new()
    {
        ["distance_raw"] = r.DistanceRaw, ["device_range_status"] = r.DeviceRangeStatus,
        ["signal_rate_mcps_1616"] = r.SignalRateMcps1616, ["ambient_rate_mcps_1616"] = r.AmbientRateMcps1616,
        ["effective_spad_count_88"] = r.EffectiveSpadCount88,
    };

    public static Dictionary<string, object?> Vl53lxHistogram(Vl53lxHistogramRaw r) => new()
    {
        ["interrupt_status"] = r.InterruptStatus, ["range_status"] = r.RangeStatus, ["report_status"] = r.ReportStatus,
        ["stream_count"] = r.StreamCount, ["dss_actual_effective_spads"] = r.DssActualEffectiveSpads,
        ["reference_phase"] = r.ReferencePhase, ["vcsel_start"] = r.VcselStart, ["bins"] = r.Bins.ToArray(),
    };

    // ── TMF8829 (contract 14) ────────────────────────────────────────────────

    /// <summary><c>_arrays(res)</c>: peaks, rows/cols of the arrays present, then the nullable arrays.</summary>
    public static Dictionary<string, object?> Tmf8829Arrays(Tmf8829PixelResults r) => new()
    {
        ["peaks"] = r.Peaks,
        ["rows"] = r.Rows,
        ["cols"] = r.Cols,
        ["distance"] = Flat(r.Distance),
        ["snr"] = Flat(r.Snr),
        ["signal"] = Flat(r.Signal),
        ["noise"] = Flat(r.Noise),
        ["xtalk"] = Flat(r.Xtalk),
    };

    public static Dictionary<string, object?> Tmf8829Decode(int report, ReadOnlySpan<byte> payload)
    {
        switch ((Tmf8829Rpt)report)
        {
            case Tmf8829Rpt.RegData:
                var r = Tmf8829RegData.Unpack(payload);
                return new() { ["timestamp_us"] = r.TimestampUs, ["data"] = Vectors.ToHex(r.Data) };
            case Tmf8829Rpt.Info:
                var i = Tmf8829Info.Unpack(payload);
                return new()
                {
                    ["int_edges"] = i.IntEdges, ["frames"] = i.Frames, ["frames_aborted"] = i.FramesAborted,
                    ["frame_errors"] = i.FrameErrors, ["spi_timeouts"] = i.SpiTimeouts, ["int_level"] = i.IntLevel,
                    ["streaming"] = i.Streaming, ["int_mask"] = i.IntMask, ["last_int_status"] = i.LastIntStatus,
                    ["last_fifo_status"] = i.LastFifoStatus,
                };
            case Tmf8829Rpt.Frame:
                var f = Tmf8829FrameChunk.Unpack(payload);
                return new()
                {
                    ["timestamp_us"] = f.TimestampUs, ["full_size"] = f.FullSize, ["offset"] = f.Offset,
                    ["frame_seq"] = f.FrameSeq, ["edge_us"] = f.EdgeUs, ["data"] = Vectors.ToHex(f.Data),
                };
            case Tmf8829Rpt.Event:
                var e = Tmf8829Event.Unpack(payload);
                return new()
                {
                    ["timestamp_us"] = e.TimestampUs, ["int_status"] = e.IntStatus, ["code"] = e.Code,
                    ["frame_id"] = e.FrameId, ["fifo_status"] = e.FifoStatus,
                };
            default:
                throw new ArgumentException($"unknown TMF8829 report {report}");
        }
    }

    /// <summary>One completed frame of the reassembly shape.</summary>
    /// <summary>vl53l8.json decode shape: one RPT_VL53_FRAME chunk (contract 04 §3).</summary>
    public static Dictionary<string, object?> Vl53l8Decode(int report, byte[] payload)
    {
        if (report != (int)Vl53l8Rpt.Vl53Frame)
            throw new ArgumentException($"unknown VL53L8 report {report}");
        var c = FrameChunk.Unpack(payload);
        return new()
        {
            ["timestamp_us"] = c.TimestampUs, ["full_size"] = c.FullSize, ["offset"] = c.Offset,
            ["data"] = Vectors.ToHex(c.Data),
        };
    }

    /// <summary>RPT_VL53_FRAME payloads through one reassembler; vl53l8.json reassembly shape (contract 04 §4).</summary>
    public static Dictionary<string, object?> Vl53l8Reassembly(IEnumerable<byte[]> chunks)
    {
        var r = new FrameReassembler();
        var frames = new List<object?>();
        foreach (var ch in chunks)
            if (r.Feed(FrameChunk.Unpack(ch)) is { } f)
                frames.Add(new Dictionary<string, object?>
                {
                    ["timestamp_us"] = f.TimestampUs, ["size"] = f.Frame.Length, ["sha256"] = Sha256(f.Frame),
                });
        return new() { ["frames"] = frames, ["completed"] = r.Completed, ["discarded"] = r.Discarded };
    }

    public static Dictionary<string, object?> Tmf8829Done(Tmf8829RawFrame f) => new()
    {
        ["frame_seq"] = f.FrameSeq, ["timestamp_us"] = f.TimestampUs, ["edge_us"] = f.EdgeUs,
        ["size"] = f.Data.Length, ["sha256"] = Sha256(f.Data),
    };

    /// <summary>FRAME-chunk payloads fed to one reassembler; the reassembly shape.</summary>
    public static Dictionary<string, object?> Tmf8829Reassembly(IEnumerable<byte[]> chunks)
    {
        var r = new Tmf8829FrameReassembler();
        var frames = new List<object?>();
        foreach (var ch in chunks)
            if (r.Feed(Tmf8829FrameChunk.Unpack(ch)) is { } f)
                frames.Add(Tmf8829Done(f));
        return new()
        {
            ["frames"] = frames, ["completed"] = r.Completed, ["discarded"] = r.Discarded, ["seq_gaps"] = r.SeqGaps,
        };
    }

    public static Dictionary<string, object?> Tmf8829Header(Tmf8829FrameInfo info) => new()
    {
        ["fifo_status"] = info.FifoStatus, ["systick"] = info.Systick, ["id"] = info.Id,
        ["kind"] = info.Kind, ["fp_mode"] = info.FpMode, ["layout"] = info.Layout,
        ["payload"] = info.Payload, ["f_number"] = info.FNumber,
        ["temperature"] = info.Temperature.Select(x => (long)x).ToArray(), ["bdv"] = info.Bdv,
        ["ref_pos"] = info.RefPos.Select(x => (long)x).ToArray(),
        ["t0_integration"] = info.T0Integration, ["t1_integration"] = info.T1Integration,
        ["frame_status"] = info.FrameStatus, ["reserved"] = info.Reserved, ["eof"] = info.Eof,
        ["size"] = info.Size,
    };

    /// <summary>tmf8829.json frames shape: header, problems, then results or data_sha256.</summary>
    public static Dictionary<string, object?> Tmf8829Frame(byte[] raw)
    {
        var info = Tmf8829Frames.ParseFrame(raw);
        var o = new Dictionary<string, object?> { ["header"] = Tmf8829Header(info), ["problems"] = info.Problems() };
        if (info.Kind == Tmf8829Frames.FidResults)
            o["results"] = Tmf8829Arrays(Tmf8829Frames.ParseResults(raw, info));
        else
            o["data_sha256"] = Sha256(raw.AsSpan(21, raw.Length - 33));
        return o;
    }

    /// <summary>One measurement of the measurements <c>expect</c> list.</summary>
    public static Dictionary<string, object?> Tmf8829Meas(Tmf8829Measurement m, Tmf8829MeasurementBuilder b)
    {
        var got = new Dictionary<string, object?>
        {
            ["timestamp_us"] = m.TimestampUs, ["flags"] = m.Flags, ["fp_mode"] = m.FpMode,
            ["f_number"] = m.FNumber,
            ["frame_times_us"] = m.Times.Select(t => t.TimestampUs).ToArray(),
            ["t1_len_us"] = m.Times.Select(t => (long)t.T1LenUs).ToArray(),
            ["ratio_q16"] = b.Clock.RatioQ16,
            ["n_histograms"] = m.Histograms.Count, ["n_ref_frames"] = m.RefFrames.Count,
            ["n_problems"] = m.Problems.Count, ["distance_in_mm"] = m.Frames[^1].DistanceInMm,
        };
        foreach (var (key, value) in Tmf8829Arrays(m.Results))
            got[key] = value;
        return got;
    }
}
