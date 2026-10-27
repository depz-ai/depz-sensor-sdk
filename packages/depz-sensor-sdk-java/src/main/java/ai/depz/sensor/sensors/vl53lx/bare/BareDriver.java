package ai.depz.sensor.sensors.vl53lx.bare;

import ai.depz.sensor.sensors.vl53lx.Vl53lxPlatform;
import java.util.Arrays;
import java.util.HashMap;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;

/**
 * ST's VL53LX BareDriver ranging path, on top of {@link DeviceImage}: {@code VL53LX_read_p2p_data()},
 * the histogram preset modes, {@code VL53LX_set_timeouts_us()}, {@code VL53LX_init_and_start_range()}
 * and {@code VL53LX_get_histogram_bin_data()}. Nothing here writes a register on its own: the preset
 * builders edit the host-side image and the whole span 0x0001..0x0087 goes out as one transfer when
 * ranging starts.
 */
public final class BareDriver {
    // The distance-mode tails (vl53lx_api_preset_modes.c): bin sequence, VCSEL periods, timeouts, phases.
    private record Tail(int[] even, int[] odd, int vcselA, int vcselB, int mmA, int mmB, int rangeA, int rangeB,
            int calVcselStart, int validPhaseHigh, String phaseRtn, String phaseRef, int extraSequence) {}

    private static final Map<String, Tail> TAILS = Map.of(
            "long", new Tail(new int[] {7, 0, 1, 2, 3, 4}, new int[] {0, 1, 2, 3, 4, 5}, 0x09, 0x0B, 0x0021, 0x001B,
                    0x0029, 0x0022, 0x09, 0x88, "tp_init_phase_rtn_hist_long", "tp_init_phase_ref_hist_long", 0),
            "medium", new Tail(new int[] {7, 0, 1, 1, 2, 2}, new int[] {0, 1, 2, 1, 2, 3}, 0x05, 0x07, 0x0036, 0x0028,
                    0x0044, 0x0033, 0x05, 0x48, "tp_init_phase_rtn_hist_med", "tp_init_phase_ref_hist_med", 0),
            "short", new Tail(new int[] {7, 7, 0, 1, 1, 1}, new int[] {0, 1, 1, 1, 2, 2}, 0x03, 0x05, 0x0052, 0x0037,
                    0x0066, 0x0044, 0x03, 0x28, "tp_init_phase_rtn_hist_short", "tp_init_phase_ref_hist_short",
                    BareRegs.SEQUENCE_MM1_EN));

    private static final Map<String, String> PHASECAL_KEY = Map.of(
            "short", "tp_phasecal_timeout_hist_short_us",
            "medium", "tp_phasecal_timeout_hist_med_us",
            "long", "tp_phasecal_timeout_hist_long_us");

    private final Vl53lxPlatform p;
    private final DeviceImage img;
    private final NvmReader nvm;
    private final Map<String, Long> tuning = new HashMap<>(BareTables.TUNING);
    private final HistConfig histCfg = new HistConfig();
    private HistPostProcessConfig hpp = new HistPostProcessConfig();
    private final LlState state = new LlState();

    // Carried from frame to frame: the C driver keeps one bin_data struct and reads last frame's ambient out of
    // it when it picks the bin sequence, before this frame's ambient is estimated.
    private long ambientEventsSum;

    // Factory data with no register of its own (read_p2p_data).
    private byte[] rtnGoodSpadsMap = new byte[32];
    private NvmReader.OpticalCentre opticalCentre;
    private NvmReader.CalPeakRateMap calPeakRateMap;
    private NvmReader.AdditionalOffsetCalData addOffCalData;
    private Map<String, Long> fmtDmaxCal;
    private Map<String, Long> mmRoi;
    private long resultOscCalibrateVal;

    private String presetMode = "medium";
    private long phasecalConfigTimeoutUs = 1000;
    private long mmConfigTimeoutUs = 2000;
    private long rangeConfigTimeoutUs = 13000;
    private long interMeasurementPeriodMs = 100;
    private long measurementMode;

    public BareDriver(Vl53lxPlatform platform) {
        p = platform;
        img = new DeviceImage(platform);
        nvm = new NvmReader(platform, img);
    }

    public DeviceImage img() {
        return img;
    }

    public NvmReader nvm() {
        return nvm;
    }

    /** The tuning parameters, a mutable copy of {@link BareTables#TUNING}. */
    public Map<String, Long> tuning() {
        return tuning;
    }

    public HistConfig histCfg() {
        return histCfg;
    }

    public HistPostProcessConfig hpp() {
        return hpp;
    }

    public LlState state() {
        return state;
    }

    public byte[] rtnGoodSpadsMap() {
        return rtnGoodSpadsMap;
    }

    public NvmReader.OpticalCentre opticalCentre() {
        return opticalCentre;
    }

