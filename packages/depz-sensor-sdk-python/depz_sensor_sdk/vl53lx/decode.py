"""Stateless decoders of the blocks the 1D-family bridge streams (contract 12
§4) — the "base" every SDK implements, reference for the golden vectors.

What a block says on its own, without the driver state an init leaves behind:

* `decode_die_block` — the 17-byte VL53L1-die result block at 0x0089 (L1CX,
  L1CB, L3CX, L4CD, L4CX light drivers). Fully decoded: the two ULDs that read
  it differ only in `DIE_VARIANTS`.
* `decode_l0x_raw` — the raw fields of the VL53L0X 12-byte block at 0x14. The
  final range status and sigma need the device data cached at init
  (`VL53L0X.parse_result_block`), so they are not part of the base.
* `decode_histogram_raw` — the status bytes and 24 photon bins of the 83-byte
  histogram block at 0x0088. Turning bins into targets needs the preset, the
  VCSEL period and the frame-pair history — the full driver's job.

These are the same arithmetic the ports in `uld/` run; tests pin them to the
ports frame for frame.
"""

from __future__ import annotations

from dataclasses import dataclass

from .uld.bare import core as _bare
from .uld.vl53l1_die import RESULT_BLOCK_LEN as DIE_BLOCK_LEN
from .uld.vl53l1_die import STATUS_RTN

__all__ = [
    "DIE_BLOCK_ADDR",
    "DIE_BLOCK_LEN",
    "DIE_VARIANTS",
    "L0X_BLOCK_ADDR",
    "L0X_BLOCK_LEN",
    "HISTOGRAM_BLOCK_ADDR",
    "HISTOGRAM_BLOCK_LEN",
    "HISTOGRAM_BINS",
    "DieResult",
    "L0xRaw",
    "HistogramRaw",
    "decode_die_block",
    "decode_l0x_raw",
    "decode_histogram_raw",
]

DIE_BLOCK_ADDR = 0x0089
L0X_BLOCK_ADDR = 0x14
L0X_BLOCK_LEN = 12
HISTOGRAM_BLOCK_ADDR = _bare.HISTOGRAM_BIN_DATA_I2C_INDEX
HISTOGRAM_BLOCK_LEN = _bare.HISTOGRAM_BIN_DATA_I2C_SIZE_BYTES
HISTOGRAM_BINS = _bare.HISTOGRAM_BUFFER_SIZE

#: (signal-rate byte offset, per-SPAD scale K) of the two ULDs that read the
#: die block. "l4": VL53L4CD ULD — also the L3CX ULP and L4CX-as-L4CD;
#: "l1": VL53L1X ULD (crosstalk-corrected peak signal at 0x0098, K = 25).
DIE_VARIANTS = {"l4": (5, 256), "l1": (15, 25)}


@dataclass(frozen=True)
class DieResult:
    range_status: int  # ULD status via STATUS_RTN (0 = valid)
    distance_mm: int
    sigma_mm: int
    signal_rate_kcps: int
    ambient_rate_kcps: int
    signal_per_spad_kcps: int
    ambient_per_spad_kcps: int
    number_of_spad: int
    stream_count: int


def decode_die_block(raw: bytes, variant: str = "l4") -> DieResult:
    """The 17-byte die block (0x0089..0x0099) as the named ULD reads it."""
    if len(raw) < DIE_BLOCK_LEN:
        raise ValueError(f"die result block needs {DIE_BLOCK_LEN} bytes, got {len(raw)}")
    signal_at, k = DIE_VARIANTS[variant]
    status = raw[0] & 0x1F
    if status < len(STATUS_RTN):
        status = STATUS_RTN[status]
    raw_spads = int.from_bytes(raw[3:5], "big")  # 8.8
    signal = int.from_bytes(raw[signal_at : signal_at + 2], "big") * 8
    ambient = int.from_bytes(raw[7:9], "big") * 8
    return DieResult(
        range_status=status,
        distance_mm=int.from_bytes(raw[13:15], "big"),
        sigma_mm=int.from_bytes(raw[9:11], "big") // 4,
        signal_rate_kcps=signal,
        ambient_rate_kcps=ambient,
        signal_per_spad_kcps=signal * k // raw_spads if raw_spads else 0,
        ambient_per_spad_kcps=ambient * k // raw_spads if raw_spads else 0,
        number_of_spad=raw_spads // 256,
        stream_count=raw[2],
    )


@dataclass(frozen=True)
class L0xRaw:
    distance_raw: int  # mm (quarter-mm when RangeFractionalEnable, off by default)
    device_range_status: int  # raw byte 0; the PAL status needs the init state
    signal_rate_mcps_1616: int  # FixPoint16.16 Mcps (9.7 on the wire << 9)
    ambient_rate_mcps_1616: int
    effective_spad_count_88: int  # 8.8


def decode_l0x_raw(raw: bytes) -> L0xRaw:
    """Raw fields of the VL53L0X block at 0x14 (VL53L0X_GetRangingMeasurementData
    before the PAL status/sigma step)."""
    if len(raw) < L0X_BLOCK_LEN:
        raise ValueError(f"VL53L0X result block needs {L0X_BLOCK_LEN} bytes, got {len(raw)}")
    return L0xRaw(
        distance_raw=(raw[10] << 8) + raw[11],
        device_range_status=raw[0],
        signal_rate_mcps_1616=((raw[6] << 8) + raw[7]) << 9,
        ambient_rate_mcps_1616=((raw[8] << 8) + raw[9]) << 9,
        effective_spad_count_88=(raw[2] << 8) + raw[3],
    )


@dataclass(frozen=True)
class HistogramRaw:
    interrupt_status: int
    range_status: int
    report_status: int
    stream_count: int
    dss_actual_effective_spads: int
    reference_phase: int
    vcsel_start: int
    bins: tuple[int, ...]  # 24 photon counts


def decode_histogram_raw(raw: bytes) -> HistogramRaw:
    """The 83-byte histogram block at 0x0088: status bytes and the 24 bins
    (bin 23's low byte is carried in a separate MSB/LSB pair)."""
    if len(raw) < HISTOGRAM_BLOCK_LEN:
        raise ValueError(f"histogram block needs {HISTOGRAM_BLOCK_LEN} bytes, got {len(raw)}")
    off = HISTOGRAM_BLOCK_ADDR
    buf = bytearray(raw)
    buf[_bare.RESULT__HISTOGRAM_BIN_23_0 - off] = (
        (buf[_bare.RESULT__HISTOGRAM_BIN_23_0_MSB - off] << 2)
        + buf[_bare.RESULT__HISTOGRAM_BIN_23_0_LSB - off]
    ) & 0xFF
    base = _bare.RESULT__HISTOGRAM_BIN_0_2 - off
    bins = tuple(
        int.from_bytes(buf[base + 3 * i : base + 3 * i + 3], "big") for i in range(HISTOGRAM_BINS)
    )
    ref = _bare.PHASECAL_RESULT__REFERENCE_PHASE - off
    return HistogramRaw(
        interrupt_status=buf[0],
        range_status=buf[1],
        report_status=buf[2],
        stream_count=buf[3],
        dss_actual_effective_spads=int.from_bytes(buf[4:6], "big"),
        reference_phase=int.from_bytes(buf[ref : ref + 2], "big"),
        vcsel_start=buf[_bare.PHASECAL_RESULT__VCSEL_START - off],
        bins=bins,
    )
