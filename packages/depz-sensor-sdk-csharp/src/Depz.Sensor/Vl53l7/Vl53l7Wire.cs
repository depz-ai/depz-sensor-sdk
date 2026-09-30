using System.Buffers.Binary;

namespace Depz.Sensor.Vl53l7;

/// <summary>
/// Commands the VL53L5CX / VL53L7CX / VL53L7CH I2C bridge adds on top of the
/// VL53L8 register bridge (contracts/11_SENSOR_VL53L7.md §2). READ_REG 0x32,
/// WRITE_REG 0x33, START_STREAM 0x35 and STOP_STREAM 0x36 are the VL53L8 ones
/// (<see cref="Vl53l8.Vl53l8Cmd"/>), with the tighter I2C limits of
/// <see cref="Vl53l7Wire"/>.
/// </summary>
public enum Vl53l7Cmd
{
    PinCtrl = 0x34,
    GetInfo = 0x37,
    SetI2cSpeed = 0x38,
}

/// <summary>
/// Reports the L5/L7 bridge adds. RPT_REG_DATA 0x91 / RPT_VL53_FRAME 0x93 are
/// the VL53L8 ones (<see cref="Vl53l8.Vl53l8Rpt"/>).
/// </summary>
public enum Vl53l7Rpt
{
    /// <summary>RPT_VL53_INFO — note: carries <b>no</b> echoed command byte.</summary>
    Vl53Info = 0x92,
}

/// <summary>
/// VL53_PIN_CTRL actions. None is a true sensor reset (the board has no power
/// GPIO): after <see cref="LpnOff"/> or <see cref="SoftCycle"/> the host must
/// re-run init() (firmware download included).
/// </summary>
public static class Vl53l7PinAction
{
    /// <summary>Stop streaming, drive LPn low: sensor I2C interface off (reads NACK).</summary>
    public const byte LpnOff = 0;

    /// <summary>Drive LPn high: interface on (power-up default).</summary>
    public const byte LpnOn = 1;

    /// <summary>Pulse I2C_RST.</summary>
    public const byte I2cRst = 2;

    /// <summary>Stop streaming, LPn low 1 ms, high, I2C_RST pulse; clears the I2C error counters.</summary>
    public const byte SoftCycle = 3;
}

/// <summary><see cref="Vl53l7Info.LastI2cError"/> values.</summary>
public static class Vl53l7I2cError
{
    public const byte Ok = 0;
    public const byte Nack = 1;
    public const byte Timeout = 2;   // 250 ms deadline
    public const byte BusError = 3;
}

/// <summary>
/// VL53L5/L7 I2C-bridge wire limits and command payload codecs (contract 11 §2).
/// All wire fields are little-endian.
/// </summary>
public static class Vl53l7Wire
{
    /// <summary>VL53LMZ_READ_MAX: READ_REG <c>len</c> 1..1536 (hosts MUST split larger reads here, not at 2048).</summary>
    public const int ReadMaxLen = 1536;

    /// <summary>VL53LMZ_XFER_MAX: WRITE_REG data length 1..2048.</summary>
    public const int WriteMaxLen = 2048;

    /// <summary>Bytes of frame data per RPT_VL53_FRAME chunk (VL53L8: 1528).</summary>
    public const int StreamChunkMax = 1536;

    /// <summary>Max frame_size accepted by START_STREAM (as VL53L8).</summary>
    public const int StreamTotalMax = 8192;

    /// <summary>RPT_VL53_INFO payload size.</summary>
    public const int InfoSize = 20;

    /// <summary>Nominal SCL steps the firmware carries a timing for; others snap to the nearest.</summary>
    public static ReadOnlySpan<int> I2cKhzSteps => new[] { 100, 200, 400, 500, 600, 700, 800, 900, 1000 };

