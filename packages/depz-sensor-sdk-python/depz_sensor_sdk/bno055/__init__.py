"""BNO055 9-axis IMU over the firmware register bridge
(contracts/13_SENSOR_BNO055.md).

The MCU is a thin I2C bridge: it reads and writes BNO055 registers on
request and streams one configured register block on a timer. Everything
else — operating mode, units, axis remap, calibration, decoding — is host
logic expressed as register access, and lives here. The BNO055 runs its own
sensor fusion on-chip; there is no host-side fusion driver to port.
"""

from __future__ import annotations

import threading
import time
from contextlib import contextmanager
from dataclasses import dataclass
from typing import Any, Iterator

from .._bridge import RegisterBridge, StreamArm
from ..device import DepzDevice
from ..stream import Stream
from ..errors import DepzError, InvalidStateError
from ..protocol._wire import check_uint
from ..protocol.bno055 import (
    I2C_ERROR_NAMES,
    RESET_TIMEOUT_S,
    TRIGGER_INT,
    TRIGGER_TIMER,
    XFER_MAX,
    BNO055_BRIDGE,
    Bno055Cmd,
    Bno055Info,
    Bno055Rpt,
    StreamData,
    pack_start_stream,
)
from ..transport import Packet
from .regs import (
    CALIB_PROFILE_LEN,
    FULL_BLOCK,
    FUSION_ACCEL_LSB,
    INTERRUPT_SETTING_REGS,
    MAG_LSB,
    MODE_SWITCH_FROM_CONFIG_S,
    MODE_SWITCH_TO_CONFIG_S,
    PLACEMENTS,
    QUAT_BLOCK,
    QUAT_LSB,
    REG1_ACC_CONFIG,
    REG1_GYR_CONFIG_0,
    REG1_INT_EN,
    REG1_INT_MSK,
    REG1_MAG_CONFIG,
    REG1_UNIQUE_ID,
    REG_AXIS_MAP_CONFIG,
    REG_CALIB_PROFILE,
    REG_CALIB_STAT,
    REG_INT_STA,
    REG_OPR_MODE,
    REG_PAGE_ID,
    REG_PWR_MODE,
    REG_SIC_MATRIX,
    REG_ST_RESULT,
    REG_QUA_DATA,
    REG_SYS_CLK_STATUS,
    REG_SYS_STATUS,
    REG_SYS_ERR,
    REG_SYS_TRIGGER,
    REG_TEMP_SOURCE,
    REG_UNIT_SEL,
    BOOT_SETTLE_TIMEOUT_S,
    FUSION_START_TIMEOUT_S,
    SELF_TEST_S,
    STUCK_SELF_TEST_POLLS,
    SYS_STATUS_BOOTING,
    SIC_IDENTITY,
    SYS_TRIGGER_CLK_SEL,
    SYS_TRIGGER_RST_INT,
    SYS_TRIGGER_SELF_TEST,
    UNIQUE_ID_LEN,
    AccelConfig,
    AxisRemap,
    CalibrationProfile,
    CalibStatus,
    GyroConfig,
    MagConfig,
    OprMode,
    PwrMode,
    RawBlock,
    SystemStatus,
    TempSource,
    Units,
    decode_block,
    pack_sic_matrix,
    unpack_sic_matrix,
)

__all__ = [
    "Bno055",
    "Bno055Info",
    "Bno055Sample",
    "OprMode",
    "PwrMode",
    "TempSource",
    "Units",
    "AxisRemap",
    "CalibStatus",
    "CalibrationProfile",
    "SystemStatus",
    "AccelConfig",
    "GyroConfig",
    "MagConfig",
    "RawBlock",
    "decode_block",
    "FULL_BLOCK",
    "QUAT_BLOCK",
    "PLACEMENTS",
    "SIC_IDENTITY",
    "TRIGGER_TIMER",
    "TRIGGER_INT",
    "I2C_ERROR_NAMES",
]

Vec3 = tuple[float, float, float]


