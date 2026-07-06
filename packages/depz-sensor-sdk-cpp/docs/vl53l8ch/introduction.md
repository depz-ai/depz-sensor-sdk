# VL53L8CH — introduction

The **VL53L8CH** is the superset of the [VL53L8CX](../vl53l8cx/introduction.md):
the same STMicroelectronics multizone Time-of-Flight imager, same register
protocol, same host-side decode — **plus Compact Network Histograms (CNH)**. It
carries its own production USB PID (`0xED40`, VL53LMZ 2.0.16).

Everything the CX decodes, the CH decodes **identically** — they stream the same
results-frame layout, so the single `depz::vl53l8::decode_frame` serves both.
Start with the CX docs:

- [VL53L8CX introduction](../vl53l8cx/introduction.md) — what the ToF sensor is,
  the frame, host-side decode, the key concepts.
- [VL53L8CX user guide](../vl53l8cx/guide.md) — reassembly, decoding a frame, the
  zone grid, and the advanced-feature DCI codecs.

This page and the [CH guide](guide.md) cover **only what CH adds on top**.

## What CH adds: Compact Network Histograms

A normal frame gives you one distance (and status/signal/…) per zone. CNH adds a
per-**aggregate** distance **histogram**: for each aggregate of zones, the photon
return counts binned by range. That exposes the raw return structure the
single-distance pipeline collapses — multiple returns in one zone, partial
occlusion, glass/edge effects, material signatures.

In the decode surface the CH difference is minimal and honest:

- **Shared ranging path** — the CH results frame decodes through the exact same
  `decode_frame`, using the CH footer-id offset `FOOTER_ID_OFF_CH` (= 4) instead
  of the CX offset (= 12), selectable via `depz::vl53l8::Variant::CH` /
  `footer_id_off(Variant::CH)`.
- **CNH histogram decode is an extension point** — the CH-only CNH stream is a
  documented stub in `depz/vl53l8.hpp`, intentionally **not** implemented rather
  than faked. When the CNH wire format is contract-pinned, the codec lands in
  that section alongside a golden-vector suite.

## When to use CH over CX

Reach for CH only when you need the raw return histograms — multi-return
analysis, material/reflectivity work, glass and edge disambiguation. For plain
depth imaging the [CX](../vl53l8cx/introduction.md) decode is identical and
simpler.

## See also

- [VL53L8CH user guide](guide.md) — the CH footer offset and the CNH extension
  point.
- [VL53L8CX docs](../vl53l8cx/introduction.md) — the base sensor CH shares.
- [API reference](api.md) — the one CH-specific symbol (`FOOTER_ID_OFF_CH`); the
  shared surface lives in the [CX reference](../vl53l8cx/api.md).
