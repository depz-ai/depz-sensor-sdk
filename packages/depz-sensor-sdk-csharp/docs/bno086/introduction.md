# BNO086 — introduction

The **BNO086** is a CEVA/Hillcrest 9-axis IMU with an on-chip sensor-fusion hub
(SH-2). It fuses accelerometer, gyroscope and magnetometer into ready-to-use
orientation, motion and activity outputs. On the DEPZ sensor line the MCU is an
**SHTP pass-through bridge** — the full SH-2 host stack runs on the host. The C#
SDK ports the pieces that decode cleanly against golden vectors: **SHTP framing
and reassembly**, the **SH-2 control encoders**, and the **input-report
decoders** (`Depz.Sensor.Bno086`).

## What it does

- **Orientation** — rotation vector (absolute), game rotation vector (drift-free
  relative), geomagnetic and AR/VR-stabilised variants, all as unit quaternions
  (`RotationVector`).
- **Motion** — calibrated / uncalibrated accelerometer, gyroscope, magnetometer,
  linear acceleration, gravity (`Acceleration`, `Gyroscope`, `Magnetometer`,
  `UncalibratedGyroscope`, `UncalibratedMagnetometer`), plus a dense
  gyro-integrated rotation vector on its own channel (`GyroIntegratedRV`).
- **Events & classifiers** — tap, step counter/detector, significant motion,
  stability classifier, personal-activity classifier, shake, and raw ADC samples
  (`TapDetector`, `StepCounter`, `StabilityClassifier`, `RawSensor`, …).
- **Control** — Set/Get Feature, Product ID, FRS read/write and the generic
  command request, all as byte-exact cargo encoders (`Sh2Control`).

## When to use it

Use the BNO086 whenever you need **orientation or motion**: heading/attitude for
a robot or handheld, gesture and activity detection, image stabilisation, dead
reckoning. It complements the ranging sensors — SR04/VL53L8 tell you *where
things are*, the BNO086 tells you *how the device itself is moving and facing*.

## Key concepts

- **Two layers over packets** — the bridge delivers SHTP frames inside DEPZ
  `RPT_DATA`; `ShtpLayer` reassembles SHTP cargos (continuation-bit fragments),
  and `Sh2Reports` decodes the cargo into typed reports. You think in reports,
  not request/reply.
- **Raw integers are authoritative; scaling is the caller's.** Each report keeps
  its wire integers (`XRaw`, `IRaw`, …). Apply the fixed **Q point**
  (`value = raw / 2^Q`) yourself: accel Q8 (m/s²), gyro Q9 (rad/s), mag Q4 (µT),
  quaternions Q14, rotation-vector accuracy Q12 (**radians**), gyro-RV angular
  velocity Q10 (rad/s).
- **Channels** — SHTP channel 2 is SH-2 control, 3/4 are input reports (non-wake
  / wake), 5 is the dense gyro-integrated RV (`ShtpChannel`). Channel 3/4 cargos
  carry a 0xFB base-timebase reference and a per-report delay (100 µs ticks); the
  SDK folds both into an absolute `TimestampUs`.
- **Correlation is SH-2-level** — per firmware ERRATA E2 the bridge ACKs every
  send immediately and delivers all inbound SHTP as unsolicited data; the SDK
  reassembles and decodes at the SH-2 layer for you.

## See also

- [BNO086 user guide](guide.md) — SHTP reassembly, encoding control reports,
  decoding input cargos, report types and units, gotchas.
- [API reference](api.md) — `ShtpLayer`, `Sh2Control`, `Sh2Reports`, `SensorId`,
  the report records, and `GyroIntegratedRV`.
