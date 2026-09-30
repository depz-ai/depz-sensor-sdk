---
title: VL53L1CB — introduction
description: What the VL53L1CB single-zone time-of-flight sensor is, which drivers it runs on the host, when to use it, and the key concepts of the VL53L 1D family.
---

# VL53L1CB — introduction

The **VL53L1CB** is the VL53L1 die in ST's cover-glass-ready module: up to ~8 m single-zone ranging, the same drivers as the VL53L1CX. It is one of the six VL53L 1D boards that
share one firmware (`APP_VL53L0_4`, contract 12): the MCU is a thin I2C
**register bridge** that knows no sensor, and every ST driver runs **on the
host**, inside this SDK (`Vl53l1cb`, a subclass of the family class `Vl53lx`).

## What it does

- Single-zone distance up to **~8 m**, one `Vl53lxMeasurement` per
  frame, with a range status, sigma (std-dev estimate), signal and ambient
  rates.
- INT-driven **push streaming**: the MCU reads the result block on each
  data-ready edge and ships it with a microsecond timestamp.
- Driver kinds on this board: `uld`, `histogram` (default `uld`).

| driver | what it is | modes | timing budget |
|---|---|---|---|
| `uld` | the VL53L1X Ultra Lite Driver: the die computes one distance per frame | `long` (default after init), `short` | only the tabulated budgets: 20, 33, 50, 100, 200, 500 ms in `long`; 15 ms is added in `short` |
| `histogram` | ST's Bare Driver: the die hands the host 24 photon-count bins per frame and the SDK finds **up to four targets** in them | `short`, `medium`, `long` | any 2–550 ms (default 33 ms) |

## When to use it

Use it where the sensor sits behind a window or a product enclosure, and for the longest reach in the family. It is the board that usually needs a crosstalk calibration.

## Key concepts

- **Product × driver kind** — `init(driver)` binds the pair; the product comes from the board's device name (the class fixes it). `supports(group)` tells what the pair can do — ask it before offering a setting.
- **`configure()` before every run** — it re-initialises the sensor and applies budget, period, mode and stored calibrations. That is the only way to know what the configuration registers hold.
- **Same measurement for every product** — `Vl53lxMeasurement`; on the histogram driver `targets` lists every return, strongest first.
- **Configure only when stopped** — while ranging the stream owns the bus; configuration calls throw until `stopRanging()`.
- **Crosstalk** — light bounced back by the cover glass looks like a close target. `calibrateXtalk()` against a target measures it once; store the value and re-apply it with `configure({ xtalkKcps })`.
- **Region of interest** — on the light driver, as on the VL53L1CX.
- **Timestamps** are device microseconds as `bigint` (`timestampUs`) on every measurement.

## See also

- [VL53L1CB guide](guide.md) — open, configure, stream, the
  measurement, calibration, gotchas.
- [API reference](api.md) — `Vl53l1cb`, the family class `Vl53lx`,
  `Vl53lxMeasurement`, `Vl53lxInfo`.
