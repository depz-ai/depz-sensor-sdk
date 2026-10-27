"""In-process fake APP_BNO055 register bridge + a register-level model of the
BNO055 behind it, over a LoopbackLink pair (contract 13).

The bridge side answers READ_REG (RPT_BNO_REG_DATA or RPT_STATUS error),
WRITE_REG / RESET / START_STREAM / STOP_STREAM (RPT_STATUS) and GET_INFO
(RPT_BNO_INFO). The sensor side models what the host logic depends on: two
register pages selected by PAGE_ID, "only CONFIG ↔ mode transitions exist"
(§4.1), suspend ignoring OPR_MODE writes (§4.6), the zero-quaternion gap
after entering fusion (§5), the self-test leftover SYS_STATUS 4 (§5),
SYS_TRIGGER CLK_SEL and INT_STA clear-on-read. Mirrors the TS fake
(packages/depz-sensor-sdk-ts/test/fake-bno055.ts).
"""

from __future__ import annotations

import struct
import threading
from dataclasses import dataclass

from depz_sensor_sdk.errors import DeviceLostError, LinkClosedError
from depz_sensor_sdk.transport import CrcType, Packet, PacketParser, build_packet
from depz_sensor_sdk.transport.link import LoopbackLink

OPR_MODE = 0x3D
PWR_MODE = 0x3E
SYS_TRIGGER = 0x3F
SYS_STATUS = 0x39
SYS_ERR = 0x3A
PAGE_ID = 0x07
QUA_DATA = 0x20
INT_STA = 0x37
ST_RESULT = 0x36

#: RPT_BNO_INFO of the live board (vectors/bno055.json info_live_board).
LIVE_INFO_HEX = "28a0fb320f1103150100000000006f012b09d105000000000000000000000000000000007d00"
#: Quaternion a running fusion outputs (vectors stream_20_8).
LIVE_QUAT = bytes.fromhex("bd2963f9efcf0000")


@dataclass
class RegOp:
    op: str  # "r" | "w"
    page: int
    addr: int
    data: bytes  # bytes written, or `length` zero bytes for a read


