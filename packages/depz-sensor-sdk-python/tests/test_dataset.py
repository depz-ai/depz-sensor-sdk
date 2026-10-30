"""Dataset recording/playback (contract 09): multi-device, time-synced."""

import json
import threading
import time
from pathlib import Path

import pytest

from depz_sensor_sdk import Sr04
from depz_sensor_sdk.dataset import DatasetReader, DatasetWriter, SessionRecorder
from depz_sensor_sdk.errors import InvalidStateError

from fake_device import FakeSr04


def _make_rig(mcu_time_us: int):
    fake = FakeSr04()
    fake.mcu_time_us = mcu_time_us
    dev = Sr04(fake.link, timeout=1.0)
    return fake, dev


def test_two_devices_merged_timeline(tmp_path: Path):
    """Two fake sensors with wildly different device clocks must land on one
    host timeline ordered by capture time."""
    fake_a, dev_a = _make_rig(mcu_time_us=1_000_000)  # device clock ~1 s
    fake_b, dev_b = _make_rig(mcu_time_us=9_000_000_000)  # device clock ~2.5 h
    path = tmp_path / "session.depzdata"
    try:
        rec = SessionRecorder(path, note="unit test")
        id_a = rec.add(dev_a)
        id_b = rec.add(dev_b)
        assert (id_a, id_b) == ("d0", "d1")
        rec.start()

        # interleave: a, b, a, b — each send bumps the fake clock
        fake_a.send_measurement(0x37)
        time.sleep(0.02)
        fake_b.send_measurement(0x37)
        time.sleep(0.02)
        fake_a.send_measurement(0x37)
        time.sleep(0.02)
        fake_b.send_measurement(0x36)
        time.sleep(0.1)  # let the reader threads deliver
        rec.stop()
    finally:
        dev_a.close()
        dev_b.close()
        fake_a.close()
        fake_b.close()

    reader = DatasetReader(path)
    assert set(reader.devices) == {"d0", "d1"}
    for meta in reader.devices.values():
        assert meta["sensor_type"] == "sr04"
        assert "offset_us" in meta["time_sync"]

    records = list(reader)
    assert len(records) == 4
    # merged order must follow host time regardless of device clocks
    ts = [r.t_host_us for r in records]
    assert ts == sorted(ts)
    assert [r.device_id for r in records] == ["d0", "d1", "d0", "d1"]
    assert {r.kind for r in records} == {"sr04"}
    assert records[3].value["source"] == "once"

    # host-time mapping: t = device_ts - offset (contract 09)
    hdr = json.loads(path.read_text(encoding="utf-8").splitlines()[0])
    off_a = hdr["devices"]["d0"]["time_sync"]["offset_us"]
    assert all(abs(r.t_host_us) < 10**15 for r in records)
    assert isinstance(off_a, int)


def test_playback_pacing_and_stop(tmp_path: Path):
    fake, dev = _make_rig(mcu_time_us=1_000_000)
    fake.sample_period_us = 50_000  # 50 ms device-time steps
    path = tmp_path / "run.depzdata"
    try:
        rec = SessionRecorder(path)
        rec.add(dev)
        rec.start()
        for _ in range(4):
            fake.send_measurement(0x37)
        time.sleep(0.1)
        rec.stop()
    finally:
        dev.close()
        fake.close()

    reader = DatasetReader(path)
    got: list[int] = []
    t0 = time.monotonic()
    reader.play(lambda r: got.append(r.t_host_us), speed=0)  # as fast as possible
    assert len(got) == 4
    assert time.monotonic() - t0 < 0.2

    # paced at 2x: 3 gaps × 50 ms = 150 ms of data → ~75 ms wall
    got.clear()
    t0 = time.monotonic()
    reader.play(lambda r: got.append(r.t_host_us), speed=2.0)
    wall = time.monotonic() - t0
    assert len(got) == 4
    assert 0.04 <= wall <= 0.4

    # stop event aborts playback
    stop = threading.Event()
    seen = []

    def cb(r):
        seen.append(r)
        if len(seen) == 2:
            stop.set()

    reader.play(cb, speed=0.5, stop=stop)
    assert len(seen) == 2


