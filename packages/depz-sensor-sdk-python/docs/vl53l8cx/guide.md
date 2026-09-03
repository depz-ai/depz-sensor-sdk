# VL53L8CX — user guide

Hands-on guide to `Vl53l8Cx`, the base ToF sensor. For what the sensor is, CX
vs CH, and the concepts, read the [introduction](introduction.md); for exact
signatures see the [API reference](api.md). The **VL53L8CH** superset (CNH
histograms) has its [own guide](../vl53l8ch/guide.md) — it inherits everything
below.

## Contents

- [Open and initialize](#open-and-initialize)
- [Hello-world: 8×8 depth frames](#hello-world-8x8-depth-frames)
- [Configuration](#configuration)
- [The frame: fields and the heatmap grid](#the-frame-fields-and-the-heatmap-grid)
- [Streaming](#streaming)
- [Advanced features](#advanced-features)
- [Gotchas](#gotchas)

## Open and initialize

```python
from depz_sensor_sdk import open_device
from depz_sensor_sdk.vl53l8 import Vl53l8Cx, RESOLUTION_8X8

dev = open_device("/dev/ttyACM0")   # returns a Vl53l8Cx (or Vl53l8Ch)
assert isinstance(dev, Vl53l8Cx)
dev.init(progress=print)            # tens of seconds: downloads the FW blob
```

`init()` downloads the ~84 KB sensor firmware and applies the default config.
The blob variant is fixed by the class (`Vl53l8Cx` → cx, `Vl53l8Ch` → ch), so
you don't pass a variant. Pass `write_progress=lambda done, total: ...` to
track the big blob writes.

## Hello-world: 8×8 depth frames

```python
from depz_sensor_sdk import open_device
from depz_sensor_sdk.vl53l8 import Vl53l8Cx, RESOLUTION_8X8

with open_device("/dev/ttyACM0") as dev:
    assert isinstance(dev, Vl53l8Cx)
    dev.init(progress=print)
    dev.set_resolution(RESOLUTION_8X8)
    dev.set_ranging_frequency_hz(15)     # must be >= 2 Hz
    dev.start_ranging()
    try:
        for frame in dev.frames():       # bounded, drop-oldest iterator
            grid = frame.grid()          # 8×8 NumPy array of mm
            print("center:", grid[3:5, 3:5].mean(),
                  "min:", grid.min(), "°C:", frame.silicon_temp_degc)
    except KeyboardInterrupt:
        pass
    finally:
        dev.stop_ranging()
```

## Configuration

Call these **after `init()` and while not ranging** (they raise otherwise).

```python
dev.set_resolution(RESOLUTION_8X8)     # 16 (4×4) or 64 (8×8) zones
dev.set_ranging_frequency_hz(15)       # Hz, >= 2; max 60 @4×4, 15 @8×8
dev.set_ranging_mode(RANGING_MODE_CONTINUOUS)   # or RANGING_MODE_AUTONOMOUS
dev.set_integration_time_ms(20)        # 2–1000 ms; autonomous mode only
dev.set_sharpener_percent(20)          # 0–99 % edge sharpener (0 disables)
dev.set_target_order(TARGET_ORDER_CLOSEST)      # or TARGET_ORDER_STRONGEST
```

Every setter has a matching getter that reads the value back from the sensor.
Constants live in `depz_sensor_sdk.vl53l8`.

## The frame: fields and the heatmap grid

`Vl53l8Frame` carries every per-zone output as a NumPy array sized to the
active resolution, plus the per-frame silicon temperature:

| field | dtype | meaning |
|---|---|---|
| `distance_mm` | int32 | per-zone distance in mm |
| `target_status` | uint8 | 5/9 = valid, 255 = no target |
| `nb_target_detected` | uint8 | targets found in the zone |
| `signal_per_spad` | float64 | signal rate, kcps/SPAD |
| `ambient_per_spad` | float64 | ambient rate, kcps/SPAD |
| `range_sigma_mm` | float64 | range std-dev estimate, mm |
| `reflectance` | uint8 | estimated reflectance, % |
| `silicon_temp_degc` | int | per-frame sensor temperature, °C |

`frame.grid("distance_mm")` reshapes any zone field to a `(4,4)` / `(8,8)`
array (row-major). Mask invalid zones on `target_status`:

```python
import numpy as np
d = frame.grid("distance_mm").astype(float)
d[frame.grid("target_status") == 255] = np.nan   # blank the no-target zones
```

## Streaming

```python
# iterator — subscribe before or after start_ranging()
it = dev.frames(maxsize=8)
frame = next(it)
print("dropped:", it.dropped_count)

# callback — fires on the reader thread; keep it quick
unsub = dev.on_frame(lambda f: q.put(f))

# convenience: block for the next frame
frame = dev.get_frame(timeout=2.0)     # raises DepzTimeoutError on silence
```

Link health is observable: `dev.reassembler_discards` counts chunked frames
dropped on a gap; `dev.frame_parse_errors` counts frames that failed ULD
parsing (kept separate so the two stay meaningful).

## Advanced features

These wrap the ST ULD plugins. All require `init()` and **not while ranging**.

```python
# power modes — WAKEUP / SLEEP / DEEP_SLEEP (waking from deep sleep re-runs init())
from depz_sensor_sdk.vl53l8 import POWER_MODE_SLEEP, POWER_MODE_WAKEUP
dev.set_power_mode(POWER_MODE_SLEEP)
dev.set_power_mode(POWER_MODE_WAKEUP)

# crosstalk margin (kcps/SPAD) + on-device calibration against a flat target
dev.set_xtalk_margin(50.0)
dev.calibrate_xtalk(reflectance_percent=16, nb_samples=4, distance_mm=600)

# calibration-data save/restore (the 776-byte xtalk blob IS the L8CX caldata)
blob = dev.get_caldata_xtalk()         # persist this somewhere
dev.set_caldata_xtalk(blob)            # restore on a later boot

# per-zone detection thresholds (interrupt-on-threshold)
dev.set_detection_thresholds([
    {"low_thresh": 200, "high_thresh": 600, "measurement": 1,   # DIST_MM
     "type": 0, "zone_num": 128, "operation": 0},               # in-window, all zones
])
dev.set_detection_thresholds_enable(True)

# motion indicator — surfaces per-frame motion in frame.motion
dev.configure_motion_indicator(distance_min_mm=400, distance_max_mm=1500)
```

Threshold `measurement` selectors and `THRESH_*` window/operation constants
live in `depz_sensor_sdk.vl53l8.uld`. `calibrate_xtalk` is a verbatim ST-ULD
port but is **not** verified on live hardware in this SDK — the tested
save/restore route is the `get_caldata_xtalk` / `set_caldata_xtalk` blob.

## Gotchas

- **`init()` takes tens of seconds** every power-up (84 KB blob). Show progress.
- **Ranging must be ≥ 2 Hz** — below that the sensor streams nothing.
- **Config while ranging raises** — the stream owns the register bank; stop,
  reconfigure, restart.
- **Waking from DEEP_SLEEP re-downloads the firmware** (`init()` runs again).
- **Integration time only bites in autonomous mode** — it has no effect in
  continuous ranging.
- **Callbacks run on the reader thread** — hand heavy work to a queue.
- **CNH is CH-only** — `configure_cnh()` does not exist on `Vl53l8Cx`; see the
  [VL53L8CH guide](../vl53l8ch/guide.md).
