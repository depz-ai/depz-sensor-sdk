"""VL53L8CX / VL53L8CH device classes over the wire (contract 04) against the
in-process bridge fake `FakeVl53l8` (tests/fake_vl53l8.py): init (firmware
download, checksum, NVM / xtalk / configuration uploads), every configuration
setter and getter as DCI round trips, the not-while-ranging guard, start/stop
of the INT-driven stream, 4×4 and 8×8 frames with all layers, the motion and
CNH blocks, the chunk reassembly rules of §4, poll-mode reads (§6), xtalk
calibration save / load (§7), power modes and the bridge error statuses. The
frame sizes asserted are the ones the lab boards streamed
(contracts/vectors/recordings/vl53l8*_8x8_15hz*)."""

from __future__ import annotations

import base64
import struct
import threading
import time

import numpy as np
import pytest

from depz_sensor_sdk import discovery
from depz_sensor_sdk.dataset import DatasetReader, SessionRecorder
from depz_sensor_sdk.errors import DepzError, DepzTimeoutError, LinkClosedError, StatusError
from depz_sensor_sdk.protocol.vl53l8 import (
    FrameChunk,
    FrameReassembler,
    Vl53l8Cmd,
    pack_write_reg,
)
from depz_sensor_sdk.usb_ids import DEPZ_USB_VID, PID_VL53L8CH, PID_VL53L8CX
from depz_sensor_sdk.vl53l8 import (
    POWER_MODE_DEEP_SLEEP,
    POWER_MODE_SLEEP,
    POWER_MODE_WAKEUP,
    RANGING_MODE_AUTONOMOUS,
    RANGING_MODE_CONTINUOUS,
    RESOLUTION_4X4,
    RESOLUTION_8X8,
    TARGET_ORDER_CLOSEST,
    TARGET_ORDER_STRONGEST,
    CnhConfig,
    Vl53l8ch,
    Vl53l8cx,
    Vl53l8cxError,
    Vl53l8Frame,
    cnh,
    uld,
)

from fake_vl53l8 import SCENE_BASE_MM, SCENE_STEP_MM, FakeVl53l8, cnh_block, expected_cnh

# Frame sizes the lab boards streamed (START_STREAM frame_size in the captures).
CX_8X8_FRAME = 1452
CH_8X8_FRAME = 1444
CH_8X8_CNH_16X20_FRAME = 3156
CX_4X4_FRAME = 540
CH_4X4_FRAME = 532
STREAM_CHUNK = 1528


@pytest.fixture()
def cx():
    fake = FakeVl53l8()
    dev = Vl53l8cx(fake.link, timeout=0.5)
    try:
        yield fake, dev
    finally:
        dev.close()
        fake.close()


@pytest.fixture()
def ch():
    fake = FakeVl53l8()
    dev = Vl53l8ch(fake.link, timeout=0.5)
    try:
        yield fake, dev
    finally:
        dev.close()
        fake.close()


def _cnh_16x20() -> CnhConfig:
    cfg = CnhConfig()
    cfg.init_config(start_bin=10, num_bins=20, sub_sample=2)
    cfg.create_agg_map(64, 0, 0, 2, 2, 4, 4)
    return cfg


def _assert_frame(frame: Vl53l8Frame, expected: dict) -> None:
    """Every contract 04 §5 field, in its unit, against the scene the fake rendered."""
    assert frame.timestamp_us == expected["timestamp_us"]
    assert frame.resolution == expected["resolution"]
    assert frame.distance_mm.dtype == np.int32
    assert frame.distance_mm.tolist() == expected["distance_mm"]
    assert frame.target_status.tolist() == expected["target_status"]
    assert frame.nb_target_detected.tolist() == expected["nb_target_detected"]
    assert frame.signal_per_spad.tolist() == expected["signal_per_spad"]
    assert frame.ambient_per_spad.tolist() == expected["ambient_per_spad"]
    assert frame.nb_spads_enabled.tolist() == expected["nb_spads_enabled"]
    assert frame.range_sigma_mm.tolist() == expected["range_sigma_mm"]
    assert frame.reflectance.tolist() == expected["reflectance"]
    assert frame.silicon_temp_degc == expected["silicon_temp_degc"]
    assert frame.motion == expected.get("motion")


# ── discovery / is_alive ──────────────────────────────────────────────────────


@pytest.mark.parametrize("pid, cls", [(PID_VL53L8CX, Vl53l8cx), (PID_VL53L8CH, Vl53l8ch)])
def test_open_on_link_resolves_cx_or_ch_by_pid(pid, cls):
    """CX and CH run the same APP_VL53L8 firmware; only the PID tells them apart."""
    fake = FakeVl53l8()
    dev = discovery.open_on_link(fake.link, DEPZ_USB_VID, pid)
    try:
        assert type(dev) is cls
        assert dev.get_software_name() == FakeVl53l8.SOFTWARE_NAME
        assert dev.is_alive()
        # the probe reads the ids on page 0 and leaves the sensor on page 2
        assert fake.sensor.page == 0x02
    finally:
        dev.close()
        fake.close()


def test_is_alive_false_on_foreign_silicon(cx):
    fake, dev = cx
    fake.sensor.reg[(0, 0x00)] = 0xEB  # not 0xF0 / 0x0C
    assert dev.is_alive() is False


def test_is_alive_false_when_the_bus_fails(cx):
    fake, dev = cx
    fake.truncate_reads = True  # READ_REG answers short: a DepzError inside the ULD
    assert dev.is_alive() is False


def test_uld_and_variant_need_init(cx):
    _fake, dev = cx
    with pytest.raises(DepzError):
        dev.uld
    with pytest.raises(DepzError):
        dev.variant


# ── init: firmware download + uploads ────────────────────────────────────────


