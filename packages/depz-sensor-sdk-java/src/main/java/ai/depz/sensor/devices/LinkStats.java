package ai.depz.sensor.devices;

/** Link counters (contract 07 §2 {@code stats}); updated by the reader thread. */
public final class LinkStats {
    volatile long txPackets;
    volatile long rxPackets;
    volatile long txBytes;
    volatile long rxBytes;
    volatile long crcErrors;
    volatile long headerErrors;
    volatile long trashBytes;
    volatile long seqGaps;
    volatile long deviceSeqErrors;

    public long txPackets() {
        return txPackets;
    }

    public long rxPackets() {
        return rxPackets;
    }

    public long txBytes() {
        return txBytes;
    }

    public long rxBytes() {
        return rxBytes;
    }

    public long crcErrors() {
        return crcErrors;
    }

    public long headerErrors() {
        return headerErrors;
    }

    public long trashBytes() {
        return trashBytes;
    }

    /** Gaps in the device → host sequence numbers, seen by the host. */
    public long seqGaps() {
        return seqGaps;
    }

    /** RPT_SEQUENCE_ERROR reports: gaps the device saw in host → device packets. */
    public long deviceSeqErrors() {
        return deviceSeqErrors;
    }
}
