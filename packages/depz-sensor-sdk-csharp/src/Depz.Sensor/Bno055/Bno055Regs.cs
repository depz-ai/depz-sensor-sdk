using System.Buffers.Binary;

namespace Depz.Sensor.Bno055;

/// <summary>OPR_MODE (0x3D) bits 3:0 (the register reads back 0x10 after reset — mask).</summary>
public enum Bno055OprMode : byte
{
    Config = 0x00,
    AccOnly = 0x01,
    MagOnly = 0x02,
    GyroOnly = 0x03,
    AccMag = 0x04,
    AccGyro = 0x05,
    MagGyro = 0x06,
    Amg = 0x07,
    Imu = 0x08,
    Compass = 0x09,
    M4g = 0x0A,
    NdofFmcOff = 0x0B,
    Ndof = 0x0C,
}

/// <summary>PWR_MODE (0x3E) bits 1:0.</summary>
public enum Bno055PwrMode : byte
{
    Normal = 0x00,
    LowPower = 0x01,
    Suspend = 0x02,
}

/// <summary>TEMP_SOURCE (0x40) bits 1:0.</summary>
public enum Bno055TempSource : byte
{
    Accel = 0x00,
    Gyro = 0x01,
}

/// <summary>Three raw i16 words in register order (x, y, z).</summary>
public readonly record struct Bno055Vec3(short X, short Y, short Z);

/// <summary>Raw Euler angles in register order: heading, roll, pitch.</summary>
public readonly record struct Bno055Euler(short Heading, short Roll, short Pitch);

/// <summary>Raw quaternion in register order: w, x, y, z (1.0 = 2^14).</summary>
public readonly record struct Bno055Quat(short W, short X, short Y, short Z);

/// <summary>
/// Output units (UNIT_SEL 0x3B). The SDK default (<see cref="Default"/>,
/// UNIT_SEL = 0x00) is m/s², dps, degrees, °C, Windows orientation. The
/// sensor's own power-on value is 0x80 (Android), so a fresh sensor must be
/// told. Bits as the silicon implements them (contract 13 §4.2; the datasheet
/// §4.3.60 bit table is off by one); unknown bits are dropped on unpack.
/// Physical value = raw / LSB.
/// </summary>
public sealed record Bno055Units(
    bool AccelMg = false, bool GyroRps = false, bool EulerRad = false, bool TempF = false, bool Android = false)
{
    public const byte AccMg = 0x01;
    public const byte GyrRps = 0x02;
    public const byte EulRad = 0x04;
    public const byte TempFahrenheit = 0x10;
    public const byte OriAndroid = 0x80;

    public static readonly Bno055Units Default = new();

    public byte Pack() => (byte)(
        (AccelMg ? AccMg : 0) | (GyroRps ? GyrRps : 0) | (EulerRad ? EulRad : 0)
        | (TempF ? TempFahrenheit : 0) | (Android ? OriAndroid : 0));

    public static Bno055Units Unpack(byte value) => new(
        (value & AccMg) != 0,
        (value & GyrRps) != 0,
        (value & EulRad) != 0,
        (value & TempFahrenheit) != 0,
        (value & OriAndroid) != 0);

    /// <summary>ACC_DATA LSB: 1 per mg, else 100 per m/s² (LIA/GRV: <see cref="Bno055Regs.FusionAccelLsb"/>).</summary>
    public double AccelLsb => AccelMg ? 1.0 : 100.0;

    /// <summary>Angular-rate LSB: 900 per rad/s, else 16 per dps.</summary>
    public double GyroLsb => GyroRps ? 900.0 : 16.0;

    /// <summary>Euler LSB: 900 per radian, else 16 per degree.</summary>
    public double EulerLsb => EulerRad ? 900.0 : 16.0;

    /// <summary>Temperature LSB: 1 LSB = 2 °F (0.5 LSB/°F), else 1 LSB = 1 °C.</summary>
    public double TempLsb => TempF ? 0.5 : 1.0;
}

