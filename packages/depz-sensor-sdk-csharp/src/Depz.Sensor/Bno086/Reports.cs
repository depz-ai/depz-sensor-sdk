using System.Buffers.Binary;

namespace Depz.Sensor.Bno086;

/// <summary>SH-2 input report IDs (contracts/05_SENSOR_BNO086.md §5).</summary>
public enum SensorId
{
    Accelerometer = 0x01,
    Gyroscope = 0x02,
    Magnetometer = 0x03,
    LinearAcceleration = 0x04,
    RotationVector = 0x05,
    Gravity = 0x06,
    UncalibratedGyroscope = 0x07,
    GameRotationVector = 0x08,
    GeomagneticRotationVector = 0x09,
    Pressure = 0x0A,
    AmbientLight = 0x0B,
    Humidity = 0x0C,
    Proximity = 0x0D,
    Temperature = 0x0E,
    UncalibratedMagnetometer = 0x0F,
    TapDetector = 0x10,
    StepCounter = 0x11,
    SignificantMotion = 0x12,
    StabilityClassifier = 0x13,
    RawAccelerometer = 0x14,
    RawGyroscope = 0x15,
    RawMagnetometer = 0x16,
    StepDetector = 0x18,
    ShakeDetector = 0x19,
    FlipDetector = 0x1A,
    PickupDetector = 0x1B,
    StabilityDetector = 0x1C,
    PersonalActivityClassifier = 0x1E,
    SleepDetector = 0x1F,
    TiltDetector = 0x20,
    PocketDetector = 0x21,
    CircleDetector = 0x22,
    HeartRateMonitor = 0x23,
    ArvrStabilizedRv = 0x28,
    ArvrStabilizedGameRv = 0x29,
    GyroIntegratedRv = 0x2A,
}

/// <summary>Base for a decoded report. Raw wire integers are authoritative.</summary>
public abstract record BnoReport(int SensorIdValue, long TimestampUs)
{
    /// <summary>Vector "type" tag (matches the concrete record name).</summary>
    public abstract string Kind { get; }

    /// <summary>Field name → value (long, null, int[] or string), matching the golden vector fields.</summary>
    public abstract IReadOnlyDictionary<string, object?> Fields();

    protected void AddHead(Dictionary<string, object?> d)
    {
        d["sensor_id"] = (long)SensorIdValue;
        d["timestamp_us"] = TimestampUs;
    }
}

/// <summary>Channel-3/4 report with the common SH-2 header fields.</summary>
public abstract record InputReport(int SensorIdValue, long TimestampUs, int Seq, int Accuracy, long DelayUs)
    : BnoReport(SensorIdValue, TimestampUs)
{
    protected void AddInputHead(Dictionary<string, object?> d)
    {
        AddHead(d);
        d["seq"] = (long)Seq;
        d["accuracy"] = (long)Accuracy;
        d["delay_us"] = DelayUs;
    }
}

public sealed record Acceleration(
    int SensorIdValue, long TimestampUs, int Seq, int Accuracy, long DelayUs,
    int XRaw, int YRaw, int ZRaw)
    : InputReport(SensorIdValue, TimestampUs, Seq, Accuracy, DelayUs)
{
    public override string Kind => "Acceleration";
    public override IReadOnlyDictionary<string, object?> Fields()
    {
        var d = new Dictionary<string, object?>();
        AddInputHead(d);
        d["x_raw"] = (long)XRaw; d["y_raw"] = (long)YRaw; d["z_raw"] = (long)ZRaw;
        return d;
    }
}

public sealed record Gyroscope(
    int SensorIdValue, long TimestampUs, int Seq, int Accuracy, long DelayUs,
    int XRaw, int YRaw, int ZRaw)
    : InputReport(SensorIdValue, TimestampUs, Seq, Accuracy, DelayUs)
{
    public override string Kind => "Gyroscope";
    public override IReadOnlyDictionary<string, object?> Fields()
    {
        var d = new Dictionary<string, object?>();
        AddInputHead(d);
        d["x_raw"] = (long)XRaw; d["y_raw"] = (long)YRaw; d["z_raw"] = (long)ZRaw;
        return d;
    }
}

public sealed record Magnetometer(
    int SensorIdValue, long TimestampUs, int Seq, int Accuracy, long DelayUs,
    int XRaw, int YRaw, int ZRaw)
    : InputReport(SensorIdValue, TimestampUs, Seq, Accuracy, DelayUs)
{
    public override string Kind => "Magnetometer";
    public override IReadOnlyDictionary<string, object?> Fields()
    {
        var d = new Dictionary<string, object?>();
        AddInputHead(d);
        d["x_raw"] = (long)XRaw; d["y_raw"] = (long)YRaw; d["z_raw"] = (long)ZRaw;
        return d;
    }
}

