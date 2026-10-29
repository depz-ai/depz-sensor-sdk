"""`.depzdata` datasets: decoded, multi-device, time-synced recording &
playback (contracts/09_DATASET_FORMAT.md).

`SessionRecorder` records any mix of connected sensors onto one shared host
timeline (it runs `sync_time` on every device first). `DatasetReader` merges
records by host time and can pace them for playback.
"""

from __future__ import annotations

import base64
import gzip
import heapq
import io
import json
import math
import sys
import threading
import time
from dataclasses import dataclass, field
from enum import Enum
from pathlib import Path
from typing import IO, TYPE_CHECKING, Any, Callable, Iterator

from ._jsonl import is_blank, json_object
from .device import DeviceBase, call_guarded, host_now_us
from .errors import InvalidStateError
from .sr04 import Sr04, Sr04Measurement

if TYPE_CHECKING:  # the sensor modules are imported lazily below (numpy is optional)
    from .bno055 import Bno055Sample
    from .bno086.reports import Report
    from .tmf8829 import Tmf8829Measurement
    from .vl53l4 import Vl53l4Measurement
    from .vl53l8 import Vl53l8, Vl53l8Frame
    from .vl53lx import Vl53lx, Vl53lxMeasurement

SCHEMA = "depz.dataset/2"
#: The schema id this SDK writes (same value as `SCHEMA`; the TS SDK's name).
DATASET_SCHEMA = SCHEMA

# Report attribute → record key, for the scalar fields contract 09 `bno086` keeps.
_BNO086_RECORD_KEYS = (
    ("i", "i"), ("j", "j"), ("k", "k"), ("real", "real"),
    ("x", "x"), ("y", "y"), ("z", "z"),
    ("accuracy_rad", "accuracy_rad"), ("accuracy", "accuracy"),
    ("steps", "steps"), ("latency_us", "latency_us"), ("flags", "flags"),
    ("double_tap", "double_tap"), ("motion", "motion"),
    ("classification", "classification"), ("most_likely_state", "most_likely_state"),
    ("value", "value"),
    ("intent", "intent"), ("request", "request"), ("flushed_sensor", "flushed_sensor"),
)
# The raw ADC streams (0x14..0x16): only these keep their integer counts.
_BNO086_RAW_IDS = frozenset((0x14, 0x15, 0x16))


def bno086_record_value(r: Any) -> dict[str, Any]:
    """One BNO086 report as its contract 09 `bno086` record value.

    snake_case, only the fields the report carries: quaternion / vector
    components, accuracy, uncalibrated bias, gyro-integrated angular velocity,
    detector and classifier outputs, and the integer counts of the raw ADC
    streams. Matches the TS SDK's `bno086RecordValue` key for key.
    """
    v: dict[str, Any] = {"type": type(r).__name__, "sensor_id": r.sensor_id}
    for key, out in _BNO086_RECORD_KEYS:
        val = getattr(r, key, None)
        if isinstance(val, (bool, int, float)):
            v[out] = val
    for key, out in (("bias", "bias"), ("angular_velocity", "angular_velocity"),
                     ("confidences", "confidences")):
        val = getattr(r, key, None)
        if val is not None:
            v[out] = list(val)
    if type(r).__name__ == "GenericEvent":
        v["value_raw"] = r.value_raw
    if r.sensor_id in _BNO086_RAW_IDS:
        for key in ("x_raw", "y_raw", "z_raw", "temperature_raw", "sensor_timestamp_us"):
            val = getattr(r, key, None)
            if val is not None:
                v[key] = val
    if getattr(r, "timestamp_us", None) is not None:
        v["timestamp_us"] = r.timestamp_us
    return v


def _open_text(path: str | Path, mode: str) -> IO[str]:
    p = Path(path)
    if p.suffix == ".gz":
        # What gzip.open(mode + "t") does internally, spelled out for the type.
        return io.TextIOWrapper(gzip.GzipFile(p, mode + "b"), encoding="utf-8", newline="\n")
    # newline="\n": lines end at LF only, as in every SDK (a lone CR is not a line end)
    return open(p, mode, encoding="utf-8", newline="\n")


