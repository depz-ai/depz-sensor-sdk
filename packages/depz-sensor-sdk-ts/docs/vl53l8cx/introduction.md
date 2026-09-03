---
title: VL53L8CX — introduction
description: What the VL53L8CX time-of-flight sensor is, what it measures, when to use it, and its key concepts. The base ToF class; the CH superset is documented separately.
---

# VL53L8CX — introduction

The **VL53L8CX** is a multi-zone time-of-flight depth sensor. It ranges a
**4×4 or 8×8 grid** of zones simultaneously and streams a low-resolution depth
image at up to ~15 Hz. On the DEPZ boards the MCU is a thin SPI bridge; the
full ST ULD driver runs **on the host** inside this SDK.

`Vl53l8Cx` is the base ToF class — the dev-default silicon. There is also a
superset part, **VL53L8CH** (`Vl53l8Ch`, production USB PID `0xED40`), which
adds Compact-Network-Histogram output on top of everything here; it has its own
[introduction](../vl53l8ch/introduction.md) and [guide](../vl53l8ch/guide.md).
`openDevice()` picks the class from the USB model hint; both are
`instanceof Vl53l8Cx`.

## What it measures

Per frame, each zone carries: `distanceMm`, `targetStatus` (5/9 = valid,
255 = no target), `nbTargetDetected`, `signalPerSpad` and `ambientPerSpad`
(kcps/SPAD), `nbSpadsEnabled`, `rangeSigmaMm`, `reflectance` (%). Plus a
per-frame `siliconTempDegc` and an optional `motion` block. Arrays are sized to
the active resolution (16 or 64), row-major.

## When to use it

- Gesture/proximity zones, obstacle maps, people counting, level/volume — any
  time you need a coarse depth *image*, not a single distance.
- Not for a single axial range where an SR04 is cheaper.
- Reach for the [CH superset](../vl53l8ch/introduction.md) only when you need
  per-aggregate distance histograms (CNH).

## Key concepts

- **`init()` downloads the sensor firmware** (~84 KB) on every power-up — a few
  seconds (up to ~25 s over some CDC stacks). Configure only after `init()`.
- **Ranging ≥ 2 Hz** (`MIN_RANGING_FREQUENCY_HZ`) — below that the sensor never
  streams.
- **Config while ranging throws** — the stream owns the register bank; stop,
  reconfigure, restart.
- **Advanced features** (UM3109): power modes, crosstalk margin/calibration and
  caldata save/restore, detection thresholds, and the motion indicator.
- **Timestamps** are device microseconds as `bigint` on every frame.

The class is [`Vl53l8Cx`](api.md); `zoneGrid(arr, resolution)` reshapes any
per-zone array to a 2-D grid. See the [guide](guide.md) for code.
