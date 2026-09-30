"""VL53L4CD full stack over a committed real-hardware capture (no device).

Recorded 2026-09-28 from the lab VL53L4CD (TL7TKSLW8Z, APP_VL53L4_v0.83) by the
Python SDK: identify, device name, bridge info, init (config block at 400 kHz,
VHV poll, 1 MHz), range timing 33 ms, read-backs, two single shots, 20
streamed frames, stop. Replay is causal and strict_tx, so every poll loop of
the ULD must re-issue byte-identical requests. TS, C and C++ replay it too.
"""

import itertools
import json
from pathlib import Path

from depz_sensor_sdk.device import DeviceBase
from depz_sensor_sdk.discovery import _identify, _promote
from depz_sensor_sdk.transport.record_replay import ReplayLink
from depz_sensor_sdk.vl53l4 import Vl53l4cd

RECORDINGS = Path(__file__).resolve().parents[3] / "contracts" / "vectors" / "recordings"
FIELDS = ("timestamp_us", "range_status", "distance_mm", "sigma_mm", "signal_rate_kcps",
          "ambient_rate_kcps", "signal_per_spad_kcps", "ambient_per_spad_kcps",
          "number_of_spad", "stream_count")


def _as_dict(m):
    return {k: getattr(m, k) for k in FIELDS}


def test_vl53l4cd_session_replay():
    exp = json.loads((RECORDINGS / "vl53l4cd_session.expected.json").read_text())
    dev = DeviceBase(ReplayLink(RECORDINGS / "vl53l4cd_session.depzrec", strict_tx=True), timeout=2.0)
    dev = _promote(dev, _identify(dev))
    assert isinstance(dev, Vl53l4cd)
    try:
        assert dev.get_software_name() == exp["software_name"]
        assert dev.get_device_name() == exp["device_name"]
        info = dev.bridge_info()
        assert (info.model_id, info.fw_status, info.i2c_khz) == (
            exp["info"]["model_id"], exp["info"]["fw_status"], exp["info"]["i2c_khz"])
        dev.init()
        dev.set_range_timing(33, 0)
        assert list(dev.get_range_timing()) == exp["timing"]
        assert dev.get_offset_mm() == exp["offset_mm"]
        assert dev.get_xtalk_kcps() == exp["xtalk_kcps"]
        once = [dev.measure_once() for _ in range(len(exp["once"]))]
        stream = dev.measurements(maxsize=len(exp["frames"]) + 64)
        dev.start_ranging()
        frames = list(itertools.islice(stream, len(exp["frames"])))
        dev.stop_ranging()
    finally:
        dev.close()
    assert [_as_dict(m) for m in once] == exp["once"]
    assert [_as_dict(m) for m in frames] == exp["frames"]
    assert dev.stream_parse_errors == 0
