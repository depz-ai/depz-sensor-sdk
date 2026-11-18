namespace Depz.Sensor.Vl53l4;

/// <summary>What the sensor did, not what the caller asked: a boot or data-ready timeout, a short
/// result block, an oscillator word of 0, a failed xtalk calibration. A refused argument is an
/// <see cref="ArgumentException"/> (contract 07 §3).</summary>
public sealed class Vl53l4Exception : Exception
{
    public Vl53l4Exception(string message) : base(message) { }
}

/// <summary>
/// VL53L4CD_ResultsData_t plus the sensor's own frame counter.
/// <see cref="StreamCount"/> (RESULT__STREAM_COUNT, wraps at 255) tells a frame
/// the host never received from one the sensor never produced.
/// </summary>
public sealed record Vl53l4Results(
    int RangeStatus,
    int DistanceMm,
    int AmbientRateKcps,
    int AmbientPerSpadKcps,
    int SignalRateKcps,
    int SignalPerSpadKcps,
    int NumberOfSpad,
    int SigmaMm,
    int StreamCount);
