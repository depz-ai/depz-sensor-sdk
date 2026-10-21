"""TMF8829 presets and config-page fields (contracts/14_SENSOR_TMF8829.md §4)."""

from __future__ import annotations

REG_CFG = 0x22  # config page 0x22..0xDF
CFG_SIZE = 0xDF - REG_CFG + 1  # 190 B

#: preset name → load command (each loads its config page)
PRESETS: dict[str, int] = {
    "8x8": 0x40,
    "8x8-lr": 0x41,  # long range
    "8x8-ha": 0x42,  # high accuracy
    "16x16": 0x43,
    "16x16-ha": 0x44,
    "32x32": 0x45,
    "32x32-ha": 0x46,
    "48x32": 0x47,
    "48x32-ha": 0x48,
    "8x8-xr": 0x49,  # extended range, distances in mm
}

#: the preset's own kilo_iterations (a preset rescales a stale value, it does not reset it)
PRESET_KILO_ITERATIONS: dict[str, int] = {
    "8x8": 586,
    "8x8-lr": 586,
    "8x8-ha": 586,
    "8x8-xr": 586,
    "16x16": 586,
    "16x16-ha": 586,
    "32x32": 2344,
    "32x32-ha": 2344,
    "48x32": 3516,
    "48x32-ha": 3516,
}

DEFAULT_PERIOD_MS = 33  # the sensor's own value after init()

#: field → (register, bit shift, width in bits); width 16 = little-endian register pair
CFG_FIELDS: dict[str, tuple[int, int, int]] = {
    "period_ms": (0x22, 0, 16),
    "kilo_iterations": (0x24, 0, 16),
    "fp_mode": (0x26, 0, 3),
    "nr_peaks": (0x2A, 0, 3),
    "signal_strength": (0x2A, 3, 1),
    "noise_strength": (0x2A, 4, 1),
    "xtalk": (0x2A, 5, 1),
    "dump_histograms": (0x2B, 0, 1),
    "ref_spad_frame": (0x2C, 0, 1),
    "power_modes": (0x2E, 0, 8),
    "ha_kilo_iterations": (0x4A, 0, 16),
    "dual_mode": (0x4C, 0, 2),
    "fov_correction": (0x78, 0, 4),
    "distance_in_mm": (0x8F, 0, 1),
}


def cfg_get(page: bytes | bytearray, name: str) -> int:
    reg, shift, width = CFG_FIELDS[name]
    i = reg - REG_CFG
    if width == 16:
        return page[i] | (page[i + 1] << 8)
    return (page[i] >> shift) & ((1 << width) - 1)


def cfg_set(page: bytearray, name: str, value: int) -> None:
    reg, shift, width = CFG_FIELDS[name]
    i = reg - REG_CFG
    if not 0 <= value < (1 << width):
        raise ValueError(f"{name}: {value} out of range for {width} bit")
    if width == 16:
        page[i], page[i + 1] = value & 0xFF, value >> 8
    else:
        mask = ((1 << width) - 1) << shift
        page[i] = (page[i] & ~mask) | (value << shift)


def config_fields(page: bytes | bytearray) -> dict[str, int]:
    """Every named field of a config page."""
    return {name: cfg_get(page, name) for name in CFG_FIELDS}
