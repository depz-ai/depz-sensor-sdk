---
title: VL53L3CX — introduction
description: What the VL53L3CX single-zone time-of-flight sensor is, which drivers it runs on the host, when to use it, and the key concepts of the VL53L 1D family.
---

# VL53L3CX — introduction

The **VL53L3CX** is ST's multi-target ToF ranger: up to ~3 m, with a histogram that can resolve several objects along the beam. It is one of the six VL53L 1D boards that
share one firmware (`APP_VL53L0_4`, contract 12): the MCU is a thin I2C
**register bridge** that knows no sensor, and every ST driver runs **on the
host**, inside this SDK (`Vl53l3cx`, a subclass of the family class `Vl53lx`).

## What it does

- Single-zone distance up to **~3 m**, one `Vl53lxMeasurement` per
  frame, with a range status, sigma (std-dev estimate), signal and ambient
  rates.
- INT-driven **push streaming**: the MCU reads the result block on each
  data-ready edge and ships it with a microsecond timestamp.
- Driver kinds on this board: `ulp`, `histogram` (default `ulp`).

| driver | what it is | modes | timing budget |
|---|---|---|---|
| `ulp` | ST's Ultra Low Power driver: one target, minimal bus traffic | none | any 10–200 ms |
| `histogram` | ST's Bare Driver: the die hands the host 24 photon-count bins per frame and the SDK finds **up to four targets** in them | `short`, `medium`, `long` | any 2–550 ms (default 33 ms) |

## When to use it

Use it when you need to see through: a glass pane and the object behind it, or two people at different distances — the histogram driver reports up to four targets per frame. The ULP driver is the low-power single-target option.

## Key concepts

- **Product × driver kind** — `init(driver)` binds the pair; the product comes from the board's device name (the class fixes it). `supports(group)` tells what the pair can do — ask it before offering a setting.
- **`configure()` before every run** — it re-initialises the sensor and applies budget, period, mode and stored calibrations. That is the only way to know what the configuration registers hold.
- **Same measurement for every product** — `Vl53lxMeasurement`; on the histogram driver `targets` lists every return, strongest first.
- **Configure only when stopped** — while ranging the stream owns the bus; configuration calls throw until `stopRanging()`.
- **ULP has no offset or crosstalk calibration** — it is the lean driver; the histogram driver has none either.
- **Timestamps** are device microseconds as `bigint` (`timestampUs`) on every measurement.

## See also

- [VL53L3CX guide](guide.md) — open, configure, stream, the
  measurement, calibration, gotchas.
- [API reference](api.md) — `Vl53l3cx`, the family class `Vl53lx`,
  `Vl53lxMeasurement`, `Vl53lxInfo`.
