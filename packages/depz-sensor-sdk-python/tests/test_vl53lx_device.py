"""`Vl53lx` (APP_VL53L0_4 bridge, contract 12) driven through an in-process
fake bridge with a VL53L1-family register file behind it (fake_vl53lx.py) —
no hardware. Wire bytes are checked against vectors/vl53lx.json, decoded
frames against its `die_block` / `decode` entries. The full-stack captures
(test_scenarios, test_vl53lx_configure_replay) cover the init/stream path of
each pair; this covers what they cannot: refusals, bridge error statuses,
timeouts, the capability-gated passthroughs, calibrations, poll mode, queues
and the histogram decode chain. The TypeScript vl53lx-device tests hold the
same.
"""

from __future__ import annotations

import threading
import time

import pytest

from depz_sensor_sdk.errors import DepzError, DepzTimeoutError, InvalidStateError, LinkClosedError, StatusError
from depz_sensor_sdk.vl53lx import (
    PLOTTABLE_STATUSES,
    XSHUT_OFF,
    XSHUT_RESET,
    ProtocolError,
    Target,
    Vl53Error,
    Vl53l0x,
    Vl53l1cx,
    Vl53l4cx,
    Vl53lx,
    Vl53lxMeasurement,
    plot_distances,
    primary_target,
)
from depz_sensor_sdk.vl53lx.uld import registry
from depz_sensor_sdk.vl53lx.uld.base import Measurement, SensorDriver
from depz_sensor_sdk.vl53lx.uld.l0x import VL53L0X
from depz_sensor_sdk.vl53lx.uld.vl53l1_die import phase_window_mm
from fake_vl53lx import DIE_OSC, DIE_PLL, FakeVl53lxBridge

# vectors/vl53lx.json
L4_BLOCK = bytes.fromhex("090901d3200285008900082ba702700285")  # die_block vl53l4cx_uld_as_l4cd_50ms_f2
L4_BLOCK_F9 = bytes.fromhex("090908d320027f008a00092bce026f027f")  # ..._f9
L1_BLOCK = bytes.fromhex("090901439007d9013f000d2234024007d9")  # die_block vl53l1cx_uld_long_33ms_f2
START_STREAM_DIE = "890011000001860001"  # encode start_stream_die_block
INFO_WRAPPED = bytes.fromhex("ffffffff0700000003000000020100e80302010c000000")  # decode info_wrapped_counters

L4CD_BOARD = "DEPZ ToF Sensor VL53L4CD USB v2.1 TESTSERIAL"
L4CX_BOARD = "DEPZ ToF Sensor VL53L4CX USB v2.1 TOVJALN523"
L1CX_BOARD = "DEPZ ToF Sensor VL53L1CX USB v2.1 SER"
L3CX_BOARD = "DEPZ ToF Sensor VL53L3CX USB v2.1 SER"
L0X_BOARD = "DEPZ ToF Sensor VL53L0X USB v2.1 SER"
UNSTAMPED = "DEPZ ToF Sensor USB v2.1"

READ_REG, WRITE_REG, XSHUT, START_STREAM, STOP_STREAM = 0x32, 0x33, 0x34, 0x35, 0x36
GET_INFO, SET_I2C_SPEED, SET_ADDR_WIDTH, CLEAR_I2C_ERRORS = 0x37, 0x38, 0x39, 0x3A
GET_DEVICE_NAME = 0x03


@pytest.fixture()
def rigs():
    made: list[tuple[Vl53lx, FakeVl53lxBridge]] = []
    yield made
    for dev, fake in made:
        dev.close()
        fake.close()


@pytest.fixture()
def connect(rigs):
    def make(board=L4CD_BOARD, cls=Vl53lx, *, timeout=0.5, **fake_opts):
        fake = FakeVl53lxBridge(board, **fake_opts)
        dev = cls(fake.link, timeout=timeout)
        rigs.append((dev, fake))
        return dev, fake
    return make


@pytest.fixture()
def ready(connect):
    def make(board=L4CD_BOARD, product=None, cls=Vl53lx):
        dev, fake = connect(board, cls)
        dev.init(product=product)
        fake.rx.clear()
        return dev, fake
    return make


def settle(fake: FakeVl53lxBridge, dev: Vl53lx) -> None:
    """Wait until the reader thread has handled everything the fake sent: a
    GET_INFO round trip behind the reports."""
    dev.bridge_info()
    fake.rx.pop()


def _later(fn, delay: float = 0.05) -> threading.Thread:
    t = threading.Thread(target=lambda: (time.sleep(delay), fn()), daemon=True)
    t.start()
    return t


