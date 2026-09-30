"""BNO055 full stack over committed real-hardware captures (no device).

Recorded 2026-09-25 from a live board on APP_BNO055_v0.12 (sensor SW 03.11)
by the Python SDK: identify, device name, bridge info, sensor reset (with
the boot-settle poll), configure (units, mode, fusion-start poll), the
calibration profile read (a CONFIG round trip), system status, then a
timer stream. Replay is causal and strict_tx, so every poll loop must
re-issue byte-identical requests, and every streamed block must decode to
the sidecar's raw values.
"""

import itertools
import json
from dataclasses import asdict
from pathlib import Path

import pytest

from depz_sensor_sdk.bno055 import Bno055, CalibrationProfile, OprMode, Units
from depz_sensor_sdk.bno055.regs import FUSION_ACCEL_LSB, MAG_LSB, QUAT_LSB, decode_block
from depz_sensor_sdk.device import DeviceBase
from depz_sensor_sdk.discovery import _identify, _promote
from depz_sensor_sdk.transport.record_replay import ReplayLink

RECORDINGS = Path(__file__).resolve().parents[3] / "contracts" / "vectors" / "recordings"
CAPTURES = sorted(p.name[: -len(".expected.json")] for p in RECORDINGS.glob("bno055_*.expected.json"))


def test_captures_present():
    assert len(CAPTURES) >= 2


@pytest.mark.parametrize("stem", CAPTURES)
def test_bno055_full_stack_replay(stem):
    exp = json.loads((RECORDINGS / f"{stem}.expected.json").read_text())
    units = Units.unpack(exp["unit_sel"])
    block = tuple(exp["block"])
    n = len(exp["frames"])

    dev = DeviceBase(ReplayLink(RECORDINGS / f"{stem}.depzrec", strict_tx=True), timeout=2.0)
    dev = _promote(dev, _identify(dev))
    assert isinstance(dev, Bno055)
    try:
        assert dev.get_software_name() == exp["software_name"]
        assert dev.get_device_name() == exp["device_name"]
        info = dev.bridge_info()
        assert asdict(info) == exp["info"]
        assert info.ids_ok
        dev.reset_sensor()
        dev.configure(OprMode(exp["mode"]), units)
        profile = dev.read_calibration_profile()
        assert profile == CalibrationProfile.from_dict(exp["calibration_profile"])
        assert asdict(dev.system_status()) == exp["system_status"]
        stream = dev.samples(maxsize=n + 8)
        dev.start_stream(exp["period_ms"], block)
        frames = list(itertools.islice(stream, n))
        dev.stop_stream()
    finally:
        dev.close()

    assert dev.stream_parse_errors == 0
    assert len(frames) == n
    for got, want in zip(frames, exp["frames"]):
        assert got.timestamp_us == want["timestamp_us"]
        assert got.addr == want["addr"] == block[0]
        assert got.raw.hex() == want["raw"]
        assert got.units == units
        raw = decode_block(got.addr, got.raw)
        assert {k: (list(v) if isinstance(v, tuple) else v) for k, v in asdict(raw).items()} == want[
            "decoded"
        ]
        # Scaling by the stream's units (contract 13 §4.2).
        assert raw.quaternion is not None and any(raw.quaternion), "fusion must be live from frame 0"
        assert got.quaternion == tuple(v / QUAT_LSB for v in raw.quaternion)
        if raw.accel is not None:
            assert got.accel == tuple(v / units.accel_lsb for v in raw.accel)
            assert got.mag == tuple(v / MAG_LSB for v in raw.mag)
            assert got.gyro == tuple(v / units.gyro_lsb for v in raw.gyro)
            assert got.euler == tuple(v / units.euler_lsb for v in raw.euler)
            assert got.gravity == tuple(v / FUSION_ACCEL_LSB for v in raw.gravity)
            assert got.temperature == raw.temperature / units.temp_lsb
            assert got.calibration is not None
        else:
            assert got.accel is None and got.calibration is None

    # Streamed at the requested period on the MCU clock.
    ts = [f.timestamp_us for f in frames]
    assert all(abs((b - a) - 1000 * exp["period_ms"]) < 500 for a, b in zip(ts, ts[1:]))
