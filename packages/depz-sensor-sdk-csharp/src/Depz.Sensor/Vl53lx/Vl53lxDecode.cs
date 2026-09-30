using System.Buffers.Binary;
using Depz.Sensor.Vl53l4;

namespace Depz.Sensor.Vl53lx;

/// <summary>
/// Which ULD reads the die block (contract 12 §4). They differ only in the
/// signal-rate byte offset S and the per-SPAD scale K.
/// </summary>
public enum Vl53lxDieVariant
{
    /// <summary>VL53L4CD ULD — also the L3CX ULP and L4CX-as-L4CD: (S, K) = (5, 256).</summary>
    L4,

    /// <summary>VL53L1X ULD — crosstalk-corrected peak signal at 0x0098: (S, K) = (15, 25).</summary>
    L1,
}

/// <summary>The 17-byte die block decoded as one ULD reads it.</summary>
public sealed record Vl53lxDieResult(
    int RangeStatus,
    int DistanceMm,
    int SigmaMm,
    int SignalRateKcps,
    int AmbientRateKcps,
    int SignalPerSpadKcps,
    int AmbientPerSpadKcps,
    int NumberOfSpad,
    int StreamCount);

/// <summary>
/// Raw fields of the VL53L0X 12-byte block at 0x14. The PAL range status,
/// sigma and dmax need the device data cached by init — full driver, not base.
/// </summary>
public sealed record Vl53lxL0xRaw(
    int DistanceRaw,
    int DeviceRangeStatus,
    long SignalRateMcps1616,
    long AmbientRateMcps1616,
    int EffectiveSpadCount88);

/// <summary>
/// Status bytes and the 24 photon bins of the 83-byte histogram block at
/// 0x0088. Bins → targets is the full driver's job.
/// </summary>
public sealed record Vl53lxHistogramRaw(
    int InterruptStatus,
    int RangeStatus,
    int ReportStatus,
    int StreamCount,
    int DssActualEffectiveSpads,
    int ReferencePhase,
    int VcselStart,
    IReadOnlyList<int> Bins);

/// <summary>
/// Stateless decoders of the blocks the 1D-family bridge streams (contract 12
/// §4) — the "base" every SDK implements. What a block says on its own,
/// without the driver state an init leaves behind.
/// </summary>
public static class Vl53lxDecode
{
    /// <summary>Die result block (L1CX, L1CB, L3CX, L4CD, L4CX light drivers).</summary>
    public const ushort DieBlockAddr = 0x0089;
    public const int DieBlockLen = 17;

    /// <summary>VL53L0X result block (register-address width 1).</summary>
    public const ushort L0xBlockAddr = 0x14;
    public const int L0xBlockLen = 12;

    /// <summary>Histogram block, RESULT__INTERRUPT_STATUS .. RESULT__HISTOGRAM_BIN_23_0_LSB.</summary>
    public const ushort HistogramBlockAddr = 0x0088;
    public const int HistogramBlockLen = 0x00DA - 0x0088 + 1; // 83
    public const int HistogramBins = 24;

    private const int HistBin0 = 0x008E - HistogramBlockAddr;
    private const int HistBin23Low = 0x00D5 - HistogramBlockAddr;
    private const int HistReferencePhase = 0x00D6 - HistogramBlockAddr;
    private const int HistVcselStart = 0x00D8 - HistogramBlockAddr;
    private const int HistBin23Msb = 0x00D9 - HistogramBlockAddr;
    private const int HistBin23Lsb = 0x00DA - HistogramBlockAddr;

    /// <summary>(signal-rate byte offset, per-SPAD scale K) of a die variant.</summary>
    public static (int SignalAt, int K) DieVariantParams(Vl53lxDieVariant variant) => variant switch
    {
        Vl53lxDieVariant.L4 => (5, 256),
        Vl53lxDieVariant.L1 => (15, 25),
        _ => throw new ArgumentOutOfRangeException(nameof(variant)),
    };

    /// <summary>Parse the vector/wire name of a die variant (<c>"l4"</c> / <c>"l1"</c>).</summary>
    public static Vl53lxDieVariant ParseDieVariant(string name) => name switch
    {
        "l4" => Vl53lxDieVariant.L4,
        "l1" => Vl53lxDieVariant.L1,
        _ => throw new ArgumentException($"unknown die variant: {name}", nameof(name)),
    };