def _when_waiting(dev: Vl53lx, fn) -> threading.Thread:
    """Run `fn` once the next get_measurement is waiting. It takes the *next*
    measurement, and one sent before it subscribes is not that one: a fixed
    delay raced it (the full suite under coverage on Windows)."""
    n = len(dev._measure_queues)

    def run() -> None:
        deadline = time.monotonic() + 5.0
        while len(dev._measure_queues) <= n and time.monotonic() < deadline:
            time.sleep(0.001)
        fn()

    t = threading.Thread(target=run, daemon=True)
    t.start()
    return t


# ── pure helpers ─────────────────────────────────────────────────────────────

def _t(distance_mm, status):
    return Target(distance_mm, status, "", 0, 0, 0, distance_mm, distance_mm)


def _m(distance_mm=0, status=0, targets=()):
    return Vl53lxMeasurement._from(0, Measurement(distance_mm=distance_mm, status=status,
                                                  targets=targets))


def test_plottable_statuses_are_0_6_11():
    assert PLOTTABLE_STATUSES == (0, 6, 11)


def test_plot_distances_keeps_plottable_targets_in_driver_order():
    m = _m(targets=(_t(600, 6), _t(900, 4), _t(1500, 11), _t(30, 0)))
    assert plot_distances(m) == [600, 1500, 30]
    assert primary_target(m) == _t(600, 6)


def test_light_driver_uses_the_single_distance_or_nothing():
    assert plot_distances(_m(624, 0)) == [624]
    assert plot_distances(_m(624, 4)) == []
    assert primary_target(_m(624)) is None
    assert primary_target(_m(targets=(_t(1, 7),))) is None
    six = _m(624, 6)
    assert (six.valid, six.plottable) == (False, True)


# ── identity and init ────────────────────────────────────────────────────────

def test_before_init_no_driver_no_product(connect):
    dev, _ = connect()
    assert dev.initialized is False
    assert (dev.product, dev.driver_kind) == (None, None)
    with pytest.raises(InvalidStateError):
        dev.driver
    with pytest.raises(InvalidStateError):
        dev.supports("offset")
    with pytest.raises(InvalidStateError):
        dev.get_offset_mm()


def test_init_binds_the_product_and_sets_the_address_width_first(connect):
    dev, fake = connect()
    dev.init()
    assert dev.initialized is True
    assert (dev.product, dev.driver_kind) == ("VL53L4CD", "uld")
    assert dev.supports("offset") is True
    assert dev.supports("roi") is False
    assert dev.modes == ()

    cmds = fake.cmds()
    # device name, then SET_ADDR_WIDTH 2 before the first register access
    assert cmds[:2] == [GET_DEVICE_NAME, SET_ADDR_WIDTH]
    assert fake.rx[1][1].hex() == "02"
    assert cmds.index(SET_ADDR_WIDTH) < cmds.index(READ_REG)
    # every init runs at 400 kHz, then the die's 1 MHz ceiling
    assert [p.hex() for p in fake.sent(SET_I2C_SPEED)] == ["9001", "e803"]
    # the blob at 0x2D with byte 0 forced to FM+ (0x12)
    blob = next(d for a, d in fake.writes() if a == 0x2D)
    assert len(blob) == 91 and blob[0] == 0x12
    # VL53L4CD_SensorInit() closes with ALGO__RANGE_IGNORE_THRESHOLD_MCPS
    assert (0x0024, [0x05, 0x00]) in fake.writes()
    # CLEAR_I2C_ERRORS closes every init (contract 12 §2)
    assert cmds[-1] == CLEAR_I2C_ERRORS
    assert fake.sent(CLEAR_I2C_ERRORS) == [b""]


def test_l0x_sets_address_width_1_before_anything_else(connect, monkeypatch):
    """Contract 12 §1: VL53L0X is the one 1-byte-address part; the width goes
    on before the driver's first register access. A class with its own
    product does not need the device name."""
    monkeypatch.setattr(VL53L0X, "sensor_init", lambda self: None)
    dev, fake = connect(L0X_BOARD, Vl53l0x)
    dev.init()
    assert fake.cmds() == [SET_ADDR_WIDTH, CLEAR_I2C_ERRORS]
    assert fake.sent(SET_ADDR_WIDTH) == [b"\x01"]


def test_device_name_is_read_once(connect):
    dev, fake = connect()
    assert dev.detected == "VL53L4CD"
    dev.init()
    dev.notes()
    dev.identify_product()
    assert len(fake.sent(GET_DEVICE_NAME)) == 1
    assert dev.board_name == L4CD_BOARD


