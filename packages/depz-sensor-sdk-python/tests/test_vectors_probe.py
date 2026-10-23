"""Golden-vector consumer: probe.json (class resolution, contract 02 §4.4)."""

from depz_sensor_sdk.discovery import SENSOR_CLASS_NAMES, resolve_sensor_class, sensor_class


def test_probe_class_names(vectors, seen):
    v = vectors("probe.json")
    assert list(SENSOR_CLASS_NAMES) == v["classes"]
    for name in SENSOR_CLASS_NAMES:  # every name but the bootloader has a class
        if name != "bootloader":
            sensor_class(name)
    seen("probe/classes")


def test_probe_resolve_vectors(vectors, seen):
    for case in vectors("probe.json")["resolve"]:
        got = resolve_sensor_class(
            case["software_name"], case["device_name"], case["usb_vid"], case["usb_pid"]
        )
        assert got == case["class"], case["id"]
        seen(case)
