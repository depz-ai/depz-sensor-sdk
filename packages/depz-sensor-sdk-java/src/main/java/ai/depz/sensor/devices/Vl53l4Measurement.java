package ai.depz.sensor.devices;

import ai.depz.sensor.sensors.vl53l4.Vl53l4Uld;

/**
 * One VL53L4CD ranging result ({@code VL53L4CD_ResultsData_t} plus the MCU timestamp: of the INT
 * edge when streamed, of the result read when polled).
 */
public record Vl53l4Measurement(
        long timestampUs,
        int rangeStatus,
        int distanceMm,
        int sigmaMm,
        int signalRateKcps,
        int ambientRateKcps,
        int signalPerSpadKcps,
        int ambientPerSpadKcps,
        int numberOfSpad,
        int streamCount) {
    public boolean valid() {
        return rangeStatus == 0;
    }

    /** What {@link #rangeStatus()} means ({@link Vl53l4Uld#RANGE_STATUS_NAMES}). */
    public String statusText() {
        String s = Vl53l4Uld.RANGE_STATUS_NAMES.get(rangeStatus);
        return s != null ? s : "unknown (" + rangeStatus + ")";
    }

    static Vl53l4Measurement from(long timestampUs, Vl53l4Uld.Results r) {
        return new Vl53l4Measurement(timestampUs, r.rangeStatus(), r.distanceMm(), r.sigmaMm(), r.signalRateKcps(),
                r.ambientRateKcps(), r.signalPerSpadKcps(), r.ambientPerSpadKcps(), r.numberOfSpad(), r.streamCount());
    }
}
