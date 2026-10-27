"""VL53L5CX / VL53L7CX / VL53L7CH device classes over the wire (contract 11)
against the in-process I2C board fake `FakeVl53l7` (tests/fake_vl53l7.py):
what the L5/L7 family does differently from VL53L8 — init over the L5/L7
ULD branch (silicon check, `l7cx` without a checksum word, module_type read
and the class / module-type warning), the 1536-byte read split, 1 Hz ranging,
the board commands PIN_CTRL / GET_INFO / SET_I2C_SPEED and their refusals,
the L5CX/L7CX ULD limits (no deep sleep, no threshold auto-stop) and the
xtalk "coverglass too good" flag. Everything the classes inherit unchanged
is covered by test_vl53l8_device.py."""

from __future__ import annotations

import struct
import warnings

import pytest

from depz_sensor_sdk import discovery
from depz_sensor_sdk.device import DeviceBase
from depz_sensor_sdk.errors import BusyError, DepzError, StatusError
from depz_sensor_sdk.protocol.vl53l7 import I2cError, PinAction, Vl53l7Info
from depz_sensor_sdk.protocol.vl53l8 import RegData, Vl53l8Cmd, Vl53l8Rpt
from depz_sensor_sdk.usb_ids import DEPZ_USB_VID, PID_VL53L5CX, PID_VL53L7CH, PID_VL53L7CX
from depz_sensor_sdk.vl53l7 import (
    MODULE_TYPE_MZ,
    MODULE_TYPE_MZEVO,
    RESOLUTION_8X8,
    Vl53l5cx,
    Vl53l7ch,
    Vl53l7cx,
    Vl53l7Frame,
)
from depz_sensor_sdk.vl53l8 import POWER_MODE_DEEP_SLEEP, POWER_MODE_SLEEP, POWER_MODE_WAKEUP, Vl53l8cxError, uld

from fake_vl53l7 import FakeVl53l7

L7_8X8_FRAME = 1444  # contract 11 §2: 8×8 = 1444 B, one 1536-byte chunk
L7_4X4_FRAME = 1060


@pytest.fixture()
def l7():
    fake = FakeVl53l7()
    dev = Vl53l7cx(fake.link, timeout=0.5)
    try:
        yield fake, dev
    finally:
        dev.close()
        fake.close()


def _device(fake, cls):
    return cls(fake.link, timeout=0.5)


# ── identity ─────────────────────────────────────────────────────────────────


@pytest.mark.parametrize("pid, cls", [(PID_VL53L5CX, Vl53l5cx), (PID_VL53L7CX, Vl53l7cx), (PID_VL53L7CH, Vl53l7ch)])
def test_open_on_link_resolves_the_class_by_pid(pid, cls):
    fake = FakeVl53l7()
    dev = discovery.open_on_link(fake.link, DEPZ_USB_VID, pid)
    try:
        assert type(dev) is cls
        assert dev.get_software_name() == FakeVl53l7.SOFTWARE_NAME
        assert dev.is_alive()
        assert dev.module_type is None  # readable only after init()
    finally:
        dev.close()
        fake.close()


def test_open_on_link_falls_back_to_the_part_in_the_device_name():
    fake = FakeVl53l7()
    dev = discovery.open_on_link(fake.link)  # a dev board: no production PID
    try:
        assert type(dev) is Vl53l7cx  # "… VL53L7CX USB v2.1 …"
    finally:
        dev.close()
        fake.close()


def test_is_alive_refuses_l8_silicon_on_an_l7_board(l7):
    fake, dev = l7
    fake.sensor.reg[(0, 0x01)] = uld.SI_REVISION_L8
    assert dev.is_alive() is False
    fake.sensor.reg[(0, 0x01)] = uld.SI_REVISION_CUT12
    assert dev.is_alive() is True


# ── init: the L5/L7 ULD branch ───────────────────────────────────────────────


