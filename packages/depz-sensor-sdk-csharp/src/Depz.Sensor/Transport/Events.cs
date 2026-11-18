namespace Depz.Sensor.Transport;

/// <summary>Base type for events produced by <see cref="PacketParser"/>.</summary>
public abstract record ParserEvent;

/// <summary>A fully decoded frame with a valid header (and payload CRC, if any).</summary>
public sealed record Packet(int Cmd, int Seq, byte[] Payload) : ParserEvent;

/// <summary>
/// Bytes discarded while hunting for a valid frame. Boundaries between
/// consecutive <see cref="Trash"/> events depend on read chunking; only the
/// concatenated byte stream is deterministic.
/// </summary>
public sealed record Trash(byte[] Data) : ParserEvent;

/// <summary>A frame with a valid header whose payload CRC failed; dropped.</summary>
public sealed record CrcError(int Cmd, int Seq) : ParserEvent;
