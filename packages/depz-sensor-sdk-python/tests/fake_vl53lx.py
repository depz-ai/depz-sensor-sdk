"""In-process fake of the `APP_VL53L0_4` register bridge (contract 12,
protocol v2.01) over a LoopbackLink. The TypeScript test/fake-vl53lx-bridge.ts
is the same fake.

The MCU side knows no sensor, so neither does this fake: behind the bridge
sits a 64 KiB register file pre-loaded with what a booted VL53L1-family die
answers (boot status 3, oscillator, model id, PLL, ROI centre), enough for the
host ULDs (L4CD ULD, L1X ULD) to run their init, timing, offset, xtalk,
threshold, calibration and ranging sequences against it. The result block
(0x0089, 17 B) and the data-ready bit (0x0031 bit 0) are plain registers; the
INT polarity bit of 0x0030 is set by the blob.

Every packet the host sends is logged (`rx`), so tests assert the exact wire
bytes. `fail_next(cmd, status)` answers the next `cmd` with an error status
(or ``"silent"``: no reply), `stream()` pushes an RPT_VL53_STREAM.

A `die` model (fake_vl53l0x.py) can take over the register file: the bridge
hands it every READ_REG / WRITE_REG, and a `DieNack` it raises goes back as
ERR_HARDWARE_FAULT, the status a NACKing sensor produces.
"""

from __future__ import annotations

import struct
import threading
from collections import defaultdict, deque

from depz_sensor_sdk.errors import DeviceLostError, LinkClosedError
from depz_sensor_sdk.transport import CrcType, Packet, PacketParser, build_packet
from depz_sensor_sdk.transport.link import LoopbackLink

DIE_MODEL_ID = 0xEBAA
DIE_OSC = 0xBCCC
DIE_PLL = 0x0123

READ_REG, WRITE_REG, XSHUT, START_STREAM, STOP_STREAM = 0x32, 0x33, 0x34, 0x35, 0x36
GET_INFO, SET_I2C_SPEED, SET_ADDR_WIDTH, CLEAR_I2C_ERRORS = 0x37, 0x38, 0x39, 0x3A
GET_DEVICE_NAME = 0x03
ERR_INVALID_CMD = 0x02
ERR_HARDWARE_FAULT = 0x09


class DieNack(Exception):
    """Raised by a die model: the sensor did not ACK the register access."""