public sealed record UncalibratedGyroscope(
    int SensorIdValue, long TimestampUs, int Seq, int Accuracy, long DelayUs,
    int XRaw, int YRaw, int ZRaw, int BiasXRaw, int BiasYRaw, int BiasZRaw)
    : InputReport(SensorIdValue, TimestampUs, Seq, Accuracy, DelayUs)
{
    public override string Kind => "UncalibratedGyroscope";
    public override IReadOnlyDictionary<string, object?> Fields()
    {
        var d = new Dictionary<string, object?>();
        AddInputHead(d);
        d["x_raw"] = (long)XRaw; d["y_raw"] = (long)YRaw; d["z_raw"] = (long)ZRaw;
        d["bias_x_raw"] = (long)BiasXRaw; d["bias_y_raw"] = (long)BiasYRaw; d["bias_z_raw"] = (long)BiasZRaw;
        return d;
    }
}

public sealed record UncalibratedMagnetometer(
    int SensorIdValue, long TimestampUs, int Seq, int Accuracy, long DelayUs,
    int XRaw, int YRaw, int ZRaw, int BiasXRaw, int BiasYRaw, int BiasZRaw)
    : InputReport(SensorIdValue, TimestampUs, Seq, Accuracy, DelayUs)
{
    public override string Kind => "UncalibratedMagnetometer";
    public override IReadOnlyDictionary<string, object?> Fields()
    {
        var d = new Dictionary<string, object?>();
        AddInputHead(d);
        d["x_raw"] = (long)XRaw; d["y_raw"] = (long)YRaw; d["z_raw"] = (long)ZRaw;
        d["bias_x_raw"] = (long)BiasXRaw; d["bias_y_raw"] = (long)BiasYRaw; d["bias_z_raw"] = (long)BiasZRaw;
        return d;
    }
}

public sealed record RotationVector(
    int SensorIdValue, long TimestampUs, int Seq, int Accuracy, long DelayUs,
    int IRaw, int JRaw, int KRaw, int RealRaw, int? AccuracyRaw)
    : InputReport(SensorIdValue, TimestampUs, Seq, Accuracy, DelayUs)
{
    public override string Kind => "RotationVector";
    public override IReadOnlyDictionary<string, object?> Fields()
    {
        var d = new Dictionary<string, object?>();
        AddInputHead(d);
        d["i_raw"] = (long)IRaw; d["j_raw"] = (long)JRaw; d["k_raw"] = (long)KRaw;
        d["real_raw"] = (long)RealRaw;
        d["accuracy_raw"] = AccuracyRaw is null ? null : (long)AccuracyRaw.Value;
        return d;
    }
}

public sealed record ScalarReport(
    int SensorIdValue, long TimestampUs, int Seq, int Accuracy, long DelayUs, long ValueRaw)
    : InputReport(SensorIdValue, TimestampUs, Seq, Accuracy, DelayUs)
{
    public override string Kind => "ScalarReport";
    public override IReadOnlyDictionary<string, object?> Fields()
    {
        var d = new Dictionary<string, object?>();
        AddInputHead(d);
        d["value_raw"] = ValueRaw;
        return d;
    }
}

public sealed record TapDetector(
    int SensorIdValue, long TimestampUs, int Seq, int Accuracy, long DelayUs, int Flags)
    : InputReport(SensorIdValue, TimestampUs, Seq, Accuracy, DelayUs)
{
    public override string Kind => "TapDetector";
    public override IReadOnlyDictionary<string, object?> Fields()
    {
        var d = new Dictionary<string, object?>();
        AddInputHead(d);
        d["flags"] = (long)Flags;
        return d;
    }
}

public sealed record StepCounter(
    int SensorIdValue, long TimestampUs, int Seq, int Accuracy, long DelayUs, long LatencyUs, long Steps)
    : InputReport(SensorIdValue, TimestampUs, Seq, Accuracy, DelayUs)
{
    public override string Kind => "StepCounter";
    public override IReadOnlyDictionary<string, object?> Fields()
    {
        var d = new Dictionary<string, object?>();
        AddInputHead(d);
        d["latency_us"] = LatencyUs; d["steps"] = Steps;
        return d;
    }
}

