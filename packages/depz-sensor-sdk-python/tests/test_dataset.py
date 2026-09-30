"""Dataset recording/playback (contract 09): multi-device, time-synced."""

import json
import threading
import time
from pathlib import Path

from depz_sensor_sdk import Sr04
from depz_sensor_sdk.dataset import DatasetReader, SessionRecorder

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
    hdr = json.loads(path.read_text().splitlines()[0])
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
