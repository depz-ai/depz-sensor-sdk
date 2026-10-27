"""The histogram driver (uld/bare/driver.py), its register image
(uld/bare/image.py, uld/bare/regs.py) and the driver contract (uld/base.py)
over the fake bridge, no hardware. The scenario captures cover init, mode,
timing and the streamed frames; this covers the paths they never take — the
boot timeout, polled reads, the contract methods the session does not call,
multi-block transfers, the codec's signed and masked fields, and the
read-only refusals.
"""

from __future__ import annotations

import json
from pathlib import Path

import pytest

from depz_sensor_sdk.errors import InvalidStateError
from depz_sensor_sdk.vl53lx import Vl53Error, Vl53lx
from depz_sensor_sdk.vl53lx.uld.base import BridgePlatform, Measurement, SensorDriver
from depz_sensor_sdk.vl53lx.uld.bare import regs
from depz_sensor_sdk.vl53lx.uld.bare.image import DeviceImage
from depz_sensor_sdk.vl53lx.uld.bare.regs import BLOCKS, Block, F, RegBlock
from fake_vl53lx import DIE_MODEL_ID, DIE_OSC, FakeVl53lxBridge

L4CX_BOARD = "DEPZ ToF Sensor VL53L4CX USB v2.1 TOVJALN523"
VECTORS = Path(__file__).resolve().parents[3] / "contracts" / "vectors" / "vl53lx.json"
READ_REG, WRITE_REG = 0x32, 0x33
FIRMWARE__SYSTEM_STATUS = 0x00E5
SYSTEM__INTERRUPT_CLEAR = 0x0086
HISTOGRAM_BIN_DATA_I2C_INDEX = 0x0088  # the 83-byte frame starts one before the result block


def _histogram_vector(name):
    for c in json.loads(VECTORS.read_text())["histogram_raw"]:
        if c["name"] == name:
            return bytes.fromhex(c["raw"]), c["expect"]
    raise KeyError(name)


@pytest.fixture()
def l4cx():
    fake = FakeVl53lxBridge(L4CX_BOARD)
    dev = Vl53lx(fake.link, timeout=0.5)
    dev.init()
    fake.rx.clear()
    yield dev, fake
    dev.close()
    fake.close()


# ── uld/bare/driver.py ───────────────────────────────────────────────────────

def test_identity_mode_and_driver_info_come_off_the_image(l4cx):
    dev, fake = l4cx
    drv = dev.driver
    assert drv.model_id() == DIE_MODEL_ID
    assert fake.sent(READ_REG)[-1] == (0x010F).to_bytes(2, "little") + (2).to_bytes(2, "little")
    assert dev.get_mode() == "medium"
    # a 33 ms budget: (33000 − 1700) / 6 = 5216 us of range timeout, and the
    # MM1/MM2 offset the fake's unprogrammed NVM gives
    assert drv.driver_info() == {"range timeout us": 5216, "MM1/MM2 offset mm": 0.0}
    assert dev.get_range_timing() == (32, 0)  # 5216 × 6 + 1700 = 32996 us, floored


def test_a_die_that_never_boots_times_out(l4cx):
    dev, fake = l4cx
    fake.regs[FIRMWARE__SYSTEM_STATUS] = 0x00
    with pytest.raises(Vl53Error, match="FIRMWARE__SYSTEM_STATUS == 0x03"):
        dev.driver.wait_boot(timeout_s=0.02)
    with pytest.raises(Vl53Error, match="FIRMWARE__SYSTEM_STATUS == 0x03"):
        dev.configure(budget_ms=50)


def test_a_budget_outside_the_dies_range_is_refused_before_any_traffic(l4cx):
    dev, fake = l4cx
    assert dev.driver.BUDGET_MS == (2, 200)  # an L4 die: FDA_MAX 200 ms
    with pytest.raises(Vl53Error, match="timing_budget_ms must be 2..200"):
        dev.driver.set_range_timing(201, 0)
    with pytest.raises(Vl53Error, match="timing_budget_ms must be 2..200"):
        dev.driver.set_range_timing(1, 0)
    assert fake.rx == []
    assert dev.get_range_timing() == (32, 0)  # unchanged


