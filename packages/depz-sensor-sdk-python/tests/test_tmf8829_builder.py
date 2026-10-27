"""TMF8829 MeasurementBuilder (contract 14 §3 "frames per measurement", §6)
on the golden frames of contracts/vectors/tmf8829.json and byte-patched
variants of them: frames of an unknown kind, frame-status / EOF / FIFO
problems that are logged without dropping the measurement, the three
half-frame faults of a 32x32 / 48x32 stream (second half alone, two first
halves, halves of different formats), a result format without peaks, the
measurement properties and the contract 09 `tmf8829` record value. The
golden streams themselves are checked in test_vectors_tmf8829.py."""

from __future__ import annotations

import json
import struct
from pathlib import Path

import pytest

from depz_sensor_sdk.tmf8829 import FP_GRID, MeasurementBuilder, Tmf8829Measurement, parse_frame
from depz_sensor_sdk.tmf8829.builder import record_value
from depz_sensor_sdk.tmf8829.frames import FRAME_EOF, LAYOUT_NOISE, LAYOUT_SUBFRAME, FrameError

VECTORS = Path(__file__).resolve().parents[3] / "contracts" / "vectors" / "tmf8829.json"
V = json.loads(VECTORS.read_text(encoding="utf-8"))
RAW = {c["name"]: bytes.fromhex(c["raw"]) for c in V["frames"]}
HEADER = {c["name"]: c["header"] for c in V["frames"]}
MEAS = {m["name"]: m for m in V["measurements"]}

# Byte offsets in a frame (contract 14 §3): pre-header 5 B, header 16 B, footer 12 B.
OFF_FIFO_STATUS = 0
OFF_ID = 5
OFF_F_NUMBER = 9
FOOTER_STATUS = -4
FOOTER_EOF = -2


def patched(raw: bytes, offset: int, value: bytes | int) -> bytes:
    b = bytearray(raw)
    if isinstance(value, int):
        value = bytes([value])
    offset = offset if offset >= 0 else len(raw) + offset
    b[offset:offset + len(value)] = value
    return bytes(b)


def play(builder: MeasurementBuilder, name: str, raws: dict[int, bytes] | None = None) -> list[Tmf8829Measurement]:
    """Feed a golden stream (optionally with some frames replaced by index)."""
    out = []
    for i, f in enumerate(MEAS[name]["frames"]):
        raw = (raws or {}).get(i, RAW[f["raw_case"]])
        m = builder.add(raw, f["timestamp_us"], f["edge_us"])
        if m is not None:
            out.append(m)
    return out


def stream(name: str, raws: dict[int, bytes] | None = None) -> tuple[MeasurementBuilder, list[Tmf8829Measurement]]:
    b = MeasurementBuilder()
    return b, play(b, name, raws)


# ── measurement properties ───────────────────────────────────────────────────


@pytest.mark.parametrize("name", list(MEAS))
def test_measurement_properties_follow_the_last_results_frame(name):
    _, got = stream(name)
    want = MEAS[name]["expect"]
    assert len(got) == len(want)
    for m, e in zip(got, want):
        assert m.shape == (e["rows"], e["cols"]) == FP_GRID[m.fp_mode]
        assert m.peaks == e["peaks"] == m.results.peaks
        assert m.f_number == m.frames[-1].f_number == e["f_number"]
        # die temperature: header byte 0 of the (last) results frame, °C
        assert m.temperature_c == m.frames[-1].temperature[0]
        assert all(f.kind == 0x10 for f in m.frames)
        assert len(m.frames) == len(m.times) == (2 if e["rows"] == 32 else 1)
        if len(m.times) == 2:
            assert m.timestamp_us == (m.times[0].timestamp_us + m.times[1].timestamp_us) // 2
            assert m.flags == m.times[0].flags | m.times[1].flags
        else:
            assert (m.timestamp_us, m.flags) == (m.times[0].timestamp_us, m.times[0].flags)


def test_temperature_c_comes_from_the_second_half_of_a_merged_pair():
    _, (m,) = stream("32x32")
    assert m.temperature_c == HEADER["32x32#1"]["temperature"][0] == 24


# ── problems that do not drop a measurement (§6) ─────────────────────────────


def test_a_frame_of_unknown_kind_is_counted_and_logged_for_the_next_measurement():
    b = MeasurementBuilder()
    for fid in (0x40, 0x00, 0xF3):
        assert b.add(patched(RAW["8x8#0"], OFF_ID, fid), 2125575099, 2125575071) is None
    assert b.parse_errors == 3
    got = play(b, "8x8")
    assert [m.f_number for m in got] == [1, 2]
    assert got[0].problems == [
        "frame id 0x40: unknown kind",
        "frame id 0x00: unknown kind",
        "frame id 0xF3: unknown kind",
    ]
    assert got[1].problems == []  # problems are per measurement


