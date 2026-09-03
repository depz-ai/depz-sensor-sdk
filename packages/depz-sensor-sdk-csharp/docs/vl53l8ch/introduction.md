# VL53L8CH — introduction

The **VL53L8CH** is the superset of the [VL53L8CX](../vl53l8cx/introduction.md):
the same STMicroelectronics multizone Time-of-Flight imager, same register
protocol, same host-side ULD, and — crucially for this SDK — the **same
results-frame layout** — **plus Compact Network Histograms (CNH)**. It also
carries its own production USB PID (`0xED40`, hinted `vl53l8ch`).

Because CX and CH stream the identical results frame, the C# decode surface is
**shared**: `Vl53l8FrameDecoder`, `FrameReassembler`, `Vl53l8Advanced` and
`MotionConfig` all serve both variants. Start with the CX docs — everything
there applies to CH:

- [VL53L8CX introduction](../vl53l8cx/introduction.md) — what the ToF sensor is,
  the frame, the decode pipeline, the key concepts.
- [VL53L8CX user guide](../vl53l8cx/guide.md) — reassembly, decoding a frame, the
  frame fields, and the advanced DCI codecs.

This page and the [CH guide](guide.md) cover **only what CH adds on top**: the
CNH histogram block, and the CH frame-id footer offset.

## What CH adds: Compact Network Histograms

A normal frame gives you one distance (and status/signal/…) per zone. CNH adds a
per-**aggregate** distance **histogram**: for each aggregate of zones, the photon
return counts binned by range. That exposes the raw return structure the
single-distance pipeline collapses — multiple returns in one zone, partial
occlusion, glass/edge effects, material signatures.

- **The shared decode already serves CH.** Select the CH footer offset with
  `Vl53l8FrameDecoder.ForVariant(Vl53l8Variant.Ch)` (frame-id echo 4 bytes from
  the end, vs 12 for CX — `Vl53l8FrameDecoder.FooterIdOffsetCh`).
- **`Vl53l8Cnh`** — the CH-only histogram-decode extension point.
  `Vl53l8Cnh.Variant` is always `Vl53l8Variant.Ch`.

## Extension point: CNH decode is not yet implemented

`Vl53l8Cnh.DecodeHistogram` is **deliberately stubbed, not faked** — it throws
`NotSupportedException` rather than return fabricated data. There is no golden
CNH vector to verify a port against yet, so a fabricated decoder would be
untrustworthy. The shared CX/CH results-frame decode and the advanced DCI codecs
are fully implemented and golden-vector-backed; CNH is the one CH-specific piece
still pending. Wire the real histogram parse into `Vl53l8Cnh` once a capture plus
a reference decode exist.

## When to use CH over CX

Reach for CH only when you need the raw return histograms — multi-return
analysis, material/reflectivity work, glass and edge disambiguation. For plain
depth imaging the [CX](../vl53l8cx/introduction.md) decode is identical and
already complete.

## See also

- [VL53L8CH user guide](guide.md) — selecting the CH decoder, the CNH extension
  point, gotchas.
- [VL53L8CX docs](../vl53l8cx/introduction.md) — the base sensor CH shares.
- [API reference](api.md) — `Vl53l8Cnh` (the shared CX surface is documented in
  the [VL53L8CX API reference](../vl53l8cx/api.md)).
