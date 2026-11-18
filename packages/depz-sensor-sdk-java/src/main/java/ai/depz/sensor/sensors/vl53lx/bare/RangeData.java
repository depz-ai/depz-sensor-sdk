package ai.depz.sensor.sensors.vl53lx.bare;

/**
 * {@code VL53LX_range_data_t}: one target. The event sums and phases are what the algorithm
 * produced; the {@code *RangeMm} fields and the rates are what they mean.
 */
public final class RangeData {
    public long rangeId;
    public long startBin;            // VL53LX_p_012, window start
    public long firstBin;            // VL53LX_p_019, first bin over threshold
    public long peakBin;             // VL53LX_p_023
    public long lastBin;             // VL53LX_p_024
    public long endBin;              // VL53LX_p_013
    public long widthBins;           // VL53LX_p_025, bins over threshold
    public long windowBins;          // VL53LX_p_029, bins in the window
    public long vcselWidth;
    public long fastOscFrequency;
    public long zeroDistancePhase;
    public long spads;               // VL53LX_p_004
    public long totalPeriodsElapsed;
    public long peakDurationUs;
    public long woiDurationUs;
    public long ambientEvents;       // VL53LX_p_016
    public long totalEvents;         // VL53LX_p_017
    public long signalEvents;        // VL53LX_p_010
    public long peakSignalCountRateMcps;
    public long avgSignalCountRateMcps;
    public long ambientCountRateMcps;
    public long totalRatePerSpadMcps;
    public long signalEventsPerSpadKcps;   // VL53LX_p_009
    public long sigma;               // VL53LX_p_002, 9.7 fixed point mm
    public long phaseStart;          // VL53LX_p_026
    public long phaseMean;           // VL53LX_p_011
    public long phaseEnd;            // VL53LX_p_027
    public long minRangeMm;
    public long medianRangeMm;
    public long maxRangeMm;
    public long rangeStatus = Hist.DEVICE_ERROR_NO_UPDATE;

    public long sigmaMm() {
        return sigma >> 7;
    }

    /** The peak signal rate is 9.7 Mcps. */
    public long signalKcps() {
        return peakSignalCountRateMcps * 1000 / 128;
    }

    public long ambientKcps() {
        return ambientCountRateMcps * 1000 / 128;
    }

    @Override
    public String toString() {
        return "<RangeData " + medianRangeMm + " mm [" + minRangeMm + ".." + maxRangeMm + "] sigma " + sigmaMm()
                + " mm signal " + signalKcps() + " kcps status " + rangeStatus + ">";
    }
}