class FakeVl53lxBridge:
    SOFTWARE_NAME = "APP_VL53L0_4_v0.25"

    def __init__(self, device_name: str = "DEPZ ToF Sensor VL53L4CD USB v2.1 TESTSERIAL",
                 *, no_clear_i2c_errors: bool = False):
        self.link, self._side = LoopbackLink.pair()
        self.device_name = device_name
        self.no_clear = no_clear_i2c_errors
        self.regs = bytearray(0x10000)
        self.rx: list[tuple[int, bytes]] = []  # every packet the host sent, in order
        self.mcu_time_us = 5_000_000
        self.info = bytes(23)  # RPT_VL53_INFO payload served on GET_INFO
        self.die = None  # optional register-behaviour model (read / write)
        self._failures: dict[int, deque] = defaultdict(deque)
        self.truncate_next_read = 0  # bytes to drop from the next READ_REG answer
        # A booted die.
        self.regs[0x00E5] = 0x03
        self.set_word(0x0006, DIE_OSC)
        self.set_word(0x010F, DIE_MODEL_ID)
        self.set_word(0x00DE, DIE_PLL)
        self.regs[0x013E] = 0x91  # ROI_CONFIG__MODE_ROI_CENTRE_SPAD

        self._tx_seq = 0
        self._parser = PacketParser()
        self._thread = threading.Thread(target=self._run, daemon=True)
        self._thread.start()

    def close(self) -> None:
        self.link.close()

    # ── register file ──
    def set_word(self, addr: int, v: int) -> None:
        self.regs[addr:addr + 2] = v.to_bytes(2, "big")

    def word(self, addr: int) -> int:
        return int.from_bytes(self.regs[addr:addr + 2], "big")

    def dword(self, addr: int) -> int:
        return int.from_bytes(self.regs[addr:addr + 4], "big")

    def set_result_block(self, raw: bytes) -> None:
        """Put a 17-byte result block at 0x0089 (what the next poll reads)."""
        self.regs[0x0089:0x0089 + len(raw)] = raw

    def _read_regs(self, addr: int, n: int) -> bytes:
        if self.die is not None:
            return bytes(self.die.read(addr, n))
        return bytes(self.regs[addr:addr + n])

    def _write_regs(self, addr: int, data: bytes) -> None:
        if self.die is not None:
            self.die.write(addr, data)
            return
        self.regs[addr:addr + len(data)] = data

    # ── traffic ──
    def fail_next(self, cmd: int, status: int | str) -> None:
        self._failures[cmd].append(status)

    def sent(self, cmd: int) -> list[bytes]:
        """Payloads of one opcode, in order."""
        return [p for c, p in self.rx if c == cmd]

    def cmds(self) -> list[int]:
        return [c for c, _ in self.rx]

    def writes(self) -> list[tuple[int, list[int]]]:
        """Every WRITE_REG as (addr, data)."""
        return [(struct.unpack_from("<H", p)[0], list(p[2:])) for p in self.sent(WRITE_REG)]

    def stream(self, data: bytes, ts_us: int, addr: int = 0x0089, length: int | None = None) -> None:
        """RPT_VL53_STREAM: ts u64, addr u16, len u16, data."""
        n = len(data) if length is None else length
        self.send(0x93, struct.pack("<QHH", ts_us, addr, n) + bytes(data))

    def send(self, cmd: int, payload: bytes = b"") -> None:
        try:
            self._side.write(build_packet(cmd, payload, self._tx_seq, CrcType.NONE))
        except LinkClosedError:
            return
        self._tx_seq = (self._tx_seq + 1) & 0xFF

    def _status(self, cmd: int, status: int) -> None:
        self.send(0x80, bytes((cmd, status)))

    def _text(self, cmd: int, text: str) -> None:
        self.send(0x81, bytes((cmd,)) + text.encode() + b"\x00")

    def _run(self) -> None:
        while True:
            try:
                data = self._side.read(timeout=1.0)
            except (LinkClosedError, DeviceLostError):
                return
            for ev in self._parser.feed(data):
                if isinstance(ev, Packet):
                    self._handle(ev.cmd, bytes(ev.payload))

    def _handle(self, cmd: int, p: bytes) -> None:
        self.rx.append((cmd, p))
        fail = self._failures[cmd].popleft() if self._failures[cmd] else None
        if fail == "silent":
            return
        if fail is not None:
            self._status(cmd, fail)
            return
        if cmd == GET_DEVICE_NAME:
            self._text(cmd, self.device_name)
        elif cmd == 0x04:
            self._text(cmd, self.SOFTWARE_NAME)
        elif cmd == READ_REG:
            # READ_REG → RPT_VL53_REG_DATA: echoed opcode, ts u64, data
            addr, n = struct.unpack("<HH", p)
            try:
                data = self._read_regs(addr, n)
            except DieNack:
                self._status(cmd, ERR_HARDWARE_FAULT)
                return
            if self.truncate_next_read:
                data, self.truncate_next_read = data[: len(data) - self.truncate_next_read], 0
            self.send(0x91, struct.pack("<BQ", cmd, self.mcu_time_us) + data)
            self.mcu_time_us += 100
        elif cmd == WRITE_REG:
            (addr,) = struct.unpack_from("<H", p)
            try:
                self._write_regs(addr, p[2:])
            except DieNack:
                self._status(cmd, ERR_HARDWARE_FAULT)
                return
            self._status(cmd, 0)
        elif cmd == GET_INFO:
            self.send(0x92, self.info)
        elif cmd == CLEAR_I2C_ERRORS and self.no_clear:
            self._status(cmd, ERR_INVALID_CMD)
        elif cmd in (XSHUT, START_STREAM, STOP_STREAM, SET_I2C_SPEED, SET_ADDR_WIDTH,
                     CLEAR_I2C_ERRORS):
            self._status(cmd, 0)
        else:
            self._status(cmd, ERR_INVALID_CMD)
