"""Regression tests for VL53L8 bug fixes (error typing + metric separation)."""

from __future__ import annotations

import struct

import pytest

from depz_sensor_sdk.errors import DepzTimeoutError
from depz_sensor_sdk.transport import Packet
from depz_sensor_sdk.transport.link import LoopbackLink
from depz_sensor_sdk.protocol.vl53l8 import Vl53l8Rpt
from depz_sensor_sdk.vl53l8 import Vl53l8Cx
from depz_sensor_sdk.vl53l8.uld import Vl53l8cxError


def _dev():
    a, _b = LoopbackLink.pair()
    return Vl53l8Cx(a, timeout=0.2)


def test_get_frame_raises_depz_timeout():
    dev = _dev()
    try:
        with pytest.raises(DepzTimeoutError):
            dev.get_frame(timeout=0.05)
    finally:
        dev.close()


class _RaisingUld:
    def parse_frame(self, raw):
        raise Vl53l8cxError(2, "corrupt")  # STATUS_CORRUPTED_FRAME


def _frame_chunk(ts: int, full: int, off: int, data: bytes) -> bytes:
    return struct.pack("<QHH", ts, full, off) + data


def test_parse_error_counted_separately_from_reassembly():
    dev = _dev()
    try:
        dev._uld = _RaisingUld()
        payload = _frame_chunk(1, 8, 0, b"\x00" * 8)  # completes a frame
        pkt = Packet(cmd=Vl53l8Rpt.VL53_FRAME, seq=0, payload=payload)
        handled = dev._handle_report(pkt)
        assert handled is True
        assert dev.frame_parse_errors == 1
        # a parse failure must NOT be miscounted as a reassembler gap discard
        assert dev.reassembler_discards == 0
    finally:
        dev.close()
