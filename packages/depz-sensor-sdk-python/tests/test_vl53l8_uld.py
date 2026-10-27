"""VL53L8 host ULD (vl53l8/uld.py) against a scripted register/DCI platform:
init (FW download, checksum, NVM/xtalk/config upload), start/stop ranging,
poll-mode data-ready + frame parsing, power modes, xtalk caldata readback and
the xtalk calibration run — the commands on the wire, the argument checks and
the error paths a port (Rust/Java/C#) must reproduce. Contract 04 §5–§7; the
register sequences are the ST ULD C driver's (VL53LMZ 2.0.16 for check_data_ready
/ stop_ranging / GetRangingData)."""

from __future__ import annotations

import struct

import pytest

from depz_sensor_sdk.errors import InvalidStateError
from depz_sensor_sdk.vl53l8 import uld
from depz_sensor_sdk.vl53l8.uld import (
    POWER_MODE_DEEP_SLEEP,
    POWER_MODE_SLEEP,
    POWER_MODE_WAKEUP,
    RESOLUTION_4X4,
    RESOLUTION_8X8,
    UI_CMD_END,
    UI_CMD_START,
    VL53L8CX,
    Vl53l8cxError,
    swap_buffer,
)

from fake_vl53l8 import FakeUldPlatform


class ScriptedPlatform(FakeUldPlatform):
    """FakeUldPlatform plus what init/calibration need: a command status that
    answers both the NVM poll (byte 0 == 2) and the DCI polls (byte 1 == 3),
    firmware banks kept out of the register map, per-register read scripts and
    a canned reply to the GET_XTALK command."""

    def __init__(self):
        super().__init__()
        self.status = bytes([0x02, 0x03, 0x00, 0x00])
        self.banks: list[tuple[int, int]] = []   # (page, length) of FW banks
        self.page = None
        self.scripts: dict[int, list[int]] = {}  # addr -> successive byte reads
        self.xtalk_reply: bytes | None = None
        self._xtalk_pending = False
        self.reads: list[tuple[int, int]] = []

    def wr_multi(self, addr, data):
        data = bytes(data)
        if addr == 0x7FFF and len(data) == 1:
            self.page = data[0]
        if len(data) >= 0x5000:
            self.writes.append((addr, data))
            self.banks.append((self.page, len(data)))
            return
        if addr == 0x2FB8:
            self._xtalk_pending = True
            self._pending_read = None
            self.writes.append((addr, data))
            return
        if addr == UI_CMD_END - 11:
            self._xtalk_pending = False
        super().wr_multi(addr, data)

    def rd_multi(self, addr, size):
        self.reads.append((addr, size))
        if addr == uld.UI_CMD_STATUS:
            return self.status[:size].ljust(size, b'\x00')
        if addr == UI_CMD_START and self._xtalk_pending and self.xtalk_reply is not None:
            return self.xtalk_reply[:size]
        if size == 1 and self.scripts.get(addr):
            seq = self.scripts[addr]
            return bytes([seq.pop(0) if len(seq) > 1 else seq[0]])
        return super().rd_multi(addr, size)


def _set_u32(p, addr, value, swapped=True):
    raw = struct.pack('<I', value)
    if swapped:
        raw = swap_buffer(raw)
    for i, b in enumerate(raw):
        p.reg[addr + i] = b


def _booting_platform(variant='cx'):
    """A platform that boots: GO2 status answers, fw-access answer, and the
    variant's FW checksum at 0x2FFC (read swapped, as the driver does)."""
    p = ScriptedPlatform()
    p.reg[0x06] = 0x01                       # sensor boot / MCU boot
    p.reg[0x21] = 0x10 if variant in uld.I2C_VARIANTS else 0x04
    if uld.FW_CHECKSUM[variant] is not None:
        _set_u32(p, 0x2FFC, uld.FW_CHECKSUM[variant])
    if variant in uld.I2C_VARIANTS:
        p.reg[0x00], p.reg[0x01] = 0xF0, 0x02  # L5/L7 silicon, cut 1.2
    return p


def _driver(variant='cx', p=None):
    p = p or ScriptedPlatform()
    return VL53L8CX(p, variant=variant), p


def _ranging_driver(variant='cx', resolution=RESOLUTION_8X8):
    """A driver with its zone config set (no init) and the FW frame-size
    reply primed to agree with the host formula."""
    drv, p = _driver(variant)
    side = 4 if resolution == RESOLUTION_4X4 else 8
    p.dci[uld.DCI_ZONE_CONFIG] = bytes([side, side, 0, 0, 0, 0, 0, 0])
    return drv, p


def _fw_reports(p, size):
    p.dci[0x5440] = bytes(8) + struct.pack('<H', size) + bytes(2)


