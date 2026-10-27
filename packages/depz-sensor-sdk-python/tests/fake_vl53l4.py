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

    def seed_result_block(self, raw: bytes) -> None:
        """Place a 17-byte result block at 0x0089..0x0099 (what GetResult /
        the stream reads), without logging a host write."""
        for i, b in enumerate(raw):
            self.reg[uld.RESULT_BLOCK_ADDR + i] = b


def result_block(distance_mm: int, *, status: int = 9, sigma_mm: int = 3,
                 signal_kcps: int = 1200, spads: int = 2048, stream_count: int = 1,
                 ambient_kcps: int = 0) -> bytes:
    """A raw 17-byte result block (big-endian registers 0x0089..0x0099).
    `status` is the raw RESULT__RANGE_STATUS (9 → ULD status 0 "valid");
    `spads` the raw RESULT__SPAD_NB word (number_of_spad = spads // 256)."""
    raw = bytearray(17)
    raw[0] = status
    raw[2] = stream_count
    raw[3:5] = spads.to_bytes(2, "big")
    raw[5:7] = (signal_kcps // 8).to_bytes(2, "big")
    raw[7:9] = (ambient_kcps // 8).to_bytes(2, "big")
    raw[9:11] = (sigma_mm * 4).to_bytes(2, "big")
    raw[13:15] = distance_mm.to_bytes(2, "big")
    return bytes(raw)


def make_driver() -> tuple[VL53L4CD, FakeVl53l4Platform]:
    p = FakeVl53l4Platform()
    return VL53L4CD(p), p


# ── wire-level fake (LoopbackLink) ───────────────────────────────────────────


class FakeVl53l4cd:
    """In-process fake VL53L4 bridge firmware speaking the wire protocol
    (contract 10) over a LoopbackLink, registers backed by FakeVl53l4Platform.

    The common commands (names, SYNC_TIME, temperature) come from FakeSr04;
    the bridge commands READ_REG / WRITE_REG / XSHUT / START_STREAM /
    STOP_STREAM / GET_INFO / SET_I2C_SPEED are answered here. `send_result()`
    emits one RPT_VL53_STREAM with a 17-byte result block, as the INT edge
    would. `fail_next(cmd, status)` answers the next `cmd` with that error
    status ("silent": no answer at all); `short_read_next` drops the last byte
    of the next REG_DATA. XSHUT OFF powers the sensor down (register access
    answers ERR_HARDWARE_FAULT, §2); RESET wipes the register file back to
    the boot values. `rx` logs every bridge command received.
    """

    SOFTWARE_NAME = "APP_VL53L4CD_v1.2"
    DEVICE_NAME = "DEPZ VL53L4CD USB v1.0"

    ERR_HARDWARE_FAULT = 0x09

    def __init__(self):
        import struct

        from fake_device import FakeSr04

        self._struct = struct
        self.platform = FakeVl53l4Platform()
        self.streaming = False
        self.stream_addr = 0
        self.stream_len = 0
        self.powered = True  # XSHUT level
        self.int_edges = 0
        self.i2c_errors = 0
        self.rx: list[tuple[int, bytes]] = []
        self.short_read_next = False
        self._failures: dict[int, list] = {}
        outer = self

        class _Fw(FakeSr04):
            SOFTWARE_NAME = outer.SOFTWARE_NAME
            DEVICE_NAME = outer.DEVICE_NAME

            def _handle(self, pkt):
                if 0x03 <= pkt.cmd <= 0x07:
                    super()._handle(pkt)
                else:
                    outer._handle_bridge(self, pkt)

        self._fw = _Fw()
        self.link = self._fw.link

    @property
    def mcu_time_us(self) -> int:
        return self._fw.mcu_time_us

    def close(self) -> None:
        self._fw.close()

    def fail_next(self, cmd: int, status) -> None:
        """Answer the next `cmd` with RPT_STATUS `status`, or nothing at all
        for "silent"."""
        self._failures.setdefault(cmd, []).append(status)

    def cmds(self) -> list[int]:
        return [cmd for cmd, _ in self.rx]

    def set_result_block(self, raw: bytes) -> None:
        self.platform.seed_result_block(raw)

    @property
    def i2c_khz(self) -> int:
        return self.platform.speeds[-1] if self.platform.speeds else uld.I2C_KHZ_BOOT

    def _handle_bridge(self, fw, pkt) -> None:
        cmd, payload, st = pkt.cmd, pkt.payload, self._struct
        self.rx.append((cmd, bytes(payload)))
        queued = self._failures.get(cmd)
        if queued:
            fail = queued.pop(0)
            if fail != "silent":
                fw._status(cmd, fail)
            return
        if cmd in (0x32, 0x33) and not self.powered:
            self.i2c_errors += 1  # an unpowered sensor NACKs (contract 10 §2)
            fw._status(cmd, self.ERR_HARDWARE_FAULT)
        elif cmd == 0x32:  # READ_REG
            addr, n = st.unpack("<HH", payload)
            data = self.platform.rd_multi(addr, n)
            if self.short_read_next:
                self.short_read_next = False
                data = data[:-1]
            fw._send(0x91, st.pack("<BQ", cmd, fw.mcu_time_us) + data)
            fw.mcu_time_us += 100
        elif cmd == 0x33:  # WRITE_REG
            (addr,) = st.unpack_from("<H", payload)
            self.platform.wr_multi(addr, payload[2:])
            fw._status(cmd, 0x00)
        elif cmd == 0x34:  # XSHUT
            (action,) = st.unpack("<B", payload)
            if action == 0:
                self.powered = False
                self.streaming = False
            elif action == 1:
                self.powered = True
            else:  # RESET: boot values only, the ULD configuration is gone
                self.platform.reg = FakeVl53l4Platform().reg
                self.platform.writes.clear()
                self.powered = True
                self.streaming = False
            fw._status(cmd, 0x00)
        elif cmd == 0x35:  # START_STREAM
            self.stream_addr, self.stream_len, _flags = st.unpack("<HHB", payload)
            self.streaming = True
            fw._status(cmd, 0x00)
        elif cmd == 0x36:  # STOP_STREAM
            self.streaming = False
            fw._status(cmd, 0x00)
        elif cmd == 0x37:  # GET_INFO
            model_id = int.from_bytes(self.platform.rd_multi(0x010F, 2), "big") if self.powered else 0
            fw._send(0x92, st.pack(
                "<IIIBHBBBBH",
                self.int_edges, 0, self.i2c_errors, 1 if self.i2c_errors else 0,
                model_id, self.platform.reg.get(uld.FIRMWARE__SYSTEM_STATUS, 0),
                1 if model_id == uld.MODEL_ID_VL53L4CD else 0,
                1 if self.powered else 0, self.platform.reg.get(0x0031, 0) & 1,
                self.i2c_khz,
            ))
        elif cmd == 0x38:  # SET_I2C_SPEED
            (khz,) = st.unpack("<H", payload)
            self.platform.set_i2c_speed(khz)
            fw._status(cmd, 0x00)
        else:
            fw._status(cmd, 0x02)  # ERR_INVALID_CMD

    def send_result(self, distance_mm: int, *, status: int = 9, sigma_mm: int = 3,
                    signal_kcps: int = 1200, spads: int = 2048, stream_count: int = 1) -> bytes:
        """Emit one streamed result block (big-endian registers 0x0089..0x0099);
        returns the raw block so a test can decode the expectation itself."""
        raw = result_block(distance_mm, status=status, sigma_mm=sigma_mm,
                           signal_kcps=signal_kcps, spads=spads, stream_count=stream_count)
        self.int_edges += 1
        self._fw.mcu_time_us += 30_000
        self._fw._send(
            0x93,
            self._struct.pack("<QHH", self._fw.mcu_time_us, self.stream_addr, len(raw)) + raw,
        )
        return raw