public sealed record StepDetector(
    int SensorIdValue, long TimestampUs, int Seq, int Accuracy, long DelayUs, long LatencyUs)
    : InputReport(SensorIdValue, TimestampUs, Seq, Accuracy, DelayUs)
{
    public override string Kind => "StepDetector";
    public override IReadOnlyDictionary<string, object?> Fields()
    {
        var d = new Dictionary<string, object?>();
        AddInputHead(d);
        d["latency_us"] = LatencyUs;
        return d;
    }
}

public sealed record SignificantMotion(
    int SensorIdValue, long TimestampUs, int Seq, int Accuracy, long DelayUs, int Motion)
    : InputReport(SensorIdValue, TimestampUs, Seq, Accuracy, DelayUs)
{
    public override string Kind => "SignificantMotion";
    public override IReadOnlyDictionary<string, object?> Fields()
    {
        var d = new Dictionary<string, object?>();
        AddInputHead(d);
        d["motion"] = (long)Motion;
        return d;
    }
}

public sealed record StabilityClassifier(
    int SensorIdValue, long TimestampUs, int Seq, int Accuracy, long DelayUs, int Classification)
    : InputReport(SensorIdValue, TimestampUs, Seq, Accuracy, DelayUs)
{
    public override string Kind => "StabilityClassifier";
    public override IReadOnlyDictionary<string, object?> Fields()
    {
        var d = new Dictionary<string, object?>();
        AddInputHead(d);
        d["classification"] = (long)Classification;
        return d;
    }
}

public sealed record ShakeDetector(
    int SensorIdValue, long TimestampUs, int Seq, int Accuracy, long DelayUs, int Flags)
    : InputReport(SensorIdValue, TimestampUs, Seq, Accuracy, DelayUs)
{
    public override string Kind => "ShakeDetector";
    public override IReadOnlyDictionary<string, object?> Fields()
    {
        var d = new Dictionary<string, object?>();
        AddInputHead(d);
        d["flags"] = (long)Flags;
        return d;
    }
}

public sealed record GenericEvent(
    int SensorIdValue, long TimestampUs, int Seq, int Accuracy, long DelayUs, long ValueRaw)
    : InputReport(SensorIdValue, TimestampUs, Seq, Accuracy, DelayUs)
{
    public override string Kind => "GenericEvent";
    public override IReadOnlyDictionary<string, object?> Fields()
    {
        var d = new Dictionary<string, object?>();
        AddInputHead(d);
        d["value_raw"] = ValueRaw;
        return d;
    }
}

public sealed record PersonalActivityClassifier(
    int SensorIdValue, long TimestampUs, int Seq, int Accuracy, long DelayUs,
    int PageNumber, int EndOfSequence, int MostLikelyState, int[] Confidences)
    : InputReport(SensorIdValue, TimestampUs, Seq, Accuracy, DelayUs)
{
    public override string Kind => "PersonalActivityClassifier";
    public override IReadOnlyDictionary<string, object?> Fields()
    {
        var d = new Dictionary<string, object?>();
        AddInputHead(d);
        d["page_number"] = (long)PageNumber;
        d["end_of_sequence"] = (long)EndOfSequence;
        d["most_likely_state"] = (long)MostLikelyState;
        d["confidences"] = Confidences;
        return d;
    }
}

public sealed record RawSensor(
    int SensorIdValue, long TimestampUs, int Seq, int Accuracy, long DelayUs,
    int XRaw, int YRaw, int ZRaw, long SensorTimestampUs, int TemperatureRaw)
    : InputReport(SensorIdValue, TimestampUs, Seq, Accuracy, DelayUs)
{
    public override string Kind => "RawSensor";
    public override IReadOnlyDictionary<string, object?> Fields()
    {
        var d = new Dictionary<string, object?>();
        AddInputHead(d);
        d["x_raw"] = (long)XRaw; d["y_raw"] = (long)YRaw; d["z_raw"] = (long)ZRaw;
        d["sensor_timestamp_us"] = SensorTimestampUs;
        d["temperature_raw"] = (long)TemperatureRaw;
        return d;
    }
}

public sealed record GyroIntegratedRV(
    int SensorIdValue, long TimestampUs,
    int IRaw, int JRaw, int KRaw, int RealRaw, int VxRaw, int VyRaw, int VzRaw)
    : BnoReport(SensorIdValue, TimestampUs)
{
    public override string Kind => "GyroIntegratedRV";
    public override IReadOnlyDictionary<string, object?> Fields()
    {
        var d = new Dictionary<string, object?>();
        AddHead(d);
        d["i_raw"] = (long)IRaw; d["j_raw"] = (long)JRaw; d["k_raw"] = (long)KRaw;
        d["real_raw"] = (long)RealRaw;
        d["vx_raw"] = (long)VxRaw; d["vy_raw"] = (long)VyRaw; d["vz_raw"] = (long)VzRaw;
        return d;
    }
}

