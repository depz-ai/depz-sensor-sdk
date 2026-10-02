"""
uld/vl53lx/core.py — preset modes, timing maths and the ranging cycle.

This is the part of the BareDriver that turns a set of tuning numbers into a
device image and back: `VL53LX_read_p2p_data()` (the factory data), the
`VL53LX_preset_mode_histogram_*()` builders, `VL53LX_set_timeouts_us()`,
`VL53LX_init_and_start_range()` and `VL53LX_get_histogram_bin_data()`.

Two things about it are worth knowing before reading:

**The device is configured in one shot.** Nothing here writes a register on its
own. Every preset builder edits the host-side image, and the whole span
0x0001..0x0087 goes out as one transfer when ranging starts — which is what the
C driver does, and what keeps the USB round trips down to one per start.

**Clearing the interrupt is not a register write.** In histogram mode ST clears
it by running `init_and_start_range()` again at GENERAL_ONWARDS level, that is
by rewriting 0x0044..0x0087. It is not two bytes, so it does not fit the
bridge's `START_STREAM` clear-step list; this driver therefore runs polled, and
INT streaming is a separate question (see the plan, E8.7).

Reference C driver: ../../../temp/STSW-IMG033_L3/…/VL53L3CX_BareDriver/
"""

from depz_sensor_sdk.vl53lx.uld.bare.image import DeviceImage, RANGE_START_BLOCKS
from depz_sensor_sdk.vl53lx.uld.bare.nvm import NvmReader
from depz_sensor_sdk.vl53lx.uld.bare.tuning import TUNING, DEFAULTS

# ── register map (vl53lx_register_map.h, vl53lx_hist_map.h) ──────────────────
POWER_MANAGEMENT__GO1_POWER_FORCE = 0x0083
FIRMWARE__ENABLE                  = 0x0085
RESULT__OSC_CALIBRATE_VAL         = 0x00DE
PATCH__CTRL                       = 0x0470
PATCH__JMP_ENABLES                = 0x0472
PATCH__DATA_ENABLES               = 0x0474
PATCH__OFFSET_0                   = 0x0476
PATCH__ADDRESS_0                  = 0x0496

HISTOGRAM_BIN_DATA_I2C_INDEX      = 0x0088       # result__interrupt_status
RESULT__HISTOGRAM_BIN_0_2         = 0x008E
RESULT__HISTOGRAM_BIN_23_0        = 0x00D5
PHASECAL_RESULT__REFERENCE_PHASE  = 0x00D6
PHASECAL_RESULT__VCSEL_START      = 0x00D8
RESULT__HISTOGRAM_BIN_23_0_MSB    = 0x00D9
RESULT__HISTOGRAM_BIN_23_0_LSB    = 0x00DA
HISTOGRAM_BIN_DATA_I2C_SIZE_BYTES = (RESULT__HISTOGRAM_BIN_23_0_LSB
                                     - HISTOGRAM_BIN_DATA_I2C_INDEX + 1)

# ── register settings (vl53lx_register_settings.h, vl53lx_ll_device.h) ───────
DEVICEMEASUREMENTMODE_BACKTOBACK = 0x20
DEVICEMEASUREMENTMODE_ABORT      = 0x80
DEVICEMEASUREMENTMODE_STOP_MASK  = 0x0F
DEVICEMEASUREMENTMODE_MODE_MASK  = 0xF0
DEVICESCHEDULERMODE_STREAMING    = 0x01
DEVICESCHEDULERMODE_HISTOGRAM    = 0x02
DEVICEREADOUTMODE_SINGLE_SD      = 0x00 << 2
DEVICEREADOUTMODE_DUAL_SD        = 0x01 << 2
GROUPEDPARAMETERHOLD_ID_MASK     = 0x02
INTERRUPT_CONFIG_NEW_SAMPLE_READY = 0x20
CLEAR_RANGE_INT                  = 0x01
RANGE_STATUS__RANGE_STATUS_MASK  = 0x1F

DEVICEINTERRUPTPOLARITY_ACTIVE_LOW = 0x10
DEVICEGPIOMODE_OUTPUT_RANGE_AND_ERROR_INTERRUPTS = 0x01
DEVICEDSSMODE__TARGET_RATE       = 1

SEQUENCE_VHV_EN      = 0x01
SEQUENCE_PHASECAL_EN = 0x02
SEQUENCE_DSS1_EN     = 0x08
SEQUENCE_DSS2_EN     = 0x10
SEQUENCE_MM1_EN      = 0x20
SEQUENCE_MM2_EN      = 0x40
SEQUENCE_RANGE_EN    = 0x80

SPAD_ARRAY_WIDTH  = 16
SPAD_ARRAY_HEIGHT = 16
RTN_SPAD_APERTURE_TRANSMISSION = 0x0038
RTN_SPAD_UNITY_TRANSMISSION    = 0x0100

AMBIENT_WINDOW_VCSEL_PERIODS = 256
RANGING_WINDOW_VCSEL_PERIODS = 2048
MACRO_PERIOD_VCSEL_PERIODS   = (AMBIENT_WINDOW_VCSEL_PERIODS
                                + RANGING_WINDOW_VCSEL_PERIODS)
HISTOGRAM_BUFFER_SIZE = 24

# The three preset modes VL53L3CX/VL53L4CX expose, by the distance mode that
# picks them (VL53LX_ComputeDevicePresetMode).
DISTANCE_MODES = ('short', 'medium', 'long')

# VL53LX_SetMeasurementTimingBudgetMicroSeconds: the budget is not the range
# timeout — a fixed guard comes off the top and the rest is split six ways.
TIMING_GUARD_US = 1700
TIMING_DIVISOR  = 6
FDA_MAX_TIMING_BUDGET_US = 550000
# The L4CX BareDriver (STSW-IMG029) narrows both for an L4 die: a lower budget
# ceiling, and no short mode at all (see `BareDriver.is_l4()`).
L4_FDA_MAX_TIMING_BUDGET_US = 200000
L4_DISTANCE_MODES = ('medium', 'long')


# ── SPAD geometry (vl53lx_core.c, vl53lx_core_support.c) ─────────────────────
def encode_row_col(row: int, col: int) -> int:
    if row > 7:
        return (128 + (col << 3) + (15 - row)) & 0xFF
    return (((15 - col) << 3) + row) & 0xFF


def decode_row_col(spad_number: int) -> tuple:
    """-> (row, col)"""
    if spad_number > 127:
        return 8 + ((255 - spad_number) & 0x07), (spad_number - 128) >> 3
    return spad_number & 0x07, (127 - spad_number) >> 3


def decode_zone_size(encoded_xy_size: int) -> tuple:
    """-> (width, height)"""
    return encoded_xy_size & 0x0F, encoded_xy_size >> 4


def encode_zone_size(width: int, height: int) -> int:
    return ((height << 4) + width) & 0xFF


