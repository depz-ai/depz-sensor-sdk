"""Exception hierarchy of the DEPZ sensor SDK.

Status code names follow contracts/02_COMMON_COMMANDS.md §3.
"""

from __future__ import annotations


class DepzError(Exception):
    """Base class for all SDK errors."""


class DepzTimeoutError(DepzError, TimeoutError):
    """A request got no matching reply within the timeout."""

    def __init__(self, cmd: int, timeout: float):
        self.cmd = cmd
        self.timeout = timeout
        super().__init__(f"no reply to cmd 0x{cmd:02X} within {timeout:.3f}s")


_STATUS_NAMES = {
    0x00: "OK",
    0x01: "ERROR",
    0x02: "ERR_INVALID_CMD",
    0x03: "ERR_PAYLOAD_FORMAT",
    0x04: "ERR_INVALID_PARAM",
    0x05: "ERR_PAYLOAD_CRC",
    0x06: "ERR_BUSY",
    0x07: "ERR_CMD_NOT_SUPPORTED",
    0x08: "ERR_NOT_INITIALIZED",
    0x09: "ERR_HARDWARE_FAULT",
}


class StatusError(DepzError):
    """Device answered a request with a non-OK RPT_STATUS."""

    def __init__(self, cmd: int, status: int):
        self.cmd = cmd
        self.status = status
        self.status_name = _STATUS_NAMES.get(status, f"0x{status:02X}")
        super().__init__(
            f"cmd 0x{cmd:02X} failed: {self.status_name} (0x{status:02X})"
        )


class BusyError(StatusError):
    """Device answered ERR_BUSY; the operation may be retried later."""

    def __init__(self, cmd: int):
        super().__init__(cmd, 0x06)


class InvalidReplyError(DepzError):
    """The reply to a request does not decode (contract 07 §3): shorter than
    the fields its contract defines, or a value the decoder refuses. Only that
    request fails; the reader goes on and the device stays usable."""

    def __init__(self, cmd: int, cause: BaseException):
        self.cmd = cmd
        super().__init__(f"reply to cmd 0x{cmd:02X} does not decode: {cause}")
        self.__cause__ = cause


class DeviceLostError(DepzError):
    """The serial link dropped (unplug, reboot) while in use."""


class NoDepzDeviceError(DepzError):
    """Discovery found nothing to open (contract 07 §3).

    Raised by `open_device()` / `open_on_link()` when no candidate serial
    port has a known DEPZ USB identity, none matches a requested
    serial/index, or nothing DEPZ answers the probe on the port path or link
    given.
    """


class LinkClosedError(DepzError):
    """Operation attempted on a closed link/device."""


class InvalidStateError(DepzError):
    """The call is not valid in the current state of the device or object
    (contract 07 §3): `to_host_time_us` before `sync_time`, a sensor call
    before `init()` / `configure()` or while streaming, `add()` after
    recording started, a closed `DatasetWriter` or `DatasetPlayer`,
    `wait()` without the player thread, a write to a read-only register
    block, a VL53Lx board name without a product number, a packaged
    firmware blob of the wrong size."""


class ReplayMismatchError(DepzError):
    """A strict `ReplayLink` saw the host write bytes the capture does not
    hold (contract 08): the SDK under test diverged from the recorded
    protocol."""
