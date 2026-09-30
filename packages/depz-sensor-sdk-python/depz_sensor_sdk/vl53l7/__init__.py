"""VL53L5CX / VL53L7CX / VL53L7CH ToF sensors over the I2C register bridge.

One firmware (`APP_VL53L7`) serves all three boards (contracts/11): the MCU is
a thin I2C bridge whose wire protocol is the VL53L8 one (contract 04) plus
three commands. The sensor-side logic is the same ST ULD family as VL53L8, so
these classes reuse `Vl53l8cx` and override only what differs:

- the sensor-firmware blob set (`l7cx` for L5CX/L7CX, `l7ch` for L7CH) and
  the L5/L7 branch of the ULD boot sequence (in `vl53l8.uld`);
- register-bridge transfer limits (reads ≤ 1536 B per call);
- 1 Hz ranging works (L8 needs ≥ 2 Hz);
- board commands: pin control, bridge counters, I2C speed.

Frames are the same `Vl53l8Frame` (exported here as `Vl53l7Frame`).

The advanced ULD plugins (power modes, xtalk calibration, detection
thresholds, motion indicator) share the VL53L8 DCI sequences — verified
against ST ULD 2.0.1 (L5CX/L7CX) and VL53LMZ 2.0.16 (L7CH). L5CX/L7CX have
no deep sleep and no threshold auto-stop.
"""

from __future__ import annotations

import warnings

from ..protocol.vl53l7 import (
    READ_MAX_LEN,
    WRITE_MAX_LEN,
    I2cError,
    PinAction,
    Vl53l7Cmd,
    Vl53l7Info,
    Vl53l7Rpt,
    pack_pin_ctrl,
    pack_set_i2c_speed,
)
from ..device import DeviceBase
from ..vl53l8 import CnhConfig, CnhMixin, Vl53l8cx, Vl53l8Frame
from ..vl53l8.uld import (
    MODULE_TYPE_MZ,
    MODULE_TYPE_MZEVO,
    MODULE_TYPE_NAMES,
    RANGING_MODE_AUTONOMOUS,
    RANGING_MODE_CONTINUOUS,
    RESOLUTION_4X4,
    RESOLUTION_8X8,
    TARGET_ORDER_CLOSEST,
    TARGET_ORDER_STRONGEST,
)

__all__ = [
    "Vl53l7cx",
    "Vl53l5cx",
    "Vl53l7ch",
    "Vl53l7Frame",
    "Vl53l7Info",
    "PinAction",
    "I2cError",
    "CnhConfig",
    "MODULE_TYPE_MZ",
    "MODULE_TYPE_MZEVO",
    "RESOLUTION_4X4",
    "RESOLUTION_8X8",
    "RANGING_MODE_CONTINUOUS",
    "RANGING_MODE_AUTONOMOUS",
    "TARGET_ORDER_CLOSEST",
    "TARGET_ORDER_STRONGEST",
]

Vl53l7Frame = Vl53l8Frame

MIN_RANGING_FREQUENCY_HZ = 1  # L5/L7 range and stream at 1 Hz (contract 11)

