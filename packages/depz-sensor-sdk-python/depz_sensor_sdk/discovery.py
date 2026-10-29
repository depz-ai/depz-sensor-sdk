"""Device discovery: pick the right DEPZ serial port by USB identity, then
probe it (contract 02 §4).

Default selection considers only ports whose USB (vid, pid) is a known DEPZ
identity (`usb_ids`), ordered by USB iSerial — never "first port the OS
enumerated". The protocol probe (GET_NAME_ACTIVE_SOFTWARE) still decides what a
device actually is; the USB table only chooses which port(s) to talk to.
"""

from __future__ import annotations

import os
import re
import stat
import warnings
from dataclasses import dataclass
from typing import Literal

from serial.tools import list_ports

from .device import DeviceBase
from .errors import DepzError, DepzTimeoutError, DeviceLostError, NoDepzDeviceError
from .protocol._wire import check_uint
from .protocol.identity import SensorType, parse_software_name
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


def _device_node(path: str) -> int | None:
    """The device number of the character device at `path`, None for anything
    else (a regular file, a missing path). A zero device number is None too:
    a platform that stats every port as an empty character device must not
    make them one device."""
    try:
        st = os.stat(path)
    except OSError:
        return None
    if not stat.S_ISCHR(st.st_mode) or st.st_rdev == 0:
        return None
    return st.st_rdev


def _port_id(target: str, enumerated: list[_PortId] | None = None) -> _PortId:
    """`target` with the USB identity of the enumerated port it names
    (contract 02 §4.3, ERRATA A5): the port whose path is the same string,
    else — on POSIX — the one whose resolved path is the same
    (`/dev/serial/by-id/…` is a symlink to the node), else the one that is
    the same character device under another name. On Windows a COM name is
    matched by string only: a COM port is not a file, and resolving one
    (realpath, stat) opens it — a DTR pulse on every board of the bench. A
    path the OS does not list keeps `target` with no USB identity. The port
    stays `target`: an explicit path is opened as given."""
    if enumerated is None:
        enumerated = _enumerate_ports()
    match = next((pi for pi in enumerated if pi.port == target), None)
    if match is None and os.name == "posix":
        real = os.path.realpath(target)
        match = next((pi for pi in enumerated if os.path.realpath(pi.port) == real), None)
        if match is None:
            node = _device_node(target)
            if node is not None:
                match = next((pi for pi in enumerated if _device_node(pi.port) == node), None)
    if match is None:
        return _PortId(target, None, None, None)
    return _PortId(target, match.vid, match.pid, match.usb_serial)


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


def _probe(dev: DeviceBase) -> tuple[str, str, str]:
    """The probe of contract 02 §4.4 on an open connection: exactly
    GET_NAME_ACTIVE_SOFTWARE (required — its failure propagates), then
    GET_DEVICE_NAME and GET_SERIAL (optional — a timeout or error leaves ""),
    in that order. Returns (software_name, device_name, serial_number)."""
    software = dev.get_software_name()
    try:
        device_name = dev.get_device_name()
    except DepzError:
        device_name = ""
    try:
        serial_number = dev.get_serial_number()
    except DepzError:
        serial_number = ""
    return software, device_name, serial_number


def probe_port(port: str, *, timeout: float = 0.2) -> DeviceInfo | None:
    """Open `port`, run the contract 02 §4.4 probe (GET_NAME_ACTIVE_SOFTWARE,
    GET_DEVICE_NAME, GET_SERIAL), close.

    Returns None when nothing DEPZ-shaped answers. Note: probing opens the
    port — skip ports owned by other software via the `ports=` argument of
    `list_depz_devices`.
    """
    try:
        return _probe_port(port, timeout)
    except (DeviceLostError, DepzError):
        return None