class FakeBno055:
    def __init__(self):
        self.link, self._side = LoopbackLink.pair()
        self.regs = [bytearray(256), bytearray(256)]  # page 0 / page 1
        self.sent: list[tuple[int, bytes]] = []  # every host command, in order
        self.ops: list[RegOp] = []  # every register access, with the page in force
        self.mcu_time_us = 5_000_000
        self.info = bytes.fromhex(LIVE_INFO_HEX)
        self.fail_next: dict[int, list[int]] = {}  # opcode -> RPT_STATUS codes to answer with
        self.fail_write_reg: dict[int, int] = {}  # page-0 addr -> status, once
        self.silent: set[int] = set()  # opcodes never answered (host times out)
        self.short_read_by = 0  # next READ_REG answers this many bytes fewer
        self.sys_status_script: list[int] = []  # consumed one per read before the model
        self.fusion_zero_reads_after_switch = 2  # QUA_DATA reads still zero after CONFIG → fusion
        self.fusion_stuck = False  # fusion never starts
        self.fusion_quat = LIVE_QUAT
        self.self_test_result = 0x0F
        self.self_test_error = 0
        self.streams: list[bytes] = []  # START_STREAM payloads accepted
        self.streaming = False
        self.reset_count = 0
        self._fusion_zero_left = 0
        self._self_test_leftover = False
        self._tx_seq = 0
        self._lock = threading.Lock()
        self._parser = PacketParser()
        self.power_on()
        self._thread = threading.Thread(target=self._run, daemon=True)
        self._thread.start()

    def close(self):
        self.link.close()

    # ── model controls ───────────────────────────────────────────────────────

    def power_on(self) -> None:
        """Power-on register values (contract 13 §2 / §4.2)."""
        for page in self.regs:
            page[:] = bytes(256)
        self.regs[0][0:7] = bytes([0xA0, 0xFB, 0x32, 0x0F, 0x11, 0x15, 0x03])
        self.regs[0][0x3B] = 0x80  # UNIT_SEL: Android orientation
        self.regs[0][0x41] = 0x24  # AXIS_MAP_CONFIG P1
        self.regs[0][ST_RESULT] = 0x0F
        self.regs[1][0x50:0x60] = bytes(0xC0 + i for i in range(16))  # UNIQUE_ID
        self.regs[1][0x08] = 0x0D  # ACC_CONFIG reset value
        self.regs[1][0x09] = 0x6D
        self.regs[1][0x0A] = 0x38
        self._self_test_leftover = False
        self._fusion_zero_left = 0

    @property
    def page(self) -> int:
        return self.regs[0][PAGE_ID] & 1

    @property
    def opr_mode(self) -> int:
        return self.regs[0][OPR_MODE] & 0x0F

    def set_mode(self, mode: int) -> None:
        """Put the model into an operating mode directly (a previous session left it there)."""
        self.regs[0][OPR_MODE] = mode
        if mode >= 8:
            self.regs[0][QUA_DATA : QUA_DATA + 8] = self.fusion_quat

    def writes(self) -> list[str]:
        """Writes only, as "page:addr=hex" — the shape most assertions want."""
        return [f"{o.page}:{o.addr:02x}={o.data.hex()}" for o in self.ops if o.op == "w"]

    def reads(self, addr: int) -> int:
        return sum(1 for o in self.ops if o.op == "r" and o.addr == addr)

    def push_stream(
        self, addr: int, data: bytes, timestamp_us: int | None = None, len_field: int | None = None
    ) -> None:
        """Emit RPT_BNO_REG_STREAM."""
        ts = self.mcu_time_us if timestamp_us is None else timestamp_us
        n = len(data) if len_field is None else len_field
        self._send(0x93, struct.pack("<QBB", ts, addr, n) + bytes(data))

    def push_raw(self, cmd: int, payload: bytes) -> None:
        self._send(cmd, payload)

    # ── device-side TX ───────────────────────────────────────────────────────

    def _send(self, cmd: int, payload: bytes = b"") -> None:
        try:
            with self._lock:
                self._side.write(build_packet(cmd, payload, self._tx_seq, CrcType.NONE))
                self._tx_seq = (self._tx_seq + 1) & 0xFF
        except LinkClosedError:
            return

    def _status(self, cmd: int, status: int) -> None:
        self._send(0x80, bytes((cmd, status)))

    # ── device-side RX loop ──────────────────────────────────────────────────

    def _run(self) -> None:
        while True:
            try:
                data = self._side.read(timeout=1.0)
            except (LinkClosedError, DeviceLostError):
                return
            for ev in self._parser.feed(data):
                if isinstance(ev, Packet):
                    self._handle(ev)

    def _handle(self, pkt: Packet) -> None:
        cmd, payload = pkt.cmd, bytes(pkt.payload)
        self.sent.append((cmd, payload))
        if cmd in self.silent:
            return
        fails = self.fail_next.get(cmd)
        if fails:
            self._status(cmd, fails.pop(0))
            return
        self.mcu_time_us += 1000
        if cmd == 0x32:  # READ_REG
            if len(payload) != 2:
                return self._status(cmd, 0x03)
            addr, n = payload
            if n < 1 or n > 128 or addr + n > 0x100:
                return self._status(cmd, 0x04)
            self.ops.append(RegOp("r", self.page, addr, bytes(n)))
            data = self._read(addr, n)
            if self.short_read_by > 0:
                data = data[: max(0, len(data) - self.short_read_by)]
                self.short_read_by = 0
            return self._send(0x91, struct.pack("<BQ", cmd, self.mcu_time_us) + data)
        if cmd == 0x33:  # WRITE_REG
            if not 2 <= len(payload) <= 129:
                return self._status(cmd, 0x03)
            addr, data = payload[0], payload[1:]
            if addr + len(data) > 0x100:
                return self._status(cmd, 0x04)
            if self.page == 0 and addr in self.fail_write_reg:
                return self._status(cmd, self.fail_write_reg.pop(addr))
            self.ops.append(RegOp("w", self.page, addr, data))
            self._write(addr, data)
            return self._status(cmd, 0x00)
        if cmd == 0x34:  # RESET
            self.reset_count += 1
            self.streaming = False
            self.power_on()
            return self._status(cmd, 0x00)
        if cmd == 0x35:  # START_STREAM
            if len(payload) != 5:
                return self._status(cmd, 0x03)
            self.streams.append(payload)
            self.streaming = True
            return self._status(cmd, 0x00)
        if cmd == 0x36:  # STOP_STREAM
            self.streaming = False
            return self._status(cmd, 0x00)
        if cmd == 0x37:  # GET_INFO
            return self._send(0x92, self.info)
        return self._status(cmd, 0x02)

    # ── sensor model ─────────────────────────────────────────────────────────

    def _read(self, addr: int, n: int) -> bytes:
        page = self.page
        out = bytearray(self.regs[page][addr : addr + n])
        if page == 0:
            end = addr + n
            if addr <= SYS_STATUS < end:
                out[SYS_STATUS - addr] = self._sys_status()
            if addr <= QUA_DATA < end and self._fusion_zero_left > 0:
                self._fusion_zero_left -= 1
                lo, hi = QUA_DATA - addr, min(n, QUA_DATA - addr + 8)
                out[lo:hi] = bytes(hi - lo)
                if self._fusion_zero_left == 0 and not self.fusion_stuck:
                    self.regs[0][QUA_DATA : QUA_DATA + 8] = self.fusion_quat
            if addr <= INT_STA < end:
                self.regs[0][INT_STA] = 0  # clears on read
        return bytes(out)

    def _sys_status(self) -> int:
        if self.sys_status_script:
            return self.sys_status_script.pop(0)
        if self._self_test_leftover:
            return 4
        mode = self.opr_mode
        return 0 if mode == 0 else 5 if mode >= 8 else 6

    def _write(self, addr: int, data: bytes) -> None:
        page = self.page
        for i, v in enumerate(data):
            reg = addr + i
            if page == 0 and reg == OPR_MODE:
                self._write_opr_mode(v & 0x0F)
            elif page == 0 and reg == SYS_TRIGGER:
                self._write_sys_trigger(v)
            elif reg == PAGE_ID:
                self.regs[0][PAGE_ID] = v & 1  # PAGE_ID is on both pages
            else:
                self.regs[page][reg] = v

    def _write_opr_mode(self, mode: int) -> None:
        current = self.opr_mode
        suspended = (self.regs[0][PWR_MODE] & 3) == 2
        if mode != 0 and (current != 0 or suspended):
            return  # silently ignored (§4.1, §4.6)
        self.regs[0][OPR_MODE] = mode
        if mode == 0:
            self.regs[0][0x08:0x34] = bytes(0x34 - 0x08)  # CONFIG reads zero everywhere
        else:
            self._self_test_leftover = False
            if mode >= 8:
                self._fusion_zero_left = (
                    1 << 60 if self.fusion_stuck else self.fusion_zero_reads_after_switch
                )
                if self._fusion_zero_left == 0:
                    self.regs[0][QUA_DATA : QUA_DATA + 8] = self.fusion_quat

    def _write_sys_trigger(self, v: int) -> None:
        self.regs[0][SYS_TRIGGER] = v & 0x80  # CLK_SEL sticks, action bits self-clear
        if v & 0x01 and self.opr_mode == 0:
            self.regs[0][ST_RESULT] = self.self_test_result
            self.regs[0][SYS_ERR] = self.self_test_error
            self._self_test_leftover = True
        if v & 0x40:
            self.regs[0][INT_STA] = 0
