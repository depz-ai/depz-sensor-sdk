# VL53L8CH — introduction

The **VL53L8CH** is the superset of the [VL53L8CX](../vl53l8cx/introduction.md):
the same ST multizone Time-of-Flight imager on the same SPI register bridge,
driven by the same host-side ULD — **plus Compact Network Histograms (CNH)**.
In this C SDK it is the same sensor class with the model
`DEPZ_VL53L8_MODEL_L8CH`: `init()` downloads the VL53LMZ firmware (ULD 2.0.16)
instead of the VL53L8CX one, and the CNH calls become available.

Everything the CX does, the CH does the same way — start there:

- [VL53L8CX introduction](../vl53l8cx/introduction.md) — what the ToF sensor
  is, the host-side driver, the key concepts.
- [VL53L8CX user guide](../vl53l8cx/guide.md) — open, initialise, configure,
  stream, the frame, thresholds, motion, crosstalk, power modes, record and
  replay, the decode layer.

This page and the [CH guide](guide.md) cover **only what CH adds on top**.

## What CH adds: Compact Network Histograms

A normal frame gives you one distance (and status, signal, ...) per zone. CNH
adds a per-**aggregate** distance **histogram**: for each group of zones, the
photon return counts binned by distance. That exposes the raw return structure
the single-distance pipeline collapses — several returns in one zone, partial
occlusion, glass and edge effects, material signatures.

## What the C SDK covers

- **Open** — `depz_open_device()` attaches the CH model when the board
  enumerates on the VL53L8CH production PID `0xED40`
  (`depz_vl53l8_get_model()` returns `DEPZ_VL53L8_MODEL_L8CH`); on any other
  link, `depz_vl53l8_open_link(link, DEPZ_VL53L8_MODEL_L8CH, &dev)`.
- **The whole VL53L8CX class** — init with the CH firmware blob,
  configuration, the advanced features and ranging, unchanged. Crosstalk
  calibration uses the VL53LMZ calibration table, as the firmware requires.
- **CNH setup** — `depz_vl53l8_cnh_setup` and the ST plugin helpers that fill
  it (`depz_vl53l8_cnh_init_config`, `_create_agg_map`, `_required_memory`),
  then `depz_vl53l8_configure_cnh()` arms it for the next `start_ranging()`.
- **CNH in the stream** — every frame then carries the raw CNH block in
  `cnh[0 .. cnh_len)`; `depz_vl53l8ch_decode_cnh()` turns it into
  per-aggregate integer histograms (a byte-exact port of the ST CNH plugin
  decode), with the decode parameters from `depz_vl53l8_cnh_decode_config()`.
- **Decode layer** — for your own transport: the CH frame decoder
  (`depz_vl53l8ch_decode_frame`, which also copies the CNH block out) and the
  CNH decode, checked against a golden vector.

The class, CNH included, was checked on a real VL53L8CH (firmware
`APP_VL53L8_v0.92`): a 1708-byte CNH block in every frame at 15 Hz, decoding
fine — see [verified on hardware](guide.md#verified-on-hardware).

## When to use CH over CX

Reach for CH only when you need the raw return histograms — multi-return
analysis, material and reflectivity work, glass and edge disambiguation. For
plain depth imaging the [CX](../vl53l8cx/introduction.md) path is identical and
sufficient (and a CH board ranges exactly like a CX without CNH armed).

## Key concepts

- **Aggregate** — a group of zones whose returns are summed into one
  histogram. The aggregate map assigns zones to aggregates, e.g. 8×8 zones in
  2×2 blocks = 16 aggregates.
- **Bins** — each histogram has `feature_length` bins. One CNH bin sums
  `sub_sample` bins of the sensor's own histogram, starting at `start_bin`; a
  sensor bin is 37.5348 mm of distance.
- **Buffer limit** — the sensor holds at most 6160 bytes of CNH
  (`DEPZ_VL53L8_CNH_MAX_BYTES`); aggregates × bins must fit, which
  `depz_vl53l8_cnh_required_memory()` checks.

## See also

- [VL53L8CH user guide](guide.md) — open a CH, configure CNH, decode the
  histograms from the stream, record and replay, the decode layer.
- [VL53L8CX docs](../vl53l8cx/introduction.md) — the base sensor and class CH
  shares.
- [API reference](api.md) — the CH-only symbols: the CNH setup and decode; the
  shared surface is the [CX reference](../vl53l8cx/api.md).