    public NvmReader.CalPeakRateMap calPeakRateMap() {
        return calPeakRateMap;
    }

    public NvmReader.AdditionalOffsetCalData addOffCalData() {
        return addOffCalData;
    }

    public Map<String, Long> fmtDmaxCal() {
        return fmtDmaxCal;
    }

    public Map<String, Long> mmRoi() {
        return mmRoi;
    }

    public long resultOscCalibrateVal() {
        return resultOscCalibrateVal;
    }

    public String presetMode() {
        return presetMode;
    }

    public long phasecalConfigTimeoutUs() {
        return phasecalConfigTimeoutUs;
    }

    public long mmConfigTimeoutUs() {
        return mmConfigTimeoutUs;
    }

    public long rangeConfigTimeoutUs() {
        return rangeConfigTimeoutUs;
    }

    public long interMeasurementPeriodMs() {
        return interMeasurementPeriodMs;
    }

    public long measurementMode() {
        return measurementMode;
    }

    private long tp(String name) {
        return tuning.get(name);
    }

    /** VL53LX_read_p2p_data(): three register blocks and four NVM regions (~0.1 s over USB). */
    public void readP2pData() {
        img.pull("static_nvm_managed");
        img.pull("customer_nvm_managed");
        img.pull("nvm_copy_data");
        rtnGoodSpadsMap = BareMath.rtnGoodSpads(img.nvmCopyData());

        RegBlock customer = img.customerNvm();
        hpp.crosstalkCompensationPlaneOffsetKcps = customer.get("algo__crosstalk_compensation_plane_offset_kcps");
        hpp.crosstalkCompensationXPlaneGradientKcps = customer.get("algo__crosstalk_compensation_x_plane_gradient_kcps");
        hpp.crosstalkCompensationYPlaneGradientKcps = customer.get("algo__crosstalk_compensation_y_plane_gradient_kcps");

        opticalCentre = nvm.readOpticalCentre();
        calPeakRateMap = nvm.readCalPeakRateMap();
        NvmReader.AdditionalOffsetCalData cal = nvm.readAdditionalOffsetCalData();

        // Our boards have no FMT offset calibration. ST's fallback: nominal MM1/MM2 peak rates, and the effective
        // SPAD counts worked out here.
        if (cal.mmInnerPeakSignalCountRtnMcps() == 0 && cal.mmOuterPeakSignalCountRtnMcps() == 0) {
            long[] io = BareMath.calcMmEffectiveSpads(
                    (int) img.nvmCopyData().get("roi_config__mode_roi_centre_spad"),
                    (int) img.nvmCopyData().get("roi_config__mode_roi_xy_size"),
                    0xC7, 0xFF, rtnGoodSpadsMap, BareRegs.RTN_SPAD_APERTURE_TRANSMISSION);
            cal = new NvmReader.AdditionalOffsetCalData(io[0], io[1], 0x0080, 0x0180);
        }
        addOffCalData = cal;

        NvmReader.FmtRangeData fmt = nvm.readFmtRangeResults();
        Map<String, Long> dmax = new LinkedHashMap<>();
        dmax.put("ref__actual_effective_spads", (long) fmt.actualEffectiveRtnSpads());
        dmax.put("ref__peak_signal_count_rate_mcps", (long) fmt.peakSignalCountRateRtnMcps());
        dmax.put("ref__distance_mm", (long) fmt.measuredDistanceMm());
        dmax.put("ref_reflectance_pc",
                (long) (calPeakRateMap.calReflectancePc() != 0 ? calPeakRateMap.calReflectancePc() : 0x0014));
        dmax.put("coverglass_transmission", 0x0100L);
        fmtDmaxCal = dmax;

        resultOscCalibrateVal = p.readWord(BareRegs.RESULT_OSC_CALIBRATE_VAL);

        if (img.staticNvm().get("osc_measured__fast_osc__frequency") < 0x1000) {
            img.staticNvm().set("osc_measured__fast_osc__frequency", 0xBCCC);
        }

        // VL53LX_get_mode_mitigation_roi
        int[] yx = BareMath.decodeRowCol((int) img.nvmCopyData().get("roi_config__mode_roi_centre_spad"));
        int y = yx[0];
        int x = yx[1];
        long xySize = img.nvmCopyData().get("roi_config__mode_roi_xy_size");
        Map<String, Long> roi = new LinkedHashMap<>();
        roi.put("x_centre", (long) x);
        roi.put("y_centre", (long) y);
        roi.put("width", xySize & 0x0F);
        roi.put("height", xySize >> 4);
        mmRoi = roi;

        if (opticalCentre.xCentre() == 0 && opticalCentre.yCentre() == 0) {
            opticalCentre = new NvmReader.OpticalCentre(x << 4, y << 4);
        }
    }