def test_vl53lx_and_bno055_records(tmp_path: Path):
    """Contract 09 `vl53lx` / `bno055` kinds. The device classes ride the fake
    SR04 link (it answers the common commands the recorder needs); samples
    are pushed straight into the subscribers, as the reader thread does."""
    from depz_sensor_sdk.bno055 import Bno055, Bno055Sample, Units
    from depz_sensor_sdk.bno055.regs import FULL_BLOCK
    from depz_sensor_sdk.vl53lx import Target, Vl53l1cx, Vl53lxMeasurement

    fake_a, fake_b = FakeSr04(), FakeSr04()
    lx = Vl53l1cx(fake_a.link, timeout=1.0)
    imu = Bno055(fake_b.link, timeout=1.0)
    path = tmp_path / "session.depzdata"
    try:
        rec = SessionRecorder(path)
        rec.add(lx)
        rec.add(imu)
        rec.start()
        lx._product, lx._driver_kind = "VL53L1CX", "histogram"
        t1 = Target(612, 0, "Range valid", 812.5, 3.1, 4.2, 600, 625)
        t2 = Target(1480, 11, "Merged pulse", 90.0, 3.1, 9.9, 1460, 1500)
        m = Vl53lxMeasurement(2_000_000, 612, 0, "Range valid", 812.5, 3.1, 4.2, 12.5, (t1, t2))
        for cb in list(lx._measure_cbs):
            cb(m)
        block = bytes.fromhex(
            "c5ffb7ff9a032f00e2ffadfefeffffff0000cd046a0578e6b52963f9efcf0000"
            "0000fbff0600b1ff0cfeb6fe1b0133")
        s = Bno055Sample.decode(3_000_000, FULL_BLOCK[0], block, Units(euler_rad=True))
        for cb in list(imu._sample_cbs):
            cb(s)
        rec.stop()
    finally:
        lx.close()
        imu.close()
        fake_a.close()
        fake_b.close()

    recs = {r.kind: r for r in DatasetReader(path)}
    v = recs["vl53lx"].value
    assert (v["product"], v["driver"], v["status"], v["distance_mm"]) == ("VL53L1CX", "histogram", 0, 612)
    assert v["targets"] == [
        {"distance_mm": 612, "status": 0, "signal_kcps": 812.5},
        {"distance_mm": 1480, "status": 11, "signal_kcps": 90.0},
    ]
    b = recs["bno055"].value
    assert b["unit_sel"] == 0x04
    assert b["quaternion"] == list(s.quaternion)
    assert b["euler"] == list(s.euler) and b["gravity"] == list(s.gravity)
    assert b["temperature"] == s.temperature
    assert b["calib"] == [s.calibration.system, s.calibration.gyro, s.calibration.accel,
                          s.calibration.mag]


