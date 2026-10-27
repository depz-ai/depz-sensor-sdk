"""Differential-testing adapter for the Python SDK (contracts/16_DIFFTEST.md).

A long-lived process speaking JSON Lines on stdin/stdout:
    {"id": n, "op": "<op>", "in": {...}}  ->  {"id": n, "out": ...}
                                          or  {"id": n, "error": "invalid" | "unsupported"}
`invalid` = the SDK rejected the input (raised); `unsupported` = this SDK does not
implement the op. Run: `uv run python tests/difftest_adapter.py` (driver:
scripts/difftest.py). Output shapes come from tests/vector_shapes.py, the same
helpers the golden-vector tests assert with.
"""

from __future__ import annotations

import json
import math
import struct
import sys
import threading
from dataclasses import asdict
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import vector_shapes as S  # noqa: E402
from fake_vl53l8 import make_driver  # noqa: E402

from depz_sensor_sdk import CnhConfig  # noqa: E402
from depz_sensor_sdk.bno055 import regs as R  # noqa: E402
from depz_sensor_sdk.bno086 import sh2  # noqa: E402
from depz_sensor_sdk.bno086.reports import parse_gyro_rv_cargo, parse_input_cargo  # noqa: E402
from depz_sensor_sdk.bno086.shtp import ShtpHeader, ShtpLayer  # noqa: E402
from depz_sensor_sdk.dataset import bno086_record_value  # noqa: E402
from depz_sensor_sdk.discovery import (  # noqa: E402
    _PortId,
    _serial_sort_key,
    _vl53l7_class,
    _vl53lx_class,
    resolve_sensor_class,
)
from depz_sensor_sdk.protocol import (  # noqa: E402
    SequenceErrorReport,
    StatusReport,
    SyncPinConfig,
    TemperatureReport,
    TextReport,
    pack_set_payload_crc_type,
    pack_sync_time,
    parse_software_name,
    strip_device_string,
    sync_time_offset_rtt,
)
from depz_sensor_sdk.protocol import bno055 as B055  # noqa: E402
from depz_sensor_sdk.protocol import sr04 as SR  # noqa: E402
from depz_sensor_sdk.protocol import tmf8829 as TP  # noqa: E402
from depz_sensor_sdk.protocol import vl53l4 as L4P  # noqa: E402
from depz_sensor_sdk.protocol import vl53l7 as L7P  # noqa: E402
from depz_sensor_sdk.protocol import vl53l8 as L8P  # noqa: E402
from depz_sensor_sdk.protocol import vl53lx as LXP  # noqa: E402
from depz_sensor_sdk.protocol.bootloader import FwDepzImage  # noqa: E402
from depz_sensor_sdk.protocol._wire import check_uint  # noqa: E402
from depz_sensor_sdk.tmf8829 import CaptureTimer, SensorClock, cfg_get, cfg_set  # noqa: E402
from depz_sensor_sdk.transport import (  # noqa: E402
    CrcType,
    PacketParser,
    build_packet,
    crc8_maxim,
    crc16_ccitt_false,
    crc16_modbus,
    crc32_iso_hdlc,
)
from depz_sensor_sdk.usb_ids import is_known_depz_usb, usb_model_hint  # noqa: E402
from depz_sensor_sdk.vl53l4 import uld as L4  # noqa: E402
from depz_sensor_sdk.vl53l8 import cnh  # noqa: E402
from depz_sensor_sdk.vl53l8 import uld as L8  # noqa: E402
from depz_sensor_sdk.vl53lx import decode as LXD  # noqa: E402
from depz_sensor_sdk.vl53lx.uld import registry  # noqa: E402

# Ops this SDK declares it cannot do, with the reason (never a silent gap).
UNSUPPORTED: dict[str, str] = {}


def h(s: str) -> bytes:
    return bytes.fromhex(s)


# ── ops ───────────────────────────────────────────────────────────────────────


def op_crc(i):
    raw = h(i["input"])
    return {
        "crc8_maxim": crc8_maxim(raw),
        "crc16_modbus": crc16_modbus(raw),
        "crc32_iso_hdlc": crc32_iso_hdlc(raw),
        "crc16_ccitt_false": crc16_ccitt_false(raw),
    }


def op_framing_encode(i):
    return {"frame": build_packet(i["cmd"], h(i["payload"]), i["seq"], CrcType(i["crc_type"])).hex()}