def test_identify_product_reports_both_reaches_and_the_model_id_check(ready):
    dev, _ = ready()
    info = dev.identify_product()
    assert {k: info[k] for k in (
        "board", "detected", "product", "driver", "driver_class", "kinds", "model_id",
        "model_id_ok", "modes", "reach_mm", "budget_ms", "budget_choices", "histogram",
        "max_khz", "caveat")} == {
        "board": L4CD_BOARD, "detected": "VL53L4CD", "product": "VL53L4CD", "driver": "uld",
        "driver_class": "VL53L4", "kinds": ("uld", "histogram"), "model_id": 0xEBAA,
        "model_id_ok": True, "modes": (), "reach_mm": 1300, "budget_ms": (10, 200),
        "budget_choices": (), "histogram": False, "max_khz": 1000, "caveat": None,
    }
    # §5: the 0x38 window at the die's own oscillator, ~1388 mm
    assert info["driver_reach_mm"] == phase_window_mm(DIE_OSC, 0x38) == 1388
    assert "calib_xtalk" in info["supports"]
    assert dev.notes() == []


def test_a_wrong_model_id_fails_the_cross_check_not_the_init(ready):
    dev, fake = ready()
    fake.set_word(0x010F, 0xEACC)
    info = dev.identify_product()
    assert (info["model_id"], info["model_id_ok"]) == (0xEACC, False)


def test_borrowing_a_siblings_driver(ready):
    dev, _ = ready(L4CX_BOARD, "VL53L4CD")
    info = dev.identify_product()
    assert (info["detected"], info["product"]) == ("VL53L4CX", "VL53L4CD")
    assert (info["reach_mm"], info["driver_reach_mm"]) == (6000, 1388)
    notes = dev.notes()
    assert len(notes) == 2
    assert "borrowing this driver - the board says it is a VL53L4CX" in notes[0]
    assert "reaches 1388 mm, the board is rated 6000 mm" in notes[1]


def test_an_unstamped_board_needs_the_product_named(connect):
    dev, fake = connect(UNSTAMPED)
    assert dev.detected is None
    assert dev.driver_kinds() == ()
    with pytest.raises(InvalidStateError):
        dev.init()
    assert fake.sent(SET_ADDR_WIDTH) == []
    dev.init(product="VL53L4CD")
    assert dev.product == "VL53L4CD"
    assert dev.notes() == ["VL53L4CD: board name carries no product number, named by hand"]


def test_a_pair_the_table_has_no_row_for_is_refused_before_any_traffic(connect):
    dev, fake = connect(L4CX_BOARD, Vl53l4cx)
    assert dev.driver_kinds() == ("histogram",)
    with pytest.raises(NotImplementedError, match="VL53L4CX has no 'uld' driver - it has histogram"):
        dev.init("uld")
    with pytest.raises(NotImplementedError, match="no such product"):
        dev.init(product="VL53L9ZZ")
    assert [c for c in fake.cmds() if c != GET_DEVICE_NAME] == []
    assert dev.initialized is False


def test_driver_kinds_of_a_named_product_and_of_the_class_product(connect):
    dev, _ = connect(UNSTAMPED, Vl53l1cx)
    assert dev.driver_kinds() == ("uld", "histogram")
    assert dev.driver_kinds("VL53L3CX") == ("ulp", "histogram")


def test_a_v2_00_bridge_is_told_to_reflash(connect):
    dev, _ = connect(no_clear_i2c_errors=True)
    with pytest.raises(DepzError, match=r"predates APP_VL53L0_4_v0.24.*reflash") as exc:
        dev.init()
    assert not isinstance(exc.value, ProtocolError)
    assert dev.initialized is False


def test_any_other_clear_i2c_errors_failure_is_a_protocol_error(connect):
    dev, fake = connect()
    fake.fail_next(CLEAR_I2C_ERRORS, 0x09)
    with pytest.raises(ProtocolError):
        dev.init()


def test_a_silent_clear_i2c_errors_is_a_protocol_error(connect):
    dev, fake = connect()
    fake.fail_next(CLEAR_I2C_ERRORS, "silent")
    with pytest.raises(ProtocolError):
        dev.init()


def test_a_refused_set_addr_width_fails_init_as_protocol_error(connect):
    dev, fake = connect()
    fake.fail_next(SET_ADDR_WIDTH, 0x04)
    with pytest.raises(ProtocolError):
        dev.init()
    assert dev.initialized is False


# ── the BridgeLink the ULD ports talk to ─────────────────────────────────────

def test_reads_past_253_bytes_in_chunks(connect):
    dev, fake = connect()
    fake.regs[0:300] = bytes(i & 0xFF for i in range(300))
    assert dev._bridge.read_reg(0, 300) == bytes(i & 0xFF for i in range(300))
    assert [p.hex() for p in fake.sent(READ_REG)] == ["0000fd00", "fd002f00"]


def test_writes_past_253_bytes_in_chunks(connect):
    dev, fake = connect()
    data = bytes((i * 7) & 0xFF for i in range(300))
    dev._bridge.write_reg(0x1000, data)
    assert [(a, len(d)) for a, d in fake.writes()] == [(0x1000, 253), (0x1000 + 253, 47)]
    assert bytes(fake.regs[0x1000:0x1000 + 300]) == data


