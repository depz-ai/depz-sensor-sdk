using System.Buffers.Binary;

namespace Depz.Sensor.Tmf8829;

/// <summary>
/// TMF8829 SPI register-bridge command opcodes, protocol v1.00
/// (contracts/14_SENSOR_TMF8829.md §2). Same numbering as the VL53 bridges but
/// TMF8829 payloads: an 8-bit register address and self-sized frames. 0x34 is
/// reserved; 0x30/0x31 (AUX sync) answer ERR_CMD_NOT_SUPPORTED.
/// </summary>
public enum Tmf8829Cmd
{
    ReadReg = 0x32,
    WriteReg = 0x33,
    StartStream = 0x35,
    StopStream = 0x36,
    GetInfo = 0x37,
}

/// <summary>TMF8829 report opcodes (contract 14 §2).</summary>
public enum Tmf8829Rpt
{
    RegData = 0x91,

    /// <summary>RPT_TMF_INFO — note: carries <b>no</b> echoed command byte.</summary>
    Info = 0x92,

    /// <summary>RPT_TMF_FRAME — unsolicited frame chunk.</summary>
    Frame = 0x93,

    /// <summary>RPT_TMF_EVENT — unsolicited interrupt / bad-header event.</summary>
    Event = 0x94,
}

/// <summary><see cref="Tmf8829Event.Code"/> values.</summary>
public static class Tmf8829EventCode
{
    /// <summary>MOTION/PROXIMITY interrupt, no frame; its bits already cleared.</summary>
    public const byte Int = 0;

    /// <summary>Frame header refused, frame not read (expected once on STOP mid histogram dump).</summary>
    public const byte BadHeader = 1;
}

/// <summary>
/// Wire limits and command payload codecs of the TMF8829 bridge
/// (<c>APP_TMF8829</c>). The MCU is a thin SPI register bridge: the host runs
/// the sensor (firmware download, config page, presets, frame parsing, clock
/// correction). Mirrors <c>depz_sensor_sdk/protocol/tmf8829.py</c> 1:1.
/// TMF_STOP_STREAM and TMF_GET_INFO carry an empty payload.
/// </summary>
public static class Tmf8829Wire
{
    /// <summary>TMF_READ_REG <c>len</c> 1..1024.</summary>
    public const int ReadMax = 1024;

    /// <summary>TMF_WRITE_REG data 1..1023 bytes (1024 B buffer minus the address).</summary>
    public const int WriteMax = 1023;

    /// <summary>Frame bytes per RPT_TMF_FRAME.</summary>
    public const int ChunkMax = 1024;

    /// <summary>INT_STATUS bits the bridge serves (TMF_START_STREAM <c>int_mask</c>).</summary>
    public const byte IntResults = 0x01;
    public const byte IntMotion = 0x02;
    public const byte IntProximity = 0x04;
    public const byte IntHistograms = 0x08;
    public const byte IntMaskAll = 0x0F;

    /// <summary>TMF_START_STREAM flags: emit the ref-SPAD frame before each results frame.</summary>
    public const byte SfRefFrame = 0x01;

    public const int RegDataHeaderSize = 9;
    public const int FrameChunkHeaderSize = 21;
    public const int EventSize = 12;
    public const int InfoSize = 25;

    /// <summary>
    /// Command timeout while streaming: a 25 KB histogram frame in flight
    /// delays READ_REG / WRITE_REG replies (SDKs may use it for all commands;
    /// 200 ms otherwise).
    /// </summary>
    public static readonly TimeSpan RegisterTimeout = TimeSpan.FromSeconds(1);

    /// <summary>TMF_READ_REG payload: addr u8, len u16 LE (len 1..<see cref="ReadMax"/>; no addr+len limit — the sensor stays at 0xFF, the FIFO).</summary>
    public static byte[] PackReadReg(int addr, int len)
    {
        if (addr < 0 || addr > 0xFF)
            throw new ArgumentOutOfRangeException(nameof(addr), addr, "addr must be 0..255");
        if (len < 1 || len > ReadMax)
            throw new ArgumentOutOfRangeException(nameof(len), len, $"length must be 1..{ReadMax}");
        var buf = new byte[3];
        buf[0] = (byte)addr;
        BinaryPrimitives.WriteUInt16LittleEndian(buf.AsSpan(1), (ushort)len);
        return buf;
    }

    /// <summary>TMF_WRITE_REG payload: addr u8, then data[1..<see cref="WriteMax"/>].</summary>
    public static byte[] PackWriteReg(int addr, ReadOnlySpan<byte> data)
    {
        if (addr < 0 || addr > 0xFF)
            throw new ArgumentOutOfRangeException(nameof(addr), addr, "addr must be 0..255");
        if (data.Length < 1 || data.Length > WriteMax)
            throw new ArgumentOutOfRangeException(nameof(data), data.Length, $"data must be 1..{WriteMax} bytes");
        var buf = new byte[1 + data.Length];
        buf[0] = (byte)addr;
        data.CopyTo(buf.AsSpan(1));
        return buf;
    }

