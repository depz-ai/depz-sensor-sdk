package ai.depz.sensor.transport;

/** A frame with a valid header whose payload CRC failed; dropped. */
public record CrcError(int cmd, int seq) implements Event {}
