package ai.depz.sensor.devices;

import ai.depz.sensor.sensors.vl53lx.Vl53lxRange;
import ai.depz.sensor.sensors.vl53lx.Vl53lxTarget;
import java.util.List;
import java.util.Map;

/**
 * One ranging result of the 1D family, the same shape for every product. On the histogram driver
 * {@code targets} holds every return, strongest first, the top level repeats the first and {@code bins}
 * carries the raw 24-bin histogram; the light drivers leave both empty.
 */
public record Vl53lxMeasurement(
        long timestampUs,
        int distanceMm,
        int status,
        String statusText,
        double signalKcps,
        double ambientKcps,
        double sigmaMm,
        double spads,
        List<Vl53lxTarget> targets,
        Map<String, Object> extra,
        Object bins) {

    /**
     * Statuses that mean "this distance is real": a histogram product reports 6 on the first frame of a
     * stream and 11 for two merged targets. Prefer {@link #plottable()} over {@link #valid()} there.
     */
    public static final List<Integer> PLOTTABLE_STATUSES = List.of(0, 6, 11);

    public boolean valid() {
        return status == 0;
    }

    public boolean plottable() {
        return PLOTTABLE_STATUSES.contains(status);
    }

    /** The distances a chart draws: the plottable targets in driver order, or the single distance. */
    public List<Integer> plotDistances() {
        if (!targets.isEmpty()) {
            return targets.stream().filter(t -> PLOTTABLE_STATUSES.contains(t.status())).map(Vl53lxTarget::distanceMm).toList();
        }
        return plottable() ? List.of(distanceMm) : List.of();
    }

    /** The one target a single-number readout shows, or null. */
    public Vl53lxTarget primaryTarget() {
        return targets.stream().filter(t -> PLOTTABLE_STATUSES.contains(t.status())).findFirst().orElse(null);
    }

    static Vl53lxMeasurement from(long timestampUs, Vl53lxRange r, Object bins) {
        return new Vl53lxMeasurement(timestampUs, r.distanceMm(), r.status(), r.statusText(), r.signalKcps(),
                r.ambientKcps(), r.sigmaMm(), r.spads(), r.targets(), r.extra(), bins);
    }
}
