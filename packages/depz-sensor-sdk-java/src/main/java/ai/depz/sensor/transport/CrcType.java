package ai.depz.sensor.transport;

/** Payload CRC type carried in the two top bits of the frame data-size field. */
public enum CrcType {
    NONE(0, 0),
    CRC8(1, 1),
    CRC16(2, 2),
    CRC32(3, 4);

    public final int value;
    public final int size;

    CrcType(int value, int size) {
        this.value = value;
        this.size = size;
    }

    public static CrcType fromValue(int value) {
        for (CrcType t : values()) {
            if (t.value == value) {
                return t;
            }
        }
        throw new IllegalArgumentException("invalid crc_type: " + value);
    }
}
