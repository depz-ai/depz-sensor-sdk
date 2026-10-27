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
    /// <summary>Register bytes one READ_REG / WRITE_REG moves at most (the bridge's limit).</summary>
    public const int ChunkSize = 2048;

    /// <summary>VL53_READ_REG payload: addr u16 LE, length u16 LE.</summary>
    public static byte[] PackReadReg(int addr, int length)
    {
        Protocol.Wire.CheckUint("addr", addr, 16);
        Protocol.Wire.CheckUint("length", length, 16);
        var b = new byte[4];
        BinaryPrimitives.WriteUInt16LittleEndian(b, (ushort)addr);
        BinaryPrimitives.WriteUInt16LittleEndian(b.AsSpan(2), (ushort)length);
        return b;
    }

    /// <summary>VL53_WRITE_REG payload: addr u16 LE, then at least one data byte (contract 04 §2).</summary>
    public static byte[] PackWriteReg(int addr, ReadOnlySpan<byte> data)
    {
        Protocol.Wire.CheckUint("addr", addr, 16);
        if (data.Length == 0)
            throw new ArgumentException("WRITE_REG data must be at least 1 byte", nameof(data));
        var b = new byte[2 + data.Length];
        BinaryPrimitives.WriteUInt16LittleEndian(b, (ushort)addr);
        data.CopyTo(b.AsSpan(2));
        return b;
    }

    /// <summary>
    /// VL53_START_STREAM payload: the frame size the board reads per INT, u16 LE,
    /// 1..<see cref="StreamTotalMax"/> (contract 04 §4 rule 4 — the firmware refuses anything else too).
    /// </summary>
    public static byte[] PackStartStream(int frameSize)
    {
        Protocol.Wire.CheckUint("frameSize", frameSize, 16);
        if (frameSize < 1 || frameSize > StreamTotalMax)
            throw new ArgumentOutOfRangeException(nameof(frameSize), frameSize, $"frame_size must be 1..{StreamTotalMax}");
        var b = new byte[2];
        BinaryPrimitives.WriteUInt16LittleEndian(b, (ushort)frameSize);
        return b;
    }

    /// <summary>Bytes of frame data per RPT_VL53_FRAME chunk.</summary>
    public const int StreamChunkMax = 1528;

    /// <summary>Max frame_size accepted by START_STREAM.</summary>
    public const int StreamTotalMax = 8192;
}

/// <summary>
/// One RPT_VL53_FRAME chunk: device timestamp, the full frame size, this
/// chunk's byte offset into the frame, and the chunk payload.
/// </summary>
/// <summary>RPT_VL53_REG_DATA: the echoed READ_REG opcode, the MCU timestamp and the register bytes.</summary>
public sealed record Vl53l8RegData(int Cmd, ulong TimestampUs, byte[] Data)
{
    public static Vl53l8RegData Unpack(ReadOnlySpan<byte> payload)
    {
        Protocol.Wire.NeedBytes("RPT_VL53_REG_DATA", payload, 9);
        return new(payload[0], BinaryPrimitives.ReadUInt64LittleEndian(payload[1..]), payload[9..].ToArray());
    }
}

public sealed record FrameChunk(ulong TimestampUs, int FullSize, int Offset, byte[] Data)
{
    /// <summary>Parse a RPT_VL53_FRAME payload: ts u64 LE, full u16 LE, off u16 LE, then data.</summary>
    public static FrameChunk Unpack(ReadOnlySpan<byte> payload)
    {
        Protocol.Wire.NeedBytes("RPT_VL53_FRAME", payload, 12);
        ulong ts = BinaryPrimitives.ReadUInt64LittleEndian(payload);
        int full = BinaryPrimitives.ReadUInt16LittleEndian(payload[8..]);
        int off = BinaryPrimitives.ReadUInt16LittleEndian(payload[10..]);
        return new FrameChunk(ts, full, off, payload[12..].ToArray());
    }
}

/// <summary>
/// Rebuilds full sensor frames from chunked RPT_VL53_FRAME reports.
///
/// Rules (contract 04 §4): reset on offset==0; chunks must be contiguous — a
/// gap discards the frame in progress; a frame completes when the accumulated
/// bytes equal <c>FullSize</c>. A chunk out of the rule-4 bounds (more than
/// <c>chunkMax</c> bytes, <c>FullSize</c> over <see cref="Vl53l8Wire.StreamTotalMax"/>,
/// or running past <c>FullSize</c>) is damaged: dropped and counted as a lost
/// frame like the other damaged input, never thrown. Byte-exact with the
/// reference Python <c>FrameReassembler</c>.
/// </summary>
public sealed class FrameReassembler
{
    private readonly int _chunkMax;
    private byte[] _buf = Array.Empty<byte>();
    private int _fullSize;
    private ulong _timestampUs;

    public int Completed { get; private set; }
    public int Discarded { get; private set; }

    /// <param name="chunkMax">The device's VL53_STREAM_FRAME_MAX:
    /// <see cref="Vl53l8Wire.StreamChunkMax"/>, or <c>Vl53l7Wire.StreamChunkMax</c> (1536)
    /// on the I2C board (contract 11 §3).</param>
    public FrameReassembler(int chunkMax = Vl53l8Wire.StreamChunkMax)
    {
        _chunkMax = chunkMax;
    }

    /// <summary>Feed one chunk; returns (timestampUs, frameBytes) when a frame completes, else null.</summary>
    public (ulong TimestampUs, byte[] Frame)? Feed(FrameChunk chunk)
    {
        if (chunk.Data.Length > _chunkMax || chunk.FullSize > Vl53l8Wire.StreamTotalMax ||
            chunk.Offset + chunk.Data.Length > chunk.FullSize)
        {
            // rule 4: damaged — the frame in progress is lost (counted, rules
            // 1–2) and so is the frame an offset-0 chunk starts (counted); a
            // damaged orphan with nothing in progress is dropped like any orphan
            if (_buf.Length != 0)
                Discarded++;
            if (chunk.Offset == 0)
                Discarded++;
            _buf = Array.Empty<byte>();
            _fullSize = 0;
            return null;
        }
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
        return null;
    }
}
