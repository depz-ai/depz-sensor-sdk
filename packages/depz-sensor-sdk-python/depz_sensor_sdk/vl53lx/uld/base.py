"""
uld/base.py — what every sensor driver in this package shares.

BridgePlatform is ST's platform layer (RdByte/WrWord/WaitMs...) implemented on
top of vl53_link; SensorDriver is the contract the session layer talks to, and
Measurement is the one result shape every caller understands.

The contract below is the whole of it: if a method is not declared here, a
caller that is not the driver's own module has no business calling it. Two
things are deliberately NOT in it:

  * which model id this product should answer — that lives in uld/registry.py,
    next to the product table it belongs to;
  * anything a single diagnostic wanted — a driver describes the sensor, not
    the tool that prints it.
"""

import time
from collections import namedtuple

from depz_sensor_sdk.vl53lx._link import (Vl53Device, Vl53Error,   # noqa: F401  (re-exported)
                       VL53_XSHUT_RESET)

# ── ULD platform layer (Platform/platform.h) ─────────────────────────────────
class BridgePlatform:
    """VL53L4CD_RdByte/RdWord/RdDWord/WrByte/WrWord/WrDWord/WaitMs over the
    bridge. Register values are big-endian, exactly as the C driver assumes."""

    def __init__(self, dev: Vl53Device):
        self.dev = dev

    def rd_multi(self, addr: int, size: int) -> bytes:
        return self.dev.read_reg(addr, size)

    def wr_multi(self, addr: int, data: bytes):
        self.dev.write_reg(addr, data)

    def rd_byte(self, addr: int) -> int:
        return self.dev.read_reg(addr, 1)[0]

    def rd_word(self, addr: int) -> int:
        return int.from_bytes(self.dev.read_reg(addr, 2), 'big')

    def rd_dword(self, addr: int) -> int:
        return int.from_bytes(self.dev.read_reg(addr, 4), 'big')

    def wr_byte(self, addr: int, value: int):
        self.dev.write_reg(addr, bytes([value & 0xFF]))

    def wr_word(self, addr: int, value: int):
        self.dev.write_reg(addr, (value & 0xFFFF).to_bytes(2, 'big'))

    def wr_dword(self, addr: int, value: int):
        self.dev.write_reg(addr, (value & 0xFFFFFFFF).to_bytes(4, 'big'))

    def set_i2c_speed(self, speed: int):
        """Not part of the C platform layer: the bridge owns the bus timing.
        Only a driver calls this — it runs its init at the speed an
        unconfigured sensor is specified for and raises the bus to its own
        ceiling afterwards. Nothing above the driver picks a bus speed."""
        self.dev.set_i2c_speed(speed)

    def set_addr_width(self, width: int):
        """Also outside the C platform layer: how wide a register address the
        bridge puts on the wire. Sticky, so it is set once per init."""
        self.dev.set_addr_width(width)

    def xshut_reset(self):
        """Also outside the C platform layer: the bridge owns XSHUT. A driver
        needs it when the sensor stops answering altogether — a soft reset
        cannot be written to a part that no longer ACKs."""
        self.dev.xshut(VL53_XSHUT_RESET)

    @staticmethod
    def sleep_ms(ms: int):
        time.sleep(ms / 1000.0)

# ── Driver contract ──────────────────────────────────────────────────────────
Target = namedtuple(
    'Target', 'distance_mm status status_text signal_kcps ambient_kcps '
              'sigma_mm min_range_mm max_range_mm')
Target.__doc__ = """One return of a multi-target frame, in the terms the CLI
and the GUI already print. `min_range_mm`/`max_range_mm` are the edges of the
target's own pulse, which only the histogram parts know; the others repeat them
from `distance_mm`."""


class Measurement:
    """One range result, the same shape for every sensor of the family.

    On the multi-target parts (L3CX, L4CX in histogram mode) `targets` holds
    every return the frame produced, as `Target`s, closest or strongest first
    depending on the target order — `distance_mm` and the rates repeat
    `targets[0]`. On the single-target ones it stays empty. `extra` is for
    whatever a driver wants to show that nobody else has — the GUI prints it,
    nothing branches on it.
    """

    __slots__ = ('distance_mm', 'status', 'status_text', 'signal_kcps',
                 'ambient_kcps', 'sigma_mm', 'spads', 'targets', 'extra')

    def __init__(self, distance_mm=0, status=0, status_text='', signal_kcps=0,
                 ambient_kcps=0, sigma_mm=0, spads=0, targets=(), extra=None):
        self.distance_mm  = distance_mm
        self.status       = status
        self.status_text  = status_text
        self.signal_kcps  = signal_kcps
        self.ambient_kcps = ambient_kcps
        self.sigma_mm     = sigma_mm
        self.spads        = spads
        self.targets      = targets
        self.extra        = extra or {}

    @property
    def valid(self) -> bool:
        return self.status == 0

    def __repr__(self):
        return (f'<Measurement {self.distance_mm} mm status {self.status} '
                f'({self.status_text})>')


