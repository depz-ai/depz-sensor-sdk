"""
uld/l1.py — VL53L1X ULD 3.5.5 in Python: VL53L1CX and VL53L1CB.

A faithful port of VL53L1X_api.c + VL53L1X_calibration.c — register sequences
and integer widths included. Do not "simplify" them.

One class serves both products: the two dies answer the same model id (0xEACC)
and the same register map, and it is the board name from the bootloader
metablock that tells them apart (uld/registry.py).

The die is the one uld/vl53l1_die.py describes, and `VL53L1Die` is its body, so
the register map, the result block, boot and reset, the interrupt, the
thresholds, offset, crosstalk, the ROI and the temperature update all come from
there instead of being written out a second time. What genuinely differs from
the VL53L4CD ULD, and is therefore what this module holds, is the ranging
configuration: a discrete timing-budget table instead of arithmetic, a distance
mode (short / long), an init that resets the die first and fixes the interrupt
polarity, a start code with no free-running mode, and its own two calibrations.

**Crosstalk is in kcps here, not in cps.** ST's L1X wrapper converts the 7.9
kcps register at 0x0016 to counts per second in its own API; the L4CD wrapper
does not. The tool speaks one unit for the whole family, so the shared
set_xtalk/get_xtalk keep the register's own kcps — the only place where this
port deliberately parts with the C driver.

Reference C driver: ../../temp/STSW-IMG009_L1/STSW-IMG009_v3.5.5/API/core/
"""

from depz_sensor_sdk.vl53lx._link import Vl53Error
from depz_sensor_sdk.vl53lx.uld.vl53l1_die import (CONFIG_END, CONFIG_ADDR,
                            INNER_OFFSET_MM, INTERMEASUREMENT_MS,
                            OUTER_OFFSET_MM, PHASECAL_CONFIG__TIMEOUT_MACROP,
                            RANGE_CONFIG__TIMEOUT_MACROP_A_HI,
                            RANGE_CONFIG__TIMEOUT_MACROP_B_HI,
                            RANGE_CONFIG__VALID_PHASE_HIGH,
                            RANGE_CONFIG__VCSEL_PERIOD_A,
                            RANGE_CONFIG__VCSEL_PERIOD_B,
                            RANGE_OFFSET_MM, RESULT__OSC_CALIBRATE_VAL,
                            SD_CONFIG__INITIAL_PHASE_SD0, SD_CONFIG__WOI_SD0,
                            SYSTEM__INTERRUPT_CONFIG_GPIO,
                            SYSTEM__MODE_START, THRESH_HIGH, THRESH_LOW,
                            XTALK_PLANE_OFFSET_KCPS, VL53L1Die)

ULD_VERSION = (3, 5, 5)

# VL51L1X_DEFAULT_CONFIGURATION[] — 91 bytes, registers 0x2D..0x87.
# Byte 0 (register 0x2D) is overridden with CONFIG_FMP_BYTE by sensor_init(),
# exactly as the C comment on that byte says ("set bit 2 and 5 to 1 for fast
# plus mode"): FM+ pads work at every step down to 100 kHz, so it is set
# unconditionally and never cleared (see the plan note "FM+ на датчике").
DEFAULT_CONFIGURATION = bytes([
    0x00, 0x00, 0x00, 0x01, 0x02, 0x00, 0x02, 0x08,   # 0x2D..0x34
    0x00, 0x08, 0x10, 0x01, 0x01, 0x00, 0x00, 0x00,   # 0x35..0x3C
    0x00, 0xff, 0x00, 0x0F, 0x00, 0x00, 0x00, 0x00,   # 0x3D..0x44
    0x00, 0x20, 0x0b, 0x00, 0x00, 0x02, 0x0a, 0x21,   # 0x45..0x4C
    0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x00, 0xc8,   # 0x4D..0x54
    0x00, 0x00, 0x38, 0xff, 0x01, 0x00, 0x08, 0x00,   # 0x55..0x5C
    0x00, 0x01, 0xcc, 0x0f, 0x01, 0xf1, 0x0d, 0x01,   # 0x5D..0x64
    0x68, 0x00, 0x80, 0x08, 0xb8, 0x00, 0x00, 0x00,   # 0x65..0x6C
    0x00, 0x0f, 0x89, 0x00, 0x00, 0x00, 0x00, 0x00,   # 0x6D..0x74
    0x00, 0x00, 0x01, 0x0f, 0x0d, 0x0e, 0x0e, 0x00,   # 0x75..0x7C
    0x00, 0x02, 0xc7, 0xff, 0x9B, 0x00, 0x00, 0x00,   # 0x7D..0x84
    0x01, 0x00, 0x00,                                 # 0x85..0x87
])
assert len(DEFAULT_CONFIGURATION) == CONFIG_END - CONFIG_ADDR + 1

