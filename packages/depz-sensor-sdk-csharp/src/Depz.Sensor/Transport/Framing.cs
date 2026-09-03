namespace Depz.Sensor.Transport;

/// <summary>
/// Packet framing (contracts/01_TRANSPORT_FRAMING.md). Byte-exact with the
/// firmware and the reference Python SDK, including the
/// empty-payload-never-carries-CRC rule (contracts/ERRATA.md E6).
/// </summary>
public static class Framing
{
    public static readonly byte[] Magic = { 0xA5, 0xC3 };
    public const int HeaderSize = 7;
    public const int MaxPayload = 0x3FFF;

    /// <summary>CRC trailer for a payload; empty payloads never carry CRC bytes (E6).</summary>
    public static byte[] PayloadCrcBytes(CrcType crcType, ReadOnlySpan<byte> payload)
    {
        if (crcType == CrcType.None || payload.Length == 0)
            return Array.Empty<byte>();
        switch (crcType)
        {
            case CrcType.Crc8:
                return new[] { Crc.Crc8Maxim(payload) };
            case CrcType.Crc16:
            {
                ushort c = Crc.Crc16Modbus(payload);
                return new[] { (byte)(c & 0xFF), (byte)(c >> 8) };
            }
            default: // Crc32
            {
                uint c = Crc.Crc32IsoHdlc(payload);
                return new[]
                {
                    (byte)(c & 0xFF),
                    (byte)((c >> 8) & 0xFF),
                    (byte)((c >> 16) & 0xFF),
                    (byte)((c >> 24) & 0xFF),
                };
            }
        }
    }

    /// <summary>
    /// Frame one packet. The <paramref name="crcType"/> bits are set in the
    /// header even for an empty payload (matching device TX), but CRC bytes are
    /// only appended for non-empty payloads (E6). <paramref name="seq"/> is
    /// taken modulo 256.
    /// </summary>
    public static byte[] BuildPacket(int cmd, ReadOnlySpan<byte> payload = default, int seq = 0, CrcType crcType = CrcType.None)
    {
        if (payload.Length > MaxPayload)
            throw new ArgumentException($"payload too long: {payload.Length} > {MaxPayload}", nameof(payload));

        int dataSize = payload.Length | ((int)crcType << 14);
        byte dsLo = (byte)(dataSize & 0xFF);
        byte dsHi = (byte)((dataSize >> 8) & 0xFF);
        byte cmdByte = (byte)(cmd & 0xFF);
        byte seqByte = (byte)(seq & 0xFF);
        byte hdrCrc = Crc.Crc8Maxim(new[] { dsLo, dsHi, cmdByte, seqByte });

        byte[] trailer = PayloadCrcBytes(crcType, payload);
        var frame = new byte[HeaderSize + payload.Length + trailer.Length];
        frame[0] = Magic[0];
        frame[1] = Magic[1];
        frame[2] = dsLo;
        frame[3] = dsHi;
        frame[4] = cmdByte;
        frame[5] = seqByte;
        frame[6] = hdrCrc;
        payload.CopyTo(frame.AsSpan(HeaderSize));
        trailer.CopyTo(frame.AsSpan(HeaderSize + payload.Length));
        return frame;
    }
}
