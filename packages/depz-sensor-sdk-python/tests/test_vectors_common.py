from depz_sensor_sdk.protocol import (
    SequenceErrorReport,
    StatusReport,
    SyncPinConfig,
    TemperatureReport,
    TextReport,
    pack_sync_time,
    sync_time_offset_rtt,
)


def test_encode_vectors(vectors):
    for case in vectors("common_commands.json")["encode"]:
        if case["kind"] == "sync_time_request":
            assert pack_sync_time(case["pc_timestamp_us"]).hex() == case["payload"]
        elif case["kind"] == "set_payload_crc_type":
            assert bytes([case["crc_type"]]).hex() == case["payload"]
        elif case["kind"] == "sync_pin_config":
            cfg = SyncPinConfig(case["pin"], case["mode"], case["polarity"])
            assert cfg.pack().hex() == case["payload"]
        else:
            raise AssertionError(f"unknown encode kind {case['kind']}")


def test_decode_vectors(vectors):
    for case in vectors("common_commands.json")["decode"]:
        payload = bytes.fromhex(case["payload"])
        expect = case["expect"]
        if case["report"] == 0x80:
            rep = StatusReport.unpack(payload)
            assert (rep.cmd, rep.status) == (expect["cmd"], expect["status"])
        elif case["report"] == 0x81:
            rep = TextReport.unpack(payload)
            assert (rep.cmd, rep.text) == (expect["cmd"], expect["text"])
        elif case["report"] == 0x83:
            rep = TemperatureReport.unpack(payload)
            assert rep.timestamp_us == expect["timestamp_us"]
            assert rep.raw_decidegrees == expect["raw_decidegrees"]
        elif case["report"] == 0x84:
            rep = SequenceErrorReport.unpack(payload)
            assert (rep.expected_seq, rep.received_seq) == (
                expect["expected_seq"],
                expect["received_seq"],
            )
        else:
            raise AssertionError(f"unknown report 0x{case['report']:02X}")


def test_sync_time_math(vectors):
    for case in vectors("common_commands.json")["sync_time_math"]:
        off, rtt = sync_time_offset_rtt(case["t1"], case["t2"], case["t3"], case["t4"])
        assert off == case["offset_us"], case["name"]
        assert rtt == case["rtt_us"], case["name"]
