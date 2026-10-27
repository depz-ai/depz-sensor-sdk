"""Fake VL53L8 sensors for host-side tests (no hardware).

Two levels: `FakeUldPlatform` emulates the register bridge + DCI transport
(swap_buffer + header/footer framing) under uld.VL53L8CX directly, so the real
byte sequences in uld.py are exercised end to end; `FakeVl53l8` (below) is the
whole board on the wire — the APP_VL53L8 bridge firmware of contract 04 over a
LoopbackLink in front of a `FakeVl53l8Sensor` chip model — for the Vl53l8cx /
Vl53l8ch device classes, the recorder and the CLI.
"""

from __future__ import annotations

import struct

from depz_sensor_sdk.vl53l8 import cnh, uld
from depz_sensor_sdk.vl53l8.uld import VL53L8CX, swap_buffer

UI_CMD_STATUS = 0x2C00
UI_CMD_START = 0x2C04
UI_CMD_END = 0x2FFF


class FakeUldPlatform:
    """Emulates the register bridge + DCI transport for uld.VL53L8CX."""

    def __init__(self):
        self.reg: dict[int, int] = {}
        self.dci: dict[int, bytes] = {}
        self._pending_read: tuple[int, int] | None = None
        self.writes: list[tuple[int, bytes]] = []

    def sleep_ms(self, ms):
        pass

    def wr_multi(self, addr: int, data: bytes) -> None:
        data = bytes(data)
        self.writes.append((addr, data))
        # DCI read command: 12 bytes written to UI_CMD_END-11 (0x2FF4)
        if addr == UI_CMD_END - 11 and len(data) == 12:
            index = (data[0] << 8) | data[1]
            size = ((data[2] & 0xFF) << 4) | ((data[3] & 0xFF) >> 4)
            self._pending_read = (index, size)
            return
        # DCI write: header(4) + swap(payload) + footer(8), footer signature
        if len(data) >= 12 and data[-5] == 0x0F and data[-4] == 0x05 and data[-3] == 0x01:
            index = (data[0] << 8) | data[1]
            data_size = ((data[2] & 0xFF) << 4) | ((data[3] & 0xFF) >> 4)
            payload = swap_buffer(data[4 : 4 + data_size])
            self.dci[index] = payload
            return
        # plain byte writes → registers
        for i, b in enumerate(data):
            self.reg[addr + i] = b

    def rd_multi(self, addr: int, size: int) -> bytes:
        if addr == UI_CMD_STATUS:
            return bytes([0x00, 0x03, 0x00, 0x00])[:size].ljust(size, b"\x00")
        if addr == UI_CMD_START and self._pending_read is not None:
            index, dsize = self._pending_read
            stored = self.dci.get(index, b"\x00" * dsize)
            stored = stored[:dsize].ljust(dsize, b"\x00")
            raw = (b"\x00" * 4 + stored).ljust(dsize + 12, b"\x00")
            return swap_buffer(raw)
        return bytes([self.reg.get(addr + i, 0) for i in range(size)])


def make_driver():
    """Return (driver, platform) with a VL53L8CX bypassing blob-file init."""
    p = FakeUldPlatform()
    drv = VL53L8CX.__new__(VL53L8CX)
    drv.p = p
    drv.variant = "cx"
    drv._output_enable_w3 = 0xC0000000
    drv._frame_tail = 32
    drv._footer_id_off = 12
    drv._motion_present = False
    drv.default_xtalk = b"\x00" * uld.XTALK_BUFFER_SIZE
    return drv, p


# ── wire-level fake (LoopbackLink) ───────────────────────────────────────────
#
# A VL53L8 board as the host sees it: `FakeVl53l8Sensor` is the sensor chip
# behind the SPI bridge (paged register map, UI command area, DCI store,
# firmware banks, a deterministic scene rendered into ranging frames laid out
# exactly like the frames in contracts/vectors/recordings/vl53l8*_8x8_15hz*),
# `FakeVl53l8` is the APP_VL53L8 bridge firmware speaking contract 04 over a
# LoopbackLink. The sensor's variant (cx / ch) is not fixed up front: like the
# real silicon it becomes whatever firmware blob the host downloads.