def test_an_error_status_on_a_register_access_is_a_protocol_error(connect):
    dev, fake = connect()
    fake.fail_next(READ_REG, 0x09)
    with pytest.raises(ProtocolError):
        dev._bridge.read_reg(0x010F, 2)
    fake.fail_next(WRITE_REG, 0x09)
    with pytest.raises(ProtocolError):
        dev._bridge.write_reg(0x0087, b"\x40")
    fake.fail_next(SET_I2C_SPEED, 0x04)
    with pytest.raises(ProtocolError):
        dev._bridge.set_i2c_speed(1000)
    fake.fail_next(XSHUT, 0x09)
    with pytest.raises(ProtocolError):
        dev._bridge.xshut(XSHUT_RESET)


def test_a_short_register_read_is_a_protocol_error(connect):
    dev, fake = connect()
    fake.truncate_next_read = 1  # the bridge answers one byte of two
    with pytest.raises(ProtocolError, match="expected 2, got 1"):
        dev._bridge.read_reg(0x0089, 2)


def test_a_read_past_0xffff_is_refused_before_the_wire(connect):
    """contract 12 §2 / 10 §1: addr + len <= 0x10000 is the host's check (ERRATA A13)."""
    dev, fake = connect()
    reads = len(fake.sent(READ_REG))
    with pytest.raises(ValueError):
        dev._bridge.read_reg(0xFFFF, 2)
    assert len(fake.sent(READ_REG)) == reads


def test_no_answer_is_a_protocol_error_too(connect):
    dev, fake = connect(timeout=0.03)
    fake.fail_next(SET_I2C_SPEED, "silent")
    with pytest.raises(ProtocolError):
        dev._bridge.set_i2c_speed(400)


def test_argument_errors_pass_through_unchanged(connect):
    dev, fake = connect()
    with pytest.raises(ValueError):
        dev._bridge.set_i2c_speed(70000)
    with pytest.raises(ValueError):
        dev._bridge.set_addr_width(3)
    assert fake.sent(SET_I2C_SPEED) == fake.sent(SET_ADDR_WIDTH) == []


def test_bridge_info_decodes_rpt_vl53_info(connect):
    dev, fake = connect()
    fake.info = INFO_WRAPPED
    info = dev.bridge_info()
    assert (info.int_edges, info.slots_skipped, info.i2c_errors, info.last_i2c_error,
            info.xshut_level, info.int_level, info.i2c_khz, info.addr_width, info.n_clear,
            info.frames_dropped) == (4294967295, 7, 3, 2, 1, 0, 1000, 2, 1, 12)


def test_xshut_drops_the_driver_and_the_stream(ready):
    dev, fake = ready()
    dev.start_ranging()
    dev.xshut(XSHUT_OFF)
    assert fake.sent(XSHUT) == [b"\x00"]
    assert dev.ranging is False
    assert dev.initialized is False
    with pytest.raises(DepzError, match="init"):
        dev.driver


# ── configuration and the capability-gated passthroughs ──────────────────────

def test_configure_reinits_then_timing_offset_xtalk_signal(ready):
    dev, fake = ready()
    dev.configure(budget_ms=33, offset_mm=-10, xtalk_kcps=20, signal_kcps=1024)
    assert len(fake.sent(CLEAR_I2C_ERRORS)) == 1
    # vectors/vl53l4.json tuning: offset -10 → 65496, xtalk 20 → 10240, signal 1024 → 128
    assert fake.writes()[-7:] == [
        (0x001E, [0xFF, 0xD8]),
        (0x0020, [0, 0]),
        (0x0022, [0, 0]),
        (0x0018, [0, 0]),
        (0x001A, [0, 0]),
        (0x0016, [0x28, 0x00]),
        (0x0066, [0x00, 0x80]),
    ]
    assert dev.get_offset_mm() == -10
    assert dev.get_xtalk_kcps() == 20
    assert dev.get_signal_threshold_kcps() == 1024
    budget, inter = dev.get_range_timing()
    assert inter == 0 and abs(budget - 33) <= 1
    assert dev.get_mode() is None


def test_an_inter_measurement_period_selects_autonomous_mode(ready):
    dev, fake = ready()
    dev.configure(budget_ms=50, inter_ms=100)
    assert fake.dword(0x006C) == int(1.055 * 100 * (DIE_PLL & 0x3FF))
    fake.rx.clear()
    dev.start_ranging()
    assert fake.writes() == [(0x0087, [0x40])]


def test_a_period_not_above_the_budget_is_refused(ready):
    dev, _ = ready()
    with pytest.raises(Vl53Error):
        dev.configure(budget_ms=50, inter_ms=20)
    with pytest.raises(Vl53Error, match="10..200"):
        dev.configure(budget_ms=300)


def test_a_mode_on_a_product_without_modes_is_refused_before_the_reinit(ready):
    dev, fake = ready()
    with pytest.raises(DepzError, match="VL53L4CD/uld does not support 'mode'"):
        dev.configure(mode="long")
    assert fake.rx == []