@pytest.mark.parametrize("cls, variant", [(Vl53l8cx, "cx"), (Vl53l8ch, "ch")])
def test_init_downloads_the_variant_blob_and_uploads_defaults(cls, variant):
    fake = FakeVl53l8()
    dev = cls(fake.link, timeout=0.5)
    notes: list[str] = []
    progress: list[tuple[int, int]] = []
    try:
        dev.init(progress=notes.append, write_progress=lambda d, t: progress.append((d, t)))
        s = fake.sensor
        # the sensor booted exactly the blob this class ships: checksum matched
        assert s.boots == 1
        assert s.variant == variant == dev.variant == dev.uld.variant
        assert [len(s.fw_banks[p]) for p in (0x09, 0x0A, 0x0B)] == [0x8000, 0x8000, 0x5000]
        # write_progress tracks the three bank downloads in 2 KB steps
        assert progress[0] == (2048, 0x8000) and progress[-1] == (0x5000, 0x5000)
        assert all(d <= t for d, t in progress)
        # NVM read → offset data (488 B, 4x4 shape), default xtalk (776 B), default config (972 B)
        assert [len(b) for b in s.offset_uploads] == [uld.OFFSET_BUFFER_SIZE]
        assert [len(b) for b in s.xtalk_uploads] == [uld.XTALK_BUFFER_SIZE]
        assert s.config_uploads == 1
        assert dev.uld.offset_data == s.nvm[: uld.OFFSET_BUFFER_SIZE]
        # pipe control (one target per zone) and single-range DCI writes
        assert s.dci[uld.DCI_PIPE_CONTROL] == bytes([uld.NB_TARGET_PER_ZONE, 0x00, 0x01, 0x00])
        assert s.dci[uld.DCI_SINGLE_RANGE] == struct.pack("<I", 1)
        assert notes[0] == "SW reboot..." and notes[-1] == "Sensor init done"
        assert "Sensor FW checksum OK" in notes
        # ULD default after init: 4x4, 1 Hz, continuous, 5 ms, 5 %, closest, awake
        assert dev.get_resolution() == RESOLUTION_4X4
        assert dev.get_ranging_frequency_hz() == 1
        assert dev.get_ranging_mode() == RANGING_MODE_CONTINUOUS
        assert dev.get_integration_time_ms() == 5
        assert dev.get_sharpener_percent() == 5
        assert dev.get_target_order() == TARGET_ORDER_CLOSEST
        assert dev.get_power_mode() == POWER_MODE_WAKEUP
        assert dev.get_xtalk_margin() == 0
    finally:
        dev.close()
        fake.close()


def test_init_refuses_a_corrupted_firmware_download(cx):
    fake, dev = cx
    fake.sensor.corrupt_firmware = True
    with pytest.raises(Vl53l8cxError) as e:
        dev.init()
    assert e.value.code == uld.STATUS_FW_CHECKSUM_FAIL
    with pytest.raises(DepzError):
        dev.uld  # not initialised


def test_init_accepts_only_the_class_variant(cx):
    fake, dev = cx
    dev.init("cx")  # redundant but allowed
    assert fake.sensor.variant == "cx"


# ── configuration round trips (DCI over READ_REG / WRITE_REG) ────────────────


def test_resolution_round_trip_re_uploads_offset_and_xtalk(cx):
    fake, dev = cx
    dev.init()
    s = fake.sensor
    dev.set_resolution(RESOLUTION_8X8)
    assert dev.get_resolution() == RESOLUTION_8X8
    assert s.dci[uld.DCI_ZONE_CONFIG][:2] == bytes([8, 8])
    assert s.dci[uld.DCI_DSS_CONFIG][4] == 16 and s.dci[uld.DCI_DSS_CONFIG][9] == 1
    # each set_resolution re-sends offset + xtalk shaped for that grid
    assert len(s.offset_uploads) == 2 and len(s.xtalk_uploads) == 2
    assert s.xtalk_uploads[-1] == dev.uld.default_xtalk  # 8x8: the blob verbatim
    dev.set_resolution(RESOLUTION_4X4)
    assert dev.get_resolution() == RESOLUTION_4X4
    assert s.dci[uld.DCI_DSS_CONFIG][4] == 64 and s.dci[uld.DCI_DSS_CONFIG][9] == 4
    assert s.xtalk_uploads[-1] != dev.uld.default_xtalk  # 4x4: grid folded
    assert len(s.xtalk_uploads[-1]) == uld.XTALK_BUFFER_SIZE
    with pytest.raises(ValueError):
        dev.set_resolution(32)


def test_frequency_round_trip_and_floor(cx):
    fake, dev = cx
    dev.init()
    writes = len(fake.sensor.writes)
    with pytest.raises(ValueError):
        dev.set_ranging_frequency_hz(1)  # the sensor never ranges below 2 Hz (contract 04 §4)
    assert len(fake.sensor.writes) == writes  # refused host-side, nothing on the bus
    dev.set_ranging_frequency_hz(15)
    assert dev.get_ranging_frequency_hz() == 15
    assert fake.sensor.dci[uld.DCI_FREQ_HZ] == bytes([0, 15, 0, 0])


def test_frequency_ceiling_follows_the_resolution(cx):
    """contract 11 §3 (ERRATA A13): at most 60 Hz at 4×4 and 15 Hz at 8×8,
    refused host-side — nothing on the bus."""
    fake, dev = cx
    dev.init()  # 4×4 after init
    dev.set_ranging_frequency_hz(60)
    assert dev.get_ranging_frequency_hz() == 60
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
    dev.set_resolution(RESOLUTION_4X4)
    dev.set_ranging_frequency_hz(16)  # the ceiling moved back with the resolution
    assert dev.get_ranging_frequency_hz() == 16


