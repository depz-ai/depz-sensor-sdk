/* vl53lx_bare.h — ST's VL53LX BareDriver on the host (VL53L1CX / L1CB /
 * L3CX / L4CD / L4CX in histogram mode), ported from the Python SDK's
 * vl53lx/uld/bare/: the core (preset modes, timing, the ranging cycle, the
 * histogram frame, the NVM) in vl53lx_bare.c, the post-processing (24 bins
 * into targets, the frame-to-frame checks) in vl53lx_bare_hist.c. Private. */
#ifndef DEPZ_VL53LX_BARE_H
#define DEPZ_VL53LX_BARE_H

#include "vl53lx_bare_tables.h"
#include "vl53lx_internal.h"

#define VLXB_BUFFER_SIZE          24  /* HISTOGRAM_BUFFER_SIZE */
#define VLXB_MAX_BIN_SEQ_LENGTH   6
#define VLXB_MAX_BIN_SEQ_CODE     15
#define VLXB_MAX_PULSES           8
#define VLXB_MAX_RANGE_RESULTS    4
#define VLXB_RANGING_WINDOW_VCSEL_PERIODS 2048
#define VLXB_RANGESTATUS_NONE     255

/* VL53LX_histogram_bin_data_t (core.py HistogramBinData). */
typedef struct {
    int64_t interrupt_status, range_status, report_status, stream_count;
    int64_t dss_actual_effective_spads;
    int64_t reference_phase, vcsel_start;
    int64_t bin_data[VLXB_BUFFER_SIZE];
    int64_t zone_id, first_bin, number_of_bins, bins_in_data;
    int64_t cal_config_vcsel_start, vcsel_width, fast_osc_frequency, vcsel_period;
    int64_t bin_seq[VLXB_MAX_BIN_SEQ_LENGTH], bin_rep[VLXB_MAX_BIN_SEQ_LENGTH];
    int64_t min_bin_value, max_bin_value;
    int64_t number_of_ambient_bins, number_of_ambient_samples;
    int64_t ambient_events_sum, ambient_per_bin;
    int64_t total_periods_elapsed, peak_duration_us, woi_duration_us;
    int64_t zero_distance_phase;
    int64_t roi_centre_spad, roi_xy_size;
} vlxb_bins;

/* VL53LX_hist_post_process_config_t (core.py HistPostProcessConfig). */
typedef struct {
    int64_t hist_algo_select, hist_target_order, filter_woi0, filter_woi1, hist_amb_est_method;
    int64_t ambient_thresh_sigma0, ambient_thresh_sigma1, ambient_thresh_events_scaler;
    int64_t min_ambient_thresh_events, noise_threshold, signal_total_events_limit;
    int64_t sigma_estimator_sigma_ref_mm, sigma_thresh, range_offset_mm, gain_factor;
    int64_t valid_phase_low, valid_phase_high;
    int64_t phase_tolerance, event_sigma, event_min_spad_count, min_max_tolerance;
    int64_t crosstalk_compensation_enable;
    int64_t xtalk_plane_offset_kcps, xtalk_x_plane_gradient_kcps, xtalk_y_plane_gradient_kcps;
} vlxb_hpp;

/* VL53LX_range_data_t (hist.py RangeData), the fields the port reads. */
typedef struct {
    int64_t range_id, start_bin, first_bin, peak_bin, last_bin, end_bin, width_bins, window_bins;
    int64_t vcsel_width, fast_osc_frequency, zero_distance_phase, spads, total_periods_elapsed;
    int64_t peak_duration_us, woi_duration_us;
    int64_t ambient_events, total_events, signal_events;
    int64_t peak_signal_count_rate_mcps, avg_signal_count_rate_mcps, ambient_count_rate_mcps;
    int64_t total_rate_per_spad_mcps, signal_events_per_spad_kcps;
    int64_t sigma, phase_start, phase_mean, phase_end;
    int64_t min_range_mm, median_range_mm, max_range_mm;
    int64_t range_status;
} vlxb_range;

/* VL53LX_range_results_t, single zone. */
typedef struct {
    int64_t stream_count;
    size_t n_targets;
    vlxb_range targets[VLXB_MAX_RANGE_RESULTS];
} vlxb_results;

typedef enum {
    VLXB_STATE_NONE = 0,       /* FrameHistory before its first frame */
    VLXB_STATE_SW_STANDBY,
    VLXB_STATE_WAIT_GPH_SYNC,
    VLXB_STATE_OUTPUT_DATA,
    VLXB_STATE_DSS_AUTO
} vlxb_state_id;

/* hist.py FrameHistory: the previous frame, as the consistency checks read it. */
typedef struct {
    vlxb_state_id rd_device_state;
    int64_t total_periods_elapsed, spads;
    size_t n_targets;
    struct { int64_t ambient_events, total_events, phase_mean, range_status; } targets[VLXB_MAX_RANGE_RESULTS];
} vlxb_history;

/* hist.py: VL53LX_hist_process_data (crosstalk off) and the history. */
void vlxb_process_data(const vlxb_bins *in, const vlxb_hpp *hpp, vlxb_results *out);
void vlxb_history_reset(vlxb_history *h);
void vlxb_history_apply(vlxb_history *h, vlxb_results *results, const vlxb_bins *bins,
                        vlxb_state_id rd_device_state, const vlxb_hpp *hpp);
/* ConvertStatusHisto() and the status names. */
int vlxb_convert_status(int64_t device_error);
const char *vlxb_status_text(int status);

/* Shared arithmetic (core.py, used by hist.py too). */
int64_t vlxb_calc_pll_period_us(int64_t fast_osc_frequency);
int64_t vlxb_decode_vcsel_period(int64_t reg);
int64_t vlxb_duration_maths(int64_t pll_period_us, int64_t vcsel_parm_pclks, int64_t window_vclks,
                            int64_t elapsed_mclks);

#endif /* DEPZ_VL53LX_BARE_H */
