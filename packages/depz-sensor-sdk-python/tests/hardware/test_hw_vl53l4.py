"""VL53L4CD real-hardware smoke tests (contract 10)."""

import pytest

from _support import port_lease
from depz_sensor_sdk import Vl53l4Cd

pytestmark = [pytest.mark.hardware, pytest.mark.hardware_vl53l4, pytest.mark.timeout(120)]


@pytest.fixture
def vl53l4(vl53l4_info):
    with port_lease(vl53l4_info.resolve_port()):
        dev = Vl53l4Cd(vl53l4_info.resolve_port())
        try:
            yield dev
        finally:
            try:
                dev.stop_ranging()
            except Exception:
                pass
            dev.close()


def test_bridge_identity_and_init(vl53l4):
    info = vl53l4.bridge_info()
    assert info.model_id == 0xEBAA
    assert info.fw_status == 3
    assert vl53l4.is_alive()
    vl53l4.init()
    assert vl53l4.initialized
    timing = vl53l4.get_range_timing()
    assert 10 <= timing.timing_budget_ms <= 200


def test_streams_sane_measurements(vl53l4):
    vl53l4.init()
    vl53l4.set_range_timing(50, 0)
    try:
        vl53l4.start_ranging()
        sample = vl53l4.get_measurement(timeout=3.0)
        assert sample.timestamp_us > 0
        assert 0 <= sample.distance_mm <= 8190
        assert 0 <= sample.stream_count <= 255
    finally:
        vl53l4.stop_ranging()


def test_reset_clears_host_initialization_state(vl53l4):
    vl53l4.init()
    vl53l4.reset_sensor()
    assert not vl53l4.initialized
    assert vl53l4.is_alive()
