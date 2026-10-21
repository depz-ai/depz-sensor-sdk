package ai.depz.sensor.sensors.tmf8829;

import static ai.depz.sensor.sensors.tmf8829.SensorClock.EDGE_LATE_US;
import static ai.depz.sensor.sensors.tmf8829.SensorClock.FP_SUBINTS;
import static ai.depz.sensor.sensors.tmf8829.SensorClock.FT_EDGE_LATE;
import static ai.depz.sensor.sensors.tmf8829.SensorClock.FT_NOMINAL;
import static ai.depz.sensor.sensors.tmf8829.SensorClock.FT_STALLED;
import static ai.depz.sensor.sensors.tmf8829.SensorClock.FT_T1_UNSURE;
import static ai.depz.sensor.sensors.tmf8829.SensorClock.U32;

/**
 * When a results frame was captured (contracts/14_SENSOR_TMF8829.md §5). A
 * results frame is N equal sub-integrations back to back; the footer has the
 * SYSTICK of the start of the first (t0) and of the last (t1). The window is
 * {@code [t0, t1 + T1]}, T1 the stream's shortest {@code (t1 − t0)/(N − 1)};
 * its centre is the frame time. Call {@link #reset()} at every stream start.
 */
public final class CaptureTimer {
    /**
     * Capture time of one results frame, MCU µs: {@code timestampUs} the centre
     * of the capture window, {@code flags} FT_*, {@code t1LenUs} the
     * sub-integration length used, {@code t0Us}/{@code t1Us} the starts of the
     * first/last sub-integration, {@code edgeUs} the INT edge credited (0 =
     * none), {@code pairUs} the MCU time of the header read, {@code ratioQ16}
     * the clock ratio after this frame.
     */
    public record FrameTime(
            long timestampUs, int flags, long t1LenUs, long t0Us, long t1Us,
            long edgeUs, long pairUs, long ratioQ16) {}

    public final SensorClock clock;
    private boolean haveLagMin;
    private long lagMin;
    private boolean histSeen;
    private long subMin;

    public CaptureTimer(SensorClock clock) {
        this.clock = clock;
        reset();
    }

    public void reset() {
        haveLagMin = false;
        lagMin = 0;
        histSeen = false;
        subMin = 0;
    }

    /** A histogram frame was seen in the stream. */
    public void histograms() {
        histSeen = true;
    }

    public FrameTime results(int fpMode, long systick, long t0Tick, long t1Tick,
                             long pairUs, long edgeUs, int flags) {
        SensorClock c = clock;
        if (c.nominal) {
            flags |= FT_NOMINAL;
        }
        long t0 = c.toUs(pairUs, systick, t0Tick);
        long t1 = c.toUs(pairUs, systick, t1Tick);

        if (edgeUs != 0 && !histSeen) {
            long lag = edgeUs - t1;
            if (lag <= 0 || (haveLagMin && lag > lagMin + EDGE_LATE_US)) {
                flags |= FT_EDGE_LATE;
            } else if (!haveLagMin || lag < lagMin) {
                haveLagMin = true;
                lagMin = lag;
            }
        }

        int n1 = (fpMode >= 0 && fpMode < FP_SUBINTS.length ? FP_SUBINTS[fpMode] : 2) - 1;
        long sub = ((t1Tick - t0Tick) & U32) / n1;
        if (subMin == 0 || sub < subMin) {
            subMin = sub;
        }
        if (sub > subMin + 1) {
            flags |= FT_STALLED;
        }
        if (histSeen && n1 > 1) {
            flags |= FT_T1_UNSURE;
        }

        long t1Len = ((subMin * c.ratioQ16) >> 16) & U32;
        return new FrameTime(
            Math.floorDiv(t0 + t1 + t1Len, 2L), flags, t1Len, t0, t1, edgeUs, pairUs, c.ratioQ16);
    }

    public FrameTime resultsFrame(Frames.FrameInfo f, long pairUs, long edgeUs, int flags) {
        return results(f.fpMode(), f.systick(), f.t0Integration(), f.t1Integration(), pairUs, edgeUs, flags);
    }
}
