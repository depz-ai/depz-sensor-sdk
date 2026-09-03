namespace Depz.Sensor.Transport;

/// <summary>
/// Incremental frame parser. Feed arbitrary byte chunks; get events.
///
/// Event order is invariant to chunking (contract 01 §5) except <see cref="Trash"/>
/// event boundaries — concatenate Trash data when comparing streams. Byte-exact
/// with the reference Python <c>PacketParser</c>, including the
/// empty-payload-no-CRC rule (ERRATA E6) and single-byte resync on a corrupt
/// header.
/// </summary>
public sealed class PacketParser
{
    private readonly List<byte> _buf = new();

    public int Packets { get; private set; }
    public int CrcErrors { get; private set; }
    public int HeaderErrors { get; private set; }
    public int TrashBytes { get; private set; }

    /// <summary>Current unconsumed bytes (residue).</summary>
    public byte[] Residue() => _buf.ToArray();

    public IReadOnlyList<ParserEvent> Feed(ReadOnlySpan<byte> data)
    {
        foreach (byte b in data)
            _buf.Add(b);
        var outEvents = new List<ParserEvent>();
        while (true)
        {
            ParserEvent? ev = ParseOne();
            if (ev is null)
                break;
            outEvents.Add(ev);
        }
        return outEvents;
    }

    private Trash EmitTrash(int count)
    {
        var data = new byte[count];
        _buf.CopyTo(0, data, 0, count);
        _buf.RemoveRange(0, count);
        TrashBytes += count;
        return new Trash(data);
    }

    private int FindMagic()
    {
        for (int i = 0; i + 1 < _buf.Count; i++)
        {
            if (_buf[i] == Framing.Magic[0] && _buf[i + 1] == Framing.Magic[1])
                return i;
        }
        return -1;
    }

    private ParserEvent? ParseOne()
    {
        int pos = FindMagic();
        if (pos == -1)
        {
            // Keep the last byte: it may be a split 0xA5.
            if (_buf.Count > 1)
                return EmitTrash(_buf.Count - 1);
            return null;
        }
        if (pos > 0)
            return EmitTrash(pos);
        if (_buf.Count < Framing.HeaderSize)
            return null;

        int dataSize = _buf[2] | (_buf[3] << 8);
        int payloadSize = dataSize & Framing.MaxPayload;
        var crcType = (CrcType)((dataSize >> 14) & 0x03);

        byte[] hdrInput = { _buf[2], _buf[3], _buf[4], _buf[5] };
        if (Crc.Crc8Maxim(hdrInput) != _buf[6])
        {
            // Header corrupt: advance one byte past the magic start and let the
            // magic hunt resync (firmware: rb_skip(off + 1)).
            HeaderErrors++;
            return EmitTrash(1);
        }

        // ERRATA E6: empty payload never carries CRC bytes.
        int crcSize = payloadSize > 0 ? crcType.Size() : 0;
        int total = Framing.HeaderSize + payloadSize + crcSize;
        if (_buf.Count < total)
            return null;

        int cmd = _buf[4];
        int seq = _buf[5];
        var payload = new byte[payloadSize];
        _buf.CopyTo(Framing.HeaderSize, payload, 0, payloadSize);
        var trailer = new byte[crcSize];
        _buf.CopyTo(Framing.HeaderSize + payloadSize, trailer, 0, crcSize);
        _buf.RemoveRange(0, total);

        if (crcSize > 0 && !Framing.PayloadCrcBytes(crcType, payload).AsSpan().SequenceEqual(trailer))
        {
            CrcErrors++;
            return new CrcError(cmd, seq);
        }
        Packets++;
        return new Packet(cmd, seq, payload);
    }
}
