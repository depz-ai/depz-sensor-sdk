"""Golden-vector consumer: vl53l4.json (contract 10)."""

from depz_sensor_sdk.protocol.vl53l4 import (
    RegData,
    StreamData,
    Vl53l4Info,
    pack_read_reg,
    pack_set_i2c_speed,
    pack_start_stream,
    pack_write_reg,
    pack_xshut,
)
from depz_sensor_sdk.vl53l4 import uld


def test_encode_vectors(vectors):
    for case in vectors("vl53l4.json")["encode"]:
        kind = case["kind"]
        if kind == "read_reg":
            payload = pack_read_reg(case["addr"], case["len"])
        elif kind == "write_reg":
            payload = pack_write_reg(case["addr"], bytes.fromhex(case["data"]))
        elif kind == "xshut":
            payload = pack_xshut(case["action"])
        elif kind == "start_stream":
            payload = pack_start_stream(case["addr"], case["len"], case["flags"])
        elif kind == "set_i2c_speed":
            payload = pack_set_i2c_speed(case["khz"])
        else:
            raise AssertionError(f"unknown encode kind {kind!r}")
        assert payload.hex() == case["payload"], case["name"]


def test_decode_vectors(vectors):
    for case in vectors("vl53l4.json")["decode"]:
        payload = bytes.fromhex(case["payload"])
        expect = case["expect"]
        if case["report"] == 0x91:
            rd = RegData.unpack(payload)
            assert rd.cmd == expect["cmd"], case["name"]
            assert rd.timestamp_us == expect["timestamp_us"], case["name"]
            assert rd.data.hex() == expect["data"], case["name"]
        elif case["report"] == 0x92:
            info = Vl53l4Info.unpack(payload)
            for field, value in expect.items():
                assert getattr(info, field) == value, (case["name"], field)
        elif case["report"] == 0x93:
            sd = StreamData.unpack(payload)
            assert sd.timestamp_us == expect["timestamp_us"], case["name"]
            assert sd.addr == expect["addr"], case["name"]
            assert sd.length == expect["len"], case["name"]
            assert sd.data.hex() == expect["data"], case["name"]
        else:
            raise AssertionError(f"unknown report {case['report']:#x}")


def test_result_block_vectors(vectors):
    for case in vectors("vl53l4.json")["result_block"]:
        r = uld.parse_result_block(bytes.fromhex(case["raw"]))
        for field, value in case["expect"].items():
            assert getattr(r, field) == value, (case["name"], field)


def test_timing_encode_vectors(vectors):
    for case in vectors("vl53l4.json")["timing"]["encode"]:
        a, b, im = uld.range_timing_registers(
            case["timing_budget_ms"],
            case["inter_measurement_ms"],
            case["osc_frequency"],
            case["clock_pll"],
        )
        assert a == case["range_config_a"], case["name"]
        assert b == case["range_config_b"], case["name"]
        assert im == case["intermeasurement_raw"], case["name"]


def test_timing_decode_vectors(vectors):
    for case in vectors("vl53l4.json")["timing"]["decode"]:
        budget, inter = uld.decode_range_timing(
            case["intermeasurement_raw"],
            case["clock_pll"],
            case["osc_frequency"],
            case["range_config_a"],
        )
        assert budget == case["timing_budget_ms"], case["name"]
        assert inter == case["inter_measurement_ms"], case["name"]


def test_tuning_vectors(vectors):
    encoders = {
        "offset": uld.offset_raw,
        "xtalk": uld.xtalk_raw,
        "signal_threshold": uld.signal_threshold_raw,
        "sigma_threshold": uld.sigma_threshold_raw,
    }
    decoders = {
        "offset": uld.decode_offset,
        "xtalk": uld.decode_xtalk,
        "signal_threshold": uld.decode_signal_threshold,
        "sigma_threshold": uld.decode_sigma_threshold,
    }
    for case in vectors("vl53l4.json")["tuning"]:
        assert encoders[case["kind"]](case["value"]) == case["raw"], case["name"]
        assert decoders[case["kind"]](case["raw"]) == case["value"], case["name"]


def test_config_block_vector(vectors):
    block = vectors("vl53l4.json")["config_block"]
    assert block["addr"] == uld.CONFIG_ADDR
    assert uld.config_block().hex() == block["data"]
    # byte 0 is the FM+ pad override; the tail matches ST's stock table
    assert uld.config_block()[0] == uld.CONFIG_FMP_BYTE
    assert uld.config_block()[1:] == uld.DEFAULT_CONFIGURATION[1:]
