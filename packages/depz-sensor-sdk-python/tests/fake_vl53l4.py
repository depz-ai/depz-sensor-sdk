"""Fake VL53L4CD register platform for host-side ULD tests (no hardware).

A plain byte-addressed register map seeded with the boot/identity/oscillator
values a real sensor answers with, so the real register sequences in
vl53l4/uld.py run end to end. The default configuration block leaves the
sensor "data ready" (0x0030 = 0x11 → int_pol 0; 0x0031 bit 0 = 0), which is
exactly what sensor_init()'s VHV wait needs.
"""

from __future__ import annotations

from depz_sensor_sdk.vl53l4 import uld
from depz_sensor_sdk.vl53l4.uld import VL53L4CD

# Typical silicon values (also frozen in contracts/vectors/vl53l4.json).
OSC_FREQUENCY_TYPICAL = 0x3980
CLOCK_PLL_TYPICAL = 0x0A5C


class FakeVl53l4Platform:
    """Emulates the I2C register bridge for uld.VL53L4CD."""

    def __init__(self):
        self.reg: dict[int, int] = {}
        self.writes: list[tuple[int, bytes]] = []
        self.speeds: list[int] = []  # set_i2c_speed history
        # booted sensor with its identity and oscillator words
        self.reg[uld.FIRMWARE__SYSTEM_STATUS] = 0x03
        self._seed_word(uld.IDENTIFICATION__MODEL_ID, uld.MODEL_ID_VL53L4CD)
        self._seed_word(uld.OSC_FREQUENCY, OSC_FREQUENCY_TYPICAL)
        self._seed_word(uld.RESULT__OSC_CALIBRATE_VAL, CLOCK_PLL_TYPICAL)

    def _seed_word(self, addr: int, value: int) -> None:
        self.reg[addr] = (value >> 8) & 0xFF
        self.reg[addr + 1] = value & 0xFF

    def rd_multi(self, addr: int, size: int) -> bytes:
        return bytes(self.reg.get(addr + i, 0) for i in range(size))

    def wr_multi(self, addr: int, data: bytes) -> None:
        data = bytes(data)
        self.writes.append((addr, data))
        for i, b in enumerate(data):
            self.reg[addr + i] = b

    def set_i2c_speed(self, khz: int) -> None:
        self.speeds.append(khz)

    def sleep_ms(self, ms: int) -> None:
        pass

    def written_to(self, addr: int) -> list[bytes]:
        """Every write that started exactly at `addr`, in order."""
        return [data for a, data in self.writes if a == addr]


def make_driver() -> tuple[VL53L4CD, FakeVl53l4Platform]:
    p = FakeVl53l4Platform()
    return VL53L4CD(p), p
