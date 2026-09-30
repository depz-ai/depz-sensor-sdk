"""Full 1D-family stack over committed real-hardware captures (no device).

Recorded 2026-09-28 from live boards on APP_VL53L0_4_v0.24 by the Python SDK:
identify, device name (picks the product class), init(driver), configure
(budget, mode), stream N frames. Replay is causal and strict_tx, so the whole
ULD — boot polls, NVM reads, the histogram preset — must re-issue
byte-identical requests, and every decoded frame (distance, status, targets,
histogram bins) must match the sidecar. The histogram driver is stateful (A/B
frame pairs, phase-consistency history): the short-preset capture pins its
alternating slot-0 artefact frame for frame.
"""

import itertools
import json
from pathlib import Path

import pytest

from depz_sensor_sdk.device import DeviceBase
from depz_sensor_sdk.discovery import _identify, _promote
from depz_sensor_sdk.transport.record_replay import ReplayLink
from depz_sensor_sdk.vl53lx import Vl53lx

RECORDINGS = Path(__file__).resolve().parents[3] / "contracts" / "vectors" / "recordings"
_FAMILY = ("vl53l0x_", "vl53l1cx_", "vl53l1cb_", "vl53l3cx_", "vl53l4cx_", "vl53lx_l4cd_")
CAPTURES = sorted(
    p.name[: -len(".expected.json")]
    for p in RECORDINGS.glob("*.expected.json")
    if p.name.startswith(_FAMILY)
)


@pytest.mark.parametrize("stem", CAPTURES)
def test_vl53lx_full_stack_replay(stem):
    expected = json.loads((RECORDINGS / f"{stem}.expected.json").read_text())

    dev = DeviceBase(ReplayLink(RECORDINGS / f"{stem}.depzrec", strict_tx=True), timeout=2.0)
    dev = _promote(dev, _identify(dev))
    assert isinstance(dev, Vl53lx)
    assert type(dev).__name__ == expected["class"]
    try:
        assert dev.get_software_name() == expected["software_name"]
        dev.init(expected["driver"], product=expected.get("product_arg"))
        assert dev.product == expected["product"]
        dev.configure(budget_ms=expected["budget_ms"], mode=expected["mode"])
        assert list(dev.get_range_timing()) == expected["timing"]
        stream = dev.measurements(maxsize=len(expected["frames"]) + 8)
        dev.start_ranging()
        frames = list(itertools.islice(stream, len(expected["frames"])))
        dev.stop_ranging()
    finally:
        dev.close()

    assert dev.stream_parse_errors == 0
    assert len(frames) == len(expected["frames"])
    for got, want in zip(frames, expected["frames"]):
        assert got.timestamp_us == want["timestamp_us"]
        assert got.distance_mm == want["distance_mm"]
        assert got.status == want["status"]
        assert [[t.distance_mm, t.status] for t in got.targets] == want["targets"]
        if "bins" in want:
            b = got.bins
            assert list(b.bin_data[: b.number_of_bins]) == want["bins"]["bin_data"]
            assert b.vcsel_period == want["bins"]["vcsel_period"]
            assert b.result__stream_count == want["bins"]["stream_count"]
        else:
            assert got.bins is None