def op_framing_decode(i):
    chunks = [h(c) for c in i["chunks"]] if "chunks" in i else [h(i["stream"])]
    return S.framing_collect(PacketParser(), chunks)


def op_identity(i):
    ident = parse_software_name(strip_device_string(h(i["raw"])))
    return {
        "mode": ident.mode,
        "sensor_type": ident.sensor_type.value if ident.sensor_type else None,
        "software_name": ident.software_name,
        "version": ident.version,
    }


def op_usb_identity(i):
    return {"known": is_known_depz_usb(i["vid"], i["pid"]), "model": usb_model_hint(i["vid"], i["pid"])}


def op_usb_serial_ordering(i):
    pis = [_PortId(p["port"], None, None, p["serial"]) for p in i["ports"]]
    pis.sort(key=_serial_sort_key)
    return {"order": [pi.port for pi in pis]}


def op_probe_resolve(i):
    return {"class": resolve_sensor_class(i["software_name"], i["device_name"], i["usb_vid"], i["usb_pid"])}


def op_common_encode(i):
    kind = i["kind"]
    if kind == "sync_time_request":
        p = pack_sync_time(i["pc_timestamp_us"])
    elif kind == "set_payload_crc_type":
        p = pack_set_payload_crc_type(i["crc_type"])
    elif kind == "sync_pin_config":
        p = SyncPinConfig(i["pin"], i["mode"], i["polarity"]).pack()
    else:
        raise ValueError(kind)
    return {"payload": p.hex()}


def op_common_decode(i):
    p, rpt = h(i["payload"]), i["report"]
    if rpt == 0x80:
        r = StatusReport.unpack(p)
        return {"cmd": r.cmd, "status": r.status}
    if rpt == 0x81:
        r = TextReport.unpack(p)
        return {"cmd": r.cmd, "text": r.text}
    if rpt == 0x83:
        r = TemperatureReport.unpack(p)
        return {"timestamp_us": r.timestamp_us, "raw_decidegrees": r.raw_decidegrees}
    if rpt == 0x84:
        r = SequenceErrorReport.unpack(p)
        return {"expected_seq": r.expected_seq, "received_seq": r.received_seq}
    raise ValueError(rpt)


def op_common_event(i):
    """The event the device layer emits for an unsolicited common report —
    `common_decode`'s twin through the dispatcher (contract 07 §2/§3): the
    packet goes in on a loopback link with nothing pending, and the first
    event out is the answer. A report longer than its fields decodes with the
    tail ignored; one nobody decodes stays visible as hex text."""
    from depz_sensor_sdk.device import (
        DeviceBase,
        DeviceEvent,
        SequenceErrorEvent,
        TemperatureEvent,
        TextEvent,
        UnsolicitedStatusEvent,
    )
    from depz_sensor_sdk.transport import CrcType, build_packet
    from depz_sensor_sdk.transport.link import LoopbackLink

    host, side = LoopbackLink.pair()
    got: list[DeviceEvent] = []
    arrived = threading.Event()
    dev = DeviceBase(host, timeout=1.0)
    try:
        dev.on_event(lambda ev: (got.append(ev), arrived.set()))
        side.write(build_packet(i["report"], h(i["payload"]), 0, CrcType.NONE))
        if not arrived.wait(2.0):
            raise TimeoutError("the device emitted no event")
    finally:
        dev.close()
        side.close()
    ev = got[0]
    if isinstance(ev, UnsolicitedStatusEvent):
        return {"event": "unsolicited_status", "status": ev.status}
    if isinstance(ev, SequenceErrorEvent):
        return {"event": "sequence_error", "expected_seq": ev.expected_seq,
                "received_seq": ev.received_seq, "reported_by_device": ev.reported_by_device}
    if isinstance(ev, TextEvent):
        return {"event": "text", "cmd": ev.cmd, "text": ev.text}
    if isinstance(ev, TemperatureEvent):
        return {"event": "temperature", "timestamp_us": ev.timestamp_us, "celsius": ev.celsius}
    raise TypeError(type(ev).__name__)