def test_ranging_mode_round_trip(cx):
    fake, dev = cx
    dev.init()
    dev.set_ranging_mode(RANGING_MODE_AUTONOMOUS)
    assert dev.get_ranging_mode() == RANGING_MODE_AUTONOMOUS
    assert fake.sensor.dci[uld.DCI_RANGING_MODE][1:4:2] == bytes([0x3, 0x2])
    assert fake.sensor.dci[uld.DCI_SINGLE_RANGE] == struct.pack("<I", 1)
    dev.set_ranging_mode(RANGING_MODE_CONTINUOUS)
    assert dev.get_ranging_mode() == RANGING_MODE_CONTINUOUS
    assert fake.sensor.dci[uld.DCI_RANGING_MODE][1:4:2] == bytes([0x1, 0x3])
    assert fake.sensor.dci[uld.DCI_SINGLE_RANGE] == struct.pack("<I", 0)
    with pytest.raises(Vl53l8cxError):
        dev.set_ranging_mode(2)


def test_integration_time_round_trip(cx):
    fake, dev = cx
    dev.init()
    dev.set_integration_time_ms(20)
    assert dev.get_integration_time_ms() == 20
    assert fake.sensor.dci[uld.DCI_INT_TIME][:4] == struct.pack("<I", 20_000)
    for bad in (1, 1001):
        with pytest.raises(Vl53l8cxError):
            dev.set_integration_time_ms(bad)


def test_sharpener_round_trip_reads_back_what_was_set(cx):
    fake, dev = cx
    dev.init()
    for pct in (0, 25, 99):
        dev.set_sharpener_percent(pct)
        assert dev.get_sharpener_percent() == pct
    assert fake.sensor.dci[uld.DCI_SHARPENER][0xD] == 99 * 255 // 100
    with pytest.raises(Vl53l8cxError):
        dev.set_sharpener_percent(100)


def test_target_order_round_trip(cx):
    fake, dev = cx
    dev.init()
    dev.set_target_order(TARGET_ORDER_STRONGEST)
    assert dev.get_target_order() == TARGET_ORDER_STRONGEST
    assert fake.sensor.dci[uld.DCI_TARGET_ORDER][0] == TARGET_ORDER_STRONGEST
    dev.set_target_order(TARGET_ORDER_CLOSEST)
    assert dev.get_target_order() == TARGET_ORDER_CLOSEST
    with pytest.raises(Vl53l8cxError):
        dev.set_target_order(3)


def test_xtalk_margin_round_trip(cx):
    fake, dev = cx
    dev.init()
    dev.set_xtalk_margin(3)
    assert dev.get_xtalk_margin() == 3
    assert fake.sensor.dci[uld.DCI_XTALK_CFG][:4] == struct.pack("<I", 3 * 2048)
    for bad in (2.5, -1, 10001):
        with pytest.raises(Vl53l8cxError):
            dev.set_xtalk_margin(bad)


def test_detection_thresholds_round_trip(cx):
    fake, dev = cx
    dev.init()
    assert dev.get_detection_thresholds_enable() == 0
    thresholds = [
        {"low_thresh": 200, "high_thresh": 600, "measurement": uld.DIST_MM,
         "type": uld.THRESH_IN_WINDOW, "zone_num": 5, "operation": uld.THRESH_OP_NONE},
        {"low_thresh": 1, "high_thresh": 3, "measurement": uld.SIGNAL_PER_SPAD_KCPS,
         "type": uld.THRESH_GREATER_THAN_MAX, "zone_num": 6, "operation": uld.THRESH_OP_AND},
    ]
    dev.set_detection_thresholds(thresholds)
    got = dev.get_detection_thresholds()
    assert len(got) == uld.NB_THRESHOLDS
    assert got[:2] == thresholds
    assert got[2] == {"low_thresh": 0, "high_thresh": 0, "measurement": 0, "type": 0,
                      "zone_num": 0, "operation": 0}
    # on the wire the distance is ×4 and the signal ×2048 (plugin scaling)
    raw = fake.sensor.dci[uld.DCI_DET_THRESH_START]
    assert struct.unpack_from("<ii", raw, 0) == (800, 2400)
    assert struct.unpack_from("<ii", raw, 12) == (2048, 3 * 2048)
    assert fake.sensor.dci[uld.DCI_DET_THRESH_VALID_STATUS] == bytes([5] * 8)
    dev.set_detection_thresholds_enable(True)
    assert dev.get_detection_thresholds_enable() == 1
    assert fake.sensor.dci[uld.DCI_DET_THRESH_CONFIG][0x11] == 0x04
    dev.set_detection_thresholds_enable(False)
    assert dev.get_detection_thresholds_enable() == 0
    assert fake.sensor.dci[uld.DCI_DET_THRESH_CONFIG][0x11] == 0x0C
    dev.set_detection_thresholds_auto_stop(True)
    assert fake.sensor.dci[uld.DCI_PIPE_CONTROL][3] == 1
    dev.set_detection_thresholds_auto_stop(False)
    assert fake.sensor.dci[uld.DCI_PIPE_CONTROL][3] == 0
    with pytest.raises(ValueError):
        dev.set_detection_thresholds([{}] * (uld.NB_THRESHOLDS + 1))


def test_power_modes_sleep_wake_deep_sleep(cx):
    fake, dev = cx
    dev.init()
    s = fake.sensor
    assert dev.get_power_mode() == POWER_MODE_WAKEUP
    dev.set_power_mode(POWER_MODE_SLEEP)
    assert dev.get_power_mode() == POWER_MODE_SLEEP
    assert s.reg[(0, 0x09)] == 0x02
    dev.set_power_mode(POWER_MODE_SLEEP)  # already there: nothing to do
    dev.set_power_mode(POWER_MODE_WAKEUP)
    assert dev.get_power_mode() == POWER_MODE_WAKEUP
    assert s.boots == 1  # a plain wake keeps the firmware
    dev.set_power_mode(POWER_MODE_DEEP_SLEEP)
    assert dev.get_power_mode() == POWER_MODE_DEEP_SLEEP
    assert s.reg[(0, 0x0F)] == 0x43
    dev.set_power_mode(POWER_MODE_WAKEUP)  # the blob is lost in deep sleep: init() again
    assert dev.get_power_mode() == POWER_MODE_WAKEUP
    assert s.boots == 2 and s.config_uploads == 2
    with pytest.raises(Vl53l8cxError):
        dev.set_power_mode(7)
    # a register value the ULD does not know → error, page restored
    s.reg[(0, 0x09)] = 0x07
    with pytest.raises(Vl53l8cxError):
        dev.get_power_mode()
    assert s.page == 0x02


