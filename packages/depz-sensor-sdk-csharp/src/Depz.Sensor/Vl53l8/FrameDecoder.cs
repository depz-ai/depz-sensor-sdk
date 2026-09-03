using System.Buffers.Binary;

namespace Depz.Sensor.Vl53l8;

/// <summary>
/// The two VL53L8 ToF silicon variants in the DEPZ family. Both stream the
/// <b>same</b> ULD results-frame layout, so <see cref="Vl53l8FrameDecoder"/> and
/// the <see cref="Vl53l8Advanced"/> DCI codecs serve both; the differences are:
/// <list type="bullet">
///   <item>USB product id — <see cref="Ch"/> ships as production PID 0xED40;
///   <see cref="Cx"/> is the development default and enumerates under the raw
///   ST VID/PID.</item>
///   <item>Results-frame footer-id offset — CX FW (ULD 2.1.0) echoes the frame
///   id 12 bytes before the end, CH FW (VL53LMZ 2.0.16) 4 bytes
///   (<see cref="Vl53l8FrameDecoder.ForVariant"/>).</item>
///   <item>CNH histograms — CH-only, not yet decoded (<see cref="Vl53l8Cnh"/>).</item>
/// </list>
/// </summary>
public enum Vl53l8Variant
{
    /// <summary>VL53L8CX — base ToF; development-default silicon.</summary>
    Cx,

    /// <summary>VL53L8CH — CX plus CNH histograms and its own production USB PID (0xED40).</summary>
    Ch,
}

/// <summary>
/// One decoded ranging frame. Per-zone arrays are sized to the active
/// resolution (16 for 4×4, 64 for 8×8); zone index runs row-major. Raw wire
/// integers are preserved; <see cref="DistanceMm"/> and <see cref="RangeSigmaMm"/>
/// carry the ST GetRangingData fixed-point scaling applied (÷4 and ÷128).
/// </summary>
public sealed record Vl53l8Frame(
    ulong TimestampUs,
    int Resolution,
    int SiliconTempDegc,
    int[] DistanceMm,
    byte[] TargetStatus,
    byte[] NbTargetDetected,
    uint[] SignalPerSpad,
    uint[] AmbientPerSpad,
    uint[] NbSpadsEnabled,
    double[] RangeSigmaMm,
    byte[] Reflectance);

/// <summary>
/// VL53L8 results-frame decoder (contracts/04_SENSOR_VL53L8.md). Verbatim port
/// of the verifiable parse path of the ST ULD's GetRangingData / parse_frame.
///
/// Serves <b>both</b> ToF variants — VL53L8CX and VL53L8CH stream the identical
/// results-frame layout; only the frame-id footer offset differs per variant
/// (see <see cref="Vl53l8Variant"/> / <see cref="ForVariant"/>). The CH-only CNH
/// histogram block is a separate, not-yet-decoded extension point
/// (<see cref="Vl53l8Cnh"/>); the live register-bridge init/config that produces
/// these frames is hardware-dependent and out of scope (<see cref="Vl53l8Uld"/>).
/// </summary>
public sealed class Vl53l8FrameDecoder
{
    /// <summary>Raised when a frame's header/footer id words disagree (corrupt frame).</summary>
    public sealed class CorruptedFrameException : Exception
    {
        public CorruptedFrameException() : base("VL53L8CX CORRUPTED_FRAME (header/footer id mismatch)") { }
    }

    // Block-header indices (union Block_header idx[31:16]); NB_TARGET_PER_ZONE == 1.
    private const int MetadataIdx = 0x54B4;
    private const int AmbientRateIdx = 0x54D0;
    private const int SpadCountIdx = 0x55D0;
    private const int NbTargetDetectedIdx = 0xDB84;
    private const int SignalRateIdx = 0xDBC4;
    private const int RangeSigmaMmIdx = 0xDEC4;
    private const int DistanceIdx = 0xDF44;
    private const int ReflectanceEstPcIdx = 0xE044;
    private const int TargetStatusIdx = 0xE084;

    /// <summary>Frame-id footer offset for VL53L8CX FW (ULD 2.1.0): 12 bytes from end.</summary>
    public const int FooterIdOffsetCx = 12;

    /// <summary>Frame-id footer offset for VL53L8CH FW (VL53LMZ 2.0.16): 4 bytes from end.</summary>
    public const int FooterIdOffsetCh = 4;

    /// <summary>
    /// Footer-id byte offset from the frame end. The released CX FW (ULD 2.1.0)
    /// places the frame-id echo 12 bytes before the end; CH (VL53LMZ 2.0.16)
    /// uses 4. Default matches the CX silicon the replay fixture was captured on.
    /// </summary>
    private readonly int _footerIdOff;

    public Vl53l8FrameDecoder(int footerIdOff = FooterIdOffsetCx) => _footerIdOff = footerIdOff;

    /// <summary>
    /// Decoder tuned for a specific ToF variant (selects the frame-id footer
    /// offset). The decode body is shared — CX and CH stream the same frame.
    /// </summary>
    public static Vl53l8FrameDecoder ForVariant(Vl53l8Variant variant) => variant switch
    {
        Vl53l8Variant.Cx => new Vl53l8FrameDecoder(FooterIdOffsetCx),
        Vl53l8Variant.Ch => new Vl53l8FrameDecoder(FooterIdOffsetCh),
        _ => throw new ArgumentOutOfRangeException(nameof(variant)),
    };

