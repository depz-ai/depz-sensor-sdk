"""Register behaviour of a VL53L0X die behind the bridge fake (fake_vl53lx.py):
what the VL53L0X ULD port (uld/l0x.py) needs to run its init, calibrations and
ranging profiles with no hardware.

The bridge fake is a flat register file; this plugs in as its `die` and keeps
one 256-byte bank per page (register 0xFF selects it, as on the die), with
the registers that *behave* modelled on top: the soft reset and the NACKs
around it, the NVM strobe and data, the single-shot start bit, data-ready and
interrupt clear, the result block of each shot and the reference-signal
measurement the SPAD management converges on.

The defaults are what the live VL53L0X board (TB9BGETA6M) answered in the
committed capture vectors/recordings/vl53l0x_uld_long-range_33ms.depzrec:
stop variable 0x3C, oscillator word 0x975C, NVM reference-SPAD record "6
non-aperture", good SPAD map FB FF FF DF B7 0F, VHV 0x22, phase 0x01,
OSC_CALIBRATE_VAL 0x28, one NACK on the model id after a soft reset.
"""

from __future__ import annotations

from collections import defaultdict, deque

from fake_vl53lx import DieNack, FakeVl53lxBridge

L0X_BOARD = "DEPZ ToF Sensor VL53L0X USB v2.1 TB9BGETA6M"
MODEL_ID = 0xEE
START_SELECT = 0xB4             # GLOBAL_CONFIG_REF_EN_START_SELECT
FIRST_APERTURE_SPAD = 192       # SPAD indices from here lie in an aperture quadrant

# Result blocks (12 B at 0x14) of the capture: the first frame of a run, no
# target yet (8191 mm, device status 0x58), and a flat target at 635 mm
# (status 0x5A: internal 11, signal 0x0271 in 9.7, ambient 0x022B, 0x06C1
# effective SPADs in 8.8).
FRAME_NO_TARGET = bytes.fromhex("5802ffffffffffffffff1fff")
FRAME_635_MM = bytes.fromhex("5a06c1040071022b011b027b")

# NVM words (index -> dword) of the capture plus the identification words the
# driver can ask for with options 2 and 4 (product id, module id / revision,
# UID, the 400 mm factory measurement).
NVM_DEFAULTS = {
    0x6B: 0x1D80060B,   # reference SPADs: (>>8)&0x7F = 6, (>>15)&1 = 0 (non-aperture)
    0x24: 0xFBFFFFDF,   # good SPAD map bytes 0..3
    0x25: 0xB70F0000,   # good SPAD map bytes 4..5
    0x02: 0x2A000000,   # module id (first byte)
    0x7B: 0x0C345678,   # revision (first byte) / part UID upper
    0x7C: 0x9ABCDEF0,   # part UID lower
    0x73: 0x000000A5,   # signal rate at 400 mm, high byte
    0x74: 0x3C000000,   # ... low byte -> 0xA53C in 9.7
    0x75: 0x00000019,   # distance at 400 mm (1/16 mm), high byte
    0x76: 0x40000000,   # ... low byte -> 0x1940 = 6464 = 404 mm
}
PRODUCT_ID = "VL53L0X-TEST-18CHR"


def nvm_product_id(text: str) -> dict:
    """The four NVM words 0x77..0x7A holding an 18-character product id:
    7 bits per character, MSB first, two padding bits at the end."""
    assert len(text) == 18
    bits = "".join(f"{ord(c) & 0x7F:07b}" for c in text) + "00"
    return {0x77 + i: int(bits[32 * i:32 * i + 32], 2) for i in range(4)}


