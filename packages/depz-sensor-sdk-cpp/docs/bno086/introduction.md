# BNO086 — introduction

The **BNO086** is a CEVA/Hillcrest 9-axis IMU with an on-chip sensor-fusion hub
(SH-2). It fuses accelerometer, gyroscope and magnetometer into ready-to-use
orientation, motion and activity outputs. On the DEPZ sensor line the MCU is an
**SHTP pass-through bridge** — the SHTP framing and SH-2 report decode run on the
host, in `depz::bno086` (`depz/bno086.hpp`).

This SDK is the **decode layer**: SHTP frame reassembly (`ShtpLayer`), the SH-2
control-channel command **encoders**, and the input-report **decoders**
(`parse_input_cargo`, `parse_gyro_rv_cargo`). It is pure codecs — no I/O; you own
the port and the request/reply loop (see the [common guide](../guide.md)).

## What it does

- **Orientation** — rotation vector (absolute), game rotation vector (drift-free
  relative), geomagnetic and AR/VR-stabilised variants, all as unit quaternions.
- **Motion** — calibrated / uncalibrated accelerometer, gyroscope, magnetometer,
  linear acceleration, gravity, plus a dense gyro-integrated rotation vector on
  its own SHTP channel.
- **Events & classifiers** — tap, step counter/detector, significant motion,
  stability and personal-activity classifiers, and raw ADC samples.
- **Control encoders** — Set/Get Feature, Product ID, generic Command, and the
  FRS read/write requests, ready to frame and send.

## When to use it

Use the BNO086 whenever you need **orientation or motion**: heading/attitude for
a robot or handheld, gesture and activity detection, image stabilisation, dead
reckoning. It complements the ranging sensors — [SR04](../sr04/introduction.md) /
[VL53L8](../vl53l8cx/introduction.md) tell you *where things are*, the BNO086
tells you *how the device itself is moving and facing*.

## Key concepts

- **SHTP first, SH-2 on top** — inbound bytes are SHTP frames on six channels;
  `ShtpLayer::feed` reassembles them into an `ShtpCargo` (channel + payload).
  Control-channel cargos are SH-2 responses; input-channel cargos (3/4) and the
  gyro-RV channel (5) are sensor reports.
- **Enable, then decode** — you turn a report on by sending a Set Feature command
  (`sh2_build_set_feature`); the hub then pushes that report, which you decode
  with `parse_input_cargo`.
- **Raw integers are authoritative; floats are derived.** Each `Report` keeps its
  wire integers (`x_raw`, `i_raw`, …); scale them with a fixed **Q point**
  (`value = raw / 2^Q`): accel Q8 (m/s²), gyro Q9 (rad/s), mag Q4 (µT),
  quaternions Q14, rotation-vector accuracy Q12 (**radians**,
  `RV_ACCURACY_Q`), gyro-RV angular velocity Q10 (`GYRO_RV_ANGVEL_Q`, rad/s).
- **Correlation is SH-2-level** — per firmware ERRATA E2 the bridge ACKs every
  send immediately and delivers *all* inbound SHTP as unsolicited data, in ≤64-
  byte (`MAX_TX_FRAME`) MCU slots; you reassemble SHTP and correlate at the SH-2
  layer.
- **Timestamps** — input cargos carry a base-timebase reference (`0xFB`,
  `BASE_TIMESTAMP_REF`) and a per-report delay (100 µs ticks);
  `parse_input_cargo` folds both into an absolute `timestamp_us`.

## See also

- [BNO086 user guide](guide.md) — SHTP reassembly, enabling reports, decoding
  report types, the Q-point conversions, gotchas.
- [API reference](api.md) — `ShtpLayer`, `Report`, `parse_input_cargo`,
  `parse_gyro_rv_cargo`, and the SH-2 command encoders.
