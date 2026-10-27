"""SerialLink behaviour over a pty pair — no hardware, POSIX only.

A pseudo-terminal slave is a real tty to pyserial (termios, flock, select),
so the pty master stands in for the device: what the device would send is
written to the master, what the link sends is read back from it, and closing
the master is an unplug. The contract-07 Link semantics under test: read
returns a chunk or b"" on timeout; a lost device surfaces as DeviceLostError
and the link is closed afterwards; I/O on a closed link is LinkClosedError;
one owner per port (exclusive open); close() is idempotent and leaves the tty
cooked for the next opener (see test_serial_link_termios.py for the flags).
"""

from __future__ import annotations

import os
import threading
import time

import pytest

termios = pytest.importorskip("termios")

import serial

from depz_sensor_sdk.errors import DeviceLostError, LinkClosedError
from depz_sensor_sdk.transport import serial_link
from depz_sensor_sdk.transport.serial_link import SerialLink


@pytest.fixture
def pty():
    """(master_fd, slave_fd, slave_path); the slave fd stays ours so the tty
    outlives the link and its termios can be inspected after close()."""
    master, slave = os.openpty()
    try:
        yield master, slave, os.ttyname(slave)
    finally:
        for fd in (master, slave):
            try:
                os.close(fd)
            except OSError:
                pass


def test_open_failure_is_device_lost():
    with pytest.raises(DeviceLostError, match="cannot open") as info:
        SerialLink("/dev/ttyDEPZ-does-not-exist")
    assert isinstance(info.value.__cause__, serial.SerialException)


def test_write_read_timeout_and_close(pty):
    master, slave, path = pty
    link = SerialLink(path)
    try:
        assert link.port == path
        assert not link.closed

        link.write(b"\x01\x02\x03")
        assert os.read(master, 16) == b"\x01\x02\x03"

        # One chunk per read: the first byte plus whatever is already waiting.
        os.write(master, b"abcdef")
        time.sleep(0.05)
        assert link.read(timeout=0.5) == b"abcdef"

        # Nothing pending: b"" after the timeout, not an exception.
        t0 = time.monotonic()
        assert link.read(timeout=0.1) == b""
        assert 0.05 <= time.monotonic() - t0 < 1.0
        assert link.read() == b""  # default (short) timeout
    finally:
        link.close()

    assert link.closed
    link.close()  # idempotent
    with pytest.raises(LinkClosedError):
        link.read()
    with pytest.raises(LinkClosedError):
        link.write(b"x")
    # The tty is handed back cooked, not in pyserial's raw state.
    attr = termios.tcgetattr(slave)
    assert attr[3] & termios.ICANON
    assert attr[1] & termios.OPOST


def test_second_opener_is_refused_unless_non_exclusive(pty):
    _, _, path = pty
    first = SerialLink(path)
    try:
        with pytest.raises(DeviceLostError, match="exclusively lock"):
            SerialLink(path)
        shared = SerialLink(path, exclusive=False)  # opt-out, e.g. for sniffing
        shared.close()
    finally:
        first.close()


def test_unplug_during_read_is_device_lost(pty):
    master, _, path = pty
    link = SerialLink(path)
    os.close(master)  # the device is gone
    with pytest.raises(DeviceLostError, match=path):
        link.read(timeout=0.5)
    assert link.closed
    with pytest.raises(LinkClosedError):
        link.write(b"x")
    link.close()  # still safe on a dead tty


def test_unplug_during_write_is_device_lost(pty):
    master, _, path = pty
    link = SerialLink(path)
    os.close(master)
    with pytest.raises(DeviceLostError, match="write failed") as info:
        link.write(b"\x00" * 8)
    assert isinstance(info.value.__cause__, serial.SerialException)
    assert link.closed


def test_close_under_a_blocked_reader(pty):
    """close() from another thread while read() waits in select: the reader
    ends with a link-gone error (not pyserial's TypeError on the dropped fd)
    no later than its own timeout, and close() itself completes."""
    _, _, path = pty
    link = SerialLink(path)
    entered = threading.Event()
    outcome: dict[str, object] = {}

    def reader() -> None:
        entered.set()
        try:
            outcome["data"] = link.read(timeout=2.0)
        except Exception as exc:  # noqa: BLE001 - we assert on the type below
            outcome["error"] = exc

    th = threading.Thread(target=reader, daemon=True)
    th.start()
    assert entered.wait(1.0)
    time.sleep(0.1)  # let the reader settle into select()
    link.close()
    th.join(10.0)
    assert not th.is_alive()
    assert isinstance(outcome.get("error"), (DeviceLostError, LinkClosedError)), outcome
    assert link.closed


class _TermiosErrorSerial:
    """pyserial stand-in whose timeout setter raises a bare termios.error —
    not an OSError subclass, so it must be named in the except tuple or it
    escapes the device reader thread."""

    is_open = True

    def __init__(self, *args, **kwargs):
        pass

    @property
    def timeout(self):
        return 0.05

    @timeout.setter
    def timeout(self, value):
        raise termios.error(5, "Input/output error")

    def read(self, size=1):
        raise AssertionError("not reached")

    def write(self, data):
        raise termios.error(5, "Input/output error")

    def fileno(self):
        raise serial.SerialException("port closed")

    def close(self):
        raise serial.SerialException("already closed")


def test_bare_termios_error_is_device_lost(monkeypatch):
    monkeypatch.setattr(serial_link.serial, "Serial", _TermiosErrorSerial)
    link = SerialLink("/dev/ttyFAKE")
    with pytest.raises(DeviceLostError, match="Input/output error"):
        link.read()
    assert link.closed

    link = SerialLink("/dev/ttyFAKE")
    with pytest.raises(DeviceLostError):
        link.write(b"x")
    assert link.closed

    link = SerialLink("/dev/ttyFAKE")
    link.close()  # fileno()/close() failing: best-effort, nothing escapes
    assert link.closed
