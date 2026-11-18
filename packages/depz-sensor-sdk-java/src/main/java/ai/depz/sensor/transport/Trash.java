package ai.depz.sensor.transport;

/**
 * Bytes discarded while hunting for a valid frame. Boundaries between
 * consecutive Trash events depend on read chunking; only the concatenated
 * byte stream is deterministic (contract 01 §5).
 */
public record Trash(byte[] data) implements Event {}
