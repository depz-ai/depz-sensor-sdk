using System.Buffers.Binary;
using System.Text;
using Depz.Sensor.Transport;

namespace Depz.Sensor.Protocol;

/// <summary><c>.fwdepz</c> bootloader container parse/validate (contracts/06 §2).</summary>
public static class FwDepz
{
    public static readonly byte[] FwDepzMagic = Encoding.ASCII.GetBytes("FWDEPZ00");
    public const int FwDepzHeaderSize = 64;

    /// <summary>Parse/validate failure. <see cref="Code"/> matches the vector <c>error</c> strings.</summary>
    public sealed class FwDepzException : Exception
    {
        public string Code { get; }
        public FwDepzException(string code, string message) : base(message) => Code = code;
    }

    /// <summary>Parsed and validated <c>.fwdepz</c> firmware container.</summary>
    public sealed record FwDepzImage(uint LoadAddr, uint FwSize, uint FwCrc32, int CurSec, int TotSec, byte[] Payload)
    {
        public bool PayloadCrcOk => Crc.Crc32IsoHdlc(Payload) == FwCrc32;

        /// <summary>
        /// Parse and validate. Validation order (contract 06 §2): length, magic,
        /// then CRC-16/CCITT-FALSE over bytes [0..61] vs the u16 LE at offset 62,
        /// then fw_size == payload length.
        /// </summary>
        public static FwDepzImage Parse(byte[] blob)
        {
            if (blob.Length < FwDepzHeaderSize)
                throw new FwDepzException("too_short", $"file too short: {blob.Length} < {FwDepzHeaderSize}");

            if (!blob.AsSpan(0, 8).SequenceEqual(FwDepzMagic))
                throw new FwDepzException("magic", "bad magic (not a .fwdepz file)");

            int hdrCrc = blob[62] | (blob[63] << 8);
            ushort actual = Crc.Crc16CcittFalse(blob.AsSpan(0, 62));
            if (hdrCrc != actual)
                throw new FwDepzException("header_crc",
                    $"header CRC mismatch: stored=0x{hdrCrc:X4} actual=0x{actual:X4}");

            uint loadAddr = BinaryPrimitives.ReadUInt32LittleEndian(blob.AsSpan(8));
            uint fwSize = BinaryPrimitives.ReadUInt32LittleEndian(blob.AsSpan(12));
            uint fwCrc32 = BinaryPrimitives.ReadUInt32LittleEndian(blob.AsSpan(16));
            int curSec = blob[20];
            int totSec = blob[21];
            byte[] payload = blob[FwDepzHeaderSize..];

            if (fwSize != payload.Length)
                throw new FwDepzException("size", $"fw_size={fwSize} but payload is {payload.Length} bytes");

            return new FwDepzImage(loadAddr, fwSize, fwCrc32, curSec, totSec, payload);
        }
    }
}