@dataclass(frozen=True)
class DatasetRecord:
    device_id: str
    t_host_us: int
    kind: str
    value: dict[str, Any]


def _record(path: Path, lineno: int, line: str) -> DatasetRecord:
    """One record line (contract 09): `d` and `k` strings, `t` an integer
    (host µs), `v` an object."""
    ev = json_object(path, lineno, line)
    d, t, k, v = ev.get("d"), ev.get("t"), ev.get("k"), ev.get("v")
    if not isinstance(d, str):
        raise ValueError(f"{path}:{lineno}: 'd' must be a device id string, got {d!r}")
    if not isinstance(t, int) or isinstance(t, bool):
        raise ValueError(f"{path}:{lineno}: 't' must be an integer (host µs), got {t!r}")
    if not isinstance(k, str):
        raise ValueError(f"{path}:{lineno}: 'k' must be a record kind string, got {k!r}")
    if not isinstance(v, dict):
        raise ValueError(f"{path}:{lineno}: 'v' must be an object, got {v!r}")
    return DatasetRecord(d, t, k, v)


@dataclass
class _DeviceEntry:
    device: DeviceBase
    meta: dict[str, Any]
    unsubscribes: list[Callable[[], None]] = field(default_factory=list)


class DatasetWriter:
    """Low-level thread-safe record writer. Prefer `SessionRecorder`."""

    def __init__(self, path: str | Path, devices: dict[str, dict[str, Any]], *, note: str = ""):
        self._file = _open_text(path, "w")
        header: dict[str, Any] = {
            "schema": SCHEMA,
            "created_utc": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
            "devices": devices,
        }
        if note:
            header["note"] = note
        self._file.write(json.dumps(header, ensure_ascii=False) + "\n")
        self._lock = threading.Lock()
        self.records_written = 0

    def write(self, device_id: str, t_host_us: int, kind: str, value: dict[str, Any]) -> None:
        line = json.dumps(
            {"d": device_id, "t": t_host_us, "k": kind, "v": value}, ensure_ascii=False
        )
        with self._lock:
            if self._file.closed:
                raise InvalidStateError("DatasetWriter is closed")
            self._file.write(line + "\n")
            self.records_written += 1

    def close(self) -> None:
        with self._lock:
            self._file.flush()
            self._file.close()


