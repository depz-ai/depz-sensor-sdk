"""Detection / discovery: USB-id table + open_device selection logic.

Uses a fake list_ports (monkeypatched) + a fake probe/open so nothing touches
real hardware.
"""

from __future__ import annotations

import stat
import sys
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


# POSIX only: on Windows a COM name is matched by string (a COM port is not a file).
posix_only = pytest.mark.skipif(sys.platform == "win32", reason="port aliases are POSIX")


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
    assert usb_model_hint(DEPZ_USB_VID, 0xED4C) == "tmf8829"
    assert usb_model_hint(DEPZ_USB_VID, 0xED4D) is None  # in-range candidate but unnamed
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


# ── an explicit path names its port under any alias (02 §4.3, ERRATA A5) ─────


@pytest.fixture()
def spy_resolve(monkeypatch):
    """Answer the probe with a VL53L8 / VL53L7 firmware that stamps no part in
    its device name, and record which class opens which port — so only the
    port's USB id can tell CH from CX."""
    calls = {"opened": []}

    def fake_probe(port, timeout):
        return discovery.DeviceInfo(
            port=port, mode="app", sensor_type=None,
            software_name=calls.get("software", "APP_VL53L8_ToF_v1.0"), fw_version="1.0",
            device_name="DEPZ ToF", serial_number="SN1",
        )

    def fake_sensor_class(name):
        def open_(port, *, timeout):
            calls["opened"].append((name, port))
            return SimpleNamespace(cls=name, port=port)
        return open_

    monkeypatch.setattr(discovery, "_probe_port", fake_probe)
    monkeypatch.setattr(discovery, "sensor_class", fake_sensor_class)
    return calls


@pytest.fixture()
def by_id_alias(tmp_path):
    """A `/dev/serial/by-id/…`-style symlink to a port node (a regular file
    stands in for the node: realpath resolves either)."""
    node = tmp_path / "ttyACM0"
    node.write_bytes(b"")
    (tmp_path / "by-id").mkdir()
    alias = tmp_path / "by-id" / "usb-DEPZ_ToF_SN1-if00"
    alias.symlink_to(node)
    return str(node), str(alias)


@posix_only
def test_str_port_symlink_alias_resolves_with_the_enumerated_usb_id(
    fake_ports, spy_resolve, by_id_alias, recwarn
):
    node, alias = by_id_alias
    fake_ports["ports"] = [_port(node, DEPZ_USB_VID, PID_VL53L8CH, "SN1")]
    dev = discovery.open_device(alias)
    # the USB id of the node the alias points at: L8CH, not the CX default;
    # the port opened is the path given, not the node
    assert spy_resolve["opened"] == [("vl53l8ch", alias)]
    assert dev.port == alias
    assert not any(issubclass(w.category, UserWarning) for w in recwarn.list)


@posix_only
def test_str_port_same_device_node_resolves_with_the_enumerated_usb_id(
    fake_ports, spy_resolve, monkeypatch
):
    # Two names of one character device that are not symlinks of each other
    # (a udev rule's extra node): matched by the device number.
    from depz_sensor_sdk.usb_ids import PID_VL53L7CH

    nodes = {"/dev/ttyACM0": 0x10A00, "/dev/depz-tof": 0x10A00, "/dev/ttyACM1": 0x10A01}
    monkeypatch.setattr(discovery, "_device_node", nodes.get)
    fake_ports["ports"] = [
        _port("/dev/ttyACM1", DEPZ_USB_VID, PID_VL53L8CH, "SN2"),
        _port("/dev/ttyACM0", DEPZ_USB_VID, PID_VL53L7CH, "SN1"),
    ]
    spy_resolve["software"] = "APP_VL53L7_ToF_v1.0"
    dev = discovery.open_device("/dev/depz-tof")
    assert spy_resolve["opened"] == [("vl53l7ch", "/dev/depz-tof")]
    assert dev.port == "/dev/depz-tof"


@posix_only
def test_device_node_is_the_char_device_number_or_none(tmp_path):
    regular = tmp_path / "f"
    regular.write_bytes(b"")
    assert discovery._device_node(str(regular)) is None
    assert discovery._device_node(str(tmp_path / "missing")) is None
    assert discovery._device_node("/dev/null") is not None


