"""Multi-sensor scenarios, symmetric across the four sensor types (SR04,
VL53L8CX, VL53L8CH, BNO086) and including repeated same-type: sync_time_all
across a mixed fleet (CX and CH simultaneously, plus two-of-a-kind), a
SessionRecorder with two SR04 of the same type on one host timeline, and
discovery ordering/selection when two SR04 serials are present."""

from __future__ import annotations

import struct
import threading
import time
from pathlib import Path
from types import SimpleNamespace

import pytest

from depz_sensor_sdk import Bno086, Sr04, sync_time_all
from depz_sensor_sdk.dataset import DatasetReader, SessionRecorder
from depz_sensor_sdk.errors import DeviceLostError, LinkClosedError
from depz_sensor_sdk.transport import CrcType, Packet, PacketParser, build_packet
from depz_sensor_sdk.transport.link import LoopbackLink
from depz_sensor_sdk.vl53l8 import Vl53l8Ch, Vl53l8Cx

from fake_device import FakeSr04


# ── a minimal sync-only device-side responder ─────────────────────────────────


class FakeSyncBridge:
    """Answers just enough of the wire protocol (identity + SYNC_TIME) for any
    DeviceBase subclass to run sync_time()/sync_time_all() without hardware, so
    the four device classes can share one timeline in-process."""

    def __init__(self, mcu_time_us: int = 1_000_000):
        self.link, self._side = LoopbackLink.pair()
        self.mcu_time_us = mcu_time_us
        self._tx_seq = 0
        self._parser = PacketParser()
        self._thread = threading.Thread(target=self._run, daemon=True)
        self._thread.start()

    def close(self):
        self.link.close()

    def _send(self, cmd: int, payload: bytes = b"") -> None:
        try:
            self._side.write(build_packet(cmd, payload, self._tx_seq, CrcType.NONE))
        except LinkClosedError:
            return
        self._tx_seq = (self._tx_seq + 1) & 0xFF

    def _run(self) -> None:
        while True:
            try:
                data = self._side.read(timeout=1.0)
            except (LinkClosedError, DeviceLostError):
                return
            for ev in self._parser.feed(data):
                if isinstance(ev, Packet):
                    self._handle(ev)

    def _handle(self, pkt: Packet) -> None:
        if pkt.cmd == 0x06:  # SYNC_TIME
            (t1,) = struct.unpack("<Q", pkt.payload)
            t2 = self.mcu_time_us + 500
            t3 = t2 + 20
            self._send(0x82, struct.pack("<QQQ", t1, t2, t3))
        else:
            self._send(0x80, bytes((pkt.cmd, 0x02)))  # ERR_INVALID_CMD


# ── sync_time_all across the four sensor types (CX and CH simultaneously) ──────


def test_sync_time_all_four_sensor_types():
    # one of each: SR04, VL53L8CX, VL53L8CH, BNO086 — distinct device clocks.
    specs = [
        (Sr04, 1_000_000),
        (Vl53l8Cx, 2_000_000),
        (Vl53l8Ch, 3_000_000),
        (Bno086, 4_000_000),
    ]
    bridges = [FakeSyncBridge(mcu) for _cls, mcu in specs]
    devs = [cls(b.link, timeout=1.0) for (cls, _mcu), b in zip(specs, bridges)]
    try:
        result = sync_time_all(devs, samples=3)
        assert set(result) == set(devs)
        # all four distinct classes are present on the shared timeline
        assert {type(d) for d in devs} == {Sr04, Vl53l8Cx, Vl53l8Ch, Bno086}
        for dev in devs:
            ts = result[dev]
            assert ts.rtt_us >= 0
            assert dev.time_sync is ts
            # each device now maps its own clock onto the shared host timeline
            assert dev.to_host_time_us(4242 + ts.offset_us) == 4242
    finally:
        for d in devs:
            d.close()
        for b in bridges:
            b.close()


def test_sync_time_all_repeated_tof_variants():
    # two-of-a-kind (two VL53L8CX) together with a VL53L8CH: repeats and the
    # CX/CH pair share one sync_time_all timeline.
    specs = [
        (Vl53l8Cx, 1_500_000),
        (Vl53l8Cx, 900_000_000),  # very different clock, same model
        (Vl53l8Ch, 42_000_000),
    ]
    bridges = [FakeSyncBridge(mcu) for _cls, mcu in specs]
    devs = [cls(b.link, timeout=1.0) for (cls, _mcu), b in zip(specs, bridges)]
    try:
        result = sync_time_all(devs, samples=3)
        assert set(result) == set(devs)
        assert sum(isinstance(d, Vl53l8Cx) for d in devs) == 3  # CH is a CX subclass
        assert sum(type(d) is Vl53l8Cx for d in devs) == 2  # two plain CX
        assert sum(type(d) is Vl53l8Ch for d in devs) == 1
        for dev in devs:
            ts = result[dev]
            assert dev.to_host_time_us(777 + ts.offset_us) == 777
    finally:
        for d in devs:
            d.close()
        for b in bridges:
            b.close()


