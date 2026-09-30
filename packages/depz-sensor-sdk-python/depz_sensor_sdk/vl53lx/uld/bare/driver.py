"""
uld/vl53lx/driver.py — the SensorDriver the host talks to, on top of BareDriver.

`core.BareDriver` is ST's low-level driver: it configures the die and hands
back a 24-bin histogram frame, and `hist.py` turns that frame into targets.
Neither of them knows anything about the session layer, the CLI or the GUI.
This module is the adapter between the two — the same contract `uld/l4.py` and
`uld/l1.py` implement, so ranging, streaming and every UI work on the histogram
products without a special case.

Two products run this driver: VL53L3CX and VL53L4CX. Over I2C they are the same
machine; which one is on the board comes from `uld/registry.py`, never from the
model id, because the die ids do not separate the family cleanly.

**What this driver gives that the ULP/ULD one does not:** several targets per
frame. What it costs: the range is about 12..16 mm shorter than the ULP driver
reports on our boards (the offset calibration these boards ship without — see
the plan), and the status is `6 (no wrap-around check done)` rather than
`0 (valid)`, because the checks that would upgrade it need a frame history the
BareDriver does not keep.
"""

import time

from depz_sensor_sdk.vl53lx.uld.base import Measurement, SensorDriver, Vl53Error
from depz_sensor_sdk.vl53lx.uld.bare import hist
from depz_sensor_sdk.vl53lx.uld.bare.core import (BareDriver, CLEAR_RANGE_INT, DISTANCE_MODES,
                             HISTOGRAM_BIN_DATA_I2C_INDEX,
                             HISTOGRAM_BIN_DATA_I2C_SIZE_BYTES,
                             TIMING_DIVISOR, TIMING_GUARD_US)
from depz_sensor_sdk.vl53lx._link import I2C_KHZ_BOOT

# The VL53L1 die's own registers, the few this adapter needs by name.
GPIO_HV_MUX__CTRL       = 0x0030
GPIO__TIO_HV_STATUS     = 0x0031
SYSTEM__INTERRUPT_CLEAR = 0x0086
FIRMWARE__SYSTEM_STATUS = 0x00E5
PAD_I2C_HV__CONFIG      = 0x002D

# Bits 2 and 5 (counting from 1), the same byte `VL53L4CD_I2C_FAST_MODE_PLUS`
# writes to this register in the C ULD. ST's histogram preset leaves the pad at
# 0x00, which is why this driver used to be stuck at 400 kHz; the pad is a
# property of the die, not of the preset, so it is set here and never cleared.
# FM+ pads work at every step down to 100 kHz.
FMP_PAD_CONFIG = 0x12

# How far the 24 bins reach before the phase wraps, per preset mode. The bin
# is 199 mm wide in all three; what changes is the VCSEL period behind it.
REACH_BY_MODE_MM = {'short': 1600, 'medium': 2400, 'long': 4000}

DEFAULT_MODE = 'medium'


