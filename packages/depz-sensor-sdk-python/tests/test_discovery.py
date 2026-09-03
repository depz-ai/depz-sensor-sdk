"""Detection / discovery: USB-id table + open_device selection logic.

Uses a fake list_ports (monkeypatched) + a fake probe/open so nothing touches
real hardware.
"""

from __future__ import annotations

from types import SimpleNamespace

import pytest

from depz_sensor_sdk import discovery
from depz_sensor_sdk.errors import NoDepzDeviceError
from depz_sensor_sdk.usb_ids import (
    DEPZ_USB_VID,
    DEV_USB_PID,
    DEV_USB_VID,
    PID_BNO086,
    PID_SR04,
    PID_VL53L8,
    PID_VL53L8CH,
    PID_VL53L8CX,
    is_known_depz_usb,
    usb_model_hint,
)


# ── usb_ids table ─────────────────────────────────────────────────────────────


def test_known_usb_ids():
    assert is_known_depz_usb(DEPZ_USB_VID, PID_SR04)
    assert is_known_depz_usb(DEPZ_USB_VID, PID_VL53L8)
    assert is_known_depz_usb(DEPZ_USB_VID, PID_BNO086)
    assert is_known_depz_usb(DEV_USB_VID, DEV_USB_PID)  # dev default
    # in-range but unmapped PID is still a candidate
    assert is_known_depz_usb(DEPZ_USB_VID, 60744)


def test_unknown_usb_ids():
    assert not is_known_depz_usb(0x1234, 0x5678)
    assert not is_known_depz_usb(DEPZ_USB_VID, 0x1234)  # wrong pid
    assert not is_known_depz_usb(None, None)
    assert not is_known_depz_usb(DEV_USB_VID, 0x0000)  # dev vid wrong pid


def test_model_hint():
    assert usb_model_hint(DEPZ_USB_VID, PID_SR04) == "sr04"
    assert usb_model_hint(DEPZ_USB_VID, PID_BNO086) == "bno086"
    assert usb_model_hint(DEPZ_USB_VID, PID_VL53L8CX) == "vl53l8cx"  # CX production PID
    assert usb_model_hint(DEPZ_USB_VID, PID_VL53L8CH) == "vl53l8ch"
    assert usb_model_hint(DEV_USB_VID, DEV_USB_PID) == "dev"
    assert usb_model_hint(DEPZ_USB_VID, 0xED4C) is None  # in-range candidate but unnamed
    assert usb_model_hint(0x1234, 0x5678) is None


# ── open_device selection ─────────────────────────────────────────────────────


def _port(device, vid, pid, serial):
    return SimpleNamespace(device=device, vid=vid, pid=pid, serial_number=serial)


@pytest.fixture()
def fake_ports(monkeypatch):
    """Install a fake list_ports.comports() returning `state['ports']`."""
    state = {"ports": []}
    monkeypatch.setattr(
        discovery.list_ports, "comports", lambda: list(state["ports"])
    )
    return state


@pytest.fixture()
def spy_open(monkeypatch):
    """Replace the port-open path so no hardware is touched; record the port."""
    calls = {"opened": []}

    def fake_open_probed(port, timeout):
        calls["opened"].append(port)
        return SimpleNamespace(port=port)

    monkeypatch.setattr(discovery, "_open_probed", fake_open_probed)
    return calls


def test_default_picks_smallest_serial(fake_ports, spy_open):
    fake_ports["ports"] = [
        _port("/dev/ttyACM2", DEPZ_USB_VID, PID_SR04, "SN000009"),
        _port("/dev/ttyACM0", DEPZ_USB_VID, PID_BNO086, "SN000005"),
        _port("/dev/ttyS0", None, None, None),  # not DEPZ, ignored
    ]
    dev = discovery.open_device()
    assert dev.port == "/dev/ttyACM0"  # SN000005 sorts first
    assert spy_open["opened"] == ["/dev/ttyACM0"]


def test_index_orders_by_serial(fake_ports, spy_open):
    fake_ports["ports"] = [
        _port("/dev/ttyACM2", DEPZ_USB_VID, PID_SR04, "SN000009"),
        _port("/dev/ttyACM0", DEPZ_USB_VID, PID_BNO086, "SN000005"),
        _port("/dev/ttyACM1", DEPZ_USB_VID, PID_VL53L8, "SN000007"),
    ]
    assert discovery.open_device(0).port == "/dev/ttyACM0"  # SN...05
    assert discovery.open_device(1).port == "/dev/ttyACM1"  # SN...07
    assert discovery.open_device(2).port == "/dev/ttyACM2"  # SN...09


def test_index_out_of_range_raises(fake_ports, spy_open):
    fake_ports["ports"] = [
        _port("/dev/ttyACM0", DEPZ_USB_VID, PID_SR04, "SN000005"),
    ]
    with pytest.raises(NoDepzDeviceError):
        discovery.open_device(3)


def test_serial_selects_exact(fake_ports, spy_open):
    fake_ports["ports"] = [
        _port("/dev/ttyACM2", DEPZ_USB_VID, PID_SR04, "SN000009"),
        _port("/dev/ttyACM0", DEPZ_USB_VID, PID_BNO086, "SN000005"),
    ]
    dev = discovery.open_device(serial="SN000009")
    assert dev.port == "/dev/ttyACM2"


def test_serial_no_match_raises(fake_ports, spy_open):
    fake_ports["ports"] = [
        _port("/dev/ttyACM0", DEPZ_USB_VID, PID_SR04, "SN000005"),
    ]
    with pytest.raises(NoDepzDeviceError):
        discovery.open_device(serial="SN999999")


def test_no_device_raises(fake_ports, spy_open):
    fake_ports["ports"] = [
        _port("/dev/ttyS0", None, None, None),
        _port("/dev/ttyUSB0", 0x2341, 0x0043, "arduino"),  # some other device
    ]
    with pytest.raises(NoDepzDeviceError):
        discovery.open_device()


def test_str_port_unknown_vid_pid_warns(fake_ports, spy_open):
    fake_ports["ports"] = [
        _port("/dev/ttyUSB0", 0x2341, 0x0043, "arduino"),
    ]
    with pytest.warns(UserWarning):
        dev = discovery.open_device("/dev/ttyUSB0")
    assert dev.port == "/dev/ttyUSB0"
    assert spy_open["opened"] == ["/dev/ttyUSB0"]


def test_str_port_known_no_warning(fake_ports, spy_open, recwarn):
    fake_ports["ports"] = [
        _port("/dev/ttyACM0", DEPZ_USB_VID, PID_SR04, "SN000005"),
    ]
    dev = discovery.open_device("/dev/ttyACM0")
    assert dev.port == "/dev/ttyACM0"
    assert not any(issubclass(w.category, UserWarning) for w in recwarn.list)


def test_none_serial_sorts_last(fake_ports, spy_open):
    fake_ports["ports"] = [
        _port("/dev/ttyACM1", DEPZ_USB_VID, PID_SR04, None),  # no serial
        _port("/dev/ttyACM0", DEPZ_USB_VID, PID_BNO086, "SN000005"),
    ]
    # default picks the one WITH a serial (None sorts last)
    assert discovery.open_device().port == "/dev/ttyACM0"
    assert discovery.open_device(1).port == "/dev/ttyACM1"


def test_dev_default_unit_is_candidate(fake_ports, spy_open):
    fake_ports["ports"] = [
        _port("/dev/ttyACM0", DEV_USB_VID, DEV_USB_PID, "SN000005"),
    ]
    assert discovery.open_device().port == "/dev/ttyACM0"
