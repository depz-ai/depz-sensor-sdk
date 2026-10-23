"""Tmf8829 device class over a LoopbackLink: the far end plays the bridge and
emits RPT_TMF_FRAME chunks of the golden frames (contracts/vectors/
tmf8829.json); the device reassembles them into measurements. Covers
get_measurement() (next one / timeout / close) and clock_ratio_q16.

Against the fake bridge + sensor (fake_tmf8829.py): the host sensor sequence
of contract 14 §4 on the wire (init / configure / start / stop), argument
refusals, sensor and bridge error statuses, events, register access and the
streamed frames → measurements path checked against the golden measurement
vectors. The TypeScript tmf8829-device tests hold the same."""

from __future__ import annotations

import hashlib
import json
import struct
import threading
import time
from pathlib import Path

import pytest

from depz_sensor_sdk import Tmf8829
from depz_sensor_sdk.errors import DepzError, DepzTimeoutError, LinkClosedError, StatusError
from depz_sensor_sdk.protocol.tmf8829 import EventCode, Tmf8829Event, Tmf8829Rpt
from depz_sensor_sdk.tmf8829 import Tmf8829Error, Tmf8829InterruptEvent, firmware_image
from depz_sensor_sdk.tmf8829.clock import NOMINAL_Q16
from depz_sensor_sdk.transport import CrcType, build_packet
from depz_sensor_sdk.transport.link import LoopbackLink
from fake_tmf8829 import FakeTmf8829

VECTORS = Path(__file__).resolve().parents[3] / "contracts" / "vectors" / "tmf8829.json"
V = json.loads(VECTORS.read_text(encoding="utf-8"))
RAW = {c["name"]: bytes.fromhex(c["raw"]) for c in V["frames"]}
MEAS = {m["name"]: m for m in V["measurements"]}
CHUNK = 1000  # bytes of frame data per RPT_TMF_FRAME


class Bridge:
    """The board side of the link: sends frame chunks, answers nothing."""

    def __init__(self):
        self.link, self._side = LoopbackLink.pair()
        self._tx_seq = 0
        self._frame_seq = 0

    def emit_frame(self, raw: bytes, timestamp_us: int, edge_us: int) -> None:
        for off in range(0, len(raw), CHUNK):
            head = struct.pack("<QHHBQ", timestamp_us, len(raw), off, self._frame_seq, edge_us)
            self._side.write(build_packet(Tmf8829Rpt.FRAME, head + raw[off:off + CHUNK],
                                          self._tx_seq, CrcType.NONE))
            self._tx_seq = (self._tx_seq + 1) & 0xFF
        self._frame_seq = (self._frame_seq + 1) & 0xFF

    def play(self, name: str) -> None:
        for f in MEAS[name]["frames"]:
            self.emit_frame(RAW[f["raw_case"]], f["timestamp_us"], f["edge_us"])


@pytest.fixture()
def rig():
    bridge = Bridge()
    dev = Tmf8829(bridge.link, timeout=0.5)
    yield bridge, dev
    dev.close()


def _later(fn, delay: float = 0.05) -> threading.Thread:
    t = threading.Thread(target=lambda: (time.sleep(delay), fn()), daemon=True)
    t.start()
    return t


def test_get_measurement_returns_the_next_measurement(rig):
    bridge, dev = rig
    it = dev.measurements(4)
    frames = MEAS["8x8"]["frames"]
    want = MEAS["8x8"]["expect"]

    def emit(i):
        bridge.emit_frame(RAW[frames[i]["raw_case"]], frames[i]["timestamp_us"], frames[i]["edge_us"])

    _later(lambda: emit(0))
    m = dev.get_measurement(timeout=2.0)
    assert (m.f_number, m.timestamp_us) == (want[0]["f_number"], want[0]["timestamp_us"])
    _later(lambda: emit(1))
    m = dev.get_measurement(timeout=2.0)
    assert (m.f_number, m.timestamp_us) == (want[1]["f_number"], want[1]["timestamp_us"])
    # the one-shot queues are gone afterwards; the iterator saw both
    assert [next(it).f_number for _ in range(2)] == [w["f_number"] for w in want]
    assert len(dev._meas_queues) == 1


