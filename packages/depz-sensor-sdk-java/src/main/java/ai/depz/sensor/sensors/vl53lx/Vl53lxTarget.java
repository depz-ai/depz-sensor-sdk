package ai.depz.sensor.sensors.vl53lx;

/**
 * One return of a multi-target frame. {@code minRangeMm} / {@code maxRangeMm} are the edges of the
 * target's own pulse, which only the histogram driver knows.
 */
public record Vl53lxTarget(int distanceMm, int status, String statusText, double signalKcps, double ambientKcps,
        double sigmaMm, int minRangeMm, int maxRangeMm) {}
