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
