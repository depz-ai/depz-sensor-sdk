package ai.depz.sensor.devices;

import java.util.concurrent.atomic.AtomicLong;

/** Link counters (contract 07 §2 {@code stats}); updated by the reader thread and the sender. */
public final class LinkStats {
    final AtomicLong txPackets = new AtomicLong();
    final AtomicLong rxPackets = new AtomicLong();
    final AtomicLong txBytes = new AtomicLong();
    final AtomicLong rxBytes = new AtomicLong();
    final AtomicLong crcErrors = new AtomicLong();
    final AtomicLong headerErrors = new AtomicLong();
    final AtomicLong trashBytes = new AtomicLong();
    final AtomicLong seqGaps = new AtomicLong();
    final AtomicLong deviceSeqErrors = new AtomicLong();
    final AtomicLong callbackErrors = new AtomicLong();
    final AtomicLong reportErrors = new AtomicLong();

    public long txPackets() {
        return txPackets.get();
    }

    public long rxPackets() {
        return rxPackets.get();
    }

    public long txBytes() {
        return txBytes.get();
    }

    public long rxBytes() {
        return rxBytes.get();
    }

    public long crcErrors() {
        return crcErrors.get();
    }

    public long headerErrors() {
        return headerErrors.get();
    }

    public long trashBytes() {
        return trashBytes.get();
    }

    /** Gaps in the device → host sequence numbers, seen by the host. */
    public long seqGaps() {
        return seqGaps.get();
    }

    /** RPT_SEQUENCE_ERROR reports: gaps the device saw in host → device packets. */
    public long deviceSeqErrors() {
        return deviceSeqErrors.get();
    }

    /** Callbacks that threw on the reader thread (contract 07 §3). */
    public long callbackErrors() {
        return callbackErrors.get();
    }

    /** Reports whose handler threw: the packet was dropped (contract 07 §3). */
    public long reportErrors() {
        return reportErrors.get();
    }
}
