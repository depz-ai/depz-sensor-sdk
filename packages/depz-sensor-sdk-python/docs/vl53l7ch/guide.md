# VL53L7CH — user guide

`Vl53l7ch` = the [VL53L7CX](../vl53l7cx/guide.md) class + CNH. Everything in
the VL53L7CX guide (board commands, pin control) and the
[VL53L8CX guide](../vl53l8cx/guide.md) (configuration, frames, streaming,
advanced features) applies. This page covers CNH and what the VL53LMZ
firmware adds.

## Open and initialize

```python
from depz_sensor_sdk import open_device
from depz_sensor_sdk.vl53l7 import RESOLUTION_8X8, Vl53l7ch

dev = open_device("/dev/ttyACM0")   # returns a Vl53l7ch for the L7CH board
assert isinstance(dev, Vl53l7ch)
dev.init(progress=print)            # downloads the CH (VL53LMZ) blob
dev.set_resolution(RESOLUTION_8X8)
dev.set_ranging_frequency_hz(15)
```

## CNH histograms

Build and size-check a `CnhConfig` exactly as on the
[VL53L8CH](../vl53l8ch/guide.md#cnh-histograms), arm it while stopped, then
range — the histograms arrive **in the stream**, in every frame:

```python
from depz_sensor_sdk import CnhConfig
from depz_sensor_sdk.vl53l8 import cnh

cfg = CnhConfig()
cfg.init_config(start_bin=10, num_bins=20, sub_sample=2)
cfg.create_agg_map(RESOLUTION_8X8, 0, 0, 2, 2, 4, 4)     # 16 aggregates
assert cfg.required_memory() <= 6160

dev.configure_cnh(cfg)
dev.start_ranging()
frame = dev.get_frame(timeout=2.0)
histograms = cnh.decode(cfg, frame.cnh_raw)   # per-aggregate histograms
grid = frame.grid()                           # the usual depth image, same frame
dev.stop_ranging()
```

A frame with CNH is larger than one stream chunk (3156 B for 16 aggregates ×
20 bins, up to ~7.6 KB); the SDK reassembles the chunks, so on the L7CH you
just stream.

## What the VL53LMZ firmware adds

```python
from depz_sensor_sdk.vl53l8.uld import POWER_MODE_DEEP_SLEEP, POWER_MODE_WAKEUP

dev.set_power_mode(POWER_MODE_DEEP_SLEEP)   # L7CH only on this board
dev.set_power_mode(POWER_MODE_WAKEUP)       # wake from deep sleep re-runs init()
```

The detection-threshold **auto-stop** also works here, as on the VL53L8
([VL53L8CX guide](../vl53l8cx/guide.md#advanced-features)).

## Gotchas

- **`configure_cnh()` while stopped** — like every config call.
- **Size-check first** — `required_memory()` must be ≤ 6160 bytes.
- **Waking from deep sleep re-downloads the firmware.**
- All the [VL53L7CX gotchas](../vl53l7cx/guide.md#gotchas) apply.