class VL53LX(SensorDriver):
    """VL53L3CX / VL53L4CX in histogram mode: multi-target ranging."""

    ADDR_WIDTH = 2
    # One write clears the interrupt and lets the next frame run. ST's
    # `VL53LX_clear_interrupt_and_enable_next_range()` rewrites 0x0044..0x0087
    # instead, but those 68 bytes are there to stage the next zone's dynamic
    # configuration, and we have one zone. Measured over 40 frames on both
    # dies: no misses, `result__stream_count` unbroken. See the plan.
    CLEAR_STEPS = ((SYSTEM__INTERRUPT_CLEAR, CLEAR_RANGE_INT),)
    MAX_KHZ = 1000              # with Fast Mode Plus on the sensor pad
    SUPPORTS = frozenset({'mode', 'timing'})
    # This product's ranging modes are device preset modes: they pick how far
    # the 24 bins reach before the phase wraps. Applied like the timing
    # budget, before ranging starts.
    MODES = DISTANCE_MODES
    # TIMING_GUARD_US comes off the budget before it is divided, and the range
    # timeout is capped at FDA_MAX_TIMING_BUDGET_US: 2..551 ms. Rounded inwards.
    BUDGET_MS = (2, 550)
    HISTOGRAM = True

    def __init__(self, platform, product: str):
        super().__init__(platform, product)
        self.bare = BareDriver(platform)
        self._mode = DEFAULT_MODE
        self._budget_ms = 33
        self._inter_ms = 0
        # The A/B alternation rides on the ll-driver state: `get_histogram_bin_data()`
        # reads the bin sequence and the VCSEL period out of it, and in ST's
        # polled loop it is advanced by the call that clears the interrupt.
        # Here the clear is one register write — by the bridge, when streaming
        # — so the state is advanced when a frame arrives instead. Which frame
        # that is comes from the device's own `result__stream_count`, not from
        # a call count: `cmd_stream` decodes the same sample twice, once to
        # print and once to count the valid ones, and that must not step the
        # state twice.
        self._last_stream_count = None
        # The phase and event consistency checks read the frame before this
        # one; the BareDriver keeps no history, so it lives here.
        self._history = hist.FrameHistory()

    # ── identity ──
    def model_id(self) -> int:
        return self.p.rd_word(0x010F)

    # ── lifecycle ──
    def wait_boot(self, timeout_s: float = 1.0):
        deadline = time.monotonic() + timeout_s
        while True:
            if self.p.rd_byte(FIRMWARE__SYSTEM_STATUS) == 0x03:
                return
            if time.monotonic() > deadline:
                raise Vl53Error('timeout waiting for FIRMWARE__SYSTEM_STATUS == 0x03')
            self.p.sleep_ms(1)

    def sensor_init(self):
        """Reset the part, configure it for histogram ranging and leave the
        bus at this die's ceiling.

        A reset, not just a boot check: whatever ran before left the sensor
        ranging under a different configuration, and the BareDriver's register
        image assumes the reset defaults underneath it.

        The bus is raised only after the FM+ pad byte has reached the device,
        the same order `uld/l4.py` keeps: everything above runs at the 400 kHz
        an unconfigured part is specified for.
        """
        self.p.set_addr_width(self.ADDR_WIDTH)
        self.p.set_i2c_speed(I2C_KHZ_BOOT)
        self.p.xshut_reset()
        self.wait_boot()

        self.bare.data_init()
        self.set_mode(self._mode)
        self.p.wr_byte(PAD_I2C_HV__CONFIG, FMP_PAD_CONFIG)
        if self.MAX_KHZ != I2C_KHZ_BOOT:
            self.p.set_i2c_speed(self.MAX_KHZ)

    def set_mode(self, name: str):
        """short / medium / long — the device preset mode, which sets how far
        the 24 bins reach before the phase wraps (1.6 / 2.4 / 4.0 m). The bin
        itself is 199 mm wide in all three, so this does not change how far
        apart two targets have to be to be told apart."""
        if name not in DISTANCE_MODES:
            raise Vl53Error(f'no such mode: {name} '
                            f'(have {", ".join(self.MODES)})')
        self._mode = name
        self.bare.set_distance_mode(name)
        # The preset rewrites the whole static config, pad byte included, and
        # that image is what start_ranging() puts on the wire in one 135-byte
        # write. Put FM+ back or the first such write would switch the pad off
        # under a 1 MHz bus.
        self.bare.img.static_config.pad_i2c_hv__config = FMP_PAD_CONFIG
        self.set_range_timing(self._budget_ms, self._inter_ms)

    def get_mode(self) -> str:
        return self._mode

    def reach_mm(self):
        return REACH_BY_MODE_MM[self._mode]

    # ── driver-specific readout ──
    def driver_info(self) -> dict:
        """The two numbers this driver has that the contract has no field
        for: how long one range actually runs, and the MM1/MM2 offset the
        histogram post-processing applies."""
        return {
            'range timeout us': self.bare.range_config_timeout_us,
            'MM1/MM2 offset mm': self.bare.hpp.range_offset_mm / 4,
        }

    def start_ranging(self):
        self._last_stream_count = None
        self._history.reset()
        self.bare.load_patch()
        self.bare.init_and_start_range()
        # The interrupt line comes out of reset already asserted, so the first
        # data-ready is stale and would read the untouched register file as a
        # frame. One clear deasserts it; this is what ST's
        # ClearInterruptAndStartMeasurement is for.
        self.bare.clear_interrupt_and_start_next_range()

    def stop_ranging(self):
        self.bare.stop_range()
        self.bare.unload_patch()

    # ── data ──
    def stream_block(self) -> tuple:
        return HISTOGRAM_BIN_DATA_I2C_INDEX, HISTOGRAM_BIN_DATA_I2C_SIZE_BYTES

    def decode(self, raw: bytes) -> Measurement:
        return self.to_measurement(self.bin_data(raw))

    def read_measurement(self) -> Measurement:
        return self.decode(None)

    def bin_data(self, raw=None):
        """The raw frame, for callers that want the bins themselves — the
        `--bins` printout and the GUI's histogram panel. `raw` is the streamed
        block when the bridge read it; None reads it here."""
        if raw is None:
            raw = self.p.rd_multi(*self.stream_block())
        stream_count = raw[3]                   # result__stream_count
        if stream_count != self._last_stream_count:
            if self._last_stream_count is not None:
                # What `clear_interrupt_and_start_next_range()` does to the
                # state machine, without the register writes: one frame further
                # on, which flips the A/B timing status and steps the bin
                # sequence.
                self.bare.state.update_rd(
                    self.bare.img.system_control.system__mode_start,
                    self.bare.img.dynamic_config.system__grouped_parameter_hold)
            self._last_stream_count = stream_count
        return self.bare.get_histogram_bin_data(raw)

    def to_measurement(self, bins) -> Measurement:
        results = hist.process_data(bins, self.bare.hpp)
        # Once per frame, and only here: the history is a state machine, and
        # decoding the same frame twice would step it twice.
        self._history.apply(results, bins, self.bare.state.rd_device_state,
                            self.bare.hpp)
        return hist.to_measurement(results, bins)

    # ── interrupt ──
    def check_for_data_ready(self) -> bool:
        int_pol = 0 if ((self.p.rd_byte(GPIO_HV_MUX__CTRL) & 0x10) >> 4) == 1 else 1
        return (self.p.rd_byte(GPIO__TIO_HV_STATUS) & 1) == int_pol

    def clear_interrupt(self):
        self.p.wr_byte(SYSTEM__INTERRUPT_CLEAR, CLEAR_RANGE_INT)

    # ── timing ──
    def set_range_timing(self, timing_budget_ms: int, inter_measurement_ms: int):
        """The family-wide timing call. The budget covers the whole
        measurement, of which the range timeout is one sixth after a fixed
        guard comes off — ST's own arithmetic, in `BareDriver`."""
        self._budget_ms = timing_budget_ms
        self._inter_ms = inter_measurement_ms
        self.bare.set_measurement_timing_budget_us(timing_budget_ms * 1000)
        # 0 means continuous here as everywhere else in the tool; the device
        # wants a period, and back-to-back is a period of zero.
        self.bare.set_inter_measurement_period_ms(inter_measurement_ms)

    def get_range_timing(self) -> tuple:
        """-> (timing budget ms, inter-measurement ms)."""
        budget_us = (self.bare.range_config_timeout_us * TIMING_DIVISOR
                     + TIMING_GUARD_US)
        return budget_us // 1000, self.bare.inter_measurement_period_ms