/// <summary>CALIB_STAT (0x35) <c>sys&lt;7:6&gt; gyr&lt;5:4&gt; acc&lt;3:2&gt; mag&lt;1:0&gt;</c>: 0 = not calibrated … 3 = fully.</summary>
public readonly record struct Bno055CalibStatus(int System, int Gyro, int Accel, int Mag)
{
    public static Bno055CalibStatus Unpack(byte value) =>
        new(value >> 6 & 3, value >> 4 & 3, value >> 2 & 3, value & 3);

    public byte Pack() => (byte)((System & 3) << 6 | (Gyro & 3) << 4 | (Accel & 3) << 2 | (Mag & 3));

    public bool FullyCalibrated => System == 3 && Gyro == 3 && Accel == 3 && Mag == 3;
}

/// <summary>
/// Sensor offsets and radii, registers 0x55..0x6A (22 bytes, 11 × i16 LE:
/// acc_offset xyz, mag_offset xyz, gyr_offset xyz, acc_radius, mag_radius).
/// Readable and writable only in CONFIG; write all 22 bytes in one transfer
/// (the sensor latches each group on its MSB). Offsets are in sensor LSB and do
/// not depend on UNIT_SEL.
/// </summary>
public sealed record Bno055CalibrationProfile(
    Bno055Vec3 AccelOffset, Bno055Vec3 MagOffset, Bno055Vec3 GyroOffset, short AccelRadius, short MagRadius)
{
    public byte[] Pack()
    {
        var buf = new byte[Bno055Regs.CalibProfileLen];
        var s = buf.AsSpan();
        short[] v =
        {
            AccelOffset.X, AccelOffset.Y, AccelOffset.Z,
            MagOffset.X, MagOffset.Y, MagOffset.Z,
            GyroOffset.X, GyroOffset.Y, GyroOffset.Z,
            AccelRadius, MagRadius,
        };
        for (int i = 0; i < v.Length; i++)
            BinaryPrimitives.WriteInt16LittleEndian(s[(2 * i)..], v[i]);
        return buf;
    }

    /// <exception cref="ArgumentException">Unless <paramref name="data"/> is exactly 22 bytes.</exception>
    public static Bno055CalibrationProfile Unpack(ReadOnlySpan<byte> data)
    {
        if (data.Length != Bno055Regs.CalibProfileLen)
            throw new ArgumentException(
                $"calibration profile is {Bno055Regs.CalibProfileLen} bytes, got {data.Length}", nameof(data));
        var w = new short[11];
        for (int i = 0; i < w.Length; i++)
            w[i] = BinaryPrimitives.ReadInt16LittleEndian(data[(2 * i)..]);
        return new(
            new Bno055Vec3(w[0], w[1], w[2]),
            new Bno055Vec3(w[3], w[4], w[5]),
            new Bno055Vec3(w[6], w[7], w[8]),
            w[9], w[10]);
    }
}

/// <summary>Source axis codes of <see cref="Bno055AxisRemap"/>.</summary>
public enum Bno055Axis : byte
{
    X = 0,
    Y = 1,
    Z = 2,
}

