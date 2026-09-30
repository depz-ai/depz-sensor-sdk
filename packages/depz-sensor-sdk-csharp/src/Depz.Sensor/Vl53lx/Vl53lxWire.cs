using System.Buffers.Binary;
using Depz.Sensor.Vl53l4;

namespace Depz.Sensor.Vl53lx;

/// <summary>
/// VL53L 1D-family register-bridge command opcodes, protocol v2.00
/// (contracts/12_SENSOR_VL53LX.md §2). The contract-10 opcodes plus
/// <see cref="SetAddrWidth"/>.
/// </summary>
public enum Vl53lxCmd
{
    ReadReg = 0x32,
    WriteReg = 0x33,
    Xshut = 0x34,
    StartStream = 0x35,
    StopStream = 0x36,
    GetInfo = 0x37,
    SetI2cSpeed = 0x38,
    SetAddrWidth = 0x39,
    /// <summary>New in v2.01 (firmware v0.24), no payload: zero the I2C error counter; sent after every sensor init.</summary>
    ClearI2cErrors = 0x3A,
}

/// <summary>VL53L 1D-family report opcodes (same ids as contract 10).</summary>
public enum Vl53lxRpt
{
    RegData = 0x91,
    Info = 0x92,
    Stream = 0x93,
}

/// <summary>One interrupt-release step the bridge plays after every streamed block read.</summary>
public readonly record struct Vl53lxClearStep(ushort Addr, byte Value);

/// <summary>
/// v2.00 wire codecs. One firmware (<c>APP_VL53L0_4</c>) serves six products;
/// it is the contract-10 bridge with the sensor-specific facts moved to the
/// host: the register-address width (<see cref="PackSetAddrWidth"/>, new), the
/// interrupt-release writes (carried by <see cref="PackStartStream"/>) and the
/// boot handshake (no longer inside XSHUT). READ_REG, WRITE_REG, XSHUT,
/// SET_I2C_SPEED and the REG_DATA / STREAM reports are identical on the wire
/// to contract 10 and are forwarded to <see cref="Vl53l4Wire"/>
/// (<see cref="Vl53l4RegData"/>, <see cref="Vl53l4StreamData"/>).
/// </summary>
public static class Vl53lxWire
{
    /// <summary>Most interrupt-release steps a stream may carry (VL53_CLEAR_STEPS_WIRE_MAX).</summary>
    public const int ClearStepsMax = 4;

    /// <summary>RPT_VL53_INFO payload size in v2.00.</summary>
    public const int InfoSize = 23;

    /// <summary>Max read length / write data length per transfer (contract 10).</summary>
    public const int XferMax = Vl53l4Wire.XferMax;

    /// <summary>START_STREAM flags bit 1: INT active high (contract 10).</summary>
    public const byte FlagIntActHigh = Vl53l4Wire.FlagIntActHigh;

    /// <summary>VL53_READ_REG payload (contract 10): addr u16 LE, len u16 LE.</summary>
    public static byte[] PackReadReg(ushort addr, ushort len) => Vl53l4Wire.PackReadReg(addr, len);

    /// <summary>VL53_WRITE_REG payload (contract 10): addr u16 LE, then the raw register data.</summary>
    public static byte[] PackWriteReg(ushort addr, ReadOnlySpan<byte> data) => Vl53l4Wire.PackWriteReg(addr, data);

    /// <summary>
    /// VL53_XSHUT payload: action u8 (<see cref="Vl53l4Xshut"/>). In v2.00 RESET
    /// has no boot handshake — the host polls the boot register itself.
    /// </summary>
    public static byte[] PackXshut(byte action) => Vl53l4Wire.PackXshut(action);

    /// <summary>VL53_SET_I2C_SPEED payload (contract 10): khz u16 LE.</summary>
    public static byte[] PackSetI2cSpeed(ushort khz) => Vl53l4Wire.PackSetI2cSpeed(khz);