def test_snap_budget_clamps_to_the_drivers_range_when_any_budget_goes(ready):
    dev, _ = ready()
    assert dev.budget_choices() == ()
    assert (dev.snap_budget(5), dev.snap_budget(77), dev.snap_budget(500)) == (10, 77, 200)


def test_offset_xtalk_thresholds_signal_sigma_round_trip(ready):
    dev, fake = ready()
    dev.set_offset_mm(10)
    assert fake.word(0x001E) == 40
    assert dev.get_offset_mm() == 10
    dev.set_xtalk_kcps(3)
    assert fake.word(0x0016) == 1536
    assert dev.get_xtalk_kcps() == 3
    dev.set_detection_thresholds(100, 300, 3)
    assert fake.regs[0x0046] == 3
    assert (fake.word(0x0072), fake.word(0x0074)) == (300, 100)
    assert dev.get_detection_thresholds() == (100, 300, 3)
    dev.set_signal_threshold_kcps(2048)
    assert fake.word(0x0066) == 256
    assert dev.get_signal_threshold_kcps() == 2048
    dev.set_sigma_threshold_mm(15)
    assert fake.word(0x0064) == 60
    assert dev.get_sigma_threshold_mm() == 15
    with pytest.raises(Vl53Error, match="16383"):
        dev.set_sigma_threshold_mm(16384)


def test_an_unsupported_group_is_refused_with_no_traffic(ready):
    dev, fake = ready()
    for call in (dev.get_roi, lambda: dev.set_roi(4, 4), dev.get_roi_center,
                 lambda: dev.set_roi_center(199), dev.perform_ref_spad_management):
        with pytest.raises(DepzError, match="VL53L4CD/uld does not support"):
            call()
    assert fake.rx == []


def test_the_refusal_names_the_caveat_and_comes_before_the_reinit(ready):
    dev, fake = ready(L3CX_BOARD)
    assert dev.driver_kind == "ulp"
    with pytest.raises(DepzError,
                       match=r"VL53L3CX/ulp does not support 'offset' \(single-target ranging only"):
        dev.configure(offset_mm=5)
    with pytest.raises(DepzError, match="'calib_xtalk'"):
        dev.calibrate_xtalk(600)
    assert fake.rx == []
    assert any(n.startswith("VL53L3CX/ulp: single-target") for n in dev.notes())


def test_temperature_update_reruns_vhv_and_restores_the_loop_bound(ready):
    dev, fake = ready()
    dev.start_temperature_update()
    assert fake.writes() == [
        (0x0008, [0x81]),
        (0x000B, [0x92]),
        (0x0087, [0x21]),
        (0x0086, [0x01]),
        (0x0087, [0x80]),
        (0x0008, [0x09]),
        (0x000B, [0x00]),
    ]


# ── calibrations (VL53L4CD_calibration.c) ────────────────────────────────────

def test_offset_calibration_target_minus_mean_programmed_x4(ready):
    dev, fake = ready()
    fake.set_result_block(L4_BLOCK)  # 624 mm
    assert dev.calibrate_offset(600, 5) == -24
    assert fake.word(0x001E) == -96 & 0xFFFF
    assert dev.get_offset_mm() == -24
    # offsets zeroed first, 10 heat frames + 5 samples each released
    w = fake.writes()
    assert w[:3] == [(0x001E, [0, 0]), (0x0020, [0, 0]), (0x0022, [0, 0])]
    assert len([a for a, _ in w if a == 0x0086]) == 15


def test_xtalk_calibration_from_the_valid_samples_after_the_first(ready):
    dev, fake = ready()
    fake.set_result_block(L4_BLOCK)  # 624 mm, 5160 kcps, 211 SPADs
    assert dev.calibrate_xtalk(650, 5) == 1
    # (1 - 624/650) × 5160/211 = 0.978 kcps → ×512
    assert fake.word(0x0016) == 500


def test_xtalk_calibration_fails_with_no_valid_sample(ready):
    dev, fake = ready()
    bad = bytearray(L4_BLOCK)
    bad[0] = 0x04  # raw 4 → status 2, signal below threshold
    fake.set_result_block(bytes(bad))
    with pytest.raises(Vl53Error, match="no valid samples"):
        dev.calibrate_xtalk(650, 5)


def test_xtalk_calibration_fails_past_127_kcps(ready):
    dev, fake = ready()
    hot = bytearray(L4_BLOCK)
    hot[3:7] = bytes((0x01, 0x00, 0xFF, 0xFF))  # 1 SPAD, 524280 kcps
    fake.set_result_block(bytes(hot))
    with pytest.raises(Vl53Error, match="> 127"):
        dev.calibrate_xtalk(5000, 5)