def test_decode_is_bin_data_then_to_measurement_on_the_handed_in_frame(l4cx):
    dev, fake = l4cx
    raw, expect = _histogram_vector("vl53l4cx_histogram_medium_50ms_f2")
    dev.driver.start_ranging()
    fake.rx.clear()
    m = dev.driver.decode(raw)
    assert fake.sent(READ_REG) == []  # nothing read: the frame was handed in
    assert isinstance(m, Measurement)
    assert m.extra["stream_count"] == expect["stream_count"] == 1
    assert m.targets and m.distance_mm == m.targets[0].distance_mm
    assert m.status == m.targets[0].status
    assert all(t.min_range_mm <= t.max_range_mm for t in m.targets)


def test_read_measurement_reads_the_83_byte_block_itself(l4cx):
    dev, fake = l4cx
    raw, expect = _histogram_vector("vl53l4cx_histogram_medium_50ms_f2")
    fake.regs[HISTOGRAM_BIN_DATA_I2C_INDEX:HISTOGRAM_BIN_DATA_I2C_INDEX + 83] = raw
    dev.driver.start_ranging()
    fake.rx.clear()
    m = dev.driver.read_measurement()
    assert fake.sent(READ_REG) == [(0x0088).to_bytes(2, "little") + (83).to_bytes(2, "little")]
    assert m.extra["stream_count"] == expect["stream_count"]
    assert m.spads == expect["dss_actual_effective_spads"] >> 8


def test_measure_once_polls_data_ready_and_clears_the_interrupt_with_one_byte(l4cx):
    dev, fake = l4cx
    raw, _ = _histogram_vector("vl53l4cx_histogram_medium_50ms_f2")
    fake.regs[HISTOGRAM_BIN_DATA_I2C_INDEX:HISTOGRAM_BIN_DATA_I2C_INDEX + 83] = raw
    m = dev.measure_once(timeout=0.5)
    assert m.bins is not None and m.bins.result__stream_count == 1
    # data-ready: polarity byte, then the GPIO status, both reads
    reads = fake.sent(READ_REG)
    assert (0x0030).to_bytes(2, "little") + (1).to_bytes(2, "little") in reads
    assert (0x0031).to_bytes(2, "little") + (1).to_bytes(2, "little") in reads
    # the one-byte clear, then the stop: system_control pushed as a block
    # (abort in system__mode_start), then the patch unloaded with the
    # firmware stopped and restarted
    writes = fake.writes()
    i = writes.index((SYSTEM__INTERRUPT_CLEAR, [0x01]))
    assert writes[i + 1] == (0x0083, [0x00, 0x00, 0x01, 0x01, 0x86])  # mode_start 0x86 = abort
    assert writes[i + 2:] == [(0x0085, [0x00]), (0x0083, [0x00]), (0x0470, [0x00]),
                              (0x0085, [0x01])]  # unload_patch


def test_data_ready_follows_the_polarity_bit(l4cx):
    dev, fake = l4cx
    drv = dev.driver
    fake.regs[0x0030] = 0x11  # active low: ready when GPIO reads 0
    fake.regs[0x0031] = 0x02
    assert drv.check_for_data_ready() is True
    fake.regs[0x0031] = 0x03
    assert drv.check_for_data_ready() is False
    fake.regs[0x0030] = 0x01  # active high: the other way round
    assert drv.check_for_data_ready() is True
    with pytest.raises(Vl53Error, match="timeout waiting for data ready"):
        fake.regs[0x0031] = 0x02
        drv.wait_data_ready(timeout_s=0.02)
    fake.rx.clear()
    drv.clear_interrupt()
    assert fake.writes() == [(SYSTEM__INTERRUPT_CLEAR, [0x01])]


