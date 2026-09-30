---
title: VL53L0X — introduction
description: What the VL53L0X single-zone time-of-flight sensor is, which drivers it runs on the host, when to use it, and the key concepts of the VL53L 1D family.
---

# VL53L0X — introduction

The **VL53L0X** is the first-generation ST single-zone ToF ranger: one laser distance per measurement up to ~2 m. It is one of the six VL53L 1D boards that
share one firmware (`APP_VL53L0_4`, contract 12): the MCU is a thin I2C
**register bridge** that knows no sensor, and every ST driver runs **on the
host**, inside this SDK (`Vl53l0x`, a subclass of the family class `Vl53lx`).

## What it does

- Single-zone distance up to **~2 m**, one `Vl53lxMeasurement` per
  frame, with a range status, sigma (std-dev estimate), signal and ambient
  rates.
- INT-driven **push streaming**: the MCU reads the result block on each
  data-ready edge and ships it with a microsecond timestamp.
- One driver on this board: `uld`.

| driver | what it is | modes | timing budget |
|---|---|---|---|
| `uld` | ST's VL53L0X API 1.0.4 (the PAL), ported to the host | `default`, `long-range`, `high-speed`, `high-accuracy` (the four ST example profiles) | any 20–200 ms |

## When to use it

Use it for a cheap, narrow-beam distance up to ~2 m when a single number is enough: presence, level, proximity. For more reach, targets through a cover glass or several targets per frame, pick a VL53L1/L3/L4 board.

## Key concepts

- **Product × driver kind** — `init(driver)` binds the pair; the product comes from the board's device name (the class fixes it). `supports(group)` tells what the pair can do — ask it before offering a setting.
- **`configure()` before every run** — it re-initialises the sensor and applies budget, period, mode and stored calibrations. That is the only way to know what the configuration registers hold.
- **Same measurement for every product** — `Vl53lxMeasurement`; on the histogram driver `targets` lists every return, strongest first.
- **Configure only when stopped** — while ranging the stream owns the bus; configuration calls throw until `stopRanging()`.
- **1-byte register addresses, 400 kHz** — the only family member that needs them; the SDK switches the bridge for you.
- **Reference SPADs** — `performRefSpadManagement()` re-measures them (done at init; repeat after a big temperature change).
- **Calibrate the offset** — uncalibrated boards read several centimetres long; `calibrateOffset()` against a flat target fixes it.
- **Timestamps** are device microseconds as `bigint` (`timestampUs`) on every measurement.

## See also

- [VL53L0X guide](guide.md) — open, configure, stream, the
  measurement, calibration, gotchas.
- [API reference](api.md) — `Vl53l0x`, the family class `Vl53lx`,
  `Vl53lxMeasurement`, `Vl53lxInfo`.
