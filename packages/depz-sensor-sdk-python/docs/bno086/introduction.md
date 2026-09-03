# BNO086 — introduction

The **BNO086** is a CEVA/Hillcrest 9-axis IMU with an on-chip sensor-fusion hub
(SH-2). It fuses accelerometer, gyroscope and magnetometer into ready-to-use
orientation, motion and activity outputs. On the DEPZ sensor line the MCU is an
**SHTP pass-through bridge** — the full SH-2 host stack runs **on the host**,
inside `depz_sensor_sdk.bno086`.

## What it does

- **Orientation** — rotation vector (absolute), game rotation vector
  (drift-free relative), geomagnetic and AR/VR-stabilised variants, all as unit
  quaternions.
- **Motion** — calibrated / uncalibrated accelerometer, gyroscope,
  magnetometer, linear acceleration, gravity, plus a dense gyro-integrated
  rotation vector on its own channel.
- **Events & classifiers** — tap, step counter/detector, significant motion,
  stability and personal-activity classifiers, shake/flip/pickup/tilt, and raw
  ADC samples.
- **Housekeeping** — tare and reorientation, ME calibration control, dynamic
  calibration (DCD) save, FRS record read/write, and diagnostics (oscillator
  type, error queue, event counts).

## When to use it

Use the BNO086 whenever you need **orientation or motion**: heading/attitude for
a robot or handheld, gesture and activity detection, image stabilisation, dead
reckoning. It complements the ranging sensors — SR04/VL53L8 tell you *where
things are*, the BNO086 tells you *how the device itself is moving and facing*.

## Key concepts

- **Enable, then stream** — you turn a report on with `enable(sensor, hz)` (Set
  Feature); the hub then pushes that report at the granted rate. You consume a
  stream of typed `Report` objects, not request/reply.
- **Raw integers are authoritative; floats are derived.** Each report keeps its
  wire integers (`x_raw`, …) and exposes scaled properties using a fixed **Q
  point** (`value = raw / 2**Q`): accel Q8 (m/s²), gyro Q9 (rad/s), mag Q4 (µT),
  quaternions Q14, rotation-vector accuracy Q12 (**radians**), gyro-RV angular
  velocity Q10 (rad/s).
- **Rate is a grid** — the hub rounds the requested rate to its 1 kHz/2ⁿ grid.
  `enable(..., verify=True)` reads the granted rate back and warns (never
  raises) if it lands outside 0.9–2.1× your request.
- **Correlation is SH-2-level** — per firmware ERRATA E2, the bridge ACKs every
  send immediately and delivers *all* inbound SHTP as unsolicited data; the SDK
  reassembles SHTP and correlates responses at the SH-2 layer for you.
- **Timestamps** — channel-3 cargos carry a base-timebase reference and
  per-report delay (100 µs ticks); the SDK folds both into an absolute MCU-clock
  `timestamp_us`.

## See also

- [BNO086 user guide](guide.md) — hello-world, enabling reports, report types,
  tare/calibration, FRS, diagnostics, gotchas.
- [API reference](../api.md) — `Bno086`, `SensorId`, `RotationVector`,
  `GyroIntegratedRV`, and the report catalog.
