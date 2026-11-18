package ai.depz.sensor.transport;

/** A successfully decoded frame. {@code cmd}/{@code seq} are unsigned bytes 0..255. */
public record Packet(int cmd, int seq, byte[] payload) implements Event {}
