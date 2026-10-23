"""A multi-fragment BNO086 input cargo is timestamped from its FIRST fragment.

The 0xFB base delta refers to the capture of the transfer that started the
cargo (contract 05 §3, §5); a continuation arriving later must not shift the
reports. The bench bridge never fragments input cargos (2026-10-21), so this
pins the rule with a synthetic three-fragment cargo through the device's RX
path.
"""

import struct

from depz_sensor_sdk.bno086 import Bno086
from depz_sensor_sdk.bno086.reports import SensorId
from depz_sensor_sdk.bno086.shtp import ShtpChannel, fragment_cargo
from depz_sensor_sdk.protocol.bno086 import Bno086Rpt
from depz_sensor_sdk.transport.framing import Packet

from fake_device import FakeSr04


def test_multi_fragment_cargo_uses_first_fragment_capture():
    # 0xFB base delta 1.0 ms, then 12 accelerometer reports, delay 0..11 × 100 µs.
    cargo = bytes([0xFB]) + struct.pack("<i", 10)
    for i in range(12):
        cargo += bytes((SensorId.ACCELEROMETER, i, 0, i)) + struct.pack("<hhh", 0, 0, 2511)
    frames = fragment_cargo(ShtpChannel.INPUT_NORMAL, cargo, 0, max_frame=64)
    assert len(frames) == 3
    captures = [7_000_000, 7_002_000, 7_004_000]

    fake = FakeSr04()
    imu = Bno086(fake.link, timeout=1.0)
    got = []
    imu.on_report(got.append)
    try:
        for frame, cap in zip(frames, captures):
            imu._handle_report(Packet(Bno086Rpt.DATA, 0, bytes(1) + struct.pack("<Q", cap) + frame))
    finally:
        imu.close()
        fake.close()

    assert [r.timestamp_us for r in got] == [captures[0] - 1000 + 100 * i for i in range(12)]