# ── uld/bare/image.py ────────────────────────────────────────────────────────

def test_pull_range_is_one_read_spanning_the_blocks(l4cx):
    dev, fake = l4cx
    img = dev.driver.bare.img
    fake.set_word(0x0018, 0xFFFE)  # algo__crosstalk_compensation_x_plane_gradient_kcps
    fake.regs[0x001E] = 0xE0       # algo__part_to_part_range_offset_mm, high byte
    fake.regs[0x001F] = 0x04       # → 0xE004 & 0x1FFF = 0x0004
    fake.rx.clear()
    img.pull_range(("static_nvm_managed", "customer_nvm_managed"))
    # 0x0001..0x0023: static_nvm (11) + the reserved 0x000C + customer_nvm (23)
    assert fake.sent(READ_REG) == [(0x0001).to_bytes(2, "little") + (35).to_bytes(2, "little")]
    assert img.static_nvm_managed.osc_measured__fast_osc__frequency == DIE_OSC
    assert img.customer_nvm_managed.algo__crosstalk_compensation_x_plane_gradient_kcps == -2
    assert img.customer_nvm_managed.algo__part_to_part_range_offset_mm == 4


def test_push_range_goes_out_as_one_write_with_the_reserved_byte_zeroed(l4cx):
    dev, fake = l4cx
    img = dev.driver.bare.img
    img.static_nvm_managed.vhv_config__init = 0x5A
    img.customer_nvm_managed.mm_config__outer_offset_mm = -1
    fake.rx.clear()
    img.push_range(("static_nvm_managed", "customer_nvm_managed"))
    (addr, data), = fake.writes()
    assert (addr, len(data)) == (0x0001, 35)
    assert data[10] == 0x5A      # last byte of static_nvm
    assert data[11] == 0x00      # the hole at 0x000C
    assert data[33:35] == [0xFF, 0xFF]
    assert fake.word(0x0022) == 0xFFFF


def test_an_unknown_block_is_an_attribute_error(l4cx):
    dev, _ = l4cx
    img = dev.driver.bare.img
    with pytest.raises(AttributeError, match="no register block nonsense"):
        img.nonsense
    with pytest.raises(KeyError):
        img.pull("nonsense")


def test_dump_lists_every_field_of_the_named_blocks(l4cx):
    dev, _ = l4cx
    img = dev.driver.bare.img
    text = img.dump(["system_control"])
    lines = text.splitlines()
    assert lines[0] == "--- system_control @ 0x0083 (5 B)"
    assert len(lines) == 1 + len(BLOCKS["system_control"].fields)
    assert lines[-1].split() == ["system__mode_start", str(img.system_control.system__mode_start)]
    # default: all blocks, in BLOCKS order
    headers = [l for l in img.dump().splitlines() if l.startswith("--- ")]
    assert [h.split()[1] for h in headers] == list(BLOCKS)


def test_a_read_only_block_is_refused_before_the_wire(monkeypatch):
    monkeypatch.setitem(BLOCKS, "ro_test", Block(0x0F50, 1, False, (F("x", 0, 1, False, None),)))

    class NoWire:
        def wr_multi(self, addr, data):
            raise AssertionError("must not write")

    img = DeviceImage(NoWire())
    with pytest.raises(InvalidStateError):
        img.push("ro_test")
    with pytest.raises(InvalidStateError):
        img.push_range(("system_control", "ro_test"))


# ── uld/bare/regs.py ─────────────────────────────────────────────────────────

