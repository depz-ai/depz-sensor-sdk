"""Shared support for the real-hardware QA suite.

Everything here is deliberately dependency-light (stdlib + pyserial) so the
hardware suite runs anywhere the SDK itself runs.

Design rules this module exists to enforce:

* **Stable identity, never a bare device path.** ``/dev/ttyACM0`` is whatever
  the kernel handed out this boot. Tests select by USB iSerial (the DEPZ line
  burns a unique one per unit) and resolve the path at use time.
* **Exclusive port leases.** Two tests (or a test and the viewer bridge) must
  never open the same port at once. ``port_lease`` takes an OS-level flock.
* **No unbounded waits.** Every helper that blocks takes a deadline and raises
  a diagnostic-rich error instead of hanging forever.
* **Hang forensics.** When a wait does expire, ``dump_diagnostics`` captures
  thread stacks / fds / resource counters *before* the test dies, so a CI
  failure is debuggable without a reproduction.
"""

from __future__ import annotations

import collections
import contextlib
import fcntl
import faulthandler
import json
import os
import re
import subprocess
import sys
import threading
import time
import traceback
from dataclasses import asdict, dataclass, field
from pathlib import Path
from typing import Any, Callable, Iterator

REPO_ROOT = Path(__file__).resolve().parents[4]
ARTIFACTS_DIR = Path(os.environ.get("DEPZ_QA_ARTIFACTS", REPO_ROOT / "artifacts" / "qa"))
LOCK_DIR = Path(os.environ.get("DEPZ_QA_LOCKDIR", "/tmp/depz-qa-locks"))

# The HC-SR04 *module* signals "no echo" by holding ECHO high for ~58.3 ms.
# That is below the firmware's 65 535 µs timeout, so it arrives as an
# ordinary-looking measurement (~58 310–58 324 µs observed = ~10 002 mm), not
# as the contract-03 0xFFFF sentinel — only the firmware's own timeout is
# sentinelled. Genuine range tops out near 4 m (~23 ms round trip), so any
# echo above this bound is that hardware signature and must be judged as a
# legitimate no-echo reading, not an implausible distance. See F-23.
SR04_MODULE_NO_ECHO_US = 30_000

# Sensor families this suite knows how to drive end to end.
FAMILY_SR04 = "sr04"
FAMILY_VL53L4CD = "vl53l4cd"
FAMILY_VL53L8CX = "vl53l8cx"
FAMILY_VL53L8CH = "vl53l8ch"
FAMILY_BNO086 = "bno086"
ALL_FAMILIES = (FAMILY_SR04, FAMILY_VL53L4CD, FAMILY_VL53L8CX, FAMILY_VL53L8CH, FAMILY_BNO086)


# ── device inventory ─────────────────────────────────────────────────────────


@dataclass(frozen=True)
class HwDevice:
    """One physically present DEPZ unit, identified by USB iSerial."""

    stable_id: str  # "<family>:<usb_serial>" — survives replug/renumbering
    family: str  # sr04 | vl53l8cx | vl53l8ch | bno086
    usb_serial: str
    usb_vid: int
    usb_pid: int
    port: str  # current device path — resolve fresh, never cache across replug
    by_id_path: str | None  # /dev/serial/by-id symlink when the OS provides one
    software_name: str = ""
    fw_version: str = ""
    device_name: str = ""
    protocol_serial: str = ""

    def resolve_port(self) -> str:
        """Re-resolve the device path from the stable USB serial.

        A port path is only valid until the device re-enumerates. Anything that
        reopens after a reset/replug must call this rather than reuse ``port``.
        """
        for pi in _enumerate_ports():
            if pi["usb_serial"] == self.usb_serial:
                return pi["port"]
        raise LookupError(f"{self.stable_id}: no port currently exposes USB serial {self.usb_serial}")


