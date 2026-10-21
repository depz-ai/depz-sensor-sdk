"""Golden-vector consumers: tmf8829.json and tmf8829_frame_time.json (contract 14)."""

import hashlib
from dataclasses import asdict

import pytest

from depz_sensor_sdk.protocol import tmf8829 as P
from depz_sensor_sdk.tmf8829 import (
    FIRMWARE_LOAD_ADDR,
    CaptureTimer,
    MeasurementBuilder,
    SensorClock,
    cfg_get,
    cfg_set,
    firmware_image,
    parse_frame,
    parse_results,
)
from depz_sensor_sdk.tmf8829.frames import FID_RESULTS


def _flat(a):
    return None if a is None else [int(x) for x in a.reshape(-1)]


def _arrays(res) -> dict:
    rows, cols = res.shape
    return {
        "peaks": res.peaks,
        "rows": rows,
        "cols": cols,
        "distance": _flat(res.distance),
        "snr": _flat(res.snr),
        "signal": _flat(res.signal),
        "noise": _flat(res.noise),
        "xtalk": _flat(res.xtalk),
    }


def test_encode(vectors):
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


def test_encode_errors(vectors):
    for c in vectors("tmf8829.json")["encode_errors"]:
        with pytest.raises(ValueError):
            if c["kind"] == "read_reg":
                P.pack_read_reg(c["addr"], c["len"])
            elif c["kind"] == "write_reg":
                P.pack_write_reg(c["addr"], bytes.fromhex(c["data"]))
            else:
                P.pack_start_stream(c["int_mask"], c["flags"])


def test_decode(vectors):
    for c in vectors("tmf8829.json")["decode"]:
        payload = bytes.fromhex(c["payload"])
        if c["report"] == P.Tmf8829Rpt.REG_DATA:
            r = P.RegData.unpack(payload)
            got = {"timestamp_us": r.timestamp_us, "data": r.data.hex()}
        elif c["report"] == P.Tmf8829Rpt.INFO:
            got = asdict(P.Tmf8829Info.unpack(payload))
        elif c["report"] == P.Tmf8829Rpt.FRAME:
            f = P.FrameChunk.unpack(payload)
            got = {**asdict(f), "data": f.data.hex()}
        else:
            got = asdict(P.Tmf8829Event.unpack(payload))
        assert got == c["expect"], c["name"]


def test_reassembly(vectors):
    for c in vectors("tmf8829.json")["reassembly"]:
        r = P.FrameReassembler()
        done = []
        for ch in c["chunks"]:
            f = r.feed(P.FrameChunk.unpack(bytes.fromhex(ch)))
            if f is not None:
                done.append(
                    {
                        "frame_seq": f.frame_seq,
                        "timestamp_us": f.timestamp_us,
                        "edge_us": f.edge_us,
                        "size": len(f.data),
                        "sha256": hashlib.sha256(f.data).hexdigest(),
                    }
                )
        got = {
            "frames": done,
            "completed": r.completed,
            "discarded": r.discarded,
            "seq_gaps": r.seq_gaps,
        }
        assert got == c["expect"], c["name"]


def test_frames(vectors):
    for c in vectors("tmf8829.json")["frames"]:
        raw = bytes.fromhex(c["raw"])
        info = parse_frame(raw)
        h = c["header"]
        assert (
            info.fifo_status,
            info.systick,
            info.id,
            info.kind,
            info.fp_mode,
            info.layout,
        ) == (
            h["fifo_status"],
            h["systick"],
            h["id"],
            h["kind"],
            h["fp_mode"],
            h["layout"],
        ), c["name"]
        assert (
            info.payload,
            info.f_number,
            list(info.temperature),
            info.bdv,
            list(info.ref_pos),
        ) == (h["payload"], h["f_number"], h["temperature"], h["bdv"], h["ref_pos"]), c[
            "name"
        ]
        assert (
            info.t0_integration,
            info.t1_integration,
            info.frame_status,
            info.reserved,
            info.eof,
            info.size,
        ) == (
            h["t0_integration"],
            h["t1_integration"],
            h["frame_status"],
            h["reserved"],
            h["eof"],
            h["size"],
        ), c["name"]
        assert info.problems() == c["problems"], c["name"]
        if info.kind == FID_RESULTS:
            assert _arrays(parse_results(raw, info)) == c["results"], c["name"]
        else:
            assert hashlib.sha256(raw[21:-12]).hexdigest() == c["data_sha256"], c[
                "name"
            ]


def test_measurements(vectors):
    v = vectors("tmf8829.json")
    raw_by_case = {c["name"]: bytes.fromhex(c["raw"]) for c in v["frames"]}
    for s in v["measurements"]:
        b = MeasurementBuilder()
        got = []
        for f in s["frames"]:
            m = b.add(raw_by_case[f["raw_case"]], f["timestamp_us"], f["edge_us"])
            if m is None:
                continue
            item = {
                "timestamp_us": m.timestamp_us,
                "flags": m.flags,
                "fp_mode": m.fp_mode,
                "f_number": m.f_number,
                "frame_times_us": [t.timestamp_us for t in m.times],
                "t1_len_us": [t.t1_len_us for t in m.times],
                "ratio_q16": b.clock.ratio_q16,
                "n_histograms": len(m.histograms),
                "n_ref_frames": len(m.ref_frames),
                "n_problems": len(m.problems),
                "distance_in_mm": m.frames[-1].distance_in_mm,
            }
            item.update(_arrays(m.results))
            got.append(item)
        assert got == s["expect"], s["name"]


def test_config_fields(vectors):
    cfg = vectors("tmf8829.json")["config"]
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


def test_firmware_image(vectors):
    fw = vectors("tmf8829.json")["firmware"]
    img = firmware_image()
    assert FIRMWARE_LOAD_ADDR == fw["load_addr"]
    assert (len(img), len(img) // 4, hashlib.sha256(img).hexdigest()) == (
        fw["size"],
        fw["words"],
        fw["sha256"],
    )


def test_frame_time(vectors):
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
    assert checked == 970
