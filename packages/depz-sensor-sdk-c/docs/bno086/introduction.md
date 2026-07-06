# BNO086 — introduction

The **BNO086** is a CEVA/Hillcrest 9-axis IMU with an on-chip sensor-fusion hub
(SH-2). It fuses accelerometer, gyroscope and magnetometer into ready-to-use
orientation, motion and activity outputs. On the DEPZ sensor line the MCU is an
**SHTP pass-through bridge** — the full SH-2 host stack runs on the host, and
this C SDK provides the three layers you need to speak it: SHTP framing, SH-2
control-message encoders, and input-report parsers.

## What the SDK covers

- **SHTP framing** (contract 05 §3) — `depz_shtp_layer` reassembles inbound
  cargos across the 6 channels and builds outbound single-fragment frames
  (`depz_shtp_next_frame`), tracking per-channel sequence counters. Header
  pack/unpack is `depz_shtp_pack_header` / `depz_shtp_unpack_header`.
- **SH-2 control encoders** (contract 05 §6) — build the request payloads:
  Set/Get Feature, Product ID, Command, and the FRS read/write messages
  (`depz_bno_pack_set_feature`, `depz_bno_pack_get_feature_request`, …).
- **Input-report parsers** (contract 05 §5) — `depz_bno_parse_input_cargo()`
  turns a channel-3/4 cargo into typed `depz_bno_report`s (accel, gyro, mag,
  rotation vector, detectors, classifiers, raw), handling the 0xFB base-timestamp
  and 0xFA rebase; `depz_bno_parse_gyro_rv()` parses the channel-5
  gyro-integrated rotation vector.

## When to use it

Use the BNO086 whenever you need **orientation or motion**: heading/attitude for
a robot or handheld, gesture and activity detection, image stabilisation, dead
reckoning. It complements the ranging sensors — SR04/VL53L8 tell you *where
things are*, the BNO086 tells you *how the device itself is moving and facing*.

## Key concepts

- **Enable, then parse** — you turn a report on by encoding a Set Feature
  (`depz_bno_pack_set_feature`) wrapped in an SHTP control frame; the hub then
  pushes input reports you reassemble and parse. There is no request/reply at the
  bridge level.
- **Raw integers are authoritative.** Every `depz_bno_report` field is the wire
  integer (`x_raw`, `i_raw`, …); apply the SH-2 Q-point yourself (`value = raw /
  2**Q`): accel Q8 (m/s²), gyro Q9 (rad/s), mag Q4 (µT), quaternions Q14,
  rotation-vector accuracy Q12 (radians), gyro-RV angular velocity Q10 (rad/s).
- **Timestamps fold in** — channel-3 cargos carry a base-timebase reference
  (0xFB) and per-report delay in 100 µs ticks; the parser folds both, plus the
  bridge capture time, into an absolute `timestamp_us`.
- **Correlation is SH-2-level** — per firmware ERRATA E2 the bridge ACKs every
  send immediately and delivers all inbound SHTP as unsolicited data; you
  reassemble SHTP (`depz_shtp_feed`) and correlate responses at the SH-2 layer.
- **One TX slot is 64 bytes** (`DEPZ_SHTP_MAX_TX_FRAME`, ERRATA E2) — the SDK
  only builds single-fragment TX frames; oversize payloads are rejected.

## See also

- [BNO086 user guide](guide.md) — SHTP reassembly, enabling a report, parsing
  reports, gotchas.
- [API reference](api.md) — `depz_shtp_*`, `depz_bno_pack_*`, `depz_bno_report`,
  and the report parsers.
