"""`Vl53l4cd` (contract 10 register bridge) over the in-process fake bridge
FakeVl53l4cd (tests/fake_vl53l4.py), no hardware: init over the wire, the
XSHUT power states, bridge diagnostics, bus re-timing, every configuration
getter/setter as register round trips, both calibrations, the INT-driven
stream (callbacks, bounded iterator, get_measurement timeouts and close),
poll-mode measure_once, chunked register transfers and the bridge error
statuses (StatusError / short reads / a failing STOP_STREAM)."""

from __future__ import annotations

import struct
import threading
import time

import pytest

from depz_sensor_sdk.errors import DepzError, DepzTimeoutError, InvalidStateError, LinkClosedError, StatusError
from depz_sensor_sdk.protocol.vl53l4 import XFER_MAX, Vl53l4Rpt
from depz_sensor_sdk.transport import Packet
from depz_sensor_sdk.vl53l4 import (
    I2C_KHZ_BOOT,
    I2C_KHZ_DEFAULT,
    MODEL_ID_VL53L4CD,
    WINDOW_OUT,
    Vl53l4cd,
    Vl53l4cdError,
    Vl53l4Measurement,
    XSHUT_OFF,
    XSHUT_ON,
    XSHUT_RESET,
)
from depz_sensor_sdk.vl53l4 import uld as uld_mod

from fake_vl53l4 import CLOCK_PLL_TYPICAL, OSC_FREQUENCY_TYPICAL, FakeVl53l4cd, result_block

READ_REG, WRITE_REG, XSHUT = 0x32, 0x33, 0x34
START_STREAM, STOP_STREAM, GET_INFO, SET_I2C_SPEED = 0x35, 0x36, 0x37, 0x38
ERR_HARDWARE_FAULT = 0x09


@pytest.fixture()
def connect():
    made: list[tuple[Vl53l4cd, FakeVl53l4cd]] = []

    def make(*, timeout: float = 0.5) -> tuple[Vl53l4cd, FakeVl53l4cd]:
        fake = FakeVl53l4cd()
        dev = Vl53l4cd(fake.link, timeout=timeout)
        made.append((dev, fake))
        return dev, fake

    yield make
    for dev, fake in made:
        dev.close()
        fake.close()


@pytest.fixture()
def ready(connect):
    def make() -> tuple[Vl53l4cd, FakeVl53l4cd]:
        dev, fake = connect()
        dev.init()
        fake.rx.clear()
        fake.platform.writes.clear()
        return dev, fake

    return make


def settle(dev: Vl53l4cd, fake: FakeVl53l4cd) -> None:
    """Wait until the reader thread has handled everything the fake sent
    before: a GET_INFO round trip queues behind the reports."""
    dev.bridge_info()
    fake.rx.pop()


def _later(fn, delay: float = 0.05) -> threading.Thread:
    t = threading.Thread(target=lambda: (time.sleep(delay), fn()), daemon=True)
    t.start()
    return t


# ── init / identity ──────────────────────────────────────────────────────────


def test_init_runs_the_uld_boot_sequence_over_the_bridge(connect):
    dev, fake = connect()
    assert not dev.initialized
    dev.init()
    assert dev.initialized and not dev.ranging
    # config block at the boot speed, then the bridge is re-timed to 1 MHz
    assert fake.platform.speeds == [I2C_KHZ_BOOT, I2C_KHZ_DEFAULT]
    assert fake.platform.written_to(uld_mod.CONFIG_ADDR) == [uld_mod.config_block()]
    speed_cmds = [i for i, c in enumerate(fake.cmds()) if c == SET_I2C_SPEED]
    config_write = fake.cmds().index(WRITE_REG)
    assert speed_cmds[0] < config_write < speed_cmds[1]
    # init's 50 ms continuous default reads back as 49 (ST's integer math;
    # vectors/vl53l4.json timing/decode continuous_readback)
    assert dev.get_range_timing() == (49, 0)
    assert dev.uld is not None and isinstance(dev.uld, uld_mod.VL53L4CD)


def test_init_at_a_chosen_bus_speed(connect):
    dev, fake = connect()
    dev.init(bus_khz=400)
    assert fake.platform.speeds == [400]
    assert dev.bridge_info().i2c_khz == 400


