"""
uld/vl53lx/nvm.py — the raw NVM, which the register blocks do not cover.

Part of the factory calibration never appears in the register map: the optical
centre, the 5x5 peak-rate map, the mode-mitigation offset calibration and the
FMT range results. To get at them the driver stops the firmware, powers the NVM
array and clocks words out of it one at a time through
`RANGING_CORE__NVM_CTRL__*` — four writes and a read per 32-bit word.

That cost is the reason this module reads regions, not the whole array. A full
NVM dump is 512 words, which over USB is some 2500 round trips; the four
regions the driver actually wants are 21 words between them.

`VL53LX_read_nvm_raw_data()` wraps every region in its own enable/disable pair,
and so does `read_region()` here — the sensor must not be left with its
firmware stopped, and one exception in the middle of a multi-region read would
do exactly that.

Reference: vl53lx_nvm.c, vl53lx_nvm_map.h.
"""

from collections import namedtuple

# ── registers (vl53lx_register_map.h) ────────────────────────────────────────
POWER_MANAGEMENT__GO1_POWER_FORCE = 0x0083
FIRMWARE__ENABLE                  = 0x0085
RANGING_CORE__CLK_CTRL1           = 0x0683
RANGING_CORE__NVM_CTRL__MODE      = 0x0780
RANGING_CORE__NVM_CTRL__PDN       = 0x0781
RANGING_CORE__NVM_CTRL__READN     = 0x0783
RANGING_CORE__NVM_CTRL__PULSE_WIDTH_MSB = 0x0784
RANGING_CORE__NVM_CTRL__DATAOUT_MMM     = 0x0790
RANGING_CORE__NVM_CTRL__ADDR            = 0x0794

# ── timings (vl53lx_nvm.h, vl53lx_ll_device.h) ───────────────────────────────
NVM_POWER_UP_DELAY_US              = 50
NVM_READ_TRIGGER_DELAY_US          = 5
ENABLE_POWERFORCE_SETTLING_TIME_US = 250
NVM_CTRL_PULSE_WIDTH               = 0x0004

# ── FMT regions (vl53lx_nvm_map.h), as (byte index, byte size) ───────────────
FMT_OPTICAL_CENTRE            = (0x00B8, 4)
FMT_CAL_PEAK_RATE_MAP         = (0x015C, 56)
FMT_ADDITIONAL_OFFSET_CAL     = (0x0194, 8)
FMT_RANGE_RESULTS__140MM_DARK = (0x01AC, 16)

PEAK_RATE_MAP_WIDTH   = 5
PEAK_RATE_MAP_HEIGHT  = 5
PEAK_RATE_MAP_SAMPLES = PEAK_RATE_MAP_WIDTH * PEAK_RATE_MAP_HEIGHT

# ── decoded shapes (vl53lx_ll_def.h) ─────────────────────────────────────────
OpticalCentre = namedtuple('OpticalCentre', 'x_centre y_centre')

CalPeakRateMap = namedtuple(
    'CalPeakRateMap',
    'cal_distance_mm cal_reflectance_pc max_samples width height peak_rate_mcps')

AdditionalOffsetCalData = namedtuple(
    'AdditionalOffsetCalData',
    'result__mm_inner_actual_effective_spads '
    'result__mm_outer_actual_effective_spads '
    'result__mm_inner_peak_signal_count_rtn_mcps '
    'result__mm_outer_peak_signal_count_rtn_mcps')

FmtRangeData = namedtuple(
    'FmtRangeData',
    'result__actual_effective_rtn_spads '
    'ref_spad_array__num_requested_ref_spads '
    'ref_spad_array__ref_location '
    'result__peak_signal_count_rate_rtn_mcps '
    'result__ambient_count_rate_rtn_mcps '
    'result__peak_signal_count_rate_ref_mcps '
    'result__ambient_count_rate_ref_mcps '
    'measured_distance_mm measured_distance_stdev_mm')


def _u16(buf, off):
    return int.from_bytes(buf[off:off + 2], 'big')