class SessionRecorder:
    """Record decoded data from several devices onto one host timeline.

    Usage::

        with SessionRecorder("run.depzdata") as rec:
            rec.add(sr04)      # runs sync_time, hooks the measurement stream
            rec.add(vl53l8)    # hooks the frame stream
            sr04.start(); vl53l8.start_ranging()
            time.sleep(10)
        # exit unhooks and closes the file

    Devices must already be open; the recorder never reconfigures them —
    start/stop streaming yourself.
    """

    def __init__(
        self,
        path: str | Path,
        *,
        note: str = "",
        vl53l8_layers: bool = False,
        temperature_hz: float | None = 1.0,
    ):
        self._path = path
        self._note = note
        self._vl53l8_layers = vl53l8_layers  # include signal/ambient/sigma/reflectance
        # ~temperature_hz `temperature` records per device (contract 09):
        # the MCU thermometer, plus the sensor die where frames carry one.
        # None disables the sampler thread.
        self._temperature_hz = temperature_hz
        self._entries: dict[str, _DeviceEntry] = {}
        self._writer: DatasetWriter | None = None
        self._temp_thread: threading.Thread | None = None
        self._temp_stop = threading.Event()
        self._last_silicon: dict[str, float] = {}

    def add(self, device: DeviceBase, *, device_id: str | None = None, sync_samples: int = 5) -> str:
        """Register a device (before the first record is written). Runs
        `sync_time` so its timestamps land on the shared host timeline."""
        if self._writer is not None:
            raise InvalidStateError("add() all devices before recording starts")
        did = device_id or f"d{len(self._entries)}"
        sync = device.sync_time(samples=sync_samples)
        meta: dict[str, Any] = {
            "time_sync": {"offset_us": sync.offset_us, "rtt_us": sync.rtt_us},
        }
        try:
            meta["serial"] = device.get_serial_number()
            meta["software_name"] = device.get_software_name()
        except Exception:
            pass
        from .protocol.identity import parse_software_name

        ident = parse_software_name(meta.get("software_name", ""))
        meta["sensor_type"] = ident.sensor_type.value if ident.sensor_type else "unknown"
        self._entries[did] = _DeviceEntry(device, meta)
        return did

    def start(self) -> None:
        if self._writer is not None:
            return
        self._writer = DatasetWriter(
            self._path,
            {did: e.meta for did, e in self._entries.items()},
            note=self._note,
        )
        for did, entry in self._entries.items():
            self._hook(did, entry)
        if self._temperature_hz:
            self._temp_stop.clear()
            self._temp_thread = threading.Thread(
                target=self._temperature_loop, name="depz-rec-temp", daemon=True
            )
            self._temp_thread.start()

    def _temperature_loop(self) -> None:
        """~temperature_hz per device: MCU temperature (contract 02) and the
        last sensor-die reading seen in a frame, as `temperature` records."""
        period = 1.0 / float(self._temperature_hz or 1.0)
        while not self._temp_stop.wait(period):
            writer = self._writer
            if writer is None:
                return
            for did, entry in self._entries.items():
                silicon = self._last_silicon.get(did)
                if silicon is not None:
                    writer.write(did, host_now_us(), "temperature",
                                 {"celsius": silicon, "source": "sensor"})
                try:
                    celsius = entry.device.read_mcu_temperature()
                except Exception:
                    continue
                writer.write(did, host_now_us(), "temperature",
                             {"celsius": celsius, "source": "mcu"})

    def _hook(self, did: str, entry: _DeviceEntry) -> None:
        dev = entry.device
        offset = entry.meta["time_sync"]["offset_us"]
        writer = self._writer
        assert writer is not None

        if isinstance(dev, Sr04):

            def on_measure(m: Sr04Measurement, _did: str = did, _off: int = offset) -> None:
                writer.write(_did, m.timestamp_us - _off, "sr04",
                             {"echo_us": m.echo_time_us, "source": m.source})

            entry.unsubscribes.append(dev.on_measurement(on_measure))
            return

        vl53l8_cls: type[Vl53l8] | None
        try:
            from .vl53l8 import Vl53l8 as vl53l8_cls
        except ImportError:
            vl53l8_cls = None
        if vl53l8_cls is not None and isinstance(dev, vl53l8_cls):
            layers = self._vl53l8_layers

            def on_frame(f: Vl53l8Frame, _did: str = did, _off: int = offset, _dev: Vl53l8 = dev) -> None:
                t = f.timestamp_us - _off
                self._last_silicon[_did] = f.silicon_temp_degc
                v: dict[str, Any] = {
                    "resolution": f.resolution,
                    "silicon_temp_degc": f.silicon_temp_degc,
                    "distance_mm": f.distance_mm[: f.resolution].tolist(),
                    "target_status": f.target_status[: f.resolution].tolist(),
                    "nb_target_detected": f.nb_target_detected[: f.resolution].tolist(),
                }
                if layers:
                    v["signal_per_spad"] = f.signal_per_spad[: f.resolution].tolist()
                    v["ambient_per_spad"] = f.ambient_per_spad[: f.resolution].tolist()
                    v["range_sigma_mm"] = f.range_sigma_mm[: f.resolution].tolist()
                    v["reflectance"] = f.reflectance[: f.resolution].tolist()
                if f.motion is not None:
                    v["motion"] = f.motion
                writer.write(_did, t, "vl53l8", v)
                # CH: persist the raw CNH block riding this frame, with the
                # armed config's decode parameters (contract 09 `vl53l8_cnh`).
                cfg = getattr(_dev, "cnh_config", None)
                if f.cnh_raw and cfg is not None:
                    writer.write(_did, t, "vl53l8_cnh", {
                        "nb_aggregates": cfg.nb_of_aggregates,
                        "feature_length": cfg.feature_length,
                        "sum_span": cfg.sum_span,
                        "start_bin": cfg.ref_bin_offset // 2048,
                        "cnh_raw_b64": base64.b64encode(f.cnh_raw).decode("ascii"),
                    })

            entry.unsubscribes.append(dev.on_frame(on_frame))
            return

        from .vl53l4 import Vl53l4cd, Vl53l4Measurement

        if isinstance(dev, Vl53l4cd):

            def on_result(m: Vl53l4Measurement, _did: str = did, _off: int = offset) -> None:
                writer.write(_did, m.timestamp_us - _off, "vl53l4", {
                    "range_status": m.range_status,
                    "distance_mm": m.distance_mm,
                    "sigma_mm": m.sigma_mm,
                    "signal_rate_kcps": m.signal_rate_kcps,
                    "ambient_rate_kcps": m.ambient_rate_kcps,
                    "number_of_spad": m.number_of_spad,
                    "stream_count": m.stream_count,
                })

            entry.unsubscribes.append(dev.on_measurement(on_result))
            return

        from .vl53lx import Vl53lx, Vl53lxMeasurement

        if isinstance(dev, Vl53lx):

            def on_lx(m: Vl53lxMeasurement, _did: str = did, _off: int = offset, _dev: Vl53lx = dev) -> None:
                v: dict[str, Any] = {
                    "product": _dev.product,
                    "driver": _dev.driver_kind,
                    "status": m.status,
                    "distance_mm": m.distance_mm,
                    "sigma_mm": m.sigma_mm,
                    "signal_kcps": m.signal_kcps,
                    "ambient_kcps": m.ambient_kcps,
                    "spads": m.spads,
                }
                if m.targets:
                    v["targets"] = [
                        {"distance_mm": t.distance_mm, "status": t.status,
                         "signal_kcps": t.signal_kcps}
                        for t in m.targets
                    ]
                writer.write(_did, m.timestamp_us - _off, "vl53lx", v)

            entry.unsubscribes.append(dev.on_measurement(on_lx))
            return

        from .bno055 import Bno055, Bno055Sample

        if isinstance(dev, Bno055):

            def on_sample(s: Bno055Sample, _did: str = did, _off: int = offset) -> None:
                v: dict[str, Any] = {"unit_sel": s.units.pack()}
                for key in ("quaternion", "euler", "accel", "gyro", "mag",
                            "linear_accel", "gravity"):
                    val = getattr(s, key)
                    if val is not None:
                        v[key] = list(val)
                if s.temperature is not None:
                    v["temperature"] = s.temperature
                if s.calibration is not None:
                    c = s.calibration
                    v["calib"] = [c.system, c.gyro, c.accel, c.mag]
                writer.write(_did, s.timestamp_us - _off, "bno055", v)

            entry.unsubscribes.append(dev.on_sample(on_sample))
            return

        from .tmf8829 import Tmf8829, Tmf8829Measurement
        from .tmf8829.builder import record_value as tmf8829_record_value

        if isinstance(dev, Tmf8829):

            def on_measurement(m: Tmf8829Measurement, _did: str = did, _off: int = offset) -> None:
                writer.write(_did, m.timestamp_us - _off, "tmf8829", tmf8829_record_value(m))

            entry.unsubscribes.append(dev.on_measurement(on_measurement))
            return

        from .bno086 import Bno086

        if isinstance(dev, Bno086):

            def on_report(r: Report, _did: str = did, _off: int = offset) -> None:
                # Contract 09 `bno086`: every enabled sensor's report, snake_case,
                # full quaternion / vector components, at its measurement time
                # (0xFB/0xFA + delay) like every other sensor. Reports of
                # different sensors in one cargo carry out-of-order device times,
                # so `t` may step back within a device; readers sort.
                writer.write(_did, r.timestamp_us - _off, "bno086", bno086_record_value(r))

            entry.unsubscribes.append(dev.on_report(on_report))
            return
        # Unknown device class: nothing to hook (future sensors extend here).

    def stop(self) -> None:
        self._temp_stop.set()
        if self._temp_thread is not None:
            self._temp_thread.join(timeout=3.0)
            self._temp_thread = None
        for entry in self._entries.values():
            for unsub in entry.unsubscribes:
                try:
                    unsub()
                except ValueError:
                    pass
            entry.unsubscribes.clear()
        if self._writer is not None:
            self._writer.close()
            self._writer = None

    @property
    def records_written(self) -> int:
        return self._writer.records_written if self._writer else 0

    def __enter__(self) -> "SessionRecorder":
        return self

    def __exit__(self, *exc: object) -> None:
        if self._writer is None and self._entries:
            pass  # never started
        else:
            self.stop()