def test_configure_motion_indicator_writes_the_plugin_config(cx):
    fake, dev = cx
    dev.init()
    dev.set_resolution(RESOLUTION_8X8)
    cfg = dev.configure_motion_indicator(400, 1500)
    assert cfg.nb_of_aggregates == 16 and cfg.map_id[63] == 15
    assert fake.sensor.dci[uld.DCI_MOTION_DETECTOR_CFG] == cfg.pack()
    assert len(cfg.pack()) == 156
    with pytest.raises(Vl53l8cxError):
        dev.configure_motion_indicator(400, 2000)  # span > 1500 mm


@pytest.mark.parametrize("call", [
    ("set_resolution", (RESOLUTION_8X8,)),
    ("set_ranging_frequency_hz", (15,)),
    ("set_ranging_mode", (RANGING_MODE_AUTONOMOUS,)),
    ("set_integration_time_ms", (20,)),
    ("set_sharpener_percent", (10,)),
    ("set_target_order", (TARGET_ORDER_STRONGEST,)),
    ("set_power_mode", (POWER_MODE_SLEEP,)),
    ("set_xtalk_margin", (1,)),
    ("calibrate_xtalk", (50, 4, 600)),
    ("get_caldata_xtalk", ()),
    ("set_caldata_xtalk", (bytes(uld.XTALK_BUFFER_SIZE),)),
    ("set_detection_thresholds_enable", (True,)),
    ("set_detection_thresholds", ([],)),
    ("set_detection_thresholds_auto_stop", (True,)),
    ("configure_motion_indicator", ()),
    ("start_ranging", ()),
], ids=lambda c: c[0] if isinstance(c, tuple) else c)
def test_configuration_is_refused_while_ranging(ch, call):
    """The stream owns the register bank: a setter while ranging is refused
    before anything reaches the bus (contract 04 §5)."""
    fake, dev = ch
    dev.init()
    dev.set_ranging_frequency_hz(15)
    dev.start_ranging()
    ops = len(fake.sensor.writes), len(fake.sensor.reads)
    name, args = call
    with pytest.raises(DepzError, match="stop_ranging"):
        getattr(dev, name)(*args)
    assert (len(fake.sensor.writes), len(fake.sensor.reads)) == ops
    with pytest.raises(DepzError, match="stop_ranging"):
        dev.configure_cnh(_cnh_16x20())
    dev.stop_ranging()


# ── start / stop ranging ──────────────────────────────────────────────────────


@pytest.mark.parametrize("cls, resolution, frame_size", [
    (Vl53l8cx, RESOLUTION_8X8, CX_8X8_FRAME),
    (Vl53l8ch, RESOLUTION_8X8, CH_8X8_FRAME),
    (Vl53l8cx, RESOLUTION_4X4, CX_4X4_FRAME),
    (Vl53l8ch, RESOLUTION_4X4, CH_4X4_FRAME),
])
def test_start_ranging_arms_the_stream_with_the_frame_size(cls, resolution, frame_size):
    fake = FakeVl53l8()
    dev = cls(fake.link, timeout=0.5)
    try:
        dev.init()
        dev.set_resolution(resolution)
        dev.set_ranging_frequency_hz(15)
        assert dev.ranging is False
        dev.start_ranging()
        assert dev.ranging is True
        assert fake.streaming and fake.sensor.ranging
        assert fake.frame_size == frame_size == dev.uld.data_read_size
        assert dev.uld.frame_size_mismatch is None  # the FW agreed with the formula
        # the xshut bypass is on and the sensor sits on page 2 for the stream
        assert fake.sensor.reg[(0, 0x09)] == 0x05 and fake.sensor.page == 0x02
        dev.stop_ranging()
        assert dev.ranging is False
        assert not fake.streaming and not fake.sensor.ranging
        # STOP_STREAM went first: the host never touched the bus while streaming
        assert fake.bus_ops_while_streaming == 0
        assert fake.sensor.reg[(0, 0x09)] == 0x04
        dev.stop_ranging()  # idempotent, nothing on the bus
        assert fake.sensor.writes[-1][1] == 0x7FFF
    finally:
        dev.close()
        fake.close()


def test_start_ranging_refuses_a_laser_safety_fault(cx):
    fake, dev = cx
    dev.init()
    dev.set_ranging_frequency_hz(15)
    fake.sensor.laser_safety_fault = True
    with pytest.raises(Vl53l8cxError) as e:
        dev.start_ranging()
    assert e.value.code == uld.STATUS_LASER_SAFETY
    assert dev.ranging is False and not fake.streaming


def test_start_stream_rejected_by_the_bridge_stops_the_sensor_again(cx):
    """A bridge that refuses START_STREAM (ERR_INVALID_PARAM) surfaces as a
    StatusError, the host does not believe it is ranging and the sensor the
    ULD already started is stopped again."""
    fake, dev = cx
    dev.init()
    dev.set_ranging_frequency_hz(15)
    dev._WRITE_CHUNK = 2048
    fake.frame_size_limit = 0
    orig = fake._handle_bridge

    def refuse_start(fw, pkt):
        if pkt.cmd == Vl53l8Cmd.START_STREAM:
            fw._status(pkt.cmd, FakeVl53l8.ERR_INVALID_PARAM)
        else:
            orig(fw, pkt)

    fake._handle_bridge = refuse_start
    with pytest.raises(StatusError) as e:
        dev.start_ranging()
    assert e.value.status == FakeVl53l8.ERR_INVALID_PARAM
    assert dev.ranging is False
    assert fake.sensor.ranging is False
    fake._handle_bridge = orig
    dev.set_ranging_frequency_hz(30)  # not wedged: reconfiguration works


