---
title: TMF8829 — introduction
description: What the TMF8829 multizone direct time-of-flight sensor is, what it outputs (8×8 … 48×32 zones, up to 4 peaks, histograms), when to pick it over the VL53L8, and its key concepts (host-run sensor, presets, half-frames, clock correction, no sleep).
---

# TMF8829 — introduction

The **TMF8829** is an ams-OSRAM multizone **direct** time-of-flight (dToF)
sensor: a laser flash and a SPAD array that time each photon's return, giving
a depth image of up to **48×32 zones** with **up to 4 distances (peaks) per
zone**, and on request the raw distance histograms behind them. On the DEPZ
board (USB PID `0xED4C`, firmware `APP_TMF8829_v0.51`+) the MCU is a thin SPI
**register bridge** (contract 14) — the host runs the sensor: reset, sensor
firmware download, presets, start/stop, frame parsing and clock correction.
In this SDK that is the `Tmf8829` class.

## What it does

- **Depth grids** of 8×8, 16×16, 32×32 or 48×32 zones (the last one is 32 rows
  × 48 columns), picked by one of ten **presets**: `8x8`, `8x8-lr` (long
  range), `8x8-ha` (high accuracy), `8x8-xr` (extended range), `16x16`,
  `16x16-ha`, `32x32`, `32x32-ha`, `48x32`, `48x32-ha`.
- **Up to 4 peaks per zone** — several objects at different distances in one
  zone (glass and what is behind it, an edge and the wall) each get their own
  distance and SNR, optionally a signal strength, plus per-zone noise and
  crosstalk.
- **Clock-corrected distances in mm** (`distanceMm`) next to the raw sensor
  integers, and a **capture time** in MCU µs for every measurement.
- **Raw histograms** (the photon counts per distance bin) and **reference-SPAD
  frames** for calibration and research work.
- Rates at a 1 ms period: ~30 measurements/s on 8×8 and 16×16, ~15 on 32×32,
  ~10 on 48×32. Histograms slow it down a lot (~10/s and ~1.7/s) — USB
  becomes the limit.

## When to use it

Use the TMF8829 when 8×8 is too coarse: it is the only DEPZ ToF board with
16×16 and larger grids, and its histogram dump is the full raw histogram
rather than the aggregated CNH of the [VL53L8CH](../vl53l8ch/introduction.md).
For a plain 8×8 depth image the [VL53L8CX](../vl53l8cx/introduction.md) is
simpler and has power modes; the TMF8829 has none (see below).

## Key concepts

- **The host runs the sensor** — `init()` soft-resets the sensor, downloads
  its 14.4 KB firmware through the sensor's bootloader and starts it (about
  0.1–0.15 s). The image is ams-OSRAM's `tmf8829_application` (MIT), embedded
  in the SDK with its license (`TMF8829_FIRMWARE_LICENSE`).
- **Presets + config page** — `configure(preset, opts)` loads a preset, then
  writes its own fields into the sensor's 190-byte config page. A preset load
  does **not** reset the result format, the period or the histogram switch, so
  `configure()` writes all of them on every call (with defaults when you omit
  them) — the result never depends on an earlier configuration.
- **Half-frames** — 32×32 and 48×32 arrive as two half-frames (even rows,
  then odd rows); the SDK merges them into one measurement.
- **Distance units** — the sensor reports 0.25 mm units (`8x8-xr`: mm), on
  its own clock, which runs ~1.5 % slow. The SDK measures the clock ratio
  against the MCU and corrects every distance: `distanceMm` is real
  millimetres; `results.distance` keeps the raw integers.
- **Grid orientation** — the SDK keeps the sensor's own row-major order. For a
  board held USB down and seen from behind (looking where the sensor looks),
  the **row** index runs left → right and the **column** index top → bottom:
  the grid is transposed against the scene. Transpose it yourself to draw the
  scene (the DEPZ viewer does).
- **No sleep** — the sensor's enable pin is wired high on this board: there is
  no standby and no power-mode API. Recovery from any state is `init()`.
- **One sequence at a time** — register sequences (init, configure,
  start/stop) are serialised inside the class, so concurrent `await`s cannot
  interleave mid-sequence.

## See also

- [TMF8829 guide](guide.md) — hello-world, presets and fields, the
  measurement, histograms, capture time, diagnostics, gotchas.
- [API reference](api.md) — `Tmf8829`, `Tmf8829Measurement`, `Tmf8829Config`,
  the frame parser, the clock model and the bridge codecs.
