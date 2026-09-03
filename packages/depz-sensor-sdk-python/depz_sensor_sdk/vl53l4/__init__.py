"""VL53L4CD ToF sensor: host-side ULD over the firmware register bridge.

The MCU is a thin I2C bridge (contracts/10): the full ST ULD 2.2.3 driver runs
here on the host (`uld.py`, absorbed from the firmware repo's hardware-proven
Python port). Result streaming is INT-driven push from the device — one
17-byte result block per RPT_VL53_STREAM, no reassembly needed.
"""

from __future__ import annotations

import queue
import time
from dataclasses import dataclass
from typing import Callable

from ..device import DeviceBase, StreamIterator, StreamQueue
from ..errors import DepzError, DepzTimeoutError, LinkClosedError, StatusError
from ..protocol.vl53l4 import (
    I2C_KHZ_STEPS,
    XFER_MAX,
    XSHUT_OFF,
    XSHUT_ON,
    XSHUT_RESET,
    RegData,
    StreamData,
    Vl53l4Cmd,
    Vl53l4Info,
    Vl53l4Rpt,
    pack_read_reg,
    pack_set_i2c_speed,
    pack_start_stream,
    pack_write_reg,
    pack_xshut,
)
from ..transport import Packet
from .uld import (
    I2C_KHZ_BOOT,
    I2C_KHZ_DEFAULT,
    MODEL_ID_VL53L4CD,
    RANGE_STATUS_NAMES,
    RESULT_BLOCK_ADDR,
    RESULT_BLOCK_LEN,
    VL53L4CD,
    WINDOW_ABOVE,
    WINDOW_BELOW,
    WINDOW_IN,
    WINDOW_OUT,
    ResultsData,
    Vl53l4cdError,
    parse_result_block,
)

#: Slice length for close-aware blocking waits (see Vl53l8Cx.get_frame).
_CLOSE_POLL_S = 0.05

__all__ = [
    "Vl53l4Cd",
    "Vl53l4Measurement",
    "Vl53l4Info",
    "Vl53l4cdError",
    "MODEL_ID_VL53L4CD",
    "RANGE_STATUS_NAMES",
    "WINDOW_BELOW",
    "WINDOW_ABOVE",
    "WINDOW_OUT",
    "WINDOW_IN",
    "XSHUT_OFF",
    "XSHUT_ON",
    "XSHUT_RESET",
    "I2C_KHZ_BOOT",
    "I2C_KHZ_DEFAULT",
    "I2C_KHZ_STEPS",
]


@dataclass(frozen=True)
class Vl53l4Measurement:
    """One decoded ranging result (VL53L4CD_ResultsData_t + MCU timestamp)."""

    timestamp_us: int  # MCU uptime at the INT edge (stream) / read (poll)
    range_status: int  # 0 = valid (RANGE_STATUS_NAMES)
    distance_mm: int
    sigma_mm: int
    signal_rate_kcps: int
    ambient_rate_kcps: int
    signal_per_spad_kcps: int
    ambient_per_spad_kcps: int
    number_of_spad: int
    stream_count: int  # sensor frame counter, wraps at 255

    @property
    def valid(self) -> bool:
        return self.range_status == 0

    @property
    def status_text(self) -> str:
        return RANGE_STATUS_NAMES.get(self.range_status, f"unknown ({self.range_status})")

    @staticmethod
    def _from_results(timestamp_us: int, r: ResultsData) -> "Vl53l4Measurement":
        return Vl53l4Measurement(
            timestamp_us=timestamp_us,
            range_status=r.range_status,
            distance_mm=r.distance_mm,
            sigma_mm=r.sigma_mm,
            signal_rate_kcps=r.signal_rate_kcps,
            ambient_rate_kcps=r.ambient_rate_kcps,
            signal_per_spad_kcps=r.signal_per_spad_kcps,
            ambient_per_spad_kcps=r.ambient_per_spad_kcps,
            number_of_spad=r.number_of_spad,
            stream_count=r.stream_count,
        )


