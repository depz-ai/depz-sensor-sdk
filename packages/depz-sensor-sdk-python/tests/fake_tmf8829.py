"""In-process fake TMF8829 SPI register bridge + sensor over a LoopbackLink
(contracts/14_SENSOR_TMF8829.md §2, §4). The TypeScript test/fake-tmf8829.ts
is the same fake.

The bridge part answers TMF_READ_REG / TMF_WRITE_REG / TMF_START_STREAM /
TMF_STOP_STREAM / TMF_GET_INFO. The sensor part is a 256-byte register file
with the side effects the host sequence relies on: soft reset → bootloader,
bootloader commands at CMD_STAT (I2C_OFF, W_FIFO_BOTH, START_RAM_APP), the
firmware download through FIFO 0xFF, application commands (presets,
LOAD_CONFIG_PAGE, WRITE_PAGE, MEASURE, STOP) and the config page 0x22..0xDF
with the "preset does not reset everything" behaviour. Every command the host
sends is logged (`log`) so tests can assert the wire bytes.

Frames are pushed by the test (`emit_frame`, `emit_event`), chunked exactly as
the bridge does (≤1024 data bytes per RPT_TMF_FRAME).
"""

from __future__ import annotations

import struct
import threading
from typing import Callable

from depz_sensor_sdk.errors import DeviceLostError, LinkClosedError
from depz_sensor_sdk.transport import CrcType, Packet, PacketParser, build_packet
from depz_sensor_sdk.transport.link import LoopbackLink

REG_APP_ID = 0x00
REG_CMD_STAT = 0x08
REG_SERIAL = 0x1C
REG_CID_RID = 0x20
REG_CFG = 0x22
CFG_SIZE = 190
REG_INT_STATUS = 0xE1
REG_ID = 0xE3
REG_RESET = 0xF7
REG_ENABLE = 0xF8
REG_FIFOSTATUS = 0xFA
REG_FIFO = 0xFF

#: fp_mode each preset command loads
PRESET_FP_MODE = {0x40: 0, 0x41: 0, 0x42: 0, 0x43: 2, 0x44: 2, 0x45: 3, 0x46: 3,
                  0x47: 5, 0x48: 5, 0x49: 0}


