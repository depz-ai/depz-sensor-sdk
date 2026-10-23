"""SDK results -> golden-vector shapes, shared by the vector tests and the
differential-testing adapter (tests/difftest_adapter.py, contracts/16_DIFFTEST.md).

Each helper turns what the public API returns into exactly the JSON shape the
matching vector group writes under `expect`, so a test asserts `shape == expect`
and the adapter answers with the same `shape`.
"""

from __future__ import annotations

import dataclasses
import hashlib
from dataclasses import asdict

from depz_sensor_sdk.dataset import bno086_record_value
from depz_sensor_sdk.protocol import tmf8829 as P
from depz_sensor_sdk.tmf8829 import MeasurementBuilder, parse_frame, parse_results
from depz_sensor_sdk.tmf8829.frames import FID_RESULTS
from depz_sensor_sdk.transport import CrcError, Packet, PacketParser, Trash


def lists(d: dict) -> dict:
    """Tuples -> lists, one level deep (dataclass asdict of codec records)."""
    return {k: (list(v) if isinstance(v, tuple) else v) for k, v in d.items()}


# ── framing (contract 01 §5) ──────────────────────────────────────────────────


def framing_collect(parser: PacketParser, chunks) -> dict:
    """Feed `chunks` in order; the framing_decode.json `expect` shape."""
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


# ── BNO086 (contract 05) ──────────────────────────────────────────────────────


def bno086_cargo(c) -> dict:
    """One reassembled SHTP cargo, bno086_shtp.json reassembly shape."""
    return {"channel": c.channel, "seq": c.seq, "payload": c.payload.hex(), "capture_us": c.capture_us}


def bno086_fields(rep) -> dict:
    """The raw `fields` of a decoded SH-2 report (bytes hex, tuples lists, bools ints)."""
    fields = {}
    for k, v in dataclasses.asdict(rep).items():
        if isinstance(v, bytes):
            v = v.hex()
        elif isinstance(v, tuple):
            v = list(v)
        fields[k] = int(v) if isinstance(v, bool) else v
    return fields


def bno086_serialize(rep) -> dict:
    """bno086_reports.json report shape: type, raw fields, contract 09 record."""
    return {"type": type(rep).__name__, "fields": bno086_fields(rep), "record": bno086_record_value(rep)}


# ── TMF8829 (contract 14) ─────────────────────────────────────────────────────


def _flat(a):
    return None if a is None else [int(x) for x in a.reshape(-1)]


def tmf8829_arrays(res) -> dict:
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


def tmf8829_decode(report: int, payload: bytes) -> dict:
    """tmf8829.json decode shape for one report payload."""
    if report == P.Tmf8829Rpt.REG_DATA:
        r = P.RegData.unpack(payload)
        return {"timestamp_us": r.timestamp_us, "data": r.data.hex()}
    if report == P.Tmf8829Rpt.INFO:
        return asdict(P.Tmf8829Info.unpack(payload))
    if report == P.Tmf8829Rpt.FRAME:
        f = P.FrameChunk.unpack(payload)
        return {**asdict(f), "data": f.data.hex()}
    if report == P.Tmf8829Rpt.EVENT:
        return asdict(P.Tmf8829Event.unpack(payload))
    raise ValueError(f"unknown TMF8829 report {report:#x}")


def tmf8829_reassembly(chunks) -> dict:
    """Feed FRAME-chunk payloads to one reassembler; the reassembly shape."""
    r = P.FrameReassembler()
    done = []
    for ch in chunks:
        f = r.feed(P.FrameChunk.unpack(ch))
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
    return {"frames": done, "completed": r.completed, "discarded": r.discarded, "seq_gaps": r.seq_gaps}


_HEADER_FIELDS = (
    "fifo_status", "systick", "id", "kind", "fp_mode", "layout", "payload", "f_number",
    "temperature", "bdv", "ref_pos", "t0_integration", "t1_integration", "frame_status",
    "reserved", "eof", "size",
)


def tmf8829_frame(raw: bytes) -> dict:
    """tmf8829.json frames shape: header, problems, then results or data_sha256."""
    info = parse_frame(raw)
    header = {}
    for k in _HEADER_FIELDS:
        v = getattr(info, k)
        header[k] = list(v) if isinstance(v, tuple) else v
    out = {"header": header, "problems": info.problems()}
    if info.kind == FID_RESULTS:
        out["results"] = tmf8829_arrays(parse_results(raw, info))
    else:
        out["data_sha256"] = hashlib.sha256(raw[21:-12]).hexdigest()
    return out


def tmf8829_measurements(frames) -> list:
    """`frames` = (raw, timestamp_us, edge_us) in order; the measurements expect list."""
    b = MeasurementBuilder()
    got = []
    for raw, ts, edge in frames:
        m = b.add(raw, ts, edge)
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
        item.update(tmf8829_arrays(m.results))
        got.append(item)
    return got
