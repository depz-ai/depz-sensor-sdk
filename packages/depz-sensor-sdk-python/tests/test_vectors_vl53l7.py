"""Golden-vector consumer: vl53l7.json (contract 11)."""

import pytest

from depz_sensor_sdk.discovery import _vl53l7_class
from depz_sensor_sdk.protocol.vl53l7 import (
    INFO_SIZE,
    Vl53l7Info,
    pack_pin_ctrl,
    pack_set_i2c_speed,
)
from depz_sensor_sdk.protocol.vl53l8 import pack_read_reg, pack_write_reg


def test_encode_vectors(vectors):
    for case in vectors("vl53l7.json")["encode"]:
        kind = case["kind"]
        if kind == "read_reg":
            payload = pack_read_reg(case["addr"], case["len"])
        elif kind == "write_reg":
            payload = pack_write_reg(case["addr"], bytes.fromhex(case["data"]))
        elif kind == "pin_ctrl":
            payload = pack_pin_ctrl(case["action"])
        elif kind == "set_i2c_speed":
            payload = pack_set_i2c_speed(case["khz"])
        else:
            raise AssertionError(f"unknown encode kind {kind!r}")
        assert payload.hex() == case["payload"], case["name"]


def test_decode_vectors(vectors):
    for case in vectors("vl53l7.json")["decode"]:
        assert case["report"] == 0x92, case["name"]
        info = Vl53l7Info.unpack(bytes.fromhex(case["payload"]))
        for field, value in case["expect"].items():
            assert getattr(info, field) == value, (case["name"], field)


def test_info_rejects_a_short_payload():
    with pytest.raises(ValueError):
        Vl53l7Info.unpack(bytes(INFO_SIZE - 1))


def test_model_vectors(vectors):
    for case in vectors("vl53l7.json")["model"]:
        cls = _vl53l7_class(case["usb_model"], case["device_name"])
        assert cls.__name__.lower() == case["expect"], case["name"]