def op_common_seq_gap(i):
    """The host-observed seq gaps (contract 01 §5 step 5, ERRATA A1): one
    packet per `seqs` entry (an unknown report 0x7F, payload 00 — each becomes
    a hex `text` event) goes in on a loopback link; once every packet's own
    event is out, the `seq_gap` events and `stats.seq_gaps` are the answer.
    A gap is surfaced before its packet, so none can still be on the way."""
    from depz_sensor_sdk.device import DeviceBase, DeviceEvent, SeqGapEvent, TextEvent
    from depz_sensor_sdk.transport import CrcType, build_packet
    from depz_sensor_sdk.transport.link import LoopbackLink

    seqs = i["seqs"]
    if any(not isinstance(s, int) or isinstance(s, bool) or not 0 <= s <= 0xFF for s in seqs):
        raise ValueError("a seq is a byte")
    host, side = LoopbackLink.pair()
    got: list[DeviceEvent] = []
    texts = threading.Semaphore(0)

    def on_event(ev: DeviceEvent) -> None:
        got.append(ev)
        if isinstance(ev, TextEvent):
            texts.release()

    dev = DeviceBase(host, timeout=1.0)
    try:
        dev.on_event(on_event)
        for s in seqs:
            side.write(build_packet(0x7F, b"\x00", s, CrcType.NONE))
        for _ in seqs:
            if not texts.acquire(timeout=2.0):
                raise TimeoutError("a packet produced no event")
        seq_gaps = dev.stats.seq_gaps
    finally:
        dev.close()
        side.close()
    return {"gaps": [{"expected_seq": e.expected_seq, "received_seq": e.received_seq,
                      "missed": e.missed} for e in got if isinstance(e, SeqGapEvent)],
            "seq_gaps": seq_gaps}


def op_sync_time_math(i):
    off, rtt = sync_time_offset_rtt(i["t1"], i["t2"], i["t3"], i["t4"])
    return {"offset_us": off, "rtt_us": rtt}


def op_sr04_encode(i):
    if i["kind"] == "set_sample_period":
        return {"payload": SR.pack_sample_period(i["period_us"]).hex()}
    if i["kind"] == "set_echo_decay":
        return {"payload": SR.pack_echo_decay(i["decay_us"]).hex()}
    raise ValueError(i["kind"])


def op_sr04_decode(i):
    p, rpt = h(i["payload"]), i["report"]
    if rpt == 0x91:
        d = SR.Sr04Data.unpack(p)
        return {"source_cmd": d.source_cmd, "timestamp_us": d.timestamp_us, "echo_time_us": d.echo_time_us}
    if rpt == 0x92:
        return {"period_us": SR.unpack_sample_period(p)}
    if rpt == 0x93:
        return {"decay_us": SR.unpack_echo_decay(p)}
    raise ValueError(rpt)


def op_fwdepz_parse(i):
    img = FwDepzImage.parse(h(i["file"]))
    return {
        "load_addr": img.load_addr,
        "fw_size": img.fw_size,
        "fw_crc32": img.fw_crc32,
        "cur_sec": img.cur_sec,
        "tot_sec": img.tot_sec,
        "payload_crc_ok": img.payload_crc_ok,
    }


# ── BNO055 ──


def op_bno055_encode(i):
    kind = i["kind"]
    if kind == "read_reg":
        p = B055.pack_read_reg(i["addr"], i["len"])
    elif kind == "write_reg":
        p = B055.pack_write_reg(i["addr"], h(i["data"]))
    elif kind == "start_stream":
        p = B055.pack_start_stream(i["trigger"], i["addr"], i["len"], i["period_ms"])
    elif kind in ("reset", "stop_stream", "get_info"):
        p = b""
    else:
        raise ValueError(kind)
    return {"payload": p.hex()}


def op_bno055_decode(i):
    p, rpt = h(i["payload"]), i["report"]
    if rpt == 0x92:
        return asdict(B055.Bno055Info.unpack(p))
    if rpt == 0x91:
        r = B055.RegData.unpack(p)
        return {"cmd": r.cmd, "timestamp_us": r.timestamp_us, "data": r.data.hex()}
    if rpt == 0x93:
        s = B055.StreamData.unpack(p)
        return {"timestamp_us": s.timestamp_us, "addr": s.addr, "len": s.length, "data": s.data.hex()}
    raise ValueError(rpt)


def op_bno055_units(i):
    u = R.Units.unpack(i["unit_sel"])
    return {**asdict(u), "repack": u.pack()}


def op_bno055_calib_stat(i):
    s = R.CalibStatus.unpack(i["value"])
    return {**asdict(s), "fully_calibrated": s.fully_calibrated, "repack": s.pack()}


