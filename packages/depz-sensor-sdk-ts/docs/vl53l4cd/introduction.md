---
title: VL53L4CD — introduction
description: What the VL53L4CD single-zone time-of-flight sensor measures, when to use it, and its key concepts in the TypeScript SDK.
---

# VL53L4CD — introduction

The **VL53L4CD** is a STMicroelectronics single-zone Time-of-Flight ranging
sensor: one laser distance per measurement, up to ~1.3 m, with millimetre
resolution and a per-sample quality estimate. On the DEPZ sensor line the MCU
is a thin I2C **register bridge** (contract 10) — the full ST ULD 2.2.3
driver runs **on the host**, inside this SDK (`Vl53l4Cd`).

## What it does

- Single-target distance from ~1 mm to ~1300 mm, one `Vl53l4Measurement` per
  sample, at up to ~100 Hz (10 ms timing budget).
- Each measurement carries far more than distance: a **range status** (0 =
  valid, with `statusText` naming the failure otherwise), **sigma** (range
  std-dev estimate, mm), signal/ambient rates, SPAD count, and the sensor's
  own wrapping frame counter (`streamCount`).
- INT-driven **push streaming**: the sensor signals data-ready and the MCU
  ships the 17-byte result block to the host — one report per measurement, no
  reassembly.
- The full ULD configuration surface is exposed: timing budget and
  inter-measurement period, offset and crosstalk correction (with on-device
  calibration), signal/sigma limits and distance-window interrupts.

## When to use it

Use the VL53L4CD when you need one accurate, fast, narrow-beam distance —
presence detection, level sensing, proximity switching — in the dark or in
sunlight, where the [SR04](../sr04/introduction.md)'s wide ultrasonic cone is
too blunt. It is a **single zone**, not an imager: for a depth image use the
[VL53L8CX/CH](../vl53l8cx/introduction.md), and for orientation use the
[BNO086](../bno086/introduction.md).

## Key concepts

- **Host-side ULD** — configuration and result decoding are pure TypeScript
  (`sensors/vl53l4/uld.ts`), a faithful port of ST's ULD 2.2.3. The device is
  just a register bridge.
- **`init()` is light** — the VL53L4CD carries its own firmware, so there is
  no blob download (unlike the VL53L8). `init()` runs the ULD boot sequence +
  VHV calibration in well under a second, and must run once per power-up.
- **Configure only when stopped** — while ranging, the INT-driven stream owns
  the register bank; config methods throw until you `stopRanging()`.
- **Continuous vs autonomous** — `setRangeTiming(budgetMs, interMs)`: budget
  10–200 ms; `interMs = 0` ranges back-to-back (continuous), `interMs >
  budget` is the autonomous low-power mode. Default is 50 ms continuous
  (~20 Hz).
- **XSHUT wipes the config** — `resetSensor()` / `xshut()` power-cycle the
  sensor; every setting is lost and `init()` is required again (the SDK
  tracks this via `dev.initialized`).
- **`bridgeInfo()`** — MCU-side diagnostics (INT edge / error counters, pin
  levels, bus speed), safe to call even while streaming.

The class is [`Vl53l4Cd`](api.md); its results are `Vl53l4Measurement`
`{ timestampUs, distanceMm, sigmaMm, valid, statusText, … }`. See the
[guide](guide.md) for runnable code.