def test_is_alive_checks_the_model_id_and_swallows_bridge_errors(connect):
    dev, fake = connect()
    assert dev.is_alive()
    fake.platform._seed_word(uld_mod.IDENTIFICATION__MODEL_ID, 0xEACC)
    assert not dev.is_alive()
    fake.fail_next(READ_REG, ERR_HARDWARE_FAULT)
    assert not dev.is_alive()


# ── XSHUT ────────────────────────────────────────────────────────────────────


def test_xshut_off_powers_the_sensor_down_and_forgets_the_init(ready):
    dev, fake = ready()
    dev.start_ranging()
    dev.xshut(XSHUT_OFF)
    assert fake.rx[-1] == (XSHUT, bytes([XSHUT_OFF]))
    assert not fake.powered and not fake.streaming
    assert not dev.initialized and not dev.ranging
    # every register access now answers ERR_HARDWARE_FAULT (contract 10 §2)
    assert not dev.is_alive()
    with pytest.raises(StatusError):
        dev.get_offset_mm()
    with pytest.raises(StatusError):
        dev.init()
    info = dev.bridge_info()
    assert (info.xshut_level, info.initialized, info.model_id) == (0, 0, 0)
    assert info.i2c_errors >= 3 and info.last_i2c_error == 1
    dev.xshut(XSHUT_ON)
    assert fake.powered and dev.is_alive()


def test_reset_sensor_wipes_the_configuration_and_needs_init_again(ready):
    dev, fake = ready()
    dev.set_offset_mm(7)
    dev.reset_sensor()
    assert fake.rx[-1] == (XSHUT, bytes([XSHUT_RESET]))
    assert not dev.initialized
    assert dev.get_offset_mm() == 0  # back to the boot register file
    assert fake.platform.written_to(uld_mod.CONFIG_ADDR) == []
    dev.init()
    assert dev.initialized
    assert fake.platform.written_to(uld_mod.CONFIG_ADDR) == [uld_mod.config_block()]


# ── bridge diagnostics / bus speed ───────────────────────────────────────────


def test_bridge_info_decodes_the_21_byte_report(ready):
    dev, fake = ready()
    info = dev.bridge_info()
    assert fake.rx[-1] == (GET_INFO, b"")
    assert (info.model_id, info.fw_status, info.initialized) == (MODEL_ID_VL53L4CD, 0x03, 1)
    assert (info.xshut_level, info.i2c_khz, info.int_edges) == (1, I2C_KHZ_DEFAULT, 0)
    # counters move with the INT edges; GET_INFO is safe while streaming
    dev.start_ranging()
    fake.send_result(100)
    fake.send_result(101)
    assert dev.bridge_info().int_edges == 2


def test_set_i2c_speed_khz_retimes_the_bridge(ready):
    dev, fake = ready()
    dev.set_i2c_speed_khz(400)
    assert fake.rx[-1] == (SET_I2C_SPEED, struct.pack("<H", 400))
    assert dev.bridge_info().i2c_khz == 400


# ── configuration round trips (init first; not while ranging) ────────────────


def test_every_configuration_method_round_trips_through_the_registers(ready):
    dev, fake = ready()
    dev.set_range_timing(100, 250)
    # readback is ST's integer math over the programmed words (held byte-exact
    # by vectors/vl53l4.json timing/*): compare with the pure decode
    a, _b, inter = uld_mod.range_timing_registers(100, 250, OSC_FREQUENCY_TYPICAL, CLOCK_PLL_TYPICAL)
    assert dev.uld.rd_word(uld_mod.RANGE_CONFIG_A) == a
    assert dev.uld.rd_dword(uld_mod.INTERMEASUREMENT_MS) == inter
    assert dev.get_range_timing() == uld_mod.decode_range_timing(
        inter, CLOCK_PLL_TYPICAL, OSC_FREQUENCY_TYPICAL, a
    )
    assert 240 <= dev.get_range_timing()[1] < 250  # the 1.055 / 1.065 factor pair reads short
    dev.set_range_timing(33)
    assert dev.get_range_timing()[1] == 0  # continuous again
    dev.set_offset_mm(-12)
    assert dev.get_offset_mm() == -12
    assert fake.platform.written_to(uld_mod.RANGE_OFFSET_MM)[-1] == (-48 & 0xFFFF).to_bytes(2, "big")
    dev.set_xtalk_kcps(20)
    assert dev.get_xtalk_kcps() == 20
    assert fake.platform.written_to(uld_mod.XTALK_PLANE_OFFSET_KCPS)[-1] == (20 << 9).to_bytes(2, "big")
    dev.set_detection_thresholds(100, 300, WINDOW_OUT)
    assert dev.get_detection_thresholds() == (100, 300, WINDOW_OUT)
    assert fake.platform.written_to(uld_mod.SYSTEM__INTERRUPT)[-1] == bytes([WINDOW_OUT])
    dev.set_signal_threshold_kcps(1024)
    assert dev.get_signal_threshold_kcps() == 1024
    assert fake.platform.written_to(uld_mod.MIN_COUNT_RATE_RTN_LIMIT_MCPS)[-1] == (1024 >> 3).to_bytes(2, "big")
    dev.set_sigma_threshold_mm(15)
    assert dev.get_sigma_threshold_mm() == 15
    assert fake.platform.written_to(uld_mod.RANGE_CONFIG__SIGMA_THRESH)[-1] == (15 << 2).to_bytes(2, "big")


