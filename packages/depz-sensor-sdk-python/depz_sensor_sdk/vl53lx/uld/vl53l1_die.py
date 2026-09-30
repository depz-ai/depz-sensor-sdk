"""
uld/vl53l1_die.py — the VL53L1 die: register map, result block, and the
VL53L4CD ULD body.

Two different things live here, and the split matters:

`VL53L1ResultBlock` is what every part built on this die has in common no
matter which ULD drives it — the 17-byte result block at 0x0089, the
range-status table, the register-address width, the interrupt-release write and
the bus ceiling. Both drivers of the die mix it in: `VL53L1Die` below, and
`uld/l1.py`, which is a port of a different C driver entirely.

`VL53L1Die` is the second thing: the body of the VL53L4CD ULD. VL53L4CD,
VL53L4CX and VL53L3CX run the same ranging sequence, the same timing
arithmetic and the same threshold registers, so that body is written once here
and `uld/l4.py` and `uld/l3.py` fill in what is theirs — the configuration
blob, the writes that close init, and the calibration or ROI their own ULD has.
They are siblings over this class; neither is a kind of the other.

`uld/l1.py` shares only `VL53L1ResultBlock`: the VL53L1CX/L1CB run ST's own
VL53L1X ULD, a separate C driver with a tabulated timing budget and a distance
mode, so it is a port of its own.

Reference C driver: ../../temp/STSW-IMG026/VL53L4CD_ULD_Driver/ (VL53L4CD_api.c)
"""

import time

from depz_sensor_sdk.vl53lx._link import I2C_KHZ_BOOT, ProtocolError, Vl53Error
from depz_sensor_sdk.vl53lx.uld.base import Measurement, SensorDriver

# The register map of the die. One name per register, spelled the way the full
# VL53L1 map does: the VL53L4CD ULD header abbreviates a few of them
# (SYSTEM__INTERRUPT, RANGE_CONFIG_A/B, SYSTEM_START) and the VL53L1X ULD
# header writes them out, and two spellings of one address is how the two
# ports started drifting apart.
SOFT_RESET                            = 0x0000
I2C_SLAVE__DEVICE_ADDRESS             = 0x0001
OSC_FREQUENCY                         = 0x0006    # unnamed in the C driver
VHV_CONFIG__TIMEOUT_MACROP_LOOP_BOUND = 0x0008
XTALK_PLANE_OFFSET_KCPS               = 0x0016
XTALK_X_PLANE_GRADIENT_KCPS           = 0x0018
XTALK_Y_PLANE_GRADIENT_KCPS           = 0x001A
RANGE_OFFSET_MM                       = 0x001E
INNER_OFFSET_MM                       = 0x0020
OUTER_OFFSET_MM                       = 0x0022
GPIO_HV_MUX__CTRL                     = 0x0030
GPIO__TIO_HV_STATUS                   = 0x0031
SYSTEM__INTERRUPT_CONFIG_GPIO         = 0x0046
PHASECAL_CONFIG__TIMEOUT_MACROP       = 0x004B
RANGE_CONFIG__TIMEOUT_MACROP_A_HI     = 0x005E
RANGE_CONFIG__VCSEL_PERIOD_A          = 0x0060
RANGE_CONFIG__TIMEOUT_MACROP_B_HI     = 0x0061
RANGE_CONFIG__VCSEL_PERIOD_B          = 0x0063
RANGE_CONFIG__SIGMA_THRESH            = 0x0064
MIN_COUNT_RATE_RTN_LIMIT_MCPS         = 0x0066
RANGE_CONFIG__VALID_PHASE_HIGH        = 0x0069
INTERMEASUREMENT_MS                   = 0x006C
THRESH_HIGH                           = 0x0072
THRESH_LOW                            = 0x0074
SD_CONFIG__WOI_SD0                    = 0x0078
SD_CONFIG__INITIAL_PHASE_SD0          = 0x007A
ROI_CONFIG__USER_ROI_CENTRE_SPAD      = 0x007F
ROI_CONFIG__USER_ROI_XY_SIZE          = 0x0080
SYSTEM__INTERRUPT_CLEAR               = 0x0086
SYSTEM__MODE_START                    = 0x0087
RESULT__RANGE_STATUS                  = 0x0089
RESULT__SPAD_NB                       = 0x008C
RESULT__SIGNAL_RATE                   = 0x008E
RESULT__AMBIENT_RATE                  = 0x0090
RESULT__SIGMA                         = 0x0092
RESULT__DISTANCE                      = 0x0096
RESULT__OSC_CALIBRATE_VAL             = 0x00DE
FIRMWARE__SYSTEM_STATUS               = 0x00E5
IDENTIFICATION__MODEL_ID              = 0x010F
ROI_CONFIG__MODE_ROI_CENTRE_SPAD      = 0x013E

