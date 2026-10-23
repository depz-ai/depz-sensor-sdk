<!-- generated from ../bno086/ by scripts/gen_bno085_docs.py — edit those pages -->
# BNO085 — introduction

> **BNO085.** It runs the same SH-2 sensor hub as the BNO086 and the SDK drives it with the very same class, so everything here applies as written — code keeps the `bno086` names. The parts tell themselves apart by the Product ID part number: 10004148 for the BNO085, 10004563 for the BNO086.

The **BNO085** is a CEVA/Hillcrest 9-axis IMU with an on-chip sensor-fusion hub
(SH-2). It fuses accelerometer, gyroscope and magnetometer into ready-to-use
orientation, motion and activity outputs. On the DEPZ sensor line the MCU is an
**SHTP pass-through bridge** — the SH-2 host stack runs **on the host**: the
live device `devices::Bno086Device` (requests correlated at the SH-2 layer,
feature read-back, calibration, FRS, flush), over the verifiable layer
([`bno086`](api.md)): SHTP framing and cargo reassembly, the SH-2 input-report
parsers, the control-request builders and the hub's response decoders
(`bno086::hub`).

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
reckoning. It complements the ranging sensors — [SR04](../sr04/introduction.md) /
[VL53L8](../vl53l8cx/introduction.md) tell you *where things are*, the BNO085
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

- [BNO085 user guide](guide.md) — reassemble SHTP, decode reports, build control
  requests, gotchas.
- [API reference](api.md) — `ShtpLayer`, `Report`, `parse_input_cargo`,
  `parse_gyro_rv_cargo`, and the `sh2` builders.
- [Common guide](../guide.md) — framing, CRC, discovery shared by every sensor.