def test_frame_status_eof_and_fifo_problems_keep_the_measurement():
    raw = patched(RAW["8x8#0"], FOOTER_STATUS, 0x00)  # not valid
    b, got = stream("8x8", {0: raw})
    assert [m.f_number for m in got] == [1, 2]
    assert got[0].problems == ["frame id=0x10 fN=1 225 B: status 0x00 not valid"]
    assert got[0].results.distance.ravel().tolist() == MEAS["8x8"]["expect"][0]["distance"]
    assert got[1].problems == []
    assert b.parse_errors == 0

    raw = patched(RAW["8x8#1"], FOOTER_STATUS, 0x01 | 0xC0 | 0x08 | 0x20)
    raw = patched(raw, FOOTER_EOF, struct.pack("<H", 0xBEEF))
    raw = patched(raw, OFF_FIFO_STATUS, 0x30)
    _, got = stream("8x8", {1: raw})
    assert got[1].problems == [
        "frame id=0x10 fN=2 225 B: EOF 0xBEEF, aborted, HV_CP_OVERLOAD, VCDRV_BURST_EXCEEDED, "
        "txfifo_underrun, rxfifo_overrun"
    ]
    assert got[1].timestamp_us == MEAS["8x8"]["expect"][1]["timestamp_us"]


def test_a_histogram_frame_with_a_bad_status_is_logged_on_its_measurement():
    bad_hist = patched(RAW["8x8_hist#1"], FOOTER_STATUS, 0xC1)
    _, (m,) = stream("8x8_hist", {1: bad_hist})
    assert len(m.histograms) == 2
    assert m.problems == ["frame id=0x20 fN=2 25377 B: aborted"]
    assert m.histograms[1].raw == bad_hist


def test_a_corrupt_results_frame_is_dropped_with_a_problem_and_frees_the_half():
    # 32x32 first half, then a second half whose data size does not match its layout
    short = RAW["32x32#1"][:-100] + RAW["32x32#1"][-12:]
    b, got = stream("32x32", {1: short})
    assert got == [] and b.parse_errors == 1
    # the pending first half is gone: the next full pair builds normally and carries the problem
    (m,) = play(b, "32x32")
    assert m.f_number == 2
    assert len(m.problems) == 1 and m.problems[0].startswith("fN=2: result data ")
    assert "expected 512 px" in m.problems[0]


# ── half-frame faults of 32x32 / 48x32 streams (§3) ──────────────────────────


def test_a_second_half_without_a_first_is_dropped_with_its_pending_frames():
    b = MeasurementBuilder()
    frames = MEAS["32x32_ref"]["frames"]
    # ref-SPAD frame of the second half, then the second half alone
    assert b.add(RAW["32x32_ref#2"], frames[2]["timestamp_us"], 0) is None
    assert b.add(RAW["32x32_ref#3"], frames[3]["timestamp_us"], frames[3]["edge_us"]) is None
    assert b.parse_errors == 0  # a stream fault, not a parse error
    (m,) = play(b, "32x32_ref")
    assert m.problems == ["second half fN=2 without a first one - dropped"]
    assert len(m.ref_frames) == 2  # only the pair's own ref frames: the orphan's was dropped
    assert m.ref_frames == [RAW["32x32_ref#0"], RAW["32x32_ref#2"]]
    assert m.results.distance.ravel().tolist() == MEAS["32x32_ref"]["expect"][0]["distance"]


def test_two_first_halves_in_a_row_keep_the_newer_one():
    first_again = patched(RAW["32x32#0"], OFF_F_NUMBER, struct.pack("<I", 7))
    b = MeasurementBuilder()
    f = MEAS["32x32"]["frames"]
    assert b.add(RAW["32x32#0"], f[0]["timestamp_us"], f[0]["edge_us"]) is None
    assert b.add(first_again, f[0]["timestamp_us"] + 60_000, f[0]["edge_us"] + 60_000) is None
    m = b.add(RAW["32x32#1"], f[1]["timestamp_us"], f[1]["edge_us"])
    assert m is not None
    assert m.problems == ["two first halves in a row (fN 1, 7)"]
    assert [x.f_number for x in m.frames] == [7, 2]
    assert m.f_number == 2
    assert m.times[0].pair_us == f[0]["timestamp_us"] + 60_000
    assert m.timestamp_us == (m.times[0].timestamp_us + m.times[1].timestamp_us) // 2


