"""VL53L4CD ULD driver (ST STSW-IMG026 2.2.3) over the register bridge.

Port of ``VL53L4CD_api.c`` + ``VL53L4CD_calibration.c``, absorbed from the
firmware repo's hardware-proven Python reference. The MCU owns nothing but the
I2C bus, XSHUT, INT and one streaming FSM — every register sequence below goes
over VL53_READ_REG / VL53_WRITE_REG (contracts/10_SENSOR_VL53L4.md §1).

Register sequences are a faithful port of the C code, integer widths included;
do not "simplify" them. Pure codec/math pieces (`parse_result_block`,
`range_timing_registers`, `decode_range_timing`, `config_block`) are module
functions so the golden-vector generator and the other SDKs can hold them to
byte-exact parity.
"""

from __future__ import annotations

from dataclasses import dataclass
from typing import Callable, Protocol

ULD_VERSION = (2, 2, 3, 0)

# ── Registers (VL53L4CD_api.h) ───────────────────────────────────────────────
SOFT_RESET = 0x0000
I2C_SLAVE__DEVICE_ADDRESS = 0x0001
OSC_FREQUENCY = 0x0006  # unnamed in the C driver
VHV_CONFIG__TIMEOUT_MACROP_LOOP_BOUND = 0x0008
XTALK_PLANE_OFFSET_KCPS = 0x0016
XTALK_X_PLANE_GRADIENT_KCPS = 0x0018
XTALK_Y_PLANE_GRADIENT_KCPS = 0x001A
RANGE_OFFSET_MM = 0x001E
INNER_OFFSET_MM = 0x0020
OUTER_OFFSET_MM = 0x0022
GPIO_HV_MUX__CTRL = 0x0030
GPIO__TIO_HV_STATUS = 0x0031
SYSTEM__INTERRUPT = 0x0046
RANGE_CONFIG_A = 0x005E
RANGE_CONFIG_B = 0x0061
RANGE_CONFIG__SIGMA_THRESH = 0x0064
MIN_COUNT_RATE_RTN_LIMIT_MCPS = 0x0066
INTERMEASUREMENT_MS = 0x006C
THRESH_HIGH = 0x0072
THRESH_LOW = 0x0074
SYSTEM__INTERRUPT_CLEAR = 0x0086
SYSTEM_START = 0x0087
RESULT__RANGE_STATUS = 0x0089
RESULT__SPAD_NB = 0x008C
RESULT__SIGNAL_RATE = 0x008E
RESULT__AMBIENT_RATE = 0x0090
RESULT__SIGMA = 0x0092
RESULT__DISTANCE = 0x0096
RESULT__OSC_CALIBRATE_VAL = 0x00DE
FIRMWARE__SYSTEM_STATUS = 0x00E5
IDENTIFICATION__MODEL_ID = 0x010F

MODEL_ID_VL53L4CD = 0xEBAA

# Detection-threshold window modes (SYSTEM__INTERRUPT).
WINDOW_BELOW, WINDOW_ABOVE, WINDOW_OUT, WINDOW_IN = 0, 1, 2, 3

CONFIG_ADDR = 0x002D
CONFIG_END = 0x0087

# VL53L4CD_DEFAULT_CONFIGURATION[] — 91 bytes, registers 0x2D..0x87.
# `config_block()` always overrides byte 0 (register 0x2D) with CONFIG_FMP_BYTE
# (0x12) to put the sensor's I2C pad in Fast Mode Plus — exactly what
# VL53L4CD_I2C_FAST_MODE_PLUS does in the C ULD. FM+ pads work at every bus
# step down to 100 kHz, so it is set unconditionally and never cleared
# (clearing it mid-block NACKs and truncates the write).
DEFAULT_CONFIGURATION = bytes([
    0x00, 0x00, 0x00, 0x11, 0x02, 0x00, 0x02, 0x08,   # 0x2D..0x34
    0x00, 0x08, 0x10, 0x01, 0x01, 0x00, 0x00, 0x00,   # 0x35..0x3C
    0x00, 0xff, 0x00, 0x0F, 0x00, 0x00, 0x00, 0x00,   # 0x3D..0x44
    0x00, 0x20, 0x0b, 0x00, 0x00, 0x02, 0x14, 0x21,   # 0x45..0x4C
    0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x00, 0xc8,   # 0x4D..0x54
    0x00, 0x00, 0x38, 0xff, 0x01, 0x00, 0x08, 0x00,   # 0x55..0x5C
    0x00, 0x01, 0xcc, 0x07, 0x01, 0xf1, 0x05, 0x00,   # 0x5D..0x64
    0xa0, 0x00, 0x80, 0x08, 0x38, 0x00, 0x00, 0x00,   # 0x65..0x6C
    0x00, 0x0f, 0x89, 0x00, 0x00, 0x00, 0x00, 0x00,   # 0x6D..0x74
    0x00, 0x00, 0x01, 0x07, 0x05, 0x06, 0x06, 0x00,   # 0x75..0x7C
    0x00, 0x02, 0xc7, 0xff, 0x9B, 0x00, 0x00, 0x00,   # 0x7D..0x84
    0x01, 0x00, 0x00,                                 # 0x85..0x87
])
assert len(DEFAULT_CONFIGURATION) == CONFIG_END - CONFIG_ADDR + 1