def test_calibration_arguments_out_of_range_are_refused_before_any_traffic(ready):
    dev, fake = ready()
    with pytest.raises(Vl53Error, match="5..255"):
        dev.calibrate_offset(600, 4)
    with pytest.raises(Vl53Error, match="10..1000"):
        dev.calibrate_offset(1001)
    with pytest.raises(Vl53Error, match="10..5000"):
        dev.calibrate_xtalk(5001)
    with pytest.raises(Vl53Error):
        dev.calibrate_xtalk(600, 256)
    assert fake.rx == []


# ── VL53L1X ULD: modes and tabulated budgets ─────────────────────────────────

def test_l1_modes_budget_choices_that_move_with_the_mode_and_snap(ready):
    dev, _ = ready(L1CX_BOARD)
    assert dev.modes == ("long", "short")
    assert dev.get_mode() == "long"
    assert dev.budget_choices() == (20, 33, 50, 100, 200, 500)
    assert (dev.snap_budget(40), dev.snap_budget(1000)) == (33, 500)
    dev.configure(mode="short", budget_ms=15)
    assert dev.get_mode() == "short"
    assert dev.budget_choices()[0] == 15
    assert dev.get_range_timing() == (15, 15)
    assert dev.identify_product()["driver_reach_mm"] == phase_window_mm(DIE_OSC, 0x38)


def test_l1_a_budget_the_mode_does_not_have_is_refused_not_rounded(ready):
    dev, _ = ready(L1CX_BOARD)
    with pytest.raises(Vl53Error, match="pick one of 20, 33, 50, 100, 200, 500"):
        dev.configure(budget_ms=40)
    with pytest.raises(Vl53Error, match="no such mode"):
        dev.configure(mode="medium")


def test_l1_roi_size_and_centre(ready):
    dev, fake = ready(L1CX_BOARD)
    dev.set_roi(8, 6)
    assert fake.regs[0x007F] == 0x91  # the optical centre the die reports
    assert fake.regs[0x0080] == 0x57
    assert dev.get_roi() == (8, 6)
    dev.set_roi(16, 16)  # over 10: the centre moves to 199
    assert dev.get_roi_center() == 199
    dev.set_roi_center(0x88)
    assert dev.get_roi_center() == 0x88


def test_l1_streams_decode_with_the_l1_variant(ready):
    dev, fake = ready(L1CX_BOARD)
    dev.start_ranging()
    _when_waiting(dev, lambda: fake.stream(L1_BLOCK, 1))
    m = dev.get_measurement(timeout=1.0)
    assert (m.distance_mm, m.status, m.sigma_mm, m.signal_kcps, m.ambient_kcps, m.spads) == (
        576, 0, 3, 16072, 2552, 67)
    assert m.extra == {"signal_per_spad_kcps": 23, "ambient_per_spad_kcps": 3, "stream_count": 1}


# ── ranging ──────────────────────────────────────────────────────────────────

def test_start_arms_the_stream_with_the_die_block_and_its_release(ready):
    dev, fake = ready()
    dev.start_ranging()
    assert fake.writes() == [(0x0087, [0x21])]  # continuous
    assert [p.hex() for p in fake.sent(START_STREAM)] == [START_STREAM_DIE]
    assert dev.ranging is True


def test_each_streamed_block_is_decoded_once_to_every_subscriber(ready):
    dev, fake = ready()
    seen: list[Vl53lxMeasurement] = []
    unsub = dev.on_measurement(seen.append)
    it = dev.measurements()
    dev.start_ranging()
    fake.stream(L4_BLOCK, 123_456)
    fake.stream(L4_BLOCK_F9, 123_789)
    settle(fake, dev)
    assert [m.timestamp_us for m in seen] == [123_456, 123_789]
    assert seen[0] == Vl53lxMeasurement(
        timestamp_us=123_456, distance_mm=624, status=0, status_text="valid",
        signal_kcps=5160, ambient_kcps=1096, sigma_mm=2, spads=211, targets=(),
        extra={"signal_per_spad_kcps": 24, "ambient_per_spad_kcps": 5, "stream_count": 1},
        bins=None,
    )
    assert (seen[0].valid, seen[0].plottable) == (True, True)
    assert seen[1].distance_mm == 623
    assert seen[1].extra["stream_count"] == 8
    assert [next(it).distance_mm, next(it).distance_mm] == [624, 623]
    unsub()
    fake.stream(L4_BLOCK, 1)
    settle(fake, dev)
    assert len(seen) == 2


def test_a_nonzero_status_is_not_valid_but_6_is_plottable(ready):
    dev, fake = ready()
    dev.start_ranging()
    blk = bytearray(L4_BLOCK)
    blk[0] = 0x13  # raw 19 → status 6
    _when_waiting(dev, lambda: fake.stream(bytes(blk), 1))
    m = dev.get_measurement(timeout=1.0)
    assert (m.status, m.valid, m.plottable) == (6, False, True)