def test_device_node_is_none_for_a_zero_device_number(monkeypatch):
    # Windows stats an open COM port as a character device with st_rdev == 0:
    # a zero number is no node, else every COM port would be one device.
    zeroed = SimpleNamespace(st_mode=stat.S_IFCHR, st_rdev=0)
    monkeypatch.setattr(discovery.os, "stat", lambda path, *a, **kw: zeroed)
    assert discovery._device_node("COM3") is None


def test_str_port_on_windows_is_matched_by_string_only(fake_ports, spy_resolve, monkeypatch):
    # A COM port is not a file: resolving one (realpath, stat) opens it — a
    # DTR pulse on every board of the bench. On Windows the given name is
    # compared with the enumerated names as a string and nothing else is
    # touched: `\\.\COM10` borrows no USB id from COM3, resolves without one
    # and warns; COM3 itself matches.
    monkeypatch.setattr(discovery.os, "name", "nt")

    def touched(path, *a, **kw):
        raise AssertionError(f"{path} was resolved on Windows")

    monkeypatch.setattr(discovery.os.path, "realpath", touched)
    monkeypatch.setattr(discovery.os, "stat", touched)
    fake_ports["ports"] = [_port("COM3", 0x1234, 0x5678, "SN1"), _port("COM4", DEPZ_USB_VID, PID_VL53L8CH, "SN2")]
    with pytest.warns(UserWarning, match="USB id unknown is not a known DEPZ device"):
        dev = discovery.open_device(r"\\.\COM10")
    assert dev.port == r"\\.\COM10"
    dev = discovery.open_device("COM4")
    assert spy_resolve["opened"] == [("vl53l8cx", r"\\.\COM10"), ("vl53l8ch", "COM4")]
    assert dev.port == "COM4"


def test_str_port_not_enumerated_warns_and_resolves_without_a_usb_id(
    fake_ports, spy_resolve
):
    fake_ports["ports"] = [_port("/dev/ttyACM0", DEPZ_USB_VID, PID_VL53L8CH, "SN1")]
    with pytest.warns(UserWarning, match="USB id unknown is not a known DEPZ device"):
        dev = discovery.open_device("/dev/depz-unlisted")
    assert spy_resolve["opened"] == [("vl53l8cx", "/dev/depz-unlisted")]
    assert dev.port == "/dev/depz-unlisted"


@posix_only
def test_list_depz_devices_annotates_an_aliased_port(fake_ports, spy_resolve, by_id_alias):
    node, alias = by_id_alias
    fake_ports["ports"] = [_port(node, DEPZ_USB_VID, PID_VL53L8CH, "SN1")]
    (info,) = discovery.list_depz_devices(ports=[alias], match_usb=False)
    assert (info.port, info.usb_vid, info.usb_pid, info.usb_serial) == (
        alias, DEPZ_USB_VID, PID_VL53L8CH, "SN1",
    )
    assert info.usb_model_hint == "vl53l8ch"
    # with the USB filter on (default), an alias of a DEPZ port is a candidate
    # too; a path the OS does not list has no USB id and is not probed
    assert discovery.list_depz_devices(ports=[alias]) == [info]
    assert discovery.list_depz_devices(ports=[alias + "-nope"]) == []


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


# ── open_on_link: the §4.4 probe, then the class in place ─────────────────────


def _logging_fake(software_name: str | None = None):
    from fake_device import FakeSr04

    fake = FakeSr04()
    if software_name is not None:
        fake.SOFTWARE_NAME = software_name
    log: list[int] = []
    handle = fake._handle

    def spy(pkt):
        log.append(pkt.cmd)
        handle(pkt)

    fake._handle = spy
    return fake, log


