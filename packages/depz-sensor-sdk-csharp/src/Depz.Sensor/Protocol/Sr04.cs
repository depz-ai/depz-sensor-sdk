using System.Buffers.Binary;

namespace Depz.Sensor.Protocol;

/// <summary>SR04 command opcodes (contracts/03_SENSOR_SR04.md).</summary>
public enum Sr04Cmd
{
    GetSamplePeriod = 0x32,
    SetSamplePeriod = 0x33,
    GetEchoDecay = 0x34,
    SetEchoDecay = 0x35,
    MeasureOnce = 0x36,
    StartMeasurementLoop = 0x37,
    StopMeasurementLoop = 0x38,
}

/// <summary>SR04 report opcodes.</summary>
public enum Sr04Rpt
{
    Data = 0x91,
    SamplePeriod = 0x92,
    EchoDecay = 0x93,
}

/// <summary>SR04 measurement sample: source cmd (0x36 single / 0x37 loop), timestamp, echo time.</summary>
public sealed record Sr04Data(int SourceCmd, ulong TimestampUs, int EchoTimeUs)
{
    public static Sr04Data Unpack(ReadOnlySpan<byte> payload) => new(
        payload[0],
        BinaryPrimitives.ReadUInt64LittleEndian(payload[1..]),
        BinaryPrimitives.ReadUInt16LittleEndian(payload[9..]));
}

public static class Sr04
{
    /// <summary>echo_time_us sentinel: no echo received.</summary>
    public const int EchoTimeout = 0xFFFF;

    public const int SamplePeriodDefaultUs = 50_000;
    public const int EchoDecayDefaultUs = 5_000;
    public const int EchoDecayMinUs = 4_000;
    public const int EchoDecayMaxUs = 65_000;

    public static byte[] PackSamplePeriod(uint periodUs)
    {
        var buf = new byte[4];
        BinaryPrimitives.WriteUInt32LittleEndian(buf, periodUs);
        return buf;
    }

    public static uint UnpackSamplePeriod(ReadOnlySpan<byte> payload) =>
        BinaryPrimitives.ReadUInt32LittleEndian(payload);

    public static byte[] PackEchoDecay(ushort decayUs)
    {
        var buf = new byte[2];
        BinaryPrimitives.WriteUInt16LittleEndian(buf, decayUs);
        return buf;
    }

    public static ushort UnpackEchoDecay(ReadOnlySpan<byte> payload) =>
        BinaryPrimitives.ReadUInt16LittleEndian(payload);

    /// <summary>
    /// Round-trip echo time → distance in mm; null for the timeout sentinel.
    /// Default speed of sound 343 m/s; with <paramref name="airTempC"/> uses
    /// c = 331.3 + 0.606·T (m/s).
    /// </summary>
    public static double? DistanceMmFromEcho(int echoTimeUs, double? airTempC = null)
    {
        if (echoTimeUs == EchoTimeout)
            return null;
        double cMs = airTempC is null ? 343.0 : 331.3 + 0.606 * airTempC.Value;
        return echoTimeUs * cMs / 2000.0;
    }
}
