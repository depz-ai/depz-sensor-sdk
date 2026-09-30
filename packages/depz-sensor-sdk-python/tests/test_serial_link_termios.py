"""SerialLink must leave the tty in a sane (cooked) state on close.

pyserial mutates termios to raw on open and never restores it; on Linux that
state persists per-tty across close() and then breaks the next opener that
doesn't fully reconfigure — notably Chromium Web Serial ("no reply to cmd
0x04", fixed only by `stty sane` or a replug). ``_restore_tty_sane`` undoes it.
Exercised over a pty (no hardware, POSIX only).
"""

from __future__ import annotations

import os

import pytest

termios = pytest.importorskip("termios")

from depz_sensor_sdk.transport.serial_link import _restore_tty_sane


def _set_raw(fd: int) -> None:
    a = termios.tcgetattr(fd)
    a[0] &= ~(termios.BRKINT | termios.ICRNL | termios.IXON)  # raw input
    a[1] &= ~termios.OPOST  # no output post-processing
    a[3] &= ~(termios.ISIG | termios.ICANON | termios.IEXTEN | termios.ECHO
              | termios.ECHOK | termios.ECHOCTL | termios.ECHOKE)
    a[6][termios.VMIN] = 0
    a[6][termios.VTIME] = 0
    termios.tcsetattr(fd, termios.TCSANOW, a)


def test_restore_tty_sane_undoes_raw():
    master, slave = os.openpty()
    try:
        _set_raw(slave)
        a = termios.tcgetattr(slave)
        assert not (a[3] & termios.ICANON)  # confirm we really made it raw
        assert a[6][termios.VMIN] == 0

        _restore_tty_sane(slave)

        a = termios.tcgetattr(slave)
        assert a[3] & termios.ICANON, "ICANON restored"
        assert a[3] & termios.ISIG, "ISIG restored"
        assert a[3] & termios.IEXTEN, "IEXTEN restored"
        assert a[3] & termios.ECHO, "ECHO restored"
        assert a[1] & termios.OPOST, "OPOST restored"
        assert a[1] & termios.ONLCR, "ONLCR restored"
        assert a[0] & termios.ICRNL, "ICRNL restored"
        assert not (a[0] & termios.IXON), "IXON stays off (binary-safe)"
        # cc entries read back as bytes once ICANON is set (VMIN aliases VEOF).
        assert a[6][termios.VMIN] in (1, b"\x01"), "VMIN restored to 1"
    finally:
        os.close(master)
        os.close(slave)


def test_restore_tty_sane_survives_bad_fd():
    # A closed/negative fd must not raise — close() is best-effort.
    _restore_tty_sane(-1)
    r, w = os.pipe()
    os.close(r)
    os.close(w)
    _restore_tty_sane(r)  # already closed