def test_bno086_records(tmp_path: Path):
    """Contract 09 `bno086`: every report the subscribers see, snake_case, full
    components; `t` is the report's measurement time on the host timeline (its
    device `timestamp_us` minus the sync offset), not arrival time — an IMU
    recorded next to a ToF must share the ToF's clock."""
    from depz_sensor_sdk.bno086 import Bno086
    from depz_sensor_sdk.bno086.reports import Acceleration, RotationVector

    fake = FakeSr04()
    imu = Bno086(fake.link, timeout=1.0)
    path = tmp_path / "session.depzdata"
    rv = RotationVector(0x05, 5_000_300, 7, 3, 0, 1 << 13, 0, 0, 14189, 820)
    acc = Acceleration(0x01, 5_000_100, 8, 2, 0, 0, 0, 2511)  # 9.8 m/s² on z
    try:
        rec = SessionRecorder(path)
        rec.add(imu)
        rec.start()
        # One cargo, device times out of order: the file holds them as they
        # came, the reader hands them back sorted by `t`.
        for r in (rv, acc):
            for cb, _ in list(imu._report_cbs):
                cb(r)
        rec.stop()
    finally:
        imu.close()
        fake.close()

    reader = DatasetReader(path)
    offset = reader.devices["d0"]["time_sync"]["offset_us"]
    recs = [r for r in reader if r.kind == "bno086"]
    assert [r.value["type"] for r in recs] == ["Acceleration", "RotationVector"]
    assert [r.t_host_us for r in recs] == [5_000_100 - offset, 5_000_300 - offset]
    q = recs[1].value
    assert (q["sensor_id"], q["i"], q["real"]) == (0x05, 0.5, rv.real)
    assert q["accuracy_rad"] == rv.accuracy_rad and q["timestamp_us"] == 5_000_300
    assert "x" not in q
    a = recs[0].value
    assert (a["sensor_id"], a["z"], a["timestamp_us"]) == (0x01, acc.z, 5_000_100)
    assert "i" not in a and "accuracy_rad" not in a


def test_reader_sorts_within_a_device(tmp_path: Path):
    """Contract 09: `t` may step back within a device (BNO086 cargos, arrival-
    time temperature records); the reader still yields one sorted timeline."""
    path = tmp_path / "s.depzdata"
    w = DatasetWriter(path, {"d0": {}, "d1": {}})
    for d, t in (("d0", 300), ("d0", 100), ("d1", 200), ("d0", 250), ("d1", 50)):
        w.write(d, t, "event", {})
    w.close()
    with pytest.raises(InvalidStateError):
        w.write("d0", 400, "event", {})  # contract 07 §3 (ERRATA A17)
    with open(path, "a", encoding="utf-8", newline="\n") as f:
        f.write("\n   \n")  # blank lines are not records
    assert [(r.device_id, r.t_host_us) for r in DatasetReader(path)] == [
        ("d1", 50), ("d0", 100), ("d1", 200), ("d0", 250), ("d0", 300)]


def test_committed_dual_sr04_dataset(seen):
    """The committed fixture reads back record for record: header devices,
    every line as a record, one host-time-sorted timeline (TS reads it too)."""
    path = Path(__file__).resolve().parents[3] / "contracts/vectors/recordings/dataset_dual_sr04.depzdata"
    lines = [json.loads(line) for line in path.read_text(encoding="utf-8").splitlines() if line.strip()]
    reader = DatasetReader(path)
    assert reader.devices == lines[0]["devices"]
    assert set(reader.devices) == {"d0", "d1"}
    recs = list(reader)
    assert [(r.device_id, r.t_host_us, r.kind, r.value) for r in recs] == [
        (ev["d"], ev["t"], ev["k"], ev["v"]) for ev in lines[1:]
    ]
    assert [r.t_host_us for r in recs] == sorted(r.t_host_us for r in recs)
    assert {r.kind for r in recs} == {"sr04"}
    seen("recordings/dataset_dual_sr04.depzdata")


class _MuteFakeSr04(FakeSr04):
    """Firmware that refuses identity and temperature: the recorder must
    still register and record the device (contract 09: meta is best effort)."""

    def _handle(self, pkt):
        if pkt.cmd in (0x04, 0x05, 0x07):
            self._status(pkt.cmd, 0x07)  # ERR_CMD_NOT_SUPPORTED
            return
        super()._handle(pkt)


