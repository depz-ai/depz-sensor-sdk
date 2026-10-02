"""VL53L 1D ToF family — VL53L0X, VL53L1CX, VL53L1CB, VL53L3CX, VL53L4CD,
VL53L4CX — over the `APP_VL53L0_4` register bridge (contracts/12).

The MCU is a thin I2C bridge that knows no sensor; every ULD runs here on the
host. The ports in `uld/` are absorbed from the firmware repo's hardware-proven
host (`tof_vl53l0_4/tools/uld/`), unchanged except for their imports; `_link`
gives them the bridge they were written against.

Two axes pick how a board is driven (see `uld/registry.py`):

* **product** — whose parameter set to load. Normally the part the board's
  device name carries; naming a neighbour borrows its driver (a VL53L4CX run
  as ``product="VL53L4CD"`` gets the light ULD with its calibrations).
* **driver kind** — ``"uld"`` (Ultra Lite), ``"ulp"`` (Ultra Low Power, L3CX
  only) or ``"histogram"`` (ST's Bare Driver: the die hands over 24 photon
  bins and the host finds the targets — several per frame).

Every product answers with the same `Vl53lxMeasurement`. What differs is
`supports(group)`: ask it before offering an offset, a ROI, a mode...
"""

from __future__ import annotations

import queue
import time
from dataclasses import dataclass, field
from typing import Any, Callable

from ..device import DeviceBase, StreamIterator, StreamQueue
from ..errors import DepzError, DepzTimeoutError, LinkClosedError
from ..protocol.vl53lx import (
    XSHUT_OFF,
    XSHUT_ON,
    XSHUT_RESET,
    StreamData,
    Vl53lxCmd,
    Vl53lxInfo,
    Vl53lxRpt,
    pack_start_stream,
    pack_xshut,
)
from ..transport import Packet
from ._link import BridgeLink, ProtocolError, Vl53Error
from .uld import registry
from .uld.base import BridgePlatform, Measurement, SensorDriver, Target

__all__ = [
    "Vl53lx",
    "Vl53l0x",
    "Vl53l1cx",
    "Vl53l1cb",
    "Vl53l3cx",
    "Vl53l4cx",
    "Vl53lxMeasurement",
    "Vl53lxInfo",
    "Target",
    "Vl53Error",
    "ProtocolError",
    "PRODUCTS",
    "DRIVER_KINDS",
    "PLOTTABLE_STATUSES",
    "XSHUT_OFF",
    "XSHUT_ON",
    "XSHUT_RESET",
    "primary_target",
    "plot_distances",
]

PRODUCTS = registry.PRODUCTS
DRIVER_KINDS = registry.DRIVER_KINDS

#: Statuses that mean "this distance is real". A histogram product reports 6
#: on the first frame of a stream (no predecessor for the wrap check) and 11
#: when the pulse it ranged was two merged targets. Use this, not
#: ``status == 0``, as the validity test on histogram products.
PLOTTABLE_STATUSES = (0, 6, 11)

#: Slice length for close-aware blocking waits (see Vl53l8cx.get_frame).
_CLOSE_POLL_S = 0.05


@dataclass(frozen=True)
class Vl53lxMeasurement:
    """One ranging result, the same shape for every product of the family.

    On the histogram driver `targets` holds every return of the frame,
    strongest signal first, and the top-level fields repeat ``targets[0]``;
    `bins` carries the raw 24-bin histogram. On the light drivers `targets` is
    empty and `bins` is None. `extra` is driver-specific (stream count,
    per-SPAD rates...) — show it, don't branch on it."""

    timestamp_us: int  # MCU uptime at the INT edge (stream) / read (poll)
    distance_mm: int
    status: int
    status_text: str
    signal_kcps: float
    ambient_kcps: float
    sigma_mm: float
    spads: float
    targets: tuple[Target, ...] = ()
    extra: dict = field(default_factory=dict)
    bins: Any = None

    @property
    def valid(self) -> bool:
        """``status == 0``. On histogram products prefer `plottable`."""
        return self.status == 0

    @property
    def plottable(self) -> bool:
        """The frame has a usable range (PLOTTABLE_STATUSES)."""
        return self.status in PLOTTABLE_STATUSES

    @staticmethod
    def _from(timestamp_us: int, m: Measurement, bins: Any = None) -> "Vl53lxMeasurement":
        return Vl53lxMeasurement(
            timestamp_us=timestamp_us,
            distance_mm=m.distance_mm,
            status=m.status,
            status_text=m.status_text,
            signal_kcps=m.signal_kcps,
            ambient_kcps=m.ambient_kcps,
            sigma_mm=m.sigma_mm,
            spads=m.spads,
            targets=tuple(m.targets),
            extra=dict(m.extra),
            bins=bins,
        )


