"""Golden-vector consumers: tmf8829.json and tmf8829_frame_time.json (contract 14)."""

import hashlib
import json
from pathlib import Path

import pytest

from depz_sensor_sdk.protocol import tmf8829 as P
from depz_sensor_sdk.tmf8829 import (
    CFG_FIELDS,
    FIRMWARE_LOAD_ADDR,
    CaptureTimer,
    SensorClock,
    cfg_get,
    cfg_set,
    firmware_image,
)
from vector_shapes import (
    tmf8829_decode,
    tmf8829_frame,
    tmf8829_measurements,
    tmf8829_reassembly,
)

RECORDINGS = Path(__file__).resolve().parents[3] / "contracts" / "vectors" / "recordings"


def test_encode(vectors, seen):
    for c in vectors("tmf8829.json")["encode"]:
        if c["cmd"] == P.Tmf8829Cmd.READ_REG:
            payload = P.pack_read_reg(c["addr"], c["len"])
        elif c["cmd"] == P.Tmf8829Cmd.WRITE_REG:
            payload = P.pack_write_reg(c["addr"], bytes.fromhex(c["data"]))
        elif c["cmd"] == P.Tmf8829Cmd.START_STREAM:
            payload = P.pack_start_stream(c["int_mask"], c["flags"])
        else:
            payload = b""
        assert payload.hex() == c["payload"], c["name"]
        seen(c)


def test_encode_errors(vectors, seen):
    for c in vectors("tmf8829.json")["encode_errors"]:
        with pytest.raises(ValueError):
            if c["kind"] == "read_reg":
                P.pack_read_reg(c["addr"], c["len"])
            elif c["kind"] == "write_reg":
                P.pack_write_reg(c["addr"], bytes.fromhex(c["data"]))
            else:
                P.pack_start_stream(c["int_mask"], c["flags"])
        seen(c)


def test_decode(vectors, seen):
    for c in vectors("tmf8829.json")["decode"]:
        got = tmf8829_decode(c["report"], bytes.fromhex(c["payload"]))
        assert got == c["expect"], c["name"]
        seen(c)


def test_reassembly(vectors, seen):
    for c in vectors("tmf8829.json")["reassembly"]:
        got = tmf8829_reassembly(bytes.fromhex(ch) for ch in c["chunks"])
        assert got == c["expect"], c["name"]
        seen(c)


def test_frames(vectors, seen):
    for c in vectors("tmf8829.json")["frames"]:
        want = {"header": c["header"], "problems": c["problems"]}
        want.update({"results": c["results"]} if "results" in c else {"data_sha256": c["data_sha256"]})
        assert tmf8829_frame(bytes.fromhex(c["raw"])) == want, c["name"]
        seen(c)


def test_measurements(vectors, seen):
    v = vectors("tmf8829.json")
    raw_by_case = {c["name"]: bytes.fromhex(c["raw"]) for c in v["frames"]}
    for s in v["measurements"]:
        frames = [(raw_by_case[f["raw_case"]], f["timestamp_us"], f["edge_us"]) for f in s["frames"]]
        assert tmf8829_measurements(frames) == s["expect"], s["name"]
        seen(s)


def test_config_fields(vectors, seen):
    cfg = vectors("tmf8829.json")["config"]
    assert {name: list(f) for name, f in CFG_FIELDS.items()} == cfg["fields"]
    page = bytes.fromhex(cfg["page"])
    for name, value in cfg["get"].items():
        assert cfg_get(page, name) == value, name
    for c in cfg["set"]:
        pg = bytearray(page)
        cfg_set(pg, c["field"], c["value"])
        assert bytes(pg).hex() == c["page"], c["field"]
    for c in cfg["set_errors"]:
        with pytest.raises(ValueError):
            cfg_set(bytearray(page), c["field"], c["value"])
    seen("tmf8829/config")


def test_firmware_image(vectors, seen):
    fw = vectors("tmf8829.json")["firmware"]
    img = firmware_image()
    assert FIRMWARE_LOAD_ADDR == fw["load_addr"]
    assert (len(img), len(img) // 4, hashlib.sha256(img).hexdigest()) == (
        fw["size"],
        fw["words"],
        fw["sha256"],
    )
    seen("tmf8829/firmware")


def test_frame_time(vectors, seen):
    v = vectors("tmf8829_frame_time.json")
    col = {name: i for i, name in enumerate(v["columns"])}
    checked = 0
    for s in v["streams"]:
        clock = SensorClock()
        timer = CaptureTimer(clock)
        for row in s["frames"]:
            fid, ts, tick = row[col["id"]], row[col["ts"]], row[col["systick"]]
            kind = fid & 0xF0
            if kind == 0x30:
                continue
            flags = clock.add(ts, tick)
            if kind == 0x20:
                timer.histograms()
                continue
            t = timer.results(
                fid & 0x0F,
                tick,
                row[col["t0"]],
                row[col["t1"]],
                ts,
                row[col["edge_us"]],
                flags,
            )
            assert (t.timestamp_us, t.t1_len_us, t.flags, clock.ratio_q16) == (
                row[col["exp_timestamp_us"]],
                row[col["exp_t1_len_us"]],
                row[col["exp_flags"]],
                row[col["exp_ratio_q16"]],
            ), (s["scenario"], row)
            checked += 1
        seen(s)
    assert checked == 970


def test_frame_time_recording(seen):
    """The board's own capture, replayed raw: each `scenario` line starts a stream."""
    clock = SensorClock()
    timer = CaptureTimer(clock)
    checked = 0
    lines = (RECORDINGS / "tmf8829_frame_time.jsonl").read_text().splitlines()
    for line in lines:
        v = json.loads(line)
        if "scenario" in v:
            clock.reset()
            timer.reset()
            continue
        kind = v["id"] & 0xF0
        if kind == 0x30:
            continue
        flags = clock.add(v["ts"], v["systick"])
        if kind == 0x20:
            timer.histograms()
            continue
        t = timer.results(
            v["id"] & 0x0F, v["systick"], v["t0"], v["t1"], v["ts"], v["edge_us"], flags
        )
        got = {
            "timestamp_us": t.timestamp_us,
            "t1_len_us": t.t1_len_us,
            "flags": t.flags,
            "ratio_q16": clock.ratio_q16,
        }
        assert got == v["exp"], v
        checked += 1
    assert checked == 970
    seen("recordings/tmf8829_frame_time.jsonl")