# Detection-threshold window modes (SYSTEM__INTERRUPT_CONFIG_GPIO), see Example_6.
WINDOW_BELOW, WINDOW_ABOVE, WINDOW_OUT, WINDOW_IN = 0, 1, 2, 3

# The configuration blob spans the same registers on every part of the die;
# what is in it does not. Byte 0 (register 0x2D) always carries
# CONFIG_FMP_BYTE, which puts the sensor's I2C pad in Fast Mode Plus — exactly
# what VL53L4CD_I2C_FAST_MODE_PLUS does in the C ULD (whose comment calls the
# bits "2 and 5", counting from 1). FM+ pads work at every step down to
# 100 kHz, so it is set unconditionally and never cleared; clearing it
# mid-block NACKs and truncates the write (see plan note "FM+ на датчике").
CONFIG_ADDR     = 0x002D
CONFIG_END      = 0x0087
CONFIG_FMP_BYTE = 0x12

# The block the MCU streams: RESULT__RANGE_STATUS .. 0x0099. Every field of
# VL53L4CD_ResultsData_t in one read.
RESULT_BLOCK_ADDR = RESULT__RANGE_STATUS
RESULT_BLOCK_LEN  = 17

# How this die releases its interrupt, handed to the bridge with the stream:
# a single write. VL53L0X takes two (0x0B <- 0x01, then 0x0B <- 0x00).
CLEAR_STEPS = ((SYSTEM__INTERRUPT_CLEAR, 0x01),)

# Register addresses are two bytes wide on every part of the family but VL53L0X.
ADDR_WIDTH = 2

# GetResult() raw status -> ULD status (status_rtn[24] in VL53L4CD_api.c).
STATUS_RTN = (255, 255, 255, 5, 2, 4, 1, 7, 3,
              0, 255, 255, 9, 13, 255, 255, 255, 255, 10, 6,
              255, 255, 11, 12)

# UM2931, "Range status description".
RANGE_STATUS_NAMES = {
    0:  'valid',
    1:  'sigma above threshold',
    2:  'signal below threshold',
    3:  'distance below detection threshold',
    4:  'phase out of valid limit',
    5:  'hardware fail',
    6:  'no wrap-around check done',
    7:  'wrapped target, phase mismatch',
    8:  'processing fail',
    9:  'crosstalk signal fail',
    10: 'interrupt error',
    11: 'merged target',
    12: 'signal too low',
    255: 'other error',
}


class ResultsData:
    """VL53L4CD_ResultsData_t."""

    __slots__ = ('range_status', 'distance_mm', 'ambient_rate_kcps',
                 'ambient_per_spad_kcps', 'signal_rate_kcps',
                 'signal_per_spad_kcps', 'number_of_spad', 'sigma_mm',
                 'stream_count')

    def __init__(self, **kw):
        for name in self.__slots__:
            setattr(self, name, kw.get(name, 0))

    @property
    def status_text(self) -> str:
        return RANGE_STATUS_NAMES.get(self.range_status, f'unknown ({self.range_status})')

    def __repr__(self):
        return (f'<ResultsData {self.distance_mm} mm  sigma {self.sigma_mm} mm  '
                f'signal {self.signal_rate_kcps} kcps  ambient {self.ambient_rate_kcps} kcps  '
                f'spads {self.number_of_spad}  status {self.range_status} ({self.status_text})>')


