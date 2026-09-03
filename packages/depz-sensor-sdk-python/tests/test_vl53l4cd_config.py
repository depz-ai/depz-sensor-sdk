"""VL53L4CD configuration surface: the ULD init/register sequences over the
fake register platform, get/set roundtrips, and the not-while-ranging guards
plus stream decode on the device wrapper."""

from __future__ import annotations

import struct

import pytest

from depz_sensor_sdk.errors import DepzError
from depz_sensor_sdk.protocol.vl53l4 import StreamData, Vl53l4Rpt
from depz_sensor_sdk.transport import Packet
from depz_sensor_sdk.transport.link import LoopbackLink
from depz_sensor_sdk.vl53l4 import (
    WINDOW_IN,
    Vl53l4Cd,
    Vl53l4Measurement,
)
from depz_sensor_sdk.vl53l4 import uld as uld_mod

from fake_vl53l4 import make_driver

# ── ULD init sequence ─────────────────────────────────────────────────────────


def test_sensor_init_runs_at_boot_speed_then_retimes():
    drv, p = make_driver()
    drv.sensor_init()
    assert p.speeds[0] == uld_mod.I2C_KHZ_BOOT
    assert p.speeds[-1] == uld_mod.I2C_KHZ_DEFAULT


def test_sensor_init_at_400_never_retimes():
    drv, p = make_driver()
    drv.sensor_init(bus_khz=uld_mod.I2C_KHZ_BOOT)
    assert p.speeds == [uld_mod.I2C_KHZ_BOOT]


def test_sensor_init_writes_config_block_with_fmp_byte():
    drv, p = make_driver()
    drv.sensor_init()
    blocks = p.written_to(uld_mod.CONFIG_ADDR)
    assert blocks and blocks[0] == uld_mod.config_block()
    assert blocks[0][0] == uld_mod.CONFIG_FMP_BYTE


def test_sensor_init_vhv_sequence_and_defaults():
    drv, p = make_driver()
    drv.sensor_init()
    starts = [d[0] for d in p.written_to(uld_mod.SYSTEM_START)]
    assert starts[:2] == [0x40, 0x80]  # start VHV, then stop ranging
    assert p.written_to(uld_mod.SYSTEM__INTERRUPT_CLEAR)[0] == b"\x01"
    assert p.written_to(uld_mod.VHV_CONFIG__TIMEOUT_MACROP_LOOP_BOUND)[-1] == b"\x09"
    assert p.written_to(0x000B)[-1] == b"\x00"
    assert p.written_to(0x0024)[-1] == b"\x05\x00"
    # init leaves the default 50 ms continuous timing programmed
    assert drv.get_range_timing()[1] == 0


# ── ULD get/set roundtrips ────────────────────────────────────────────────────


def test_range_timing_written_registers_decode_back():
    drv, p = make_driver()
    drv.set_range_timing(50, 0)
    a = int.from_bytes(p.written_to(uld_mod.RANGE_CONFIG_A)[-1], "big")
    b = int.from_bytes(p.written_to(uld_mod.RANGE_CONFIG_B)[-1], "big")
    exp_a, exp_b, exp_im = uld_mod.range_timing_registers(
        50, 0, int.from_bytes(p.rd_multi(uld_mod.OSC_FREQUENCY, 2), "big")
    )
    assert (a, b) == (exp_a, exp_b)
    assert int.from_bytes(p.written_to(uld_mod.INTERMEASUREMENT_MS)[-1], "big") == exp_im
    budget, inter = drv.get_range_timing()
    assert inter == 0
    assert abs(budget - 50) <= 1  # the C driver's math loses ≤1 ms in roundtrip


def test_range_timing_autonomous_mode():
    drv, p = make_driver()
    drv.set_range_timing(50, 1000)
    assert int.from_bytes(p.written_to(uld_mod.INTERMEASUREMENT_MS)[-1], "big") > 0
    budget, inter = drv.get_range_timing()
    assert inter > 0
    drv.start_ranging()
    assert p.written_to(uld_mod.SYSTEM_START)[-1] == b"\x40"  # autonomous


def test_start_ranging_continuous_mode_byte():
    drv, p = make_driver()
    drv.set_range_timing(50, 0)
    drv.start_ranging()
    assert p.written_to(uld_mod.SYSTEM_START)[-1] == b"\x21"
    drv.stop_ranging()
    assert p.written_to(uld_mod.SYSTEM_START)[-1] == b"\x80"