def _enumerate_ports() -> list[dict[str, Any]]:
    from serial.tools import list_ports

    from depz_sensor_sdk.usb_ids import is_known_depz_usb, usb_model_hint

    out = []
    for p in list_ports.comports():
        if not is_known_depz_usb(p.vid, p.pid):
            continue
        out.append(
            {
                "port": p.device,
                "usb_vid": p.vid,
                "usb_pid": p.pid,
                "usb_serial": p.serial_number,
                "model_hint": usb_model_hint(p.vid, p.pid),
            }
        )
    out.sort(key=lambda d: (d["usb_serial"] is None, d["usb_serial"] or "", d["port"]))
    return out


def _by_id_map() -> dict[str, str]:
    """port path → /dev/serial/by-id symlink (Linux; {} elsewhere)."""
    base = Path("/dev/serial/by-id")
    if not base.is_dir():
        return {}
    out: dict[str, str] = {}
    for link in base.iterdir():
        with contextlib.suppress(OSError):
            out[os.path.realpath(link)] = str(link)
    return out


#: Family resolution is USB-hint first, then confirmed by the firmware probe.
#: The CX and CH ship distinct PIDs (0xED4B / 0xED40) and distinct firmware
#: names, so the hint and the probe must agree — a mismatch is a real defect
#: worth surfacing rather than papering over.
_HINT_TO_FAMILY = {
    "sr04": FAMILY_SR04,
    "vl53l4cd": FAMILY_VL53L4CD,
    "vl53l8cx": FAMILY_VL53L8CX,
    "vl53l8ch": FAMILY_VL53L8CH,
    "bno086": FAMILY_BNO086,
}


def discover_hw(*, probe_timeout: float = 0.5) -> list[HwDevice]:
    """Enumerate + probe every attached DEPZ unit.

    Probing opens each port briefly, so this must not run while another test
    holds a lease. It is session-scoped in ``conftest`` for that reason.
    """
    from depz_sensor_sdk.discovery import probe_port

    by_id = _by_id_map()
    devices: list[HwDevice] = []
    for pi in _enumerate_ports():
        family = _HINT_TO_FAMILY.get(pi["model_hint"] or "")
        if family is None:
            continue
        if not pi["usb_serial"]:
            continue
        info = None
        with contextlib.suppress(Exception), port_lease(pi["port"], timeout=10.0):
            info = probe_port(pi["port"], timeout=probe_timeout)
        devices.append(
            HwDevice(
                stable_id=f"{family}:{pi['usb_serial']}",
                family=family,
                usb_serial=pi["usb_serial"],
                usb_vid=pi["usb_vid"],
                usb_pid=pi["usb_pid"],
                port=pi["port"],
                by_id_path=by_id.get(os.path.realpath(pi["port"])),
                software_name=getattr(info, "software_name", "") or "",
                fw_version=getattr(info, "fw_version", "") or "",
                device_name=getattr(info, "device_name", "") or "",
                protocol_serial=getattr(info, "serial_number", "") or "",
            )
        )
    devices.sort(key=lambda d: d.stable_id)
    return devices


def open_family(dev: HwDevice, **kw):
    """Construct the right SDK class for ``dev`` on its current port.

    Deliberately bypasses ``open_device()``'s own probe: the fixture already
    knows what the unit is, and re-probing on every open would double the
    open/close churn a reconnect test is trying to measure.
    """
    from depz_sensor_sdk import Bno086, Sr04, Vl53l4Cd
    from depz_sensor_sdk.vl53l8 import Vl53l8Ch, Vl53l8Cx

    port = dev.resolve_port()
    cls = {
        FAMILY_SR04: Sr04,
        FAMILY_VL53L4CD: Vl53l4Cd,
        FAMILY_VL53L8CX: Vl53l8Cx,
        FAMILY_VL53L8CH: Vl53l8Ch,
        FAMILY_BNO086: Bno086,
    }[dev.family]
    return cls(port, **kw)


# ── exclusive port leases ────────────────────────────────────────────────────


