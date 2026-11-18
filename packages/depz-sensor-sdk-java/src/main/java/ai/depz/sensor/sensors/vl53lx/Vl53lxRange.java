package ai.depz.sensor.sensors.vl53lx;

import java.util.List;
import java.util.Map;

/**
 * One range result as a driver reports it, the same shape for every product of the family. On the
 * histogram driver {@code targets} holds every return and the top level repeats the first; the light
 * drivers leave it empty. {@code extra} is driver-specific — show it, don't branch on it.
 */
public record Vl53lxRange(int distanceMm, int status, String statusText, double signalKcps, double ambientKcps,
        double sigmaMm, double spads, List<Vl53lxTarget> targets, Map<String, Object> extra) {
    public boolean valid() {
        return status == 0;
    }
}
