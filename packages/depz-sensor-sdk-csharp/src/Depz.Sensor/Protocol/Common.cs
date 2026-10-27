using System.Buffers.Binary;
using System.Text;

namespace Depz.Sensor.Protocol;

/// <summary>Common command opcodes (contracts/02_COMMON_COMMANDS.md).</summary>
public enum Cmd
{
    Bootloader = 0x01,
    DeviceReset = 0x02,
    GetDeviceName = 0x03,
    GetNameActiveSoftware = 0x04,
    GetSerial = 0x05,
    SyncTime = 0x06,
    GetMcuTemperature = 0x07,
    GetPayloadCrcType = 0x08,
    SetPayloadCrcType = 0x09,
    UsbTestTxStart = 0x1C,
    UsbTestTxStop = 0x1D,
    UsbTestRxData = 0x1E,
    UsbTestStats = 0x1F,
    GetSyncPinConfig = 0x30,
    SetSyncPinConfig = 0x31,
}

/// <summary>Common report opcodes.</summary>
public enum Rpt
{
    Status = 0x80,
    Text = 0x81,
    SyncTime = 0x82,
    Temperature = 0x83,
    SequenceError = 0x84,
    PayloadCrcType = 0x87,
    UsbTestData = 0x88,
    UsbTestStats = 0x8F,
    SyncPinConfig = 0x90,
}

public enum Status
{
    Ok = 0x00,
    Error = 0x01,
    ErrInvalidCmd = 0x02,
    ErrPayloadFormat = 0x03,
    ErrInvalidParam = 0x04,
    ErrPayloadCrc = 0x05,
    ErrBusy = 0x06,
    ErrCmdNotSupported = 0x07,
    ErrNotInitialized = 0x08,
    ErrHardwareFault = 0x09,
}

public enum SyncPinMode
{
    Disable = 0x00,
    In = 0x01,
    OutStart = 0x02,
    OutEnd = 0x03,
    OutBoth = 0x04,
}

public enum SyncPinPolarity
{
    IdleLow = 0x00,
    IdleHigh = 0x01,
}

/// <summary>Echoed-cmd byte value for an unsolicited report.</summary>
public static class Reports
{
    public const int Unsolicited = 0x00;
}

/// <summary>Status report: echoed request opcode (0x00 = unsolicited) + status code.</summary>
public sealed record StatusReport(int Cmd, int Status)
{
    public static StatusReport Unpack(ReadOnlySpan<byte> payload)
    {
        Wire.NeedBytes("RPT_STATUS", payload, 2);
        return new(payload[0], payload[1]);
    }
}

/// <summary>Text report: echoed opcode + ASCII string (trailing 0x00/0xFF stripped).</summary>
public sealed record TextReport(int Cmd, string Text)
{
    public static TextReport Unpack(ReadOnlySpan<byte> payload)
    {
        Wire.NeedBytes("RPT_TEXT", payload, 1);
        return new(payload[0], Common.StripDeviceString(payload[1..]));
    }
}

/// <summary>Sync-time report: T1 (echoed), T2 (mcu rx), T3 (mcu tx), all µs.</summary>
public sealed record SyncTimeReport(ulong PcTimestampUs, ulong McuRxUs, ulong McuTxUs)
{
    public static SyncTimeReport Unpack(ReadOnlySpan<byte> payload)
    {
        Wire.NeedBytes("RPT_SYNC_TIME", payload, 24);
        return new(
            BinaryPrimitives.ReadUInt64LittleEndian(payload),
            BinaryPrimitives.ReadUInt64LittleEndian(payload[8..]),
            BinaryPrimitives.ReadUInt64LittleEndian(payload[16..]));
    }
}

/// <summary>Temperature report: timestamp µs + raw int16 in units of 0.1 °C.</summary>
public sealed record TemperatureReport(ulong TimestampUs, short RawDecidegrees)
{
    public double Celsius => RawDecidegrees / 10.0;

    public static TemperatureReport Unpack(ReadOnlySpan<byte> payload)
    {
        Wire.NeedBytes("RPT_TEMPERATURE", payload, 10);
        return new(
            BinaryPrimitives.ReadUInt64LittleEndian(payload),
            BinaryPrimitives.ReadInt16LittleEndian(payload[8..]));
    }
}