def op_bno055_calibration_profile(i):
    p = R.CalibrationProfile.unpack(h(i["bytes"]))
    return {**S.lists(asdict(p)), "repack": p.pack().hex()}


def op_bno055_axis_remap(i):
    a = R.AxisRemap.unpack(i["config"], i["sign"])
    return {**asdict(a), "repack": list(a.pack())}


def op_bno055_axis_remap_pack(i):
    a = R.AxisRemap(i["x"], i["y"], i["z"], i["x_negative"], i["y_negative"], i["z_negative"])
    return {"repack": list(a.pack())}


def op_bno055_sensor_config(i):
    sensor = i["sensor"]
    if sensor == "accel":
        a = R.AccelConfig.unpack(i["value"])
        return {**asdict(a), "repack": a.pack()}
    if sensor == "gyro":
        g = R.GyroConfig.unpack(h(i["bytes"]))
        return {**asdict(g), "repack": g.pack().hex()}
    if sensor == "mag":
        m = R.MagConfig.unpack(i["value"])
        return {**asdict(m), "repack": m.pack()}
    raise ValueError(sensor)


def op_bno055_block(i):
    return S.lists(asdict(R.decode_block(i["addr"], h(i["data"]))))


# ── BNO086 ──


def op_bno086_shtp_header_pack(i):
    return {"bytes": ShtpHeader(i["length"], i["channel"], i["seq"], i["continuation"]).pack().hex()}


def op_bno086_shtp_header_unpack(i):
    hdr = ShtpHeader.unpack(h(i["bytes"]))
    return {"length": hdr.length, "channel": hdr.channel, "seq": hdr.seq, "continuation": hdr.continuation}


def op_bno086_shtp_tx_seq(i):
    layer = ShtpLayer()
    return {"frames": [layer.next_frame(p["channel"], h(p["payload"])).hex() for p in i["packets"]]}


def op_bno086_shtp_reassembly(i):
    rx = ShtpLayer()
    cargos = []
    for f, t in zip(i["frames"], i["captures_us"], strict=True):
        c = rx.feed(h(f), t)
        if c is not None:
            cargos.append(S.bno086_cargo(c))
    return {"cargos": cargos, "discarded": rx.discarded}


def op_bno086_control_encode(i):
    kind = i["kind"]
    if kind == "set_feature":
        p = sh2.build_set_feature(
            i["sensor_id"], i["interval_us"], i["batch_us"], i["sensitivity"], i["flags"], i["cfg_word"]
        )
    elif kind == "get_feature_request":
        p = sh2.build_get_feature_request(i["sensor_id"])
    elif kind == "product_id_request":
        p = sh2.build_product_id_request()
    elif kind == "command_request":
        p = sh2.build_command_request(i["seq"], i["command"], h(i["params"]))
    elif kind == "frs_read_request":
        p = sh2.build_frs_read_request(i["frs_type"], i["offset_words"], i["block_words"])
    elif kind == "frs_write_request":
        p = sh2.build_frs_write_request(i["frs_type"], i["length_words"])
    elif kind == "frs_write_data":
        p = sh2.build_frs_write_data(i["offset_words"], i["words"])
    else:
        raise ValueError(kind)
    return {"payload": p.hex()}


_CONTROL_DECODERS = {
    "feature_response": sh2.FeatureResponse.unpack,
    "product_id": sh2.ProductId.unpack,
    "command_response": sh2.CommandResponse.unpack,
    "frs_read_response": sh2.FrsReadResponse.unpack,
    "frs_write_response": sh2.FrsWriteResponse.unpack,
}


def op_bno086_control_decode(i):
    """A control-channel response (contract 05 §4 table) decoded: its fields,
    or invalid when shorter than them (contract 07 §3)."""
    return S.lists(asdict(_CONTROL_DECODERS[i["kind"]](h(i["payload"]))))


def op_bno086_input_cargo(i):
    reps = parse_input_cargo(h(i["cargo"]), i["capture_timestamp_us"])
    return {"reports": [{"type": type(r).__name__, "fields": S.bno086_fields(r)} for r in reps]}


def op_bno086_record(i):
    reps = parse_input_cargo(h(i["cargo"]), i["capture_timestamp_us"])
    return {"records": [bno086_record_value(r) for r in reps]}


def op_bno086_gyro_rv(i):
    r = parse_gyro_rv_cargo(h(i["cargo"]), i["capture_timestamp_us"])
    return None if r is None else {"type": type(r).__name__, "fields": S.bno086_fields(r)}