def test_str_port_nobody_answers_is_no_depz_device(fake_ports, monkeypatch):
    """Probe silence on an explicit port is the same condition as on a link —
    `NoDepzDeviceError` for both, never `DeviceLostError` (ERRATA A14)."""
    fake_ports["ports"] = [_port("/dev/mute", DEPZ_USB_VID, PID_SR04, "A")]
    monkeypatch.setattr(discovery, "_probe_port", lambda port, timeout: None)
    with pytest.raises(NoDepzDeviceError, match="no DEPZ device answered on /dev/mute"):
        discovery.open_device("/dev/mute", timeout=0.01)


def test_str_port_that_cannot_be_opened_names_the_open_failure(fake_ports, monkeypatch):
    """A port another process holds stays `NoDepzDeviceError`, but the message
    names the open failure and chains it (contract 07 §3); `probe_port` still
    answers None for it."""
    import serial

    from depz_sensor_sdk.errors import DeviceLostError
    from depz_sensor_sdk.transport import serial_link

    fake_ports["ports"] = [_port("/dev/held", DEPZ_USB_VID, PID_SR04, "A")]

    def refuse(port, *args, **kwargs):
        raise serial.SerialException(f"[Errno 16] could not open port {port}: Device or resource busy")

    monkeypatch.setattr(serial_link.serial, "Serial", refuse)
    with pytest.raises(NoDepzDeviceError) as info:
        discovery.open_device("/dev/held", timeout=0.01)
    msg = str(info.value)
    assert msg.startswith("no DEPZ device answered on /dev/held: could not open the port (")
    assert "Device or resource busy" in msg
    assert isinstance(info.value.__cause__, DeviceLostError)
    assert discovery.probe_port("/dev/held", timeout=0.01) is None


def test_open_on_link_probes_three_requests_then_attaches():
    from depz_sensor_sdk.sr04 import Sr04

    fake, log = _logging_fake()
    dev = discovery.open_on_link(fake.link, DEPZ_USB_VID, PID_SR04, timeout=1.0)
    try:
        assert type(dev) is Sr04
        assert log == [0x04, 0x03, 0x05]  # name, device name, serial — nothing else
        assert dev.get_sample_period_us() == 50_000  # the same connection goes on
    finally:
        dev.close()


def test_open_on_link_unknown_app_stays_base_device():
    from depz_sensor_sdk.device import DeviceBase

    fake, log = _logging_fake("APP_NEWSENSOR_v1.2")
    dev = discovery.open_on_link(fake.link, timeout=1.0)
    try:
        assert type(dev) is DeviceBase
        assert log == [0x04, 0x03, 0x05]
    finally:
        dev.close()


def test_open_on_link_refuses_bootloader():
    from depz_sensor_sdk.errors import DepzError

    fake, _ = _logging_fake("BOOTDEPZ_v1.1")
    with pytest.raises(DepzError, match="bootloader"):
        discovery.open_on_link(fake.link, timeout=1.0)


# ── the remaining open_device targets (02 §4.3) ───────────────────────────────


def test_a_bool_target_is_refused_not_taken_for_index_0_or_1(fake_ports, spy_open):
    fake_ports["ports"] = [_port("/dev/ttyACM0", DEPZ_USB_VID, PID_SR04, "SN000005")]
    with pytest.raises(TypeError, match="None, int, str, DeviceInfo or Link"):
        discovery.open_device(True)
    assert spy_open["opened"] == []


def test_index_counts_among_the_candidates_with_the_serial(fake_ports, spy_open):
    fake_ports["ports"] = [
        _port("/dev/ttyACM0", DEPZ_USB_VID, PID_SR04, "SN000005"),
        _port("/dev/ttyACM1", DEPZ_USB_VID, PID_BNO086, "SN000009"),
    ]
    assert discovery.open_device(0, serial="SN000009").port == "/dev/ttyACM1"
    with pytest.raises(NoDepzDeviceError, match="index 1 .1 candidate"):
        discovery.open_device(1, serial="SN000009")


def test_str_port_with_another_usb_serial_warns_and_opens_it(fake_ports, spy_open):
    fake_ports["ports"] = [_port("/dev/ttyACM0", DEPZ_USB_VID, PID_SR04, "SN000005")]
    with pytest.warns(UserWarning, match="USB serial 'SN000005' != requested 'SN000009'"):
        dev = discovery.open_device("/dev/ttyACM0", serial="SN000009")
    assert dev.port == "/dev/ttyACM0"


