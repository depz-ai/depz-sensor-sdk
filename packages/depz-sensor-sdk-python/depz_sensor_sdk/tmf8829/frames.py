"""TMF8829 frame layout and pixel decoding (contracts/14_SENSOR_TMF8829.md §3).

A frame is the sensor bytes from register 0xFA on: 5-byte pre-header
(FIFOSTATUS, SYSTICK), 16-byte header, data (pixel results or histograms),
12-byte footer.
"""

from __future__ import annotations

import struct
from dataclasses import dataclass

import numpy as np

PRE_HEADER_SIZE = 5
HEADER_SIZE = 16
FOOTER_SIZE = 12
FRAME_HEAD = PRE_HEADER_SIZE + HEADER_SIZE  # 21 B the bridge reads @0xFA
FRAME_EOF = 0xE0F7

FID_MASK = 0xF0
FID_RESULTS = 0x10
FID_HISTOGRAMS = 0x20
FID_REF_SPAD = 0x30

LAYOUT_PEAKS = 0x07
LAYOUT_SIGNAL = 0x08
LAYOUT_NOISE = 0x10
LAYOUT_XTALK = 0x20
LAYOUT_SUBFRAME = 0x40  # 32x32/48x32: set on the second (odd-row) half-frame

FRAME_VALID = 0x01
FRAME_ABORTED = 0xC0
FRAME_WARNINGS = {
    0x08: "HV_CP_OVERLOAD",
    0x10: "VCDRV_OVERLOAD",
    0x20: "VCDRV_BURST_EXCEEDED",
}
FOOTER_DIST_IN_MM = 0x01  # footer.reserved b0: distances in mm, else 0.25 mm

FIFO_TX_UNDERRUN = 0x10
FIFO_RX_OVERRUN = 0x20

#: fp_mode → (rows, cols) of the full focal plane
FP_GRID = {0: (8, 8), 1: (8, 8), 2: (16, 16), 3: (32, 32), 4: (32, 32), 5: (32, 48)}
FP_NAMES = {0: "8x8A", 1: "8x8B", 2: "16x16", 3: "32x32", 4: "32x32s", 5: "48x32"}


class FrameError(ValueError):
    """A frame whose bytes do not match its own header."""


@dataclass(frozen=True)
class FrameInfo:
    """Pre-header, header and footer of one frame (any kind)."""

    fifo_status: int
    systick: int
    id: int
    layout: int
    payload: int
    f_number: int
    temperature: tuple[int, int, int]
    bdv: int
    ref_pos: tuple[int, int]
    t0_integration: int
    t1_integration: int
    frame_status: int
    reserved: int
    eof: int
    size: int

    @property
    def kind(self) -> int:
        """FID_RESULTS, FID_HISTOGRAMS or FID_REF_SPAD."""
        return self.id & FID_MASK

    @property
    def fp_mode(self) -> int:
        return self.id & 0x0F

    @property
    def is_second_half(self) -> bool:
        return bool(self.layout & LAYOUT_SUBFRAME)

    @property
    def distance_in_mm(self) -> bool:
        return bool(self.reserved & FOOTER_DIST_IN_MM)

    def problems(self) -> list[str]:
        """What is wrong with the frame, human-readable; empty when fine."""
        out = []
        if self.eof != FRAME_EOF:
            out.append(f"EOF 0x{self.eof:04X}")
        if not self.frame_status & FRAME_VALID:
            out.append(f"status 0x{self.frame_status:02X} not valid")
        if self.frame_status & FRAME_ABORTED:
            out.append("aborted")
        for bit, name in FRAME_WARNINGS.items():
            if self.frame_status & bit:
                out.append(name)
        if self.fifo_status & FIFO_TX_UNDERRUN:
            out.append("txfifo_underrun")
        if self.fifo_status & FIFO_RX_OVERRUN:
            out.append("rxfifo_overrun")
        return out


def frame_size(head: bytes) -> int:
    """Full frame size (pre-header included) from its first 21 bytes."""
    return PRE_HEADER_SIZE + 4 + int(struct.unpack_from("<H", head, 7)[0])


def parse_frame(raw: bytes) -> FrameInfo:
    if len(raw) < FRAME_HEAD + FOOTER_SIZE:
        raise FrameError(f"frame of {len(raw)} B is shorter than header + footer")
    fifo_status, systick = struct.unpack_from("<BI", raw, 0)
    fid, layout, payload, f_number, t0, t1, t2, bdv, ref0, ref1 = struct.unpack_from(
        "<BBHI3bBHH", raw, PRE_HEADER_SIZE
    )
    t0i, t1i, status, reserved, eof = struct.unpack_from(
        "<IIBBH", raw, len(raw) - FOOTER_SIZE
    )
    return FrameInfo(
        fifo_status,
        systick,
        fid,
        layout,
        payload,
        f_number,
        (t0, t1, t2),
        bdv,
        (ref0, ref1),
        t0i,
        t1i,
        status,
        reserved,
        eof,
        len(raw),
    )