class NvmReader:
    """Raw NVM access. Holds no state between reads — the sensor does."""

    def __init__(self, platform, image):
        self.p = platform
        self.img = image

    # ── VL53LX_nvm_enable / _disable ──
    def _enable(self):
        # disable_firmware / enable_powerforce, both through system_control so
        # the image stays truthful about what the device was told.
        self.img.system_control.firmware__enable = 0
        self.p.wr_byte(FIRMWARE__ENABLE, 0)
        self.img.system_control.power_management__go1_power_force = 1
        self.p.wr_byte(POWER_MANAGEMENT__GO1_POWER_FORCE, 1)
        self.p.sleep_ms(1)                       # 250 us settling, rounded up

        self.p.wr_byte(RANGING_CORE__NVM_CTRL__PDN, 0x01)
        self.p.wr_byte(RANGING_CORE__CLK_CTRL1, 0x05)
        self.p.sleep_ms(1)                       # 50 us power-up, rounded up
        self.p.wr_byte(RANGING_CORE__NVM_CTRL__MODE, 0x01)
        self.p.wr_word(RANGING_CORE__NVM_CTRL__PULSE_WIDTH_MSB,
                       NVM_CTRL_PULSE_WIDTH)

    def _disable(self):
        self.p.wr_byte(RANGING_CORE__NVM_CTRL__READN, 0x01)
        self.p.wr_byte(RANGING_CORE__NVM_CTRL__PDN, 0x00)
        self.img.system_control.power_management__go1_power_force = 0
        self.p.wr_byte(POWER_MANAGEMENT__GO1_POWER_FORCE, 0)
        self.img.system_control.firmware__enable = 1
        self.p.wr_byte(FIRMWARE__ENABLE, 1)

    # ── VL53LX_nvm_read ──
    def _read_words(self, start_word: int, count: int) -> bytes:
        out = bytearray()
        for addr in range(start_word, start_word + count):
            self.p.wr_byte(RANGING_CORE__NVM_CTRL__ADDR, addr)
            self.p.wr_byte(RANGING_CORE__NVM_CTRL__READN, 0x00)
            # 5 us trigger delay: a USB round trip is three orders of magnitude
            # longer, so the next transfer is the wait.
            self.p.wr_byte(RANGING_CORE__NVM_CTRL__READN, 0x01)
            out += self.p.rd_multi(RANGING_CORE__NVM_CTRL__DATAOUT_MMM, 4)
        return bytes(out)

    # ── VL53LX_read_nvm_raw_data ──
    def read_region(self, region) -> bytes:
        """(byte index, byte size) -> the bytes. Both must be word-aligned, as
        every region in the FMT map is."""
        index, size = region
        if index & 3 or size & 3:
            raise ValueError(f'NVM region 0x{index:04X}+{size} is not '
                             f'word-aligned')
        self._enable()
        try:
            return self._read_words(index >> 2, size >> 2)
        finally:
            self._disable()

    # ── the four decoders the driver needs ──
    def optical_centre(self) -> OpticalCentre:
        buf = self.read_region(FMT_OPTICAL_CENTRE)
        # The x centre is stored as a distance down from 0x0100; on a part that
        # was never programmed the subtraction overflows and the driver zeroes
        # it, which is what makes read_p2p_data fall back to the mm ROI centre.
        x = 0x0100 - buf[2]
        return OpticalCentre(x if x <= 0xFF else 0, buf[3])

    def cal_peak_rate_map(self) -> CalPeakRateMap:
        buf = self.read_region(FMT_CAL_PEAK_RATE_MAP)
        return CalPeakRateMap(
            cal_distance_mm=_u16(buf, 0),
            cal_reflectance_pc=_u16(buf, 2) >> 6,
            max_samples=PEAK_RATE_MAP_SAMPLES,
            width=PEAK_RATE_MAP_WIDTH,
            height=PEAK_RATE_MAP_HEIGHT,
            peak_rate_mcps=tuple(_u16(buf, 4 + 2 * i)
                                 for i in range(PEAK_RATE_MAP_SAMPLES)))

    def additional_offset_cal_data(self) -> AdditionalOffsetCalData:
        buf = self.read_region(FMT_ADDITIONAL_OFFSET_CAL)
        return AdditionalOffsetCalData(*(_u16(buf, 2 * i) for i in range(4)))

    def fmt_range_results(self, region=FMT_RANGE_RESULTS__140MM_DARK):
        buf = self.read_region(region)
        return FmtRangeData(
            result__actual_effective_rtn_spads=_u16(buf, 0),
            ref_spad_array__num_requested_ref_spads=buf[2],
            ref_spad_array__ref_location=buf[3],
            result__peak_signal_count_rate_rtn_mcps=_u16(buf, 4),
            result__ambient_count_rate_rtn_mcps=_u16(buf, 6),
            result__peak_signal_count_rate_ref_mcps=_u16(buf, 8),
            result__ambient_count_rate_ref_mcps=_u16(buf, 10),
            measured_distance_mm=_u16(buf, 12),
            measured_distance_stdev_mm=_u16(buf, 14))
