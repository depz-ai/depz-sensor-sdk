"""In-process fake SR04 firmware speaking the wire protocol over a LoopbackLink.

Mirrors the observable behavior of `usonic_sr04` cmd_handler.c closely enough
for DeviceBase/Sr04 unit tests: same reply kinds, same busy semantics, same
RPT_DATA layouts.
"""

from __future__ import annotations

import struct
import threading

from depz_sensor_sdk.errors import LinkClosedError, DeviceLostError
from depz_sensor_sdk.transport import CrcType, Packet, PacketParser, build_packet
from depz_sensor_sdk.transport.link import LoopbackLink


class FakeSr04:
    SOFTWARE_NAME = "APP_usonic_SR04_v0.95"
    DEVICE_NAME = "DEPZ Usonic SR04"
    SERIAL = "SN0042"

    def __init__(self):
        self.link, self._side = LoopbackLink.pair()
        self.sample_period_us = 50_000
        self.echo_decay_us = 5_000
        self.loop_running = False
        self.echo_time_us = 5831  # ~1 m
        self.mcu_time_us = 1_000_000
        self.temperature_raw = 273  # 27.3 °C
        self._tx_seq = 0
        self._parser = PacketParser()
        self._thread = threading.Thread(target=self._run, daemon=True)
        self._thread.start()

    def close(self):
        self.link.close()

    # ── device-side TX ───────────────────────────────────────────────────────

    def _send(self, cmd: int, payload: bytes = b"") -> None:
        try:
            self._side.write(build_packet(cmd, payload, self._tx_seq, CrcType.NONE))
        except LinkClosedError:
            return
        self._tx_seq = (self._tx_seq + 1) & 0xFF

    def _status(self, cmd: int, status: int) -> None:
        self._send(0x80, bytes((cmd, status)))

    def _text(self, cmd: int, text: str) -> None:
        self._send(0x81, bytes((cmd,)) + text.encode() + b"\x00")

    def send_measurement(self, source_cmd: int, echo: int | None = None) -> None:
        """Emit an RPT_DATA as the ISR callback would."""
        self.mcu_time_us += self.sample_period_us
        self._send(
            0x91,
            struct.pack("<BQH", source_cmd, self.mcu_time_us,
                        self.echo_time_us if echo is None else echo),
        )

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
        cmd, payload = pkt.cmd, pkt.payload
        if cmd == 0x03:
            self._text(cmd, self.DEVICE_NAME)
        elif cmd == 0x04:
            self._text(cmd, self.SOFTWARE_NAME)
        elif cmd == 0x05:
            self._text(cmd, self.SERIAL)
        elif cmd == 0x06:  # SYNC_TIME
            (t1,) = struct.unpack("<Q", payload)
            # Model a zero device-processing gap (t3 == t2) so the host
            # round-trip always dominates: rtt = (t4-t1) - (t3-t2) = t4-t1 >= 0
            # on any host, including a ~0µs in-process loopback. The offset
            # semantics (device_clock - host_clock) are preserved.
            t2 = self.mcu_time_us + 500
            t3 = t2
            self._send(0x82, struct.pack("<QQQ", t1, t2, t3))
        elif cmd == 0x07:
            self._send(0x83, struct.pack("<Qh", self.mcu_time_us, self.temperature_raw))
        elif cmd == 0x31:  # SET_SYNC_PIN_CONFIG
            pin, mode, pol = struct.unpack("<BBB", payload)
            if not (1 <= pin <= 5) or mode > 4 or pol > 1:
                self._status(cmd, 0x04)
            else:
                self._status(cmd, 0x00)
        elif cmd == 0x30:  # GET_SYNC_PIN_CONFIG
            (pin,) = struct.unpack("<B", payload)
            if not (1 <= pin <= 5):
                self._status(cmd, 0x04)
            else:
                self._send(0x90, struct.pack("<BBB", pin, 0, 0))
        elif cmd == 0x32:
            self._send(0x92, struct.pack("<I", self.sample_period_us))
        elif cmd == 0x33:
            (self.sample_period_us,) = struct.unpack("<I", payload)
            self._status(cmd, 0x00)
        elif cmd == 0x34:
            self._send(0x93, struct.pack("<H", self.echo_decay_us))
        elif cmd == 0x35:
            (req,) = struct.unpack("<H", payload)
            self.echo_decay_us = min(max(req, 4000), 65000)
            self._status(cmd, 0x00)
        elif cmd == 0x36:  # MEASURE_ONCE
            if self.loop_running:
                self._status(cmd, 0x06)  # ERR_BUSY
            else:
                self.send_measurement(0x36)  # no OK ack — data IS the reply
        elif cmd == 0x37:
            self.loop_running = True
            self._status(cmd, 0x00)
        elif cmd == 0x38:
            self.loop_running = False
            self._status(cmd, 0x00)
        else:
            self._status(cmd, 0x02)  # ERR_INVALID_CMD
