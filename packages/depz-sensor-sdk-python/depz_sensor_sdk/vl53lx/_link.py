"""What the absorbed ULD ports (`uld/`) expect from the firmware repo's
`vl53_link`, provided over the SDK's DeviceBase.

The ports were written against `tof_vl53l0_4/tools/vl53_link.py`; they import
from it only the two error types, the boot bus speed, the XSHUT action code and
a device with read_reg / write_reg / set_i2c_speed / set_addr_width / xshut.
`BridgeLink` is that device. Keeping the ports' own names here, instead of
editing the ports, keeps them diffable against the firmware repo.
"""

from __future__ import annotations

from typing import TYPE_CHECKING

from ..device import DeviceBase
from ..errors import DepzError, DepzTimeoutError, StatusError
from ..protocol.vl53lx import (
    XFER_MAX,
    XSHUT_OFF,
    XSHUT_ON,
    XSHUT_RESET,
    RegData,
    Vl53lxCmd,
    Vl53lxRpt,
    pack_read_reg,
    pack_set_addr_width,
    pack_set_i2c_speed,
    pack_write_reg,
    pack_xshut,
)

if TYPE_CHECKING:  # pragma: no cover
    from . import Vl53lx

__all__ = [
    "Vl53Error",
    "ProtocolError",
    "I2C_KHZ_BOOT",
    "VL53_XSHUT_OFF",
    "VL53_XSHUT_ON",
    "VL53_XSHUT_RESET",
    "BridgeLink",
    "Vl53Device",
]

#: RPT_STATUS code a bridge answers an unknown command with.
ERR_INVALID_CMD = 0x02

#: The only bus speed an unconfigured sensor is specified for; every driver
#: runs its init here and raises the bus to its own ceiling afterwards.
I2C_KHZ_BOOT = 400

VL53_XSHUT_OFF, VL53_XSHUT_ON, VL53_XSHUT_RESET = XSHUT_OFF, XSHUT_ON, XSHUT_RESET


class Vl53Error(DepzError):
    """A ULD-level failure: the sensor did not do what the driver needed
    (timeout waiting for data-ready or boot, a calibration that failed...)."""


class ProtocolError(DepzError):
    """The bridge refused a register command or did not answer it — the
    firmware repo's single error for both. The ports catch it where the sensor
    legitimately NACKs for a while (e.g. right after a soft reset)."""


class BridgeLink:
    """The `Vl53Device` surface the ports use, over one open SDK device.

    Register transfers are split at the bridge's 253-byte limit; a non-OK
    status or a missing answer surfaces as `ProtocolError`, exactly what the
    ports catch."""

    def __init__(self, dev: "Vl53lx"):
        self._dev = dev
        self.last_timestamp_us = 0  # MCU timestamp of the latest register read

    def _call(self, fn, *args, **kwargs):
        try:
            return fn(*args, **kwargs)
        except (StatusError, DepzTimeoutError) as exc:
            raise ProtocolError(str(exc)) from exc

    def read_reg(self, addr: int, length: int) -> bytes:
        out = bytearray()
        while length > 0:
            n = min(length, XFER_MAX)
            rep: RegData = self._call(
                self._dev.request,
                Vl53lxCmd.READ_REG,
                pack_read_reg(addr, n),
                matcher=DeviceBase.expect_report(Vl53lxRpt.REG_DATA, RegData.unpack),
                timeout=2.0,
            )
            if len(rep.data) != n:
                raise ProtocolError(f"READ_REG 0x{addr:04X}: expected {n}, got {len(rep.data)}")
            self.last_timestamp_us = rep.timestamp_us
            out.extend(rep.data)
            addr += n
            length -= n
        return bytes(out)

    def write_reg(self, addr: int, data: bytes) -> None:
        done = 0
        while done < len(data):
            chunk = bytes(data[done : done + XFER_MAX])
            self._call(
                self._dev.request,
                Vl53lxCmd.WRITE_REG,
                pack_write_reg(addr, chunk),
                ok_completes=True,
                timeout=2.0,
            )
            addr += len(chunk)
            done += len(chunk)

    def set_i2c_speed(self, khz: int) -> None:
        self._call(
            self._dev.request, Vl53lxCmd.SET_I2C_SPEED, pack_set_i2c_speed(khz), ok_completes=True
        )

    def set_addr_width(self, width: int) -> None:
        self._call(
            self._dev.request,
            Vl53lxCmd.SET_ADDR_WIDTH,
            pack_set_addr_width(width),
            ok_completes=True,
        )

    def clear_i2c_errors(self) -> None:
        """Zero the bridge's `i2c_errors` / `last_i2c_error`. Sent once a sensor
        init is through: a resetting die NACKs its own address for a moment,
        and only the driver knows those NACKs were expected. A v2.00 bridge
        (firmware older than v0.24) does not know the command."""
        try:
            self._dev.request(Vl53lxCmd.CLEAR_I2C_ERRORS, b"", ok_completes=True)
        except StatusError as exc:
            if exc.status == ERR_INVALID_CMD:
                raise DepzError(
                    "the board's firmware predates APP_VL53L0_4_v0.24 (protocol v2.01) "
                    "and cannot clear its I2C error counter — reflash it"
                ) from exc
            raise ProtocolError(str(exc)) from exc
        except DepzTimeoutError as exc:
            raise ProtocolError(str(exc)) from exc

    def xshut(self, action: int) -> None:
        self._call(
            self._dev.request, Vl53lxCmd.XSHUT, pack_xshut(action), ok_completes=True, timeout=1.0
        )


#: The name the ports import.
Vl53Device = BridgeLink
