"""Golden-vector consumers: bno086_shtp.json + bno086_reports.json."""

import dataclasses

from depz_sensor_sdk.bno086 import sh2
from depz_sensor_sdk.bno086.reports import (
    GYRO_RV_ANGVEL_Q,
    Q_POINTS,
    RV_ACCURACY_Q,
    parse_gyro_rv_cargo,
    parse_input_cargo,
)
from depz_sensor_sdk.bno086.shtp import ExecutableCommand, ShtpChannel, ShtpHeader, ShtpLayer
from depz_sensor_sdk.protocol.bno086 import Bno086Cmd
from depz_sensor_sdk.transport import build_packet
from depz_sensor_sdk.dataset import bno086_record_value

# ── bno086_shtp.json ──────────────────────────────────────────────────────────


def test_shtp_header_pack_unpack(vectors):
    for case in vectors("bno086_shtp.json")["header"]:
        hdr = ShtpHeader(case["length"], case["channel"], case["seq"], case["continuation"])
        assert hdr.pack().hex() == case["bytes"], case["name"]
        back = ShtpHeader.unpack(bytes.fromhex(case["bytes"]))
        assert back == hdr, case["name"]


def test_shtp_per_channel_tx_seq(vectors):
    layer = ShtpLayer()
    for case in vectors("bno086_shtp.json")["tx_seq"]:
        frame = layer.next_frame(case["channel"], bytes.fromhex(case["payload"]))
        assert frame.hex() == case["frame"]


def test_shtp_reassembly(vectors):
    for case in vectors("bno086_shtp.json")["reassembly"]:
        rx = ShtpLayer()
        cargos = []
        for f in case["frames"]:
            c = rx.feed(bytes.fromhex(f))
            if c is not None:
                cargos.append({"channel": c.channel, "seq": c.seq, "payload": c.payload.hex()})
        assert cargos == case["expect"]["cargos"], case["name"]
        assert rx.discarded == case["expect"]["discarded"], case["name"]


def test_sh2_control_encodes(vectors):
    for case in vectors("bno086_shtp.json")["control_encode"]:
        kind = case["kind"]
        if kind == "set_feature":
            got = sh2.build_set_feature(
                case["sensor_id"],
                case["interval_us"],
                case["batch_us"],
                case["sensitivity"],
                case["flags"],
                case["cfg_word"],
            )
        elif kind == "get_feature_request":
            got = sh2.build_get_feature_request(case["sensor_id"])
        elif kind == "product_id_request":
            got = sh2.build_product_id_request()
        elif kind == "command_request":
            got = sh2.build_command_request(
                case["seq"], case["command"], bytes.fromhex(case["params"])
            )
        elif kind == "frs_read_request":
            got = sh2.build_frs_read_request(
                case["frs_type"], case["offset_words"], case["block_words"]
            )
        elif kind == "frs_write_request":
            got = sh2.build_frs_write_request(case["frs_type"], case["length_words"])
        elif kind == "frs_write_data":
            got = sh2.build_frs_write_data(case["offset_words"], case["words"])
        else:
            raise AssertionError(kind)
        assert got.hex() == case["payload"], case["name"]


def test_executable_sleep_power_on_frames(vectors):
    commands = {"sleep": ExecutableCommand.SLEEP, "power_on": ExecutableCommand.ON}
    layer = ShtpLayer()  # fresh: sleep then power_on → channel-1 seq 0, 1
    for case in vectors("bno086_shtp.json")["executable"]:
        command = commands[case["name"]]
        assert int(command) == case["command"], case["name"]
        assert case["channel"] == ShtpChannel.EXECUTABLE
        payload = bytes((command,))
        assert payload.hex() == case["payload"], case["name"]
        frame = layer.next_frame(ShtpChannel.EXECUTABLE, payload)
        assert frame.hex() == case["shtp_frame"], case["name"]
        assert case["bridge_cmd"] == Bno086Cmd.SEND_SHTP_PACKET
        bridge = build_packet(case["bridge_cmd"], frame, case["bridge_seq"], case["crc_type"])
        assert bridge.hex() == case["bridge_frame"], case["name"]


def test_feature_flag_bits(vectors):
    flags = vectors("bno086_shtp.json")["feature_flags"]
    assert flags == {"wakeup": sh2.FeatureFlag.WAKEUP, "always_on": sh2.FeatureFlag.ALWAYS_ON}


# ── bno086_reports.json ───────────────────────────────────────────────────────


def _serialize(rep) -> dict:
    fields = {}
    for k, v in dataclasses.asdict(rep).items():
        if isinstance(v, bytes):
            v = v.hex()
        elif isinstance(v, tuple):
            v = list(v)
        fields[k] = int(v) if isinstance(v, bool) else v
    # `record`: the contract 09 dataset record value the recorder writes
    return {"type": type(rep).__name__, "fields": fields, "record": bno086_record_value(rep)}


def test_q_point_table_matches(vectors):
    data = vectors("bno086_reports.json")
    assert {f"0x{int(k):02X}": v for k, v in Q_POINTS.items()} == data["q_points"]
    assert data["rv_accuracy_q"] == RV_ACCURACY_Q
    assert data["gyro_rv_angvel_q"] == GYRO_RV_ANGVEL_Q


def test_input_cargo_decode_vectors(vectors):
    for case in vectors("bno086_reports.json")["input_cargos"]:
        reps = parse_input_cargo(bytes.fromhex(case["cargo"]), case["capture_timestamp_us"])
        assert [_serialize(r) for r in reps] == case["expect"], case["name"]


def test_gyro_rv_decode_vectors(vectors):
    for case in vectors("bno086_reports.json")["gyro_rv"]:
        rep = parse_gyro_rv_cargo(bytes.fromhex(case["cargo"]), case["capture_timestamp_us"])
        assert rep is not None, case["name"]
        assert _serialize(rep) == case["expect"], case["name"]
