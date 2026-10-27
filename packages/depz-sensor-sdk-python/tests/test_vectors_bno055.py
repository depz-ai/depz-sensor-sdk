"""Golden-vector consumer: bno055.json (contract 13)."""

from dataclasses import asdict

import pytest

from depz_sensor_sdk.bno055 import regs as R
from depz_sensor_sdk.protocol.bno055 import (
    TRIGGER_INT,
    TRIGGER_TIMER,
    Bno055Info,
    RegData,
    StreamData,
    pack_read_reg,
    pack_start_stream,
    pack_write_reg,
)


def _lists(d: dict) -> dict:
    return {k: (list(v) if isinstance(v, tuple) else v) for k, v in d.items()}


def test_encode_vectors(vectors, seen):
    for c in vectors("bno055.json")["encode"]:
        kind = c["kind"]
        if kind == "read_reg":
            payload = pack_read_reg(c["addr"], c["len"])
        elif kind == "write_reg":
            payload = pack_write_reg(c["addr"], bytes.fromhex(c["data"]))
        elif kind == "start_stream":
            payload = pack_start_stream(c["trigger"], c["addr"], c["len"], c["period_ms"])
        elif kind in ("reset", "stop_stream", "get_info"):
            payload = b""
        else:
            raise AssertionError(f"unknown encode kind {kind!r}")
        assert payload.hex() == c["payload"], c["name"]
        seen(c)


@pytest.mark.parametrize("addr, length", [(0x00, 0), (0x00, 129), (0xFF, 2), (0x80, 129), (0x100, 1), (-1, 1)])
def test_read_reg_refuses_len_outside_1_128_and_past_the_page(addr, length):
    """contract 13 §1 (ERRATA A13): `len` 1..128 and addr+len <= 0x100, checked host-side."""
    with pytest.raises(ValueError):
        pack_read_reg(addr, length)


def test_read_reg_accepts_the_limits():
    assert pack_read_reg(0x00, 128) == bytes.fromhex("0080")
    assert pack_read_reg(0x80, 128) == bytes.fromhex("8080")
    assert pack_read_reg(0xFF, 1) == bytes.fromhex("ff01")


def test_write_reg_refuses_a_write_past_the_page():
    with pytest.raises(ValueError):
        pack_write_reg(0xFF, b"\x01\x02")
    assert pack_write_reg(0xFF, b"\x01") == bytes.fromhex("ff01")


@pytest.mark.parametrize("period_ms", [0, 60001, 65535])
def test_start_stream_timer_period_outside_1_60000_is_refused(period_ms):
    """contract 13 §2 (ERRATA A13): TIMER reads every period_ms, 1..60000."""
    with pytest.raises(ValueError, match="period_ms"):
        pack_start_stream(TRIGGER_TIMER, 0x08, 46, period_ms)


def test_start_stream_timer_limits_and_int_watchdog_zero_are_accepted():
    assert pack_start_stream(TRIGGER_TIMER, 0x08, 46, 1) == bytes.fromhex("00082e0100")
    assert pack_start_stream(TRIGGER_TIMER, 0x08, 46, 60000) == bytes.fromhex("00082e60ea")
    # INT: period_ms is the missed-edge watchdog, 0 = none
    assert pack_start_stream(TRIGGER_INT, 0x08, 6, 0) == bytes.fromhex("0108060000")


def test_decode_vectors(vectors, seen):
    for c in vectors("bno055.json")["decode"]:
        payload = bytes.fromhex(c["payload"])
        if c["report"] == 0x92:
            assert len(payload) == 38
            assert asdict(Bno055Info.unpack(payload)) == c["expect"], c["name"]
        elif c["report"] == 0x91:
            r = RegData.unpack(payload)
            assert (r.cmd, r.timestamp_us, r.data.hex()) == (
                c["expect"]["cmd"],
                c["expect"]["timestamp_us"],
                c["expect"]["data"],
            ), c["name"]
        elif c["report"] == 0x93:
            s = StreamData.unpack(payload)
            assert (s.timestamp_us, s.addr, s.length, s.data.hex()) == (
                c["expect"]["timestamp_us"],
                c["expect"]["addr"],
                c["expect"]["len"],
                c["expect"]["data"],
            ), c["name"]
        else:
            raise AssertionError(f"unknown report {c['report']:#x}")
        seen(c)


def test_units_vectors(vectors, seen):
    for c in vectors("bno055.json")["units"]:
        u = R.Units.unpack(c["unit_sel"])
        assert asdict(u) == c["expect"], c["unit_sel"]
        assert u.pack() == c["repack"]
        seen(c)


def test_calib_stat_vectors(vectors, seen):
    for c in vectors("bno055.json")["calib_stat"]:
        s = R.CalibStatus.unpack(c["value"])
        assert asdict(s) == c["expect"]
        assert s.fully_calibrated == c["fully_calibrated"]
        assert s.pack() == c["value"]
        seen(c)


def test_calibration_profile_vectors(vectors, seen):
    for c in vectors("bno055.json")["calibration_profile"]:
        p = R.CalibrationProfile.unpack(bytes.fromhex(c["bytes"]))
        assert _lists(asdict(p)) == c["expect"], c["name"]
        assert p.pack().hex() == c["bytes"]
        seen(c)


def test_axis_remap_vectors(vectors, seen):
    v = vectors("bno055.json")
    for c in v["axis_remap"]:
        a = R.AxisRemap.unpack(c["config"], c["sign"])
        assert asdict(a) == c["expect"], c["name"]
        assert list(a.pack()) == c["repack"] == [c["config"], c["sign"]]
        assert R.AxisRemap.placement(c["name"]) == a
        seen(c)
    for c in v["axis_remap_invalid"]:
        with pytest.raises(ValueError):
            R.AxisRemap(c["x"], c["y"], c["z"]).pack()
        seen(c)


def test_sensor_config_vectors(vectors, seen):
    sc = vectors("bno055.json")["sensor_config"]
    for c in sc["accel"]:
        a = R.AccelConfig.unpack(c["value"])
        assert asdict(a) == c["expect"]
        assert a.pack() == c["value"]
    for c in sc["gyro"]:
        g = R.GyroConfig.unpack(bytes.fromhex(c["bytes"]))
        assert asdict(g) == c["expect"]
        assert g.pack().hex() == c["bytes"]
    for c in sc["mag"]:
        m = R.MagConfig.unpack(c["value"])
        assert asdict(m) == c["expect"]
        assert m.pack() == c["value"] & 0x7F  # bit 7 reserved
    seen("bno055/sensor_config")


def test_block_vectors(vectors, seen):
    for c in vectors("bno055.json")["blocks"]:
        got = R.decode_block(c["addr"], bytes.fromhex(c["data"]))
        assert _lists(asdict(got)) == c["expect"], c["name"]
        seen(c)
