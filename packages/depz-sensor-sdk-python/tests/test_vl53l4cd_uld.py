"""VL53L4CD ULD (vl53l4/uld.py) over the fake register platform: the paths
test_vl53l4cd_config.py leaves out — boot and data-ready timeouts, interrupt
polarity, autonomous-mode start, the temperature update, offset and crosstalk
calibration (VL53L4CD_calibration.c), and the refusals of the pure register
codecs (contract 07 §3: out of range is refused, never wrapped)."""

from __future__ import annotations

import pytest

from depz_sensor_sdk.vl53l4 import uld as uld_mod
from depz_sensor_sdk.vl53l4.uld import (
    RESULT_BLOCK_ADDR,
    RESULT_BLOCK_LEN,
    VL53L4CD,
    Vl53l4cdError,
)

from fake_vl53l4 import CLOCK_PLL_TYPICAL, FakeVl53l4Platform, make_driver, result_block


def _starts(p: FakeVl53l4Platform) -> list[int]:
    return [d[0] for d in p.written_to(uld_mod.SYSTEM_START)]


# ── boot / data-ready ────────────────────────────────────────────────────────


def test_wait_boot_times_out_when_firmware_status_never_reaches_3():
    drv, p = make_driver()
    p.reg[uld_mod.FIRMWARE__SYSTEM_STATUS] = 0x01
    with pytest.raises(Vl53l4cdError, match="FIRMWARE__SYSTEM_STATUS"):
        drv.wait_boot(timeout_ms=3)
    # one poll per millisecond, then the refusal — nothing written
    assert p.writes == []


def test_sensor_init_fails_on_a_sensor_that_never_boots():
    drv, p = make_driver()
    p.reg[uld_mod.FIRMWARE__SYSTEM_STATUS] = 0x00
    with pytest.raises(Vl53l4cdError):
        drv.sensor_init()
    # the configuration block is never written to an unbooted sensor
    assert p.written_to(uld_mod.CONFIG_ADDR) == []


def test_data_ready_follows_the_interrupt_polarity_bit():
    drv, p = make_driver()
    # GPIO_HV_MUX__CTRL bit 4 set → INT active low → ready when TIO bit 0 == 0
    p.reg[uld_mod.GPIO_HV_MUX__CTRL] = 0x10
    p.reg[uld_mod.GPIO__TIO_HV_STATUS] = 0x00
    assert drv.check_for_data_ready()
    p.reg[uld_mod.GPIO__TIO_HV_STATUS] = 0x01
    assert not drv.check_for_data_ready()
    # bit 4 clear → INT active high → ready when TIO bit 0 == 1
    p.reg[uld_mod.GPIO_HV_MUX__CTRL] = 0x00
    assert drv.check_for_data_ready()
    p.reg[uld_mod.GPIO__TIO_HV_STATUS] = 0x00
    assert not drv.check_for_data_ready()


def test_wait_data_ready_times_out():
    drv, p = make_driver()
    p.reg[uld_mod.GPIO_HV_MUX__CTRL] = 0x10
    p.reg[uld_mod.GPIO__TIO_HV_STATUS] = 0x01  # never ready
    with pytest.raises(Vl53l4cdError, match="data ready"):
        drv.wait_data_ready(timeout_ms=5)
    with pytest.raises(Vl53l4cdError):
        drv.wait_data_ready(timeout_ms=0)  # still one poll, then the refusal


# ── ranging modes / results ──────────────────────────────────────────────────


def test_start_ranging_picks_continuous_or_autonomous_from_the_period():
    drv, p = make_driver()
    drv.sensor_init()
    drv.start_ranging()
    assert _starts(p)[-1] == 0x21  # continuous (INTERMEASUREMENT_MS == 0)
    drv.set_range_timing(50, 100)
    assert drv.rd_dword(uld_mod.INTERMEASUREMENT_MS) == int(
        1.055 * 100 * (CLOCK_PLL_TYPICAL & 0x3FF)
    )
    drv.start_ranging()
    assert _starts(p)[-1] == 0x40  # autonomous low power


def test_get_result_reads_the_block_once_and_decodes_it():
    drv, p = make_driver()
    p.seed_result_block(result_block(291, status=9, sigma_mm=3, signal_kcps=1200,
                                     spads=2048, stream_count=7, ambient_kcps=40))
    r = drv.get_result()
    assert (r.distance_mm, r.range_status, r.sigma_mm, r.stream_count) == (291, 0, 3, 7)
    assert (r.signal_rate_kcps, r.ambient_rate_kcps, r.number_of_spad) == (1200, 40, 8)
    assert (r.signal_per_spad_kcps, r.ambient_per_spad_kcps) == (150, 5)
    assert r.status_text == "valid"
    assert uld_mod.parse_result_block(result_block(1, status=4)).status_text == (
        "signal below threshold"
    )


def test_results_data_status_text_names_unknown_statuses():
    r = uld_mod.parse_result_block(result_block(1, status=0x1F))
    assert r.range_status == 0x1F  # past the LUT: passed through
    assert r.status_text == "unknown (31)"