class DatasetReader:
    """Read a `.depzdata` file; iterate records merged by host time."""

    def __init__(self, path: str | Path):
        self._path = Path(path)
        with _open_text(self._path, "r") as f:
            self.header: dict[str, Any] = json_object(self._path, 1, f.readline())
        if not str(self.header.get("schema", "")).startswith("depz.dataset/"):
            raise ValueError(f"{path}: not a depz.dataset file")

        self._records: list[DatasetRecord] | None = None

    @property
    def devices(self) -> dict[str, dict[str, Any]]:
        devices: dict[str, dict[str, Any]] = self.header.get("devices", {})
        return devices

    @property
    def records(self) -> list[DatasetRecord]:
        """Every record, merged by host time (the order of iteration). Loaded
        on first access and cached — the whole file is held in memory."""
        if self._records is None:
            self._records = list(self)
        return self._records

    @property
    def duration_us(self) -> int:
        """Host-time span from the first record to the last (0 when the
        dataset has fewer than two records)."""
        recs = self.records
        if not recs:
            return 0
        return recs[-1].t_host_us - recs[0].t_host_us

    def _raw_iter(self) -> Iterator[DatasetRecord]:
        with _open_text(self._path, "r") as f:
            f.readline()  # header
            for lineno, line in enumerate(f, 2):
                if is_blank(line):
                    continue
                yield _record(self._path, lineno, line)

    def __iter__(self) -> Iterator[DatasetRecord]:
        """Records merged by `t`. Per-device order is only roughly monotonic
        (BNO086 cargos, arrival-time `temperature` records), so each stream is
        stable-sorted before the k-way merge (contract 09)."""
        streams: dict[str, list[DatasetRecord]] = {}
        for rec in self._raw_iter():
            streams.setdefault(rec.device_id, []).append(rec)
        for recs in streams.values():
            recs.sort(key=lambda r: r.t_host_us)
        yield from heapq.merge(*streams.values(), key=lambda r: r.t_host_us)

    def play(
        self,
        callback: Callable[[DatasetRecord], None],
        *,
        speed: float = 1.0,
        start_t_us: int | None = None,
        stop: threading.Event | None = None,
    ) -> None:
        """Deliver records paced by their timestamps (speed=2.0 → twice as
        fast; speed=0 → as fast as possible), blocking until the end or
        `stop`. For pause / seek / speed changes use `DatasetPlayer`."""
        first_t: int | None = None
        wall0 = time.monotonic()
        for rec in self:
            if start_t_us is not None and rec.t_host_us < start_t_us:
                continue
            if stop is not None and stop.is_set():
                return
            if speed > 0:
                if first_t is None:
                    first_t = rec.t_host_us
                due = wall0 + (rec.t_host_us - first_t) / 1e6 / speed
                delay = due - time.monotonic()
                if delay > 0:
                    if stop is not None:
                        if stop.wait(delay):
                            return
                    else:
                        time.sleep(delay)
            callback(rec)


