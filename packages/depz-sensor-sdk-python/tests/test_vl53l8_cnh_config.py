"""CnhConfig (VL53L8CH compact network histogram, contract 04 §7): the
argument checks (refused, never wrapped — contract 07 §3), the on-device
buffer sizing for every cnh_cfg flag combination, the distance axis, and the
block decoder's ping / pong selection against the fake's synthetic block."""

from __future__ import annotations

import pytest

from depz_sensor_sdk.vl53l8 import CnhConfig, cnh

from fake_vl53l8 import cnh_block, expected_cnh


def _cfg(resolution=64, start_bin=10, num_bins=20, sub_sample=2, agg=(0, 0, 2, 2, 4, 4)):
    cfg = CnhConfig()
    cfg.init_config(start_bin, num_bins, sub_sample)
    cfg.create_agg_map(resolution, *agg)
    return cfg


def test_init_config_sets_the_example12_flags():
    cfg = CnhConfig()
    cfg.init_config(start_bin=10, num_bins=20, sub_sample=2)
    assert cfg.ref_bin_offset == 10 * 2048
    assert cfg.feature_length == 20 and cfg.sum_span == 2
    assert cfg.cnh_cfg == 0x3F and cfg.cnh_flex_shift == 1
    assert len(cfg.pack()) == 156


@pytest.mark.parametrize("kwargs", [
    {"start_bin": 1048576}, {"start_bin": -1048577}, {"start_bin": 1.5},
    {"num_bins": 256}, {"num_bins": -1}, {"sub_sample": 256}, {"sub_sample": "2"},
])
def test_init_config_refuses_out_of_range_fields(kwargs):
    args = {"start_bin": 10, "num_bins": 20, "sub_sample": 2, **kwargs}
    with pytest.raises(cnh.CnhConfigError):
        CnhConfig().init_config(**args)


def test_agg_map_8x8_merged_2x2():
    cfg = _cfg()
    assert cfg.nb_of_aggregates == 16
    # zone (row, col) → aggregate (row // 2) * 4 + col // 2
    assert cfg.map_id[:8] == [0, 0, 1, 1, 2, 2, 3, 3]
    assert cfg.map_id[63] == 15


def test_agg_map_4x4_sub_window_leaves_other_zones_unmapped():
    cfg = _cfg(resolution=16, agg=(1, 1, 1, 1, 2, 2))
    assert cfg.nb_of_aggregates == 4
    assert cfg.map_id[5:7] == [0, 1] and cfg.map_id[9:11] == [2, 3]
    assert cfg.map_id[0] == -1 and cfg.map_id[15] == -1 and cfg.map_id[16:] == [-1] * 48


@pytest.mark.parametrize("resolution, agg", [
    (32, (0, 0, 1, 1, 4, 4)),  # not a zone grid
    (16, (0, 0, 1, 1, 5, 4)),  # 5 columns on a 4-wide grid
    (64, (1, 0, 2, 2, 4, 4)),  # 1 + 4·2 > 8
    (64, (0, 0, 0, 1, 4, 4)),  # merge_x < 1
    (64, (-1, 0, 1, 1, 4, 4)),  # start_x < 0
    (64, (0, 0, 1, 1, 4.0, 4)),  # not an integer
])
def test_agg_map_refuses_maps_off_the_grid(resolution, agg):
    with pytest.raises(cnh.CnhConfigError):
        _cfg(resolution=resolution, agg=agg)


def test_required_memory_matches_the_lab_capture_and_its_limits():
    assert _cfg().required_memory() == 1708  # 16 aggregates × 20 bins (vl53l8ch_cnh_8x8_15hz)
    blank = CnhConfig()
    blank.init_config(10, 20, 2)
    with pytest.raises(cnh.CnhConfigError, match="agg map"):
        blank.required_memory()
    big = _cfg(num_bins=100, agg=(0, 0, 1, 1, 8, 8))  # 64 × 100 bins
    with pytest.raises(cnh.CnhConfigError, match="reduce aggregates or bins"):
        big.required_memory()


def test_max_bins_is_the_largest_fitting_config():
    for nb_agg in (1, 16, 64):
        feat = cnh.max_bins(nb_agg)
        assert cnh._calc_required_memory(cnh._DEFAULT_CNH_CFG, nb_agg, feat) <= cnh.CNH_MAX_DATA_BYTES
        assert cnh._calc_required_memory(cnh._DEFAULT_CNH_CFG, nb_agg, feat + 1) > cnh.CNH_MAX_DATA_BYTES
    assert cnh.max_bins(16) >= 20  # the lab config fits
    cfg = _cfg(num_bins=cnh.max_bins(16))
    assert cfg.required_memory() <= cnh.CNH_MAX_DATA_BYTES


def test_sizing_with_variance_and_ping_pong_enabled():
    """The decoder assumes both off; the sizing still follows the C plugin for
    the other flag combinations."""
    base = cnh._calc_required_memory(cnh.MI_SFE_DISABLE_PING_PONG | cnh.MI_SFE_DISABLE_VARIANCE, 16, 20)
    with_var = cnh._calc_required_memory(cnh.MI_SFE_DISABLE_PING_PONG, 16, 20)
    assert with_var == base + 16 * 20 * 4 + 16 * 20  # VARIANCE_INT + VARIANCE_FRAC (padded)
    pingpong = cnh._calc_required_memory(cnh.MI_SFE_DISABLE_VARIANCE, 16, 20)
    assert pingpong == 2 * (base - cnh.CNH_PER_HEADER_WORDS * 4) + cnh.CNH_PER_HEADER_WORDS * 4
    assert cnh.max_bins(16, 0) < cnh.max_bins(16)


def test_distance_axis():
    cfg = _cfg()
    assert cfg.bin_center_mm(0) == pytest.approx((10 + 1) * cnh.CNH_BIN_WIDTH_MM)
    assert cfg.bin_center_mm(19) == pytest.approx((10 + 38 + 1) * cnh.CNH_BIN_WIDTH_MM)
    lo, hi = cfg.min_max_distance_mm()
    assert lo == int(cfg.bin_center_mm(0) + 5 * cnh.CNH_BIN_WIDTH_MM)
    assert hi == int(cfg.bin_center_mm(19) - 5 * cnh.CNH_BIN_WIDTH_MM)
    assert lo < hi


@pytest.mark.parametrize("state", [cnh.MI_STATE_PING, 1])
def test_decode_selects_the_buffer_the_header_names(state):
    cfg = _cfg(num_bins=3, agg=(0, 0, 4, 4, 2, 2))  # 4 aggregates × 3 bins
    raw = cnh_block(4, 3, state=state)
    decoded = cnh.decode(cfg, raw)
    want = expected_cnh(4, 3)
    assert decoded["ref_residual"] == want["ref_residual"]
    assert decoded["aggregates"] == want["aggregates"]
    assert decoded["aggregates"][3]["hist_raw"] == [400, 401, 402]
    assert decoded["aggregates"][3]["hist"] == [200.0, 200.5, 201.0]