    /**
     * VL53LX_data_init + VL53LX_DataInit: the factory data, the default preset mode and timing
     * budget. Leaves the sensor untouched apart from the reads.
     */
    public void dataInit() {
        state.reset();
        hpp = new HistPostProcessConfig();
        readP2pData();
        setPresetMode("medium", null, 1000L);
        setMeasurementTimingBudgetUs(33333);
    }

    /** VL53LX_set_preset_mode() for the three histogram ranging modes, by distance mode. */
    public void setPresetMode(String mode) {
        setPresetMode(mode, null, null);
    }

    /**
     * VL53LX_set_preset_mode() for the three histogram ranging modes, by distance mode; a null DSS
     * target or period keeps the tuning default / the current period.
     */
    public void setPresetMode(String mode, Long dssTargetMcps, Long interMeasurementPeriodMs) {
        if (!BareRegs.DISTANCE_MODES.contains(mode)) {
            throw new IllegalArgumentException("unknown distance mode '" + mode + "'");
        }
        presetMode = mode;
        long dss = dssTargetMcps != null ? dssTargetMcps : tp("tp_dss_target_histo_mcps");
        long period = interMeasurementPeriodMs != null ? interMeasurementPeriodMs : this.interMeasurementPeriodMs;

        // VL53LX_get_preset_mode_timing_cfg: only the phasecal timeout differs between the three modes.
        long phasecalUs = tp(PHASECAL_KEY.get(mode));
        long mmUs = tp("tp_mm_timeout_histo_us");
        long rangeUs = tp("tp_range_timeout_histo_us");

        state.reset();
        presetModeHistogramRanging();
        presetTail(TAILS.get(mode));

        img.staticConfig().set("dss_config__target_total_rate_mcps", dss);
        setTimeoutsUs(phasecalUs, mmUs, rangeUs);
        setInterMeasurementPeriodMs(period);
        updateRangeOffset();
    }

    // get_device_results() recomputes hpp.range_offset_mm every frame under MM1_MM2_OFFSETS; its inputs are the
    // factory data and the user ROI, so once per preset is the same answer.
    private void updateRangeOffset() {
        if (addOffCalData == null) {
            return;
        }
        RegBlock customer = img.customerNvm();
        RegBlock nvmData = img.nvmCopyData();
        RegBlock dynamic = img.dynamicConfig();
        hpp.rangeOffsetMm = BareMath.combineMm1Mm2Offsets(
                customer.get("mm_config__inner_offset_mm"), customer.get("mm_config__outer_offset_mm"),
                (int) nvmData.get("roi_config__mode_roi_centre_spad"), (int) nvmData.get("roi_config__mode_roi_xy_size"),
                (int) dynamic.get("roi_config__user_roi_centre_spad"),
                (int) dynamic.get("roi_config__user_roi_requested_global_xy_size"),
                addOffCalData, rtnGoodSpadsMap, img.generalConfig().get("dss_config__aperture_attenuation"));
    }

    /** {@code IsL4()} of the L4CX BareDriver: the die, read from NVM, not the product name. 0xEC is the L4ED. */
    public boolean isL4() {
        RegBlock n = img.nvmCopyData();
        long model = n.get("identification__model_id");
        return n.get("identification__module_type") == 0xAA && (model == 0xEB || model == 0xEC);
    }

    public List<String> distanceModesForDie() {
        return isL4() ? BareRegs.L4_DISTANCE_MODES : BareRegs.DISTANCE_MODES;
    }

    public long fdaMaxTimingBudgetUs() {
        return isL4() ? BareRegs.L4_FDA_MAX_TIMING_BUDGET_US : BareRegs.FDA_MAX_TIMING_BUDGET_US;
    }

    /**
     * VL53LX_SetDistanceMode(): the preset mode with the caller's timeouts put back. Short is refused
     * on an L4 die, as the L4CX BareDriver does (the A frame of the pair ranges on the wrong side of
     * the wrap).
     */
    public void setDistanceMode(String mode) {
        if (!distanceModesForDie().contains(mode)) {
            throw new IllegalArgumentException("distance mode " + mode + " not available on this die");
        }
        long phasecalUs = phasecalConfigTimeoutUs;
        long mmUs = mmConfigTimeoutUs;
        long rangeUs = rangeConfigTimeoutUs;
        setPresetMode(mode);
        setTimeoutsUs(phasecalUs, mmUs, rangeUs);
    }

