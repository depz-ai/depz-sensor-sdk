package ai.depz.sensor.sensors.vl53lx.bare;

/**
 * One field of a register block: byte offset, width in bytes, signedness and the mask the C codec
 * applies (null: none).
 */
public record RegField(String name, int off, int width, boolean signed, Long mask) {}