public sealed record UnknownReport(int SensorIdValue, long TimestampUs, byte[] Data)
    : BnoReport(SensorIdValue, TimestampUs)
{
    public override string Kind => "UnknownReport";
    public override IReadOnlyDictionary<string, object?> Fields()
    {
        var d = new Dictionary<string, object?>();
        AddHead(d);
        d["data"] = Convert.ToHexString(Data).ToLowerInvariant();
        return d;
    }
}

/// <summary>
/// SH-2 input-report parsers (contracts/05_SENSOR_BNO086.md §5). Byte-exact
/// with the reference Python <c>reports.py</c>. Raw wire integers preserved;
/// scaling (Q points) is the caller's concern.
/// </summary>
public static class Sh2Reports
{
    private const byte BaseTimestampRef = 0xFB; // + i32 base delta (100 µs ticks)
    private const byte TimestampRebase = 0xFA;  // + i32 rebase delta (100 µs ticks)

    public const int GyroIntegratedRvId = 0x2A;

    // Total report length on the wire, 4-byte SH-2 header included.
    private static readonly IReadOnlyDictionary<int, int> ReportLengths = new Dictionary<int, int>
    {
        [0x01] = 10, [0x02] = 10, [0x03] = 10, [0x04] = 10, [0x05] = 14, [0x06] = 10,
        [0x07] = 16, [0x08] = 12, [0x09] = 14, [0x0A] = 8, [0x0B] = 8, [0x0C] = 6,
        [0x0D] = 6, [0x0E] = 6, [0x0F] = 16, [0x10] = 5, [0x11] = 12, [0x12] = 6,
        [0x13] = 6, [0x14] = 16, [0x15] = 16, [0x16] = 16, [0x18] = 8, [0x19] = 6,
        [0x1A] = 6, [0x1B] = 8, [0x1C] = 6, [0x1E] = 16, [0x1F] = 6, [0x20] = 6,
        [0x21] = 6, [0x22] = 6, [0x23] = 6, [0x28] = 14, [0x29] = 12, [0x2A] = 14,
    };

    private static int I16(ReadOnlySpan<byte> p, int off) => BinaryPrimitives.ReadInt16LittleEndian(p[off..]);
    private static int U16(ReadOnlySpan<byte> p, int off) => BinaryPrimitives.ReadUInt16LittleEndian(p[off..]);
    private static uint U32(ReadOnlySpan<byte> p, int off) => BinaryPrimitives.ReadUInt32LittleEndian(p[off..]);
    private static int I32(ReadOnlySpan<byte> p, int off) => BinaryPrimitives.ReadInt32LittleEndian(p[off..]);

    /// <summary>
    /// Parse a channel-3/4 cargo into typed reports. <paramref name="captureTimestampUs"/>
    /// is the bridge RPT_DATA capture time; 0xFB base and 0xFA rebase adjust it.
    /// </summary>
    public static List<BnoReport> ParseInputCargo(byte[] payload, long captureTimestampUs)
    {
        var outList = new List<BnoReport>();
        long baseUs = captureTimestampUs;
        int pos = 0;
        int n = payload.Length;
        while (pos < n)
        {
            byte rid = payload[pos];
            if (rid == BaseTimestampRef && pos + 5 <= n)
            {
                int delta = I32(payload, pos + 1);
                baseUs = captureTimestampUs - (long)delta * 100;
                pos += 5;
                continue;
            }
            if (rid == TimestampRebase && pos + 5 <= n)
            {
                int delta = I32(payload, pos + 1);
                baseUs += (long)delta * 100;
                pos += 5;
                continue;
            }
            if (!ReportLengths.TryGetValue(rid, out int length) || pos + length > n)
            {
                outList.Add(new UnknownReport(rid, baseUs, payload[pos..]));
                break;
            }
            ReadOnlySpan<byte> rep = payload.AsSpan(pos, length);
            int seq = rep[1];
            int status = rep[2];
            int delayLsb = rep[3];
            int accuracy = status & 0x03;
            long delayUs = (long)((((status >> 2) << 8) | delayLsb)) * 100;
            long ts = baseUs + delayUs;
            outList.Add(DecodeReport(rid, rep, ts, seq, accuracy, delayUs));
            pos += length;
        }
        return outList;
    }

