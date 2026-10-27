using System.Buffers.Binary;
using Depz.Sensor.Devices;

namespace Depz.Sensor.Tmf8829;

/// <summary>A frame whose bytes do not match its own header (contract 14 §3).</summary>
public sealed class Tmf8829FrameException : FormatException
{
    public Tmf8829FrameException(string message) : base(message) { }
}

/// <summary>
/// Pre-header, header and footer of one frame of any kind
/// (contracts/14_SENSOR_TMF8829.md §3). A frame is the sensor bytes from
/// register 0xFA on: 5-byte pre-header (FIFOSTATUS, SYSTICK), 16-byte header,
/// data, 12-byte footer. <see cref="Systick"/>, <see cref="T0Integration"/>
/// and <see cref="T1Integration"/> are sensor SYSTICK (125 kHz) values.
/// <see cref="Temperature"/> holds 3 signed bytes, <see cref="RefPos"/> 2 u16.
/// </summary>
public sealed record Tmf8829FrameInfo(
    byte FifoStatus,
    uint Systick,
    byte Id,
    byte Layout,
    ushort Payload,
    uint FNumber,
    sbyte[] Temperature,
    byte Bdv,
    ushort[] RefPos,
    uint T0Integration,
    uint T1Integration,
    byte FrameStatus,
    byte Reserved,
    ushort Eof,
    int Size)
{
    /// <summary><see cref="Tmf8829Frames.FidResults"/>, <see cref="Tmf8829Frames.FidHistograms"/> or <see cref="Tmf8829Frames.FidRefSpad"/>.</summary>
    public int Kind => Id & Tmf8829Frames.FidMask;

    /// <summary>Focal-plane mode: 0 8×8A, 1 8×8B, 2 16×16, 3 32×32, 4 32×32s, 5 48×32.</summary>
    public int FpMode => Id & 0x0F;

    /// <summary>32×32/48×32: the second (odd-row) half-frame.</summary>
    public bool IsSecondHalf => (Layout & Tmf8829Frames.LayoutSubframe) != 0;

    /// <summary>Distances in mm (footer.reserved b0, preset 8x8-xr), else 0.25 mm.</summary>
    public bool DistanceInMm => (Reserved & Tmf8829Frames.FooterDistInMm) != 0;

    /// <summary>What is wrong with the frame, human-readable; empty when fine.</summary>
    public List<string> Problems()
    {
        var o = new List<string>();
        if (Eof != Tmf8829Frames.FrameEof)
            o.Add($"EOF 0x{Eof:X4}");
        if ((FrameStatus & Tmf8829Frames.FrameValid) == 0)
            o.Add($"status 0x{FrameStatus:X2} not valid");
        if ((FrameStatus & Tmf8829Frames.FrameAborted) != 0)
            o.Add("aborted");
        foreach (var (bit, name) in Tmf8829Frames.FrameWarnings)
            if ((FrameStatus & bit) != 0)
                o.Add(name);
        if ((FifoStatus & Tmf8829Frames.FifoTxUnderrun) != 0)
            o.Add("txfifo_underrun");
        if ((FifoStatus & Tmf8829Frames.FifoRxOverrun) != 0)
            o.Add("rxfifo_overrun");
        return o;
    }
}

/// <summary>
/// Decoded results of one frame or of a merged measurement. Raw sensor
/// integers: distance in 0.25 mm (or mm with the footer flag), not yet
/// clock-corrected. Per-peak arrays (<see cref="Distance"/>, <see cref="Snr"/>,
/// <see cref="Signal"/>) are flat <c>[peaks][rows][cols]</c>, index
/// <c>(p * rows + r) * cols + c</c>; <see cref="Noise"/> / <see cref="Xtalk"/>
/// are flat <c>[rows][cols]</c>. Absent fields are null.
/// </summary>
public sealed record Tmf8829PixelResults(
    int Peaks,
    int GridRows,
    int GridCols,
    ushort[]? Distance,
    byte[]? Snr,
    ushort[]? Signal,
    ushort[]? Noise,
    ushort[]? Xtalk)
{
    private bool HasArrays => Distance is not null || Noise is not null || Xtalk is not null;

    /// <summary>Rows of the arrays (0 when the result format carries none).</summary>
    public int Rows => HasArrays ? GridRows : 0;

    /// <summary>Columns of the arrays (0 when the result format carries none).</summary>
    public int Cols => HasArrays ? GridCols : 0;

    /// <summary>Raw distance of peak <paramref name="p"/> at (<paramref name="r"/>, <paramref name="c"/>).</summary>
    public ushort DistanceAt(int p, int r, int c) =>
        (Distance ?? throw new InvalidStateException("no peaks in this result format"))[(p * GridRows + r) * GridCols + c];
}

