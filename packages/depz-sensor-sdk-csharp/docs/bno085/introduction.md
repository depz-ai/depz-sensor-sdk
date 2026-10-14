<!-- generated from ../bno086/ by scripts/gen_bno085_docs.py — edit those pages -->
# BNO085 — introduction

> **BNO085.** It runs the same SH-2 sensor hub as the BNO086 and the SDK drives it with the very same class, so everything here applies as written — code keeps the `bno086` names. The parts tell themselves apart by the Product ID part number: 10004148 for the BNO085, 10004563 for the BNO086.

The **BNO085** is a CEVA/Hillcrest 9-axis IMU with an on-chip sensor-fusion hub
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

### What the DEPZ boards actually run

Checked on the boards (BNO085 with SH-2 3.2.13, BNO086 with SH-2 3.12.6,
2026-10-06). The SDK decodes every report above, but the sensor itself:

- **refuses** significant motion, flip, pickup, sleep, tilt, pocket and circle —
  enabling them reads back a 0 Hz interval;
- sends the **raw ADC** reports only while an orientation report is on;
- refuses the **tap detector** while the step counter runs;
- needs a non-zero enable mask in the **personal-activity classifier**'s
  feature configuration word (`0x1FF` = every activity), else it refuses;
- reports the **detectors at their full rate** whether anything happened or
  not: an event is a report whose content changed (tap / shake: non-zero flags).
  The step detector's report carries only a latency, so it cannot be told from
  a step — count steps with the step counter.

Right after enabling, the Get Feature read-back can be a stale 0 for up to a
second: read it again before treating a feature as refused.

## When to use it

Use the BNO085 whenever you need **orientation or motion**: heading/attitude for
a robot or handheld, gesture and activity detection, image stabilisation, dead
reckoning. It complements the ranging sensors — SR04/VL53L8 tell you *where
things are*, the BNO085 tells you *how the device itself is moving and facing*.

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

- [BNO085 user guide](guide.md) — SHTP reassembly, encoding control reports,
  decoding input cargos, report types and units, gotchas.
- [API reference](api.md) — `ShtpLayer`, `Sh2Control`, `Sh2Reports`, `SensorId`,
  the report records, and `GyroIntegratedRV`.
