"""Golden-vector consumer: vl53lx.json (contract 12)."""

from dataclasses import asdict

import pytest

from depz_sensor_sdk.discovery import _vl53lx_class
from depz_sensor_sdk.protocol.vl53lx import (
    INFO_SIZE,
    Vl53lxInfo,
    pack_read_reg,
    pack_set_addr_width,
    pack_set_i2c_speed,
    pack_start_stream,
    pack_write_reg,
    pack_xshut,
)
from depz_sensor_sdk.vl53lx import decode as D
from depz_sensor_sdk.vl53lx.uld import registry


def test_encode_vectors(vectors):
    for c in vectors("vl53lx.json")["encode"]:
        kind = c["kind"]
        if kind == "set_addr_width":
            payload = pack_set_addr_width(c["width"])
        elif kind == "read_reg":
            payload = pack_read_reg(c["addr"], c["len"])
        elif kind == "write_reg":
            payload = pack_write_reg(c["addr"], bytes.fromhex(c["data"]))
        elif kind == "xshut":
            payload = pack_xshut(c["action"])
        elif kind == "set_i2c_speed":
            payload = pack_set_i2c_speed(c["khz"])
        elif kind == "start_stream":
            clear = tuple((a, v) for a, v in c["clear"])
            payload = pack_start_stream(c["addr"], c["len"], clear, c["flags"])
        else:
            raise AssertionError(f"unknown encode kind {kind!r}")
        assert payload.hex() == c["payload"], c["name"]


def test_start_stream_refuses_five_clear_steps():
    with pytest.raises(ValueError):
        pack_start_stream(0x0089, 17, ((0x86, 1),) * 5)


def test_decode_vectors(vectors):
    for c in vectors("vl53lx.json")["decode"]:
        assert asdict(Vl53lxInfo.unpack(bytes.fromhex(c["payload"]))) == c["expect"], c["name"]
    with pytest.raises(ValueError):
        Vl53lxInfo.unpack(bytes(INFO_SIZE - 1))


def test_product_table(vectors):
    rows = vectors("vl53lx.json")["products"]
    assert [r["product"] for r in rows] == list(registry.PRODUCTS)
    for r in rows:
        row = registry.TABLE[r["product"]]
        assert row.model_id == r["model_id"] and row.reach_mm == r["reach_mm"]
        assert list(registry.driver_kinds(r["product"])) == r["driver_kinds"]
        drv = row.drivers[r["default_driver"]]
        assert (drv.ADDR_WIDTH, drv.MAX_KHZ) == (r["addr_width"], r["max_khz"])
        assert [list(s) for s in drv.CLEAR_STEPS] == r["clear_steps"]


def test_model_vectors(vectors):
    for c in vectors("vl53lx.json")["model"]:
        assert _vl53lx_class(c["usb_model"], c["device_name"]).__name__ == c["expect_class"], c["name"]
        assert registry.product_from_board_name(c["device_name"]) == c["expect_product"], c["name"]


def test_block_decoders(vectors):
    v = vectors("vl53lx.json")
    for c in v["die_block"]:
        got = D.decode_die_block(bytes.fromhex(c["raw"]), c["variant"])
        assert asdict(got) == c["expect"], c["name"]
    for c in v["l0x_raw"]:
        assert asdict(D.decode_l0x_raw(bytes.fromhex(c["raw"]))) == c["expect"], c["name"]
    for c in v["histogram_raw"]:
        got = asdict(D.decode_histogram_raw(bytes.fromhex(c["raw"])))
        got["bins"] = list(got["bins"])
        assert got == c["expect"], c["name"]
