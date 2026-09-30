"""Full VL53L8CH stack over committed real-hardware captures (no device).

Recorded 2026-09-25 from the lab board TMNQ8E3PRR on APP_VL53L8_v0.92: init
(CH / VL53LMZ blob download over the SPI bridge), 8x8 @ 15 Hz, then the same
with a 16-aggregate × 20-bin CNH block. The CNH block (1708 B) rides inside
every streamed frame — no poll mode. Replay is causal and strict_tx: the
whole ULD sequence must re-issue byte-identical requests, and every decoded
frame (with the VL53LMZ footer at size-4) must match the sidecar.
"""

import itertools
import json
from pathlib import Path

import pytest

from depz_sensor_sdk.transport.record_replay import ReplayLink
from depz_sensor_sdk.vl53l8 import RESOLUTION_8X8, CnhConfig, Vl53l8ch, cnh

RECORDINGS = Path(__file__).resolve().parents[3] / "contracts" / "vectors" / "recordings"

CASES = [("vl53l8ch_8x8_15hz_3s", False), ("vl53l8ch_cnh_8x8_15hz", True)]


def _cnh_config() -> CnhConfig:
    cfg = CnhConfig()
    cfg.init_config(start_bin=10, num_bins=20, sub_sample=2)
    cfg.create_agg_map(RESOLUTION_8X8, 0, 0, 2, 2, 4, 4)
    return cfg


@pytest.mark.parametrize("stem,with_cnh", CASES, ids=[c[0] for c in CASES])
def test_vl53l8ch_full_stack_replay(stem, with_cnh):
    expected = json.loads((RECORDINGS / f"{stem}.expected.json").read_text())
    # Over a bare link there is no USB PID — the class is fixed by hand.
    dev = Vl53l8ch(ReplayLink(RECORDINGS / f"{stem}.depzrec", strict_tx=True), timeout=2.0)
    try:
        assert dev.get_software_name() == expected["software_name"]
        dev.init()
        dev.set_resolution(RESOLUTION_8X8)
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
        assert got.resolution == want["resolution"] == 64
        assert got.silicon_temp_degc == want["silicon_temp_degc"]
        assert got.distance_mm.tolist() == want["distance_mm"]
        assert got.target_status.tolist() == want["target_status"]
        assert got.nb_target_detected.tolist() == want["nb_target_detected"]
        if with_cnh:
            assert got.cnh_raw is not None and got.cnh_raw.hex() == want["cnh_raw"]
        else:
            assert got.cnh_raw is None
    if with_cnh:
        hist = cnh.decode(_cnh_config(), frames[-1].cnh_raw)
        assert len(hist) > 0