def plot_distances(m: Vl53lxMeasurement) -> list[int]:
    """The distances a chart should draw for one measurement: the plottable
    targets in driver order (invalid ones dropped, not holding a slot), or the
    single distance on a light driver. On the short/medium histogram presets
    slot 0 alternates between a real target and a phase artefact — use this
    instead of ``targets[0]``."""
    if m.targets:
        return [t.distance_mm for t in m.targets if t.status in PLOTTABLE_STATUSES]
    return [m.distance_mm] if m.plottable else []


def primary_target(m: Vl53lxMeasurement) -> Target | None:
    """The one target a single-number readout should show — the first
    plottable one — or None when the frame produced nothing usable."""
    for t in m.targets:
        if t.status in PLOTTABLE_STATUSES:
            return t
    return None


class Vl53lx(DeviceBase):
    """A board of the VL53L 1D family (firmware ``APP_VL53L0_4``).

    ``init()`` binds the (product, driver kind) pair and runs the ULD's
    sensor_init; ``configure()`` re-initialises and applies the ranging
    configuration — call it before every run. Then ``start_ranging()`` arms
    the MCU stream and measurements arrive via ``on_measurement`` /
    ``measurements()`` / ``get_measurement()``. Configuration must not change
    while ranging: the stream owns the bus (contract 12).

    This generic class serves any product (the product is read from the board's
    device name); the per-product subclasses (`Vl53l0x`...) fix it."""

    #: The product this class drives; None = read it from the device name.
    PRODUCT: str | None = None

    def _init_subclass_state(self) -> None:
        self._bridge = BridgeLink(self)
        self._platform = BridgePlatform(self._bridge)
        self._driver: SensorDriver | None = None
        self._product: str | None = None
        self._driver_kind: str | None = None
        self._caveat: str | None = None
        self._board_name: str | None = None
        self._measure_cbs: list[Callable[[Vl53lxMeasurement], None]] = []
        self._measure_queues: list[StreamQueue] = []
        self._ranging = False
        self._stream_parse_errors = 0

    # ── identity ─────────────────────────────────────────────────────────────

    @property
    def board_name(self) -> str:
        """The board's device name (bootloader metablock), e.g.
        ``DEPZ ToF Sensor VL53L4CX USB v2.1 TOVJALN523``."""
        if self._board_name is None:
            self._board_name = self.get_device_name()
        return self._board_name

    @property
    def detected(self) -> str | None:
        """The product the device name carries, or None on an unstamped
        board (then pass ``product=`` to init)."""
        return registry.product_from_board_name(self.board_name)

    @property
    def product(self) -> str | None:
        """The product init() bound (None before init)."""
        return self._product

    @property
    def driver_kind(self) -> str | None:
        return self._driver_kind

    @property
    def driver(self) -> SensorDriver:
        """The ULD port in use — escape hatch for product-specific calls
        (L0X reference SPADs, L1 distance modes by register...)."""
        if self._driver is None:
            raise DepzError("call init() first")
        return self._driver

    @property
    def initialized(self) -> bool:
        return self._driver is not None

    def driver_kinds(self, product: str | None = None) -> tuple[str, ...]:
        """The driver kinds a product has (default: this board's)."""
        name = product or self.PRODUCT or self.detected
        return registry.driver_kinds(name) if name else ()

    # ── lifecycle ────────────────────────────────────────────────────────────

    def init(self, driver: str | None = None, *, product: str | None = None) -> None:
        """Bind the (product, driver kind) pair and initialise the sensor.

        `product` defaults to this class's product, else the board's device
        name; `driver` defaults to the product's first kind in DRIVER_KINDS
        order (``uld``, else ``ulp``, else ``histogram``). A pair the table has
        no row for raises NotImplementedError naming what the product has."""
        self._require_not_ranging()
        name = product or self.PRODUCT or self.detected
        if name is None:
            raise DepzError(
                f"board {self.board_name!r} carries no product number — "
                "pass product= explicitly"
            )
        kind = driver or registry.driver_kinds(name)[0]
        driver_cls, caveat = registry.driver_for(name, kind)
        # Sticky on the bridge and 2 after a reset: set before the first
        # register access, model id included.
        self._bridge.set_addr_width(driver_cls.ADDR_WIDTH)
        drv = driver_cls(self._platform, name)
        self._sensor_init(drv)
        self._driver, self._product, self._driver_kind, self._caveat = drv, name, kind, caveat

    def _sensor_init(self, drv) -> None:
        # A reset makes the die NACK for a moment (the VL53L0X soft reset does
        # it); the driver waits those out, and they are no bus fault.
        drv.sensor_init()
        self._bridge.clear_i2c_errors()

    def identify(self) -> dict:
        """Everything about what is connected (needs init()). `model_id_ok`
        is a cross-check only: L1CX/L1CB and L4CD/L4CX share their ids."""
        drv = self.driver
        model_id = drv.model_id()
        return {
            "board": self.board_name,
            "detected": self.detected,
            "product": self._product,
            "driver": self._driver_kind,
            "driver_class": type(drv).__name__,
            "kinds": registry.driver_kinds(self._product),
            "model_id": model_id,
            "model_id_ok": registry.model_id_ok(self._product, model_id),
            "supports": frozenset(drv.SUPPORTS),
            "modes": tuple(drv.MODES),
            "reach_mm": registry.reach_mm(self.detected or self._product),
            "driver_reach_mm": drv.reach_mm(),
            "budget_ms": tuple(drv.BUDGET_MS),
            "budget_choices": self.budget_choices(),
            "histogram": drv.HISTOGRAM,
            "max_khz": drv.MAX_KHZ,
            "caveat": self._caveat,
        }

    def notes(self) -> list[str]:
        """The lines a UI should show about the chosen pair: product named by
        hand, a borrowed driver, a pair that reaches less than the board is
        rated for, the driver's caveat."""
        drv = self.driver
        out = []
        if self.detected is None:
            out.append(f"{self._product}: board name carries no product number, named by hand")
        elif self.detected != self._product:
            out.append(
                f"{self._product}: borrowing this driver - the board says it is a {self.detected}"
            )
        rated = registry.reach_mm(self.detected or self._product)
        reach = drv.reach_mm()
        if rated and reach and reach < rated:
            out.append(
                f"{self._product}/{self._driver_kind}: this pair reaches {reach} mm, the board "
                f"is rated {rated} mm - past {reach} mm the phase is out of the valid window "
                "and frames come back with status 4"
            )
        if self._caveat:
            out.append(f"{self._product}/{self._driver_kind}: {self._caveat}")
        return out

    def supports(self, group: str) -> bool:
        """Whether this product/driver serves an optional capability group:
        mode, timing, offset, calib_offset, xtalk, calib_xtalk, thresholds,
        signal_thresh, sigma_thresh, roi, temp_update, refspad."""
        return group in self.driver.SUPPORTS

    @property
    def modes(self) -> tuple[str, ...]:
        """Named ranging modes, first = what init leaves; () if none."""
        return tuple(self.driver.MODES)

    # ── sensor power (XSHUT) and bridge diagnostics ─────────────────────────

    def xshut(self, action: int) -> None:
        """Drive XSHUT: XSHUT_OFF / XSHUT_ON / XSHUT_RESET (1 ms pulse + 5 ms
        wait; the host confirms the boot). OFF and RESET stop the stream; the
        sensor then holds none of the configuration — init() again."""
        self.request(Vl53lxCmd.XSHUT, pack_xshut(action), ok_completes=True, timeout=1.0)
        self._ranging = False
        self._driver = None

    def bridge_info(self) -> Vl53lxInfo:
        """RPT_VL53_INFO — the bridge's own counters and settings; touches no
        sensor register, safe while streaming."""
        return self.request(
            Vl53lxCmd.GET_INFO,
            matcher=DeviceBase.expect_report(Vl53lxRpt.INFO, Vl53lxInfo.unpack),
        )

    # ── configuration (init() first; not while ranging) ─────────────────────

    def configure(
        self,
        budget_ms: int = 50,
        inter_ms: int = 0,
        mode: str | None = None,
        offset_mm: int | None = None,
        xtalk_kcps: int | None = None,
        signal_kcps: int | None = None,
    ) -> None:
        """Re-initialise the sensor and apply a ranging configuration.

        The re-init is deliberate: it is the only way to know what the
        configuration registers hold, and it puts the bus at the product's
        ceiling. `mode` (one of `modes`) goes on before the budget — a mode
        change rewrites the timing. `offset_mm` / `xtalk_kcps` re-apply a stored
        calibration (the sensor keeps those only until a reset).
        `signal_kcps` replaces the blob's signal threshold — the re-init puts
        it back to the default, so a lowered one goes here rather than in a
        `set_signal_threshold_kcps()` before configure: frames past the default
        threshold come back status 2 with the distance right (L1 long at ~4 m,
        a light driver borrowed onto a die without the lens it was tuned for).
        `inter_ms=0` = continuous; otherwise the period between measurements
        (must exceed the budget). An unsupported group is refused before the
        re-init."""
        self._require_not_ranging()
        drv = self.driver
        for group, value in (("offset", offset_mm), ("xtalk", xtalk_kcps),
                             ("signal_thresh", signal_kcps)):
            if value is not None:
                self._need(group)
        self._sensor_init(drv)
        if mode is not None:
            drv.set_mode(mode)
        drv.set_range_timing(budget_ms, inter_ms)
        if offset_mm is not None:
            drv.set_offset(offset_mm)
        if xtalk_kcps is not None:
            drv.set_xtalk(xtalk_kcps)
        if signal_kcps is not None:
            drv.set_signal_threshold(signal_kcps)

    def get_range_timing(self) -> tuple[int, int]:
        """→ (timing_budget_ms, inter_measurement_ms) read back from the
        sensor; 0 for the period means continuous."""
        return self.driver.get_range_timing()

    def get_mode(self) -> str | None:
        """The ranging mode in use, or None on a product without modes."""
        return self.driver.get_mode() if self.supports("mode") else None

    def budget_choices(self) -> tuple[int, ...]:
        """The only budgets accepted right now (ascending), or () when any
        integer in ``identify()['budget_ms']`` will do. Can move with the mode
        (VL53L1: 15 ms exists in short mode only)."""
        return tuple(self.driver.budget_choices())

    def snap_budget(self, budget_ms: int) -> int:
        """The nearest budget this product will actually accept."""
        choices = self.budget_choices()
        if choices:
            return min(choices, key=lambda c: abs(c - budget_ms))
        low, high = self.driver.BUDGET_MS
        return max(low, min(high, budget_ms))

    # Capability-gated passthroughs to the ULD port. Each raises DepzError on
    # a product/driver that does not serve the group (see supports()).

    def get_offset_mm(self) -> int:
        return self._need("offset").get_offset()

    def set_offset_mm(self, offset_mm: int) -> None:
        self._require_not_ranging()
        self._need("offset").set_offset(offset_mm)

    def get_xtalk_kcps(self) -> int:
        return self._need("xtalk").get_xtalk()

    def set_xtalk_kcps(self, xtalk_kcps: int) -> None:
        self._require_not_ranging()
        self._need("xtalk").set_xtalk(xtalk_kcps)

    def calibrate_offset(self, target_dist_mm: int, nb_samples: int | None = None) -> int:
        """Offset calibration against a flat target at `target_dist_mm`;
        returns the offset now programmed. Store it on the host: it lives in
        sensor RAM and is lost on reset."""
        self._require_not_ranging()
        drv = self._need("calib_offset")
        args = (target_dist_mm,) if nb_samples is None else (target_dist_mm, nb_samples)
        return drv.calibrate_offset(*args)

    def calibrate_xtalk(self, target_dist_mm: int, nb_samples: int | None = None) -> int:
        """Crosstalk calibration against a target at `target_dist_mm`;
        returns the xtalk now programmed (kcps). Store it on the host."""
        self._require_not_ranging()
        drv = self._need("calib_xtalk")
        args = (target_dist_mm,) if nb_samples is None else (target_dist_mm, nb_samples)
        return drv.calibrate_xtalk(*args)

    def get_detection_thresholds(self) -> tuple[int, int, int]:
        """→ (distance_low_mm, distance_high_mm, window)."""
        return self._need("thresholds").get_detection_thresholds()

    def set_detection_thresholds(
        self, distance_low_mm: int, distance_high_mm: int, window: int
    ) -> None:
        """Arm the distance-window interrupt: INT (and so the stream) only on
        a qualifying event. Stays armed until the next init/configure."""
        self._require_not_ranging()
        self._need("thresholds").set_detection_thresholds(
            distance_low_mm, distance_high_mm, window
        )

    def get_signal_threshold_kcps(self) -> int:
        return self._need("signal_thresh").get_signal_threshold()

    def set_signal_threshold_kcps(self, signal_kcps: int) -> None:
        self._require_not_ranging()
        self._need("signal_thresh").set_signal_threshold(signal_kcps)

    def get_sigma_threshold_mm(self) -> int:
        return self._need("sigma_thresh").get_sigma_threshold()

    def set_sigma_threshold_mm(self, sigma_mm: int) -> None:
        self._require_not_ranging()
        self._need("sigma_thresh").set_sigma_threshold(sigma_mm)

    def get_roi(self) -> tuple[int, int]:
        """→ (x, y) SPAD window size."""
        return self._need("roi").get_roi()

    def set_roi(self, x: int, y: int) -> None:
        self._require_not_ranging()
        self._need("roi").set_roi(x, y)

    def get_roi_center(self) -> int:
        return self._need("roi").get_roi_center()

    def set_roi_center(self, center_spad: int) -> None:
        self._require_not_ranging()
        self._need("roi").set_roi_center(center_spad)

    def start_temperature_update(self) -> None:
        """Re-run VHV after an ambient change over 8 °C."""
        self._require_not_ranging()
        self._need("temp_update").start_temperature_update()

    def perform_ref_spad_management(self) -> tuple:
        """VL53L0X: re-measure the reference SPADs."""
        self._require_not_ranging()
        return self._need("refspad").perform_ref_spad_management()

    # ── ranging ──────────────────────────────────────────────────────────────

    def start_ranging(self) -> None:
        """Start the sensor's ranging loop and arm the MCU stream: one
        RPT_VL53_STREAM per INT edge, followed on the MCU by the driver's
        interrupt-release writes."""
        self._require_not_ranging()
        drv = self.driver
        drv.start_ranging()
        addr, length = drv.stream_block()
        self.request(
            Vl53lxCmd.START_STREAM,
            pack_start_stream(addr, length, tuple(drv.CLEAR_STEPS)),
            ok_completes=True,
        )
        self._ranging = True

    def stop_ranging(self) -> None:
        if not self._ranging:
            return
        # Clear host state and stop the sensor even if the STOP_STREAM ack
        # fails (link hiccup): otherwise the device is wedged "ranging".
        try:
            self.request(Vl53lxCmd.STOP_STREAM, ok_completes=True)
        finally:
            self._ranging = False
            self.driver.stop_ranging()

    @property
    def ranging(self) -> bool:
        return self._ranging

    def measure_once(self, timeout: float = 1.0) -> Vl53lxMeasurement:
        """Single poll-mode measurement: start ranging, wait for data-ready,
        read, release the interrupt, stop. Raises while the stream runs."""
        self._require_not_ranging()
        drv = self.driver
        drv.start_ranging()
        try:
            drv.wait_data_ready(timeout)
            m, bins = self._read_polled(drv)
            drv.clear_interrupt()
        finally:
            drv.stop_ranging()
        return Vl53lxMeasurement._from(self._bridge.last_timestamp_us, m, bins)

    def on_measurement(self, cb: Callable[[Vl53lxMeasurement], None]) -> Callable[[], None]:
        """Subscribe to streamed measurements (reader-thread context; don't
        block). Returns an unsubscribe function."""
        self._measure_cbs.append(cb)
        return lambda: self._measure_cbs.remove(cb)

    def measurements(self, maxsize: int = 64) -> StreamIterator:
        """Blocking iterator over measurements (bounded, drop-oldest).
        Subscribes immediately — call before or after start_ranging()."""
        return StreamIterator(self._measure_queues, maxsize, lambda: self.closed)

    def get_measurement(self, timeout: float = 2.0) -> Vl53lxMeasurement:
        """Wait for the next streamed measurement. Raises DepzTimeoutError
        after `timeout`, LinkClosedError as soon as the device is closed."""
        if self.closed:
            raise LinkClosedError("device is closed")
        q = StreamQueue(1)
        self._measure_queues.append(q)
        try:
            deadline = time.monotonic() + timeout
            while True:
                remaining = deadline - time.monotonic()
                if remaining <= 0:
                    raise DepzTimeoutError(Vl53lxRpt.STREAM, timeout) from None
                try:
                    return q.get(timeout=min(_CLOSE_POLL_S, remaining))
                except queue.Empty:
                    if self.closed:
                        raise LinkClosedError(
                            "device closed while waiting for a measurement"
                        ) from None
        finally:
            self._measure_queues.remove(q)

    @property
    def stream_parse_errors(self) -> int:
        """Stream reports dropped because the driver failed to decode them."""
        return self._stream_parse_errors

    # ── internal ─────────────────────────────────────────────────────────────

    def _need(self, group: str) -> SensorDriver:
        drv = self.driver
        if group not in drv.SUPPORTS:
            raise DepzError(
                f"{self._product}/{self._driver_kind} does not support {group!r}"
                + (f" ({self._caveat})" if self._caveat else "")
            )
        return drv

    def _require_not_ranging(self) -> None:
        if self._ranging:
            raise DepzError("stop_ranging() first — the stream owns the bus")

    @staticmethod
    def _read_polled(drv: SensorDriver):
        if drv.HISTOGRAM:
            bins = drv.bin_data()
            return drv.to_measurement(bins), bins
        return drv.read_measurement(), None

    def _decode(self, drv: SensorDriver, data: bytes):
        # Decoded exactly once per sample: the histogram driver steps its own
        # frame-pair state on every frame it sees.
        if drv.HISTOGRAM:
            bins = drv.bin_data(data)
            return drv.to_measurement(bins), bins
        return drv.decode(data), None

    def _handle_report(self, pkt: Packet) -> bool:
        if pkt.cmd != Vl53lxRpt.STREAM or len(pkt.payload) < 12:
            return False
        drv = self._driver
        if drv is None:
            return True
        sample = StreamData.unpack(pkt.payload)
        try:
            m, bins = self._decode(drv, sample.data)
        except Exception:  # noqa: BLE001 — a bad sample must not kill the reader
            self._stream_parse_errors += 1
            return True
        out = Vl53lxMeasurement._from(sample.timestamp_us, m, bins)
        for cb in list(self._measure_cbs):
            cb(out)
        for q in list(self._measure_queues):
            q.put(out)
        return True