    // VL53LX_preset_mode_standard_ranging(): the base every histogram mode is built on.
    private void presetModeStandardRanging() {
        RegBlock stat = img.staticConfig();
        RegBlock general = img.generalConfig();
        RegBlock timing = img.timingConfig();
        RegBlock dynamic = img.dynamicConfig();
        RegBlock system = img.systemControl();

        stat.set("dss_config__target_total_rate_mcps", 0x0A00);
        stat.set("debug__ctrl", 0x00);
        stat.set("test_mode__ctrl", 0x00);
        stat.set("clk_gating__ctrl", 0x00);
        stat.set("nvm_bist__ctrl", 0x00);
        stat.set("nvm_bist__num_nvm_words", 0x00);
        stat.set("nvm_bist__start_address", 0x00);
        stat.set("host_if__status", 0x00);
        stat.set("pad_i2c_hv__config", 0x00);
        stat.set("pad_i2c_hv__extsup_config", 0x00);
        stat.set("gpio_hv_pad__ctrl", 0x00);
        stat.set("gpio_hv_mux__ctrl",
                BareRegs.DEVICE_INTERRUPT_POLARITY_ACTIVE_LOW | BareRegs.DEVICE_GPIO_MODE_OUTPUT_RANGE_AND_ERROR_INTERRUPTS);
        stat.set("gpio__tio_hv_status", 0x02);
        stat.set("gpio__fio_hv_status", 0x00);
        stat.set("ana_config__spad_sel_pswidth", 0x02);
        stat.set("ana_config__vcsel_pulse_width_offset", 0x08);
        stat.set("ana_config__fast_osc__config_ctrl", 0x00);
        stat.set("sigma_estimator__effective_pulse_width_ns", tp("tp_lite_sigma_est_pulse_width_ns"));
        stat.set("sigma_estimator__effective_ambient_width_ns", tp("tp_lite_sigma_est_amb_width_ns"));
        stat.set("sigma_estimator__sigma_ref_mm", tp("tp_lite_sigma_ref_mm"));
        stat.set("algo__crosstalk_compensation_valid_height_mm", 0x01);
        stat.set("spare_host_config__static_config_spare_0", 0x00);
        stat.set("spare_host_config__static_config_spare_1", 0x00);
        stat.set("algo__range_ignore_threshold_mcps", 0x0000);
        stat.set("algo__range_ignore_valid_height_mm", 0xFF);
        stat.set("algo__range_min_clip", tp("tp_lite_min_clip"));
        stat.set("algo__consistency_check__tolerance", tp("tp_consistency_lite_phase_tolerance"));
        stat.set("spare_host_config__static_config_spare_2", 0x00);
        stat.set("sd_config__reset_stages_msb", 0x00);
        stat.set("sd_config__reset_stages_lsb", 0x00);

        general.set("gph_config__stream_count_update_value", 0x00);
        general.set("global_config__stream_divider", 0x00);
        general.set("system__interrupt_config_gpio", BareRegs.INTERRUPT_CONFIG_NEW_SAMPLE_READY);
        general.set("cal_config__vcsel_start", 0x0B);
        general.set("cal_config__repeat_rate", tp("tp_cal_repeat_rate"));
        general.set("global_config__vcsel_width", 0x02);
        general.set("phasecal_config__timeout_macrop", 0x0D);
        general.set("phasecal_config__target", tp("tp_phasecal_target"));
        general.set("phasecal_config__override", 0x00);
        general.set("dss_config__roi_mode_control", BareRegs.DEVICE_DSS_MODE_TARGET_RATE);
        general.set("system__thresh_rate_high", 0x0000);
        general.set("system__thresh_rate_low", 0x0000);
        general.set("dss_config__manual_effective_spads_select", 0x8C00);
        general.set("dss_config__manual_block_select", 0x00);
        general.set("dss_config__aperture_attenuation", 0x38);
        general.set("dss_config__max_spads_limit", 0xFF);
        general.set("dss_config__min_spads_limit", 0x01);

        timing.set("mm_config__timeout_macrop_a_hi", 0x00);
        timing.set("mm_config__timeout_macrop_a_lo", 0x1A);
        timing.set("mm_config__timeout_macrop_b_hi", 0x00);
        timing.set("mm_config__timeout_macrop_b_lo", 0x20);
        timing.set("range_config__timeout_macrop_a_hi", 0x01);
        timing.set("range_config__timeout_macrop_a_lo", 0xCC);
        timing.set("range_config__vcsel_period_a", 0x0B);
        timing.set("range_config__timeout_macrop_b_hi", 0x01);
        timing.set("range_config__timeout_macrop_b_lo", 0xF5);
        timing.set("range_config__vcsel_period_b", 0x09);
        timing.set("range_config__sigma_thresh", tp("tp_lite_med_sigma_thresh_mm"));
        timing.set("range_config__min_count_rate_rtn_limit_mcps", tp("tp_lite_med_min_count_rate_rtn_mcps"));
        timing.set("range_config__valid_phase_low", 0x08);
        timing.set("range_config__valid_phase_high", 0x78);
        timing.set("system__intermeasurement_period", 0x00000000);
        timing.set("system__fractional_enable", 0x00);

        // The struct's initial state; every histogram mode overwrites it.
        HistConfig h = histCfg;
        h.lowAmbEvenBin01 = 0x07;
        h.lowAmbEvenBin23 = 0x21;
        h.lowAmbEvenBin45 = 0x43;
        h.lowAmbOddBin01 = 0x10;
        h.lowAmbOddBin23 = 0x32;
        h.lowAmbOddBin45 = 0x54;
        h.midAmbEvenBin01 = 0x07;
        h.midAmbEvenBin23 = 0x21;
        h.midAmbEvenBin45 = 0x43;
        h.midAmbOddBin01 = 0x10;
        h.midAmbOddBin2 = 0x02;
        h.midAmbOddBin34 = 0x43;
        h.midAmbOddBin5 = 0x05;
        h.userBinOffset = 0x00;
        h.highAmbEvenBin01 = 0x07;
        h.highAmbEvenBin23 = 0x21;
        h.highAmbEvenBin45 = 0x43;
        h.highAmbOddBin01 = 0x10;
        h.highAmbOddBin23 = 0x32;
        h.highAmbOddBin45 = 0x54;
        h.ambThreshLow = 0xFFFF;
        h.ambThreshHigh = 0xFFFF;
        h.spadArraySelection = 0x00;

        dynamic.set("system__grouped_parameter_hold_0", 0x01);
        dynamic.set("system__thresh_high", 0x0000);
        dynamic.set("system__thresh_low", 0x0000);
        dynamic.set("system__enable_xtalk_per_quadrant", 0x00);
        dynamic.set("system__seed_config", tp("tp_lite_seed_cfg"));
        dynamic.set("sd_config__woi_sd0", 0x0B);
        dynamic.set("sd_config__woi_sd1", 0x09);
        dynamic.set("sd_config__initial_phase_sd0", tp("tp_init_phase_rtn_lite_med"));
        dynamic.set("sd_config__initial_phase_sd1", tp("tp_init_phase_ref_lite_med"));
        dynamic.set("system__grouped_parameter_hold_1", 0x01);
        dynamic.set("sd_config__first_order_select", tp("tp_lite_first_order_select"));
        dynamic.set("sd_config__quantifier", tp("tp_lite_quantifier"));
        dynamic.set("roi_config__user_roi_centre_spad", 0xC7);
        dynamic.set("roi_config__user_roi_requested_global_xy_size", 0xFF);
        dynamic.set("system__sequence_config", BareRegs.SEQUENCE_VHV_EN | BareRegs.SEQUENCE_PHASECAL_EN
                | BareRegs.SEQUENCE_DSS1_EN | BareRegs.SEQUENCE_DSS2_EN | BareRegs.SEQUENCE_MM2_EN
                | BareRegs.SEQUENCE_RANGE_EN);
        dynamic.set("system__grouped_parameter_hold", 0x02);

        system.set("system__stream_count_ctrl", 0x00);
        system.set("firmware__enable", 0x01);
        system.set("system__interrupt_clear", BareRegs.CLEAR_RANGE_INT);
        system.set("system__mode_start", BareRegs.DEVICE_SCHEDULER_MODE_STREAMING
                | BareRegs.DEVICE_READOUT_MODE_SINGLE_SD | BareRegs.DEVICE_MEASUREMENT_MODE_BACK_TO_BACK);
    }

