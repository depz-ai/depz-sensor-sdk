"""`.depzdata` datasets: decoded, multi-device, time-synced recording &
playback (contracts/09_DATASET_FORMAT.md).

`SessionRecorder` records any mix of connected sensors onto one shared host
timeline (it runs `sync_time` on every device first). `DatasetReader` merges
records by host time and can pace them for playback.
"""

from __future__ import annotations

import gzip
import heapq
import io
import logging
import math
import threading
import time
import zlib
from contextlib import contextmanager
from dataclasses import dataclass, field
from enum import Enum
from pathlib import Path
from typing import IO, Any, Callable, Iterator

from ._jsonl import dump_line, is_blank, json_object
from ._waits import wait_limit
from .device import DepzDevice, host_now_us
from .stream import call_guarded
from .errors import InvalidStateError
from .protocol.identity import parse_software_name

_log = logging.getLogger("depz_sensor_sdk")

SCHEMA = "depz.dataset/2"

# Longest single wait of a player, seconds: a tiny speed stretches a gap past
# what a lock timeout takes; the player re-evaluates after every wake anyway.
_MAX_WAIT_S = 60.0

def _open_text(path: str | Path, mode: str) -> IO[str]:
    p = Path(path)
    if p.suffix == ".gz":
        # What gzip.open(mode + "t") does internally, spelled out for the type.
        return io.TextIOWrapper(gzip.GzipFile(p, mode + "b"), encoding="utf-8", newline="\n")
    # newline="\n": lines end at LF only, as in every SDK (a lone CR is not a line end)
    return open(p, mode, encoding="utf-8", newline="\n")


@contextmanager
def _refusing_damaged_gzip(path: Path) -> Iterator[None]:
    """A cut or damaged `.depzdata.gz` read inside this is the bad-argument
    error of a damaged file (contract 09), not what the gzip module raises."""
    try:
        yield
    except (EOFError, zlib.error, gzip.BadGzipFile) as bad:
        raise ValueError(f"{path}: cut or damaged gzip: {bad}") from None


@dataclass(frozen=True, kw_only=True)
class DatasetRecord:
    device_id: str
    t_host_us: int
    kind: str
    value: dict[str, Any]


def _record(path: Path, lineno: int, line: str) -> DatasetRecord:
    """One record line (contract 09): `d` and `k` strings, `t` an integer
    within int64 (host µs), `v` an object."""
    ev = json_object(path, lineno, line)
    d, t, k, v = ev.get("d"), ev.get("t"), ev.get("k"), ev.get("v")
    if not isinstance(d, str):
        raise ValueError(f"{path}:{lineno}: 'd' must be a device id string, got {d!r}")
    if not isinstance(t, int) or isinstance(t, bool):
        raise ValueError(f"{path}:{lineno}: 't' must be an integer (host µs), got {t!r}")
    if not -(2**63) <= t < 2**63:  # as the C, C#, Java and Rust readers hold it
        raise ValueError(f"{path}:{lineno}: 't' must be an integer within int64, got {t}")
    if not isinstance(k, str):
        raise ValueError(f"{path}:{lineno}: 'k' must be a record kind string, got {k!r}")
    if not isinstance(v, dict):
        raise ValueError(f"{path}:{lineno}: 'v' must be an object, got {v!r}")
    return DatasetRecord(device_id=d, t_host_us=t, kind=k, value=v)


@dataclass
class _DeviceEntry:
    device: DepzDevice
    meta: dict[str, Any]
    unsubscribes: list[Callable[[], None]] = field(default_factory=list)