#: Header/footer id the fake stamps on every frame (the lab VL53L8CX streamed 0x0017).
FRAME_ID = 0x0017
#: Deterministic scene: a plane tilted away from the sensor, 25 mm per zone step.
SCENE_BASE_MM = 400
SCENE_STEP_MM = 25
SILICON_TEMP_DEGC = 37
MODULE_TYPE_MZPLUS = 2
FW_BANK_PAGES = (0x09, 0x0A, 0x0B)
STATUS_READY = bytes([0x02, 0x03, 0x00, 0x00])  # NVM poll wants [0]==2, DCI polls [1]==3
UI_CMD_START_RANGING = bytes([0x00, 0x03, 0x00, 0x00])
HEADER_WORDS = 4  # 16-byte frame head: status word, 0, header id, START block
TAIL_BYTES = {"cx": 32, "ch": 24, "l7cx": 24, "l7ch": 24}
FOOTER_ID_OFF = {"cx": 12, "ch": 4, "l7cx": 4, "l7ch": 4}
CNH_REF_RESIDUAL_RAW = 3 * 2048


def _load_blobs(variants) -> dict[str, bytes]:
    """firmware.bin of each variant, from its own set or the set it borrows
    the file from (as the ULD resolves it: l7ch ships the ch firmware)."""
    out = {}
    for v in variants:
        path = uld.VARIANT_DATA_DIR[v] / "firmware.bin"
        if not path.exists():
            path = uld._BLOB_FALLBACK_DIR[v] / "firmware.bin"
        out[v] = path.read_bytes()
    return out


