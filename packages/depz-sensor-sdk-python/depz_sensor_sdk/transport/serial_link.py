"""pyserial-backed Link."""

from __future__ import annotations

import serial

from ..errors import DeviceLostError, LinkClosedError
from .link import Link


class SerialLink(Link):
    """CDC-ACM serial port as a Link. Baud rate is nominal (device ignores it).

    Opens the port **exclusively**. A CDC-ACM tty is not single-opener on
    POSIX: a second ``open()`` succeeds and the two readers then consume from
    the same file description, so each receives a random subset of the device's
    bytes and both see corrupt framing. `exclusive=True` makes pyserial take an
    ``flock``, turning that silent corruption into an immediate, actionable
    error (contract 07 §1: one owner per transport). Windows already opens
    CDC ports with exclusive sharing, where the flag is a no-op.
    """

    def __init__(
        self,
        port: str,
        baudrate: int = 115200,
        *,
        open_timeout: float = 2.0,
        exclusive: bool = True,
    ):
        try:
            self._ser = serial.Serial(
                port,
                baudrate,
                timeout=0.05,
                write_timeout=open_timeout,
                exclusive=exclusive,
            )
        except serial.SerialException as exc:
            raise DeviceLostError(f"cannot open {port}: {exc}") from exc
        self._port = port
        self._closed = False

    @property
    def port(self) -> str:
        return self._port

    def read(self, timeout: float | None = None) -> bytes:
        if self._closed:
            raise LinkClosedError(f"{self._port} closed")
        try:
            self._ser.timeout = timeout if timeout is not None else 0.05
            data = self._ser.read(1)
            waiting = self._ser.in_waiting
            if data and waiting:
                data += self._ser.read(waiting)
            return data
        except (serial.SerialException, OSError, TypeError) as exc:
            # TypeError: pyserial's read races close() (fd becomes None).
            self._closed = True
            raise DeviceLostError(f"{self._port}: {exc}") from exc

    def write(self, data: bytes) -> None:
        if self._closed:
            raise LinkClosedError(f"{self._port} closed")
        try:
            self._ser.write(data)
        except (serial.SerialException, OSError) as exc:
            self._closed = True
            raise DeviceLostError(f"{self._port}: {exc}") from exc

    def close(self) -> None:
        if not self._closed:
            self._closed = True
            try:
                self._ser.close()
            except (serial.SerialException, OSError):
                pass

    @property
    def closed(self) -> bool:
        return self._closed
