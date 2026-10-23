package ai.depz.sensor.sensors.vl53lx.bare;

import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;

/**
 * {@code VL53LX_histogram_bin_data_t}: 24 bins plus everything the post-processing needs to read
 * them — the VCSEL period they were taken at, the phase of zero distance, the bin sequence and the
 * ambient estimate.
 */
public final class HistogramBinData implements Cloneable {
    public long resultInterruptStatus;
    public long resultRangeStatus;
    public long resultReportStatus;
    public long resultStreamCount;
    public long resultDssActualEffectiveSpads;
    public long phasecalResultReferencePhase;
    public long phasecalResultVcselStart;
    public long[] binData = new long[BareRegs.HISTOGRAM_BUFFER_SIZE];
    public long zoneId;
    public long firstBin;                                              // VL53LX_p_019
    public long numberOfBins = BareRegs.HISTOGRAM_BUFFER_SIZE;         // VL53LX_p_020, buffer
    public long binsInData = BareRegs.HISTOGRAM_BUFFER_SIZE;           // VL53LX_p_021, in use
    public long calConfigVcselStart;
    public long vcselWidth;
    public long fastOscFrequency;                                      // VL53LX_p_015
    public long vcselPeriod;                                           // VL53LX_p_005, the register value
    public int[] binSeq = new int[6];
    public long[] binRep = new long[6];                                // how often each code was sampled
    public long minBinValue;
    public long maxBinValue;
    public long numberOfAmbientBins;
    public long numberOfAmbientSamples;
    public long ambientEventsSum;
    public long ambientPerBin;                                         // VL53LX_p_028
    public long totalPeriodsElapsed;
    public long peakDurationUs;
    public long woiDurationUs;
    public long zeroDistancePhase;
    public long roiConfigUserRoiCentreSpad;
    public long roiConfigUserRoiRequestedGlobalXySize;

    public long rangeStatus() {
        return resultRangeStatus & BareRegs.RANGE_STATUS_MASK;
    }

    /** A copy whose arrays are copies too. */
    @Override
    public HistogramBinData clone() {
        try {
            HistogramBinData c = (HistogramBinData) super.clone();
            c.binData = binData.clone();
            c.binSeq = binSeq.clone();
            c.binRep = binRep.clone();
            return c;
        } catch (CloneNotSupportedException e) {
            throw new AssertionError(e);
        }
    }

    private static List<Long> list(long[] a) {
        List<Long> out = new ArrayList<>(a.length);
        for (long x : a) {
            out.add(x);
        }
        return out;
    }

    /** The fields under the C driver's (and the Python SDK's) names. */
    public Map<String, Object> fields() {
        Map<String, Object> m = new LinkedHashMap<>();
        m.put("result__interrupt_status", resultInterruptStatus);
        m.put("result__range_status", resultRangeStatus);
        m.put("result__report_status", resultReportStatus);
        m.put("result__stream_count", resultStreamCount);
        m.put("result__dss_actual_effective_spads", resultDssActualEffectiveSpads);
        m.put("phasecal_result__reference_phase", phasecalResultReferencePhase);
        m.put("phasecal_result__vcsel_start", phasecalResultVcselStart);
        m.put("bin_data", list(binData));
        m.put("zone_id", zoneId);
        m.put("first_bin", firstBin);
        m.put("number_of_bins", numberOfBins);
        m.put("bins_in_data", binsInData);
        m.put("cal_config__vcsel_start", calConfigVcselStart);
        m.put("vcsel_width", vcselWidth);
        m.put("fast_osc_frequency", fastOscFrequency);
        m.put("vcsel_period", vcselPeriod);
        List<Long> seq = new ArrayList<>(binSeq.length);
        for (int x : binSeq) {
            seq.add((long) x);
        }
        m.put("bin_seq", seq);
        m.put("bin_rep", list(binRep));
        m.put("min_bin_value", minBinValue);
        m.put("max_bin_value", maxBinValue);
        m.put("number_of_ambient_bins", numberOfAmbientBins);
        m.put("number_of_ambient_samples", numberOfAmbientSamples);
        m.put("ambient_events_sum", ambientEventsSum);
        m.put("ambient_per_bin", ambientPerBin);
        m.put("total_periods_elapsed", totalPeriodsElapsed);
        m.put("peak_duration_us", peakDurationUs);
        m.put("woi_duration_us", woiDurationUs);
        m.put("zero_distance_phase", zeroDistancePhase);
        m.put("roi_config__user_roi_centre_spad", roiConfigUserRoiCentreSpad);
        m.put("roi_config__user_roi_requested_global_xy_size", roiConfigUserRoiRequestedGlobalXySize);
        return m;
    }

    @Override
    public String toString() {
        StringBuilder bins = new StringBuilder();
        for (int i = 0; i < binData.length; i++) {
            bins.append(i == 0 ? "" : ", ").append(binData[i]);
        }
        return "<HistogramBinData stream " + resultStreamCount + " status " + rangeStatus() + " spads "
                + resultDssActualEffectiveSpads + " bins [" + bins + "]>";
    }
}
