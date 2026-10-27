"""Golden-vector consumer: vl53l8.json (contract 04 §2–§4, the streaming side
of the SPI register bridge): START_STREAM encode, RPT_VL53_FRAME chunk decode
and the FrameReassembler rules over chunks a live board emitted."""

import pytest

from depz_sensor_sdk.protocol.vl53l8 import FrameChunk, Vl53l8Rpt, pack_start_stream
from vector_shapes import vl53l8_decode, vl53l8_reassembly


def test_encode_vectors(vectors, seen):
    for case in vectors("vl53l8.json")["encode"]:
        assert case["kind"] == "start_stream", case["name"]
        assert pack_start_stream(case["frame_size"]).hex() == case["payload"], case["name"]
        seen(case)


def test_decode_vectors(vectors, seen):
    for case in vectors("vl53l8.json")["decode"]:
        assert case["report"] == Vl53l8Rpt.VL53_FRAME, case["name"]
        assert vl53l8_decode(case["report"], bytes.fromhex(case["payload"])) == case["expect"], case["name"]
        seen(case)


def test_frame_chunk_rejects_a_short_payload():
    with pytest.raises(ValueError):
        FrameChunk.unpack(bytes(11))
    assert FrameChunk.unpack(bytes(12)).data == b""


def test_reassembly_vectors(vectors, seen):
    for case in vectors("vl53l8.json")["reassembly"]:
        got = vl53l8_reassembly(bytes.fromhex(ch) for ch in case["chunks"])
        assert got == case["expect"], case["name"]
        seen(case)