@dataclass(frozen=True, kw_only=True)
class Bno055Sample:
    """One decoded register block (streamed or polled).

    Every channel is None when the block did not cover it; values are scaled
    by the units the sensor was set to when the stream started (`units`).
    In non-fusion modes Euler / quaternion / linear accel / gravity read
    zero; in CONFIG mode everything does."""

    timestamp_us: int  # MCU uptime: trigger time (stream) / read completion (poll)
    addr: int
    raw: bytes
    units: Units
    accel: Vec3 | None  # m/s² or mg
    mag: Vec3 | None  # µT
    gyro: Vec3 | None  # dps or rps
    euler: Vec3 | None  # (heading, roll, pitch), degrees or radians
    quaternion: tuple[float, float, float, float] | None  # (w, x, y, z), unit
    linear_accel: Vec3 | None  # acceleration minus gravity, always m/s²
    gravity: Vec3 | None  # always m/s²
    temperature: float | None  # °C or °F
    calibration: CalibStatus | None

    @staticmethod
    def decode(timestamp_us: int, addr: int, data: bytes, units: Units) -> "Bno055Sample":
        r = decode_block(addr, data)

        def vec3(v: tuple[int, int, int] | None, lsb: float) -> tuple[float, float, float] | None:
            return None if v is None else (v[0] / lsb, v[1] / lsb, v[2] / lsb)

        def vec4(
            v: tuple[int, int, int, int] | None, lsb: float
        ) -> tuple[float, float, float, float] | None:
            return None if v is None else (v[0] / lsb, v[1] / lsb, v[2] / lsb, v[3] / lsb)

        return Bno055Sample(
            timestamp_us=timestamp_us,
            addr=addr,
            raw=bytes(data),
            units=units,
            accel=vec3(r.accel, units.accel_lsb),
            mag=vec3(r.mag, MAG_LSB),
            gyro=vec3(r.gyro, units.gyro_lsb),
            euler=vec3(r.euler, units.euler_lsb),
            quaternion=vec4(r.quaternion, QUAT_LSB),
            linear_accel=vec3(r.linear_accel, FUSION_ACCEL_LSB),
            gravity=vec3(r.gravity, FUSION_ACCEL_LSB),
            temperature=None if r.temperature is None else r.temperature / units.temp_lsb,
            calibration=None if r.calib_stat is None else CalibStatus.unpack(r.calib_stat),
        )


