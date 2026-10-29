"""Property-based tests of the reference parsers (hypothesis).

What a golden vector cannot say — "for EVERY input" — is said here for the
byte-level decoders: the framing parser and the CRCs (contract 01), the
VL53L8 chunk reassembler (contract 04 §4), the BNO086 SHTP layer and SH-2
cargo decoders (contract 05 §3–§4), the `.depzrec` replay reader (contract
08) and the `.depzdata` reader (contract 09).

Two kinds of property:

* **round trip** — what the SDK encodes, its decoder gives back unchanged,
  whatever the chunking, frame size or interleaving;
* **arbitrary input never crashes** — a decoder fed corrupted, shuffled,
  duplicated or truncated input either returns or raises one of the SDK's
  own error kinds (contract 07 §3): `DepzError`, or `ValueError` for a
  caller's bad argument. Anything else (`struct.error`, `KeyError`,
  `IndexError`, `TypeError`...) is a crash — and a bug.

The example counts are bounded (`_PROFILE`) so the whole file adds a few
seconds to the suite; it is deterministic (`derandomize`) so CI does not
flake. To hunt with more examples run it with
`HYPOTHESIS_PROFILE=hunt uv run pytest tests/test_property_parsers.py`.
"""

from __future__ import annotations

import binascii
import json
import os
import struct
import zlib
from pathlib import Path

import pytest
from hypothesis import HealthCheck, given, settings
from hypothesis import strategies as st

from depz_sensor_sdk.bno086 import sh2
from depz_sensor_sdk.bno086.reports import (
    BASE_TIMESTAMP_REF,
    FLUSH_COMPLETED,
    REPORT_LENGTHS,
    TIMESTAMP_REBASE,
    UnknownReport,
    parse_gyro_rv_cargo,
    parse_input_cargo,
)
from depz_sensor_sdk.bno086.shtp import (
    NUM_CHANNELS,
    SHTP_HEADER_SIZE,
    ShtpHeader,
    ShtpLayer,
    fragment_cargo,
)
from depz_sensor_sdk.dataset import DatasetReader, DatasetWriter
from depz_sensor_sdk.errors import DepzError, ReplayMismatchError
from depz_sensor_sdk.protocol.vl53l8 import (
    STREAM_CHUNK_MAX,
    STREAM_TOTAL_MAX,
    FrameChunk,
    FrameReassembler,
)
from depz_sensor_sdk.transport import (
    HEADER_SIZE,
    MAX_PAYLOAD,
    CrcType,
    Packet,
    PacketParser,
    Trash,
    build_packet,
)
from depz_sensor_sdk.transport.crc import (
    crc8_maxim,
    crc16_ccitt_false,
    crc16_modbus,
    crc32_iso_hdlc,
)
from depz_sensor_sdk.transport.link import Link
from depz_sensor_sdk.transport.record_replay import RecordingLink, ReplayLink
from vector_shapes import framing_collect

settings.register_profile(
    "ci", max_examples=60, deadline=None, derandomize=True, database=None,
    suppress_health_check=[HealthCheck.too_slow],
)
settings.register_profile("hunt", max_examples=3000, deadline=None, database=None)
settings.load_profile(os.environ.get("HYPOTHESIS_PROFILE", "ci"))

#: The error kinds a decoder may raise on bad input (contract 07 §3).
SDK_ERRORS = (DepzError, ValueError)

_u8 = st.integers(0, 0xFF)
_u16 = st.integers(0, 0xFFFF)
_u64 = st.integers(0, (1 << 64) - 1)


def _chunked(data: bytes, cuts: list[int]) -> list[bytes]:
    """`data` split at the (sorted, deduplicated) `cuts`; empty chunks kept."""
    out, prev = [], 0
    for c in sorted(set(cuts)):
        out.append(data[prev:c])
        prev = c
    out.append(data[prev:])
    return out


def _cuts(n: int) -> st.SearchStrategy[list[int]]:
    return st.lists(st.integers(0, n), max_size=12)


# ── CRC (contract 01 §3) ─────────────────────────────────────────────────────


