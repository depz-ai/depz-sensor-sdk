---
title: VL53L5CX — introduction
description: What the VL53L5CX 63° 8×8 time-of-flight sensor is, how it relates to the VL53L7CX on the same I2C board, and when to pick it.
---

# VL53L5CX — introduction

The **VL53L5CX** is STMicroelectronics' original multizone time-of-flight
sensor: an 8×8 (or 4×4) depth image over a **63° diagonal** field of view, up
to ~4 m. On the DEPZ line it sits on the same I2C board as the VL53L7CX
(`APP_VL53L7` firmware, contract 11) and runs the same firmware blob. In the
SDK it is `Vl53l5cx` — a `Vl53l7cx` with a different expected module type,
and so, through it, a `Vl53l8cx`.

## What it does

Exactly what the [VL53L7CX](../vl53l7cx/introduction.md) does — per-zone
distances, statuses and rates, 4×4 up to 60 Hz and 8×8 up to 15 Hz, 1 Hz
supported, the advanced ULD features without deep sleep and threshold
auto-stop, the board commands — with a **narrower, longer-reaching beam**
(63° instead of 90°). After `init()` it reports module type **MZ**
(`MODULE_TYPE_MZ`, 0).

## When to use it

Pick the VL53L5CX when you want a multizone depth image aimed at something
rather than a wide room view: a doorway, a conveyor, a hand in front of a
screen. For the wide view take the VL53L7CX; for raw histograms the VL53L7CH.

## See also

- [VL53L5CX guide](guide.md)
- [VL53L7CX introduction](../vl53l7cx/introduction.md) and
  [guide](../vl53l7cx/guide.md) — board commands, L5/L7 limits.
- [VL53L8CX guide](../vl53l8cx/guide.md) — configuration, frames, streaming,
  advanced features.
- [API reference](api.md)
