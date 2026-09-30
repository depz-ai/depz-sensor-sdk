# VL53L7CX — user guide

Hands-on guide to `Vl53l7cx`. It subclasses `Vl53l8cx`, so the
[VL53L8CX guide](../vl53l8cx/guide.md) applies line for line — configuration,
the frame and its heatmap grid, streaming, the advanced features. This page
covers only what the L7 board adds. For concepts see the
[introduction](introduction.md); for signatures the [API reference](api.md).

## Contents

- [Open and initialize](#open-and-initialize)
- [Hello-world: 8×8 depth frames](#hello-world-8x8-depth-frames)
- [Board commands](#board-commands)
- [Advanced features on L5/L7](#advanced-features-on-l5l7)
- [Gotchas](#gotchas)

## Open and initialize

```python
from depz_sensor_sdk import open_device
from depz_sensor_sdk.vl53l7 import Vl53l7cx, RESOLUTION_8X8

dev = open_device("/dev/ttyACM0")   # returns a Vl53l7cx (USB id / device name)
assert isinstance(dev, Vl53l7cx)
dev.init(progress=print)            # ~1.4 s: downloads the L5/L7 firmware blob
print(dev.module_type)              # 1 = MODULE_TYPE_MZEVO (L7)
```

All three L5/L7 boards run the same firmware, so the class comes from the
board's USB id, then from its device name (`… VL53L7CX USB v2.1 …`). After
`init()` the sensor reports its module type; if it contradicts the class (a
board stamped L7 carrying an L5), `init()` warns — ranging still works, the
blob is shared.

## Hello-world: 8×8 depth frames

```python
from depz_sensor_sdk import open_device
from depz_sensor_sdk.vl53l7 import Vl53l7cx, RESOLUTION_8X8

with open_device("/dev/ttyACM0") as dev:
    assert isinstance(dev, Vl53l7cx)
    dev.init()
    dev.set_resolution(RESOLUTION_8X8)
    dev.set_ranging_frequency_hz(15)      # 1..15 Hz at 8×8 (1 Hz works on L5/L7)
    dev.start_ranging()
    try:
        for frame in dev.frames():        # bounded, drop-oldest iterator
            grid = frame.grid()           # 8×8 NumPy array of mm
            print("centre:", grid[3:5, 3:5].mean(), "min:", grid.min())
    except KeyboardInterrupt:
        pass
    finally:
        dev.stop_ranging()
```

The centre of an 8×8 grid is the four zones 27, 28, 35, 36 — `grid[3:5, 3:5]`.

## Board commands

```python
from depz_sensor_sdk.vl53l7 import PinAction

info = dev.get_bridge_info()     # Vl53l7Info: pin levels, bus speed, I2C counters
info.i2c_khz, info.i2c_errors, info.last_i2c_error

dev.set_i2c_speed_khz(400)       # → 400; snaps to 100, 200, 400, 500 … 1000 kHz

dev.pin_ctrl(PinAction.SOFT_CYCLE)   # LPn low 1 ms, high, I2C_RST pulse
dev.init()                           # the sensor lost its firmware — reload it
```

`get_bridge_info()` takes the bus away from the stream for a moment: read it
before and after a run, not during one. `set_i2c_speed_khz()` raises
`BusyError` mid-transfer — stop ranging first.

| `PinAction` | effect |
|---|---|
| `LPN_OFF` | stop streaming, LPn low: sensor I2C interface off (state lost) |
| `LPN_ON` | LPn high: interface on (the power-up default) |
| `I2C_RST` | pulse the sensor's I2C_RST |
| `SOFT_CYCLE` | stop streaming, LPn low 1 ms, high, I2C_RST pulse (state lost) |

## Advanced features on L5/L7

Crosstalk calibration and caldata, detection thresholds, the motion indicator
and sleep/wake work exactly as in the
[VL53L8CX guide](../vl53l8cx/guide.md#advanced-features), with two limits of
the L5CX / L7CX firmware (ST ULD 2.0.1):

- **No `POWER_MODE_DEEP_SLEEP`** — only sleep and wake-up; deep sleep raises.
- **No threshold auto-stop** — detection thresholds work, the auto-stop flag
  does not exist.

```python
dev.calibrate_xtalk(reflectance_percent=16, nb_samples=4, distance_mm=600)
if dev.xtalk_calibration_failed:
    print("nothing to calibrate — the cover glass is too good; defaults kept")
```

## Gotchas

- **`init()` after every pin reset** — `LPN_OFF` / `SOFT_CYCLE` wipe the
  sensor firmware.
- **Bridge info during ranging costs a frame** — read counters around a run.
- **Module type is only known after `init()`** — `module_type` is `None` before.
- All the [VL53L8CX gotchas](../vl53l8cx/guide.md#gotchas) apply, except the
  ≥ 2 Hz floor: here 1 Hz is fine.
