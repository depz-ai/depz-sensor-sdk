"""In-process fake BNO086 bridge + sensor hub over a LoopbackLink.

Mirrors the observable behavior of APP_BNO086 (per ERRATA E2): SEND_SHTP_PACKET
is ACKed with RPT_STATUS immediately (OK, or ERR_BUSY when told to), and every
sensor-originated SHTP frame — solicited or not — arrives as RPT_DATA(cmd=0).
The SH-2 side answers product id, feature get/set, commands and FRS with
canned-but-consistent responses.
"""

from __future__ import annotations

import struct
import threading

from depz_sensor_sdk.errors import DeviceLostError, LinkClosedError
from depz_sensor_sdk.transport import CrcType, Packet, PacketParser, build_packet
from depz_sensor_sdk.transport.link import LoopbackLink
from depz_sensor_sdk.bno086.shtp import (
    NUM_CHANNELS,
    SHTP_HEADER_SIZE,
    ShtpChannel,
    ShtpHeader,
    fragment_cargo,
)

CMD_SEND_SHTP = 0x34


class FakeBno086:
    SOFTWARE_NAME = "APP_BNO086_v0.95"
    DEVICE_NAME = "DEPZ BNO086"
    SERIAL = "SN0086"
    ADVERTISEMENT = bytes.fromhex("000101020a")  # opaque channel-0 blob
    FRS_RECORDS: dict[int, list[int]] = {
        0x2D3E: [0x10000000, 0, 0, 0x40000000],  # system orientation (Q30)
        0xE302: [0x00040404, 0x00500000, 0x4000, (0x0800 << 16) | 4,
                 2500, 0x01190032, 0x00001000, (0x0011 << 16) | 8,
                 0, 100000],  # accelerometer metadata rev 4
    }

    def __init__(self):
        self.link, self._side = LoopbackLink.pair()
        self.mcu_time_us = 1_000_000
        self.busy_remaining = 0  # ERR_BUSY the next N SEND_SHTP_PACKETs
        self.interval_factor = 1.0  # granted = requested * factor
        self.send_shtp_attempts = 0  # every SEND_SHTP_PACKET incl. busy ones
        self.features: dict[int, int] = {}  # sensor -> granted interval_us
        self.last_set_feature: bytes | None = None
        self.tare_requests: list[bytes] = []
        self.frs_writes: dict[int, list[int]] = {}
        self._frs_wr_type: int | None = None
        self._frs_wr_len = 0
        self._frs_wr_words: dict[int, int] = {}
        self._tx_seq = 0
        self._shtp_tx_seq = [0] * NUM_CHANNELS  # device->host, per channel
        self._parser = PacketParser()
        self._lock = threading.Lock()
        self._thread = threading.Thread(target=self._run, daemon=True)
        self._thread.start()

    def close(self):
        self.link.close()

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

    def _text(self, cmd: int, text: str) -> None:
        self._send(0x81, bytes((cmd,)) + text.encode() + b"\x00")

    def push_shtp_cargo(self, channel: int, payload: bytes, *, max_frame: int = 4096) -> None:
        """Emit an SHTP cargo as RPT_DATA(cmd=0) frames; `max_frame` < cargo
        size exercises multi-frame reassembly on the host."""
        seq = self._shtp_tx_seq[channel]
        frames = fragment_cargo(channel, payload, seq, max_frame=max_frame)
        self._shtp_tx_seq[channel] = (seq + len(frames)) & 0xFF
        for frame in frames:
            self.mcu_time_us += 100
            self._send(0x91, struct.pack("<BQ", 0x00, self.mcu_time_us) + frame)

    def push_input_cargo(self, payload: bytes, *, max_frame: int = 4096) -> int:
        """Push a channel-3 cargo; returns the capture timestamp of the LAST
        frame (the one that completes the cargo)."""
        self.push_shtp_cargo(ShtpChannel.INPUT_NORMAL, payload, max_frame=max_frame)
        return self.mcu_time_us

    def push_gyro_rv_cargo(self, payload: bytes) -> int:
        self.push_shtp_cargo(ShtpChannel.GYRO_RV, payload)
        return self.mcu_time_us

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
        elif cmd == 0x32:  # SENSOR_RESET: ack, then reset advertisement flow
            self._status(cmd, 0x00)
            self._shtp_tx_seq = [0] * NUM_CHANNELS
            self.push_shtp_cargo(ShtpChannel.COMMAND, self.ADVERTISEMENT)
            self.push_shtp_cargo(ShtpChannel.EXECUTABLE, b"\x01")  # reset done
        elif cmd == 0x33:  # SENSOR_WAKE_UP
            self._status(cmd, 0x00)
        elif cmd == CMD_SEND_SHTP:
            self.send_shtp_attempts += 1
            if self.busy_remaining > 0:
                self.busy_remaining -= 1
                self._status(cmd, 0x06)  # ERR_BUSY, frame dropped
                return
            self._status(cmd, 0x00)
            self._on_shtp(payload)
        else:
            self._status(cmd, 0x02)  # ERR_INVALID_CMD

    # ── SH-2 side ────────────────────────────────────────────────────────────

    def _on_shtp(self, frame: bytes) -> None:
        if len(frame) < SHTP_HEADER_SIZE:
            return
        hdr = ShtpHeader.unpack(frame)
        payload = frame[SHTP_HEADER_SIZE:]
        if hdr.channel != ShtpChannel.CONTROL or not payload:
            return
        rid = payload[0]
        if rid == 0xF9:  # product id request
            resp = struct.pack("<BBBBIIH", 0xF8, 0x01, 3, 8, 10003608, 475, 4) + b"\x00\x00"
            self.push_shtp_cargo(ShtpChannel.CONTROL, resp)
        elif rid == 0xFD:  # set feature
            self.last_set_feature = bytes(payload)
            _, sensor, _, _, interval, _, _ = struct.unpack_from("<BBBHIII", payload)
            granted = int(interval * self.interval_factor)
            if granted == 0:
                self.features.pop(sensor, None)
            else:
                self.features[sensor] = granted
            self._send_feature_response(sensor)
        elif rid == 0xFE:  # get feature request
            self._send_feature_response(payload[1])
        elif rid == 0xF2:  # command request
            self._on_command(payload)
        elif rid == 0xF4:  # FRS read request
            _, _, offset, ftype, block = struct.unpack_from("<BBHHH", payload)
            self._frs_read(ftype)
        elif rid == 0xF7:  # FRS write request
            _, _, length, ftype = struct.unpack_from("<BBHH", payload)
            self._frs_wr_type = ftype
            self._frs_wr_len = length
            self._frs_wr_words = {}
            self._frs_write_response(4, 0)  # write mode ready
        elif rid == 0xF6:  # FRS write data
            _, _, offset, d0, d1 = struct.unpack_from("<BBHII", payload)
            if self._frs_wr_type is None:
                self._frs_write_response(6, offset)  # not in write mode
                return
            self._frs_wr_words[offset] = d0
            if offset + 1 < self._frs_wr_len:
                self._frs_wr_words[offset + 1] = d1
            if len(self._frs_wr_words) >= self._frs_wr_len:
                words = [self._frs_wr_words[i] for i in range(self._frs_wr_len)]
                self.frs_writes[self._frs_wr_type] = words
                self._frs_write_response(3, offset)  # write completed
                self._frs_wr_type = None
            else:
                self._frs_write_response(0, offset)  # words received

    def _send_feature_response(self, sensor: int) -> None:
        interval = self.features.get(sensor, 0)
        resp = struct.pack("<BBBHIII", 0xFC, sensor, 0, 0, interval, 0, 0)
        self.push_shtp_cargo(ShtpChannel.CONTROL, resp)

    def _on_command(self, payload: bytes) -> None:
        seq, command = payload[1], payload[2]
        params = payload[3:12]
        if command == 0x03:  # tare — no response
            self.tare_requests.append(bytes(payload))
            return
        if command == 0x09:  # periodic DCD config — no response
            return
        if command == 0x06:  # DCD save
            self._command_response(seq, command, [0])
            return
        if command == 0x07:  # ME calibration
            if params[3] == 0x01:  # get
                self._command_response(seq, command, [0, 1, 1, 0, 0])
            else:
                self._command_response(seq, command, [0])
            return
        if command == 0x0A:  # get oscillator type — r[0] IS the type
            self._command_response(seq, command, [1])  # EXT_CRYSTAL
            return
        if command == 0x0B:  # clear DCD and reset — no response, device resets
            self._shtp_tx_seq = [0] * NUM_CHANNELS
            self.push_shtp_cargo(ShtpChannel.COMMAND, self.ADVERTISEMENT)
            self.push_shtp_cargo(ShtpChannel.EXECUTABLE, b"\x01")  # reset done
            return
        if command == 0x01:  # errors — stream records then a 255 terminator
            recs = [
                [1, 0, 3, 0x10, 2, 5],
                [2, 1, 4, 0x20, 3, 6],
            ]
            for i, r in enumerate(recs):
                self._command_response(seq, command, r, resp_seq=i)
            self._command_response(seq, command, [0, 0, 255], resp_seq=len(recs))
            return
        if command == 0x02:  # counts
            if params[0] == 0:  # get: two responses
                offered, accepted, on, attempted = 100, 90, 80, 70
                r0 = [0, 0, 0] + list(struct.pack("<II", offered, accepted))
                r1 = [0, 0, 0] + list(struct.pack("<II", on, attempted))
                self._command_response(seq, command, r0, resp_seq=0)
                self._command_response(seq, command, r1, resp_seq=1)
            else:  # clear
                self._command_response(seq, command, [0])
            return
        self._command_response(seq, command, [0xFF])  # unknown -> error status

    def _command_response(
        self, cmd_seq: int, command: int, r: list[int], resp_seq: int = 0
    ) -> None:
        resp = bytes((0xF1, 0, command, cmd_seq, resp_seq)) + bytes(r).ljust(11, b"\x00")
        self.push_shtp_cargo(ShtpChannel.CONTROL, resp)

    def _frs_write_response(self, status: int, offset: int) -> None:
        self.push_shtp_cargo(
            ShtpChannel.CONTROL, struct.pack("<BBH", 0xF5, status, offset)
        )

    def _frs_read(self, ftype: int) -> None:
        words = self.FRS_RECORDS.get(ftype)
        if words is None:
            self._frs_read_response(1, 0, 0, [0, 0], ftype)  # unrecognized
            return
        for off in range(0, len(words), 2):
            chunk = words[off : off + 2]
            last = off + 2 >= len(words)
            status = 3 if last else 0  # read completed on the final packet
            self._frs_read_response(status, len(chunk), off, chunk, ftype)

    def _frs_read_response(
        self, status: int, datalen: int, offset: int, words: list[int], ftype: int
    ) -> None:
        w = list(words) + [0, 0]
        resp = struct.pack(
            "<BBHIIHH", 0xF3, (datalen << 4) | status, offset, w[0], w[1], ftype, 0
        )
        self.push_shtp_cargo(ShtpChannel.CONTROL, resp)
