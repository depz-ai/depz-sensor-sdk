package ai.depz.sensor.devices;

/**
 * The unsolicited / diagnostic events a device emits (contract 07 §2). The common ones are the
 * nested records; a sensor class adds its own.
 */
public interface DeviceEvent {
    /**
     * A gap in packet sequence numbers; {@code reportedByDevice}: the board saw a gap in host TX
     * (RPT_SEQUENCE_ERROR), else the host saw one in the board's packets.
     */
    record SequenceError(int expectedSeq, int receivedSeq, boolean reportedByDevice) implements DeviceEvent {}

    /** A packet whose payload CRC did not check. */
    record LinkCrcError(int cmd, int seq) implements DeviceEvent {}

    /** Bytes the parser skipped while looking for a packet header. */
    record Trash(byte[] data) implements DeviceEvent {}

    /** An RPT_STATUS no request was waiting for (echoed cmd 0x00, or nobody in flight). */
    record UnsolicitedStatus(int status) implements DeviceEvent {
        public boolean isHardwareFault() {
            return status == ai.depz.sensor.protocol.Common.Status.ERR_HARDWARE_FAULT.value;
        }
    }

    /** RPT_TEXT nobody asked for, or a report no class routes (its payload as hex). */
    record Text(int cmd, String text) implements DeviceEvent {}

    /** An unsolicited MCU temperature report. */
    record Temperature(long timestampUs, double celsius) implements DeviceEvent {}

    /** The connection dropped (unplug, I/O error); the device is unusable from here on. */
    record Disconnected(String reason) implements DeviceEvent {}
}