# ── .depzrec / .depzdata readers (contracts 08, 09) ──


def _temp_file(text: str, suffix: str) -> Path:
    import tempfile

    fd, name = tempfile.mkstemp(suffix=suffix)
    with open(fd, "w", encoding="utf-8", newline="\n") as f:
        f.write(text)
    return Path(name)


def op_depzrec_read(i):
    """`text` as a capture: every rx chunk the loose replay serves once the
    host has written more tx bytes than any event waits for, concatenated —
    or invalid for a damaged file."""
    from depz_sensor_sdk.transport.record_replay import ReplayLink

    path = _temp_file(i["text"], ".depzrec")
    try:
        link = ReplayLink(path)
        link.write(b"\x00" * 65536)
        rx = bytearray()
        while not link.exhausted:
            rx.extend(link.read(0))
        link.close()
    finally:
        path.unlink()
    return {"rx": bytes(rx).hex()}


def op_depzdata_read(i):
    """`text` as a dataset: its records in reader order as [d, t, k] — or
    invalid for a damaged file."""
    from depz_sensor_sdk.dataset import DatasetReader

    path = _temp_file(i["text"], ".depzdata")
    try:
        records = DatasetReader(path).records
    finally:
        path.unlink()
    return {"records": [[r.device_id, r.t_host_us, r.kind] for r in records]}


# ── TMF8829 ──


def op_tmf8829_encode(i):
    cmd = i["cmd"]
    if cmd == TP.Tmf8829Cmd.READ_REG:
        p = TP.pack_read_reg(i["addr"], i["len"])
    elif cmd == TP.Tmf8829Cmd.WRITE_REG:
        p = TP.pack_write_reg(i["addr"], h(i["data"]))
    elif cmd == TP.Tmf8829Cmd.START_STREAM:
        p = TP.pack_start_stream(i["int_mask"], i["flags"])
    else:
        p = b""
    return {"payload": p.hex()}


def op_tmf8829_decode(i):
    return S.tmf8829_decode(i["report"], h(i["payload"]))


def op_tmf8829_reassembly(i):
    return S.tmf8829_reassembly(h(c) for c in i["chunks"])


def op_tmf8829_frame(i):
    return S.tmf8829_frame(h(i["raw"]))


def op_tmf8829_measurements(i):
    frames = [(h(f["raw"]), f["timestamp_us"], f["edge_us"]) for f in i["frames"]]
    return {"measurements": S.tmf8829_measurements(frames)}


def op_tmf8829_config_get(i):
    return {"value": cfg_get(h(i["page"]), i["field"])}


def op_tmf8829_config_set(i):
    pg = bytearray(h(i["page"]))
    cfg_set(pg, i["field"], i["value"])
    return {"page": bytes(pg).hex()}


def op_tmf8829_frame_time(i):
    clock = SensorClock()
    timer = CaptureTimer(clock)
    times = []
    for r in i["rows"]:
        check_uint("id", r["id"], 8)  # the frame-id byte, split here as the SDK's frame parser does
        kind = r["id"] & 0xF0
        if kind == 0x30:
            continue
        flags = clock.add(r["ts"], r["systick"])
        if kind == 0x20:
            timer.histograms()
            continue
        t = timer.results(r["id"] & 0x0F, r["systick"], r["t0"], r["t1"], r["ts"], r["edge_us"], flags)
        times.append(
            {"timestamp_us": t.timestamp_us, "t1_len_us": t.t1_len_us, "flags": t.flags, "ratio_q16": clock.ratio_q16}
        )
    return {"times": times}


# ── VL53L4CD ──

_L4_INFO = (
    "int_edges", "slots_skipped", "i2c_errors", "last_i2c_error", "model_id", "fw_status",
    "initialized", "xshut_level", "int_level", "i2c_khz",
)
_L4_RESULT = (
    "range_status", "distance_mm", "ambient_rate_kcps", "ambient_per_spad_kcps", "signal_rate_kcps",
    "signal_per_spad_kcps", "number_of_spad", "sigma_mm", "stream_count",
)
_L4_TUNING = {
    "offset": (L4.offset_raw, L4.decode_offset),
    "xtalk": (L4.xtalk_raw, L4.decode_xtalk),
    "signal_threshold": (L4.signal_threshold_raw, L4.decode_signal_threshold),
    "sigma_threshold": (L4.sigma_threshold_raw, L4.decode_sigma_threshold),
}


