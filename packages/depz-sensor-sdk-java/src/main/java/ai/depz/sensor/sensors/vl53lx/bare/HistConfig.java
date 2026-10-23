package ai.depz.sensor.sensors.vl53lx.bare;

/**
 * {@code VL53LX_histogram_config_t}. It has no registers of its own: the bin sequence is smuggled
 * into static_config and timing_config fields that mean something else in lite mode
 * ({@link #copyToStaticCfg}).
 */
public final class HistConfig {
    public long lowAmbEvenBin01;
    public long lowAmbEvenBin23;
    public long lowAmbEvenBin45;
    public long lowAmbOddBin01;
    public long lowAmbOddBin23;
    public long lowAmbOddBin45;
    public long midAmbEvenBin01;
    public long midAmbEvenBin23;
    public long midAmbEvenBin45;
    public long midAmbOddBin01;
    public long midAmbOddBin2;
    public long midAmbOddBin34;
    public long midAmbOddBin5;
    public long userBinOffset;
    public long highAmbEvenBin01;
    public long highAmbEvenBin23;
    public long highAmbEvenBin45;
    public long highAmbOddBin01;
    public long highAmbOddBin23;
    public long highAmbOddBin45;
    public long ambThreshLow;
    public long ambThreshHigh;
    public long spadArraySelection;

    /**
     * VL53LX_init_histogram_config_structure(): six even and six odd bin codes, packed two to a
     * byte, repeated across the three ambient levels.
     */
    public void setBinSequence(int[] even, int[] odd) {
        lowAmbEvenBin01 = (even[1] << 4) + even[0];
        lowAmbEvenBin23 = (even[3] << 4) + even[2];
        lowAmbEvenBin45 = (even[5] << 4) + even[4];
        lowAmbOddBin01 = (odd[1] << 4) + odd[0];
        lowAmbOddBin23 = (odd[3] << 4) + odd[2];
        lowAmbOddBin45 = (odd[5] << 4) + odd[4];

        midAmbEvenBin01 = lowAmbEvenBin01;
        midAmbEvenBin23 = lowAmbEvenBin23;
        midAmbEvenBin45 = lowAmbEvenBin45;
        midAmbOddBin01 = lowAmbOddBin01;
        midAmbOddBin2 = odd[2];
        midAmbOddBin34 = (odd[4] << 4) + odd[3];
        midAmbOddBin5 = odd[5];
        userBinOffset = 0x00;

        highAmbEvenBin01 = lowAmbEvenBin01;
        highAmbEvenBin23 = lowAmbEvenBin23;
        highAmbEvenBin45 = lowAmbEvenBin45;
        highAmbOddBin01 = lowAmbOddBin01;
        highAmbOddBin23 = lowAmbOddBin23;
        highAmbOddBin45 = lowAmbOddBin45;

        ambThreshLow = 0xFFFF;
        ambThreshHigh = 0xFFFF;
        spadArraySelection = 0x00;
    }

    /** VL53LX_copy_hist_cfg_to_static_cfg(): lite-mode fields the firmware reads as the bin sequence. */
    public void copyToStaticCfg(RegBlock stat, RegBlock timing, RegBlock dynamic) {
        stat.set("sigma_estimator__effective_pulse_width_ns", highAmbEvenBin01);
        stat.set("sigma_estimator__effective_ambient_width_ns", highAmbEvenBin23);
        stat.set("sigma_estimator__sigma_ref_mm", highAmbEvenBin45);
        stat.set("algo__crosstalk_compensation_valid_height_mm", highAmbOddBin01);
        stat.set("spare_host_config__static_config_spare_0", highAmbOddBin23);
        stat.set("spare_host_config__static_config_spare_1", highAmbOddBin45);

        stat.set("algo__range_ignore_threshold_mcps", (midAmbEvenBin01 << 8) + midAmbEvenBin23);
        stat.set("algo__range_ignore_valid_height_mm", midAmbEvenBin45);
        stat.set("algo__range_min_clip", midAmbOddBin01);
        stat.set("algo__consistency_check__tolerance", midAmbOddBin2);
        stat.set("spare_host_config__static_config_spare_2", midAmbOddBin34);
        stat.set("sd_config__reset_stages_msb", midAmbOddBin5);
        stat.set("sd_config__reset_stages_lsb", userBinOffset);

        timing.set("range_config__sigma_thresh", (lowAmbEvenBin01 << 8) + lowAmbEvenBin23);
        timing.set("range_config__min_count_rate_rtn_limit_mcps", (lowAmbEvenBin45 << 8) + lowAmbOddBin01);
        timing.set("range_config__valid_phase_low", lowAmbOddBin23);
        timing.set("range_config__valid_phase_high", lowAmbOddBin45);

        dynamic.set("system__thresh_high", ambThreshLow);
        dynamic.set("system__thresh_low", ambThreshHigh);
        dynamic.set("system__enable_xtalk_per_quadrant", spadArraySelection);
    }

    /**
     * VL53LX_hist_get_bin_sequence_config(): which sequence the device used for this frame, even or
     * odd by the stream count.
     */
    public int[] binSequence(long streamCount, long ambientEventsSum) {
        String level = "mid";
        if (ambientEventsSum > 1024 * ambThreshHigh) {
            level = "high";
        }
        if (ambientEventsSum < 1024 * ambThreshLow) {
            level = "low";
        }

        long[] packed;
        if ((streamCount & 0x01) == 0) {
            packed = switch (level) {
                case "low" -> new long[] {lowAmbEvenBin01, lowAmbEvenBin23, lowAmbEvenBin45};
                case "high" -> new long[] {highAmbEvenBin01, highAmbEvenBin23, highAmbEvenBin45};
                default -> new long[] {midAmbEvenBin01, midAmbEvenBin23, midAmbEvenBin45};
            };
        } else if (level.equals("mid")) {
            // The mid odd sequence is the one packed differently.
            return new int[] {
                (int) (midAmbOddBin01 & 0x0F), (int) (midAmbOddBin01 >> 4), (int) midAmbOddBin2,
                (int) (midAmbOddBin34 >> 4), (int) (midAmbOddBin34 & 0x0F), (int) (midAmbOddBin5 & 0x0F),
            };
        } else {
            packed = level.equals("low")
                    ? new long[] {lowAmbOddBin01, lowAmbOddBin23, lowAmbOddBin45}
                    : new long[] {highAmbOddBin01, highAmbOddBin23, highAmbOddBin45};
        }
        int[] seq = new int[6];
        for (int i = 0; i < 3; i++) {
            seq[2 * i] = (int) (packed[i] & 0x0F);
            seq[2 * i + 1] = (int) (packed[i] >> 4);
        }
        return seq;
    }
}
