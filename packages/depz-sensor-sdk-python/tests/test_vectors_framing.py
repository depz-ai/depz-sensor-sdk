import random

from depz_sensor_sdk.transport import CrcType, PacketParser, build_packet
from vector_shapes import framing_collect as _collect


def test_encode_vectors(vectors, seen):
    for case in vectors("framing_encode.json")["cases"]:
        frame = build_packet(
            case["cmd"],
            bytes.fromhex(case["payload"]),
            case["seq"],
            CrcType(case["crc_type"]),
        )
        assert frame.hex() == case["frame"], case["name"]
        seen(case)


def _random_chunks(stream: bytes, rng: random.Random):
    out = []
    i = 0
    while i < len(stream):
        n = rng.randint(1, 37)
        out.append(stream[i : i + n])
        i += n
    return out


def test_decode_vectors_chunking_invariance(vectors, seen):
    """Contract 01 §5: identical results whole, byte-by-byte, random splits."""
    for case in vectors("framing_decode.json")["cases"]:
        stream = bytes.fromhex(case["stream"])
        expect = case["expect"]

        whole = _collect(PacketParser(), [stream])
        assert whole == expect, f"{case['name']}: whole-feed mismatch"

        bytewise = _collect(PacketParser(), [stream[i : i + 1] for i in range(len(stream))])
        assert bytewise == expect, f"{case['name']}: byte-by-byte mismatch"

        rng = random.Random(0xDE92)
        for round_ in range(3):
            split = _collect(PacketParser(), _random_chunks(stream, rng))
            assert split == expect, f"{case['name']}: random-split round {round_}"
        seen(case)