def _probe_port(port: str, timeout: float) -> DeviceInfo | None:
    """`probe_port`, except that a port that cannot be opened raises the open
    failure (busy, no permission, gone) instead of returning None — so
    `open_device` can name it (contract 07 §3)."""
    dev = DeviceBase(port, timeout=timeout)
    try:
        software, device_name, serial_number = _probe(dev)
        ident = parse_software_name(software)
        return DeviceInfo(
            port=port,
            mode=ident.mode,
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
    enumerated = _enumerate_ports()
    if ports is None:
        candidates = list(enumerated)
    else:
        candidates = [_port_id(p, enumerated) for p in ports]
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
                    }
                )
            )
    return found


def open_device(
    target: str | int | DeviceInfo | Link | None = None,
    *,
    serial: str | None = None,
    timeout: float = 0.2,
) -> DeviceBase:
    """Open the right sensor class for `target` (contract 02 §4.3, §4.4).

    - ``None`` (default): the DEPZ candidate port with the alphabetically
      smallest USB serial (index 0); with ``serial=`` the candidate whose USB
      serial matches. No candidate → `NoDepzDeviceError`.
    - ``int N``: the Nth DEPZ candidate, candidates sorted by USB serial
      (0-based). Out of range → `NoDepzDeviceError`.
    - ``str`` (a port path): open exactly that port; its USB (vid, pid) is
      that of the enumerated port the path names under any alias
      (`/dev/serial/by-id/…`, a symlink, another name of the same device
      node). If it is not a known DEPZ id — or the OS does not list the
      port — warn but proceed. Nothing DEPZ answering the probe there →
      `NoDepzDeviceError`, as on a link.
    - ``DeviceInfo``: its ``.port``, the class resolved from its probe answers.
    - ``Link``/transport: `open_on_link` (probe and continue on that link).

    On a port the probe runs on a connection of its own (`probe_port`), which
    is closed; the resolved class then opens the port afresh, so host packet
    sequence numbers restart at 0. Bootloader-mode devices raise (use the
    contract 06 client).
    """
    if isinstance(target, Link):
        return open_on_link(target, timeout=timeout)

    if isinstance(target, DeviceInfo):
        if serial is not None and target.usb_serial not in (None, serial):
            warnings.warn(
                f"requested serial {serial!r} but DeviceInfo serial is "
                f"{target.usb_serial!r}",
                stacklevel=2,
            )
        return _open_resolved(target.port, target, target.usb_vid, target.usb_pid, timeout)

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
        pi = _port_id(target)
        if not is_known_depz_usb(pi.vid, pi.pid):
            warnings.warn(
                f"{target}: USB id "
                f"{_fmt_vidpid(pi.vid, pi.pid)} is not a known DEPZ device — "
                "opening anyway (unprogrammed unit / custom setup?)",
                stacklevel=2,
            )
        if serial is not None and pi.usb_serial not in (None, serial):
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


def open_on_link(
    link: Link,
    usb_vid: int | None = None,
    usb_pid: int | None = None,
    *,
    timeout: float = 0.2,
) -> DeviceBase:
    """Open a device on an already-open `link` that cannot be reopened (a
    replay or recording link, an adopted transport) — contract 02 §4.4.

    Runs the same probe as `probe_port` on that link (GET_NAME_ACTIVE_SOFTWARE,
    GET_DEVICE_NAME, GET_SERIAL), resolves the class with
    `resolve_sensor_class` from the answers and the port's USB (vid, pid) —
    which the caller supplies, a link knows none — and continues as that class
    on the same connection (`attach_sensor_class`); host sequence numbers run
    on from the probe. The returned device owns `link`: on failure it is
    closed. No answer to GET_NAME_ACTIVE_SOFTWARE → `NoDepzDeviceError`;
    a bootloader → `DepzError`.
    """
    dev = DeviceBase(link, timeout=timeout)
    try:
        try:
            software, device_name, _serial = _probe(dev)
        except DepzError as exc:
            raise NoDepzDeviceError(f"no DEPZ device answered on the link ({exc})") from exc
        return attach_sensor_class(
            dev, resolve_sensor_class(software, device_name, usb_vid, usb_pid)
        )
    except BaseException:
        dev.close()
        raise


def _fmt_vidpid(vid: int | None, pid: int | None) -> str:
    if vid is None or pid is None:
        return "unknown"
    return f"{vid:04X}:{pid:04X}"


