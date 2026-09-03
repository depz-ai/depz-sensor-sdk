"""Full VL53L8 stack over the committed real-hardware capture (no device).

The fixture was recorded from a live VL53L8CX (SN000005): init (fw download),
8x8 @15 Hz, 45 frames. Replay is causal (rx gated on tx prefix), so the whole
ULD init sequence — polls included — must re-issue byte-identical requests.
This is simultaneously a protocol regression test (strict_tx) and a parser
E2E: every decoded frame must match the recorded sidecar exactly.
"""

import itertools
import json
from pathlib import Path

import pytest

from depz_sensor_sdk.device import DeviceBase
from depz_sensor_sdk.discovery import _identify, _promote
from depz_sensor_sdk.transport.record_replay import ReplayLink
from depz_sensor_sdk.vl53l8 import RESOLUTION_8X8, Vl53l8Cx

RECORDINGS = Path(__file__).resolve().parents[3] / "contracts" / "vectors" / "recordings"
FIXTURE = RECORDINGS / "vl53l8_8x8_15hz_3s.depzrec"
EXPECTED = RECORDINGS / "vl53l8_8x8_15hz_3s.expected.json"


@pytest.mark.slow  # ULD boot polling sleeps in real time (~15 s)
def test_vl53l8_full_stack_replay():
    expected = json.loads(EXPECTED.read_text())

    dev = DeviceBase(ReplayLink(FIXTURE, strict_tx=True), timeout=2.0)
    dev = _promote(dev, _identify(dev))
    assert isinstance(dev, Vl53l8Cx)
    try:
        assert dev.get_software_name() == expected["software_name"]
        dev.init()
        dev.set_resolution(RESOLUTION_8X8)
        dev.set_ranging_frequency_hz(15)
        # Subscribe BEFORE starting: replay serves the whole session
        # instantly, and the queue must be sized for all frames (drop-oldest
        # would otherwise eat some and islice() would wait forever).
        stream = dev.frames(maxsize=len(expected["frames"]) + 8)
        dev.start_ranging()
        frames = list(itertools.islice(stream, len(expected["frames"])))
        dev.stop_ranging()
    finally:
        dev.close()

    assert len(frames) == len(expected["frames"])
    for got, want in zip(frames, expected["frames"]):
        assert got.timestamp_us == want["timestamp_us"]
        assert got.resolution == want["resolution"]
        assert got.silicon_temp_degc == want["silicon_temp_degc"]
        assert got.distance_mm.tolist() == want["distance_mm"]
        assert got.target_status.tolist() == want["target_status"]
        assert got.nb_target_detected.tolist() == want["nb_target_detected"]