def test_stop_ranging_clears_state_even_if_stop_stream_fails(cx):
    fake, dev = cx
    dev.init()
    dev.set_ranging_frequency_hz(15)
    dev.start_ranging()
    orig = fake._handle_bridge

    def refuse_stop(fw, pkt):
        if pkt.cmd == Vl53l8Cmd.STOP_STREAM:
            fw._status(pkt.cmd, 0x01)  # ERROR
        else:
            orig(fw, pkt)

    fake._handle_bridge = refuse_stop
    with pytest.raises(StatusError):
        dev.stop_ranging()
    assert dev.ranging is False
    assert fake.sensor.ranging is False  # the ULD stop still ran
    dev.set_ranging_frequency_hz(30)  # not wedged: reconfiguration works


# ── frames ────────────────────────────────────────────────────────────────────


@pytest.mark.parametrize("resolution, side", [(RESOLUTION_4X4, 4), (RESOLUTION_8X8, 8)])
def test_frames_carry_every_layer_in_contract_units(cx, resolution, side):
    fake, dev = cx
    dev.init()
    dev.set_resolution(resolution)
    dev.set_ranging_frequency_hz(15)
    it = dev.frames(maxsize=4)  # subscribes now: get_frame() only sees frames sent after it is called
    dev.start_ranging()
    for _ in range(3):
        expected = fake.send_frame()
        frame = next(it)
        _assert_frame(frame, expected)
        assert frame.grid().shape == (side, side)
        assert frame.grid("target_status").shape == (side, side)
    # the scene: a plane 25 mm further per zone step, the far corner empty
    assert frame.grid()[0, 0] == SCENE_BASE_MM
    assert frame.grid()[0, 1] == frame.grid()[1, 0] == SCENE_BASE_MM + SCENE_STEP_MM
    assert frame.grid()[side - 1, side - 1] == 0
    assert frame.grid("target_status")[side - 1, side - 1] == 255
    assert frame.grid("nb_target_detected")[side - 1, side - 1] == 0
    assert frame.cnh_raw is None and frame.motion is None
    assert dev.frame_parse_errors == 0 and dev.reassembler_discards == 0
    assert fake.chunks_sent == 3  # ≤ 1528 B: one chunk per frame
    dev.stop_ranging()
    it.close()


def test_frame_timestamps_follow_the_ranging_period(cx):
    fake, dev = cx
    dev.init()
    dev.set_ranging_frequency_hz(15)
    it = dev.frames(maxsize=4)  # subscribes before the frames are sent
    dev.start_ranging()
    stamps = []
    for _ in range(2):
        fake.send_frame()
        stamps.append(next(it).timestamp_us)
    assert stamps[1] - stamps[0] == 1_000_000 // 15
    dev.stop_ranging()
    it.close()


def test_frames_iterator_and_on_frame_callback(cx):
    fake, dev = cx
    dev.init()
    dev.set_ranging_frequency_hz(15)
    seen: list[Vl53l8Frame] = []
    unsubscribe = dev.on_frame(seen.append)
    it = dev.frames(maxsize=4)  # subscribes now, before the stream starts
    dev.start_ranging()
    expected = [fake.send_frame() for _ in range(3)]
    got = [next(it) for _ in range(3)]
    for frame, exp in zip(got, expected):
        _assert_frame(frame, exp)
    assert [f.timestamp_us for f in seen] == [e["timestamp_us"] for e in expected]
    unsubscribe()
    fake.send_frame()
    next(it)
    assert len(seen) == 3  # unsubscribed callbacks see nothing more
    assert it.dropped_count == 0
    dev.stop_ranging()
    it.close()


def test_frames_iterator_ends_when_the_device_closes(cx):
    fake, dev = cx
    it = dev.frames()
    threading.Timer(0.05, dev.close).start()
    with pytest.raises(StopIteration):
        next(it)


def test_a_scene_change_shows_in_the_next_frame(cx):
    fake, dev = cx
    dev.init()
    dev.set_ranging_frequency_hz(15)
    it = dev.frames(maxsize=4)  # subscribes before the frames are sent
    dev.start_ranging()
    fake.send_frame()
    first = next(it)
    fake.sensor.distance_offset_mm = 100
    fake.sensor.silicon_temp_degc = 41
    expected = fake.send_frame()
    second = next(it)
    _assert_frame(second, expected)
    assert (second.distance_mm[:15] - first.distance_mm[:15] == 100).all()
    assert second.silicon_temp_degc == 41
    dev.stop_ranging()
    it.close()


def test_motion_indicator_output_rides_in_the_frame(cx):
    fake, dev = cx
    dev.init()
    dev.set_resolution(RESOLUTION_8X8)
    dev.set_ranging_frequency_hz(15)
    dev.configure_motion_indicator(400, 1500)
    it = dev.frames(maxsize=4)  # subscribes before the frame is sent
    dev.start_ranging()
    expected = fake.send_frame()
    frame = next(it)
    _assert_frame(frame, expected)
    assert frame.motion["global_indicator_1"] == 0x1234
    assert frame.motion["nb_of_aggregates"] == 16
    assert frame.motion["motion"][:3] == [0, 1000, 2000]
    dev.stop_ranging()
    it.close()


def test_ch_cnh_frame_streams_in_chunks_and_decodes(ch):
    """A CH frame with a 16×20 CNH block is 3156 B (the lab capture): three
    ≤1528 B chunks reassembled into one frame whose CNH block decodes."""
    fake, dev = ch
    dev.init()
    dev.set_resolution(RESOLUTION_8X8)
    dev.set_ranging_frequency_hz(15)
    assert dev.cnh_config is None
    cfg = _cnh_16x20()
    dev.configure_cnh(cfg)
    assert dev.cnh_config is cfg
    assert fake.sensor.dci[cnh.MI_CFG_DEV_IDX] == cfg.pack()
    it = dev.frames(maxsize=4)  # subscribes before the frame is sent
    dev.start_ranging()
    assert fake.frame_size == CH_8X8_CNH_16X20_FRAME
    expected = fake.send_frame()
    assert fake.chunks_sent == 3
    frame = next(it)
    _assert_frame(frame, expected)
    assert len(frame.cnh_raw) == cfg.required_memory() == 1708
    decoded = cnh.decode(cfg, frame.cnh_raw)
    want = expected_cnh(16, 20)
    assert decoded["ref_residual"] == want["ref_residual"] == 3.0
    assert len(decoded["aggregates"]) == 16
    for got, exp in zip(decoded["aggregates"], want["aggregates"]):
        assert got["hist_raw"] == exp["hist_raw"]
        assert got["hist_scaler"] == exp["hist_scaler"]
        assert got["hist"] == exp["hist"]
        assert got["ambient"] == exp["ambient"]
    assert dev.reassembler_discards == 0
    dev.stop_ranging()
    it.close()


