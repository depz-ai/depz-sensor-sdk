"""Device discovery: pick the right DEPZ serial port by USB identity, then
probe it (contract 02 §4).

Default selection considers only ports whose USB (vid, pid) is a known DEPZ
identity (`usb_ids`), ordered by USB iSerial — never "first port the OS
enumerated". The protocol probe (GET_NAME_ACTIVE_SOFTWARE) still decides what a
device actually is; the USB table only chooses which port(s) to talk to.
"""

from __future__ import annotations

import warnings
from dataclasses import dataclass
from typing import Literal

from serial.tools import list_ports

from .device import DeviceBase
from .errors import DepzError, DepzTimeoutError, DeviceLostError, NoDepzDeviceError
from .protocol.identity import Identity, SensorType, parse_software_name
from .sr04 import Sr04
from .transport.link import Link
from .usb_ids import is_known_depz_usb, usb_model_hint


@dataclass(frozen=True)
class DeviceInfo:
    port: str
    mode: Literal["app", "bootloader", "unknown"]
    sensor_type: SensorType | None
    software_name: str
    fw_version: str
    device_name: str
    serial_number: str  # device protocol serial (GET_SERIAL)
    usb_vid: int | None = None  # informative only — never filtered on
    usb_pid: int | None = None
    usb_serial: str | None = None  # USB iSerial, used for ordering/selection

    @property
    def usb_model_hint(self) -> str | None:
        return usb_model_hint(self.usb_vid, self.usb_pid)


@dataclass(frozen=True)
class _PortId:
    """One enumerated serial port with its USB metadata."""

    port: str
    vid: int | None
    pid: int | None
    usb_serial: str | None


def _enumerate_ports() -> list[_PortId]:
    return [
        _PortId(p.device, p.vid, p.pid, p.serial_number)
        for p in list_ports.comports()
    ]


def _serial_sort_key(pi: _PortId) -> tuple[int, str, str]:
    """Sort by USB serial; None/empty sorts last, tie-break by port path."""
    if pi.usb_serial:
        return (0, pi.usb_serial, pi.port)
    return (1, "", pi.port)


def _depz_candidates() -> list[_PortId]:
    """Enumerated ports with a known DEPZ USB identity, ordered by serial."""
    cands = [pi for pi in _enumerate_ports() if is_known_depz_usb(pi.vid, pi.pid)]
    cands.sort(key=_serial_sort_key)
    return cands


def probe_port(port: str, *, timeout: float = 0.2) -> DeviceInfo | None:
    """Open `port`, ask GET_NAME_ACTIVE_SOFTWARE (+ name/serial), close.

    Returns None when nothing DEPZ-shaped answers. Note: probing opens the
    port — skip ports owned by other software via the `ports=` argument of
    `list_depz_devices`.
    """
    try:
        dev = DeviceBase(port, timeout=timeout)
    except (DeviceLostError, DepzError):
        return None
    try:
        software = dev.get_software_name()
        ident = parse_software_name(software)
        try:
            device_name = dev.get_device_name()
        except (DepzTimeoutError, DepzError):
            device_name = ""
        try:
            serial_number = dev.get_serial_number()
        except (DepzTimeoutError, DepzError):
            serial_number = ""
        return DeviceInfo(
            port=port,
            mode=ident.mode,  # type: ignore[arg-type]
            sensor_type=ident.sensor_type,
            software_name=ident.software_name,
            fw_version=ident.version,
            device_name=device_name,
            serial_number=serial_number,
        )
    except (DepzTimeoutError, DeviceLostError, DepzError):
        return None
    finally:
        dev.close()


