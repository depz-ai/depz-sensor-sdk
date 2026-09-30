"""pyserial-backed Link."""

from __future__ import annotations

import serial

try:  # POSIX only; Windows CDC ports don't share persisted termios state.
    import termios
except ImportError:  # pragma: no cover - Windows
    termios = None  # type: ignore[assignment]

from ..errors import DeviceLostError, LinkClosedError
from .link import Link

# I/O that races close(): pyserial can raise SerialException/OSError, a
# TypeError once its fd goes None, or — when a timeout change reconfigures the
# port after the fd is already gone — a bare ``termios.error`` (not an OSError
# subclass, so it must be named explicitly or it escapes the reader thread).
_LINK_IO_ERRORS: tuple[type[BaseException], ...] = (
    (serial.SerialException, OSError, TypeError)
    + ((termios.error,) if termios is not None else ())
)


def _restore_tty_sane(fd: int) -> None:
    """Leave the tty in a sane (cooked) state for the *next* opener.

    On Linux the kernel keeps termios per-tty across ``close()``, and pyserial
    mutates it to raw on open but never restores it (it just ``os.close``s the
    fd). The raw state it leaves behind (VMIN=0, echo/ISIG/IEXTEN off, output
    post-processing off) then poisons the next program that opens the port
    without fully reconfiguring it — in particular Chromium's Web Serial, which
    starts from the existing termios and afterwards can't talk to the device
    ("no reply to cmd 0x04"), fixed by nothing short of ``stty sane`` or a
    replug. Restoring the standard flags on our way out keeps the port usable
    by other clients. Best-effort: a dead fd (or one racing close) just no-ops.

    (Only in-process, graceful closes run this — a SIGKILL/segfault can't; the
    recovery there is still ``stty -F <port> sane`` or an unplug/replug.)
    """
    if termios is None:
        return
    try:
        attr = termios.tcgetattr(fd)  # [iflag, oflag, cflag, lflag, isp, osp, cc]
    except (termios.error, OSError, ValueError):
        return
    # Cooked input, but IXON off: XON/XOFF flow control would let a 0x13 byte
    # in binary frames stall transmission. Matches `stty sane` on Linux.
    attr[0] = (attr[0] | termios.BRKINT | termios.ICRNL | termios.IMAXBEL
               | termios.PARMRK) & ~(termios.IGNBRK | termios.INLCR
                                     | termios.IGNCR | termios.IXOFF
                                     | termios.IXON)
    attr[1] |= termios.OPOST | termios.ONLCR
    attr[3] |= (termios.ISIG | termios.ICANON | termios.IEXTEN | termios.ECHO
                | termios.ECHOE | termios.ECHOK | termios.ECHOCTL
                | termios.ECHOKE)
    attr[6][termios.VMIN] = 1
    attr[6][termios.VTIME] = 0
    try:
        termios.tcsetattr(fd, termios.TCSANOW, attr)
    except (termios.error, OSError, ValueError):
        pass


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
        except _LINK_IO_ERRORS as exc:
            # Races close(): fd becomes None (TypeError) or the timeout write
            # reconfigures a dead fd (termios.error). Either way the link is gone.
            self._closed = True
            raise DeviceLostError(f"{self._port}: {exc}") from exc

    def write(self, data: bytes) -> None:
        if self._closed:
            raise LinkClosedError(f"{self._port} closed")
        try:
            self._ser.write(data)
        except _LINK_IO_ERRORS as exc:
            self._closed = True
            raise DeviceLostError(f"{self._port}: {exc}") from exc

    def close(self) -> None:
        if not self._closed:
            self._closed = True
            # Restore a sane tty BEFORE dropping the fd, or the raw state
            # pyserial left behind persists and breaks the next opener
            # (e.g. Chromium Web Serial). Needs a live fd, so do it first.
            try:
                fd = self._ser.fileno()
            except (serial.SerialException, OSError, ValueError, TypeError):
                fd = None
            if fd is not None and fd >= 0:
                _restore_tty_sane(fd)
            try:
                self._ser.close()
            except (serial.SerialException, OSError):
                pass

    @property
    def closed(self) -> bool:
        return self._closed
