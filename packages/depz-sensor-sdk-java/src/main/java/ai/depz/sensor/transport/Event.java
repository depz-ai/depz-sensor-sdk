package ai.depz.sensor.transport;

/** Base type for {@link PacketParser} events: {@link Packet}, {@link Trash}, {@link CrcError}. */
public sealed interface Event permits Packet, Trash, CrcError {}
