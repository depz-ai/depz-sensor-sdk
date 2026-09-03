# VL53L8CX — introduction

The **VL53L8CX** is a STMicroelectronics multizone Time-of-Flight ranging
sensor: a tiny laser depth **imager** that returns a 4×4 or 8×8 grid of
distances up to ~4 m. On the DEPZ sensor line the MCU is a thin SPI **register
bridge** — the full ST ULD (Ultra-Lite Driver) runs on the host. This Java SDK
ports the **pure decode half** of that driver:
`ai.depz.sensor.sensors.vl53l8.Vl53l8Uld` (results-frame parsing +
advanced-DCI codecs) and `FrameReassembler` (chunk transport).

This is the base ToF sensor. Its superset — the **VL53L8CH**, which adds Compact
Network Histograms — has its own [introduction](../vl53l8ch/introduction.md) and
[guide](../vl53l8ch/guide.md); the decode surface here applies to it unchanged.

## What it does

- Per-zone distance over a **16-zone (4×4)** or **64-zone (8×8)** grid, at up to
  60 Hz (4×4) / 15 Hz (8×8).
- Each frame carries far more than distance: per-zone target status, number of
  targets, signal and ambient rate, range sigma, reflectance, plus a per-frame
  silicon temperature — all surfaced as raw-integer arrays on
  `Vl53l8Uld.Results`.
- INT-driven **push streaming**: the MCU ships each frame to the host in
  ≤`FrameReassembler.STREAM_CHUNK_MAX` (1528) byte chunks; `FrameReassembler`
  rebuilds the full frame.
- Advanced ST features are exposed as pure DCI codecs: crosstalk margin
  (`xtalkMarginToRaw`), per-zone detection thresholds
  (`packDetectionThresholds`), and the motion indicator (`MotionConfig`,
  `defaultMotionConfig`).

## CX vs CH

Two silicon/firmware variants share the same results-frame wire layout, so
**one decoder serves both** — `Vl53l8Uld.parseFrame` is shared. The only decode
difference is the footer-id offset, selected by `Vl53l8Uld.Variant`:

- **`Variant.CX`** — the base ranging sensor (this page), footer-id offset 12
  (ULD 2.1.0). Also the dev/unprogrammed default (ST dev USB id).
- **`Variant.CH`** — the superset that additionally emits **Compact Network
  Histograms (CNH)** and carries its own production USB PID `0xED40`,
  footer-id offset 4. See the [VL53L8CH docs](../vl53l8ch/introduction.md).

## Scope: decode only

The **live ULD init / register-bridge driver** — firmware download, DCI
read/write, `setResolution`, `startRanging`, power modes, xtalk calibration — is
hardware-dependent and out of scope; it is surfaced as the stub
`Vl53l8Uld.liveDriverStubbed()`. The advanced-DCI codecs here **build/pack** the
payloads that driver would send; wiring them to a live bridge is the extension
point. Frame parsing and the advanced-DCI codecs are complete and vector-tested.

## When to use it

Use the VL53L8 when you need a **depth image** — gesture/zone occupancy, small
obstacle maps, people counting, hand tracking — in a package far smaller and
cheaper than a stereo camera, indoors or in the dark. For a single distance the
SR04 is simpler; for orientation use the BNO086. Reach for **CH/CNH** only when
you need raw return histograms.

## Key concepts

- **Host-side decode** — frame parsing is pure Java (`Vl53l8Uld.parseFrame`), a
  faithful port of the ST driver's `GetRangingData`. The device is just a
  register bridge.
- **Two-step frame path** — `FrameReassembler.unpackFrameChunk` decodes each
  chunk report; `feed` rebuilds the frame; `parseFrame` decodes it. Reset on
  `offset == 0`; a gap discards the frame in progress.
- **`Vl53l8Uld.Results`** — one parsed frame. Arrays are sized to the active
  resolution (16 or 64 zones), row-major; `resolution()` reports which. Fixed-
  point scaling is applied (distance /4, range sigma /128). `targetStatus == 255`
  = no target.
- **Variant footer** — pass `Variant.CX` or `Variant.CH` to `parseFrame`; a
  header/footer id mismatch throws `Vl53l8Uld.Vl53l8Error`.

## See also

- [VL53L8CX user guide](guide.md) — reassemble, parse, the frame fields, the
  advanced-DCI codecs, gotchas.
- [VL53L8CH docs](../vl53l8ch/introduction.md) — the CNH superset.
- [API reference](api.md) — `Vl53l8Uld`, `FrameReassembler`, and the ToF
  constants.