class _BridgePlatform:
    """ULD `platform` object mapped onto the firmware register bridge."""

    def __init__(self, dev: "Vl53l4Cd"):
        self._dev = dev
        self.last_timestamp_us = 0  # MCU timestamp of the latest register read

    def rd_multi(self, addr: int, size: int) -> bytes:
        out = bytearray()
        while size > 0:
            n = min(size, XFER_MAX)
            rep: RegData = self._dev.request(
                Vl53l4Cmd.READ_REG,
                pack_read_reg(addr, n),
                matcher=DeviceBase.expect_report(Vl53l4Rpt.REG_DATA, RegData.unpack),
                timeout=2.0,
            )
            if len(rep.data) != n:
                raise DepzError(f"READ_REG 0x{addr:04X}: expected {n}, got {len(rep.data)}")
            self.last_timestamp_us = rep.timestamp_us
            out.extend(rep.data)
            addr += n
            size -= n
        return bytes(out)

    def wr_multi(self, addr: int, data: bytes) -> None:
        done = 0
        while done < len(data):
            chunk = data[done : done + XFER_MAX]
            self._dev.request(
                Vl53l4Cmd.WRITE_REG,
                pack_write_reg(addr, chunk),
                ok_completes=True,
                timeout=2.0,
            )
            addr += len(chunk)
            done += len(chunk)

    def set_i2c_speed(self, khz: int) -> None:
        self._dev.request(
            Vl53l4Cmd.SET_I2C_SPEED, pack_set_i2c_speed(khz), ok_completes=True
        )

    def sleep_ms(self, ms: int) -> None:
        time.sleep(ms / 1000.0)