/// <summary>
/// Which chip axis feeds each output axis, and its sign
/// (AXIS_MAP_CONFIG 0x41 = <c>z&lt;5:4&gt; y&lt;3:2&gt; x&lt;1:0&gt;</c>,
/// AXIS_MAP_SIGN 0x42 = <c>x 2, y 1, z 0</c>, 1 = negative). <c>X = Bno055Axis.Y</c>
/// means "output X is the chip's Y axis". The default is P1.
/// </summary>
public sealed record Bno055AxisRemap(
    Bno055Axis X = Bno055Axis.X, Bno055Axis Y = Bno055Axis.Y, Bno055Axis Z = Bno055Axis.Z,
    bool XNegative = false, bool YNegative = false, bool ZNegative = false)
{
    /// <summary>
    /// → (AXIS_MAP_CONFIG, AXIS_MAP_SIGN). The sensor keeps its old mapping when
    /// given one that uses an axis twice, so this refuses it up front.
    /// </summary>
    /// <exception cref="ArgumentException">If X/Y/Z are not a permutation of the three axes.</exception>
    public (byte Config, byte Sign) Pack()
    {
        int seen = 0;
        foreach (var a in new[] { X, Y, Z })
        {
            if ((byte)a > 2 || (seen & (1 << (byte)a)) != 0)
                throw new ArgumentException($"axis remap must be a permutation of X/Y/Z, got {this}");
            seen |= 1 << (byte)a;
        }
        byte config = (byte)((byte)Z << 4 | (byte)Y << 2 | (byte)X);
        byte sign = (byte)((XNegative ? 4 : 0) | (YNegative ? 2 : 0) | (ZNegative ? 1 : 0));
        return (config, sign);
    }

    public static Bno055AxisRemap Unpack(byte config, byte sign) => new(
        (Bno055Axis)(config & 3), (Bno055Axis)(config >> 2 & 3), (Bno055Axis)(config >> 4 & 3),
        (sign & 4) != 0, (sign & 2) != 0, (sign & 1) != 0);

    /// <summary>Datasheet §3.4: placement → (AXIS_MAP_CONFIG, AXIS_MAP_SIGN), P0..P7 in order.</summary>
    public static readonly IReadOnlyList<(string Name, byte Config, byte Sign)> Placements = new[]
    {
        ("P0", (byte)0x21, (byte)0x04),
        ("P1", (byte)0x24, (byte)0x00),
        ("P2", (byte)0x24, (byte)0x06),
        ("P3", (byte)0x21, (byte)0x02),
        ("P4", (byte)0x24, (byte)0x03),
        ("P5", (byte)0x21, (byte)0x01),
        ("P6", (byte)0x21, (byte)0x07),
        ("P7", (byte)0x24, (byte)0x05),
    };

    /// <summary>Datasheet §3.4 mounting preset P0..P7 (case-insensitive; P1 is the default).</summary>
    /// <exception cref="ArgumentException">For any other name.</exception>
    public static Bno055AxisRemap Placement(string name)
    {
        foreach (var (n, config, sign) in Placements)
            if (string.Equals(n, name, StringComparison.OrdinalIgnoreCase))
                return Unpack(config, sign);
        throw new ArgumentException($"unknown placement '{name}'; expected P0..P7", nameof(name));
    }
}

/// <summary>
/// ACC_CONFIG (page 1, 0x08) as register codes <c>range&lt;1:0&gt; bandwidth&lt;4:2&gt;
/// power&lt;7:5&gt;</c>: indexes into <see cref="Bno055Regs.AccRangeG"/>,
/// <see cref="Bno055Regs.AccBandwidthHz"/>, <see cref="Bno055Regs.AccPowerNames"/>.
/// Power-on value 0x0D = ±4 g, 62.5 Hz, normal. Effective in non-fusion modes only.
/// </summary>
public readonly record struct Bno055AccelConfig(int Range, int Bandwidth, int Power)
{
    public static readonly Bno055AccelConfig Default = new(1, 3, 0);

    public byte Pack() => (byte)((Power & 7) << 5 | (Bandwidth & 7) << 2 | (Range & 3));

    public static Bno055AccelConfig Unpack(byte value) => new(value & 3, value >> 2 & 7, value >> 5 & 7);
}

