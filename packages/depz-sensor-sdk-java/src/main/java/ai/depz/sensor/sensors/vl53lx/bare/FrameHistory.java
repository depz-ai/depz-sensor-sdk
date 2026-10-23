package ai.depz.sensor.sensors.vl53lx.bare;

import java.util.ArrayList;
import java.util.List;

/**
 * The frame before this one, reduced to what the consistency checks read
 * ({@code VL53LX_zone_hist_info_t} and {@code VL53LX_zone_objects_t} for one zone). The engine
 * alternates two VCSEL periods, so one frame cannot say whether a target is inside the unambiguous
 * range or an echo wrapped into it; the API compares consecutive frames. The BareDriver keeps no
 * history, so the driver owns one of these and hands it every frame.
 */
public final class FrameHistory {
    private BareDeviceState rdDeviceState;
    private List<Hist.PrevTarget> targets = new ArrayList<>();
    private long totalPeriodsElapsed;
    private long spads;

    long totalPeriodsElapsed() {
        return totalPeriodsElapsed;
    }

    long spads() {
        return spads;
    }

    /** Forget the stream so far: the next {@link #apply} only remembers. */
    public void reset() {
        rdDeviceState = null;
        totalPeriodsElapsed = 0;
        spads = 0;
        targets = new ArrayList<>();
    }

    /**
     * Check {@code results} against the remembered frame, then remember it — C's order: both checks
     * run before hist_data and range_results are copied into the zone history.
     */
    public void apply(RangeResults results, HistogramBinData bins, BareDeviceState rdDeviceState,
            HistPostProcessConfig hpp) {
        phaseConsistencyCheck(results, hpp);
        this.rdDeviceState = rdDeviceState;
        totalPeriodsElapsed = bins.totalPeriodsElapsed;
        spads = bins.resultDssActualEffectiveSpads;
        List<Hist.PrevTarget> kept = new ArrayList<>();
        for (RangeData t : results.targets) {
            kept.add(new Hist.PrevTarget(t.ambientEvents, t.totalEvents, t.phaseMean, t.rangeStatus));
        }
        targets = kept;
    }

    // VL53LX_hist_phase_consistency_check(): every ranged target starts guilty and has to find itself in the
    // previous frame, within the phase tolerance and with consistent events.
    private void phaseConsistencyCheck(RangeResults results, HistPostProcessConfig hpp) {
        long phaseTolerance = hpp.consistencyPhaseTolerance << 8; // a whole phase unit; phases are 8.8
        // C also admits RANGING_GATHER_DATA, the multizone path.
        if (rdDeviceState != BareDeviceState.RANGING_OUTPUT_DATA) {
            return;
        }
        if (phaseTolerance == 0) {
            return;
        }
        for (RangeData target : results.targets) {
            if (target.rangeStatus != Hist.DEVICE_ERROR_RANGE_COMPLETE
                    && target.rangeStatus != Hist.DEVICE_ERROR_RANGE_COMPLETE_NO_WRAP_CHECK) {
                continue;
            }
            target.rangeStatus = !targets.isEmpty() ? Hist.DEVICE_ERROR_PHASE_CONSISTENCY
                    : Hist.DEVICE_ERROR_PREV_RANGE_NO_TARGETS;
            for (Hist.PrevTarget prev : targets) {
                if (Math.abs(target.phaseMean - prev.phaseMean()) >= phaseTolerance) {
                    continue;
                }
                long status = Hist.eventsConsistencyCheck(hpp.consistencyEventSigma, hpp.consistencyEventMinSpadCount,
                        this, prev, target);
                if (status == Hist.DEVICE_ERROR_RANGE_COMPLETE) {
                    status = Hist.mergedPulseCheck(hpp.consistencyMinMaxTolerance, target);
                }
                target.rangeStatus = status;
            }
        }
    }
}
