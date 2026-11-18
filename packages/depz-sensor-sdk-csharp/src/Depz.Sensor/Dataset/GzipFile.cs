using System.Buffers.Binary;
using System.IO.Compression;
using Depz.Sensor.Transport;

namespace Depz.Sensor.Dataset;

// GZipStream ends quietly at a cut: what came so far would pass for the whole file. The trailer (RFC 1952: CRC-32 and
// length of the content) is what says a member is whole. A file of several members one after the other is legal, as in
// the reference; the last trailer speaks only for the last one, so each is checked on its own. GZipStream does not say
// where a member ended, and a `1F 8B` is no mark (compressed bytes hold it at random, trying each would be quadratic):
// the end is read from the deflate data itself, in one pass, without producing its content.
internal static class GzipFile
{
    private const int MinMember = 18; // a 10-byte header and the 8-byte trailer

    /// <summary>The content of the file, or null when it is cut short or its content does not match a trailer.</summary>
    public static byte[]? Inflate(byte[] gzip)
    {
        if (Member(gzip, 0, gzip.Length) is byte[] whole)
            return whole; // the usual file
        using var all = new MemoryStream();
        for (int start = 0; start < gzip.Length;)
        {
            int end = MemberEnd(gzip, start);
            if (end < 0 || Member(gzip, start, end - start) is not byte[] member)
                return null;
            all.Write(member, 0, member.Length);
            start = end;
            while (start < gzip.Length && gzip[start] == 0)
                start++; // zeros after a member are padding, as in the reference
        }
        return all.ToArray();
    }

    // The content of the bytes as one whole gzip stream, or null when they are not one.
    private static byte[]? Member(byte[] gzip, int offset, int count)
    {
        if (count < MinMember)
            return null;
        byte[] content;
        try
        {
            using var ms = new MemoryStream(gzip, offset, count);
            using var gz = new GZipStream(ms, CompressionMode.Decompress);
            using var inflated = new MemoryStream();
            gz.CopyTo(inflated);
            content = inflated.ToArray();
        }
        catch (InvalidDataException)
        {
            return null;
        }
        var trailer = gzip.AsSpan(offset + count - 8, 8);
        return BinaryPrimitives.ReadUInt32LittleEndian(trailer) == Crc.Crc32IsoHdlc(content)
               && BinaryPrimitives.ReadUInt32LittleEndian(trailer.Slice(4)) == (uint)content.Length
            ? content
            : null;
    }

    // Where the member starting at `start` ends, its trailer included, or -1 when the bytes hold no whole one.
    private static int MemberEnd(byte[] b, int start)
    {
        if (b.Length - start < MinMember || b[start] != 0x1F || b[start + 1] != 0x8B || b[start + 2] != 8)
            return -1;
        byte flags = b[start + 3];
        int pos = start + 10;
        if ((flags & 0x04) != 0) // FEXTRA
            pos += 2 + (b[pos] | (b[pos + 1] << 8));
        if ((flags & 0x08) != 0) // FNAME
            pos = AfterZero(b, pos);
        if ((flags & 0x10) != 0) // FCOMMENT
            pos = AfterZero(b, pos);
        if (pos < 0)
            return -1;
        if ((flags & 0x02) != 0) // FHCRC
            pos += 2;
        if (pos >= b.Length)
            return -1;
        int end = new DeflateWalk(b, pos).End();
        return end < 0 || b.Length - end < 8 ? -1 : end + 8;
    }

    private static int AfterZero(byte[] b, int pos)
    {
        if (pos < 0 || pos >= b.Length)
            return -1;
        int zero = Array.IndexOf(b, (byte)0, pos);
        return zero < 0 ? -1 : zero + 1;
    }

    // Reads the blocks of a deflate stream (RFC 1951) and only skips over what they hold. It need not refuse all that a
    // decoder refuses: a member it finds the end of still goes through GZipStream.
    private sealed class DeflateWalk
    {
        private static readonly byte[] LengthExtra =
            { 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0 };

        private static readonly byte[] DistanceExtra =
            { 0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13 };

        private static readonly byte[] CodeLengthOrder =
            { 16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15 };

        private static readonly Huffman FixedLiterals = FixedLiteralCode();
        private static readonly Huffman FixedDistances = new(Enumerable.Repeat((byte)5, 30).ToArray(), 0, 30);

        private readonly byte[] _data;
        private int _pos;
        private int _bits;
        private int _bitCount;

        public DeflateWalk(byte[] data, int pos)
        {
            _data = data;
            _pos = pos;
        }

        // The index just past the stream, or -1 when it is cut short or makes no sense.
        public int End()
        {
            try
            {
                bool last;
                do
                {
                    last = Bits(1) == 1;
                    switch (Bits(2))
                    {
                        case 0:
                            SkipStored();
                            break;
                        case 1:
                            SkipCodes(FixedLiterals, FixedDistances);
                            break;
                        case 2:
                            SkipDynamic();
                            break;
                        default:
                            return -1;
                    }
                } while (!last);
                return _pos;
            }
            catch (InvalidDataException)
            {
                return -1;
            }
        }

