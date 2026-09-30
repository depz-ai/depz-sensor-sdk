"""DEPZ sensor line SDK — SR04, VL53L4CD, VL53L8CX/CH, VL53L5CX/L7CX/L7CH,
VL53L0X/L1CX/L1CB/L3CX/L4CX, BNO086, BNO055 over USB CDC-ACM.

Public surface is defined by contracts/07_SDK_FACADE.md.
"""

from . import transport, protocol
from .device import (
    DeviceBase,
    DeviceEvent,
    DisconnectedEvent,
    LinkCrcErrorEvent,
    LinkStats,
    SequenceErrorEvent,
    StreamQueue,
    TemperatureEvent,
    TextEvent,
    TimeSync,
    TrashEvent,
    UnsolicitedStatusEvent,
    host_now_us,
    sync_time_all,
)
from .bootloader import BootloaderDevice, find_bootloader_port, update_firmware
from .dataset import DatasetReader, DatasetRecord, SessionRecorder
from .protocol.bootloader import FwDepzImage
from .device import StreamIterator
from .discovery import DeviceInfo, list_depz_devices, open_device, probe_port
from .vl53l4 import Vl53l4cd, Vl53l4Info, Vl53l4Measurement
from .vl53l8 import CnhConfig, Vl53l8, Vl53l8ch, Vl53l8cx, Vl53l8Frame
from .vl53l7 import Vl53l5cx, Vl53l7ch, Vl53l7cx, Vl53l7Frame, Vl53l7Info
from .vl53lx import (
    Vl53l0x,
    Vl53l1cb,
    Vl53l1cx,
    Vl53l3cx,
    Vl53l4cx,
    Vl53lx,
    Vl53lxInfo,
    Vl53lxMeasurement,
)
from .errors import (
    BusyError,
    DepzError,
    DepzTimeoutError,
    DeviceLostError,
    LinkClosedError,
    NoDepzDeviceError,
    StatusError,
)
from .usb_ids import (
    DEPZ_USB_VID,
    DEPZ_PID_MODEL,
    is_known_depz_usb,
    usb_model_hint,
)
from .sr04 import Sr04, Sr04Measurement
from .bno086 import Bno086, GyroIntegratedRV, RotationVector, SensorId
from .bno055 import (
    AxisRemap,
    Bno055,
    Bno055Info,
    Bno055Sample,
    CalibrationProfile,
    CalibStatus,
    OprMode,
    Units,
)

try:
    from importlib.metadata import PackageNotFoundError, version as _pkg_version

    __version__ = _pkg_version("depz-sensor-sdk")
except (ImportError, PackageNotFoundError):  # pragma: no cover - source checkout fallback
    __version__ = "0.1.1"

__all__ = [
    # modules
    "transport",
    "protocol",
    # discovery
    "DeviceInfo",
    "list_depz_devices",
    "probe_port",
    "open_device",
    # usb identity table
    "DEPZ_USB_VID",
    "DEPZ_PID_MODEL",
    "is_known_depz_usb",
    "usb_model_hint",
    # device
    "DeviceBase",
    "TimeSync",
    "LinkStats",
    "StreamQueue",
    "host_now_us",
    "sync_time_all",
    # events
    "DeviceEvent",
    "SequenceErrorEvent",
    "LinkCrcErrorEvent",
    "TrashEvent",
    "UnsolicitedStatusEvent",
    "TextEvent",
    "TemperatureEvent",
    "DisconnectedEvent",
    # sensors
    "Sr04",
    "Sr04Measurement",
    "Vl53l4cd",
    "Vl53l4Measurement",
    "Vl53l4Info",
    "Vl53l8",
    "Vl53l8cx",
    "Vl53l8ch",
    "Vl53l8Frame",
    "Vl53l5cx",
    "Vl53l7cx",
    "Vl53l7ch",
    "Vl53l7Frame",
    "Vl53l7Info",
    "Vl53lx",
    "Vl53l0x",
    "Vl53l1cx",
    "Vl53l1cb",
    "Vl53l3cx",
    "Vl53l4cx",
    "Vl53lxMeasurement",
    "Vl53lxInfo",
    "CnhConfig",
    "Bno086",
    "SensorId",
    "RotationVector",
    "GyroIntegratedRV",
    "Bno055",
    "Bno055Info",
    "Bno055Sample",
    "OprMode",
    "Units",
    "AxisRemap",
    "CalibStatus",
    "CalibrationProfile",
    "StreamIterator",
    # datasets (contract 09)
    "SessionRecorder",
    "DatasetReader",
    "DatasetRecord",
    # bootloader / firmware update (contract 06)
    "BootloaderDevice",
    "FwDepzImage",
    "update_firmware",
    "find_bootloader_port",
    # errors
    "DepzError",
    "DepzTimeoutError",
    "StatusError",
    "BusyError",
    "DeviceLostError",
    "LinkClosedError",
    "NoDepzDeviceError",
    "__version__",
]
