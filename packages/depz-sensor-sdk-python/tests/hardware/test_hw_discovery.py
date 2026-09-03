"""Real-hardware discovery and identification (contract 02 §4).

These tests answer: does the SDK find the units that are actually plugged in,
call them what they are, and behave predictably when a port is missing, busy,
or not a DEPZ device?
"""

from __future__ import annotations

import pytest
import serial

from _support import (
    FAMILY_VL53L8CH,
    FAMILY_VL53L8CX,
    port_lease,
)

from depz_sensor_sdk import (
    DeviceLostError,
    NoDepzDeviceError,
    is_known_depz_usb,
    list_depz_devices,
    open_device,
    probe_port,
    usb_model_hint,
)
from depz_sensor_sdk.protocol.identity import SensorType

# Bounded like every other hardware module: discovery opens ports, and a port
# held by a stale process (or a device that stopped answering) would otherwise
# hang the whole run with nothing reported. 120s is ~30x the suite's normal
# runtime — loose enough never to flake, tight enough to fail rather than hang.
pytestmark = [pytest.mark.hardware, pytest.mark.timeout(120)]


def test_enumeration_finds_every_attached_unit(hw_inventory):
    """Every DEPZ unit the OS shows must be USB-recognised by the SDK."""
    if not hw_inventory:
        pytest.skip("no DEPZ hardware attached")
    for dev in hw_inventory:
        assert is_known_depz_usb(dev.usb_vid, dev.usb_pid), (
            f"{dev.stable_id}: USB {dev.usb_vid:#06x}:{dev.usb_pid:#06x} not in the SDK table"
        )


def test_every_unit_has_a_stable_by_id_path(hw_inventory):
    """Discovery must be anchorable to something better than /dev/ttyACMn.

    Port paths are reassigned on every replug; a QA suite that pins them is
    testing the kernel's enumeration order, not the SDK.
    """
    for dev in hw_inventory:
        assert dev.usb_serial, f"{dev.stable_id}: no USB iSerial — cannot be identified stably"
        assert dev.by_id_path, f"{dev.stable_id}: no /dev/serial/by-id symlink"


def test_probe_reports_correct_sensor_type(hw_inventory):
    """The firmware probe is authoritative — it must agree with the USB hint."""
    expected = {
        "sr04": SensorType.SR04,
        "vl53l8cx": SensorType.VL53L8,
        "vl53l8ch": SensorType.VL53L8,
        "bno086": SensorType.BNO086,
    }
    for dev in hw_inventory:
        with port_lease(dev.resolve_port()):
            info = probe_port(dev.resolve_port(), timeout=0.5)
        assert info is not None, f"{dev.stable_id}: present but did not answer the probe"
        assert info.sensor_type == expected[dev.family], (
            f"{dev.stable_id}: probe says {info.sensor_type}, USB hint says {dev.family}"
        )
        assert info.mode == "app"
        assert info.fw_version, f"{dev.stable_id}: no firmware version reported"


def test_usb_hint_distinguishes_cx_from_ch(hw_inventory):
    """CX and CH share a firmware name; only the USB PID tells them apart.

    Regression guard for the CX/CH split: both report ``APP_VL53L8_v*``, so if
    the PID→model hint ever regresses, ``open_device`` silently hands back the
    wrong class and CNH breaks. Verified against real silicon here.
    """
    tof = [d for d in hw_inventory if d.family in (FAMILY_VL53L8CX, FAMILY_VL53L8CH)]
    if not tof:
        pytest.skip("no VL53L8 device attached")
    for dev in tof:
        assert usb_model_hint(dev.usb_vid, dev.usb_pid) == dev.family


def test_list_depz_devices_matches_inventory(hw_inventory):
    with_leases = []
    try:
        for dev in hw_inventory:
            cm = port_lease(dev.resolve_port())
            cm.__enter__()
            with_leases.append(cm)
        # list_depz_devices opens each port itself; leases are released first.
        for cm in with_leases:
            cm.__exit__(None, None, None)
        with_leases.clear()

        found = list_depz_devices(timeout=0.5)
        assert len(found) == len(hw_inventory), (
            f"discovery found {len(found)} device(s), {len(hw_inventory)} are attached"
        )
        found_serials = {f.usb_serial for f in found}
        assert found_serials == {d.usb_serial for d in hw_inventory}
    finally:
        for cm in with_leases:
            cm.__exit__(None, None, None)