def list_depz_devices(
    *, ports: list[str] | None = None, timeout: float = 0.2, match_usb: bool = True
) -> list[DeviceInfo]:
    """Probe candidate serial ports and return every DEPZ device found.

    With `match_usb` (default) only ports whose USB (vid, pid) is a known DEPZ
    identity are probed — fast, and it avoids poking unrelated devices. Pass
    `match_usb=False` for the legacy probe-every-port behavior. `ports`
    overrides enumeration with an explicit list (still USB-annotated when the
    OS knows the port). Results are ordered by USB iSerial.
    """
    enumerated = {pi.port: pi for pi in _enumerate_ports()}
    if ports is None:
        candidates = list(enumerated.values())
        if match_usb:
            candidates = [pi for pi in candidates if is_known_depz_usb(pi.vid, pi.pid)]
    else:
        candidates = [
            enumerated.get(p, _PortId(p, None, None, None)) for p in ports
        ]
        if match_usb:
            candidates = [pi for pi in candidates if is_known_depz_usb(pi.vid, pi.pid)]
    candidates.sort(key=_serial_sort_key)

    found: list[DeviceInfo] = []
    for pi in candidates:
        info = probe_port(pi.port, timeout=timeout)
        if info is not None:
            found.append(
                DeviceInfo(
                    **{
                        **info.__dict__,
                        "usb_vid": pi.vid,
                        "usb_pid": pi.pid,
                        "usb_serial": pi.usb_serial,
                    }  # type: ignore[arg-type]
                )
            )
    return found


def open_device(
    target: str | int | DeviceInfo | Link | None = None,
    *,
    serial: str | None = None,
    timeout: float = 0.2,
):
    """Open the right sensor class for `target`.

    - ``None`` (default): the DEPZ candidate port with the alphabetically
      smallest USB serial (index 0); with ``serial=`` the candidate whose USB
      serial matches. No candidate → `NoDepzDeviceError`.
    - ``int N``: the Nth DEPZ candidate, candidates sorted by USB serial
      (0-based). Out of range → `NoDepzDeviceError`.
    - ``str`` (a port path): open exactly that port; if its USB (vid, pid) is
      not a known DEPZ id, warn but proceed.
    - ``DeviceInfo``: its ``.port``.
    - ``Link``/transport: opened directly (loopback/replay).

    Bootloader-mode devices raise until the bootloader client lands
    (contract 06).
    """
    if isinstance(target, Link):
        dev = DeviceBase(target, timeout=timeout)
        ident = _identify(dev)
        return _promote(dev, ident)

    if isinstance(target, DeviceInfo):
        if serial is not None and target.usb_serial not in (None, serial):
            warnings.warn(
                f"requested serial {serial!r} but DeviceInfo serial is "
                f"{target.usb_serial!r}",
                stacklevel=2,
            )
        return _open_port(
            target.port,
            target.sensor_type,
            target.mode,
            timeout,
            usb_model_hint(target.usb_vid, target.usb_pid),
        )

    if isinstance(target, bool):  # guard: bool is an int subclass
        raise TypeError("open_device target must be None, int, str, DeviceInfo or Link")

    if isinstance(target, int):
        candidates = _depz_candidates()
        if serial is not None:
            candidates = [pi for pi in candidates if pi.usb_serial == serial]
        if not 0 <= target < len(candidates):
            raise NoDepzDeviceError(
                f"no DEPZ candidate at index {target} "
                f"({len(candidates)} candidate(s) found)"
            )
        return _open_probed(candidates[target].port, timeout)

    if isinstance(target, str):
        enumerated = {pi.port: pi for pi in _enumerate_ports()}
        pi = enumerated.get(target)
        if pi is not None and not is_known_depz_usb(pi.vid, pi.pid):
            warnings.warn(
                f"{target}: USB id "
                f"{_fmt_vidpid(pi.vid, pi.pid)} is not a known DEPZ device — "
                "opening anyway (unprogrammed unit / custom setup?)",
                stacklevel=2,
            )
        if serial is not None and pi is not None and pi.usb_serial not in (None, serial):
            warnings.warn(
                f"{target}: USB serial {pi.usb_serial!r} != requested {serial!r}",
                stacklevel=2,
            )
        return _open_probed(target, timeout)

    # target is None: select a candidate by serial / smallest-serial default.
    candidates = _depz_candidates()
    if serial is not None:
        match = [pi for pi in candidates if pi.usb_serial == serial]
        if not match:
            raise NoDepzDeviceError(f"no DEPZ device with USB serial {serial!r}")
        return _open_probed(match[0].port, timeout)
    if not candidates:
        raise NoDepzDeviceError(
            "no DEPZ device found (no serial port has a known DEPZ USB id). "
            "Pass a port path explicitly to open an unprogrammed unit."
        )
    return _open_probed(candidates[0].port, timeout)