class Vl53l4Cd(DeviceBase):
    """VL53L4CD single-zone ToF device.

    `init()` runs the ULD boot sequence (no firmware blob — the sensor carries
    its own), then configure and `start_ranging()`. Configuration methods must
    not be called while ranging: the INT-driven stream owns the register bank
    (contract 10). Measurements stream via callbacks (`on_measurement`) and/or
    the pull iterator (`measurements()`)."""

    def _init_subclass_state(self) -> None:
        self._platform = _BridgePlatform(self)
        self._uld = VL53L4CD(self._platform)
        self._measure_cbs: list[Callable[[Vl53l4Measurement], None]] = []
        self._measure_queues: list[StreamQueue] = []
        self._ranging = False
        self._initialized = False
        self._stream_parse_errors = 0

    # ── lifecycle ────────────────────────────────────────────────────────────

    @property
    def uld(self) -> VL53L4CD:
        """The underlying ULD driver (escape hatch for raw register access)."""
        return self._uld

    @property
    def initialized(self) -> bool:
        """True after a successful init(). Cleared by reset_sensor() and
        xshut() — a power-cycled sensor holds none of the ULD configuration."""
        return self._initialized

    def is_alive(self) -> bool:
        """True when the sensor answers with the VL53L4CD model id (0xEBAA)."""
        try:
            return self._uld.is_alive()
        except (Vl53l4cdError, StatusError, DepzTimeoutError, DepzError):
            return False

    def init(self, bus_khz: int = I2C_KHZ_DEFAULT) -> None:
        """Initialise the sensor: default configuration block + VHV calibration
        (ULD sensor_init). Takes well under a second; the bus is left at
        `bus_khz` (one of I2C_KHZ_STEPS)."""
        self._require_not_ranging()
        self._uld.sensor_init(bus_khz)
        self._initialized = True

    # ── sensor power (XSHUT pin) ─────────────────────────────────────────────

    def xshut(self, action: int) -> None:
        """Drive the XSHUT pin: XSHUT_OFF / XSHUT_ON / XSHUT_RESET. OFF and
        RESET stop any active stream on the bridge; a power-cycled sensor
        needs init() again."""
        timeout = 2.0 if action == XSHUT_RESET else None
        self.request(Vl53l4Cmd.XSHUT, pack_xshut(action), ok_completes=True, timeout=timeout)
        self._ranging = False
        self._initialized = False

    def reset_sensor(self) -> None:
        """Hardware sensor reset via XSHUT (blocks ~3 ms on the MCU). The ULD
        configuration is wiped — call init() again."""
        self.xshut(XSHUT_RESET)

    # ── bridge diagnostics ───────────────────────────────────────────────────

    def bridge_info(self) -> Vl53l4Info:
        """RPT_VL53_INFO: sensor identity, pin levels and bridge counters.
        Counters are free-running (wrap silently) — watch increments. Safe to
        call while streaming."""
        return self.request(
            Vl53l4Cmd.GET_INFO,
            matcher=DeviceBase.expect_report(Vl53l4Rpt.INFO, Vl53l4Info.unpack),
        )

    def set_i2c_speed_khz(self, khz: int) -> None:
        """Re-time the bridge's I2C bus to the nominal step nearest `khz`
        (I2C_KHZ_STEPS). Not while ranging — re-timing refuses a transfer in
        flight (ERR_BUSY). Read back the programmed step via bridge_info()."""
        self._require_not_ranging()
        self._platform.set_i2c_speed(khz)

    # ── configuration (init() first; not while ranging) ─────────────────────

    def get_range_timing(self) -> tuple[int, int]:
        """→ (timing_budget_ms, inter_measurement_ms). inter_measurement 0
        means continuous mode."""
        return self._uld.get_range_timing()

    def set_range_timing(self, timing_budget_ms: int, inter_measurement_ms: int = 0) -> None:
        """Set the timing budget (10–200 ms) and inter-measurement period.
        `inter_measurement_ms=0` selects continuous ranging; a value larger
        than the budget selects autonomous low-power mode. Not while ranging."""
        self._require_not_ranging()
        self._uld.set_range_timing(timing_budget_ms, inter_measurement_ms)

    def get_offset_mm(self) -> int:
        """Configured ranging offset in mm (signed)."""
        return self._uld.get_offset()

    def set_offset_mm(self, offset_mm: int) -> None:
        """Set the ranging offset correction in mm. Not while ranging."""
        self._require_not_ranging()
        self._uld.set_offset(offset_mm)

    def get_xtalk_kcps(self) -> int:
        """Configured crosstalk compensation in kcps (0 = disabled)."""
        return self._uld.get_xtalk()

    def set_xtalk_kcps(self, xtalk_kcps: int) -> None:
        """Set the crosstalk compensation in kcps. Not while ranging."""
        self._require_not_ranging()
        self._uld.set_xtalk(xtalk_kcps)

    def get_detection_thresholds(self) -> tuple[int, int, int]:
        """→ (distance_low_mm, distance_high_mm, window). Window is one of
        WINDOW_BELOW / WINDOW_ABOVE / WINDOW_OUT / WINDOW_IN."""
        return self._uld.get_detection_thresholds()

    def set_detection_thresholds(
        self, distance_low_mm: int, distance_high_mm: int, window: int
    ) -> None:
        """Program the distance-window interrupt (INT only fires when the
        window condition holds). Not while ranging."""
        self._require_not_ranging()
        self._uld.set_detection_thresholds(distance_low_mm, distance_high_mm, window)

    def get_signal_threshold_kcps(self) -> int:
        return self._uld.get_signal_threshold()

    def set_signal_threshold_kcps(self, signal_kcps: int) -> None:
        """Discard measurements whose return signal is below `signal_kcps`.
        Not while ranging."""
        self._require_not_ranging()
        self._uld.set_signal_threshold(signal_kcps)

    def get_sigma_threshold_mm(self) -> int:
        return self._uld.get_sigma_threshold()

    def set_sigma_threshold_mm(self, sigma_mm: int) -> None:
        """Discard measurements whose sigma exceeds `sigma_mm` (≤ 16383).
        Not while ranging."""
        self._require_not_ranging()
        self._uld.set_sigma_threshold(sigma_mm)

    def start_temperature_update(self) -> None:
        """Re-run VHV calibration; recommended after a >8 °C ambient change.
        Not while ranging (runs a short ranging burst internally)."""
        self._require_not_ranging()
        self._uld.start_temperature_update()

    def calibrate_offset(self, target_dist_mm: int, nb_samples: int = 20) -> int:
        """Offset calibration against a target at `target_dist_mm` (10–1000).
        Blocks for the sample burst; returns the offset now programmed."""
        self._require_not_ranging()
        return self._uld.calibrate_offset(target_dist_mm, nb_samples)

    def calibrate_xtalk(self, target_dist_mm: int, nb_samples: int = 20) -> int:
        """Crosstalk calibration against a target at `target_dist_mm` (10–5000).
        Blocks for the sample burst; returns the xtalk now programmed (kcps)."""
        self._require_not_ranging()
        return self._uld.calibrate_xtalk(target_dist_mm, nb_samples)

    # ── ranging ──────────────────────────────────────────────────────────────

    def start_ranging(self) -> None:
        """Start the sensor's ranging loop and arm the MCU stream: one
        RPT_VL53_STREAM per INT edge carrying the 17-byte result block."""
        self._require_not_ranging()
        self._uld.start_ranging()
        self.request(
            Vl53l4Cmd.START_STREAM,
            pack_start_stream(RESULT_BLOCK_ADDR, RESULT_BLOCK_LEN),
            ok_completes=True,
        )
        self._ranging = True

    def stop_ranging(self) -> None:
        if not self._ranging:
            return
        # Clear host state and stop the sensor even if the MCU STOP_STREAM ack
        # fails (link hiccup): otherwise the device is wedged "ranging" and no
        # reconfiguration is possible.
        try:
            self.request(Vl53l4Cmd.STOP_STREAM, ok_completes=True)
        finally:
            self._ranging = False
            self._uld.stop_ranging()

    @property
    def ranging(self) -> bool:
        return self._ranging

    def measure_once(self, timeout: float = 1.0) -> Vl53l4Measurement:
        """Single poll-mode measurement: start ranging, wait for data-ready,
        read the result block, stop. Raises while the stream is running."""
        self._require_not_ranging()
        self._uld.start_ranging()
        try:
            self._uld.wait_data_ready(int(timeout * 1000))
            result = self._uld.get_result()
            self._uld.clear_interrupt()
        finally:
            self._uld.stop_ranging()
        return Vl53l4Measurement._from_results(self._platform.last_timestamp_us, result)

    def on_measurement(self, cb: Callable[[Vl53l4Measurement], None]) -> Callable[[], None]:
        """Subscribe to streamed measurements (reader-thread context; don't
        block). Returns an unsubscribe function."""
        self._measure_cbs.append(cb)
        return lambda: self._measure_cbs.remove(cb)

    def measurements(self, maxsize: int = 64) -> StreamIterator:
        """Blocking iterator over measurements (bounded, drop-oldest;
        `dropped_count` on the returned iterator). Subscribes immediately."""
        return StreamIterator(self._measure_queues, maxsize, lambda: self.closed)

    def get_measurement(self, timeout: float = 2.0) -> Vl53l4Measurement:
        """Convenience: wait for the next streamed measurement.

        Raises `DepzTimeoutError` when nothing arrives within `timeout`, and
        `LinkClosedError` as soon as the device is closed while waiting."""
        if self.closed:
            raise LinkClosedError("device is closed")
        q = StreamQueue(1)
        self._measure_queues.append(q)
        try:
            deadline = time.monotonic() + timeout
            while True:
                remaining = deadline - time.monotonic()
                if remaining <= 0:
                    raise DepzTimeoutError(Vl53l4Rpt.STREAM, timeout) from None
                try:
                    return q.get(timeout=min(_CLOSE_POLL_S, remaining))
                except queue.Empty:
                    # A sample already queued wins over a concurrent close; only
                    # an *empty* queue on a closed device ends the wait.
                    if self.closed:
                        raise LinkClosedError(
                            "device closed while waiting for a measurement"
                        ) from None
        finally:
            self._measure_queues.remove(q)

    @property
    def stream_parse_errors(self) -> int:
        """Stream reports dropped because the result block failed to decode
        (short block from a reconfigured stream, corrupt read)."""
        return self._stream_parse_errors

    @property
    def stream_dropped_counts(self) -> list[int]:
        return [q.dropped_count for q in self._measure_queues]

    # ── internal ─────────────────────────────────────────────────────────────

    def _require_not_ranging(self) -> None:
        if self._ranging:
            raise DepzError("stop_ranging() first — the stream owns the register bank")

    def _handle_report(self, pkt: Packet) -> bool:
        if pkt.cmd != Vl53l4Rpt.STREAM or len(pkt.payload) < 12:
            return False
        sample = StreamData.unpack(pkt.payload)
        try:
            result = parse_result_block(sample.data)
        except Vl53l4cdError:
            # Somebody re-armed the stream on a different block — count, don't
            # crash the reader thread.
            self._stream_parse_errors += 1
            return True
        m = Vl53l4Measurement._from_results(sample.timestamp_us, result)
        for cb in list(self._measure_cbs):
            cb(m)
        for q in list(self._measure_queues):
            q.put(m)
        return True