/// <summary>A raw histogram frame (25 377 B); decoding the bins is left to the caller (ams histogram layout).</summary>
public sealed record Tmf8829HistogramFrame(Tmf8829FrameInfo Info, byte[] Raw)
{
    /// <summary>The bytes between header and footer.</summary>
    public ReadOnlySpan<byte> Data =>
        Raw.AsSpan(Tmf8829Frames.FrameHead, Raw.Length - Tmf8829Frames.FrameHead - Tmf8829Frames.FooterSize);
}

/// <summary>
/// TMF8829 frame layout and pixel decoding (contract 14 §3). Mirrors
/// <c>depz_sensor_sdk/tmf8829/frames.py</c>.
/// </summary>
public static class Tmf8829Frames
{
    public const int PreHeaderSize = 5;
    public const int HeaderSize = 16;
    public const int FooterSize = 12;

    /// <summary>21 B the bridge reads at 0xFA.</summary>
    public const int FrameHead = PreHeaderSize + HeaderSize;

    public const ushort FrameEof = 0xE0F7;

    public const int FidMask = 0xF0;
    public const int FidResults = 0x10;
    public const int FidHistograms = 0x20;
    public const int FidRefSpad = 0x30;

    public const int LayoutPeaks = 0x07;
    public const int LayoutSignal = 0x08;
    public const int LayoutNoise = 0x10;
    public const int LayoutXtalk = 0x20;

    /// <summary>32×32/48×32: set on the second (odd-row) half-frame.</summary>
    public const int LayoutSubframe = 0x40;

    public const int FrameValid = 0x01;
    public const int FrameAborted = 0xC0;

    /// <summary>frame_status warning bits, in report order.</summary>
    public static readonly IReadOnlyList<(int Bit, string Name)> FrameWarnings = new[]
    {
        (0x08, "HV_CP_OVERLOAD"),
        (0x10, "VCDRV_OVERLOAD"),
        (0x20, "VCDRV_BURST_EXCEEDED"),
    };

    /// <summary>footer.reserved b0: distances in mm, else 0.25 mm.</summary>
    public const int FooterDistInMm = 0x01;

    public const int FifoTxUnderrun = 0x10;
    public const int FifoRxOverrun = 0x20;

    /// <summary>fp_mode → (rows, cols) of the full focal plane.</summary>
    public static readonly IReadOnlyDictionary<int, (int Rows, int Cols)> FpGrid = new Dictionary<int, (int, int)>
    {
        [0] = (8, 8), [1] = (8, 8), [2] = (16, 16), [3] = (32, 32), [4] = (32, 32), [5] = (32, 48),
    };

    public static readonly IReadOnlyDictionary<int, string> FpNames = new Dictionary<int, string>
    {
        [0] = "8x8A", [1] = "8x8B", [2] = "16x16", [3] = "32x32", [4] = "32x32s", [5] = "48x32",
    };

    /// <summary>Full frame size (pre-header included) from its first 21 bytes: 5 + 4 + payload.</summary>
    public static int FrameSize(ReadOnlySpan<byte> head) =>
        PreHeaderSize + 4 + BinaryPrimitives.ReadUInt16LittleEndian(head[7..]);

    /// <summary>Parse pre-header, header and footer.</summary>
    /// <exception cref="Tmf8829FrameException">Frame shorter than header + footer.</exception>
    public static Tmf8829FrameInfo ParseFrame(ReadOnlySpan<byte> raw)
    {
        if (raw.Length < FrameHead + FooterSize)
            throw new Tmf8829FrameException($"frame of {raw.Length} B is shorter than header + footer");
        var h = raw[PreHeaderSize..];
        var f = raw[(raw.Length - FooterSize)..];
        return new Tmf8829FrameInfo(
            raw[0],
            BinaryPrimitives.ReadUInt32LittleEndian(raw[1..]),
            h[0],
            h[1],
            BinaryPrimitives.ReadUInt16LittleEndian(h[2..]),
            BinaryPrimitives.ReadUInt32LittleEndian(h[4..]),
            new[] { (sbyte)h[8], (sbyte)h[9], (sbyte)h[10] },
            h[11],
            new[] { BinaryPrimitives.ReadUInt16LittleEndian(h[12..]), BinaryPrimitives.ReadUInt16LittleEndian(h[14..]) },
            BinaryPrimitives.ReadUInt32LittleEndian(f),
            BinaryPrimitives.ReadUInt32LittleEndian(f[4..]),
            f[8],
            f[9],
            BinaryPrimitives.ReadUInt16LittleEndian(f[10..]),
            raw.Length);
    }

