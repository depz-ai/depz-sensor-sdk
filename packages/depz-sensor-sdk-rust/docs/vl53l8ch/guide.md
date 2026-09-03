# VL53L8CH — user guide

Hands-on guide to the CNH addition on top of the ToF decode layer. For what the
sensor is and why CH exists, read the [introduction](introduction.md); for exact
signatures see the [API reference](api.md).

**CH shares the entire VL53L8CX decode surface.** Chunk reassembly, frame
decode, the results fields and scaling, the grid, the `Variant` selector, and
every advanced DCI codec work exactly as in the
[VL53L8CX user guide](../vl53l8cx/guide.md) — read that first. This page covers
**only the CNH addition** (which is, today, an extension point).

## Contents

- [Decode CH frames the CX way](#decode-ch-frames-the-cx-way)
- [Surfacing the CNH block](#surfacing-the-cnh-block)
- [The CNH extension point](#the-cnh-extension-point)
- [Gotchas](#gotchas)

## Decode CH frames the CX way

There is no separate CH decoder. Reassemble and decode a CH frame with the same
[`FrameReassembler`](../vl53l8cx/api.md) and [`parse_frame`](../vl53l8cx/api.md)
as the CX — and, because the DEPZ firmware streams ULD-2.1.0-footer frames on
both silicons, with `Variant::Cx` geometry:

```rust
use depz_sensor_sdk::vl53l8::{parse_frame, Variant};

let res = parse_frame(&completed_frame, Variant::Cx)?;   // CH capture, CX footer geometry
// res.distance_mm / target_status / … exactly as on the VL53L8CX
```

Everything in the [CX results table](../vl53l8cx/guide.md#the-results-fields-and-scaling)
applies unchanged.

## Surfacing the CNH block

When a CH frame carries a Compact-Network-Histogram output block (block id
[`CNH_DATA_IDX`](api.md) = `0xC048`), `parse_frame` captures its raw bytes into
the one CH-specific field, [`Vl53l8Results::cnh_raw`](../vl53l8cx/api.md):

```rust
let res = parse_frame(&completed_frame, Variant::Cx)?;
match &res.cnh_raw {
    Some(block) => println!("CNH block present: {} bytes (raw)", block.len()),
    None => {}   // CX frame, or a CH frame without a CNH block
}
```

So the normal depth image and the raw histogram block travel together in one
`Vl53l8Results` — the per-zone arrays are fully decoded; the CNH block is
handed back verbatim.

## The CNH extension point

Decoding `cnh_raw` into per-aggregate, per-bin distance histograms is a
**CH-only feature that is not yet implemented** in this crate. The SDK
surfaces the raw block rather than fabricate a decode it cannot verify against a
golden vector — mirroring the live-ULD register bridge, which is likewise left
as a documented stub (see [What it is not](../guide.md#what-it-is-not)).

Practically, that means: this crate gets you the raw CNH bytes and the full
per-zone depth frame; turning the bytes into histograms (and arming CNH over the
wire in the first place) is host/firmware work above this decode layer.

## Gotchas

- **No CH-specific decoder to call** — reassemble and `parse_frame` exactly as
  for the [CX](../vl53l8cx/guide.md); the CH path is the CX path.
- **Use `Variant::Cx` for DEPZ CH captures** — the firmware embeds the ULD 2.1.0
  footer on both silicons; `Variant::Ch` is for genuine ULD-2.0.16 footer
  frames.
- **`cnh_raw` is raw, not decoded** — it is `Option<Vec<u8>>`; per-zone
  histograms are an extension point. It is `None` on CX frames and on CH frames
  without a CNH block.
- **Everything else is inherited** — the CX gotchas (reassemble before decode,
  raw fixed-point scaling, resolution from the frame, no live init/config) apply
  here too; see the [CX guide](../vl53l8cx/guide.md#gotchas).