    // VL53LX_preset_mode_histogram_ranging(): standard ranging, then the histogram engine.
    private void presetModeHistogramRanging() {
        presetModeStandardRanging();

        img.staticConfig().set("dss_config__target_total_rate_mcps", 0x1400);
        histCfg.setBinSequence(new int[] {7, 0, 1, 2, 3, 4}, new int[] {0, 1, 2, 3, 4, 5});

        RegBlock timing = img.timingConfig();
        RegBlock dynamic = img.dynamicConfig();
        timing.set("range_config__vcsel_period_a", 0x09);
        timing.set("range_config__vcsel_period_b", 0x0B);
        dynamic.set("sd_config__woi_sd0", 0x09);
        dynamic.set("sd_config__woi_sd1", 0x0B);
        timing.set("mm_config__timeout_macrop_a_hi", 0x00);
        timing.set("mm_config__timeout_macrop_a_lo", 0x20);
        timing.set("mm_config__timeout_macrop_b_hi", 0x00);
        timing.set("mm_config__timeout_macrop_b_lo", 0x1A);
        timing.set("range_config__timeout_macrop_a_hi", 0x00);
        timing.set("range_config__timeout_macrop_a_lo", 0x28);
        timing.set("range_config__timeout_macrop_b_hi", 0x00);
        timing.set("range_config__timeout_macrop_b_lo", 0x21);
        img.generalConfig().set("phasecal_config__timeout_macrop", 0xF5);

        hpp.validPhaseLow = 0x08;
        hpp.validPhaseHigh = 0x88;

        histCfg.copyToStaticCfg(img.staticConfig(), timing, dynamic);
        dynamic.set("system__sequence_config", BareRegs.SEQUENCE_VHV_EN | BareRegs.SEQUENCE_PHASECAL_EN
                | BareRegs.SEQUENCE_DSS1_EN | BareRegs.SEQUENCE_DSS2_EN | BareRegs.SEQUENCE_RANGE_EN);
        img.systemControl().set("system__mode_start", BareRegs.DEVICE_SCHEDULER_MODE_HISTOGRAM
                | BareRegs.DEVICE_READOUT_MODE_DUAL_SD | BareRegs.DEVICE_MEASUREMENT_MODE_BACK_TO_BACK);
    }

