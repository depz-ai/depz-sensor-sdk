using System.Numerics;
using Depz.Sensor.Bno086;
using Depz.Sensor.Devices;
using Depz.Sensor.Tmf8829;
using Depz.Sensor.Vl53l8;

namespace Depz.Sensor.Dataset;

/// <summary>
/// The contract 09 record value (<c>v</c>) of each kind, from the device classes' results — what
/// <see cref="SessionRecorder"/> writes, public for writers of their own. Key for key the reference
/// Python SDK's values.
/// </summary>
public static class DatasetRecords
{
    // The raw ADC streams (0x14..0x16): only these keep their integer counts.
    private static readonly HashSet<int> Bno086RawIds = new() { 0x14, 0x15, 0x16 };

    /// <summary><c>sr04</c>: <c>{echo_us, source}</c>.</summary>
    public static Dictionary<string, object?> Sr04(Sr04Measurement m) => new()
    {
        ["echo_us"] = (long)m.EchoTimeUs, ["source"] = m.Source,
    };

    /// <summary><c>vl53l4</c>: the result fields, distance raw whatever the status.</summary>
    public static Dictionary<string, object?> Vl53l4(Vl53l4Measurement m) => new()
    {
        ["range_status"] = (long)m.RangeStatus,
        ["distance_mm"] = (long)m.DistanceMm,
        ["sigma_mm"] = (long)m.SigmaMm,
        ["signal_rate_kcps"] = (long)m.SignalRateKcps,
        ["ambient_rate_kcps"] = (long)m.AmbientRateKcps,
        ["number_of_spad"] = (long)m.NumberOfSpad,
        ["stream_count"] = (long)m.StreamCount,
    };

    /// <summary><c>vl53l8</c>: per-zone arrays cut to the resolution; <paramref name="layers"/> adds signal and
    /// ambient (kcps/SPAD), sigma (mm) and reflectance (%) as reals; <c>motion</c> when the indicator is configured.</summary>
    public static Dictionary<string, object?> Vl53l8(Vl53l8Frame f, bool layers = false)
    {
        int n = f.Resolution;
        var v = new Dictionary<string, object?>
        {
            ["resolution"] = (long)n,
            ["silicon_temp_degc"] = (long)f.SiliconTempDegc,
            ["distance_mm"] = Ints(f.DistanceMm.Take(n)),
            ["target_status"] = Ints(f.TargetStatus.Take(n).Select(b => (long)b)),
            ["nb_target_detected"] = Ints(f.NbTargetDetected.Take(n).Select(b => (long)b)),
        };
        if (layers)
        {
            v["signal_per_spad"] = Doubles(f.SignalPerSpad.Take(n));
            v["ambient_per_spad"] = Doubles(f.AmbientPerSpad.Take(n));
            v["range_sigma_mm"] = Doubles(f.RangeSigmaMm.Take(n));
            v["reflectance"] = Doubles(f.Reflectance.Take(n));
        }
        if (f.Motion is Vl53l8Motion mo)
        {
            v["motion"] = new Dictionary<string, object?>
            {
                ["global_indicator_1"] = (long)mo.GlobalIndicator1,
                ["global_indicator_2"] = (long)mo.GlobalIndicator2,
                ["status"] = (long)mo.Status,
                ["nb_of_detected_aggregates"] = (long)mo.NbOfDetectedAggregates,
                ["nb_of_aggregates"] = (long)mo.NbOfAggregates,
                ["motion"] = Ints(mo.Motion.Select(x => (long)x)),
            };
        }
        return v;
    }

    /// <summary><c>vl53l8_cnh</c>: the raw CNH block of a frame with the armed setup's decode parameters.</summary>
    public static Dictionary<string, object?> Vl53l8Cnh(byte[] cnhRaw, Vl53l8CnhSetup setup) => new()
    {
        ["nb_aggregates"] = (long)setup.NbOfAggregates,
        ["feature_length"] = (long)setup.FeatureLength,
        ["sum_span"] = (long)setup.SumSpan,
        ["start_bin"] = (long)Math.Floor(setup.RefBinOffset / 2048.0),
        ["cnh_raw_b64"] = Convert.ToBase64String(cnhRaw),
    };