def test_a_block_the_driver_cannot_decode_is_counted_and_the_reader_survives(ready):
    dev, fake = ready()
    dev.start_ranging()
    fake.stream(L4_BLOCK[:5], 1)
    settle(fake, dev)
    assert dev.stream_parse_errors == 1
    _when_waiting(dev, lambda: fake.stream(L4_BLOCK, 2))
    assert dev.get_measurement(timeout=1.0).distance_mm == 624


def test_a_malformed_stream_report_is_counted_and_dropped(ready):
    """The length field says 17, only 5 bytes follow (contract 07 §3)."""
    dev, fake = ready()
    dev.start_ranging()
    fake.stream(L4_BLOCK[:5], 1, 0x0089, 17)
    settle(fake, dev)
    assert dev.stream_parse_errors == 1
    _when_waiting(dev, lambda: fake.stream(L4_BLOCK, 2))
    assert dev.get_measurement(timeout=1.0).distance_mm == 624
    assert dev.bridge_info().addr_width == 0  # requests still answered


def test_a_stream_report_shorter_than_its_header_is_not_a_measurement(ready):
    dev, fake = ready()
    seen = []
    dev.on_measurement(seen.append)
    dev.start_ranging()
    fake.send(0x93, bytes(11))
    settle(fake, dev)
    assert seen == [] and dev.stream_parse_errors == 0


def test_a_stream_report_before_init_is_swallowed_not_decoded(connect):
    dev, fake = connect()
    seen = []
    dev.on_measurement(seen.append)
    fake.stream(L4_BLOCK, 1)
    settle(fake, dev)
    assert seen == [] and dev.stream_parse_errors == 0


def test_measurements_is_bounded_drop_oldest_and_deregisters_on_close(ready):
    dev, fake = ready()
    it = dev.measurements(maxsize=1)
    assert dev.stream_dropped_counts == [0]
    dev.start_ranging()
    fake.stream(L4_BLOCK, 1)
    fake.stream(L4_BLOCK, 2)
    fake.stream(L4_BLOCK_F9, 3)
    settle(fake, dev)
    assert dev.stream_dropped_counts == [2]
    assert next(it).timestamp_us == 3
    it.close()
    assert dev.stream_dropped_counts == []


def test_get_measurement_times_out_and_leaves_no_queue(ready):
    dev, _ = ready()
    with pytest.raises(DepzTimeoutError):
        dev.get_measurement(timeout=0.02)
    assert dev.stream_dropped_counts == []


def test_get_measurement_ends_when_the_device_closes(ready):
    dev, _ = ready()
    _later(dev.close)
    with pytest.raises(LinkClosedError, match="device closed"):
        dev.get_measurement(timeout=2.0)
    with pytest.raises(LinkClosedError, match="device is closed"):
        dev.get_measurement(timeout=2.0)


def test_configuration_is_refused_while_the_stream_owns_the_bus(ready):
    dev, fake = ready()
    dev.start_ranging()
    fake.rx.clear()
    for call in (
        dev.init,
        dev.configure,
        lambda: dev.set_offset_mm(1),
        lambda: dev.set_xtalk_kcps(1),
        lambda: dev.calibrate_offset(100),
        lambda: dev.calibrate_xtalk(100),
        lambda: dev.set_detection_thresholds(1, 2, 3),
        lambda: dev.set_signal_threshold_kcps(1),
        lambda: dev.set_sigma_threshold_mm(1),
        lambda: dev.set_roi(4, 4),
        lambda: dev.set_roi_center(1),
        dev.start_temperature_update,
        dev.perform_ref_spad_management,
        dev.start_ranging,
        dev.measure_once,
    ):
        with pytest.raises(InvalidStateError):
            call()
    assert fake.rx == []
    dev.bridge_info()  # touches no sensor register: allowed while streaming


def test_stop_ranging_stops_the_stream_then_the_sensor(ready):
    dev, fake = ready()
    dev.stop_ranging()  # not ranging: no-op
    assert fake.rx == []
    dev.start_ranging()
    fake.rx.clear()
    dev.stop_ranging()
    assert fake.cmds() == [STOP_STREAM, WRITE_REG]
    assert fake.writes() == [(0x0087, [0x80])]
    assert dev.ranging is False


def test_a_failed_stop_stream_still_stops_the_sensor_and_clears_the_flag(ready):
    dev, fake = ready()
    dev.start_ranging()
    fake.rx.clear()
    fake.fail_next(STOP_STREAM, 0x01)
    with pytest.raises(StatusError):
        dev.stop_ranging()
    assert dev.ranging is False
    assert fake.writes() == [(0x0087, [0x80])]
    dev.set_offset_mm(0)  # configuration allowed again