# Distance modes (VL53L1X_SetDistanceMode). The register codes stay inside
# this module: callers name a mode, see MODES / set_mode() below.
DISTANCE_SHORT, DISTANCE_LONG = 1, 2
MODE_CODES = {'short': DISTANCE_SHORT, 'long': DISTANCE_LONG}
MODE_NAMES = {code: name for name, code in MODE_CODES.items()}

# VL53L1X_SetTimingBudgetInMs(): the budget is not computed on this part, it is
# looked up. {distance mode: {budget ms: (MACROP_A_HI, MACROP_B_HI)}}.
# 15 ms exists in short mode only.
TIMING_BUDGETS = {
    DISTANCE_SHORT: {
        15:  (0x001D, 0x0027),
        20:  (0x0051, 0x006E),
        33:  (0x00D6, 0x006E),
        50:  (0x01AE, 0x01E8),
        100: (0x02E1, 0x0388),
        200: (0x03E1, 0x0496),
        500: (0x0591, 0x05C1),
    },
    DISTANCE_LONG: {
        20:  (0x001E, 0x0022),
        33:  (0x0060, 0x006E),
        50:  (0x00AD, 0x00C6),
        100: (0x01CC, 0x01EA),
        200: (0x02D9, 0x02F8),
        500: (0x048F, 0x04A4),
    },
}

# VL53L1X_GetTimingBudgetInMs() reads MACROP_A_HI back; both modes share this
# one table because no value collides between them.
_BUDGET_BY_MACROP_A = {a: ms
                       for table in TIMING_BUDGETS.values()
                       for ms, (a, _b) in table.items()}