def test_repeated_probe_is_stable(any_info):
    """Probing must be idempotent — it opens and closes the port each time."""
    dev = any_info
    results = []
    for _ in range(5):
        with port_lease(dev.resolve_port()):
            info = probe_port(dev.resolve_port(), timeout=0.5)
        assert info is not None, f"{dev.stable_id}: probe #{len(results)} returned nothing"
        results.append((info.sensor_type, info.software_name, info.serial_number))
    assert len(set(results)) == 1, f"probe is not stable across repeats: {set(results)}"


def test_probe_of_non_depz_port_returns_none():
    """A non-DEPZ serial node must probe as 'not a DEPZ device', not raise."""
    import os

    if not os.path.exists("/dev/ttyS0"):
        pytest.skip("no /dev/ttyS0 to use as a non-DEPZ port")
    try:
        assert probe_port("/dev/ttyS0", timeout=0.2) is None
    except (DeviceLostError, serial.SerialException, OSError):
        pytest.skip("/dev/ttyS0 not openable on this host")


def test_probe_of_missing_port_returns_none():
    assert probe_port("/dev/depz-does-not-exist", timeout=0.2) is None


def test_open_device_by_usb_serial(any_info):
    """Selecting by USB serial must open that exact unit."""
    dev = any_info
    with port_lease(dev.resolve_port()):
        obj = open_device(serial=dev.usb_serial, timeout=0.5)
        try:
            assert obj.port == dev.resolve_port()
            assert obj.get_serial_number() == dev.protocol_serial
        finally:
            obj.close()


def test_open_device_unknown_serial_raises():
    with pytest.raises(NoDepzDeviceError):
        open_device(serial="NO-SUCH-SERIAL-XYZ", timeout=0.2)


def test_open_device_index_out_of_range(hw_inventory):
    with pytest.raises(NoDepzDeviceError):
        open_device(len(hw_inventory) + 50, timeout=0.2)


def test_open_missing_port_raises_device_lost():
    with pytest.raises((DeviceLostError, NoDepzDeviceError)):
        open_device("/dev/depz-does-not-exist", timeout=0.2)


def test_busy_port_is_rejected_not_corrupted(any_info):
    """A second opener must fail cleanly rather than silently share the fd.

    POSIX does not make a tty single-opener: without ``exclusive=True`` two
    processes both ``open()`` and then steal each other's bytes at random. This
    test pins whichever behaviour the SDK actually has so a regression to
    silent byte-stealing is caught.
    """
    dev = any_info
    port = dev.resolve_port()
    with port_lease(port):
        holder = serial.Serial(port, 115200, timeout=0.05, exclusive=True)
        try:
            with pytest.raises((serial.SerialException, DeviceLostError, OSError)):
                open_device(port, timeout=0.3)
        finally:
            holder.close()


def test_all_four_families_present(hw_inventory):
    """On a bench, all four sensor types must be attached.

    The distinction that matters:

    * **Nothing attached** — you are not on the bench (a laptop, a CI runner).
      Skip; there is no claim to check.
    * **Some attached, not all** — you *are* on the bench and a family is
      missing or not answering. That FAILS: the run genuinely did not cover
      that sensor, and the report must say so rather than quietly claim
      four-sensor coverage it never had.
    """
    if not hw_inventory:
        pytest.skip("no DEPZ hardware attached — not a bench")
    families = {d.family for d in hw_inventory}
    missing = {"sr04", "vl53l8cx", "vl53l8ch", "bno086"} - families
    if missing:
        pytest.fail(
            f"on a bench with {sorted(families)} attached, but "
            f"{sorted(missing)} missing — this run does not cover them"
        )