/// <summary>Sequence-error report: expected vs received seq bytes.</summary>
public sealed record SequenceErrorReport(int ExpectedSeq, int ReceivedSeq)
{
    public static SequenceErrorReport Unpack(ReadOnlySpan<byte> payload)
    {
        Wire.NeedBytes("RPT_SEQUENCE_ERROR", payload, 2);
        return new(payload[0], payload[1]);
    }
}

/// <summary>Sync-pin configuration (pin 1..5, mode, polarity).</summary>
public sealed record SyncPinConfig(int Pin, SyncPinMode Mode, SyncPinPolarity Polarity)
{
    /// <summary>First / last logical AUX sync pin (contract 02 §6).</summary>
    public const int PinMin = 1, PinMax = 5;

    /// <summary>
    /// SET_SYNC_PIN_CONFIG payload. Contract 02 §6: pin 1..5, mode a
    /// <see cref="SyncPinMode"/> (0..4), polarity a <see cref="SyncPinPolarity"/> (0..1).
    /// </summary>
    /// <exception cref="ArgumentOutOfRangeException">a field outside those — nothing is written.</exception>
    public byte[] Pack()
    {
        if (Pin < PinMin || Pin > PinMax)
            throw new ArgumentOutOfRangeException(nameof(Pin), Pin, $"sync pin must be {PinMin}..{PinMax}");
        if (!Enum.IsDefined(typeof(SyncPinMode), Mode))
            throw new ArgumentOutOfRangeException(nameof(Mode), Mode, "not a SyncPinMode (0..4)");
        if (!Enum.IsDefined(typeof(SyncPinPolarity), Polarity))
            throw new ArgumentOutOfRangeException(nameof(Polarity), Polarity, "not a SyncPinPolarity (0..1)");
        return new[] { (byte)Pin, (byte)Mode, (byte)Polarity };
    }

    public static SyncPinConfig Unpack(ReadOnlySpan<byte> payload)
    {
        Wire.NeedBytes("RPT_SYNC_PIN_CONFIG", payload, 3);
        return new(payload[0], (SyncPinMode)payload[1], (SyncPinPolarity)payload[2]);
    }
}

/// <summary>Common command/report payload codecs (contract 02).</summary>
public static class Common
{
    /// <summary>Encode a SYNC_TIME request payload (u64 LE µs).</summary>
    public static byte[] PackSyncTime(ulong pcTimestampUs)
    {
        var buf = new byte[8];
        BinaryPrimitives.WriteUInt64LittleEndian(buf, pcTimestampUs);
        return buf;
    }

    /// <summary>SET_PAYLOAD_CRC_TYPE takes 0..3 (contract 02 §2).</summary>
    public const int PayloadCrcTypeMax = 3;

    /// <summary>SET_PAYLOAD_CRC_TYPE payload: crc_type u8, 0..3 (contract 02 §2) — checked before anything is written (contract 07 §3).</summary>
    /// <exception cref="ArgumentOutOfRangeException">Not a CrcType 0..3.</exception>
    public static byte[] PackSetPayloadCrcType(int crcType)
    {
        if (crcType < 0 || crcType > PayloadCrcTypeMax)
            throw new ArgumentOutOfRangeException(nameof(crcType), crcType, "not a CrcType (0..3)");
        return new[] { (byte)crcType };
    }

    /// <summary>
    /// NTP-style clock math, all µs (contract 02 §5). Returns (offsetUs, rttUs)
    /// where offset = device_clock - host_clock, computed with truncation toward
    /// zero on the sum (C# long division already truncates toward zero).
    /// </summary>
    public static (long OffsetUs, long RttUs) SyncTimeOffsetRtt(long t1, long t2, long t3, long t4)
    {
        long num = (t2 - t1) + (t3 - t4);
        long offset = num / 2;
        long rtt = (t4 - t1) - (t3 - t2);
        return (offset, rtt);
    }

    /// <summary>
    /// Decode an ASCII device string, dropping trailing NUL/0xFF filler. Every
    /// byte ≥ 0x80 becomes U+FFFD (contract 02 §4.1: never '?', never UTF-8 or
    /// Latin-1); an embedded 0x00 before the filler is kept as a character.
    /// </summary>
    public static string StripDeviceString(ReadOnlySpan<byte> raw)
    {
        int end = raw.Length;
        while (end > 0 && (raw[end - 1] == 0x00 || raw[end - 1] == 0xFF))
            end--;
        var sb = new StringBuilder(end);
        foreach (byte b in raw[..end])
            sb.Append(b < 0x80 ? (char)b : '\uFFFD');
        return sb.ToString();
    }
}
