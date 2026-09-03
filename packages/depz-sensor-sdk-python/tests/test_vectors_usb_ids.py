"""Golden-vector consumer: usb_ids.json (identity table + serial ordering)."""

from depz_sensor_sdk.discovery import _PortId, _serial_sort_key
from depz_sensor_sdk.usb_ids import is_known_depz_usb, usb_model_hint


def test_usb_identity_vectors(vectors):
    for case in vectors("usb_ids.json")["identity"]:
        vid, pid = case["vid"], case["pid"]
        assert is_known_depz_usb(vid, pid) == case["known"], case
        assert usb_model_hint(vid, pid) == case["model"], case


def test_usb_serial_ordering_vectors(vectors):
    for case in vectors("usb_ids.json")["serial_ordering"]:
        pis = [_PortId(p["port"], None, None, p["serial"]) for p in case["ports"]]
        pis.sort(key=_serial_sort_key)
        assert [pi.port for pi in pis] == case["order"], case["name"]
