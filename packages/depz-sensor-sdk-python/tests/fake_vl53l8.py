"""Fake VL53L8 register/DCI platform for host-side ULD tests (no hardware).

Emulates the register bridge + DCI transport (swap_buffer + header/footer
framing) so the real byte sequences in uld.py are exercised end to end.
"""

from __future__ import annotations

from depz_sensor_sdk.vl53l8 import uld
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
