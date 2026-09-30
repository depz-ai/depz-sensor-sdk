using System.Buffers.Binary;

namespace Depz.Sensor.Bno055;

/// <summary>
/// BNO055 register-bridge command opcodes, protocol v0.10
/// (contracts/13_SENSOR_BNO055.md §2). 0x30/0x31 are the common sync pins.
/// </summary>
public enum Bno055Cmd
{
    ReadReg = 0x32,
    WriteReg = 0x33,

    /// <summary>Pulses nRESET; answered only after the chip-ID handshake (~0.5 s — allow ≥ 1.5 s).</summary>
    Reset = 0x34,

    StartStream = 0x35,
    StopStream = 0x36,
    GetInfo = 0x37,
}

/// <summary>BNO055 report opcodes (contract 13 §3).</summary>
public enum Bno055Rpt
{
    RegData = 0x91,
    Info = 0x92,
    Stream = 0x93,
}

/// <summary>BNO_START_STREAM trigger.</summary>
public enum Bno055Trigger : byte
{
    /// <summary>Read every period_ms — the only data trigger on sensor SW rev 03.11.</summary>
    Timer = 0,

    /// <summary>Read on the INT rising edge; period_ms is a missed-edge watchdog (0 disables it).</summary>
    Int = 1,
}

/// <summary>
/// Wire codecs of the BNO055 bridge (<c>APP_BNO055</c>). The firmware is a thin
/// register bridge: the MCU owns the I2C bus (sensor at 7-bit 0x28, 400 kHz
/// fixed), the nRESET/INT pins and one streaming loop; operating mode, units,
/// axis remap, calibration and decoding are host logic expressed as register
/// access (<see cref="Bno055Regs"/>). Mirrors
/// <c>depz_sensor_sdk/protocol/bno055.py</c> 1:1. BNO_RESET, BNO_STOP_STREAM and
/// BNO_GET_INFO carry an empty payload.
/// </summary>
public static class Bno055Wire
{
    /// <summary>Max bytes per READ_REG / WRITE_REG / streamed block; addr + len ≤ 0x100.</summary>
    public const int XferMax = 128;

    /// <summary>RPT_BNO_INFO payload size.</summary>
    public const int InfoSize = 38;

    /// <summary>BNO_RESET answers after the sensor's ~0.5 s boot handshake.</summary>
    public static readonly TimeSpan ResetTimeout = TimeSpan.FromSeconds(3);

    /// <summary>Identity registers 0x00..0x03 of a healthy BNO055.</summary>
    public const byte ExpectedChipId = 0xA0;
    public const byte ExpectedAccId = 0xFB;
    public const byte ExpectedMagId = 0x32;
    public const byte ExpectedGyrId = 0x0F;

    /// <summary>BNO_READ_REG payload: addr u8, len u8.</summary>
    public static byte[] PackReadReg(byte addr, byte len) => new[] { addr, len };

    /// <summary>BNO_WRITE_REG payload: addr u8, then data[1..128].</summary>
    public static byte[] PackWriteReg(byte addr, ReadOnlySpan<byte> data)
    {
        var buf = new byte[1 + data.Length];
        buf[0] = addr;
        data.CopyTo(buf.AsSpan(1));
        return buf;
    }

    /// <summary>
    /// BNO_START_STREAM payload (5 bytes): trigger u8, addr u8, len u8,
    /// period_ms u16 LE. Replaces any running stream.
    /// </summary>
    public static byte[] PackStartStream(Bno055Trigger trigger, byte addr, byte len, ushort periodMs)
    {
        var buf = new byte[5];
        buf[0] = (byte)trigger;
        buf[1] = addr;
        buf[2] = len;
        BinaryPrimitives.WriteUInt16LittleEndian(buf.AsSpan(3), periodMs);
        return buf;
    }
}

/// <summary>RPT_BNO_REG_DATA: echoed READ_REG opcode, MCU uptime at I2C-read completion, register bytes.</summary>
public sealed record Bno055RegData(byte Cmd, ulong TimestampUs, byte[] Data)
{
    /// <summary>Parse a RPT_BNO_REG_DATA payload: cmd u8, timestamp u64 LE, then data.</summary>
    public static Bno055RegData Unpack(ReadOnlySpan<byte> payload) => new(
        payload[0],
        BinaryPrimitives.ReadUInt64LittleEndian(payload[1..]),
        payload[9..].ToArray());
}