class PlayerState(str, Enum):
    """`DatasetPlayer` state (contract 09 §Player). A `str` enum, so
    `player.state == "playing"` works too."""

    IDLE = "idle"        # never played
    PLAYING = "playing"  # delivering records paced by the clock
    PAUSED = "paused"    # stopped mid-timeline; play() resumes
    DONE = "done"        # past the last record; play() restarts from the top


class _Delivered:
    """_step_locked(): one record went out, call again."""

    __slots__ = ()


_DELIVERED = _Delivered()


class DatasetPlayer:
    """Timeline player over a dataset: paced delivery with play / pause /
    seek / speed — the engine behind a viewer's playback mode
    (contract 09 §Player; the TS SDK's `DatasetPlayer`).

    Usage::

        player = DatasetPlayer(DatasetReader("run.depzdata"), speed=2.0)
        player.on_record(lambda r: print(r.device_id, r.kind, r.t_host_us))
        player.play()
        player.wait()            # until done (or paused)

    Timing: playback runs on a media clock anchored to the host clock.
    Record ``i`` is due at ``anchor_wall + (t_i - anchor_t) / speed``; the
    anchor is (now, t of the last delivered record — or of the first record
    when none was) at play / resume / seek, so the first record goes out at
    once and every later one after its gap to the previous one. Pacing is
    against the clock, not cumulative: a slow consumer gets overdue records
    back to back and playback does not drift. ``speed=0`` delivers as fast as
    possible.

    Threads: by default records and the ``done`` state are delivered on a
    player thread (started on the first `play`); state changes made by a
    call (`play`, `pause`, `seek_us`) are reported on the calling thread
    before it returns. Callbacks run under the player's lock, so they see a
    consistent player, may call its methods, and once `pause` returns no
    record callback is running or will start. Like device stream callbacks
    (contract 07) they must not block — never wait in one for another thread
    that uses this player. A callback that raises is logged (logger
    ``depz_sensor_sdk``), counted in `callback_errors` and dropped: the other
    callbacks still get the record and playback goes on (contract 09).

    ``threaded=False`` starts no thread: the owner calls `tick` from its own
    loop (a GUI timer, or a test with a fake ``clock``).
    """

    def __init__(
        self,
        reader: "DatasetReader | str | Path",
        *,
        speed: float = 1.0,
        clock: Callable[[], float] = time.monotonic,
        threaded: bool = True,
    ):
        """`reader` is a `DatasetReader` or a path to a `.depzdata` file.
        `clock` returns seconds (monotonic); `threaded=False` → drive with
        `tick`."""
        self.reader = reader if isinstance(reader, DatasetReader) else DatasetReader(reader)
        self._recs = self.reader.records
        self._clock = clock
        self._threaded = threaded
        self._speed = self._check_speed(speed)
        self._idx = 0  # next record to deliver
        self._state = PlayerState.IDLE
        self._anchor_wall = 0.0
        self._anchor_t: float = 0
        self._cond = threading.Condition(threading.RLock())
        self._record_cbs: list[Callable[[DatasetRecord], None]] = []
        self._state_cbs: list[Callable[[PlayerState], None]] = []
        self._thread: threading.Thread | None = None
        self._closed = False
        self._callback_errors = 0

    # ── read-only views ──────────────────────────────────────────────────────

    @property
    def state(self) -> PlayerState:
        return self._state

    @property
    def duration_us(self) -> int:
        """Span of the timeline: last record's `t` minus the first's."""
        return self.reader.duration_us

    @property
    def position_us(self) -> int:
        """Current position, µs from the first record: the `t` of the last
        delivered record (after a seek: of the record just before the
        target). 0 before anything was delivered."""
        with self._cond:
            if not self._recs or self._idx == 0:
                return 0
            return self._recs[min(self._idx, len(self._recs)) - 1].t_host_us - self._recs[0].t_host_us

    @property
    def callback_errors(self) -> int:
        """Record / state callbacks that raised and were dropped (contract 09)."""
        return self._callback_errors

    @property
    def speed(self) -> float:
        return self._speed

    @speed.setter
    def speed(self, value: float) -> None:
        """Change the playback speed (≥ 0; 0 = as fast as possible). While
        playing it applies at once: the remaining part of the current wait
        is rescaled, records already delivered stay delivered."""
        value = self._check_speed(value)
        with self._cond:
            if self._state is PlayerState.PLAYING:
                now = self._clock()
                self._anchor_t = self._media_now(now)
                self._anchor_wall = now
            self._speed = value
            self._cond.notify_all()

    # ── subscriptions ────────────────────────────────────────────────────────

    def on_record(self, cb: Callable[[DatasetRecord], None]) -> Callable[[], None]:
        """Subscribe to delivered records. Returns an unsubscribe function
        (idempotent)."""
        with self._cond:
            self._record_cbs.append(cb)
        return lambda: self._unsubscribe(self._record_cbs, cb)

    def on_state(self, cb: Callable[[PlayerState], None]) -> Callable[[], None]:
        """Subscribe to state changes (called with the new state on every
        transition). Returns an unsubscribe function (idempotent)."""
        with self._cond:
            self._state_cbs.append(cb)
        return lambda: self._unsubscribe(self._state_cbs, cb)

    def _unsubscribe(self, cbs: list[Any], cb: Any) -> None:
        with self._cond:
            if cb in cbs:
                cbs.remove(cb)

    # ── control ──────────────────────────────────────────────────────────────

    def play(self) -> None:
        """Start, or resume after `pause`. No-op while playing. From `done`
        (or after a seek past the end) playback restarts from the first
        record. An empty dataset goes straight to `done`."""
        with self._cond:
            self._check_open()
            if self._state is PlayerState.PLAYING:
                return
            if self._idx >= len(self._recs):
                self._idx = 0
            self._anchor(self._clock())
            self._set_state(PlayerState.PLAYING)
            if not self._recs:
                self._set_state(PlayerState.DONE)
                return
            self._ensure_thread()
            self._cond.notify_all()

    def pause(self) -> None:
        """Stop delivering; `play` resumes from the next record, which then
        waits its full gap again. No-op unless playing."""
        with self._cond:
            if self._state is PlayerState.PLAYING:
                self._set_state(PlayerState.PAUSED)
                self._cond.notify_all()

    def seek_us(self, offset_us: float) -> None:
        """Move to `offset_us` µs from the first record: the next record
        delivered is the first one with ``t >= t_first + offset_us`` (a
        negative offset means the start). Records between the old and the
        new position are skipped, not delivered. The state does not change,
        except that a seek past the end while playing ends playback
        (`done`); while playing, delivery continues from the new position."""
        if not isinstance(offset_us, (int, float)) or math.isnan(offset_us):
            raise ValueError(f"seek_us: offset must be a number, got {offset_us!r}")
        with self._cond:
            self._check_open()
            if not self._recs:
                return
            target = self._recs[0].t_host_us + offset_us
            lo, hi = 0, len(self._recs)
            while lo < hi:
                mid = (lo + hi) // 2
                if self._recs[mid].t_host_us < target:
                    lo = mid + 1
                else:
                    hi = mid
            self._idx = lo
            if self._state is PlayerState.PLAYING:
                if self._idx >= len(self._recs):
                    self._set_state(PlayerState.DONE)
                else:
                    self._anchor(self._clock())
                self._cond.notify_all()

    def tick(self) -> float | None:
        """Deliver every record that is due now (on the calling thread).
        Returns the seconds until the next one is due, or None when not
        playing. For ``threaded=False``; harmless with a player thread."""
        with self._cond:
            while True:
                r = self._step_locked()
                if not isinstance(r, _Delivered):
                    return r

    def wait(self, timeout: float | None = None) -> bool:
        """Block until the player is not playing (done, or paused by someone
        else). True if so, False on timeout. Needs the player thread."""
        if not self._threaded:
            raise InvalidStateError("wait() needs threaded=True; drive tick() instead")
        with self._cond:
            return self._cond.wait_for(lambda: self._state is not PlayerState.PLAYING, timeout)

    def close(self) -> None:
        """Stop playback and the player thread. Idempotent."""
        with self._cond:
            if self._closed:
                return
            if self._state is PlayerState.PLAYING:
                self._set_state(PlayerState.PAUSED)
            self._closed = True
            self._cond.notify_all()
        t = self._thread
        if t is not None and t is not threading.current_thread():
            t.join(timeout=2.0)
        self._thread = None

    def __enter__(self) -> "DatasetPlayer":
        return self

    def __exit__(self, *exc: object) -> None:
        self.close()

    # ── internals (all under self._cond) ─────────────────────────────────────

    @staticmethod
    def _check_speed(value: float) -> float:
        if not isinstance(value, (int, float)) or not math.isfinite(value) or value < 0:
            raise ValueError(f"speed must be a finite number >= 0, got {value!r}")
        return float(value)

    def _check_open(self) -> None:
        if self._closed:
            raise InvalidStateError("DatasetPlayer is closed")

    def _anchor(self, now: float) -> None:
        """Media clock restarts at `now` from the last delivered record's `t`
        (the first record's `t` when none was), so the next record waits its
        own gap and the first one none."""
        self._anchor_wall = now
        self._anchor_t = self._recs[max(self._idx - 1, 0)].t_host_us if self._recs else 0

    def _media_now(self, now: float) -> float:
        """Media time (µs, host timeline) at `now` while playing."""
        if self._speed <= 0:
            return self._recs[max(self._idx - 1, 0)].t_host_us
        return self._anchor_t + (now - self._anchor_wall) * 1e6 * self._speed

    def _set_state(self, state: PlayerState) -> None:
        self._state = state
        self._cond.notify_all()
        self._deliver(self._state_cbs, state)

    def _deliver(self, cbs: list[Any], item: Any) -> None:
        """Hand `item` to every callback; one that raises is counted, logged
        and dropped — the rest still get it, playback goes on (contract 09)."""
        for cb in list(cbs):
            if not call_guarded(cb, item):
                self._callback_errors += 1

    def _step_locked(self) -> float | None | _Delivered:
        """Deliver the next record if it is due: `_DELIVERED`, else the
        seconds to wait (float), or None when not playing."""
        if self._state is not PlayerState.PLAYING:
            return None
        if self._idx >= len(self._recs):
            self._set_state(PlayerState.DONE)
            return None
        rec = self._recs[self._idx]
        if self._speed > 0:
            now = self._clock()
            due = self._anchor_wall + (rec.t_host_us - self._anchor_t) / 1e6 / self._speed
            if due > now:
                return due - now
        self._idx += 1
        self._deliver(self._record_cbs, rec)
        if self._state is PlayerState.PLAYING and self._idx >= len(self._recs):
            self._set_state(PlayerState.DONE)
        return _DELIVERED

    def _ensure_thread(self) -> None:
        if not self._threaded or (self._thread is not None and self._thread.is_alive()):
            return
        self._thread = threading.Thread(target=self._run, name="depz-dataset-player", daemon=True)
        self._thread.start()

    def _run(self) -> None:
        cond = self._cond
        with cond:
            while not self._closed:
                try:
                    r = self._step_locked()
                except Exception:
                    threading.excepthook(threading.ExceptHookArgs(
                        (*sys.exc_info(), threading.current_thread())))
                    continue
                if isinstance(r, _Delivered):
                    # let control calls (pause/seek from other threads) in
                    # between records, even at speed=0
                    cond.release()
                    try:
                        time.sleep(0)
                    finally:
                        cond.acquire()
                    continue
                cond.wait(r)