    /// <summary>
    /// TMF_START_STREAM payload: int_mask u8 (non-zero subset of
    /// <see cref="IntMaskAll"/>), flags u8 (only <see cref="SfRefFrame"/>).
    /// Implies STOP first and restarts <c>frame_seq</c> at 0.
    /// </summary>
    public static byte[] PackStartStream(int intMask, int flags = 0)
    {
        if (intMask < 1 || intMask > IntMaskAll)
            throw new ArgumentOutOfRangeException(nameof(intMask), intMask, "int_mask must be a non-zero subset of 0x0F");
        if ((flags & ~SfRefFrame) != 0)
            throw new ArgumentOutOfRangeException(nameof(flags), flags, "flags: only SF_REF_FRAME (0x01) is defined");
        return new[] { (byte)intMask, (byte)flags };
    }
}

/// <summary>RPT_TMF_REG_DATA: register bytes and the MCU time the SPI read completed.</summary>
public sealed record Tmf8829RegData(ulong TimestampUs, byte[] Data)
{
    /// <summary>Parse a RPT_TMF_REG_DATA payload: cmd u8 (= 0x32), timestamp u64 LE, then data.</summary>
    /// <exception cref="ArgumentException">Payload shorter than 9 bytes, or the echoed cmd is not READ_REG.</exception>
    public static Tmf8829RegData Unpack(ReadOnlySpan<byte> p)
    {
        if (p.Length < Tmf8829Wire.RegDataHeaderSize)
            throw new ArgumentException(
                $"RPT_TMF_REG_DATA: need >={Tmf8829Wire.RegDataHeaderSize} bytes, got {p.Length}", nameof(p));
        if (p[0] != (byte)Tmf8829Cmd.ReadReg)
            throw new ArgumentException($"RPT_TMF_REG_DATA: echoed cmd 0x{p[0]:X2}, expected 0x32", nameof(p));
        return new Tmf8829RegData(
            BinaryPrimitives.ReadUInt64LittleEndian(p[1..]),
            p[Tmf8829Wire.RegDataHeaderSize..].ToArray());
    }
}

/// <summary>
/// RPT_TMF_INFO (25 bytes): bridge state only, the sensor is never touched.
/// Carries no echoed command byte. <see cref="IntEdges"/> runs from MCU
/// power-up; the frame counters restart at TMF_START_STREAM.
/// <see cref="IntLevel"/>: PA4 level now, 0 = asserted.
/// </summary>
public sealed record Tmf8829Info(
    uint IntEdges,
    uint Frames,
    uint FramesAborted,
    uint FrameErrors,
    uint SpiTimeouts,
    byte IntLevel,
    bool Streaming,
    byte IntMask,
    byte LastIntStatus,
    byte LastFifoStatus)
{
    /// <summary>Parse a RPT_TMF_INFO payload (<c>&lt;IIIIIBBBBB</c>, 25 bytes LE).</summary>
    /// <exception cref="ArgumentException">If the payload is shorter than 25 bytes.</exception>
    public static Tmf8829Info Unpack(ReadOnlySpan<byte> p)
    {
        if (p.Length < Tmf8829Wire.InfoSize)
            throw new ArgumentException(
                $"RPT_TMF_INFO: expected {Tmf8829Wire.InfoSize} bytes, got {p.Length}", nameof(p));
        return new Tmf8829Info(
            BinaryPrimitives.ReadUInt32LittleEndian(p),
            BinaryPrimitives.ReadUInt32LittleEndian(p[4..]),
            BinaryPrimitives.ReadUInt32LittleEndian(p[8..]),
            BinaryPrimitives.ReadUInt32LittleEndian(p[12..]),
            BinaryPrimitives.ReadUInt32LittleEndian(p[16..]),
            p[20], p[21] != 0, p[22], p[23], p[24]);
    }
}