@contextlib.contextmanager
def port_lease(port: str, *, timeout: float = 30.0) -> Iterator[None]:
    """Hold an exclusive OS-level lease on ``port`` for the duration.

    Serial ports are single-opener in practice but the kernel does not enforce
    it for CDC-ACM: a second ``open()`` succeeds and both readers then steal
    each other's bytes. An flock on a side file gives us a cooperative lock
    that any DEPZ-aware process (tests, bridge, CLI) can honour.
    """
    LOCK_DIR.mkdir(parents=True, exist_ok=True)
    lock_path = LOCK_DIR / (re.sub(r"[^A-Za-z0-9]", "_", port) + ".lock")
    fd = os.open(lock_path, os.O_RDWR | os.O_CREAT, 0o666)
    deadline = time.monotonic() + timeout
    try:
        while True:
            try:
                fcntl.flock(fd, fcntl.LOCK_EX | fcntl.LOCK_NB)
                break
            except BlockingIOError:
                if time.monotonic() >= deadline:
                    holder = os.read(fd, 128).decode(errors="replace")
                    os.lseek(fd, 0, os.SEEK_SET)
                    raise TimeoutError(
                        f"port {port} lease busy for >{timeout}s (held by: {holder!r})"
                    ) from None
                time.sleep(0.05)
        os.ftruncate(fd, 0)
        os.write(fd, f"pid={os.getpid()} t={time.time():.0f}".encode())
        yield
    finally:
        with contextlib.suppress(OSError):
            fcntl.flock(fd, fcntl.LOCK_UN)
        os.close(fd)


# ── resource metrics ─────────────────────────────────────────────────────────


@dataclass
class ResourceSample:
    """A point-in-time snapshot of the resources a leak would grow."""

    t: float
    rss_kb: int
    fd_count: int
    tty_fd_count: int  # fds pointing at a /dev/tty* node — leaked serial handles
    thread_count: int
    py_thread_count: int
    depz_reader_threads: int  # SDK reader threads still alive
    asyncio_tasks: int

    def as_dict(self) -> dict[str, Any]:
        return asdict(self)


def _proc_status_field(name: str) -> int:
    try:
        text = Path("/proc/self/status").read_text()
    except OSError:
        return -1
    m = re.search(rf"^{name}:\s+(\d+)", text, re.M)
    return int(m.group(1)) if m else -1


def _fd_paths() -> list[str]:
    try:
        base = "/proc/self/fd"
        out = []
        for name in os.listdir(base):
            with contextlib.suppress(OSError):
                out.append(os.readlink(f"{base}/{name}"))
        return out
    except OSError:
        return []


def _running_asyncio_tasks() -> int:
    """Tasks on the *running* loop, or 0 from sync code.

    `asyncio.all_tasks()` only has meaning inside a loop; calling it from a
    sync test must not fabricate a loop just to count zero tasks.
    """
    import asyncio

    try:
        asyncio.get_running_loop()
    except RuntimeError:
        return 0
    return len(asyncio.all_tasks())


def sample_resources() -> ResourceSample:
    fds = _fd_paths()
    try:
        n_tasks = _running_asyncio_tasks()
    except Exception:
        n_tasks = 0
    return ResourceSample(
        t=time.monotonic(),
        rss_kb=_proc_status_field("VmRSS"),
        fd_count=len(fds),
        tty_fd_count=sum(1 for p in fds if "/dev/tty" in p),
        thread_count=_proc_status_field("Threads"),
        py_thread_count=threading.active_count(),
        depz_reader_threads=sum(
            1 for t in threading.enumerate() if t.name.startswith("depz-reader-")
        ),
        asyncio_tasks=n_tasks,
    )