class FakeVl53l0xDie:
    """Plug into a FakeVl53lxBridge as its `die`."""

    def __init__(self, bridge: FakeVl53lxBridge, *, nvm: dict | None = None):
        bridge.die = self
        self.page = 0
        self.banks: dict[int, bytearray] = defaultdict(lambda: bytearray(256))
        self.nvm = dict(NVM_DEFAULTS)
        self.nvm.update(nvm_product_id(PRODUCT_ID))
        if nvm:
            self.nvm.update(nvm)

        # ── knobs ──
        self.frames: deque[bytes] = deque()   # result block per ranging shot; the last one repeats
        self.frame = FRAME_635_MM
        self.ref_rate_per_spad = 400          # 9.7 Mcps each enabled non-aperture reference SPAD adds
        self.ref_rate_per_aperture_spad = 100  # ... each aperture one
        self.ready_after_polls = 0            # RESULT_INTERRUPT_STATUS polls before a shot is ready
        self.start_bit_reads = 0              # SYSRANGE_START reads that still show the start bit
        self.start_bit_stuck = False
        self.nacks_after_reset = 1
        self.reset_ignored = False            # the soft reset does nothing
        self.strobe_stuck = False             # the NVM read strobe never comes
        self.interrupt_sticky = False         # SYSTEM_INTERRUPT_CLEAR does not clear
        self.spad_map_stuck = False           # writes to the SPAD enables are lost
        self.interrupt_idle = 0x40            # RESULT_INTERRUPT_STATUS with nothing pending

        # ── what happened ──
        self.shots: list[tuple[int, int]] = []   # (SYSRANGE_START value, sequence config)
        self.ref_peaks: list[int] = []           # reference rate of each reference shot

        self._in_reset = False
        self._nacks = 0
        self._start_reads = 0
        self._ready_in: int | None = None       # polls until ready; None = nothing pending

        b0 = self.banks[0]
        b0[0xC0] = MODEL_ID
        b0[0x84] = 0x01                 # GPIO_HV_MUX_ACTIVE_HIGH as read on the board
        b0[0xCB] = 0x22                 # VHV settings after the reference calibration
        b0[0xEE] = 0x01                 # phase cal
        b0[0xF8:0xFA] = b"\x00\x28"     # OSC_CALIBRATE_VAL
        b0[0x14:0x20] = self.frame
        b1 = self.banks[1]
        b1[0x91] = 0x3C                 # the stop variable DataInit reads
        b1[0x84:0x86] = b"\x97\x5C"     # oscillator frequency, FixPoint4.12
        self.banks[6][0x83] = 0x01

    # ── register map ──
    def spad_enables(self) -> bytes:
        return bytes(self.banks[0][0xB0:0xB6])

    def read(self, addr: int, n: int) -> bytes:
        page, bank = self.page, self.banks[self.page]
        if page == 0 and addr == 0xC0:
            if self._nacks:
                self._nacks -= 1
                raise DieNack
            return bytes([0x00 if self._in_reset else MODEL_ID]) + bytes(bank[0xC1:0xC0 + n])
        if page == 7 and addr == 0x83:
            return b"\x00" if self.strobe_stuck else b"\x10"
        if page == 7 and addr == 0x90:
            return self.nvm.get(bank[0x94], 0).to_bytes(4, "big")[:n]
        if page == 0 and addr == 0x00:
            if self._start_reads or self.start_bit_stuck:
                self._start_reads = max(0, self._start_reads - 1)
                return bytes([bank[0x00] | 0x01])
            return bytes([bank[0x00] & 0xFE])
        if page == 0 and addr == 0x13:
            ready = self._ready_in == 0
            if self._ready_in:
                self._ready_in -= 1
            return bytes([self.interrupt_idle | (0x04 if ready else 0)])
        return bytes(bank[addr:addr + n])

    def write(self, addr: int, data: bytes) -> None:
        if addr == 0xFF:
            self.page = data[0]
            return
        page, bank = self.page, self.banks[self.page]
        if page == 0 and addr == 0xBF and not self.reset_ignored:
            if data[0] == 0x00:
                self._in_reset = True
            else:
                self._in_reset = False
                self._nacks = self.nacks_after_reset
        elif page == 0 and addr == 0x00 and data[0] & 0x01:
            self._shot(data[0])
        elif page == 0 and addr == 0x0B and data[0] == 0x01 and not self.interrupt_sticky:
            self._ready_in = None
        elif page == 0 and 0xB0 <= addr <= 0xB5 and self.spad_map_stuck:
            return
        bank[addr:addr + len(data)] = data

    # ── a measurement ──
    def _shot(self, value: int) -> None:
        sequence = self.banks[0][0x01]
        self.shots.append((value, sequence))
        self._start_reads = self.start_bit_reads
        self._ready_in = self.ready_after_polls
        if sequence == 0xC0:
            # Reference-signal measurement: the peak reference rate follows
            # the SPAD enables.
            peak = self._ref_peak()
            self.ref_peaks.append(peak)
            self.banks[1][0xB6:0xB8] = peak.to_bytes(2, "big")
        elif sequence not in (0x01, 0x02):      # VHV / phase shots range nothing
            if self.frames:
                self.frame = self.frames.popleft()
            self.banks[0][0x14:0x20] = self.frame

    def _ref_peak(self) -> int:
        enables = self.spad_enables()
        peak = 0
        for index in range(8 * len(enables)):
            if (enables[index // 8] >> (index % 8)) & 1:
                aperture = START_SELECT + index >= FIRST_APERTURE_SPAD
                peak += (self.ref_rate_per_aperture_spad if aperture
                         else self.ref_rate_per_spad)
        return min(peak, 0xFFFF)


def l0x_rig(board: str = L0X_BOARD, **die_kw) -> tuple[FakeVl53lxBridge, FakeVl53l0xDie]:
    fake = FakeVl53lxBridge(board)
    return fake, FakeVl53l0xDie(fake, **die_kw)