/// <summary>
/// One RPT_TMF_FRAME chunk. <see cref="TimestampUs"/> + the frame's
/// pre-header SYSTICK are a clock pair (not the capture time);
/// <see cref="EdgeUs"/> is the INT falling edge credited to the frame, 0 = none.
/// </summary>
public sealed record Tmf8829FrameChunk(
    ulong TimestampUs, ushort FullSize, ushort Offset, byte FrameSeq, ulong EdgeUs, byte[] Data)
{
    /// <summary>Parse: timestamp u64, full_size u16, offset u16, frame_seq u8, edge_us u64, then ≤ 1024 frame bytes.</summary>
    /// <exception cref="ArgumentException">If the payload is shorter than the 21-byte header.</exception>
    public static Tmf8829FrameChunk Unpack(ReadOnlySpan<byte> p)
    {
        if (p.Length < Tmf8829Wire.FrameChunkHeaderSize)
            throw new ArgumentException(
                $"RPT_TMF_FRAME: need >={Tmf8829Wire.FrameChunkHeaderSize} bytes, got {p.Length}", nameof(p));
        return new Tmf8829FrameChunk(
            BinaryPrimitives.ReadUInt64LittleEndian(p),
            BinaryPrimitives.ReadUInt16LittleEndian(p[8..]),
            BinaryPrimitives.ReadUInt16LittleEndian(p[10..]),
            p[12],
            BinaryPrimitives.ReadUInt64LittleEndian(p[13..]),
            p[Tmf8829Wire.FrameChunkHeaderSize..].ToArray());
    }
}

/// <summary>
/// RPT_TMF_EVENT (12 bytes). <see cref="IntStatus"/> is INT_STATUS as read
/// (unmasked); <see cref="Code"/> is a <see cref="Tmf8829EventCode"/>;
/// <see cref="FrameId"/> / <see cref="FifoStatus"/> are the refused header's
/// id byte and pre-header byte (BAD_HEADER), else 0.
/// </summary>
public sealed record Tmf8829Event(ulong TimestampUs, byte IntStatus, byte Code, byte FrameId, byte FifoStatus)
{
    /// <exception cref="ArgumentException">If the payload is shorter than 12 bytes.</exception>
    public static Tmf8829Event Unpack(ReadOnlySpan<byte> p)
    {
        if (p.Length < Tmf8829Wire.EventSize)
            throw new ArgumentException(
                $"RPT_TMF_EVENT: expected {Tmf8829Wire.EventSize} bytes, got {p.Length}", nameof(p));
        return new Tmf8829Event(BinaryPrimitives.ReadUInt64LittleEndian(p), p[8], p[9], p[10], p[11]);
    }
}

/// <summary>A reassembled frame: the sensor bytes from register 0xFA on.</summary>
public sealed record Tmf8829RawFrame(ulong TimestampUs, ulong EdgeUs, byte FrameSeq, byte[] Data);

/// <summary>
/// Rebuilds frames from RPT_TMF_FRAME chunks (contract 14 §2): reset on
/// offset 0; append only a chunk of the same frame_seq (and full_size) at the
/// expected offset; complete at full_size. Never stitches chunks of different
/// frame_seq. <see cref="SeqGaps"/> counts frames the bridge dropped
/// (frame_seq jumps, u8 wrap).
/// </summary>
public sealed class Tmf8829FrameReassembler
{
    private readonly List<byte> _buf = new();
    private Tmf8829FrameChunk? _head;
    private int? _nextSeq;

    /// <summary>Frames returned complete.</summary>
    public int Completed { get; private set; }

    /// <summary>Partial frames thrown away (restarted, out of order, overrun).</summary>
    public int Discarded { get; private set; }

    /// <summary>Frames the bridge dropped, from frame_seq jumps.</summary>
    public int SeqGaps { get; private set; }

    /// <summary>Forget everything — call at every stream start (frame_seq restarts at 0).</summary>
    public void Reset()
    {
        _buf.Clear();
        _head = null;
        _nextSeq = null;
        Completed = 0;
        Discarded = 0;
        SeqGaps = 0;
    }

    /// <summary>Feed one chunk → the frame it completes, or null.</summary>
    public Tmf8829RawFrame? Feed(Tmf8829FrameChunk chunk)
    {
        if (chunk.Offset == 0)
        {
            if (_head is not null)
                Discarded++; // the previous frame never completed
            if (_nextSeq is int next && chunk.FrameSeq != next)
                SeqGaps += (chunk.FrameSeq - next) & 0xFF;
            _nextSeq = (chunk.FrameSeq + 1) & 0xFF;
            _head = chunk;
            _buf.Clear();
            _buf.AddRange(chunk.Data);
        }
        else if (_head is not null
                 && chunk.FrameSeq == _head.FrameSeq
                 && chunk.FullSize == _head.FullSize
                 && chunk.Offset == _buf.Count)
        {
            _buf.AddRange(chunk.Data);
        }
        else
        {
            if (_head is not null)
                Discarded++;
            _head = null;
            _buf.Clear();
            return null;
        }

        var head = _head;
        if (_buf.Count == head.FullSize)
        {
            var frame = new Tmf8829RawFrame(head.TimestampUs, head.EdgeUs, head.FrameSeq, _buf.ToArray());
            _head = null;
            _buf.Clear();
            Completed++;
            return frame;
        }
        if (_buf.Count > head.FullSize)
        {
            Discarded++;
            _head = null;
            _buf.Clear();
        }
        return null;
    }
}
