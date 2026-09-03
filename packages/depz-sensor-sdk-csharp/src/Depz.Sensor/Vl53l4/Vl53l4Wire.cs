using System.Buffers.Binary;

namespace Depz.Sensor.Vl53l4;

/// <summary>VL53L4CD register-bridge command opcodes (contracts/10_SENSOR_VL53L4.md).</summary>
public enum Vl53l4Cmd
{
    ReadReg = 0x32,
    WriteReg = 0x33,
    Xshut = 0x34,
    StartStream = 0x35,
    StopStream = 0x36,
    GetInfo = 0x37,
    SetI2cSpeed = 0x38,
}

/// <summary>VL53L4CD report opcodes.</summary>
public enum Vl53l4Rpt
{
    RegData = 0x91,
    Info = 0x92,
    Stream = 0x93,
}

/// <summary>
/// VL53_XSHUT actions. <see cref="Reset"/> is blocking on the MCU and is
/// answered only after the sensor's boot handshake (allow ≥ 1.5 s).
/// </summary>
public static class Vl53l4Xshut
{
    public const byte Off = 0;
    public const byte On = 1;
    public const byte Reset = 2;
}

/// <summary>
/// VL53L4CD wire limits and command payload codecs (contract 10 §1–§3). All
/// wire fields (<c>addr</c>, <c>len</c>, …) are little-endian; register
/// <b>contents</b> are big-endian and pass through the bridge untouched.
/// </summary>
public static class Vl53l4Wire
{
    /// <summary>
    /// Max read length / write data length per transfer (the STM32 I2C NBYTES
    /// field is 8-bit and a write spends two bytes on the register address; the
    /// firmware applies the same limit to both directions).
    /// </summary>
    public const int XferMax = 253;

    /// <summary>
    /// VL53_START_STREAM flags bit 1: INT active high, mirroring bit 4 of
    /// GPIO_HV_MUX__CTRL (0x0030). Clear (default): INT active low.
    /// </summary>
    public const byte FlagIntActHigh = 0x02;

    /// <summary>
    /// Nominal SCL steps the firmware carries a TIMINGR for (VL53_SET_I2C_SPEED
    /// clamps to the nearest one).
    /// </summary>
    public static ReadOnlySpan<int> I2cKhzSteps => new[] { 100, 200, 400, 500, 600, 700, 800, 900, 1000 };

    /// <summary>VL53_READ_REG payload: addr u16 LE, len u16 LE.</summary>
    public static byte[] PackReadReg(ushort addr, ushort len)
    {
        var buf = new byte[4];
        BinaryPrimitives.WriteUInt16LittleEndian(buf, addr);
        BinaryPrimitives.WriteUInt16LittleEndian(buf.AsSpan(2), len);
        return buf;
    }

    /// <summary>VL53_WRITE_REG payload: addr u16 LE, then the raw register data.</summary>
    public static byte[] PackWriteReg(ushort addr, ReadOnlySpan<byte> data)
    {
        var buf = new byte[2 + data.Length];
        BinaryPrimitives.WriteUInt16LittleEndian(buf, addr);
        data.CopyTo(buf.AsSpan(2));
        return buf;
    }

    /// <summary>VL53_XSHUT payload: action u8 (<see cref="Vl53l4Xshut"/>).</summary>
    public static byte[] PackXshut(byte action) => new[] { action };

    /// <summary>VL53_START_STREAM payload: addr u16 LE, len u16 LE, flags u8.</summary>
    public static byte[] PackStartStream(ushort addr, ushort len, byte flags = 0)
    {
        var buf = new byte[5];
        BinaryPrimitives.WriteUInt16LittleEndian(buf, addr);
        BinaryPrimitives.WriteUInt16LittleEndian(buf.AsSpan(2), len);
        buf[4] = flags;
        return buf;
    }

    /// <summary>VL53_SET_I2C_SPEED payload: khz u16 LE.</summary>
    public static byte[] PackSetI2cSpeed(ushort khz)
    {
        var buf = new byte[2];
        BinaryPrimitives.WriteUInt16LittleEndian(buf, khz);
        return buf;
    }
}

/// <summary>
/// RPT_VL53_REG_DATA — one register read. <see cref="Cmd"/> echoes the
/// READ_REG opcode (0x32); <see cref="TimestampUs"/> is MCU uptime at
/// I2C-read completion.
/// </summary>
public sealed record Vl53l4RegData(byte Cmd, ulong TimestampUs, byte[] Data)
{
    /// <summary>Parse a RPT_VL53_REG_DATA payload: cmd u8, timestamp u64 LE, then data.</summary>
    public static Vl53l4RegData Unpack(ReadOnlySpan<byte> payload) => new(
        payload[0],
        BinaryPrimitives.ReadUInt64LittleEndian(payload[1..]),
        payload[9..].ToArray());
}

/// <summary>
/// RPT_VL53_INFO — bridge diagnostics (21-byte payload). Counters are
/// free-running and wrap silently; watch increments, not absolute values.
/// <see cref="ModelId"/> expected 0xEBAA, <see cref="FwStatus"/> expected 0x03
/// (booted). <see cref="LastI2cError"/>: 0 none, 1 NACK, 2 TIMEOUT, 3 BUS_ERROR.
/// </summary>
public sealed record Vl53l4Info(
    uint IntEdges,
    uint SlotsSkipped,
    uint I2cErrors,
    byte LastI2cError,
    ushort ModelId,
    byte FwStatus,
    byte Initialized,
    byte XshutLevel,
    byte IntLevel,
    ushort I2cKhz)
{
    /// <summary>Parse a RPT_VL53_INFO payload (u32 u32 u32 u8 u16 u8 u8 u8 u8 u16, all LE).</summary>
    public static Vl53l4Info Unpack(ReadOnlySpan<byte> payload) => new(
        BinaryPrimitives.ReadUInt32LittleEndian(payload),
        BinaryPrimitives.ReadUInt32LittleEndian(payload[4..]),
        BinaryPrimitives.ReadUInt32LittleEndian(payload[8..]),
        payload[12],
        BinaryPrimitives.ReadUInt16LittleEndian(payload[13..]),
        payload[15],
        payload[16],
        payload[17],
        payload[18],
        BinaryPrimitives.ReadUInt16LittleEndian(payload[19..]));
}

/// <summary>
/// RPT_VL53_STREAM — one streamed register block. <see cref="TimestampUs"/> is
/// MCU uptime at the INT edge (the sensor event, not the I2C completion);
/// <see cref="Addr"/>/<see cref="Len"/> echo the stream configuration so each
/// report is self-describing.
/// </summary>
public sealed record Vl53l4StreamData(ulong TimestampUs, ushort Addr, ushort Len, byte[] Data)
{
    /// <summary>Parse a RPT_VL53_STREAM payload: ts u64 LE, addr u16 LE, len u16 LE, data[len].</summary>
    public static Vl53l4StreamData Unpack(ReadOnlySpan<byte> payload)
    {
        ulong ts = BinaryPrimitives.ReadUInt64LittleEndian(payload);
        ushort addr = BinaryPrimitives.ReadUInt16LittleEndian(payload[8..]);
        ushort len = BinaryPrimitives.ReadUInt16LittleEndian(payload[10..]);
        return new Vl53l4StreamData(ts, addr, len, payload.Slice(12, len).ToArray());
    }
}
