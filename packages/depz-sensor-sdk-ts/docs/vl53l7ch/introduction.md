---
title: VL53L7CH — introduction
description: What the VL53L7CH adds over the VL53L7CX — streamed Compact-Network-Histogram (CNH) output and the fuller VL53LMZ feature set — and when to pick it.
---

# VL53L7CH — introduction

The **VL53L7CH** is the superset of the [VL53L7CX](../vl53l7cx/introduction.md):
the same 90° multizone time-of-flight sensor on the same I2C board, running
ST's **VL53LMZ** firmware (the VL53L8CH blob) — **plus Compact Network
Histograms (CNH)**. In the SDK it is `Vl53l7ch`: every `Vl53l7cx` method, and
`configureCnh()` exactly as on the [VL53L8CH](../vl53l8ch/introduction.md).

## What CH adds

- **CNH histograms** — per-aggregate photon-return histograms next to the
  normal per-zone distances: multiple returns in one zone, partial occlusion,
  glass and edge effects. Configured with `CnhConfig` and decoded with
  `decodeCnh()`, as on the VL53L8CH.
- **CNH streams** — a CNH frame (up to ~7.6 KB) comes over the normal stream
  in chunks, which the SDK reassembles; no poll mode is needed.
- **The fuller VL53LMZ feature set** — deep sleep (waking re-runs `init()`)
  and the detection-threshold auto-stop, which the L5CX / L7CX firmware lacks.

## When to use it

Only when you need the raw return histograms (multi-return analysis,
material or reflectivity work). For plain depth images the VL53L7CX is the
same sensor with a lighter blob.

## See also

- [VL53L7CH guide](guide.md) — CNH on this board.
- [VL53L7CX guide](../vl53l7cx/guide.md) — board commands, L5/L7 specifics.
- [VL53L8CH guide](../vl53l8ch/guide.md) — `CnhConfig` in detail and decoding.
- [API reference](api.md) — `Vl53l7ch` and the CNH symbols.
