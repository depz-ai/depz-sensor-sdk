# BNO055 — introduction

The **BNO055** is a Bosch 9-axis absolute-orientation sensor: accelerometer,
gyroscope and magnetometer in one package, with Bosch's sensor fusion running
**on the chip**. It hands out a ready orientation (quaternion or Euler
angles), gravity and linear acceleration at 100 Hz. On the DEPZ sensor line
the MCU is a thin I2C **register bridge** (contract 13) — the host sets the
sensor up with plain register writes and the bridge streams one register block
on a timer. All of that lives in `depz_sensor_sdk.bno055` (`Bno055`).

## What it does

- **Fused orientation** at 100 Hz (NDOF / IMU modes): unit quaternion
  `(w, x, y, z)` and Euler angles `(heading, roll, pitch)`, plus the gravity
  vector and linear acceleration (acceleration with gravity removed).
- **Raw sensors** in the same read: acceleration, angular rate, magnetic field
  and temperature — one 46-byte block carries every channel at once.
- **Background calibration** with a per-sensor status 0..3, and a 22-byte
  **calibration profile** you can save and restore after a power cycle.
- **Axis remap** for any mounting (the eight datasheet placements P0..P7),
  selectable **units** (m/s² or mg, dps or rad/s, degrees or radians, °C or
  °F), built-in **self-test** and **motion interrupts** (any/no-motion,
  high-g, high-rate).

## When to use it

Use the BNO055 when you want orientation without writing a fusion filter:
tilt, heading, a pointing device, a level, a pedometer's raw signal. It is
simpler to drive than the BNO086 — no report subscriptions, one register block
— at the price of fewer outputs: no step counter, no activity classifier, no
per-report rates. For those, use the BNO086.

## Key concepts

- **Fusion on the chip** — the SDK does not compute orientation; it reads what
  the sensor computed. Nothing to port, nothing to tune on the host.
- **Operating modes** — `CONFIG` (configuration only, every output reads
  zero), seven non-fusion modes (raw sensors only) and five fusion modes:
  `IMU` (accel + gyro, relative heading), `COMPASS`, `M4G`, `NDOF_FMC_OFF` and
  `NDOF` (all three sensors, absolute heading — the default).
- **Configure in CONFIG** — most settings (units, axis remap, calibration
  profile, power mode) only take in CONFIG mode. The typed setters go there
  and back for you; `configure()` does the usual session setup in one call.
- **The stream is a timer** — the bridge reads one register block every
  `period_ms` and pushes it with a microsecond MCU timestamp. The sensor
  fuses at 100 Hz, so 10 ms is the useful floor. (The sensor firmware on these
  boards, 03.11, has no data-ready interrupt.)
- **Calibration is continuous** — the sensor keeps refining it in the
  background and you cannot switch that off. Status `3` on all four fields
  means fully calibrated.
- **Two register pages** — sensor configs, interrupt setup and the unique id
  live on page 1. The SDK switches pages for you and refuses page-1 access
  while a stream runs (the bridge would stream the wrong registers).

## See also

- [BNO055 user guide](guide.md) — hello-world, modes, units, axes, calibration,
  streaming, gotchas.
- [API reference](api.md) — `Bno055`, `Bno055Sample`, `Units`, `AxisRemap`,
  `CalibrationProfile` and the register codecs.
