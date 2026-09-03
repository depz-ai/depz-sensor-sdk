"""VL53L8CX/CH ToF sensor: host-side ULD over the firmware register bridge.

The MCU is a thin SPI bridge (contracts/04): the full ST ULD driver runs here
on the host (`uld.py`, absorbed from the firmware repo's proven Python port).
Frame streaming is INT-driven push from the device, reassembled from ≤1528-
byte chunks.
"""

from __future__ import annotations

import queue
import struct
import threading
import time
from dataclasses import dataclass
from typing import Callable

import numpy as np

from ..device import DeviceBase, StreamIterator, StreamQueue
from ..errors import DepzError, DepzTimeoutError, LinkClosedError, StatusError
from ..protocol.vl53l8 import (
    CHUNK_SIZE,
    FrameChunk,
    FrameReassembler,
    RegData,
    Vl53l8Cmd,
    Vl53l8Rpt,
    pack_read_reg,
    pack_start_stream,
    pack_write_reg,
)
from ..transport import Packet
from . import cnh as cnh_module
from . import uld as uld_module
from .cnh import CnhConfig

#: Slice length for close-aware blocking waits. Small enough that `close()`
#: releases a pending `get_frame()` promptly, large enough to stay off the CPU.
_CLOSE_POLL_S = 0.05
from .uld import (
    RANGING_MODE_AUTONOMOUS,
    RANGING_MODE_CONTINUOUS,
    RESOLUTION_4X4,
    RESOLUTION_8X8,
    TARGET_ORDER_CLOSEST,
    TARGET_ORDER_STRONGEST,
    VL53L8CX,
    Vl53l8cxError,
)

from .uld import (  # noqa: E402
    POWER_MODE_DEEP_SLEEP,
    POWER_MODE_SLEEP,
    POWER_MODE_WAKEUP,
)

