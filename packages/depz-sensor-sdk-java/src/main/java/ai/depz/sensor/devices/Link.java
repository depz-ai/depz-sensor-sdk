package ai.depz.sensor.devices;

import java.time.Duration;

/**
 * A blocking byte pipe to one device: a serial port, a {@code .depzrec} record/replay wrapper,
 * or anything a host supplies. Device classes accept any link, which keeps every layer above the
 * port testable without hardware.
 */
public interface Link {
    /**
     * The next available chunk (one byte or more), or an empty array when nothing arrived within
     * {@code timeout}.
     *
     * @throws DeviceLostException the link is closed or the device is gone
     */
    byte[] read(Duration timeout);

    /**
     * Writes {@code data} to the board.
     *
     * @throws DeviceLostException the link is closed or the device is gone
     */
    void write(byte[] data);

    void close();

    boolean isClosed();

    /** A name for messages: the port path, or what the link wraps. */
    String name();
}
