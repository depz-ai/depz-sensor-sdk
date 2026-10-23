package ai.depz.sensor.sensors.vl53lx.bare;

/**
 * {@code VL53LX_hist_post_process_config_t}, as {@code VL53LX_init_hist_post_process_config_struct()}
 * leaves it; the preset modes set the valid phase window here.
 */
public final class HistPostProcessConfig {
    private static long d(String name) {
        return BareTables.DEFAULTS.get(name);
    }

    public long histAlgoSelect = d("TUNINGPARM_HIST_ALGO_SELECT_DEFAULT");
    public long histTargetOrder = d("TUNINGPARM_HIST_TARGET_ORDER_DEFAULT");
    public long filterWoi0 = d("TUNINGPARM_HIST_FILTER_WOI_0_DEFAULT");
    public long filterWoi1 = d("TUNINGPARM_HIST_FILTER_WOI_1_DEFAULT");
    public long histAmbEstMethod = d("TUNINGPARM_HIST_AMB_EST_METHOD_DEFAULT");
    public long ambientThreshSigma0 = d("TUNINGPARM_HIST_AMB_THRESH_SIGMA_0_DEFAULT");
    public long ambientThreshSigma1 = d("TUNINGPARM_HIST_AMB_THRESH_SIGMA_1_DEFAULT");
    public long ambientThreshEventsScaler = d("TUNINGPARM_HIST_AMB_EVENTS_SCALER_DEFAULT");
    public long minAmbientThreshEvents = d("TUNINGPARM_HIST_MIN_AMB_THRESH_EVENTS_DEFAULT");
    public long noiseThreshold = d("TUNINGPARM_HIST_NOISE_THRESHOLD_DEFAULT");
    public long signalTotalEventsLimit = d("TUNINGPARM_HIST_SIGNAL_TOTAL_EVENTS_LIMIT_DEFAULT");
    public long sigmaEstimatorSigmaRefMm = d("TUNINGPARM_HIST_SIGMA_EST_REF_MM_DEFAULT");
    public long sigmaThresh = d("TUNINGPARM_HIST_SIGMA_THRESH_MM_DEFAULT");
    public long rangeOffsetMm;
    public long gainFactor = d("TUNINGPARM_HIST_GAIN_FACTOR_DEFAULT");
    public long validPhaseLow = 0x08;
    public long validPhaseHigh = 0x88;
    public long consistencyPhaseTolerance = d("TUNINGPARM_CONSISTENCY_HIST_PHASE_TOLERANCE_DEFAULT");
    public long consistencyEventSigma = d("TUNINGPARM_CONSISTENCY_HIST_EVENT_SIGMA_DEFAULT");
    public long consistencyEventMinSpadCount = d("TUNINGPARM_CONSISTENCY_HIST_EVENT_SIGMA_MIN_SPAD_LIMIT_DEFAULT");
    public long consistencyMinMaxTolerance = d("TUNINGPARM_CONSISTENCY_HIST_MIN_MAX_TOLERANCE_MM_DEFAULT");
    public long crosstalkCompensationEnable;
    public long crosstalkDetectMinValidRangeMm = d("TUNINGPARM_XTALK_DETECT_MIN_VALID_RANGE_MM_DEFAULT");
    public long crosstalkDetectMaxValidRangeMm = d("TUNINGPARM_XTALK_DETECT_MAX_VALID_RANGE_MM_DEFAULT");
    public long crosstalkDetectMaxValidRateKcps = d("TUNINGPARM_XTALK_DETECT_MAX_VALID_RATE_KCPS_DEFAULT");
    public long crosstalkDetectMaxSigmaMm = d("TUNINGPARM_XTALK_DETECT_MAX_SIGMA_MM_DEFAULT");
    public long crosstalkDetectEventSigma = d("TUNINGPARM_XTALK_DETECT_EVENT_SIGMA_DEFAULT");
    public long crosstalkDetectMinMaxTolerance = d("TUNINGPARM_XTALK_DETECT_MIN_MAX_TOLERANCE_DEFAULT");
    // Filled in by readP2pData() from the customer NVM block.
    public long crosstalkCompensationPlaneOffsetKcps;
    public long crosstalkCompensationXPlaneGradientKcps;
    public long crosstalkCompensationYPlaneGradientKcps;
}