class DatasetWriter:
    """Low-level thread-safe record writer. Prefer `SessionRecorder`."""

    def __init__(self, path: str | Path, devices: dict[str, dict[str, Any]], *, note: str = ""):
        header: dict[str, Any] = {
            "schema": SCHEMA,
            "created_utc": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
            "devices": devices,
        }
        if note:
            header["note"] = note
        line = dump_line(header)  # before the file is touched: refused, it stays as it was
        self._file = _open_text(path, "w")
        self._file.write(line + "\n")
        self._lock = threading.Lock()
        self.records_written = 0

    def write(self, device_id: str, t_host_us: int, kind: str, value: dict[str, Any]) -> None:
        """One record. A device id or kind that is no string, a time that is no
        integer or falls outside int64, a value that is no object or holds NaN /
        Infinity or nests past the line rules, a lone surrogate in a string is
        refused before anything is written (`ValueError`): no reader would take
        the line, or some readers would not (contract 09)."""
        if (
            not isinstance(device_id, str)
            or not isinstance(kind, str)
            or not isinstance(value, dict)
            or not isinstance(t_host_us, int)
            or isinstance(t_host_us, bool)
        ):
            raise ValueError("a dataset record needs a device id, an integer time, a kind and an object value")
        if not -(2**63) <= t_host_us < 2**63:  # the SDKs with an int64 time refuse the line
            raise ValueError(f"a dataset record's time must fit int64, got {t_host_us}")
        line = dump_line({"d": device_id, "t": t_host_us, "k": kind, "v": value})
        with self._lock:
            if self._file.closed:
                raise InvalidStateError("DatasetWriter is closed")
            self._file.write(line + "\n")
            self.records_written += 1

    def close(self) -> None:
        """Flush and close the file. Idempotent."""
        with self._lock:
            if self._file.closed:
                return
            self._file.close()  # flushes; the file closes even when that raises


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
        self._records_written = 0
        self._temp_thread: threading.Thread | None = None
        self._temp_stop = threading.Event()
        self._last_silicon: dict[str, float] = {}

    def add(self, device: DepzDevice, *, device_id: str | None = None, sync_samples: int = 5) -> str:
        """Register a device (before the first record is written). Runs
        `sync_time` so its timestamps land on the shared host timeline. An
        id given twice is the later device's."""
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
        """One subscription per device: each item of its record stream becomes
        a record of the class's kind, its side records ride along, and a die
        temperature the item carries feeds the `temperature` sampler. A class
        without a record stream (a bare `DepzDevice`) records nothing."""
        dev = entry.device
        stream, kind = dev._record_stream, dev._record_kind
        if stream is None or kind is None:
            return
        offset: int = entry.meta["time_sync"]["offset_us"]
        writer = self._writer
        assert writer is not None
        layers = self._vl53l8_layers

        def on_item(item: Any) -> None:
            t = item.timestamp_us - offset
            silicon = getattr(item, "silicon_temp_degc", None)
            if silicon is not None:
                self._last_silicon[did] = silicon
            writer.write(did, t, kind, dev._record_value(item, layers=layers))
            for extra_kind, value in dev._record_extras(item):
                writer.write(did, t, extra_kind, value)

        entry.unsubscribes.append(stream.on(on_item))

    def stop(self) -> None:
        self._temp_stop.set()
        if self._temp_thread is not None:
            self._temp_thread.join(timeout=3.0)
            self._temp_thread = None
        for entry in self._entries.values():
            for unsub in entry.unsubscribes:
                unsub()
            entry.unsubscribes.clear()
        writer = self._writer
        if writer is not None:
            # Counted once the file is closed: a callback the stream had already
            # begun when its hook went may still write until then, and that
            # record is in the file.
            try:
                writer.close()  # a file that cannot be flushed says so, with the recorder stopped
            finally:
                self._records_written = writer.records_written
                self._writer = None

    @property
    def records_written(self) -> int:
        """Records in the file being written, or in the last one after stop()."""
        return self._writer.records_written if self._writer else self._records_written

    def __enter__(self) -> "SessionRecorder":
        return self

    def __exit__(self, *exc: object) -> None:
        self.stop()


class DatasetReader:
    """Read a `.depzdata` file; iterate records merged by host time."""

    def __init__(self, path: str | Path):
        self._path = Path(path)
        with _refusing_damaged_gzip(self._path), _open_text(self._path, "r") as f:
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
        with _refusing_damaged_gzip(self._path), _open_text(self._path, "r") as f:
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
        self._delivering = 0  # callbacks running, under the lock

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
            limit = None if timeout is None else wait_limit(timeout)
            return self._cond.wait_for(lambda: self._state is not PlayerState.PLAYING, limit)

    def close(self) -> None:
        """Stop playback and the player thread. Idempotent."""
        with self._cond:
            if self._closed:
                return
            if self._state is PlayerState.PLAYING:
                self._set_state(PlayerState.PAUSED)
            self._closed = True
            self._cond.notify_all()
            # From a callback on a calling thread the lock stays held by the call
            # that ran it, and the player thread ends only once it gets the lock:
            # it ends when that call returns.
            from_callback = self._delivering > 0
        t = self._thread
        if t is not None and t is not threading.current_thread() and not from_callback:
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
        self._delivering += 1
        try:
            for cb in list(cbs):
                if not call_guarded(cb, item):
                    self._callback_errors += 1
        finally:
            self._delivering -= 1

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
                except BaseException:
                    # The clock raised, `SystemExit` included, which would end the
                    # thread with the state left playing (the callbacks are guarded): counted, logged
                    # and paused — `wait()` returns, `play()` resumes — not retried
                    # in a busy loop (bug hunt 2026-11-03 JAVA-4; CPP-4).
                    self._callback_errors += 1
                    _log.exception("the player clock raised; playback paused")
                    if self._state is PlayerState.PLAYING:
                        self._set_state(PlayerState.PAUSED)
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
                # clamped: a tiny speed gives waits past threading.TIMEOUT_MAX
                # (OverflowError); the loop re-evaluates after every wake
                cond.wait(r if r is None else min(r, _MAX_WAIT_S))
