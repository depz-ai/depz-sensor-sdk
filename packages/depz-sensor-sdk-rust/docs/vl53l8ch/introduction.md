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

In this crate CNH is decoded end to end, verified against the live-VL53L8CH
golden vector `contracts/vectors/vl53l8_cnh.json`:

- **`Vl53l8Results::cnh_raw`** — when a CH frame carries a CNH output block
  (block id [`CNH_DATA_IDX`](api.md#cnh_data_idx) = `0xC048`), `parse_frame`
  copies its raw bytes into `cnh_raw: Option<Vec<u8>>`. It is `None` on CX
  frames and on CH frames without a CNH block.
- **[`decode_cnh`](api.md#decode_cnh)** — unpacks that raw block into
  per-aggregate histograms ([`CnhData`](api.md#cnhdata)), given the aggregate
  count and bins per aggregate ([`CnhDecodeConfig`](api.md#cnhdecodeconfig))
  the sensor runs. The block itself does not record them.

Arming a CNH configuration on the board is not done from this crate (see
[What it is not](../guide.md#what-it-is-not)); the Python or TypeScript SDK
drives the board.

## Frame decode: the CH footer sits at size−4

The results-frame blocks are the same as on the CX, and one `parse_frame`
serves both. The one difference is where the frame-id footer sits: the
VL53L8CH firmware (VL53LMZ 2.0.16) puts it 4 bytes from the frame end, the
VL53L8CX firmware (ULD 2.1.0) 12 bytes. [`Variant`](../vl53l8cx/api.md)
selects it — decode CH frames with `Variant::Ch`; `Variant::Cx` fails the
header/footer check on them.

## When to use CH over CX

Reach for CH only when you need the raw return histograms — multi-return
analysis, material/reflectivity work, glass and edge disambiguation. For plain
depth imaging the [CX](../vl53l8cx/introduction.md) is identical and simpler.

## See also

- [VL53L8CH user guide](guide.md) — decoding CH frames and their CNH histograms.
- [VL53L8CX docs](../vl53l8cx/introduction.md) — the base sensor CH shares.
- [API reference](api.md) — `decode_cnh`, `CnhDecodeConfig`, `CnhData`,
  `CnhAggregate`, `CnhError`, `CNH_DATA_IDX` (the shared frame decode is in the
  [CX reference](../vl53l8cx/api.md)).