class FakeTmf8829:
    def __init__(self):
        self.link, self._side = LoopbackLink.pair()
        self.log: list[tuple[int, bytes]] = []  # everything the host sent, in order
        self.regs = bytearray(256)  # sensor register file
        self.cfg = bytearray(CFG_SIZE)  # what LOAD_CONFIG_PAGE copies to 0x22..
        self.fifo = bytearray()  # bytes written to the FIFO port
        self.w_fifo_params: bytes | None = None  # W_FIFO_BOTH params as received
        self.mode = "app-old"  # "app-old" | "bootloader" | "app"
        self.i2c_off = False
        self.app_version = (2, 200, 0)
        self.serial = 0x12345678

        # ── fault injection ──
        self.device_id = 0x9E  # value ID (0xE3) reads
        self.bl_status: dict[int, int] = {}  # bootloader command → status (default 0 READY)
        self.app_status: dict[int, int] = {}  # app command → status (default OK; MEASURE → ACCEPTED)
        self.busy_polls = 0  # CMD_STAT reads the command byte this many more times
        self.stuck_before_bootloader = False  # APP_ID stays 0 after a soft reset
        self.cid_override: int | None = None  # CID_RID after LOAD_CONFIG_PAGE (None = 0x16)
        self.stuck_bytes: dict[int, int] = {}  # page offset → value WRITE_PAGE keeps instead
        self.bridge_status: dict[int, int] = {}  # bridge command → status instead of the reply
        self.silent: set[int] = set()  # bridge commands left unanswered
        self.short_read = 0  # READ_REG answers this many bytes fewer
        #: called after a CMD_STAT write is applied, before its WRITE_REG reply
        self.on_command: Callable[[int], None] | None = None
        self.streaming = False
        self.int_mask = 0
        self.flags = 0

        self._pending_cmd = 0
        self._tx_seq = 0
        self._frame_seq = 0
        # Power-on: an application left running by an earlier session.
        self.regs[REG_APP_ID] = 0x01
        # A non-default config page so tests can tell what configure() wrote.
        self.cfg[0] = 33  # period_ms
        self.cfg[2:4] = (586).to_bytes(2, "little")  # kilo_iterations
        self.cfg[0x2A - REG_CFG] = 1  # nr_peaks 1
        self._parser = PacketParser()
        self._thread = threading.Thread(target=self._run, daemon=True)
        self._thread.start()

    def close(self):
        self._side.close()

    def sent(self, cmd: int) -> list[bytes]:
        """Payloads of one command id, in order."""
        return [p for c, p in self.log if c == cmd]

    def writes(self) -> list[list[int]]:
        """WRITE_REG payloads as [addr, *data]."""
        return [list(p) for p in self.sent(0x33)]

    # ── unsolicited reports ──────────────────────────────────────────────────

    def emit_frame(self, raw: bytes, timestamp_us: int, edge_us: int = 0, *,
                   seq: int | None = None, chunks: list[int] | None = None,
                   full_size: int | None = None) -> int:
        """One frame as RPT_TMF_FRAME chunks (≤1024 B each). Returns the frame_seq used."""
        seq = self._frame_seq if seq is None else seq
        self._frame_seq = (seq + 1) & 0xFF
        offsets = chunks if chunks is not None else list(range(0, len(raw), 1024))
        for i, off in enumerate(offsets):
            end = offsets[i + 1] if i + 1 < len(offsets) else len(raw)
            data = raw[off:min(end, off + 1024)]
            head = struct.pack("<QHHBQ", timestamp_us,
                               len(raw) if full_size is None else full_size, off, seq, edge_us)
            self._send(0x93, head + data)
        return seq

    def emit_event(self, code: int, int_status: int, frame_id: int = 0,
                   fifo_status: int = 0, ts: int = 987654321) -> None:
        self._send(0x94, struct.pack("<QBBBB", ts, int_status, code, frame_id, fifo_status))

    def emit_raw(self, cmd: int, payload: bytes) -> None:
        """Any raw report (malformed-input tests)."""
        self._send(cmd, payload)

    # ── transport ────────────────────────────────────────────────────────────

    def _send(self, cmd: int, payload: bytes = b"") -> None:
        try:
            self._side.write(build_packet(cmd, payload, self._tx_seq, CrcType.NONE))
        except LinkClosedError:
            return
        self._tx_seq = (self._tx_seq + 1) & 0xFF

    def _status(self, cmd: int, st: int) -> None:
        self._send(0x80, bytes((cmd, st)))

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
        self.log.append((cmd, payload))
        if cmd in self.silent:
            return
        if cmd in self.bridge_status:
            self._status(cmd, self.bridge_status[cmd])
            return
        if cmd == 0x32:
            addr, n = struct.unpack("<BH", payload)
            data = self._read_regs(addr, max(0, n - self.short_read))
            self._send(0x91, struct.pack("<BQ", 0x32, 5_000_000) + data)
        elif cmd == 0x33:
            self._write_regs(payload[0], payload[1:])
            if payload[0] == REG_CMD_STAT and self.on_command:
                self.on_command(payload[1])
            self._status(cmd, 0x00)
        elif cmd == 0x35:
            self.streaming = True
            self.int_mask, self.flags = payload[0], payload[1]
            self._frame_seq = 0
            self._status(cmd, 0x00)
        elif cmd == 0x36:
            self.streaming = False
            self._status(cmd, 0x00)
        elif cmd == 0x37:
            self._send(0x92, struct.pack("<IIIIIBBBBB", 7, 6, 1, 0, 0, 1,
                                         int(self.streaming), self.int_mask, 0x01, 0x00))
        else:
            self._status(cmd, 0x02)  # ERR_INVALID_CMD

    # ── sensor model ─────────────────────────────────────────────────────────

    def _read_regs(self, addr: int, n: int) -> bytes:
        return bytes(self._read8(min(addr + i, REG_FIFO)) for i in range(n))

    def _read8(self, a: int) -> int:
        if a == REG_ID:
            return self.device_id
        if a == REG_ENABLE:
            # Reads 0 over SPI until the sensor's I2C is off.
            if self.mode == "bootloader" and not self.i2c_off:
                return 0
            return self.regs[a] | 0x80
        if a == REG_CMD_STAT and self.busy_polls > 0:
            self.busy_polls -= 1
            return self._pending_cmd
        return self.regs[a]

    def _write_regs(self, addr: int, data: bytes) -> None:
        if addr == REG_FIFO:
            self.fifo += data
            return
        if addr == REG_CMD_STAT:
            self._command(data)
            return
        for i, b in enumerate(data):
            a = addr + i
            if a == REG_RESET and b & 0x40:
                self._soft_reset()
            elif a == REG_INT_STATUS:
                self.regs[a] &= ~b  # write-1-to-clear
            elif a == REG_FIFOSTATUS:
                self.regs[a] &= ~(b & 0x30)
            else:
                self.regs[a] = b

    def _soft_reset(self) -> None:
        self.mode = "bootloader"
        self.i2c_off = False
        self.regs[REG_APP_ID] = 0x00 if self.stuck_before_bootloader else 0x80
        self.regs[REG_CMD_STAT] = 0
        self.fifo = bytearray()
        # init() defaults (contract 14 §4 table): dump_histograms / ref_spad_frame kept.
        self.cfg[0], self.cfg[1] = 33, 0
        self.cfg[0x2A - REG_CFG] = 1

    def _command(self, data: bytes) -> None:
        cmd = data[0]
        self._pending_cmd = cmd
        if self.mode == "bootloader":
            st = self.bl_status.get(cmd, 0)
            if st == 0:
                if cmd == 0x22:
                    self.i2c_off = True
                elif cmd == 0x45:
                    self.w_fifo_params = bytes(data[2:2 + data[1]])
                elif cmd == 0x16:
                    self.mode = "app"
                    self.regs[REG_APP_ID] = 0x01
                    self.regs[1:4] = bytes(self.app_version)
                    self.regs[REG_SERIAL:REG_SERIAL + 4] = struct.pack("<I", self.serial)
        else:
            st = self.app_status.get(cmd, 0x01 if cmd == 0x10 else 0x00)
            if st <= 0x01:
                self._app_command(cmd)
        self.regs[REG_CMD_STAT] = st

    def _app_command(self, cmd: int) -> None:
        if cmd in PRESET_FP_MODE:
            self.cfg[0x26 - REG_CFG] = PRESET_FP_MODE[cmd]
            self.regs[REG_CID_RID] = cmd
        elif cmd == 0x16:
            self.regs[REG_CFG:REG_CFG + CFG_SIZE] = self.cfg
            self.regs[REG_CID_RID] = 0x16 if self.cid_override is None else self.cid_override
        elif cmd == 0x15:
            self.cfg[:] = self.regs[REG_CFG:REG_CFG + CFG_SIZE]
            for off, v in self.stuck_bytes.items():
                self.cfg[off] = v
