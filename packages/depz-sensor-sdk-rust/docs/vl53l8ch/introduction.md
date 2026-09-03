# VL53L8CH — introduction

The **VL53L8CH** is the superset of the [VL53L8CX](../vl53l8cx/introduction.md):
the same STMicroelectronics multizone Time-of-Flight imager, same register
protocol, same host-side ULD frame layout — **plus Compact Network Histograms
(CNH)**. In this crate there is deliberately **one decoder for both variants**:
the CH shares the CX frame decoder, reassembler and advanced-DCI codecs, and
carries its own production USB PID (`0xED40`).

Everything the CX does, the CH does identically — start there:

- [VL53L8CX introduction](../vl53l8cx/introduction.md) — what the ToF sensor is,
  the frame, host-side decode, the key concepts.
- [VL53L8CX user guide](../vl53l8cx/guide.md) — reassemble/decode, the results
  fields and scaling, the grid, and the advanced DCI codecs.

This page and the [CH guide](guide.md) cover **only what CH adds on top**.

## What CH adds: Compact Network Histograms

A normal frame gives you one distance (and status/signal/…) per zone. CNH adds
a per-**aggregate** distance **histogram**: for each aggregate of zones, the
photon return counts binned by range. That exposes the raw return structure the
single-distance pipeline collapses — multiple returns in one zone, partial
occlusion, glass/edge effects, material signatures.

In this crate CNH support is **surfaced, not yet decoded** — truthfully an
extension point:

- **`Vl53l8Results::cnh_raw`** — when a CH frame carries a CNH output block
  (block id [`CNH_DATA_IDX`](api.md) = `0xC048`), `parse_frame` captures its raw
  bytes into `cnh_raw: Option<Vec<u8>>` rather than fabricate a per-zone
  histogram it cannot verify. It is `None` on CX frames and on CH frames without
  a CNH block.
- **CNH histogram decode** (raw block → per-aggregate, per-bin histograms) and
  the **live ULD init/config** that arms CNH over the wire are documented,
  not-yet-implemented extension points. See
  [What it is not](../guide.md#what-it-is-not).

## Same decoder, both variants

The DEPZ decode selects only the frame-tail footer-id geometry by
[`Variant`](../vl53l8cx/api.md), and the DEPZ firmware streams ULD-2.1.0-footer
frames on **both** CX and CH silicon — so a CH capture decodes with
`Variant::Cx` geometry through the exact CX path. The only CH-specific symbol in
the public surface is `CNH_DATA_IDX`; everything else lives in the shared
[VL53L8CX API reference](../vl53l8cx/api.md).

## When to use CH over CX

Reach for CH only when you need the raw return histograms — multi-return
analysis, material/reflectivity work, glass and edge disambiguation. For plain
depth imaging the [CX](../vl53l8cx/introduction.md) is identical and simpler.

## See also

- [VL53L8CH user guide](guide.md) — surfacing the CNH block, the extension point.
- [VL53L8CX docs](../vl53l8cx/introduction.md) — the base sensor CH shares.
- [API reference](api.md) — `CNH_DATA_IDX` (the CH surface is otherwise the
  [CX reference](../vl53l8cx/api.md)).
