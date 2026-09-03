namespace Depz.Sensor.Transport;

/// <summary>
/// CRC algorithms of the DEPZ transport (contracts/01_TRANSPORT_FRAMING.md §3).
///
/// All wire CRCs are reflected table implementations, byte-exact with the
/// firmware and the reference Python SDK. CRC-8 init is 0x00 for every device
/// (contracts/ERRATA.md E1). <see cref="Crc16CcittFalse"/> is the
/// <c>.fwdepz</c>-header-only CRC (contract 06), never on the wire.
/// </summary>
public static class Crc
{
    private static readonly byte[] Crc8Table = MakeTable8(0x8C);
    private static readonly ushort[] Crc16Table = MakeTable16(0xA001);
    private static readonly uint[] Crc32Table = MakeTable32(0xEDB88320);

    private static byte[] MakeTable8(byte polyReflected)
    {
        var table = new byte[256];
        for (int i = 0; i < 256; i++)
        {
            int crc = i;
            for (int b = 0; b < 8; b++)
                crc = (crc & 1) != 0 ? (crc >> 1) ^ polyReflected : crc >> 1;
            table[i] = (byte)crc;
        }
        return table;
    }

    private static ushort[] MakeTable16(ushort polyReflected)
    {
        var table = new ushort[256];
        for (int i = 0; i < 256; i++)
        {
            int crc = i;
            for (int b = 0; b < 8; b++)
                crc = (crc & 1) != 0 ? (crc >> 1) ^ polyReflected : crc >> 1;
            table[i] = (ushort)crc;
        }
        return table;
    }

    private static uint[] MakeTable32(uint polyReflected)
    {
        var table = new uint[256];
        for (int i = 0; i < 256; i++)
        {
            uint crc = (uint)i;
            for (int b = 0; b < 8; b++)
                crc = (crc & 1) != 0 ? (crc >> 1) ^ polyReflected : crc >> 1;
            table[i] = crc;
        }
        return table;
    }

    /// <summary>CRC-8/MAXIM: poly 0x31 reflected, init 0x00, xorout 0x00.</summary>
    public static byte Crc8Maxim(ReadOnlySpan<byte> data)
    {
        byte crc = 0x00;
        foreach (byte b in data)
            crc = Crc8Table[crc ^ b];
        return crc;
    }

    /// <summary>CRC-16/MODBUS: poly 0x8005 reflected, init 0xFFFF, xorout 0x0000.</summary>
    public static ushort Crc16Modbus(ReadOnlySpan<byte> data)
    {
        ushort crc = 0xFFFF;
        foreach (byte b in data)
            crc = (ushort)((crc >> 8) ^ Crc16Table[(crc ^ b) & 0xFF]);
        return crc;
    }

    /// <summary>CRC-32/ISO-HDLC: poly 0x04C11DB7 reflected, init/xorout 0xFFFFFFFF.</summary>
    public static uint Crc32IsoHdlc(ReadOnlySpan<byte> data)
    {
        uint crc = 0xFFFFFFFF;
        foreach (byte b in data)
            crc = (crc >> 8) ^ Crc32Table[(crc ^ b) & 0xFF];
        return crc ^ 0xFFFFFFFF;
    }

    /// <summary>
    /// CRC-16/CCITT-FALSE: poly 0x1021, init 0xFFFF, not reflected.
    /// Used only for the <c>.fwdepz</c> file header (contract 06), never on the wire.
    /// </summary>
    public static ushort Crc16CcittFalse(ReadOnlySpan<byte> data)
    {
        int crc = 0xFFFF;
        foreach (byte b in data)
        {
            crc ^= b << 8;
            for (int i = 0; i < 8; i++)
                crc = (crc & 0x8000) != 0 ? ((crc << 1) ^ 0x1021) & 0xFFFF : (crc << 1) & 0xFFFF;
        }
        return (ushort)crc;
    }
}