def op_vl53l4_encode(i):
    kind = i["kind"]
    if kind == "read_reg":
        p = L4P.pack_read_reg(i["addr"], i["len"])
    elif kind == "write_reg":
        p = L4P.pack_write_reg(i["addr"], h(i["data"]))
    elif kind == "xshut":
        p = L4P.pack_xshut(i["action"])
    elif kind == "start_stream":
        p = L4P.pack_start_stream(i["addr"], i["len"], i["flags"])
    elif kind == "set_i2c_speed":
        p = L4P.pack_set_i2c_speed(i["khz"])
    else:
        raise ValueError(kind)
    return {"payload": p.hex()}


def op_vl53l4_decode(i):
    p, rpt = h(i["payload"]), i["report"]
    if rpt == 0x91:
        r = L4P.RegData.unpack(p)
        return {"cmd": r.cmd, "timestamp_us": r.timestamp_us, "data": r.data.hex()}
    if rpt == 0x92:
        info = L4P.Vl53l4Info.unpack(p)
        return {k: getattr(info, k) for k in _L4_INFO}
    if rpt == 0x93:
        s = L4P.StreamData.unpack(p)
        return {"timestamp_us": s.timestamp_us, "addr": s.addr, "len": s.length, "data": s.data.hex()}
    raise ValueError(rpt)


def op_vl53l4_result_block(i):
    r = L4.parse_result_block(h(i["raw"]))
    return {k: getattr(r, k) for k in _L4_RESULT}


def op_vl53l4_timing_encode(i):
    a, b, im = L4.range_timing_registers(
        i["timing_budget_ms"], i["inter_measurement_ms"], i["osc_frequency"], i["clock_pll"]
    )
    return {"range_config_a": a, "range_config_b": b, "intermeasurement_raw": im}


def op_vl53l4_timing_decode(i):
    budget, inter = L4.decode_range_timing(
        i["intermeasurement_raw"], i["clock_pll"], i["osc_frequency"], i["range_config_a"]
    )
    return {"timing_budget_ms": budget, "inter_measurement_ms": inter}


def op_vl53l4_tuning_encode(i):
    return {"raw": _L4_TUNING[i["kind"]][0](i["value"])}


def op_vl53l4_tuning_decode(i):
    return {"value": _L4_TUNING[i["kind"]][1](i["raw"])}


# ── VL53L5CX / L7 ──

_L7_INFO = (
    "int_edges", "frames_dropped", "i2c_errors", "last_i2c_error", "lpn_level", "int_level",
    "i2c_khz", "frame_size", "streaming",
)


def op_vl53l7_encode(i):
    kind = i["kind"]
    if kind == "read_reg":
        p = L7P.pack_read_reg(i["addr"], i["len"])
    elif kind == "write_reg":
        p = L7P.pack_write_reg(i["addr"], h(i["data"]))
    elif kind == "pin_ctrl":
        p = L7P.pack_pin_ctrl(i["action"])
    elif kind == "set_i2c_speed":
        p = L7P.pack_set_i2c_speed(i["khz"])
    else:
        raise ValueError(kind)
    return {"payload": p.hex()}


def op_vl53l7_decode(i):
    if i["report"] != 0x92:
        raise ValueError(i["report"])
    info = L7P.Vl53l7Info.unpack(h(i["payload"]))
    return {k: getattr(info, k) for k in _L7_INFO}


def op_vl53l7_model(i):
    return {"class": _vl53l7_class(i["usb_model"], i["device_name"]).__name__.lower()}


# ── VL53L8 register bridge (contract 04) ──


def op_vl53l8_encode(i):
    kind = i["kind"]
    if kind == "start_stream":
        p = L8P.pack_start_stream(i["frame_size"])
    else:
        raise ValueError(kind)
    return {"payload": p.hex()}


def op_vl53l8_decode(i):
    return S.vl53l8_decode(i["report"], h(i["payload"]))


# ── VL53L 1D family ──


def op_vl53lx_encode(i):
    kind = i["kind"]
    if kind == "set_addr_width":
        p = LXP.pack_set_addr_width(i["width"])
    elif kind == "read_reg":
        p = LXP.pack_read_reg(i["addr"], i["len"])
    elif kind == "write_reg":
        p = LXP.pack_write_reg(i["addr"], h(i["data"]))
    elif kind == "xshut":
        p = LXP.pack_xshut(i["action"])
    elif kind == "set_i2c_speed":
        p = LXP.pack_set_i2c_speed(i["khz"])
    elif kind == "start_stream":
        p = LXP.pack_start_stream(i["addr"], i["len"], tuple((a, v) for a, v in i["clear"]), i["flags"])
    else:
        raise ValueError(kind)
    return {"payload": p.hex()}