    /// <summary>VL53L8CX_SwapBuffer: byte-reverse every 32-bit word; trailing bytes unchanged.</summary>
    public static byte[] SwapBuffer(ReadOnlySpan<byte> data)
    {
        int n = data.Length / 4;
        var outBytes = new byte[data.Length];
        for (int w = 0; w < n; w++)
        {
            outBytes[w * 4 + 0] = data[w * 4 + 3];
            outBytes[w * 4 + 1] = data[w * 4 + 2];
            outBytes[w * 4 + 2] = data[w * 4 + 1];
            outBytes[w * 4 + 3] = data[w * 4 + 0];
        }
        for (int i = n * 4; i < data.Length; i++)
            outBytes[i] = data[i];
        return outBytes;
    }

    private static (int Type, int Size, int Idx) BhFields(uint bh) =>
        ((int)(bh & 0xF), (int)((bh >> 4) & 0xFFF), (int)((bh >> 16) & 0xFFFF));

    // Floor division to match Python `//` for the signed distance scaling.
    private static int FloorDiv(int a, int b)
    {
        int q = a / b;
        if ((a % b != 0) && ((a < 0) != (b < 0)))
            q--;
        return q;
    }

    /// <summary>
    /// Parse one raw results frame (the reassembled bytes read from reg 0x00).
    /// Resolution is derived from the number-of-targets block length.
    /// </summary>
    public Vl53l8Frame ParseFrame(ulong timestampUs, byte[] raw)
    {
        int drs = raw.Length;
        byte[] buf = SwapBuffer(raw);

        var distance = new int[64];
        var targetStatus = new byte[64];
        var nbTargetDetected = new byte[64];
        var signalPerSpad = new uint[64];
        var ambientPerSpad = new uint[64];
        var nbSpadsEnabled = new uint[64];
        var rangeSigmaMm = new double[64];
        var reflectance = new byte[64];
        int siliconTemp = 0;

        int i = 16;
        while (i + 4 <= drs)
        {
            uint bh = BinaryPrimitives.ReadUInt32LittleEndian(buf.AsSpan(i));
            var (t, s, idx) = BhFields(bh);
            int msize = (0x1 < t && t < 0xD) ? t * s : s;
            // Exact-sized buffer: stop once a block would run into the footer.
            if (i + 4 + msize > drs)
                break;

            if (idx == MetadataIdx)
            {
                siliconTemp = (sbyte)buf[i + 12];
            }
            else if (idx == DistanceIdx)
            {
                distance = new int[msize / 2];
                for (int k = 0; k < distance.Length; k++)
                    distance[k] = BinaryPrimitives.ReadInt16LittleEndian(buf.AsSpan(i + 4 + k * 2));
            }
            else if (idx == TargetStatusIdx)
            {
                targetStatus = buf.AsSpan(i + 4, msize).ToArray();
            }
            else if (idx == NbTargetDetectedIdx)
            {
                nbTargetDetected = buf.AsSpan(i + 4, msize).ToArray();
            }
            else if (idx == SignalRateIdx)
            {
                signalPerSpad = ReadU32(buf, i + 4, msize / 4);
            }
            else if (idx == AmbientRateIdx)
            {
                ambientPerSpad = ReadU32(buf, i + 4, msize / 4);
            }
            else if (idx == SpadCountIdx)
            {
                nbSpadsEnabled = ReadU32(buf, i + 4, msize / 4);
            }
            else if (idx == RangeSigmaMmIdx)
            {
                rangeSigmaMm = new double[msize / 2];
                for (int k = 0; k < rangeSigmaMm.Length; k++)
                    rangeSigmaMm[k] = BinaryPrimitives.ReadUInt16LittleEndian(buf.AsSpan(i + 4 + k * 2));
            }
            else if (idx == ReflectanceEstPcIdx)
            {
                reflectance = buf.AsSpan(i + 4, msize).ToArray();
            }

            i += msize + 4;
        }

        // Fixed-point scaling (ST GetRangingData): distance ÷4, sigma ÷128.
        for (int k = 0; k < distance.Length; k++)
            distance[k] = FloorDiv(distance[k], 4);
        for (int k = 0; k < rangeSigmaMm.Length; k++)
            rangeSigmaMm[k] /= 128.0;

        // No target detected → status 255, per zone actually present this frame.
        int nzones = nbTargetDetected.Length;
        for (int z = 0; z < nzones; z++)
            if (nbTargetDetected[z] == 0 && z < targetStatus.Length)
                targetStatus[z] = 255;

        // Header/footer frame-id echo must match (variant-specific offset).
        if (buf[0x8] != buf[drs - _footerIdOff] || buf[0x9] != buf[drs - _footerIdOff + 1])
            throw new CorruptedFrameException();

        return new Vl53l8Frame(
            timestampUs,
            nzones,
            siliconTemp,
            distance,
            targetStatus,
            nbTargetDetected,
            signalPerSpad,
            ambientPerSpad,
            nbSpadsEnabled,
            rangeSigmaMm,
            reflectance);
    }

    private static uint[] ReadU32(byte[] buf, int off, int count)
    {
        var arr = new uint[count];
        for (int k = 0; k < count; k++)
            arr[k] = BinaryPrimitives.ReadUInt32LittleEndian(buf.AsSpan(off + k * 4));
        return arr;
    }
}