    /// <summary><c>vl53lx</c>: the family-wide result, plus <c>targets</c> when the driver reported several.</summary>
    public static Dictionary<string, object?> Vl53lx(Vl53lxMeasurement m, string? product, string? driver)
    {
        var v = new Dictionary<string, object?>
        {
            ["product"] = product,
            ["driver"] = driver,
            ["status"] = (long)m.Status,
            ["distance_mm"] = (long)m.DistanceMm,
            ["sigma_mm"] = m.SigmaMm,
            ["signal_kcps"] = m.SignalKcps,
            ["ambient_kcps"] = m.AmbientKcps,
            ["spads"] = m.Spads,
        };
        if (m.Targets.Count > 0)
        {
            v["targets"] = m.Targets.Select(t => (object?)new Dictionary<string, object?>
            {
                ["distance_mm"] = (long)t.DistanceMm, ["status"] = (long)t.Status, ["signal_kcps"] = t.SignalKcps,
            }).ToList();
        }
        return v;
    }

    /// <summary><c>bno055</c>: <c>unit_sel</c> and every channel the streamed block covered, scaled.</summary>
    public static Dictionary<string, object?> Bno055(Bno055Sample s)
    {
        var v = new Dictionary<string, object?> { ["unit_sel"] = (long)s.Units.Pack() };
        void Put(string key, double[]? a)
        {
            if (a is not null)
                v[key] = a.Select(x => (object?)x).ToList();
        }
        Put("quaternion", s.Quaternion);
        Put("euler", s.Euler);
        Put("accel", s.Accel);
        Put("gyro", s.Gyro);
        Put("mag", s.Mag);
        Put("linear_accel", s.LinearAccel);
        Put("gravity", s.Gravity);
        if (s.Temperature is double t)
            v["temperature"] = t;
        if (s.Calibration is { } c)
            v["calib"] = Ints(new long[] { c.System, c.Gyro, c.Accel, c.Mag });
        return v;
    }

    /// <summary><c>tmf8829</c>: one measurement — distances in mm rounded to 0.1, the raw per-zone arrays.</summary>
    public static Dictionary<string, object?> Tmf8829(Tmf8829Measurement m)
    {
        var r = m.Results;
        var (rows, cols) = m.Shape;
        var v = new Dictionary<string, object?>
        {
            ["fp_mode"] = (long)m.FpMode,
            ["rows"] = (long)rows,
            ["cols"] = (long)cols,
            ["peaks"] = (long)r.Peaks,
            ["f_number"] = (long)m.FNumber,
            ["flags"] = (long)m.Flags,
            ["temperature_c"] = (long)m.TemperatureC,
            ["distance_mm"] = m.DistanceMm is null ? new List<object?>() : m.DistanceMm.Select(x => (object?)Round1(x)).ToList(),
            ["snr"] = r.Snr is null ? new List<object?>() : Ints(Flat(r.Snr)),
        };
        if (r.Signal is not null)
            v["signal"] = Ints(Flat(r.Signal));
        if (r.Noise is not null)
            v["noise"] = Ints(Flat(r.Noise));
        if (r.Xtalk is not null)
            v["xtalk"] = Ints(Flat(r.Xtalk));
        return v;
    }

