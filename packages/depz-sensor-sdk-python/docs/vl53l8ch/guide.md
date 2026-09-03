# VL53L8CH — user guide

Hands-on guide to `Vl53l8Ch`, the CNH superset of the ToF sensor. For what the
sensor is and why CH exists, read the [introduction](introduction.md); for exact
signatures see the [API reference](api.md).

**`Vl53l8Ch` inherits the entire `Vl53l8Cx` surface.** Open/init, resolution,
frequency, ranging, the frame and heatmap grid, streaming, and every advanced
ULD feature (power, xtalk, caldata, thresholds, motion) work exactly as in the
[VL53L8CX user guide](../vl53l8cx/guide.md) — read that first. This page covers
**only the CNH addition**.

## Contents

- [Open and initialize](#open-and-initialize)
- [CNH histograms](#cnh-histograms)
- [Decoding a CNH block](#decoding-a-cnh-block)
- [Gotchas](#gotchas)

## Open and initialize

Identical to the CX, except the class and the firmware blob are the CH variant:

```python
from depz_sensor_sdk import open_device
from depz_sensor_sdk.vl53l8 import Vl53l8Ch, RESOLUTION_8X8

dev = open_device("/dev/ttyACM0")   # returns a Vl53l8Ch for the CH USB id
assert isinstance(dev, Vl53l8Ch)
dev.init(progress=print)            # downloads the CH firmware blob
dev.set_resolution(RESOLUTION_8X8)  # inherited from Vl53l8Cx
dev.set_ranging_frequency_hz(15)    # must be >= 2 Hz
```

## CNH histograms

CNH is CH-only: `configure_cnh()` exists on `Vl53l8Ch` and not on `Vl53l8Cx`.
Build a `CnhConfig`, size-check it against the device buffer, and arm it
**before** `start_ranging()`:

```python
from depz_sensor_sdk import CnhConfig
from depz_sensor_sdk.vl53l8 import Vl53l8Ch, RESOLUTION_8X8

cfg = CnhConfig()
cfg.init_config(start_bin=10, num_bins=20, sub_sample=2)   # range window + resolution
cfg.create_agg_map(RESOLUTION_8X8, 0, 0, 2, 2, 4, 4)       # tile zones → 16 aggregates
assert cfg.nb_of_aggregates == 16
assert cfg.required_memory() <= 6160                       # fits the device buffer

dev.configure_cnh(cfg)     # Vl53l8Ch only — raises AttributeError on Vl53l8Cx
dev.start_ranging()
```

`min_max_distance_mm()` reports the range window the current config covers.

## Decoding a CNH block

A full CNH frame is larger than the MCU stream cap, so CNH is read in **poll
mode**. Decode a captured raw block with the CH config that produced it:

```python
from depz_sensor_sdk.vl53l8 import cnh

histograms = cnh.decode(cfg, raw)   # per-aggregate distance histograms
```

Each parsed `Vl53l8Frame` also exposes `frame.cnh_raw` (the raw CH block, or
`None`) alongside the usual per-zone fields, so the normal depth image and the
histograms travel together.

## Gotchas

- **`configure_cnh` is CH-only** — it does not exist on `Vl53l8Cx`. Everything
  else is inherited unchanged from the [CX guide](../vl53l8cx/guide.md).
- **Arm CNH before ranging** — `configure_cnh()` must be called while stopped
  (like every other config method); it raises while ranging.
- **Size-check first** — `required_memory()` must be ≤ 6160 bytes or the block
  won't fit the device buffer.
- **CNH is polled, not pushed** — a full histogram frame exceeds the MCU stream
  cap; read it in poll mode and `cnh.decode` it.
- All the CX gotchas (heavy `init()`, ≥ 2 Hz, config-while-ranging, reader-thread
  callbacks) apply here too — see the [CX guide](../vl53l8cx/guide.md#gotchas).
