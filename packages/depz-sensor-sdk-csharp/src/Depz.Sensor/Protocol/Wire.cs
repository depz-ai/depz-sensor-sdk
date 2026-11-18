namespace Depz.Sensor.Protocol;

/// <summary>
/// Wire-field checks shared by the codecs (contract 07 §3). Encoding: every
/// argument is checked against its wire width before a byte is written — never
/// truncated, wrapped or masked (<see cref="ArgumentOutOfRangeException"/>, the
/// SDK's <c>invalid_argument</c>). Decoding: a payload shorter than the fields
/// the contract defines is rejected (<see cref="ArgumentException"/>); trailing
/// bytes are ignored by the callers.
/// </summary>
public static class Wire
{
    /// <summary><paramref name="value"/> as an unsigned integer that fits <paramref name="bits"/> (≤ 63) bits.</summary>
    /// <exception cref="ArgumentOutOfRangeException">outside 0..2^bits−1.</exception>
    public static long CheckUint(string name, long value, int bits)
    {
        if (value < 0 || value > (1L << bits) - 1)
            throw new ArgumentOutOfRangeException(name, value, $"{name}: {value} out of range for u{bits}");
        return value;
    }

    /// <summary><paramref name="value"/> as a signed integer that fits <paramref name="bits"/> (≤ 32) bits.</summary>
    /// <exception cref="ArgumentOutOfRangeException">outside −2^(bits−1)..2^(bits−1)−1.</exception>
    public static int CheckInt(string name, long value, int bits)
    {
        long lim = 1L << (bits - 1);
        if (value < -lim || value > lim - 1)
            throw new ArgumentOutOfRangeException(name, value, $"{name}: {value} out of range for i{bits}");
        return (int)value;
    }

    /// <summary>Reject a device payload shorter than the <paramref name="n"/> bytes its fields need.</summary>
    /// <exception cref="ArgumentException">when the payload is shorter.</exception>
    public static void NeedBytes(string what, ReadOnlySpan<byte> payload, int n)
    {
        if (payload.Length < n)
            throw new ArgumentException($"{what}: need {n} bytes, got {payload.Length}");
    }
}
