"""`.depzdata` datasets: decoded, multi-device, time-synced recording &
playback (contracts/09_DATASET_FORMAT.md).

`SessionRecorder` records any mix of connected sensors onto one shared host
timeline (it runs `sync_time` on every device first). `DatasetReader` merges
records by host time and can pace them for playback.
"""

from __future__ import annotations

import gzip
import heapq
import json
import threading
import time
from dataclasses import dataclass, field
from pathlib import Path
from typing import IO, Any, Callable, Iterator

from .device import DeviceBase
from .sr04 import Sr04, Sr04Measurement

SCHEMA = "depz.dataset/1"


def _open_text(path: str | Path, mode: str) -> IO[str]:
    p = Path(path)
    if p.suffix == ".gz":
        return gzip.open(p, mode + "t", encoding="utf-8")  # type: ignore[return-value]
    return open(p, mode, encoding="utf-8")


@dataclass(frozen=True)
class DatasetRecord:
    device_id: str
    t_host_us: int
    kind: str
    value: dict[str, Any]


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

    def __init__(self, path: str | Path, *, note: str = "", vl53l8_layers: bool = False):
        self._path = path
        self._note = note
        self._vl53l8_layers = vl53l8_layers  # include signal/ambient/sigma/reflectance
        self._entries: dict[str, _DeviceEntry] = {}
        self._writer: DatasetWriter | None = None

    def add(self, device: DeviceBase, *, device_id: str | None = None, sync_samples: int = 5) -> str:
        """Register a device (before the first record is written). Runs
        `sync_time` so its timestamps land on the shared host timeline."""
        if self._writer is not None:
            raise RuntimeError("add() all devices before recording starts")
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

    def _hook(self, did: str, entry: _DeviceEntry) -> None:
        dev = entry.device
        offset = entry.meta["time_sync"]["offset_us"]
        writer = self._writer
        assert writer is not None

        if isinstance(dev, Sr04):

            def on_measure(m: Sr04Measurement, _did=did, _off=offset):
                writer.write(_did, m.timestamp_us - _off, "sr04",
                             {"echo_us": m.echo_time_us, "source": m.source})

            entry.unsubscribes.append(dev.on_measurement(on_measure))
            return

        try:
            from .vl53l8 import Vl53l8, Vl53l8Frame
        except ImportError:
            Vl53l8 = None  # type: ignore[assignment]
        if Vl53l8 is not None and isinstance(dev, Vl53l8):
            layers = self._vl53l8_layers

            def on_frame(f: "Vl53l8Frame", _did=did, _off=offset):
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
                writer.write(_did, f.timestamp_us - _off, "vl53l8", v)

            entry.unsubscribes.append(dev.on_frame(on_frame))
            return

        from .vl53l4 import Vl53l4Cd, Vl53l4Measurement

        if isinstance(dev, Vl53l4Cd):

            def on_result(m: "Vl53l4Measurement", _did=did, _off=offset):
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
        # Unknown device class: nothing to hook (future sensors extend here).

    def stop(self) -> None:
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
            self.header: dict[str, Any] = json.loads(f.readline())
        if not str(self.header.get("schema", "")).startswith("depz.dataset/"):
            raise ValueError(f"{path}: not a depz.dataset file")

    @property
    def devices(self) -> dict[str, dict[str, Any]]:
        return self.header.get("devices", {})

    def _raw_iter(self) -> Iterator[DatasetRecord]:
        with _open_text(self._path, "r") as f:
            f.readline()  # header
            for line in f:
                if not line.strip():
                    continue
                ev = json.loads(line)
                yield DatasetRecord(ev["d"], ev["t"], ev["k"], ev["v"])

    def __iter__(self) -> Iterator[DatasetRecord]:
        """Records merged by `t` (per-device order is already monotonic, so a
        k-way heap merge is exact without loading the file into memory)."""
        streams: dict[str, list[DatasetRecord]] = {}
        for rec in self._raw_iter():
            streams.setdefault(rec.device_id, []).append(rec)
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
        fast; speed=0 → as fast as possible)."""
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