# ── two-of-a-kind in a SessionRecorder on one host timeline ───────────────────


def test_recorder_two_same_type_devices(tmp_path: Path):
    fake_a = FakeSr04(); fake_a.mcu_time_us = 2_000_000
    fake_b = FakeSr04(); fake_b.mcu_time_us = 500_000_000  # very different clock
    dev_a = Sr04(fake_a.link, timeout=1.0)
    dev_b = Sr04(fake_b.link, timeout=1.0)
    path = tmp_path / "two_sr04.depzdata"
    try:
        rec = SessionRecorder(path, note="two-of-a-kind")
        id_a = rec.add(dev_a, device_id="left")
        id_b = rec.add(dev_b, device_id="right")
        assert (id_a, id_b) == ("left", "right")
        rec.start()
        for d in (dev_a, dev_b):
            d.start()
        # interleave from both same-type devices
        fake_a.send_measurement(0x37); time.sleep(0.02)
        fake_b.send_measurement(0x37); time.sleep(0.02)
        fake_a.send_measurement(0x37); time.sleep(0.02)
        fake_b.send_measurement(0x37); time.sleep(0.1)
        rec.stop()
        assert rec.records_written == 0  # writer cleared on stop
    finally:
        for d in (dev_a, dev_b):
            d.stop(); d.close()
        fake_a.close(); fake_b.close()

    reader = DatasetReader(path)
    assert set(reader.devices) == {"left", "right"}
    for meta in reader.devices.values():
        assert meta["sensor_type"] == "sr04"  # both identified as SR04
        assert meta["serial"] == FakeSr04.SERIAL

    records = list(reader)
    assert len(records) == 4
    ts = [r.t_host_us for r in records]
    assert ts == sorted(ts)  # merged strictly by shared host time
    # despite wildly different device clocks, both same-type streams interleave
    assert {r.device_id for r in records} == {"left", "right"}
    assert [r.device_id for r in records] == ["left", "right", "left", "right"]


# ── discovery with duplicate model types (two SR04 serials) ───────────────────


def _port(device, vid, pid, serial):
    return SimpleNamespace(device=device, vid=vid, pid=pid, serial_number=serial)


@pytest.fixture()
def two_sr04_ports(monkeypatch):
    from depz_sensor_sdk import discovery
    from depz_sensor_sdk.usb_ids import DEPZ_USB_VID, PID_SR04

    ports = [
        _port("/dev/ttyACM3", DEPZ_USB_VID, PID_SR04, "SN000042"),
        _port("/dev/ttyACM1", DEPZ_USB_VID, PID_SR04, "SN000007"),
    ]
    monkeypatch.setattr(discovery.list_ports, "comports", lambda: list(ports))
    opened: list[str] = []

    def fake_open_probed(port, timeout):
        opened.append(port)
        return SimpleNamespace(port=port)

    monkeypatch.setattr(discovery, "_open_probed", fake_open_probed)
    return discovery, opened


def test_duplicate_sr04_ordered_by_serial(two_sr04_ports):
    discovery, _ = two_sr04_ports
    # default picks the alphabetically-smallest serial (SN000007)
    assert discovery.open_device().port == "/dev/ttyACM1"


def test_duplicate_sr04_index_selection(two_sr04_ports):
    discovery, _ = two_sr04_ports
    assert discovery.open_device(0).port == "/dev/ttyACM1"  # SN...07 first
    assert discovery.open_device(1).port == "/dev/ttyACM3"  # SN...42 second


def test_duplicate_sr04_serial_selection(two_sr04_ports):
    discovery, _ = two_sr04_ports
    # the two are the same model; only the USB serial tells them apart
    assert discovery.open_device(serial="SN000042").port == "/dev/ttyACM3"
    assert discovery.open_device(serial="SN000007").port == "/dev/ttyACM1"


def test_duplicate_sr04_index_out_of_range(two_sr04_ports):
    from depz_sensor_sdk.errors import NoDepzDeviceError

    discovery, _ = two_sr04_ports
    with pytest.raises(NoDepzDeviceError):
        discovery.open_device(2)  # only two candidates
