"""Frames in stream order → measurements (contracts/14_SENSOR_TMF8829.md §6)."""

from __future__ import annotations

from dataclasses import dataclass, field

import numpy as np

from ..protocol._wire import check_uint
from .clock import CaptureTimer, FrameTime, SensorClock
from .frames import (
    FID_HISTOGRAMS,
    FID_REF_SPAD,
    FID_RESULTS,
    FP_GRID,
    FrameError,
    LAYOUT_SUBFRAME,
    FrameInfo,
    HistogramFrame,
    PixelResults,
    merge_halves,
    parse_frame,
    parse_results,
)


@dataclass(frozen=True)
class Tmf8829Measurement:
    """One measurement: its results frame (8x8, 16x16) or both half-frames
    (32x32, 48x32) merged, with the histogram / ref-SPAD frames before it.

    `distance_mm` is [peaks, rows, cols], clock-corrected, in mm (None when
    the result format has no peaks). `results` keeps the raw sensor integers.
    `problems` lists what went wrong since the previous measurement."""

    timestamp_us: int  # capture time, MCU µs (mean of the half-frame captures)
    flags: int  # FT_* of all its results frames
    fp_mode: int
    f_number: int
    results: PixelResults
    distance_mm: np.ndarray | None
    frames: list[FrameInfo]
    times: list[FrameTime]
    histograms: list[HistogramFrame] = field(default_factory=list)
    ref_frames: list[bytes] = field(default_factory=list)  # 65 B each, zero pre-header
    problems: list[str] = field(default_factory=list)

    @property
    def shape(self) -> tuple[int, int]:
        """(rows, cols) of the full focal plane."""
        return FP_GRID[self.fp_mode]

    @property
    def peaks(self) -> int:
        return self.results.peaks

    @property
    def temperature_c(self) -> int:
        """Sensor die temperature (header byte 0), °C."""
        return self.frames[-1].temperature[0]


def distance_mm(results: PixelResults, ratio: float, in_mm: bool) -> np.ndarray | None:
    """Raw distances → mm, clock-corrected (×ratio; ÷4 for 0.25 mm units)."""
    if results.distance is None:
        return None
    d = results.distance.astype(np.float64) * ratio
    return d if in_mm else d / 4.0


class MeasurementBuilder:
    """Histogram frames between two results frames belong to the second; a
    ref-SPAD frame comes right before its results frame (each half-frame has
    its own). 32x32/48x32: the first half (layout 0x40 clear) holds the even
    rows. Call `reset()` at the start of every stream: the clock model and
    T1 are per stream."""

    def __init__(self) -> None:
        self.clock = SensorClock()
        self.timer = CaptureTimer(self.clock)
        self.reset()

    def reset(self) -> None:
        self.clock.reset()
        self.timer.reset()
        self._pending: list[tuple[FrameInfo, bytes]] = []
        self._hist: list[HistogramFrame] = []
        self._refs: list[bytes] = []
        self._half: tuple[FrameInfo, PixelResults, FrameTime] | None = None
        self._problems: list[str] = []
        self.parse_errors = 0

    def add(
        self, raw: bytes, timestamp_us: int, edge_us: int = 0
    ) -> Tmf8829Measurement | None:
        """One frame with the MCU time of its header read and its INT edge
        (0 = none) → the measurement it completes, or None."""
        # contract 07 §3: MCU µs times are u64 — checked before the frame is parsed
        check_uint("timestamp_us", timestamp_us, 64)
        check_uint("edge_us", edge_us, 64)
        try:
            f = parse_frame(raw)
        except FrameError as e:
            self.parse_errors += 1
            self._problems.append(str(e))
            return None
        if f.kind == FID_REF_SPAD:
            self._pending.append((f, raw))  # zero pre-header: no clock pair
            return None
        if f.kind not in (FID_RESULTS, FID_HISTOGRAMS):
            self.parse_errors += 1
            self._problems.append(f"frame id 0x{f.id:02X}: unknown kind")
            return None
        flags = self.clock.add(timestamp_us, f.systick)
        bad = f.problems()
        if bad:
            self._problems.append(
                f"frame id=0x{f.id:02X} fN={f.f_number} {f.size} B: {', '.join(bad)}"
            )
        if f.kind == FID_HISTOGRAMS:
            self.timer.histograms()
            self._pending.append((f, raw))
            return None

        t = self.timer.results_frame(f, timestamp_us, edge_us, flags)
        for pf, praw in self._pending:
            if pf.kind == FID_HISTOGRAMS:
                self._hist.append(HistogramFrame(pf, praw))
            else:
                self._refs.append(praw)
        self._pending = []
        try:
            res = parse_results(raw, f)
        except FrameError as e:
            self.parse_errors += 1
            self._problems.append(f"fN={f.f_number}: {e}")
            self._half = None
            return None

        if FP_GRID.get(f.fp_mode, (0, 0))[0] == 32:
            if self._half is None:
                if f.is_second_half:
                    self._problems.append(
                        f"second half fN={f.f_number} without a first one - dropped"
                    )
                    self._hist, self._refs = [], []
                    return None
                self._half = (f, res, t)
                return None
            if not f.is_second_half:
                self._problems.append(
                    f"two first halves in a row (fN {self._half[0].f_number}, {f.f_number})"
                )
                self._half = (f, res, t)
                return None
            f0, res0, t0 = self._half
            self._half = None
            if (f0.fp_mode, f0.layout & ~LAYOUT_SUBFRAME) != (f.fp_mode, f.layout & ~LAYOUT_SUBFRAME):
                # Impossible within one stream (configure() needs a stop); never crash the reader.
                self._problems.append(f"half-frames of different formats (fN {f0.f_number}, {f.f_number}) - dropped")
                self._hist, self._refs = [], []
                return None
            frames, times = [f0, f], [t0, t]
            res = merge_halves(res0, res)
        else:
            frames, times = [f], [t]

        m = Tmf8829Measurement(
            timestamp_us=sum(x.timestamp_us for x in times) // len(times),
            flags=_or(x.flags for x in times),
            fp_mode=f.fp_mode,
            f_number=f.f_number,
            results=res,
            distance_mm=distance_mm(res, self.clock.ratio, f.distance_in_mm),
            frames=frames,
            times=times,
            histograms=self._hist,
            ref_frames=self._refs,
            problems=self._problems,
        )
        self._hist, self._refs, self._problems = [], [], []
        return m


def _or(values) -> int:
    out = 0
    for v in values:
        out |= v
    return out


def record_value(m: Tmf8829Measurement) -> dict:
    """Contract 09 `tmf8829` record value of a measurement."""
    r = m.results
    rows, cols = m.shape
    v: dict = {
        "fp_mode": m.fp_mode,
        "rows": rows,
        "cols": cols,
        "peaks": r.peaks,
        "f_number": m.f_number,
        "flags": m.flags,
        "temperature_c": m.temperature_c,
        "distance_mm": [] if m.distance_mm is None else [round(float(x), 1) for x in m.distance_mm.reshape(-1)],
        "snr": [] if r.snr is None else [int(x) for x in r.snr.reshape(-1)],
    }
    for key in ("signal", "noise", "xtalk"):
        a = getattr(r, key)
        if a is not None:
            v[key] = [int(x) for x in a.reshape(-1)]
    return v
