"""What the absorbed ULD ports (`uld/`) expect from the firmware repo's
`vl53_link`, provided over the SDK's DepzDevice.

The ports were written against `tof_vl53l0_4/tools/vl53_link.py`; they import
from it only the two error types, the boot bus speed, the XSHUT action code and
a device with read_reg / write_reg / set_i2c_speed / set_addr_width / xshut.
`BridgeLink` is that device. Keeping the ports' own names here, instead of
editing the ports, keeps them diffable against the firmware repo.
"""

from __future__ import annotations

from typing import TYPE_CHECKING, Any, Callable, TypeVar

from .._bridge import RegisterBridge, ShortReplyError
from ..errors import DepzError, DepzTimeoutError, StatusError
from ..protocol.vl53lx import (
    VL53LX_BRIDGE,
    XFER_MAX,
    XSHUT_OFF,
    XSHUT_ON,
    XSHUT_RESET,
    Vl53lxCmd,
    pack_set_addr_width,
    pack_set_i2c_speed,
    pack_xshut,
)

if TYPE_CHECKING:  # pragma: no cover
    from . import Vl53lx

T = TypeVar("T")

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
    status, a missing answer or a short REG_DATA surfaces as `ProtocolError`,
    exactly what the ports catch."""

    def __init__(self, dev: Vl53lx) -> None:
        self._dev = dev
        self._regs = RegisterBridge(
            dev, cmds=VL53LX_BRIDGE, read_max=XFER_MAX, write_max=XFER_MAX, timeout=2.0
        )

    @property
    def last_timestamp_us(self) -> int:
        """MCU timestamp of the latest register read."""
        return self._regs.last_timestamp_us

    def _call(self, fn: Callable[..., T], *args: Any, **kwargs: Any) -> T:
        try:
            return fn(*args, **kwargs)
        except (StatusError, DepzTimeoutError, ShortReplyError) as exc:
            raise ProtocolError(str(exc)) from exc

    def read_reg(self, addr: int, length: int) -> bytes:
        return self._call(self._regs.read, addr, length)

    def write_reg(self, addr: int, data: bytes) -> None:
        self._call(self._regs.write, addr, data)

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