def _info(port, software, *, pid=PID_VL53L8CH, usb_serial="SN1"):
    return discovery.DeviceInfo(
        port=port, mode="app", sensor_type=None, software_name=software,
        fw_version="1.0", device_name="DEPZ ToF", serial_number="SN1",
        usb_vid=DEPZ_USB_VID, usb_pid=pid, usb_serial=usb_serial,
    )


def test_a_device_info_opens_the_class_its_answers_and_usb_id_name(fake_ports, spy_resolve):
    # No enumeration, no probe: the DeviceInfo already holds both.
    dev = discovery.open_device(_info("/dev/ttyACM3", "APP_VL53L8_ToF_v1.0"))
    assert (dev.cls, dev.port) == ("vl53l8ch", "/dev/ttyACM3")
    dev = discovery.open_device(_info("/dev/ttyACM4", "APP_VL53L8_ToF_v1.0", pid=PID_VL53L8CX))
    assert dev.cls == "vl53l8cx"
    with pytest.warns(UserWarning, match="requested serial 'SN2' but DeviceInfo serial is 'SN1'"):
        discovery.open_device(_info("/dev/ttyACM3", "APP_VL53L8_ToF_v1.0"), serial="SN2")
    assert spy_resolve["opened"] == [("vl53l8ch", "/dev/ttyACM3"), ("vl53l8cx", "/dev/ttyACM4"),
                                     ("vl53l8ch", "/dev/ttyACM3")]


def test_a_device_info_of_a_bootloader_is_refused(spy_resolve):
    from depz_sensor_sdk.errors import DepzError

    with pytest.raises(DepzError, match="bootloader mode"):
        discovery.open_device(_info("/dev/ttyACM0", "BOOTDEPZ_v1.1"))
    assert spy_resolve["opened"] == []


def test_a_link_target_is_opened_on_that_link():
    from depz_sensor_sdk.sr04 import Sr04

    fake, log = _logging_fake()
    dev = discovery.open_device(fake.link, timeout=1.0)
    try:
        assert type(dev) is Sr04 and log == [0x04, 0x03, 0x05]
    finally:
        dev.close()


def test_the_probe_goes_on_when_device_name_and_serial_fail():
    # 02 §4.4: GET_DEVICE_NAME and GET_SERIAL are optional — an error leaves "".
    fake, log = _logging_fake()
    spy = fake._handle

    def refuse_name_and_serial(pkt):
        if pkt.cmd in (0x03, 0x05):
            log.append(pkt.cmd)
            fake._status(pkt.cmd, 0x02)  # ERR_INVALID_CMD
        else:
            spy(pkt)

    fake._handle = refuse_name_and_serial
    from depz_sensor_sdk.device import DeviceBase

    dev = DeviceBase(fake.link, timeout=1.0)
    try:
        assert discovery._probe(dev) == (fake.SOFTWARE_NAME, "", "")
        assert log == [0x04, 0x03, 0x05]
    finally:
        dev.close()


def test_open_on_link_with_nothing_answering_is_no_depz_device_and_closes_the_link():
    from depz_sensor_sdk.errors import LinkClosedError

    fake, _ = _logging_fake()
    fake._handle = lambda pkt: None  # mute
    with pytest.raises(NoDepzDeviceError, match="no DEPZ device answered on the link"):
        discovery.open_on_link(fake.link, timeout=0.05)
    with pytest.raises(LinkClosedError):
        fake.link.write(b"\x00")
    fake.close()


def test_sensor_class_and_attach_refuse_what_they_cannot_do():
    from depz_sensor_sdk.sr04 import Sr04

    with pytest.raises(ValueError, match="no sensor class 'vl99'"):
        discovery.sensor_class("vl99")
    fake, _ = _logging_fake()
    dev = Sr04(fake.link, timeout=1.0)
    try:
        with pytest.raises(TypeError, match="needs a plain DeviceBase, got Sr04"):
            discovery.attach_sensor_class(dev, "bno086")
    finally:
        dev.close()
        fake.close()