@pytest.mark.parametrize("cls, variant, checksum_read", [
    (Vl53l7cx, "l7cx", False),  # ULD 2.0.1 publishes no checksum word
    (Vl53l5cx, "l7cx", False),
    (Vl53l7ch, "l7ch", True),  # VL53LMZ 2.0.16: checksum as on VL53L8CH
])
def test_init_downloads_the_l5_l7_blob(cls, variant, checksum_read):
    fake = FakeVl53l7()
    fake.sensor.module_type = MODULE_TYPE_MZ if cls is Vl53l5cx else MODULE_TYPE_MZEVO
    dev = _device(fake, cls)
    notes: list[str] = []
    try:
        with warnings.catch_warnings():
            warnings.simplefilter("error")
            dev.init(progress=notes.append)
        s = fake.sensor
        assert s.boots == 1
        assert s.variant == variant == dev.variant
        assert dev.module_type == s.module_type
        assert [len(s.fw_banks[p]) for p in (0x09, 0x0A, 0x0B)] == [0x8000, 0x8000, 0x5000]
        assert ((2, 0x2FFC, 4) in s.reads) is checksum_read
        assert ("Sensor FW checksum OK" in notes) is checksum_read
        assert notes[-1] == f"Sensor init done (module type {'MZ' if cls is Vl53l5cx else 'MZEVO'})"
        assert s.config_uploads == 1
        assert dev.get_resolution() == uld.RESOLUTION_4X4
    finally:
        dev.close()
        fake.close()


def test_init_refuses_non_l5_l7_silicon_before_downloading(l7):
    fake, dev = l7
    fake.sensor.reg[(0, 0x01)] = uld.SI_REVISION_L8  # an L8 wired to the I2C board
    with pytest.raises(Vl53l8cxError, match=r"not a VL53L5/L7 sensor: device_id=0xF0 revision_id=0x0C"):
        dev.init()
    assert fake.sensor.boots == 0 and fake.sensor.fw_banks == {}
    assert dev.module_type is None


@pytest.mark.parametrize("cls, reported, expects", [
    (Vl53l7cx, MODULE_TYPE_MZ, "MZEVO"),  # an L5 on a board labelled L7
    (Vl53l5cx, MODULE_TYPE_MZEVO, "MZ"),  # an L7 on a board labelled L5
    (Vl53l7ch, MODULE_TYPE_MZ, "MZEVO"),
])
def test_init_warns_when_the_module_type_contradicts_the_class(cls, reported, expects):
    fake = FakeVl53l7()
    fake.sensor.module_type = reported
    dev = _device(fake, cls)
    try:
        with pytest.warns(UserWarning, match=rf"{cls.__name__} expects module type {expects}, the sensor reports "
                                             rf"{'MZ' if reported == MODULE_TYPE_MZ else 'MZEVO'} — check the board"):
            dev.init()
        # a warning, not an error: the blob is shared, ranging works
        assert dev.module_type == reported
        dev.set_resolution(RESOLUTION_8X8)
        assert dev.get_resolution() == RESOLUTION_8X8
    finally:
        dev.close()
        fake.close()


def test_init_names_an_unknown_module_type_by_number(l7):
    fake, dev = l7
    fake.sensor.module_type = 7
    with pytest.warns(UserWarning, match=r"expects module type MZEVO, the sensor reports 7 "):
        dev.init()
    assert dev.module_type == 7


# ── transfer limits and the 1 Hz floor ───────────────────────────────────────


def test_register_reads_split_at_1536_bytes(l7):
    fake, dev = l7
    dev.init()
    n0 = len(fake.sensor.reads)
    data = dev._platform.rd_multi(0x0000, 4000)  # the NVM read is the ULD's largest (488 B); force a big one
    assert len(data) == 4000
    assert [size for _, _, size in fake.sensor.reads[n0:]] == [1536, 1536, 928]
    assert [addr for _, addr, _ in fake.sensor.reads[n0:]] == [0x0000, 0x0600, 0x0C00]


def test_the_bridge_refuses_a_read_above_1536_bytes(l7):
    fake, dev = l7
    with pytest.raises(StatusError) as e:  # the L8 host's 2048-byte split would fail every large read
        dev.request(Vl53l8Cmd.READ_REG, struct.pack("<HH", 0x0000, 2048),
                    matcher=DeviceBase.expect_report(Vl53l8Rpt.REG_DATA, RegData.unpack))
    assert e.value.status_name == "ERR_INVALID_PARAM"


def test_ranging_at_1_hz_is_allowed(l7):
    fake, dev = l7
    dev.init()
    dev.set_ranging_frequency_hz(1)  # L8 refuses this (contract 04 §4); L5/L7 range at 1 Hz
    assert dev.get_ranging_frequency_hz() == 1
    assert fake.sensor.dci[uld.DCI_FREQ_HZ] == bytes([0, 1, 0, 0])
    with pytest.raises(ValueError):
        dev.set_ranging_frequency_hz(0)