def test_argument_errors_pass_through_unchanged(ready):
    dev, fake = ready()
    # ERRATA A16: a refused argument is invalid_argument (ValueError), not the
    # driver's Vl53l4cdError
    fake.rx.clear()
    with pytest.raises(ValueError):
        dev.set_range_timing(50, 30)
    with pytest.raises(ValueError):
        dev.set_range_timing(9, 0)
    assert fake.rx == []  # range timing refuses before any traffic, reads included
    with pytest.raises(ValueError):
        dev.set_offset_mm(5000)
    with pytest.raises(ValueError):
        dev.set_xtalk_kcps(128)
    with pytest.raises(ValueError):
        dev.set_sigma_threshold_mm(16384)
    # the refused value never reaches its register
    for reg in (uld_mod.RANGE_CONFIG_A, uld_mod.INTERMEASUREMENT_MS, uld_mod.RANGE_OFFSET_MM,
                uld_mod.XTALK_PLANE_OFFSET_KCPS, uld_mod.RANGE_CONFIG__SIGMA_THRESH):
        assert fake.platform.written_to(reg) == []
    fake.rx.clear()
    with pytest.raises(ValueError):
        dev.calibrate_offset(5)
    with pytest.raises(ValueError):
        dev.calibrate_xtalk(100, nb_samples=4)
    assert fake.rx == []  # calibrations refuse before any traffic


def test_start_temperature_update_runs_the_vhv_burst(ready):
    dev, fake = ready()
    dev.start_temperature_update()
    p = fake.platform
    assert p.written_to(uld_mod.VHV_CONFIG__TIMEOUT_MACROP_LOOP_BOUND) == [b"\x81", b"\x09"]
    assert [d[0] for d in p.written_to(uld_mod.SYSTEM_START)] == [0x21, 0x80]
    assert not dev.ranging


def test_calibrations_run_their_sample_bursts_over_the_bridge(ready):
    dev, fake = ready()
    fake.set_result_block(result_block(120, signal_kcps=1200, spads=2048))
    assert dev.calibrate_offset(150, nb_samples=5) == 30
    assert dev.get_offset_mm() == 30
    assert dev.calibrate_xtalk(240, nb_samples=5) == 75  # (1 − 120/240) · 1200/8
    assert dev.get_xtalk_kcps() == 75
    assert not dev.ranging
    # each burst: start, (ready, result, clear) × n, stop — all over READ/WRITE_REG
    assert set(fake.cmds()) == {READ_REG, WRITE_REG}


def test_calibrations_and_temperature_update_refuse_while_ranging(ready):
    dev, fake = ready()
    dev.start_ranging()
    fake.rx.clear()
    for call in (dev.start_temperature_update, lambda: dev.calibrate_offset(100),
                 lambda: dev.calibrate_xtalk(100), dev.init):
        with pytest.raises(InvalidStateError):
            call()
    assert fake.rx == []


# ── ranging: the INT-driven stream ───────────────────────────────────────────


