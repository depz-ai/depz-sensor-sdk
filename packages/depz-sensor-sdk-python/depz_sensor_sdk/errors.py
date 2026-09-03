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


class DeviceLostError(DepzError):
    """The serial link dropped (unplug, reboot) while in use."""


class NoDepzDeviceError(DepzError):
    """Discovery found no DEPZ device matching the request.

    Raised by `open_device()`/discovery when no candidate serial port has a
    known DEPZ USB identity (or none matches a requested serial/index).
    """


class LinkClosedError(DepzError):
    """Operation attempted on a closed link/device."""