def decode_zone_limits(encoded_xy_centre: int, encoded_xy_size: int) -> tuple:
    """-> (x_ll, y_ll, x_ur, y_ur), clipped to the SPAD array."""
    y_centre, x_centre = decode_row_col(encoded_xy_centre)
    width, height = decode_zone_size(encoded_xy_size)

    x_ll = max(0, x_centre - (width + 1) // 2)
    x_ur = min(SPAD_ARRAY_WIDTH - 1, x_ll + width)
    y_ll = max(0, y_centre - (height + 1) // 2)
    y_ur = min(SPAD_ARRAY_HEIGHT - 1, y_ll + height)
    return x_ll, y_ll, x_ur, y_ur


def is_aperture_location(row: int, col: int) -> bool:
    return (row % 4, col % 4) in ((0, 2), (2, 0))


def calc_mm_effective_spads(encoded_mm_roi_centre, encoded_mm_roi_size,
                            encoded_zone_centre, encoded_zone_size,
                            good_spads, aperture_attenuation) -> tuple:
    """-> (mm inner, mm outer) effective SPADs, in 1/256 of a SPAD.

    Needed because our boards ship without the FMT offset calibration: with it
    missing the driver substitutes nominal MM1/MM2 peak rates and has to work
    out the SPAD counts they apply to itself.
    """
    mm_x_ll, mm_y_ll, mm_x_ur, mm_y_ur = decode_zone_limits(
        encoded_mm_roi_centre, encoded_mm_roi_size)
    zone_x_ll, zone_y_ll, zone_x_ur, zone_y_ur = decode_zone_limits(
        encoded_zone_centre, encoded_zone_size)

    inner = outer = 0
    for y in range(zone_y_ll, zone_y_ur + 1):
        for x in range(zone_x_ll, zone_x_ur + 1):
            spad = encode_row_col(y, x)
            if not good_spads[spad >> 3] & (1 << (spad & 0x07)):
                continue
            attenuation = (aperture_attenuation if is_aperture_location(y, x)
                           else RTN_SPAD_UNITY_TRANSMISSION)
            if mm_x_ll <= x <= mm_x_ur and mm_y_ll <= y <= mm_y_ur:
                inner += attenuation
            else:
                outer += attenuation
    return inner, outer


def combine_mm1_mm2_offsets(mm1_offset_mm, mm2_offset_mm,
                            encoded_mm_roi_centre, encoded_mm_roi_size,
                            encoded_zone_centre, encoded_zone_size,
                            cal_data, good_spads, aperture_attenuation) -> int:
    """`VL53LX_hist_combine_mm1_mm2_offsets()` -> `range_offset_mm`, in quarter
    millimetres (`range_maths()` adds it before the /4 rounding step).

    The two NVM offsets are calibrated for the MM1 (inner) and MM2 (outer)
    regions of the SPAD array; which of them the current ROI actually sees
    decides how they are weighted. The weights are the nominal MM1/MM2 peak
    rates scaled by the share of each region the ROI covers.
    """
    max_inner, max_outer = calc_mm_effective_spads(
        encoded_mm_roi_centre, encoded_mm_roi_size, 0xC7, 0xFF,
        good_spads, aperture_attenuation)
    if max_inner == 0 or max_outer == 0:
        return 0

    inner, outer = calc_mm_effective_spads(
        encoded_mm_roi_centre, encoded_mm_roi_size,
        encoded_zone_centre, encoded_zone_size,
        good_spads, aperture_attenuation)

    mm1_rate = (cal_data.result__mm_inner_peak_signal_count_rtn_mcps
                * inner // max_inner)
    mm2_rate = (cal_data.result__mm_outer_peak_signal_count_rtn_mcps
                * outer // max_outer)

    total = mm1_rate + mm2_rate
    if total == 0:
        return 0
    # The offsets are signed, so the division has to truncate towards zero the
    # way C does, not floor the way Python would.
    num = (mm1_offset_mm * mm1_rate + mm2_offset_mm * mm2_rate) * 4
    return -(-num // total) if num < 0 else num // total


def rtn_good_spads(nvm_copy_data) -> bytes:
    """The 32 `global_config__spad_enables_rtn_*` bytes as one bitmap."""
    return bytes(getattr(nvm_copy_data, f'global_config__spad_enables_rtn_{i}')
                 for i in range(32))


# ── timing maths (vl53lx_core.c, vl53lx_core_support.c) ──────────────────────
def calc_pll_period_us(fast_osc_frequency: int) -> int:
    return (1 << 30) // fast_osc_frequency if fast_osc_frequency > 0 else 0


def decode_vcsel_period(vcsel_period_reg: int) -> int:
    return (vcsel_period_reg + 1) << 1


def calc_macro_period_us(fast_osc_frequency: int, vcsel_period_reg: int) -> int:
    macro = MACRO_PERIOD_VCSEL_PERIODS * calc_pll_period_us(fast_osc_frequency)
    macro >>= 6
    macro *= decode_vcsel_period(vcsel_period_reg)
    return macro >> 6


def calc_timeout_mclks(timeout_us: int, macro_period_us: int) -> int:
    if macro_period_us == 0:
        return 0
    return ((timeout_us << 12) + (macro_period_us >> 1)) // macro_period_us


def encode_timeout(timeout_mclks: int) -> int:
    if timeout_mclks <= 0:
        return 0
    ls_byte = timeout_mclks - 1
    ms_byte = 0
    while ls_byte & 0xFFFFFF00:
        ls_byte >>= 1
        ms_byte += 1
    return ((ms_byte << 8) + (ls_byte & 0xFF)) & 0xFFFF


def decode_timeout(encoded: int) -> int:
    return ((encoded & 0x00FF) << ((encoded & 0xFF00) >> 8)) + 1


def calc_encoded_timeout(timeout_us: int, macro_period_us: int) -> int:
    return encode_timeout(calc_timeout_mclks(timeout_us, macro_period_us))


def duration_maths(pll_period_us, vcsel_parm_pclks, window_vclks,
                   elapsed_mclks) -> int:
    duration = (window_vclks * pll_period_us) >> 12
    duration *= (elapsed_mclks * vcsel_parm_pclks) >> 4
    return min(duration >> 12, 0xFFFFFFFF)


def calc_timeout_register_values(phasecal_us, mm_us, range_us,
                                 fast_osc_frequency, gen_cfg, tim_cfg):
    """Fill the six timeout registers, for VCSEL period A and then B."""
    if fast_osc_frequency == 0:
        raise ZeroDivisionError('osc_measured__fast_osc__frequency is 0')

    macro = calc_macro_period_us(fast_osc_frequency,
                                 tim_cfg.range_config__vcsel_period_a)
    gen_cfg.phasecal_config__timeout_macrop = min(
        0xFF, calc_timeout_mclks(phasecal_us, macro))

    encoded = calc_encoded_timeout(mm_us, macro)
    tim_cfg.mm_config__timeout_macrop_a_hi = encoded >> 8
    tim_cfg.mm_config__timeout_macrop_a_lo = encoded & 0xFF
    encoded = calc_encoded_timeout(range_us, macro)
    tim_cfg.range_config__timeout_macrop_a_hi = encoded >> 8
    tim_cfg.range_config__timeout_macrop_a_lo = encoded & 0xFF

    macro = calc_macro_period_us(fast_osc_frequency,
                                 tim_cfg.range_config__vcsel_period_b)
    encoded = calc_encoded_timeout(mm_us, macro)
    tim_cfg.mm_config__timeout_macrop_b_hi = encoded >> 8
    tim_cfg.mm_config__timeout_macrop_b_lo = encoded & 0xFF
    encoded = calc_encoded_timeout(range_us, macro)
    tim_cfg.range_config__timeout_macrop_b_hi = encoded >> 8
    tim_cfg.range_config__timeout_macrop_b_lo = encoded & 0xFF


# ── the histogram config struct (not a register block) ───────────────────────
HIST_CFG_FIELDS = (
    'low_amb_even_bin_0_1', 'low_amb_even_bin_2_3', 'low_amb_even_bin_4_5',
    'low_amb_odd_bin_0_1', 'low_amb_odd_bin_2_3', 'low_amb_odd_bin_4_5',
    'mid_amb_even_bin_0_1', 'mid_amb_even_bin_2_3', 'mid_amb_even_bin_4_5',
    'mid_amb_odd_bin_0_1', 'mid_amb_odd_bin_2', 'mid_amb_odd_bin_3_4',
    'mid_amb_odd_bin_5', 'user_bin_offset',
    'high_amb_even_bin_0_1', 'high_amb_even_bin_2_3', 'high_amb_even_bin_4_5',
    'high_amb_odd_bin_0_1', 'high_amb_odd_bin_2_3', 'high_amb_odd_bin_4_5',
    'amb_thresh_low', 'amb_thresh_high', 'spad_array_selection')


class HistConfig:
    """`VL53LX_histogram_config_t`. It has no registers of its own: the bin
    sequence is smuggled into static_config and timing_config fields that mean
    something else in lite mode, which `copy_to_static_cfg()` below does."""

    __slots__ = HIST_CFG_FIELDS

    def __init__(self):
        for name in HIST_CFG_FIELDS:
            setattr(self, name, 0)

    def set_bin_sequence(self, even, odd):
        """VL53LX_init_histogram_config_structure(): six even and six odd bin
        codes, packed two to a byte, then repeated across the three ambient
        levels (the device picks a level, we give it the same sequence)."""
        self.low_amb_even_bin_0_1 = (even[1] << 4) + even[0]
        self.low_amb_even_bin_2_3 = (even[3] << 4) + even[2]
        self.low_amb_even_bin_4_5 = (even[5] << 4) + even[4]
        self.low_amb_odd_bin_0_1  = (odd[1] << 4) + odd[0]
        self.low_amb_odd_bin_2_3  = (odd[3] << 4) + odd[2]
        self.low_amb_odd_bin_4_5  = (odd[5] << 4) + odd[4]

        self.mid_amb_even_bin_0_1 = self.low_amb_even_bin_0_1
        self.mid_amb_even_bin_2_3 = self.low_amb_even_bin_2_3
        self.mid_amb_even_bin_4_5 = self.low_amb_even_bin_4_5
        self.mid_amb_odd_bin_0_1  = self.low_amb_odd_bin_0_1
        self.mid_amb_odd_bin_2    = odd[2]
        self.mid_amb_odd_bin_3_4  = (odd[4] << 4) + odd[3]
        self.mid_amb_odd_bin_5    = odd[5]
        self.user_bin_offset      = 0x00

        self.high_amb_even_bin_0_1 = self.low_amb_even_bin_0_1
        self.high_amb_even_bin_2_3 = self.low_amb_even_bin_2_3
        self.high_amb_even_bin_4_5 = self.low_amb_even_bin_4_5
        self.high_amb_odd_bin_0_1  = self.low_amb_odd_bin_0_1
        self.high_amb_odd_bin_2_3  = self.low_amb_odd_bin_2_3
        self.high_amb_odd_bin_4_5  = self.low_amb_odd_bin_4_5

        self.amb_thresh_low       = 0xFFFF
        self.amb_thresh_high      = 0xFFFF
        self.spad_array_selection = 0x00

    def copy_to_static_cfg(self, static, timing, dynamic):
        """VL53LX_copy_hist_cfg_to_static_cfg(). The field names on the left
        are lite-mode names; in histogram mode the firmware reads them as the
        bin sequence."""
        static.sigma_estimator__effective_pulse_width_ns = \
            self.high_amb_even_bin_0_1
        static.sigma_estimator__effective_ambient_width_ns = \
            self.high_amb_even_bin_2_3
        static.sigma_estimator__sigma_ref_mm = self.high_amb_even_bin_4_5
        static.algo__crosstalk_compensation_valid_height_mm = \
            self.high_amb_odd_bin_0_1
        static.spare_host_config__static_config_spare_0 = \
            self.high_amb_odd_bin_2_3
        static.spare_host_config__static_config_spare_1 = \
            self.high_amb_odd_bin_4_5

        static.algo__range_ignore_threshold_mcps = \
            (self.mid_amb_even_bin_0_1 << 8) + self.mid_amb_even_bin_2_3
        static.algo__range_ignore_valid_height_mm = self.mid_amb_even_bin_4_5
        static.algo__range_min_clip = self.mid_amb_odd_bin_0_1
        static.algo__consistency_check__tolerance = self.mid_amb_odd_bin_2
        static.spare_host_config__static_config_spare_2 = \
            self.mid_amb_odd_bin_3_4
        static.sd_config__reset_stages_msb = self.mid_amb_odd_bin_5
        static.sd_config__reset_stages_lsb = self.user_bin_offset

        timing.range_config__sigma_thresh = \
            (self.low_amb_even_bin_0_1 << 8) + self.low_amb_even_bin_2_3
        timing.range_config__min_count_rate_rtn_limit_mcps = \
            (self.low_amb_even_bin_4_5 << 8) + self.low_amb_odd_bin_0_1
        timing.range_config__valid_phase_low = self.low_amb_odd_bin_2_3
        timing.range_config__valid_phase_high = self.low_amb_odd_bin_4_5

        dynamic.system__thresh_high = self.amb_thresh_low
        dynamic.system__thresh_low = self.amb_thresh_high
        dynamic.system__enable_xtalk_per_quadrant = self.spad_array_selection

    def bin_sequence(self, stream_count: int, ambient_events_sum: int) -> list:
        """VL53LX_hist_get_bin_sequence_config(): which of the three sequences
        the device used for this frame, even or odd according to the stream
        count. Ours are all the same, but the driver asks anyway."""
        level = 'mid'
        if ambient_events_sum > 1024 * self.amb_thresh_high:
            level = 'high'
        if ambient_events_sum < 1024 * self.amb_thresh_low:
            level = 'low'

        if stream_count & 0x01 == 0:
            packed = [getattr(self, f'{level}_amb_even_bin_0_1'),
                      getattr(self, f'{level}_amb_even_bin_2_3'),
                      getattr(self, f'{level}_amb_even_bin_4_5')]
        elif level == 'mid':
            # The mid odd sequence is the one packed differently.
            return [self.mid_amb_odd_bin_0_1 & 0x0F,
                    self.mid_amb_odd_bin_0_1 >> 4,
                    self.mid_amb_odd_bin_2,
                    self.mid_amb_odd_bin_3_4 >> 4,
                    self.mid_amb_odd_bin_3_4 & 0x0F,
                    self.mid_amb_odd_bin_5 & 0x0F]
        else:
            packed = [getattr(self, f'{level}_amb_odd_bin_0_1'),
                      getattr(self, f'{level}_amb_odd_bin_2_3'),
                      getattr(self, f'{level}_amb_odd_bin_4_5')]

        seq = []
        for byte in packed:
            seq += [byte & 0x0F, byte >> 4]
        return seq


class HistPostProcessConfig:
    """`VL53LX_hist_post_process_config_t`, as
    `VL53LX_init_hist_post_process_config_struct()` leaves it. E8.4 is what
    reads most of these; the preset modes set valid_phase_low/high here."""

    def __init__(self, xtalk_compensation_enable=0):
        d = DEFAULTS
        self.hist_algo_select       = d['TUNINGPARM_HIST_ALGO_SELECT_DEFAULT']
        self.hist_target_order      = d['TUNINGPARM_HIST_TARGET_ORDER_DEFAULT']
        self.filter_woi0            = d['TUNINGPARM_HIST_FILTER_WOI_0_DEFAULT']
        self.filter_woi1            = d['TUNINGPARM_HIST_FILTER_WOI_1_DEFAULT']
        self.hist_amb_est_method    = d['TUNINGPARM_HIST_AMB_EST_METHOD_DEFAULT']
        self.ambient_thresh_sigma0  = d['TUNINGPARM_HIST_AMB_THRESH_SIGMA_0_DEFAULT']
        self.ambient_thresh_sigma1  = d['TUNINGPARM_HIST_AMB_THRESH_SIGMA_1_DEFAULT']
        self.ambient_thresh_events_scaler = \
            d['TUNINGPARM_HIST_AMB_EVENTS_SCALER_DEFAULT']
        self.min_ambient_thresh_events = \
            d['TUNINGPARM_HIST_MIN_AMB_THRESH_EVENTS_DEFAULT']
        self.noise_threshold        = d['TUNINGPARM_HIST_NOISE_THRESHOLD_DEFAULT']
        self.signal_total_events_limit = \
            d['TUNINGPARM_HIST_SIGNAL_TOTAL_EVENTS_LIMIT_DEFAULT']
        self.sigma_estimator__sigma_ref_mm = \
            d['TUNINGPARM_HIST_SIGMA_EST_REF_MM_DEFAULT']
        self.sigma_thresh           = d['TUNINGPARM_HIST_SIGMA_THRESH_MM_DEFAULT']
        self.range_offset_mm        = 0
        self.gain_factor            = d['TUNINGPARM_HIST_GAIN_FACTOR_DEFAULT']
        self.valid_phase_low        = 0x08
        self.valid_phase_high       = 0x88
        self.algo__consistency_check__phase_tolerance = \
            d['TUNINGPARM_CONSISTENCY_HIST_PHASE_TOLERANCE_DEFAULT']
        self.algo__consistency_check__event_sigma = \
            d['TUNINGPARM_CONSISTENCY_HIST_EVENT_SIGMA_DEFAULT']
        self.algo__consistency_check__event_min_spad_count = \
            d['TUNINGPARM_CONSISTENCY_HIST_EVENT_SIGMA_MIN_SPAD_LIMIT_DEFAULT']
        self.algo__consistency_check__min_max_tolerance = \
            d['TUNINGPARM_CONSISTENCY_HIST_MIN_MAX_TOLERANCE_MM_DEFAULT']
        self.algo__crosstalk_compensation_enable = xtalk_compensation_enable
        self.algo__crosstalk_detect_min_valid_range_mm = \
            d['TUNINGPARM_XTALK_DETECT_MIN_VALID_RANGE_MM_DEFAULT']
        self.algo__crosstalk_detect_max_valid_range_mm = \
            d['TUNINGPARM_XTALK_DETECT_MAX_VALID_RANGE_MM_DEFAULT']
        self.algo__crosstalk_detect_max_valid_rate_kcps = \
            d['TUNINGPARM_XTALK_DETECT_MAX_VALID_RATE_KCPS_DEFAULT']
        self.algo__crosstalk_detect_max_sigma_mm = \
            d['TUNINGPARM_XTALK_DETECT_MAX_SIGMA_MM_DEFAULT']
        self.algo__crosstalk_detect_event_sigma = \
            d['TUNINGPARM_XTALK_DETECT_EVENT_SIGMA_DEFAULT']
        self.algo__crosstalk_detect_min_max_tolerance = \
            d['TUNINGPARM_XTALK_DETECT_MIN_MAX_TOLERANCE_DEFAULT']
        # Filled in by read_p2p_data() from the customer NVM block.
        self.algo__crosstalk_compensation_plane_offset_kcps = 0
        self.algo__crosstalk_compensation_x_plane_gradient_kcps = 0
        self.algo__crosstalk_compensation_y_plane_gradient_kcps = 0


# ── the histogram frame ──────────────────────────────────────────────────────
class HistogramBinData:
    """`VL53LX_histogram_bin_data_t`: 24 bins plus everything the
    post-processing needs to read them — the VCSEL period they were taken at,
    the phase of zero distance, the bin sequence and the ambient estimate."""

    def __init__(self):
        self.result__interrupt_status = 0
        self.result__range_status = 0
        self.result__report_status = 0
        self.result__stream_count = 0
        self.result__dss_actual_effective_spads = 0
        self.phasecal_result__reference_phase = 0
        self.phasecal_result__vcsel_start = 0
        self.bin_data = [0] * HISTOGRAM_BUFFER_SIZE
        self.zone_id = 0
        self.first_bin = 0                 # VL53LX_p_019
        self.number_of_bins = HISTOGRAM_BUFFER_SIZE     # VL53LX_p_020, buffer
        self.bins_in_data = HISTOGRAM_BUFFER_SIZE       # VL53LX_p_021, in use
        self.cal_config__vcsel_start = 0
        self.vcsel_width = 0
        self.fast_osc_frequency = 0        # VL53LX_p_015
        self.vcsel_period = 0              # VL53LX_p_005, the register value
        self.bin_seq = [0] * 6
        self.bin_rep = [0] * 6             # how often each code was sampled
        self.min_bin_value = 0
        self.max_bin_value = 0
        self.number_of_ambient_bins = 0
        self.number_of_ambient_samples = 0
        self.ambient_events_sum = 0
        self.ambient_per_bin = 0           # VL53LX_p_028
        self.total_periods_elapsed = 0
        self.peak_duration_us = 0
        self.woi_duration_us = 0
        self.zero_distance_phase = 0
        self.roi_config__user_roi_centre_spad = 0
        self.roi_config__user_roi_requested_global_xy_size = 0

    @property
    def range_status(self):
        return self.result__range_status & RANGE_STATUS__RANGE_STATUS_MASK

    def __repr__(self):
        return (f'<HistogramBinData stream {self.result__stream_count} '
                f'status {self.range_status} spads '
                f'{self.result__dss_actual_effective_spads} '
                f'bins {self.bin_data}>')


# ── the ll driver state machine ──────────────────────────────────────────────
# `VL53LX_update_ll_driver_{rd,cfg}_state()`, reduced to the single-zone case
# (active_zones == 0): with one zone the zone walk collapses and what is left
# is the GPH handshake and the timing-status flip. Both matter — the GPH id
# stamped into the dynamic config is what tells the device a configuration is
# new, and the timing status says whether this frame ran on VCSEL period A or
# B, which the histogram post-processing has to know.
DEVICESTATE_SW_STANDBY            = 'sw_standby'
DEVICESTATE_RANGING_WAIT_GPH_SYNC = 'wait_gph_sync'
DEVICESTATE_RANGING_OUTPUT_DATA   = 'output_data'
DEVICESTATE_RANGING_DSS_AUTO      = 'dss_auto'


class LLState:

    def __init__(self):
        self.reset()

    def reset(self):
        self.cfg_device_state = DEVICESTATE_SW_STANDBY
        self.cfg_stream_count = 0
        self.cfg_gph_id = GROUPEDPARAMETERHOLD_ID_MASK
        self.cfg_timing_status = 0
        self.rd_device_state = DEVICESTATE_SW_STANDBY
        self.rd_stream_count = 0
        self.rd_gph_id = GROUPEDPARAMETERHOLD_ID_MASK
        self.rd_timing_status = 0

    def update_rd(self, mode_start, grouped_parameter_hold):
        if mode_start & DEVICEMEASUREMENTMODE_MODE_MASK == 0:
            self.rd_device_state = DEVICESTATE_SW_STANDBY
            self.rd_stream_count = 0
            self.rd_gph_id = GROUPEDPARAMETERHOLD_ID_MASK
            self.rd_timing_status = 0
            return

        self.rd_stream_count = (0x80 if self.rd_stream_count == 0xFF
                                else self.rd_stream_count + 1)
        self.rd_gph_id ^= GROUPEDPARAMETERHOLD_ID_MASK

        if self.rd_device_state == DEVICESTATE_SW_STANDBY:
            self.rd_device_state = (
                DEVICESTATE_RANGING_WAIT_GPH_SYNC
                if grouped_parameter_hold & GROUPEDPARAMETERHOLD_ID_MASK
                else DEVICESTATE_RANGING_OUTPUT_DATA)
            self.rd_stream_count = 0
            self.rd_timing_status = 0
        elif self.rd_device_state == DEVICESTATE_RANGING_WAIT_GPH_SYNC:
            self.rd_stream_count = 0
            self.rd_device_state = DEVICESTATE_RANGING_OUTPUT_DATA
        elif self.rd_device_state == DEVICESTATE_RANGING_OUTPUT_DATA:
            self.rd_timing_status ^= 0x01
        else:
            self.reset()

    def update_cfg(self, mode_start):
        if mode_start & DEVICEMEASUREMENTMODE_MODE_MASK == 0:
            self.cfg_device_state = DEVICESTATE_SW_STANDBY
            self.cfg_stream_count = 0
            self.cfg_gph_id = GROUPEDPARAMETERHOLD_ID_MASK
            self.cfg_timing_status = 0
            return

        self.cfg_stream_count = (0x80 if self.cfg_stream_count == 0xFF
                                 else self.cfg_stream_count + 1)
        self.cfg_gph_id ^= GROUPEDPARAMETERHOLD_ID_MASK

        if self.cfg_device_state == DEVICESTATE_SW_STANDBY:
            self.cfg_timing_status ^= 0x01
            self.cfg_stream_count = 1
            self.cfg_device_state = DEVICESTATE_RANGING_DSS_AUTO
        elif self.cfg_device_state == DEVICESTATE_RANGING_DSS_AUTO:
            self.cfg_timing_status ^= 0x01
        else:
            self.reset()


# ── the driver ───────────────────────────────────────────────────────────────
class BareDriver:
    """The VL53LX BareDriver's ranging path, on top of `DeviceImage`.

    It owns what the C driver keeps in `VL53LX_LLDriverData_t` and nowhere
    else: the factory data that has no registers, the histogram config, the
    post-process config and the timeouts.
    """

    def __init__(self, platform):
        self.p = platform
        self.img = DeviceImage(platform)
        self.nvm = NvmReader(platform, self.img)
        self.tuning = dict(TUNING)

        self.hist_cfg = HistConfig()
        self.hpp = HistPostProcessConfig()

        # Factory data with no register of its own (read_p2p_data).
        self.rtn_good_spads = bytes(32)
        self.optical_centre = None
        self.cal_peak_rate_map = None
        self.add_off_cal_data = None
        self.fmt_dmax_cal = None
        self.mm_roi = None
        self.result__osc_calibrate_val = 0

        self.preset_mode = 'medium'
        self.phasecal_config_timeout_us = 1000
        self.mm_config_timeout_us = 2000
        self.range_config_timeout_us = 13000
        self.inter_measurement_period_ms = 100
        self.measurement_mode = 0
        self.state = LLState()
        # Carried from frame to frame: the C driver keeps one bin_data struct
        # and reads last frame's ambient out of it when it picks the bin
        # sequence, before this frame's ambient is estimated.
        self._ambient_events_sum = 0

    # ── VL53LX_read_p2p_data ──
    def read_p2p_data(self):
        """Pull the factory data: three register blocks and four NVM regions.

        Costs about 0.1 s over USB, almost all of it in the NVM.
        """
        self.img.pull('static_nvm_managed')
        self.img.pull('customer_nvm_managed')
        self.img.pull('nvm_copy_data')
        self.rtn_good_spads = rtn_good_spads(self.img.nvm_copy_data)

        customer = self.img.customer_nvm_managed
        self.hpp.algo__crosstalk_compensation_plane_offset_kcps = \
            customer.algo__crosstalk_compensation_plane_offset_kcps
        self.hpp.algo__crosstalk_compensation_x_plane_gradient_kcps = \
            customer.algo__crosstalk_compensation_x_plane_gradient_kcps
        self.hpp.algo__crosstalk_compensation_y_plane_gradient_kcps = \
            customer.algo__crosstalk_compensation_y_plane_gradient_kcps

        self.optical_centre = self.nvm.optical_centre()
        self.cal_peak_rate_map = self.nvm.cal_peak_rate_map()
        self.add_off_cal_data = self.nvm.additional_offset_cal_data()

        # Our boards have no FMT offset calibration. ST's fallback: nominal
        # MM1/MM2 peak rates, and the effective SPAD counts worked out here.
        cal = self.add_off_cal_data
        if (cal.result__mm_inner_peak_signal_count_rtn_mcps == 0
                and cal.result__mm_outer_peak_signal_count_rtn_mcps == 0):
            inner, outer = calc_mm_effective_spads(
                self.img.nvm_copy_data.roi_config__mode_roi_centre_spad,
                self.img.nvm_copy_data.roi_config__mode_roi_xy_size,
                0xC7, 0xFF, self.rtn_good_spads,
                RTN_SPAD_APERTURE_TRANSMISSION)
            self.add_off_cal_data = cal._replace(
                result__mm_inner_peak_signal_count_rtn_mcps=0x0080,
                result__mm_outer_peak_signal_count_rtn_mcps=0x0180,
                result__mm_inner_actual_effective_spads=inner,
                result__mm_outer_actual_effective_spads=outer)

        fmt = self.nvm.fmt_range_results()
        self.fmt_dmax_cal = {
            'ref__actual_effective_spads':
                fmt.result__actual_effective_rtn_spads,
            'ref__peak_signal_count_rate_mcps':
                fmt.result__peak_signal_count_rate_rtn_mcps,
            'ref__distance_mm': fmt.measured_distance_mm,
            'ref_reflectance_pc':
                self.cal_peak_rate_map.cal_reflectance_pc or 0x0014,
            'coverglass_transmission': 0x0100,
        }

        self.result__osc_calibrate_val = self.p.rd_word(
            RESULT__OSC_CALIBRATE_VAL)

        stat_nvm = self.img.static_nvm_managed
        if stat_nvm.osc_measured__fast_osc__frequency < 0x1000:
            stat_nvm.osc_measured__fast_osc__frequency = 0xBCCC

        # VL53LX_get_mode_mitigation_roi
        y, x = decode_row_col(
            self.img.nvm_copy_data.roi_config__mode_roi_centre_spad)
        xy_size = self.img.nvm_copy_data.roi_config__mode_roi_xy_size
        self.mm_roi = {'x_centre': x, 'y_centre': y,
                       'width': xy_size & 0x0F, 'height': xy_size >> 4}

        if (self.optical_centre.x_centre == 0
                and self.optical_centre.y_centre == 0):
            self.optical_centre = self.optical_centre._replace(
                x_centre=x << 4, y_centre=y << 4)

    # ── VL53LX_data_init + VL53LX_DataInit ──
    def data_init(self):
        """Everything the L3CX API does between boot and the first start: read
        the factory data, take the default preset mode and the default timing
        budget. Leaves the sensor untouched apart from the reads."""
        self.state.reset()
        self.hpp = HistPostProcessConfig()
        self.read_p2p_data()
        self.set_preset_mode('medium', inter_measurement_period_ms=1000)
        self.set_measurement_timing_budget_us(33333)

    # ── preset modes ──
    def set_preset_mode(self, mode='medium', dss_target_mcps=None,
                        inter_measurement_period_ms=None):
        """VL53LX_set_preset_mode() for the three histogram ranging modes.

        `mode` is the distance mode the L3CX API speaks in — short, medium or
        long — which maps one to one onto the device preset mode.
        """
        if mode not in DISTANCE_MODES:
            raise ValueError(f'unknown distance mode {mode!r}')

        self.preset_mode = mode
        tp = self.tuning
        if dss_target_mcps is None:
            dss_target_mcps = tp['tp_dss_target_histo_mcps']
        if inter_measurement_period_ms is None:
            inter_measurement_period_ms = self.inter_measurement_period_ms

        # VL53LX_get_preset_mode_timing_cfg: only the phasecal timeout differs
        # between the three modes.
        phasecal_us = tp['tp_phasecal_timeout_hist_%s_us'
                         % {'short': 'short', 'medium': 'med',
                            'long': 'long'}[mode]]
        mm_us = tp['tp_mm_timeout_histo_us']
        range_us = tp['tp_range_timeout_histo_us']

        self.state.reset()
        self._preset_mode_histogram_ranging()
        _PRESET_TAIL[mode](self)

        self.img.static_config.dss_config__target_total_rate_mcps = \
            dss_target_mcps
        self.set_timeouts_us(phasecal_us, mm_us, range_us)
        self.set_inter_measurement_period_ms(inter_measurement_period_ms)
        self._update_range_offset()

    def _update_range_offset(self):
        """`VL53LX_get_device_results()` recomputes `hpp.range_offset_mm` on
        every frame, under `VL53LX_OFFSETCORRECTIONMODE__MM1_MM2_OFFSETS` --
        the only mode this driver has. Its inputs are the factory data and the
        user ROI, so once per preset is the same answer for less work."""
        if self.add_off_cal_data is None:
            return
        customer = self.img.customer_nvm_managed
        nvm = self.img.nvm_copy_data
        dynamic = self.img.dynamic_config
        self.hpp.range_offset_mm = combine_mm1_mm2_offsets(
            customer.mm_config__inner_offset_mm,
            customer.mm_config__outer_offset_mm,
            nvm.roi_config__mode_roi_centre_spad,
            nvm.roi_config__mode_roi_xy_size,
            dynamic.roi_config__user_roi_centre_spad,
            dynamic.roi_config__user_roi_requested_global_xy_size,
            self.add_off_cal_data, self.rtn_good_spads,
            self.img.general_config.dss_config__aperture_attenuation)

    def is_l4(self) -> bool:
        """`IsL4()` of the L4CX BareDriver: the die, read from NVM, not the
        product name — an L4CD or L4CX board ranging under a borrowed name is
        still an L4. 0xEC is the L4ED."""
        nvm = self.img.nvm_copy_data
        return (nvm.identification__module_type == 0xAA
                and nvm.identification__model_id in (0xEB, 0xEC))

    def distance_modes(self) -> tuple:
        return L4_DISTANCE_MODES if self.is_l4() else DISTANCE_MODES

    def fda_max_timing_budget_us(self) -> int:
        return (L4_FDA_MAX_TIMING_BUDGET_US if self.is_l4()
                else FDA_MAX_TIMING_BUDGET_US)

    def set_distance_mode(self, mode):
        """VL53LX_SetDistanceMode(): the preset mode, with the timeouts put
        back afterwards. `set_preset_mode()` on its own resets them to the
        preset's own defaults, which would silently throw away the timing
        budget the caller asked for.

        Short is refused on an L4 die, as the L4CX BareDriver does: there the
        A frame of the short pair ranges on the wrong side of the wrap, one
        frame in two, measured on the L4CX board."""
        if mode not in self.distance_modes():
            raise ValueError(f'distance mode {mode} not available on this die')
        phasecal_us = self.phasecal_config_timeout_us
        mm_us = self.mm_config_timeout_us
        range_us = self.range_config_timeout_us
        self.set_preset_mode(mode)
        self.set_timeouts_us(phasecal_us, mm_us, range_us)

    def _preset_mode_standard_ranging(self):
        """VL53LX_preset_mode_standard_ranging(): the base every histogram
        mode is built on. Most of it survives into the histogram modes; the
        parts that do not are overwritten a few lines later."""
        tp = self.tuning
        static = self.img.static_config
        general = self.img.general_config
        timing = self.img.timing_config
        dynamic = self.img.dynamic_config
        system = self.img.system_control

        static.dss_config__target_total_rate_mcps = 0x0A00
        static.debug__ctrl = 0x00
        static.test_mode__ctrl = 0x00
        static.clk_gating__ctrl = 0x00
        static.nvm_bist__ctrl = 0x00
        static.nvm_bist__num_nvm_words = 0x00
        static.nvm_bist__start_address = 0x00
        static.host_if__status = 0x00
        static.pad_i2c_hv__config = 0x00
        static.pad_i2c_hv__extsup_config = 0x00
        static.gpio_hv_pad__ctrl = 0x00
        static.gpio_hv_mux__ctrl = (
            DEVICEINTERRUPTPOLARITY_ACTIVE_LOW
            | DEVICEGPIOMODE_OUTPUT_RANGE_AND_ERROR_INTERRUPTS)
        static.gpio__tio_hv_status = 0x02
        static.gpio__fio_hv_status = 0x00
        static.ana_config__spad_sel_pswidth = 0x02
        static.ana_config__vcsel_pulse_width_offset = 0x08
        static.ana_config__fast_osc__config_ctrl = 0x00
        static.sigma_estimator__effective_pulse_width_ns = \
            tp['tp_lite_sigma_est_pulse_width_ns']
        static.sigma_estimator__effective_ambient_width_ns = \
            tp['tp_lite_sigma_est_amb_width_ns']
        static.sigma_estimator__sigma_ref_mm = tp['tp_lite_sigma_ref_mm']
        static.algo__crosstalk_compensation_valid_height_mm = 0x01
        static.spare_host_config__static_config_spare_0 = 0x00
        static.spare_host_config__static_config_spare_1 = 0x00
        static.algo__range_ignore_threshold_mcps = 0x0000
        static.algo__range_ignore_valid_height_mm = 0xFF
        static.algo__range_min_clip = tp['tp_lite_min_clip']
        static.algo__consistency_check__tolerance = \
            tp['tp_consistency_lite_phase_tolerance']
        static.spare_host_config__static_config_spare_2 = 0x00
        static.sd_config__reset_stages_msb = 0x00
        static.sd_config__reset_stages_lsb = 0x00

        general.gph_config__stream_count_update_value = 0x00
        general.global_config__stream_divider = 0x00
        general.system__interrupt_config_gpio = \
            INTERRUPT_CONFIG_NEW_SAMPLE_READY
        general.cal_config__vcsel_start = 0x0B
        general.cal_config__repeat_rate = tp['tp_cal_repeat_rate']
        general.global_config__vcsel_width = 0x02
        general.phasecal_config__timeout_macrop = 0x0D
        general.phasecal_config__target = tp['tp_phasecal_target']
        general.phasecal_config__override = 0x00
        general.dss_config__roi_mode_control = DEVICEDSSMODE__TARGET_RATE
        general.system__thresh_rate_high = 0x0000
        general.system__thresh_rate_low = 0x0000
        general.dss_config__manual_effective_spads_select = 0x8C00
        general.dss_config__manual_block_select = 0x00
        general.dss_config__aperture_attenuation = 0x38
        general.dss_config__max_spads_limit = 0xFF
        general.dss_config__min_spads_limit = 0x01

        timing.mm_config__timeout_macrop_a_hi = 0x00
        timing.mm_config__timeout_macrop_a_lo = 0x1A
        timing.mm_config__timeout_macrop_b_hi = 0x00
        timing.mm_config__timeout_macrop_b_lo = 0x20
        timing.range_config__timeout_macrop_a_hi = 0x01
        timing.range_config__timeout_macrop_a_lo = 0xCC
        timing.range_config__vcsel_period_a = 0x0B
        timing.range_config__timeout_macrop_b_hi = 0x01
        timing.range_config__timeout_macrop_b_lo = 0xF5
        timing.range_config__vcsel_period_b = 0x09
        timing.range_config__sigma_thresh = tp['tp_lite_med_sigma_thresh_mm']
        timing.range_config__min_count_rate_rtn_limit_mcps = \
            tp['tp_lite_med_min_count_rate_rtn_mcps']
        timing.range_config__valid_phase_low = 0x08
        timing.range_config__valid_phase_high = 0x78
        timing.system__intermeasurement_period = 0x00000000
        timing.system__fractional_enable = 0x00

        # Standard ranging writes the histogram config out byte by byte rather
        # than through a bin sequence. Every histogram mode overwrites it, but
        # it has to be here for the same reason it is in the C driver: this
        # function is the only definition of the struct's initial state.
        h = self.hist_cfg
        (h.low_amb_even_bin_0_1, h.low_amb_even_bin_2_3,
         h.low_amb_even_bin_4_5) = 0x07, 0x21, 0x43
        (h.low_amb_odd_bin_0_1, h.low_amb_odd_bin_2_3,
         h.low_amb_odd_bin_4_5) = 0x10, 0x32, 0x54
        (h.mid_amb_even_bin_0_1, h.mid_amb_even_bin_2_3,
         h.mid_amb_even_bin_4_5) = 0x07, 0x21, 0x43
        (h.mid_amb_odd_bin_0_1, h.mid_amb_odd_bin_2, h.mid_amb_odd_bin_3_4,
         h.mid_amb_odd_bin_5) = 0x10, 0x02, 0x43, 0x05
        h.user_bin_offset = 0x00
        (h.high_amb_even_bin_0_1, h.high_amb_even_bin_2_3,
         h.high_amb_even_bin_4_5) = 0x07, 0x21, 0x43
        (h.high_amb_odd_bin_0_1, h.high_amb_odd_bin_2_3,
         h.high_amb_odd_bin_4_5) = 0x10, 0x32, 0x54
        h.amb_thresh_low = 0xFFFF
        h.amb_thresh_high = 0xFFFF
        h.spad_array_selection = 0x00

        dynamic.system__grouped_parameter_hold_0 = 0x01
        dynamic.system__thresh_high = 0x0000
        dynamic.system__thresh_low = 0x0000
        dynamic.system__enable_xtalk_per_quadrant = 0x00
        dynamic.system__seed_config = tp['tp_lite_seed_cfg']
        dynamic.sd_config__woi_sd0 = 0x0B
        dynamic.sd_config__woi_sd1 = 0x09
        dynamic.sd_config__initial_phase_sd0 = tp['tp_init_phase_rtn_lite_med']
        dynamic.sd_config__initial_phase_sd1 = tp['tp_init_phase_ref_lite_med']
        dynamic.system__grouped_parameter_hold_1 = 0x01
        dynamic.sd_config__first_order_select = tp['tp_lite_first_order_select']
        dynamic.sd_config__quantifier = tp['tp_lite_quantifier']
        dynamic.roi_config__user_roi_centre_spad = 0xC7
        dynamic.roi_config__user_roi_requested_global_xy_size = 0xFF
        dynamic.system__sequence_config = (
            SEQUENCE_VHV_EN | SEQUENCE_PHASECAL_EN | SEQUENCE_DSS1_EN
            | SEQUENCE_DSS2_EN | SEQUENCE_MM2_EN | SEQUENCE_RANGE_EN)
        dynamic.system__grouped_parameter_hold = 0x02

        system.system__stream_count_ctrl = 0x00
        system.firmware__enable = 0x01
        system.system__interrupt_clear = CLEAR_RANGE_INT
        system.system__mode_start = (DEVICESCHEDULERMODE_STREAMING
                                     | DEVICEREADOUTMODE_SINGLE_SD
                                     | DEVICEMEASUREMENTMODE_BACKTOBACK)

    def _preset_mode_histogram_ranging(self):
        """VL53LX_preset_mode_histogram_ranging(): standard ranging, then the
        histogram engine. The distance-mode tails below refine it."""
        self._preset_mode_standard_ranging()

        self.img.static_config.dss_config__target_total_rate_mcps = 0x1400
        self.hist_cfg.set_bin_sequence((7, 0, 1, 2, 3, 4), (0, 1, 2, 3, 4, 5))

        timing = self.img.timing_config
        dynamic = self.img.dynamic_config
        timing.range_config__vcsel_period_a = 0x09
        timing.range_config__vcsel_period_b = 0x0B
        dynamic.sd_config__woi_sd0 = 0x09
        dynamic.sd_config__woi_sd1 = 0x0B
        timing.mm_config__timeout_macrop_a_hi = 0x00
        timing.mm_config__timeout_macrop_a_lo = 0x20
        timing.mm_config__timeout_macrop_b_hi = 0x00
        timing.mm_config__timeout_macrop_b_lo = 0x1A
        timing.range_config__timeout_macrop_a_hi = 0x00
        timing.range_config__timeout_macrop_a_lo = 0x28
        timing.range_config__timeout_macrop_b_hi = 0x00
        timing.range_config__timeout_macrop_b_lo = 0x21
        self.img.general_config.phasecal_config__timeout_macrop = 0xF5

        self.hpp.valid_phase_low = 0x08
        self.hpp.valid_phase_high = 0x88

        self.hist_cfg.copy_to_static_cfg(self.img.static_config, timing,
                                         dynamic)
        dynamic.system__sequence_config = (
            SEQUENCE_VHV_EN | SEQUENCE_PHASECAL_EN | SEQUENCE_DSS1_EN
            | SEQUENCE_DSS2_EN | SEQUENCE_RANGE_EN)
        self.img.system_control.system__mode_start = (
            DEVICESCHEDULERMODE_HISTOGRAM | DEVICEREADOUTMODE_DUAL_SD
            | DEVICEMEASUREMENTMODE_BACKTOBACK)

    def _preset_tail(self, even, odd, vcsel_a, vcsel_b, mm_a, mm_b,
                     range_a, range_b, cal_vcsel_start, valid_phase_high,
                     phase_rtn, phase_ref, extra_sequence=0):
        """The shape all three histogram distance modes share: a bin sequence,
        a VCSEL period pair and the timeouts that go with it."""
        static = self.img.static_config
        general = self.img.general_config
        timing = self.img.timing_config
        dynamic = self.img.dynamic_config

        self.hist_cfg.set_bin_sequence(even, odd)
        self.hist_cfg.copy_to_static_cfg(static, timing, dynamic)

        timing.range_config__vcsel_period_a = vcsel_a
        timing.range_config__vcsel_period_b = vcsel_b
        timing.mm_config__timeout_macrop_a_hi = mm_a >> 8
        timing.mm_config__timeout_macrop_a_lo = mm_a & 0xFF
        timing.mm_config__timeout_macrop_b_hi = mm_b >> 8
        timing.mm_config__timeout_macrop_b_lo = mm_b & 0xFF
        timing.range_config__timeout_macrop_a_hi = range_a >> 8
        timing.range_config__timeout_macrop_a_lo = range_a & 0xFF
        timing.range_config__timeout_macrop_b_hi = range_b >> 8
        timing.range_config__timeout_macrop_b_lo = range_b & 0xFF

        general.cal_config__vcsel_start = cal_vcsel_start
        general.phasecal_config__timeout_macrop = 0xF5

        dynamic.sd_config__woi_sd0 = vcsel_a
        dynamic.sd_config__woi_sd1 = vcsel_b
        dynamic.sd_config__initial_phase_sd0 = self.tuning[phase_rtn]
        dynamic.sd_config__initial_phase_sd1 = self.tuning[phase_ref]
        dynamic.system__sequence_config = (
            SEQUENCE_VHV_EN | SEQUENCE_PHASECAL_EN | SEQUENCE_DSS1_EN
            | SEQUENCE_DSS2_EN | extra_sequence | SEQUENCE_RANGE_EN)

        self.hpp.valid_phase_low = 0x08
        self.hpp.valid_phase_high = valid_phase_high

        self.img.system_control.system__mode_start = (
            DEVICESCHEDULERMODE_HISTOGRAM | DEVICEREADOUTMODE_DUAL_SD
            | DEVICEMEASUREMENTMODE_BACKTOBACK)

    # ── timeouts ──
    def set_timeouts_us(self, phasecal_us, mm_us, range_us):
        self.phasecal_config_timeout_us = phasecal_us
        self.mm_config_timeout_us = mm_us
        self.range_config_timeout_us = range_us
        calc_timeout_register_values(
            phasecal_us, mm_us, range_us,
            self.img.static_nvm_managed.osc_measured__fast_osc__frequency,
            self.img.general_config, self.img.timing_config)

    def set_inter_measurement_period_ms(self, period_ms):
        if self.result__osc_calibrate_val == 0:
            raise ZeroDivisionError('result__osc_calibrate_val is 0')
        self.inter_measurement_period_ms = period_ms
        self.img.timing_config.system__intermeasurement_period = \
            period_ms * self.result__osc_calibrate_val

    def set_measurement_timing_budget_us(self, budget_us):
        """VL53LX_SetMeasurementTimingBudgetMicroSeconds(). The budget covers
        the whole measurement, of which the range timeout is one sixth after a
        fixed guard comes off."""
        if not TIMING_GUARD_US < budget_us <= 10000000:
            raise ValueError(f'timing budget {budget_us} us out of range')
        range_us = (budget_us - TIMING_GUARD_US) // TIMING_DIVISOR
        if range_us * TIMING_DIVISOR > self.fda_max_timing_budget_us():
            raise ValueError(f'timing budget {budget_us} us out of range')
        self.set_timeouts_us(self.phasecal_config_timeout_us,
                             self.mm_config_timeout_us, range_us)

    # ── the firmware patch StartMeasurement loads ──
    def load_patch(self):
        """VL53LX_load_patch(): a six-instruction phasecal patch, written with
        the firmware stopped."""
        power = {0: 0x00, 1: 0x10, 2: 0x20, 3: 0x40}.get(
            self.tuning['tp_phasecal_patch_power'], 0x00)
        self.p.wr_byte(FIRMWARE__ENABLE, 0x00)
        self._enable_powerforce()
        self.p.wr_multi(PATCH__OFFSET_0,
                        bytes([0x29, 0xC9, 0x0E, 0x40, 0x28, power]))
        self.p.wr_multi(PATCH__ADDRESS_0,
                        bytes([0x03, 0x6D, 0x03, 0x6F, 0x07, 0x29]))
        self.p.wr_multi(PATCH__JMP_ENABLES, bytes([0x00, 0x07]))
        self.p.wr_multi(PATCH__DATA_ENABLES, bytes([0x00, 0x07]))
        self.p.wr_byte(PATCH__CTRL, 0x01)
        self.p.wr_byte(FIRMWARE__ENABLE, 0x01)

    def unload_patch(self):
        self.p.wr_byte(FIRMWARE__ENABLE, 0x00)
        self._disable_powerforce()
        self.p.wr_byte(PATCH__CTRL, 0x00)
        self.p.wr_byte(FIRMWARE__ENABLE, 0x01)

    def _enable_powerforce(self):
        self.p.wr_byte(POWER_MANAGEMENT__GO1_POWER_FORCE, 0x01)
        self.p.sleep_ms(1)              # 250 us settling, rounded up

    def _disable_powerforce(self):
        self.p.wr_byte(POWER_MANAGEMENT__GO1_POWER_FORCE, 0x00)

    # ── VL53LX_init_and_start_range ──
    def init_and_start_range(self, blocks=RANGE_START_BLOCKS):
        """Write the configuration and start. `blocks` is ST's config level:
        all of them to start, and general_config onwards to clear the
        interrupt and let the next frame run."""
        dynamic = self.img.dynamic_config
        system = self.img.system_control

        self.measurement_mode = DEVICEMEASUREMENTMODE_BACKTOBACK
        system.system__mode_start = (
            (system.system__mode_start & DEVICEMEASUREMENTMODE_STOP_MASK)
            | self.measurement_mode)
        system.system__interrupt_clear = CLEAR_RANGE_INT

        gph_id = self.state.cfg_gph_id
        dynamic.system__grouped_parameter_hold_0 = gph_id | 0x01
        dynamic.system__grouped_parameter_hold_1 = gph_id | 0x01
        dynamic.system__grouped_parameter_hold = gph_id

        self.img.push_range(blocks)

        self.state.update_rd(system.system__mode_start,
                             dynamic.system__grouped_parameter_hold)
        self.state.update_cfg(system.system__mode_start)

    def clear_interrupt_and_start_next_range(self):
        """VL53LX_clear_interrupt_and_enable_next_range(): in histogram mode
        the interrupt is cleared by rewriting general_config onwards, not by a
        write to system__interrupt_clear alone."""
        self.init_and_start_range(('general_config', 'timing_config',
                                   'dynamic_config', 'system_control'))

    def stop_range(self):
        system = self.img.system_control
        system.system__mode_start = (
            (system.system__mode_start & DEVICEMEASUREMENTMODE_STOP_MASK)
            | DEVICEMEASUREMENTMODE_ABORT)
        self.img.push('system_control')
        system.system__mode_start &= DEVICEMEASUREMENTMODE_STOP_MASK
        self.state.reset()

    # ── VL53LX_get_histogram_bin_data ──
    def get_histogram_bin_data(self, raw=None) -> HistogramBinData:
        """One frame: 83 bytes off the sensor, decoded into bins plus the
        metadata the post-processing needs. `raw` lets a caller hand in bytes
        that arrived some other way (a stream report)."""
        if raw is None:
            raw = self.p.rd_multi(HISTOGRAM_BIN_DATA_I2C_INDEX,
                                  HISTOGRAM_BIN_DATA_I2C_SIZE_BYTES)
        if len(raw) < HISTOGRAM_BIN_DATA_I2C_SIZE_BYTES:
            raise ValueError(f'histogram frame is {len(raw)} bytes, need '
                             f'{HISTOGRAM_BIN_DATA_I2C_SIZE_BYTES}')
        buf = bytearray(raw)
        off = HISTOGRAM_BIN_DATA_I2C_INDEX

        d = HistogramBinData()
        d.result__interrupt_status = buf[0]
        d.result__range_status = buf[1]
        d.result__report_status = buf[2]
        d.result__stream_count = buf[3]
        d.result__dss_actual_effective_spads = int.from_bytes(buf[4:6], 'big')
        d.phasecal_result__reference_phase = int.from_bytes(
            buf[PHASECAL_RESULT__REFERENCE_PHASE - off:][:2], 'big')
        d.phasecal_result__vcsel_start = buf[PHASECAL_RESULT__VCSEL_START - off]

        # Bin 23 does not fit in its three bytes: the low byte is carried in a
        # separate MSB/LSB pair and patched back in before the bins are read.
        buf[RESULT__HISTOGRAM_BIN_23_0 - off] = (
            (buf[RESULT__HISTOGRAM_BIN_23_0_MSB - off] << 2)
            + buf[RESULT__HISTOGRAM_BIN_23_0_LSB - off]) & 0xFF
        base = RESULT__HISTOGRAM_BIN_0_2 - off
        d.bin_data = [int.from_bytes(buf[base + 3 * i:base + 3 * i + 3], 'big')
                      for i in range(HISTOGRAM_BUFFER_SIZE)]

        general = self.img.general_config
        static = self.img.static_config
        timing = self.img.timing_config

        d.cal_config__vcsel_start = general.cal_config__vcsel_start
        d.vcsel_width = ((general.global_config__vcsel_width << 4)
                         + static.ana_config__vcsel_pulse_width_offset)
        d.fast_osc_frequency = \
            self.img.static_nvm_managed.osc_measured__fast_osc__frequency
        d.roi_config__user_roi_centre_spad = \
            self.img.dynamic_config.roi_config__user_roi_centre_spad
        d.roi_config__user_roi_requested_global_xy_size = \
            self.img.dynamic_config.roi_config__user_roi_requested_global_xy_size

        d.zone_id = 0
        d.bin_seq = self.hist_cfg.bin_sequence(self.state.rd_stream_count,
                                               self._ambient_events_sum)

        if self.state.rd_timing_status == 0:
            encoded = ((timing.range_config__timeout_macrop_a_hi << 8)
                       + timing.range_config__timeout_macrop_a_lo)
            d.vcsel_period = timing.range_config__vcsel_period_a
        else:
            encoded = ((timing.range_config__timeout_macrop_b_hi << 8)
                       + timing.range_config__timeout_macrop_b_lo)
            d.vcsel_period = timing.range_config__vcsel_period_b

        d.number_of_ambient_bins = 4 * sum(1 for s in d.bin_seq
                                           if s & 0x07 == 0x07)
        d.total_periods_elapsed = decode_timeout(encoded)
        d.peak_duration_us = duration_maths(
            calc_pll_period_us(d.fast_osc_frequency), d.vcsel_width,
            RANGING_WINDOW_VCSEL_PERIODS, d.total_periods_elapsed + 1)
        d.woi_duration_us = 0

        # VL53LX_hist_calc_zero_distance_phase
        period = 2048 * decode_vcsel_period(d.vcsel_period)
        phase = (period + d.phasecal_result__reference_phase
                 + 2048 * d.phasecal_result__vcsel_start
                 - 2048 * d.cal_config__vcsel_start)
        d.zero_distance_phase = phase % period if period else 0

        # VL53LX_hist_estimate_ambient_from_ambient_bins
        if d.number_of_ambient_bins > 0:
            d.number_of_ambient_samples = d.number_of_ambient_bins
            d.ambient_events_sum = sum(d.bin_data[:d.number_of_ambient_bins])
            d.ambient_per_bin = ((d.ambient_events_sum
                                  + d.number_of_ambient_bins // 2)
                                 // d.number_of_ambient_bins)
        self._ambient_events_sum = d.ambient_events_sum
        return d


# ── the three distance-mode tails (vl53lx_api_preset_modes.c) ────────────────
# Each is the block VL53LX_preset_mode_histogram_<mode>_range() applies on top
# of histogram_ranging: bin sequence, VCSEL periods, timeouts, phases.
def _tail_long(drv):
    drv._preset_tail(even=(7, 0, 1, 2, 3, 4), odd=(0, 1, 2, 3, 4, 5),
                     vcsel_a=0x09, vcsel_b=0x0B, mm_a=0x0021, mm_b=0x001B,
                     range_a=0x0029, range_b=0x0022, cal_vcsel_start=0x09,
                     valid_phase_high=0x88,
                     phase_rtn='tp_init_phase_rtn_hist_long',
                     phase_ref='tp_init_phase_ref_hist_long')


def _tail_medium(drv):
    drv._preset_tail(even=(7, 0, 1, 1, 2, 2), odd=(0, 1, 2, 1, 2, 3),
                     vcsel_a=0x05, vcsel_b=0x07, mm_a=0x0036, mm_b=0x0028,
                     range_a=0x0044, range_b=0x0033, cal_vcsel_start=0x05,
                     valid_phase_high=0x48,
                     phase_rtn='tp_init_phase_rtn_hist_med',
                     phase_ref='tp_init_phase_ref_hist_med')


def _tail_short(drv):
    drv._preset_tail(even=(7, 7, 0, 1, 1, 1), odd=(0, 1, 1, 1, 2, 2),
                     vcsel_a=0x03, vcsel_b=0x05, mm_a=0x0052, mm_b=0x0037,
                     range_a=0x0066, range_b=0x0044, cal_vcsel_start=0x03,
                     valid_phase_high=0x28,
                     phase_rtn='tp_init_phase_rtn_hist_short',
                     phase_ref='tp_init_phase_ref_hist_short',
                     extra_sequence=SEQUENCE_MM1_EN)


_PRESET_TAIL = {'long': _tail_long, 'medium': _tail_medium,
                'short': _tail_short}