class Bno055(DepzDevice):
    """BNO055 absolute-orientation IMU.

    Typical use: `configure()` (CONFIG → units → axis remap → optional
    calibration profile → NDOF), then `start_stream(period_ms=10)` and read
    `samples`, or poll `read_sample()`. The sensor fuses on
    chip at 100 Hz; the stream is timer-driven because the data-ready
    interrupt does not exist on the sensor firmware these boards carry
    (03.11).

    Page 1 (sensor configs, interrupts, unique id) is reached by switching
    PAGE_ID; the driver always switches back to page 0 and refuses page 1
    while a stream runs — the bridge would read the wrong registers."""

    _record_kind = "bno055"

    samples: Stream[Bno055Sample]
    """The streamed-sample stream: `samples.on(cb)` subscribes (reader-thread
    context; don't block), `samples(maxsize)` iterates (bounded, drop-oldest,
    subscribes immediately), `samples.next(timeout)` waits for one;
    `dropped_counts` and `parse_errors` (short blocks) are its counters."""

    def _init_subclass_state(self) -> None:
        # One lock serialises multi-step register sequences (page switch +
        # access, CONFIG-mode round trips) across caller threads.
        self._reg_lock = threading.RLock()
        self._bridge = RegisterBridge(
            self, cmds=BNO055_BRIDGE, read_max=XFER_MAX, write_max=XFER_MAX, timeout=None
        )
        self._units: Units | None = None  # None until read from UNIT_SEL
        self._arm = StreamArm(
            self,
            Bno055Cmd.START_STREAM,
            Bno055Cmd.STOP_STREAM,
            busy="stop_stream() first — the stream reads page 0",
        )
        # The units the stream was started with, the one thing the reader
        # thread reads (the block address rides in every report).
        self._stream_units = Units()
        self.samples = Stream[Bno055Sample](self, maxsize=256, what=Bno055Rpt.STREAM)
        self._configured: dict[str, Any] | None = None  # last configure() arguments
        self._mode_before_suspend: OprMode | None = None  # put back on wake-up

    # ── identity & bridge diagnostics ────────────────────────────────────────

    def bridge_info(self) -> Bno055Info:
        """RPT_BNO_INFO: chip ids, sensor firmware revision and bridge
        counters. Safe to call while streaming."""
        return self.request(
            Bno055Cmd.GET_INFO,
            matcher=DepzDevice.expect_report(Bno055Rpt.INFO, Bno055Info.unpack),
        )

    def is_alive(self) -> bool:
        """True when the bridge passed the chip-ID handshake and the sensor
        answers with the BNO055 ids."""
        try:
            info = self.bridge_info()
        except DepzError:
            return False
        return bool(info.initialized) and info.ids_ok

    def reset_sensor(self) -> None:
        """Hardware reset via nRESET (the bridge answers after the ~0.5 s boot
        handshake). Stops any stream; the sensor comes back in CONFIG mode
        with power-on units and no calibration — call configure() again, or
        restore_configuration()."""
        with self._reg_lock:
            self.request(Bno055Cmd.RESET, ok_completes=True, timeout=RESET_TIMEOUT_S)
            self._arm.active = False
            self._units = None
            self._wait_booted()

    # ── raw register access ──────────────────────────────────────────────────

    def read_registers(self, addr: int, length: int, page: int = 0) -> bytes:
        """Read `length` bytes starting at `addr` on `page` (split into
        128-byte transfers). Page 1 is refused while streaming."""
        with self._reg_lock, self._on_page(page):
            return self._bridge.read(addr, length)

    def write_registers(self, addr: int, data: bytes, page: int = 0) -> None:
        """Write `data` starting at `addr` on `page`. Most configuration
        registers only accept writes in CONFIG mode — the sensor silently
        ignores the rest; the typed setters handle that for you."""
        with self._reg_lock, self._on_page(page):
            self._bridge.write(addr, data)

    def read_register(self, addr: int, page: int = 0) -> int:
        return self.read_registers(addr, 1, page)[0]

    def write_register(self, addr: int, value: int, page: int = 0) -> None:
        """Write one register; a `value` outside 0..255 is refused (`ValueError`)."""
        self.write_registers(addr, bytes([check_uint("value", value, 8)]), page)

    # ── operating mode, power, units, axes ──────────────────────────────────

    def get_operation_mode(self) -> OprMode:
        return OprMode(self.read_register(REG_OPR_MODE) & 0x0F)

    def set_operation_mode(self, mode: OprMode | int) -> None:
        """Switch OPR_MODE and wait out the datasheet switching time
        (7 ms from CONFIG, 19 ms into CONFIG). Into a fusion mode it also
        waits (up to 1 s) for the fusion outputs, which read zero for
        ~70 ms after every CONFIG → fusion transition.

        The sensor only switches between CONFIG and another mode: a direct
        write from one operating mode to another is silently ignored
        (measured: NDOF → AMG stays NDOF). Such a switch goes through CONFIG."""
        mode = OprMode(mode)
        with self._reg_lock:
            if mode != OprMode.CONFIG:
                current = OprMode(self._bridge.read(REG_OPR_MODE, 1)[0] & 0x0F)
                if current == mode:
                    return
                if current != OprMode.CONFIG:
                    self._switch_mode(OprMode.CONFIG)
            self._switch_mode(mode)

    def _switch_mode(self, mode: OprMode) -> None:
        """One OPR_MODE write plus its settle time (no via-CONFIG logic)."""
        with self._reg_lock:
            self._bridge.write(REG_OPR_MODE, bytes([mode]))
            time.sleep(
                MODE_SWITCH_TO_CONFIG_S if mode == OprMode.CONFIG else MODE_SWITCH_FROM_CONFIG_S
            )
            if mode.is_fusion:
                self._wait_fusion_started(strict=False)

    def get_power_mode(self) -> PwrMode:
        return PwrMode(self.read_register(REG_PWR_MODE) & 0x03)

    def set_power_mode(self, mode: PwrMode | int) -> None:
        """Normal / low power (accelerometer only until motion) / suspend.
        Written in CONFIG mode; the operating mode is restored after.

        A suspended chip ignores OPR_MODE writes and stays in CONFIG
        (measured 19.10.2026), so the operating mode is kept here and put
        back on wake-up — NDOF if the chip was already asleep when we
        connected (contract 13 §4.6)."""
        mode = PwrMode(mode)
        with self._reg_lock:
            opr = OprMode(self._bridge.read(REG_OPR_MODE, 1)[0] & 0x0F)
            pwr = self._bridge.read(REG_PWR_MODE, 1)[0] & 0x03
            if opr != OprMode.CONFIG:
                resume: OprMode | None = opr
                self._switch_mode(OprMode.CONFIG)
            elif pwr == PwrMode.SUSPEND:
                resume = self._mode_before_suspend or OprMode.NDOF
            else:
                resume = None  # CONFIG on purpose: stay there
            self._bridge.write(REG_PWR_MODE, bytes([mode]))
            if mode == PwrMode.SUSPEND:
                self._mode_before_suspend = resume
            else:
                self._mode_before_suspend = None
                if resume is not None:
                    self._switch_mode(resume)

    def get_units(self) -> Units:
        units = Units.unpack(self.read_register(REG_UNIT_SEL))
        self._units = units
        return units

    def set_units(self, units: Units) -> None:
        with self._config_mode():
            self._bridge.write(REG_UNIT_SEL, bytes([units.pack()]))
            self._units = units

    def get_axis_remap(self) -> AxisRemap:
        config, sign = self.read_registers(REG_AXIS_MAP_CONFIG, 2)
        return AxisRemap.unpack(config, sign)

    def set_axis_remap(self, remap: AxisRemap | str) -> None:
        """Remap output axes for the board's mounting: an AxisRemap, or a
        datasheet placement name "P0".."P7" (P1 = default)."""
        if isinstance(remap, str):
            remap = AxisRemap.placement(remap)
        config, sign = remap.pack()
        with self._config_mode():
            self._bridge.write(REG_AXIS_MAP_CONFIG, bytes([config, sign]))

    def get_temperature_source(self) -> TempSource:
        return TempSource(self.read_register(REG_TEMP_SOURCE) & 0x03)

    def set_temperature_source(self, source: TempSource | int) -> None:
        with self._config_mode():
            self._bridge.write(REG_TEMP_SOURCE, bytes([TempSource(source)]))

    def configure(
        self,
        mode: OprMode | int = OprMode.NDOF,
        units: Units | None = None,
        axis_remap: AxisRemap | str | None = None,
        calibration: CalibrationProfile | None = None,
    ) -> None:
        """The usual session setup: CONFIG → units → axis remap →
        calibration profile → `mode`. In a fusion mode it returns once the
        fusion outputs are live (~70 ms). Remembered for
        restore_configuration()."""
        units = units or Units()
        mode = OprMode(mode)
        if isinstance(axis_remap, str):
            axis_remap = AxisRemap.placement(axis_remap)
        with self._reg_lock:
            self._wait_booted()
            self._switch_mode(OprMode.CONFIG)
            self._bridge.write(REG_UNIT_SEL, bytes([units.pack()]))
            self._units = units
            if axis_remap is not None:
                self._bridge.write(REG_AXIS_MAP_CONFIG, bytes(axis_remap.pack()))
            if calibration is not None:
                self._bridge.write(REG_CALIB_PROFILE, calibration.pack())
            self._switch_mode(mode)
            if mode.is_fusion:
                self._wait_fusion_started(strict=True)
        self._configured = {
            "mode": mode,
            "units": units,
            "axis_remap": axis_remap,
            "calibration": calibration,
        }

    def restore_configuration(self) -> None:
        """Re-apply the last configure() — after reset_sensor(), or when
        bridge_info().sensor_resets rose (the bridge's bus recovery pulses
        nRESET and the sensor comes back in CONFIG mode). A running stream
        keeps running across it."""
        if self._configured is None:
            raise InvalidStateError("configure() has not been called")
        self.configure(**self._configured)

    # ── status, self-test, calibration ──────────────────────────────────────

    def system_status(self) -> SystemStatus:
        """ST_RESULT, SYS_CLK_STATUS, SYS_STATUS, SYS_ERR (INT_STA skipped —
        it clears on read)."""
        with self._reg_lock:
            st = self._bridge.read(REG_ST_RESULT, 1)[0]
            clk, status, err = self._bridge.read(REG_SYS_CLK_STATUS, 3)
        return SystemStatus(self_test=st, clk_status=clk, status=status, error=err)

    def self_test(self) -> SystemStatus:
        """Built-in self-test (datasheet §3.9.2): CONFIG mode, SYS_TRIGGER
        SELF_TEST, ~400 ms, then SYS_ERR = 3 and a cleared ST_RESULT bit on
        failure. The operating mode is restored after. Not while streaming."""
        self._arm.require_idle()
        with self._reg_lock:
            previous = OprMode(self._bridge.read(REG_OPR_MODE, 1)[0] & 0x0F)
            if previous != OprMode.CONFIG:
                self._switch_mode(OprMode.CONFIG)
            try:
                trigger = self._bridge.read(REG_SYS_TRIGGER, 1)[0] & SYS_TRIGGER_CLK_SEL
                self._bridge.write(REG_SYS_TRIGGER, bytes([trigger | SYS_TRIGGER_SELF_TEST]))
                time.sleep(SELF_TEST_S)
                status = self.system_status()
                # Staying in CONFIG would leave SYS_STATUS at 4 for good.
                if previous == OprMode.CONFIG:
                    self._clear_self_test_status()
                return status
            finally:
                if previous != OprMode.CONFIG:
                    self._switch_mode(previous)

    def calibration_status(self) -> CalibStatus:
        """CALIB_STAT: per-sensor 0..3. In NDOF the sensor calibrates in the
        background; see the datasheet §3.11 for the motions each needs."""
        return CalibStatus.unpack(self.read_register(REG_CALIB_STAT))

    def read_calibration_profile(self) -> CalibrationProfile:
        """Offsets and radii (0x55..0x6A). The sensor only exposes them in
        CONFIG mode — the driver switches there and back."""
        with self._config_mode():
            return CalibrationProfile.unpack(self._bridge.read(REG_CALIB_PROFILE, CALIB_PROFILE_LEN))

    def write_calibration_profile(self, profile: CalibrationProfile) -> None:
        """Restore a stored profile (datasheet §3.11.5): CONFIG, write all 22
        bytes, back to the previous mode. It is a starting point, not a lock:
        the background calibration refines it as soon as fusion resumes —
        measured on an uncalibrated magnetometer, NDOF rewrites the mag
        radius at once and the gyro offsets within ~0.5 s. Read it back
        without leaving CONFIG to verify the write itself."""
        with self._config_mode():
            self._bridge.write(REG_CALIB_PROFILE, profile.pack())

    def get_sic_matrix(self) -> tuple[int, ...]:
        """Soft-iron compensation matrix, 9 × i16 row-major, 1.0 = 16384."""
        return unpack_sic_matrix(self.read_registers(REG_SIC_MATRIX, 18))

    def set_sic_matrix(self, matrix: tuple[int, ...] = SIC_IDENTITY) -> None:
        with self._config_mode():
            self._bridge.write(REG_SIC_MATRIX, pack_sic_matrix(matrix))

    # ── page 1: raw sensor configuration (non-fusion modes) ────────────────

    def get_accel_config(self) -> AccelConfig:
        return AccelConfig.unpack(self.read_register(REG1_ACC_CONFIG, page=1))

    def set_accel_config(self, config: AccelConfig) -> None:
        """Range / bandwidth / power of the accelerometer. Fusion modes
        override it — effective in non-fusion modes only."""
        self._write_page1_config(REG1_ACC_CONFIG, bytes([config.pack()]))

    def get_gyro_config(self) -> GyroConfig:
        return GyroConfig.unpack(self.read_registers(REG1_GYR_CONFIG_0, 2, page=1))

    def set_gyro_config(self, config: GyroConfig) -> None:
        self._write_page1_config(REG1_GYR_CONFIG_0, config.pack())

    def get_mag_config(self) -> MagConfig:
        return MagConfig.unpack(self.read_register(REG1_MAG_CONFIG, page=1))

    def set_mag_config(self, config: MagConfig) -> None:
        self._write_page1_config(REG1_MAG_CONFIG, bytes([config.pack()]))

    def unique_id(self) -> bytes:
        """The chip's 16-byte unique id (page 1, 0x50..0x5F)."""
        return self.read_registers(REG1_UNIQUE_ID, UNIQUE_ID_LEN, page=1)

    # ── interrupts (motion only on SW rev 03.11) ────────────────────────────

    def get_interrupt_enable(self) -> int:
        return self.read_register(REG1_INT_EN, page=1)

    def set_interrupt_enable(self, mask: int) -> None:
        """INT_EN: which interrupt engines run (INT_* bits in regs)."""
        self.write_register(REG1_INT_EN, mask, page=1)

    def get_interrupt_mask(self) -> int:
        return self.read_register(REG1_INT_MSK, page=1)

    def set_interrupt_mask(self, mask: int) -> None:
        """INT_MSK: which enabled interrupts drive the INT pin."""
        self.write_register(REG1_INT_MSK, mask, page=1)

    def set_interrupt_setting(self, register: int, value: int) -> None:
        """Write one motion-interrupt setting register (page 1, 0x11..0x1F:
        thresholds, durations, axis selects — datasheet §3.8.2), as a raw
        byte. Not while streaming."""
        if register not in INTERRUPT_SETTING_REGS:
            raise ValueError(f"0x{register:02X} is not a page-1 interrupt setting (0x11..0x1F)")
        self._write_page1_config(register, bytes([check_uint("value", value, 8)]))

    def read_interrupt_status(self) -> int:
        """INT_STA — which interrupts fired. Clears on read."""
        return self.read_register(REG_INT_STA)

    def clear_interrupt(self) -> None:
        """SYS_TRIGGER RST_INT: reset the interrupt status bits and the INT
        pin (the bridge does this itself on an INT-triggered stream)."""
        with self._reg_lock:
            clk = self._bridge.read(REG_SYS_TRIGGER, 1)[0] & SYS_TRIGGER_CLK_SEL
            self._bridge.write(REG_SYS_TRIGGER, bytes([clk | SYS_TRIGGER_RST_INT]))

    # ── data ────────────────────────────────────────────────────────────────

    def read_sample(self, block: tuple[int, int] = FULL_BLOCK) -> Bno055Sample:
        """Poll one register block (default the full 46-byte 0x08..0x35) and
        decode it. Works alongside a stream."""
        addr, length = block
        with self._reg_lock:
            units = self._current_units()
            data = self._bridge.read(addr, length)
            ts = self._bridge.last_timestamp_us
        return Bno055Sample.decode(ts, addr, data, units)

    def read_quaternion(self) -> tuple[float, float, float, float]:
        """(w, x, y, z) — the cheapest orientation read (8 bytes)."""
        q = self.read_sample(QUAT_BLOCK).quaternion
        assert q is not None
        return q

    def start_stream(
        self,
        period_ms: int = 10,
        block: tuple[int, int] = FULL_BLOCK,
        trigger: int = TRIGGER_TIMER,
    ) -> None:
        """Arm the bridge: read `block` every `period_ms` (TIMER) and push it.
        Fusion runs at 100 Hz, so 10 ms is the useful floor; the 46-byte
        block costs ~3.2 ms of bus in NDOF. TRIGGER_INT reads on the INT
        edge instead (motion interrupts only on SW 03.11) with `period_ms`
        as a missed-edge watchdog (0 = none). Replaces a running stream."""
        addr, length = block
        if not 1 <= length <= XFER_MAX or addr + length > 0x100:
            raise ValueError(f"block 0x{addr:02X}+{length} outside 1..{XFER_MAX} / page")
        # Encoded (and so range-checked: TIMER period 1..60000) before the
        # units read below — a refused argument puts nothing on the wire.
        payload = pack_start_stream(trigger, addr, length, period_ms)
        with self._reg_lock:
            # Units first: the reader thread may decode the first sample
            # before request() below even returns.
            self._stream_units = self._current_units()
            self._arm.start(payload)

    def stop_stream(self) -> None:
        self._arm.stop()

    @property
    def streaming(self) -> bool:
        return self._arm.active

    # ── internal ────────────────────────────────────────────────────────────

    @property
    def _record_stream(self) -> Stream[Bno055Sample]:
        return self.samples

    def _record_value(self, s: Bno055Sample, *, layers: bool = False) -> dict[str, Any]:
        v: dict[str, Any] = {"unit_sel": s.units.pack()}
        for key in ("quaternion", "euler", "accel", "gyro", "mag", "linear_accel", "gravity"):
            val = getattr(s, key)
            if val is not None:
                v[key] = list(val)
        if s.temperature is not None:
            v["temperature"] = s.temperature
        if s.calibration is not None:
            c = s.calibration
            v["calib"] = [c.system, c.gyro, c.accel, c.mag]
        return v

    @contextmanager
    def _on_page(self, page: int) -> Iterator[None]:
        if page == 0:
            yield
            return
        if page != 1:
            raise ValueError(f"BNO055 has register pages 0 and 1, not {page}")
        self._arm.require_idle()
        self._bridge.write(REG_PAGE_ID, b"\x01")
        try:
            yield
        finally:
            self._bridge.write(REG_PAGE_ID, b"\x00")

    @contextmanager
    def _config_mode(self) -> Iterator[None]:
        """Run a block in CONFIG mode and put the previous mode back."""
        with self._reg_lock:
            previous = OprMode(self._bridge.read(REG_OPR_MODE, 1)[0] & 0x0F)
            if previous != OprMode.CONFIG:
                self._switch_mode(OprMode.CONFIG)
            try:
                yield
            finally:
                if previous != OprMode.CONFIG:
                    self._switch_mode(previous)

    def _clear_self_test_status(self) -> None:
        """A self-test run in CONFIG leaves SYS_STATUS at 4 until the mode
        leaves CONFIG; step into ACCONLY and back."""
        self._switch_mode(OprMode.ACCONLY)
        self._switch_mode(OprMode.CONFIG)

    def _wait_booted(self) -> None:
        """Poll SYS_STATUS until the sensor's own boot (init + POST) is over;
        a SYS_STATUS 4 that outlasts POST is a self-test leftover, cleared."""
        deadline = time.monotonic() + BOOT_SETTLE_TIMEOUT_S
        in_self_test = 0
        cleared = False
        while (status := self._bridge.read(REG_SYS_STATUS, 1)[0]) in SYS_STATUS_BOOTING:
            in_self_test = in_self_test + 1 if status == 4 else 0
            if in_self_test >= STUCK_SELF_TEST_POLLS and not cleared:
                self._clear_self_test_status()
                cleared = True
                in_self_test = 0
                continue
            if time.monotonic() > deadline:
                raise DepzError("BNO055 did not finish booting (SYS_STATUS stuck)")
            time.sleep(0.005)

    def _wait_fusion_started(self, strict: bool) -> None:
        """A running fusion never outputs the all-zero quaternion. `strict`
        raises on timeout; otherwise give up quietly (a suspended sensor
        never updates its registers)."""
        deadline = time.monotonic() + FUSION_START_TIMEOUT_S
        while not any(self._bridge.read(REG_QUA_DATA, 8)):
            if time.monotonic() > deadline:
                if strict:
                    raise DepzError("BNO055 fusion did not start (quaternion stays zero)")
                return
            time.sleep(0.01)

    def _write_page1_config(self, addr: int, data: bytes) -> None:
        self._arm.require_idle()
        with self._config_mode(), self._on_page(1):
            self._bridge.write(addr, data)

    def _current_units(self) -> Units:
        if self._units is None:
            self._units = Units.unpack(self._bridge.read(REG_UNIT_SEL, 1)[0])
        return self._units

    def _handle_report(self, pkt: Packet) -> bool:
        if pkt.cmd != Bno055Rpt.STREAM:
            return False
        try:
            # short header or body: refused (ValueError) and counted — contract 07 §2
            s = StreamData.unpack(pkt.payload)
        except ValueError:
            self.samples.parse_errors += 1
            return True
        sample = Bno055Sample.decode(s.timestamp_us, s.addr, s.data, self._stream_units)
        self.samples._emit(sample)
        return True

