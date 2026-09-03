import random

from depz_sensor_sdk.transport import (
    CrcError,
    CrcType,
    Packet,
    PacketParser,
    Trash,
    build_packet,
)


def test_encode_vectors(vectors):
    for case in vectors("framing_encode.json")["cases"]:
        frame = build_packet(
            case["cmd"],
            bytes.fromhex(case["payload"]),
            case["seq"],
            CrcType(case["crc_type"]),
        )
        assert frame.hex() == case["frame"], case["name"]


def _collect(parser: PacketParser, chunks) -> dict:
    events = []
    trash = bytearray()
    for chunk in chunks:
        for ev in parser.feed(chunk):
            if isinstance(ev, Packet):
                events.append(
                    {"type": "packet", "cmd": ev.cmd, "seq": ev.seq, "payload": ev.payload.hex()}
                )
            elif isinstance(ev, CrcError):
                events.append({"type": "crc_error", "cmd": ev.cmd, "seq": ev.seq})
            elif isinstance(ev, Trash):
                trash.extend(ev.data)
    return {
        "events": events,
        "trash": bytes(trash).hex(),
        "residue": bytes(parser._buf).hex(),
        "header_errors": parser.header_errors,
    }


def _random_chunks(stream: bytes, rng: random.Random):
    out = []
    i = 0
    while i < len(stream):
        n = rng.randint(1, 37)
        out.append(stream[i : i + n])
        i += n
    return out


def test_decode_vectors_chunking_invariance(vectors):
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