def test_ch_configure_cnh_disarms_the_motion_indicator(ch):
    """CNH and the motion indicator share DCI 0xBFAC (ERRATA B4): the CNH
    setup overwrites the motion config, so frames carry no `.motion`."""
    fake, dev = ch
    dev.init()
    dev.set_resolution(RESOLUTION_8X8)
    dev.set_ranging_frequency_hz(15)
    dev.configure_motion_indicator(400, 1500)
    cfg = _cnh_16x20()
    dev.configure_cnh(cfg)
    assert dev.cnh_config is cfg
    assert fake.sensor.dci[cnh.MI_CFG_DEV_IDX] == cfg.pack()
    it = dev.frames(maxsize=4)  # subscribes before the frame is sent
    dev.start_ranging()
    assert fake.frame_size == CH_8X8_CNH_16X20_FRAME
    expected = fake.send_frame()
    frame = next(it)
    _assert_frame(frame, expected)
    assert "motion" not in expected and frame.motion is None
    assert len(cnh.decode(cfg, frame.cnh_raw)["aggregates"]) == 16
    dev.stop_ranging()
    it.close()


def test_ch_configure_motion_indicator_disarms_cnh(ch):
    """The motion setup overwrites the CNH config in DCI 0xBFAC (ERRATA B4):
    `cnh_config` drops to None, the output list loses the CNH block and
    frames carry no `cnh_raw`."""
    fake, dev = ch
    dev.init()
    dev.set_resolution(RESOLUTION_8X8)
    dev.set_ranging_frequency_hz(15)
    dev.configure_cnh(_cnh_16x20())
    dev.configure_motion_indicator(400, 1500)
    assert dev.cnh_config is None
    it = dev.frames(maxsize=4)  # subscribes before the frame is sent
    dev.start_ranging()
    assert fake.frame_size == CH_8X8_FRAME
    expected = fake.send_frame()
    frame = next(it)
    _assert_frame(frame, expected)
    assert frame.cnh_raw is None
    assert frame.motion["nb_of_aggregates"] == 16
    dev.stop_ranging()
    it.close()


@pytest.mark.parametrize("arm", ["cnh", "motion"])
def test_ch_init_disarms_cnh_and_the_motion_indicator(ch, arm):
    """init() reloads the sensor firmware: neither a CNH block nor a motion
    output survives it on the host side (ERRATA B4)."""
    fake, dev = ch
    dev.init()
    dev.set_resolution(RESOLUTION_8X8)
    if arm == "cnh":
        dev.configure_cnh(_cnh_16x20())
    else:
        dev.configure_motion_indicator(400, 1500)
    dev.init()
    assert dev.cnh_config is None
    dev.set_resolution(RESOLUTION_8X8)
    dev.set_ranging_frequency_hz(15)
    it = dev.frames(maxsize=4)  # subscribes before the frame is sent
    dev.start_ranging()
    assert fake.frame_size == CH_8X8_FRAME
    fake.send_frame()
    frame = next(it)
    assert frame.cnh_raw is None and frame.motion is None
    dev.stop_ranging()
    it.close()


def test_cnh_block_matches_the_lab_capture_size():
    assert len(cnh_block(16, 20)) == 1708
    assert expected_cnh(2, 3)["aggregates"][1]["hist"] == [100.0, 100.5, 101.0]  # raw 200.. / 2**1


def test_corrupted_footer_counts_as_a_parse_error(cx):
    fake, dev = cx
    dev.init()
    dev.set_ranging_frequency_hz(15)
    it = dev.frames(maxsize=4)  # subscribes before the good frame is sent
    dev.start_ranging()
    fake.sensor.corrupt_footer = True
    fake.send_frame()
    with pytest.raises(DepzTimeoutError):
        dev.get_frame(timeout=0.1)
    assert dev.frame_parse_errors == 1
    assert dev.reassembler_discards == 0
    expected = fake.send_frame()  # the next frame is fine
    _assert_frame(next(it), expected)
    dev.stop_ranging()
    it.close()


def test_frames_before_init_are_dropped_silently(cx):
    fake, dev = cx
    fake.send_chunk(1, 8, 0, bytes(8))  # a complete frame, no ULD to parse it
    fake.send_report(0x93, bytes(5))  # too short for a chunk header: not a frame
    fake.send_report(0x91, bytes(9))  # an unsolicited REG_DATA: nobody asked
    with pytest.raises(DepzTimeoutError):
        dev.get_frame(timeout=0.1)
    assert dev.frame_parse_errors == 0 and dev.reassembler_discards == 0
    assert dev._reassembler.completed == 1


# ── chunk reassembly rules (contract 04 §4) through the device ───────────────


def _parts(raw: bytes, n: int) -> list[bytes]:
    return [raw[i : i + n] for i in range(0, len(raw), n)]


