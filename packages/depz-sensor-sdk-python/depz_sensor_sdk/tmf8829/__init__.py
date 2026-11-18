"""TMF8829 multizone dToF sensor over the SPI register bridge (contracts/14).

The board is a thin bridge: this class runs the sensor — soft reset, sensor
firmware download through the sensor bootloader, presets and the config page,
start/stop — and turns the bridge's chunked frame stream into measurements
(half-frame merge, histograms, ref-SPAD frames, clock model, capture time).

Typical use::

    with Tmf8829("/dev/ttyACM0") as dev:
        dev.init()
        dev.configure("32x32")
        dev.start()
        for m in dev.measurements():
            print(m.timestamp_us, m.distance_mm[0].shape)
"""

from __future__ import annotations

import struct
import threading
import time
from dataclasses import dataclass, field
from importlib import resources
from types import MappingProxyType
from typing import Any, Callable, SupportsIndex

from .._bridge import RegisterBridge, StreamArm
from ..device import DepzDevice, DeviceEvent
from ..stream import Stream
from ..errors import DepzError
from ..protocol._wire import check_uint
from ..protocol.tmf8829 import (
    INT_HISTOGRAMS,
    INT_RESULTS,
    READ_MAX,
    SF_REF_FRAME,
    TMF8829_BRIDGE,
    WRITE_MAX,
    EventCode,
    FrameChunk,
    FrameReassembler,
    Tmf8829Cmd,
    Tmf8829Event,
    Tmf8829Info,
    Tmf8829Rpt,
    pack_start_stream,
)
from ..transport import Packet
from .builder import MeasurementBuilder, Tmf8829Measurement, distance_mm, record_value
from .clock import FT_NAMES, CaptureTimer, FrameTime, SensorClock, flag_names
from .config import (
    CFG_FIELDS,
    CFG_SIZE,
    DEFAULT_PERIOD_MS,
    PRESET_KILO_ITERATIONS,
    PRESETS,
    REG_CFG,
    cfg_get,
    cfg_set,
    config_fields,
)
from .frames import (
    FP_GRID,
    FrameInfo,
    HistogramFrame,
    PixelResults,
    merge_halves,
    parse_frame,
    parse_results,
)

__all__ = [
    "Tmf8829",
    "Tmf8829Config",
    "Tmf8829Error",
    "Tmf8829Measurement",
    "Tmf8829InterruptEvent",
    "Tmf8829Info",
    "PixelResults",
    "FrameInfo",
    "FrameTime",
    "HistogramFrame",
    "MeasurementBuilder",
    "SensorClock",
    "CaptureTimer",
    "PRESETS",
    "CFG_FIELDS",
    "FP_GRID",
    "FT_NAMES",
    "cfg_get",
    "cfg_set",
    "config_fields",
    "distance_mm",
    "flag_names",
    "merge_halves",
    "parse_frame",
    "parse_results",
    "FIRMWARE_LOAD_ADDR",
    "firmware_image",
]

# Host-interface registers (sensor bootloader and application)
REG_APP_ID = 0x00
REG_CMD_STAT = 0x08
REG_SERIAL = 0x1C
REG_CID_RID = 0x20
REG_INT_STATUS = 0xE1
REG_INT_ENAB = 0xE2
REG_ID = 0xE3
REG_REVID = 0xE4
REG_RESET = 0xF7
REG_ENABLE = 0xF8
REG_FIFOSTATUS = 0xFA
REG_FIFO = 0xFF

DEVICE_ID = 0x9E
APP_ID_APP = 0x01
APP_ID_BL = 0x80
ENABLE_CPU_READY = 0x80
ENABLE_PUSEL_MASK = 0x30
ENABLE_PUSEL_BOOTMON = 0x10
ENABLE_PUSEL_RAM = 0x20
ENABLE_PON = 0x04
RESET_SOFT = 0x40
FIFO_ERROR_FLAGS = 0x30

BL_START_RAM_APP = 0x16
BL_I2C_OFF = 0x22
BL_W_FIFO_BOTH = 0x45
BL_STATUS_NAMES = MappingProxyType({0: "READY", 1: "ERR_PARAM", 2: "ERR_ADDR", 4: "ERR_FIFO"})