def test_regblock_codec_signed_and_masked_fields_round_trip():
    blk = RegBlock("customer_nvm_managed")
    assert repr(blk) == "<customer_nvm_managed 0x000D+23>"
    blk.algo__crosstalk_compensation_x_plane_gradient_kcps = -2
    blk.algo__part_to_part_range_offset_mm = -4      # 13-bit field: & 0x1FFF
    blk.ref_spad_man__num_requested_ref_spads = 0xFF  # 6-bit field: & 0x3F
    raw = blk.encode()
    assert raw[11:13] == b"\xff\xfe"
    assert raw[17:19] == b"\x1f\xfc"
    assert raw[7] == 0x3F
    again = RegBlock("customer_nvm_managed").decode(raw)
    assert again.algo__crosstalk_compensation_x_plane_gradient_kcps == -2
    assert again.algo__part_to_part_range_offset_mm == 0x1FFC  # the mask comes first
    assert dict(again.items())["ref_spad_man__num_requested_ref_spads"] == 0x3F


def test_regblock_keeps_reserved_bytes_and_refuses_unknown_fields_and_short_raw():
    blk = RegBlock("general_config")
    raw = bytearray(BLOCKS["general_config"].size)
    raw[10] = 0xA5  # the reserved hole at offset 10
    blk.decode(bytes(raw))
    assert blk.encode()[10] == 0xA5
    with pytest.raises(AttributeError, match="general_config has no field nope"):
        blk.nope
    with pytest.raises(AttributeError, match="general_config has no field nope"):
        blk.nope = 1
    with pytest.raises(ValueError, match="general_config: 3 bytes, need 22"):
        blk.decode(b"\x00\x00\x00")
    assert regs._unsign(0xFFFF, F("s", 0, 2, True, None)) == -1
    assert regs._unsign(0xFFFF, F("u", 0, 2, False, None)) == 0xFFFF


# ── uld/base.py ──────────────────────────────────────────────────────────────

def test_measurement_valid_is_status_0_and_repr_names_the_status():
    ok, bad = Measurement(distance_mm=612), Measurement(distance_mm=0, status=5, status_text="hw fail")
    assert ok.valid and not bad.valid
    assert repr(bad) == "<Measurement 0 mm status 5 (hw fail)>"
    assert ok.extra == {} and ok.targets == ()


def test_the_bare_contract_leaves_everything_product_specific_abstract():
    drv = SensorDriver(platform=None, product="VL53LXXX")
    assert drv.product == "VL53LXXX"
    for call in (drv.model_id, drv.sensor_init, drv.start_ranging, drv.stop_ranging,
                 drv.check_for_data_ready, drv.clear_interrupt, drv.stream_block,
                 drv.get_range_timing, drv.get_mode, drv.read_measurement,
                 lambda: drv.decode(b""), lambda: drv.set_range_timing(50, 0),
                 lambda: drv.set_mode("long")):
        with pytest.raises(NotImplementedError):
            call()
    assert drv.reach_mm() is None
    assert drv.driver_info() == {}
    assert drv.budget_choices() == ()
    assert drv.SUPPORTS == frozenset() and drv.MODES == () and not drv.HISTOGRAM


def test_the_default_read_measurement_reads_the_stream_block_and_decodes_it():
    class Blocky(SensorDriver):
        def stream_block(self):
            return 0x0089, 2

        def decode(self, raw):
            return Measurement(distance_mm=int.from_bytes(raw, "big"))

    class Dev:
        def read_reg(self, addr, size):
            assert (addr, size) == (0x0089, 2)
            return b"\x02\x64"

    m = Blocky(BridgePlatform(Dev()), "X").read_measurement()
    assert m.distance_mm == 612 and m.valid


def test_wait_data_ready_times_out_through_the_platform_sleep():
    slept = []

    class Never(SensorDriver):
        def check_for_data_ready(self):
            return False

    class P:
        @staticmethod
        def sleep_ms(ms):
            slept.append(ms)

    with pytest.raises(Vl53Error, match="timeout waiting for data ready"):
        Never(P(), "X").wait_data_ready(timeout_s=0.01)
    assert slept and set(slept) == {1}
