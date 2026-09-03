using System.Buffers.Binary;

namespace Depz.Sensor.Bno086;

/// <summary>
/// SH-2 control-report encoders (contracts/05_SENSOR_BNO086.md §6). Each method
/// returns the header-less cargo payload for the control channel (channel 2);
/// wrap it with <see cref="ShtpLayer.NextFrame"/> to frame it. Byte-exact with
/// the golden vectors.
/// </summary>
public static class Sh2Control
{
    // Control report IDs.
    public const byte SetFeatureCommand = 0xFD;
    public const byte GetFeatureRequest = 0xFE;
    public const byte ProductIdRequest = 0xF9;
    public const byte CommandRequest = 0xF2;
    public const byte FrsReadRequest = 0xF4;
    public const byte FrsWriteRequest = 0xF7;
    public const byte FrsWriteData = 0xF6;

    /// <summary>
    /// 0xFD Set Feature Command: enable/configure a sensor. 17-byte payload —
    /// id, flags, u16 change-sensitivity, u32 report interval µs, u32 batch
    /// interval µs, u32 sensor-specific config.
    /// </summary>
    public static byte[] SetFeature(
        int sensorId, uint intervalUs, uint batchUs = 0, int sensitivity = 0, int flags = 0, uint cfgWord = 0)
    {
        var p = new byte[17];
        p[0] = SetFeatureCommand;
        p[1] = (byte)sensorId;
        p[2] = (byte)flags;
        BinaryPrimitives.WriteUInt16LittleEndian(p.AsSpan(3), (ushort)sensitivity);
        BinaryPrimitives.WriteUInt32LittleEndian(p.AsSpan(5), intervalUs);
        BinaryPrimitives.WriteUInt32LittleEndian(p.AsSpan(9), batchUs);
        BinaryPrimitives.WriteUInt32LittleEndian(p.AsSpan(13), cfgWord);
        return p;
    }

    /// <summary>0xFE Get Feature Request.</summary>
    public static byte[] GetFeature(int sensorId) => new[] { GetFeatureRequest, (byte)sensorId };

    /// <summary>0xF9 Product ID Request.</summary>
    public static byte[] ProductId() => new byte[] { ProductIdRequest, 0x00 };

    /// <summary>
    /// 0xF2 Command Request: [id, seq, command] + params, zero-padded to 12
    /// bytes (9 param slots).
    /// </summary>
    public static byte[] Command(int seq, int command, ReadOnlySpan<byte> parameters)
    {
        if (parameters.Length > 9)
            throw new ArgumentException("command params must be <= 9 bytes", nameof(parameters));
        var p = new byte[12];
        p[0] = CommandRequest;
        p[1] = (byte)seq;
        p[2] = (byte)command;
        parameters.CopyTo(p.AsSpan(3));
        return p;
    }

    /// <summary>
    /// 0xF4 FRS Read Request: reserved, u16 read offset (words), u16 FRS type,
    /// u16 block size (words). 8-byte payload.
    /// </summary>
    public static byte[] FrsRead(int frsType, int offsetWords = 0, int blockWords = 0)
    {
        var p = new byte[8];
        p[0] = FrsReadRequest;
        p[1] = 0x00;
        BinaryPrimitives.WriteUInt16LittleEndian(p.AsSpan(2), (ushort)offsetWords);
        BinaryPrimitives.WriteUInt16LittleEndian(p.AsSpan(4), (ushort)frsType);
        BinaryPrimitives.WriteUInt16LittleEndian(p.AsSpan(6), (ushort)blockWords);
        return p;
    }

    /// <summary>0xF7 FRS Write Request: reserved, u16 length (words), u16 FRS type. 6-byte payload.</summary>
    public static byte[] FrsWrite(int frsType, int lengthWords)
    {
        var p = new byte[6];
        p[0] = FrsWriteRequest;
        p[1] = 0x00;
        BinaryPrimitives.WriteUInt16LittleEndian(p.AsSpan(2), (ushort)lengthWords);
        BinaryPrimitives.WriteUInt16LittleEndian(p.AsSpan(4), (ushort)frsType);
        return p;
    }

    /// <summary>
    /// 0xF6 FRS Write Data: reserved, u16 word offset, then the u32 data words.
    /// </summary>
    public static byte[] FrsWriteDataReport(int offsetWords, IReadOnlyList<uint> words)
    {
        var p = new byte[4 + words.Count * 4];
        p[0] = FrsWriteData;
        p[1] = 0x00;
        BinaryPrimitives.WriteUInt16LittleEndian(p.AsSpan(2), (ushort)offsetWords);
        for (int i = 0; i < words.Count; i++)
            BinaryPrimitives.WriteUInt32LittleEndian(p.AsSpan(4 + i * 4), words[i]);
        return p;
    }
}