    /// <summary><c>bno086</c>: the report in snake_case — type and sensor id, then only the fields the report
    /// carries (scaled), the integer counts of the raw ADC streams, and the device timestamp.</summary>
    public static Dictionary<string, object?> Bno086(BnoReport r)
    {
        var v = new Dictionary<string, object?> { ["type"] = r.Kind, ["sensor_id"] = (long)r.SensorIdValue };
        void Num(string key, object? value)
        {
            if (value is not null)
                v[key] = value;
        }
        switch (r)
        {
            case RotationVector q:
                (v["i"], v["j"], v["k"], v["real"]) = (q.I, q.J, q.K, q.Real);
                break;
            case GyroIntegratedRV g:
                (v["i"], v["j"], v["k"], v["real"]) = (g.I, g.J, g.K, g.Real);
                break;
            case Acceleration a:
                (v["x"], v["y"], v["z"]) = (a.X, a.Y, a.Z);
                break;
            case Gyroscope g:
                (v["x"], v["y"], v["z"]) = (g.X, g.Y, g.Z);
                break;
            case Magnetometer mg:
                (v["x"], v["y"], v["z"]) = (mg.X, mg.Y, mg.Z);
                break;
            case UncalibratedGyroscope ug:
                (v["x"], v["y"], v["z"]) = (ug.X, ug.Y, ug.Z);
                break;
            case UncalibratedMagnetometer um:
                (v["x"], v["y"], v["z"]) = (um.X, um.Y, um.Z);
                break;
        }
        if (r is RotationVector rv)
            Num("accuracy_rad", rv.AccuracyRad);
        if (r is InputReport ir)
            v["accuracy"] = (long)ir.Accuracy;
        switch (r)
        {
            case StepCounter sc:
                (v["steps"], v["latency_us"]) = (sc.Steps, sc.LatencyUs);
                break;
            case StepDetector sd:
                v["latency_us"] = sd.LatencyUs;
                break;
            case TapDetector td:
                (v["flags"], v["double_tap"]) = ((long)td.Flags, td.DoubleTap);
                break;
            case ShakeDetector sh:
                v["flags"] = (long)sh.Flags;
                break;
            case SignificantMotion sm:
                v["motion"] = (long)sm.Motion;
                break;
            case StabilityClassifier st:
                v["classification"] = (long)st.Classification;
                break;
            case PersonalActivityClassifier pa:
                v["most_likely_state"] = (long)pa.MostLikelyState;
                break;
            case ScalarReport sr:
                v["value"] = sr.Value;
                break;
            case MotionRequest mr:
                (v["intent"], v["request"]) = ((long)mr.Intent, (long)mr.Request);
                break;
            case FlushCompleted fc:
                v["flushed_sensor"] = (long)fc.FlushedSensor;
                break;
        }
        switch (r)
        {
            case UncalibratedGyroscope ug:
                v["bias"] = ug.Bias.Select(x => (object?)x).ToList();
                break;
            case UncalibratedMagnetometer um:
                v["bias"] = um.Bias.Select(x => (object?)x).ToList();
                break;
            case GyroIntegratedRV g:
                v["angular_velocity"] = g.AngularVelocity.Select(x => (object?)x).ToList();
                break;
            case PersonalActivityClassifier pa:
                v["confidences"] = Ints(pa.Confidences.Select(x => (long)x));
                break;
        }
        if (r is GenericEvent ge)
            v["value_raw"] = ge.ValueRaw;
        if (Bno086RawIds.Contains(r.SensorIdValue) && r is RawSensor raw)
        {
            v["x_raw"] = (long)raw.XRaw;
            v["y_raw"] = (long)raw.YRaw;
            v["z_raw"] = (long)raw.ZRaw;
            v["temperature_raw"] = (long)raw.TemperatureRaw;
            v["sensor_timestamp_us"] = raw.SensorTimestampUs;
        }
        v["timestamp_us"] = r.TimestampUs;
        return v;
    }

    private static List<object?> Ints(IEnumerable<long> xs) => xs.Select(x => (object?)x).ToList();

    private static List<object?> Ints(IEnumerable<int> xs) => xs.Select(x => (object?)(long)x).ToList();

    private static List<object?> Doubles(IEnumerable<double> xs) => xs.Select(x => (object?)x).ToList();

    private static IEnumerable<long> Flat(Array a)
    {
        foreach (object? x in a)
            yield return Convert.ToInt64(x, System.Globalization.CultureInfo.InvariantCulture);
    }

    /// <summary>Python's <c>round(x, 1)</c>: the nearest tenth of the exact binary value, ties to even, as the
    /// double closest to it.</summary>
    public static double Round1(double x)
    {
        if (x == 0 || double.IsNaN(x) || double.IsInfinity(x))
            return x;
        long bits = BitConverter.DoubleToInt64Bits(x);
        bool negative = bits < 0;
        int exp = (int)((bits >> 52) & 0x7FF);
        long mantissa = bits & 0xFFFFFFFFFFFFFL;
        if (exp == 0)
            exp = 1;
        else
            mantissa |= 1L << 52;
        int e = exp - 1075; // |x| = mantissa · 2^e exactly
        BigInteger scaled = new BigInteger(mantissa) * 10;
        BigInteger n;
        if (e >= 0)
        {
            n = scaled << e;
        }
        else
        {
            BigInteger den = BigInteger.One << -e;
            n = BigInteger.DivRem(scaled, den, out BigInteger rem);
            int c = (rem * 2).CompareTo(den);
            if (c > 0 || (c == 0 && !n.IsEven))
                n += 1;
        }
        double result = (double)n / 10.0;
        return negative ? -result : result;
    }
}