__all__ = [
    "Vl53l8",
    "Vl53l8Cx",
    "Vl53l8Ch",
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


@dataclass(frozen=True)
class Vl53l8Frame:
    """One parsed ranging frame. Arrays are sized to the active resolution
    (16 or 64 zones); zone index runs row-major (see datasheet zone maps)."""

    timestamp_us: int
    resolution: int  # 16 | 64
    distance_mm: np.ndarray  # int32, shape (zones,)
    target_status: np.ndarray  # uint8: 5/9 = valid, 255 = no target
    nb_target_detected: np.ndarray  # uint8
    signal_per_spad: np.ndarray  # float64 kcps/SPAD
    ambient_per_spad: np.ndarray  # float64 kcps/SPAD
    nb_spads_enabled: np.ndarray  # int32
    range_sigma_mm: np.ndarray  # float64
    reflectance: np.ndarray  # uint8 %
    silicon_temp_degc: int
    cnh_raw: bytes | None = None  # CH variant: raw CNH block (decode via cnh)
    motion: dict | None = None  # motion-indicator output when configured

    def grid(self, field: str = "distance_mm") -> np.ndarray:
        """Zone array reshaped to (4,4) or (8,8)."""
        side = 4 if self.resolution == RESOLUTION_4X4 else 8
        return getattr(self, field)[: self.resolution].reshape(side, side)


class _BridgePlatform:
    """ULD `platform` object mapped onto the firmware register bridge."""

    def __init__(self, dev: "Vl53l8Cx"):
        self._dev = dev

    def rd_multi(self, addr: int, size: int) -> bytes:
        out = bytearray()
        while size > 0:
            n = min(size, CHUNK_SIZE)
            rep: RegData = self._dev.request(
                Vl53l8Cmd.READ_REG,
                pack_read_reg(addr, n),
                matcher=DeviceBase.expect_report(Vl53l8Rpt.REG_DATA, RegData.unpack),
                timeout=2.0,
            )
            if len(rep.data) != n:
                raise DepzError(f"READ_REG 0x{addr:04X}: expected {n}, got {len(rep.data)}")
            out.extend(rep.data)
            addr += n
            size -= n
        return bytes(out)

    def wr_multi(self, addr: int, data: bytes) -> None:
        done = 0
        while done < len(data):
            chunk = data[done : done + CHUNK_SIZE]
            self._dev.request(
                Vl53l8Cmd.WRITE_REG,
                pack_write_reg(addr, chunk),
                ok_completes=True,
                timeout=2.0,
            )
            addr += len(chunk)
            done += len(chunk)
            if self._dev._write_progress is not None and len(data) > CHUNK_SIZE:
                self._dev._write_progress(done, len(data))

    def sleep_ms(self, ms: int) -> None:
        import time

        time.sleep(ms / 1000.0)


class Vl53l8Cx(DeviceBase):
    """VL53L8CX ToF device: `init()` downloads the ~84 KB sensor firmware
    (~1 s over the CDC link), then configure and `start_ranging()`.

    This is the base class for both silicon variants. The VL53L8CH superset
    (compact-network-histogram output) lives in `Vl53l8Ch`, which inherits
    every method here. All configuration methods require `init()` first and
    must not be called while ranging (the ULD talks to the current register
    bank; the stream owns it — contract 04)."""

    #: Sensor-firmware blob variant this class loads (uld.VARIANT_DATA_DIR key).
    _VARIANT = "cx"

    def _init_subclass_state(self) -> None:
        self._uld: VL53L8CX | None = None
        self._platform = _BridgePlatform(self)
        self._reassembler = FrameReassembler()
        self._frame_cbs: list[Callable[[Vl53l8Frame], None]] = []
        self._frame_queues: list[StreamQueue] = []
        self._ranging = False
        self._cnh_config: CnhConfig | None = None
        self._write_progress: Callable[[int, int], None] | None = None
        self._uld_lock = threading.Lock()
        self._resolution = RESOLUTION_4X4  # ULD default after init
        self._frame_parse_errors = 0  # frames dropped by parse failure (≠ reassembly gaps)

    # ── lifecycle ────────────────────────────────────────────────────────────

    @property
    def uld(self) -> VL53L8CX:
        """The underlying ULD driver (escape hatch for advanced DCI access)."""
        if self._uld is None:
            raise DepzError("call init() first")
        return self._uld

    @property
    def variant(self) -> str:
        """'cx' | 'ch' (valid after init())."""
        return self.uld.variant

    def is_alive(self) -> bool:
        probe = VL53L8CX(self._platform)
        try:
            return bool(probe.is_alive())
        except (Vl53l8cxError, StatusError, DepzError):
            return False

    def init(
        self,
        variant: str | None = None,
        *,
        progress: Callable[[str], None] | None = None,
        write_progress: Callable[[int, int], None] | None = None,
    ) -> None:
        """Initialize the sensor: firmware blob download + default config.

        The blob variant is fixed by the class (`Vl53l8Cx` → 'cx',
        `Vl53l8Ch` → 'ch'); `variant` is accepted only for backward
        compatibility and must match the class variant when given. `progress`
        receives phase strings; `write_progress(done, total)` tracks the big
        blob writes."""
        if variant is not None and variant != self._VARIANT:
            raise DepzError(
                f"{type(self).__name__} loads the '{self._VARIANT}' firmware "
                f"blob; use Vl53l8Ch for 'ch'"
            )
        with self._uld_lock:
            self._write_progress = write_progress
            try:
                driver = VL53L8CX(self._platform, variant=self._VARIANT)
                driver.init(progress=progress)
                self._uld = driver
            finally:
                self._write_progress = None

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
        self._require_not_ranging()
        self.uld.set_resolution(zones)
        self._resolution = zones

    def get_ranging_frequency_hz(self) -> int:
        """Configured ranging frequency in Hz."""
        return self.uld.get_ranging_frequency_hz()

    def set_ranging_frequency_hz(self, hz: int) -> None:
        """Set the ranging frequency in Hz (must be ≥ 2). Not while ranging.

        Max is 60 Hz at 4×4 and 15 Hz at 8×8; below 2 Hz the sensor never
        enters its ranging loop and streams nothing (contract 04)."""
        if hz < MIN_RANGING_FREQUENCY_HZ:
            raise ValueError(
                f"ranging frequency must be >= {MIN_RANGING_FREQUENCY_HZ} Hz: below that "
                "the sensor never enters its ranging loop and streams nothing (contract 04)"
            )
        self._require_not_ranging()
        self.uld.set_ranging_frequency_hz(hz)

    def get_ranging_mode(self) -> int:
        """RANGING_MODE_CONTINUOUS or RANGING_MODE_AUTONOMOUS."""
        return self.uld.get_ranging_mode()

    def set_ranging_mode(self, mode: int) -> None:
        """Set CONTINUOUS (free-running) or AUTONOMOUS (integrate-then-idle)
        ranging. Not while ranging."""
        self._require_not_ranging()
        self.uld.set_ranging_mode(mode)

    def get_integration_time_ms(self) -> int:
        """Configured integration time in ms."""
        return self.uld.get_integration_time_ms()

    def set_integration_time_ms(self, ms: int) -> None:
        """Set the integration time, 2–1000 ms. Autonomous mode only (no
        effect in continuous ranging). Not while ranging."""
        self._require_not_ranging()
        self.uld.set_integration_time_ms(ms)

    def get_sharpener_percent(self) -> int:
        """Configured edge-sharpener strength, 0–99 %."""
        return self.uld.get_sharpener_percent()

    def set_sharpener_percent(self, pct: int) -> None:
        """Set the edge sharpener, 0–99 % (0 disables). Not while ranging."""
        self._require_not_ranging()
        self.uld.set_sharpener_percent(pct)

    def get_target_order(self) -> int:
        """TARGET_ORDER_CLOSEST or TARGET_ORDER_STRONGEST."""
        return self.uld.get_target_order()

    def set_target_order(self, order: int) -> None:
        """Order multi-target zones by CLOSEST or STRONGEST return. Not while
        ranging."""
        self._require_not_ranging()
        self.uld.set_target_order(order)

    # ── advanced features (UM3109; init() first, not while ranging) ─────────

    def get_power_mode(self) -> int:
        """POWER_MODE_SLEEP/WAKEUP/DEEP_SLEEP (uld constants)."""
        return self.uld.get_power_mode()

    def set_power_mode(self, mode: int) -> None:
        """Enter sleep / wake / deep-sleep. Not while ranging. Waking from
        DEEP_SLEEP re-downloads the firmware blob (init())."""
        self._require_not_ranging()
        self.uld.set_power_mode(mode)

    def get_xtalk_margin(self) -> float:
        return self.uld.get_xtalk_margin()

    def set_xtalk_margin(self, margin_kcps: float) -> None:
        self._require_not_ranging()
        self.uld.set_xtalk_margin(margin_kcps)

    def calibrate_xtalk(
        self, reflectance_percent: int, nb_samples: int, distance_mm: int
    ) -> None:
        """Run on-device crosstalk calibration against a flat target at
        `distance_mm` with the given `reflectance_percent` (1..99) averaging
        `nb_samples` (1..16). The result is captured into the xtalk buffer;
        read it back with get_caldata_xtalk(). Blocks several seconds."""
        self._require_not_ranging()
        self.uld.calibrate_xtalk(reflectance_percent, nb_samples, distance_mm)

    def get_caldata_xtalk(self) -> bytes:
        """Read back the 776-byte xtalk calibration blob (save/restore)."""
        self._require_not_ranging()
        return self.uld.get_caldata_xtalk()

    def set_caldata_xtalk(self, blob: bytes) -> None:
        """Restore a previously saved 776-byte xtalk calibration blob."""
        self._require_not_ranging()
        self.uld.set_caldata_xtalk(blob)

    def get_detection_thresholds_enable(self) -> int:
        return self.uld.get_detection_thresholds_enable()

    def set_detection_thresholds_enable(self, enabled: bool) -> None:
        self._require_not_ranging()
        self.uld.set_detection_thresholds_enable(enabled)

    def get_detection_thresholds(self) -> list[dict]:
        return self.uld.get_detection_thresholds()

    def set_detection_thresholds(self, thresholds: list[dict]) -> None:
        """Program the 64 detection thresholds (interrupt-on-threshold). Each
        entry is a dict: low_thresh, high_thresh, measurement, type, zone_num,
        operation (see uld THRESH_* constants)."""
        self._require_not_ranging()
        self.uld.set_detection_thresholds(thresholds)

    def set_detection_thresholds_auto_stop(self, auto_stop: bool) -> None:
        self._require_not_ranging()
        self.uld.set_detection_thresholds_auto_stop(auto_stop)

    def configure_motion_indicator(
        self, distance_min_mm: int = 400, distance_max_mm: int = 1500
    ):
        """Enable the motion indicator over [distance_min_mm, distance_max_mm]
        and surface motion output in each frame's `.motion`. Returns the
        underlying uld MotionConfig for advanced tuning."""
        self._require_not_ranging()
        resolution = self.get_resolution()
        cfg = self.uld.motion_indicator_init(resolution)
        self.uld.motion_indicator_set_distance_motion(
            cfg, distance_min_mm, distance_max_mm
        )
        return cfg

    # ── ranging ──────────────────────────────────────────────────────────────

    def start_ranging(self) -> None:
        """Configure the output list, start the sensor and the MCU stream."""
        self._require_not_ranging()
        cnh_size = self._cnh_config.required_memory() if self._cnh_config else None
        self.get_resolution()  # refresh the cache used for frame shaping
        self.uld.start_ranging(cnh_data_size=cnh_size)
        frame_size = self.uld.data_read_size
        self._reassembler = FrameReassembler()
        self.request(
            Vl53l8Cmd.START_STREAM, pack_start_stream(frame_size), ok_completes=True
        )
        self._ranging = True

    def stop_ranging(self) -> None:
        if not self._ranging:
            return
        # Clear host state and stop the sensor even if the MCU STOP_STREAM ack
        # fails (link hiccup): otherwise the device is wedged "ranging" and no
        # reconfiguration is possible.
        try:
            self.request(Vl53l8Cmd.STOP_STREAM, ok_completes=True)
        finally:
            self._ranging = False
            self.uld.stop_ranging()

    @property
    def ranging(self) -> bool:
        return self._ranging

    def on_frame(self, cb: Callable[[Vl53l8Frame], None]) -> Callable[[], None]:
        """Subscribe to parsed frames (reader-thread context; don't block)."""
        self._frame_cbs.append(cb)
        return lambda: self._frame_cbs.remove(cb)

    def frames(self, maxsize: int = 8) -> StreamIterator:
        """Blocking iterator over parsed frames (bounded, drop-oldest).
        Subscribes immediately — call before or after start_ranging()."""
        return StreamIterator(self._frame_queues, maxsize, lambda: self.closed)

    @property
    def frame_parse_errors(self) -> int:
        """Frames dropped because ULD parsing failed (corrupt frame, bad size).
        Distinct from reassembler gap discards (`reassembler_discards`)."""
        return self._frame_parse_errors

    @property
    def reassembler_discards(self) -> int:
        """Chunked frames discarded by the reassembler (gaps / offset errors)."""
        return self._reassembler.discarded

    def get_frame(self, timeout: float = 2.0) -> Vl53l8Frame:
        """Convenience: wait for the next frame.

        Raises `DepzTimeoutError` when no frame arrives within `timeout`, and
        `LinkClosedError` as soon as the device is closed while waiting —
        a caller blocked here is released by `close()` instead of sitting out
        the full timeout on a link that can never deliver again.
        """
        if self.closed:
            raise LinkClosedError("device is closed")
        q = StreamQueue(1)
        self._frame_queues.append(q)
        try:
            deadline = time.monotonic() + timeout
            while True:
                remaining = deadline - time.monotonic()
                if remaining <= 0:
                    raise DepzTimeoutError(Vl53l8Rpt.VL53_FRAME, timeout) from None
                try:
                    return q.get(timeout=min(_CLOSE_POLL_S, remaining))
                except queue.Empty:
                    # A frame already queued wins over a concurrent close; only
                    # an *empty* queue on a closed device ends the wait.
                    if self.closed:
                        raise LinkClosedError(
                            "device closed while waiting for a frame"
                        ) from None
        finally:
            self._frame_queues.remove(q)

    # ── internal ─────────────────────────────────────────────────────────────

    def _require_not_ranging(self) -> None:
        if self._ranging:
            raise DepzError("stop_ranging() first — the stream owns the register bank")

    def _handle_report(self, pkt: Packet) -> bool:
        if pkt.cmd != Vl53l8Rpt.VL53_FRAME or len(pkt.payload) < 12:
            return False
        done = self._reassembler.feed(FrameChunk.unpack(pkt.payload))
        if done is None:
            return True
        ts, raw = done
        if self._uld is None:
            return True
        try:
            parsed = self._uld.parse_frame(raw)
        except (Vl53l8cxError, struct.error, IndexError, ValueError):
            # A parse failure is NOT a reassembly gap — count it separately so
            # link diagnostics stay meaningful (was inflating discarded).
            self._frame_parse_errors += 1
            return True
        frame = self._to_frame(ts, parsed)
        for cb in list(self._frame_cbs):
            cb(frame)
        for q in list(self._frame_queues):
            q.put(frame)
        return True

    def _to_frame(self, timestamp_us: int, parsed: dict) -> Vl53l8Frame:
        resolution = self._resolution
        return Vl53l8Frame(
            timestamp_us=timestamp_us,
            resolution=resolution,
            distance_mm=np.asarray(parsed["distance_mm"], dtype=np.int32),
            target_status=np.asarray(parsed["target_status"], dtype=np.uint8),
            nb_target_detected=np.asarray(parsed["nb_target_detected"], dtype=np.uint8),
            signal_per_spad=np.asarray(parsed["signal_per_spad"], dtype=np.float64),
            ambient_per_spad=np.asarray(parsed["ambient_per_spad"], dtype=np.float64),
            nb_spads_enabled=np.asarray(parsed["nb_spads_enabled"], dtype=np.int32),
            range_sigma_mm=np.asarray(parsed["range_sigma_mm"], dtype=np.float64),
            reflectance=np.asarray(parsed["reflectance"], dtype=np.uint8),
            silicon_temp_degc=parsed.get("silicon_temp_degc", 0),
            cnh_raw=parsed.get("cnh_raw"),
            motion=parsed.get("motion_indicator"),
        )


class Vl53l8Ch(Vl53l8Cx):
    """VL53L8CH device: the VL53L8CX superset. Inherits every CX method and
    adds Compact-Network-Histogram (CNH) output. `init()` downloads the CH
    firmware blob (VL53LMZ ULD 2.0.16). CNH is the reason to run CH firmware:
    each frame can additionally carry a per-aggregate distance histogram."""

    _VARIANT = "ch"

    def configure_cnh(self, config: CnhConfig) -> None:
        """Arm the CNH histogram block for the next start_ranging(). CH only —
        this method does not exist on Vl53l8Cx."""
        self._require_not_ranging()
        self.uld.dci_write_data(cnh_module.MI_CFG_DEV_IDX, config.pack())
        self._cnh_config = config


# Backward-compatible alias: the old flat `Vl53l8` name maps to the CX base
# class (the historic default). New code should pick Vl53l8Cx / Vl53l8Ch.
Vl53l8 = Vl53l8Cx