    /// <summary>
    /// VL53_SET_ADDR_WIDTH payload: width u8, 1 (VL53L0X) or 2. Sticky, 2 after
    /// a reset; set it before the first register access of a session.
    /// </summary>
    /// <exception cref="ArgumentOutOfRangeException">width other than 1 or 2.</exception>
    public static byte[] PackSetAddrWidth(int width)
    {
        if (width is not (1 or 2))
            throw new ArgumentOutOfRangeException(nameof(width), width, "register address width is 1 or 2 bytes");
        return new[] { (byte)width };
    }

    /// <summary>
    /// VL53_START_STREAM payload (v2.00, 6 + 3n bytes): addr u16 LE, len u16 LE,
    /// flags u8, n_clear u8, then n × {addr u16 LE, value u8} — the
    /// interrupt-release steps the bridge plays after every block read (0..4).
    /// </summary>
    /// <exception cref="ArgumentOutOfRangeException">more than <see cref="ClearStepsMax"/> steps.</exception>
    public static byte[] PackStartStream(
        ushort addr, ushort len, IReadOnlyList<Vl53lxClearStep>? clear = null, byte flags = 0)
    {
        clear ??= Array.Empty<Vl53lxClearStep>();
        if (clear.Count > ClearStepsMax)
            throw new ArgumentOutOfRangeException(
                nameof(clear), clear.Count, $"at most {ClearStepsMax} interrupt-release steps");
        var buf = new byte[6 + 3 * clear.Count];
        BinaryPrimitives.WriteUInt16LittleEndian(buf, addr);
        BinaryPrimitives.WriteUInt16LittleEndian(buf.AsSpan(2), len);
        buf[4] = flags;
        buf[5] = (byte)clear.Count;
        for (int i = 0; i < clear.Count; i++)
        {
            BinaryPrimitives.WriteUInt16LittleEndian(buf.AsSpan(6 + 3 * i), clear[i].Addr);
            buf[8 + 3 * i] = clear[i].Value;
        }
        return buf;
    }
}

/// <summary>
/// RPT_VL53_INFO (v2.00, 23 bytes) — bridge state only; the bridge reads no
/// sensor register. Counters are free-running and wrap silently: watch
/// increments. <see cref="SlotsSkipped"/> = a slot that never got the bus,
/// <see cref="I2cErrors"/> = a bus that answered badly, <see cref="FramesDropped"/>
/// = a good sample the USB TX ring had no room for (since the stream was armed).
/// <see cref="LastI2cError"/>: 0 none, 1 NACK, 2 TIMEOUT, 3 BUS_ERROR.
/// </summary>
public sealed record Vl53lxInfo(
    uint IntEdges,
    uint SlotsSkipped,
    uint I2cErrors,
    byte LastI2cError,
    byte XshutLevel,
    byte IntLevel,
    ushort I2cKhz,
    byte AddrWidth,
    byte NClear,
    uint FramesDropped)
{
    /// <summary>Parse a RPT_VL53_INFO payload (<c>&lt;IIIBBBHBBI</c>, 23 bytes LE).</summary>
    /// <exception cref="ArgumentException">If the payload is shorter than 23 bytes.</exception>
    public static Vl53lxInfo Unpack(ReadOnlySpan<byte> payload)
    {
        if (payload.Length < Vl53lxWire.InfoSize)
            throw new ArgumentException(
                $"RPT_VL53_INFO: expected {Vl53lxWire.InfoSize} bytes, got {payload.Length}", nameof(payload));
        return new Vl53lxInfo(
            BinaryPrimitives.ReadUInt32LittleEndian(payload),
            BinaryPrimitives.ReadUInt32LittleEndian(payload[4..]),
            BinaryPrimitives.ReadUInt32LittleEndian(payload[8..]),
            payload[12],
            payload[13],
            payload[14],
            BinaryPrimitives.ReadUInt16LittleEndian(payload[15..]),
            payload[17],
            payload[18],
            BinaryPrimitives.ReadUInt32LittleEndian(payload[19..]));
    }
}
