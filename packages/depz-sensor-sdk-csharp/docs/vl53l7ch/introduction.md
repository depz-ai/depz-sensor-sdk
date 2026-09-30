# VL53L7CH — introduction

The **VL53L7CH** is the superset of the [VL53L7CX](../vl53l7cx/introduction.md):
the same 90° multizone Time-of-Flight sensor on the same I2C board
(`APP_VL53L7` firmware, contract 11), running ST's **VL53LMZ** firmware (the
VL53L8CH blob) — **plus Compact Network Histograms (CNH)**. Production USB PID
`0xED4A`.

## What CH adds

- **CNH histograms** — per-aggregate photon-return histograms next to the
  normal per-zone distances: multiple returns in one zone, partial occlusion,
  glass and edge effects.
- **CNH streams** — unlike the VL53L8CH board, a CNH frame (3156 bytes for 16
  aggregates × 20 bins, up to ~7.6 KB) comes over the normal stream in
  chunks; no poll mode is needed.
- **The fuller VL53LMZ feature set** — deep sleep and the detection-threshold
  auto-stop, which the L5CX / L7CX firmware lacks.

## When to use it

Only when you need the raw return histograms (multi-return analysis,
material or reflectivity work). For plain depth images the VL53L7CX is the
same sensor with a lighter firmware.

## What this SDK offers

Everything the [VL53L7CX](../vl53l7cx/introduction.md) page lists — the
`Depz.Sensor.Vl53l7` board codecs, transfer ceilings, class resolution
(`Vl53l7Model.Vl53l7ch`, `Vl53l7Discovery.HasCnh` is true) and frame decode
with `Vl53l7Frames.CreateDecoder()` — plus the **CNH histogram decode**: the
decoder hands back the raw CNH block as `Vl53l8Frame.CnhRaw`, and
[`Vl53l8Cnh.DecodeHistogram`](api.md#vl53l8cnh) unpacks it into
per-aggregate histograms.
The CNH decode is the one the VL53L8CH uses (same blob, same layout).

Initialising the sensor, arming a CNH configuration and running a ranging
session is done today with the Python or TypeScript SDK; this SDK decodes
what the board streams.

## Key concepts

- **Decode needs the armed configuration** — `DecodeHistogram` takes the aggregate
  count and bins per aggregate the CNH configuration was armed with; the block
  itself does not carry them.
- **One frame, two results** — the depth image and the CNH block arrive in the
  same frame and decode from the same `Vl53l8Frame`.

## See also

- [VL53L7CH user guide](guide.md) — CNH frames and their decode.
- [VL53L7CX guide](../vl53l7cx/guide.md) — board commands, transfer ceilings,
  frame decode.
- [API reference](api.md)
