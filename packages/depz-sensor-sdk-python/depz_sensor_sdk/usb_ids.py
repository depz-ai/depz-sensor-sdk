"""DEPZ USB identity table (contracts/02_COMMON_COMMANDS.md §4).

A single, one-edit-to-update table of the USB VID/PID values the DEPZ sensor
line ships with. This is used by discovery to pick the *right* serial port
without poking unrelated devices; the protocol probe (GET_NAME_ACTIVE_SOFTWARE)
remains the source of truth for what a device actually is. The PID→model map
is an informational hint only.

Values from depz-ai-website/src/lib/admin/sensorApi.ts + product metadata.
"""

from __future__ import annotations

# Production VID shared by every DEPZ sensor.
DEPZ_USB_VID = 0x1BCF  # 7119

# Per-model production PIDs (VID 0x1BCF), from the official DEPZ catalog.
# Named constants only — no bare hex elsewhere. sr04 / vl53l8cx / vl53l8ch /
# bno086 are decodable by this SDK; the rest are recognized DEPZ sensors
# (different silicon) surfaced as discovery/labelling hints.
PID_SR04 = 0xEC78  # 60536 — HC-SR04 ultrasonic
PID_VL53L8CH = 0xED40  # 60736 — VL53L8CH ToF (CNH-capable)
PID_VL53L0X = 0xED41  # 60737
PID_VL53L1CB = 0xED42  # 60738
PID_VL53L1CX = 0xED43  # 60739
PID_VL53L3CX = 0xED44  # 60740
PID_VL53L4CD = 0xED45  # 60741
PID_VL53L4CX = 0xED46  # 60742
PID_VL53L4ED = 0xED47  # 60743
PID_VL53L5CX = 0xED48  # 60744
PID_VL53L7CX = 0xED49  # 60745
PID_VL53L7CH = 0xED4A  # 60746
PID_VL53L8CX = 0xED4B  # 60747 — VL53L8CX ToF (base); hw-verified 1bcf:ed4b
PID_BNO086 = 0xEE08  # 60936 — BNO086 IMU (hw-verified 1bcf:ee08)
PID_BNO085 = 0xEE09  # 60937
PID_BNO055 = 0xEE0A  # 60938

# Back-compat alias: PID_VL53L8 was the single ToF PID before the CX/CH split.
PID_VL53L8 = PID_VL53L8CH

# PID → sensor-model hint. Informational: the protocol probe is authoritative.
DEPZ_PID_MODEL: dict[int, str] = {
    PID_SR04: "sr04",
    PID_VL53L8CH: "vl53l8ch",
    PID_VL53L0X: "vl53l0x",
    PID_VL53L1CB: "vl53l1cb",
    PID_VL53L1CX: "vl53l1cx",
    PID_VL53L3CX: "vl53l3cx",
    PID_VL53L4CD: "vl53l4cd",
    PID_VL53L4CX: "vl53l4cx",
    PID_VL53L4ED: "vl53l4ed",
    PID_VL53L5CX: "vl53l5cx",
    PID_VL53L7CX: "vl53l7cx",
    PID_VL53L7CH: "vl53l7ch",
    PID_VL53L8CX: "vl53l8cx",
    PID_BNO086: "bno086",
    PID_BNO085: "bno085",
    PID_BNO055: "bno055",
}

# Any PID in this inclusive range under DEPZ_USB_VID is treated as a candidate
# DEPZ sensor even when it is not individually mapped above. This is the whole
# reserved sensor block (catalog-types.ts: sensor PIDs live in 60536..65535,
# above camera PIDs), so future models are recognized without a code change.
DEPZ_PID_RANGE = (60536, 65535)

# Dev / unprogrammed default: STMicroelectronics VID/PID that dev units carry
# before product-metadata flashing (our live bench unit is one). Also a
# candidate DEPZ port.
DEV_USB_VID = 0x0483  # 1155
DEV_USB_PID = 0x56DC  # 22236


def is_known_depz_usb(vid: int | None, pid: int | None) -> bool:
    """True when (vid, pid) is a recognized DEPZ (or dev-default) USB id."""
    if vid is None or pid is None:
        return False
    if vid == DEV_USB_VID and pid == DEV_USB_PID:
        return True
    if vid != DEPZ_USB_VID:
        return False
    if pid in DEPZ_PID_MODEL:
        return True
    lo, hi = DEPZ_PID_RANGE
    return lo <= pid <= hi


def usb_model_hint(vid: int | None, pid: int | None) -> str | None:
    """Best-guess model name for a (vid, pid), or None.

    Informational only — never used to decide how to decode a device; the
    firmware-name probe does that.
    """
    if vid is None or pid is None:
        return None
    if vid == DEV_USB_VID and pid == DEV_USB_PID:
        return "dev"
    if vid == DEPZ_USB_VID:
        return DEPZ_PID_MODEL.get(pid)
    return None
