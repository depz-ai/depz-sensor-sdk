"""
uld/l0x.py — VL53L0X API 1.0.4 in Python: the ranging path.

A faithful port of the parts of vl53l0x_api.c / vl53l0x_api_core.c /
vl53l0x_api_calibration.c that a continuous-ranging session needs:
DataInit, StaticInit (reference SPADs from NVM + tuning settings), the timing
budget arithmetic, StartMeasurement/StopMeasurement and
GetRangingMeasurementData with its PAL range status (sigma estimate included).
Register sequences, integer widths and the deliberate overflows are kept as
they are in C — do not "simplify" them.

PerformRefCalibration (VHV / phase) is here too, although the C driver leaves it
to the application: without it the sensor ranges nothing.

PerformRefSpadManagement, the offset and crosstalk calibrations and the ranging
profiles of the four ST examples are here as well. A board whose NVM
reference-SPAD record is invalid gets PerformRefSpadManagement run for it by
sensor_init().

There is no ROI on this die: VL53L0X_SetNumberOfROIZones() accepts 1 and nothing
else, and the API exposes no window - the ROI of the family starts at the L1.

Reference C driver: ../../temp/VL53L0X_1.0.4/Api/core/
"""

import time

from depz_sensor_sdk.vl53lx._link import I2C_KHZ_BOOT, ProtocolError, Vl53Error
from depz_sensor_sdk.vl53lx.uld.base import BridgePlatform, Measurement, SensorDriver

API_VERSION = (1, 0, 4)

# ── registers (vl53l0x_device.h) ─────────────────────────────────────────────
SYSRANGE_START                              = 0x00
SYSRANGE_MODE_SINGLESHOT                    = 0x00
SYSRANGE_MODE_START_STOP                    = 0x01
SYSRANGE_MODE_BACKTOBACK                    = 0x02
SYSRANGE_MODE_TIMED                         = 0x04

SYSTEM_SEQUENCE_CONFIG                      = 0x01
SYSTEM_INTERMEASUREMENT_PERIOD              = 0x04
SYSTEM_RANGE_CONFIG                         = 0x09
SYSTEM_INTERRUPT_CONFIG_GPIO                = 0x0A
SYSTEM_INTERRUPT_CLEAR                      = 0x0B
SYSTEM_THRESH_HIGH                          = 0x0C
SYSTEM_THRESH_LOW                           = 0x0E
RESULT_INTERRUPT_STATUS                     = 0x13
RESULT_RANGE_STATUS                         = 0x14
CROSSTALK_COMPENSATION_PEAK_RATE_MCPS       = 0x20
ALGO_PART_TO_PART_RANGE_OFFSET_MM           = 0x28
ALGO_PHASECAL_CONFIG_TIMEOUT                = 0x30
GLOBAL_CONFIG_VCSEL_WIDTH                   = 0x32
FINAL_RANGE_CONFIG_MIN_COUNT_RATE_RTN_LIMIT = 0x44
MSRC_CONFIG_TIMEOUT_MACROP                  = 0x46
FINAL_RANGE_CONFIG_VALID_PHASE_LOW          = 0x47
FINAL_RANGE_CONFIG_VALID_PHASE_HIGH         = 0x48
DYNAMIC_SPAD_NUM_REQUESTED_REF_SPAD         = 0x4E
DYNAMIC_SPAD_REF_EN_START_OFFSET            = 0x4F
PRE_RANGE_CONFIG_VCSEL_PERIOD               = 0x50
PRE_RANGE_CONFIG_TIMEOUT_MACROP_HI          = 0x51
PRE_RANGE_CONFIG_VALID_PHASE_LOW            = 0x56
PRE_RANGE_CONFIG_VALID_PHASE_HIGH           = 0x57
MSRC_CONFIG_CONTROL                         = 0x60
PRE_RANGE_MIN_COUNT_RATE_RTN_LIMIT          = 0x64
FINAL_RANGE_CONFIG_VCSEL_PERIOD             = 0x70
FINAL_RANGE_CONFIG_TIMEOUT_MACROP_HI        = 0x71
POWER_MANAGEMENT_GO1_POWER_FORCE            = 0x80
GPIO_HV_MUX_ACTIVE_HIGH                     = 0x84
SOFT_RESET_GO2_SOFT_RESET_N                 = 0xBF
GLOBAL_CONFIG_SPAD_ENABLES_REF_0            = 0xB0
GLOBAL_CONFIG_REF_EN_START_SELECT           = 0xB6
IDENTIFICATION_MODEL_ID                     = 0xC0
OSC_CALIBRATE_VAL                           = 0xF8

# Page 1 (write 0x01 to 0xFF first): these two share their address with a
# page-0 register, ALGO_PHASECAL_CONFIG_TIMEOUT and
# GLOBAL_CONFIG_REF_EN_START_SELECT respectively.
ALGO_PHASECAL_LIM                           = 0x30
RESULT_PEAK_SIGNAL_RATE_REF                 = 0xB6

MODEL_ID_VL53L0X = 0xEE     # one byte at 0xC0, unlike the 16-bit family;
                            # registry.TABLE is what a caller checks

# Device modes (VL53L0X_DeviceModes).
DEVICEMODE_SINGLE_RANGING     = 0
DEVICEMODE_CONTINUOUS_RANGING = 1
DEVICEMODE_CONTINUOUS_TIMED   = 3

# GPIO functionality: SYSTEM_INTERRUPT_CONFIG_GPIO value for "new sample ready".
GPIOFUNCTIONALITY_NEW_MEASURE_READY = 4

# Sequence steps (VL53L0X_SequenceStepId) and their SYSTEM_SEQUENCE_CONFIG bits.
SEQUENCESTEP_TCC, SEQUENCESTEP_DSS, SEQUENCESTEP_MSRC = 0, 1, 2
SEQUENCESTEP_PRE_RANGE, SEQUENCESTEP_FINAL_RANGE = 3, 4
_SEQ_SET   = {SEQUENCESTEP_TCC: 0x10, SEQUENCESTEP_DSS: 0x28,
              SEQUENCESTEP_MSRC: 0x04, SEQUENCESTEP_PRE_RANGE: 0x40,
              SEQUENCESTEP_FINAL_RANGE: 0x80}
_SEQ_CLEAR = {SEQUENCESTEP_TCC: 0xEF, SEQUENCESTEP_DSS: 0xD7,
              SEQUENCESTEP_MSRC: 0xFB, SEQUENCESTEP_PRE_RANGE: 0xBF,
              SEQUENCESTEP_FINAL_RANGE: 0x7F}
_SEQ_TEST  = {SEQUENCESTEP_TCC: 0x10, SEQUENCESTEP_DSS: 0x08,
              SEQUENCESTEP_MSRC: 0x04, SEQUENCESTEP_PRE_RANGE: 0x40,
              SEQUENCESTEP_FINAL_RANGE: 0x80}

VCSEL_PERIOD_PRE_RANGE, VCSEL_PERIOD_FINAL_RANGE = 0, 1

# Limit checks (VL53L0X_CHECKENABLE_*).
CHECK_SIGMA_FINAL_RANGE       = 0
CHECK_SIGNAL_RATE_FINAL_RANGE = 1
CHECK_SIGNAL_REF_CLIP         = 2
CHECK_RANGE_IGNORE_THRESHOLD  = 3
CHECK_SIGNAL_RATE_MSRC        = 4
CHECK_SIGNAL_RATE_PRE_RANGE   = 5
CHECK_NUMBER_OF_CHECKS        = 6

DEFAULT_MAX_LOOP     = 2000
TARGET_REF_RATE      = 0x0A00   # 20 Mcps in 9.7 format, the ref-SPAD target
SPEED_OF_LIGHT_IN_AIR = 2997
REF_SPAD_BUFFER_SIZE = 6

# The ranging modes of this product, straight out of the four ST examples
# (ApiExample/examples/src/vl53l0x_SingleRanging_*_Example.c): the signal and
# sigma limits, the timing budget and the two VCSEL periods each of them sets.
# 'default' is what StaticInit leaves behind, listed so a caller can go back.
# Values are (signal Mcps, sigma mm, budget us, pre-range PCLK, final PCLK).
MODES_DEFAULT = 'default'
MODE_SETTINGS = {
    'default':       (0.25, 18,  33000, 14, 10),
    'long-range':    (0.10, 60,  33000, 18, 14),
    'high-speed':    (0.25, 32,  30000, 14, 10),
    'high-accuracy': (0.25, 18, 200000, 14, 10),
}

# ST's range for each final-range VCSEL period the profiles above use: the
# period sets how far the phase stays unambiguous. Checked at a wall ~1.8 m:
# final 10 fails it with status 4 in all three short profiles, final 14 ranges.
REACH_BY_FINAL_PCLKS_MM = {10: 1200, 14: 2000}

# DefaultTuningSettings[] (vl53l0x_tuning.h, "update 02/11/2015_v36"), kept in
# its original buffer form: a run of {count, address, count bytes...} records
# ended by a zero count. load_tuning_settings() parses it exactly as the C
# driver does, so the threshold settings buffer of E5 can go through unchanged.
DEFAULT_TUNING_SETTINGS = bytes([
    0x01, 0xFF, 0x01,
    0x01, 0x00, 0x00,

    0x01, 0xFF, 0x00,
    0x01, 0x09, 0x00,
    0x01, 0x10, 0x00,
    0x01, 0x11, 0x00,

    0x01, 0x24, 0x01,
    0x01, 0x25, 0xff,
    0x01, 0x75, 0x00,

    0x01, 0xFF, 0x01,
    0x01, 0x4e, 0x2c,
    0x01, 0x48, 0x00,
    0x01, 0x30, 0x20,

    0x01, 0xFF, 0x00,
    0x01, 0x30, 0x09,
    0x01, 0x54, 0x00,
    0x01, 0x31, 0x04,
    0x01, 0x32, 0x03,
    0x01, 0x40, 0x83,
    0x01, 0x46, 0x25,
    0x01, 0x60, 0x00,
    0x01, 0x27, 0x00,
    0x01, 0x50, 0x06,
    0x01, 0x51, 0x00,
    0x01, 0x52, 0x96,
    0x01, 0x56, 0x08,
    0x01, 0x57, 0x30,
    0x01, 0x61, 0x00,
    0x01, 0x62, 0x00,
    0x01, 0x64, 0x00,
    0x01, 0x65, 0x00,
    0x01, 0x66, 0xa0,

    0x01, 0xFF, 0x01,
    0x01, 0x22, 0x32,
    0x01, 0x47, 0x14,
    0x01, 0x49, 0xff,
    0x01, 0x4a, 0x00,

    0x01, 0xFF, 0x00,
    0x01, 0x7a, 0x0a,
    0x01, 0x7b, 0x00,
    0x01, 0x78, 0x21,

    0x01, 0xFF, 0x01,
    0x01, 0x23, 0x34,
    0x01, 0x42, 0x00,
    0x01, 0x44, 0xff,
    0x01, 0x45, 0x26,
    0x01, 0x46, 0x05,
    0x01, 0x40, 0x40,
    0x01, 0x0E, 0x06,
    0x01, 0x20, 0x1a,
    0x01, 0x43, 0x40,

    0x01, 0xFF, 0x00,
    0x01, 0x34, 0x03,
    0x01, 0x35, 0x44,

    0x01, 0xFF, 0x01,
    0x01, 0x31, 0x04,
    0x01, 0x4b, 0x09,
    0x01, 0x4c, 0x05,
    0x01, 0x4d, 0x04,

    0x01, 0xFF, 0x00,
    0x01, 0x44, 0x00,
    0x01, 0x45, 0x20,
    0x01, 0x47, 0x08,
    0x01, 0x48, 0x28,
    0x01, 0x67, 0x00,
    0x01, 0x70, 0x04,
    0x01, 0x71, 0x01,
    0x01, 0x72, 0xfe,
    0x01, 0x76, 0x00,
    0x01, 0x77, 0x00,

    0x01, 0xFF, 0x01,
    0x01, 0x0d, 0x01,

    0x01, 0xFF, 0x00,
    0x01, 0x80, 0x01,
    0x01, 0x01, 0xF8,

    0x01, 0xFF, 0x01,
    0x01, 0x8e, 0x01,
    0x01, 0x00, 0x01,
    0x01, 0xFF, 0x00,
    0x01, 0x80, 0x00,

    0x00, 0x00, 0x00,
])

