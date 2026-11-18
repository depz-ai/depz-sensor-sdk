package ai.depz.sensor.sensors.vl53lx.bare;

import java.util.ArrayList;
import java.util.List;

/** {@code VL53LX_range_results_t}, reduced to the single-zone case. */
public final class RangeResults {
    public long streamCount;
    public final List<RangeData> targets = new ArrayList<>();   // VL53LX_p_003, active_results entries
    public long[] ambientDmaxMm = new long[5];                   // VL53LX_p_022
    public long wrapDmaxMm;
}