CONFIG_FMP_BYTE = 0x12

# The block the MCU streams: RESULT__RANGE_STATUS .. 0x0099 — every field of
# VL53L4CD_ResultsData_t in one read.
RESULT_BLOCK_ADDR = RESULT__RANGE_STATUS
RESULT_BLOCK_LEN = 17

# The bridge boots at 400 kHz and an unconfigured sensor is only specified for
# that speed, so init always runs its configuration block there. The bus is
# left at I2C_KHZ_DEFAULT afterwards (the block read is ~4x faster at 1 MHz).
I2C_KHZ_BOOT = 400
I2C_KHZ_DEFAULT = 1000

# GetResult() raw status -> ULD status (status_rtn[24] in VL53L4CD_api.c).
STATUS_RTN = (255, 255, 255, 5, 2, 4, 1, 7, 3,
              0, 255, 255, 9, 13, 255, 255, 255, 255, 10, 6,
              255, 255, 11, 12)

# UM2931, "Range status description".
RANGE_STATUS_NAMES = {
    0: "valid",
    1: "sigma above threshold",
    2: "signal below threshold",
    3: "distance below detection threshold",
    4: "phase out of valid limit",
    5: "hardware fail",
    6: "no wrap-around check done",
    7: "wrapped target, phase mismatch",
    8: "processing fail",
    9: "crosstalk signal fail",
    10: "interrupt error",
    11: "merged target",
    12: "signal too low",
    255: "other error",
}


class Vl53l4cdError(RuntimeError):
    pass


class Vl53l4Platform(Protocol):
    """What the ULD needs from the register bridge."""

    def rd_multi(self, addr: int, size: int) -> bytes: ...

    def wr_multi(self, addr: int, data: bytes) -> None: ...

    def sleep_ms(self, ms: int) -> None: ...

    def set_i2c_speed(self, khz: int) -> None: ...


@dataclass(frozen=True)
class ResultsData:
    """VL53L4CD_ResultsData_t plus the sensor's own frame counter."""

    range_status: int  # 0 = valid (RANGE_STATUS_NAMES)
    distance_mm: int
    ambient_rate_kcps: int
    ambient_per_spad_kcps: int
    signal_rate_kcps: int
    signal_per_spad_kcps: int
    number_of_spad: int
    sigma_mm: int
    # 0x008B RESULT__STREAM_COUNT: wraps at 255. The C ULD ignores it; it is
    # what tells a frame the host never received from one the sensor never
    # produced.
    stream_count: int

    @property
    def status_text(self) -> str:
        return RANGE_STATUS_NAMES.get(self.range_status, f"unknown ({self.range_status})")


def parse_result_block(raw: bytes) -> ResultsData:
    """Decode the streamed 0x0089..0x0099 block exactly as VL53L4CD_GetResult()
    decodes the same registers read one by one. Register contents are
    big-endian words (the bridge passes them through untouched)."""
    if len(raw) < 15:
        raise Vl53l4cdError(f"result block too short: {len(raw)} bytes")

    status = raw[0] & 0x1F
    if status < len(STATUS_RTN):
        status = STATUS_RTN[status]

    raw_spads = int.from_bytes(raw[3:5], "big")  # 0x008C
    signal_kcps = int.from_bytes(raw[5:7], "big") * 8  # 0x008E
    ambient_kcps = int.from_bytes(raw[7:9], "big") * 8  # 0x0090

    return ResultsData(
        range_status=status,
        stream_count=raw[2],
        number_of_spad=raw_spads // 256,
        signal_rate_kcps=signal_kcps,
        ambient_rate_kcps=ambient_kcps,
        sigma_mm=int.from_bytes(raw[9:11], "big") // 4,  # 0x0092
        distance_mm=int.from_bytes(raw[13:15], "big"),  # 0x0096
        signal_per_spad_kcps=signal_kcps * 256 // raw_spads if raw_spads else 0,
        ambient_per_spad_kcps=ambient_kcps * 256 // raw_spads if raw_spads else 0,
    )