def test_ranging_frequency_ceiling_60_at_4x4_and_15_at_8x8(l7):
    """contract 11 §3 (ERRATA A13): refused host-side, nothing on the bus."""
    fake, dev = l7
    dev.init()
    dev.set_ranging_frequency_hz(60)
    writes = len(fake.sensor.writes)
    with pytest.raises(ValueError, match="<= 60 Hz"):
        dev.set_ranging_frequency_hz(61)
    assert len(fake.sensor.writes) == writes
    dev.set_resolution(RESOLUTION_8X8)
    dev.set_ranging_frequency_hz(15)
    writes = len(fake.sensor.writes)
    with pytest.raises(ValueError, match="<= 15 Hz"):
        dev.set_ranging_frequency_hz(16)
    assert len(fake.sensor.writes) == writes
    assert dev.get_ranging_frequency_hz() == 15


@pytest.mark.parametrize("cls, resolution, frame_size", [
    (Vl53l7cx, uld.RESOLUTION_4X4, L7_4X4_FRAME),
    (Vl53l7cx, RESOLUTION_8X8, L7_8X8_FRAME),
    (Vl53l7ch, RESOLUTION_8X8, L7_8X8_FRAME),
])
def test_frames_stream_in_one_1536_byte_chunk(cls, resolution, frame_size):
    fake = FakeVl53l7()
    dev = _device(fake, cls)
    got: list[Vl53l7Frame] = []
    try:
        dev.init()
        dev.set_resolution(resolution)
        dev.set_ranging_frequency_hz(1)
        dev.on_frame(got.append)
        it = dev.frames(maxsize=4)  # subscribes now: get_frame() only sees frames sent after it is called
        dev.start_ranging()
        assert fake.frame_size == frame_size
        expected = fake.send_frame()
        assert fake.chunks_sent == 1
        frame = next(it)
        it.close()
        assert frame.resolution == resolution
        assert frame.distance_mm.tolist() == expected["distance_mm"]
        assert frame.timestamp_us == expected["timestamp_us"]
        dev.stop_ranging()
        assert fake.int_edges == 1 and not fake.streaming
    finally:
        dev.close()
        fake.close()


# ── board commands (contract 11 §2) ──────────────────────────────────────────


def test_get_bridge_info_decodes_the_unechoed_report(l7):
    fake, dev = l7
    info = dev.get_bridge_info()
    assert isinstance(info, Vl53l7Info)
    assert info == Vl53l7Info(0, 0, 0, I2cError.OK, 1, 0, 1000, 0, False)
    dev.init()
    dev.set_resolution(RESOLUTION_8X8)
    it = dev.frames(maxsize=4)  # subscribes before the frame is sent
    dev.start_ranging()
    fake.send_frame()
    next(it)
    it.close()
    info = dev.get_bridge_info()
    assert (info.int_edges, info.frame_size, info.streaming, info.int_level) == (1, L7_8X8_FRAME, True, 1)
    assert (info.lpn_level, info.i2c_khz, info.i2c_errors, info.last_i2c_error) == (1, 1000, 0, 0)


def test_set_i2c_speed_snaps_to_the_nearest_step_and_returns_it(l7):
    fake, dev = l7
    assert dev.set_i2c_speed_khz(430) == 400
    assert dev.set_i2c_speed_khz(960) == 1000
    assert dev.set_i2c_speed_khz(1) == 100
    assert dev.set_i2c_speed_khz(65535) == 1000
    assert fake.speed_requests == [430, 960, 1, 65535]
    assert dev.get_bridge_info().i2c_khz == 1000
    # the speed is a board setting: init() afterwards runs at it
    dev.set_i2c_speed_khz(400)
    dev.init()
    assert dev.get_bridge_info().i2c_khz == 400


@pytest.mark.parametrize("khz", [0, 65536, -1])
def test_set_i2c_speed_refuses_out_of_range_before_sending(l7, khz):
    fake, dev = l7
    with pytest.raises(ValueError, match=r"khz must be 1\.\.65535"):
        dev.set_i2c_speed_khz(khz)
    assert fake.speed_requests == []


