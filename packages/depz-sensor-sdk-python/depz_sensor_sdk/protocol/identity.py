"""Firmware-name parsing (contracts/02_COMMON_COMMANDS.md §4)."""

from __future__ import annotations

import re
from dataclasses import dataclass
from enum import Enum


class SensorType(str, Enum):
    SR04 = "sr04"
    VL53L4 = "vl53l4"
    VL53L8 = "vl53l8"
    VL53L7 = "vl53l7"  # VL53L5CX / VL53L7CX / VL53L7CH board (contract 11)
    VL53LX = "vl53lx"  # VL53L0X / L1CX / L1CB / L3CX / L4CD / L4CX board (contract 12)
    BNO086 = "bno086"
    BNO055 = "bno055"  # register bridge, fusion on chip (contract 13)
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
    ("VL53L7", SensorType.VL53L7),
    # The 1D-family bridge: boards answer APP_VL53L0_4_v*, the protocol spec
    # calls it APP_VL53LX_v* (contract 12).
    ("VL53L0_4", SensorType.VL53LX),
    ("VL53LX", SensorType.VL53LX),
    ("BNO086", SensorType.BNO086),
    ("BNO055", SensorType.BNO055),
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