def _crc_bitwise(data: bytes, poly_reflected: int, init: int, xorout: int, width: int) -> int:
    """The textbook bit-at-a-time reflected CRC — the oracle for the tables."""
    crc = init
    for b in data:
        crc ^= b
        for _ in range(8):
            crc = (crc >> 1) ^ poly_reflected if crc & 1 else crc >> 1
    return (crc ^ xorout) & ((1 << width) - 1)


@given(st.binary(max_size=600))
def test_crc_tables_match_the_bitwise_definition(data: bytes):
    assert crc8_maxim(data) == _crc_bitwise(data, 0x8C, 0x00, 0x00, 8)
    assert crc16_modbus(data) == _crc_bitwise(data, 0xA001, 0xFFFF, 0x0000, 16)
    assert crc32_iso_hdlc(data) == _crc_bitwise(data, 0xEDB88320, 0xFFFFFFFF, 0xFFFFFFFF, 32)
    assert crc32_iso_hdlc(data) == zlib.crc32(data)  # the same algorithm by another name
    assert crc16_ccitt_false(data) == binascii.crc_hqx(data, 0xFFFF)


# ── framing (contract 01 §2, §5) ─────────────────────────────────────────────

_crc_type = st.sampled_from(list(CrcType))
_payload = st.one_of(st.binary(max_size=64), st.binary(min_size=1, max_size=2400))


@st.composite
def _packet(draw) -> tuple[Packet, bytes]:
    cmd, seq = draw(_u8), draw(st.integers(0, 0xFFFFFFFF))
    payload, crc = draw(_payload), draw(_crc_type)
    return Packet(cmd, seq & 0xFF, payload), build_packet(cmd, payload, seq, crc)


@given(_packet(), st.data())
def test_framing_round_trip_under_any_chunking(packet, data):
    expect, frame = packet
    parser = PacketParser()
    events = [e for chunk in _chunked(frame, data.draw(_cuts(len(frame)))) for e in parser.feed(chunk)]
    assert events == [expect]
    assert not parser._buf and parser.header_errors == 0 and parser.trash_bytes == 0


@given(st.lists(_packet(), max_size=6), st.data())
def test_framing_back_to_back_packets_decode_in_order(packets, data):
    stream = b"".join(frame for _, frame in packets)
    parser = PacketParser()
    events = [e for chunk in _chunked(stream, data.draw(_cuts(len(stream)))) for e in parser.feed(chunk)]
    assert events == [p for p, _ in packets]
    assert not parser._buf and parser.trash_bytes == 0


@given(st.binary(max_size=40).filter(lambda b: 0xA5 not in b), _packet())
def test_framing_garbage_without_magic_is_trash_then_the_packet(prefix, packet):
    expect, frame = packet
    events = PacketParser().feed(prefix + frame)
    assert events[-1] == expect
    assert b"".join(e.data for e in events[:-1] if isinstance(e, Trash)) == prefix
    assert all(isinstance(e, Trash) for e in events[:-1])


@st.composite
def _mutated(draw, frame: bytes) -> bytes:
    """A frame with one byte flipped, a span cut out, or a tail dropped."""
    kind = draw(st.sampled_from(["flip", "cut", "truncate"]))
    if kind == "flip" and frame:
        i = draw(st.integers(0, len(frame) - 1))
        return frame[:i] + bytes((frame[i] ^ draw(st.integers(1, 0xFF)),)) + frame[i + 1 :]
    if kind == "cut" and len(frame) > 1:
        a = draw(st.integers(0, len(frame) - 1))
        return frame[:a] + frame[a + draw(st.integers(1, len(frame) - a)) :]
    return frame[: draw(st.integers(0, len(frame)))]


@st.composite
def _byte_stream(draw) -> bytes:
    """Random bytes, intact frames and damaged frames, in any order."""
    parts = draw(
        st.lists(
            st.one_of(
                st.binary(max_size=24),
                _packet().map(lambda p: p[1]),
                _packet().flatmap(lambda p: _mutated(p[1])),
            ),
            max_size=6,
        )
    )
    return b"".join(parts)


@given(_byte_stream(), st.data())
def test_framing_arbitrary_bytes_never_crash_and_chunking_is_invariant(stream, data):
    whole = framing_collect(PacketParser(), [stream])
    split = framing_collect(PacketParser(), _chunked(stream, data.draw(_cuts(len(stream)))))
    assert split == whole
    # The parser never buffers more than one maximal frame minus a byte.
    assert len(bytes.fromhex(whole["residue"])) < HEADER_SIZE + MAX_PAYLOAD + 4
    for ev in whole["events"]:
        if ev["type"] == "packet":
            assert len(bytes.fromhex(ev["payload"])) <= MAX_PAYLOAD