    // The shape all three distance modes share: a bin sequence, a VCSEL period pair and its timeouts.
    private void presetTail(Tail t) {
        RegBlock stat = img.staticConfig();
        RegBlock general = img.generalConfig();
        RegBlock timing = img.timingConfig();
        RegBlock dynamic = img.dynamicConfig();

        histCfg.setBinSequence(t.even(), t.odd());
        histCfg.copyToStaticCfg(stat, timing, dynamic);

        timing.set("range_config__vcsel_period_a", t.vcselA());
        timing.set("range_config__vcsel_period_b", t.vcselB());
        timing.set("mm_config__timeout_macrop_a_hi", t.mmA() >> 8);
        timing.set("mm_config__timeout_macrop_a_lo", t.mmA() & 0xFF);
        timing.set("mm_config__timeout_macrop_b_hi", t.mmB() >> 8);
        timing.set("mm_config__timeout_macrop_b_lo", t.mmB() & 0xFF);
        timing.set("range_config__timeout_macrop_a_hi", t.rangeA() >> 8);
        timing.set("range_config__timeout_macrop_a_lo", t.rangeA() & 0xFF);
        timing.set("range_config__timeout_macrop_b_hi", t.rangeB() >> 8);
        timing.set("range_config__timeout_macrop_b_lo", t.rangeB() & 0xFF);

        general.set("cal_config__vcsel_start", t.calVcselStart());
        general.set("phasecal_config__timeout_macrop", 0xF5);

        dynamic.set("sd_config__woi_sd0", t.vcselA());
        dynamic.set("sd_config__woi_sd1", t.vcselB());
        dynamic.set("sd_config__initial_phase_sd0", tp(t.phaseRtn()));
        dynamic.set("sd_config__initial_phase_sd1", tp(t.phaseRef()));
        dynamic.set("system__sequence_config", BareRegs.SEQUENCE_VHV_EN | BareRegs.SEQUENCE_PHASECAL_EN
                | BareRegs.SEQUENCE_DSS1_EN | BareRegs.SEQUENCE_DSS2_EN | t.extraSequence() | BareRegs.SEQUENCE_RANGE_EN);

        hpp.validPhaseLow = 0x08;
        hpp.validPhaseHigh = t.validPhaseHigh();

        img.systemControl().set("system__mode_start", BareRegs.DEVICE_SCHEDULER_MODE_HISTOGRAM
                | BareRegs.DEVICE_READOUT_MODE_DUAL_SD | BareRegs.DEVICE_MEASUREMENT_MODE_BACK_TO_BACK);
    }

    public void setTimeoutsUs(long phasecalUs, long mmUs, long rangeUs) {
        phasecalConfigTimeoutUs = phasecalUs;
        mmConfigTimeoutUs = mmUs;
        rangeConfigTimeoutUs = rangeUs;
        BareMath.calcTimeoutRegisterValues(phasecalUs, mmUs, rangeUs,
                img.staticNvm().get("osc_measured__fast_osc__frequency"), img.generalConfig(), img.timingConfig());
    }

    public void setInterMeasurementPeriodMs(long periodMs) {
        if (resultOscCalibrateVal == 0) {
            throw new ArithmeticException("result__osc_calibrate_val is 0");
        }
        interMeasurementPeriodMs = periodMs;
        img.timingConfig().set("system__intermeasurement_period", periodMs * resultOscCalibrateVal);
    }

    /**
     * VL53LX_SetMeasurementTimingBudgetMicroSeconds(): the range timeout is one sixth of the budget
     * after a fixed guard comes off.
     */
    public void setMeasurementTimingBudgetUs(long budgetUs) {
        if (!(BareRegs.TIMING_GUARD_US < budgetUs && budgetUs <= 10000000)) {
            throw new IllegalArgumentException("timing budget " + budgetUs + " us out of range");
        }
        long rangeUs = (budgetUs - BareRegs.TIMING_GUARD_US) / BareRegs.TIMING_DIVISOR;
        if (rangeUs * BareRegs.TIMING_DIVISOR > fdaMaxTimingBudgetUs()) {
            throw new IllegalArgumentException("timing budget " + budgetUs + " us out of range");
        }
        setTimeoutsUs(phasecalConfigTimeoutUs, mmConfigTimeoutUs, rangeUs);
    }

