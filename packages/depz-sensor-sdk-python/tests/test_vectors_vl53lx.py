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


def test_encode_vectors(vectors, seen):
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
        seen(c)


def test_start_stream_refuses_five_clear_steps():
    with pytest.raises(ValueError):
        pack_start_stream(0x0089, 17, ((0x86, 1),) * 5)


def test_decode_vectors(vectors, seen):
    for c in vectors("vl53lx.json")["decode"]:
        assert asdict(Vl53lxInfo.unpack(bytes.fromhex(c["payload"]))) == c["expect"], c["name"]
        seen(c)
    with pytest.raises(ValueError):
        Vl53lxInfo.unpack(bytes(INFO_SIZE - 1))


def test_product_table(vectors, seen):
    rows = vectors("vl53lx.json")["products"]
    assert [r["product"] for r in rows] == list(registry.PRODUCTS)
    for r in rows:
        row = registry.TABLE[r["product"]]
        assert row.model_id == r["model_id"] and row.reach_mm == r["reach_mm"]
        assert list(registry.driver_kinds(r["product"])) == r["driver_kinds"]
        drv = row.drivers[r["default_driver"]]
        assert (drv.ADDR_WIDTH, drv.MAX_KHZ) == (r["addr_width"], r["max_khz"])
        assert [list(s) for s in drv.CLEAR_STEPS] == r["clear_steps"]
        seen(r)


def test_model_vectors(vectors, seen):
    for c in vectors("vl53lx.json")["model"]:
        assert _vl53lx_class(c["usb_model"], c["device_name"]).__name__ == c["expect_class"], c["name"]
        assert registry.product_from_board_name(c["device_name"]) == c["expect_product"], c["name"]
        seen(c)


def test_block_decoders(vectors, seen):
    v = vectors("vl53lx.json")
    for c in v["die_block"]:
        got = D.decode_die_block(bytes.fromhex(c["raw"]), c["variant"])
        assert asdict(got) == c["expect"], c["name"]
        seen(c)
    for c in v["l0x_raw"]:
        assert asdict(D.decode_l0x_raw(bytes.fromhex(c["raw"]))) == c["expect"], c["name"]
        seen(c)
    for c in v["histogram_raw"]:
        got = asdict(D.decode_histogram_raw(bytes.fromhex(c["raw"])))
        got["bins"] = list(got["bins"])
        assert got == c["expect"], c["name"]
        seen(c)


class _TraceDevice:
    """The scripted Vl53Device under a BridgePlatform for the l0x_uld cases:
    every register op the driver issues must be the next one of the vector's
    trace, a read answered with its recorded bytes."""

    def __init__(self, ops: list[dict]) -> None:
        self.ops = ops
        self.next = 0

    def _take(self, op: str, addr: int) -> bytes:
        assert self.next < len(self.ops), f"op #{self.next} beyond the trace: {op} 0x{addr:02X}"
        want = self.ops[self.next]
        self.next += 1
        assert (want["op"], want["addr"]) == (op, addr), (
            f"op #{self.next - 1}: expected {want['op']} 0x{want['addr']:02X}, got {op} 0x{addr:02X}")
        return bytes.fromhex(want["data"])

    def read_reg(self, addr: int, size: int) -> bytes:
        data = self._take("r", addr)
        assert len(data) == size, f"read 0x{addr:02X}: {size} B asked, {len(data)} recorded"
        return data

    def write_reg(self, addr: int, data: bytes) -> None:
        assert self._take("w", addr) == bytes(data), f"write 0x{addr:02X}: {bytes(data).hex()}"


def test_l0x_uld_vectors(vectors, seen):
    """The VL53L0X ULD calls over the recorded register traces (the three ST
    divergences of 2fa4be5), on a fresh driver: the ops must come out in the
    recorded order, the trace must be used up, the host state / error match."""
    import re

    from depz_sensor_sdk.vl53lx._link import Vl53Error
    from depz_sensor_sdk.vl53lx.uld.base import BridgePlatform
    from depz_sensor_sdk.vl53lx.uld.l0x import VL53L0X

    cases = vectors("vl53lx.json")["l0x_uld"]
    assert len(cases) >= 8
    for c in cases:
        kind = c["kind"]
        trace = _TraceDevice(c["trace"])
        drv = VL53L0X(BridgePlatform(trace), "VL53L0X")  # type: ignore[arg-type]

        def call() -> None:
            if kind == "nvm_offset":
                drv.get_info_from_device(c["option"])
            elif kind == "timing_budget":
                drv.set_range_timing(c["timing_budget_ms"], c["inter_measurement_ms"])
            elif kind == "tuning":
                drv.load_tuning_settings(bytes.fromhex(c["buffer"]))
            else:
                raise AssertionError(f"unknown l0x_uld kind {kind!r}")

        if "error" in c:
            with pytest.raises(Vl53Error, match=re.escape(c["error"])):
                call()
        else:
            call()
            d = drv.d
            if kind == "nvm_offset":
                assert {"part2part_offset_adjustment_nvm_um":
                        d["Part2PartOffsetAdjustmentNVMMicroMeter"]} == c["expect"], c["name"]
            elif kind == "tuning":
                assert {"sigma_est_ref_array": d["SigmaEstRefArray"],
                        "sigma_est_eff_pulse_width": d["SigmaEstEffPulseWidth"],
                        "sigma_est_eff_amb_width": d["SigmaEstEffAmbWidth"],
                        "target_ref_rate": d["TargetRefRate"]} == c["expect"], c["name"]
        assert trace.next == len(c["trace"]), f"{c['name']}: {trace.next} of {len(c['trace'])} ops issued"
        seen(c)


def test_l1_uld_vectors(vectors, seen):
    """VL53L1X set_mode over the recorded register traces, on a fresh driver
    (ERRATA A9): the ops come out in the recorded order and use the trace up,
    and the call returns the budget in effect — a budget the new mode does not
    tabulate moved to the nearest one — or fails as the case says."""
    import re

    from depz_sensor_sdk.vl53lx._link import Vl53Error
    from depz_sensor_sdk.vl53lx.uld.base import BridgePlatform
    from depz_sensor_sdk.vl53lx.uld.l1 import VL53L1

    cases = vectors("vl53lx.json")["l1_uld"]
    assert len(cases) >= 3
    for c in cases:
        assert c["kind"] == "set_mode", c["name"]
        trace = _TraceDevice(c["trace"])
        drv = VL53L1(BridgePlatform(trace), "VL53L1CX")  # type: ignore[arg-type]
        if "error" in c:
            with pytest.raises(Vl53Error, match=re.escape(c["error"])):
                drv.set_mode(c["mode"])
        else:
            assert drv.set_mode(c["mode"]) == c["expect"]["timing_budget_ms"], c["name"]
        assert trace.next == len(c["trace"]), f"{c['name']}: {trace.next} of {len(c['trace'])} ops issued"
        seen(c)
