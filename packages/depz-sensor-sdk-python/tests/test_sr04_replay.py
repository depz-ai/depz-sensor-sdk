"""SR04 full stack over a committed real-hardware capture (no device).

Recorded 2026-09-28 from the lab SR04 (UMPVOB2461, APP_usonic_SR04_v0.97) by
the Python SDK: identify, device name, set/get sample period (20 ms), echo
decay, three single shots, the loop for 20 samples, stop. Replay is causal and
strict_tx: every request must be re-issued byte for byte, and every decoded
measurement must match the sidecar. The C and C++ SDKs replay the same file.
"""

import itertools
import json
from pathlib import Path

from depz_sensor_sdk.device import DeviceBase
from depz_sensor_sdk.discovery import _identify, _promote
from depz_sensor_sdk.sr04 import Sr04
from depz_sensor_sdk.transport.record_replay import ReplayLink

RECORDINGS = Path(__file__).resolve().parents[3] / "contracts" / "vectors" / "recordings"


def _as_dict(m):
    return {"timestamp_us": m.timestamp_us, "echo_time_us": m.echo_time_us, "source": m.source}


def test_sr04_session_replay():
    expected = json.loads((RECORDINGS / "sr04_session.expected.json").read_text())
    dev = DeviceBase(ReplayLink(RECORDINGS / "sr04_session.depzrec", strict_tx=True), timeout=2.0)
    dev = _promote(dev, _identify(dev))
    assert isinstance(dev, Sr04)
    try:
        assert dev.get_software_name() == expected["software_name"]
        assert dev.get_device_name() == expected["device_name"]
        dev.set_sample_period_us(20_000)
        assert dev.get_sample_period_us() == expected["sample_period_us"]
        assert dev.get_echo_decay_us() == expected["echo_decay_us"]
        once = [dev.measure_once() for _ in range(3)]
        stream = dev.stream(maxsize=len(expected["loop"]) + 64)
        dev.start()
        loop = list(itertools.islice(stream, len(expected["loop"])))
        dev.stop()
    finally:
        dev.close()
    assert [_as_dict(m) for m in once] == expected["once"]
    assert [_as_dict(m) for m in loop] == expected["loop"]
