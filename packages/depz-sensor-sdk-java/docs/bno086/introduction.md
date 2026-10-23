# BNO086 — introduction

The **BNO086** is a CEVA/Hillcrest 9-axis IMU with an on-chip sensor-fusion hub
(SH-2). It fuses accelerometer, gyroscope and magnetometer into ready-to-use
orientation, motion and activity outputs. On the DEPZ sensor line the MCU is an
**SHTP pass-through bridge** — the full SH-2 host stack runs on the host. The
Java SDK runs it: the live device `ai.depz.sensor.devices.Bno086Device`
(requests correlated at the SH-2 layer, feature read-back, calibration, FRS,
flush), over SHTP framing/reassembly (`Shtp`), the SH-2 control-channel encoders
and response decoders (`Sh2`, `Sh2Hub`), and the input-report decoders
(`Reports`), all under `ai.depz.sensor.sensors.bno086`.

## What it does

- **Orientation** — rotation vector (absolute), game rotation vector (drift-free
  relative), geomagnetic and AR/VR-stabilised variants, all as unit quaternions.
- **Motion** — calibrated / uncalibrated accelerometer, gyroscope, magnetometer,
  linear acceleration, gravity, plus a dense gyro-integrated rotation vector on
  its own channel.
- **Events & classifiers** — tap, step counter/detector, significant motion,
  stability and personal-activity classifiers, shake, and raw ADC samples.
- **Control encoders** — Set/Get Feature, Product ID, generic Command, and the
  FRS read/write requests (`Sh2.build*`), ready to wrap in SHTP frames.

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

Use the BNO086 whenever you need **orientation or motion**: heading/attitude for
a robot or handheld, gesture and activity detection, image stabilisation, dead
reckoning. It complements the ranging sensors — SR04/VL53L8 tell you *where
things are*, the BNO086 tells you *how the device itself is moving and facing*.

## Key concepts

- **Two layers.** `Shtp` reassembles the transport (length/channel/seq header,
  fragmentation) into whole **cargos**; `Sh2` and `Reports` interpret the SH-2
  payloads inside those cargos. You enable a report by sending a `Sh2`-built
  Set Feature cargo; the hub then streams input reports you decode with
  `Reports.parseInputCargo`.
- **Raw integers are authoritative; scaling is derived downstream.** Each decoded
  report is a `Reports.Report` (`type` tag + a `fields` map) whose values are the
  wire integers (`x_raw`, `i_raw`, …). `Reports.scaled` applies the Q points
  (accel Q8, gyro Q9, mag Q4, quaternions Q14, …; `Reports.Q_POINTS`) and
  returns the physical values under the reference SDK's names (`x`, `i`,
  `accuracy_rad`, `bias`, …).
- **Timestamps fold two references.** Channel-3/4 cargos start with a Base
  Timestamp Reference (`0xFB`) and optional rebases (`0xFA`), and each report
  adds its own 14-bit delay; `parseInputCargo` folds all of them into an absolute
  MCU-clock `timestamp_us` (see the `Reports` Javadoc).
- **Correlation is SH-2-level** — per firmware ERRATA E2 the bridge ACKs every
  send immediately and delivers all inbound SHTP as unsolicited data, and host
  TX frames must fit one MCU slot (`Shtp.MAX_TX_FRAME` = 64). `Shtp.ShtpLayer`
  reassembles and tracks per-channel sequence for you.

## See also

- [BNO086 user guide](guide.md) — SHTP reassembly, enabling reports, decoding the
  report catalog, gyro-RV, gotchas.
- [API reference](api.md) — `Shtp`, `Sh2`, `Reports`, and the report-id
  constants.
