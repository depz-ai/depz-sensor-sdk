using System.Buffers.Binary;

namespace Depz.Sensor.Vl53l8;

/// <summary>VL53L8 register-bridge command opcodes (contracts/04_SENSOR_VL53L8.md).</summary>
public enum Vl53l8Cmd
{
    ReadReg = 0x32,
    WriteReg = 0x33,
    // 0x34 intentionally unused (gap in the firmware's ID sequence)
    StartStream = 0x35,
    StopStream = 0x36,
}

/// <summary>VL53L8 report opcodes.</summary>
public enum Vl53l8Rpt
{
    RegData = 0x91,
    Vl53Frame = 0x93,
}

/// <summary>Wire limits (contract 04).</summary>
public static class Vl53l8Wire
{
    /// <summary>Bytes of frame data per RPT_VL53_FRAME chunk.</summary>
    public const int StreamChunkMax = 1528;

    /// <summary>Max frame_size accepted by START_STREAM.</summary>
    public const int StreamTotalMax = 8192;
}

/// <summary>
/// One RPT_VL53_FRAME chunk: device timestamp, the full frame size, this
/// chunk's byte offset into the frame, and the chunk payload.
/// </summary>
public sealed record FrameChunk(ulong TimestampUs, int FullSize, int Offset, byte[] Data)
{
    /// <summary>Parse a RPT_VL53_FRAME payload: ts u64 LE, full u16 LE, off u16 LE, then data.</summary>
    public static FrameChunk Unpack(ReadOnlySpan<byte> payload)
    {
        ulong ts = BinaryPrimitives.ReadUInt64LittleEndian(payload);
        int full = BinaryPrimitives.ReadUInt16LittleEndian(payload[8..]);
        int off = BinaryPrimitives.ReadUInt16LittleEndian(payload[10..]);
        return new FrameChunk(ts, full, off, payload[12..].ToArray());
    }
}

/// <summary>
/// Rebuilds full sensor frames from chunked RPT_VL53_FRAME reports.
///
/// Rules (contract 04): reset on offset==0; chunks must be contiguous — a gap
/// discards the frame in progress; a frame completes when the accumulated
/// bytes equal <c>FullSize</c>. Byte-exact with the reference Python
/// <c>FrameReassembler</c>.
/// </summary>
public sealed class FrameReassembler
{
    private byte[] _buf = Array.Empty<byte>();
    private int _fullSize;
    private ulong _timestampUs;

    public int Completed { get; private set; }
    public int Discarded { get; private set; }

    /// <summary>Feed one chunk; returns (timestampUs, frameBytes) when a frame completes, else null.</summary>
    public (ulong TimestampUs, byte[] Frame)? Feed(FrameChunk chunk)
    {
        if (chunk.Offset == 0)
        {
            if (_buf.Length != 0 && _buf.Length != _fullSize)
                Discarded++;
            _buf = (byte[])chunk.Data.Clone();
            _fullSize = chunk.FullSize;
            _timestampUs = chunk.TimestampUs;
        }
        else if (chunk.Offset == _buf.Length && _fullSize == chunk.FullSize && _buf.Length != 0)
        {
            var grown = new byte[_buf.Length + chunk.Data.Length];
            Array.Copy(_buf, grown, _buf.Length);
            Array.Copy(chunk.Data, 0, grown, _buf.Length, chunk.Data.Length);
            _buf = grown;
        }
        else
        {
            if (_buf.Length != 0)
                Discarded++;
            _buf = Array.Empty<byte>();
            _fullSize = 0;
            return null;
        }

        if (_buf.Length == _fullSize && _fullSize > 0)
        {
            byte[] frame = _buf;
            _buf = Array.Empty<byte>();
            _fullSize = 0;
            Completed++;
            return (_timestampUs, frame);
        }
        if (_buf.Length > _fullSize)
        {
            Discarded++;
            _buf = Array.Empty<byte>();
            _fullSize = 0;
        }
        return null;
    }
}
