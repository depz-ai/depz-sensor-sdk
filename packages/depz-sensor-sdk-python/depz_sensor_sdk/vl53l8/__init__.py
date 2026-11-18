"""VL53L8CX/CH ToF sensor: host-side ULD over the firmware register bridge.

The MCU is a thin SPI bridge (contracts/04): the full ST ULD driver runs here
on the host (`uld.py`, absorbed from the firmware repo's proven Python port).
Frame streaming is INT-driven push from the device, reassembled from ≤1528-
byte chunks.
"""

from __future__ import annotations

import base64
import struct
from dataclasses import dataclass
from types import MappingProxyType
from typing import TYPE_CHECKING, Any, Callable

import numpy as np
from numpy.typing import NDArray

from .._bridge import RegisterBridge, StreamArm
from ..device import DepzDevice
from ..stream import Stream
from ..errors import DepzError, InvalidStateError
from ..protocol.vl53l8 import (
    CHUNK_SIZE,
    VL53L8_BRIDGE,
    STREAM_CHUNK_MAX,
    FrameChunk,
    FrameReassembler,
    Vl53l8Cmd,
    Vl53l8Rpt,
    pack_start_stream,
)
from ..transport import Packet
from . import cnh as cnh_module
from . import uld as uld_module
from .cnh import CnhConfig
from .uld import (
    I2C_VARIANTS,
    POWER_MODE_DEEP_SLEEP,
    POWER_MODE_SLEEP,
    POWER_MODE_WAKEUP,
    RANGING_MODE_AUTONOMOUS,
    RANGING_MODE_CONTINUOUS,
    RESOLUTION_4X4,
    RESOLUTION_8X8,
    TARGET_ORDER_CLOSEST,
    TARGET_ORDER_STRONGEST,
    VL53L8CX,
    DetectionThreshold,
    MotionConfig,
    MotionIndicator,
    Vl53l8cxError,
    silicon_id_matches,
)

__all__ = [
    "Vl53l8cx",
    "Vl53l8ch",
    "Vl53l8Frame",
    "CnhConfig",
    "Vl53l8cxError",
    "RESOLUTION_4X4",
    "RESOLUTION_8X8",
    "RANGING_MODE_CONTINUOUS",
    "RANGING_MODE_AUTONOMOUS",
    "TARGET_ORDER_CLOSEST",
    "TARGET_ORDER_STRONGEST",
    "POWER_MODE_SLEEP",
    "POWER_MODE_WAKEUP",
    "POWER_MODE_DEEP_SLEEP",
]

MIN_RANGING_FREQUENCY_HZ = 2  # below this the sensor never streams (contract 04)
#: Highest ranging frequency per resolution (contract 11 §3; the same silicon
#: limit on VL53L8): 60 Hz at 4×4, 15 Hz at 8×8.
MAX_RANGING_FREQUENCY_HZ = MappingProxyType({RESOLUTION_4X4: 60, RESOLUTION_8X8: 15})


@dataclass(frozen=True, kw_only=True)
class Vl53l8Frame:
    """One parsed ranging frame. Arrays are sized to the active resolution
    (16 or 64 zones); zone index runs row-major (see datasheet zone maps)."""

    timestamp_us: int
    resolution: int  # 16 | 64
    distance_mm: NDArray[np.int32]  # mm, shape (zones,); negative → 0, as ST
    target_status: NDArray[np.uint8]  # 5/9 = valid, 255 = no target
    nb_target_detected: NDArray[np.uint8]
    signal_per_spad: NDArray[np.float64]  # kcps/SPAD
    ambient_per_spad: NDArray[np.float64]  # kcps/SPAD
    nb_spads_enabled: NDArray[np.int32]
    range_sigma_mm: NDArray[np.float64]  # mm
    reflectance: NDArray[np.float64]  # %
    silicon_temp_degc: int
    cnh_raw: bytes | None = None  # CH variant: raw CNH block (decode via cnh)
    motion: MotionIndicator | None = None  # motion-indicator output when configured

    def grid(self, field: str = "distance_mm") -> NDArray[Any]:
        """Zone array reshaped to (4,4) or (8,8)."""
        side = 4 if self.resolution == RESOLUTION_4X4 else 8
        zones: NDArray[Any] = getattr(self, field)
        return zones[: self.resolution].reshape(side, side)