class FakeVl53l8Sensor:
    """The VL53L8 chip behind the bridge: what the ULD register/DCI sequences
    of contract 04 §5–§7 talk to. Deterministic; no threads, no clock."""

    DEVICE_ID = 0xF0
    REVISION_ID = 0x0C
    #: Firmware blobs this silicon can boot (the host picks one by class).
    VARIANTS = ("cx", "ch")
    _blobs: dict[str, bytes] | None = None

    def __init__(self):
        cls = type(self)
        if cls.__dict__.get("_blobs") is None:
            cls._blobs = _load_blobs(cls.VARIANTS)
        self.page = 0
        self.reg: dict[tuple[int, int], int] = {}
        self.fw_banks: dict[int, bytearray] = {}
        self.variant: str | None = None  # set by the firmware the host downloaded
        self.boots = 0  # MCU resets (one per init)
        self.writes: list[tuple[int, int, bytes]] = []  # (page, addr, data)
        self.reads: list[tuple[int, int, int]] = []  # (page, addr, size)
        self.offset_uploads: list[bytes] = []  # 0x2E18
        self.xtalk_uploads: list[bytes] = []  # 0x2CF8
        self.config_uploads = 0  # default configuration at 0x2C34
        self.calib_tables: list[bytes] = []  # xtalk calibration tables (0x2C28)
        self.calibrations: list[tuple[int, int, int]] = []  # (distance_mm*4, refl*16, nb_samples)
        self.dci: dict[int, bytes] = self._default_dci()
        self.dci_writes: list[int] = []  # indices in write order
        self.nvm = bytes((i * 7 + 3) & 0xFF for i in range(uld.NVM_DATA_SIZE))
        self.xtalk_buf: bytes = bytes(uld.XTALK_BUFFER_SIZE)  # what the FW holds
        self.calibrated_buf: bytes | None = None  # last calibration result
        self.ranging = False
        self.frame_count = 0
        self.latched: bytes = b""  # poll-mode frame at register 0x00 (page 2)
        self.motion_configured = False
        # knobs
        self.distance_offset_mm = 0
        self.silicon_temp_degc = SILICON_TEMP_DEGC
        self.xtalk_cal_fails = False  # answer ST's "coverglass too good" (XTALK_FAILED)
        self.calib_busy_polls = 1  # result header reads answered 0xFF before done
        self.laser_safety_fault = False
        self.module_type = MODULE_TYPE_MZPLUS  # DCI 0xE0C4 byte[1]
        self.corrupt_firmware = False  # flip a byte of the downloaded blob
        self.corrupt_footer = False  # next frame: footer id ≠ header id
        self._pending: tuple | None = None
        self._calib_armed = False
        self._calib_polls = 0
        # booted sensor, awake, host-visible silicon ids on page 0
        self.reg[(0, 0x00)] = self.DEVICE_ID
        self.reg[(0, 0x01)] = self.REVISION_ID
        self.reg[(0, 0x06)] = 0x01
        self.reg[(0, 0x09)] = 0x04
        self.reg[(1, 0x21)] = 0x04

    @staticmethod
    def _default_dci() -> dict[int, bytes]:
        dss = bytearray(16)
        dss[4] = dss[6] = 64
        dss[9] = 4
        sharp = bytearray(16)
        sharp[0xD] = 5 * 255 // 100
        return {
            uld.DCI_ZONE_CONFIG: bytes([4, 4, 0, 0, 8, 8, 0, 0]),
            uld.DCI_DSS_CONFIG: bytes(dss),
            uld.DCI_FREQ_HZ: bytes([0, 1, 0, 0]),  # ST default: 1 Hz
            uld.DCI_INT_TIME: struct.pack("<I", 5000) + bytes(16),
            uld.DCI_RANGING_MODE: bytes([0, 1, 0, 3, 0, 0, 0, 0]),
            uld.DCI_TARGET_ORDER: bytes([uld.TARGET_ORDER_CLOSEST, 0, 0, 0]),
            uld.DCI_SHARPENER: bytes(sharp),
            uld.DCI_XTALK_CFG: bytes(16),
            uld.DCI_DET_THRESH_GLOBAL_CONFIG: bytes([1, 0, 1, 0, 0, 0, 0, 0]),
            uld.DCI_DET_THRESH_CONFIG: bytes(20),
            uld.DCI_DET_THRESH_START: bytes(uld.NB_THRESHOLDS * 12),
            uld.DCI_DET_THRESH_VALID_STATUS: bytes(8),
            uld.DCI_PIPE_CONTROL: bytes([1, 0, 1, 0]),
            uld.DCI_SINGLE_RANGE: bytes([1, 0, 0, 0]),
            uld.DCI_CAL_CFG: bytes(8),
        }

    # ── register bridge ──────────────────────────────────────────────────────

    def wr_multi(self, addr: int, data: bytes) -> None:
        data = bytes(data)
        self.writes.append((self.page, addr, data))
        if addr == 0x7FFF and len(data) == 1:
            self.page = data[0]
            return
        if self.page in FW_BANK_PAGES:  # firmware bank: the host writes it in 2 KB pieces
            bank = self.fw_banks.setdefault(self.page, bytearray())
            bank[addr:addr + len(data)] = data
            return
        if self.page == 2 and self._ui_write(addr, data):
            return
        for i, b in enumerate(data):
            self.reg[(self.page, addr + i)] = b
        if self.page == 0:
            self._go1_side_effects(addr, data)

    def _go1_side_effects(self, addr: int, data: bytes) -> None:
        """Page-0 control registers the ULD drives: MCU stop/boot, power."""
        for i, b in enumerate(data):
            a = addr + i
            if a == 0x14:  # provoke / undo MCU stop
                if b == 0x01:
                    self.reg[(0, 0x06)] |= 0x80
                    self.reg[(0, 0x07)] = 0x84
                    self.ranging = False
                else:
                    self.reg[(0, 0x06)] &= ~0x80
            elif a == 0x0B and b == 0x01:  # MCU reset: the downloaded FW boots
                self._boot()
            elif a == 0x09:  # power: 0x02 sleep (MCU down), 0x04 awake, 0x05 xshut bypass
                if b == 0x02:
                    self.reg[(0, 0x06)] &= ~0x01
                elif b in (0x04, 0x05):
                    self.reg[(0, 0x06)] |= 0x01

    def _boot(self) -> None:
        self.boots += 1
        fw = b"".join(bytes(self.fw_banks.get(p, b"")) for p in FW_BANK_PAGES)
        if self.corrupt_firmware and fw:
            fw = fw[:100] + bytes([fw[100] ^ 0xFF]) + fw[101:]
        self.variant = next((v for v, blob in self._blobs.items() if blob == fw), None)
        crc = (uld.FW_CHECKSUM.get(self.variant) if self.variant else None) or 0
        for i, b in enumerate(struct.pack(">I", crc)):  # read back swapped → crc
            self.reg[(2, 0x2FFC + i)] = b
        self.reg[(0, 0x06)] = 0x01
        self.ranging = False
        self.latched = b""

    def _ui_write(self, addr: int, data: bytes) -> bool:
        """Page-2 UI command area (0x2C00..0x2FFF) and the data uploads."""
        if addr == UI_CMD_END - 11 and len(data) == 12:  # DCI read request
            index = (data[0] << 8) | data[1]
            size = (data[2] << 4) | (data[3] >> 4)
            self._pending = ("dci", index, size)
            return True
        if addr == 0x2FD8:  # get_nvm_cmd
            self._pending = ("nvm",)
            return True
        if addr == 0x2FB8:  # GET_XTALK_CMD
            self._pending = ("xtalk",)
            return True
        if addr == 0x2E18:
            self.offset_uploads.append(data)
            return True
        if addr == 0x2CF8:
            self.xtalk_uploads.append(data)
            self.xtalk_buf = data
            return True
        if addr == 0x2C34:
            self.config_uploads += 1
            return True
        if addr == 0x2C28:
            self.calib_tables.append(data)
            self._calib_armed = True
            return True
        if addr == UI_CMD_END - 3 and data == UI_CMD_START_RANGING:
            if self._calib_armed:
                self._run_xtalk_calibration()
            else:
                self.ranging = True
                self.frame_count = 0
            return True
        if len(data) >= 12 and data[-5:-2] == b"\x0f\x05\x01":  # DCI write
            index = (data[0] << 8) | data[1]
            size = (data[2] << 4) | (data[3] >> 4)
            self.dci[index] = swap_buffer(data[4 : 4 + size])
            self.dci_writes.append(index)
            if index == uld.DCI_MOTION_DETECTOR_CFG:
                # the same 156-byte struct carries the motion indicator (cnh_cfg
                # byte 0) and a CNH setup (cnh_cfg flags set, CnhConfig.pack)
                self.motion_configured = self.dci[index][25] == 0
            return True
        return False

    def rd_multi(self, addr: int, size: int) -> bytes:
        self.reads.append((self.page, addr, size))
        if addr == 0x7FFF:
            return bytes([self.page]).ljust(size, b"\x00")
        if self.page == 2:
            if addr == UI_CMD_STATUS:
                return STATUS_READY[:size].ljust(size, b"\x00")
            if addr == UI_CMD_START and self._pending is not None:
                return self._ui_answer()[:size].ljust(size, b"\x00")
            if addr < 0x2C00:
                return self._result_regs(addr, size)
        return bytes(self.reg.get((self.page, addr + i), 0) for i in range(size))

    def _ui_answer(self) -> bytes:
        pending, self._pending = self._pending, None
        if pending[0] == "nvm":
            return self.nvm
        if pending[0] == "xtalk":
            return bytes(8) + self.xtalk_buf[: uld.XTALK_BUFFER_SIZE - 8] + bytes(4)
        _, index, size = pending
        return swap_buffer(bytes(4) + self._dci_read(index, size) + bytes(8))

    def _dci_read(self, index: int, size: int) -> bytes:
        if index == 0x5440:  # the FW reports the frame size it will stream
            configured = struct.unpack_from("<I", self.dci[uld.DCI_OUTPUT_CONFIG], 0)[0]
            data = bytes(8) + struct.pack("<H", configured) + bytes(2)
        elif index == uld.DCI_FW_FLAGS:
            data = bytearray(8)
            data[1] = self.module_type
            data[6] = 1 if self.laser_safety_fault else 0
        else:
            data = self.dci.get(index, b"")
        return bytes(data)[:size].ljust(size, b"\x00")

    def _result_regs(self, addr: int, size: int) -> bytes:
        """Register 0x00.. on page 2: the result frame (poll mode) and the
        xtalk-calibration completion header."""
        if self._calib_polls > 0:
            self._calib_polls -= 1
            return bytes([0xFF] * size)
        return self.latched[addr : addr + size].ljust(size, b"\x00")

    # ── xtalk calibration (plugin_xtalk) ─────────────────────────────────────

    def _run_xtalk_calibration(self) -> None:
        self._calib_armed = False
        cfg = self.dci[uld.DCI_CAL_CFG]
        dist, refl, nb = struct.unpack_from("<HHB", cfg, 0)
        self.calibrations.append((dist, refl, nb))
        self._calib_polls = self.calib_busy_polls
        if self.xtalk_cal_fails:
            self.latched = bytes([0x01, 0x05, 0x7F, 0x90])  # GO2 error: XTALK_FAILED
            return
        self.latched = bytes([0x01, 0x05, 0x05, 0x10])
        self.xtalk_buf = self.calibrated_buf = self._calibrated_xtalk(dist // 4, refl // 16, nb)

    @staticmethod
    def xtalk_grid_value(distance_mm: int, reflectance_pc: int, nb_samples: int, zone: int) -> int:
        """Signal-grid entry the fake's calibration computes (deterministic)."""
        return (reflectance_pc * 1000 + distance_mm + nb_samples * 16 + zone) & 0xFFFFFFFF

    def _calibrated_xtalk(self, distance_mm: int, reflectance_pc: int, nb_samples: int) -> bytes:
        """The xtalk buffer the FW computes for a run: the uploaded buffer with
        the 8x8 signal grid (0x34, swapped domain) filled deterministically."""
        buf = bytearray(swap_buffer(self.xtalk_buf.ljust(uld.XTALK_BUFFER_SIZE, b"\x00")))
        grid = [self.xtalk_grid_value(distance_mm, reflectance_pc, nb_samples, z) for z in range(64)]
        struct.pack_into("<64I", buf, 0x34, *grid)
        return swap_buffer(bytes(buf))

    # ── scene → frame ────────────────────────────────────────────────────────

    @property
    def resolution(self) -> int:
        zc = self.dci[uld.DCI_ZONE_CONFIG]
        return zc[0] * zc[1]

    @property
    def frequency_hz(self) -> int:
        return self.dci[uld.DCI_FREQ_HZ][1]

    def _zones(self, resolution: int):
        side = 4 if resolution == uld.RESOLUTION_4X4 else 8
        return [(z, z // side, z % side) for z in range(resolution)]

    def expected_frame(self, resolution: int | None = None) -> dict:
        """What a correct host parses from the next frame (contract 04 §5 units)."""
        r = self.resolution if resolution is None else resolution
        last = r - 1  # the far corner has no target
        out = {
            "resolution": r,
            "distance_mm": [0 if z == last else SCENE_BASE_MM + SCENE_STEP_MM * (row + col) + self.distance_offset_mm
                            for z, row, col in self._zones(r)],
            "target_status": [255 if z == last else (9 if z == 0 else 5) for z in range(r)],
            "nb_target_detected": [0 if z == last else 1 for z in range(r)],
            "signal_per_spad": [1200 + z / 4 for z in range(r)],
            "ambient_per_spad": [3 + z / 8 for z in range(r)],
            "nb_spads_enabled": [16384 // r] * r,
            "range_sigma_mm": [2 + z / 4 for z in range(r)],
            "reflectance": [30 + z / 2 for z in range(r)],
            "silicon_temp_degc": self.silicon_temp_degc,
        }
        if self.motion_configured:
            out["motion"] = {"global_indicator_1": 0x1234, "global_indicator_2": 0, "status": 0,
                             "nb_of_detected_aggregates": 1, "nb_of_aggregates": 16,
                             "motion": [1000 * a for a in range(32)]}
        return out

    def _block_data(self, idx: int, msize: int, resolution: int) -> bytes:
        r = resolution
        exp = self.expected_frame(r)
        if idx == uld.METADATA_IDX:
            return struct.pack("<I", 0x2BD) + bytes(4) + struct.pack("<b", self.silicon_temp_degc) + bytes(3)
        if idx == 0x54C0:  # COMMONDATA, as the lab board
            return struct.pack("<I", 0x02400FCE)
        if idx == uld.AMBIENT_RATE_IDX:
            return struct.pack("<%dI" % r, *(2048 * 3 + 256 * z for z in range(r)))
        if idx == uld.SPAD_COUNT_IDX:
            return struct.pack("<%dI" % r, *exp["nb_spads_enabled"])
        if idx == uld.NB_TARGET_DETECTED_IDX:
            return bytes(exp["nb_target_detected"])
        if idx == uld.SIGNAL_RATE_IDX:
            return struct.pack("<%dI" % r, *(2048 * 1200 + 512 * z for z in range(r)))
        if idx == uld.RANGE_SIGMA_MM_IDX:
            return struct.pack("<%dH" % r, *(256 + 32 * z for z in range(r)))
        if idx == uld.DISTANCE_IDX:  # the empty zone holds a negative raw: clamped to 0 by the host
            return struct.pack("<%dh" % r, *(-4 if n == 0 else d * 4
                                               for d, n in zip(exp["distance_mm"], exp["nb_target_detected"])))
        if idx == uld.REFLECTANCE_EST_PC_IDX:
            return bytes(60 + z for z in range(r))
        if idx == uld.TARGET_STATUS_IDX:
            return bytes(0 if s == 255 else s for s in exp["target_status"])
        if idx == uld.MOTION_DETEC_IDX:
            if not self.motion_configured:
                return bytes(msize)
            m = exp["motion"]
            return (struct.pack("<II", m["global_indicator_1"], m["global_indicator_2"])
                    + bytes([m["status"], m["nb_of_detected_aggregates"], m["nb_of_aggregates"], 0])
                    + struct.pack("<32I", *m["motion"]))
        if idx == uld.CNH_DATA_IDX:
            cfg = self.dci.get(cnh.MI_CFG_DEV_IDX)
            feat, nb_agg = cfg[19], cfg[20]  # feature_length, nb_of_aggregates (CnhConfig.pack layout)
            block = cnh_block(nb_agg, feat)
            assert len(block) == msize, (len(block), msize)
            return block
        return bytes(msize)

    def build_frame(self) -> bytes:
        """One raw results frame (wire order) for the output list the host
        programmed: the layout of the lab captures, blocks from byte 16, the
        variant's tail with the footer id."""
        variant = self.variant or "cx"
        output = struct.unpack("<%dI" % (len(self.dci[uld.DCI_OUTPUT_LIST]) // 4), self.dci[uld.DCI_OUTPUT_LIST])
        enables = struct.unpack("<4I", self.dci[uld.DCI_OUTPUT_ENABLES])
        size = struct.unpack_from("<I", self.dci[uld.DCI_OUTPUT_CONFIG], 0)[0]
        r = self.resolution
        self.frame_count += 1
        buf = bytearray(bytes([0x31, 0x9D, 0x05, self.frame_count & 0xFF]) + bytes(4)
                        + struct.pack("<H", FRAME_ID) + bytes(2) + struct.pack("<I", uld.START_BH))
        for i, bh in enumerate(output[1:], 1):
            if not (enables[i // 32] >> (i % 32)) & 1:
                continue
            bh_type, bh_size, bh_idx = uld._bh_fields(bh)
            msize = bh_type * bh_size if 0x1 < bh_type < 0xD else bh_size
            # L5/L7 in 4x4: per-target blocks keep 64 entries; the sensor fills
            # the first `resolution` and zero-pads the rest (contract 11 §3).
            buf += struct.pack("<I", bh) + self._block_data(bh_idx, msize, r).ljust(msize, b"\x00")
        buf += struct.pack("<II", 0x0E, 0x0F)
        assert len(buf) + 4 <= size, (len(buf), size)
        buf = buf.ljust(size, b"\x00")
        foff = FOOTER_ID_OFF[variant]
        footer_id = FRAME_ID ^ 0xFFFF if self.corrupt_footer else FRAME_ID
        self.corrupt_footer = False
        buf[size - foff : size - foff + 2] = struct.pack("<H", footer_id)
        return swap_buffer(bytes(buf))

    def latch_frame(self) -> bytes:
        """Make the next frame readable at register 0x00 (poll mode, §6)."""
        self.latched = self.build_frame()
        return self.latched


def cnh_block(nb_agg: int, feat: int, *, state: int = cnh.MI_STATE_PING) -> bytes:
    """A CNH data block (host byte order) for `nb_agg` aggregates × `feat`
    bins as the VL53LMZ CNH pipeline lays it out with ping-pong and variance
    disabled; `expected_cnh()` is its decode. `state` other than PING puts the
    data in the pong buffer (never streamed with ping-pong off; decode test)."""
    pp_words = cnh._pingpong_size_in_words(cnh.MI_SFE_DISABLE_VARIANCE, nb_agg, feat)
    header = struct.pack("<5I", state, pp_words | (cnh._BUFFER_INFO_NO_VARIANCE << 24),
                          CNH_REF_RESIDUAL_RAW, 0x10, 0)
    exp = expected_cnh(nb_agg, feat)
    data = struct.pack("<II", state, 1)
    data += struct.pack("<%di" % (nb_agg * feat), *(v for a in exp["aggregates"] for v in a["hist_raw"]))
    frac = bytes(s & 0xFF for a in exp["aggregates"] for s in a["hist_scaler"])
    data += frac.ljust(((3 + nb_agg * feat) // 4) * 4, b"\x00")
    data += struct.pack("<%di" % nb_agg, *(7 + a for a in range(nb_agg)))
    data += bytes([1] * nb_agg).ljust(((3 + nb_agg) // 4) * 4, b"\x00")
    assert len(data) == pp_words * 4
    if state != cnh.MI_STATE_PING:
        data = bytes(pp_words * 4) + data  # ping buffer first, data in pong
    return header + data


def expected_cnh(nb_agg: int, feat: int) -> dict:
    """The decode of `cnh_block(nb_agg, feat)` (cnh.decode's shape)."""
    aggs = []
    for a in range(nb_agg):
        raw = [100 * (a + 1) + b for b in range(feat)]
        scaler = [a % 2] * feat
        aggs.append({"hist_raw": raw, "hist_scaler": scaler,
                     "hist": [v / 2.0 ** s for v, s in zip(raw, scaler)],
                     "ambient": (7 + a) / 2.0})
    return {"ref_residual": CNH_REF_RESIDUAL_RAW / 2048.0, "aggregates": aggs}


class FakeVl53l8:
    """In-process APP_VL53L8 bridge firmware (contract 04) over a LoopbackLink:
    READ_REG / WRITE_REG / START_STREAM / STOP_STREAM in front of a
    `FakeVl53l8Sensor`; the common commands come from FakeSr04. Frames are
    pushed by the test with `send_frame()` (one INT edge), chunked ≤1528 B."""

    SOFTWARE_NAME = "APP_VL53L8_v0.92"
    DEVICE_NAME = "DEPZ-VL53L8"
    SERIAL = "SN000005"
    ERR_INVALID_CMD = 0x02
    ERR_PAYLOAD_FORMAT = 0x03
    ERR_INVALID_PARAM = 0x04
    SENSOR = FakeVl53l8Sensor
    READ_MAX = 2295  # READ_REG len the bridge takes (payload format limit)
    WRITE_MAX = 2302  # WRITE_REG data bytes the bridge takes

    def __init__(self):
        from fake_device import FakeSr04

        self.sensor = self.SENSOR()
        self.streaming = False
        self.frame_size = 0
        self.frames_sent = 0
        self.chunks_sent = 0
        self.bus_ops_while_streaming = 0  # READ/WRITE_REG seen between START and STOP_STREAM
        self.truncate_reads = False  # bridge fault: answer one byte short
        outer = self

        class _Fw(FakeSr04):
            SOFTWARE_NAME = outer.SOFTWARE_NAME
            DEVICE_NAME = outer.DEVICE_NAME
            SERIAL = outer.SERIAL

            def _handle(self, pkt):
                if 0x32 <= pkt.cmd <= 0x3F:
                    outer._handle_bridge(self, pkt)
                else:
                    super()._handle(pkt)

        self._fw = _Fw()
        self.link = self._fw.link

    @property
    def mcu_time_us(self) -> int:
        return self._fw.mcu_time_us

    def close(self) -> None:
        self._fw.close()

    def _handle_bridge(self, fw, pkt) -> None:
        cmd, payload = pkt.cmd, pkt.payload
        fw.mcu_time_us += 120  # one SPI transaction
        if cmd == 0x32:  # READ_REG
            if len(payload) != 4:
                fw._status(cmd, self.ERR_PAYLOAD_FORMAT)
                return
            addr, n = struct.unpack("<HH", payload)
            if not 1 <= n <= self.READ_MAX:
                fw._status(cmd, self.ERR_PAYLOAD_FORMAT)
                return
            if self.streaming:
                self.bus_ops_while_streaming += 1
            data = self.sensor.rd_multi(addr, n)
            if self.truncate_reads:
                data = data[:-1]
            fw._send(0x91, struct.pack("<BQ", cmd, fw.mcu_time_us) + data)
        elif cmd == 0x33:  # WRITE_REG
            if len(payload) < 3 or len(payload) - 2 > self.WRITE_MAX:
                fw._status(cmd, self.ERR_PAYLOAD_FORMAT)
                return
            if self.streaming:
                self.bus_ops_while_streaming += 1
            (addr,) = struct.unpack_from("<H", payload)
            self.sensor.wr_multi(addr, payload[2:])
            fw._status(cmd, 0x00)
        elif cmd == 0x35:  # START_STREAM
            if len(payload) != 2:
                fw._status(cmd, self.ERR_PAYLOAD_FORMAT)
                return
            (frame_size,) = struct.unpack("<H", payload)
            if not 1 <= frame_size <= 8192:
                fw._status(cmd, self.ERR_INVALID_PARAM)
                return
            self.frame_size = frame_size
            self.streaming = True
            fw._status(cmd, 0x00)
        elif cmd == 0x36:  # STOP_STREAM (idempotent)
            self.streaming = False
            fw._status(cmd, 0x00)
        else:
            self._handle_board(fw, cmd, payload)

    def _handle_board(self, fw, cmd: int, payload: bytes) -> None:
        """Bridge commands beyond contract 04 (the I2C board adds three)."""
        fw._status(cmd, self.ERR_INVALID_CMD)

    def send_frame(self, *, chunk_max: int = 1528) -> dict:
        """One INT edge: the sensor's next frame as RPT_VL53_FRAME chunk(s),
        timestamps spaced by the configured ranging period. Returns the
        values a correct host parses (`FakeVl53l8Sensor.expected_frame`)."""
        if not (self.streaming and self.sensor.ranging):
            raise RuntimeError("not streaming: START_STREAM + a ranging sensor first")
        expected = self.sensor.expected_frame()
        raw = self.sensor.build_frame()
        if len(raw) != self.frame_size:
            raise AssertionError(f"frame {len(raw)} B but START_STREAM said {self.frame_size}")
        self._fw.mcu_time_us += 1_000_000 // max(1, self.sensor.frequency_hz)
        expected["timestamp_us"] = self._fw.mcu_time_us
        for off in range(0, len(raw), chunk_max):
            self.send_chunk(expected["timestamp_us"], len(raw), off, raw[off : off + chunk_max])
        self.frames_sent += 1
        return expected

    def send_chunk(self, timestamp_us: int, full_size: int, offset: int, data: bytes) -> None:
        """One raw RPT_VL53_FRAME chunk (reassembly-rule tests)."""
        self.chunks_sent += 1
        self._fw._send(0x93, struct.pack("<QHH", timestamp_us, full_size, offset) + bytes(data))

    def send_report(self, report_id: int, payload: bytes) -> None:
        """Any unsolicited report, raw."""
        self._fw._send(report_id, payload)