class SensorDriver:
    """Everything a caller may use on a sensor driver.

    A concrete driver implements the methods below and fills in the class
    attributes; anything else it defines belongs to that product alone and is
    reached off the concrete class, never through this contract. `SUPPORTS`
    says which optional groups the product actually has, so a caller asks
    instead of guessing from the class.
    """

    # ── what this product is, as far as the bridge is concerned ──
    ADDR_WIDTH  = 2         # bytes of register address on the wire
    CLEAR_STEPS = ()        # ((addr, value), ...) handed to START_STREAM
    MAX_KHZ     = 400       # bus ceiling; the driver runs the bus there itself

    # Optional capability groups this driver serves. The vocabulary:
    #   'mode'          — named ranging modes, see MODES below
    #   'timing'        — set/get_range_timing
    #   'offset'        — set/get_offset            'xtalk'  — set/get_xtalk
    #   'calib_offset'  — calibrate_offset          'calib_xtalk'
    #   'thresholds'    — detection windows         'roi'
    #   'signal_thresh' 'sigma_thresh' 'temp_update' 'refspad'
    SUPPORTS = frozenset()

    # The named ranging modes this product has, in the order a UI should list
    # them, first one being what init leaves behind. One axis for the whole
    # family: the VL53L0X ranging profiles, the VL53L1 distance modes and the
    # histogram preset modes are all "a name that reconfigures how this part
    # ranges", and every product has at most one of those axes. Empty when the
    # product has none, and then 'mode' is not in SUPPORTS.
    MODES = ()

    # What set_range_timing() will accept as a budget, in ms: the inclusive
    # (min, max) a UI should offer. Some products take any integer in there,
    # some only a handful of values — see budget_choices() below.
    BUDGET_MS = (10, 200)

    # True when the frame the driver decodes is a raw histogram the caller can
    # ask for as well, through bin_data(). Only the histogram parts have one,
    # and the GUI shows its bin panel on that.
    HISTOGRAM = False

    def __init__(self, platform: BridgePlatform, product: str):
        self.p = platform
        # The product the board reports, e.g. 'VL53L4CX'. It is passed in
        # rather than held as a class attribute because one driver serves
        # several products: VL53L4CD and VL53L4CX are the same machine over
        # I2C, as are VL53L1CX and VL53L1CB. uld/registry.py is the single
        # place that says which product name goes with which driver.
        self.product = product

    # ── identity ──
    def model_id(self) -> int:
        """The model-id register of this die. What value it *should* read is
        registry.TABLE — the die ids do not separate the family on their
        own (L1CX/L1CB and L4CD/L4CX share theirs), so the check belongs next
        to the product table, not here."""
        raise NotImplementedError

    # ── lifecycle ──
    def sensor_init(self):
        """Reset and configure the sensor, and leave the bus at the highest
        speed this product runs at. Safe to re-run: it is the only way to know
        what the configuration registers hold after a previous session."""
        raise NotImplementedError

    def start_ranging(self):  raise NotImplementedError
    def stop_ranging(self):   raise NotImplementedError

    # ── interrupt ──
    def check_for_data_ready(self) -> bool: raise NotImplementedError

    def wait_data_ready(self, timeout_s: float = 1.0):
        deadline = time.monotonic() + timeout_s
        while not self.check_for_data_ready():
            if time.monotonic() > deadline:
                raise Vl53Error('timeout waiting for data ready')
            self.p.sleep_ms(1)

    def clear_interrupt(self): raise NotImplementedError

    # ── data ──
    def stream_block(self) -> tuple:
        """(addr, len) — the register block the bridge streams on each INT."""
        raise NotImplementedError

    def decode(self, raw: bytes) -> Measurement:
        """Streamed block bytes -> Measurement."""
        raise NotImplementedError

    def read_measurement(self) -> Measurement:
        """One polled result. Default: read the stream block and decode it."""
        addr, length = self.stream_block()
        return self.decode(self.p.rd_multi(addr, length))

    # ── timing ──
    def set_range_timing(self, timing_budget_ms: int, inter_measurement_ms: int):
        """Budget of one measurement, and the period between two. An
        inter-measurement of 0 means continuous on every product."""
        raise NotImplementedError

    def get_range_timing(self) -> tuple:
        """-> (timing_budget_ms, inter_measurement_ms)."""
        raise NotImplementedError

    def budget_choices(self) -> tuple:
        """The only budgets this product accepts right now, ascending, or ()
        when any integer inside BUDGET_MS will do.

        It is a method and not a constant because on some products the set
        depends on the mode in use: the VL53L1 has a 15 ms budget in short
        mode and none in long. A UI offering a budget should snap to this;
        set_range_timing() refuses anything else rather than rounding, because
        silently ranging on a different budget than asked is worse.
        """
        return ()

    # ── ranging mode ──
    def set_mode(self, name: str):
        """Apply one of MODES. Only when 'mode' is in SUPPORTS."""
        raise NotImplementedError

    def get_mode(self) -> str:
        """The mode currently applied, one of MODES."""
        raise NotImplementedError

    def reach_mm(self):
        """How far this driver's configuration can actually measure, mm, or
        None when it has not been characterised.

        Not the same as the product's rating (registry.TABLE): the VCSEL
        period in the configuration blob is what sets the unambiguous range, so
        a driver built around one product's blob reaches that product's
        distance even on a die rated for more. Past it the phase is out of the
        valid window and the frames come back with status 4.
        """
        return None

    # ── driver-specific readout ──
    def driver_info(self) -> dict:
        """Numbers this driver can show that the contract has no field for —
        a UI prints `name: value` and nothing branches on it. Empty by
        default; it is how a driver says something without growing the
        contract a method nobody else implements."""
        return {}