def test_start_temperature_update_reruns_vhv_and_restores_the_loop_bound():
    drv, p = make_driver()
    drv.sensor_init()
    p.writes.clear()
    drv.start_temperature_update()
    assert p.written_to(uld_mod.VHV_CONFIG__TIMEOUT_MACROP_LOOP_BOUND) == [b"\x81", b"\x09"]
    assert p.written_to(0x000B) == [b"\x92", b"\x00"]
    assert _starts(p) == [0x21, 0x80]
    assert p.written_to(uld_mod.SYSTEM__INTERRUPT_CLEAR) == [b"\x01"]


# ── calibration (VL53L4CD_calibration.c) ─────────────────────────────────────


class _SequencedPlatform(FakeVl53l4Platform):
    """Answers each result-block read with the next distance of `distances`
    (the last one repeats)."""

    def __init__(self, distances: list[int], **block_kw):
        super().__init__()
        self._distances = list(distances)
        self._block_kw = block_kw
        self.block_reads = 0

    def rd_multi(self, addr: int, size: int) -> bytes:
        if addr == RESULT_BLOCK_ADDR and size == RESULT_BLOCK_LEN:
            self.block_reads += 1
            d = self._distances.pop(0) if len(self._distances) > 1 else self._distances[0]
            self.seed_result_block(result_block(d, **self._block_kw))
        return super().rd_multi(addr, size)


def _sequenced(distances: list[int], **block_kw) -> tuple[VL53L4CD, _SequencedPlatform]:
    p = _SequencedPlatform(distances, **block_kw)
    drv = VL53L4CD(p)
    drv.sensor_init()
    p.writes.clear()
    return drv, p


def test_calibrate_offset_is_target_minus_the_mean_after_a_heat_loop():
    # 10 heat-loop frames are discarded, then the 5 samples average 120 mm
    drv, p = _sequenced([999] * 10 + [100, 110, 120, 130, 140])
    assert drv.calibrate_offset(150, nb_samples=5) == 30
    assert p.block_reads == 15
    # offsets zeroed first, result programmed ×4 at the end
    assert p.written_to(uld_mod.RANGE_OFFSET_MM) == [b"\x00\x00", (30 * 4).to_bytes(2, "big")]
    assert p.written_to(uld_mod.INNER_OFFSET_MM) == [b"\x00\x00"]
    assert p.written_to(uld_mod.OUTER_OFFSET_MM) == [b"\x00\x00"]
    assert _starts(p) == [0x21, 0x80, 0x21, 0x80]  # heat loop, then the samples
    assert len(p.written_to(uld_mod.SYSTEM__INTERRUPT_CLEAR)) == 15
    assert drv.get_offset() == 30


def test_calibrate_offset_can_be_negative():
    drv, p = _sequenced([200])
    assert drv.calibrate_offset(150) == -50
    assert p.block_reads == 10 + 20
    assert drv.get_offset() == -50


def test_calibrate_offset_refuses_bad_arguments_before_any_traffic():
    drv, p = _sequenced([100])
    for target, nb in ((9, 20), (1001, 20), (100, 4), (100, 256)):
        with pytest.raises(ValueError, match="nb_samples must be 5..255"):
            drv.calibrate_offset(target, nb)
    assert p.writes == [] and p.block_reads == 0


def test_calibrate_xtalk_uses_the_valid_samples_after_the_first():
    # first sample frame (300 mm) is discarded; the rest average 100 mm at
    # 1200 kcps over 8 SPADs against a 200 mm target → (1 − 0.5) · 150 = 75
    drv, p = _sequenced([999] * 10 + [300, 100, 100, 100, 100], signal_kcps=1200, spads=2048)
    assert drv.calibrate_xtalk(200, nb_samples=5) == 75
    assert p.written_to(uld_mod.XTALK_PLANE_OFFSET_KCPS) == [
        b"\x00\x00",  # compensation disabled for the burst
        (75 * 512).to_bytes(2, "big"),
    ]
    assert _starts(p) == [0x21, 0x80, 0x21, 0x80]
    assert drv.get_xtalk() == 75


def test_calibrate_xtalk_skips_invalid_frames_and_rounds_half_away():
    # valid frames: 90 mm; the status-4 frames are dropped from the average
    # (1 − 90/200) · (1000/4) = 137.5 → too much for the 127 kcps ceiling…
    drv, p = _sequenced([999] * 10 + [90], signal_kcps=1000, spads=1024)
    with pytest.raises(Vl53l4cdError, match="> 127"):
        drv.calibrate_xtalk(200, nb_samples=5)
    # …while (1 − 90/200) · (200/4) = 27.5 rounds to 28 (C round(), not banker's)
    drv, p = _sequenced([999] * 10 + [90], signal_kcps=200, spads=1024)
    assert drv.calibrate_xtalk(200, nb_samples=5) == 28
    assert p.written_to(uld_mod.XTALK_PLANE_OFFSET_KCPS)[-1] == int(27.5 * 512).to_bytes(2, "big")