def _u32s(blob):
    return list(struct.unpack('<%dI' % (len(blob) // 4), blob))


# ── constructor / is_alive ────────────────────────────────────────────────────


def test_unknown_variant_refused():
    with pytest.raises(ValueError):
        VL53L8CX(ScriptedPlatform(), variant='l9')


@pytest.mark.parametrize('variant', ['cx', 'ch', 'l7cx', 'l7ch'])
def test_every_variant_loads_its_blobs(variant):
    drv, _ = _driver(variant)
    assert len(drv.firmware) == 0x15000
    assert len(drv.default_xtalk) == uld.XTALK_BUFFER_SIZE
    assert drv.i2c == (variant in uld.I2C_VARIANTS)


def test_a_firmware_blob_of_the_wrong_size_is_invalid_state(tmp_path):
    for blob in uld.VL53L8CX_DATA_DIR.iterdir():
        (tmp_path / blob.name).write_bytes(blob.read_bytes())
    (tmp_path / 'firmware.bin').write_bytes(bytes(0x15000 - 1))
    with pytest.raises(InvalidStateError):
        VL53L8CX(ScriptedPlatform(), variant='cx', data_dir=tmp_path)


def test_is_alive_reads_ids_on_page_0_and_restores_page_2():
    drv, p = _driver()
    p.reg[0x00], p.reg[0x01] = 0xF0, 0x0C
    assert drv.is_alive() == (0xF0, 0x0C)
    assert (drv.device_id, drv.revision_id) == (0xF0, 0x0C)
    assert p.writes == [(0x7FFF, b'\x00'), (0x7FFF, b'\x02')]


# ── init ──────────────────────────────────────────────────────────────────────


def test_init_downloads_three_fw_banks_and_uploads_config():
    p = _booting_platform('cx')
    drv, _ = _driver('cx', p)
    notes = []
    drv.init(progress=notes.append)
    # 84 KB firmware in three banks on pages 0x09 / 0x0A / 0x0B, written at 0
    assert p.banks == [(0x09, 0x8000), (0x0A, 0x8000), (0x0B, 0x5000)]
    bank_writes = [d for a, d in p.writes if len(d) >= 0x5000]
    assert b''.join(bank_writes) == drv.firmware
    assert all(a == 0 for a, d in p.writes if len(d) >= 0x5000)
    # NVM command, offset (0x2E18), xtalk (0x2CF8), default config (0x2C34)
    addrs = [a for a, _ in p.writes]
    assert (0x2FD8, drv.get_nvm_cmd) in p.writes
    assert addrs.index(0x2FD8) < addrs.index(0x2E18) < addrs.index(0x2CF8) \
        < addrs.index(0x2C34)
    assert (0x2C34, drv.default_cfg) in p.writes
    assert drv.xtalk_data == drv.default_xtalk
    assert len(drv.offset_data) == uld.OFFSET_BUFFER_SIZE
    # pipe control (1 target/zone) and single-range written through DCI
    assert p.dci[uld.DCI_PIPE_CONTROL] == bytes([uld.NB_TARGET_PER_ZONE, 0, 1, 0])
    assert p.dci[uld.DCI_SINGLE_RANGE] == struct.pack('<I', 1)
    assert 'Sensor FW checksum OK' in notes
    assert notes[-1] == 'Sensor init done'
    assert drv.module_type is None          # SPI (L8) path does not read it


def test_init_offset_upload_is_4x4_with_footer():
    p = _booting_platform('cx')
    drv, _ = _driver('cx', p)
    drv.init()
    offset = next(d for a, d in p.writes if a == 0x2E18)
    assert len(offset) == uld.OFFSET_BUFFER_SIZE
    assert offset[-8:] == bytes([0, 0, 0, 0x0F, 0x03, 0x01, 0x01, 0xE4])
    xtalk = next(d for a, d in p.writes if a == 0x2CF8)
    assert len(xtalk) == uld.XTALK_BUFFER_SIZE


def test_init_wrong_fw_checksum_raises():
    p = _booting_platform('cx')
    _set_u32(p, 0x2FFC, 0xDEADBEEF)
    drv, _ = _driver('cx', p)
    with pytest.raises(Vl53l8cxError) as e:
        drv.init()
    assert e.value.code == uld.STATUS_FW_CHECKSUM_FAIL


def test_init_sensor_never_boots_times_out():
    p = _booting_platform('cx')
    p.reg[0x06] = 0x00
    drv, _ = _driver('cx', p)
    with pytest.raises(Vl53l8cxError) as e:
        drv.init()
    assert e.value.code == uld.STATUS_TIMEOUT_ERROR
    # 2 s at 10 ms: 200 polls of reg 0x06, then the timeout
    assert p.reads.count((0x06, 1)) == 201
    assert not p.banks                       # never reached the FW download


def test_init_mcu_error_on_command_status():
    p = _booting_platform('cx')
    p.status = bytes([0x02, 0x03, 0x7F, 0x00])   # GO2 byte 2 >= 0x7F
    drv, _ = _driver('cx', p)
    with pytest.raises(Vl53l8cxError) as e:
        drv.init()
    assert e.value.code == uld.MCU_ERROR


def test_init_l7cx_checks_silicon_skips_checksum_reads_module_type():
    p = _booting_platform('l7cx')
    p.dci[uld.DCI_FW_FLAGS] = bytes([0, uld.MODULE_TYPE_MZEVO, 0, 0, 0, 0, 0, 0])
    drv, _ = _driver('l7cx', p)
    notes = []
    drv.init(progress=notes.append)
    assert drv.module_type == uld.MODULE_TYPE_MZEVO
    assert notes[-1] == 'Sensor init done (module type MZEVO)'
    assert 'Sensor FW checksum OK' not in notes   # ULD 2.0.1 has no checksum
    assert (0x2FFC, 4) not in p.reads
    # I2C path: GO1 handshake (0x20 = 7, 6) before the download
    assert (0x20, b'\x07') in p.writes and (0x20, b'\x06') in p.writes


def test_init_l7cx_refuses_l8_silicon():
    p = _booting_platform('l7cx')
    p.reg[0x00], p.reg[0x01] = 0xF0, 0x0C       # an L8 answering
    drv, _ = _driver('l7cx', p)
    with pytest.raises(Vl53l8cxError) as e:
        drv.init()
    assert e.value.code == uld.STATUS_ERROR
    assert 'not a VL53L5/L7 sensor' in str(e.value)
    assert not p.banks


# ── start_ranging: output config on the wire ─────────────────────────────────


@pytest.mark.parametrize('variant,resolution,size', [
    ('cx', RESOLUTION_8X8, 1452),
    ('cx', RESOLUTION_4X4, 540),
    ('ch', RESOLUTION_8X8, 1444),     # 24-byte frame tail on the 2.0.x FW
    ('l7cx', RESOLUTION_4X4, 1060),   # per-target blocks keep 64 entries
    ('l7cx', RESOLUTION_8X8, 1444),
])
def test_start_ranging_frame_size_and_output_config(variant, resolution, size):
    drv, p = _ranging_driver(variant, resolution)
    _fw_reports(p, size)
    drv.start_ranging()
    assert drv.data_read_size == size
    assert drv.frame_size_mismatch is None
    assert drv.resolution == resolution
    assert drv.streamcount == 255
    assert p.dci[uld.DCI_OUTPUT_CONFIG] == struct.pack('<2I', size, 13)
    w3 = 0 if variant in ('ch', 'l7ch') else 0xC0000000
    assert _u32s(p.dci[uld.DCI_OUTPUT_ENABLES]) == [0xFFF, 0, 0, w3]
    out = _u32s(p.dci[uld.DCI_OUTPUT_LIST])
    assert len(out) == 12
    assert out[0] == uld.START_BH
    sizes = {uld._bh_fields(bh)[2]: uld._bh_fields(bh)[1] for bh in out}
    assert sizes[uld.AMBIENT_RATE_IDX] == resolution
    assert sizes[uld.SPAD_COUNT_IDX] == resolution
    per_target = 64 if drv.i2c else resolution
    assert sizes[uld.DISTANCE_IDX] == per_target
    assert sizes[uld.TARGET_STATUS_IDX] == per_target
    assert sizes[uld.MOTION_DETEC_IDX] == 0x8C       # not a zone-scaled type


def test_start_ranging_wire_sequence():
    drv, p = _ranging_driver()
    _fw_reports(p, 1452)
    drv.start_ranging()
    regs = [(a, d) for a, d in p.writes if len(d) in (1, 4) and a < 0x2C00
            or a == 0x7FFF or a == UI_CMD_END - 3]
    # xshut bypass (interrupt mode) on page 0, then the start command
    i = regs.index((0x09, b'\x05'))
    assert regs[i - 1] == (0x7FFF, b'\x00')
    assert regs[i + 1] == (0x7FFF, b'\x02')
    assert regs[i + 2] == (UI_CMD_END - 3, bytes([0x00, 0x03, 0x00, 0x00]))


def test_start_ranging_trusts_fw_frame_size():
    drv, p = _ranging_driver()
    _fw_reports(p, 1448)
    drv.start_ranging()
    assert drv.frame_size_mismatch == (1448, 1452)
    assert drv.data_read_size == 1448


def test_start_ranging_without_resolution_refused_before_any_write():
    drv, p = _driver()
    p.dci[uld.DCI_ZONE_CONFIG] = bytes(8)       # 0 zones
    with pytest.raises(Vl53l8cxError) as e:
        drv.start_ranging()
    assert e.value.code == uld.STATUS_INVALID_PARAM
    assert uld.DCI_OUTPUT_LIST not in p.dci
    assert (UI_CMD_END - 3, bytes([0, 3, 0, 0])) not in p.writes


def test_start_ranging_laser_safety_fault():
    drv, p = _ranging_driver()
    _fw_reports(p, 1452)
    p.dci[uld.DCI_FW_FLAGS] = bytes([0, 0, 0, 0, 0, 0, 1, 0])
    with pytest.raises(Vl53l8cxError) as e:
        drv.start_ranging()
    assert e.value.code == uld.STATUS_LASER_SAFETY


def test_start_ranging_with_cnh_appends_block_13():
    drv, p = _ranging_driver('ch')
    cnh = 1708
    size = 1444 + cnh + 4
    _fw_reports(p, size)
    drv.start_ranging(cnh_data_size=cnh)
    assert drv.data_read_size == size
    out = _u32s(p.dci[uld.DCI_OUTPUT_LIST])
    assert len(out) == 13
    assert out[12] == (uld.CNH_DATA_IDX << 16) | ((cnh // 4) << 4) | 4
    assert _u32s(p.dci[uld.DCI_OUTPUT_ENABLES])[0] == 0x1FFF
    assert p.dci[uld.DCI_OUTPUT_CONFIG] == struct.pack('<2I', size, 14)


# ── stop_ranging ──────────────────────────────────────────────────────────────

_UNDO = [(0x7FFF, b'\x00'), (0x14, b'\x00'), (0x15, b'\x00'),
         (0x09, b'\x04'), (0x7FFF, b'\x02')]


def test_stop_ranging_provokes_mcu_stop_then_undoes_it():
    drv, p = _driver()
    p.reg[0x06] = 0x80                           # MCU stopped
    p.reg[0x07] = 0x84
    drv.stop_ranging()
    assert p.writes == [(0x7FFF, b'\x00'), (0x15, b'\x16'), (0x14, b'\x01')] + _UNDO
    # auto-stop flag read raw (not swapped) from 0x2FFC
    assert p.reads[0] == (0x2FFC, 4)


def test_stop_ranging_after_auto_stop_skips_the_mcu_stop():
    drv, p = _driver()
    _set_u32(p, 0x2FFC, 0x4FF, swapped=False)
    drv.stop_ranging()
    assert p.writes == _UNDO


def test_stop_ranging_mcu_never_stops_gives_up_and_still_undoes():
    drv, p = _driver()
    p.reg[0x06] = 0x00
    drv.stop_ranging()
    # 5 s at 10 ms: 501 polls, then the stop poll's own GO2 read
    assert p.reads.count((0x06, 1)) == 502
    assert p.writes[-5:] == _UNDO


# ── check_data_ready ──────────────────────────────────────────────────────────


def _go2(p, b0, b1, b2, b3):
    p.reg.update({0: b0, 1: b1, 2: b2, 3: b3})


def test_check_data_ready_new_streamcount_only_once():
    drv, p = _driver()
    _go2(p, 5, 0x05, 0x05, 0x10)
    assert drv.check_data_ready() is True
    assert drv.streamcount == 5
    assert drv.check_data_ready() is False       # same frame
    _go2(p, 6, 0x05, 0x05, 0x10)
    assert drv.check_data_ready() is True


@pytest.mark.parametrize('go2', [
    (255, 0x05, 0x05, 0x10),     # streamcount 255 = no frame yet
    (5, 0x04, 0x05, 0x10),       # byte 1 not 0x05
    (5, 0x05, 0x04, 0x10),       # byte 2 missing bits 0x05
    (5, 0x05, 0x05, 0x00),       # byte 3 missing 0x10
])
def test_check_data_ready_not_ready(go2):
    drv, p = _driver()
    _go2(p, *go2)
    assert drv.check_data_ready() is False


def test_check_data_ready_go2_error_is_raised_with_its_status():
    drv, p = _driver()
    _go2(p, 5, 0x00, 0x42, 0x80)
    with pytest.raises(Vl53l8cxError) as e:
        drv.check_data_ready()
    assert e.value.code == 0x42


# ── get_ranging_data / parse_frame ───────────────────────────────────────────


def _frame(drv, output, fill, frame_id=b'\x12\x34'):
    """Build a raw frame (as read from reg 0x00) for the programmed output
    list: blocks in the swapped domain, `fill(idx, msize)` gives a payload."""
    buf = bytearray(drv.data_read_size)
    buf[8:10] = frame_id
    i = 16
    for bh in output:
        t, s, idx = uld._bh_fields(bh)
        msize = t * s if 0x1 < t < 0xD else s
        struct.pack_into('<I', buf, i, bh)
        buf[i + 4:i + 4 + msize] = fill(idx, msize)
        i += 4 + msize
    foff = drv._footer_id_off
    buf[drv.data_read_size - foff:drv.data_read_size - foff + 2] = frame_id
    return swap_buffer(bytes(buf))


def _fill_4x4(idx, msize):
    n = 16
    if idx == uld.DISTANCE_IDX:
        return struct.pack('<16h', *([400 * 4 + 3] + [-5] + [8] * 14))
    if idx == uld.RANGE_SIGMA_MM_IDX:
        return struct.pack('<16H', *([256] * 16))
    if idx == uld.NB_TARGET_DETECTED_IDX:
        return bytes([1, 0] + [1] * 14)
    if idx == uld.TARGET_STATUS_IDX:
        return bytes([5] * n)
    if idx == uld.SIGNAL_RATE_IDX:
        return struct.pack('<16I', *range(16))
    if idx == uld.METADATA_IDX:
        return bytes(8) + bytes([0xF6]) + bytes(msize - 9)   # -10 degC
    return bytes(msize)


def _started_4x4(variant='cx'):
    drv, p = _ranging_driver(variant, RESOLUTION_4X4)
    _fw_reports(p, 540 if variant == 'cx' else 532)
    drv.start_ranging()
    return drv, p, _u32s(p.dci[uld.DCI_OUTPUT_LIST])


def test_get_ranging_data_reads_frame_size_from_reg_0_and_scales():
    drv, p, out = _started_4x4()
    raw = _frame(drv, out, _fill_4x4)
    for i, b in enumerate(raw):
        p.reg[i] = b
    r = drv.get_ranging_data()
    assert p.reads[-1] == (0x0, 540)
    assert len(r['distance_mm']) == 16
    assert r['distance_mm'][0] == 400                  # raw / 4
    assert r['distance_mm'][1] == 0                     # negative → 0, as ST (contract 04 §5)
    assert r['distance_mm'][2] == 2
    assert r['range_sigma_mm'][0] == 2.0               # raw / 128
    assert r['target_status'][0] == 5
    assert r['target_status'][1] == 255                # zone with no target
    assert r['signal_per_spad'] == [i / 2048 for i in range(16)]  # kcps/SPAD
    assert r['silicon_temp_degc'] == -10
    assert r['cnh_raw'] is None
    assert 'motion_indicator' not in r
    assert drv.streamcount == raw[0]
    assert [b[0] for b in drv.last_blocks][:3] == [0, uld.METADATA_IDX, 0x54C0]


def test_parse_frame_header_footer_mismatch_is_corrupted():
    drv, _, out = _started_4x4()
    raw = bytearray(swap_buffer(_frame(drv, out, _fill_4x4)))
    raw[drv.data_read_size - drv._footer_id_off] ^= 0xFF
    with pytest.raises(Vl53l8cxError) as e:
        drv.parse_frame(swap_buffer(bytes(raw)))
    assert e.value.code == uld.STATUS_CORRUPTED_FRAME


def test_parse_frame_stops_at_a_block_running_past_the_end():
    drv, _, out = _started_4x4()
    # last block header claims more than the frame holds: walk stops, no crash
    out = out[:-1] + [uld._bh_set_size(out[-1], 0xFFF)]
    r = drv.parse_frame(_frame(drv, out[:-1], _fill_4x4))
    assert uld.MOTION_DETEC_IDX not in [b[0] for b in drv.last_blocks]
    assert r['distance_mm'][0] == 400


def test_parse_frame_motion_indicator_block():
    drv, _, out = _started_4x4()
    drv._motion_present = True

    def fill(idx, msize):
        if idx == uld.MOTION_DETEC_IDX:
            return (struct.pack('<II4B', 7, 9, 1, 2, 16, 0)
                    + struct.pack('<32I', *range(100, 132)))
        return _fill_4x4(idx, msize)

    r = drv.parse_frame(_frame(drv, out, fill))
    assert r['motion_indicator'] == {
        'global_indicator_1': 7, 'global_indicator_2': 9, 'status': 1,
        'nb_of_detected_aggregates': 2, 'nb_of_aggregates': 16,
        'motion': list(range(100, 132)),
    }


def test_parse_frame_motion_indicator_absent_is_none():
    drv, _, out = _started_4x4()
    drv._motion_present = True
    out = [bh for bh in out if uld._bh_fields(bh)[2] != uld.MOTION_DETEC_IDX]
    drv.data_read_size -= 144
    r = drv.parse_frame(_frame(drv, out, _fill_4x4))
    assert r['motion_indicator'] is None


def test_parse_frame_l7_trims_per_target_blocks_to_resolution():
    drv, p = _ranging_driver('l7cx', RESOLUTION_4X4)
    _fw_reports(p, 1060)
    drv.start_ranging()
    out = _u32s(p.dci[uld.DCI_OUTPUT_LIST])

    def fill(idx, msize):
        if idx == uld.DISTANCE_IDX:
            return struct.pack('<64h', *([40] * 16 + [999] * 48))
        if idx == uld.NB_TARGET_DETECTED_IDX:
            return bytes([1] * 64)
        return bytes(msize)

    r = drv.parse_frame(_frame(drv, out, fill))
    for key in ('distance_mm', 'target_status', 'signal_per_spad', 'range_sigma_mm',
                'reflectance', 'ambient_per_spad', 'nb_spads_enabled',
                'nb_target_detected'):
        assert len(r[key]) == 16, key
    assert r['distance_mm'] == [10] * 16


# ── power modes ───────────────────────────────────────────────────────────────


def test_get_power_mode_unknown_register_value_raises_and_restores_page():
    drv, p = _driver()
    p.reg[0x09] = 0x07
    with pytest.raises(Vl53l8cxError) as e:
        drv.get_power_mode()
    assert e.value.code == uld.STATUS_ERROR
    assert p.writes[-1] == (0x7FFF, b'\x02')


def test_get_power_mode_l7cx_has_no_deep_sleep():
    drv, p = _driver('l7cx')
    p.reg[0x09] = 0x02
    p.reg[0x0F] = 0x43
    assert drv.get_power_mode() == POWER_MODE_SLEEP
    assert (0x0F, 1) not in p.reads


def test_set_power_mode_same_mode_writes_nothing():
    drv, p = _driver()
    p.reg[0x09] = 0x04
    drv.set_power_mode(POWER_MODE_WAKEUP)
    assert p.writes == [(0x7FFF, b'\x00'), (0x7FFF, b'\x02')]


def test_set_power_mode_deep_sleep_sequence():
    drv, p = _driver()
    p.reg[0x09] = 0x04
    p.reg[0x06] = 0x00
    drv.set_power_mode(POWER_MODE_DEEP_SLEEP)
    assert p.writes[-4:] == [(0x7FFF, b'\x00'), (0x09, b'\x02'),
                             (0x0F, b'\x43'), (0x7FFF, b'\x02')]


def test_set_power_mode_wakeup_from_sleep_does_not_reinit(monkeypatch):
    drv, p = _driver()
    p.reg[0x09] = 0x02
    p.reg[0x0F] = 0x40
    p.reg[0x06] = 0x01
    calls = []
    monkeypatch.setattr(drv, 'init', lambda *a, **k: calls.append(1))
    drv.set_power_mode(POWER_MODE_WAKEUP)
    assert calls == []
    assert p.writes[-3:] == [(0x7FFF, b'\x00'), (0x09, b'\x04'), (0x7FFF, b'\x02')]


def test_set_power_mode_wakeup_from_deep_sleep_reinits(monkeypatch):
    drv, p = _driver()
    p.reg[0x09] = 0x02
    p.reg[0x06] = 0x01
    p.scripts[0x0F] = [0x43]
    calls = []
    monkeypatch.setattr(drv, 'init', lambda *a, **k: calls.append(1))
    drv.set_power_mode(POWER_MODE_WAKEUP)
    assert (0x0F, b'\x40') in p.writes            # leave deep sleep
    assert calls == [1]                           # FW blob was lost


def test_set_power_mode_l7cx_wakeup_and_no_deep_sleep():
    drv, p = _driver('l7cx')
    p.reg[0x09] = 0x02
    p.reg[0x06] = 0x01
    drv.set_power_mode(POWER_MODE_WAKEUP)
    assert p.writes[-3:] == [(0x7FFF, b'\x00'), (0x09, b'\x04'), (0x7FFF, b'\x02')]
    with pytest.raises(Vl53l8cxError) as e:
        drv.set_power_mode(POWER_MODE_DEEP_SLEEP)
    assert e.value.code == uld.STATUS_INVALID_PARAM


def test_set_power_mode_unknown_mode_raises():
    drv, p = _driver()
    p.reg[0x09] = 0x04
    with pytest.raises(Vl53l8cxError) as e:
        drv.set_power_mode(7)
    assert e.value.code == uld.STATUS_INVALID_PARAM


def test_set_power_mode_sleep_times_out_when_mcu_stays_awake():
    drv, p = _driver()
    p.reg[0x09] = 0x04
    p.reg[0x06] = 0x01                            # never clears bit 0
    with pytest.raises(Vl53l8cxError) as e:
        drv.set_power_mode(POWER_MODE_SLEEP)
    assert e.value.code == uld.STATUS_TIMEOUT_ERROR


# ── xtalk margin / caldata ────────────────────────────────────────────────────


@pytest.mark.parametrize('bad', [-0.5, 10000.5, float('nan')])
def test_set_xtalk_margin_out_of_range_refused(bad):
    drv, p = _driver()
    with pytest.raises(Vl53l8cxError):
        drv.set_xtalk_margin(bad)
    assert uld.DCI_XTALK_CFG not in p.dci


def _xtalk_reply():
    return bytes(range(256)) * 4


def test_get_caldata_xtalk_reads_blob_and_restores_resolution():
    drv, p = _ranging_driver('cx', RESOLUTION_4X4)
    drv.offset_data = bytes(uld.OFFSET_BUFFER_SIZE)
    drv.xtalk_data = drv.default_xtalk
    p.xtalk_reply = _xtalk_reply()
    blob = drv.get_caldata_xtalk()
    assert (0x2FB8, uld.GET_XTALK_CMD) in p.writes
    assert (UI_CMD_START, uld.XTALK_BUFFER_SIZE + 4) in p.reads
    assert len(blob) == uld.XTALK_BUFFER_SIZE
    assert blob[:-8] == p.xtalk_reply[8:uld.XTALK_BUFFER_SIZE]
    assert blob[-8:] == bytes([0, 0, 0, 0x0F, 0, 0x01, 0x03, 0x04])
    assert drv.xtalk_data == blob
    assert drv.get_resolution() == RESOLUTION_4X4


# ── detection thresholds: argument checks ────────────────────────────────────


def test_set_detection_thresholds_more_than_64_refused():
    drv, p = _driver()
    with pytest.raises(ValueError):
        drv.set_detection_thresholds([{}] * 65)
    assert uld.DCI_DET_THRESH_START not in p.dci


@pytest.mark.parametrize('field,value', [
    ('low_thresh', 1.5), ('measurement', 256), ('zone_num', -1),
    ('high_thresh', 2 ** 31 // 4),       # overflows i32 after the ×4 distance scale
])
def test_set_detection_thresholds_bad_field_refused(field, value):
    drv, p = _driver()
    t = {'measurement': uld.DIST_MM, field: value}
    with pytest.raises(ValueError):
        drv.set_detection_thresholds([t])
    assert uld.DCI_DET_THRESH_START not in p.dci


def test_set_detection_thresholds_fills_missing_with_zeros_and_marks_valid():
    drv, p = _driver()
    drv.set_detection_thresholds([])
    assert p.dci[uld.DCI_DET_THRESH_VALID_STATUS] == bytes([0x05] * 8)
    assert p.dci[uld.DCI_DET_THRESH_START] == bytes(uld.NB_THRESHOLDS * 12)


def test_detection_thresholds_auto_stop_not_on_l7cx():
    drv, p = _driver('l7cx')
    with pytest.raises(Vl53l8cxError) as e:
        drv.set_detection_thresholds_auto_stop(True)
    assert e.value.code == uld.STATUS_INVALID_PARAM
    assert uld.DCI_PIPE_CONTROL not in p.dci


# ── motion indicator ─────────────────────────────────────────────────────────


def test_motion_indicator_4x4_map_and_bad_resolution():
    drv, p = _driver()
    cfg = drv.motion_indicator_init(RESOLUTION_4X4)
    assert cfg.map_id == list(range(16)) + [-1] * 48
    with pytest.raises(Vl53l8cxError) as e:
        drv.motion_indicator_set_resolution(cfg, 32)
    assert e.value.code == uld.STATUS_INVALID_PARAM


@pytest.mark.parametrize('lo,hi', [(400, 1901), (300, 1000), (3000, 4001)])
def test_motion_distance_window_bounds(lo, hi):
    drv, _ = _driver()
    cfg = uld.MotionConfig()
    with pytest.raises(Vl53l8cxError):
        drv.motion_indicator_set_distance_motion(cfg, lo, hi)


def test_motion_distance_window_formula():
    drv, p = _driver()
    cfg = uld.MotionConfig()
    drv.motion_indicator_set_distance_motion(cfg, 1000, 2500)
    assert cfg.ref_bin_offset == int(((1000 / 37.5348) - 4.0) * 2048.5)
    assert cfg.feature_length == int(((150.0 + 30.02784) / 15.01392) + 0.5)
    assert p.dci[uld.DCI_MOTION_DETECTOR_CFG] == cfg.pack()


# ── xtalk calibration run ────────────────────────────────────────────────────


@pytest.mark.parametrize('args', [
    (0, 4, 600), (100, 4, 600),      # reflectance 1..99
    (16, 0, 600), (16, 17, 600),     # nb_samples 1..16
    (16, 4, 599), (16, 4, 3001),     # distance 600..3000
])
def test_calibrate_xtalk_argument_checks(args):
    drv, p = _driver()
    with pytest.raises(Vl53l8cxError) as e:
        drv.calibrate_xtalk(*args)
    assert e.value.code == uld.STATUS_INVALID_PARAM
    assert p.writes == []


def _calibration_driver(variant='cx'):
    drv, p = _ranging_driver(variant, RESOLUTION_4X4)
    drv.offset_data = bytes(uld.OFFSET_BUFFER_SIZE)
    drv.xtalk_data = drv.default_xtalk
    p.dci[uld.DCI_FREQ_HZ] = bytes([0, 15, 0, 0])
    p.dci[uld.DCI_INT_TIME] = struct.pack('<I', 20000) + bytes(16)
    p.dci[uld.DCI_RANGING_MODE] = bytes([0, 3, 0, 2, 0, 0, 0, 0])   # autonomous
    p.dci[uld.DCI_TARGET_ORDER] = bytes([uld.TARGET_ORDER_CLOSEST, 0, 0, 0])
    p.xtalk_reply = _xtalk_reply()
    return drv, p


@pytest.mark.parametrize('variant,table', [
    ('cx', uld.CALIBRATE_XTALK),
    ('ch', uld.CALIBRATE_XTALK_LMZ),
    ('l7cx', uld.CALIBRATE_XTALK_L7CX),
], ids=['cx', 'ch', 'l7cx'])
def test_calibrate_xtalk_run_and_restore(variant, table):
    drv, p = _calibration_driver(variant)
    drv.calibrate_xtalk(16, 4, 600)
    # the firmware's own table at 0x2C28, before the start command
    addrs = [a for a, _ in p.writes]
    assert (0x2C28, table) in p.writes
    assert addrs.index(0x2C28) < addrs.index(UI_CMD_END - 3) < addrs.index(0x2FB8)
    # target: distance ×4 at 0, reflectance ×16 at 2, samples at 4
    cal = p.dci[uld.DCI_CAL_CFG]
    assert struct.unpack_from('<HHB', cal, 0) == (2400, 256, 4)
    # the 17-block calibration output list, not the ranging one
    assert [bh & ~0xFFF0 for bh in _u32s(p.dci[uld.DCI_OUTPUT_LIST])] == \
        [bh & ~0xFFF0 for bh in uld.XTALK_CAL_OUTPUT]
    assert _u32s(p.dci[uld.DCI_OUTPUT_ENABLES]) == uld.XTALK_CAL_OUTPUT_ENABLE
    assert struct.unpack('<2I', p.dci[uld.DCI_OUTPUT_CONFIG]) == (1204, 18)
    # result read back, default config re-sent, settings restored
    assert drv.xtalk_calibration_failed is False
    assert drv.xtalk_data[:-8] == p.xtalk_reply[8:uld.XTALK_BUFFER_SIZE]
    assert addrs.index(0x2FB8) < addrs.index(0x2C34)
    assert drv.get_resolution() == RESOLUTION_4X4
    assert drv.get_ranging_frequency_hz() == 15
    assert drv.get_integration_time_ms() == 20
    assert drv.get_ranging_mode() == uld.RANGING_MODE_AUTONOMOUS
    assert drv.get_target_order() == uld.TARGET_ORDER_CLOSEST


def test_calibrate_xtalk_coverglass_too_good_is_flagged_not_raised():
    drv, p = _calibration_driver()
    _go2(p, 0x00, 0x00, 0x7F, 0x80)
    drv.calibrate_xtalk(16, 4, 600)
    assert drv.xtalk_calibration_failed is True


def test_calibrate_xtalk_waits_then_times_out():
    drv, p = _calibration_driver()
    p.reg[0] = uld.STATUS_ERROR                   # still calibrating
    with pytest.raises(Vl53l8cxError) as e:
        drv.calibrate_xtalk(16, 4, 600)
    assert e.value.code == uld.STATUS_ERROR
    assert 'xtalk calibration' in str(e.value)
    # 400 waits of 50 ms (20 s), 401 reads of the GO2 header
    assert p.reads.count((0x0, 4)) == 401
    assert (0x2FB8, uld.GET_XTALK_CMD) not in p.writes