def test_get_measurement_times_out(rig):
    _, dev = rig
    t0 = time.monotonic()
    with pytest.raises(DepzTimeoutError) as exc:
        dev.get_measurement(timeout=0.1)
    assert exc.value.cmd == Tmf8829Rpt.FRAME
    assert time.monotonic() - t0 < 1.0
    assert dev._meas_queues == []


def test_get_measurement_ends_when_the_device_closes(rig):
    _, dev = rig
    _later(dev.close)
    t0 = time.monotonic()
    with pytest.raises(LinkClosedError):
        dev.get_measurement(timeout=5.0)
    assert time.monotonic() - t0 < 2.0
    with pytest.raises(LinkClosedError):
        dev.get_measurement(timeout=0.1)  # already closed: refused at once


def test_clock_ratio_q16_follows_the_clock_model(rig):
    bridge, dev = rig
    got = []
    dev.on_measurement(got.append)
    bridge.play("8x8")
    want = MEAS["8x8"]["expect"]
    deadline = time.monotonic() + 2.0
    while len(got) < len(want) and time.monotonic() < deadline:
        time.sleep(0.01)
    assert len(got) == len(want)
    assert dev.clock_ratio_q16 == want[-1]["ratio_q16"] == got[-1].times[-1].ratio_q16
    # it reads the live model, not a constant
    dev._builder.clock.ratio_q16 = NOMINAL_Q16 + 123
    assert dev.clock_ratio_q16 == NOMINAL_Q16 + 123


# ── against the fake bridge + sensor ─────────────────────────────────────────

W = 0x33  # TMF_WRITE_REG
R = 0x32  # TMF_READ_REG


@pytest.fixture()
def fake():
    f = FakeTmf8829()
    yield f
    f.close()


@pytest.fixture()
def dev(fake):
    d = Tmf8829(fake.link, timeout=0.5)
    yield d
    d.close()


def _wait_for(out: list, n: int) -> None:
    deadline = time.monotonic() + 2.0
    while len(out) < n and time.monotonic() < deadline:
        time.sleep(0.01)
    assert len(out) == n


def _play(fake: FakeTmf8829, name: str) -> None:
    for f in MEAS[name]["frames"]:
        fake.emit_frame(RAW[f["raw_case"]], f["timestamp_us"], f["edge_us"])


# ── init ──