        private int Bits(int n)
        {
            int v = _bits;
            while (_bitCount < n)
            {
                if (_pos >= _data.Length)
                    throw new InvalidDataException("cut short");
                v |= _data[_pos++] << _bitCount;
                _bitCount += 8;
            }
            _bits = v >> n;
            _bitCount -= n;
            return v & ((1 << n) - 1);
        }

        private void SkipStored()
        {
            _bits = 0;
            _bitCount = 0; // the rest of the byte
            if (_data.Length - _pos < 4)
                throw new InvalidDataException("cut short");
            int length = _data[_pos] | (_data[_pos + 1] << 8);
            int inverse = _data[_pos + 2] | (_data[_pos + 3] << 8);
            _pos += 4;
            if ((length ^ inverse) != 0xFFFF || _data.Length - _pos < length)
                throw new InvalidDataException("bad stored block");
            _pos += length;
        }

        private void SkipDynamic()
        {
            int literals = Bits(5) + 257;
            int distances = Bits(5) + 1;
            int codeLengths = Bits(4) + 4;
            if (literals > 286 || distances > 30)
                throw new InvalidDataException("too many codes");
            byte[] lengths = new byte[literals + distances + 19];
            for (int i = 0; i < codeLengths; i++)
                lengths[CodeLengthOrder[i]] = (byte)Bits(3);
            var code = new Huffman(lengths, 0, 19);
            Array.Clear(lengths, 0, 19);
            for (int i = 0; i < literals + distances;)
            {
                int symbol = code.Decode(this);
                if (symbol < 16)
                {
                    lengths[i++] = (byte)symbol;
                    continue;
                }
                byte value = 0;
                if (symbol == 16)
                {
                    if (i == 0)
                        throw new InvalidDataException("nothing to repeat");
                    value = lengths[i - 1];
                }
                int repeat = symbol == 16 ? 3 + Bits(2) : symbol == 17 ? 3 + Bits(3) : 11 + Bits(7);
                if (i + repeat > literals + distances)
                    throw new InvalidDataException("too many lengths");
                while (repeat-- > 0)
                    lengths[i++] = value;
            }
            SkipCodes(new Huffman(lengths, 0, literals), new Huffman(lengths, literals, distances));
        }

        private void SkipCodes(Huffman literals, Huffman distances)
        {
            while (true)
            {
                int symbol = literals.Decode(this);
                if (symbol < 256)
                    continue;
                if (symbol == 256)
                    return;
                if (symbol - 257 >= LengthExtra.Length)
                    throw new InvalidDataException("bad length symbol");
                Bits(LengthExtra[symbol - 257]);
                int distance = distances.Decode(this);
                if (distance >= DistanceExtra.Length)
                    throw new InvalidDataException("bad distance symbol");
                Bits(DistanceExtra[distance]);
            }
        }

        private static Huffman FixedLiteralCode()
        {
            byte[] lengths = new byte[288];
            for (int i = 0; i < lengths.Length; i++)
                lengths[i] = (byte)(i < 144 ? 8 : i < 256 ? 9 : i < 280 ? 7 : 8);
            return new Huffman(lengths, 0, lengths.Length);
        }

        // A canonical code (RFC 1951 §3.2.2), decoded a bit at a time: how many codes each length has, and their symbols.
        private sealed class Huffman
        {
            private const int MaxBits = 15;
            private readonly int[] _count = new int[MaxBits + 1];
            private readonly int[] _symbols;

            public Huffman(byte[] lengths, int offset, int n)
            {
                for (int i = 0; i < n; i++)
                    _count[lengths[offset + i]]++;
                int left = 1;
                for (int len = 1; len <= MaxBits; len++)
                {
                    left = 2 * left - _count[len];
                    if (left < 0)
                        throw new InvalidDataException("over-subscribed code");
                }
                int[] next = new int[MaxBits + 2];
                for (int len = 1; len <= MaxBits; len++)
                    next[len + 1] = next[len] + _count[len];
                _symbols = new int[n];
                for (int i = 0; i < n; i++)
                {
                    if (lengths[offset + i] != 0)
                        _symbols[next[lengths[offset + i]]++] = i;
                }
            }

            public int Decode(DeflateWalk walk)
            {
                int code = 0, first = 0, index = 0;
                for (int len = 1; len <= MaxBits; len++)
                {
                    code |= walk.Bits(1);
                    int count = _count[len];
                    if (code - count < first)
                        return _symbols[index + (code - first)];
                    index += count;
                    first = (first + count) << 1;
                    code <<= 1;
                }
                throw new InvalidDataException("no such code");
            }
        }
    }
}
