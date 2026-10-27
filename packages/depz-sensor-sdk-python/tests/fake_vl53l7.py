"""Fake VL53L5CX / VL53L7CX / VL53L7CH board for host-side tests (no hardware).

`FakeVl53l7` is the APP_VL53L7 I2C bridge firmware of contract 11 over a
LoopbackLink: the VL53L8 bridge (fake_vl53l8.FakeVl53l8) with the three board
commands the I2C board adds (PIN_CTRL, GET_INFO, SET_I2C_SPEED), its tighter
transfer limits and the RPT_VL53_INFO counters, in front of a
`FakeVl53l7Sensor` — the same chip model as the L8 fake on L5/L7 silicon
(revision 0x02), booting the `l7cx` / `l7ch` blobs and answering the
module_type the test sets (MZ = L5, MZEVO = L7)."""

from __future__ import annotations

import struct

from depz_sensor_sdk.protocol.vl53l7 import (
    I2C_SPEED_STEPS_KHZ,
    READ_MAX_LEN,
    STREAM_CHUNK_MAX,
    WRITE_MAX_LEN,
    I2cError,
    PinAction,
)
from depz_sensor_sdk.vl53l8.uld import MODULE_TYPE_MZEVO, SI_REVISION_CUT12
from fake_vl53l8 import FakeVl53l8, FakeVl53l8Sensor


class FakeVl53l7Sensor(FakeVl53l8Sensor):
    """L5/L7 silicon (cut 1.2) behind the I2C bridge: boots the `l7cx` (ULD
    2.0.1, no checksum word) or `l7ch` (VL53LMZ 2.0.16) blob. `lpn` low =
    the I2C interface is off and every transfer NACKs."""

    REVISION_ID = SI_REVISION_CUT12
    VARIANTS = ("l7cx", "l7ch")
    _blobs = None

    def __init__(self):
        super().__init__()
        self.module_type = MODULE_TYPE_MZEVO  # a VL53L7 by default; MZ = VL53L5
        self.lpn = True
        # L5/L7 "FW access" marker the ULD 2.0.x boot polls (page 1 reg 0x21
        # bit 0x10); the L8 silicon answers 0x04 there instead.
        self.reg[(1, 0x21)] = 0x10


class FakeVl53l7(FakeVl53l8):
    """In-process APP_VL53L7 bridge (contract 11 §2)."""

    SOFTWARE_NAME = "APP_VL53L7_v0.52"
    DEVICE_NAME = "DEPZ ToF Sensor VL53L7CX USB v2.1 TXK5KAX6X4"
    SERIAL = "SN000007"
    SENSOR = FakeVl53l7Sensor
    READ_MAX = READ_MAX_LEN
    WRITE_MAX = WRITE_MAX_LEN
    ERR_BUSY = 0x06
    ERR_HARDWARE_FAULT = 0x09

    def __init__(self):
        super().__init__()
        self.int_edges = 0
        self.frames_dropped = 0
        self.i2c_errors = 0
        self.last_i2c_error = I2cError.OK
        self.i2c_khz = 1000  # firmware default since v0.52
        self.pin_actions: list[int] = []
        self.speed_requests: list[int] = []
        self.transfer_in_flight = False  # SET_I2C_SPEED answers ERR_BUSY

    # ── contract 11 §2 ───────────────────────────────────────────────────────

    def _handle_bridge(self, fw, pkt) -> None:
        cmd, payload = pkt.cmd, pkt.payload
        if cmd in (0x32, 0x33):
            # The I2C board's own limits come first: READ len 1..1536, WRITE
            # N 1..2048, both within the 16-bit register space.
            if cmd == 0x32 and len(payload) == 4:
                addr, n = struct.unpack("<HH", payload)
                if not 1 <= n <= READ_MAX_LEN or addr + n > 0x10000:
                    fw._status(cmd, self.ERR_INVALID_PARAM)
                    return
            if cmd == 0x33 and len(payload) >= 3:
                (addr,) = struct.unpack_from("<H", payload)
                if len(payload) - 2 > WRITE_MAX_LEN or addr + len(payload) - 2 > 0x10000:
                    fw._status(cmd, self.ERR_INVALID_PARAM)
                    return
            if not self.sensor.lpn:  # LPn low: the sensor NACKs
                fw.mcu_time_us += 120
                self.i2c_errors += 1
                self.last_i2c_error = I2cError.NACK
                fw._status(cmd, self.ERR_HARDWARE_FAULT)
                return
        super()._handle_bridge(fw, pkt)

    def _handle_board(self, fw, cmd: int, payload: bytes) -> None:
        if cmd == 0x34:  # PIN_CTRL
            if len(payload) != 1:
                fw._status(cmd, self.ERR_PAYLOAD_FORMAT)
                return
            action = payload[0]
            if action not in (PinAction.LPN_OFF, PinAction.LPN_ON, PinAction.I2C_RST, PinAction.SOFT_CYCLE):
                fw._status(cmd, self.ERR_INVALID_PARAM)
                return
            self.pin_actions.append(action)
            if action == PinAction.LPN_OFF:
                self.streaming = False
                self.sensor.lpn = False
            elif action == PinAction.LPN_ON:
                self.sensor.lpn = True
            elif action == PinAction.SOFT_CYCLE:
                self.streaming = False
                self.sensor.lpn = True
                self.i2c_errors = 0
                self.last_i2c_error = I2cError.OK
            fw._status(cmd, 0x00)
        elif cmd == 0x37:  # GET_INFO: bridge state only, no echoed command byte
            fw._send(0x92, self.info_payload())
        elif cmd == 0x38:  # SET_I2C_SPEED
            if len(payload) != 2:
                fw._status(cmd, self.ERR_PAYLOAD_FORMAT)
                return
            (khz,) = struct.unpack("<H", payload)
            self.speed_requests.append(khz)
            if self.transfer_in_flight or self.streaming:
                fw._status(cmd, self.ERR_BUSY)
                return
            self.i2c_khz = min(I2C_SPEED_STEPS_KHZ, key=lambda s: abs(s - khz))
            fw._status(cmd, 0x00)
        else:
            super()._handle_board(fw, cmd, payload)

    def info_payload(self) -> bytes:
        return struct.pack(
            "<IIIBBBHHB",
            self.int_edges,
            self.frames_dropped,
            self.i2c_errors,
            self.last_i2c_error,
            1 if self.sensor.lpn else 0,
            1 if self.streaming and self.sensor.ranging else 0,
            self.i2c_khz,
            self.frame_size,
            1 if self.streaming else 0,
        )

    def send_frame(self, *, chunk_max: int = STREAM_CHUNK_MAX) -> dict:
        """One INT edge → one frame in ≤ 1536-byte chunks (contract 11 §2)."""
        self.int_edges += 1
        return super().send_frame(chunk_max=chunk_max)