    /// <summary>VL53_READ_REG payload: addr u16 LE, len u16 LE (len 1..<see cref="ReadMaxLen"/>, addr+len ≤ 0x10000).</summary>
    public static byte[] PackReadReg(ushort addr, int len)
    {
        if (len < 1 || len > ReadMaxLen || addr + len > 0x10000)
            throw new ArgumentOutOfRangeException(nameof(len), len, $"READ_REG len must be 1..{ReadMaxLen} and addr+len <= 0x10000");
        var buf = new byte[4];
        BinaryPrimitives.WriteUInt16LittleEndian(buf, addr);
        BinaryPrimitives.WriteUInt16LittleEndian(buf.AsSpan(2), (ushort)len);
        return buf;
    }

    /// <summary>VL53_WRITE_REG payload: addr u16 LE, then the raw register data (1..<see cref="WriteMaxLen"/> bytes).</summary>
    public static byte[] PackWriteReg(ushort addr, ReadOnlySpan<byte> data)
    {
        if (data.Length < 1 || data.Length > WriteMaxLen || addr + data.Length > 0x10000)
            throw new ArgumentOutOfRangeException(nameof(data), data.Length, $"WRITE_REG data must be 1..{WriteMaxLen} bytes and addr+N <= 0x10000");
        var buf = new byte[2 + data.Length];
        BinaryPrimitives.WriteUInt16LittleEndian(buf, addr);
        data.CopyTo(buf.AsSpan(2));
        return buf;
    }

    /// <summary>VL53_START_STREAM payload: frame_size u16 LE (1..<see cref="StreamTotalMax"/>).</summary>
    public static byte[] PackStartStream(int frameSize)
    {
        if (frameSize < 1 || frameSize > StreamTotalMax)
            throw new ArgumentOutOfRangeException(nameof(frameSize), frameSize, $"frame_size must be 1..{StreamTotalMax}");
        var buf = new byte[2];
        BinaryPrimitives.WriteUInt16LittleEndian(buf, (ushort)frameSize);
        return buf;
    }

    /// <summary>VL53_PIN_CTRL payload: action u8 (<see cref="Vl53l7PinAction"/>).</summary>
    public static byte[] PackPinCtrl(byte action) => new[] { action };

    /// <summary>VL53_SET_I2C_SPEED payload: khz u16 LE (the device snaps to <see cref="I2cKhzSteps"/>).</summary>
    public static byte[] PackSetI2cSpeed(ushort khz)
    {
        var buf = new byte[2];
        BinaryPrimitives.WriteUInt16LittleEndian(buf, khz);
        return buf;
    }
}

/// <summary>
/// RPT_VL53_INFO (0x92) — bridge state only (the sensor is never probed). All
/// counters run from power-up / DEVICE_RESET; only SOFT_CYCLE clears the I2C
/// ones. The report carries <b>no</b> echoed command byte. Each GET_INFO takes
/// the bus from the stream and can drop the frame in flight — read it before
/// and after a run, never in a polling loop during one.
/// </summary>
public sealed record Vl53l7Info(
    uint IntEdges,
    uint FramesDropped,
    uint I2cErrors,
    byte LastI2cError,
    byte LpnLevel,
    byte IntLevel,
    ushort I2cKhz,
    ushort FrameSize,
    bool Streaming)
{
    /// <summary>
    /// Parse a RPT_VL53_INFO payload (<c>&lt;IIIBBBHHB</c>, 20 bytes LE).
    /// </summary>
    /// <exception cref="ArgumentException">If the payload is shorter than 20 bytes.</exception>
    public static Vl53l7Info Unpack(ReadOnlySpan<byte> payload)
    {
        if (payload.Length < Vl53l7Wire.InfoSize)
            throw new ArgumentException(
                $"RPT_VL53_INFO: expected {Vl53l7Wire.InfoSize} bytes, got {payload.Length}", nameof(payload));
        return new Vl53l7Info(
            BinaryPrimitives.ReadUInt32LittleEndian(payload),
            BinaryPrimitives.ReadUInt32LittleEndian(payload[4..]),
            BinaryPrimitives.ReadUInt32LittleEndian(payload[8..]),
            payload[12],
            payload[13],
            payload[14],
            BinaryPrimitives.ReadUInt16LittleEndian(payload[15..]),
            BinaryPrimitives.ReadUInt16LittleEndian(payload[17..]),
            payload[19] != 0);
    }
}