# ── VL53L8 chunk reassembly (contract 04 §4) ─────────────────────────────────


def _frame_chunks(ts: int, frame: bytes, sizes: list[int]) -> list[FrameChunk]:
    """`frame` as the firmware streams it: consecutive chunks of ≤ 1528 B."""
    out, off = [], 0
    for n in sizes:
        out.append(FrameChunk(ts, len(frame), off, frame[off : off + n]))
        off += n
    return out


@st.composite
def _streamed_frame(draw) -> tuple[int, bytes, list[FrameChunk]]:
    size = draw(st.integers(1, STREAM_TOTAL_MAX))
    frame = draw(st.binary(min_size=size, max_size=size))
    sizes, left = [], size
    while left:
        n = draw(st.integers(1, min(left, STREAM_CHUNK_MAX)))
        sizes.append(n)
        left -= n
    ts = draw(_u64)
    return ts, frame, _frame_chunks(ts, frame, sizes)


@given(st.lists(_streamed_frame(), min_size=1, max_size=4))
def test_reassembler_rebuilds_streamed_frames_back_to_back(frames):
    r = FrameReassembler()
    done = []
    for ts, frame, chunks in frames:
        for i, chunk in enumerate(chunks):
            out = r.feed(chunk)
            assert (out is not None) == (i == len(chunks) - 1), "a frame completes on its last chunk only"
            if out is not None:
                done.append(out)
    assert done == [(ts, frame) for ts, frame, _ in frames]
    assert r.completed == len(frames) and r.discarded == 0


@st.composite
def _chunk_soup(draw) -> list[FrameChunk]:
    """A frame's chunks shuffled, duplicated and truncated, plus random ones."""
    _, _, chunks = draw(_streamed_frame())
    soup = draw(st.lists(st.sampled_from(chunks), max_size=8)) if chunks else []
    soup += draw(
        st.lists(
            st.builds(FrameChunk, _u64, _u16, _u16, st.binary(max_size=STREAM_CHUNK_MAX)), max_size=3
        )
    )
    return draw(st.permutations(soup))


@given(_chunk_soup(), _streamed_frame())
def test_reassembler_survives_chunk_soup_and_recovers_on_the_next_frame(soup, fresh):
    r = FrameReassembler()
    for chunk in soup:
        out = r.feed(chunk)
        if out is not None:
            ts, frame = out
            assert len(frame) == chunk.full_size and ts == chunk.timestamp_us
    assert r.completed + r.discarded <= len(soup)
    # Rule 1: offset 0 restarts, so a clean frame after any mess completes.
    ts, frame, chunks = fresh
    outs = [r.feed(c) for c in chunks]
    assert outs[:-1] == [None] * (len(chunks) - 1) and outs[-1] == (ts, frame)


@given(st.binary(max_size=40))
def test_frame_chunk_unpack_rejects_short_payloads_with_the_sdk_error(payload):
    try:
        chunk = FrameChunk.unpack(payload)
    except SDK_ERRORS:
        assert len(payload) < 12
    else:
        assert len(payload) >= 12 and chunk.data == payload[12:]


# ── BNO086 SHTP layer (contract 05 §3) ───────────────────────────────────────

_channel = st.integers(0, NUM_CHANNELS - 1)


@st.composite
def _cargo(draw) -> tuple[int, bytes, int, int, list[bytes]]:
    """(channel, payload, seq_start, capture_us, frames) for one cargo."""
    channel, seq = draw(_channel), draw(_u8)
    payload = draw(st.binary(max_size=1200))
    max_frame = draw(st.integers(SHTP_HEADER_SIZE + 1, 64))
    capture = draw(st.integers(0, (1 << 63) - 1))  # room for the per-frame offsets added below
    return channel, payload, seq, capture, fragment_cargo(channel, payload, seq, max_frame)


