"""BNO085 full stack over a committed real-hardware capture (no device).

Recorded 2026-09-29 from the lab BNO085 (I5MFL1ONMCD, APP_BNO086_v0.99, SH-2
3.2.13) by the Python SDK: identify, device name, hardware reset, product id,
rotation vector 100 Hz + accelerometer 50 Hz + gyro-integrated RV 100 Hz (each
with its Get Feature read-back), 150 mixed reports, the three disables, ME
calibration, oscillator type, the rotation vector's metadata record, counts,
errors. Replay is causal and strict_tx, so every SH-2 request — SHTP sequence
numbers and command sequence numbers included — must be byte-identical. TS, C
and C++ replay it too.
"""

import itertools
import json
from dataclasses import asdict
from pathlib import Path

from depz_sensor_sdk.bno086 import Bno086, SensorId
from depz_sensor_sdk.device import DeviceBase
from depz_sensor_sdk.discovery import _identify, _promote
from depz_sensor_sdk.transport.record_replay import ReplayLink

RECORDINGS = Path(__file__).resolve().parents[3] / "contracts" / "vectors" / "recordings"


def test_bno086_session_replay():
    exp = json.loads((RECORDINGS / "bno086_session.expected.json").read_text())
    dev = DeviceBase(ReplayLink(RECORDINGS / "bno086_session.depzrec", strict_tx=True), timeout=2.0)
    dev = _promote(dev, _identify(dev))
    assert isinstance(dev, Bno086)
    try:
        assert dev.get_software_name() == exp["software_name"]
        assert dev.get_device_name() == exp["device_name"]
        dev.hardware_reset()
        assert asdict(dev.product_id()) == exp["product_id"]
        stream = dev.reports(maxsize=len(exp["reports"]) + 512)
        features = [asdict(dev.enable(sid, hz)) for sid, hz in exp["enable"]]
        reports = list(itertools.islice(stream, len(exp["reports"])))
        for sid, _ in exp["enable"]:
            dev.disable(sid)
        calibration = asdict(dev.get_calibration())
        oscillator = int(dev.get_oscillator_type())
        metadata = dev.get_metadata(SensorId.ROTATION_VECTOR)
        counts = asdict(dev.get_counts(SensorId.ROTATION_VECTOR))
        errors = [asdict(e) for e in dev.get_errors()]
    finally:
        dev.close()
    assert features == exp["features"]
    assert [dict(type=type(r).__name__, **asdict(r)) for r in reports] == exp["reports"]
    assert calibration == exp["calibration"]
    assert oscillator == exp["oscillator"]
    assert list(metadata.raw_words) == exp["metadata_rv_words"]
    # Word 3: revision in the high half, supply current (Q10 mA) in the low.
    assert metadata.revision == 4
    assert 5.0 < metadata.power_ma_q10 / 1024 < 5.5
    assert counts == exp["counts_rv"]
    assert errors == exp["errors"]
