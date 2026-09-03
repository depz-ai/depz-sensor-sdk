# VL53L8CH — introduction

The **VL53L8CH** is the superset of the [VL53L8CX](../vl53l8cx/introduction.md):
the same STMicroelectronics multizone Time-of-Flight imager, same register
protocol, same host-side ULD — **plus Compact Network Histograms (CNH)**. In
the SDK it is `Vl53l8Ch`, which inherits every `Vl53l8Cx` method and adds one
thing: `configure_cnh()`. It also carries its own production USB PID
(`0xED40`), so `open_device()` returns a `Vl53l8Ch` for it automatically.

Everything the CX does, the CH does identically — start there:

- [VL53L8CX introduction](../vl53l8cx/introduction.md) — what the ToF sensor is,
  the frame, host-side ULD, the key concepts.
- [VL53L8CX user guide](../vl53l8cx/guide.md) — open/init, configuration, the
  heatmap grid, streaming, and the advanced ULD features (power, xtalk,
  caldata, thresholds, motion).

This page and the [CH guide](guide.md) cover **only what CH adds on top**.

## What CH adds: Compact Network Histograms

A normal frame gives you one distance (and status/signal/…) per zone. CNH adds
a per-**aggregate** distance **histogram**: for each aggregate of zones, the
photon return counts binned by range. That exposes the raw return structure the
single-distance pipeline collapses — multiple returns in one zone, partial
occlusion, glass/edge effects, material signatures.

- **`CnhConfig`** — describes the histogram: `init_config(start_bin, num_bins,
  sub_sample)` sets the range window and resolution, `create_agg_map(...)` tiles
  zones into aggregates. `required_memory()` must fit the device buffer
  (≤ 6160 bytes) before you arm it.
- **`configure_cnh(cfg)`** — CH-only. Arms the histogram block for the next
  `start_ranging()`. It does **not** exist on `Vl53l8Cx`.
- A full CNH frame is larger than the MCU stream cap, so CNH is read in **poll
  mode**; decode a captured block with `depz_sensor_sdk.vl53l8.cnh.decode`.

## When to use CH over CX

Reach for CH only when you need the raw return histograms — multi-return
analysis, material/reflectivity work, glass and edge disambiguation. For plain
depth imaging the [CX](../vl53l8cx/introduction.md) is identical and simpler.

## See also

- [VL53L8CH user guide](guide.md) — arming CNH, `CnhConfig`, decoding frames.
- [VL53L8CX docs](../vl53l8cx/introduction.md) — the base sensor CH inherits.
- [API reference](api.md) — `Vl53l8Ch`, `CnhConfig` (inherits the
  [CX surface](../vl53l8cx/api.md)).