def wait_for_threads_to_die(prefix: str = "depz-reader-", timeout: float = 5.0) -> list[str]:
    """Wait for SDK reader threads to exit; return the names still alive.

    ``close()`` joins its own reader with a timeout and returns regardless, so
    a leaked thread is invisible to the caller — this is how we catch it.
    """
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        alive = [t.name for t in threading.enumerate() if t.name.startswith(prefix)]
        if not alive:
            return []
        time.sleep(0.05)
    return [t.name for t in threading.enumerate() if t.name.startswith(prefix)]


# ── hang forensics ───────────────────────────────────────────────────────────


# ── optional serial byte trace (DEPZ_QA_BYTE_TRACE=1) ───────────────────────

#: Ring of (monotonic_t, "tx"/"rx", bytes) shared by every SerialLink.
_BYTE_TRACE: collections.deque = collections.deque(maxlen=800)
_byte_trace_installed = False


def byte_trace_enabled() -> bool:
    return _byte_trace_installed


def install_byte_trace() -> None:
    """Tee every SerialLink read/write into a ring buffer.

    Diagnostic aid for exchange-loss events (F-24): dump_diagnostics() appends
    the ring to its bundle, so a failure dump shows whether the command
    reached the wire and whether any reply bytes ever came back. Enabled via
    ``DEPZ_QA_BYTE_TRACE=1`` (conftest); off by default because hex-dumping a
    50 Hz stream costs real time in tight reconnect loops."""
    global _byte_trace_installed
    if _byte_trace_installed:
        return
    from depz_sensor_sdk.transport import serial_link

    orig_read = serial_link.SerialLink.read
    orig_write = serial_link.SerialLink.write

    def traced_read(self, timeout=None):
        data = orig_read(self, timeout)
        if data:
            _BYTE_TRACE.append((time.monotonic(), "rx", bytes(data)))
        return data

    def traced_write(self, data):
        _BYTE_TRACE.append((time.monotonic(), "tx", bytes(data)))
        return orig_write(self, data)

    serial_link.SerialLink.read = traced_read
    serial_link.SerialLink.write = traced_write
    _byte_trace_installed = True


def dump_diagnostics(tag: str, extra: dict[str, Any] | None = None) -> Path:
    """Write a forensic bundle for a hang/failure and return its path.

    Captures what you cannot reconstruct after the fact: every thread's stack,
    the open fd table, resource counters, and whatever context the caller adds
    (cycle number, seed, stable id, lifecycle state).
    """
    ARTIFACTS_DIR.mkdir(parents=True, exist_ok=True)
    stamp = time.strftime("%Y%m%d-%H%M%S")
    path = ARTIFACTS_DIR / f"hang-{tag}-{stamp}-{os.getpid()}.txt"
    lines: list[str] = [
        f"# DEPZ QA diagnostics: {tag}",
        f"time: {time.strftime('%Y-%m-%d %H:%M:%S')}",
        f"pid: {os.getpid()}",
        "",
        "## context",
        json.dumps(extra or {}, indent=2, default=str),
        "",
        "## resources",
        json.dumps(sample_resources().as_dict(), indent=2),
        "",
        "## threads",
    ]
    frames = sys._current_frames()
    for thread in threading.enumerate():
        lines.append(f"--- {thread.name} (daemon={thread.daemon}, alive={thread.is_alive()})")
        frame = frames.get(thread.ident or -1)
        if frame is not None:
            lines.extend(traceback.format_stack(frame))
        else:
            lines.append("  <no frame>")
    lines.append("")
    lines.append("## open fds")
    for p in sorted(_fd_paths()):
        lines.append(f"  {p}")
    lines.append("")
    lines.append("## asyncio tasks")
    try:
        import asyncio

        try:
            asyncio.get_running_loop()
            tasks = asyncio.all_tasks()
        except RuntimeError:
            tasks = set()
        for task in tasks:
            lines.append(f"  {task!r}")
            stack = task.get_stack(limit=8)
            for f in stack:
                lines.append(f"      {f.f_code.co_filename}:{f.f_lineno} in {f.f_code.co_name}")
        if not tasks:
            lines.append("  <no running event loop in this thread>")
    except Exception as exc:
        lines.append(f"  <unavailable: {exc}>")
    if _BYTE_TRACE:
        lines.append("")
        lines.append("## serial byte trace (oldest first; LATE = after the dump's t0)")
        now = time.monotonic()
        for t, direction, payload in list(_BYTE_TRACE):
            mark = "->" if direction == "tx" else "<-"
            lines.append(f"  {t:.6f} ({t - now:+.3f}s) {mark} {payload.hex(' ')}")
    path.write_text("\n".join(lines))
    return path