def test_calibrate_xtalk_fails_with_no_valid_sample():
    drv, p = _sequenced([100], status=4)  # every frame "signal below threshold"
    with pytest.raises(Vl53l4cdError, match="no valid samples"):
        drv.calibrate_xtalk(200, nb_samples=5)
    assert p.written_to(uld_mod.XTALK_PLANE_OFFSET_KCPS) == [b"\x00\x00"]
    assert _starts(p)[-1] == 0x80  # ranging stopped before the refusal


def test_calibrate_xtalk_refuses_bad_arguments_before_any_traffic():
    drv, p = _sequenced([100])
    for target, nb in ((9, 20), (5001, 20), (100, 4), (100, 256)):
        with pytest.raises(ValueError, match="target 10..5000"):
            drv.calibrate_xtalk(target, nb)
    assert p.writes == [] and p.block_reads == 0


def test_collect_times_out_when_data_ready_never_comes():
    drv, p = _sequenced([100])
    p.reg[uld_mod.GPIO__TIO_HV_STATUS] = 0x01  # INT active low, never asserted
    with pytest.raises(Vl53l4cdError, match="data ready"):
        drv._collect(3, lambda i, r: None, timeout_ms=2)
    assert _starts(p) == [0x21]


# ── pure codecs: refusals and the odd corners ────────────────────────────────


def test_range_timing_registers_refusals():
    with pytest.raises(Vl53l4cdError, match="osc_frequency reads 0"):
        uld_mod.range_timing_registers(50, 0, 0)
    # osc 1: the macro period truncates to 0 in u32 → refused, not divided by
    with pytest.raises(Vl53l4cdError, match="truncates to 0"):
        uld_mod.range_timing_registers(50, 0, 1)
    # autonomous period whose INTERMEASUREMENT_MS does not fit a u32
    with pytest.raises(ValueError, match="overflows u32"):
        uld_mod.range_timing_registers(50, 5_000_000, 0x3980, 0x3FF)
    with pytest.raises(ValueError, match="out of range"):
        uld_mod.range_timing_registers(50, 0, 0x3980, 0x10000)


def test_decode_range_timing_corners():
    with pytest.raises(Vl53l4cdError, match="osc_frequency reads 0"):
        uld_mod.decode_range_timing(0, CLOCK_PLL_TYPICAL, 0, 0x01CC)
    # a zero PLL word cannot be divided by: the period reads 0 ms, the budget
    # decodes as with any PLL word
    a, b, inter = uld_mod.range_timing_registers(50, 100, 0x3980, CLOCK_PLL_TYPICAL)
    budget, period = uld_mod.decode_range_timing(inter, CLOCK_PLL_TYPICAL, 0x3980, a)
    assert period == 100 - 1  # ST's 1.055 / 1.065 factor pair reads one ms short
    assert uld_mod.decode_range_timing(inter, 0, 0x3980, a) == (budget, 0)
    # MS byte above 4 leaves the budget unshifted (the ULD's `if (ms_byte < 12)`)
    budget_shifted, _ = uld_mod.decode_range_timing(0, 0, 0x3980, 0x01CC)
    budget_unshifted, _ = uld_mod.decode_range_timing(0, 0, 0x3980, 0x05CC)
    assert budget_unshifted > budget_shifted  # MS byte 1 → >> 3; MS byte 5 → no shift
    with pytest.raises(ValueError):
        uld_mod.decode_range_timing(-1, 0, 0x3980, 0x01CC)


def test_round_half_away_matches_c_round():
    assert [uld_mod.round_half_away(x) for x in (0.5, 1.5, 2.4, -0.5, -1.5, -2.4, 0.0)] == [
        1, 2, 2, -1, -2, -2, 0,
    ]


def test_codec_ranges_are_refused_not_wrapped():
    with pytest.raises(ValueError):
        uld_mod.offset_raw(1024)
    with pytest.raises(ValueError):
        uld_mod.offset_raw(-1025)
    with pytest.raises(ValueError):
        uld_mod.offset_raw(1.5)  # not an integer
    with pytest.raises(ValueError):
        uld_mod.xtalk_raw(128)
    with pytest.raises(ValueError):
        uld_mod.signal_threshold_raw(0x10000)
    with pytest.raises(ValueError):
        uld_mod.sigma_threshold_raw(-1)
    for dec in (uld_mod.decode_offset, uld_mod.decode_xtalk,
                uld_mod.decode_signal_threshold, uld_mod.decode_sigma_threshold):
        with pytest.raises(ValueError):
            dec(0x10000)
    assert uld_mod.offset_raw(-1024) == (-4096) & 0xFFFF
    assert uld_mod.decode_offset(uld_mod.offset_raw(-1023)) == -1023
    assert uld_mod.decode_offset(uld_mod.offset_raw(1023)) == 1023
    # GetOffset's 11-bit decode is ST's (`if (Temp > 1024) Temp -= 2048`):
    # the one value -1024 reads back as +1024, as on the real driver
    assert uld_mod.decode_offset(uld_mod.offset_raw(-1024)) == 1024
