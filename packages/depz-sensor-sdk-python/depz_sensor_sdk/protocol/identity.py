"""Firmware-name parsing (contracts/02_COMMON_COMMANDS.md §4)."""

from __future__ import annotations

import re
from dataclasses import dataclass
from enum import Enum


class SensorType(str, Enum):
    SR04 = "sr04"
    VL53L4 = "vl53l4"
    VL53L8 = "vl53l8"
    BNO086 = "bno086"
    UNKNOWN = "unknown"


@dataclass(frozen=True)
class Identity:
    mode: str  # "app" | "bootloader" | "unknown"
    sensor_type: SensorType | None  # None in bootloader mode
    software_name: str
    version: str  # "" when not parseable


_VERSION_RE = re.compile(r"_v(\d+(?:\.\d+)*)$")

_PRODUCT_TOKENS = (
    ("SR04", SensorType.SR04),
    ("VL53L4", SensorType.VL53L4),
    ("VL53L8", SensorType.VL53L8),
    ("BNO086", SensorType.BNO086),
)


def parse_software_name(name: str) -> Identity:
    """Classify a GET_NAME_ACTIVE_SOFTWARE string.

    The string must already be stripped of trailing NUL/0xFF
    (`strip_device_string`).
    """
    m = _VERSION_RE.search(name)
    version = m.group(1) if m else ""
    if name.startswith("BOOTDEPZ"):
        return Identity("bootloader", None, name, version)
    if name.startswith("APP_"):
        for token, sensor in _PRODUCT_TOKENS:
            if token in name:
                return Identity("app", sensor, name, version)
        return Identity("app", SensorType.UNKNOWN, name, version)
    return Identity("unknown", None, name, version)