CMD_MEASURE = 0x10
CMD_WRITE_PAGE = 0x15
CMD_LOAD_CONFIG_PAGE = 0x16
CMD_STOP = 0xFF
STAT_OK = 0x00
STAT_ACCEPTED = 0x01
STAT_NAMES = MappingProxyType({
    0x00: "OK",
    0x01: "ACCEPTED",
    0x02: "ERR_CONFIG",
    0x03: "ERR_APPLICATION",
    0x04: "ERR_CONFIG_RESULT_SIZE",
    0x05: "ERR_CONFIG_VCSEL",
    0x06: "ERR_WAKEUP_TIMED",
    0x07: "ERR_RESET_UNEXPECTED",
    0x08: "ERR_UNKNOWN_CMD",
    0x09: "ERR_UNKNOWN_CID",
    0x0A: "ERR_STOP_0",
    0x0B: "ERR_STOP_1",
    0x0C: "ERR_STOP_2",
    0x0D: "ERR_STOP_3",
    0x0E: "ERR_OSC_TUNE",
})

#: ams tmf8829_application.hex (MIT, see data/LICENSES-MIT.TXT), application 2.200.0
FIRMWARE_LOAD_ADDR = 0x00010000
REGISTER_TIMEOUT_S = 1.0  # READ/WRITE_REG may wait behind a 25 KB histogram frame


def firmware_image() -> bytes:
    """The sensor firmware image downloaded by `init()` (14 444 B)."""
    return (
        resources.files(__package__)
        .joinpath("data/tmf8829_application.bin")
        .read_bytes()
    )


class Tmf8829Error(DepzError):
    """The sensor refused or did not answer (bootloader / application status)."""


@dataclass(frozen=True, kw_only=True)
class Tmf8829InterruptEvent(DeviceEvent):
    """RPT_TMF_EVENT that is not an expected stop artefact: a MOTION /
    PROXIMITY interrupt (code 0) or a refused frame header (code 1 — the
    sensor is lost, run `init()` again)."""

    event: Tmf8829Event


@dataclass(frozen=True, kw_only=True)
class Tmf8829Config:
    """The config written by `configure()`, as read back from the sensor."""

    preset: str
    page: bytes  # the 190-byte page 0x22..0xDF
    histograms: bool
    ref_frame: bool
    fields: dict[str, int] = field(default_factory=dict)

    @property
    def grid(self) -> tuple[int, int]:
        """(rows, cols) of a measurement."""
        return FP_GRID[self.fields["fp_mode"]]


def _is_int(v: object) -> bool:
    return isinstance(v, SupportsIndex)