def op_vl53lx_decode(i):
    if i["report"] != 0x92:
        raise ValueError(i["report"])
    return asdict(LXP.Vl53lxInfo.unpack(h(i["payload"])))


def op_vl53lx_model(i):
    return {
        "class": _vl53lx_class(i["usb_model"], i["device_name"]).__name__,
        "product": registry.product_from_board_name(i["device_name"]),
    }


def op_vl53lx_die_block(i):
    return asdict(LXD.decode_die_block(h(i["raw"]), i["variant"]))


def op_vl53lx_l0x_raw(i):
    return asdict(LXD.decode_l0x_raw(h(i["raw"])))


def op_vl53lx_histogram_raw(i):
    got = asdict(LXD.decode_histogram_raw(h(i["raw"])))
    got["bins"] = list(got["bins"])
    return got


# ── VL53L8 advanced + CNH ──


def op_vl53l8_motion_config(i):
    drv, p = make_driver()
    drv.motion_indicator_init(i["resolution"])
    return {"pack": p.dci[L8.DCI_MOTION_DETECTOR_CFG].hex()}


def op_vl53l8_detection_thresholds(i):
    drv, p = make_driver()
    drv.set_detection_thresholds(i["thresholds"])
    return {
        "start_block": p.dci[L8.DCI_DET_THRESH_START].hex(),
        "valid_status": p.dci[L8.DCI_DET_THRESH_VALID_STATUS].hex(),
    }


def op_vl53l8_xtalk_margin(i):
    drv, p = make_driver()
    drv.set_xtalk_margin(i["kcps"])
    return {"raw": struct.unpack_from("<I", p.dci[L8.DCI_XTALK_CFG], 0)[0]}


def op_vl53l8_cnh_decode(i):
    c = i["config"]
    cfg = CnhConfig()
    cfg.init_config(c["start_bin"], c["num_bins"], c["sub_sample"])
    cfg.create_agg_map(c["resolution"], *c["agg_map"])
    decoded = cnh.decode(cfg, h(i["cnh_raw"]))
    return {
        "aggregates": [
            {"hist_raw": list(a["hist_raw"]), "hist_scaler": list(a["hist_scaler"])}
            for a in decoded["aggregates"]
        ]
    }


OPS = {name[3:]: fn for name, fn in globals().items() if name.startswith("op_") and callable(fn)}


# ── canonical JSON + loop ─────────────────────────────────────────────────────


def canon(v):
    """Contract 15 §3 canonical form: enums as ints, bytes hex, tuples lists,
    non-finite floats as the strings "NaN" / "Infinity" / "-Infinity"."""
    if isinstance(v, bool) or v is None or isinstance(v, str):
        return v
    if isinstance(v, int):
        return int(v)
    if isinstance(v, float):
        if math.isnan(v):
            return "NaN"
        if math.isinf(v):
            return "Infinity" if v > 0 else "-Infinity"
        return v
    if isinstance(v, (bytes, bytearray)):
        return bytes(v).hex()
    if isinstance(v, dict):
        return {str(k): canon(x) for k, x in v.items()}
    if isinstance(v, (list, tuple)):
        return [canon(x) for x in v]
    if hasattr(v, "tolist"):  # numpy scalars / arrays
        return canon(v.tolist())
    return v


def handle(req: dict) -> dict:
    rid, op = req.get("id"), req.get("op")
    if op in UNSUPPORTED or op not in OPS:
        return {"id": rid, "error": "unsupported", "detail": UNSUPPORTED.get(op, "unknown op")}
    try:
        return {"id": rid, "out": canon(OPS[op](req["in"]))}
    except Exception as e:  # noqa: BLE001 — any SDK rejection is `invalid`
        return {"id": rid, "error": "invalid", "detail": f"{type(e).__name__}: {e}"[:300]}


def main() -> None:
    out = sys.stdout
    for line in sys.stdin:
        if not line.strip():
            continue
        out.write(json.dumps(handle(json.loads(line)), separators=(",", ":")) + "\n")
        out.flush()


if __name__ == "__main__":
    main()