def test_recorder_lifecycle_rules(tmp_path: Path):
    """add() only before start, start() once, stop() idempotent, and a
    recorder that never started leaves no file behind."""
    fake, dev = _make_rig(mcu_time_us=1_000_000)
    path = tmp_path / "run.depzdata"
    try:
        with SessionRecorder(tmp_path / "never.depzdata", temperature_hz=None) as rec0:
            rec0.add(dev)
        assert not (tmp_path / "never.depzdata").exists()
        assert rec0.records_written == 0

        with SessionRecorder(path, temperature_hz=None) as rec:
            rec.add(dev, device_id="front")
            rec.start()
            rec.start()  # no-op
            with pytest.raises(InvalidStateError):
                rec.add(dev)
            fake.send_measurement(0x37)
            time.sleep(0.05)
            assert rec.records_written == 1
        assert rec.records_written == 1  # the finished file's count survives exit
        rec.stop()  # idempotent after exit
        assert rec.records_written == 1
    finally:
        dev.close()
        fake.close()
    reader = DatasetReader(path)
    assert list(reader.devices) == ["front"]
    assert [(r.device_id, r.kind) for r in reader] == [("front", "sr04")]
    # the file is complete and closed: the records are readable twice
    assert reader.records == list(reader)


def test_recorder_tolerates_a_device_without_identity_or_temperature(tmp_path: Path):
    from depz_sensor_sdk.device import DeviceBase

    fake = _MuteFakeSr04()
    dev = DeviceBase(fake.link, timeout=1.0)  # no sensor class: nothing to hook
    path = tmp_path / "mute.depzdata"
    try:
        rec = SessionRecorder(path, temperature_hz=50)
        did = rec.add(dev)
        rec.start()
        time.sleep(0.1)  # a few sampler periods: every reading is refused
        rec.stop()
    finally:
        dev.close()
        fake.close()
    reader = DatasetReader(path)
    meta = reader.devices[did]
    assert meta["sensor_type"] == "unknown"
    assert "serial" not in meta and "software_name" not in meta
    assert "offset_us" in meta["time_sync"]
    assert list(reader) == []


def test_recorder_samples_mcu_and_sensor_temperature(tmp_path: Path):
    """Contract 09 `temperature`: ~temperature_hz records per device from the
    MCU thermometer, plus the sensor die once a frame reported it."""
    import numpy as np

    from depz_sensor_sdk.vl53l8 import Vl53l8, Vl53l8Frame

    fake = FakeSr04()
    dev = Vl53l8(fake.link, timeout=1.0)
    path = tmp_path / "temp.depzdata"
    z = 16
    frame = Vl53l8Frame(
        timestamp_us=2_000_000, resolution=z,
        distance_mm=np.arange(64, dtype=np.int32), target_status=np.full(64, 5, np.uint8),
        nb_target_detected=np.ones(64, np.uint8), signal_per_spad=np.zeros(64),
        ambient_per_spad=np.zeros(64), nb_spads_enabled=np.zeros(64, np.int32),
        range_sigma_mm=np.zeros(64), reflectance=np.zeros(64), silicon_temp_degc=41,
        motion={"global_indicator_1": 7, "motion": [0, 1, 2]},
    )
    try:
        rec = SessionRecorder(path, temperature_hz=50)
        did = rec.add(dev)
        rec.start()
        for cb in list(dev._frame_cbs):
            cb(frame)
        time.sleep(0.15)
        rec.stop()
    finally:
        dev.close()
        fake.close()
    reader = DatasetReader(path)
    offset = reader.devices[did]["time_sync"]["offset_us"]
    recs = list(reader)
    frames = [r for r in recs if r.kind == "vl53l8"]
    assert len(frames) == 1 and frames[0].t_host_us == 2_000_000 - offset
    v = frames[0].value
    assert v["distance_mm"] == list(range(z)) and v["motion"] == frame.motion
    assert "signal_per_spad" not in v  # layers off by default
    temps = [r.value for r in recs if r.kind == "temperature"]
    sources = {t["source"] for t in temps}
    assert sources == {"mcu", "sensor"}
    assert all(t["celsius"] == 41 for t in temps if t["source"] == "sensor")
    assert all(t["celsius"] == pytest.approx(27.3) for t in temps if t["source"] == "mcu")
    assert len(temps) >= 2