    /** VL53LX_load_patch(): a six-instruction phasecal patch, written with the firmware stopped. */
    public void loadPatch() {
        long pp = tp("tp_phasecal_patch_power");
        int power = pp == 1 ? 0x10 : pp == 2 ? 0x20 : pp == 3 ? 0x40 : 0x00;
        p.writeByte(BareRegs.FIRMWARE_ENABLE, 0x00);
        enablePowerforce();
        p.writeMulti(BareRegs.PATCH_OFFSET_0, new byte[] {0x29, (byte) 0xC9, 0x0E, 0x40, 0x28, (byte) power});
        p.writeMulti(BareRegs.PATCH_ADDRESS_0, new byte[] {0x03, 0x6D, 0x03, 0x6F, 0x07, 0x29});
        p.writeMulti(BareRegs.PATCH_JMP_ENABLES, new byte[] {0x00, 0x07});
        p.writeMulti(BareRegs.PATCH_DATA_ENABLES, new byte[] {0x00, 0x07});
        p.writeByte(BareRegs.PATCH_CTRL, 0x01);
        p.writeByte(BareRegs.FIRMWARE_ENABLE, 0x01);
    }

    public void unloadPatch() {
        p.writeByte(BareRegs.FIRMWARE_ENABLE, 0x00);
        disablePowerforce();
        p.writeByte(BareRegs.PATCH_CTRL, 0x00);
        p.writeByte(BareRegs.FIRMWARE_ENABLE, 0x01);
    }

    private void enablePowerforce() {
        p.writeByte(BareRegs.POWER_MANAGEMENT_GO1_POWER_FORCE, 0x01);
        p.sleepMs(1); // 250 us settling, rounded up
    }

    private void disablePowerforce() {
        p.writeByte(BareRegs.POWER_MANAGEMENT_GO1_POWER_FORCE, 0x00);
    }

    /** VL53LX_init_and_start_range() with every block: write the whole configuration and start. */
    public void initAndStartRange() {
        initAndStartRange(DeviceImage.RANGE_START_BLOCKS);
    }

    /**
     * VL53LX_init_and_start_range(): write the configuration and start. {@code blocks} is ST's config
     * level — all of them to start, general_config onwards to let the next frame run.
     */
    public void initAndStartRange(List<String> blocks) {
        RegBlock dynamic = img.dynamicConfig();
        RegBlock system = img.systemControl();

        measurementMode = BareRegs.DEVICE_MEASUREMENT_MODE_BACK_TO_BACK;
        system.set("system__mode_start",
                (system.get("system__mode_start") & BareRegs.DEVICE_MEASUREMENT_MODE_STOP_MASK) | measurementMode);
        system.set("system__interrupt_clear", BareRegs.CLEAR_RANGE_INT);

        long gphId = state.cfgGphId;
        dynamic.set("system__grouped_parameter_hold_0", gphId | 0x01);
        dynamic.set("system__grouped_parameter_hold_1", gphId | 0x01);
        dynamic.set("system__grouped_parameter_hold", gphId);

        img.pushRange(blocks);

        state.updateRd(system.get("system__mode_start"), dynamic.get("system__grouped_parameter_hold"));
        state.updateCfg(system.get("system__mode_start"));
    }

    /**
     * VL53LX_clear_interrupt_and_enable_next_range(): in histogram mode the interrupt is cleared by
     * rewriting general_config onwards.
     */
    public void clearInterruptAndStartNextRange() {
        initAndStartRange(List.of("general_config", "timing_config", "dynamic_config", "system_control"));
    }

    public void stopRange() {
        RegBlock system = img.systemControl();
        system.set("system__mode_start", (system.get("system__mode_start") & BareRegs.DEVICE_MEASUREMENT_MODE_STOP_MASK)
                | BareRegs.DEVICE_MEASUREMENT_MODE_ABORT);
        img.push("system_control");
        system.set("system__mode_start", system.get("system__mode_start") & BareRegs.DEVICE_MEASUREMENT_MODE_STOP_MASK);
        state.reset();
    }

