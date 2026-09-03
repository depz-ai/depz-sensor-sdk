"""DEPZ sensor line SDK — SR04, VL53L4CD, VL53L8CX/CH, BNO086 over USB CDC-ACM.

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
from .vl53l4 import Vl53l4Cd, Vl53l4Info, Vl53l4Measurement
from .vl53l8 import CnhConfig, Vl53l8, Vl53l8Ch, Vl53l8Cx, Vl53l8Frame
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
    "Vl53l4Cd",
    "Vl53l4Measurement",
    "Vl53l4Info",
    "Vl53l8",
    "Vl53l8Cx",
    "Vl53l8Ch",
    "Vl53l8Frame",
    "CnhConfig",
    "Bno086",
    "SensorId",
    "RotationVector",
    "GyroIntegratedRV",
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
