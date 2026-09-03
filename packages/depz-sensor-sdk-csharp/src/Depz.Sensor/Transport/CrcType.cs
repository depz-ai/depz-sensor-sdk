namespace Depz.Sensor.Transport;

/// <summary>Payload CRC type carried in the two high bits of the data-size field.</summary>
public enum CrcType
{
    None = 0,
    Crc8 = 1,
    Crc16 = 2,
    Crc32 = 3,
}

public static class CrcTypeExtensions
{
    /// <summary>Size in bytes of the CRC trailer for a non-empty payload.</summary>
    public static int Size(this CrcType crcType) => crcType switch
    {
        CrcType.None => 0,
        CrcType.Crc8 => 1,
        CrcType.Crc16 => 2,
        CrcType.Crc32 => 4,
        _ => throw new ArgumentOutOfRangeException(nameof(crcType)),
    };
}
