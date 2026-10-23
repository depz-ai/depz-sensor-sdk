"""Golden-vector consumer: usb_ids.json (identity table + serial ordering)."""

from depz_sensor_sdk.discovery import _PortId, _serial_sort_key
from depz_sensor_sdk.usb_ids import (
    DEPZ_PID_RANGE,
    DEPZ_USB_VID,
    DEV_USB_PID,
    DEV_USB_VID,
    is_known_depz_usb,
    usb_model_hint,
)


def test_usb_constants_vectors(vectors, seen):
    v = vectors("usb_ids.json")
    assert DEPZ_USB_VID == v["vid"]
    seen("usb_ids/vid")
    assert DEV_USB_VID == v["dev_vid"]
    seen("usb_ids/dev_vid")
    assert DEV_USB_PID == v["dev_pid"]
    seen("usb_ids/dev_pid")
    assert list(DEPZ_PID_RANGE) == v["pid_range"]
    seen("usb_ids/pid_range")


def test_usb_identity_vectors(vectors, seen):
    for case in vectors("usb_ids.json")["identity"]:
        vid, pid = case["vid"], case["pid"]
        assert is_known_depz_usb(vid, pid) == case["known"], case
        assert usb_model_hint(vid, pid) == case["model"], case
        seen(case)


def test_usb_serial_ordering_vectors(vectors, seen):
    for case in vectors("usb_ids.json")["serial_ordering"]:
        pis = [_PortId(p["port"], None, None, p["serial"]) for p in case["ports"]]
        pis.sort(key=_serial_sort_key)
        assert [pi.port for pi in pis] == case["order"], case["name"]
        seen(case)