def _open_probed(port: str, timeout: float) -> DeviceBase:
    try:
        info = _probe_port(port, timeout)
    except (DeviceLostError, DepzError) as exc:
        raise NoDepzDeviceError(
            f"no DEPZ device answered on {port}: could not open the port ({exc})"
        ) from exc
    if info is None:
        raise NoDepzDeviceError(f"no DEPZ device answered on {port}")
    pi = _port_id(port)
    return _open_resolved(port, info, pi.vid, pi.pid, timeout)


def _open_resolved(port: str, info: DeviceInfo, usb_vid: int | None, usb_pid: int | None,
                   timeout: float) -> DeviceBase:
    """Open the class the probe answers resolve to on a fresh connection."""
    name = resolve_sensor_class(info.software_name, info.device_name, usb_vid, usb_pid)
    if name == "bootloader":
        raise DepzError("device is in bootloader mode; use the bootloader client (contract 06)")
    return sensor_class(name)(port, timeout=timeout)


# ── class resolution (contract 02 §4.4) ───────────────────────────────────────

#: Every name `resolve_sensor_class` returns: the canonical sensor classes of
#: contract 07 §1, ``"vl53lx"`` (the generic 1D-family class: an unstamped
#: board, or a VL53L4CD on the APP_VL53L0_4 firmware), ``"tmf8829"``,
#: ``"device"`` (the base device: an unknown application, or a name that is
#: neither APP_ nor BOOTDEPZ) and ``"bootloader"`` (no sensor class: use the
#: contract 06 client). BNO085 boards run APP_BNO086 and resolve to ``"bno086"``.
SENSOR_CLASS_NAMES: tuple[str, ...] = (
    "sr04", "vl53l4cd", "vl53l8cx", "vl53l8ch", "vl53l5cx", "vl53l7cx", "vl53l7ch",
    "vl53l0x", "vl53l1cx", "vl53l1cb", "vl53l3cx", "vl53l4cx", "vl53lx",
    "bno086", "bno055", "tmf8829", "device", "bootloader",
)

_VL53L7_PART_RE = re.compile(r"VL53L([57])(CX|CH)")
_VL53L7_MODELS = ("vl53l5cx", "vl53l7cx", "vl53l7ch")
# 1D products with a class of their own; VL53L4CD on APP_VL53L0_4 is generic.
_VL53LX_OWN_CLASS = ("VL53L0X", "VL53L1CX", "VL53L1CB", "VL53L3CX", "VL53L4CX")


def _vl53l7_model(usb_model: str | None, device_name: str) -> str:
    """L5/L7 model: the production USB model, then the part stamped in the
    device name (`… VL53L7CH USB v2.1 …`), then the CX base (contract 11 §1;
    the silicon cannot tell CX from CH, and the CX blob runs on every part)."""
    if usb_model in _VL53L7_MODELS:
        return usb_model
    m = _VL53L7_PART_RE.search(device_name or "")
    if m:
        part = f"vl53l{m.group(1)}{m.group(2)}".lower()
        if part in _VL53L7_MODELS:
            return part
    return "vl53l7cx"


def _vl53lx_name(usb_model: str | None, device_name: str) -> str:
    """1D-family class: the production USB model if it is one of the six
    products, else the product stamped in the device name, else the generic
    class (contract 12 §1)."""
    from .vl53lx.uld import registry

    product = (usb_model or "").upper() or None
    if product not in registry.PRODUCTS:
        product = registry.product_from_board_name(device_name or "")
    return product.lower() if product in _VL53LX_OWN_CLASS else "vl53lx"


