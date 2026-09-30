# BNO055 — introduction

The **BNO055** is a Bosch 9-axis absolute-orientation sensor: accelerometer,
gyroscope and magnetometer in one package, with Bosch's sensor fusion running
**on the chip**. It hands out a ready orientation (quaternion or Euler
angles), gravity and linear acceleration at 100 Hz. On the DEPZ sensor line
the MCU is a thin I2C **register bridge** (`APP_BNO055` firmware, contract 13,
USB PID `0xEE0A`): the host sets the sensor up with plain register writes and
the bridge streams one register block on a timer.

## What the sensor does

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
simpler to drive than the [BNO086](../bno086/introduction.md) — no report
subscriptions, one register block — at the price of fewer outputs: no step
counter, no activity classifier, no per-report rates.

## What this SDK offers

The `Depz.Sensor.Bno055` namespace is the verifiable **codec and decode
layer**:

- the bridge wire codecs — `BNO_READ_REG`, `BNO_WRITE_REG`, `BNO_RESET`,
  `BNO_START_STREAM` / `BNO_STOP_STREAM`, `BNO_GET_INFO` payloads, and the
  `RPT_BNO_REG_DATA`, `RPT_BNO_REG_STREAM` and 38-byte
  [`Bno055Info`](api.md#bno055info) reports;
- the register map ([`Bno055Regs`](api.md#bno055regs)) and its pure codecs:
  output [`Bno055Units`](api.md#bno055units) (UNIT_SEL) and every LSB scale,
  [`Bno055CalibStatus`](api.md#bno055calibstatus), the 22-byte
  [`Bno055CalibrationProfile`](api.md#bno055calibrationprofile), the soft-iron
  matrix, [`Bno055AxisRemap`](api.md#bno055axisremap) with the placements
  P0..P7, operating and
  power modes, and the page-1 accel / gyro / mag configs;
- `Bno055Regs.DecodeBlock`: any streamed or read register window → the raw
  channel integers it covers.

What it does **not** do: open a port or run the session logic — mode switches
through CONFIG, waiting for the boot and for the fusion to start, page
switching, scaled samples. That is done today with the Python or TypeScript
SDK; this SDK builds the command payloads and decodes what the board sends.

## Key concepts

- **Fusion on the chip** — there is no host algorithm; the sensor computes
  orientation, the host reads registers.
- **Raw integers, scaled by the units in force** — `DecodeBlock` returns
  register integers; `value = raw / LSB`, with the LSB from the UNIT_SEL that
  was in force when the stream was armed. Linear acceleration and gravity are
  **always m/s²** (100 LSB), whatever UNIT_SEL says.
- **The stream is a timer** — the bridge reads one register block every
  `period_ms` and pushes it with the MCU timestamp of the tick. The sensor
  fuses at 100 Hz, so 10 ms is the useful floor; the sensor firmware on these
  boards (03.11) has no data-ready interrupt.
- **Configure in CONFIG** — units, axis remap, the calibration profile and
  the power mode only take in CONFIG mode; a write elsewhere is silently
  ignored.
- **Two register pages** — sensor configs, interrupt setup and the unique id
  live on page 1; the host owns the page select and must not switch pages
  under a running stream.

## See also

- [BNO055 user guide](guide.md) — commands, stream decode, units, calibration,
  axis remap, page-1 configs, bridge info, gotchas.
- [API reference](api.md) — `Depz.Sensor.Bno055`: codecs, register map,
  `DecodeBlock`.