def as_measurement(r: ResultsData) -> Measurement:
    """ResultsData -> the family-wide Measurement every caller speaks."""
    return Measurement(
        distance_mm=r.distance_mm,
        status=r.range_status,
        status_text=r.status_text,
        signal_kcps=r.signal_rate_kcps,
        ambient_kcps=r.ambient_rate_kcps,
        sigma_mm=r.sigma_mm,
        spads=r.number_of_spad,
        extra={'signal_per_spad_kcps': r.signal_per_spad_kcps,
               'ambient_per_spad_kcps': r.ambient_per_spad_kcps,
               'stream_count': r.stream_count},
    )


class VL53L1ResultBlock:
    """The 17-byte result block at 0x0089 and the bridge settings that come
    with the die. Mixed into every driver of this die.

    The two ULDs read the same bytes slightly differently, and the two class
    constants below are that difference in full — everything else is shared.
    """

    # Bridge-side facts of the die.
    ADDR_WIDTH  = ADDR_WIDTH
    CLEAR_STEPS = CLEAR_STEPS
    MAX_KHZ     = 1000          # with Fast Mode Plus on the sensor pad

    # Where the signal rate comes from. The VL53L4CD ULD takes it from 0x008E
    # (offset 5), the VL53L1X ULD from 0x0098 (offset 15) — the
    # crosstalk-corrected peak signal.
    SIGNAL_AT = 5
    # per-spad rate = rate_kcps * K // spad_nb_raw, where spad_nb_raw is the
    # 8.8 word at 0x008C. The VL53L4CD ULD scales by 256, the VL53L1X ULD by
    # `200.0 * raw_rate / spad_nb_raw`, which is the same thing with K = 25
    # once the rate is in kcps. ST's own difference, kept.
    PER_SPAD_K = 256

    def stream_block(self) -> tuple:
        return RESULT_BLOCK_ADDR, RESULT_BLOCK_LEN

    def parse_result_block(self, raw: bytes) -> ResultsData:
        """Decode the block exactly as this die's ULD decodes the same
        registers read one by one."""
        if len(raw) < RESULT_BLOCK_LEN:
            raise Vl53Error(f'result block too short: {len(raw)} bytes')

        status = raw[0] & 0x1F
        if status < len(STATUS_RTN):
            status = STATUS_RTN[status]

        raw_spads    = int.from_bytes(raw[3:5], 'big')      # 0x008C, 8.8
        at           = self.SIGNAL_AT
        signal_kcps  = int.from_bytes(raw[at:at + 2], 'big') * 8
        ambient_kcps = int.from_bytes(raw[7:9], 'big') * 8  # 0x0090
        k            = self.PER_SPAD_K

        return ResultsData(
            range_status=status,
            # 0x008B RESULT__STREAM_COUNT: the sensor's own frame counter,
            # wrapping at 255. The ULD ignores it; it is what tells a frame the
            # host never received from a frame the sensor never produced.
            stream_count=raw[2],
            number_of_spad=raw_spads // 256,
            signal_rate_kcps=signal_kcps,
            ambient_rate_kcps=ambient_kcps,
            # 0x0092 RESULT__SIGMA_SD0, 14.2 mm. The VL53L1X ULD does not
            # expose it; the register is there and the family-wide Measurement
            # has a field for it, so it is decoded the same way on both.
            sigma_mm=int.from_bytes(raw[9:11], 'big') // 4,
            distance_mm=int.from_bytes(raw[13:15], 'big'),  # 0x0096
            signal_per_spad_kcps=signal_kcps * k // raw_spads if raw_spads else 0,
            ambient_per_spad_kcps=ambient_kcps * k // raw_spads if raw_spads else 0,
        )

    def decode(self, raw: bytes) -> Measurement:
        return as_measurement(self.parse_result_block(raw))

    def get_result(self) -> ResultsData:
        """One block read instead of the C driver's six register reads — the
        sensor auto-increments and the decoding is identical."""
        return self.parse_result_block(
            self.p.rd_multi(RESULT_BLOCK_ADDR, RESULT_BLOCK_LEN))

    def read_measurement(self) -> Measurement:
        return as_measurement(self.get_result())