# Dmax lookup table set up by DataInit, in FixPoint16.16.
DMAX_LUT_AMB_RATE_MCPS = (0x00000000, 0x0000B333, 0x00020000, 0x0003CCCC,
                          0x00074CCC, 0x000A0000, 0x000F0000)
DMAX_LUT_DMAX_MM       = (0x04B00000, 0x044C0000, 0x03840000, 0x02EE0000,
                          0x02260000, 0x01F40000, 0x01900000)

# The block the MCU streams: RESULT_RANGE_STATUS .. 0x1F, the same 12 bytes
# GetRangingMeasurementData() reads in one go.
RESULT_BLOCK_ADDR = RESULT_RANGE_STATUS
RESULT_BLOCK_LEN  = 12

# How this sensor releases its interrupt: two writes, unlike the single
# 0x0086 <- 0x01 of the 16-bit-addressed parts. ClearInterruptMask() also reads
# RESULT_INTERRUPT_STATUS back and repeats up to three times; the bridge cannot
# do the read-and-retry, so the streamed slot plays the two writes only (see
# the plan note "Риск на E4").
CLEAR_STEPS = ((SYSTEM_INTERRUPT_CLEAR, 0x01), (SYSTEM_INTERRUPT_CLEAR, 0x00))

ADDR_WIDTH = 1      # the only part of the family with 8-bit register addresses

# VL53L0X_GetRangeStatusString() (vl53l0x_api_strings.c).
RANGE_STATUS_NAMES = {
    0:   'Range Valid',
    1:   'Sigma Fail',
    2:   'Signal Fail',
    3:   'Min Range Fail',
    4:   'Phase Fail',
    5:   'Hardware Fail',
    255: 'No Update',
}

# Reference SPAD array geometry (vl53l0x_api_calibration.c).
REF_ARRAY_SPAD_0, REF_ARRAY_SPAD_5, REF_ARRAY_SPAD_10 = 0, 5, 10
REF_ARRAY_QUADRANTS = (REF_ARRAY_SPAD_10, REF_ARRAY_SPAD_5,
                       REF_ARRAY_SPAD_0, REF_ARRAY_SPAD_5)


def _u32(value: int) -> int:
    """C uint32 wrap-around. The ULD arithmetic overflows on purpose."""
    return value & 0xFFFFFFFF


def isqrt(num: int) -> int:
    """VL53L0X_isqrt() — the bit-by-bit integer square root of the C driver."""
    res = 0
    bit = 1 << 30
    while bit > num:
        bit >>= 2
    while bit != 0:
        if num >= res + bit:
            num -= res + bit
            res = (res >> 1) + bit
        else:
            res >>= 1
        bit >>= 2
    return res


def decode_vcsel_period(vcsel_period_reg: int) -> int:
    return (vcsel_period_reg + 1) << 1


def encode_vcsel_period(vcsel_period_pclks: int) -> int:
    return (vcsel_period_pclks >> 1) - 1


def encode_timeout(timeout_macro_clks: int) -> int:
    """(LSByte * 2^MSByte) + 1 format."""
    if timeout_macro_clks <= 0:
        return 0
    ls_byte = timeout_macro_clks - 1
    ms_byte = 0
    while ls_byte & 0xFFFFFF00:
        ls_byte >>= 1
        ms_byte += 1
    return ((ms_byte << 8) + (ls_byte & 0xFF)) & 0xFFFF


def decode_timeout(encoded_timeout: int) -> int:
    return ((encoded_timeout & 0x00FF) << ((encoded_timeout & 0xFF00) >> 8)) + 1


def calc_macro_period_ps(vcsel_period_pclks: int) -> int:
    """2304 vclks per macro period, PLL period fixed at 1655 ps."""
    return 2304 * vcsel_period_pclks * 1655