/// <summary>
/// GYR_CONFIG_0/1 (page 1, 0x0A/0x0B), 2 bytes: byte0 = <c>range&lt;2:0&gt;
/// bandwidth&lt;5:3&gt;</c>, byte1 = <c>power&lt;2:0&gt;</c>. Indexes into
/// <see cref="Bno055Regs.GyrRangeDps"/>, <see cref="Bno055Regs.GyrBandwidthHz"/>,
/// <see cref="Bno055Regs.GyrPowerNames"/>. Power-on 0x38/0x00 = 2000 dps, 32 Hz, normal.
/// </summary>
public readonly record struct Bno055GyroConfig(int Range, int Bandwidth, int Power)
{
    public static readonly Bno055GyroConfig Default = new(0, 7, 0);

    public byte[] Pack() => new[] { (byte)((Bandwidth & 7) << 3 | (Range & 7)), (byte)(Power & 7) };

    public static Bno055GyroConfig Unpack(ReadOnlySpan<byte> data) =>
        new(data[0] & 7, data[0] >> 3 & 7, data[1] & 7);
}

/// <summary>
/// MAG_CONFIG (page 1, 0x09) <c>rate&lt;2:0&gt; mode&lt;4:3&gt; power&lt;6:5&gt;</c>:
/// indexes into <see cref="Bno055Regs.MagRateHz"/>, <see cref="Bno055Regs.MagOprNames"/>,
/// <see cref="Bno055Regs.MagPowerNames"/>. Bit 7 is not a field:
/// <c>Unpack(v).Pack() == (v &amp; 0x7F)</c>. Power-on 0x0B = 10 Hz, regular, normal.
/// </summary>
public readonly record struct Bno055MagConfig(int Rate, int Mode, int Power)
{
    public static readonly Bno055MagConfig Default = new(3, 1, 0);

    public byte Pack() => (byte)((Power & 3) << 5 | (Mode & 3) << 3 | (Rate & 7));

    public static Bno055MagConfig Unpack(byte value) => new(value & 7, value >> 3 & 3, value >> 5 & 3);
}

/// <summary>
/// Raw register values found in one block read. A channel is null when the
/// window addr..addr+len does not cover all of its bytes.
/// </summary>
public sealed record Bno055RawBlock(
    Bno055Vec3? Accel,
    Bno055Vec3? Mag,
    Bno055Vec3? Gyro,
    Bno055Euler? Euler,
    Bno055Quat? Quaternion,
    Bno055Vec3? LinearAccel,
    Bno055Vec3? Gravity,
    sbyte? Temperature,
    byte? CalibStat);

/// <summary>
/// BNO055 register map and the pure codecs every SDK shares
/// (contracts/13_SENSOR_BNO055.md §4, Bosch BST-BNO055-DS000 rev 1.8) — the
/// "base" layer. Mirrors <c>depz_sensor_sdk/bno055/regs.py</c> 1:1. Nothing
/// here touches the wire; <c>vectors/bno055.json</c> pins it. Scaling is
/// raw / LSB with <see cref="Bno055Units"/> and the fixed LSB constants below.
/// OUT OF SCOPE (as for every C# sensor): the live driver over the bridge
/// (mode switching, boot / fusion-start polls, page discipline, self-test).
/// </summary>
public static class Bno055Regs
{
    // ── page 0 ──────────────────────────────────────────────────────────────
    public const byte ChipId = 0x00;
    public const byte PageId = 0x07;
    public const byte AccData = 0x08;        // x, y, z  i16
    public const byte MagData = 0x0E;
    public const byte GyrData = 0x14;
    public const byte EulData = 0x1A;        // heading, roll, pitch
    public const byte QuaData = 0x20;        // w, x, y, z
    public const byte LiaData = 0x28;        // linear acceleration (gravity removed)
    public const byte GrvData = 0x2E;        // gravity vector
    public const byte Temp = 0x34;           // i8
    public const byte CalibStat = 0x35;
    public const byte StResult = 0x36;

    /// <summary>Clear-on-read — never part of a routine block read.</summary>
    public const byte IntSta = 0x37;

    public const byte SysClkStatus = 0x38;
    public const byte SysStatus = 0x39;
    public const byte SysErr = 0x3A;
    public const byte UnitSel = 0x3B;
    public const byte OprMode = 0x3D;
    public const byte PwrMode = 0x3E;
    public const byte SysTrigger = 0x3F;
    public const byte TempSource = 0x40;
    public const byte AxisMapConfig = 0x41;
    public const byte AxisMapSign = 0x42;