@given(_cargo())
def test_shtp_fragments_reassemble_to_the_cargo(cargo):
    channel, payload, seq, capture, frames = cargo
    rx = ShtpLayer()
    outs = [rx.feed(f, capture + 1250 * i) for i, f in enumerate(frames)]
    assert outs[:-1] == [None] * (len(frames) - 1)
    done = outs[-1]
    assert done is not None
    assert (done.channel, done.seq, done.payload, done.capture_us) == (channel, seq, payload, capture)
    assert rx.discarded == 0
    hdrs = [ShtpHeader.unpack(f) for f in frames]
    assert [h.seq for h in hdrs] == [(seq + i) & 0xFF for i in range(len(frames))]
    assert [h.continuation for h in hdrs] == [i > 0 for i in range(len(frames))]
    assert hdrs[0].length == SHTP_HEADER_SIZE + len(payload)


@given(_cargo(), _cargo(), st.data())
def test_shtp_channels_reassemble_independently_when_interleaved(a, b, data):
    ch_a, payload_a, _, _, frames_a = a
    ch_b, payload_b, _, _, frames_b = b
    if ch_a == ch_b:
        return  # same channel: a new first fragment restarts (tested below)
    order = data.draw(st.permutations(["a"] * len(frames_a) + ["b"] * len(frames_b)))
    rx, done, first = ShtpLayer(), {}, {}
    it = {"a": iter(frames_a), "b": iter(frames_b)}
    for i, which in enumerate(order):
        # Each frame has its own RPT_DATA capture time; a cargo must carry
        # its FIRST fragment's (contract 05 §3), whichever channel came between.
        first.setdefault(which, 1_000_000 + 7 * i)
        c = rx.feed(next(it[which]), 1_000_000 + 7 * i)
        if c is not None:
            done[which] = c
    assert (done["a"].channel, done["a"].payload, done["a"].capture_us) == (ch_a, payload_a, first["a"])
    assert (done["b"].channel, done["b"].payload, done["b"].capture_us) == (ch_b, payload_b, first["b"])
    assert rx.discarded == 0


@st.composite
def _frame_soup(draw) -> list[bytes]:
    _, _, _, _, frames = draw(_cargo())
    soup = draw(st.lists(st.sampled_from(frames), max_size=6))
    soup += draw(st.lists(st.binary(max_size=70), max_size=4))
    return draw(st.permutations(soup))


@given(_frame_soup(), _cargo())
def test_shtp_survives_frame_soup_and_recovers_on_the_next_cargo(soup, fresh):
    rx = ShtpLayer()
    for i, frame in enumerate(soup):
        c = rx.feed(frame, i)
        if c is not None:
            hdr = ShtpHeader.unpack(frame)
            assert c.channel == hdr.channel and len(c.payload) + SHTP_HEADER_SIZE >= hdr.length
    channel, payload, seq, capture, frames = fresh
    outs = [rx.feed(f, capture) for f in frames]
    assert outs[:-1] == [None] * (len(frames) - 1)
    assert outs[-1] is not None and (outs[-1].payload, outs[-1].seq) == (payload, seq)


# ── BNO086 SH-2 cargo decoders (contract 05 §4) ──────────────────────────────

_input_ids = sorted(REPORT_LENGTHS)


@st.composite
def _input_cargo(draw) -> tuple[bytes, int]:
    """A well-formed channel-3 cargo: 0xFB base, then whole reports."""
    parts = [bytes((BASE_TIMESTAMP_REF,)) + struct.pack("<i", draw(st.integers(-5000, 5000)))]
    n = 0
    for _ in range(draw(st.integers(0, 6))):
        kind = draw(st.sampled_from(["report", "report", "rebase", "flush"]))
        if kind == "report":
            rid = draw(st.sampled_from(_input_ids))
            parts.append(bytes((rid,)) + draw(st.binary(min_size=REPORT_LENGTHS[rid] - 1, max_size=REPORT_LENGTHS[rid] - 1)))
            n += 1
        elif kind == "rebase":
            parts.append(bytes((TIMESTAMP_REBASE,)) + struct.pack("<i", draw(st.integers(-500, 500))))
        else:
            parts.append(bytes((FLUSH_COMPLETED, draw(_u8))))
            n += 1
    return b"".join(parts), n


