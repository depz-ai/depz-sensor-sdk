"""A stream report shorter than its own length field is counted in
`stream_parse_errors` and dropped — it must not raise inside the reader
(the strict decoders of contract 07 §3 refuse it with ValueError)."""

from __future__ import annotations

import struct

from depz_sensor_sdk.bno055 import Bno055
from depz_sensor_sdk.protocol.bno055 import Bno055Rpt
from depz_sensor_sdk.protocol.vl53l4 import Vl53l4Rpt
from depz_sensor_sdk.protocol.vl53lx import Vl53lxRpt
from depz_sensor_sdk.transport import Packet
from depz_sensor_sdk.transport.link import LoopbackLink
from depz_sensor_sdk.vl53l4 import Vl53l4cd
from depz_sensor_sdk.vl53lx import Vl53lx


def _link():
    a, _b = LoopbackLink.pair()
    return a


def test_bno055_truncated_stream_is_counted():
    dev = Bno055(_link(), timeout=0.2)
    try:
        # length field says 6, only 2 data bytes follow
        payload = struct.pack("<QBB", 1, 0x08, 6) + b"\x01\x02"
        assert dev._handle_report(Packet(cmd=Bno055Rpt.STREAM, seq=0, payload=payload))
        assert dev.stream_parse_errors == 1
    finally:
        dev.close()


def test_vl53l4_truncated_stream_is_counted():
    dev = Vl53l4cd(_link(), timeout=0.2)
    try:
        payload = struct.pack("<QHH", 1, 0x89, 17) + b"\x00\x01"
        assert dev._handle_report(Packet(cmd=Vl53l4Rpt.STREAM, seq=0, payload=payload))
        assert dev.stream_parse_errors == 1
    finally:
        dev.close()


def test_vl53lx_truncated_stream_is_counted():
    dev = Vl53lx(_link(), timeout=0.2)
    try:
        dev._driver = object()  # any driver: the report must fail before decoding
        payload = struct.pack("<QHH", 1, 0x89, 17) + b"\x00\x01"
        assert dev._handle_report(Packet(cmd=Vl53lxRpt.STREAM, seq=0, payload=payload))
        assert dev.stream_parse_errors == 1
    finally:
        dev._driver = None
        dev.close()
