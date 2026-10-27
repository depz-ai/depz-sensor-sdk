using System.Buffers.Binary;

namespace Depz.Sensor.Bno086;

/// <summary>SHTP channels (contracts/05_SENSOR_BNO086.md §3).</summary>
public enum ShtpChannel
{
    Command = 0,     // SHTP command channel (advertisements)
    Executable = 1,  // device executable: reset/on/sleep; RX 0x01 = reset done
    Control = 2,     // SH-2 control: feature/FRS/command reports
    InputNormal = 3, // non-wake input reports (0xFB timebase + sensors)
    InputWake = 4,   // wake input reports (same cargo format as channel 3)
    GyroRv = 5,      // gyro-integrated rotation vector, dense format
}

/// <summary>
/// One-byte host writes on the executable channel (SHTP channel 1,
/// contracts/05_SENSOR_BNO086.md §6a). The hub answers none of them.
/// <c>Reset</c> restarts the hub (answered later by the 0x01 reset-complete on
/// the same channel); <c>Sleep</c> stops every report except those enabled with
/// <see cref="FeatureFlags.Wakeup"/> | <see cref="FeatureFlags.AlwaysOn"/>,
/// keeping the configuration; <c>On</c> ends <c>Sleep</c> and every report
/// resumes at its rate.
/// </summary>
public enum ExecutableCommand : byte
{
    Reset = 1,
    On = 2,
    Sleep = 3,
}

/// <summary>
/// SHTP frame header: length (bits 14:0 = cargo length incl. this 4-byte
/// header; bit 15 = continuation), channel, per-channel/per-direction seq.
/// </summary>
public sealed record ShtpHeader(int Length, int Channel, int Seq, bool Continuation = false)
{
    public const int Size = 4;
    public const int LengthMask = 0x7FFF;
    public const int ContinuationBit = 0x8000;

    /// <exception cref="ArgumentOutOfRangeException"><c>Length</c> outside u15, <c>Channel</c> or
    /// <c>Seq</c> outside u8 — never masked.</exception>
    public byte[] Pack()
    {
        Protocol.Wire.CheckUint("length", Length, 15);
        Protocol.Wire.CheckUint("channel", Channel, 8);
        Protocol.Wire.CheckUint("seq", Seq, 8);
        int word = (Length & LengthMask) | (Continuation ? ContinuationBit : 0);
        var buf = new byte[Size];
        BinaryPrimitives.WriteUInt16LittleEndian(buf, (ushort)word);
        buf[2] = (byte)Channel;
        buf[3] = (byte)Seq;
        return buf;
    }

    public static ShtpHeader Unpack(ReadOnlySpan<byte> data)
    {
        Protocol.Wire.NeedBytes("SHTP header", data, Size);
        int word = BinaryPrimitives.ReadUInt16LittleEndian(data);
        return new ShtpHeader(word & LengthMask, data[2], data[3], (word & ContinuationBit) != 0);
    }
}

/// <summary>
/// One reassembled cargo: <c>Payload</c> excludes all SHTP headers; <c>Seq</c> and
/// <c>CaptureUs</c> are the FIRST fragment's — <c>CaptureUs</c> is its
/// <c>RPT_DATA.timestamp_us</c>, the capture the cargo's 0xFB base delta refers to
/// (contract 05 §3).
/// </summary>
public sealed record ShtpCargo(int Channel, int Seq, byte[] Payload, long CaptureUs = 0);

/// <summary>
/// SHTP codec: per-channel TX sequence counters and RX cargo reassembly
/// (continuation-bit fragments). Byte-exact with the reference Python
/// <c>ShtpLayer</c>. Not thread-safe.
/// </summary>
public sealed class ShtpLayer
{
    public const int NumChannels = 6;
    public const int MaxTxFrame = 64; // one MCU transmit slot (ERRATA E2)

    private sealed class ChannelRx
    {
        public List<byte> Buf = new();
        public int Expected;
        public int Seq;
        public long CaptureUs;
    }

    private readonly int[] _txSeq = new int[NumChannels];
    private readonly ChannelRx[] _rx;

    // Incomplete / orphan cargos and junk-channel (>= 6) frames thrown away.
    public int Discarded { get; private set; }

    public ShtpLayer()
    {
        _rx = new ChannelRx[NumChannels];
        for (int i = 0; i < NumChannels; i++)
            _rx[i] = new ChannelRx();
    }

    // ── TX ───────────────────────────────────────────────────────────────────

    /// <summary>Single-fragment frame: length = header + payload.</summary>
    public static byte[] BuildFrame(int channel, ReadOnlySpan<byte> payload, int seq)
    {
        byte[] hdr = new ShtpHeader(ShtpHeader.Size + payload.Length, channel, seq & 0xFF).Pack();
        var frame = new byte[hdr.Length + payload.Length];
        hdr.CopyTo(frame, 0);
        payload.CopyTo(frame.AsSpan(hdr.Length));
        return frame;
    }