@given(_input_cargo(), st.integers(10_000_000, 1 << 40))
def test_sh2_well_formed_input_cargo_decodes_every_report(cargo, capture):
    payload, n = cargo
    reports = parse_input_cargo(payload, capture)
    assert len(reports) == n
    assert not any(isinstance(r, UnknownReport) for r in reports)


@given(st.binary(max_size=80), _u64)
def test_sh2_arbitrary_input_cargo_never_crashes(payload, capture):
    reports = parse_input_cargo(payload, capture)
    assert all(isinstance(r.timestamp_us, int) for r in reports)
    parse_gyro_rv_cargo(payload, capture)


_control_unpackers = [
    sh2.FeatureResponse.unpack,
    sh2.ProductId.unpack,
    sh2.CommandResponse.unpack,
    sh2.FrsReadResponse.unpack,
    sh2.FrsWriteResponse.unpack,
]


@given(st.binary(max_size=40))
def test_sh2_control_responses_reject_bad_payloads_with_the_sdk_error(payload):
    pieces = sh2.split_control_cargo(payload)
    assert b"".join(pieces) == payload
    for unpack in _control_unpackers:
        try:
            unpack(payload)
        except SDK_ERRORS:
            pass


# ── .depzrec replay (contract 08) ────────────────────────────────────────────


class _ScriptedLink(Link):
    """A link whose reads serve a fixed list of rx chunks and whose writes
    are remembered — the inner link of a RecordingLink under test."""

    def __init__(self, rx: list[bytes]):
        self._rx = list(rx)
        self.written: list[bytes] = []
        self._closed = False

    def read(self, timeout: float | None = None) -> bytes:
        return self._rx.pop(0) if self._rx else b""

    def write(self, data: bytes) -> None:
        self.written.append(bytes(data))

    def close(self) -> None:
        self._closed = True

    @property
    def closed(self) -> bool:
        return self._closed


_chunks = st.lists(st.binary(min_size=1, max_size=20), max_size=5)


@st.composite
def _session(draw) -> list[tuple[str, bytes]]:
    """A host↔device byte conversation: ('tx'|'rx', chunk) in time order."""
    return draw(st.lists(st.tuples(st.sampled_from(["tx", "rx"]), st.binary(min_size=1, max_size=20)), max_size=10))


def _record(path: Path, session: list[tuple[str, bytes]]) -> None:
    inner = _ScriptedLink([d for k, d in session if k == "rx"])
    link = RecordingLink(inner, path)
    for kind, data in session:
        if kind == "tx":
            link.write(data)
        else:
            assert link.read(0) == data
    link.close()


@given(_session())
def test_recording_then_replay_serves_the_same_rx_after_the_same_tx(tmp_path_factory, session):
    path = tmp_path_factory.mktemp("rec") / "s.depzrec"
    _record(path, session)
    replay = ReplayLink(path, strict_tx=True)
    assert replay.header["schema"] == "depz.rec/1"
    for kind, data in session:
        if kind == "tx":
            replay.write(data)
        else:
            assert replay.read(0) == data
    assert replay.exhausted and replay.read(0) == b""
    replay.close()


@given(_session(), st.binary(min_size=1, max_size=8))
def test_strict_replay_refuses_a_tx_the_capture_does_not_hold(tmp_path_factory, session, other):
    recorded_tx = b"".join(d for k, d in session if k == "tx")
    if recorded_tx.startswith(other):
        return
    path = tmp_path_factory.mktemp("rec") / "s.depzrec"
    _record(path, session)
    with pytest.raises(ReplayMismatchError):
        ReplayLink(path, strict_tx=True).write(other)


# ── corrupted files: .depzrec and .depzdata (contracts 08, 09) ───────────────

_json_values = st.one_of(
    st.none(), st.booleans(), st.integers(-3, 1 << 40), st.floats(allow_nan=False),
    st.text(max_size=6), st.lists(st.integers(0, 3), max_size=2), st.dictionaries(st.text(max_size=2), st.integers(), max_size=2),
)


_frac = st.floats(0, 1)


def _at(frac: float, n: int) -> int:
    """A position in 0..n chosen by a fraction, so the draws of `_corrupted`
    do not depend on the text (its timestamps differ run to run)."""
    return min(int(frac * (n + 1)), n)