class _FrameDecoder:
    """What the reader thread needs to turn RPT_VL53_FRAME chunks into
    frames, fixed when it is built: the reassembler (its discard count is
    the link diagnostic, so it outlives the ULD — every chunk is reassembled
    whether or not a driver is bound), the ULD that parses a whole frame
    (None before init() and after a power event: the frame is dropped), the
    resolution that shapes it. Built on the caller's thread, read on the
    reader only."""

    def __init__(
        self, reassembler: FrameReassembler, uld: VL53L8CX | None, resolution: int
    ) -> None:
        self._reassembler = reassembler
        self._uld = uld
        self._resolution = resolution

    def rebind(self, uld: VL53L8CX | None, resolution: int) -> _FrameDecoder:
        """The same reassembler (and its counts) with another driver."""
        return _FrameDecoder(self._reassembler, uld, resolution)

    @property
    def discarded(self) -> int:
        return self._reassembler.discarded

    def feed(self, chunk: FrameChunk) -> Vl53l8Frame | None:
        """The frame `chunk` completes, else None. A frame the ULD refuses
        raises (Vl53l8cxError, struct.error, IndexError, ValueError)."""
        done = self._reassembler.feed(chunk)
        if done is None or self._uld is None:
            return None
        ts, raw = done
        parsed = self._uld.parse_frame(raw)
        return Vl53l8Frame(
            timestamp_us=ts,
            resolution=self._resolution,
            distance_mm=np.asarray(parsed["distance_mm"], dtype=np.int32),
            target_status=np.asarray(parsed["target_status"], dtype=np.uint8),
            nb_target_detected=np.asarray(parsed["nb_target_detected"], dtype=np.uint8),
            signal_per_spad=np.asarray(parsed["signal_per_spad"], dtype=np.float64),
            ambient_per_spad=np.asarray(parsed["ambient_per_spad"], dtype=np.float64),
            nb_spads_enabled=np.asarray(parsed["nb_spads_enabled"], dtype=np.int32),
            range_sigma_mm=np.asarray(parsed["range_sigma_mm"], dtype=np.float64),
            reflectance=np.asarray(parsed["reflectance"], dtype=np.float64),
            silicon_temp_degc=parsed.get("silicon_temp_degc", 0),
            cnh_raw=parsed.get("cnh_raw"),
            motion=parsed.get("motion_indicator"),
        )