@contextlib.contextmanager
def hang_guard(tag: str, seconds: float, context: dict[str, Any] | None = None):
    """Dump diagnostics if the wrapped block outlives ``seconds``.

    This is the safety net that turns "CI hung, no idea why" into a stack dump.
    It never kills the block — pytest-timeout does that — it only makes sure
    forensics exist *before* the process is torn down.
    """
    fired = threading.Event()

    def _fire() -> None:
        if not fired.wait(seconds):
            dump_diagnostics(tag, {**(context or {}), "reason": f"exceeded {seconds}s"})

    watchdog = threading.Thread(target=_fire, name=f"hang-guard-{tag}", daemon=True)
    watchdog.start()
    try:
        yield
    finally:
        fired.set()
        watchdog.join(timeout=1.0)


def enable_faulthandler(timeout: float) -> None:
    """Last-resort: let the interpreter itself dump every stack if we wedge."""
    ARTIFACTS_DIR.mkdir(parents=True, exist_ok=True)
    fh = open(ARTIFACTS_DIR / "faulthandler.log", "a", buffering=1)
    faulthandler.enable(file=fh)
    faulthandler.dump_traceback_later(timeout, repeat=False, file=fh, exit=False)


# ── timing helpers ───────────────────────────────────────────────────────────


def wait_until(
    predicate: Callable[[], bool],
    timeout: float,
    *,
    interval: float = 0.02,
    what: str = "condition",
) -> float:
    """Poll ``predicate`` until true; return elapsed seconds or raise.

    Every hardware wait in this suite goes through here so that no test can
    accidentally block forever on a device that stopped answering.
    """
    start = time.monotonic()
    deadline = start + timeout
    while time.monotonic() < deadline:
        if predicate():
            return time.monotonic() - start
        time.sleep(interval)
    raise TimeoutError(f"timed out after {timeout}s waiting for {what}")


@dataclass
class CycleMetrics:
    """Per-cycle reconnect measurements, aggregated into the JSON report."""

    cycle: int
    ok: bool
    open_s: float = 0.0
    start_s: float = 0.0
    first_frame_s: float = 0.0
    stop_s: float = 0.0
    close_s: float = 0.0
    total_s: float = 0.0
    frames: int = 0
    retries: int = 0
    error: str = ""
    resources: dict[str, Any] = field(default_factory=dict)


def write_report(name: str, payload: dict[str, Any]) -> Path:
    """Persist a machine-readable QA result next to the other artifacts."""
    ARTIFACTS_DIR.mkdir(parents=True, exist_ok=True)
    path = ARTIFACTS_DIR / name
    path.write_text(json.dumps(payload, indent=2, default=str))
    return path


def toolchain_info() -> dict[str, str]:
    """Versions that materially change hardware behaviour, for the report."""
    import platform

    import serial

    info = {
        "python": sys.version.split()[0],
        "platform": platform.platform(),
        "pyserial": serial.__version__,
    }
    with contextlib.suppress(Exception):
        info["kernel"] = subprocess.run(
            ["uname", "-r"], capture_output=True, text=True, timeout=5
        ).stdout.strip()
    with contextlib.suppress(Exception):
        import depz_sensor_sdk

        info["depz_sensor_sdk"] = depz_sensor_sdk.__version__
    return info