def test_reassembly_gap_drops_the_frame_and_the_orphan(ch):
    fake, dev = ch
    dev.init()
    dev.set_resolution(RESOLUTION_8X8)
    dev.set_ranging_frequency_hz(15)
    dev.configure_cnh(_cnh_16x20())
    it = dev.frames(maxsize=4)  # subscribes before the chunks are sent
    dev.start_ranging()
    raw = fake.sensor.build_frame()
    a, b, c = _parts(raw, STREAM_CHUNK)
    # middle chunk lost: the frame in progress is discarded, the tail is an orphan
    fake.send_chunk(10, len(raw), 0, a)
    fake.send_chunk(10, len(raw), len(a) + len(b), c)
    # overlap: a chunk that re-sends bytes already accumulated
    fake.send_chunk(11, len(raw), 0, a)
    fake.send_chunk(11, len(raw), len(a) - 4, b)
    # full_size mismatch mid-frame
    fake.send_chunk(12, len(raw), 0, a)
    fake.send_chunk(12, len(raw) + 4, len(a), b)
    # restart on offset 0 while a frame is in progress
    fake.send_chunk(13, len(raw), 0, a)
    fake.send_chunk(14, len(raw), 0, a)
    # over-run: more bytes than full_size announced
    fake.send_chunk(15, len(a) + 4, len(a), b)
    # then a good frame completes
    expected = fake.send_frame()
    frame = next(it)
    _assert_frame(frame, expected)
    assert dev._reassembler.completed == 1
    assert dev.reassembler_discards == 5
    assert dev.frame_parse_errors == 0
    dev.stop_ranging()
    it.close()


def test_reassembler_rules_directly():
    r = FrameReassembler()
    chunk = lambda off, data, full=8, ts=1: FrameChunk(ts, full, off, data)  # noqa: E731
    assert r.feed(chunk(0, b"abcd")) is None
    assert r.feed(chunk(4, b"efgh")) == (1, b"abcdefgh")
    assert (r.completed, r.discarded) == (1, 0)
    # gap
    assert r.feed(chunk(0, b"abcd")) is None
    assert r.feed(chunk(6, b"gh")) is None
    assert r.discarded == 1
    # an orphan with nothing in progress is dropped without counting a frame
    assert r.feed(chunk(4, b"efgh")) is None
    assert r.discarded == 1
    # restart on offset 0 over a partial
    assert r.feed(chunk(0, b"abcd")) is None
    assert r.feed(chunk(0, b"abcd")) is None
    assert r.discarded == 2
    assert r.feed(chunk(4, b"efgh")) == (1, b"abcdefgh")  # the restarted frame completes
    # over-run (defensive): the single chunk exceeds full_size
    assert r.feed(chunk(0, b"abcdefghij")) is None
    assert r.discarded == 3
    assert r.feed(chunk(0, b"abcdefgh")) == (1, b"abcdefgh")
    assert (r.completed, r.discarded) == (3, 3)


def test_pack_write_reg_needs_a_data_byte():
    with pytest.raises(ValueError):
        pack_write_reg(0x7FFF, b"")
    assert pack_write_reg(0x7FFF, b"\x02") == b"\xff\x7f\x02"


# ── poll-mode read (contract 04 §6) ──────────────────────────────────────────


def test_poll_mode_read_through_the_bridge(ch):
    fake, dev = ch
    dev.init()
    dev.set_resolution(RESOLUTION_8X8)
    dev.set_ranging_frequency_hz(15)
    dev.configure_cnh(_cnh_16x20())
    dev.start_ranging()
    assert dev.uld.check_data_ready() is False  # nothing latched yet
    expected = fake.sensor.expected_frame()
    fake.sensor.latch_frame()
    assert dev.uld.check_data_ready() is True
    reads = len(fake.sensor.reads)
    parsed = dev.uld.get_ranging_data()  # 3156 B: two READ_REG chunks of ≤ 2048
    assert [n for _p, _a, n in fake.sensor.reads[reads:]] == [2048, CH_8X8_CNH_16X20_FRAME - 2048]
    assert parsed["distance_mm"] == expected["distance_mm"]
    assert parsed["target_status"] == expected["target_status"]
    assert parsed["cnh_raw"] == cnh_block(16, 20)
    assert dev.uld.check_data_ready() is False  # same stream count: no new frame
    dev.stop_ranging()


def test_short_read_reg_answer_is_an_error(cx):
    fake, dev = cx
    dev.init()
    fake.truncate_reads = True
    with pytest.raises(DepzError, match="READ_REG"):
        dev.get_resolution()


# ── xtalk calibration, save / load (contract 04 §7) ──────────────────────────


@pytest.mark.parametrize("cls, table_len", [(Vl53l8cx, len(uld.CALIBRATE_XTALK)),
                                            (Vl53l8ch, len(uld.CALIBRATE_XTALK_LMZ))])
def test_calibrate_xtalk_runs_the_fw_table_and_captures_the_grid(cls, table_len):
    fake = FakeVl53l8()
    dev = cls(fake.link, timeout=0.5)
    try:
        dev.init()
        dev.set_ranging_frequency_hz(15)
        dev.set_sharpener_percent(25)
        s = fake.sensor
        dev.calibrate_xtalk(reflectance_percent=50, nb_samples=4, distance_mm=600)
        # the firmware's own table, the target as the FW wants it (×4 mm, ×16 %)
        assert [len(t) for t in s.calib_tables] == [table_len]
        assert s.calibrations == [(600 * 4, 50 * 16, 4)]
        # the dedicated 17-block calibration list was programmed, then the default config restored
        assert s.config_uploads == 2
        assert dev.uld.xtalk_calibration_failed is False
        blob = dev.uld.xtalk_data
        assert len(blob) == uld.XTALK_BUFFER_SIZE
        assert blob[:-8] == s.calibrated_buf[:-8]
        assert blob[-8:] == bytes([0x00, 0x00, 0x00, 0x0F, 0x00, 0x01, 0x03, 0x04])
        assert blob != dev.uld.default_xtalk
        # the captured grid is the one the FW computed for this target
        grid = struct.unpack_from("<64I", uld.swap_buffer(blob), 0x34)
        assert grid == tuple(s.xtalk_grid_value(600, 50, 4, z) for z in range(64))
        # the settings around the run came back
        assert dev.get_resolution() == RESOLUTION_4X4
        assert dev.get_ranging_frequency_hz() == 15
        assert dev.get_sharpener_percent() == 25
        # save: get_caldata_xtalk re-reads the FW buffer (8x8) and restores 4x4
        saved = dev.get_caldata_xtalk()
        assert saved == blob
        assert dev.get_resolution() == RESOLUTION_4X4
        # load into a fresh sensor: the blob reaches the FW verbatim on the next 8x8 upload
        dev.set_caldata_xtalk(saved)
        dev.set_resolution(RESOLUTION_8X8)
        assert s.xtalk_uploads[-1] == saved
        with pytest.raises(Vl53l8cxError):
            dev.set_caldata_xtalk(saved[:-1])
    finally:
        dev.close()
        fake.close()