class Vl53l8cx(DepzDevice):
    """VL53L8CX ToF device: `init()` downloads the ~84 KB sensor firmware
    (~1 s over the CDC link), then configure and `start_ranging()`.

    This is the base class for both silicon variants. The VL53L8CH superset
    (compact-network-histogram output) lives in `Vl53l8ch`, which inherits
    every method here. All configuration methods require `init()` first and
    must not be called while ranging (the ULD talks to the current register
    bank; the stream owns it — contract 04)."""

    #: Sensor-firmware blob variant this class loads (uld.VARIANT_DATA_DIR key).
    _VARIANT = "cx"
    #: Register-bridge transfer limits (the I2C L5/L7 board reads less per call).
    _READ_CHUNK = CHUNK_SIZE
    _WRITE_CHUNK = CHUNK_SIZE
    #: Largest RPT_VL53_FRAME chunk the board emits (contract 04 §4 rule 4).
    _STREAM_CHUNK = STREAM_CHUNK_MAX
    #: Lowest ranging frequency that actually streams on this sensor.
    _MIN_RANGING_HZ = MIN_RANGING_FREQUENCY_HZ
    _record_kind = "vl53l8"

    frames: Stream[Vl53l8Frame]
    """The parsed-frame stream: `frames.on(cb)` subscribes (reader-thread
    context; don't block), `frames(maxsize)` iterates (bounded, drop-oldest,
    subscribes immediately — call before or after `start_ranging()`),
    `frames.next(timeout)` waits for one; `dropped_counts` and `parse_errors`
    (frames ULD parsing refused: corrupt frame, bad size — distinct from
    `reassembler_discards`) are its counters."""

    def _init_subclass_state(self) -> None:
        self._uld: VL53L8CX | None = None
        self._bridge = RegisterBridge(
            self,
            cmds=VL53L8_BRIDGE,
            read_max=self._READ_CHUNK,
            write_max=self._WRITE_CHUNK,
            timeout=2.0,
        )
        self._arm = StreamArm(
            self,
            Vl53l8Cmd.START_STREAM,
            Vl53l8Cmd.STOP_STREAM,
            busy="stop_ranging() first — the stream owns the register bank",
        )
        # The one thing the reader thread reads. The reassembler is replaced,
        # sized, by start_ranging(); init() binds the driver, a power event
        # (LPn) unbinds it.
        self._decoder = _FrameDecoder(FrameReassembler(), None, RESOLUTION_4X4)
        self.frames = Stream[Vl53l8Frame](self, maxsize=8, what=Vl53l8Rpt.VL53_FRAME)
        self._cnh_config: CnhConfig | None = None
        self._resolution = RESOLUTION_4X4  # ULD default after init

    # ── lifecycle ────────────────────────────────────────────────────────────

    @property
    def uld(self) -> VL53L8CX:
        """The underlying ULD driver (escape hatch for advanced DCI access).

        unstable — not covered by the 1.0 freeze."""
        if self._uld is None:
            raise InvalidStateError("call init() first")
        return self._uld

    @property
    def variant(self) -> str:
        """'cx' | 'ch' (valid after init())."""
        return self.uld.variant

    def is_alive(self) -> bool:
        """True when the sensor answers with its own silicon ids (a bus
        error or a foreign id: False)."""
        probe = VL53L8CX(self._bridge)
        try:
            device_id, revision_id = probe.is_alive()
            return silicon_id_matches(device_id, revision_id,
                                      self._VARIANT in I2C_VARIANTS)
        except (Vl53l8cxError, DepzError):
            return False

    def init(
        self,
        *,
        progress: Callable[[str], None] | None = None,
        write_progress: Callable[[int, int], None] | None = None,
    ) -> None:
        """Initialize the sensor: firmware blob download + default config.

        The blob variant is fixed by the class (`Vl53l8cx` → 'cx',
        `Vl53l8ch` → 'ch'). `progress` receives phase strings;
        `write_progress(done, total)` tracks the big blob writes."""
        self._bridge.write_progress = write_progress
        try:
            driver = VL53L8CX(self._bridge, variant=self._VARIANT)
            driver.init(progress=progress)
            self._uld = driver
            self._resolution = RESOLUTION_4X4  # what the sensor holds now, whatever was set before
            self._decoder = self._decoder.rebind(driver, self._resolution)
            # A fresh driver has no motion indicator armed; the CNH setup
            # is gone from the sensor too (ERRATA B4).
            self._cnh_config = None
        finally:
            self._bridge.write_progress = None

    # ── configuration (init() first; not while ranging) ─────────────────────

    def get_resolution(self) -> int:
        """Active zone count: 16 (4×4) or 64 (8×8)."""
        self._resolution = self.uld.get_resolution()
        return self._resolution

    def set_resolution(self, zones: int) -> None:
        """Select the zone grid: RESOLUTION_4X4 (16) or RESOLUTION_8X8 (64).
        Not while ranging."""
        if zones not in (RESOLUTION_4X4, RESOLUTION_8X8):
            raise ValueError("resolution is 16 (4x4) or 64 (8x8) zones")
        self._arm.require_idle()
        self.uld.set_resolution(zones)
        self._resolution = zones

    def get_ranging_frequency_hz(self) -> int:
        """Configured ranging frequency in Hz."""
        return self.uld.get_ranging_frequency_hz()

    def set_ranging_frequency_hz(self, hz: int) -> None:
        """Set the ranging frequency in Hz. Not while ranging.

        At least 2 Hz on the VL53L8 (below that the sensor never enters its
        ranging loop and streams nothing, contract 04), 1 Hz on L5/L7; at
        most 60 Hz at 4×4 and 15 Hz at 8×8 (contract 11 §3) — judged by the
        resolution last set or read through this class (4×4 after init).
        Refused host-side, nothing reaches the bus."""
        if not isinstance(hz, int) or hz < self._MIN_RANGING_HZ:
            raise ValueError(
                f"ranging frequency must be >= {self._MIN_RANGING_HZ} Hz: below that "
                "the sensor never enters its ranging loop and streams nothing (contract 04)"
            )
        max_hz = MAX_RANGING_FREQUENCY_HZ[self._resolution]
        if hz > max_hz:
            raise ValueError(
                f"ranging frequency must be <= {max_hz} Hz at {self._resolution} zones "
                f"(60 Hz at 4x4, 15 Hz at 8x8, contract 11 §3), got {hz}"
            )
        self._arm.require_idle()
        self.uld.set_ranging_frequency_hz(hz)

    def get_ranging_mode(self) -> int:
        """RANGING_MODE_CONTINUOUS or RANGING_MODE_AUTONOMOUS."""
        return self.uld.get_ranging_mode()

    def set_ranging_mode(self, mode: int) -> None:
        """Set CONTINUOUS (free-running) or AUTONOMOUS (integrate-then-idle)
        ranging. Not while ranging."""
        self._arm.require_idle()
        self.uld.set_ranging_mode(mode)

    def get_integration_time_ms(self) -> int:
        """Configured integration time in ms."""
        return self.uld.get_integration_time_ms()

    def set_integration_time_ms(self, ms: int) -> None:
        """Set the integration time, 2–1000 ms. Autonomous mode only (no
        effect in continuous ranging). Not while ranging."""
        self._arm.require_idle()
        self.uld.set_integration_time_ms(ms)

    def get_sharpener_percent(self) -> int:
        """Configured edge-sharpener strength, 0–99 %."""
        return self.uld.get_sharpener_percent()

    def set_sharpener_percent(self, pct: int) -> None:
        """Set the edge sharpener, 0–99 % (0 disables). Not while ranging."""
        self._arm.require_idle()
        self.uld.set_sharpener_percent(pct)

    def get_target_order(self) -> int:
        """TARGET_ORDER_CLOSEST or TARGET_ORDER_STRONGEST."""
        return self.uld.get_target_order()

    def set_target_order(self, order: int) -> None:
        """Order multi-target zones by CLOSEST or STRONGEST return. Not while
        ranging."""
        self._arm.require_idle()
        self.uld.set_target_order(order)

    # ── advanced features (UM3109; init() first, not while ranging) ─────────

    def get_power_mode(self) -> int:
        """POWER_MODE_SLEEP/WAKEUP/DEEP_SLEEP (uld constants)."""
        return self.uld.get_power_mode()

    def set_power_mode(self, mode: int) -> None:
        """Enter sleep / wake / deep-sleep. Not while ranging. Waking from
        DEEP_SLEEP re-downloads the firmware blob (init())."""
        self._arm.require_idle()
        self.uld.set_power_mode(mode)

    def get_xtalk_margin(self) -> int:
        """Xtalk margin, whole kcps/SPAD (as ST)."""
        return self.uld.get_xtalk_margin()

    def set_xtalk_margin(self, margin_kcps: int) -> None:
        """Xtalk margin, a whole number of kcps/SPAD, 0..10000 (as ST)."""
        self._arm.require_idle()
        self.uld.set_xtalk_margin(margin_kcps)

    def calibrate_xtalk(
        self, reflectance_percent: int, nb_samples: int, distance_mm: int
    ) -> None:
        """Run on-device crosstalk calibration against a flat target at
        `distance_mm` with the given `reflectance_percent` (1..99) averaging
        `nb_samples` (1..16). The result is captured into the xtalk buffer;
        read it back with get_caldata_xtalk(). Blocks several seconds."""
        self._arm.require_idle()
        self.uld.calibrate_xtalk(reflectance_percent, nb_samples, distance_mm)

    def get_caldata_xtalk(self) -> bytes:
        """Read back the 776-byte xtalk calibration blob (save/restore)."""
        self._arm.require_idle()
        return self.uld.get_caldata_xtalk()

    def set_caldata_xtalk(self, blob: bytes) -> None:
        """Restore a previously saved 776-byte xtalk calibration blob."""
        self._arm.require_idle()
        self.uld.set_caldata_xtalk(blob)

    def get_detection_thresholds_enable(self) -> int:
        return self.uld.get_detection_thresholds_enable()

    def set_detection_thresholds_enable(self, enabled: bool) -> None:
        self._arm.require_idle()
        self.uld.set_detection_thresholds_enable(enabled)

    def get_detection_thresholds(self) -> list[DetectionThreshold]:
        return self.uld.get_detection_thresholds()

    def set_detection_thresholds(self, thresholds: list[DetectionThreshold]) -> None:
        """Program the 64 detection thresholds (interrupt-on-threshold). Each
        entry is a dict: low_thresh, high_thresh, measurement, type, zone_num,
        operation (see uld THRESH_* constants)."""
        self._arm.require_idle()
        self.uld.set_detection_thresholds(thresholds)

    def set_detection_thresholds_auto_stop(self, auto_stop: bool) -> None:
        self._arm.require_idle()
        self.uld.set_detection_thresholds_auto_stop(auto_stop)

    def configure_motion_indicator(
        self, distance_min_mm: int = 400, distance_max_mm: int = 1500
    ) -> MotionConfig:
        """Enable the motion indicator over [distance_min_mm, distance_max_mm]
        and surface motion output in each frame's `.motion`. Returns the
        underlying uld MotionConfig for advanced tuning.

        On CH firmware the motion indicator and CNH share one DCI struct
        (0xBFAC): this disarms CNH (`cnh_config` → None, no `cnh_raw`)."""
        self._arm.require_idle()
        resolution = self.get_resolution()
        cfg = self.uld.motion_indicator_init(resolution)
        self._cnh_config = None
        self.uld.motion_indicator_set_distance_motion(
            cfg, distance_min_mm, distance_max_mm
        )
        return cfg

    # ── ranging ──────────────────────────────────────────────────────────────

    def start_ranging(self) -> None:
        """Configure the output list, start the sensor and the MCU stream."""
        self._arm.require_idle()
        cnh_size = self._cnh_config.required_memory() if self._cnh_config else None
        self.get_resolution()  # refresh the cache used for frame shaping
        uld = self.uld
        uld.start_ranging(cnh_data_size=cnh_size)
        # Replaced before the stream is armed: the first chunk may land
        # before the START_STREAM ack is seen.
        self._decoder = _FrameDecoder(
            FrameReassembler(self._STREAM_CHUNK), uld, self._resolution
        )
        self._arm.start(pack_start_stream(uld.data_read_size), undo=uld.stop_ranging)

    def stop_ranging(self) -> None:
        self._arm.stop(then=lambda: self.uld.stop_ranging())

    @property
    def ranging(self) -> bool:
        return self._arm.active

    @property
    def reassembler_discards(self) -> int:
        """Chunked frames discarded by the reassembler (gaps / offset errors).
        Distinct from `frames.parse_errors` (ULD parsing failed)."""
        return self._decoder.discarded

    @property
    def _record_stream(self) -> Stream[Vl53l8Frame]:
        return self.frames

    def _record_value(self, f: Vl53l8Frame, *, layers: bool = False) -> dict[str, Any]:
        """Contract 09 `vl53l8`; `layers` adds signal / ambient / sigma /
        reflectance per zone."""
        n = f.resolution
        v: dict[str, Any] = {
            "resolution": n,
            "silicon_temp_degc": f.silicon_temp_degc,
            "distance_mm": f.distance_mm[:n].tolist(),
            "target_status": f.target_status[:n].tolist(),
            "nb_target_detected": f.nb_target_detected[:n].tolist(),
        }
        if layers:
            v["signal_per_spad"] = f.signal_per_spad[:n].tolist()
            v["ambient_per_spad"] = f.ambient_per_spad[:n].tolist()
            v["range_sigma_mm"] = f.range_sigma_mm[:n].tolist()
            v["reflectance"] = f.reflectance[:n].tolist()
        if f.motion is not None:
            v["motion"] = f.motion
        return v

    # ── internal ─────────────────────────────────────────────────────────────

    def _handle_report(self, pkt: Packet) -> bool:
        if pkt.cmd != Vl53l8Rpt.VL53_FRAME:
            return False
        try:
            # short chunk header: refused (ValueError) and counted — contract 07 §2
            frame = self._decoder.feed(FrameChunk.unpack(pkt.payload))
        except (Vl53l8cxError, struct.error, IndexError, ValueError):
            # A parse failure is NOT a reassembly gap — count it separately so
            # link diagnostics stay meaningful (was inflating discarded).
            self.frames.parse_errors += 1
            return True
        if frame is not None:
            self.frames._emit(frame)
        return True