class VL53L1(VL53L1Die):
    """Port of VL53L1X_api.c + VL53L1X_calibration.c (ULD 3.5.5). See the
    module docstring for what it takes from the die and what is its own."""

    CONFIGURATION = DEFAULT_CONFIGURATION
    # VL53L1X_StopRanging() writes 0x00 where the L4CD ULD writes 0x80.
    STOP_MODE     = 0x00
    SUPPORTS = frozenset({'mode', 'timing', 'offset', 'xtalk', 'thresholds',
                          'signal_thresh', 'sigma_thresh', 'temp_update',
                          'calib_offset', 'calib_xtalk', 'roi'})
    MODES = ('long', 'short')       # the configuration blob boots in long
    # This ULD does not compute the budget, it looks it up, so only the
    # tabulated values exist — and 15 ms only in short mode. See
    # budget_choices() and TIMING_BUDGETS.
    BUDGET_MS = (15, 500)
    # The mode the sensor was last put into. budget_choices() answers from
    # this instead of reading PHASECAL_CONFIG__TIMEOUT_MACROP: a caller asks
    # what budgets it may offer before init has written the blob, and that
    # register reads 0 on a part fresh out of reset. get_mode() is still the
    # register, for when the truth is what is wanted.
    _applied_mode = MODES[0]

    # This ULD takes the signal from 0x0098, the crosstalk-corrected peak,
    # and scales the per-spad rates by 200.0/8 instead of 256 — see the
    # mixin in uld/vl53l1_die.py.
    SIGNAL_AT  = 15
    PER_SPAD_K = 25

    # ── init (VL53L1X_SensorInit(), on the die's sequence) ──
    def init_boot(self):
        """No plain wait_boot() here: a sensor left hung by a previous session
        answers reads with zeros and never reaches 0x03 on its own.
        reset_device() ends with the boot wait anyway."""
        self.reset_device()

    def init_after_config(self):
        """ST ships this part's configuration with an active-HIGH interrupt
        (register 0x30 = 0x01), where the L4CD blob has 0x11 — active low.
        The bridge watches the falling edge, so an active-high sensor gives it
        one edge and then nothing: measured on the VL53L1CB board, `stream`
        delivered exactly one report while polling ran at the full 20 Hz."""
        self.set_interrupt_polarity(0)

    def init_extra(self):
        """The L4CD ULD's ALGO__RANGE_IGNORE_THRESHOLD_MCPS write is not in
        this driver's init, so nothing is written here; what is left to do is
        record the mode the blob boots in, for budget_choices()."""
        self._applied_mode = self.MODES[0]

    # ── ranging ──
    def start_ranging(self):
        """VL53L1X_StartRanging(): timed mode, and only timed mode.

        Unlike the L4CD, this part has no free-running mode under the ULD
        configuration — measured on the VL53L1CB board. A zero
        inter-measurement period gives no frames at all, and 0x21
        (VL53L1_DEVICEMEASUREMENTMODE_BACKTOBACK of ST's full VL53L1 API) gives
        exactly one and then stops. set_range_timing() therefore never leaves
        the period at zero.
        """
        self.p.wr_byte(SYSTEM__MODE_START, 0x40)

    # ── ranging mode (VL53L1X_SetDistanceMode) ──
    def set_mode(self, name: str):
        """'short' or 'long'. The two differ in VCSEL period and phase
        window, so the budget is re-applied afterwards — those periods
        change what the MACROP timeouts mean."""
        try:
            mode = MODE_CODES[name]
        except KeyError:
            raise Vl53Error(f'no such mode: {name} '
                            f'(have {", ".join(self.MODES)})') from None
        budget_ms, inter_ms = self.get_range_timing()
        self._applied_mode = name

        if mode == DISTANCE_SHORT:
            self.p.wr_byte(PHASECAL_CONFIG__TIMEOUT_MACROP, 0x14)
            self.p.wr_byte(RANGE_CONFIG__VCSEL_PERIOD_A, 0x07)
            self.p.wr_byte(RANGE_CONFIG__VCSEL_PERIOD_B, 0x05)
            self.p.wr_byte(RANGE_CONFIG__VALID_PHASE_HIGH, 0x38)
            self.p.wr_word(SD_CONFIG__WOI_SD0, 0x0705)
            self.p.wr_word(SD_CONFIG__INITIAL_PHASE_SD0, 0x0606)
        else:
            self.p.wr_byte(PHASECAL_CONFIG__TIMEOUT_MACROP, 0x0A)
            self.p.wr_byte(RANGE_CONFIG__VCSEL_PERIOD_A, 0x0F)
            self.p.wr_byte(RANGE_CONFIG__VCSEL_PERIOD_B, 0x0D)
            self.p.wr_byte(RANGE_CONFIG__VALID_PHASE_HIGH, 0xB8)
            self.p.wr_word(SD_CONFIG__WOI_SD0, 0x0F0D)
            self.p.wr_word(SD_CONFIG__INITIAL_PHASE_SD0, 0x0E0E)

        # 15 ms exists in short mode only, so a mode change can invalidate the
        # budget that was set. Fall back to the nearest one this mode has.
        if budget_ms not in TIMING_BUDGETS[mode]:
            budget_ms = min(TIMING_BUDGETS[mode],
                            key=lambda ms: abs(ms - budget_ms))
        self.set_range_timing(budget_ms, inter_ms)

    def get_mode(self) -> str:
        """-> 'short' or 'long'. VL53L1X_GetDistanceMode()."""
        return MODE_NAMES[self._mode_code()]

    def _mode_code(self) -> int:
        temp = self.p.rd_byte(PHASECAL_CONFIG__TIMEOUT_MACROP)
        if temp == 0x14:
            return DISTANCE_SHORT
        if temp == 0x0A:
            return DISTANCE_LONG
        raise Vl53Error(f'PHASECAL_CONFIG__TIMEOUT_MACROP reads 0x{temp:02X}, '
                        f'which is neither distance mode')

    # ── timing ──
    def set_range_timing(self, timing_budget_ms: int, inter_measurement_ms: int):
        """set_range_timing(budget, inter) of the family contract, on top of
        VL53L1X_SetTimingBudgetInMs() + VL53L1X_SetInterMeasurementInMs().

        The budget is not free on this part: only the tabulated values exist,
        and the table depends on the distance mode.

        Neither is the period. The rest of the family reads
        `inter_measurement_ms == 0` as "free-running"; this part has no such
        mode (see start_ranging()), so a zero period is taken to mean "as fast
        as the budget allows" and is written as the budget itself. Read it back
        with get_range_timing() to see what the sensor actually got.
        """
        mode = self._mode_code()
        table = TIMING_BUDGETS[mode]
        if timing_budget_ms not in table:
            raise Vl53Error(
                f'timing budget {timing_budget_ms} ms is not one the '
                f'{MODE_NAMES[mode]} mode has - pick one of '
                f'{", ".join(str(ms) for ms in sorted(table))} ms')

        macrop_a, macrop_b = table[timing_budget_ms]
        self.p.wr_word(RANGE_CONFIG__TIMEOUT_MACROP_A_HI, macrop_a)
        self.p.wr_word(RANGE_CONFIG__TIMEOUT_MACROP_B_HI, macrop_b)

        if inter_measurement_ms == 0:
            inter_measurement_ms = timing_budget_ms
        clock_pll = self.p.rd_word(RESULT__OSC_CALIBRATE_VAL) & 0x3FF
        self.p.wr_dword(INTERMEASUREMENT_MS,
                        int(clock_pll * inter_measurement_ms * 1.075))

    def budget_choices(self) -> tuple:
        """The tabulated budgets of the mode in use. 15 ms exists in short
        mode only, so the answer moves with the mode."""
        return tuple(sorted(TIMING_BUDGETS[MODE_CODES[self._applied_mode]]))

    def get_range_timing(self) -> tuple:
        """-> (timing_budget_ms, inter_measurement_ms)."""
        macrop_a = self.p.rd_word(RANGE_CONFIG__TIMEOUT_MACROP_A_HI)
        budget_ms = _BUDGET_BY_MACROP_A.get(macrop_a, 0)

        tmp = self.p.rd_dword(INTERMEASUREMENT_MS)
        clock_pll = self.p.rd_word(RESULT__OSC_CALIBRATE_VAL) & 0x3FF
        inter_ms = int(tmp / (clock_pll * 1.065)) if clock_pll else 0
        return budget_ms, inter_ms

    # ── thresholds ──
    def set_detection_thresholds(self, distance_low_mm: int, distance_high_mm: int,
                                 window: int, int_on_no_target: int = 0):
        """VL53L1X_SetDistanceThreshold(): a read-modify-write that keeps the
        bits outside 0x6F, and an "interrupt when no target" flag the L4CD ULD
        does not have."""
        temp = self.p.rd_byte(SYSTEM__INTERRUPT_CONFIG_GPIO) & ~0x6F & 0xFF
        temp |= window
        if int_on_no_target:
            temp |= 0x40
        self.p.wr_byte(SYSTEM__INTERRUPT_CONFIG_GPIO, temp)
        self.p.wr_word(THRESH_HIGH, distance_high_mm)
        self.p.wr_word(THRESH_LOW, distance_low_mm)

    # ── calibration (VL53L1X_calibration.c) ──
    def calibrate_offset(self, target_dist_mm: int, nb_samples: int = 50) -> int:
        """VL53L1X_CalibrateOffset(), with the sample count made an argument
        (the C driver hardcodes 50)."""
        if nb_samples < 5:
            raise Vl53Error('nb_samples must be at least 5')

        self.p.wr_word(RANGE_OFFSET_MM, 0)
        self.p.wr_word(INNER_OFFSET_MM, 0)
        self.p.wr_word(OUTER_OFFSET_MM, 0)

        distances = []
        self._collect(nb_samples, lambda i, r: distances.append(r.distance_mm))

        offset_mm = target_dist_mm - sum(distances) // nb_samples
        self.p.wr_word(RANGE_OFFSET_MM, (offset_mm * 4) & 0xFFFF)
        return offset_mm

    def calibrate_xtalk(self, target_dist_mm: int, nb_samples: int = 50) -> int:
        """VL53L1X_CalibrateXtalk(). Returns kcps, not the C driver's cps."""
        if nb_samples < 5:
            raise Vl53Error('nb_samples must be at least 5')

        self.p.wr_word(XTALK_PLANE_OFFSET_KCPS, 0)      # disable compensation

        samples = []
        self._collect(nb_samples, lambda i, r: samples.append(r))

        n = float(len(samples))
        avg_distance = sum(s.distance_mm for s in samples) / n
        avg_spad_nb = sum(s.number_of_spad for s in samples) / n
        avg_signal = sum(s.signal_rate_kcps for s in samples) / n
        if avg_spad_nb == 0:
            raise Vl53Error('xtalk calibration failed: no SPADs enabled')

        cal_xtalk = int(512 * (avg_signal * (1 - avg_distance / target_dist_mm))
                        / avg_spad_nb)
        cal_xtalk = min(max(cal_xtalk, 0), 0xFFFF)
        self.p.wr_word(XTALK_PLANE_OFFSET_KCPS, cal_xtalk)
        return round(cal_xtalk / 512.0)