def test_start_ranging_starts_the_sensor_then_arms_the_stream(ready):
    dev, fake = ready()
    dev.start_ranging()
    assert dev.ranging and fake.streaming
    assert fake.platform.written_to(uld_mod.SYSTEM_START)[-1] == b"\x21"
    assert fake.rx[-1] == (START_STREAM, struct.pack("<HHB", 0x0089, 17, 0))
    assert (fake.stream_addr, fake.stream_len) == (uld_mod.RESULT_BLOCK_ADDR, uld_mod.RESULT_BLOCK_LEN)
    with pytest.raises(InvalidStateError):
        dev.start_ranging()  # already ranging
    dev.stop_ranging()
    assert not dev.ranging and not fake.streaming
    # the bridge stream is released first, then the sensor is told to stop
    assert fake.rx[-2:] == [(STOP_STREAM, b""), (WRITE_REG, struct.pack("<H", uld_mod.SYSTEM_START) + b"\x80")]


def test_stop_ranging_clears_the_state_even_when_the_bridge_refuses(ready):
    dev, fake = ready()
    dev.start_ranging()
    fake.fail_next(STOP_STREAM, ERR_HARDWARE_FAULT)
    with pytest.raises(StatusError):
        dev.stop_ranging()
    assert not dev.ranging
    # the sensor itself was still told to stop, so reconfiguration is possible
    assert fake.platform.written_to(uld_mod.SYSTEM_START)[-1] == b"\x80"
    dev.set_offset_mm(3)
    assert dev.get_offset_mm() == 3


def test_streamed_blocks_reach_callbacks_and_the_measurements_iterator(ready):
    dev, fake = ready()
    got: list[Vl53l4Measurement] = []
    unsubscribe = dev.on_measurement(got.append)
    it = dev.measurements(maxsize=4)  # subscribes now: get_measurement() only sees samples sent after it is called
    dev.start_ranging()
    raw = fake.send_result(291, sigma_mm=3, signal_kcps=1600, spads=2048, stream_count=7)
    m = next(it)
    assert m == Vl53l4Measurement._from_results(fake.mcu_time_us, uld_mod.parse_result_block(raw))
    assert (m.distance_mm, m.sigma_mm, m.signal_rate_kcps, m.number_of_spad) == (291, 3, 1600, 8)
    assert (m.stream_count, m.valid, m.status_text) == (7, True, "valid")
    assert got == [m]
    unsubscribe()
    fake.send_result(77, status=4)
    m2 = next(it)
    assert (m2.distance_mm, m2.range_status, m2.valid) == (77, 2, False)
    assert m2.status_text == "signal below threshold"
    assert got == [m]  # unsubscribed
    it.close()


def test_measurements_iterator_is_bounded_drop_oldest_and_ends_on_close(ready):
    dev, fake = ready()
    it = dev.measurements(maxsize=1)
    dev.start_ranging()
    for d in (10, 20, 30):
        fake.send_result(d)
    settle(dev, fake)
    assert it.dropped_count == 2
    assert dev.stream_dropped_counts == [2]
    assert next(it).distance_mm == 30
    dev.close()
    with pytest.raises(StopIteration):
        next(it)
    assert dev.stream_dropped_counts == []  # deregistered


def test_get_measurement_times_out_when_nothing_streams(ready):
    dev, fake = ready()
    dev.start_ranging()
    t0 = time.monotonic()
    with pytest.raises(DepzTimeoutError):
        dev.get_measurement(timeout=0.15)
    assert 0.1 <= time.monotonic() - t0 < 1.0
    assert dev.stream_dropped_counts == []  # the one-shot queue is gone


def test_get_measurement_ends_with_link_closed_on_a_closed_device(ready):
    dev, fake = ready()
    dev.start_ranging()
    _later(dev.close, 0.05)
    with pytest.raises(LinkClosedError, match="while waiting"):
        dev.get_measurement(timeout=5.0)
    with pytest.raises(LinkClosedError, match="is closed"):
        dev.get_measurement(timeout=5.0)


def test_a_queued_sample_wins_over_a_concurrent_close(ready):
    dev, fake = ready()
    dev.start_ranging()
    # a sample the reader has already queued is returned even though the
    # device is closed by the time the waiter looks again
    def deliver():
        fake.send_result(625)
        settle(dev, fake)
        dev.close()

    _later(deliver, 0.05)
    assert dev.get_measurement(timeout=5.0).distance_mm == 625
    with pytest.raises(LinkClosedError):
        dev.get_measurement(timeout=5.0)