class CnhMixin:
    """Compact-Network-Histogram (CNH) output, shared by every CH-firmware
    class (VL53L8CH here, VL53L7CH in `vl53l7`). Mix in before the device
    base class."""

    if TYPE_CHECKING:
        # Provided by the Vl53l8cx host class the mixin is combined with.
        _cnh_config: CnhConfig | None
        _arm: StreamArm

        @property
        def uld(self) -> VL53L8CX: ...

    def configure_cnh(self, config: CnhConfig) -> None:
        """Arm the CNH histogram block for the next start_ranging(). CH only —
        this method does not exist on Vl53l8cx.

        CNH and the motion indicator share one DCI struct (0xBFAC): this
        disarms the motion indicator (frames carry no `.motion`)."""
        self._arm.require_idle()
        self.uld.dci_write_data(cnh_module.MI_CFG_DEV_IDX, config.pack())
        self.uld.motion_indicator_disarm()
        self._cnh_config = config

    @property
    def cnh_config(self) -> CnhConfig | None:
        """The CNH config armed by :meth:`configure_cnh`, or ``None``.
        Recorders read this to persist decode parameters next to raw blocks."""
        return self._cnh_config

    def _record_extras(self, f: Vl53l8Frame) -> list[tuple[str, dict[str, Any]]]:
        """The raw CNH block riding a frame, with the armed config's decode
        parameters (contract 09 `vl53l8_cnh`)."""
        cfg = self._cnh_config
        if not f.cnh_raw or cfg is None:
            return []
        return [("vl53l8_cnh", {
            "nb_aggregates": cfg.nb_of_aggregates,
            "feature_length": cfg.feature_length,
            "sum_span": cfg.sum_span,
            "start_bin": cfg.ref_bin_offset // 2048,
            "cnh_raw_b64": base64.b64encode(f.cnh_raw).decode("ascii"),
        })]


class Vl53l8ch(CnhMixin, Vl53l8cx):
    """VL53L8CH device: the VL53L8CX superset. Inherits every CX method and
    adds Compact-Network-Histogram (CNH) output. `init()` downloads the CH
    firmware blob (VL53LMZ ULD 2.0.16). CNH is the reason to run CH firmware:
    each frame can additionally carry a per-aggregate distance histogram."""

    _VARIANT = "ch"

