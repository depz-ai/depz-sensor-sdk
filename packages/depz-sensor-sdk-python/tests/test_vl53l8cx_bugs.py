"""Regression tests for VL53L8 bug fixes (error typing + metric separation)."""

from __future__ import annotations

import struct

import pytest

from depz_sensor_sdk.errors import DepzTimeoutError
from depz_sensor_sdk.transport import Packet
from depz_sensor_sdk.transport.link import LoopbackLink
from depz_sensor_sdk.protocol.vl53l8 import Vl53l8Rpt
from depz_sensor_sdk.vl53l7 import Vl53l7cx
from depz_sensor_sdk.vl53l8 import Vl53l8cx
from depz_sensor_sdk.vl53l8.uld import Vl53l8cxError


def _dev():
    a, _b = LoopbackLink.pair()
    return Vl53l8cx(a, timeout=0.2)


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


class _IdPlatform:
    """Answers the is_alive register sequence with fixed silicon ids."""

    def __init__(self, device_id, revision_id, fail=False):
        self.regs = {0x00: device_id, 0x01: revision_id}
        self.fail = fail

    def wr_multi(self, addr, data):
        pass

    def rd_multi(self, addr, size):
        if self.fail:
            raise Vl53l8cxError(2, "bridge")
        return bytes([self.regs[addr]])


@pytest.mark.parametrize("cls, device_id, revision_id, alive", [
    (Vl53l8cx, 0xF0, 0x0C, True),
    (Vl53l8cx, 0xF0, 0x02, False),   # an L5/L7 answers, but it is not an L8
    (Vl53l8cx, 0x00, 0x0C, False),
    (Vl53l7cx, 0x00, 0x02, True),
    (Vl53l7cx, 0xF0, 0x01, True),
    (Vl53l7cx, 0x00, 0x01, False),
    (Vl53l7cx, 0xF0, 0x0C, False),   # an L8 on an L7 board
])
def test_is_alive_checks_silicon_ids(cls, device_id, revision_id, alive):
    a, _b = LoopbackLink.pair()
    dev = cls(a, timeout=0.2)
    try:
        dev._platform = _IdPlatform(device_id, revision_id)
        assert dev.is_alive() is alive
    finally:
        dev.close()


def test_is_alive_false_on_bridge_error():
    dev = _dev()
    try:
        dev._platform = _IdPlatform(0xF0, 0x0C, fail=True)
        assert dev.is_alive() is False
    finally:
        dev.close()
