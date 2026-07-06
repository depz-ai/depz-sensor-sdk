# VL53L8CH — introduction

The **VL53L8CH** is the superset of the [VL53L8CX](../vl53l8cx/introduction.md):
the same STMicroelectronics multizone Time-of-Flight imager, the same
register-bridge frame format, the same host-side decode — **plus Compact Network
Histograms (CNH)**. It carries its own production USB PID (`0xED40`); both
variants report the same `VL53L8` firmware identity, so the CX/CH split is named
by `depz_vl53l8_variant` (`DEPZ_VL53L8_VARIANT_CX` / `DEPZ_VL53L8_VARIANT_CH`).

Everything the CX decode does, the CH does identically — start there:

- [VL53L8CX introduction](../vl53l8cx/introduction.md) — what the ToF sensor is,
  the register-bridge frame, the decode path, the key concepts.
- [VL53L8CX user guide](../vl53l8cx/guide.md) — reassemble + decode a frame, the
  frame fields, and the advanced DCI codecs (xtalk, thresholds, motion).

This page and the [CH guide](guide.md) cover **only what CH adds on top** — and,
truthfully, what it does *not yet* add in this SDK.

## What CH adds: Compact Network Histograms

A normal frame gives you one distance (and status/signal/…) per zone. CNH adds a
per-**aggregate** distance **histogram**: for each aggregate of zones, the photon
return counts binned by range. That exposes the raw return structure the
single-distance pipeline collapses — multiple returns in one zone, partial
occlusion, glass/edge effects, material signatures.

## What the C SDK covers today

The C SDK's coverage of CH is exactly its coverage of CX — the **shared**
register-bridge decode path plus the advanced DCI codecs. A VL53L8CX capture is
replayed through this shared decode in the test suite; a CH device exercises the
same code.

- **Shared (implemented, verified):** chunk parse, frame reassembly, the
  ranging-frame decoder (`depz_vl53l8_decode_frame`), and the advanced DCI codecs.
  All documented in the [VL53L8CX API reference](../vl53l8cx/api.md).
- **CH-only extension point (NOT implemented):** **CNH decode.** The CNH
  histogram frame is a distinct on-wire layout and is left as a named,
  documented gap in `src/vl53l8_decode.c` — no fake CH implementation is
  provided. This is the one thing CH does that the SDK does not yet decode.

So there are **no CH-exclusive symbols** in the API beyond the variant enum that
names the split. When CNH decode lands, it will appear here.

## When to use CH over CX

Reach for CH only when you need the raw return histograms — multi-return
analysis, material/reflectivity work, glass and edge disambiguation. For plain
depth imaging the [CX](../vl53l8cx/introduction.md) decode is identical and
sufficient.

## See also

- [VL53L8CH user guide](guide.md) — how CH maps onto the shared decode, and the
  CNH gap.
- [VL53L8CX docs](../vl53l8cx/introduction.md) — the base sensor CH shares.
- [API reference](api.md) — `depz_vl53l8_variant` (the CX/CH split marker);
  the shared surface is the [CX reference](../vl53l8cx/api.md).