class VL53L1Die(VL53L1ResultBlock, SensorDriver):
    """The body every driver of this die shares: identity, boot, the ranging
    loop, the interrupt, the threshold, offset, crosstalk and ROI registers.

    Its shape is the VL53L4CD ULD's, because that is the widest of the three
    ports; where the VL53L1X ULD genuinely differs it overrides (see
    uld/l1.py). A concrete product supplies CONFIGURATION (its own blob),
    SUPPORTS, and whatever else is its own — see uld/l4.py, uld/l3.py and
    uld/l1.py.
    """

    # The 91-byte blob written at CONFIG_ADDR, product-specific.
    CONFIGURATION = b''
    # What StopRanging writes: the L4CD ULD 0x80, the L3CX ULP 0x00.
    STOP_MODE = 0x80

    # ── identity ──
    def model_id(self) -> int:
        return self.p.rd_word(IDENTIFICATION__MODEL_ID)

    def set_i2c_address(self, new_address: int):
        """Note: the bridge always addresses 0x29, so this makes the sensor
        unreachable. Present for completeness with the C driver."""
        self.p.wr_byte(I2C_SLAVE__DEVICE_ADDRESS, new_address >> 1)

    # ── init ──
    def boot_state(self) -> int:
        return self.p.rd_byte(FIRMWARE__SYSTEM_STATUS)

    def wait_boot(self, timeout_s: float = 1.0):
        deadline = time.monotonic() + timeout_s
        while True:
            if self.boot_state() == 0x03:
                return
            if time.monotonic() > deadline:
                raise Vl53Error('timeout waiting for FIRMWARE__SYSTEM_STATUS == 0x03')
            self.p.sleep_ms(1)

    def reset_device(self, timeout_s: float = 1.0):
        """Pull the die through its soft reset (VL53L1_software_reset()).

        Neither ULD's SensorInit does this, and on a fresh power-up neither has
        to. Run on a sensor a previous session left configured, though, the
        configuration write goes through without a single error and the sensor
        then never raises data-ready again — measured on the VL53L1CB board,
        second `range` in a row. The VL53L0X driver carries the same note for
        the same reason. Only uld/l1.py calls it (see init_boot()).

        A part that was interrupted mid-range can also stop ACKing writes while
        still answering reads with zeros; there is no soft reset to write then,
        so the XSHUT line does it instead.
        """
        try:
            self.p.wr_byte(SOFT_RESET, 0x00)
            self.p.sleep_ms(1)
            self.p.wr_byte(SOFT_RESET, 0x01)
            self.wait_boot(timeout_s)
        except (Vl53Error, ProtocolError):
            self.p.xshut_reset()
            self.wait_boot(timeout_s)

    def sensor_init(self):
        """Initialise the sensor and leave the bus at this die's ceiling.

        The configuration block is written at I2C_KHZ_BOOT (400 kHz) because
        that is the only speed an unconfigured sensor is specified for; the
        bridge is re-timed to MAX_KHZ after it. Byte 0 of the block always
        carries CONFIG_FMP_BYTE (Fast Mode Plus pad): FM+ works at every step,
        entering it never NACKs, and leaving it does — so it is set once and
        never cleared. A sensor reset just re-runs this whole sequence.

        The three ULDs of the die run the same sequence and differ only in
        what they hang off the three hooks below.
        """
        self.p.set_addr_width(self.ADDR_WIDTH)
        self.p.set_i2c_speed(I2C_KHZ_BOOT)
        self.init_boot()

        config = bytes([CONFIG_FMP_BYTE]) + self.CONFIGURATION[1:]

        # The C driver writes the 91 bytes one register at a time; the sensor
        # auto-increments, so one transaction does the same job.
        self.p.wr_multi(CONFIG_ADDR, config)
        if self.MAX_KHZ != I2C_KHZ_BOOT:
            self.p.set_i2c_speed(self.MAX_KHZ)
        self.init_after_config()

        self.p.wr_byte(SYSTEM__MODE_START, 0x40)          # start VHV
        self.wait_data_ready()
        self.clear_interrupt()
        self.stop_ranging()
        self.p.wr_byte(VHV_CONFIG__TIMEOUT_MACROP_LOOP_BOUND, 0x09)
        self.p.wr_byte(0x000B, 0x00)
        self.init_extra()

        self.set_range_timing(50, 0)

    def init_boot(self):
        """How a part gets to a booted die. The two VL53L4CD-lineage ULDs just
        wait; the VL53L1X one resets first — see reset_device()."""
        self.wait_boot()

    def init_after_config(self):
        """Writes a part adds right after the configuration blob, before VHV.
        Only uld/l1.py has one: its blob boots the interrupt active-high."""

    def init_extra(self):
        """Writes a part adds at the end of init, before the timing is set."""

    # ── ranging ──
    def clear_interrupt(self):
        self.p.wr_byte(SYSTEM__INTERRUPT_CLEAR, 0x01)

    def start_ranging(self):
        # 0 = continuous, anything else = autonomous low power.
        mode = 0x21 if self.p.rd_dword(INTERMEASUREMENT_MS) == 0 else 0x40
        self.p.wr_byte(SYSTEM__MODE_START, mode)

    def stop_ranging(self):
        self.p.wr_byte(SYSTEM__MODE_START, self.STOP_MODE)

    def get_interrupt_polarity(self) -> int:
        return 0 if (self.p.rd_byte(GPIO_HV_MUX__CTRL) & 0x10) else 1

    def set_interrupt_polarity(self, polarity: int):
        temp = self.p.rd_byte(GPIO_HV_MUX__CTRL) & 0xEF
        self.p.wr_byte(GPIO_HV_MUX__CTRL, temp | ((not (polarity & 1)) << 4))

    def check_for_data_ready(self) -> bool:
        # Polarity first, as both C drivers read it.
        int_pol = self.get_interrupt_polarity()
        return (self.p.rd_byte(GPIO__TIO_HV_STATUS) & 1) == int_pol

    # ── timing ──
    def set_range_timing(self, timing_budget_ms: int, inter_measurement_ms: int):
        osc_frequency = self.p.rd_word(OSC_FREQUENCY)
        if osc_frequency == 0:
            raise Vl53Error('osc_frequency reads 0')
        if not 10 <= timing_budget_ms <= 200:
            raise Vl53Error('timing_budget_ms must be 10..200')

        timing_budget_us = timing_budget_ms * 1000
        macro_period_us = ((2304 * (0x40000000 // osc_frequency)) & 0xFFFFFFFF) >> 6

        if inter_measurement_ms == 0:                   # continuous
            self.p.wr_dword(INTERMEASUREMENT_MS, 0)
            timing_budget_us -= 2500
        elif inter_measurement_ms > timing_budget_ms:   # autonomous low power
            clock_pll = self.p.rd_word(RESULT__OSC_CALIBRATE_VAL) & 0x3FF
            factor = 1.055 * inter_measurement_ms * clock_pll
            self.p.wr_dword(INTERMEASUREMENT_MS, int(factor))
            timing_budget_us = (timing_budget_us - 4300) // 2
        else:
            raise Vl53Error('inter_measurement_ms must be 0 or > timing_budget_ms')

        timing_budget_us = (timing_budget_us << 12) & 0xFFFFFFFF
        for reg, mult in ((RANGE_CONFIG__TIMEOUT_MACROP_A_HI, 16),
                          (RANGE_CONFIG__TIMEOUT_MACROP_B_HI, 12)):
            tmp = ((macro_period_us * mult) & 0xFFFFFFFF) >> 6
            ls_byte = ((timing_budget_us + (tmp >> 1)) // tmp) - 1
            ms_byte = 0
            while ls_byte & 0xFFFFFF00:
                ls_byte >>= 1
                ms_byte += 1
            self.p.wr_word(reg, ((ms_byte << 8) + (ls_byte & 0xFF)) & 0xFFFF)

    def get_range_timing(self) -> tuple:
        """-> (timing_budget_ms, inter_measurement_ms)."""
        tmp = self.p.rd_dword(INTERMEASUREMENT_MS)
        clock_pll = self.p.rd_word(RESULT__OSC_CALIBRATE_VAL) & 0x3FF
        clock_pll = int(1.065 * clock_pll) & 0xFFFF
        inter_measurement_ms = (tmp // clock_pll) & 0xFFFF if clock_pll else 0

        osc_frequency = self.p.rd_word(OSC_FREQUENCY)
        if osc_frequency == 0:
            raise Vl53Error('osc_frequency reads 0')
        range_config_macrop_high = self.p.rd_word(RANGE_CONFIG__TIMEOUT_MACROP_A_HI)

        macro_period_us = ((2304 * (0x40000000 // osc_frequency)) & 0xFFFFFFFF) >> 6
        ls_byte = (range_config_macrop_high & 0x00FF) << 4
        ms_byte = (range_config_macrop_high & 0xFF00) >> 8
        ms_byte = (0x04 - (ms_byte - 1) - 1) & 0xFFFFFFFF
        macro_period_us = (macro_period_us * 16) & 0xFFFFFFFF

        budget = ((((ls_byte + 1) * (macro_period_us >> 6))
                   - ((macro_period_us >> 6) >> 1)) & 0xFFFFFFFF) >> 12
        if ms_byte < 12:
            budget >>= ms_byte
        budget = budget + 2500 if tmp == 0 else budget * 2 + 4300
        return budget // 1000, inter_measurement_ms

    # ── thresholds ──
    def set_detection_thresholds(self, distance_low_mm: int, distance_high_mm: int,
                                 window: int):
        self.p.wr_byte(SYSTEM__INTERRUPT_CONFIG_GPIO, window)
        self.p.wr_word(THRESH_HIGH, distance_high_mm)
        self.p.wr_word(THRESH_LOW, distance_low_mm)

    def get_detection_thresholds(self) -> tuple:
        """-> (distance_low_mm, distance_high_mm, window)."""
        high = self.p.rd_word(THRESH_HIGH)
        low = self.p.rd_word(THRESH_LOW)
        return low, high, self.p.rd_byte(SYSTEM__INTERRUPT_CONFIG_GPIO) & 0x07

    def set_signal_threshold(self, signal_kcps: int):
        self.p.wr_word(MIN_COUNT_RATE_RTN_LIMIT_MCPS, signal_kcps >> 3)

    def get_signal_threshold(self) -> int:
        return (self.p.rd_word(MIN_COUNT_RATE_RTN_LIMIT_MCPS) << 3) & 0xFFFF

    def set_sigma_threshold(self, sigma_mm: int):
        if sigma_mm > (0xFFFF >> 2):
            raise Vl53Error('sigma_mm must be <= 16383')
        self.p.wr_word(RANGE_CONFIG__SIGMA_THRESH, sigma_mm << 2)

    def get_sigma_threshold(self) -> int:
        return self.p.rd_word(RANGE_CONFIG__SIGMA_THRESH) >> 2

    # ── offset ──
    def set_offset(self, offset_mm: int):
        self.p.wr_word(RANGE_OFFSET_MM, (offset_mm * 4) & 0xFFFF)
        self.p.wr_word(INNER_OFFSET_MM, 0)
        self.p.wr_word(OUTER_OFFSET_MM, 0)

    def get_offset(self) -> int:
        temp = ((self.p.rd_word(RANGE_OFFSET_MM) << 3) & 0xFFFF) >> 5
        return temp - 2048 if temp > 1024 else temp

    # ── crosstalk (kcps on both ULDs; ST's L1X wrapper converts to cps, and
    # this port keeps the register's own unit so the tool speaks one) ──
    def set_xtalk(self, xtalk_kcps: int):
        self.p.wr_word(XTALK_X_PLANE_GRADIENT_KCPS, 0x0000)
        self.p.wr_word(XTALK_Y_PLANE_GRADIENT_KCPS, 0x0000)
        self.p.wr_word(XTALK_PLANE_OFFSET_KCPS, (xtalk_kcps << 9) & 0xFFFF)

    def get_xtalk(self) -> int:
        return round(self.p.rd_word(XTALK_PLANE_OFFSET_KCPS) / 512.0)

    # ── ROI (VL53L1X_SetROI / VL53L3CX_ULP_SetROI, the same registers) ──
    def set_roi(self, x: int, y: int):
        """An X by Y window of SPADs, 4..16 each way."""
        optical_center = self.p.rd_byte(ROI_CONFIG__MODE_ROI_CENTRE_SPAD)
        x = min(x, 16)
        y = min(y, 16)
        if x > 10 or y > 10:
            optical_center = 199
        self.p.wr_byte(ROI_CONFIG__USER_ROI_CENTRE_SPAD, optical_center)
        self.p.wr_byte(ROI_CONFIG__USER_ROI_XY_SIZE, (y - 1) << 4 | (x - 1))

    def get_roi(self) -> tuple:
        """-> (x, y)."""
        temp = self.p.rd_byte(ROI_CONFIG__USER_ROI_XY_SIZE)
        return (temp & 0x0F) + 1, ((temp & 0xF0) >> 4) + 1

    def set_roi_center(self, center_spad: int):
        self.p.wr_byte(ROI_CONFIG__USER_ROI_CENTRE_SPAD, center_spad)

    def get_roi_center(self) -> int:
        return self.p.rd_byte(ROI_CONFIG__USER_ROI_CENTRE_SPAD)

    # ── temperature ──
    def start_temperature_update(self):
        """Recommended after a >8 degC ambient change, see Example_3."""
        self.p.wr_byte(VHV_CONFIG__TIMEOUT_MACROP_LOOP_BOUND, 0x81)  # full VHV
        self.p.wr_byte(0x000B, 0x92)
        self.start_ranging()
        self.wait_data_ready()
        self.clear_interrupt()
        self.stop_ranging()
        self.p.wr_byte(VHV_CONFIG__TIMEOUT_MACROP_LOOP_BOUND, 0x09)
        self.p.wr_byte(0x000B, 0x00)

    # ── calibration ──
    def _collect(self, nb_samples: int, on_sample, timeout_s: float = 5.0):
        """The ranging loop every calibration shares: data-ready, GetResult,
        ClearInterrupt, `nb_samples` times. What is done with the samples is
        each ULD's own arithmetic — see calibrate_offset/calibrate_xtalk in
        uld/l4.py and uld/l1.py."""
        self.start_ranging()
        for i in range(nb_samples):
            self.wait_data_ready(timeout_s)
            result = self.get_result()
            self.clear_interrupt()
            on_sample(i, result)
        self.stop_ranging()