    /// <summary>
    /// The 17-byte die block (0x0089..0x0099) as the named ULD reads it. The
    /// status maps through the contract-10 table (<see cref="Vl53l4Uld.StatusRtn"/>);
    /// raw status ≥ 24 passes through unmapped.
    /// </summary>
    /// <exception cref="ArgumentException">block shorter than 17 bytes.</exception>
    public static Vl53lxDieResult DecodeDieBlock(ReadOnlySpan<byte> raw, Vl53lxDieVariant variant = Vl53lxDieVariant.L4)
    {
        if (raw.Length < DieBlockLen)
            throw new ArgumentException($"die result block needs {DieBlockLen} bytes, got {raw.Length}", nameof(raw));
        var (signalAt, k) = DieVariantParams(variant);
        int status = raw[0] & 0x1F;
        if (status < Vl53l4Uld.StatusRtn.Length)
            status = Vl53l4Uld.StatusRtn[status];
        int rawSpads = BinaryPrimitives.ReadUInt16BigEndian(raw[3..]); // 8.8
        int signal = BinaryPrimitives.ReadUInt16BigEndian(raw[signalAt..]) * 8;
        int ambient = BinaryPrimitives.ReadUInt16BigEndian(raw[7..]) * 8;
        return new Vl53lxDieResult(
            RangeStatus: status,
            DistanceMm: BinaryPrimitives.ReadUInt16BigEndian(raw[13..]),
            SigmaMm: BinaryPrimitives.ReadUInt16BigEndian(raw[9..]) / 4,
            SignalRateKcps: signal,
            AmbientRateKcps: ambient,
            SignalPerSpadKcps: rawSpads == 0 ? 0 : signal * k / rawSpads,
            AmbientPerSpadKcps: rawSpads == 0 ? 0 : ambient * k / rawSpads,
            NumberOfSpad: rawSpads / 256,
            StreamCount: raw[2]);
    }

    /// <summary>
    /// Raw fields of the VL53L0X block at 0x14 (VL53L0X_GetRangingMeasurementData
    /// before the PAL status/sigma step). Distance is mm (quarter-mm when
    /// RangeFractionalEnable); rates are FixPoint16.16 Mcps (9.7 on the wire &lt;&lt; 9).
    /// </summary>
    /// <exception cref="ArgumentException">block shorter than 12 bytes.</exception>
    public static Vl53lxL0xRaw DecodeL0xRaw(ReadOnlySpan<byte> raw)
    {
        if (raw.Length < L0xBlockLen)
            throw new ArgumentException($"VL53L0X result block needs {L0xBlockLen} bytes, got {raw.Length}", nameof(raw));
        return new Vl53lxL0xRaw(
            DistanceRaw: BinaryPrimitives.ReadUInt16BigEndian(raw[10..]),
            DeviceRangeStatus: raw[0],
            SignalRateMcps1616: (long)BinaryPrimitives.ReadUInt16BigEndian(raw[6..]) << 9,
            AmbientRateMcps1616: (long)BinaryPrimitives.ReadUInt16BigEndian(raw[8..]) << 9,
            EffectiveSpadCount88: BinaryPrimitives.ReadUInt16BigEndian(raw[2..]));
    }

    /// <summary>
    /// The 83-byte histogram block at 0x0088: status bytes and the 24 bins of
    /// 3 big-endian bytes. Bin 23's low byte is carried separately as an
    /// MSB/LSB pair — <c>((MSB &lt;&lt; 2) + LSB) &amp; 0xFF</c> — and patched
    /// in (on a copy; the input is not modified) before the bins are read.
    /// </summary>
    /// <exception cref="ArgumentException">block shorter than 83 bytes.</exception>
    public static Vl53lxHistogramRaw DecodeHistogramRaw(ReadOnlySpan<byte> raw)
    {
        if (raw.Length < HistogramBlockLen)
            throw new ArgumentException($"histogram block needs {HistogramBlockLen} bytes, got {raw.Length}", nameof(raw));
        byte[] buf = raw[..HistogramBlockLen].ToArray();
        buf[HistBin23Low] = (byte)(((buf[HistBin23Msb] << 2) + buf[HistBin23Lsb]) & 0xFF);
        var bins = new int[HistogramBins];
        for (int i = 0; i < HistogramBins; i++)
        {
            int o = HistBin0 + 3 * i;
            bins[i] = (buf[o] << 16) | (buf[o + 1] << 8) | buf[o + 2];
        }
        return new Vl53lxHistogramRaw(
            InterruptStatus: buf[0],
            RangeStatus: buf[1],
            ReportStatus: buf[2],
            StreamCount: buf[3],
            DssActualEffectiveSpads: BinaryPrimitives.ReadUInt16BigEndian(buf.AsSpan(4)),
            ReferencePhase: BinaryPrimitives.ReadUInt16BigEndian(buf.AsSpan(HistReferencePhase)),
            VcselStart: buf[HistVcselStart],
            Bins: bins);
    }
}