def test_a_refused_start_stream_stops_the_sensor_again(ready):
    dev, fake = ready()
    fake.fail_next(START_STREAM, 0x03)
    with pytest.raises(StatusError) as e:
        dev.start_ranging()
    assert e.value.status == 0x03  # the original error, not one from the stop
    assert dev.ranging is False
    # the driver started the die, so it is stopped again
    assert fake.writes()[-1] == (0x0087, [0x80])
    dev.set_offset_mm(0)  # configuration allowed again


# ── measure_once (poll) ──────────────────────────────────────────────────────

def test_measure_once_starts_waits_reads_releases_stops(ready):
    dev, fake = ready()
    fake.set_result_block(L4_BLOCK)
    m = dev.measure_once()
    assert (m.distance_mm, m.valid, m.bins) == (624, True, None)
    # the timestamp of the last register read: the block itself
    assert m.timestamp_us == fake.mcu_time_us - 100
    assert "89001100" in [p.hex() for p in fake.sent(READ_REG)]
    w = fake.writes()
    assert w[0] == (0x0087, [0x21])
    assert w[-2:] == [(0x0086, [0x01]), (0x0087, [0x80])]
    assert fake.sent(START_STREAM) == []


def test_a_data_ready_timeout_still_stops_the_sensor(ready):
    dev, fake = ready()
    fake.regs[0x0031] = 0x01  # INT never reaches the active level
    with pytest.raises(Vl53Error, match="timeout waiting for data ready"):
        dev.measure_once(timeout=0.03)
    assert fake.writes()[-1:] == [(0x0087, [0x80])]
    assert dev.ranging is False


# ── the histogram (Bare Driver) decode path, with a stub driver ──────────────

class StubHistogram(SensorDriver):
    """A histogram driver whose bin_data / to_measurement the test controls."""

    HISTOGRAM = True
    CLEAR_STEPS = ((0x0086, 0x01),)
    SUPPORTS = frozenset({"timing"})

    def __init__(self, platform, product):
        super().__init__(platform, product)
        self.bin_calls: list[bytes] = []
        self.bin = lambda raw: {"first": raw[0]}

    def model_id(self):
        return 0xEBAA

    def sensor_init(self):
        pass

    def start_ranging(self):
        pass

    def stop_ranging(self):
        pass

    def check_for_data_ready(self):
        return True

    def clear_interrupt(self):
        pass

    def stream_block(self):
        return 0x0088, 83

    def decode(self, raw):
        raise AssertionError("the histogram path must not call decode()")

    def bin_data(self, raw=None):
        if raw is None:  # polled: the driver reads its own block
            raw = self.p.rd_multi(*self.stream_block())
        self.bin_calls.append(raw)
        return self.bin(raw)

    def to_measurement(self, bins):
        d = bins["first"]
        target = Target(d, 0, "valid", 1, 0, 1, d, d)
        return Measurement(distance_mm=d, targets=(target, target._replace(distance_mm=d * 2,
                                                                             status=4)))


@pytest.fixture()
def stub(connect, monkeypatch):
    monkeypatch.setitem(registry.TABLE["VL53L4CX"].drivers, "histogram", StubHistogram)
    dev, fake = connect(L4CX_BOARD)
    dev.init()
    fake.rx.clear()
    return dev, fake, dev.driver


def test_histogram_each_frame_steps_bin_data_once_and_carries_its_bins(stub):
    dev, fake, drv = stub
    seen = []
    dev.on_measurement(seen.append)
    dev.start_ranging()
    assert [p.hex() for p in fake.sent(START_STREAM)] == ["880053000001860001"]
    fake.stream(bytes((7, 0, 0)), 1, 0x0088)
    settle(fake, dev)
    assert len(drv.bin_calls) == 1
    assert len(seen) == 1
    assert seen[0].bins == {"first": 7}
    assert [t.distance_mm for t in seen[0].targets] == [7, 14]
    assert plot_distances(seen[0]) == [7]


def test_histogram_a_failing_decode_is_counted_not_raised_into_the_reader(stub):
    dev, fake, drv = stub
    dev.start_ranging()

    def bad(raw):
        raise ValueError("bad frame")

    drv.bin = bad
    fake.stream(b"\x01", 1, 0x0088)
    settle(fake, dev)
    assert dev.stream_parse_errors == 1
    drv.bin = lambda raw: {"first": raw[0]}
    _when_waiting(dev, lambda: fake.stream(b"\x03", 3, 0x0088))
    assert dev.get_measurement(timeout=1.0).distance_mm == 3


def test_histogram_measure_once_reads_the_83_byte_block_through_bin_data(stub):
    dev, fake, drv = stub
    fake.regs[0x0088] = 42
    m = dev.measure_once()
    assert [p.hex() for p in fake.sent(READ_REG)] == ["88005300"]
    assert len(drv.bin_calls) == 1 and len(drv.bin_calls[0]) == 83
    assert (m.bins, m.distance_mm) == ({"first": 42}, 42)
