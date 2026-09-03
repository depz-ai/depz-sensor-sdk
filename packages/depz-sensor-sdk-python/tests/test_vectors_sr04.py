from depz_sensor_sdk.protocol.sr04 import (
    Sr04Data,
    pack_echo_decay,
    pack_sample_period,
    unpack_echo_decay,
    unpack_sample_period,
)


def test_sr04_encode_vectors(vectors):
    for case in vectors("sr04.json")["encode"]:
        if case["kind"] == "set_sample_period":
            assert pack_sample_period(case["period_us"]).hex() == case["payload"]
        elif case["kind"] == "set_echo_decay":
            assert pack_echo_decay(case["decay_us"]).hex() == case["payload"]
        else:
            raise AssertionError(case["kind"])


def test_sr04_decode_vectors(vectors):
    for case in vectors("sr04.json")["decode"]:
        payload = bytes.fromhex(case["payload"])
        expect = case["expect"]
        if case["report"] == 0x91:
            d = Sr04Data.unpack(payload)
            assert d.source_cmd == expect["source_cmd"], case["name"]
            assert d.timestamp_us == expect["timestamp_us"], case["name"]
            assert d.echo_time_us == expect["echo_time_us"], case["name"]
        elif case["report"] == 0x92:
            assert unpack_sample_period(payload) == expect["period_us"]
        elif case["report"] == 0x93:
            assert unpack_echo_decay(payload) == expect["decay_us"]
        else:
            raise AssertionError(hex(case["report"]))