def test_undecodable_stream_blocks_are_counted_not_fatal(ready):
    dev, fake = ready()
    it = dev.measurements(maxsize=4)  # subscribes before the blocks are sent
    dev.start_ranging()
    fake.stream_len = 4  # somebody re-armed the stream on a different block
    fake._fw._send(0x93, struct.pack("<QHH", 1, 0x0089, 4) + b"\x00" * 4)
    fake.stream_len = 17
    fake.send_result(624)
    assert next(it).distance_mm == 624
    it.close()
    assert dev.stream_parse_errors == 1
    # a report shorter than its header is not a stream sample at all
    assert not dev._handle_report(Packet(cmd=Vl53l4Rpt.STREAM, seq=0, payload=b"\x00" * 11))
    assert dev.stream_parse_errors == 1


# ── poll mode ────────────────────────────────────────────────────────────────


def test_measure_once_polls_one_result_and_stops(ready):
    dev, fake = ready()
    fake.set_result_block(result_block(333, stream_count=5))
    m = dev.measure_once()
    assert (m.distance_mm, m.stream_count, m.valid) == (333, 5, True)
    assert m.timestamp_us == fake.mcu_time_us - 100  # the block read's MCU stamp
    starts = [d[0] for d in fake.platform.written_to(uld_mod.SYSTEM_START)]
    assert starts == [0x21, 0x80]
    assert fake.platform.written_to(uld_mod.SYSTEM__INTERRUPT_CLEAR) == [b"\x01"]
    assert START_STREAM not in fake.cmds() and not dev.ranging


def test_measure_once_times_out_and_still_stops_the_sensor(ready):
    dev, fake = ready()
    fake.platform.reg[uld_mod.GPIO__TIO_HV_STATUS] = 0x01  # INT never asserted
    with pytest.raises(Vl53l4cdError, match="data ready"):
        dev.measure_once(timeout=0.01)
    assert fake.platform.written_to(uld_mod.SYSTEM_START)[-1] == b"\x80"


# ── register transfers and bridge errors ─────────────────────────────────────


def test_register_transfers_past_253_bytes_go_in_chunks(ready):
    dev, fake = ready()
    data = bytes(range(256)) + bytes(44)  # 300 bytes
    dev.uld.p.wr_multi(0x0100, data)
    writes = [p for c, p in fake.rx if c == WRITE_REG]
    assert [struct.unpack_from("<H", p)[0] for p in writes] == [0x0100, 0x0100 + XFER_MAX]
    assert [len(p) - 2 for p in writes] == [XFER_MAX, 300 - XFER_MAX]
    fake.rx.clear()
    assert dev.uld.p.rd_multi(0x0100, 300) == data
    reads = [struct.unpack("<HH", p) for c, p in fake.rx if c == READ_REG]
    assert reads == [(0x0100, XFER_MAX), (0x0100 + XFER_MAX, 300 - XFER_MAX)]


def test_a_short_register_read_is_an_error(ready):
    dev, fake = ready()
    fake.short_read_next = True
    with pytest.raises(DepzError, match="expected 2, got 1"):
        dev.get_offset_mm()


def test_bridge_error_statuses_raise_status_error(ready):
    dev, fake = ready()
    fake.fail_next(WRITE_REG, ERR_HARDWARE_FAULT)
    with pytest.raises(StatusError):
        dev.set_offset_mm(1)
    fake.fail_next(READ_REG, ERR_HARDWARE_FAULT)
    with pytest.raises(StatusError):
        dev.get_range_timing()
    fake.fail_next(XSHUT, ERR_HARDWARE_FAULT)  # RESET boot handshake timed out
    with pytest.raises(StatusError):
        dev.reset_sensor()
    fake.fail_next(START_STREAM, ERR_HARDWARE_FAULT)
    with pytest.raises(StatusError):
        dev.start_ranging()
    assert not dev.ranging


def test_no_answer_is_a_timeout(connect):
    dev, fake = connect(timeout=0.1)
    fake.fail_next(GET_INFO, "silent")
    with pytest.raises(DepzTimeoutError):
        dev.bridge_info()
    fake.fail_next(START_STREAM, "silent")
    dev.init()
    with pytest.raises(DepzTimeoutError):
        dev.start_ranging()
    assert not dev.ranging
