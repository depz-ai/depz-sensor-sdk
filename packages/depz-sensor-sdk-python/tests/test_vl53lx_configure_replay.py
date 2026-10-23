"""1D-family configure() variations over the committed real-hardware captures.

The captures themselves (identify, init, configure, the streamed frames, the
`refused` histogram preset) are asserted by the contract 15 scenarios
(test_scenarios.py). These tests replay a capture up to init, strict_tx, and
then drive configure() in ways the capture never recorded.
"""

import json
import time
from pathlib import Path

import pytest

from depz_sensor_sdk.errors import DepzError, DepzTimeoutError
from depz_sensor_sdk.transport.record_replay import ReplayLink
from scenario_open import open_legacy

RECORDINGS = Path(__file__).resolve().parents[3] / "contracts" / "vectors" / "recordings"


def _replay_to_init(stem):
    expected = json.loads((RECORDINGS / f"{stem}.expected.json").read_text())
    # The captures hold the pre-§4.4 probe (their scenarios are `probe: legacy`).
    dev, _ = open_legacy(ReplayLink(RECORDINGS / f"{stem}.depzrec", strict_tx=True),
                         None, None, timeout=2.0)
    dev.get_software_name()
    dev.init(expected["driver"], product=expected.get("product_arg"))
    return dev, expected


def test_configure_signal_kcps_goes_on_after_the_reinit():
    """The re-init puts the signal threshold back to the blob's, so configure
    applies `signal_kcps` last. The capture has no such write: the driver call
    is recorded instead, after the re-init and timing replayed strictly."""
    dev, expected = _replay_to_init("vl53l1cx_uld_long_33ms")
    calls = []
    dev.driver.set_signal_threshold = calls.append
    try:
        dev.configure(budget_ms=expected["budget_ms"], mode=expected["mode"], signal_kcps=512)
        assert calls == [512]
        assert list(dev.get_range_timing()) == expected["timing"]
    finally:
        dev.close()


def test_configure_refuses_an_unsupported_group_before_the_reinit():
    """L0X has no signal threshold: refused with no traffic, so the capture's
    own configure still replays strictly afterwards."""
    dev, expected = _replay_to_init("vl53l0x_uld_long-range_33ms")
    try:
        with pytest.raises(DepzError, match="signal_thresh"):
            dev.configure(budget_ms=expected["budget_ms"], mode=expected["mode"], signal_kcps=512)
        dev.configure(budget_ms=expected["budget_ms"], mode=expected["mode"])
        assert list(dev.get_range_timing()) == expected["timing"]
    finally:
        dev.close()


def test_stream_dropped_counts_per_measurement_queue():
    """Each live measurements() queue reports its own drop-oldest count; a
    finished get_measurement() leaves none behind."""
    dev, expected = _replay_to_init("vl53l1cx_uld_long_33ms")
    try:
        assert dev.stream_dropped_counts == []
        dev.configure(budget_ms=expected["budget_ms"], mode=expected["mode"])
        dev.get_range_timing()
        small, big = dev.measurements(maxsize=2), dev.measurements(maxsize=4096)
        dev.start_ranging()
        deadline = time.monotonic() + 5.0
        while big._queue._q.qsize() < 20 and time.monotonic() < deadline:
            time.sleep(0.01)
        n = big._queue._q.qsize()
        assert n >= 20
        assert dev.stream_dropped_counts == [n - 2, 0]
        assert dev.stream_dropped_counts == [small.dropped_count, big.dropped_count]
        with pytest.raises(DepzTimeoutError):  # the capture has no more frames
            dev.get_measurement(timeout=0.1)
        assert len(dev.stream_dropped_counts) == 2
        small.close()
        assert dev.stream_dropped_counts == [0]
    finally:
        dev.close()


def test_identify_is_the_firmware_name_as_on_every_device():
    """identify() is DeviceBase's parsed firmware name; the product dict is
    identify_product() (it was identify() until 0.7.0)."""
    from depz_sensor_sdk.protocol.identity import SensorType

    dev, _ = open_legacy(ReplayLink(RECORDINGS / "vl53l1cx_uld_long_33ms.depzrec", strict_tx=True),
                         None, None, timeout=2.0)
    try:
        assert dev.identify().sensor_type is SensorType.VL53LX
    finally:
        dev.close()
