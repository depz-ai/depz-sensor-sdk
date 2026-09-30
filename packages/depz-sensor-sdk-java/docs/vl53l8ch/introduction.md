# VL53L8CH — introduction

The **VL53L8CH** is the superset of the [VL53L8CX](../vl53l8cx/introduction.md):
the same STMicroelectronics multizone Time-of-Flight imager, same register
protocol, same host-side ULD — **plus Compact Network Histograms (CNH)**. In the
Java SDK there is no separate CH class: the results-frame blocks are the same,
so the shared `Vl53l8Uld` decoder serves both parts. The only frame-decode
difference is where the frame-id footer sits: the VL53L8CH firmware (VL53LMZ
2.0.16) keeps it at size−4, the VL53L8CX firmware (ULD 2.1.0) at size−12.
`Vl53l8Uld.Variant.CH` selects the CH offset.
CH also carries its own production USB PID (`0xED40`, `UsbIds.PID_VL53L8`).

Everything the CX decoder does, the CH does identically — start there:

- [VL53L8CX introduction](../vl53l8cx/introduction.md) — what the ToF sensor is,
  the frame path, host-side decode, the key concepts.
- [VL53L8CX user guide](../vl53l8cx/guide.md) — reassemble/parse a frame, the
  `Results` fields, and the advanced-DCI codecs (xtalk, thresholds, motion).

This page and the [CH guide](guide.md) cover **only what CH adds on top**.

## What CH adds: Compact Network Histograms

A normal frame gives you one distance (and status/signal/…) per zone. CNH adds a
per-**aggregate** distance **histogram**: for each aggregate of zones, the photon
return counts binned by range. That exposes the raw return structure the
single-distance pipeline collapses — multiple returns in one zone, partial
occlusion, glass/edge effects, material signatures.

In this SDK CNH is decoded end to end, verified against the live-VL53L8CH
golden vector `contracts/vectors/vl53l8_cnh.json`:

- `Vl53l8Uld.Results.cnhRaw` — the raw CNH block bytes when a CH frame carries
  them (`null` on CX and on CH frames without CNH). The shared `parseFrame`
  copies them out verbatim.
- `Vl53l8Uld.decodeCnh(nbOfAggregates, featureLength, raw)` — unpacks that raw
  block into a `CnhResult`: one `CnhAggregate` histogram per aggregate plus the
  reference-residual word. It needs the aggregate count and bins per aggregate
  the sensor runs; the block itself does not record them.

Arming a CNH configuration on the board is not done from this SDK — the
Python or TypeScript SDK drives the board.

## When to use CH over CX

Reach for CH only when you need the raw return histograms — multi-return
analysis, material/reflectivity work, glass and edge disambiguation. For plain
depth imaging the [CX](../vl53l8cx/introduction.md) decode is identical: pass
`Variant.CX` instead of `Variant.CH` and the same `Results` come out.

## See also

- [VL53L8CH user guide](guide.md) — the `Variant.CH` footer, `cnhRaw`, and the
  CNH decode.
- [VL53L8CX docs](../vl53l8cx/introduction.md) — the base decode surface CH
  reuses.
- [API reference](api.md) — `Vl53l8Uld.Variant`, `Vl53l8Uld.decodeCnh`,
  `Vl53l8Uld.CnhResult`, `Vl53l8Uld.CnhAggregate` (the CH-specific surface; the
  shared decode lives in the [CX API](../vl53l8cx/api.md)).