def test_init_runs_the_contract_sequence_and_downloads_the_firmware(fake, dev):
    progress = []
    ver, serial = dev.init(progress=lambda d, t: progress.append((d, t)))
    assert (ver, serial) == ("2.200.0", 0x12345678)
    assert (dev.app_version, dev.sensor_serial) == ("2.200.0", 0x12345678)
    assert dev.config is None

    w = fake.writes()
    assert w[0] == [0xF8, 0x14]  # PON | bootmonitor
    assert w[1] == [0xF7, 0x40]  # soft reset
    assert list(fake.sent(R)[0]) == [0xE3, 1, 0]  # ID read first after the reset
    assert w[2] == [0x08, 0x22]  # BL I2C_OFF
    # W_FIFO_BOTH: addr 0x00010000, words 3611
    assert w[3] == [0x08, 0x45, 6, 0x00, 0x00, 0x01, 0x00, 3611 & 0xFF, 3611 >> 8]
    assert fake.w_fifo_params == bytes([0, 0, 1, 0, 0x1B, 0x0E])
    fifo = [x for x in w if x[0] == 0xFF]
    assert len(fifo) == -(-14444 // 1023)
    assert all(len(x) - 1 <= 1023 for x in fifo)
    assert bytes(fake.fifo) == firmware_image()
    assert hashlib.sha256(fake.fifo).hexdigest() == V["firmware"]["sha256"]
    after = w[w.index(fifo[-1]) + 1:]
    assert after[0] == [0x08, 0x16]  # START_RAM_APP
    # ENABLE read back 0x94 (cpu_ready | PON | bootmon) → powerup_select = RAM
    assert after[1] == [0xF8, 0xA4]
    assert len(progress) == len(fifo)
    assert progress[-1] == (14444, 14444)
    assert all(b[0] > a[0] for a, b in zip(progress, progress[1:]))
    # app version 0x00..0x03, then the serial 0x1C..0x1F
    assert [list(p) for p in fake.sent(R)[-2:]] == [[0x00, 4, 0], [0x1C, 4, 0]]
    assert fake.mode == "app"


def test_init_survives_a_busy_cmd_stat(fake, dev):
    fake.busy_polls = 3
    assert dev.init() == ("2.200.0", 0x12345678)


def test_init_refuses_a_wrong_sensor_id_before_downloading(fake, dev):
    fake.device_id = 0x00  # silent SPI
    with pytest.raises(Tmf8829Error, match=r"sensor ID 0x00, expected 0x9E"):
        dev.init()
    assert len(fake.fifo) == 0
    assert len(fake.writes()) == 2


def test_init_times_out_when_the_bootloader_never_shows(fake, dev):
    fake.stuck_before_bootloader = True
    with pytest.raises(Tmf8829Error, match=r"sensor bootloader: timeout, reg 0x00 = 0x00"):
        dev.init()


def test_init_names_a_bootloader_status_or_gives_it_in_hex(fake, dev):
    fake.bl_status[0x45] = 2
    with pytest.raises(Tmf8829Error, match=r"^bootloader cmd 0x45 -> ERR_ADDR$"):
        dev.init()
    assert len(fake.fifo) == 0
    fake.bl_status = {0x16: 7}
    with pytest.raises(Tmf8829Error, match=r"^bootloader cmd 0x16 -> 0x7$"):
        dev.init()


def test_init_stops_a_running_stream_first(fake, dev):
    dev.configure()
    dev.start()
    assert dev.streaming
    dev.init()
    assert not dev.streaming
    assert len(fake.sent(0x36)) == 1  # TMF_STOP_STREAM
    w = fake.writes()
    assert w.index([0x08, 0xFF]) < next(i for i, x in enumerate(w) if x[0] == 0xF7)


def test_a_failed_init_does_not_wedge_the_next_one(fake, dev):
    fake.device_id = 0x12
    with pytest.raises(DepzError):
        dev.init()
    fake.device_id = 0x9E
    assert dev.init() == ("2.200.0", 0x12345678)


# ── configure ──


def test_configure_writes_every_field_a_preset_does_not_reset(fake, dev):
    # Stale fields a preset does not reset (contract 14 §4 table).
    fake.cfg[0x2B - 0x22] = 1  # dump_histograms
    fake.cfg[0x2C - 0x22] = 1  # ref_spad_frame
    fake.cfg[0x2A - 0x22] = 0x3C  # 4 peaks + all strengths
    fake.cfg[0] = 100  # period
    cfg = dev.configure()

    w = fake.writes()
    assert w[0] == [0x08, 0x40]  # preset 8x8
    assert w[1] == [0x08, 0x16]  # LOAD_CONFIG_PAGE
    assert w[2][0] == 0x22 and len(w[2]) == 1 + 190  # the whole page
    assert w[3] == [0x08, 0x15]  # WRITE_PAGE
    assert w[4] == [0x08, 0x16]  # read back
    reads = [list(p) for p in fake.sent(R)]
    assert [0x20, 1, 0] in reads  # CID_RID
    assert [0x22, 190, 0] in reads  # the page

    assert (cfg.preset, cfg.grid, cfg.histograms, cfg.ref_frame) == ("8x8", (8, 8), False, False)
    assert len(cfg.page) == 190
    want = {"period_ms": 33, "kilo_iterations": 586, "fp_mode": 0, "nr_peaks": 1,
            "signal_strength": 0, "noise_strength": 0, "xtalk": 0,
            "dump_histograms": 0, "ref_spad_frame": 0}
    assert {k: cfg.fields[k] for k in want} == want
    assert dev.config is cfg


def test_configure_writes_the_caller_options(fake, dev):
    cfg = dev.configure("32x32", period_ms=300, nr_peaks=2, signal_strength=True,
                        noise_strength=True, xtalk=True, histograms=True, ref_frame=True,
                        fields={"power_modes": 3})
    assert fake.writes()[0] == [0x08, 0x45]
    page = fake.writes()[2][1:]
    assert page[0] | page[1] << 8 == 300
    assert page[2] | page[3] << 8 == 2344  # preset kilo_iterations
    assert page[0x2A - 0x22] == 2 | 0x08 | 0x10 | 0x20
    assert (page[0x2B - 0x22], page[0x2C - 0x22], page[0x2E - 0x22]) == (1, 1, 3)
    assert (cfg.grid, cfg.histograms, cfg.ref_frame, cfg.fields["fp_mode"]) == ((32, 32), True, True, 3)


def test_configure_takes_an_explicit_kilo_iterations(fake, dev):
    cfg = dev.configure("48x32", kilo_iterations=1000)
    page = fake.writes()[2][1:]
    assert page[2] | page[3] << 8 == 1000
    assert cfg.fields["kilo_iterations"] == 1000
    assert cfg.grid == (32, 48)


@pytest.mark.parametrize("kwargs", [
    {"preset": "9x9"},
    {"nr_peaks": 0},
    {"nr_peaks": 5},
    {"nr_peaks": 1.5},
])
def test_configure_refuses_bad_arguments_before_sending(fake, dev, kwargs):
    with pytest.raises(ValueError, match=r"unknown preset|nr_peaks .* must be 1\.\.4"):
        dev.configure(**kwargs)
    assert fake.log == []


def test_configure_refuses_while_streaming(fake, dev):
    dev.start()
    n = len(fake.log)
    with pytest.raises(DepzError, match="while streaming"):
        dev.configure()
    assert len(fake.log) == n


def test_configure_names_the_sensor_refusal_of_the_page(fake, dev):
    fake.app_status[0x15] = 0x04
    with pytest.raises(Tmf8829Error, match=r"^sensor cmd 0x15 -> ERR_CONFIG_RESULT_SIZE$"):
        dev.configure("48x32", nr_peaks=4)
    fake.app_status[0x15] = 0x0F
    with pytest.raises(Tmf8829Error, match=r"^sensor cmd 0x15 -> 0xf$"):
        dev.configure()


def test_configure_rejects_a_field_that_does_not_read_back(fake, dev):
    fake.stuck_bytes[0x78 - 0x22] = 5  # fov_correction: 3 written, 5 read (measured)
    with pytest.raises(Tmf8829Error,
                       match="config fields did not stick: fov_correction wrote 3, reads 5"):
        dev.configure("8x8", fields={"fov_correction": 3})
    assert dev.config is None


def test_config_page_checks_cid_rid(fake, dev):
    fake.cid_override = 0x40
    with pytest.raises(Tmf8829Error, match="CID_RID 0x40 after LOAD_CONFIG_PAGE"):
        dev.config_page()


def test_config_page_returns_the_sensor_page(fake, dev):
    fake.cfg[5] = 0xAB
    page = dev.config_page()
    assert len(page) == 190 and page[5] == 0xAB


def test_a_sensor_command_that_stays_busy_times_out(fake, dev):
    fake.busy_polls = 10**9
    with pytest.raises(Tmf8829Error, match=r"sensor cmd 0x16: timeout, reg 0x08 = 0x16"):
        dev._app_cmd(0x16, timeout=0.05)


# ── start / stop ──


def test_start_configures_8x8_when_needed_and_arms_the_stream(fake, dev):
    dev.start()
    assert dev.streaming
    assert dev.config.preset == "8x8"
    w = fake.writes()
    i = next(i for i, x in enumerate(w) if x[0] == 0xFA)
    assert w[i:i + 3] == [[0xFA, 0x30], [0xE1, 0xFF], [0xE2, 0x01]]
    assert fake.sent(0x35) == [bytes([0x01, 0x00])]
    assert w[-1] == [0x08, 0x10]  # MEASURE after the stream is armed
    start_i = next(i for i, (c, _) in enumerate(fake.log) if c == 0x35)
    meas_i = next(i for i, (c, p) in enumerate(fake.log) if c == W and p[:2] == b"\x08\x10")
    assert start_i < meas_i

    dev.start()  # no-op while streaming
    assert len(fake.sent(0x35)) == 1


def test_start_arms_histograms_and_the_ref_frame_from_the_config(fake, dev):
    dev.configure("8x8", histograms=True, ref_frame=True)
    dev.start()
    assert [0xE2, 0x09] in fake.writes()
    assert fake.sent(0x35) == [bytes([0x09, 0x01])]


def test_start_disarms_the_bridge_when_measure_is_refused(fake, dev):
    dev.configure()
    fake.app_status[0x10] = 0x03
    with pytest.raises(Tmf8829Error, match=r"^sensor cmd 0x10 -> ERR_APPLICATION$"):
        dev.start()
    assert not dev.streaming
    assert len(fake.sent(0x36)) == 1


def test_stop_sends_stop_clears_interrupts_then_disarms_the_bridge(fake, dev):
    dev.start()
    n = len(fake.log)
    dev.stop()
    tail = [[c, *p] for c, p in fake.log[n:] if c != R]
    assert tail == [[W, 0x08, 0xFF], [W, 0xE2, 0x00], [W, 0xE1, 0xFF], [0x36]]
    assert not dev.streaming
    # Not streaming: the sensor is still stopped but the bridge is left alone.
    dev.stop()
    assert len(fake.sent(0x36)) == 1


def test_stop_swallows_the_bad_header_a_stop_raises_other_events_pass(fake, dev):
    events = []
    unsub = dev.on_event(events.append)
    dev.start()
    fake.on_command = lambda cmd: (
        fake.emit_event(EventCode.BAD_HEADER, 0x08, 0xFF, 0x04) if cmd == 0xFF else None)
    dev.stop()
    assert events == []

    fake.emit_event(EventCode.INT, 0x02)
    fake.emit_event(EventCode.BAD_HEADER, 0x01, 0x55, 0x00)  # sensor lost
    _wait_for(events, 2)
    assert all(isinstance(e, Tmf8829InterruptEvent) for e in events)
    assert events[0].event == Tmf8829Event(987654321, 0x02, 0, 0, 0)
    assert (events[1].event.code, events[1].event.frame_id) == (1, 0x55)
    unsub()
    fake.emit_event(EventCode.INT, 0x04)
    time.sleep(0.05)
    assert len(events) == 2


def test_a_truncated_event_is_ignored(fake, dev):
    events = []
    dev.on_event(events.append)
    fake.emit_raw(0x94, bytes(5))
    fake.emit_event(EventCode.INT, 0x04)  # the reader is still alive
    _wait_for(events, 1)
    assert events[0].event.int_status == 0x04


def test_stop_raises_the_sensor_stop_status_and_leaves_streaming_off(fake, dev):
    dev.start()
    fake.app_status[0xFF] = 0x0A
    with pytest.raises(Tmf8829Error, match=r"^sensor cmd 0xFF -> ERR_STOP_0$"):
        dev.stop()
    assert not dev.streaming


def test_close_stops_a_running_stream(fake, dev):
    dev.start()
    dev.close()
    assert [0x08, 0xFF] in fake.writes()
    assert len(fake.sent(0x36)) == 1


def test_close_survives_a_refused_stop(fake, dev):
    dev.start()
    fake.app_status[0xFF] = 0x0A
    dev.close()  # the STOP error is swallowed, the link is closed anyway
    assert dev.closed


# ── streaming → measurements ──


@pytest.mark.parametrize("name", list(MEAS))
def test_streamed_frames_match_the_golden_measurements(fake, dev, name):
    got = []
    dev.on_measurement(got.append)
    dev.start()
    _play(fake, name)
    want = MEAS[name]["expect"]
    _wait_for(got, len(want))
    for m, e in zip(got, want):
        assert m.timestamp_us == e["timestamp_us"]
        assert (m.flags, m.fp_mode, m.f_number) == (e["flags"], e["fp_mode"], e["f_number"])
        assert [t.timestamp_us for t in m.times] == e["frame_times_us"]
        assert m.times[-1].ratio_q16 == e["ratio_q16"]
        assert (len(m.histograms), len(m.ref_frames), len(m.problems)) == (
            e["n_histograms"], e["n_ref_frames"], e["n_problems"])
        assert all(f.distance_in_mm == e["distance_in_mm"] for f in m.frames)
        assert (m.peaks, *m.shape) == (e["peaks"], e["rows"], e["cols"])
        assert m.results.distance.ravel().tolist() == e["distance"]
        assert m.results.snr.ravel().tolist() == e["snr"]
        for k in ("signal", "noise", "xtalk"):
            arr = getattr(m.results, k)
            assert (None if arr is None else arr.ravel().tolist()) == e[k]
        # distance_mm = raw × ratio (/4 in 0.25 mm units), ratio = q16 / (8 << 16)
        d0 = e["distance"][0] * e["ratio_q16"] / (8 << 16)
        assert m.distance_mm.ravel()[0] == pytest.approx(d0 if e["distance_in_mm"] else d0 / 4)
    assert (dev.frames_dropped, dev.frames_discarded, dev.frame_parse_errors) == (0, 0, 0)
    assert dev.clock_ratio_q16 == want[-1]["ratio_q16"]


def test_histogram_frames_are_reassembled_from_1024_byte_chunks(fake, dev):
    got = []
    dev.on_measurement(got.append)
    dev.start()
    _play(fake, "8x8_hist")
    _wait_for(got, 1)
    assert [len(h.raw) for h in got[0].histograms] == [25377, 25377]
    assert got[0].histograms[0].raw == RAW["8x8_hist#0"]


def test_measurements_reach_the_iterator_get_measurement_and_every_subscriber(fake, dev):
    a, b = [], []
    dev.on_measurement(lambda m: a.append(m.f_number))
    unsub_b = dev.on_measurement(lambda m: b.append(m.f_number))
    it = dev.measurements(4)
    dev.start()
    _later(lambda: fake.emit_frame(RAW["8x8#0"], 2125575099, 2125575071))
    assert dev.get_measurement(timeout=2.0).f_number == 1
    unsub_b()
    fake.emit_frame(RAW["8x8#1"], 2125672180, 2125672161)
    _wait_for(a, 2)
    assert b == [1]
    assert [next(it).f_number for _ in range(2)] == [1, 2]


def test_lost_and_broken_frames_are_counted_not_raised(fake, dev):
    got = []
    dev.on_measurement(got.append)
    dev.start()
    fake.emit_frame(RAW["8x8#0"], 2125575099, 2125575071, seq=0)
    # frame_seq jumps 1 → 3: two frames dropped by the bridge
    fake.emit_frame(RAW["8x8#1"], 2125672180, 2125672161, seq=3)
    # a frame cut short (STOP mid frame): only its first chunk arrives
    fake.emit_frame(RAW["8x8_full#0"], 2125937131, 0, seq=4, chunks=[0])
    fake.emit_frame(RAW["8x8#1"][:40], 1, 0, seq=5)  # truncated frame: parse error
    fake.emit_raw(0x93, bytes(10))  # truncated chunk header: ignored
    _wait_for(got, 2)
    time.sleep(0.05)
    assert (dev.frames_dropped, dev.frames_discarded, dev.frame_parse_errors) == (2, 1, 1)

    # start() resets the counters (frame_seq restarts at 0)
    dev.stop()
    dev.start()
    assert (dev.frames_dropped, dev.frames_discarded, dev.frame_parse_errors) == (0, 0, 0)


# ── register access and diagnostics ──


def test_reads_go_in_1024_byte_requests_that_stop_advancing_at_the_fifo(fake, dev):
    out = dev.read_registers(0xF0, 2000)
    assert len(out) == 2000
    assert [list(p) for p in fake.sent(R)] == [[0xF0, 0x00, 0x04], [0xFF, 976 & 0xFF, 976 >> 8]]


def test_writes_to_the_fifo_go_in_1023_byte_chunks_all_at_0xff(fake, dev):
    progress = []
    dev.write_registers(0xFF, bytes([7]) * 2100, lambda d, t: progress.append((d, t)))
    assert [(p[0], len(p) - 1) for p in fake.sent(W)] == [(0xFF, 1023), (0xFF, 1023), (0xFF, 54)]
    assert len(fake.fifo) == 2100
    assert progress == [(1023, 2100), (2046, 2100), (2100, 2100)]


def test_write_register_sends_the_low_byte(fake, dev):
    dev.write_register(0xE2, 0x109)
    assert fake.sent(W) == [bytes([0xE2, 0x09])]


@pytest.mark.parametrize("call", [
    lambda d: d.read_registers(0x100, 1),
    lambda d: d.read_registers(-1, 1),
    lambda d: d.write_registers(0x100, b"\x00"),
])
def test_an_address_outside_0_255_is_refused_before_sending(fake, dev, call):
    with pytest.raises(ValueError, match="addr must be 0..255"):
        call(dev)
    assert fake.log == []


def test_a_short_read_reg_reply_is_rejected(fake, dev):
    fake.short_read = 1
    with pytest.raises(DepzError, match=r"^READ_REG 0x00: expected 4, got 3$"):
        dev.read_registers(0x00, 4)


def test_a_bridge_error_status_surfaces_and_later_calls_still_work(fake, dev):
    fake.bridge_status[W] = 0x03
    with pytest.raises(StatusError):
        dev.write_registers(0xE2, b"\x01")
    fake.bridge_status.clear()
    dev.write_registers(0xE2, b"\x01")
    assert fake.regs[0xE2] == 1


def test_an_unanswered_read_reg_times_out_after_the_1s_register_timeout(fake, dev):
    fake.silent.add(R)
    t0 = time.monotonic()
    with pytest.raises(DepzTimeoutError):
        dev.read_registers(0x00, 1)
    assert time.monotonic() - t0 >= 0.9  # not the 0.5 s device default


def test_sensor_state_reads_the_host_interface_registers(fake, dev):
    dev.init()
    st = dev.sensor_state()
    assert list(st) == ["ID", "REVID", "ENABLE", "RESET", "APP_ID", "CMD_STAT",
                        "INT_STATUS", "INT_ENAB", "FIFOSTATUS"]
    assert (st["ID"], st["APP_ID"], st["ENABLE"] & 0x80) == (0x9E, 0x01, 0x80)


def test_get_bridge_info_decodes_rpt_tmf_info(fake, dev):
    dev.start()
    info = dev.get_bridge_info()
    assert (info.int_edges, info.frames, info.frames_aborted) == (7, 6, 1)
    assert (info.streaming, info.int_mask) == (True, 0x01)