    private static BnoReport DecodeReport(int rid, ReadOnlySpan<byte> rep, long ts, int seq, int accuracy, long delayUs)
    {
        switch (rid)
        {
            case 0x01: case 0x04: case 0x06:
                return new Acceleration(rid, ts, seq, accuracy, delayUs, I16(rep, 4), I16(rep, 6), I16(rep, 8));
            case 0x02:
                return new Gyroscope(rid, ts, seq, accuracy, delayUs, I16(rep, 4), I16(rep, 6), I16(rep, 8));
            case 0x03:
                return new Magnetometer(rid, ts, seq, accuracy, delayUs, I16(rep, 4), I16(rep, 6), I16(rep, 8));
            case 0x07:
                return new UncalibratedGyroscope(rid, ts, seq, accuracy, delayUs,
                    I16(rep, 4), I16(rep, 6), I16(rep, 8), I16(rep, 10), I16(rep, 12), I16(rep, 14));
            case 0x0F:
                return new UncalibratedMagnetometer(rid, ts, seq, accuracy, delayUs,
                    I16(rep, 4), I16(rep, 6), I16(rep, 8), I16(rep, 10), I16(rep, 12), I16(rep, 14));
            case 0x05: case 0x09: case 0x28:
                return new RotationVector(rid, ts, seq, accuracy, delayUs,
                    I16(rep, 4), I16(rep, 6), I16(rep, 8), I16(rep, 10), I16(rep, 12));
            case 0x08: case 0x29:
                return new RotationVector(rid, ts, seq, accuracy, delayUs,
                    I16(rep, 4), I16(rep, 6), I16(rep, 8), I16(rep, 10), null);
            case 0x0A: case 0x0B:
                return new ScalarReport(rid, ts, seq, accuracy, delayUs, U32(rep, 4));
            case 0x0C: case 0x0D:
                return new ScalarReport(rid, ts, seq, accuracy, delayUs, U16(rep, 4));
            case 0x0E:
                return new ScalarReport(rid, ts, seq, accuracy, delayUs, I16(rep, 4));
            case 0x10:
                return new TapDetector(rid, ts, seq, accuracy, delayUs, rep[4]);
            case 0x11:
                return new StepCounter(rid, ts, seq, accuracy, delayUs, U32(rep, 4), U16(rep, 8));
            case 0x18:
                return new StepDetector(rid, ts, seq, accuracy, delayUs, U32(rep, 4));
            case 0x12:
                return new SignificantMotion(rid, ts, seq, accuracy, delayUs, U16(rep, 4));
            case 0x13:
                return new StabilityClassifier(rid, ts, seq, accuracy, delayUs, rep[4]);
            case 0x19:
                return new ShakeDetector(rid, ts, seq, accuracy, delayUs, U16(rep, 4));
            case 0x1E:
                return new PersonalActivityClassifier(rid, ts, seq, accuracy, delayUs,
                    rep[4] & 0x7F, (rep[4] & 0x80) != 0 ? 1 : 0, rep[5], ToIntArray(rep.Slice(6, 10)));
            case 0x14: case 0x16:
                return new RawSensor(rid, ts, seq, accuracy, delayUs,
                    I16(rep, 4), I16(rep, 6), I16(rep, 8), U32(rep, 12), 0);
            case 0x15:
                return new RawSensor(rid, ts, seq, accuracy, delayUs,
                    I16(rep, 4), I16(rep, 6), I16(rep, 8), U32(rep, 12), I16(rep, 10));
            default:
                return new GenericEvent(rid, ts, seq, accuracy, delayUs, U16(rep, 4));
        }
    }

    private static int[] ToIntArray(ReadOnlySpan<byte> span)
    {
        var arr = new int[span.Length];
        for (int i = 0; i < span.Length; i++)
            arr[i] = span[i];
        return arr;
    }

    /// <summary>
    /// Parse a channel-5 cargo (gyro-integrated RV, dense). Two shapes: bare
    /// 7×i16, or prefixed with 0xFB + i32 base delta + u16 delay. Returns null
    /// when the cargo is too short.
    /// </summary>
    public static GyroIntegratedRV? ParseGyroRvCargo(byte[] payload, long captureTimestampUs)
    {
        long ts = captureTimestampUs;
        ReadOnlySpan<byte> p = payload;
        if (p.Length >= 1 && p[0] == BaseTimestampRef)
        {
            if (p.Length < 5 + 2 + 14)
                return null;
            int delta = I32(p, 1);
            int delay = U16(p, 5);
            ts = captureTimestampUs - (long)delta * 100 + (long)delay * 100;
            p = p[7..];
        }
        if (p.Length < 14)
            return null;
        return new GyroIntegratedRV(GyroIntegratedRvId, ts,
            I16(p, 0), I16(p, 2), I16(p, 4), I16(p, 6), I16(p, 8), I16(p, 10), I16(p, 12));
    }
}