def config_block() -> bytes:
    """The 91-byte block sensor_init() writes at CONFIG_ADDR: the ST default
    configuration with byte 0 forced to CONFIG_FMP_BYTE (Fast Mode Plus)."""
    return bytes([CONFIG_FMP_BYTE]) + DEFAULT_CONFIGURATION[1:]


def range_timing_registers(
    timing_budget_ms: int,
    inter_measurement_ms: int,
    osc_frequency: int,
    clock_pll: int = 0,
) -> tuple[int, int, int]:
    """SetRangeTiming register math → (RANGE_CONFIG_A, RANGE_CONFIG_B,
    INTERMEASUREMENT_MS raw dword).

    `osc_frequency` is the word read from 0x0006; `clock_pll` is the word read
    from RESULT__OSC_CALIBRATE_VAL (used only in autonomous mode, i.e. when
    `inter_measurement_ms > 0`)."""
    if osc_frequency == 0:
        raise Vl53l4cdError("osc_frequency reads 0")
    if not 10 <= timing_budget_ms <= 200:
        raise Vl53l4cdError("timing_budget_ms must be 10..200")

    timing_budget_us = timing_budget_ms * 1000
    macro_period_us = ((2304 * (0x40000000 // osc_frequency)) & 0xFFFFFFFF) >> 6

    if inter_measurement_ms == 0:  # continuous
        intermeasurement_raw = 0
        timing_budget_us -= 2500
    elif inter_measurement_ms > timing_budget_ms:  # autonomous low power
        factor = 1.055 * inter_measurement_ms * (clock_pll & 0x3FF)
        intermeasurement_raw = int(factor)
        timing_budget_us = (timing_budget_us - 4300) // 2
    else:
        raise Vl53l4cdError("inter_measurement_ms must be 0 or > timing_budget_ms")

    timing_budget_us = (timing_budget_us << 12) & 0xFFFFFFFF
    words = []
    for mult in (16, 12):  # RANGE_CONFIG_A, RANGE_CONFIG_B
        tmp = ((macro_period_us * mult) & 0xFFFFFFFF) >> 6
        ls_byte = ((timing_budget_us + (tmp >> 1)) // tmp) - 1
        ms_byte = 0
        while ls_byte & 0xFFFFFF00:
            ls_byte >>= 1
            ms_byte += 1
        words.append(((ms_byte << 8) + (ls_byte & 0xFF)) & 0xFFFF)
    return words[0], words[1], intermeasurement_raw


def decode_range_timing(
    intermeasurement_raw: int,
    clock_pll: int,
    osc_frequency: int,
    range_config_a: int,
) -> tuple[int, int]:
    """GetRangeTiming register math → (timing_budget_ms, inter_measurement_ms).

    Inputs are the raw register reads: INTERMEASUREMENT_MS dword, the
    RESULT__OSC_CALIBRATE_VAL word, the 0x0006 word and the RANGE_CONFIG_A
    word."""
    if osc_frequency == 0:
        raise Vl53l4cdError("osc_frequency reads 0")

    pll = int(1.065 * (clock_pll & 0x3FF)) & 0xFFFF
    inter_measurement_ms = (intermeasurement_raw // pll) & 0xFFFF if pll else 0

    macro_period_us = ((2304 * (0x40000000 // osc_frequency)) & 0xFFFFFFFF) >> 6
    ls_byte = (range_config_a & 0x00FF) << 4
    ms_byte = (range_config_a & 0xFF00) >> 8
    ms_byte = (0x04 - (ms_byte - 1) - 1) & 0xFFFFFFFF
    macro_period_us = (macro_period_us * 16) & 0xFFFFFFFF

    budget = ((((ls_byte + 1) * (macro_period_us >> 6))
               - ((macro_period_us >> 6) >> 1)) & 0xFFFFFFFF) >> 12
    if ms_byte < 12:
        budget >>= ms_byte
    budget = budget + 2500 if intermeasurement_raw == 0 else budget * 2 + 4300
    return budget // 1000, inter_measurement_ms


# ── Threshold / offset / xtalk raw codecs (register word ↔ user units) ───────

def offset_raw(offset_mm: int) -> int:
    """RANGE_OFFSET_MM word for SetOffset (INNER/OUTER are zeroed alongside)."""
    return (offset_mm * 4) & 0xFFFF


def decode_offset(raw_word: int) -> int:
    """GetOffset: RANGE_OFFSET_MM word → signed millimetres."""
    temp = ((raw_word << 3) & 0xFFFF) >> 5
    return temp - 2048 if temp > 1024 else temp


def xtalk_raw(xtalk_kcps: int) -> int:
    """XTALK_PLANE_OFFSET_KCPS word for SetXtalk."""
    return (xtalk_kcps << 9) & 0xFFFF


def decode_xtalk(raw_word: int) -> int:
    """GetXtalk: XTALK_PLANE_OFFSET_KCPS word → kcps."""
    return round(raw_word / 512.0)


def signal_threshold_raw(signal_kcps: int) -> int:
    return signal_kcps >> 3


def decode_signal_threshold(raw_word: int) -> int:
    return (raw_word << 3) & 0xFFFF


def sigma_threshold_raw(sigma_mm: int) -> int:
    if sigma_mm > (0xFFFF >> 2):
        raise Vl53l4cdError("sigma_mm must be <= 16383")
    return sigma_mm << 2


def decode_sigma_threshold(raw_word: int) -> int:
    return raw_word >> 2


class VL53L4CD:
    """Port of VL53L4CD_api.c + VL53L4CD_calibration.c (ULD 2.2.3)."""

    def __init__(self, platform: Vl53l4Platform):
        self.p = platform

    # ── register access helpers (16-bit addr, big-endian contents) ───────────

    def rd_byte(self, addr: int) -> int:
        return self.p.rd_multi(addr, 1)[0]

    def rd_word(self, addr: int) -> int:
        return int.from_bytes(self.p.rd_multi(addr, 2), "big")

    def rd_dword(self, addr: int) -> int:
        return int.from_bytes(self.p.rd_multi(addr, 4), "big")

    def wr_byte(self, addr: int, value: int) -> None:
        self.p.wr_multi(addr, bytes([value & 0xFF]))

    def wr_word(self, addr: int, value: int) -> None:
        self.p.wr_multi(addr, (value & 0xFFFF).to_bytes(2, "big"))

    def wr_dword(self, addr: int, value: int) -> None:
        self.p.wr_multi(addr, (value & 0xFFFFFFFF).to_bytes(4, "big"))

    # ── identity ─────────────────────────────────────────────────────────────

    def get_sensor_id(self) -> int:
        return self.rd_word(IDENTIFICATION__MODEL_ID)

    def is_alive(self) -> bool:
        return self.get_sensor_id() == MODEL_ID_VL53L4CD

    # ── init ─────────────────────────────────────────────────────────────────

    def wait_boot(self, timeout_ms: int = 1000) -> None:
        for _ in range(max(1, timeout_ms)):
            if self.rd_byte(FIRMWARE__SYSTEM_STATUS) == 0x03:
                return
            self.p.sleep_ms(1)
        raise Vl53l4cdError("timeout waiting for FIRMWARE__SYSTEM_STATUS == 0x03")

    def sensor_init(self, bus_khz: int = I2C_KHZ_DEFAULT) -> None:
        """Initialise the sensor and leave the bus at `bus_khz`.

        The configuration block is written at I2C_KHZ_BOOT (400 kHz) because
        that is the only speed an unconfigured sensor is specified for; the
        bridge is re-timed to `bus_khz` after it. A sensor reset just re-runs
        this whole sequence."""
        self.p.set_i2c_speed(I2C_KHZ_BOOT)
        self.wait_boot()

        # The C driver writes the 91 bytes one register at a time; the sensor
        # auto-increments, so one transaction does the same job.
        self.p.wr_multi(CONFIG_ADDR, config_block())
        if bus_khz != I2C_KHZ_BOOT:
            self.p.set_i2c_speed(bus_khz)

        self.wr_byte(SYSTEM_START, 0x40)  # start VHV
        self.wait_data_ready()
        self.clear_interrupt()
        self.stop_ranging()
        self.wr_byte(VHV_CONFIG__TIMEOUT_MACROP_LOOP_BOUND, 0x09)
        self.wr_byte(0x000B, 0x00)
        self.wr_word(0x0024, 0x0500)

        self.set_range_timing(50, 0)

    # ── ranging ──────────────────────────────────────────────────────────────

    def clear_interrupt(self) -> None:
        self.wr_byte(SYSTEM__INTERRUPT_CLEAR, 0x01)

    def start_ranging(self) -> None:
        # 0 = continuous, anything else = autonomous low power.
        mode = 0x21 if self.rd_dword(INTERMEASUREMENT_MS) == 0 else 0x40
        self.wr_byte(SYSTEM_START, mode)

    def stop_ranging(self) -> None:
        self.wr_byte(SYSTEM_START, 0x80)

    def check_for_data_ready(self) -> bool:
        int_pol = 0 if ((self.rd_byte(GPIO_HV_MUX__CTRL) & 0x10) >> 4) == 1 else 1
        return (self.rd_byte(GPIO__TIO_HV_STATUS) & 1) == int_pol

    def wait_data_ready(self, timeout_ms: int = 1000) -> None:
        for _ in range(max(1, timeout_ms)):
            if self.check_for_data_ready():
                return
            self.p.sleep_ms(1)
        raise Vl53l4cdError("timeout waiting for data ready")

    def get_result(self) -> ResultsData:
        """One block read instead of the C driver's six register reads — the
        sensor auto-increments and the decoding is identical."""
        return parse_result_block(self.p.rd_multi(RESULT_BLOCK_ADDR, RESULT_BLOCK_LEN))

    # ── timing ───────────────────────────────────────────────────────────────

    def set_range_timing(self, timing_budget_ms: int, inter_measurement_ms: int) -> None:
        clock_pll = (
            self.rd_word(RESULT__OSC_CALIBRATE_VAL)
            if inter_measurement_ms > 0
            else 0
        )
        a, b, inter_raw = range_timing_registers(
            timing_budget_ms,
            inter_measurement_ms,
            self.rd_word(OSC_FREQUENCY),
            clock_pll,
        )
        self.wr_dword(INTERMEASUREMENT_MS, inter_raw)
        self.wr_word(RANGE_CONFIG_A, a)
        self.wr_word(RANGE_CONFIG_B, b)

    def get_range_timing(self) -> tuple[int, int]:
        """→ (timing_budget_ms, inter_measurement_ms)."""
        return decode_range_timing(
            self.rd_dword(INTERMEASUREMENT_MS),
            self.rd_word(RESULT__OSC_CALIBRATE_VAL),
            self.rd_word(OSC_FREQUENCY),
            self.rd_word(RANGE_CONFIG_A),
        )

    # ── offset ───────────────────────────────────────────────────────────────

    def set_offset(self, offset_mm: int) -> None:
        self.wr_word(RANGE_OFFSET_MM, offset_raw(offset_mm))
        self.wr_word(INNER_OFFSET_MM, 0)
        self.wr_word(OUTER_OFFSET_MM, 0)

    def get_offset(self) -> int:
        return decode_offset(self.rd_word(RANGE_OFFSET_MM))

    # ── crosstalk ────────────────────────────────────────────────────────────

    def set_xtalk(self, xtalk_kcps: int) -> None:
        self.wr_word(XTALK_X_PLANE_GRADIENT_KCPS, 0x0000)
        self.wr_word(XTALK_Y_PLANE_GRADIENT_KCPS, 0x0000)
        self.wr_word(XTALK_PLANE_OFFSET_KCPS, xtalk_raw(xtalk_kcps))

    def get_xtalk(self) -> int:
        return decode_xtalk(self.rd_word(XTALK_PLANE_OFFSET_KCPS))

    # ── thresholds ───────────────────────────────────────────────────────────

    def set_detection_thresholds(
        self, distance_low_mm: int, distance_high_mm: int, window: int
    ) -> None:
        self.wr_byte(SYSTEM__INTERRUPT, window)
        self.wr_word(THRESH_HIGH, distance_high_mm)
        self.wr_word(THRESH_LOW, distance_low_mm)

    def get_detection_thresholds(self) -> tuple[int, int, int]:
        """→ (distance_low_mm, distance_high_mm, window)."""
        high = self.rd_word(THRESH_HIGH)
        low = self.rd_word(THRESH_LOW)
        return low, high, self.rd_byte(SYSTEM__INTERRUPT) & 0x07

    def set_signal_threshold(self, signal_kcps: int) -> None:
        self.wr_word(MIN_COUNT_RATE_RTN_LIMIT_MCPS, signal_threshold_raw(signal_kcps))

    def get_signal_threshold(self) -> int:
        return decode_signal_threshold(self.rd_word(MIN_COUNT_RATE_RTN_LIMIT_MCPS))

    def set_sigma_threshold(self, sigma_mm: int) -> None:
        self.wr_word(RANGE_CONFIG__SIGMA_THRESH, sigma_threshold_raw(sigma_mm))

    def get_sigma_threshold(self) -> int:
        return decode_sigma_threshold(self.rd_word(RANGE_CONFIG__SIGMA_THRESH))

    # ── temperature ──────────────────────────────────────────────────────────

    def start_temperature_update(self) -> None:
        """Recommended after a >8 °C ambient change (ST Example_3)."""
        self.wr_byte(VHV_CONFIG__TIMEOUT_MACROP_LOOP_BOUND, 0x81)
        self.wr_byte(0x000B, 0x92)
        self.start_ranging()
        self.wait_data_ready()
        self.clear_interrupt()
        self.stop_ranging()
        self.wr_byte(VHV_CONFIG__TIMEOUT_MACROP_LOOP_BOUND, 0x09)
        self.wr_byte(0x000B, 0x00)

    # ── calibration (VL53L4CD_calibration.c) ─────────────────────────────────

    def _collect(
        self,
        nb_samples: int,
        on_sample: Callable[[int, ResultsData], None],
        timeout_ms: int = 5000,
    ) -> None:
        """The ranging loop both calibrations share: data-ready, GetResult,
        ClearInterrupt, `nb_samples` times."""
        self.start_ranging()
        for i in range(nb_samples):
            self.wait_data_ready(timeout_ms)
            result = self.get_result()
            self.clear_interrupt()
            on_sample(i, result)
        self.stop_ranging()

    def calibrate_offset(self, target_dist_mm: int, nb_samples: int = 20) -> int:
        if not 5 <= nb_samples <= 255 or not 10 <= target_dist_mm <= 1000:
            raise Vl53l4cdError("nb_samples must be 5..255, target 10..1000 mm")

        self.wr_word(RANGE_OFFSET_MM, 0)
        self.wr_word(INNER_OFFSET_MM, 0)
        self.wr_word(OUTER_OFFSET_MM, 0)

        self._collect(10, lambda i, r: None)  # device heat loop

        distances: list[int] = []
        self._collect(nb_samples, lambda i, r: distances.append(r.distance_mm))

        offset_mm = target_dist_mm - sum(distances) // nb_samples
        self.wr_word(RANGE_OFFSET_MM, offset_raw(offset_mm))
        return offset_mm

    def calibrate_xtalk(self, target_dist_mm: int, nb_samples: int = 20) -> int:
        if not 5 <= nb_samples <= 255 or not 10 <= target_dist_mm <= 5000:
            raise Vl53l4cdError("nb_samples must be 5..255, target 10..5000 mm")

        self.wr_word(XTALK_PLANE_OFFSET_KCPS, 0)  # disable compensation

        self._collect(10, lambda i, r: None)  # device heat loop

        samples: list[ResultsData] = []

        def keep(i: int, r: ResultsData) -> None:
            # Discard invalid measurements and the first frame.
            if r.range_status == 0 and i > 0:
                samples.append(r)

        self._collect(nb_samples, keep)

        if not samples:
            raise Vl53l4cdError("xtalk calibration failed: no valid samples")

        n = float(len(samples))
        avg_distance = sum(s.distance_mm for s in samples) / n
        avg_spad_nb = sum(s.number_of_spad for s in samples) / n
        avg_signal = sum(s.signal_rate_kcps for s in samples) / n

        tmp_xtalk = (1.0 - avg_distance / float(target_dist_mm)) * (avg_signal / avg_spad_nb)
        if tmp_xtalk > 127:  # 127 kcps is the max xtalk value (65536/512)
            raise Vl53l4cdError(f"xtalk calibration failed: {tmp_xtalk:.1f} kcps > 127")

        self.wr_word(XTALK_PLANE_OFFSET_KCPS, int(tmp_xtalk * 512.0) & 0xFFFF)
        return round(tmp_xtalk)