class Vl53l7cx(Vl53l8cx):
    """VL53L7CX ToF device (8×8 zones, 90° field of view). `init()` downloads
    the ~84 KB L5/L7 sensor firmware (~1.4 s at the board's default 1 MHz
    I2C), then configure and `start_ranging()` exactly as on `Vl53l8cx`.

    Base class of the I2C family: `Vl53l5cx` (same blob, MZ module) and
    `Vl53l7ch` (CH blob, adds CNH) inherit it."""

    _VARIANT = "l7cx"
    _READ_CHUNK = READ_MAX_LEN
    _WRITE_CHUNK = WRITE_MAX_LEN
    _MIN_RANGING_HZ = MIN_RANGING_FREQUENCY_HZ
    #: module_type this class expects after init() (L5 and L7 share a blob).
    _MODULE_TYPE = MODULE_TYPE_MZEVO

    def init(self, variant=None, *, progress=None, write_progress=None) -> None:
        """Initialize the sensor: firmware blob download + default config.

        Refuses non-L5/L7 silicon. After the sensor firmware runs, its
        `module_type` tells L5 (MZ) from L7 (MZEVO); a mismatch with this
        class (a board labelled L7 carrying an L5, or vice versa) is reported
        as a warning — ranging still works, since the blob is shared."""
        super().init(variant, progress=progress, write_progress=write_progress)
        got = self.module_type
        if got is not None and got != self._MODULE_TYPE:
            warnings.warn(
                f"{type(self).__name__} expects module type "
                f"{MODULE_TYPE_NAMES[self._MODULE_TYPE]}, the sensor reports "
                f"{MODULE_TYPE_NAMES.get(got, got)} — check the board's device name",
                stacklevel=2,
            )

    @property
    def module_type(self) -> int | None:
        """Sensor module type read at init(): MODULE_TYPE_MZ (0) = VL53L5CX,
        MODULE_TYPE_MZEVO (1) = VL53L7CX/CH. None before init()."""
        return self._uld.module_type if self._uld is not None else None

    # ── board commands (contract 11 §2) ──────────────────────────────────────

    def get_bridge_info(self) -> Vl53l7Info:
        """Bridge counters and pin levels (never touches the sensor). Read it
        before and after a run, not during one: each call takes the bus from
        the stream and can itself cost a frame."""
        return self.request(
            Vl53l7Cmd.GET_INFO,
            matcher=DeviceBase.expect_report(Vl53l7Rpt.VL53_INFO, Vl53l7Info.unpack),
        )

    def set_i2c_speed_khz(self, khz: int) -> int:
        """Set the sensor-bus SCL frequency; the board snaps to the nearest of
        100, 200, 400, 500 … 1000 kHz. Returns the value now in effect. Raises
        `BusyError` mid-transfer — stop ranging first."""
        if not 1 <= khz <= 0xFFFF:
            raise ValueError("khz must be 1..65535")
        self.request(Vl53l7Cmd.SET_I2C_SPEED, pack_set_i2c_speed(khz), ok_completes=True)
        return self.get_bridge_info().i2c_khz

    def pin_ctrl(self, action: int) -> None:
        """Drive the sensor's LPn / I2C_RST pins (`PinAction`). LPN_OFF and
        SOFT_CYCLE drop the sensor's state: run `init()` again afterwards."""
        action = PinAction(action)
        self.request(Vl53l7Cmd.PIN_CTRL, pack_pin_ctrl(action), ok_completes=True)
        if action in (PinAction.LPN_OFF, PinAction.SOFT_CYCLE):
            self._ranging = False
            self._uld = None

    # ── ULD plugins (contract 11 §4) ─────────────────────────────────────────
    # Power modes, detection thresholds, motion indicator and xtalk
    # calibration are inherited from Vl53l8cx: their DCI sequences are the
    # same on L5/L7 (verified against ST ULD 2.0.1 and VL53LMZ 2.0.16). The
    # ULD switches in the L5/L7 xtalk table and calibration output list, and
    # enforces the L5CX/L7CX limits: no DEEP_SLEEP, no threshold auto-stop.

    @property
    def xtalk_calibration_failed(self) -> bool:
        """True when the last calibrate_xtalk() found nothing to calibrate (ST
        XTALK_FAILED: "coverglass too good") — the sensor keeps its default
        xtalk data."""
        return bool(self._uld is not None and self._uld.xtalk_calibration_failed)


class Vl53l5cx(Vl53l7cx):
    """VL53L5CX ToF device (8×8 zones, 63° field of view). Same board, blob
    and API as `Vl53l7cx`; the sensor reports module type MZ."""

    _MODULE_TYPE = MODULE_TYPE_MZ


class Vl53l7ch(CnhMixin, Vl53l7cx):
    """VL53L7CH ToF device: the VL53L7CX superset. `init()` downloads the CH
    firmware blob (VL53LMZ ULD 2.0.16, the same blob as VL53L8CH) and adds
    Compact-Network-Histogram output (`configure_cnh`). CNH frames up to
    ~7.6 KB stream in chunks (≤ 8192 B total)."""

    _VARIANT = "l7ch"