    /// <summary>9 × i16, row-major, 1.0 = 16384.</summary>
    public const byte SicMatrix = 0x43;

    /// <summary>acc/mag/gyr offsets + acc/mag radius, <see cref="CalibProfileLen"/> bytes.</summary>
    public const byte CalibProfile = 0x55;

    public const int CalibProfileLen = 22;

    // ── page 1 ──────────────────────────────────────────────────────────────
    public const byte P1AccConfig = 0x08;
    public const byte P1MagConfig = 0x09;
    public const byte P1GyrConfig0 = 0x0A;
    public const byte P1GyrConfig1 = 0x0B;
    public const byte P1AccSleepConfig = 0x0C;
    public const byte P1GyrSleepConfig = 0x0D;
    public const byte P1IntMsk = 0x0F;
    public const byte P1IntEn = 0x10;

    /// <summary>Page-1 0x11..0x1F are the motion-interrupt settings, written raw.</summary>
    public const byte P1AccAmThres = 0x11;

    public const byte P1GyrAmSet = 0x1F;
    public const byte P1UniqueId = 0x50;     // 16 bytes
    public const int UniqueIdLen = 16;

    /// <summary>The block that carries every output channel: 0x08 (ACC_DATA_X_LSB) … 0x35 (CALIB_STAT).</summary>
    public const byte FullBlockAddr = AccData;

    public const byte FullBlockLen = CalibStat - AccData + 1;   // 46

    /// <summary>Quaternion only — the cheapest orientation read (8 bytes, ~1.2 ms of bus).</summary>
    public const byte QuatBlockAddr = QuaData;

    public const byte QuatBlockLen = 8;

    // SYS_TRIGGER bits (preserve CLK_SEL when writing).
    public const byte SysTriggerSelfTest = 0x01;
    public const byte SysTriggerRstSys = 0x20;
    public const byte SysTriggerRstInt = 0x40;
    public const byte SysTriggerClkSel = 0x80;

    // INT_EN / INT_MSK / INT_STA bits. The DRDY bits drive the pin only on sensor
    // firmware 03.14+; the boards in this line carry 03.11.
    public const byte IntAccBsxDrdy = 0x01;
    public const byte IntMagDrdy = 0x02;
    public const byte IntGyrAm = 0x04;
    public const byte IntGyrHighRate = 0x08;
    public const byte IntGyrDrdy = 0x10;
    public const byte IntAccHighG = 0x20;
    public const byte IntAccAm = 0x40;
    public const byte IntAccNm = 0x80;

    // ST_RESULT bits (1 = passed).
    public const byte StAcc = 0x01;
    public const byte StMag = 0x02;
    public const byte StGyr = 0x04;
    public const byte StMcu = 0x08;
    public const byte ExpectedSelfTest = StAcc | StMag | StGyr | StMcu;

    /// <summary>MAG_DATA LSB per µT, not selectable.</summary>
    public const double MagLsb = 16.0;

    /// <summary>Quaternion LSB, 2^14, unit-less.</summary>
    public const double QuatLsb = 16384.0;

    /// <summary>
    /// Linear acceleration and gravity ignore the ACC_Unit bit: always m/s² at
    /// 100 LSB — measured on SW rev 03.11 (datasheet Tables 3-33/3-35 claim mg).
    /// </summary>
    public const double FusionAccelLsb = 100.0;

    /// <summary>True for the fusion operating modes (IMU and above).</summary>
    public static bool IsFusion(Bno055OprMode mode) => mode >= Bno055OprMode.Imu;