    /// <summary>(rows, cols) one results frame carries: half the rows for 32×32/48×32.</summary>
    public static (int Rows, int Cols) FrameRows(int fpMode)
    {
        var (rows, cols) = FpGrid[fpMode];
        return rows == 32 ? (rows / 2, cols) : (rows, cols);
    }

    /// <summary>Bytes per results pixel for a layout byte.</summary>
    public static int PixelSize(int layout)
    {
        int n = 0;
        if ((layout & LayoutNoise) != 0) n += 2;
        if ((layout & LayoutXtalk) != 0) n += 2;
        int peak = 3 + ((layout & LayoutSignal) != 0 ? 2 : 0);
        return n + (layout & LayoutPeaks) * peak;
    }

    /// <summary>
    /// Pixel results of one results frame, row-major: <c>[noise u16]</c>,
    /// <c>[xtalk u16]</c>, then peaks × (distance u16, snr u8 [, signal u16]).
    /// </summary>
    /// <exception cref="Tmf8829FrameException">Not a results frame, unknown fp_mode, or data size ≠ pixels × pixel size.</exception>
    public static Tmf8829PixelResults ParseResults(ReadOnlySpan<byte> raw, Tmf8829FrameInfo? info = null)
    {
        var f = info ?? ParseFrame(raw);
        if (f.Kind != FidResults)
            throw new Tmf8829FrameException($"frame id 0x{f.Id:X2} is not a results frame");
        if (!FpGrid.ContainsKey(f.FpMode))
            throw new Tmf8829FrameException($"unknown fp_mode {f.FpMode}");
        var (rows, cols) = FrameRows(f.FpMode);
        int layout = f.Layout;
        int pxSize = PixelSize(layout);
        int n = rows * cols;
        int dataSize = f.Size - FrameHead - FooterSize;
        if (pxSize * n != dataSize)
            throw new Tmf8829FrameException(
                $"result data {dataSize} B, expected {n} px × {pxSize} B (layout 0x{layout:X2})");

        int peaks = layout & LayoutPeaks;
        bool hasNoise = (layout & LayoutNoise) != 0;
        bool hasXtalk = (layout & LayoutXtalk) != 0;
        bool hasSignal = (layout & LayoutSignal) != 0 && peaks > 0;
        var noise = hasNoise ? new ushort[n] : null;
        var xtalk = hasXtalk ? new ushort[n] : null;
        var dist = peaks > 0 ? new ushort[peaks * n] : null;
        var snr = peaks > 0 ? new byte[peaks * n] : null;
        var signal = hasSignal ? new ushort[peaks * n] : null;

        var data = raw[FrameHead..];
        for (int i = 0; i < n; i++)
        {
            var px = data.Slice(i * pxSize, pxSize);
            int o = 0;
            if (noise is not null) { noise[i] = BinaryPrimitives.ReadUInt16LittleEndian(px[o..]); o += 2; }
            if (xtalk is not null) { xtalk[i] = BinaryPrimitives.ReadUInt16LittleEndian(px[o..]); o += 2; }
            for (int p = 0; p < peaks; p++)
            {
                dist![p * n + i] = BinaryPrimitives.ReadUInt16LittleEndian(px[o..]);
                snr![p * n + i] = px[o + 2];
                o += 3;
                if (signal is not null) { signal[p * n + i] = BinaryPrimitives.ReadUInt16LittleEndian(px[o..]); o += 2; }
            }
        }
        return new Tmf8829PixelResults(peaks, rows, cols, dist, snr, signal, noise, xtalk);
    }

    /// <summary>32×32/48×32: the first half-frame holds the even rows, the second the odd ones.</summary>
    public static Tmf8829PixelResults MergeHalves(Tmf8829PixelResults first, Tmf8829PixelResults second)
    {
        int rows = first.GridRows, cols = first.GridCols;

        T[]? Merge<T>(T[]? a, T[]? b)
        {
            if (a is null || b is null)
                return null;
            int planes = a.Length / (rows * cols);
            var m = new T[a.Length * 2];
            for (int p = 0; p < planes; p++)
                for (int i = 0; i < rows; i++)
                {
                    Array.Copy(a, (p * rows + i) * cols, m, (p * 2 * rows + 2 * i) * cols, cols);
                    Array.Copy(b, (p * rows + i) * cols, m, (p * 2 * rows + 2 * i + 1) * cols, cols);
                }
            return m;
        }

        return new Tmf8829PixelResults(
            first.Peaks,
            rows * 2,
            cols,
            Merge(first.Distance, second.Distance),
            Merge(first.Snr, second.Snr),
            Merge(first.Signal, second.Signal),
            Merge(first.Noise, second.Noise),
            Merge(first.Xtalk, second.Xtalk));
    }
}