def test_range_timing_bounds():
    drv, _ = make_driver()
    with pytest.raises(uld_mod.Vl53l4cdError):
        drv.set_range_timing(9, 0)
    with pytest.raises(uld_mod.Vl53l4cdError):
        drv.set_range_timing(201, 0)
    with pytest.raises(uld_mod.Vl53l4cdError):
        drv.set_range_timing(50, 30)  # 0 < inter <= budget is invalid


def test_offset_roundtrip():
    drv, _ = make_driver()
    for mm in (10, -10, 0, 255):
        drv.set_offset(mm)
        assert drv.get_offset() == mm


def test_xtalk_roundtrip():
    drv, _ = make_driver()
    drv.set_xtalk(20)
    assert drv.get_xtalk() == 20
    drv.set_xtalk(0)
    assert drv.get_xtalk() == 0


def test_detection_thresholds_roundtrip():
    drv, _ = make_driver()
    drv.set_detection_thresholds(100, 300, WINDOW_IN)
    assert drv.get_detection_thresholds() == (100, 300, WINDOW_IN)


def test_signal_and_sigma_threshold_roundtrip():
    drv, _ = make_driver()
    drv.set_signal_threshold(1024)
    assert drv.get_signal_threshold() == 1024
    drv.set_sigma_threshold(15)
    assert drv.get_sigma_threshold() == 15
    with pytest.raises(uld_mod.Vl53l4cdError):
        drv.set_sigma_threshold(0x4000)  # > 16383


def test_is_alive_checks_model_id():
    drv, p = make_driver()
    assert drv.is_alive()
    p._seed_word(uld_mod.IDENTIFICATION__MODEL_ID, 0x0000)
    assert not drv.is_alive()


def test_parse_result_block_too_short_raises():
    with pytest.raises(uld_mod.Vl53l4cdError):
        uld_mod.parse_result_block(b"\x00" * 14)


# ── device wrapper (LoopbackLink; no fake firmware needed for guards) ────────


def _dev() -> Vl53l4Cd:
    a, _b = LoopbackLink.pair()
    return Vl53l4Cd(a, timeout=0.2)


def test_config_while_ranging_raises():
    dev = _dev()
    try:
        dev._ranging = True  # pretend the stream owns the register bank
        for call in (
            lambda: dev.set_range_timing(50, 0),
            lambda: dev.set_offset_mm(5),
            lambda: dev.set_xtalk_kcps(10),
            lambda: dev.set_detection_thresholds(100, 300, WINDOW_IN),
            lambda: dev.set_signal_threshold_kcps(1024),
            lambda: dev.set_sigma_threshold_mm(15),
            lambda: dev.set_i2c_speed_khz(400),
            lambda: dev.init(),
            lambda: dev.measure_once(),
            dev.start_ranging,
        ):
            with pytest.raises(DepzError):
                call()
    finally:
        dev.close()


def test_stop_ranging_noop_when_not_ranging():
    dev = _dev()
    try:
        dev.stop_ranging()  # no request goes out, no timeout
        assert not dev.ranging
    finally:
        dev.close()


def _stream_payload(ts: int, block: bytes) -> bytes:
    return struct.pack("<QHH", ts, uld_mod.RESULT_BLOCK_ADDR, len(block)) + block


def test_handle_report_decodes_stream_sample():
    dev = _dev()
    try:
        got: list[Vl53l4Measurement] = []
        dev.on_measurement(got.append)
        block = bytes.fromhex("0900070f0001f4006400280000012300ff")[:17]
        handled = dev._handle_report(
            Packet(cmd=Vl53l4Rpt.STREAM, seq=0, payload=_stream_payload(1234, block))
        )
        assert handled
        assert got and got[0].timestamp_us == 1234
        assert got[0].distance_mm == 291
        assert got[0].valid and got[0].status_text == "valid"
        assert got[0].stream_count == 7
    finally:
        dev.close()


def test_handle_report_counts_short_blocks():
    dev = _dev()
    try:
        handled = dev._handle_report(
            Packet(cmd=Vl53l4Rpt.STREAM, seq=0, payload=_stream_payload(1, b"\x00\x01"))
        )
        assert handled
        assert dev.stream_parse_errors == 1
    finally:
        dev.close()


def test_handle_report_ignores_other_reports():
    dev = _dev()
    try:
        assert not dev._handle_report(Packet(cmd=0x91, seq=0, payload=b"\x00" * 11))
    finally:
        dev.close()


def test_stream_data_echo_fields():
    block = b"\x09" + bytes(16)
    sd = StreamData.unpack(_stream_payload(42, block))
    assert (sd.timestamp_us, sd.addr, sd.length, sd.data) == (
        42,
        uld_mod.RESULT_BLOCK_ADDR,
        17,
        block,
    )