@dataclass(frozen=True)
class PixelResults:
    """Decoded results of one frame or of a merged measurement.

    `distance`, `snr`, `signal` are [peaks, rows, cols]; `noise`, `xtalk`
    [rows, cols]. Raw sensor integers: distance in 0.25 mm (or mm with the
    footer flag), not yet clock-corrected. Absent fields are None."""

    peaks: int
    distance: np.ndarray | None
    snr: np.ndarray | None
    signal: np.ndarray | None
    noise: np.ndarray | None
    xtalk: np.ndarray | None

    @property
    def shape(self) -> tuple[int, int]:
        for a in (self.distance, self.noise, self.xtalk):
            if a is not None:
                return a.shape[-2:]
        return (0, 0)


def _pixel_dtype(layout: int) -> np.dtype:
    fields: list[tuple[str, str]] = []
    if layout & LAYOUT_NOISE:
        fields.append(("noise", "<u2"))
    if layout & LAYOUT_XTALK:
        fields.append(("xtalk", "<u2"))
    for p in range(layout & LAYOUT_PEAKS):
        fields += [(f"d{p}", "<u2"), (f"s{p}", "u1")]
        if layout & LAYOUT_SIGNAL:
            fields.append((f"g{p}", "<u2"))
    return np.dtype(fields)


def frame_rows(fp_mode: int) -> tuple[int, int]:
    """(rows, cols) one results frame carries: half the rows for 32x32/48x32."""
    rows, cols = FP_GRID[fp_mode]
    return (rows // 2, cols) if rows == 32 else (rows, cols)


def parse_results(raw: bytes, info: FrameInfo | None = None) -> PixelResults:
    """Pixel results of one results frame, row-major."""
    f = info or parse_frame(raw)
    if f.kind != FID_RESULTS:
        raise FrameError(f"frame id 0x{f.id:02X} is not a results frame")
    if f.fp_mode not in FP_GRID:
        raise FrameError(f"unknown fp_mode {f.fp_mode}")
    rows, cols = frame_rows(f.fp_mode)
    dt = _pixel_dtype(f.layout)
    n = rows * cols
    data_size = f.size - FRAME_HEAD - FOOTER_SIZE
    if dt.itemsize * n != data_size:
        raise FrameError(
            f"result data {data_size} B, expected {n} px × {dt.itemsize} B (layout 0x{f.layout:02X})"
        )
    px = np.frombuffer(raw, dtype=dt, count=n, offset=FRAME_HEAD)
    peaks = f.layout & LAYOUT_PEAKS

    def per_peak(prefix: str) -> np.ndarray | None:
        if f"{prefix}0" not in (dt.names or ()):
            return None
        return np.stack([px[f"{prefix}{p}"] for p in range(peaks)]).reshape(
            peaks, rows, cols
        )

    def plain(name: str) -> np.ndarray | None:
        return px[name].reshape(rows, cols).copy() if name in (dt.names or ()) else None

    return PixelResults(
        peaks,
        per_peak("d"),
        per_peak("s"),
        per_peak("g"),
        plain("noise"),
        plain("xtalk"),
    )


def merge_halves(first: PixelResults, second: PixelResults) -> PixelResults:
    """32x32/48x32: the first half-frame holds the even rows, the second the odd ones."""

    def merge(a: np.ndarray | None, b: np.ndarray | None) -> np.ndarray | None:
        if a is None or b is None:
            return None
        shape = list(a.shape)
        shape[-2] *= 2
        m = np.empty(shape, dtype=a.dtype)
        m[..., 0::2, :] = a
        m[..., 1::2, :] = b
        return m

    return PixelResults(
        first.peaks,
        merge(first.distance, second.distance),
        merge(first.snr, second.snr),
        merge(first.signal, second.signal),
        merge(first.noise, second.noise),
        merge(first.xtalk, second.xtalk),
    )


@dataclass(frozen=True)
class HistogramFrame:
    """A raw histogram frame (25 377 B); decoding the bins is left to the
    caller (ams histogram layout)."""

    info: FrameInfo
    raw: bytes

    @property
    def data(self) -> bytes:
        return self.raw[FRAME_HEAD : len(self.raw) - FOOTER_SIZE]
