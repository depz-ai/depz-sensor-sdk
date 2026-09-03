# VL53L8CX — introduction

The **VL53L8CX** is a STMicroelectronics multizone Time-of-Flight ranging
sensor: a tiny laser depth **imager** that returns a 4×4 or 8×8 grid of
distances up to ~4 m. On the DEPZ sensor line the MCU is a thin SPI **register
bridge** — the full ST ULD (Ultra-Lite Driver) runs on the host. The C# SDK
ports the **verifiable decode path** of that driver: results-frame decode and
the advanced DCI payload codecs (`Depz.Sensor.Vl53l8`).

This is the base ToF sensor. Its superset — the **VL53L8CH**, which adds Compact
Network Histograms — has its own [introduction](../vl53l8ch/introduction.md) and
[guide](../vl53l8ch/guide.md); everything here applies to it unchanged, because
both variants stream the **same ULD results-frame layout**.

## What it does

- Per-zone distance over a **16-zone (4×4)** or **64-zone (8×8)** grid, at up to
  60 Hz (4×4) / 15 Hz (8×8).
- Each frame carries far more than distance: per-zone target status, number of
  targets, signal and ambient rate, range sigma, reflectance, plus a per-frame
  silicon temperature (`Vl53l8Frame`).
- The device ships each frame to the host in ≤1528-byte chunks
  (`Vl53l8Wire.StreamChunkMax`); `FrameReassembler` rebuilds the full frame,
  which `Vl53l8FrameDecoder` then parses.
- Advanced ST features are exposed as pure DCI **payload codecs**: the
  motion-indicator config (`MotionConfig`), the detection-threshold block and
  xtalk-margin scaling (`Vl53l8Advanced`).

## CX vs CH

Two silicon/firmware variants share the same register protocol and the **same
results-frame layout** (`Vl53l8Variant`):

- **`Vl53l8Variant.Cx`** — the base ranging sensor (this page). The development
  default; enumerates under the raw ST VID/PID until reprogrammed.
- **`Vl53l8Variant.Ch`** — a superset that additionally emits **Compact Network
  Histograms (CNH)**, and carries its own production USB PID (`0xED40`). See the
  [VL53L8CH docs](../vl53l8ch/introduction.md).

A single decode path serves both. The only variant-visible difference in the
decode layer is the frame-id footer offset (CX FW / ULD 2.1.0 = 12 bytes from
the end; CH FW / VL53LMZ 2.0.16 = 4), selected with
`Vl53l8FrameDecoder.ForVariant(...)`.

## When to use it

Use the VL53L8 when you need a **depth image** — gesture/zone occupancy, small
obstacle maps, people counting, hand tracking — in a package far smaller and
cheaper than a stereo camera, indoors or in the dark. For a single distance the
SR04 is simpler; for orientation use the BNO086. Reach for **CH/CNH** only when
you need raw return histograms.

## Key concepts

- **Host-side ULD, decode only** — this SDK is the verifiable half: frame decode
  and DCI payload codecs. The live register-bridge init/config that produces the
  frames is hardware-dependent and out of scope (`Vl53l8Uld`, a throwing stub).
- **Chunk reassembly** — `FrameReassembler` resets on offset 0, requires
  contiguous chunks (a gap discards the frame in progress), and completes when
  the accumulated bytes equal `FullSize`. It exposes `Completed` / `Discarded`
  counters.
- **`Vl53l8Frame`** — one parsed frame. Per-zone arrays are sized to the active
  resolution (16 or 64), row-major; `TargetStatus` 5/9 = valid, 255 = no target.
  Raw wire integers are preserved with the ST fixed-point scaling applied
  (`DistanceMm` ÷4, `RangeSigmaMm` ÷128).
- **Corrupt-frame guard** — `Vl53l8FrameDecoder.ParseFrame` throws
  `CorruptedFrameException` when the header/footer frame-id words disagree.

## See also

- [VL53L8CX user guide](guide.md) — reassembly, decoding a frame, the frame
  fields, the advanced DCI codecs, gotchas.
- [VL53L8CH docs](../vl53l8ch/introduction.md) — the CNH superset.
- [API reference](api.md) — `Vl53l8FrameDecoder`, `Vl53l8Frame`, `FrameReassembler`,
  `Vl53l8Advanced`, `MotionConfig`, and the ToF enums.
