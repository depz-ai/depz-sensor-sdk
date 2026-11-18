using Depz.Sensor.Protocol;
using Depz.Sensor.Transport;

namespace Depz.Sensor.Devices;

/// <summary>One SR04 ranging result. <see cref="Source"/> is <c>"once"</c> (a host command or a
/// SYNC_IN edge) or <c>"loop"</c>; <see cref="Valid"/> is false for the no-echo timeout.</summary>
public sealed record Sr04Measurement(ulong TimestampUs, int EchoTimeUs, string Source)
{
    public bool Valid => EchoTimeUs != Protocol.Sr04.EchoTimeout;

    /// <summary>Distance at 343 m/s, or null when no echo came back.</summary>
    public double? DistanceMm => Protocol.Sr04.DistanceMmFromEcho(EchoTimeUs);

    /// <summary>Distance with the speed of sound for <paramref name="airTempC"/> °C.</summary>
    public double? DistanceMmAt(double airTempC) => Protocol.Sr04.DistanceMmFromEcho(EchoTimeUs, airTempC);

    /// <summary>Null for a source cmd contract 03 §2 does not define (0x36 / 0x37 only, ERRATA
    /// B11): not a measurement, it falls through to the hex text event.</summary>
    internal static Sr04Measurement? From(Sr04Data d) => d.SourceCmd switch
    {
        (int)Sr04Cmd.MeasureOnce => new(d.TimestampUs, d.EchoTimeUs, "once"),
        (int)Sr04Cmd.StartMeasurementLoop => new(d.TimestampUs, d.EchoTimeUs, "loop"),
        _ => null,
    };
}

/// <summary>
/// HC-SR04 ultrasonic ranger (contract 03). Measurements stream to
/// <see cref="Measurements"/>: loop samples and the single shots an AUX SYNC_IN edge triggers.
/// </summary>
public sealed class Sr04Device : DepzDevice
{
    public Sr04Device(ILink link, TimeSpan? timeout = null) : base(link, timeout) =>
        Measurements = Stream<Sr04Measurement>((int)Sr04Rpt.Data, defaultCapacity: 256);

    internal Sr04Device(DepzDevice from) : base(from) =>
        Measurements = Stream<Sr04Measurement>((int)Sr04Rpt.Data, defaultCapacity: 256);

    /// <summary>The measurement stream (contract 07 §3): <c>Measurements.On(cb)</c>, <c>Measurements.Subscribe()</c>
    /// (256 by default), <c>Measurements.Next()</c>; <c>ParseErrors</c> counts RPT_DATA reports shorter than their
    /// 11 bytes, dropped (contract 07 §2).</summary>
    public SensorStream<Sr04Measurement> Measurements { get; }

    // ── configuration ────────────────────────────────────────────────────────

    /// <summary>The stored minimum interval between measurement starts, µs (default 50000) —
    /// not the effective rate, which the echo window throttles (contract 03 §3).</summary>
    public long GetSamplePeriodUs() =>
        Request((int)Sr04Cmd.GetSamplePeriod, null,
            ExpectReport((int)Sr04Rpt.SamplePeriod, p => Protocol.Sr04.UnpackSamplePeriod(p)));

    /// <summary>Set the minimum interval between measurement starts, µs (u32 wire field).</summary>
    public void SetSamplePeriodUs(long periodUs)
    {
        if (periodUs < 0 || periodUs > uint.MaxValue)
            throw new ArgumentOutOfRangeException(nameof(periodUs), periodUs,
                $"sample period must be 0..{uint.MaxValue} µs (u32 wire field, contract 03 §2)");
        RequestOk((int)Sr04Cmd.SetSamplePeriod, Protocol.Sr04.PackSamplePeriod((uint)periodUs));
    }

    /// <summary>The post-echo settle pause, µs (4000–65000).</summary>
    public int GetEchoDecayUs() =>
        Request((int)Sr04Cmd.GetEchoDecay, null,
            ExpectReport((int)Sr04Rpt.EchoDecay, p => (int)Protocol.Sr04.UnpackEchoDecay(p)));

    /// <summary>Set the settle pause. The device clamps it to 4000–65000 µs silently, so this
    /// reads it back and returns the value in effect. Must fit the u16 wire field.</summary>
    public int SetEchoDecayUs(int decayUs)
    {
        if (decayUs < 0 || decayUs > ushort.MaxValue)
            throw new ArgumentOutOfRangeException(nameof(decayUs), decayUs,
                $"echo decay must be 0..{ushort.MaxValue} µs (u16 wire field; the device then clamps to "
                + $"{Protocol.Sr04.EchoDecayMinUs}..{Protocol.Sr04.EchoDecayMaxUs}, contract 03 §3)");
        RequestOk((int)Sr04Cmd.SetEchoDecay, Protocol.Sr04.PackEchoDecay((ushort)decayUs));
        return GetEchoDecayUs();
    }

    // ── measuring ────────────────────────────────────────────────────────────

    /// <summary>One shot. The reply comes when the echo completes (or times out at ~65.5 ms),
    /// hence the 1 s default. <see cref="BusyException"/> while the loop runs.</summary>
    public Sr04Measurement MeasureOnce(TimeSpan? timeout = null)
    {
        static bool Match(Packet p, out Sr04Measurement m)
        {
            m = null!;
            // the reply is the RPT_DATA sourced by this command; a short one fails the request
            // (InvalidReplyException, contract 07 §3), no timeout
            if (p.Cmd != (int)Sr04Rpt.Data || p.Payload.Length < 1 || p.Payload[0] != (byte)Sr04Cmd.MeasureOnce)
                return false;
            m = Sr04Measurement.From(Sr04Data.Unpack(p.Payload))!;
            return true;
        }
        return Request<Sr04Measurement>((int)Sr04Cmd.MeasureOnce, null, Match, timeout ?? TimeSpan.FromSeconds(1));
    }

    /// <summary>Start the measurement loop (idempotent).</summary>
    public void Start() => RequestOk((int)Sr04Cmd.StartMeasurementLoop);

    /// <summary>Stop the measurement loop (idempotent).</summary>
    public void Stop() => RequestOk((int)Sr04Cmd.StopMeasurementLoop);

    private protected override bool HandleReport(Packet packet)
    {
        if (packet.Cmd != (int)Sr04Rpt.Data)
            return false;
        Sr04Measurement? m;
        try
        {
            // short: refused and counted — contract 07 §2
            m = Sr04Measurement.From(Sr04Data.Unpack(packet.Payload));
        }
        catch (ArgumentException)
        {
            Measurements.CountParseError();
            return true;
        }
        if (m is null)
            return false;
        Measurements.Emit(m);
        return true;
    }
}