def resolve_sensor_class(
    software_name: str,
    device_name: str = "",
    usb_vid: int | None = None,
    usb_pid: int | None = None,
) -> str:
    """The canonical class name for a device's probe answers and its port's
    USB (vid, pid) — contract 02 §4.4, pure. One of `SENSOR_CLASS_NAMES`.

    `software_name` is the GET_NAME_ACTIVE_SOFTWARE answer, `device_name` the
    GET_DEVICE_NAME answer ("" when there was none); the USB id is None when
    unknown (a replay, a bridge). Vectors: ``vectors/probe.json``.
    """
    # Each USB id, when known, is a u16 (contract 07 §3) — even if the other is not.
    if usb_vid is not None:
        check_uint("usb_vid", usb_vid, 16)
    if usb_pid is not None:
        check_uint("usb_pid", usb_pid, 16)
    ident = parse_software_name(software_name)
    if ident.mode == "bootloader":
        return "bootloader"
    model = usb_model_hint(usb_vid, usb_pid)
    sensor = ident.sensor_type
    if sensor == SensorType.SR04:
        return "sr04"
    if sensor == SensorType.VL53L8:
        # CX and CH share firmware name and silicon ids: only the production
        # PID tells them apart; the dev default (0483:56DC) falls to the CX base.
        return "vl53l8ch" if model == "vl53l8ch" else "vl53l8cx"
    if sensor == SensorType.VL53L7:
        return _vl53l7_model(model, device_name)
    if sensor == SensorType.VL53LX:
        return _vl53lx_name(model, device_name)
    if sensor == SensorType.VL53L4:
        return "vl53l4cd"
    if sensor == SensorType.BNO086:
        return "bno086"
    if sensor == SensorType.BNO055:
        return "bno055"
    if sensor == SensorType.TMF8829:
        return "tmf8829"
    return "device"


def sensor_class(name: str) -> type[DeviceBase]:
    """The Python class for a `resolve_sensor_class` name (not "bootloader")."""
    if name == "device":
        return DeviceBase
    if name == "sr04":
        return Sr04
    if name == "vl53l4cd":
        from .vl53l4 import Vl53l4cd

        return Vl53l4cd
    if name in ("vl53l8cx", "vl53l8ch"):
        from . import vl53l8

        return vl53l8.Vl53l8ch if name == "vl53l8ch" else vl53l8.Vl53l8cx
    if name in _VL53L7_MODELS:
        from .vl53l7 import Vl53l5cx, Vl53l7ch, Vl53l7cx

        return {"vl53l5cx": Vl53l5cx, "vl53l7cx": Vl53l7cx, "vl53l7ch": Vl53l7ch}[name]
    if name == "vl53lx" or name.upper() in _VL53LX_OWN_CLASS:
        from .vl53lx import CLASS_BY_PRODUCT, Vl53lx

        return CLASS_BY_PRODUCT.get(name.upper(), Vl53lx)
    if name == "bno086":
        from .bno086 import Bno086

        return Bno086
    if name == "bno055":
        from .bno055 import Bno055

        return Bno055
    if name == "tmf8829":
        from .tmf8829 import Tmf8829

        return Tmf8829
    raise ValueError(f"no sensor class {name!r}")


def attach_sensor_class(dev: DeviceBase, class_name: str) -> DeviceBase:
    """Continue the open base device `dev` as `class_name` on the same
    connection — no reopen, sequence numbers and reader run on (contract 02
    §4.4). `dev` must be a plain `DeviceBase` (no subclass yet); returns it,
    now an instance of that class. "bootloader" raises `DepzError`.

    State first, class swap second: the reader thread may dispatch a report
    between the two statements."""
    if class_name == "bootloader":
        raise DepzError("device is in bootloader mode; use the bootloader client (contract 06)")
    cls = sensor_class(class_name)
    if type(dev) is not DeviceBase:
        raise TypeError(f"attach_sensor_class needs a plain DeviceBase, got {type(dev).__name__}")
    if cls is DeviceBase:
        return dev
    cls._init_subclass_state(dev)
    dev.__class__ = cls
    return dev


# Kept for the vector generators and tests that pick a family class directly.
def _vl53l7_class(usb_model: str | None, device_name: str) -> type[DeviceBase]:
    return sensor_class(_vl53l7_model(usb_model, device_name))


def _vl53lx_class(usb_model: str | None, device_name: str) -> type[DeviceBase]:
    return sensor_class(_vl53lx_name(usb_model, device_name))