def test_calibrate_xtalk_reports_coverglass_too_good(cx):
    fake, dev = cx
    dev.init()
    fake.sensor.xtalk_cal_fails = True
    dev.calibrate_xtalk(reflectance_percent=20, nb_samples=1, distance_mm=3000)
    assert dev.uld.xtalk_calibration_failed is True
    assert fake.sensor.calibrations == [(3000 * 4, 20 * 16, 1)]
    assert dev.get_resolution() == RESOLUTION_4X4


@pytest.mark.parametrize("args", [(0, 4, 600), (100, 4, 600), (50, 0, 600), (50, 17, 600),
                                  (50, 4, 599), (50, 4, 3001)])
def test_calibrate_xtalk_argument_checks_stay_off_the_bus(cx, args):
    fake, dev = cx
    dev.init()
    writes = len(fake.sensor.writes)
    with pytest.raises(Vl53l8cxError):
        dev.calibrate_xtalk(*args)
    assert len(fake.sensor.writes) == writes


# ── get_frame and close ───────────────────────────────────────────────────────


def test_get_frame_on_a_closed_device(cx):
    fake, dev = cx
    dev.close()
    with pytest.raises(LinkClosedError):
        dev.get_frame(timeout=1.0)


def test_close_releases_a_pending_get_frame(cx):
    fake, dev = cx
    threading.Timer(0.1, dev.close).start()
    t0 = time.monotonic()
    with pytest.raises(LinkClosedError):
        dev.get_frame(timeout=5.0)
    assert time.monotonic() - t0 < 2.0


def test_a_queued_frame_wins_over_a_concurrent_close(cx):
    fake, dev = cx
    dev.init()
    dev.set_ranging_frequency_hz(15)
    dev.start_ranging()
    expected = fake.send_frame()
    time.sleep(0.05)
    dev.close()
    # the frame was parsed before close(): subscribers queued earlier got it
    assert dev.closed
    frame = dev._to_frame(expected["timestamp_us"], dev.uld.parse_frame(fake.sensor.build_frame()))
    assert frame.resolution == RESOLUTION_4X4


# ── bridge statuses (contract 04 §2) ─────────────────────────────────────────


def test_bridge_rejects_malformed_register_commands(cx):
    fake, dev = cx
    for cmd, payload, status in [
        (Vl53l8Cmd.READ_REG, b"\x00\x00", FakeVl53l8.ERR_PAYLOAD_FORMAT),  # 2 B, not 4
        (Vl53l8Cmd.READ_REG, struct.pack("<HH", 0, 0), FakeVl53l8.ERR_PAYLOAD_FORMAT),  # len 0
        (Vl53l8Cmd.READ_REG, struct.pack("<HH", 0, 2296), FakeVl53l8.ERR_PAYLOAD_FORMAT),
        (Vl53l8Cmd.WRITE_REG, b"\xff\x7f", FakeVl53l8.ERR_PAYLOAD_FORMAT),  # no data byte
        (Vl53l8Cmd.START_STREAM, b"\x00\x00", FakeVl53l8.ERR_INVALID_PARAM),  # 0
        (Vl53l8Cmd.START_STREAM, struct.pack("<H", 8193), FakeVl53l8.ERR_INVALID_PARAM),
        (Vl53l8Cmd.START_STREAM, b"\x01", FakeVl53l8.ERR_PAYLOAD_FORMAT),
        (0x34, b"", FakeVl53l8.ERR_INVALID_CMD),  # the gap in the id sequence
    ]:
        with pytest.raises(StatusError) as e:
            dev.request(cmd, payload, ok_completes=True)
        assert (e.value.cmd, e.value.status) == (cmd, status)
    assert not fake.streaming


# ── recorder: CNH records next to the frames (contract 09) ───────────────────


def test_session_recorder_persists_cnh_blocks(ch, tmp_path):
    fake, dev = ch
    dev.init()
    dev.set_resolution(RESOLUTION_8X8)
    dev.set_ranging_frequency_hz(15)
    cfg = _cnh_16x20()
    dev.configure_cnh(cfg)
    out = tmp_path / "cnh.depzdata"
    rec = SessionRecorder(out, vl53l8_layers=True, temperature_hz=None)
    did = rec.add(dev)
    rec.start()
    it = dev.frames(maxsize=4)  # subscribes now: get_frame() only sees frames sent after it is called
    dev.start_ranging()
    expected = [fake.send_frame() for _ in range(2)]
    for _ in expected:
        next(it)
    it.close()
    dev.stop_ranging()
    rec.stop()
    reader = DatasetReader(out)
    assert reader.devices[did]["sensor_type"] == "vl53l8"
    frames = [r for r in reader if r.kind == "vl53l8"]
    blocks = [r for r in reader if r.kind == "vl53l8_cnh"]
    assert len(frames) == len(blocks) == 2
    for fr, blk, exp in zip(frames, blocks, expected):
        assert fr.t_host_us == blk.t_host_us
        assert fr.value["distance_mm"] == exp["distance_mm"]
        assert fr.value["signal_per_spad"] == exp["signal_per_spad"]
        assert blk.value["nb_aggregates"] == 16 and blk.value["feature_length"] == 20
        assert blk.value["sum_span"] == 2 and blk.value["start_bin"] == 10
        assert base64.b64decode(blk.value["cnh_raw_b64"]) == cnh_block(16, 20)