def calc_timeout_mclks(timeout_period_us: int, vcsel_period_pclks: int) -> int:
    macro_period_ns = (calc_macro_period_ps(vcsel_period_pclks) + 500) // 1000
    return ((timeout_period_us * 1000) + (macro_period_ns // 2)) // macro_period_ns


def calc_timeout_us(timeout_period_mclks: int, vcsel_period_pclks: int) -> int:
    macro_period_ns = (calc_macro_period_ps(vcsel_period_pclks) + 500) // 1000
    return ((timeout_period_mclks * macro_period_ns) + 500) // 1000


def is_aperture(spad_index: int) -> bool:
    """Which quadrant a SPAD index falls in decides whether it is an aperture
    SPAD."""
    return REF_ARRAY_QUADRANTS[spad_index >> 6] != REF_ARRAY_SPAD_0


def get_next_good_spad(good_spad_array: list, size: int, curr: int) -> int:
    """-> index of the next enabled bit at or after `curr`, or -1."""
    start_index = curr // 8
    fine_offset = curr % 8
    for coarse_index in range(start_index, size):
        data_byte = good_spad_array[coarse_index]
        fine_index = 0
        if coarse_index == start_index:
            data_byte >>= fine_offset
            fine_index = fine_offset
        while fine_index < 8:
            if data_byte & 1:
                return coarse_index * 8 + fine_index
            data_byte >>= 1
            fine_index += 1
    return -1


class RangingMeasurementData:
    """VL53L0X_RangingMeasurementData_t, the fields this port fills."""

    __slots__ = ('range_status', 'distance_mm', 'range_fractional_part',
                 'signal_rate_mcps', 'ambient_rate_mcps',
                 'effective_spad_rtn_count', 'dmax_mm', 'sigma_mm',
                 'device_range_status')

    def __init__(self, **kw):
        for name in self.__slots__:
            setattr(self, name, kw.get(name, 0))

    @property
    def status_text(self) -> str:
        return RANGE_STATUS_NAMES.get(self.range_status,
                                      f'unknown ({self.range_status})')


class VL53L0X(SensorDriver):
    """Port of the VL53L0X API 1.0.4 ranging path."""

    ADDR_WIDTH  = ADDR_WIDTH
    CLEAR_STEPS = CLEAR_STEPS
    MAX_KHZ     = 400           # no Fast Mode Plus on this die
    SUPPORTS    = frozenset({'mode', 'timing', 'offset', 'xtalk',
                             'calib_offset', 'calib_xtalk', 'refspad'})
    MODES       = tuple(MODE_SETTINGS)
    # Below ~20 ms the enabled sequence steps no longer fit in the budget and
    # set_measurement_timing_budget() refuses it.
    BUDGET_MS   = (20, 200)

    def __init__(self, platform: BridgePlatform, part: str):
        super().__init__(platform, part)
        # PALDevData / DeviceSpecificParameters, the state the C driver keeps
        # beside the sensor. Every field is written before it is read.
        self.d = {
            'ReadDataFromDeviceDone': 0,
            'LinearityCorrectiveGain': 1000,
            'OscFrequencyMHz': 618660,
            'XTalkCompensationRateMegaCps': 0,
            'XTalkCompensationEnable': 0,
            'StopVariable': 0,
            'SequenceConfig': 0,
            'RangeFractionalEnable': 0,
            'DeviceMode': DEVICEMODE_SINGLE_RANGING,
            # Not in the C driver: the continuous mode last chosen, so a
            # single shot in between (the calibrations run on one) does not
            # leave start_ranging() stranded. None until one is chosen.
            'ContinuousMode': None,
            'MeasurementTimingBudgetMicroSeconds': 0,
            'Pin0GpioFunctionality': GPIOFUNCTIONALITY_NEW_MEASURE_READY,
            'ReferenceSpadCount': 0,
            'ReferenceSpadType': 0,
            'RefGoodSpadMap': [0] * REF_SPAD_BUFFER_SIZE,
            'RefSpadEnables': [0] * REF_SPAD_BUFFER_SIZE,
            'PreRangeVcselPulsePeriod': 0,
            'FinalRangeVcselPulsePeriod': 0,
            'PreRangeTimeoutMicroSecs': 0,
            'FinalRangeTimeoutMicroSecs': 0,
            'TargetRefRate': TARGET_REF_RATE,
            'LimitChecksEnable': [0] * CHECK_NUMBER_OF_CHECKS,
            'LimitChecksValue': [0] * CHECK_NUMBER_OF_CHECKS,
            'Part2PartOffsetAdjustmentNVMMicroMeter': 0,
            'SignalRateMeasFixed400mm': 0,
            'ModuleId': 0,
            'Revision': 0,
            'ProductId': '',
            'PartUIDUpper': 0,
            'PartUIDLower': 0,
        }
        # Set by static_init(): False when the NVM reference-SPAD record is out
        # of range and perform_ref_spad_management() has to supply it instead.
        self.ref_spads_from_nvm = True
        # Which of MODES is applied. The five registers a mode writes can be
        # read back (driver_info()), but the budget comes back rounded, so
        # the name is remembered rather than inferred. static_init() leaves
        # the sensor in 'default'.
        self._mode = MODES_DEFAULT

    # ── SensorDriver contract ──
    def stream_block(self) -> tuple:
        return RESULT_BLOCK_ADDR, RESULT_BLOCK_LEN

    def decode(self, raw: bytes) -> Measurement:
        return _as_measurement(self.parse_result_block(raw))

    def read_measurement(self) -> Measurement:
        return _as_measurement(self.get_result())

    # ── identity ──
    def model_id(self) -> int:
        return self.p.rd_byte(IDENTIFICATION_MODEL_ID)

    def wait_boot(self, timeout_s: float = 1.0):
        """The VL53L0X has no firmware-status register; the model ID answering
        is what says the die is up after an XSHUT pulse."""
        deadline = time.monotonic() + timeout_s
        while True:
            try:
                if self.p.rd_byte(IDENTIFICATION_MODEL_ID) == MODEL_ID_VL53L0X:
                    return
            except ProtocolError:
                pass                # NACK while the die is still booting
            if time.monotonic() > deadline:
                raise Vl53Error('timeout waiting for MODEL_ID 0xEE at 0xC0')
            self.p.sleep_ms(1)

    # ── init ──
    def sensor_init(self):
        """DataInit + StaticInit, and leave the sensor in continuous mode.

        The bus stays at I2C_KHZ_BOOT throughout: this die has no Fast Mode
        Plus pad, so 400 kHz is both the boot speed and MAX_KHZ.
        """
        self.p.set_addr_width(ADDR_WIDTH)
        self.p.set_i2c_speed(I2C_KHZ_BOOT)
        self.wait_boot()

        self.reset_device()
        self.data_init()
        self.static_init()
        if not self.ref_spads_from_nvm:
            self._ref_spad_management()
        self.perform_ref_calibration()
        self.set_device_mode(DEVICEMODE_CONTINUOUS_RANGING)
        self._mode = MODES_DEFAULT      # what static_init() leaves behind

        if self.MAX_KHZ != I2C_KHZ_BOOT:
            self.p.set_i2c_speed(self.MAX_KHZ)

    def reset_device(self, timeout_s: float = 1.0):
        """VL53L0X_ResetDevice(): pull the die through its soft reset.

        Not optional here, and not in the C driver either — DataInit and
        StaticInit configure a *fresh* device. Started on a sensor left
        configured by a previous session, the sequence runs through without a
        single error and then ranges nothing: signal noise, ambient pinned
        around 7 Mcps whatever the light, distance stuck at 8190 mm. Measured
        on the VL53L0X board (COM7); one XSHUT reset in front of the same code
        turns it into 411 mm, status 0, ambient 70 kcps.
        """
        deadline = time.monotonic() + timeout_s

        def poll_model_id(want_zero: bool):
            # While the die reboots it NACKs its own address for a moment, and
            # the bridge reports that as ERR_HARDWARE_FAULT. The C driver reads
            # in a loop and ignores the status until MODEL_ID comes back, so a
            # NACK here is just "not booted yet", not a fault. Measured on Linux
            # 23.09.2026: 1-2 NACKs per reset on both VL53L0X boards, init
            # failed every time; on Windows the USB round trip is slow enough
            # to miss the window, which is why COM7 never showed it.
            while True:
                try:
                    byte = self.p.rd_byte(IDENTIFICATION_MODEL_ID)
                except ProtocolError:
                    byte = None
                if byte is not None and (byte == 0x00) == want_zero:
                    return
                if time.monotonic() > deadline:
                    raise Vl53Error('timeout in the soft reset of the sensor')

        self.p.wr_byte(SOFT_RESET_GO2_SOFT_RESET_N, 0x00)
        poll_model_id(True)
        self.p.sleep_ms(1)
        self.p.wr_byte(SOFT_RESET_GO2_SOFT_RESET_N, 0x01)
        poll_model_id(False)
        self.p.sleep_ms(1)

    def data_init(self):
        """VL53L0X_DataInit().

        The C function also calls GetDeviceParameters() to prime its parameter
        struct; the only field of it that outlives DataInit is the timing
        budget, which static_init() reads from the device itself. The I2C pad
        is left in its default 1.8 V mode (USE_I2C_2V8 is not defined in the
        reference build either) — the same setting the VL53L4CD boards of this
        family run with.
        """
        self.p.wr_byte(0x88, 0x00)              # I2C standard mode
        d = self.d
        d['ReadDataFromDeviceDone'] = 0
        d['LinearityCorrectiveGain'] = 1000
        d['OscFrequencyMHz'] = 618660
        d['XTalkCompensationRateMegaCps'] = 0
        d['XTalkCompensationEnable'] = 0
        d['DeviceMode'] = DEVICEMODE_SINGLE_RANGING

        self.p.wr_byte(0x80, 0x01)
        self.p.wr_byte(0xFF, 0x01)
        self.p.wr_byte(0x00, 0x00)
        d['StopVariable'] = self.p.rd_byte(0x91)
        self.p.wr_byte(0x00, 0x01)
        self.p.wr_byte(0xFF, 0x00)
        self.p.wr_byte(0x80, 0x00)

        for check in range(CHECK_NUMBER_OF_CHECKS):
            self.set_limit_check_enable(check, 1)

        self.set_limit_check_enable(CHECK_SIGNAL_REF_CLIP, 0)
        self.set_limit_check_enable(CHECK_RANGE_IGNORE_THRESHOLD, 0)
        self.set_limit_check_enable(CHECK_SIGNAL_RATE_MSRC, 0)
        self.set_limit_check_enable(CHECK_SIGNAL_RATE_PRE_RANGE, 0)

        self.set_limit_check_value(CHECK_SIGMA_FINAL_RANGE, 18 * 65536)
        self.set_limit_check_value(CHECK_SIGNAL_RATE_FINAL_RANGE, 25 * 65536 // 100)
        self.set_limit_check_value(CHECK_SIGNAL_REF_CLIP, 35 * 65536)
        self.set_limit_check_value(CHECK_RANGE_IGNORE_THRESHOLD, 0)

        d['SequenceConfig'] = 0xFF
        self.p.wr_byte(SYSTEM_SEQUENCE_CONFIG, 0xFF)

    def static_init(self):
        """VL53L0X_StaticInit(): reference SPADs, tuning settings, GPIO."""
        d = self.d
        self.get_info_from_device(1)

        count = d['ReferenceSpadCount']
        aperture_spads = d['ReferenceSpadType']
        # An out-of-range NVM record means the die was never factory-programmed
        # with one; the reference SPADs then have to be measured instead, which
        # sensor_init() does as soon as the rest of StaticInit is in place.
        self.ref_spads_from_nvm = not (
            aperture_spads > 1 or (aperture_spads == 1 and count > 32)
            or (aperture_spads == 0 and count > 12))
        if self.ref_spads_from_nvm:
            self.set_reference_spads(count, aperture_spads)

        self.load_tuning_settings(DEFAULT_TUNING_SETTINGS)

        self.set_gpio_config(GPIOFUNCTIONALITY_NEW_MEASURE_READY, polarity_low=True)

        self.p.wr_byte(0xFF, 0x01)
        tempword = self.p.rd_word(0x84)
        self.p.wr_byte(0xFF, 0x00)
        d['OscFrequencyMHz'] = tempword << 4        # FixPoint4.12 -> 16.16

        # Of GetDeviceParameters() only the timing budget survives the call, and
        # the sequence-step edits below re-apply it.
        d['MeasurementTimingBudgetMicroSeconds'] = \
            self.get_measurement_timing_budget()

        d['RangeFractionalEnable'] = self.p.rd_byte(SYSTEM_RANGE_CONFIG) & 1
        d['SequenceConfig'] = self.p.rd_byte(SYSTEM_SEQUENCE_CONFIG)

        # MSRC and TCC off by default, as in the C driver.
        self.set_sequence_step_enable(SEQUENCESTEP_TCC, 0)
        self.set_sequence_step_enable(SEQUENCESTEP_MSRC, 0)

        d['PreRangeVcselPulsePeriod'] = \
            self.get_vcsel_pulse_period(VCSEL_PERIOD_PRE_RANGE)
        d['FinalRangeVcselPulsePeriod'] = \
            self.get_vcsel_pulse_period(VCSEL_PERIOD_FINAL_RANGE)
        d['PreRangeTimeoutMicroSecs'] = \
            self.get_sequence_step_timeout(SEQUENCESTEP_PRE_RANGE)
        d['FinalRangeTimeoutMicroSecs'] = \
            self.get_sequence_step_timeout(SEQUENCESTEP_FINAL_RANGE)

    # ── reference calibration (VHV / phase) ──
    def perform_single_ref_calibration(self, vhv_init_byte: int):
        self.p.wr_byte(SYSRANGE_START, SYSRANGE_MODE_START_STOP | vhv_init_byte)
        self.wait_data_ready()          # measurement_poll_for_completion()
        self.clear_interrupt()
        self.p.wr_byte(SYSRANGE_START, 0x00)

    def ref_calibration_io_read(self, vhv_enable: bool, phase_enable: bool) -> tuple:
        """VL53L0X_ref_calibration_io() in its read direction -> (vhv, phase)."""
        self.p.wr_byte(0xFF, 0x01)
        self.p.wr_byte(0x00, 0x00)
        self.p.wr_byte(0xFF, 0x00)
        vhv = self.p.rd_byte(0xCB) if vhv_enable else 0
        phase = self.p.rd_byte(0xEE) if phase_enable else 0
        self.p.wr_byte(0xFF, 0x01)
        self.p.wr_byte(0x00, 0x01)
        self.p.wr_byte(0xFF, 0x00)
        return vhv, phase & 0xEF

    def perform_ref_calibration(self) -> tuple:
        """VL53L0X_PerformRefCalibration(): VHV, then phase -> (vhv, phase).

        Not optional in practice: without it the VCSEL bias is wrong and every
        frame comes back with signal 0 and device status 4 (measured on the
        VL53L0X board, COM7). The C driver leaves it to the application, and
        every ST example calls it right after StaticInit — so does sensor_init().
        """
        sequence_config = self.d['SequenceConfig']

        self.p.wr_byte(SYSTEM_SEQUENCE_CONFIG, 0x01)        # VHV only
        self.perform_single_ref_calibration(0x40)
        vhv, _ = self.ref_calibration_io_read(True, False)

        self.p.wr_byte(SYSTEM_SEQUENCE_CONFIG, 0x02)        # phase only
        self.perform_single_ref_calibration(0x00)
        _, phase = self.ref_calibration_io_read(False, True)

        self.p.wr_byte(SYSTEM_SEQUENCE_CONFIG, sequence_config)
        self.d['SequenceConfig'] = sequence_config
        return vhv, phase

    # ── NVM ──
    def device_read_strobe(self):
        self.p.wr_byte(0x83, 0x00)
        for _ in range(DEFAULT_MAX_LOOP):
            if self.p.rd_byte(0x83) != 0x00:
                break
        else:
            raise Vl53Error('timeout waiting for the NVM read strobe')
        self.p.wr_byte(0x83, 0x01)

    def get_info_from_device(self, option: int):
        """VL53L0X_get_info_from_device(): reads the fuse copy of the reference
        SPAD record (option 1), the product identification (option 2) and the
        part UID plus the factory offset (option 4)."""
        d = self.d
        done = d['ReadDataFromDeviceDone']
        if done == 7:
            return
        p = self.p

        p.wr_byte(0x80, 0x01)
        p.wr_byte(0xFF, 0x01)
        p.wr_byte(0x00, 0x00)

        p.wr_byte(0xFF, 0x06)
        p.wr_byte(0x83, p.rd_byte(0x83) | 4)
        p.wr_byte(0xFF, 0x07)
        p.wr_byte(0x81, 0x01)
        p.sleep_ms(1)                           # VL53L0X_PollingDelay()
        p.wr_byte(0x80, 0x01)

        def nvm_dword(index: int) -> int:
            p.wr_byte(0x94, index)
            self.device_read_strobe()
            return p.rd_dword(0x90)

        good_spad_map = list(d['RefGoodSpadMap'])
        ref_spad_count = ref_spad_type = 0
        product_id = ''
        module_id = revision = 0
        part_uid_upper = part_uid_lower = 0
        signal_rate_meas_1104_400mm = dist_meas_1104_400mm = 0

        if (option & 1) == 1 and (done & 1) == 0:
            tmp = nvm_dword(0x6B)
            ref_spad_count = (tmp >> 8) & 0x7F
            ref_spad_type = (tmp >> 15) & 0x01

            tmp = nvm_dword(0x24)
            good_spad_map[0] = (tmp >> 24) & 0xFF
            good_spad_map[1] = (tmp >> 16) & 0xFF
            good_spad_map[2] = (tmp >> 8) & 0xFF
            good_spad_map[3] = tmp & 0xFF

            tmp = nvm_dword(0x25)
            good_spad_map[4] = (tmp >> 24) & 0xFF
            good_spad_map[5] = (tmp >> 16) & 0xFF

        if (option & 2) == 2 and (done & 2) == 0:
            p.wr_byte(0x94, 0x02)
            self.device_read_strobe()
            module_id = p.rd_byte(0x90)

            p.wr_byte(0x94, 0x7B)
            self.device_read_strobe()
            revision = p.rd_byte(0x90)

            chars = []
            tmp = nvm_dword(0x77)
            chars += [(tmp >> 25) & 0x7F, (tmp >> 18) & 0x7F,
                      (tmp >> 11) & 0x7F, (tmp >> 4) & 0x7F]
            byte = (tmp & 0x00F) << 3

            tmp = nvm_dword(0x78)
            chars += [byte + ((tmp >> 29) & 0x7F), (tmp >> 22) & 0x7F,
                      (tmp >> 15) & 0x7F, (tmp >> 8) & 0x7F, (tmp >> 1) & 0x7F]
            byte = (tmp & 0x001) << 6

            tmp = nvm_dword(0x79)
            chars += [byte + ((tmp >> 26) & 0x7F), (tmp >> 19) & 0x7F,
                      (tmp >> 12) & 0x7F, (tmp >> 5) & 0x7F]
            byte = (tmp & 0x01F) << 2

            tmp = nvm_dword(0x7A)
            chars += [byte + ((tmp >> 30) & 0x7F), (tmp >> 23) & 0x7F,
                      (tmp >> 16) & 0x7F, (tmp >> 9) & 0x7F, (tmp >> 2) & 0x7F]
            product_id = ''.join(chr(c & 0x7F) for c in chars)

        if (option & 4) == 4 and (done & 4) == 0:
            part_uid_upper = nvm_dword(0x7B)
            part_uid_lower = nvm_dword(0x7C)

            signal_rate_meas_1104_400mm = (nvm_dword(0x73) & 0xFF) << 8
            signal_rate_meas_1104_400mm |= (nvm_dword(0x74) & 0xFF000000) >> 24
            dist_meas_1104_400mm = (nvm_dword(0x75) & 0xFF) << 8
            dist_meas_1104_400mm |= (nvm_dword(0x76) & 0xFF000000) >> 24

        p.wr_byte(0x81, 0x00)
        p.wr_byte(0xFF, 0x06)
        p.wr_byte(0x83, p.rd_byte(0x83) & 0xFB)
        p.wr_byte(0xFF, 0x01)
        p.wr_byte(0x00, 0x01)
        p.wr_byte(0xFF, 0x00)
        p.wr_byte(0x80, 0x00)

        if (option & 1) == 1 and (done & 1) == 0:
            d['ReferenceSpadCount'] = ref_spad_count
            d['ReferenceSpadType'] = ref_spad_type
            d['RefGoodSpadMap'] = good_spad_map

        if (option & 2) == 2 and (done & 2) == 0:
            d['ModuleId'] = module_id
            d['Revision'] = revision
            d['ProductId'] = product_id

        if (option & 4) == 4 and (done & 4) == 0:
            d['PartUIDUpper'] = part_uid_upper
            d['PartUIDLower'] = part_uid_lower
            d['SignalRateMeasFixed400mm'] = signal_rate_meas_1104_400mm << 9
            offset_um = 0
            if dist_meas_1104_400mm != 0:
                offset_1104_mm = _u32(dist_meas_1104_400mm - (400 << 4))
                offset_um = -((offset_1104_mm * 1000) >> 4)
            d['Part2PartOffsetAdjustmentNVMMicroMeter'] = offset_um

        d['ReadDataFromDeviceDone'] = done | option

    # ── reference SPADs ──
    def set_reference_spads(self, count: int, is_aperture_spads: int):
        """VL53L0X_set_reference_spads(): apply the NVM record through the good
        SPAD map and verify it read back."""
        start_select = 0xB4
        spad_array_size = REF_SPAD_BUFFER_SIZE
        max_spad_count = 44

        self.p.wr_byte(0xFF, 0x01)
        self.p.wr_byte(DYNAMIC_SPAD_REF_EN_START_OFFSET, 0x00)
        self.p.wr_byte(DYNAMIC_SPAD_NUM_REQUESTED_REF_SPAD, 0x2C)
        self.p.wr_byte(0xFF, 0x00)
        self.p.wr_byte(GLOBAL_CONFIG_REF_EN_START_SELECT, start_select)

        spad_array = [0] * spad_array_size
        current_spad_index = 0
        if is_aperture_spads:
            while (not is_aperture(start_select + current_spad_index)
                   and current_spad_index < max_spad_count):
                current_spad_index += 1

        good = self.d['RefGoodSpadMap']
        for _ in range(count):
            next_good = get_next_good_spad(good, spad_array_size, current_spad_index)
            if next_good == -1:
                raise Vl53Error('ran out of good reference SPADs')
            if is_aperture(start_select + next_good) != bool(is_aperture_spads):
                raise Vl53Error('the good SPAD map leaves the requested quadrant')
            current_spad_index = next_good
            spad_array[current_spad_index // 8] |= 1 << (current_spad_index % 8)
            current_spad_index += 1

        self.p.wr_multi(GLOBAL_CONFIG_SPAD_ENABLES_REF_0, bytes(spad_array))
        check = self.p.rd_multi(GLOBAL_CONFIG_SPAD_ENABLES_REF_0, spad_array_size)
        if bytes(spad_array) != check:
            raise Vl53Error('reference SPAD map did not read back')

        self.d['RefSpadEnables'] = spad_array
        self.d['ReferenceSpadCount'] = count
        self.d['ReferenceSpadType'] = is_aperture_spads

    # ── tuning settings ──
    def load_tuning_settings(self, buffer: bytes):
        """VL53L0X_load_tuning_settings(): {count, address, bytes...} records,
        with count 0xFF carrying a host-side sigma parameter instead."""
        index = 0
        while buffer[index] != 0:
            number_of_writes = buffer[index]
            index += 1
            if number_of_writes == 0xFF:
                # SigmaEstRefArray / EffPulseWidth / EffAmbWidth / targetRefRate:
                # host-side sigma parameters this port does not use (the sigma
                # estimate of the 1.0.4 driver has them as constants).
                index += 3
            elif number_of_writes <= 4:
                address = buffer[index]
                index += 1
                self.p.wr_multi(address, buffer[index:index + number_of_writes])
                index += number_of_writes
            else:
                raise Vl53Error(f'bad tuning record at offset {index}')

    # ── GPIO / interrupt ──
    def set_gpio_config(self, functionality: int, polarity_low: bool = True):
        """VL53L0X_SetGpioConfig() for pin 0 in its ranging-interrupt role."""
        self.p.wr_byte(SYSTEM_INTERRUPT_CONFIG_GPIO, functionality)
        data = 0x00 if polarity_low else 0x10
        current = self.p.rd_byte(GPIO_HV_MUX_ACTIVE_HIGH)
        self.p.wr_byte(GPIO_HV_MUX_ACTIVE_HIGH, (current & 0xEF) | data)
        self.d['Pin0GpioFunctionality'] = functionality
        self.clear_interrupt()

    def clear_interrupt(self):
        """VL53L0X_ClearInterruptMask(): two writes, then confirm — up to three
        rounds. The streamed slot plays the two writes only; this is the polled
        path and the arming/teardown path."""
        for _ in range(3):
            self.p.wr_byte(SYSTEM_INTERRUPT_CLEAR, 0x01)
            self.p.wr_byte(SYSTEM_INTERRUPT_CLEAR, 0x00)
            if (self.p.rd_byte(RESULT_INTERRUPT_STATUS) & 0x07) == 0x00:
                return
        raise Vl53Error('interrupt not cleared after three rounds')

    def get_interrupt_mask_status(self) -> int:
        byte = self.p.rd_byte(RESULT_INTERRUPT_STATUS)
        if byte & 0x18:
            raise Vl53Error(f'range error reported in RESULT_INTERRUPT_STATUS '
                            f'0x{byte:02X}')
        return byte & 0x07

    def check_for_data_ready(self) -> bool:
        if self.d['Pin0GpioFunctionality'] == GPIOFUNCTIONALITY_NEW_MEASURE_READY:
            return self.get_interrupt_mask_status() == \
                GPIOFUNCTIONALITY_NEW_MEASURE_READY
        return bool(self.p.rd_byte(RESULT_RANGE_STATUS) & 0x01)

    # ── sequence steps ──
    def get_sequence_step_enables(self) -> dict:
        config = self.p.rd_byte(SYSTEM_SEQUENCE_CONFIG)
        return {step: bool(config & mask) for step, mask in _SEQ_TEST.items()}

    def set_sequence_step_enable(self, step: int, enabled: int):
        config = self.p.rd_byte(SYSTEM_SEQUENCE_CONFIG)
        new = (config | _SEQ_SET[step]) if enabled else (config & _SEQ_CLEAR[step])
        if new == config:
            return
        self.p.wr_byte(SYSTEM_SEQUENCE_CONFIG, new)
        self.d['SequenceConfig'] = new
        # The budget is spread over the enabled steps, so it has to be re-applied.
        self.set_measurement_timing_budget(
            self.d['MeasurementTimingBudgetMicroSeconds'])

    def get_vcsel_pulse_period(self, period_type: int) -> int:
        reg = (PRE_RANGE_CONFIG_VCSEL_PERIOD
               if period_type == VCSEL_PERIOD_PRE_RANGE
               else FINAL_RANGE_CONFIG_VCSEL_PERIOD)
        return decode_vcsel_period(self.p.rd_byte(reg))

    def get_sequence_step_timeout(self, step: int) -> int:
        if step in (SEQUENCESTEP_TCC, SEQUENCESTEP_DSS, SEQUENCESTEP_MSRC):
            vcsel = self.get_vcsel_pulse_period(VCSEL_PERIOD_PRE_RANGE)
            mclks = decode_timeout(self.p.rd_byte(MSRC_CONFIG_TIMEOUT_MACROP))
            return calc_timeout_us(mclks, vcsel)

        if step == SEQUENCESTEP_PRE_RANGE:
            vcsel = self.get_vcsel_pulse_period(VCSEL_PERIOD_PRE_RANGE)
            mclks = decode_timeout(
                self.p.rd_word(PRE_RANGE_CONFIG_TIMEOUT_MACROP_HI))
            return calc_timeout_us(mclks, vcsel)

        if step == SEQUENCESTEP_FINAL_RANGE:
            steps = self.get_sequence_step_enables()
            pre_mclks = 0
            if steps[SEQUENCESTEP_PRE_RANGE]:
                pre_mclks = decode_timeout(
                    self.p.rd_word(PRE_RANGE_CONFIG_TIMEOUT_MACROP_HI))
            vcsel = self.get_vcsel_pulse_period(VCSEL_PERIOD_FINAL_RANGE)
            final_mclks = decode_timeout(
                self.p.rd_word(FINAL_RANGE_CONFIG_TIMEOUT_MACROP_HI))
            return calc_timeout_us((final_mclks - pre_mclks) & 0xFFFF, vcsel)

        raise Vl53Error(f'no timeout for sequence step {step}')

    def set_sequence_step_timeout(self, step: int, timeout_us: int):
        if step in (SEQUENCESTEP_TCC, SEQUENCESTEP_DSS, SEQUENCESTEP_MSRC):
            vcsel = self.get_vcsel_pulse_period(VCSEL_PERIOD_PRE_RANGE)
            mclks = calc_timeout_mclks(timeout_us, vcsel)
            encoded = 255 if mclks > 256 else (mclks - 1) & 0xFF
            self.p.wr_byte(MSRC_CONFIG_TIMEOUT_MACROP, encoded)
        elif step == SEQUENCESTEP_PRE_RANGE:
            vcsel = self.get_vcsel_pulse_period(VCSEL_PERIOD_PRE_RANGE)
            mclks = calc_timeout_mclks(timeout_us, vcsel)
            self.p.wr_word(PRE_RANGE_CONFIG_TIMEOUT_MACROP_HI,
                           encode_timeout(mclks))
            self.d['PreRangeTimeoutMicroSecs'] = timeout_us
        elif step == SEQUENCESTEP_FINAL_RANGE:
            # The final-range timeout register carries pre-range + final range,
            # and the two steps run at different VCSEL periods, so the sum is
            # taken in macro periods.
            steps = self.get_sequence_step_enables()
            pre_mclks = 0
            if steps[SEQUENCESTEP_PRE_RANGE]:
                pre_mclks = decode_timeout(
                    self.p.rd_word(PRE_RANGE_CONFIG_TIMEOUT_MACROP_HI))
            vcsel = self.get_vcsel_pulse_period(VCSEL_PERIOD_FINAL_RANGE)
            mclks = calc_timeout_mclks(timeout_us, vcsel) + pre_mclks
            self.p.wr_word(FINAL_RANGE_CONFIG_TIMEOUT_MACROP_HI,
                           encode_timeout(mclks))
            self.d['FinalRangeTimeoutMicroSecs'] = timeout_us
        else:
            raise Vl53Error(f'no timeout for sequence step {step}')

    # ── timing budget ──
    # Fixed overheads of the scheduler, in microseconds
    # (set_measurement_timing_budget_micro_seconds()).
    _START_OVERHEAD_US       = 1910
    _END_OVERHEAD_US         = 960
    _MSRC_OVERHEAD_US        = 660
    _TCC_OVERHEAD_US         = 590
    _DSS_OVERHEAD_US         = 690
    _PRE_RANGE_OVERHEAD_US   = 660
    _FINAL_RANGE_OVERHEAD_US = 550

    def set_measurement_timing_budget(self, budget_us: int):
        steps = self.get_sequence_step_enables()
        final_budget_us = budget_us - (self._START_OVERHEAD_US
                                       + self._END_OVERHEAD_US)

        def take(sub_timeout):
            nonlocal final_budget_us
            if sub_timeout >= final_budget_us:
                raise Vl53Error(f'timing budget {budget_us} us is too small for '
                                'the enabled sequence steps')
            final_budget_us -= sub_timeout

        if steps[SEQUENCESTEP_TCC] or steps[SEQUENCESTEP_MSRC] or steps[SEQUENCESTEP_DSS]:
            # TCC, MSRC and DSS share one timeout.
            msrc_us = self.get_sequence_step_timeout(SEQUENCESTEP_MSRC)
            if steps[SEQUENCESTEP_TCC]:
                take(msrc_us + self._TCC_OVERHEAD_US)
            if steps[SEQUENCESTEP_DSS]:
                take(2 * (msrc_us + self._DSS_OVERHEAD_US))
            elif steps[SEQUENCESTEP_MSRC]:
                take(msrc_us + self._MSRC_OVERHEAD_US)

        if steps[SEQUENCESTEP_PRE_RANGE]:
            pre_us = self.get_sequence_step_timeout(SEQUENCESTEP_PRE_RANGE)
            take(pre_us + self._PRE_RANGE_OVERHEAD_US)

        if steps[SEQUENCESTEP_FINAL_RANGE]:
            # Whatever is left goes to the final range.
            final_budget_us -= self._FINAL_RANGE_OVERHEAD_US
            self.set_sequence_step_timeout(SEQUENCESTEP_FINAL_RANGE,
                                           final_budget_us)
            self.d['MeasurementTimingBudgetMicroSeconds'] = budget_us

    def get_measurement_timing_budget(self) -> int:
        steps = self.get_sequence_step_enables()
        budget_us = self._START_OVERHEAD_US + self._END_OVERHEAD_US

        if steps[SEQUENCESTEP_TCC] or steps[SEQUENCESTEP_MSRC] or steps[SEQUENCESTEP_DSS]:
            msrc_us = self.get_sequence_step_timeout(SEQUENCESTEP_MSRC)
            if steps[SEQUENCESTEP_TCC]:
                budget_us += msrc_us + self._TCC_OVERHEAD_US
            if steps[SEQUENCESTEP_DSS]:
                budget_us += 2 * (msrc_us + self._DSS_OVERHEAD_US)
            elif steps[SEQUENCESTEP_MSRC]:
                budget_us += msrc_us + self._MSRC_OVERHEAD_US

        if steps[SEQUENCESTEP_PRE_RANGE]:
            budget_us += (self.get_sequence_step_timeout(SEQUENCESTEP_PRE_RANGE)
                          + self._PRE_RANGE_OVERHEAD_US)
        if steps[SEQUENCESTEP_FINAL_RANGE]:
            budget_us += (self.get_sequence_step_timeout(SEQUENCESTEP_FINAL_RANGE)
                          + self._FINAL_RANGE_OVERHEAD_US)

        self.d['MeasurementTimingBudgetMicroSeconds'] = budget_us
        return budget_us

    def set_inter_measurement_period(self, period_ms: int):
        osc_calibrate_val = self.p.rd_word(OSC_CALIBRATE_VAL)
        value = period_ms * osc_calibrate_val if osc_calibrate_val else period_ms
        self.p.wr_dword(SYSTEM_INTERMEASUREMENT_PERIOD, value)

    def get_inter_measurement_period(self) -> int:
        osc_calibrate_val = self.p.rd_word(OSC_CALIBRATE_VAL)
        value = self.p.rd_dword(SYSTEM_INTERMEASUREMENT_PERIOD)
        return value // osc_calibrate_val if osc_calibrate_val else value

    # ── the family-wide timing entry point ──
    def set_range_timing(self, timing_budget_ms: int, inter_measurement_ms: int):
        """Timing budget and inter-measurement period, the way the CLI and GUI
        of this family ask for them: inter-measurement 0 means back-to-back
        continuous ranging, anything else the timed continuous mode."""
        self.set_measurement_timing_budget(timing_budget_ms * 1000)
        self.set_inter_measurement_period(inter_measurement_ms)
        self.set_device_mode(DEVICEMODE_CONTINUOUS_RANGING
                             if inter_measurement_ms == 0
                             else DEVICEMODE_CONTINUOUS_TIMED)

    def get_range_timing(self) -> tuple:
        """-> (timing_budget_ms, inter_measurement_ms)."""
        return (self.get_measurement_timing_budget() // 1000,
                self.get_inter_measurement_period())

    # ── ranging ──
    def set_device_mode(self, mode: int):
        if mode not in (DEVICEMODE_SINGLE_RANGING, DEVICEMODE_CONTINUOUS_RANGING,
                        DEVICEMODE_CONTINUOUS_TIMED):
            raise Vl53Error(f'device mode {mode} is not supported')
        self.d['DeviceMode'] = mode
        if mode != DEVICEMODE_SINGLE_RANGING:
            self.d['ContinuousMode'] = mode

    def arm_stop_variable(self):
        """The undocumented prologue every VL53L0X_StartMeasurement() opens
        with: hand the stop variable read by DataInit back to the device."""
        self.p.wr_byte(0x80, 0x01)
        self.p.wr_byte(0xFF, 0x01)
        self.p.wr_byte(0x00, 0x00)
        self.p.wr_byte(0x91, self.d['StopVariable'])
        self.p.wr_byte(0x00, 0x01)
        self.p.wr_byte(0xFF, 0x00)
        self.p.wr_byte(0x80, 0x00)

    def start_ranging(self):
        """VL53L0X_StartMeasurement() for the two continuous modes.

        A single shot (a calibration, a poll) leaves the device mode on single
        ranging; the continuous mode chosen before it is resumed.
        """
        if (self.d['DeviceMode'] == DEVICEMODE_SINGLE_RANGING
                and self.d['ContinuousMode'] is not None):
            self.set_device_mode(self.d['ContinuousMode'])
        self.arm_stop_variable()

        mode = self.d['DeviceMode']
        if mode == DEVICEMODE_CONTINUOUS_RANGING:
            self.p.wr_byte(SYSRANGE_START, SYSRANGE_MODE_BACKTOBACK)
        elif mode == DEVICEMODE_CONTINUOUS_TIMED:
            self.p.wr_byte(SYSRANGE_START, SYSRANGE_MODE_TIMED)
        else:
            raise Vl53Error('start_ranging() needs a continuous device mode')

    def stop_ranging(self):
        """VL53L0X_StopMeasurement()."""
        self.p.wr_byte(SYSRANGE_START, SYSRANGE_MODE_SINGLESHOT)
        self.p.wr_byte(0xFF, 0x01)
        self.p.wr_byte(0x00, 0x00)
        self.p.wr_byte(0x91, 0x00)
        self.p.wr_byte(0x00, 0x01)
        self.p.wr_byte(0xFF, 0x00)

    def perform_single_ranging_measurement(self) -> RangingMeasurementData:
        """VL53L0X_PerformSingleRangingMeasurement(): one shot, start to finish.

        The calibrations below are built on it, and it leaves the device mode on
        single ranging — start_ranging() sets the mode it needs anyway.
        """
        self.set_device_mode(DEVICEMODE_SINGLE_RANGING)
        self.arm_stop_variable()
        self.p.wr_byte(SYSRANGE_START, 0x01)
        for _ in range(DEFAULT_MAX_LOOP):
            if not (self.p.rd_byte(SYSRANGE_START) & SYSRANGE_MODE_START_STOP):
                break
        else:
            raise Vl53Error('the single-shot start bit never cleared')
        self.wait_data_ready()
        data = self.get_result()
        self.clear_interrupt()
        return data

    def get_result(self) -> RangingMeasurementData:
        return self.parse_result_block(
            self.p.rd_multi(RESULT_BLOCK_ADDR, RESULT_BLOCK_LEN))

    # ── result decoding ──
    def parse_result_block(self, raw: bytes) -> RangingMeasurementData:
        """VL53L0X_GetRangingMeasurementData() on the 12 bytes at 0x14.

        Pure arithmetic over the block and the cached device data — no register
        access, so a streamed frame decodes exactly like a polled one. The two
        limit checks that would read the sensor (signal-reference clip and the
        range-ignore threshold) are the ones DataInit leaves disabled.
        """
        if len(raw) < RESULT_BLOCK_LEN:
            raise Vl53Error(f'result block too short: {len(raw)} bytes')

        distance = (raw[10] << 8) + raw[11]
        signal_rate_mcps = ((raw[6] << 8) + raw[7]) << 9        # 9.7 -> 16.16
        ambient_rate_mcps = ((raw[8] << 8) + raw[9]) << 9
        effective_spads = (raw[2] << 8) + raw[3]                # 8.8 format
        device_range_status = raw[0]

        # LinearityCorrectiveGain is 1000 unless SetLinearityCorrectiveGain()
        # says otherwise, and the crosstalk correction of the C driver hangs off
        # that branch; this port never leaves the default.
        if self.d['RangeFractionalEnable']:
            range_mm = distance >> 2
            fractional = (distance & 0x03) << 6
        else:
            range_mm = distance
            fractional = 0

        data = RangingMeasurementData(
            distance_mm=range_mm,
            range_fractional_part=fractional,
            signal_rate_mcps=signal_rate_mcps,
            ambient_rate_mcps=ambient_rate_mcps,
            effective_spad_rtn_count=effective_spads,
            device_range_status=device_range_status,
        )
        self.get_pal_range_status(data)
        return data

    def get_total_xtalk_rate(self, data: RangingMeasurementData) -> int:
        if not self.d['XTalkCompensationEnable']:
            return 0
        total = data.effective_spad_rtn_count * self.d['XTalkCompensationRateMegaCps']
        return (total + 0x80) >> 8

    def calc_sigma_estimate(self, data: RangingMeasurementData) -> int:
        """VL53L0X_calc_sigma_estimate(), FixPoint16.16 in and out.

        The C function branches on pRangingMeasurementData->RangeStatus, which
        its caller has not filled in yet; a zero-initialised struct — what the
        ST examples pass — takes the computing branch, and so does this port.
        """
        c_pulse_effective_width_centi_ns   = 800
        c_ambient_effective_width_centi_ns = 600
        c_dflt_final_range_integration_time_ms = 0x00190000      # 25 ms
        c_vcsel_pulse_width_ps = 4700
        c_sigma_est_max        = 0x028F87AE
        c_sigma_est_rtn_max    = 0xF000
        c_amb_to_signal_ratio_max = 0xF0000000 // c_ambient_effective_width_centi_ns
        c_tof_per_mm_ps        = 0x0006999A
        c_16bit_rounding_param = 0x00008000
        c_max_xtalk_kcps       = 0x00320000
        c_pll_period_ps        = 1655

        ambient_rate_kcps = (data.ambient_rate_mcps * 1000) >> 16
        xtalk_comp_rate_mcps = self.get_total_xtalk_rate(data)
        total_signal_rate_mcps = data.signal_rate_mcps + xtalk_comp_rate_mcps

        peak_signal_rate_kcps = ((total_signal_rate_mcps * 1000) + 0x8000) >> 16
        xtalk_comp_rate_kcps = min(xtalk_comp_rate_mcps * 1000, c_max_xtalk_kcps)

        final_range_timeout_us = self.d['FinalRangeTimeoutMicroSecs']
        final_range_vcsel_pclks = self.d['FinalRangeVcselPulsePeriod']
        final_range_macro_pclks = calc_timeout_mclks(final_range_timeout_us,
                                                     final_range_vcsel_pclks)
        pre_range_timeout_us = self.d['PreRangeTimeoutMicroSecs']
        pre_range_vcsel_pclks = self.d['PreRangeVcselPulsePeriod']
        pre_range_macro_pclks = calc_timeout_mclks(pre_range_timeout_us,
                                                   pre_range_vcsel_pclks)

        vcsel_width = 2 if final_range_vcsel_pclks == 8 else 3
        peak_vcsel_duration_us = vcsel_width * 2048 * (pre_range_macro_pclks
                                                       + final_range_macro_pclks)
        peak_vcsel_duration_us = (peak_vcsel_duration_us + 500) // 1000
        peak_vcsel_duration_us *= c_pll_period_ps
        peak_vcsel_duration_us = (peak_vcsel_duration_us + 500) // 1000

        total_signal_rate_2408 = (total_signal_rate_mcps + 0x80) >> 8
        vcsel_total_events_rtn = \
            ((total_signal_rate_2408 * peak_vcsel_duration_us) + 0x80) >> 8

        if peak_signal_rate_kcps == 0:
            return c_sigma_est_max

        vcsel_total_events_rtn = max(1, vcsel_total_events_rtn)

        sigma_estimate_p1 = c_pulse_effective_width_centi_ns
        sigma_estimate_p2 = (ambient_rate_kcps << 16) // peak_signal_rate_kcps
        sigma_estimate_p2 = min(sigma_estimate_p2, c_amb_to_signal_ratio_max)
        sigma_estimate_p2 *= c_ambient_effective_width_centi_ns
        sigma_estimate_p3 = 2 * isqrt(vcsel_total_events_rtn * 12)

        delta_t_ps = data.distance_mm * c_tof_per_mm_ps

        diff1_mcps = _u32(_u32((peak_signal_rate_kcps << 16)
                               - 2 * xtalk_comp_rate_kcps) + 500) // 1000
        diff2_mcps = ((peak_signal_rate_kcps << 16) + 500) // 1000
        diff1_mcps <<= 8
        xtalk_correction = abs(diff1_mcps // diff2_mcps) << 8

        pw_mult = delta_t_ps // c_vcsel_pulse_width_ps
        pw_mult = _u32(pw_mult * _u32((1 << 16) - xtalk_correction))
        pw_mult = (pw_mult + c_16bit_rounding_param) >> 16
        pw_mult += 1 << 16
        # Squaring 1.xx would leave 32 bits, so the C driver halves it first.
        pw_mult >>= 1
        pw_mult = (pw_mult * pw_mult) >> 14

        sqr1 = ((pw_mult * sigma_estimate_p1) + 0x8000) >> 16
        sqr1 *= sqr1
        sqr2 = (sigma_estimate_p2 + 0x8000) >> 16
        sqr2 *= sqr2

        sqrt_result_centi_ns = isqrt(sqr1 + sqr2) << 16
        sigma_est_rtn = ((sqrt_result_centi_ns + 50) // 100) // sigma_estimate_p3
        sigma_est_rtn *= SPEED_OF_LIGHT_IN_AIR
        sigma_est_rtn = (sigma_est_rtn + 5000) // 10000
        sigma_est_rtn = min(sigma_est_rtn, c_sigma_est_rtn_max)

        final_range_integration_time_ms = (final_range_timeout_us
                                           + pre_range_timeout_us + 500) // 1000
        # 1 mm * 25 ms / the actual integration time.
        sigma_est_ref = isqrt((c_dflt_final_range_integration_time_ms
                               + final_range_integration_time_ms // 2)
                              // final_range_integration_time_ms) << 8
        sigma_est_ref = (sigma_est_ref + 500) // 1000

        sigma_estimate = 1000 * isqrt(sigma_est_rtn * sigma_est_rtn
                                      + sigma_est_ref * sigma_est_ref)
        if (peak_signal_rate_kcps < 1 or vcsel_total_events_rtn < 1
                or sigma_estimate > c_sigma_est_max):
            sigma_estimate = c_sigma_est_max
        return sigma_estimate

    def calc_dmax(self, amb_rate_meas: int) -> int:
        """VL53L0X_calc_dmax(): the ambient-rate -> max-range lookup with linear
        interpolation between its points."""
        amb = DMAX_LUT_AMB_RATE_MCPS
        dmax = DMAX_LUT_DMAX_MM
        if amb_rate_meas <= amb[0]:
            return dmax[0] >> 16
        if amb_rate_meas >= amb[-1]:
            return dmax[-1] >> 16

        index1 = next(i for i, a in enumerate(amb) if amb_rate_meas <= a)
        index0 = index1 - 1 if index1 else 0
        if index0 == index1:
            return dmax[index0] >> 16

        amb0, amb1 = amb[index0], amb[index1]
        dmax0, dmax1 = dmax[index0], dmax[index1]
        if amb1 == amb0:
            return dmax0 >> 16
        linear_slope = _u32(dmax0 - dmax1) // ((amb1 - amb0) >> 8)
        return (((amb1 - amb_rate_meas) >> 8) * linear_slope + dmax1) >> 16

    def get_pal_range_status(self, data: RangingMeasurementData):
        """VL53L0X_get_pal_range_status(): device status plus the enabled limit
        checks, mapped onto the PAL range status the whole family reports.

        DataInit leaves the signal-reference clip and the range-ignore threshold
        disabled, so only the sigma check runs here — and it is arithmetic over
        cached data, which keeps a streamed frame decodable offline.
        """
        d = self.d
        internal = (data.device_range_status & 0x78) >> 3
        none_flag = internal in (0, 5, 7, 12, 13, 14, 15)

        sigma_limit_flag = False
        if d['LimitChecksEnable'][CHECK_SIGMA_FINAL_RANGE]:
            sigma_estimate = self.calc_sigma_estimate(data)
            data.sigma_mm = sigma_estimate >> 16
            data.dmax_mm = self.calc_dmax(data.ambient_rate_mcps)
            sigma_limit_value = d['LimitChecksValue'][CHECK_SIGMA_FINAL_RANGE]
            sigma_limit_flag = (sigma_limit_value > 0
                                and sigma_estimate > sigma_limit_value)

        signal_ref_clip_flag = False        # check disabled by DataInit
        range_ignore_flag = False           # check disabled by DataInit

        if none_flag:
            status = 255
        elif internal in (1, 2, 3):
            status = 5                      # hardware fail
        elif internal in (6, 9):
            status = 4                      # phase fail
        elif internal in (8, 10) or signal_ref_clip_flag:
            status = 3                      # min range
        elif internal == 4 or range_ignore_flag:
            status = 2                      # signal fail
        elif sigma_limit_flag:
            status = 1                      # sigma fail
        else:
            status = 0
        data.range_status = status

    # ── VCSEL pulse period and the ranging profiles ──
    def set_vcsel_pulse_period(self, period_type: int, pclks: int):
        """VL53L0X_set_vcsel_pulse_period(): phase limits for the new period,
        then the period itself with its timeouts re-encoded around it.

        The timeouts are stored in macro periods, which the VCSEL period sets,
        so each one is read back in the old period and written in the new.
        """
        limits = {
            VCSEL_PERIOD_PRE_RANGE:   {12: 0x18, 14: 0x30, 16: 0x40, 18: 0x50},
            VCSEL_PERIOD_FINAL_RANGE: {8: 0x10, 10: 0x28, 12: 0x38, 14: 0x48},
        }
        if pclks % 2 or pclks not in limits.get(period_type, {}):
            kind = ('pre-range' if period_type == VCSEL_PERIOD_PRE_RANGE
                    else 'final range')
            raise Vl53Error(f'VCSEL period {pclks} PCLK is out of range for '
                            f'the {kind}')

        if period_type == VCSEL_PERIOD_PRE_RANGE:
            self.p.wr_byte(PRE_RANGE_CONFIG_VALID_PHASE_HIGH,
                           limits[period_type][pclks])
            self.p.wr_byte(PRE_RANGE_CONFIG_VALID_PHASE_LOW, 0x08)
        else:
            # The final range also retunes the VCSEL width and the phase-cal
            # limits; the pre-range does not.
            width, timeout, phasecal_lim = {8:  (0x02, 0x0C, 0x30),
                                            10: (0x03, 0x09, 0x20),
                                            12: (0x03, 0x08, 0x20),
                                            14: (0x03, 0x07, 0x20)}[pclks]
            self.p.wr_byte(FINAL_RANGE_CONFIG_VALID_PHASE_HIGH,
                           limits[period_type][pclks])
            self.p.wr_byte(FINAL_RANGE_CONFIG_VALID_PHASE_LOW, 0x08)
            self.p.wr_byte(GLOBAL_CONFIG_VCSEL_WIDTH, width)
            self.p.wr_byte(ALGO_PHASECAL_CONFIG_TIMEOUT, timeout)
            self.p.wr_byte(0xFF, 0x01)
            self.p.wr_byte(ALGO_PHASECAL_LIM, phasecal_lim)
            self.p.wr_byte(0xFF, 0x00)

        vcsel_period_reg = encode_vcsel_period(pclks)
        if period_type == VCSEL_PERIOD_PRE_RANGE:
            pre_timeout = self.get_sequence_step_timeout(SEQUENCESTEP_PRE_RANGE)
            msrc_timeout = self.get_sequence_step_timeout(SEQUENCESTEP_MSRC)
            self.p.wr_byte(PRE_RANGE_CONFIG_VCSEL_PERIOD, vcsel_period_reg)
            self.set_sequence_step_timeout(SEQUENCESTEP_PRE_RANGE, pre_timeout)
            self.set_sequence_step_timeout(SEQUENCESTEP_MSRC, msrc_timeout)
            self.d['PreRangeVcselPulsePeriod'] = pclks
        else:
            final_timeout = self.get_sequence_step_timeout(SEQUENCESTEP_FINAL_RANGE)
            self.p.wr_byte(FINAL_RANGE_CONFIG_VCSEL_PERIOD, vcsel_period_reg)
            self.set_sequence_step_timeout(SEQUENCESTEP_FINAL_RANGE, final_timeout)
            self.d['FinalRangeVcselPulsePeriod'] = pclks

        self.set_measurement_timing_budget(
            self.d['MeasurementTimingBudgetMicroSeconds'])
        self.perform_phase_calibration()

    def perform_phase_calibration(self):
        """VL53L0X_perform_phase_calibration() with restore_config = 1: what a
        VCSEL period change needs, without reading the result back."""
        sequence_config = self.d['SequenceConfig']
        self.p.wr_byte(SYSTEM_SEQUENCE_CONFIG, 0x02)
        self.perform_single_ref_calibration(0x00)
        self.p.wr_byte(SYSTEM_SEQUENCE_CONFIG, sequence_config)
        self.d['SequenceConfig'] = sequence_config

    def set_mode(self, name: str):
        """Apply one of MODES: the sigma and signal limits, the timing budget
        and both VCSEL periods, in the order the ST examples use them."""
        try:
            signal_mcps, sigma_mm, budget_us, pre_pclks, final_pclks = \
                MODE_SETTINGS[name]
        except KeyError:
            raise Vl53Error(f'no such mode: {name} '
                            f'(have {", ".join(self.MODES)})') from None

        self.set_limit_check_enable(CHECK_SIGMA_FINAL_RANGE, 1)
        self.set_limit_check_enable(CHECK_SIGNAL_RATE_FINAL_RANGE, 1)
        self.set_limit_check_value(CHECK_SIGNAL_RATE_FINAL_RANGE,
                                   int(signal_mcps * 65536))
        self.set_limit_check_value(CHECK_SIGMA_FINAL_RANGE, sigma_mm * 65536)
        self.set_measurement_timing_budget(budget_us)
        self.set_vcsel_pulse_period(VCSEL_PERIOD_PRE_RANGE, pre_pclks)
        self.set_vcsel_pulse_period(VCSEL_PERIOD_FINAL_RANGE, final_pclks)
        self._mode = name

    def get_mode(self) -> str:
        return self._mode

    def reach_mm(self):
        """Looked up by the final-range VCSEL period read from the sensor, so
        it follows the configuration, not the profile name. None for a period
        no profile here sets."""
        return REACH_BY_FINAL_PCLKS_MM.get(
            self.get_vcsel_pulse_period(VCSEL_PERIOD_FINAL_RANGE))

    def driver_info(self) -> dict:
        """What the five registers a mode writes currently read back as."""
        return {
            'signal_mcps': self.get_limit_check_value(
                CHECK_SIGNAL_RATE_FINAL_RANGE) / 65536.0,
            'sigma_mm': self.get_limit_check_value(
                CHECK_SIGMA_FINAL_RANGE) / 65536.0,
            'budget_us': self.get_measurement_timing_budget(),
            'pre_pclks': self.get_vcsel_pulse_period(VCSEL_PERIOD_PRE_RANGE),
            'final_pclks': self.get_vcsel_pulse_period(VCSEL_PERIOD_FINAL_RANGE),
        }

    # ── offset ──
    def set_offset_um(self, offset_um: int):
        """VL53L0X_set_offset_calibration_data_micro_meter(): the register is
        10.2 format in mm, so the value is stored in steps of 250 um."""
        offset_um = max(-512000, min(511000, offset_um))
        steps = abs(offset_um) // 250            # C truncates toward zero
        encoded = steps if offset_um >= 0 else 4096 - steps
        self.p.wr_word(ALGO_PART_TO_PART_RANGE_OFFSET_MM, encoded & 0xFFFF)

    def get_offset_um(self) -> int:
        register = self.p.rd_word(ALGO_PART_TO_PART_RANGE_OFFSET_MM) & 0x0FFF
        return (register - 4096) * 250 if register > 2047 else register * 250

    def set_offset(self, offset_mm: int):
        """The family-wide contract speaks whole millimetres; the register
        holds 250 um steps, so nothing is lost either way."""
        self.set_offset_um(offset_mm * 1000)

    def get_offset(self) -> int:
        return round(self.get_offset_um() / 1000.0)

    def perform_offset_calibration(self, cal_distance_mm: int,
                                   nb_samples: int = 50) -> int:
        """VL53L0X_perform_offset_calibration() -> the offset in micrometres.

        TCC and the range-ignore threshold are switched off for the run, as in
        the C driver; TCC is put back afterwards, the RIT is not (DataInit
        leaves it off anyway).
        """
        if cal_distance_mm <= 0:
            raise Vl53Error('the calibration distance must be positive')

        self.set_offset_um(0)
        tcc_was_on = self.get_sequence_step_enables()[SEQUENCESTEP_TCC]
        self.set_sequence_step_enable(SEQUENCESTEP_TCC, 0)
        self.set_limit_check_enable(CHECK_RANGE_IGNORE_THRESHOLD, 0)

        sum_ranging, count = 0, 0
        for _ in range(nb_samples):
            data = self.perform_single_ranging_measurement()
            if data.range_status == 0:
                sum_ranging = (sum_ranging + data.distance_mm) & 0xFFFF
                count += 1
        if count == 0:
            raise Vl53Error('offset calibration got no valid measurement - '
                            'check that a target is in front of the sensor')

        mean_mm = (2 * sum_ranging + count) // (2 * count)       # round half up
        if mean_mm == 0:
            # The device clamps a negative range at zero, so a target nearer
            # than the factory offset (~58 mm on the board of the bench) reads
            # 0 mm with the offset zeroed and every offset derived from it is
            # the target distance itself. ST calibrates at 100..400 mm.
            raise Vl53Error('the mean range came out 0 mm - the target is '
                            'closer than the part-to-part offset; move it out '
                            'to 100..400 mm and calibrate again')
        offset_um = (cal_distance_mm - mean_mm) * 1000
        self.set_offset_um(offset_um)

        if tcc_was_on:
            self.set_sequence_step_enable(SEQUENCESTEP_TCC, 1)
        return offset_um

    def calibrate_offset(self, target_dist_mm: int, nb_samples: int = 50) -> int:
        """The family-wide contract: an offset in whole millimetres."""
        return round(self.perform_offset_calibration(target_dist_mm,
                                                     nb_samples) / 1000.0)

    # ── crosstalk ──
    def set_xtalk_enable(self, enable: int):
        """VL53L0X_SetXTalkCompensationEnable(): switching it off zeroes the
        register but keeps the rate in the host-side copy."""
        rate = self.d['XTalkCompensationRateMegaCps'] if enable else 0
        self.p.wr_word(CROSSTALK_COMPENSATION_PEAK_RATE_MCPS,
                       (rate >> 3) & 0xFFFF)            # 16.16 -> 3.13
        self.d['XTalkCompensationEnable'] = 1 if enable else 0

    def set_xtalk_rate_mcps(self, rate_mcps: int):
        """VL53L0X_SetXTalkCompensationRateMegaCps(), FixPoint16.16 in."""
        if self.d['XTalkCompensationEnable']:
            self.p.wr_word(CROSSTALK_COMPENSATION_PEAK_RATE_MCPS,
                           (rate_mcps >> 3) & 0xFFFF)
        self.d['XTalkCompensationRateMegaCps'] = rate_mcps

    def get_xtalk_rate_mcps(self) -> int:
        return self.p.rd_word(CROSSTALK_COMPENSATION_PEAK_RATE_MCPS) << 3

    def set_xtalk(self, xtalk_kcps: int):
        """The family-wide contract speaks kcps; the ULD speaks Mcps 16.16."""
        self.set_xtalk_enable(1)
        self.set_xtalk_rate_mcps((xtalk_kcps << 16) // 1000)

    def get_xtalk(self) -> int:
        return (self.get_xtalk_rate_mcps() * 1000) >> 16

    def perform_xtalk_calibration(self, cal_distance_mm: int,
                                  nb_samples: int = 50) -> int:
        """VL53L0X_perform_xtalk_calibration() -> the rate in Mcps 16.16.

        The target sits at a known distance behind the cover glass; the
        crosstalk is the part of the mean signal per SPAD that the mean range
        does not account for. A mean range at or beyond the target means there
        is nothing to compensate and the rate comes out zero.
        """
        if cal_distance_mm <= 0:
            raise Vl53Error('the calibration distance must be positive')

        self.set_xtalk_enable(0)
        self.set_limit_check_enable(CHECK_RANGE_IGNORE_THRESHOLD, 0)

        sum_ranging, sum_signal, sum_spads, count = 0, 0, 0, 0
        for _ in range(nb_samples):
            data = self.perform_single_ranging_measurement()
            if data.range_status == 0:
                # The C sums are uint16 / uint32; kept that way on purpose.
                sum_ranging = (sum_ranging + data.distance_mm) & 0xFFFF
                sum_signal = _u32(sum_signal + data.signal_rate_mcps)
                sum_spads = (sum_spads
                             + data.effective_spad_rtn_count // 256) & 0xFFFF
                count += 1
        if count == 0:
            raise Vl53Error('crosstalk calibration got no valid measurement - '
                            'check that a target is in front of the sensor')

        mean_signal = sum_signal // count
        mean_range = _u32(sum_ranging << 16) // count
        mean_spads = _u32(sum_spads << 16) // count
        mean_spads_int = (mean_spads + 0x8000) >> 16
        cal_distance = cal_distance_mm << 16

        if mean_spads_int == 0 or mean_range >= cal_distance:
            rate_mcps = 0
        else:
            per_spad = mean_signal // mean_spads_int
            per_spad = _u32(per_spad
                            * ((1 << 16) - mean_range // cal_distance_mm))
            rate_mcps = (per_spad + 0x8000) >> 16

        self.set_xtalk_enable(1)
        self.set_xtalk_rate_mcps(rate_mcps)
        return rate_mcps

    def calibrate_xtalk(self, target_dist_mm: int, nb_samples: int = 50) -> int:
        """The family-wide contract: a crosstalk rate in kcps."""
        return (self.perform_xtalk_calibration(target_dist_mm,
                                               nb_samples) * 1000) >> 16

    # ── reference SPAD management ──
    def get_reference_spads(self) -> tuple:
        return self.d['ReferenceSpadCount'], self.d['ReferenceSpadType']

    def _set_ref_spad_map(self, spad_array: list):
        self.p.wr_multi(GLOBAL_CONFIG_SPAD_ENABLES_REF_0, bytes(spad_array))

    def _get_ref_spad_map(self) -> list:
        return list(self.p.rd_multi(GLOBAL_CONFIG_SPAD_ENABLES_REF_0,
                                    REF_SPAD_BUFFER_SIZE))

    def _enable_ref_spads(self, aperture_spads: int, spad_array: list,
                          start: int, offset: int, spad_count: int) -> int:
        """enable_ref_spads(): append @p spad_count good SPADs of the requested
        kind to @p spad_array, apply it and read it back -> the next index."""
        good = self.d['RefGoodSpadMap']
        current_spad = offset
        for _ in range(spad_count):
            next_good = get_next_good_spad(good, REF_SPAD_BUFFER_SIZE,
                                           current_spad)
            if next_good == -1:
                raise Vl53Error('ran out of good reference SPADs')
            if is_aperture(start + next_good) != bool(aperture_spads):
                raise Vl53Error('the good SPAD map leaves the requested quadrant')
            current_spad = next_good
            spad_array[current_spad // 8] |= 1 << (current_spad % 8)
            current_spad += 1

        self._set_ref_spad_map(spad_array)
        if self._get_ref_spad_map() != spad_array:
            raise Vl53Error('reference SPAD map did not read back')
        return current_spad

    def _perform_ref_signal_measurement(self) -> int:
        """perform_ref_signal_measurement(): one shot with only the reference
        steps enabled -> the peak reference signal rate in 9.7 format."""
        sequence_config = self.d['SequenceConfig']
        self.p.wr_byte(SYSTEM_SEQUENCE_CONFIG, 0xC0)
        self.perform_single_ranging_measurement()
        self.p.wr_byte(0xFF, 0x01)
        peak = self.p.rd_word(RESULT_PEAK_SIGNAL_RATE_REF)
        self.p.wr_byte(0xFF, 0x00)
        self.p.wr_byte(SYSTEM_SEQUENCE_CONFIG, sequence_config)
        self.d['SequenceConfig'] = sequence_config
        return peak

    def perform_ref_spad_management(self) -> tuple:
        """Re-measure the reference SPADs on a live session -> (count,
        is_aperture). Not in the C driver: the stop variable goes back first.

        stop_ranging() (VL53L0X_StopMeasurement) zeroes register 0x91, and the
        VHV/phase single shots this runs do not arm it the way every
        StartMeasurement does. Left at 0, the reference rate reads far too high,
        the aperture SPADs get picked and every frame after is Signal Fail
        (TB9BGETA6M, 30.09.2026: stop, then >150 ms, then this -> 12 aperture;
        re-arming first -> 5 non-aperture, as from the NVM). init() calls the
        bare routine: after the reset 0x91 already holds the stop variable.
        """
        self.arm_stop_variable()
        return self._ref_spad_management()

    def _ref_spad_management(self) -> tuple:
        """VL53L0X_perform_ref_spad_management() -> (count, is_aperture).

        What StaticInit normally takes from the NVM, measured instead: enable
        the minimum number of non-aperture reference SPADs, then add one at a
        time until the peak reference rate lands as close as it gets to 20 Mcps.
        If even the minimum overshoots, the run repeats on aperture SPADs. A
        board whose NVM reference-SPAD record is invalid needs this.
        """
        start_select = 0xB4
        minimum_spad_count = 3
        max_spad_count = 44
        target_ref_rate = self.d['TargetRefRate']

        spad_array = [0] * REF_SPAD_BUFFER_SIZE
        self.p.wr_byte(0xFF, 0x01)
        self.p.wr_byte(DYNAMIC_SPAD_REF_EN_START_OFFSET, 0x00)
        self.p.wr_byte(DYNAMIC_SPAD_NUM_REQUESTED_REF_SPAD, 0x2C)
        self.p.wr_byte(0xFF, 0x00)
        self.p.wr_byte(GLOBAL_CONFIG_REF_EN_START_SELECT, start_select)
        self.p.wr_byte(POWER_MANAGEMENT_GO1_POWER_FORCE, 0x00)
        self.perform_ref_calibration()

        need_apt_spads = 0
        current_spad_index = self._enable_ref_spads(
            need_apt_spads, spad_array, start_select, 0, minimum_spad_count)
        peak = self._perform_ref_signal_measurement()

        if peak > target_ref_rate:
            # Too bright even at the minimum: start over on aperture SPADs.
            spad_array = [0] * REF_SPAD_BUFFER_SIZE
            while (not is_aperture(start_select + current_spad_index)
                   and current_spad_index < max_spad_count):
                current_spad_index += 1
            need_apt_spads = 1
            current_spad_index = self._enable_ref_spads(
                need_apt_spads, spad_array, start_select, current_spad_index,
                minimum_spad_count)
            peak = self._perform_ref_signal_measurement()
            if peak > target_ref_rate:
                # Nothing more to give: the minimum aperture set is the answer.
                self._store_ref_spads(minimum_spad_count, 1, spad_array)
                return minimum_spad_count, 1

        ref_spad_count = minimum_spad_count
        last_spad_array = list(spad_array)
        last_diff = abs(peak - target_ref_rate)

        while peak < target_ref_rate:
            next_good = get_next_good_spad(self.d['RefGoodSpadMap'],
                                           REF_SPAD_BUFFER_SIZE,
                                           current_spad_index)
            if next_good == -1:
                raise Vl53Error('ran out of good reference SPADs')
            if is_aperture(start_select + next_good) != bool(need_apt_spads):
                break               # the quadrant is exhausted, stop here

            ref_spad_count += 1
            current_spad_index = next_good
            if current_spad_index // 8 >= REF_SPAD_BUFFER_SIZE:
                raise Vl53Error('reference SPAD index out of range')
            spad_array[current_spad_index // 8] |= 1 << (current_spad_index % 8)
            current_spad_index += 1
            self._set_ref_spad_map(spad_array)

            peak = self._perform_ref_signal_measurement()
            diff = abs(peak - target_ref_rate)
            if peak > target_ref_rate:
                if diff > last_diff:
                    # The previous map came closer to the target; go back to it.
                    self._set_ref_spad_map(last_spad_array)
                    spad_array = list(last_spad_array)
                    ref_spad_count -= 1
                break
            last_diff = diff
            last_spad_array = list(spad_array)

        self._store_ref_spads(ref_spad_count, need_apt_spads, spad_array)
        return ref_spad_count, need_apt_spads

    def _store_ref_spads(self, count: int, is_aperture_spads: int,
                         spad_array: list):
        self.d['RefSpadEnables'] = list(spad_array)
        self.d['ReferenceSpadCount'] = count
        self.d['ReferenceSpadType'] = is_aperture_spads

    # ── limit checks ──
    def set_limit_check_enable(self, check: int, enable: int):
        d = self.d
        if check >= CHECK_NUMBER_OF_CHECKS:
            raise Vl53Error(f'no limit check {check}')

        if enable == 0:
            value = 0
            disable = 1
        else:
            value = d['LimitChecksValue'][check]
            disable = 0

        if check == CHECK_SIGNAL_RATE_FINAL_RANGE:
            self.p.wr_word(FINAL_RANGE_CONFIG_MIN_COUNT_RATE_RTN_LIMIT,
                           (value >> 9) & 0xFFFF)
        elif check == CHECK_SIGNAL_RATE_MSRC:
            current = self.p.rd_byte(MSRC_CONFIG_CONTROL)
            self.p.wr_byte(MSRC_CONFIG_CONTROL, (current & 0xFE) | (disable << 1))
        elif check == CHECK_SIGNAL_RATE_PRE_RANGE:
            current = self.p.rd_byte(MSRC_CONFIG_CONTROL)
            self.p.wr_byte(MSRC_CONFIG_CONTROL, (current & 0xEF) | (disable << 4))
        # The other three checks are host-side arithmetic only.

        d['LimitChecksEnable'][check] = 0 if enable == 0 else 1

    def set_limit_check_value(self, check: int, value: int):
        d = self.d
        if not d['LimitChecksEnable'][check]:
            d['LimitChecksValue'][check] = value        # disabled: keep it here
            return

        if check == CHECK_SIGNAL_RATE_FINAL_RANGE:
            self.p.wr_word(FINAL_RANGE_CONFIG_MIN_COUNT_RATE_RTN_LIMIT,
                           (value >> 9) & 0xFFFF)
        elif check in (CHECK_SIGNAL_RATE_MSRC, CHECK_SIGNAL_RATE_PRE_RANGE):
            self.p.wr_word(PRE_RANGE_MIN_COUNT_RATE_RTN_LIMIT,
                           (value >> 9) & 0xFFFF)
        d['LimitChecksValue'][check] = value

    def get_limit_check_value(self, check: int) -> int:
        if check == CHECK_SIGNAL_RATE_FINAL_RANGE:
            return self.p.rd_word(FINAL_RANGE_CONFIG_MIN_COUNT_RATE_RTN_LIMIT) << 9
        if check in (CHECK_SIGNAL_RATE_MSRC, CHECK_SIGNAL_RATE_PRE_RANGE):
            return self.p.rd_word(PRE_RANGE_MIN_COUNT_RATE_RTN_LIMIT) << 9
        return self.d['LimitChecksValue'][check]


def _as_measurement(r: RangingMeasurementData) -> Measurement:
    """RangingMeasurementData -> the family-wide Measurement.

    The rates are FixPoint16.16 mega counts per second in the ULD and kcps
    everywhere in this tool, so they are scaled by 1000/65536 here.
    """
    return Measurement(
        distance_mm=r.distance_mm,
        status=r.range_status,
        status_text=r.status_text,
        signal_kcps=(r.signal_rate_mcps * 1000) >> 16,
        ambient_kcps=(r.ambient_rate_mcps * 1000) >> 16,
        sigma_mm=r.sigma_mm,
        spads=r.effective_spad_rtn_count >> 8,      # 8.8 format
        extra={'dmax_mm': r.dmax_mm,
               'device_range_status': r.device_range_status},
    )