    /**
     * VL53LX_get_histogram_bin_data(): one frame, 83 bytes, decoded into bins plus the metadata the
     * post-processing needs. {@code raw} is bytes that arrived some other way (a stream report);
     * null reads them here.
     */
    public HistogramBinData getHistogramBinData(byte[] raw) {
        if (raw == null) {
            raw = p.readMulti(BareRegs.HISTOGRAM_BIN_DATA_I2C_INDEX, BareRegs.HISTOGRAM_BIN_DATA_I2C_SIZE_BYTES);
        }
        if (raw.length < BareRegs.HISTOGRAM_BIN_DATA_I2C_SIZE_BYTES) {
            throw new IllegalArgumentException("histogram frame is " + raw.length + " bytes, need "
                    + BareRegs.HISTOGRAM_BIN_DATA_I2C_SIZE_BYTES);
        }
        int[] buf = new int[raw.length];
        for (int i = 0; i < raw.length; i++) {
            buf[i] = raw[i] & 0xFF;
        }
        final int off = BareRegs.HISTOGRAM_BIN_DATA_I2C_INDEX;

        HistogramBinData d = new HistogramBinData();
        d.resultInterruptStatus = buf[0];
        d.resultRangeStatus = buf[1];
        d.resultReportStatus = buf[2];
        d.resultStreamCount = buf[3];
        d.resultDssActualEffectiveSpads = ((long) buf[4] << 8) | buf[5];
        d.phasecalResultReferencePhase = ((long) buf[BareRegs.PHASECAL_RESULT_REFERENCE_PHASE - off] << 8)
                | buf[BareRegs.PHASECAL_RESULT_REFERENCE_PHASE - off + 1];
        d.phasecalResultVcselStart = buf[BareRegs.PHASECAL_RESULT_VCSEL_START - off];

        // Bin 23 does not fit in its three bytes: its low byte comes in a separate MSB/LSB pair.
        buf[BareRegs.RESULT_HISTOGRAM_BIN_23_0 - off] = ((buf[BareRegs.RESULT_HISTOGRAM_BIN_23_0_MSB - off] << 2)
                + buf[BareRegs.RESULT_HISTOGRAM_BIN_23_0_LSB - off]) & 0xFF;
        int b0 = BareRegs.RESULT_HISTOGRAM_BIN_0_2 - off;
        for (int i = 0; i < BareRegs.HISTOGRAM_BUFFER_SIZE; i++) {
            d.binData[i] = ((long) buf[b0 + 3 * i] << 16) | (buf[b0 + 3 * i + 1] << 8) | buf[b0 + 3 * i + 2];
        }

        RegBlock general = img.generalConfig();
        RegBlock stat = img.staticConfig();
        RegBlock timing = img.timingConfig();

        d.calConfigVcselStart = general.get("cal_config__vcsel_start");
        d.vcselWidth = (general.get("global_config__vcsel_width") << 4) + stat.get("ana_config__vcsel_pulse_width_offset");
        d.fastOscFrequency = img.staticNvm().get("osc_measured__fast_osc__frequency");
        d.roiConfigUserRoiCentreSpad = img.dynamicConfig().get("roi_config__user_roi_centre_spad");
        d.roiConfigUserRoiRequestedGlobalXySize = img.dynamicConfig().get("roi_config__user_roi_requested_global_xy_size");

        d.zoneId = 0;
        d.binSeq = histCfg.binSequence(state.rdStreamCount, ambientEventsSum);

        long encoded;
        if (state.rdTimingStatus == 0) {
            encoded = (timing.get("range_config__timeout_macrop_a_hi") << 8) + timing.get("range_config__timeout_macrop_a_lo");
            d.vcselPeriod = timing.get("range_config__vcsel_period_a");
        } else {
            encoded = (timing.get("range_config__timeout_macrop_b_hi") << 8) + timing.get("range_config__timeout_macrop_b_lo");
            d.vcselPeriod = timing.get("range_config__vcsel_period_b");
        }

        d.numberOfAmbientBins = 4L * Arrays.stream(d.binSeq).filter(s -> (s & 0x07) == 0x07).count();
        d.totalPeriodsElapsed = BareMath.decodeTimeout(encoded);
        d.peakDurationUs = BareMath.durationMaths(BareMath.calcPllPeriodUs(d.fastOscFrequency), d.vcselWidth,
                BareRegs.RANGING_WINDOW_VCSEL_PERIODS, d.totalPeriodsElapsed + 1);
        d.woiDurationUs = 0;

        // VL53LX_hist_calc_zero_distance_phase
        long period = 2048 * BareMath.decodeVcselPeriod(d.vcselPeriod);
        long phase = period + d.phasecalResultReferencePhase + 2048 * d.phasecalResultVcselStart
                - 2048 * d.calConfigVcselStart;
        d.zeroDistancePhase = period != 0 ? Hist.pyMod(phase, period) : 0;

        // VL53LX_hist_estimate_ambient_from_ambient_bins
        if (d.numberOfAmbientBins > 0) {
            d.numberOfAmbientSamples = d.numberOfAmbientBins;
            long sum = 0;
            for (int i = 0; i < (int) d.numberOfAmbientBins; i++) {
                sum += d.binData[i];
            }
            d.ambientEventsSum = sum;
            d.ambientPerBin = (d.ambientEventsSum + d.numberOfAmbientBins / 2) / d.numberOfAmbientBins;
        }
        ambientEventsSum = d.ambientEventsSum;
        return d;
    }

    /** {@link #getHistogramBinData(byte[])} reading the frame off the sensor. */
    public HistogramBinData getHistogramBinData() {
        return getHistogramBinData(null);
    }
}
