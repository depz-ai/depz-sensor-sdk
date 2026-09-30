"""Full VL53L5CX / VL53L7CH stack over committed real-hardware captures (no device).

Recorded 2026-09-24 from live boards on APP_VL53L7_v0.53 (L5CX TS5J50RYCG,
L7CH TXK5KAX6X4): identify, init (L5/L7 boot branch + fw download over the
I2C bridge), 8x8 (and one 4x4) @15 Hz. The CNH capture adds a 16-aggregate × 20-bin
histogram block, so every frame (3156 B) arrives in chunked RPT_VL53_FRAMEs.
Replay is causal and strict_tx: the whole ULD sequence must re-issue
byte-identical requests, and every decoded frame must match the sidecar.
"""

import itertools
import json
from pathlib import Path

import pytest

from depz_sensor_sdk.device import DeviceBase
from depz_sensor_sdk.discovery import _identify, _promote
from depz_sensor_sdk.transport.record_replay import ReplayLink
from depz_sensor_sdk.vl53l7 import RESOLUTION_4X4, RESOLUTION_8X8, Vl53l5cx, Vl53l7ch
from depz_sensor_sdk.vl53l8 import CnhConfig

RECORDINGS = Path(__file__).resolve().parents[3] / "contracts" / "vectors" / "recordings"

CASES = [
    ("vl53l5cx_8x8_15hz_3s", Vl53l5cx, RESOLUTION_8X8, False),
    # 4x4 pins the L5/L7 output-list rule: per-target blocks stay 64 entries
    # on the wire and the parser trims them to the 16 real zones.
    ("vl53l5cx_4x4_15hz", Vl53l5cx, RESOLUTION_4X4, False),
    ("vl53l7ch_8x8_15hz_3s", Vl53l7ch, RESOLUTION_8X8, False),
    ("vl53l7ch_cnh_8x8_15hz", Vl53l7ch, RESOLUTION_8X8, True),
]


def _cnh_config() -> CnhConfig:
    cfg = CnhConfig()
    cfg.init_config(start_bin=10, num_bins=20, sub_sample=2)
    cfg.create_agg_map(RESOLUTION_8X8, 0, 0, 2, 2, 4, 4)
    return cfg


@pytest.mark.parametrize("stem,cls,resolution,with_cnh", CASES, ids=[c[0] for c in CASES])
def test_vl53l7_full_stack_replay(stem, cls, resolution, with_cnh):
    expected = json.loads((RECORDINGS / f"{stem}.expected.json").read_text())

    dev = DeviceBase(ReplayLink(RECORDINGS / f"{stem}.depzrec", strict_tx=True), timeout=2.0)
    dev = _promote(dev, _identify(dev))
    # The class comes from the recorded device name: the firmware name alone
    # (APP_VL53L7_*) cannot tell L5CX / L7CX / L7CH apart.
    assert type(dev) is cls
    try:
        assert dev.get_software_name() == expected["software_name"]
        dev.init()
        assert dev.module_type == expected["module_type"]
        dev.set_resolution(resolution)
        dev.set_ranging_frequency_hz(15)
        if with_cnh:
            dev.configure_cnh(_cnh_config())
        stream = dev.frames(maxsize=len(expected["frames"]) + 8)
        dev.start_ranging()
        frames = list(itertools.islice(stream, len(expected["frames"])))
        dev.stop_ranging()
    finally:
        dev.close()

    assert dev.frame_parse_errors == 0
    assert len(frames) == len(expected["frames"])
    for got, want in zip(frames, expected["frames"]):
        assert got.timestamp_us == want["timestamp_us"]
        assert got.resolution == want["resolution"] == resolution
        assert got.silicon_temp_degc == want["silicon_temp_degc"]
        assert got.distance_mm.tolist() == want["distance_mm"]
        assert got.target_status.tolist() == want["target_status"]
        assert got.nb_target_detected.tolist() == want["nb_target_detected"]
        if with_cnh:
            assert got.cnh_raw is not None and got.cnh_raw.hex() == want["cnh_raw"]
        else:
            assert got.cnh_raw is None