def test_recorder_works_without_the_vl53l8_extra(tmp_path: Path, monkeypatch):
    """The VL53L8 class is optional (numpy); the recorder still hooks the other
    sensors when its import fails."""
    import sys

    from depz_sensor_sdk.bno055 import Bno055, Bno055Sample, Units
    from depz_sensor_sdk.bno055.regs import FULL_BLOCK

    monkeypatch.setitem(sys.modules, "depz_sensor_sdk.vl53l8", None)
    fake = FakeSr04()
    imu = Bno055(fake.link, timeout=1.0)
    path = tmp_path / "imu.depzdata"
    try:
        rec = SessionRecorder(path, temperature_hz=None)
        rec.add(imu)
        rec.start()
        block = bytes.fromhex(
            "c5ffb7ff9a032f00e2ffadfefeffffff0000cd046a0578e6b52963f9efcf0000"
            "0000fbff0600b1ff0cfeb6fe1b0133")
        s = Bno055Sample.decode(3_000_000, FULL_BLOCK[0], block, Units())
        for cb in list(imu._sample_cbs):
            cb(s)
        rec.stop()
    finally:
        imu.close()
        fake.close()
    assert [r.kind for r in DatasetReader(path)] == ["bno055"]


def test_tmf8829_records(tmp_path: Path):
    """Contract 09 `tmf8829`: the builder's record value at the measurement's
    host time."""
    from depz_sensor_sdk.tmf8829 import Tmf8829
    from depz_sensor_sdk.tmf8829.builder import record_value

    from test_tmf8829_builder import MEAS, stream

    _, (m, *_) = stream(next(iter(MEAS)))
    fake = FakeSr04()
    dev = Tmf8829(fake.link, timeout=1.0)
    path = tmp_path / "tmf.depzdata"
    try:
        rec = SessionRecorder(path, temperature_hz=None)
        did = rec.add(dev)
        rec.start()
        for cb in list(dev._meas_cbs):
            cb(m)
        rec.stop()
    finally:
        dev.close()
        fake.close()
    reader = DatasetReader(path)
    offset = reader.devices[did]["time_sync"]["offset_us"]
    (r,) = list(reader)
    assert (r.kind, r.t_host_us) == ("tmf8829", m.timestamp_us - offset)
    assert r.value == record_value(m)


def test_reader_rejects_a_foreign_file(tmp_path: Path):
    other = tmp_path / "x.depzdata"
    other.write_text(json.dumps({"schema": "depz.rec/1"}) + "\n", encoding="utf-8", newline="\n")
    with pytest.raises(ValueError, match="not a depz.dataset"):
        DatasetReader(other)


def test_reader_refuses_a_line_nested_100000_deep(tmp_path: Path):
    # contract 08 «Line rules»: a 200 KB line of `[[[[…` is a damaged line
    # naming its number, not a RecursionError out of the JSON parser
    path = tmp_path / "deep.depzdata"
    deep = "[" * 100_000 + "]" * 100_000
    path.write_text(json.dumps({"schema": "depz.dataset/2"}) + '\n{"d": "d0", "t": 1, "k": "x", "v": {"a": '
                    + deep + "}}\n", encoding="utf-8", newline="\n")
    with pytest.raises(ValueError, match=r":2: .*nested deeper than 256"):
        DatasetReader(path).records


def test_play_from_an_offset_and_stop_during_a_wait(tmp_path: Path):
    path = tmp_path / "s.depzdata"
    w = DatasetWriter(path, {"d0": {}})
    for t in (100, 200, 300, 5_000_000):  # the last one 4.7 s after the third
        w.write("d0", t, "event", {})
    w.close()
    reader = DatasetReader(path)

    got: list[int] = []
    reader.play(lambda r: got.append(r.t_host_us), speed=0, start_t_us=200)
    assert got == [200, 300, 5_000_000]

    # stop raised while play() waits for the 4.7 s gap ends it at once
    stop = threading.Event()
    got.clear()
    threading.Timer(0.05, stop.set).start()
    t0 = time.monotonic()
    reader.play(lambda r: got.append(r.t_host_us), speed=1.0, stop=stop)
    assert got == [100, 200, 300]
    assert time.monotonic() - t0 < 1.0