@st.composite
def _corrupted(draw, text: str) -> str:
    """`text` damaged like a disk or an editor would: a span cut, junk
    inserted, a tail lost, a line doubled, or one JSON field retyped."""
    lines = text.split("\n")
    kind = draw(st.sampled_from(["cut", "insert", "truncate", "dup_line", "field", "field", "raw_line"]))
    a, b = _at(draw(_frac), len(text)), _at(draw(_frac), len(text))
    i = _at(draw(_frac), len(lines) - 1)
    if kind == "cut":
        return text[: min(a, b)] + text[max(a, b) + 1 :]
    if kind == "insert":
        return text[:a] + draw(st.text(max_size=8)) + text[a:]
    if kind == "truncate":
        return text[:a]
    if kind == "dup_line":
        lines.insert(i, lines[i])
        return "\n".join(lines)
    if kind == "raw_line":
        lines[i] = draw(st.sampled_from(["", "[]", "1", "null", '"x"', "{}", '{"a": 1}', "\x00", "{"]))
        return "\n".join(lines)
    key_frac, drop, value = draw(_frac), draw(st.booleans()), draw(_json_values)
    try:
        obj = json.loads(lines[i])
    except ValueError:
        return text
    if not isinstance(obj, dict) or not obj:
        return text
    key = sorted(obj)[_at(key_frac, len(obj) - 1)]
    if drop:
        del obj[key]
    else:
        obj[key] = value
    lines[i] = json.dumps(obj)
    return "\n".join(lines)


def _drain_replay(path: Path) -> None:
    link = ReplayLink(path, strict_tx=True)
    for _ in range(40):
        link.write(b"\xa5")
        if link.read(0) == b"" and link.exhausted:
            break
    link.close()


@given(st.data())
def test_replay_link_on_a_corrupted_capture_raises_only_the_sdk_errors(tmp_path_factory, data):
    d = tmp_path_factory.mktemp("rec")
    good = d / "good.depzrec"
    _record(good, data.draw(_session()))
    bad = d / "bad.depzrec"
    bad.write_text(data.draw(_corrupted(good.read_text(encoding="utf-8"))), encoding="utf-8", newline="\n")
    try:
        _drain_replay(bad)
    except SDK_ERRORS:
        pass


_record_value = st.dictionaries(st.text(max_size=4), _json_values, max_size=3)


@st.composite
def _dataset(draw) -> list[tuple[str, int, str, dict]]:
    ids = draw(st.lists(st.sampled_from(["d0", "d1", "d2"]), min_size=1, max_size=3))
    return draw(
        st.lists(
            st.tuples(st.sampled_from(ids), st.integers(0, 50), st.sampled_from(["sr04", "vl53l8", "temperature", "x"]), _record_value),
            max_size=8,
        )
    )


def _write_dataset(path: Path, records) -> None:
    w = DatasetWriter(path, {"d0": {"sensor_type": "sr04"}, "d1": {}, "d2": {}})
    for d, t, k, v in records:
        w.write(d, t, k, v)
    w.close()


@given(_dataset())
def test_dataset_round_trip_merges_by_host_time_keeping_per_device_order(tmp_path_factory, records):
    path = tmp_path_factory.mktemp("ds") / "s.depzdata"
    _write_dataset(path, records)
    got = [(r.device_id, r.t_host_us, r.kind, r.value) for r in DatasetReader(path).records]
    assert sorted(map(json.dumps, got)) == sorted(map(json.dumps, records))
    assert [t for _, t, _, _ in got] == sorted(t for _, t, _, _ in got)
    for dev in {d for d, *_ in records}:
        assert [r for r in got if r[0] == dev] == sorted((r for r in records if r[0] == dev), key=lambda r: r[1])


@given(st.data())
def test_dataset_reader_on_a_corrupted_file_raises_only_the_sdk_errors(tmp_path_factory, data):
    d = tmp_path_factory.mktemp("ds")
    good = d / "good.depzdata"
    _write_dataset(good, data.draw(_dataset()))
    bad = d / "bad.depzdata"
    bad.write_text(data.draw(_corrupted(good.read_text(encoding="utf-8"))), encoding="utf-8", newline="\n")
    try:
        reader = DatasetReader(bad)
        reader.devices
        reader.records
        reader.duration_us
    except SDK_ERRORS:
        pass
