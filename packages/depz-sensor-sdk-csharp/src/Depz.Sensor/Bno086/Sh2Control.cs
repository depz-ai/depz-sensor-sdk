using System.Buffers.Binary;

namespace Depz.Sensor.Bno086;

/// <summary>
/// Set Feature <c>flags</c> bits (SH-2 §6.5.4) for
/// <see cref="Sh2Control.SetFeature"/>. <c>Wakeup | AlwaysOn</c> keeps a sensor
/// reporting through <see cref="Sh2Control.Sleep"/>: on a DEPZ BNO086
/// (SH-2 3.12.6, 2026-10-16) such a report kept its full rate during sleep,
/// delivered on SHTP channel 4 (wake input), while every other report stopped.
/// </summary>
public static class FeatureFlags
{
    /// <summary>Bit 2: wake-up enabled — the sensor reports on channel 4.</summary>
    public const byte Wakeup = 0x04;

    /// <summary>Bit 3: always-on — the sensor keeps running while the hub sleeps.</summary>
    public const byte AlwaysOn = 0x08;
}

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
    /// <exception cref="ArgumentOutOfRangeException"><paramref name="sensorId"/> / <paramref name="flags"/>
    /// outside u8 or <paramref name="sensitivity"/> outside u16 — nothing is written.</exception>
    public static byte[] SetFeature(
        int sensorId, uint intervalUs, uint batchUs = 0, int sensitivity = 0, int flags = 0, uint cfgWord = 0)
    {
        Protocol.Wire.CheckUint("sensorId", sensorId, 8);
        Protocol.Wire.CheckUint("flags", flags, 8);
        Protocol.Wire.CheckUint("sensitivity", sensitivity, 16);
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
    /// <exception cref="ArgumentOutOfRangeException"><paramref name="sensorId"/> outside u8.</exception>
    public static byte[] GetFeature(int sensorId) =>
        new[] { GetFeatureRequest, (byte)Protocol.Wire.CheckUint("sensorId", sensorId, 8) };

    /// <summary>0xF9 Product ID Request.</summary>
    public static byte[] ProductId() => new byte[] { ProductIdRequest, 0x00 };

    /// <summary>
    /// SH-2 "Sleep" for the <b>executable</b> channel (channel 1, not control):
    /// <c>[0x03]</c>. Frame it with
    /// <c>NextFrame((int)ShtpChannel.Executable, Sh2Control.Sleep())</c> and send
    /// it through the bridge's SEND_SHTP_PACKET (0x34); the hub sends no
    /// response. Every report stops, except those enabled with
    /// <see cref="FeatureFlags.Wakeup"/> | <see cref="FeatureFlags.AlwaysOn"/>,
    /// which keep their rate; the configuration (enabled sensors, rates) is
    /// kept. <see cref="PowerOn"/> ends it. On a DEPZ BNO086 (SH-2 3.12.6,
    /// 2026-10-16) all reports stopped within one sample; a Wakeup|AlwaysOn
    /// report kept streaming at full rate on channel 4.
    /// </summary>
    public static byte[] Sleep() => new[] { (byte)ExecutableCommand.Sleep };

    /// <summary>
    /// SH-2 "On" for the <b>executable</b> channel (channel 1): <c>[0x02]</c> —
    /// ends <see cref="Sleep"/>. Every report that was enabled comes back at its
    /// rate, without re-enabling; the hub sends no response. Not the bridge's
    /// SENSOR_WAKE_UP (0x33), which pulses the WAKE/PS0 pin — a hardware
    /// signal, not an SH-2 command.
    /// </summary>
    public static byte[] PowerOn() => new[] { (byte)ExecutableCommand.On };

    /// <summary>
    /// 0xF2 Command Request: [id, seq, command] + params, zero-padded to 12
    /// bytes (9 param slots).
    /// </summary>
    public static byte[] Command(int seq, int command, ReadOnlySpan<byte> parameters)
    {
        if (parameters.Length > 9)
            throw new ArgumentException("command params must be <= 9 bytes", nameof(parameters));
        Protocol.Wire.CheckUint("seq", seq, 8);
        Protocol.Wire.CheckUint("command", command, 8);
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
    /// <exception cref="ArgumentOutOfRangeException">an argument outside u16.</exception>
    public static byte[] FrsRead(int frsType, int offsetWords = 0, int blockWords = 0)
    {
        Protocol.Wire.CheckUint("offsetWords", offsetWords, 16);
        Protocol.Wire.CheckUint("frsType", frsType, 16);
        Protocol.Wire.CheckUint("blockWords", blockWords, 16);
        var p = new byte[8];
        p[0] = FrsReadRequest;
        p[1] = 0x00;
        BinaryPrimitives.WriteUInt16LittleEndian(p.AsSpan(2), (ushort)offsetWords);
        BinaryPrimitives.WriteUInt16LittleEndian(p.AsSpan(4), (ushort)frsType);
        BinaryPrimitives.WriteUInt16LittleEndian(p.AsSpan(6), (ushort)blockWords);
        return p;
    }

    /// <summary>0xF7 FRS Write Request: reserved, u16 length (words), u16 FRS type. 6-byte payload.</summary>
    /// <exception cref="ArgumentOutOfRangeException">an argument outside u16.</exception>
    public static byte[] FrsWrite(int frsType, int lengthWords)
    {
        Protocol.Wire.CheckUint("lengthWords", lengthWords, 16);
        Protocol.Wire.CheckUint("frsType", frsType, 16);
        var p = new byte[6];
        p[0] = FrsWriteRequest;
        p[1] = 0x00;
        BinaryPrimitives.WriteUInt16LittleEndian(p.AsSpan(2), (ushort)lengthWords);
        BinaryPrimitives.WriteUInt16LittleEndian(p.AsSpan(4), (ushort)frsType);
        return p;
    }

    /// <summary>
    /// 0xF6 FRS Write Data: reserved, u16 word offset, then two u32 data words —
    /// always 12 bytes (contract 05 §6); one word leaves the second slot zero.
    /// </summary>
    /// <exception cref="ArgumentException">not 1 or 2 words.</exception>
    /// <exception cref="ArgumentOutOfRangeException"><paramref name="offsetWords"/> outside u16.</exception>
    public static byte[] FrsWriteDataReport(int offsetWords, IReadOnlyList<uint> words)
    {
        if (words.Count < 1 || words.Count > 2)
            throw new ArgumentException("FRS write data carries 1 or 2 words", nameof(words));
        Protocol.Wire.CheckUint("offsetWords", offsetWords, 16);
        var p = new byte[12];
        p[0] = FrsWriteData;
        p[1] = 0x00;
        BinaryPrimitives.WriteUInt16LittleEndian(p.AsSpan(2), (ushort)offsetWords);
        for (int i = 0; i < words.Count; i++)
            BinaryPrimitives.WriteUInt32LittleEndian(p.AsSpan(4 + i * 4), words[i]);
        return p;
    }
}
