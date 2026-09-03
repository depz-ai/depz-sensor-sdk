# BNO086 — introduction

The **BNO086** is a CEVA/Hillcrest 9-axis IMU with an on-chip sensor-fusion hub
(SH-2). It fuses accelerometer, gyroscope and magnetometer into ready-to-use
orientation, motion and activity outputs. On the DEPZ sensor line the MCU is an
**SHTP pass-through bridge** — the SH-2 host stack is decoded **on the host**,
and this crate provides that verifiable layer ([`bno086`](api.md)): SHTP framing
and cargo reassembly, the SH-2 input-report parsers, and the control-request
builders.

## What it does

- **Orientation** — rotation vector (absolute), game rotation vector
  (drift-free relative), geomagnetic and AR/VR-stabilised variants, all as unit
  quaternions ([`Report::RotationVector`](api.md), Q14).
- **Motion** — calibrated / uncalibrated accelerometer, gyroscope,
  magnetometer, linear acceleration, gravity
  ([`Report::Vector3`](api.md) / [`Vector3WithBias`](api.md)), plus a dense
  gyro-integrated rotation vector on its own channel
  ([`Report::GyroIntegratedRv`](api.md)).
- **Events & classifiers** — tap, step counter/detector, significant motion,
  stability and personal-activity classifiers, shake, and raw ADC samples — the
  full [`Report`](api.md) catalog.
- **Control** — Set/Get Feature, Product ID, Command Request, and FRS
  read/write requests, built by [`sh2`](api.md) and framed by the SHTP layer.

## When to use it

Use the BNO086 whenever you need **orientation or motion**: heading/attitude for
a robot or handheld, gesture and activity detection, image stabilisation, dead
reckoning. It complements the ranging sensors — [SR04](../sr04/introduction.md) /
[VL53L8](../vl53l8cx/introduction.md) tell you *where things are*, the BNO086
tells you *how the device itself is moving and facing*.

## Key concepts

- **Two layers: SHTP then SH-2.** [`ShtpLayer`](api.md) reassembles per-channel
  cargos from framed fragments; the SH-2 report parsers
  ([`parse_input_cargo`](api.md), [`parse_gyro_rv_cargo`](api.md)) turn a cargo
  into typed [`Report`](api.md)s.
- **Enable, then stream.** You build a Set Feature command
  ([`build_set_feature`](api.md)) to turn a report on; the hub then pushes that
  report, which you decode. You think in reports, not request/reply.
- **Raw integers are authoritative; floats are derived.** Each report keeps its
  wire integers (`x_raw`, …); scale with a fixed **Q point**
  ([`q_point`](api.md), `value = raw / 2**Q`): accel Q8 (m/s²), gyro Q9 (rad/s),
  mag Q4 (µT), quaternions Q14, rotation-vector accuracy Q12 (**radians**),
  gyro-RV angular velocity Q10 (rad/s).
- **Correlation is SH-2-level.** Per firmware **ERRATA E2**, the bridge ACKs
  every send immediately and delivers *all* inbound SHTP as unsolicited data;
  you reassemble SHTP and correlate responses at the SH-2 layer.
- **Timestamps.** Channel-3/4 cargos carry a base-timebase reference (0xFB) and
  a per-report delay (100 µs ticks); the parsers fold both into an absolute
  MCU-clock `timestamp_us`.

## See also

- [BNO086 user guide](guide.md) — reassemble SHTP, decode reports, build control
  requests, gotchas.
- [API reference](api.md) — `ShtpLayer`, `Report`, `parse_input_cargo`,
  `parse_gyro_rv_cargo`, and the `sh2` builders.
- [Common guide](../guide.md) — framing, CRC, discovery shared by every sensor.
