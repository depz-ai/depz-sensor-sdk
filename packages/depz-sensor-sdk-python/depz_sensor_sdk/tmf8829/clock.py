"""Sensor clock model and frame capture time (contracts/14_SENSOR_TMF8829.md §5).

Integer arithmetic exactly as the contract (and Alexey's reference host); the
vectors in `contracts/vectors/tmf8829_frame_time.json` pin it bit for bit.
"""

from __future__ import annotations

from dataclasses import dataclass

from .frames import FrameInfo

NOMINAL_Q16 = 8 << 16  # SYSTICK 125 kHz = 8 MCU µs per tick, Q16
CLOCK_WINDOW_MIN_US = 200_000  # first ratio estimate
CLOCK_WINDOW_SLIDE_US = 1_000_000  # then a 1..2 s window
EDGE_LATE_US = 2000

FT_NOMINAL = 0x01  # clock ratio not measured yet (nominal 8 µs/tick used)
FT_CLOCK_RESET = 0x02  # clock model restarted at this frame (SYSTICK jump)
FT_EDGE_LATE = 0x04  # INT edge > 2 ms later after t1 than usual in this stream
FT_STALLED = 0x08  # sub-integrations not back to back (histogram FIFO wait)
FT_T1_UNSURE = 0x10  # histograms + 32x32/48x32: T1 may be too long, centre late
FT_NAMES = {
    FT_NOMINAL: "nominal",
    FT_CLOCK_RESET: "clock_reset",
    FT_EDGE_LATE: "edge_late",
    FT_STALLED: "stalled",
    FT_T1_UNSURE: "t1_unsure",
}

#: sub-integrations per results frame by fp_mode
FP_SUBINTS = {0: 2, 1: 2, 2: 2, 3: 4, 4: 4, 5: 6}

_U32 = 0xFFFFFFFF


def _s32(x: int) -> int:
    return ((x + 0x80000000) & _U32) - 0x80000000


def flag_names(flags: int) -> list[str]:
    return [name for bit, name in FT_NAMES.items() if flags & bit]


class SensorClock:
    """Sensor SYSTICK (125 kHz) → MCU µs, fed one clock pair per frame (the
    MCU time of the header read and the pre-header SYSTICK sampled by it).
    The ratio is taken over a sliding 1..2 s window, first after 200 ms; it
    is also the distance correction (ams tmf8829ClockCorrection)."""

    def __init__(self) -> None:
        self.reset()

    def reset(self) -> None:
        self._valid = False
        self._last: tuple[int, int] = (0, 0)
        self._anchor: tuple[int, int] = (0, 0)
        self._mid: tuple[int, int] | None = None
        self.ratio_q16 = NOMINAL_Q16
        self.nominal = True

    @property
    def ratio(self) -> float:
        """MCU/sensor clock ratio (≈ 1.015): distances are scaled by it."""
        return self.ratio_q16 / NOMINAL_Q16

    @staticmethod
    def _step_ok(d_us: int, d_tick: int) -> bool:
        expect = d_tick * 8
        tol = d_us // 10 + 50
        return expect + tol >= d_us and expect <= d_us + tol

    def add(self, us: int, tick: int) -> int:
        """Add a pair → FT_CLOCK_RESET if it does not fit the model (which then
        restarts from it), else 0."""
        flags = 0
        if self._valid and not self._step_ok(
            us - self._last[0], (tick - self._last[1]) & _U32
        ):
            self._valid = False
            flags = FT_CLOCK_RESET
        self._last = (us, tick)
        if not self._valid:
            self._valid = True
            self._anchor = self._last
            self._mid = None
            self.ratio_q16 = NOMINAL_Q16
            self.nominal = True
            return flags
        span = us - self._anchor[0]
        if span >= CLOCK_WINDOW_MIN_US:
            self.ratio_q16 = ((span << 16) // ((tick - self._anchor[1]) & _U32)) & _U32
            self.nominal = False
        if self._mid is None and span >= CLOCK_WINDOW_SLIDE_US:
            self._mid = self._last
        elif self._mid is not None and span >= 2 * CLOCK_WINDOW_SLIDE_US:
            self._anchor, self._mid = self._mid, None
        return flags

    def to_us(self, pair_us: int, pair_tick: int, tick: int) -> int:
        """SYSTICK `tick` → MCU µs through the pair (pair_us, pair_tick)."""
        return pair_us + ((_s32(tick - pair_tick) * self.ratio_q16) >> 16)


@dataclass(frozen=True)
class FrameTime:
    """Capture time of one results frame, MCU µs."""

    timestamp_us: int  # centre of the capture window
    flags: int  # FT_*
    t1_len_us: int  # sub-integration length used
    t0_us: int  # start of the first sub-integration
    t1_us: int  # start of the last one
    edge_us: int  # INT edge credited to the frame, 0 = none
    pair_us: int  # MCU time of the header read (the clock pair)
    ratio_q16: int  # clock ratio after this frame


class CaptureTimer:
    """When a results frame was captured. A results frame is N equal
    sub-integrations back to back; the footer has the SYSTICK of the start of
    the first (t0) and of the last (t1). The window is [t0, t1 + T1], T1 the
    stream's shortest (t1 − t0)/(N − 1); its centre is the frame time."""

    def __init__(self, clock: SensorClock) -> None:
        self.clock = clock
        self.reset()

    def reset(self) -> None:
        self._lag_min: int | None = None
        self._hist_seen = False
        self._sub_min = 0

    def histograms(self) -> None:
        """A histogram frame was seen in the stream."""
        self._hist_seen = True

    def results(
        self,
        fp_mode: int,
        systick: int,
        t0_tick: int,
        t1_tick: int,
        pair_us: int,
        edge_us: int,
        flags: int,
    ) -> FrameTime:
        c = self.clock
        if c.nominal:
            flags |= FT_NOMINAL
        t0 = c.to_us(pair_us, systick, t0_tick)
        t1 = c.to_us(pair_us, systick, t1_tick)

        if edge_us and not self._hist_seen:
            lag = edge_us - t1
            if lag <= 0 or (
                self._lag_min is not None and lag > self._lag_min + EDGE_LATE_US
            ):
                flags |= FT_EDGE_LATE
            elif self._lag_min is None or lag < self._lag_min:
                self._lag_min = lag

        n1 = FP_SUBINTS.get(fp_mode, 2) - 1
        sub = ((t1_tick - t0_tick) & _U32) // n1
        if not self._sub_min or sub < self._sub_min:
            self._sub_min = sub
        if sub > self._sub_min + 1:
            flags |= FT_STALLED
        if self._hist_seen and n1 > 1:
            flags |= FT_T1_UNSURE

        t1_len = ((self._sub_min * c.ratio_q16) >> 16) & _U32
        return FrameTime(
            (t0 + t1 + t1_len) // 2,
            flags,
            t1_len,
            t0,
            t1,
            edge_us,
            pair_us,
            c.ratio_q16,
        )

    def results_frame(
        self, f: FrameInfo, pair_us: int, edge_us: int, flags: int
    ) -> FrameTime:
        return self.results(
            f.fp_mode,
            f.systick,
            f.t0_integration,
            f.t1_integration,
            pair_us,
            edge_us,
            flags,
        )