def test_set_i2c_speed_is_busy_while_streaming(l7):
    fake, dev = l7
    dev.init()
    dev.start_ranging()
    with pytest.raises(BusyError):
        dev.set_i2c_speed_khz(400)
    assert fake.i2c_khz == 1000
    dev.stop_ranging()
    assert dev.set_i2c_speed_khz(400) == 400


def test_lpn_off_drops_the_sensor_state_and_needs_init(l7):
    fake, dev = l7
    dev.init()
    dev.start_ranging()
    dev.pin_ctrl(PinAction.LPN_OFF)
    assert fake.pin_actions == [PinAction.LPN_OFF]
    assert not fake.streaming
    assert dev.ranging is False and dev.module_type is None
    with pytest.raises(DepzError, match="call init"):
        dev.get_resolution()  # nothing goes to the bus: the host knows the sensor is gone
    # LPn low: the sensor NACKs — the bridge reports a hardware fault and counts it
    assert dev.is_alive() is False
    info = dev.get_bridge_info()
    assert (info.lpn_level, info.i2c_errors, info.last_i2c_error) == (0, 1, I2cError.NACK)
    dev.pin_ctrl(PinAction.LPN_ON)
    assert dev.is_alive() is True
    assert dev.module_type is None  # LPN_ON restores the bus, not the firmware
    dev.init()
    assert dev.module_type == MODULE_TYPE_MZEVO


def test_i2c_rst_keeps_the_sensor_state(l7):
    fake, dev = l7
    dev.init()
    dev.set_resolution(RESOLUTION_8X8)
    dev.pin_ctrl(PinAction.I2C_RST)
    assert fake.pin_actions == [PinAction.I2C_RST]
    assert dev.module_type == MODULE_TYPE_MZEVO
    assert dev.get_resolution() == RESOLUTION_8X8  # the ULD is still valid


def test_soft_cycle_stops_the_stream_clears_the_i2c_counters_and_needs_init(l7):
    fake, dev = l7
    dev.init()
    dev.start_ranging()
    fake.i2c_errors, fake.last_i2c_error = 3, I2cError.TIMEOUT
    dev.pin_ctrl(3)  # a plain int is accepted
    assert fake.pin_actions == [PinAction.SOFT_CYCLE]
    assert not fake.streaming and dev.ranging is False and dev.module_type is None
    info = dev.get_bridge_info()
    assert (info.streaming, info.lpn_level, info.i2c_errors, info.last_i2c_error) == (False, 1, 0, I2cError.OK)
    dev.init()
    assert fake.sensor.boots == 2


def test_an_unknown_pin_action_is_refused_before_sending(l7):
    fake, dev = l7
    with pytest.raises(ValueError):
        dev.pin_ctrl(4)
    assert fake.pin_actions == []


# ── ULD plugins: the L5CX/L7CX limits and the xtalk flag ─────────────────────


def test_l7cx_has_sleep_and_wakeup_but_no_deep_sleep(l7):
    fake, dev = l7
    dev.init()
    dev.set_power_mode(POWER_MODE_SLEEP)
    assert dev.get_power_mode() == POWER_MODE_SLEEP
    assert (2, 0x000F, 1) not in fake.sensor.reads  # ULD 2.0.1 never reads the deep-sleep marker
    dev.set_power_mode(POWER_MODE_WAKEUP)
    assert dev.get_power_mode() == POWER_MODE_WAKEUP
    with pytest.raises(Vl53l8cxError, match="no deep sleep"):
        dev.set_power_mode(POWER_MODE_DEEP_SLEEP)
    with pytest.raises(Vl53l8cxError, match="auto-stop: not in the L5CX/L7CX ULD"):
        dev.set_detection_thresholds_auto_stop(True)


def test_xtalk_calibration_failed_flag(l7):
    fake, dev = l7
    assert dev.xtalk_calibration_failed is False  # before init(): nothing calibrated
    dev.init()
    dev.calibrate_xtalk(reflectance_percent=50, nb_samples=4, distance_mm=600)
    assert dev.xtalk_calibration_failed is False
    assert [len(t) for t in fake.sensor.calib_tables] == [len(uld.CALIBRATE_XTALK_L7CX)]
    fake.sensor.xtalk_cal_fails = True
    dev.calibrate_xtalk(reflectance_percent=20, nb_samples=1, distance_mm=3000)
    assert dev.xtalk_calibration_failed is True
    assert fake.sensor.calibrations == [(600 * 4, 50 * 16, 4), (3000 * 4, 20 * 16, 1)]