class Tmf8829(DepzDevice):
    """TMF8829 ToF device (8×8 … 48×32 zones, up to 4 peaks per zone,
    histograms). `init()` downloads the 14.4 KB sensor firmware (~0.15 s),
    `configure(preset, …)` writes the config page, `start()` streams
    measurements to `measurements`, `stop()` ends it.

    The sensor's EN pin is wired high: there is no sleep, and recovery from
    any state is `init()` (it starts with a soft reset)."""

    _record_kind = "tmf8829"

    measurements: Stream[Tmf8829Measurement]
    """The measurement stream: `measurements.on(cb)` subscribes (reader-thread
    context; don't block), `measurements(maxsize)` iterates (bounded,
    drop-oldest, subscribes immediately — call before or after `start()`),
    `measurements.next(timeout)` waits for one; `dropped_counts` and
    `parse_errors` (a FRAME chunk or EVENT shorter than its header, reset by
    `start()`; `frame_parse_errors` adds the builder's) are its counters."""

    def _init_subclass_state(self) -> None:
        self._reg_lock = (
            threading.RLock()
        )  # the bridge takes one register command at a time
        self._bridge = RegisterBridge(
            self,
            cmds=TMF8829_BRIDGE,
            read_max=READ_MAX,
            write_max=WRITE_MAX,
            timeout=REGISTER_TIMEOUT_S,
            sticky_addr=REG_FIFO,
        )
        self._reassembler = FrameReassembler()
        self._builder = MeasurementBuilder()
        self.measurements = Stream[Tmf8829Measurement](self, maxsize=16, what=Tmf8829Rpt.FRAME)
        self._arm = StreamArm(
            self,
            Tmf8829Cmd.START_STREAM,
            Tmf8829Cmd.STOP_STREAM,
            busy="configure() while streaming: stop() first",
        )
        self._stopping = False
        self._config: Tmf8829Config | None = None
        self.app_version: str | None = None
        self.sensor_serial: int | None = None

    # ── register access ──────────────────────────────────────────────────────

    def read_registers(self, addr: int, length: int) -> bytes:
        """Read `length` bytes from `addr` in ≤1024-byte requests; reads that
        reach the FIFO port 0xFF stay there (the sensor does not auto-increment it).
        A reply shorter or longer than asked raises `DepzError`."""
        with self._reg_lock:
            return self._bridge.read(addr, length)

    def read_register(self, addr: int) -> int:
        return self.read_registers(addr, 1)[0]

    def write_registers(
        self, addr: int, data: bytes, progress: Callable[[int, int], None] | None = None
    ) -> None:
        """Write `data` from `addr` in ≤1023-byte requests (writes to 0xFF stay at 0xFF)."""
        with self._reg_lock:
            self._bridge.write(addr, data, progress)

    def write_register(self, addr: int, value: int) -> None:
        """Write one register; a `value` outside 0..255 is refused (`ValueError`)."""
        self.write_registers(addr, bytes([check_uint("value", value, 8)]))

    def _poll(
        self, addr: int, done: Callable[[int], bool], timeout: float, what: str
    ) -> int:
        deadline = time.monotonic() + timeout
        while True:
            v = self.read_register(addr)
            if done(v):
                return v
            if time.monotonic() > deadline:
                raise Tmf8829Error(f"{what}: timeout, reg 0x{addr:02X} = 0x{v:02X}")

    def sensor_state(self) -> dict[str, int]:
        """Raw host-interface registers, for diagnostics."""
        regs = {
            "ID": REG_ID,
            "REVID": REG_REVID,
            "ENABLE": REG_ENABLE,
            "RESET": REG_RESET,
            "APP_ID": REG_APP_ID,
            "CMD_STAT": REG_CMD_STAT,
            "INT_STATUS": REG_INT_STATUS,
            "INT_ENAB": REG_INT_ENAB,
            "FIFOSTATUS": REG_FIFOSTATUS,
        }
        return {k: self.read_register(a) for k, a in regs.items()}

    # ── bridge ───────────────────────────────────────────────────────────────

    def get_bridge_info(self) -> Tmf8829Info:
        """Bridge counters (never touches the sensor, safe mid-stream)."""
        return self.request(
            Tmf8829Cmd.GET_INFO,
            matcher=DepzDevice.expect_report(Tmf8829Rpt.INFO, Tmf8829Info.unpack),
        )

    # ── init: sensor bootloader + firmware download ──────────────────────────

    def _bl_cmd(self, cmd: int, params: bytes = b"", timeout: float = 0.1) -> None:
        payload = bytes([cmd]) if not params else bytes([cmd, len(params)]) + params
        self.write_registers(REG_CMD_STAT, payload)
        st = self._poll(
            REG_CMD_STAT, lambda v: v != cmd, timeout, f"bootloader cmd 0x{cmd:02X}"
        )
        if st != 0:
            raise Tmf8829Error(
                f"bootloader cmd 0x{cmd:02X} -> {BL_STATUS_NAMES.get(st, hex(st))}"
            )

    def init(
        self, *, progress: Callable[[int, int], None] | None = None
    ) -> tuple[str, int]:
        """Bring the sensor from any state to its running RAM application:
        soft reset, bootloader, firmware download, start. Returns
        (application version, sensor serial). Stops a running stream first.
        `progress(done, total)` tracks the image download."""
        if self._arm.active:
            self.stop()
        with self._reg_lock:
            self.write_register(REG_ENABLE, ENABLE_PON | ENABLE_PUSEL_BOOTMON)
            time.sleep(0.003)
            self.write_register(REG_RESET, RESET_SOFT)
            time.sleep(0.003)
            dev = self.read_register(REG_ID)
            if dev != DEVICE_ID:
                raise Tmf8829Error(
                    f"sensor ID 0x{dev:02X}, expected 0x{DEVICE_ID:02X} (no TMF8829 on the bus?)"
                )
            self._poll(REG_APP_ID, lambda v: v == APP_ID_BL, 0.1, "sensor bootloader")
            self._bl_cmd(BL_I2C_OFF)
            # ENABLE reads 0 over SPI until the sensor's I2C is off.
            self._poll(
                REG_ENABLE, lambda v: bool(v & ENABLE_CPU_READY), 0.1, "cpu_ready"
            )
            image = firmware_image()
            image += b"\xff" * (-len(image) % 4)
            self._bl_cmd(
                BL_W_FIFO_BOTH, struct.pack("<IH", FIRMWARE_LOAD_ADDR, len(image) // 4)
            )
            self.write_registers(REG_FIFO, image, progress)
            self._bl_cmd(BL_START_RAM_APP)
            self._poll(REG_APP_ID, lambda v: v == APP_ID_APP, 0.1, "START_RAM_APP")
            en = self.read_register(
                REG_ENABLE
            )  # wake-ups must boot the RAM application
            self.write_register(
                REG_ENABLE, (en & ~ENABLE_PUSEL_MASK) | ENABLE_PUSEL_RAM
            )
            ver = self.read_registers(REG_APP_ID, 4)
            self.app_version = f"{ver[1]}.{ver[2]}.{ver[3]}"
            self.sensor_serial = struct.unpack(
                "<I", self.read_registers(REG_SERIAL, 4)
            )[0]
        self._config = None
        return self.app_version, self.sensor_serial

    # ── application commands, config page ────────────────────────────────────

    def _app_cmd(
        self, cmd: int, accept: tuple[int, ...] = (STAT_OK,), timeout: float = 1.0
    ) -> int:
        self.write_register(REG_CMD_STAT, cmd)
        st = self._poll(
            REG_CMD_STAT, lambda v: v < 0x10, timeout, f"sensor cmd 0x{cmd:02X}"
        )
        if st not in accept:
            raise Tmf8829Error(
                f"sensor cmd 0x{cmd:02X} -> {STAT_NAMES.get(st, hex(st))}"
            )
        return st

    def config_page(self) -> bytes:
        """LOAD_CONFIG_PAGE → the 190-byte page 0x22..0xDF now in the sensor."""
        with self._reg_lock:
            self._app_cmd(CMD_LOAD_CONFIG_PAGE)
            cid = self.read_register(REG_CID_RID)
            if cid != CMD_LOAD_CONFIG_PAGE:
                raise Tmf8829Error(f"CID_RID 0x{cid:02X} after LOAD_CONFIG_PAGE")
            return self.read_registers(REG_CFG, CFG_SIZE)

    def configure(
        self,
        preset: str = "8x8",
        *,
        period_ms: int = DEFAULT_PERIOD_MS,
        kilo_iterations: int | None = None,
        nr_peaks: int = 1,
        signal_strength: bool = False,
        noise_strength: bool = False,
        xtalk: bool = False,
        histograms: bool = False,
        ref_frame: bool = False,
        fields: dict[str, int] | None = None,
    ) -> Tmf8829Config:
        """Load `preset` and write the config page. Every field a preset does
        not reset is written (contract 14 §4), so the result never depends on
        an earlier config. `fields` sets any other page field by name.
        Raises `Tmf8829Error` when the sensor refuses the page (e.g.
        ERR_CONFIG_RESULT_SIZE: the result format is too big for the grid)
        or a written field does not read back. Not while streaming."""
        if preset not in PRESETS:
            raise ValueError(f"unknown preset {preset!r}; one of {', '.join(PRESETS)}")
        self._arm.require_idle()
        # The 3-bit field takes 0..7, the sensor reports at most 4.
        if not _is_int(nr_peaks) or not 1 <= nr_peaks <= 4:
            raise ValueError(f"nr_peaks {nr_peaks}, must be 1..4")
        want = {
            "period_ms": period_ms,
            "kilo_iterations": PRESET_KILO_ITERATIONS[preset]
            if kilo_iterations is None
            else kilo_iterations,
            "nr_peaks": nr_peaks,
            "signal_strength": int(signal_strength),
            "noise_strength": int(noise_strength),
            "xtalk": int(xtalk),
            "dump_histograms": int(histograms),
            "ref_spad_frame": int(ref_frame),
        }
        want.update(fields or {})
        # Every page field is checked — a name the page has, an integer that
        # fits its width — before the preset command goes to the sensor
        # (contract 07 §3): a refused argument leaves the sensor as it was.
        scratch = bytearray(CFG_SIZE)
        for name, value in want.items():
            if name not in CFG_FIELDS:
                raise ValueError(
                    f"unknown config field {name!r}; one of {', '.join(CFG_FIELDS)}"
                )
            if not _is_int(value):
                raise ValueError(f"{name}: must be an integer, got {value!r}")
            cfg_set(scratch, name, value)  # ValueError when out of range for its width
        with self._reg_lock:
            self._app_cmd(PRESETS[preset])
            page = bytearray(self.config_page())
            for k, v in want.items():
                cfg_set(page, k, v)
            self.write_registers(REG_CFG, bytes(page))
            self._app_cmd(CMD_WRITE_PAGE)
            back = self.config_page()
        stuck = {
            k: (v, cfg_get(back, k)) for k, v in want.items() if cfg_get(back, k) != v
        }
        if stuck:
            raise Tmf8829Error(
                "config fields did not stick: "
                + ", ".join(f"{k} wrote {w}, reads {r}" for k, (w, r) in stuck.items())
            )
        self._config = Tmf8829Config(
            preset=preset,
            page=back,
            histograms=histograms,
            ref_frame=ref_frame,
            fields=config_fields(back),
        )
        return self._config

    @property
    def config(self) -> Tmf8829Config | None:
        """The last `configure()` result (None after `init()`)."""
        return self._config

    # ── streaming ────────────────────────────────────────────────────────────

    @property
    def streaming(self) -> bool:
        return self._arm.active

    def start(self) -> None:
        """MEASURE with the bridge stream armed. Configures the default 8x8
        preset first when `configure()` was not called since `init()`. A no-op
        while streaming — on an open device: a lost one says so (contract 07 §3)."""
        self._check_open()
        if self._arm.active:
            return
        cfg = self._config or self.configure()
        int_mask = INT_RESULTS | (INT_HISTOGRAMS if cfg.histograms else 0)
        with self._reg_lock:
            self.write_register(
                REG_FIFOSTATUS, FIFO_ERROR_FLAGS
            )  # sticky under/overrun
            self.write_register(REG_INT_STATUS, 0xFF)
            self.write_register(REG_INT_ENAB, int_mask)
            self._reassembler.reset()
            self._builder.reset()
            self.measurements.parse_errors = 0
            self._arm.start(pack_start_stream(int_mask, SF_REF_FRAME if cfg.ref_frame else 0))
            try:
                self._app_cmd(CMD_MEASURE, accept=(STAT_OK, STAT_ACCEPTED))
            except Exception:
                self._arm.stop()
                raise

    def stop(self) -> None:
        """Sensor STOP, then the bridge stream (the frames already queued
        drain). The bad-header event a STOP raises mid histogram dump is
        expected and swallowed."""
        with self._reg_lock:
            self._stopping = True
            try:
                self._app_cmd(CMD_STOP)
                self.write_register(REG_INT_ENAB, 0)
                self.write_register(REG_INT_STATUS, 0xFF)
                self._arm.stop()
            finally:
                self._arm.active = False
                self._stopping = False

    def close(self) -> None:
        if self._arm.active and not self.closed:
            try:
                self.stop()
            except DepzError:
                pass
        super().close()

    @property
    def _record_stream(self) -> Stream[Tmf8829Measurement]:
        return self.measurements

    def _record_value(self, m: Tmf8829Measurement, *, layers: bool = False) -> dict[str, Any]:
        return record_value(m)

    @property
    def frames_dropped(self) -> int:
        """Frames the bridge dropped (frame_seq gaps) in this stream."""
        return self._reassembler.seq_gaps

    @property
    def frames_discarded(self) -> int:
        """Frames lost to reassembly (incomplete, out-of-order chunks) in this stream."""
        return self._reassembler.discarded

    @property
    def frame_parse_errors(self) -> int:
        """Reports of this class dropped because they failed to decode: a
        FRAME chunk or EVENT shorter than its header (contract 07 §2), or a
        frame the builder could not parse. Reset by `start()`."""
        return self.measurements.parse_errors + self._builder.parse_errors

    @property
    def clock_ratio_q16(self) -> int:
        """Current sensor-vs-board clock ratio in Q16 (contract 14 §5; nominal
        8 << 16 = 8 µs per SYSTICK), for diagnostics."""
        return self._builder.clock.ratio_q16

    def _handle_report(self, pkt: Packet) -> bool:
        if pkt.cmd == Tmf8829Rpt.FRAME:
            try:
                # short header: refused (ValueError) and counted — contract 07 §2
                chunk = FrameChunk.unpack(pkt.payload)
            except ValueError:
                self.measurements.parse_errors += 1
                return True
            raw = self._reassembler.feed(chunk)
            if raw is None:
                return True
            m = self._builder.add(raw.data, raw.timestamp_us, raw.edge_us)
            if m is not None:
                self.measurements._emit(m)
            return True
        if pkt.cmd == Tmf8829Rpt.EVENT:
            try:
                ev = Tmf8829Event.unpack(pkt.payload)
            except ValueError:
                self.measurements.parse_errors += 1
                return True
            if ev.code == EventCode.BAD_HEADER and self._stopping:
                return True  # STOP mid histogram dump: expected (contract 14 §2)
            self.events._emit(Tmf8829InterruptEvent(event=ev))
            return True
        return False
