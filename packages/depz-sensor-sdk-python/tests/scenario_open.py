"""How a contract 15 scenario opens its capture (15_SCENARIOS.md §1 `probe`).

Shared by the scenario runner and the tests that replay a scenario's capture
part-way (test_vl53lx_configure_replay.py).
"""

from __future__ import annotations

from depz_sensor_sdk.device import DeviceBase
from depz_sensor_sdk.discovery import attach_sensor_class, open_on_link, resolve_sensor_class
from depz_sensor_sdk.errors import DepzError
from depz_sensor_sdk.protocol.identity import SensorType, parse_software_name
from depz_sensor_sdk.transport.link import Link
from depz_sensor_sdk.usb_ids import DEPZ_PID_MODEL, DEPZ_USB_VID

# The firmware families whose old in-place probe also asked GET_DEVICE_NAME.
_LEGACY_ASKS_NAME = (SensorType.VL53L7, SensorType.VL53LX)


def usb_id(model: str | None) -> tuple[int | None, int | None]:
    """The production (vid, pid) of a USB-table model — what open_device
    would know from enumeration, and a replay has no way to see."""
    if model is None:
        return None, None
    pids = [pid for pid, m in DEPZ_PID_MODEL.items() if m == model]
    if not pids:
        raise ValueError(f"usb_model {model!r} is not in the USB-id table")
    return DEPZ_USB_VID, pids[0]


def open_legacy(link: Link, usb_vid: int | None, usb_pid: int | None, *, timeout: float):
    """A `probe: legacy` capture holds the probe the SDK made before contract
    02 §4.4: GET_NAME_ACTIVE_SOFTWARE, plus GET_DEVICE_NAME for the L5/L7 and
    1D families only. Issue exactly those requests, resolve the class with
    resolve_sensor_class, attach it in place. Returns (device, software_name)."""
    dev = DeviceBase(link, timeout=timeout)
    try:
        software = dev.get_software_name()
        device_name = ""
        if parse_software_name(software).sensor_type in _LEGACY_ASKS_NAME:
            try:
                device_name = dev.get_device_name()
            except DepzError:
                device_name = ""
        cls = resolve_sensor_class(software, device_name, usb_vid, usb_pid)
        return attach_sensor_class(dev, cls), software
    except BaseException:
        dev.close()
        raise


def open_scenario_link(sc: dict, link: Link, *, timeout: float):
    """Open a scenario's capture the way its `probe` field says. Returns
    (device, software name the legacy probe read — None for a canonical
    probe, whose answers open_on_link keeps to itself)."""
    vid, pid = usb_id(sc.get("usb_model"))
    probe = sc.get("probe", "canonical")
    if probe == "legacy":
        return open_legacy(link, vid, pid, timeout=timeout)
    if probe != "canonical":
        raise ValueError(f"unknown probe {probe!r}")
    return open_on_link(link, vid, pid, timeout=timeout), None
