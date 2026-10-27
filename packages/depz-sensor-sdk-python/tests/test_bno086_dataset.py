"""Record a BNO086 session to a contract 09 dataset and play it back: every
report of the fake hub lands as a `bno086` record (bno086_record_value) on
the host timeline, and DatasetReader.play delivers the same records in order."""

from __future__ import annotations

import struct
import time
from pathlib import Path

from depz_sensor_sdk import Bno086
from depz_sensor_sdk.dataset import DatasetReader, SessionRecorder, bno086_record_value

from fake_bno086 import FakeBno086


def _report(rid: int, body: bytes, *, seq: int = 0, accuracy: int = 0, delay: int = 0) -> bytes:
    status = (accuracy & 0x03) | ((delay >> 8) << 2)
    return bytes((rid, seq, status, delay & 0xFF)) + body


def _timebase(delta_100us: int) -> bytes:
    return bytes([0xFB]) + struct.pack("<i", delta_100us)


def _wait(pred, timeout: float = 2.0) -> None:
    deadline = time.monotonic() + timeout
    while not pred() and time.monotonic() < deadline:
        time.sleep(0.005)
    assert pred()


def test_record_bno086_reports_then_play_back(tmp_path: Path):
    fake = FakeBno086()
    dev = Bno086(fake.link, timeout=1.0)
    path = tmp_path / "imu.depzdata"
    got: list = []
    try:
        rec = SessionRecorder(path, note="bno086 unit test", temperature_hz=None)
        did = rec.add(dev)
        rec.start()
        # Subscribed after the recorder: by the time this callback has seen
        # the report, the recorder's has written it.
        unsubscribe = dev.on_report(got.append)
        cargo = (
            _timebase(0)
            + _report(0x01, struct.pack("<3h", 0, 0, 2521), seq=1, accuracy=3)  # accelerometer 1 g
            + _report(0x1C, struct.pack("<H", 1), seq=2)  # stability detector: entered stable
        )
        fake.push_input_cargo(cargo)
        _wait(lambda: len(got) == 2)
        unsubscribe()
        rec.stop()
    finally:
        dev.close()
        fake.close()

    reader = DatasetReader(path)
    assert reader.devices[did]["sensor_type"] == "bno086"
    offset = reader.devices[did]["time_sync"]["offset_us"]
    recs = reader.records
    assert [(r.device_id, r.kind) for r in recs] == [(did, "bno086")] * 2
    assert [r.value for r in recs] == [bno086_record_value(r) for r in got]
    assert [r.t_host_us for r in recs] == [r.timestamp_us - offset for r in got]
    assert recs[1].value["type"] == "GenericEvent" and recs[1].value["value_raw"] == 1

    played: list = []
    reader.play(played.append, speed=0)  # as fast as possible
    assert played == recs