    // ── page-1 sensor configuration tables ──────────────────────────────────
    public static readonly IReadOnlyList<int> AccRangeG = new[] { 2, 4, 8, 16 };
    public static readonly IReadOnlyList<double> AccBandwidthHz = new[] { 7.81, 15.63, 31.25, 62.5, 125.0, 250.0, 500.0, 1000.0 };
    public static readonly IReadOnlyList<string> AccPowerNames =
        new[] { "normal", "suspend", "low power 1", "standby", "low power 2", "deep suspend" };
    public static readonly IReadOnlyList<int> GyrRangeDps = new[] { 2000, 1000, 500, 250, 125 };
    public static readonly IReadOnlyList<int> GyrBandwidthHz = new[] { 523, 230, 116, 47, 23, 12, 64, 32 };
    public static readonly IReadOnlyList<string> GyrPowerNames =
        new[] { "normal", "fast power up", "deep suspend", "suspend", "advanced powersave" };
    public static readonly IReadOnlyList<int> MagRateHz = new[] { 2, 6, 8, 10, 15, 20, 25, 30 };
    public static readonly IReadOnlyList<string> MagOprNames = new[] { "low power", "regular", "enhanced regular", "high accuracy" };
    public static readonly IReadOnlyList<string> MagPowerNames = new[] { "normal", "sleep", "suspend", "force" };

    // ── SIC matrix ──────────────────────────────────────────────────────────

    /// <summary>Soft-iron matrix identity, 1.0 = 16384.</summary>
    public static readonly IReadOnlyList<short> SicIdentity = new short[] { 16384, 0, 0, 0, 16384, 0, 0, 0, 16384 };

    /// <summary>Soft-iron matrix, 9 × i16 LE row-major, 1.0 = 16384 (datasheet §3.11.4).</summary>
    /// <exception cref="ArgumentException">Unless exactly 9 elements.</exception>
    public static byte[] PackSicMatrix(IReadOnlyList<short> m)
    {
        if (m.Count != 9)
            throw new ArgumentException("SIC matrix has 9 elements", nameof(m));
        var buf = new byte[18];
        for (int i = 0; i < 9; i++)
            BinaryPrimitives.WriteInt16LittleEndian(buf.AsSpan(2 * i), m[i]);
        return buf;
    }

    public static short[] UnpackSicMatrix(ReadOnlySpan<byte> data)
    {
        var m = new short[9];
        for (int i = 0; i < 9; i++)
            m[i] = BinaryPrimitives.ReadInt16LittleEndian(data[(2 * i)..]);
        return m;
    }

    // ── output block decode ─────────────────────────────────────────────────

    /// <summary>Unpack whatever channels the register window starting at <paramref name="addr"/> holds.</summary>
    public static Bno055RawBlock DecodeBlock(byte addr, ReadOnlySpan<byte> data)
    {
        int end = addr + data.Length;
        bool Covers(int reg, int bytes) => addr <= reg && reg + bytes <= end;
        short W(ReadOnlySpan<byte> d, int reg, int word) =>
            BinaryPrimitives.ReadInt16LittleEndian(d[(reg - addr + 2 * word)..]);
        Bno055Vec3? Vec(ReadOnlySpan<byte> d, int reg) =>
            Covers(reg, 6) ? new Bno055Vec3(W(d, reg, 0), W(d, reg, 1), W(d, reg, 2)) : null;

        return new Bno055RawBlock(
            Vec(data, AccData),
            Vec(data, MagData),
            Vec(data, GyrData),
            Covers(EulData, 6) ? new Bno055Euler(W(data, EulData, 0), W(data, EulData, 1), W(data, EulData, 2)) : null,
            Covers(QuaData, 8)
                ? new Bno055Quat(W(data, QuaData, 0), W(data, QuaData, 1), W(data, QuaData, 2), W(data, QuaData, 3))
                : null,
            Vec(data, LiaData),
            Vec(data, GrvData),
            Covers(Temp, 1) ? (sbyte)data[Temp - addr] : null,
            Covers(CalibStat, 1) ? data[CalibStat - addr] : null);
    }
}
