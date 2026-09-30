# VL53L7CH — introduction

The **VL53L7CH** is the superset of the [VL53L7CX](../vl53l7cx/introduction.md):
the same 90° multizone Time-of-Flight sensor on the same I2C board
(`APP_VL53L7` firmware, USB PID `0xED4A`), running ST's **VL53LMZ** sensor
firmware (the same blob as the [VL53L8CH](../vl53l8ch/introduction.md)) —
**plus Compact Network Histograms (CNH)**. In this C SDK it is the live
multizone class (`depz_vl53l8_*`) with the model `DEPZ_VL53L8_MODEL_L7CH`.

## What CH adds

- **CNH histograms** — per-aggregate photon-return histograms next to the
  normal per-zone distances: multiple returns in one zone, partial
  occlusion, glass and edge effects. Set up and decoded exactly as on the
  [VL53L8CH](../vl53l8ch/guide.md).
- **The fuller VL53LMZ feature set** — deep sleep and the detection-threshold
  auto-stop, which the L5CX / L7CX firmware lacks.

## What the SDK covers

- **Open** — `depz_open_device()` picks `DEPZ_VL53L8_MODEL_L7CH` for the PID
  `0xED4A` (or a device name with `VL53L7CH` in it); by hand,
  `depz_vl53l8_open_link(link, DEPZ_VL53L8_MODEL_L7CH, &dev)`.
- **Everything the VL53L7CX has** — `init()` (here the VL53LMZ firmware with
  the VL53L7 default configuration, ~1.3 s), configuration, ranging, the
  advanced features, the board commands — see the
  [VL53L7CX guide](../vl53l7cx/guide.md).
- **CNH** — `depz_vl53l8_cnh_setup` and its helpers, then
  `depz_vl53l8_configure_cnh()`; every frame then carries the CNH block in
  `cnh[0 .. cnh_len)`, and `depz_vl53l8ch_decode_cnh()` turns it into
  per-aggregate histograms.
- **Decode layer** — for your own transport, `depz_vl53l7_decode_frame()`
  copies the CNH block out of a frame, and `depz_vl53l8ch_decode_cnh()`
  decodes it.

The class, CNH included, was checked on a real VL53L7CH (`APP_VL53L7_v0.53`):
`init()` in 1.29 s, 8×8 at 15 Hz, a 1708-byte CNH block in every frame — see
[verified on hardware](../vl53l7cx/guide.md#verified-on-hardware).

## When to use it

Only when you need the raw return histograms (multi-return analysis,
material or reflectivity work). For plain depth images the VL53L7CX is the
same sensor with a lighter sensor firmware.

## See also

- [VL53L7CH user guide](guide.md) — open, arm CNH, decode it from the stream,
  record and replay, the decode layer.
- [VL53L8CH guide](../vl53l8ch/guide.md) — the CNH setup in detail (the same
  calls).
- [VL53L7CX guide](../vl53l7cx/guide.md) — everything L5/L7 shares.
- [API reference](api.md) — the L5/L7 section plus the CNH decode.