@pytest.mark.parametrize("second, label", [
    (patched(RAW["32x32#1"], OFF_ID, 0x14), "fp_mode 3 vs 4 (same grid)"),
    (RAW["48x32_2p_sig#1"], "32x32 vs 48x32"),
])
def test_halves_of_different_formats_are_dropped_with_their_pending_frames(second, label):
    b = MeasurementBuilder()
    f = MEAS["32x32_ref"]["frames"]
    assert b.add(RAW["32x32_ref#0"], f[0]["timestamp_us"], 0) is None
    assert b.add(RAW["32x32#0"], f[1]["timestamp_us"], f[1]["edge_us"]) is None
    assert b.add(second, f[3]["timestamp_us"], f[3]["edge_us"]) is None, label
    assert b.parse_errors == 0
    # the next clean pair is unaffected and carries the problem
    (m,) = play(b, "32x32")
    assert m.problems == ["half-frames of different formats (fN 1, 2) - dropped"]
    assert m.ref_frames == []
    assert m.results.distance.ravel().tolist() == MEAS["32x32"]["expect"][0]["distance"]


def test_the_second_half_is_told_by_the_subframe_bit():
    assert not parse_frame(RAW["32x32#0"]).is_second_half
    assert parse_frame(RAW["32x32#1"]).is_second_half
    assert parse_frame(RAW["32x32#1"]).layout & LAYOUT_SUBFRAME


# ── a result format without peaks ────────────────────────────────────────────


def noise_only_8x8_frame(f_number: int = 1) -> bytes:
    """8x8 results frame, layout 0x10 (noise only, no peaks): 64 × u16."""
    data = struct.pack("<64H", *(100 + i for i in range(64)))
    full = 21 + len(data) + 12
    head = struct.pack("<BI", 0, 277999756) + struct.pack(
        "<BBHI3bBHH", 0x10, LAYOUT_NOISE, full - 9, f_number, 21, 21, 20, 73, 1271, 1261
    )
    footer = struct.pack("<IIBBH", 277995705, 277997621, 0x01, 0, FRAME_EOF)
    return head + data + footer


def test_a_frame_without_peaks_has_no_distance():
    b = MeasurementBuilder()
    m = b.add(noise_only_8x8_frame(), 2125575099, 2125575071)
    assert m is not None
    assert (m.peaks, m.shape, m.problems) == (0, (8, 8), [])
    assert m.distance_mm is None and m.results.distance is None and m.results.snr is None
    assert m.results.noise.shape == (8, 8)
    assert m.results.noise.ravel().tolist() == [100 + i for i in range(64)]
    assert m.results.xtalk is None and m.results.signal is None
    v = record_value(m)
    assert (v["distance_mm"], v["snr"], v["peaks"]) == ([], [], 0)
    assert v["noise"] == [100 + i for i in range(64)]
    assert "signal" not in v and "xtalk" not in v


def test_a_frame_shorter_than_header_plus_footer_is_a_counted_parse_error():
    with pytest.raises(FrameError):
        parse_frame(RAW["8x8#0"][:32])
    b = MeasurementBuilder()
    assert b.add(RAW["8x8#0"][:20], 2125575099, 2125575071) is None
    assert b.add(b"", 2125575099, 0) is None
    assert b.parse_errors == 2
    got = play(b, "8x8")
    assert [m.f_number for m in got] == [1, 2]
    assert got[0].problems == [
        "frame of 20 B is shorter than header + footer",
        "frame of 0 B is shorter than header + footer",
    ]


# ── contract 09 record value ─────────────────────────────────────────────────


@pytest.mark.parametrize("name", ["8x8", "8x8_full", "8x8_xr", "16x16", "32x32", "48x32_2p_sig", "8x8_hist", "32x32_ref"])
def test_record_value_is_the_contract_09_tmf8829_record(name):
    _, got = stream(name)
    for m, e in zip(got, MEAS[name]["expect"]):
        v = record_value(m)
        base = ["fp_mode", "rows", "cols", "peaks", "f_number", "flags", "temperature_c", "distance_mm", "snr"]
        extra = [k for k in ("signal", "noise", "xtalk") if e[k] is not None]
        assert list(v) == base + extra
        assert {k: v[k] for k in base[:6]} == {k: e[k] for k in base[:6]}
        assert v["temperature_c"] == m.temperature_c
        n = e["peaks"] * e["rows"] * e["cols"]
        assert len(v["distance_mm"]) == len(v["snr"]) == n
        # mm, clock-corrected (× ratio, ÷ 4 unless the 8x8-xr footer flag), rounded to 0.1 mm
        ratio = e["ratio_q16"] / (8 << 16)
        div = 1 if e["distance_in_mm"] else 4
        assert v["distance_mm"] == [round(d * ratio / div, 1) for d in e["distance"]]
        assert v["snr"] == e["snr"]
        for k in extra:
            assert v[k] == e[k], k
            assert len(v[k]) == (n if k == "signal" else e["rows"] * e["cols"])
        assert all(isinstance(x, int) for x in v["snr"] + sum((v[k] for k in extra), []))
        assert all(isinstance(x, float) for x in v["distance_mm"])
        json.dumps(v)  # plain JSON types only