    /// <summary>
    /// Split a cargo into wire frames of at most <paramref name="maxFrame"/>
    /// bytes. The first fragment advertises the TOTAL cargo length; each
    /// continuation carries the remaining length with the continuation bit set.
    /// seq increments per frame.
    /// </summary>
    public static List<byte[]> FragmentCargo(int channel, byte[] payload, int seqStart, int maxFrame = MaxTxFrame)
    {
        if (maxFrame <= ShtpHeader.Size)
            throw new ArgumentException("max_frame must exceed the 4-byte SHTP header", nameof(maxFrame));
        int room = maxFrame - ShtpHeader.Size;
        var frames = new List<byte[]>();
        int off = 0;
        int seq = seqStart & 0xFF;
        int total = ShtpHeader.Size + payload.Length;
        while (true)
        {
            int chunkLen = Math.Min(room, payload.Length - off);
            int remaining = total - off; // includes one header
            byte[] hdr = new ShtpHeader(remaining, channel, seq, Continuation: off > 0).Pack();
            var frame = new byte[hdr.Length + chunkLen];
            hdr.CopyTo(frame, 0);
            Array.Copy(payload, off, frame, hdr.Length, chunkLen);
            frames.Add(frame);
            off += chunkLen;
            seq = (seq + 1) & 0xFF;
            if (off >= payload.Length)
                return frames;
        }
    }

    /// <summary>Build a single-fragment frame, consuming the channel's TX seq.</summary>
    public byte[] NextFrame(int channel, ReadOnlySpan<byte> payload)
    {
        if (ShtpHeader.Size + payload.Length > MaxTxFrame)
            throw new ArgumentException($"TX cargo {payload.Length}B exceeds the {MaxTxFrame}B MCU slot");
        if (channel < 0 || channel >= NumChannels)
            throw new ArgumentOutOfRangeException(nameof(channel), channel,
                $"SHTP channel {channel} out of range 0..{NumChannels - 1}");
        int seq = _txSeq[channel];
        _txSeq[channel] = (seq + 1) & 0xFF;
        return BuildFrame(channel, payload, seq);
    }

    public int TxSeq(int channel) => _txSeq[channel];

    // ── RX ───────────────────────────────────────────────────────────────────

    /// <summary>
    /// Consume one inbound frame; return the cargo when complete, else null.
    /// <paramref name="captureUs"/> is the frame's <c>RPT_DATA.timestamp_us</c>; the
    /// cargo carries the FIRST fragment's — the base its 0xFB timestamps refer to
    /// (contract 05 §3), not the fragment that completed it. It is the MCU uptime
    /// (a u64 µs): a negative one throws <see cref="ArgumentOutOfRangeException"/>.
    /// </summary>
    public ShtpCargo? Feed(byte[] frame, long captureUs = 0)
    {
        if (captureUs < 0)
            throw new ArgumentOutOfRangeException(nameof(captureUs), captureUs, "captureUs is a u64 (MCU uptime µs)");
        if (frame.Length < ShtpHeader.Size)
            return null;
        ShtpHeader hdr = ShtpHeader.Unpack(frame);
        if (hdr.Length < ShtpHeader.Size)
            return null; // empty/padding header ("no data" read)
        if (hdr.Channel >= NumChannels)
        {
            Discarded++; // junk channel
            return null;
        }
        ReadOnlySpan<byte> chunk = frame.AsSpan(ShtpHeader.Size);
        ChannelRx rx = _rx[hdr.Channel];
        if (!hdr.Continuation)
        {
            if (rx.Expected != 0) // a partial, even one with no bytes yet
                Discarded++;
            rx.Buf = new List<byte>(chunk.ToArray());
            rx.Expected = hdr.Length - ShtpHeader.Size;
            rx.Seq = hdr.Seq;
            rx.CaptureUs = captureUs;
        }
        else
        {
            if (rx.Expected == 0)
            {
                Discarded++;
                return null;
            }
            rx.Buf.AddRange(chunk.ToArray());
        }

        if (rx.Buf.Count < rx.Expected)
            return null;
        if (rx.Buf.Count > rx.Expected) // overrun — junk framing
        {
            Discarded++;
            rx.Buf = new List<byte>();
            rx.Expected = 0;
            return null;
        }
        var cargo = new ShtpCargo(hdr.Channel, rx.Seq, rx.Buf.ToArray(), rx.CaptureUs);
        rx.Buf = new List<byte>();
        rx.Expected = 0;
        return cargo;
    }

    /// <summary>Forget all TX seq counters and partial cargos (sensor reset).</summary>
    public void Reset()
    {
        Array.Clear(_txSeq, 0, _txSeq.Length);
        for (int i = 0; i < NumChannels; i++)
            _rx[i] = new ChannelRx();
    }
}
