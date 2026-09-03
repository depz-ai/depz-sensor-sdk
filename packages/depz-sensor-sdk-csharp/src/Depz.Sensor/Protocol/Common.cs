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
    ThroughputTxStart = 0x1C,
    ThroughputTxStop = 0x1D,
    ThroughputRxData = 0x1E,
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
    ThroughputData = 0x88,
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
    public static StatusReport Unpack(ReadOnlySpan<byte> payload) => new(payload[0], payload[1]);
}

/// <summary>Text report: echoed opcode + ASCII string (trailing 0x00/0xFF stripped).</summary>
public sealed record TextReport(int Cmd, string Text)
{
    public static TextReport Unpack(ReadOnlySpan<byte> payload) =>
        new(payload[0], Common.StripDeviceString(payload[1..]));
}

/// <summary>Sync-time report: T1 (echoed), T2 (mcu rx), T3 (mcu tx), all µs.</summary>
public sealed record SyncTimeReport(ulong PcTimestampUs, ulong McuRxUs, ulong McuTxUs)
{
    public static SyncTimeReport Unpack(ReadOnlySpan<byte> payload) => new(
        BinaryPrimitives.ReadUInt64LittleEndian(payload),
        BinaryPrimitives.ReadUInt64LittleEndian(payload[8..]),
        BinaryPrimitives.ReadUInt64LittleEndian(payload[16..]));
}

/// <summary>Temperature report: timestamp µs + raw int16 in units of 0.1 °C.</summary>
public sealed record TemperatureReport(ulong TimestampUs, short RawDecidegrees)
{
    public double Celsius => RawDecidegrees / 10.0;

    public static TemperatureReport Unpack(ReadOnlySpan<byte> payload) => new(
        BinaryPrimitives.ReadUInt64LittleEndian(payload),
        BinaryPrimitives.ReadInt16LittleEndian(payload[8..]));
}

/// <summary>Sequence-error report: expected vs received seq bytes.</summary>
public sealed record SequenceErrorReport(int ExpectedSeq, int ReceivedSeq)
{
    public static SequenceErrorReport Unpack(ReadOnlySpan<byte> payload) => new(payload[0], payload[1]);
}

/// <summary>Sync-pin configuration (pin 1..5, mode, polarity).</summary>
public sealed record SyncPinConfig(int Pin, SyncPinMode Mode, SyncPinPolarity Polarity)
{
    public byte[] Pack() => new[] { (byte)Pin, (byte)Mode, (byte)Polarity };

    public static SyncPinConfig Unpack(ReadOnlySpan<byte> payload) =>
        new(payload[0], (SyncPinMode)payload[1], (SyncPinPolarity)payload[2]);
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

    /// <summary>Decode an ASCII device string, dropping trailing NUL/0xFF filler.</summary>
    public static string StripDeviceString(ReadOnlySpan<byte> raw)
    {
        int end = raw.Length;
        while (end > 0 && (raw[end - 1] == 0x00 || raw[end - 1] == 0xFF))
            end--;
        return Encoding.ASCII.GetString(raw[..end]);
    }
}