class Vl53l0x(Vl53lx):
    """VL53L0X (2 m, 1-byte register addresses, 400 kHz): ULD API 1.0.4 with
    ranging profiles (default / long-range / high-speed / high-accuracy),
    offset and crosstalk calibration, reference-SPAD management."""

    PRODUCT = "VL53L0X"


class Vl53l1cx(Vl53lx):
    """VL53L1CX (4 m): VL53L1X ULD (short / long distance modes, tabulated
    budgets, calibrations, thresholds, ROI) or the histogram driver."""

    PRODUCT = "VL53L1CX"


class Vl53l1cb(Vl53lx):
    """VL53L1CB (8 m, cover-glass module): same die and drivers as VL53L1CX;
    usually the one that needs a crosstalk calibration."""

    PRODUCT = "VL53L1CB"


class Vl53l3cx(Vl53lx):
    """VL53L3CX (3 m): ST's ULP driver (single target) or the histogram driver
    (several targets per frame)."""

    PRODUCT = "VL53L3CX"


class Vl53l4cx(Vl53lx):
    """VL53L4CX (6 m): the histogram driver only — to run it light, borrow a
    sibling's with ``init(product="VL53L4CD")`` (~1.4 m, full calibrations)."""

    PRODUCT = "VL53L4CX"


#: Class per product (VL53L4CD on this firmware uses the generic class).
CLASS_BY_PRODUCT: dict[str, type[Vl53lx]] = {
    "VL53L0X": Vl53l0x,
    "VL53L1CX": Vl53l1cx,
    "VL53L1CB": Vl53l1cb,
    "VL53L3CX": Vl53l3cx,
    "VL53L4CX": Vl53l4cx,
}