def _fmt_vidpid(vid: int | None, pid: int | None) -> str:
    if vid is None or pid is None:
        return "unknown"
    return f"{vid:04X}:{pid:04X}"


def _open_probed(port: str, timeout: float):
    info = probe_port(port, timeout=timeout)
    if info is None:
        raise DeviceLostError(f"no DEPZ device answered on {port}")
    pi = {p.port: p for p in _enumerate_ports()}.get(port)
    model = usb_model_hint(pi.vid, pi.pid) if pi is not None else None
    return _open_port(port, info.sensor_type, info.mode, timeout, model)


def _open_port(
    port: str,
    sensor: SensorType | None,
    mode: str,
    timeout: float,
    usb_model: str | None = None,
):
    if mode == "bootloader":
        raise DepzError("device is in bootloader mode; use the bootloader client (M2)")
    return _open_by_sensor(port, sensor, timeout, usb_model)


def _identify(dev: DeviceBase) -> Identity:
    return parse_software_name(dev.get_software_name())


def _open_by_sensor(
    port: str, sensor: SensorType | None, timeout: float, usb_model: str | None = None
):
    if sensor == SensorType.SR04:
        return Sr04(port, timeout=timeout)
    if sensor == SensorType.VL53L8:
        # CX and CH both report firmware name APP_VL53L8_v*, and the silicon
        # device_id/revision is identical (0xF0/0x0C), so the USB PID is the
        # only thing that tells them apart: 0xED4B → CX, 0xED40 → CH (both
        # ship; both are hw-verified in usb_ids.py). A dev unit carrying the
        # STMicro default 0x56DC can't be identified at all, so it falls back
        # to the CX base — the safe choice, since CH is a strict superset.
        from .vl53l8 import Vl53l8Ch, Vl53l8Cx

        cls = Vl53l8Ch if usb_model == "vl53l8ch" else Vl53l8Cx
        return cls(port, timeout=timeout)
    if sensor == SensorType.VL53L4:
        from .vl53l4 import Vl53l4Cd

        return Vl53l4Cd(port, timeout=timeout)
    if sensor == SensorType.BNO086:
        from .bno086 import Bno086

        return Bno086(port, timeout=timeout)
    return DeviceBase(port, timeout=timeout)


def _promote(dev: DeviceBase, ident: Identity):
    """Re-wrap an already-open Link-based DeviceBase as the right subclass.

    State first, class swap second: the reader thread may dispatch a report
    between the two statements."""
    if ident.sensor_type == SensorType.SR04:
        Sr04._init_subclass_state(dev)  # type: ignore[arg-type]
        dev.__class__ = Sr04
        return dev
    if ident.sensor_type == SensorType.VL53L8:
        from .vl53l8 import Vl53l8

        Vl53l8._init_subclass_state(dev)  # type: ignore[arg-type]
        dev.__class__ = Vl53l8
        return dev
    if ident.sensor_type == SensorType.VL53L4:
        from .vl53l4 import Vl53l4Cd

        Vl53l4Cd._init_subclass_state(dev)  # type: ignore[arg-type]
        dev.__class__ = Vl53l4Cd
        return dev
    if ident.sensor_type == SensorType.BNO086:
        from .bno086 import Bno086

        Bno086._init_subclass_state(dev)  # type: ignore[arg-type]
        dev.__class__ = Bno086
        return dev
    return dev
