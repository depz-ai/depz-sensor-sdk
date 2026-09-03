---
title: BNO086 — introduction
description: What the BNO086 9-axis IMU / sensor hub is, what it outputs, when to use it, and its key concepts (SH-2 reports, SHTP, fusion).
---

# BNO086 — introduction

The **BNO086** is a 9-axis IMU with an on-chip sensor hub. It fuses
accelerometer, gyroscope and magnetometer into ready-to-use outputs —
orientation quaternions, calibrated motion vectors, and a catalog of activity
detectors — so the host consumes **decisions, not raw counts**. On the DEPZ
boards the MCU is an SHTP pass-through; the full SH-2 stack runs **on the host**
inside this SDK.

## What it outputs

~25 SH-2 report types, each enabled independently at its own rate. The common
ones:

- **Rotation Vector** (0x05) and **Game Rotation Vector** (0x08) — orientation
  quaternion (`i, j, k, real`), the RV variants with a heading-accuracy
  estimate in radians.
- **Accelerometer / Linear Acceleration / Gravity** (m/s²),
  **Gyroscope** (rad/s), **Magnetometer** (µT) — calibrated, with the raw wire
  integers preserved alongside the scaled values.
- **Gyro-Integrated RV** (0x2A) — a dense high-rate quaternion + angular
  velocity on its own SHTP channel.
- Detectors/classifiers — step counter, tap, shake, stability, significant
  motion, personal-activity, and more.

Every report carries `sensorId`, an absolute `timestampUs` (`bigint`, MCU clock
corrected by timebase + report delay), and — for input reports — `seq` and
`accuracy`.

## When to use it

- Orientation/AHRS, gesture and activity detection, dead-reckoning aids —
  anything needing fused motion rather than raw IMU counts.
- If you only need raw accel/gyro, the raw report IDs are available too, but the
  fused outputs are the point of the part.

## Key concepts

- **Enable, then stream** — `enable(sensorId, hz)` (or the `enableRotationVector`
  sugar) sets a report rate; the hub grants a rate on its 1 kHz/2ⁿ grid and the
  SDK verifies it (a far-off grant warns, never throws).
- **SH-2 / SHTP** — control exchanges (features, FRS, commands) and streaming
  reports ride SHTP channels; the SDK reassembles multi-frame cargos and
  correlates responses at the SH-2 layer (the bridge returns everything as
  unsolicited data).
- **Calibration & tare** — `tareNow()`, `setCalibration()`, `saveDcd()`, and
  FRS record read/write for persistent configuration.
- **Reset** — `hardwareReset()` (nRST) and `clearDcdAndReset()` restart all
  SHTP/SH-2 state. Reset is optional (open + `enable()` + read works without
  it). On firmware newer than v0.95 the SH-2 reset-complete is emitted and the
  reset is confirmed reliably (ERRATA E9, now fixed); on older units (≤ v0.95)
  it was never emitted, so the call is best-effort and still safe there.

The class is [`Bno086`](../api.md); report shapes live in `SensorId` and the
`Report` union. See the [guide](guide.md) for code.