/// <summary>
/// RPT_BNO_REG_STREAM — one streamed register block. <see cref="Addr"/> /
/// <see cref="Length"/> echo the stream configuration so each report is
/// self-describing; <see cref="TimestampUs"/> is the trigger time (timer expiry
/// or INT edge), not the I2C completion.
/// </summary>
public sealed record Bno055StreamData(ulong TimestampUs, byte Addr, byte Length, byte[] Data)
{
    /// <summary>Parse a RPT_BNO_REG_STREAM payload: timestamp u64 LE, addr u8, len u8, data[len].</summary>
    public static Bno055StreamData Unpack(ReadOnlySpan<byte> payload)
    {
        byte len = payload[9];
        return new(
            BinaryPrimitives.ReadUInt64LittleEndian(payload),
            payload[8],
            len,
            payload.Slice(10, len).ToArray());
    }
}

/// <summary>
/// RPT_BNO_INFO (38 bytes) — sensor identity (registers 0x00..0x06) plus bridge
/// diagnostics. Counters are free-running and wrap silently: watch increments.
/// Read*Us, <see cref="SlotsSkipped"/> and <see cref="LoopMaxUs"/> reset at
/// START_STREAM. A rising <see cref="SensorResets"/> means the bridge pulsed
/// nRESET to recover the bus: the sensor is back in CONFIG and the host must
/// restore its configuration. <see cref="SwRev"/> is BCD (0x0311 = 03.11).
/// <see cref="LastI2cError"/>: 0 none, 1 NACK, 2 TIMEOUT, 3 BUS_ERROR.
/// </summary>
public sealed record Bno055Info(
    byte I2cAddr,
    byte ChipId,
    byte AccId,
    byte MagId,
    byte GyrId,
    ushort SwRev,
    byte BlRev,
    byte Initialized,
    byte IntLevel,
    uint IntEdges,
    ushort ReadMinUs,
    ushort ReadMaxUs,
    ushort ReadAvgUs,
    uint TxDropped,
    uint I2cErrors,
    uint SlotsSkipped,
    ushort BusRecoveries,
    byte LastI2cError,
    byte SensorResets,
    ushort LoopMaxUs)
{
    /// <summary>Parse a RPT_BNO_INFO payload (<c>&lt;BBBBBHBBBIHHHIIIHBBH</c>, 38 bytes LE).</summary>
    /// <exception cref="ArgumentException">If the payload is shorter than 38 bytes.</exception>
    public static Bno055Info Unpack(ReadOnlySpan<byte> p)
    {
        if (p.Length < Bno055Wire.InfoSize)
            throw new ArgumentException(
                $"RPT_BNO_INFO: expected {Bno055Wire.InfoSize} bytes, got {p.Length}", nameof(p));
        return new Bno055Info(
            p[0], p[1], p[2], p[3], p[4],
            BinaryPrimitives.ReadUInt16LittleEndian(p[5..]),
            p[7], p[8], p[9],
            BinaryPrimitives.ReadUInt32LittleEndian(p[10..]),
            BinaryPrimitives.ReadUInt16LittleEndian(p[14..]),
            BinaryPrimitives.ReadUInt16LittleEndian(p[16..]),
            BinaryPrimitives.ReadUInt16LittleEndian(p[18..]),
            BinaryPrimitives.ReadUInt32LittleEndian(p[20..]),
            BinaryPrimitives.ReadUInt32LittleEndian(p[24..]),
            BinaryPrimitives.ReadUInt32LittleEndian(p[28..]),
            BinaryPrimitives.ReadUInt16LittleEndian(p[32..]),
            p[34],
            p[35],
            BinaryPrimitives.ReadUInt16LittleEndian(p[36..]));
    }

    /// <summary>All four identity registers hold the healthy BNO055 values.</summary>
    public bool IdsOk =>
        ChipId == Bno055Wire.ExpectedChipId && AccId == Bno055Wire.ExpectedAccId
        && MagId == Bno055Wire.ExpectedMagId && GyrId == Bno055Wire.ExpectedGyrId;

    /// <summary>Sensor firmware revision as Bosch writes it: 0x0311 → "03.11".</summary>
    public string SwRevText => $"{SwRev >> 8:X2}.{SwRev & 0xFF:X2}";
}
